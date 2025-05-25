#include "stdafx.h"
#include "Core/RSXEngLock.hpp"
#include "Core/RSXReservationLock.hpp"
#include "RSXThread.h"

namespace rsx
{
	namespace reports
	{
		ZCULL_control::ZCULL_control()
		{
			for (auto& query : m_occlusion_query_data)
			{
				m_free_occlusion_pool.push(&query);
			}

			for (auto& stat : m_statistics_map)
			{
				stat.flags = stat.result = 0;
			}
		}

		ZCULL_control::~ZCULL_control()
		{
			std::scoped_lock lock(m_pages_mutex);

			for (auto& block : m_locked_pages)
			{
				for (auto& p : block)
				{
					if (p.second.prot != utils::protection::rw)
					{
						utils::memory_protect(vm::base(p.first), utils::get_page_size(), utils::protection::rw);
					}
				}

				block.clear();
			}
		}

		void ZCULL_control::set_active(class ::rsx::thread* ptimer, bool state, bool flush_queue)
		{
			if (state != host_queries_active)
			{
				host_queries_active = state;

				if (state)
				{
					ensure(unit_enabled && m_current_task == nullptr);
					allocate_new_query(ptimer);
					begin_occlusion_query(m_current_task);
				}
				else
				{
					ensure(m_current_task);
					if (m_current_task->num_draws)
					{
						end_occlusion_query(m_current_task);
						m_current_task->active = false;
						m_current_task->pending = true;
						m_current_task->sync_tag = m_timer++;
						m_current_task->timestamp = m_tsc;

						m_pending_writes.push_back({});
						m_pending_writes.back().query = m_current_task;
						ptimer->async_tasks_pending++;
					}
					else
					{
						discard_occlusion_query(m_current_task);
						free_query(m_current_task);
						m_current_task->active = false;
					}

					m_current_task = nullptr;
					update(ptimer, 0u, flush_queue);
				}
			}
		}

		void ZCULL_control::check_state(class ::rsx::thread* ptimer, bool flush_queue)
		{
			// NOTE: Only enable host queries if pixel count is active to save on resources
			// Can optionally be enabled for either stats enabled or zpass enabled for accuracy
			const bool data_stream_available = zpass_count_enabled; // write_enabled && (zpass_count_enabled || stats_enabled);
			if (host_queries_active && !data_stream_available)
			{
				// Stop
				set_active(ptimer, false, flush_queue);
			}
			else if (!host_queries_active && data_stream_available && unit_enabled)
			{
				// Start
				set_active(ptimer, true, flush_queue);
			}
		}

		void ZCULL_control::set_enabled(class ::rsx::thread* ptimer, bool state, bool flush_queue)
		{
			if (state != unit_enabled)
			{
				unit_enabled = state;
				check_state(ptimer, flush_queue);
			}
		}

		void ZCULL_control::set_status(class ::rsx::thread* ptimer, bool surface_active, bool zpass_active, bool zcull_stats_active, bool flush_queue)
		{
			write_enabled = surface_active;
			zpass_count_enabled = zpass_active;
			stats_enabled = zcull_stats_active;

			check_state(ptimer, flush_queue);

			// Disabled since only ZPASS is implemented right now
			if (false) //(m_current_task && m_current_task->active)
			{
				// Data check
				u32 expected_type = 0;
				if (zpass_active) expected_type |= CELL_GCM_ZPASS_PIXEL_CNT;
				if (zcull_stats_active) expected_type |= CELL_GCM_ZCULL_STATS;

				if (m_current_task->data_type != expected_type) [[unlikely]]
				{
					rsx_log.error("ZCULL queue interrupted by data type change!");

					// Stop+start the current setup
					set_active(ptimer, false, false);
					set_active(ptimer, true, false);
				}
			}
		}

