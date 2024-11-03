#pragma once

#include <util/types.hpp>

#include <unordered_map>
#include <functional>
#include <deque>

namespace rsx
{
	struct host_gpu_context_t
	{
		u64 magic = 0xCAFEBABE;
		u64 event_counter = 0;
		u64 texture_load_request_event = 0;
		u64 texture_load_complete_event = 0;
		u64 last_label_acquire_event = 0;
		u64 last_label_release2_event = 0;
		u64 commands_complete_event = 0;

		inline u64 inc_counter() volatile
		{
			// Workaround for volatile increment warning. GPU can see this value directly, but currently we do not modify it on the device.
			event_counter = event_counter + 1;
			return event_counter;
		}

		inline bool in_flight_commands_completed() const volatile
		{
			return last_label_release2_event <= commands_complete_event;
		}

		inline bool texture_loads_completed() const volatile
		{
			// Return true if all texture load requests are done.
			return texture_load_complete_event == texture_load_request_event;
		}

		inline bool has_unflushed_texture_loads() const volatile
		{
			return texture_load_request_event > last_label_release2_event;
		}

		inline u64 on_texture_load_acquire() volatile
		{
			texture_load_request_event = inc_counter();
			return texture_load_request_event;
		}

		inline void on_texture_load_release() volatile
		{
			// Normally released by the host device, but implemented nonetheless for software fallback
			texture_load_complete_event = texture_load_request_event;
		}

		inline u64 on_label_acquire() volatile
		{
			last_label_acquire_event = inc_counter();
			return last_label_acquire_event;
		}

		inline void on_label_release() volatile
		{
			last_label_release2_event = last_label_acquire_event;
		}

		inline bool needs_label_release() const volatile
		{
			return last_label_acquire_event > last_label_release2_event;
		}
	};

	struct host_gpu_write_op_t
	{
		int dispatch_class = 0;
		void* userdata = nullptr;
	};

	struct host_dispatch_handler_t
	{
		int dispatch_class = 0;
		std::function<bool(const volatile host_gpu_context_t*, const host_gpu_write_op_t*)> handler;
	};

	class RSXDMAWriter
	{
	public:
		RSXDMAWriter(void* mem)
			: m_host_context_ptr(new (mem)host_gpu_context_t)
		{}

		RSXDMAWriter(host_gpu_context_t* pctx)
			: m_host_context_ptr(pctx)
		{}

		void update();

		void register_handler(host_dispatch_handler_t handler);
		void deregister_handler(int dispatch_class);

		void enqueue(const host_gpu_write_op_t& request);
		void drain_label_queue();

		volatile host_gpu_context_t* host_ctx() const
		{
			return m_host_context_ptr;
		}

	private:
		std::unordered_map<int, host_dispatch_handler_t> m_dispatch_handlers;
		volatile host_gpu_context_t* m_host_context_ptr = nullptr;

		std::deque<host_gpu_write_op_t> m_job_queue;
	};
}
