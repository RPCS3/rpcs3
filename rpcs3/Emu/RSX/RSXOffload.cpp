#include "stdafx.h"

#include "Emu/Memory/vm.h"
#include "Common/BufferUtils.h"
#include "Core/RSXReservationLock.hpp"
#include "RSXOffload.h"
#include "RSXThread.h"

#include "Utilities/lockless.h"

#include <thread>
#include "util/asm.hpp"

namespace rsx
{
	struct dma_manager::offload_thread
	{
		lf_queue<transport_packet> m_work_queue;
		atomic_t<u64> m_enqueued_count = 0;
		atomic_t<u64> m_processed_count = 0;
		transport_packet* m_current_job = nullptr;

		thread_base* current_thread_ = nullptr;

		void operator ()()
		{
			if (!g_cfg.video.multithreaded_rsx)
			{
				// Abort if disabled
				return;
			}

			current_thread_ = thread_ctrl::get_current();
			ensure(current_thread_);

			if (g_cfg.core.thread_scheduler != thread_scheduler_mode::os)
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
						const u32 vm_addr = vm::try_get_addr(job.src).first;
						rsx::reservation_lock<true, 1> rsx_lock(vm_addr, job.length, g_cfg.video.strict_rendering_mode && vm_addr);
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
					default: fmt::throw_exception("Unreachable");
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

	// initialization
	void dma_manager::init()
	{
		m_thread = std::make_shared<named_thread<offload_thread>>();
	}

	// General transport
	void dma_manager::copy(void *dst, std::vector<u8>& src, u32 length) const
	{
		if (length <= max_immediate_transfer_size || !g_cfg.video.multithreaded_rsx)
		{
			std::memcpy(dst, src.data(), length);
		}
		else
		{
			m_thread->m_enqueued_count++;
			m_thread->m_work_queue.push(dst, src, length);
		}
	}

	void dma_manager::copy(void *dst, void *src, u32 length) const
	{
		if (length <= max_immediate_transfer_size || !g_cfg.video.multithreaded_rsx)
		{
			const u32 vm_addr = vm::try_get_addr(src).first;
			rsx::reservation_lock<true, 1> rsx_lock(vm_addr, length, g_cfg.video.strict_rendering_mode && vm_addr);
			std::memcpy(dst, src, length);
		}
		else
		{
			m_thread->m_enqueued_count++;
			m_thread->m_work_queue.push(dst, src, length);
		}
	}

	// Vertex utilities
	void dma_manager::emulate_as_indexed(void *dst, rsx::primitive_type primitive, u32 count)
	{
		if (!g_cfg.video.multithreaded_rsx)
		{
			write_index_array_for_non_indexed_non_native_primitive_to_buffer(
				static_cast<char*>(dst), primitive, count);
		}
		else
		{
			m_thread->m_enqueued_count++;
			m_thread->m_work_queue.push(dst, primitive, count);
		}
	}

	// Backend callback
	void dma_manager::backend_ctrl(u32 request_code, void* args)
	{
		ensure(g_cfg.video.multithreaded_rsx);

		m_thread->m_enqueued_count++;
		m_thread->m_work_queue.push(request_code, args);
	}

	// Synchronization
	bool dma_manager::is_current_thread() const
	{
		if (auto cpu = thread_ctrl::get_current())
		{
			return m_thread->current_thread_ == cpu;
		}

		return false;
	}

	bool dma_manager::sync() const
	{
		auto& _thr = *m_thread;

		if (_thr.m_enqueued_count.load() <= _thr.m_processed_count.load()) [[likely]]
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

			while (_thr.m_enqueued_count.load() > _thr.m_processed_count.load())
			{
				rsxthr->on_semaphore_acquire_wait();
				utils::pause();
			}
		}
		else
		{
			while (_thr.m_enqueued_count.load() > _thr.m_processed_count.load())
				utils::pause();
		}

		return true;
	}

	void dma_manager::join()
	{
		sync();
		*m_thread = thread_state::aborting;
	}

	void dma_manager::set_mem_fault_flag()
	{
		ensure(is_current_thread()); // "Access denied"
		m_mem_fault_flag.release(true);
	}

	void dma_manager::clear_mem_fault_flag()
	{
		ensure(is_current_thread()); // "Access denied"
		m_mem_fault_flag.release(false);
	}

	// Fault recovery
	utils::address_range32 dma_manager::get_fault_range(bool writing) const
	{
		const auto m_current_job = ensure(m_thread->m_current_job);

		void *address = nullptr;
		u32 range = m_current_job->length;

		switch (m_current_job->type)
		{
		case raw_copy:
			address = (writing) ? m_current_job->dst : m_current_job->src;
			break;
		case vector_copy:
			ensure(writing);
			address = m_current_job->dst;
			break;
		case index_emulate:
			ensure(writing);
			address = m_current_job->dst;
			range = get_index_count(static_cast<rsx::primitive_type>(m_current_job->aux_param0), m_current_job->length);
			break;
		default:
			fmt::throw_exception("Unreachable");
		}

		return utils::address_range32::start_length(vm::get_addr(address), range);
	}
}
