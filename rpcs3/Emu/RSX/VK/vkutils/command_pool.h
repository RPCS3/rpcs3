#pragma once

#include "../VulkanAPI.h"

namespace vk
{
	class render_device;

	class command_pool
	{
		vk::render_device* owner = nullptr;
		VkCommandPool pool       = nullptr;

	public:
		command_pool()  = default;
		~command_pool() = default;

		void create(vk::render_device& dev);
		void destroy();

		vk::render_device& get_owner();

		operator VkCommandPool();
	};
}
