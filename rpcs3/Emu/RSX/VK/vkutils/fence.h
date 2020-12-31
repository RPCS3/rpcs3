#pragma once

#include "../VulkanAPI.h"
#include "util/atomic.hpp"

namespace vk
{
	struct fence
	{
		atomic_t<bool> flushed = false;
		VkFence handle         = VK_NULL_HANDLE;
		VkDevice owner         = VK_NULL_HANDLE;

		fence(VkDevice dev);
		~fence();

		void reset();
		void signal_flushed();
		void wait_flush();

		operator bool() const;
	};
}
