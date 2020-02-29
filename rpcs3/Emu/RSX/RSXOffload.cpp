#include "stdafx.h"

#include "Common/BufferUtils.h"
#include "RSXOffload.h"
#include "RSXThread.h"
#include "rsx_utils.h"

#include <thread>
#include <atomic>

namespace rsx
{
	struct dma_manager::offload_thread
	{
		lf_queue<transport_packet> m_work_queue;
		atomic_t<u64> m_enqueued_count = 0;
		atomic_t<u64> m_processed_count = 0;
		transport_packet* m_current_job = nullptr;

		std::thread::id m_thread_id;

		void operator ()()
		{
			if (!g_cfg.video.multithreaded_rsx)
			{
				// Abort if disabled
				return;
			}

			m_thread_id = std::this_thread::get_id();

			if (g_cfg.core.thread_scheduler_enabled)
			{
				thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(thread_class::rsx));
			}

			while (thread_ctrl::state() != thread_state::aborting)
			{
				for (auto&& job : m_work_queue.pop_all())
				{
					m_current_job = &job;

					switch (job.type)
					{
					case raw_copy:
					{
						std::memcpy(job.dst, job.src, job.length);
						break;
					}
					case vector_copy:
					{
						std::memcpy(job.dst, job.opt_storage.data(), job.length);
						break;
					}
					case index_emulate:
					{
						write_index_array_for_non_indexed_non_native_primitive_to_buffer(static_cast<char*>(job.dst), static_cast<rsx::primitive_type>(job.aux_param0), job.length);
						break;
					}
					case callback:
					{
						rsx::get_current_renderer()->renderctl(job.aux_param0, job.src);
						break;
					}
					default: ASSUME(0); fmt::throw_exception("Unreachable" HERE);
					}

					m_processed_count.release(m_processed_count + 1);
				}

				m_current_job = nullptr;

				if (m_enqueued_count.load() == m_processed_count.load())
				{
					m_processed_count.notify_all();
					std::this_thread::yield();
				}
			}

			m_processed_count = -1;
			m_processed_count.notify_all();
		}

		static constexpr auto thread_name = "RSX Offloader"sv;
	};


	using dma_thread = named_thread<dma_manager::offload_thread>;

	static_assert(std::is_default_constructible_v<dma_thread>);

	// initialization
	void dma_manager::init()
	{
	}

	// General transport
	void dma_manager::copy(void *dst, std::vector<u8>& src, u32 length)
	{
		if (length <= max_immediate_transfer_size || !g_cfg.video.multithreaded_rsx)
		{
			std::memcpy(dst, src.data(), length);
		}
		else
		{
			g_fxo->get<dma_thread>()->m_enqueued_count++;
			g_fxo->get<dma_thread>()->m_work_queue.push(dst, src, length);
		}
	}

	void dma_manager::copy(void *dst, void *src, u32 length)
	{
		if (length <= max_immediate_transfer_size || !g_cfg.video.multithreaded_rsx)
		{
			std::memcpy(dst, src, length);
		}
		else
		{
			g_fxo->get<dma_thread>()->m_enqueued_count++;
			g_fxo->get<dma_thread>()->m_work_queue.push(dst, src, length);
		}
	}

	// Vertex utilities
	void dma_manager::emulate_as_indexed(void *dst, rsx::primitive_type primitive, u32 count)
	{
		if (!g_cfg.video.multithreaded_rsx)
		{
			write_index_array_for_non_indexed_non_native_primitive_to_buffer(
				reinterpret_cast<char*>(dst), primitive, count);
		}
		else
		{
			g_fxo->get<dma_thread>()->m_enqueued_count++;
			g_fxo->get<dma_thread>()->m_work_queue.push(dst, primitive, count);
		}
	}

	// Backend callback
	void dma_manager::backend_ctrl(u32 request_code, void* args)
	{
		verify(HERE), g_cfg.video.multithreaded_rsx;

		g_fxo->get<dma_thread>()->m_enqueued_count++;
		g_fxo->get<dma_thread>()->m_work_queue.push(request_code, args);
	}

	// Synchronization
	bool dma_manager::is_current_thread() const
	{
		return std::this_thread::get_id() == g_fxo->get<dma_thread>()->m_thread_id;
	}

	bool dma_manager::sync()
	{
		const auto _thr = g_fxo->get<dma_thread>();

		if (_thr->m_enqueued_count.load() <= _thr->m_processed_count.load()) [[likely]]
		{
			// Nothing to do
			return true;
		}

		if (auto rsxthr = get_current_renderer(); rsxthr->is_current_thread())
		{
			if (m_mem_fault_flag)
			{
				// Abort if offloader is in recovery mode
				return false;
			}

			while (_thr->m_enqueued_count.load() > _thr->m_processed_count.load())
			{
				rsxthr->on_semaphore_acquire_wait();
				_mm_pause();
			}
		}
		else
		{
			while (_thr->m_enqueued_count.load() > _thr->m_processed_count.load())
				_mm_pause();
		}

		return true;
	}

	void dma_manager::join()
	{
		*g_fxo->get<dma_thread>() = thread_state::aborting;
		sync();
	}

	void dma_manager::set_mem_fault_flag()
	{
		verify("Access denied" HERE), is_current_thread();
		m_mem_fault_flag.release(true);
	}

	void dma_manager::clear_mem_fault_flag()
	{
		verify("Access denied" HERE), is_current_thread();
		m_mem_fault_flag.release(false);
	}

	// Fault recovery
	utils::address_range dma_manager::get_fault_range(bool writing) const
	{
		const auto m_current_job = verify(HERE, g_fxo->get<dma_thread>()->m_current_job);

		void *address = nullptr;
		u32 range = m_current_job->length;

		switch (m_current_job->type)
		{
		case raw_copy:
			address = (writing) ? m_current_job->dst : m_current_job->src;
			break;
		case vector_copy:
			verify(HERE), writing;
			address = m_current_job->dst;
			break;
		case index_emulate:
			verify(HERE), writing;
			address = m_current_job->dst;
			range = get_index_count(static_cast<rsx::primitive_type>(m_current_job->aux_param0), m_current_job->length);
			break;
		default:
			ASSUME(0);
			fmt::throw_exception("Unreachable" HERE);
		}

		const uintptr_t addr = uintptr_t(address);
		const uintptr_t base = uintptr_t(vm::g_base_addr);

		verify(HERE), addr > base;
		return utils::address_range::start_length(u32(addr - base), range);
	}
}
