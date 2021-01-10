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

		void create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count)
		{
			ensure(subpool_count);

			VkDescriptorPoolCreateInfo infos = {};
			infos.flags = 0;
			infos.maxSets = max_sets;
			infos.poolSizeCount = size_descriptors_count;
			infos.pPoolSizes = sizes;
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

			m_owner = &dev;
			m_device_pools.resize(subpool_count);

			for (auto& pool : m_device_pools)
			{
				CHECK_RESULT(vkCreateDescriptorPool(dev, &infos, nullptr, &pool));
			}

			m_current_pool_handle = m_device_pools[0];
		}

		void destroy()
		{
			if (m_device_pools.empty()) return;

			for (auto& pool : m_device_pools)
			{
				vkDestroyDescriptorPool((*m_owner), pool, nullptr);
				pool = VK_NULL_HANDLE;
			}

			m_owner = nullptr;
		}

		bool valid()
		{
			return (!m_device_pools.empty());
		}

		operator VkDescriptorPool()
		{
			return m_current_pool_handle;
		}

		void reset(VkDescriptorPoolResetFlags flags)
		{
			m_current_pool_index = (m_current_pool_index + 1) % u32(m_device_pools.size());
			m_current_pool_handle = m_device_pools[m_current_pool_index];
			CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_current_pool_handle, flags));
		}
	};
}
