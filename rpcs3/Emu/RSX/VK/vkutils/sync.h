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

		inline u64 inc_counter() volatile
		{
			// Workaround for volatile increment warning. GPU can see this value directly, but currently we do not modify it on the device.
			event_counter = event_counter + 1;
			return event_counter;
		}
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

	class device_marker_pool
	{
		std::unique_ptr<buffer> m_buffer;
		volatile u32* m_mapped = nullptr;
		u64 m_offset = 0;
		u32 m_count = 0;

		void create_impl();

	public:
		device_marker_pool(const vk::render_device& dev, u32 count);
		std::tuple<VkBuffer, u64, volatile u32*> allocate();

		const vk::render_device* pdev = nullptr;
	};

	class gpu_debug_marker
	{
		std::string m_message;
		bool m_printed = false;

		VkDevice m_device = VK_NULL_HANDLE;
		VkBuffer m_buffer = VK_NULL_HANDLE;
		u64 m_buffer_offset = 0;
		volatile u32* m_value = nullptr;

	public:
		gpu_debug_marker(device_marker_pool& pool, std::string message);
		~gpu_debug_marker();
		gpu_debug_marker(const event&) = delete;

		void signal(const command_buffer& cmd, VkPipelineStageFlags stages, VkAccessFlags access);
		void dump();
		void dump() const;

		static void insert(
			const vk::render_device& dev,
			const vk::command_buffer& cmd,
			std::string message,
			VkPipelineStageFlags stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkAccessFlags access = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
	};

	class debug_marker_scope
	{
		const vk::render_device* m_device;
		const vk::command_buffer* m_cb;
		std::string m_message;
		u32 m_tag;

	public:
		debug_marker_scope(const vk::command_buffer& cmd, const std::string& text);
		~debug_marker_scope();
	};

	VkResult wait_for_fence(fence* pFence, u64 timeout = 0ull);
	VkResult wait_for_event(event* pEvent, u64 timeout = 0ull);
}
