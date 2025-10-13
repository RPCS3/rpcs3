#pragma once

#include "../VulkanAPI.h"

// Custom extensions to vulkan core
namespace vk
{
	struct image_view;
	struct sampler;

	// VkImage and VkImageView can be reused by drivers and are not safe for identity checking.
	// NVIDIA driver will crash horribly if such a thing happens, so we use our own resourceId field to uniquely identify an image and its descendants such as image views.
	struct VkDescriptorImageInfoEx : public VkDescriptorImageInfo
	{
		u64 resourceId = 0ull;

		VkDescriptorImageInfoEx() = default;
		VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler, VkImageLayout layout);
		VkDescriptorImageInfoEx(const vk::image_view& view, const vk::sampler& sampler);
		VkDescriptorImageInfoEx(const vk::image_view& view, VkSampler sampler);
		VkDescriptorImageInfoEx(const vk::image_view& view);
	};
}

// Re-export
using VkDescriptorImageInfoEx = vk::VkDescriptorImageInfoEx;

