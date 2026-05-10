#pragma once

#include "../VulkanAPI.h"
#include "buffer_object.h"
#include "device.h"

#include "util/atomic.hpp"

namespace vk
{
	class command_buffer;
	class gpu_label;
	class image;

	enum class sync_domain
	{
		host = 0,
		gpu = 1
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
		enum class sync_backend
		{
			events_v1,
			events_v2,
			gpu_label
		};

		const vk::render_device* m_device = nullptr;
		sync_domain m_domain = sync_domain::host;
		sync_backend m_backend = sync_backend::events_v1;

		// For events_v1 and events_v2
		VkEvent m_vk_event = VK_NULL_HANDLE;

		// For gpu_label
		std::unique_ptr<gpu_label> m_label{};

		void resolve_dependencies(const command_buffer& cmd, const VkDependencyInfoKHR& dependency);

	public:
		event(const render_device& dev, sync_domain domain);
		~event();
		event(const event&) = delete;

		void signal(const command_buffer& cmd, const VkDependencyInfoKHR& dependency);
		void host_signal() const;
		void gpu_wait(const command_buffer& cmd, const VkDependencyInfoKHR& dependency) const;
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

	// Custom primitives
	class gpu_label_pool
	{
	public:
		gpu_label_pool(const vk::render_device& dev, u32 count);
		virtual ~gpu_label_pool();

		std::tuple<VkBuffer, u64, volatile u32*> allocate();

	private:
		void create_impl();

		const vk::render_device* pdev = nullptr;
		std::unique_ptr<buffer> m_buffer{};
		volatile u32* m_mapped = nullptr;
		u64 m_offset = 0;
		u32 m_count = 0;
	};

	class gpu_label
	{
	protected:
		enum label_constants : u32
		{
			set_ = 0xCAFEBABE,
			reset_ = 0xDEADBEEF
		};

		VkBuffer m_buffer_handle = VK_NULL_HANDLE;
		u64 m_buffer_offset = 0;
		volatile u32* m_ptr = nullptr;

	public:
		gpu_label(gpu_label_pool& pool);
		virtual ~gpu_label();

		void signal(const vk::command_buffer& cmd, const VkDependencyInfoKHR& dependency);
		void reset() { *m_ptr = label_constants::reset_; }
		void set() { *m_ptr = label_constants::set_; }
		bool signaled() const { return label_constants::set_ == *m_ptr; }
	};

	class gpu_debug_marker_pool : public gpu_label_pool
	{
		using gpu_label_pool::gpu_label_pool;
	};

	class gpu_debug_marker : public gpu_label
	{
		std::string m_message;
		bool m_printed = false;

	public:
		gpu_debug_marker(gpu_debug_marker_pool& pool, std::string message);
		~gpu_debug_marker();
		gpu_debug_marker(const event&) = delete;

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
		u64 m_tag;

	public:
		debug_marker_scope(const vk::command_buffer& cmd, const std::string& text);
		~debug_marker_scope();
	};

	VkResult wait_for_fence(fence* pFence, u64 timeout = 0ull);
	VkResult wait_for_event(event* pEvent, u64 timeout = 0ull);
}
