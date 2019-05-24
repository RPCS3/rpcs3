#pragma once

#include "VKHelpers.h"

namespace vk
{
	u64 get_renderpass_key(const std::vector<vk::image*>& images);
	u64 get_renderpass_key(const std::vector<vk::image*>& images, u64 previous_key);
	u64 get_renderpass_key(VkFormat surface_format);
	VkRenderPass get_renderpass(VkDevice dev, u64 renderpass_key);

	void clear_renderpass_cache(VkDevice dev);
}
