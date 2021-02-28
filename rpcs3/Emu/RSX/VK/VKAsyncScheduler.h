#pragma once

#include "vkutils/commands.h"
#include "vkutils/sync.h"

#include "Utilities/Thread.h"

#define VK_MAX_ASYNC_COMPUTE_QUEUES 256

namespace vk
{
	struct xqueue_event
	{
		std::unique_ptr<event> queue1_signal;
		std::unique_ptr<event> queue2_signal;
		u64 completion_eid;
	};

	class AsyncTaskScheduler : private rsx::ref_counted
	{
		// Vulkan resources
		std::vector<command_buffer> m_async_command_queue;
		command_pool m_command_pool;

		// Running state
		command_buffer* m_last_used_cb = nullptr;
		command_buffer* m_current_cb = nullptr;
		usz m_next_cb_index = 0;

		// Sync
		event* m_sync_label = nullptr;
		bool m_sync_required = false;

		static constexpr u32 events_pool_size = 16384;
		std::vector<xqueue_event> m_events_pool;
		atomic_t<u32> m_next_event_id = 0;

		lf_queue<xqueue_event*> m_event_queue;
		shared_mutex m_submit_mutex;

		void delayed_init();
		void insert_sync_event();

	public:
		AsyncTaskScheduler() = default;
		~AsyncTaskScheduler();

		command_buffer* get_current();
		event* get_primary_sync_label();

		void flush(VkSemaphore wait_semaphore = VK_NULL_HANDLE, VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		// Thread entry-point
		void operator()();

		static constexpr auto thread_name = "Vulkan Async Scheduler"sv;
	};

	using async_scheduler_thread = named_thread<AsyncTaskScheduler>;
}