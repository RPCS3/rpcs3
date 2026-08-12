#include "stdafx.h"
#include "VKCommandStream.h"
#include "VKResourceManager.h"
#include "vkutils/descriptors.h"
#include "vkutils/sync.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/RSXOffload.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/system_config.h"

namespace vk
{
	// global submit guard to prevent race condition on queue submit
	shared_mutex g_submit_mutex;

	void acquire_global_submit_lock()
	{
		g_submit_mutex.lock();
	}

	void release_global_submit_lock()
	{
		g_submit_mutex.unlock();
	}

	FORCE_INLINE
	static void queue_submit_impl(const queue_submit_t& submit_info)
	{
		ensure(submit_info.pfence);
		acquire_global_submit_lock();
		VkSubmitInfo info
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = submit_info.wait_semaphores_count,
			.pWaitSemaphores = submit_info.wait_semaphores.data(),
			.pWaitDstStageMask = submit_info.wait_stages.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &submit_info.commands,
			.signalSemaphoreCount = submit_info.signal_semaphores_count,
			.pSignalSemaphores = submit_info.signal_semaphores.data()
		};

		vkQueueSubmit(submit_info.queue, 1, &info, submit_info.pfence->handle);
		release_global_submit_lock();

		// Signal fence
		submit_info.pfence->signal_flushed();
	}

	void queue_submit(const queue_submit_t& submit_info, VkBool32 flush)
	{
		rsx::get_current_renderer()->get_stats().submit_count++;

		// Access to this method must be externally synchronized.
		// Offloader is guaranteed to never call this for async flushes.
		vk::descriptors::flush();

		if (!flush && g_cfg.video.multithreaded_rsx)
		{
			auto packet = new queue_submit_t(submit_info);
			g_fxo->get<rsx::dma_manager>().backend_ctrl(rctrl_queue_submit, packet);
		}
		else
		{
			queue_submit_impl(submit_info);
		}
	}

	void queue_submit(const queue_submit_t* packet)
	{
		// Flush-only version used by asynchronous submit processing (MTRSX)
		queue_submit_impl(*packet);
	}

	void driver_manager_t::operator()()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			const auto wake_token = m_wake_event.observe();
			const auto last_eid = m_last_completed_eid.load();

			if (auto eid = m_eid_ctr.load(); eid != last_eid)
			{
				vk::get_resource_manager()->eid_completed(eid);

				m_last_completed_eid.store(eid);
				m_completed_signal++;
				m_completed_signal.notify_all();
			}

			thread_ctrl::wait_on(m_wake_event, wake_token);
		}
	}

	void driver_manager_t::notify_completed(u64 eid)
	{
		m_eid_ctr.atomic_op([eid](u64& value)
		{
			value = std::max(value, eid);
		});

		m_wake_event++;
		m_wake_event.notify_one();
	}

	void driver_manager_t::drain()
	{
		const u64 target_eid = m_eid_ctr.load(); //<- Last request at time of calling

		// Now, we spam queue wake until our EID is reached
		while (true)
		{
			const u32 completion_token = m_completed_signal.observe();
			if (m_last_completed_eid.load() >= target_eid)
			{
				// Abort, watermark reached.
				break;
			}

			if (thread_ctrl::state() == thread_state::aborting)
			{
				// Abort, emulation state changed.
				break;
			}

			// Wake the worker thread.
			m_wake_event++;
			m_wake_event.notify_one();

			// Wait for worker thread.
			thread_ctrl::wait_on(m_completed_signal, completion_token);
		}
	}
}
