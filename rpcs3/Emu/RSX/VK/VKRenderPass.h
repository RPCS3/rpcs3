#pragma once

#include "VKHelpers.h"

namespace vk
{
	u64 get_renderpass_key(const std::vector<vk::image*>& images);
	u64 get_renderpass_key(const std::vector<vk::image*>& images, u64 previous_key);
	u64 get_renderpass_key(VkFormat surface_format);
	VkRenderPass get_renderpass(VkDevice dev, u64 renderpass_key);

	void clear_renderpass_cache(VkDevice dev);

	// Renderpass scope management helpers.
	// NOTE: These are not thread safe by design.
	void begin_renderpass(VkDevice dev, VkCommandBuffer cmd, u64 renderpass_key, VkFramebuffer target, const coordu& framebuffer_region);
	void begin_renderpass(VkCommandBuffer cmd, VkRenderPass pass, VkFramebuffer target, const coordu& framebuffer_region);
	void end_renderpass(VkCommandBuffer cmd);
	bool is_renderpass_open(VkCommandBuffer cmd);
}
