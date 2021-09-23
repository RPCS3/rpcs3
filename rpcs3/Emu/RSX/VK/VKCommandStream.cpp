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

	void queue_submit(VkQueue queue, const VkSubmitInfo* info, fence* pfence, VkBool32 flush)
	{
		if (rsx::get_current_renderer()->is_current_thread())
		{
			vk::descriptors::flush();
		}

		if (!flush && g_cfg.video.multithreaded_rsx)
		{
			auto packet = new submit_packet(queue, pfence, info);
			g_fxo->get<rsx::dma_manager>().backend_ctrl(rctrl_queue_submit, packet);
		}
		else
		{
			acquire_global_submit_lock();
			vkQueueSubmit(queue, 1, info, pfence->handle);
			release_global_submit_lock();

			// Signal fence
			pfence->signal_flushed();
		}
	}
}
