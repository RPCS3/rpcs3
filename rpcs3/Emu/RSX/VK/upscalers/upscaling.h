#pragma once

#include "../vkutils/commands.h"
#include "../vkutils/image.h"

namespace vk
{
	namespace upscaling_flags_
	{
		enum upscaling_flags
		{
			UPSCALE_DEFAULT_VIEW = (1 << 0),
			UPSCALE_LEFT_VIEW    = (1 << 0),
			UPSCALE_RIGHT_VIEW   = (1 << 1),
			UPSCALE_AND_COMMIT   = (1 << 2)
		};
	}

	using namespace upscaling_flags_;

	struct upscaler
	{
		virtual ~upscaler() {}

		virtual vk::viewable_image* scale_output(
			const vk::command_buffer& cmd,          // CB
			vk::viewable_image* src,                // Source input
			VkImage present_surface,                // Present target. May be VK_NULL_HANDLE for some passes
			VkImageLayout present_surface_layout,   // Present surface layout, or VK_IMAGE_LAYOUT_UNDEFINED if no present target is provided
			const VkImageBlit& request,             // Scaling request information
			rsx::flags32_t mode                     // Mode
		) = 0;
	};
}
