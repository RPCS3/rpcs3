#pragma once

#include "../VulkanAPI.h"
#include <algorithm>
#include <vector>

namespace vk
{
	class supported_extensions
	{
	private:
		std::vector<VkExtensionProperties> m_vk_exts;

	public:
		enum enumeration_class
		{
			instance = 0,
			device   = 1
		};

		supported_extensions(enumeration_class _class, const char* layer_name = nullptr, VkPhysicalDevice pdev = VK_NULL_HANDLE)
		{
			u32 count;
			if (_class == enumeration_class::instance)
			{
				if (vkEnumerateInstanceExtensionProperties(layer_name, &count, nullptr) != VK_SUCCESS)
					return;
			}
			else
			{
				ensure(pdev);
				if (vkEnumerateDeviceExtensionProperties(pdev, layer_name, &count, nullptr) != VK_SUCCESS)
					return;
			}

			m_vk_exts.resize(count);
			if (_class == enumeration_class::instance)
			{
				vkEnumerateInstanceExtensionProperties(layer_name, &count, m_vk_exts.data());
			}
			else
			{
				vkEnumerateDeviceExtensionProperties(pdev, layer_name, &count, m_vk_exts.data());
			}
		}

		bool is_supported(const char* ext)
		{
			return std::any_of(m_vk_exts.cbegin(), m_vk_exts.cend(), [&](const VkExtensionProperties& p) { return std::strcmp(p.extensionName, ext) == 0; });
		}
	};
}
