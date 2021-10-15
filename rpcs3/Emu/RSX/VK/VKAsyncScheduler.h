#pragma once

#include "vkutils/commands.h"
#include "vkutils/sync.h"

#include "Utilities/Thread.h"

#define VK_MAX_ASYNC_COMPUTE_QUEUES 256

namespace vk
{
	enum class xqueue_event_type
	{
		label,
		barrier
	};

	struct xqueue_event
	{
		// Type
		xqueue_event_type type;

		// Payload
		std::unique_ptr<event> queue1_signal;
		std::unique_ptr<event> queue2_signal;

		// Identifiers
		u64 completion_eid;
		u64 uid;

		xqueue_event(u64 eid, u64 _uid)
			: type(xqueue_event_type::barrier), completion_eid(eid), uid(_uid)
		{}

		xqueue_event(std::unique_ptr<event>& trigger, std::unique_ptr<event>& payload, u64 eid, u64 _uid)
			: type(xqueue_event_type::label), queue1_signal(std::move(trigger)), queue2_signal(std::move(payload)),
			  completion_eid(eid), uid(_uid)
		{}
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
		std::vector<xqueue_event> m_barriers_pool;
		atomic_t<u64> m_submit_count = 0;

		// Scheduler
		shared_mutex m_config_mutex;
		bool m_options_initialized = false;
		bool m_use_host_scheduler = false;

		// Sync
		event* m_sync_label = nullptr;
		atomic_t<bool> m_sync_required = false;
		u64 m_sync_label_debug_uid = 0;

		static constexpr u32 events_pool_size = 16384;
		std::vector<xqueue_event> m_events_pool;
		atomic_t<u32> m_next_event_id = 0;

		lf_queue<xqueue_event*> m_event_queue;
		shared_mutex m_submit_mutex;

		void init_config_options();
		void delayed_init();
		void insert_sync_event();

	public:
		AsyncTaskScheduler(const std::string_view& name) : thread_name(name) {} // This ctor stops default initialization by fxo
		~AsyncTaskScheduler();

		command_buffer* get_current();
		event* get_primary_sync_label();
		u64 get_primary_sync_label_debug_uid();

		void flush(VkBool32 force_flush, VkSemaphore wait_semaphore = VK_NULL_HANDLE, VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		void kill();

		// Thread entry-point
		void operator()();

		const std::string_view thread_name;
	};

	using async_scheduler_thread = named_thread<AsyncTaskScheduler>;
}
