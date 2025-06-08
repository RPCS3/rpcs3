#pragma once

#include "vkutils/commands.h"
#include "vkutils/descriptors.h"
#include "VKDataHeapManager.h"
#include "VKResourceManager.h"

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/rsx_utils.h"
#include "Emu/RSX/rsx_cache.h"
#include "Utilities/mutex.h"
#include "util/asm.hpp"

#include <optional>
#include <thread>

// Initial heap allocation values. The heaps are growable and will automatically increase in size to accomodate demands
#define VK_ATTRIB_RING_BUFFER_SIZE_M 64
#define VK_TEXTURE_UPLOAD_RING_BUFFER_SIZE_M 64
#define VK_UBO_RING_BUFFER_SIZE_M 16
#define VK_TRANSFORM_CONSTANTS_BUFFER_SIZE_M 16
#define VK_FRAGMENT_CONSTANTS_BUFFER_SIZE_M 16
#define VK_INDEX_RING_BUFFER_SIZE_M 16

#define VK_MAX_ASYNC_CB_COUNT 512
#define VK_MAX_ASYNC_FRAMES 2

#define FRAME_PRESENT_TIMEOUT 10000000ull // 10 seconds
#define GENERAL_WAIT_TIMEOUT  2000000ull  // 2 seconds

namespace vk
{
	struct buffer_view;
	struct program_cache;
	struct pipeline_props;

	using vertex_cache = rsx::vertex_cache::default_vertex_cache<rsx::vertex_cache::uploaded_range>;
	using weak_vertex_cache = rsx::vertex_cache::weak_vertex_cache;
	using null_vertex_cache = vertex_cache;

	using shader_cache = rsx::shaders_cache<vk::pipeline_props, vk::program_cache>;

	struct vertex_upload_info
	{
		VkPrimitiveTopology primitive;
		u32 vertex_draw_count;
		u32 allocated_vertex_count;
		u32 first_vertex;
		u32 vertex_index_base;
		u32 vertex_index_offset;
		u32 persistent_window_offset;
		u32 volatile_window_offset;
		std::optional<std::tuple<VkDeviceSize, VkIndexType>> index_info;
	};

	struct command_buffer_chunk : public vk::command_buffer
	{
		u64 eid_tag = 0;
		u64 reset_id = 0;
		shared_mutex guard_mutex;

		command_buffer_chunk() = default;

		inline void tag()
		{
			eid_tag = vk::get_event_id();
		}

		void reset()
		{
			if (is_pending && !poke())
			{
				wait(FRAME_PRESENT_TIMEOUT);
			}

			++reset_id;
			CHECK_RESULT(vkResetCommandBuffer(commands, 0));
		}

		bool poke()
		{
			reader_lock lock(guard_mutex);

			if (!is_pending)
			{
				return true;
			}

			if (!m_submit_fence->flushed)
			{
				return false;
			}

			if (vkGetFenceStatus(pool->get_owner(), m_submit_fence->handle) == VK_SUCCESS)
			{
				lock.upgrade();

				if (is_pending)
				{
					m_submit_fence->reset();
					vk::on_event_completed(eid_tag);

					is_pending = false;
					eid_tag = 0;
				}
			}

			return !is_pending;
		}

		VkResult wait(u64 timeout = 0ull)
		{
			reader_lock lock(guard_mutex);

			if (!is_pending)
			{
				return VK_SUCCESS;
			}

			const auto ret = vk::wait_for_fence(m_submit_fence, timeout);

			lock.upgrade();

			if (is_pending)
			{
				m_submit_fence->reset();
				vk::on_event_completed(eid_tag);

				is_pending = false;
				eid_tag = 0;
			}

			return ret;
		}

		void flush()
		{
			reader_lock lock(guard_mutex);

			if (!is_pending)
			{
				return;
			}

			m_submit_fence->wait_flush();
		}
	};

	struct occlusion_data
	{
		rsx::simple_array<u32> indices;
		command_buffer_chunk* command_buffer_to_wait = nullptr;
		u64 command_buffer_sync_id = 0;

