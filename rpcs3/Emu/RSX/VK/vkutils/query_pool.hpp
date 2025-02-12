#pragma once

#include "../VulkanAPI.h"
#include "../../rsx_utils.h"
#include "shared.h"

namespace vk
{
	class query_pool : public rsx::ref_counted
	{
		VkQueryPool m_query_pool;
		VkDevice m_device;
		u32 m_size;

	public:
		query_pool(VkDevice dev, VkQueryType type, u32 size)
			: m_query_pool(VK_NULL_HANDLE)
			, m_device(dev)
			, m_size(size)
		{
			VkQueryPoolCreateInfo info{};
			info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.queryType  = type;
			info.queryCount = size;
			CHECK_RESULT(vkCreateQueryPool(dev, &info, nullptr, &m_query_pool));

			// Take 'size' references on this object
			ref_count.release(static_cast<s32>(size));
		}

		~query_pool()
		{
			vkDestroyQueryPool(m_device, m_query_pool, nullptr);
		}

		operator VkQueryPool()
		{
			return m_query_pool;
		}

		inline u32 size() const
		{
			return m_size;
		}
	};
}
