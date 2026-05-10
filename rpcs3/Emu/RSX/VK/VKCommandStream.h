#pragma once

#include "VulkanAPI.h"

namespace vk
{
	struct fence;

	enum rctrl_command : u32 // callback commands
	{
		rctrl_queue_submit = 0x80000000,
		rctrl_run_gc       = 0x80000001,
	};

	struct submit_packet
	{
		// Core components
		VkQueue      queue;
		fence*       pfence;
		VkSubmitInfo submit_info;

		// Pointer redirection storage
		VkSemaphore  wait_semaphore;
		VkSemaphore  signal_semaphore;
		VkFlags      wait_flags;

		submit_packet(VkQueue _q, fence* _f, const VkSubmitInfo* info) :
			queue(_q), pfence(_f), submit_info(*info),
			wait_semaphore(0), signal_semaphore(0), wait_flags(0)
		{
			if (info->waitSemaphoreCount)
			{
				wait_semaphore = *info->pWaitSemaphores;
				submit_info.pWaitSemaphores = &wait_semaphore;
			}

			if (info->signalSemaphoreCount)
			{
				signal_semaphore = *info->pSignalSemaphores;
				submit_info.pSignalSemaphores = &signal_semaphore;
			}

			if (info->pWaitDstStageMask)
			{
				wait_flags = *info->pWaitDstStageMask;
				submit_info.pWaitDstStageMask = &wait_flags;
			}
		}
	};
}
