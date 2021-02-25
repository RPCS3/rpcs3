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

		void create(vk::render_device& dev, u32 queue_family = 0);
		void destroy();

		vk::render_device& get_owner() const;
		u32 get_queue_family() const;

		operator VkCommandPool() const;
	};

	class command_buffer
	{
	private:
		bool is_open = false;
		bool is_pending = false;
		fence* m_submit_fence = nullptr;

	protected:
		command_pool* pool = nullptr;
		VkCommandBuffer commands = nullptr;

	public:
		enum access_type_hint
		{
			flush_only, //Only to be submitted/opened/closed via command flush
			all         //Auxiliary, can be submitted/opened/closed at any time
		}
		access_hint = flush_only;

		enum command_buffer_data_flag : u32
		{
			cb_has_occlusion_task = 1,
			cb_has_blit_transfer = 2,
			cb_has_dma_transfer = 4,
			cb_has_open_query = 8,
			cb_load_occluson_task = 16,
			cb_has_conditional_render = 32
		};
		u32 flags = 0;

	public:
		command_buffer() = default;
		~command_buffer() = default;

		void create(command_pool& cmd_pool, bool auto_reset = false);
		void destroy();

		void begin();
		void end();
		void submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, fence* pfence, VkPipelineStageFlags pipeline_stage_flags, VkBool32 flush = VK_FALSE);

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
