#pragma once
#include "image.h"

namespace vk
{
	VkSampler null_sampler();
	image_view* null_image_view(command_buffer&, VkImageViewType type);
	image* get_typeless_helper(VkFormat format, rsx::format_class format_class, u32 requested_width, u32 requested_height);
	buffer* get_scratch_buffer(u64 min_required_size);

	void clear_scratch_resources();
}
