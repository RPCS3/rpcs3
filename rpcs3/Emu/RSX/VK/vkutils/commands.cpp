#include "commands.h"
#include "device.h"
#include "shared.h"
#include "sync.h"

namespace vk
{
	// This queue flushing method to be implemented by the backend as behavior depends on config
	void queue_submit(const queue_submit_t& submit_info, VkBool32 flush);

	void command_pool::create(vk::render_device& dev, u32 queue_family_id)
	{
		owner = &dev;
		queue_family = queue_family_id;

		VkCommandPoolCreateInfo infos = {};
		infos.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		infos.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		infos.queueFamilyIndex = queue_family;

		CHECK_RESULT(vkCreateCommandPool(dev, &infos, nullptr, &pool));
	}

	void command_pool::destroy()
	{
		if (!pool)
			return;

		vkDestroyCommandPool((*owner), pool, nullptr);
		pool = nullptr;
	}

	vk::render_device& command_pool::get_owner() const
	{
		return (*owner);
	}

	u32 command_pool::get_queue_family() const
	{
		return queue_family;
	}

	command_pool::operator VkCommandPool() const
	{
		return pool;
	}

	void command_buffer::create(command_pool& cmd_pool)
	{
		VkCommandBufferAllocateInfo infos = {};
		infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		infos.commandBufferCount = 1;
		infos.commandPool = +cmd_pool;
		infos.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		CHECK_RESULT(vkAllocateCommandBuffers(cmd_pool.get_owner(), &infos, &commands));

		m_submit_fence = new fence(cmd_pool.get_owner());
		pool = &cmd_pool;
	}

	void command_buffer::destroy()
	{
		vkFreeCommandBuffers(pool->get_owner(), (*pool), 1, &commands);

		if (m_submit_fence)
		{
			//vkDestroyFence(pool->get_owner(), m_submit_fence, nullptr);
			delete m_submit_fence;
			m_submit_fence = nullptr;
		}
	}

	void command_buffer::begin()
	{
		if (m_submit_fence && is_pending)
		{
			wait_for_fence(m_submit_fence);
			is_pending = false;

			//CHECK_RESULT(vkResetFences(pool->get_owner(), 1, &m_submit_fence));
			m_submit_fence->reset();
			CHECK_RESULT(vkResetCommandBuffer(commands, 0));
		}

		if (is_open)
			return;

		VkCommandBufferInheritanceInfo inheritance_info = {};
		inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

		VkCommandBufferBeginInfo begin_infos = {};
		begin_infos.pInheritanceInfo = &inheritance_info;
		begin_infos.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_infos.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		CHECK_RESULT(vkBeginCommandBuffer(commands, &begin_infos));
		is_open = true;
	}

	void command_buffer::end()
	{
		if (!is_open)
		{
			rsx_log.error("commandbuffer->end was called but commandbuffer is not in a recording state");
			return;
		}

		CHECK_RESULT(vkEndCommandBuffer(commands));
		is_open = false;
	}

	void command_buffer::submit(queue_submit_t& submit_info, VkBool32 flush)
	{
		if (is_open)
		{
			rsx_log.error("commandbuffer->submit was called whilst the command buffer is in a recording state");
			return;
		}

		// Check for hanging queries to avoid driver hang
		ensure((flags & cb_has_open_query) == 0); // "close and submit of commandbuffer with a hanging query!"

		if (!submit_info.pfence)
		{
			submit_info.pfence = m_submit_fence;
			is_pending = bool(submit_info.pfence);
		}

		submit_info.commands = this->commands;
		queue_submit(submit_info, flush);
		clear_flags();
	}
}