		void ZCULL_control::read_report(::rsx::thread* ptimer, vm::addr_t sink, u32 type)
		{
			if (m_current_task && type == CELL_GCM_ZPASS_PIXEL_CNT)
			{
				m_current_task->owned = true;
				end_occlusion_query(m_current_task);
				m_pending_writes.push_back({});

				m_current_task->active = false;
				m_current_task->pending = true;
				m_current_task->timestamp = m_tsc;
				m_current_task->sync_tag = m_timer++;
				m_pending_writes.back().query = m_current_task;

				allocate_new_query(ptimer);
				begin_occlusion_query(m_current_task);
			}
			else
			{
				// Spam; send null query down the pipeline to copy the last result
				// Might be used to capture a timestamp (verify)

				if (m_pending_writes.empty())
				{
					// No need to queue this if there is no pending request in the pipeline anyway
					write(sink, ptimer->timestamp(), type, m_statistics_map[m_statistics_tag_id].result);
					return;
				}

				m_pending_writes.push_back({});
			}

			auto forwarder = &m_pending_writes.back();
			m_statistics_map[m_statistics_tag_id].flags |= 1;

			for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); It++)
			{
				if (!It->sink)
				{
					It->counter_tag = m_statistics_tag_id;
					It->sink = sink;
					It->type = type;

					if (forwarder != &(*It))
					{
						// Not the last one in the chain, forward the writing operation to the last writer
						// Usually comes from truncated queries caused by disabling the testing
						ensure(It->query);

						It->forwarder = forwarder;
						It->query->owned = true;
					}

					continue;
				}

				break;
			}

			on_report_enqueued(sink);

			ptimer->async_tasks_pending++;

