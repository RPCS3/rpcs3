#include "command_pool.h"
#include "render_device.h"
#include "shared.h"

namespace vk
{
	void command_pool::create(vk::render_device& dev)
	{
		owner                         = &dev;
		VkCommandPoolCreateInfo infos = {};
		infos.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		infos.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		CHECK_RESULT(vkCreateCommandPool(dev, &infos, nullptr, &pool));
	}

	void command_pool::destroy()
	{
		if (!pool)
			return;

		vkDestroyCommandPool((*owner), pool, nullptr);
		pool = nullptr;
	}

	vk::render_device& command_pool::get_owner()
	{
		return (*owner);
	}

	command_pool::operator VkCommandPool()
	{
		return pool;
	}
}
