#include "stdafx.h"
#include "VKCommandStream.h"
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
	static void queue_submit_impl(VkQueue queue, const VkSubmitInfo* info, fence* pfence)
	{
		acquire_global_submit_lock();
		vkQueueSubmit(queue, 1, info, pfence->handle);
		release_global_submit_lock();

		// Signal fence
		pfence->signal_flushed();
	}

	void queue_submit(VkQueue queue, const VkSubmitInfo* info, fence* pfence, VkBool32 flush)
	{
		// Access to this method must be externally synchronized.
		// Offloader is guaranteed to never call this for async flushes.
		vk::descriptors::flush();

		if (!flush && g_cfg.video.multithreaded_rsx)
		{
			auto packet = new submit_packet(queue, pfence, info);
			g_fxo->get<rsx::dma_manager>().backend_ctrl(rctrl_queue_submit, packet);
		}
		else
		{
			queue_submit_impl(queue, info, pfence);
		}
	}

	void queue_submit(const vk::submit_packet* packet)
	{
		// Flush-only version used by asynchronous submit processing (MTRSX)
		queue_submit_impl(packet->queue, &packet->submit_info, packet->pfence);
	}
}
