#pragma once

#include "../VulkanAPI.h"

// Custom extensions to vulkan core
namespace vk
{
	struct buffer;
	struct buffer_view;
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

	struct VkDescriptorBufferViewEx
	{
		u64 resourceId = 0ull;
		VkBufferView view = VK_NULL_HANDLE;

		VkDescriptorBufferViewEx() = default;
		VkDescriptorBufferViewEx(const vk::buffer_view& view);
	};

	struct VkDescriptorBufferInfoEx : public VkDescriptorBufferInfo
	{
		u64 resourceId = 0ull;

		VkDescriptorBufferInfoEx() = default;
		VkDescriptorBufferInfoEx(const vk::buffer& buffer, u64 offset, u64 range);
	};
}

// Re-export
using VkDescriptorImageInfoEx = vk::VkDescriptorImageInfoEx;
using VkDescriptorBufferViewEx = vk::VkDescriptorBufferViewEx;
using VkDescriptorBufferInfoEx = vk::VkDescriptorBufferInfoEx;

