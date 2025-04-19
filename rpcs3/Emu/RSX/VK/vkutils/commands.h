#pragma once

#include "../VulkanAPI.h"
#include "device.h"
#include "sync.h"

namespace vk
{
	class command_pool
	{
		vk::render_device* owner = nullptr;
		VkCommandPool pool       = nullptr;
		u32 queue_family         = 0;

	public:
		command_pool()  = default;
		~command_pool() = default;

		void create(vk::render_device& dev, u32 queue_family);
		void destroy();

		vk::render_device& get_owner() const;
		u32 get_queue_family() const;

		operator VkCommandPool() const;
	};

	struct queue_submit_t
	{
		VkQueue queue = VK_NULL_HANDLE;
		fence* pfence = nullptr;
		VkCommandBuffer commands = VK_NULL_HANDLE;
		std::array<VkSemaphore, 4> wait_semaphores;
		std::array<VkSemaphore, 4> signal_semaphores;
		std::array<VkPipelineStageFlags, 4> wait_stages;
		u32 wait_semaphores_count = 0;
		u32 signal_semaphores_count = 0;

		queue_submit_t() = default;
		queue_submit_t(VkQueue queue_, vk::fence* fence_)
			: queue(queue_), pfence(fence_) {}

		queue_submit_t(const queue_submit_t& other)
		{
			std::memcpy(static_cast<void*>(this), &other, sizeof(queue_submit_t));
		}

		inline queue_submit_t& wait_on(VkSemaphore semaphore, VkPipelineStageFlags stage)
		{
			ensure(wait_semaphores_count < 4);
			wait_semaphores[wait_semaphores_count] = semaphore;
			wait_stages[wait_semaphores_count++] = stage;
			return *this;
		}

		inline queue_submit_t& queue_signal(VkSemaphore semaphore)
		{
			ensure(signal_semaphores_count < 4);
			signal_semaphores[signal_semaphores_count++] = semaphore;
			return *this;
		}
	};

	class command_buffer
	{
	protected:
		bool is_open = false;
		bool is_pending = false;
		fence* m_submit_fence = nullptr;

		command_pool* pool = nullptr;
		VkCommandBuffer commands = nullptr;

	public:
		enum access_type_hint
		{
			flush_only, // Only to be submitted/opened/closed via command flush
			all         // Auxiliary, can be submitted/opened/closed at any time
		}
		access_hint = flush_only;

		enum command_buffer_data_flag : u32
		{
			cb_has_occlusion_task = 0x01,
			cb_has_blit_transfer  = 0x02,
			cb_has_dma_transfer   = 0x04,
			cb_has_open_query     = 0x08,
			cb_load_occluson_task = 0x10,
			cb_has_conditional_render = 0x20,
			cb_reload_dynamic_state   = 0x40
		};
		u32 flags = 0;

	public:
		command_buffer() = default;
		~command_buffer() = default;

		void create(command_pool& cmd_pool);
		void destroy();

		void begin();
		void end();
		void submit(queue_submit_t& submit_info, VkBool32 flush = VK_FALSE);

		// Properties
		command_pool& get_command_pool() const
		{
			return *pool;
		}

		u32 get_queue_family() const
		{
			return pool->get_queue_family();
		}

		void clear_flags()
		{
			flags = 0;
		}

		void set_flag(command_buffer_data_flag flag)
		{
			flags |= flag;
		}

		operator VkCommandBuffer() const
		{
			return commands;
		}

		bool is_recording() const
		{
			return is_open;
		}
	};
}