		bool is_current(command_buffer_chunk* cmd) const
		{
			return (command_buffer_to_wait == cmd && command_buffer_sync_id == cmd->reset_id);
		}

		void set_sync_command_buffer(command_buffer_chunk* cmd)
		{
			command_buffer_to_wait = cmd;
			command_buffer_sync_id = cmd->reset_id;
		}

		void sync()
		{
			if (command_buffer_to_wait->reset_id == command_buffer_sync_id)
			{
				// Allocation stack is FIFO and very long so no need to actually wait for fence signal
				command_buffer_to_wait->flush();
			}
		}
	};

	struct frame_context_t
	{
		VkSemaphore acquire_signal_semaphore = VK_NULL_HANDLE;
		VkSemaphore present_wait_semaphore = VK_NULL_HANDLE;

		rsx::flags32_t flags = 0;

		u32 present_image = -1;
		command_buffer_chunk* swap_command_buffer = nullptr;

		data_heap_manager::managed_heap_snapshot_t heap_snapshot;
		u64 last_frame_sync_time = 0;

		// Copy shareable information
		void grab_resources(frame_context_t& other)
		{
			present_wait_semaphore = other.present_wait_semaphore;
			acquire_signal_semaphore = other.acquire_signal_semaphore;
			flags = other.flags;
			heap_snapshot = other.heap_snapshot;
		}

		void tag_frame_end()
		{
			heap_snapshot = data_heap_manager::get_heap_snapshot();
			last_frame_sync_time = rsx::get_shared_tag();
		}

		void reset_heap_ptrs()
		{
			last_frame_sync_time = 0;
			heap_snapshot.clear();
		}
	};

	struct flush_request_task
	{
		atomic_t<bool> pending_state{ false };  //Flush request status; true if rsx::thread is yet to service this request
		atomic_t<int> num_waiters{ 0 };  //Number of threads waiting for this request to be serviced
		bool hard_sync = false;

		flush_request_task() = default;

		void post(bool _hard_sync)
		{
			hard_sync = (hard_sync || _hard_sync);
			pending_state = true;
			num_waiters++;
		}

		void remove_one()
		{
			num_waiters--;
		}

		void clear_pending_flag()
		{
			hard_sync = false;
			pending_state.store(false);
		}

		bool pending() const
		{
			return pending_state.load();
		}

		void consumer_wait() const
		{
			while (num_waiters.load() != 0)
			{
				utils::pause();
			}
		}

		void producer_wait() const
		{
			while (pending_state.load())
			{
				std::this_thread::yield();
			}
		}
	};

	struct present_surface_info
	{
		u32 address;
		u32 format;
		u32 width;
		u32 height;
		u32 pitch;
		u8  eye;
	};

	struct draw_call_t
	{
		u32 subdraw_id;
	};

	template<int Count>
	class command_buffer_chain
	{
		atomic_t<u32> m_current_index = 0;
		std::array<vk::command_buffer_chunk, Count> m_cb_list;

	public:
		command_buffer_chain() = default;

		void create(command_pool& pool, vk::command_buffer::access_type_hint access)
		{
			for (auto& cb : m_cb_list)
			{
				cb.create(pool);
				cb.access_hint = access;
			}
		}

		void destroy()
		{
			for (auto& cb : m_cb_list)
			{
				cb.destroy();
			}
		}

		void poke_all()
		{
			for (auto& cb : m_cb_list)
			{
				cb.poke();
			}
		}

		void wait_all()
		{
			for (auto& cb : m_cb_list)
			{
				cb.wait();
			}
		}

		inline command_buffer_chunk* next()
		{
			const auto result_id = ++m_current_index % Count;
			auto result = &m_cb_list[result_id];

			if (!result->poke())
			{
				rsx_log.error("CB chain has run out of free entries!");
			}

			return result;
		}

		inline command_buffer_chunk* get()
		{
			return &m_cb_list[m_current_index % Count];
		}
	};
}
