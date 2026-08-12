#pragma once

#include "VulkanAPI.h"
#include "Utilities/Thread.h"

namespace vk
{
	struct fence;

	enum rctrl_command : u32 // callback commands
	{
		rctrl_queue_submit = 0x80000000,
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

	struct driver_manager_t
	{
		static constexpr std::string_view thread_name = "Vulkan Driver Manager"sv;
		void operator()();

		void notify_completed(u64 eid);
		void drain();

	private:
		atomic_t<u64> m_eid_ctr = 0ull;
		atomic_t<u64> m_last_completed_eid = 0ull;

		atomic_t<u32> m_wake_event = 0u;
		atomic_t<u32> m_completed_signal = 0u;     //<- Works around atomic_engine's lack of 64-bit observables support
	};

	using driver_manager_thread = named_thread<driver_manager_t>;
}
