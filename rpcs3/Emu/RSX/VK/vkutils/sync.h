#pragma once

#include "../VulkanAPI.h"
#include "buffer_object.h"
#include "device.h"

#include "util/atomic.hpp"

namespace vk
{
	class command_buffer;

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

	class event
	{
		VkDevice m_device = VK_NULL_HANDLE;
		VkEvent m_vk_event = VK_NULL_HANDLE;

		std::unique_ptr<buffer> m_buffer;
		volatile u32* m_value = nullptr;

	public:
		event(const render_device& dev);
		~event();
		void signal(const command_buffer& cmd, VkPipelineStageFlags stages, VkAccessFlags access);
		VkResult status() const;
	};

	VkResult wait_for_fence(fence* pFence, u64 timeout = 0ull);
	VkResult wait_for_event(event* pEvent, u64 timeout = 0ull);
}
