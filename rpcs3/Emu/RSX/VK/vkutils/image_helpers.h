#pragma once
#include "../VulkanAPI.h"

namespace rsx
{
	struct texture_channel_remap_t;
}

namespace vk
{
	class image;
	class command_buffer;

	extern VkComponentMapping default_component_map;

	VkImageAspectFlags get_aspect_flags(VkFormat format);
	VkComponentMapping apply_swizzle_remap(const std::array<VkComponentSwizzle, 4>& base_remap, const rsx::texture_channel_remap_t& remap_vector);

	void change_image_layout(const vk::command_buffer& cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, const VkImageSubresourceRange& range,
		u32 src_queue_family = VK_QUEUE_FAMILY_IGNORED, u32 dst_queue_family = VK_QUEUE_FAMILY_IGNORED,
		u32 src_access_mask_bits = 0xFFFFFFFF, u32 dst_access_mask_bits = 0xFFFFFFFF);

	void change_image_layout(const vk::command_buffer& cmd, vk::image* image, VkImageLayout new_layout, const VkImageSubresourceRange& range);
	void change_image_layout(const vk::command_buffer& cmd, vk::image* image, VkImageLayout new_layout);
}
