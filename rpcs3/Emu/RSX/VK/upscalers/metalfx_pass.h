#pragma once

#include "upscaling.h"

namespace vk
{
	class metalfx_upscale_pass : public upscaler
	{
	public:
		metalfx_upscale_pass();
		~metalfx_upscale_pass() override;

		static bool is_available();

		vk::viewable_image* scale_output(
			const vk::command_buffer& cmd,          // CB
			vk::viewable_image* src,                // Source input
			VkImage present_surface,                // Present target. May be VK_NULL_HANDLE for some passes
			VkImageLayout present_surface_layout,   // Present surface layout, or VK_IMAGE_LAYOUT_UNDEFINED if no present target is provided
			const VkImageBlit& request,             // Scaling request information
			rsx::flags32_t mode                     // Mode
		) override;

	private:
		struct impl;
		std::unique_ptr<impl> m_impl;
	};
}
