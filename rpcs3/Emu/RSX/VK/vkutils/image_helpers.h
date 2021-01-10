#pragma once
#include "../VulkanAPI.h"

namespace vk
{
	class image;
	extern VkComponentMapping default_component_map;

	VkImageAspectFlags get_aspect_flags(VkFormat format);
	VkComponentMapping apply_swizzle_remap(const std::array<VkComponentSwizzle, 4>& base_remap, const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap_vector);

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, const VkImageSubresourceRange& range);
	void change_image_layout(VkCommandBuffer cmd, vk::image* image, VkImageLayout new_layout, const VkImageSubresourceRange& range);
	void change_image_layout(VkCommandBuffer cmd, vk::image* image, VkImageLayout new_layout);
}
