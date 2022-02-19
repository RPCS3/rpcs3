#pragma once

#include "../VulkanAPI.h"
#include "buffer_object.h"
#include "device.h"

#include "util/atomic.hpp"

namespace vk
{
	class command_buffer;

	enum class sync_domain
	{
		any = 0,
		gpu = 1
	};

	struct host_data_t // Pick a better name
	{
		u64 magic = 0xCAFEBABE;
		u64 event_counter = 0;
		u64 texture_load_request_event = 0;
		u64 texture_load_complete_event = 0;
		u64 last_label_release_event = 0;
		u64 last_label_submit_event = 0;
		u64 commands_complete_event = 0;
		u64 last_label_request_timestamp = 0;
	};

	struct fence
	{
		atomic_t<bool> flushed = false;
		VkFence handle         = VK_NULL_HANDLE;
		VkDevice owner         = VK_NULL_HANDLE;

		fence(VkDevice dev);
		~fence();
		fence(const fence&) = delete;

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
		event(const render_device& dev, sync_domain domain);
		~event();
		event(const event&) = delete;

		void signal(const command_buffer& cmd, VkPipelineStageFlags stages, VkAccessFlags access);
		void host_signal() const;
		void gpu_wait(const command_buffer& cmd) const;
		VkResult status() const;
		void reset() const;
	};

	class semaphore
	{
		VkSemaphore m_handle = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;

		semaphore() = default;

	public:
		semaphore(const render_device& dev);
		~semaphore();
		semaphore(const semaphore&) = delete;

		operator VkSemaphore() const;
	};

	VkResult wait_for_fence(fence* pFence, u64 timeout = 0ull);
	VkResult wait_for_event(event* pEvent, u64 timeout = 0ull);
}
