#pragma once

#include "upscaling.h"

namespace vk
{
	struct nearest_upscale_pass : public upscaler
	{
		vk::viewable_image* scale_output(
			const vk::command_buffer& cmd,          // CB
			vk::viewable_image* src,                // Source input
			VkImage present_surface,                // Present target. May be VK_NULL_HANDLE for some passes
			VkImageLayout present_surface_layout,   // Present surface layout, or VK_IMAGE_LAYOUT_UNDEFINED if no present target is provided
			const VkImageBlit& request,             // Scaling request information
			rsx::flags32_t mode                     // Mode
		) override
		{
			if (mode & UPSCALE_AND_COMMIT)
			{
				ensure(present_surface);

				src->push_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				vkCmdBlitImage(cmd, src->value, src->current_layout, present_surface, present_surface_layout, 1, &request, VK_FILTER_NEAREST);
				src->pop_layout(cmd);
				return nullptr;
			}

			// Upscaling source only is unsupported
			return src;
		}
	};
}