			if (m_statistics_map[m_statistics_tag_id].result != 0)
			{
				// Flush guaranteed results; only one positive is needed
				update(ptimer);
			}
		}

		void ZCULL_control::allocate_new_query(::rsx::thread* ptimer)
		{
			int retries = 0;
			while (true)
			{
				if (!m_free_occlusion_pool.empty())
				{
					m_current_task = m_free_occlusion_pool.top();
					m_free_occlusion_pool.pop();

					m_current_task->data_type = 0;
					m_current_task->num_draws = 0;
					m_current_task->result = 0;
					m_current_task->active = true;
					m_current_task->owned = false;
					m_current_task->sync_tag = 0;
					m_current_task->timestamp = 0;

					// Flags determine what kind of payload is carried by queries in the 'report'
					if (zpass_count_enabled) m_current_task->data_type |= CELL_GCM_ZPASS_PIXEL_CNT;
					if (stats_enabled) m_current_task->data_type |= CELL_GCM_ZCULL_STATS;

					return;
				}

				if (retries > 0)
				{
					fmt::throw_exception("Allocation failed!");
				}

				// All slots are occupied, try to pop the earliest entry

				if (!m_pending_writes.front().query)
				{
					// If this happens, the assert above will fire. There should never be a queue header with no work to be done
					rsx_log.error("Close to our death.");
				}

				m_next_tsc = 0;
				update(ptimer, m_pending_writes.front().sink);

				retries++;
			}
		}

		void ZCULL_control::free_query(occlusion_query_info* query)
		{
			query->pending = false;
			m_free_occlusion_pool.push(query);
		}

		void ZCULL_control::clear(class ::rsx::thread* ptimer, u32 type)
		{
			if (!(type & CELL_GCM_ZPASS_PIXEL_CNT))
			{
				// Other types do not generate queries at the moment
				return;
			}

			if (!m_pending_writes.empty())
			{
				//Remove any dangling/unclaimed queries as the information is lost anyway
				auto valid_size = m_pending_writes.size();
				for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
				{
					if (!It->sink)
					{
						discard_occlusion_query(It->query);
						free_query(It->query);
						valid_size--;
						ptimer->async_tasks_pending--;
						continue;
					}

					break;
				}

				m_pending_writes.resize(valid_size);
			}

			if (m_pending_writes.empty())
			{
				// Clear can be invoked from flip as a workaround to prevent query leakage.
				m_statistics_map[m_statistics_tag_id].flags = 0;
			}

			if (m_statistics_map[m_statistics_tag_id].flags)
			{
				// Move to the next slot if this one is still in use.
				m_statistics_tag_id = (m_statistics_tag_id + 1) % max_stat_registers;
			}

			auto& current_stats = m_statistics_map[m_statistics_tag_id];
			if (current_stats.flags != 0)
			{
				// This shouldn't happen
				rsx_log.error("Allocating a new ZCULL statistics slot %u overwrites previous data.", m_statistics_tag_id);
			}

			// Clear value before use
			current_stats.result = 0;
		}

		void ZCULL_control::on_draw()
		{
			if (m_current_task)
			{
				m_current_task->num_draws++;
				m_current_task->sync_tag = m_timer++;
			}
		}

		void ZCULL_control::on_sync_hint(sync_hint_payload_t payload)
		{
			m_sync_tag = std::max(m_sync_tag, payload.query->sync_tag);
		}

		void ZCULL_control::write(vm::addr_t sink, u64 timestamp, u32 type, u32 value)
		{
			ensure(sink);

			auto scale_result = [](u32 value)
			{
				const auto scale = rsx::get_resolution_scale_percent();
				const auto result = (value * 10000ull) / (scale * scale);
				return std::max(1u, static_cast<u32>(result));
			};

			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
				if (value)
				{
					value = (g_cfg.video.precise_zpass_count) ?
						scale_result(value) :
						u16{ umax };
				}
				break;
			case CELL_GCM_ZCULL_STATS3:
				value = (value || !write_enabled || !stats_enabled) ? 0 : u16{ umax };
				break;
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS:
			default:
				// Not implemented
				value = (write_enabled && stats_enabled) ? -1 : 0;
				break;
			}

			rsx::reservation_lock<true> lock(sink, 16);
			auto report = vm::get_super_ptr<atomic_t<CellGcmReportData>>(sink);
			report->store({timestamp, value, 0});
		}

		void ZCULL_control::write(queued_report_write* writer, u64 timestamp, u32 value)
		{
			write(writer->sink, timestamp, writer->type, value);
			on_report_completed(writer->sink);

			for (auto& addr : writer->sink_alias)
			{
				write(addr, timestamp, writer->type, value);
			}
		}

		void ZCULL_control::retire(::rsx::thread* ptimer, queued_report_write* writer, u32 result)
		{
			if (!writer->forwarder)
			{
				// No other queries in the chain, write result
				const auto value = (writer->type == CELL_GCM_ZPASS_PIXEL_CNT) ? m_statistics_map[writer->counter_tag].result : result;
				write(writer, ptimer->timestamp(), value);
			}

			if (writer->query && writer->query->sync_tag == ptimer->cond_render_ctrl.eval_sync_tag)
			{
				bool eval_failed;
				if (!writer->forwarder) [[likely]]
				{
					// Normal evaluation
					eval_failed = (result == 0u);
				}
				else
				{
					// Eval was inserted while ZCULL was active but not enqueued to write to memory yet
					// write(addr) -> enable_zpass_stats -> eval_condition -> write(addr)
					// In this case, use what already exists in memory, not the current counter
					eval_failed = (vm::_ref<CellGcmReportData>(writer->sink).value == 0u);
				}

				ptimer->cond_render_ctrl.set_eval_result(ptimer, eval_failed);
			}
		}

		void ZCULL_control::sync(::rsx::thread* ptimer)
		{
			if (m_pending_writes.empty())
			{
				// Nothing to do
				return;
			}

			if (!m_critical_reports_in_flight)
			{
				// Valid call, but nothing important queued up
				return;
			}

			if (g_cfg.video.relaxed_zcull_sync)
			{
				update(ptimer, 0, true);
				return;
			}

			// Quick reverse scan to push commands ahead of time
			for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
			{
				if (It->sink && It->query && It->query->num_draws)
				{
					if (It->query->sync_tag > m_sync_tag)
					{
						// rsx_log.trace("[Performance warning] Query hint emit during sync command.");
						ptimer->sync_hint(FIFO::interrupt_hint::zcull_sync, { .query = It->query });
					}

					break;
				}
			}

			u32 processed = 0;
			const bool has_unclaimed = (m_pending_writes.back().sink == 0);

			// Write all claimed reports unconditionally
			for (auto& writer : m_pending_writes)
			{
				if (!writer.sink)
					break;

				auto query = writer.query;
				auto& counter = m_statistics_map[writer.counter_tag];

				if (query)
				{
					ensure(query->pending);

					const bool implemented = (writer.type == CELL_GCM_ZPASS_PIXEL_CNT || writer.type == CELL_GCM_ZCULL_STATS3);
					const bool have_result = counter.result && !g_cfg.video.precise_zpass_count;

					if (implemented && !have_result && query->num_draws)
					{
						get_occlusion_query_result(query);
						counter.result += query->result;
					}
					else
					{
						// Already have a hit, no need to retest
						discard_occlusion_query(query);
					}

					free_query(query);
				}

				retire(ptimer, &writer, counter.result);
				processed++;
			}

			if (!has_unclaimed)
			{
				ensure(processed == m_pending_writes.size());
				m_pending_writes.clear();
			}
			else
			{
				auto remaining = m_pending_writes.size() - processed;
				ensure(remaining > 0);

				if (remaining == 1)
				{
					m_pending_writes[0] = std::move(m_pending_writes.back());
					m_pending_writes.resize(1);
				}
				else
				{
					std::move(m_pending_writes.begin() + processed, m_pending_writes.end(), m_pending_writes.begin());
					m_pending_writes.resize(remaining);
				}
			}

			// Delete all statistics caches but leave the current one
			const u32 current_index = m_statistics_tag_id;
			const u32 previous_index = (current_index + max_stat_registers - 1) % max_stat_registers;
			for (u32 index = previous_index; index != current_index;)
			{
				if (m_statistics_map[index].flags == 0)
				{
					break;
				}

				m_statistics_map[index].flags = 0;
				index = (index + max_stat_registers - 1) % max_stat_registers;
			}

			//Decrement jobs counter
			ptimer->async_tasks_pending -= processed;
		}

		void ZCULL_control::update(::rsx::thread* ptimer, u32 sync_address, bool hint)
		{
			if (m_pending_writes.empty())
			{
				return;
			}

			const auto& front = m_pending_writes.front();
			if (!front.sink)
			{
				// No writables in queue, abort
				return;
			}

			if (!sync_address)
			{
				if (hint || ptimer->async_tasks_pending + 0u >= max_safe_queue_depth)
				{
					// Prepare the whole queue for reading. This happens when zcull activity is disabled or queue is too long
					for (auto It = m_pending_writes.rbegin(); It != m_pending_writes.rend(); ++It)
					{
						if (It->query)
						{
							if (It->query->num_draws && It->query->sync_tag > m_sync_tag)
							{
								ptimer->sync_hint(FIFO::interrupt_hint::zcull_sync, { .query = It->query });
								ensure(It->query->sync_tag <= m_sync_tag);
							}

							break;
						}
					}
				}

				if (m_tsc = get_system_time(); m_tsc < m_next_tsc)
				{
					return;
				}
				else
				{
					// Schedule ahead
					m_next_tsc = m_tsc + min_zcull_tick_us;

					// Schedule a queue flush if needed
					if (!g_cfg.video.relaxed_zcull_sync && m_critical_reports_in_flight &&
						front.query && front.query->num_draws && front.query->sync_tag > m_sync_tag)
					{
						const auto elapsed = m_tsc - front.query->timestamp;
						if (elapsed > max_zcull_delay_us)
						{
							ptimer->sync_hint(FIFO::interrupt_hint::zcull_sync, { .query = front.query });
							ensure(front.query->sync_tag <= m_sync_tag);
						}

						return;
					}
				}
			}

			u32 processed = 0;
			for (auto& writer : m_pending_writes)
			{
				if (!writer.sink)
					break;

				auto query = writer.query;
				auto& counter = m_statistics_map[writer.counter_tag];

				const bool force_read = (sync_address != 0);
				if (force_read && writer.sink == sync_address && !writer.forwarder)
				{
					// Forced reads end here
					sync_address = 0;
				}

				if (query)
				{
					ensure(query->pending);

					const bool implemented = (writer.type == CELL_GCM_ZPASS_PIXEL_CNT || writer.type == CELL_GCM_ZCULL_STATS3);
					const bool have_result = counter.result && !g_cfg.video.precise_zpass_count;

					if (!implemented || !query->num_draws || have_result)
					{
						discard_occlusion_query(query);
					}
					else if (force_read || check_occlusion_query_status(query))
					{
						get_occlusion_query_result(query);
						counter.result += query->result;
					}
					else
					{
						// Too early; abort
						ensure(!force_read && implemented);
						break;
					}

					free_query(query);
				}

				// Release the stat tag for this object. Slots are all or nothing.
				m_statistics_map[writer.counter_tag].flags = 0;

				retire(ptimer, &writer, counter.result);
				processed++;
			}

			if (processed)
			{
				auto remaining = m_pending_writes.size() - processed;
				if (remaining == 1)
				{
					m_pending_writes[0] = std::move(m_pending_writes.back());
					m_pending_writes.resize(1);
				}
				else if (remaining)
				{
					std::move(m_pending_writes.begin() + processed, m_pending_writes.end(), m_pending_writes.begin());
					m_pending_writes.resize(remaining);
				}
				else
				{
					m_pending_writes.clear();
				}

				ptimer->async_tasks_pending -= processed;
			}
		}

		flags32_t ZCULL_control::read_barrier(::rsx::thread* ptimer, u32 memory_address, u32 memory_range, flags32_t flags)
		{
			if (m_pending_writes.empty())
				return result_none;

			const auto memory_end = memory_address + memory_range;

			AUDIT(memory_end >= memory_address);

			u32 sync_address = 0;
			occlusion_query_info* query = nullptr;

			for (auto It = m_pending_writes.crbegin(); It != m_pending_writes.crend(); ++It)
			{
				if (sync_address)
				{
					if (It->query)
					{
						sync_address = It->sink;
						query = It->query;
						break;
					}

					continue;
				}

				if (It->sink >= memory_address && It->sink < memory_end)
				{
					sync_address = It->sink;

					// NOTE: If application is spamming requests, there may be no query attached
					if (It->query)
					{
						query = It->query;
						break;
					}
				}
			}

			if (!sync_address || !query)
			{
				return result_none;
			}

			// Disable optimizations across the accessed range if reports reside within
			{
				std::scoped_lock lock(m_pages_mutex);

				const auto location1 = rsx::classify_location(memory_address);
				const auto location2 = rsx::classify_location(memory_end - 1);

				if (!m_pages_accessed[location1])
				{
					disable_optimizations(ptimer, location1);
				}

				if (!m_pages_accessed[location2])
				{
					disable_optimizations(ptimer, location2);
				}
			}

			if (!(flags & sync_defer_copy))
			{
				if (!(flags & sync_no_notify))
				{
					if (query->sync_tag > m_sync_tag) [[unlikely]]
					{
						ptimer->sync_hint(FIFO::interrupt_hint::zcull_sync, { .query = query });
						ensure(m_sync_tag >= query->sync_tag);
					}
				}

				// There can be multiple queries all writing to the same address, loop to flush all of them
				while (query->pending)
				{
					update(ptimer, sync_address);
				}
				return result_none;
			}

			return result_zcull_intr;
		}

		flags32_t ZCULL_control::read_barrier(class ::rsx::thread* ptimer, u32 memory_address, occlusion_query_info* query)
		{
			// Called by cond render control. Internal RSX usage, do not disable optimizations
			while (query->pending)
			{
				update(ptimer, memory_address);
			}

			return result_none;
		}

		query_search_result ZCULL_control::find_query(vm::addr_t sink_address, bool all)
		{
			query_search_result result{};
			u32 stat_id = 0;

			for (auto It = m_pending_writes.crbegin(); It != m_pending_writes.crend(); ++It)
			{
				if (stat_id) [[unlikely]]
				{
					if (It->counter_tag != stat_id)
					{
						if (result.found)
						{
							// Some result was found, return it instead
							break;
						}

						// Zcull stats were cleared between this query and the required stats, result can only be 0
						return { true, 0, {} };
					}

					if (It->query && It->query->num_draws)
					{
						result.found = true;
						result.queries.push_back(It->query);

						if (!all)
						{
							break;
						}
					}
				}
				else if (It->sink == sink_address)
				{
					if (It->query && It->query->num_draws)
					{
						result.found = true;
						result.queries.push_back(It->query);

						if (!all)
						{
							break;
						}
					}

					stat_id = It->counter_tag;
				}
			}

			return result;
		}

		u32 ZCULL_control::copy_reports_to(u32 start, u32 range, u32 dest)
		{
			u32 bytes_to_write = 0;
			const auto memory_range = utils::address_range32::start_length(start, range);
			for (auto& writer : m_pending_writes)
			{
				if (!writer.sink)
					break;

				if (!writer.forwarder && memory_range.overlaps(writer.sink))
				{
					u32 address = (writer.sink - start) + dest;
					writer.sink_alias.push_back(vm::cast(address));
				}
			}

			return bytes_to_write;
		}

		void ZCULL_control::on_report_enqueued(vm::addr_t address)
		{
			const auto location = rsx::classify_location(address);
			std::scoped_lock lock(m_pages_mutex);

			if (!m_pages_accessed[location]) [[ likely ]]
			{
				const auto page_address = utils::page_start(static_cast<u32>(address));
				auto& page = m_locked_pages[location][page_address];
				page.add_ref();

				if (page.prot == utils::protection::rw)
				{
					utils::memory_protect(vm::base(page_address), utils::get_page_size(), utils::protection::no);
					page.prot = utils::protection::no;
				}
			}
			else
			{
				m_critical_reports_in_flight++;
			}
		}

		void ZCULL_control::on_report_completed(vm::addr_t address)
		{
			const auto location = rsx::classify_location(address);
			if (!m_pages_accessed[location])
			{
				const auto page_address = utils::page_start(static_cast<u32>(address));
				std::scoped_lock lock(m_pages_mutex);

				if (auto found = m_locked_pages[location].find(page_address);
					found != m_locked_pages[location].end())
				{
					auto& page = found->second;

					ensure(page.has_refs());
					page.release();
				}
			}

			if (m_pages_accessed[location])
			{
				m_critical_reports_in_flight--;
			}
		}

		void ZCULL_control::disable_optimizations(::rsx::thread*, u32 location)
		{
			// Externally synchronized
			rsx_log.warning("Reports area at location %s was accessed. ZCULL optimizations will be disabled.", location_tostring(location));
			m_pages_accessed[location] = true;

			// Unlock pages
			for (auto& p : m_locked_pages[location])
			{
				const auto this_address = p.first;
				auto& page = p.second;

				if (page.prot != utils::protection::rw)
				{
					utils::memory_protect(vm::base(this_address), utils::get_page_size(), utils::protection::rw);
					page.prot = utils::protection::rw;
				}

				while (page.has_refs())
				{
					m_critical_reports_in_flight++;
					page.release();
				}
			}

			m_locked_pages[location].clear();
		}

		bool ZCULL_control::on_access_violation(u32 address)
		{
			const auto page_address = utils::page_start(address);
			const auto location = rsx::classify_location(address);

			if (m_pages_accessed[location])
			{
				// Already faulted, no locks possible
				return false;
			}

			bool need_disable_optimizations = false;
			{
				reader_lock lock(m_pages_mutex);

				if (auto found = m_locked_pages[location].find(page_address);
					found != m_locked_pages[location].end())
				{
					lock.upgrade();

					auto& fault_page = m_locked_pages[location][page_address];
					if (fault_page.prot != utils::protection::rw)
					{
						if (fault_page.has_refs())
						{
							// R/W to active block
							need_disable_optimizations = true;    // Defer actual operation
							m_pages_accessed[location] = true;
						}
						else
						{
							// R/W to stale block, unload it and move on
							utils::memory_protect(vm::base(page_address), utils::get_page_size(), utils::protection::rw);
							m_locked_pages[location].erase(page_address);

							return true;
						}
					}
				}
			}

			// Deadlock avoidance, do not pause RSX FIFO eng while holding the pages lock
			if (need_disable_optimizations)
			{
				auto thr = rsx::get_current_renderer();
				rsx::eng_lock rlock(thr);
				std::scoped_lock lock(m_pages_mutex);
				disable_optimizations(thr, location);

				thr->m_eng_interrupt_mask |= rsx::pipe_flush_interrupt;
				return true;
			}

			return false;
		}


		// Conditional rendering helpers
		void conditional_render_eval::reset()
		{
			eval_address = 0;
			eval_sync_tag = 0;
			eval_sources.clear();

			eval_failed = false;
		}

		bool conditional_render_eval::disable_rendering() const
		{
			return (enabled && eval_failed);
		}

		bool conditional_render_eval::eval_pending() const
		{
			return (enabled && eval_address);
		}

		void conditional_render_eval::enable_conditional_render(::rsx::thread* pthr, u32 address)
		{
			if (hw_cond_active)
			{
				ensure(enabled);
				pthr->end_conditional_rendering();
			}

			reset();

			enabled = true;
			eval_address = address;
		}

		void conditional_render_eval::disable_conditional_render(::rsx::thread* pthr)
		{
			if (hw_cond_active)
			{
				ensure(enabled);
				pthr->end_conditional_rendering();
			}

			reset();
			enabled = false;
		}

		void conditional_render_eval::set_eval_sources(std::vector<occlusion_query_info*>& sources)
		{
			eval_sources = std::move(sources);
			eval_sync_tag = eval_sources.front()->sync_tag;
		}

		void conditional_render_eval::set_eval_result(::rsx::thread* pthr, bool failed)
		{
			if (hw_cond_active)
			{
				ensure(enabled);
				pthr->end_conditional_rendering();
			}

			reset();
			eval_failed = failed;
		}

		void conditional_render_eval::eval_result(::rsx::thread* pthr)
		{
			vm::ptr<CellGcmReportData> result = vm::cast(eval_address);
			const bool failed = (result->value == 0u);
			set_eval_result(pthr, failed);
		}
	}
}