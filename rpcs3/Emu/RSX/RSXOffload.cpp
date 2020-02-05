#include "stdafx.h"

#include "Common/BufferUtils.h"
#include "Emu/System.h"
#include "RSXOffload.h"
#include "RSXThread.h"
#include "rsx_utils.h"

#include <thread>
#include <atomic>

namespace rsx
{
	// initialization
	void dma_manager::init()
	{
		m_worker_state = thread_state::created;
		m_enqueued_count.store(0);
		m_processed_count = 0;

		// Empty work queue in case of stale contents
		m_work_queue.pop_all();

		thread_ctrl::spawn("RSX offloader", [this]()
		{
			if (!g_cfg.video.multithreaded_rsx)
			{
				// Abort
				return;
			}

			// Register thread id
			m_thread_id = std::this_thread::get_id();

			if (g_cfg.core.thread_scheduler_enabled)
			{
				thread_ctrl::set_thread_affinity_mask(thread_ctrl::get_affinity_mask(thread_class::rsx));
			}

			while (m_worker_state != thread_state::finished)
			{
				if (m_enqueued_count.load() != m_processed_count)
				{
					for (m_current_job = m_work_queue.pop_all(); m_current_job; m_current_job.pop_front())
					{
						switch (m_current_job->type)
						{
						case raw_copy:
							memcpy(m_current_job->dst, m_current_job->src, m_current_job->length);
							break;
						case vector_copy:
							memcpy(m_current_job->dst, m_current_job->opt_storage.data(), m_current_job->length);
							break;
						case index_emulate:
							write_index_array_for_non_indexed_non_native_primitive_to_buffer(
								reinterpret_cast<char*>(m_current_job->dst),
								static_cast<rsx::primitive_type>(m_current_job->aux_param0),
								m_current_job->length);
							break;
						case callback:
							rsx::get_current_renderer()->renderctl(m_current_job->aux_param0, m_current_job->src);
							break;
						default:
							ASSUME(0);
							fmt::throw_exception("Unreachable" HERE);
						}

						std::atomic_thread_fence(std::memory_order_release);
						++m_processed_count;
					}
				}
				else
				{
					// Yield
					std::this_thread::yield();
				}
			}

			m_processed_count = m_enqueued_count.load();
		});
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
			++m_enqueued_count;
			m_work_queue.push(dst, src, length);
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
			++m_enqueued_count;
			m_work_queue.push(dst, src, length);
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
			++m_enqueued_count;
			m_work_queue.push(dst, primitive, count);
		}
	}

	// Backend callback
	void dma_manager::backend_ctrl(u32 request_code, void* args)
	{
		verify(HERE), g_cfg.video.multithreaded_rsx;

		++m_enqueued_count;
		m_work_queue.push(request_code, args);
	}

	// Synchronization
	bool dma_manager::is_current_thread() const
	{
		return (std::this_thread::get_id() == m_thread_id);
	}

	bool dma_manager::sync()
	{
		if (m_enqueued_count.load() == m_processed_count) [[likely]]
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

			while (m_enqueued_count.load() != m_processed_count)
			{
				rsxthr->on_semaphore_acquire_wait();
				_mm_pause();
			}
		}
		else
		{
			while (m_enqueued_count.load() != m_processed_count)
				_mm_pause();
		}

		return true;
	}

	void dma_manager::join()
	{
		m_worker_state = thread_state::finished;
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
		verify(HERE), m_current_job;

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
