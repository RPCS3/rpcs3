#pragma once

#include "vkutils/commands.h"
#include "vkutils/sync.h"
#include "Utilities/mutex.h"

#define VK_MAX_ASYNC_COMPUTE_QUEUES 256

namespace vk
{
	class AsyncTaskScheduler
	{
		// Vulkan resources
		std::vector<command_buffer> m_async_command_queue;
		command_pool m_command_pool;

		// Running state
		command_buffer* m_last_used_cb = nullptr;
		command_buffer* m_current_cb = nullptr;
		usz m_next_cb_index = 0;
		atomic_t<u64> m_submit_count = 0;

		// Scheduler
		shared_mutex m_config_mutex;
		bool m_options_initialized = false;
		bool m_use_host_scheduler = false;

		// Sync
		event* m_sync_label = nullptr;
		atomic_t<bool> m_sync_required = false;
		VkDependencyInfoKHR m_dependency_info{};

		static constexpr u32 events_pool_size = 16384;
		std::vector<std::unique_ptr<vk::event>> m_events_pool;
		atomic_t<u64> m_next_event_id = 0;

		std::vector<std::unique_ptr<vk::semaphore>> m_semaphore_pool;
		atomic_t<u64> m_next_semaphore_id = 0;

		shared_mutex m_submit_mutex;

		void init_config_options(vk_gpu_scheduler_mode mode, const VkDependencyInfoKHR& queue_dependency);
		void delayed_init();
		void insert_sync_event();

	public:
		AsyncTaskScheduler(vk_gpu_scheduler_mode mode, const VkDependencyInfoKHR& queue_dependency);
		~AsyncTaskScheduler();

		command_buffer* get_current();
		event* get_primary_sync_label();
		semaphore* get_sema();

		void flush(queue_submit_t& submit_info, VkBool32 force_flush);
		void destroy();

		// Inline getters
		inline bool is_recording() const { return m_current_cb != nullptr; }
		inline bool is_host_mode() const { return m_use_host_scheduler; }
	};
}
