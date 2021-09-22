#pragma once

#include "../VulkanAPI.h"
#include "device.h"

#include <vector>

namespace vk
{
	class descriptor_pool
	{
		const vk::render_device* m_owner = nullptr;

		std::vector<VkDescriptorPool> m_device_pools;
		VkDescriptorPool m_current_pool_handle = VK_NULL_HANDLE;
		u32 m_current_pool_index = 0;

	public:
		descriptor_pool() = default;
		~descriptor_pool() = default;

		void create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count);
		void destroy();
		void reset(VkDescriptorPoolResetFlags flags);

		bool valid() const;
		operator VkDescriptorPool();
	};
}
