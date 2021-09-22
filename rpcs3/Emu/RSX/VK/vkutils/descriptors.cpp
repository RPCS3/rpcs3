#include "descriptors.h"

namespace vk
{
	void descriptor_pool::create(const vk::render_device& dev, VkDescriptorPoolSize* sizes, u32 size_descriptors_count, u32 max_sets, u8 subpool_count)
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

	void descriptor_pool::destroy()
	{
		if (m_device_pools.empty()) return;

		for (auto& pool : m_device_pools)
		{
			vkDestroyDescriptorPool((*m_owner), pool, nullptr);
			pool = VK_NULL_HANDLE;
		}

		m_owner = nullptr;
	}

	bool descriptor_pool::valid() const
	{
		return (!m_device_pools.empty());
	}

	descriptor_pool::operator VkDescriptorPool()
	{
		return m_current_pool_handle;
	}

	void descriptor_pool::reset(VkDescriptorPoolResetFlags flags)
	{
		m_current_pool_index = (m_current_pool_index + 1) % u32(m_device_pools.size());
		m_current_pool_handle = m_device_pools[m_current_pool_index];
		CHECK_RESULT(vkResetDescriptorPool(*m_owner, m_current_pool_handle, flags));
	}
}
