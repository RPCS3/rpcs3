#pragma once

#include "upscaling.h"

namespace vk
{
	// MetalFX spatial upscaler.
	//
	// This pass is only functional on macOS builds running on top of MoltenVK, where the
	// VK_EXT_metal_objects extension lets us hand the underlying MTLTextures of our VkImages
	// to Apple's MTLFXSpatialScaler. On every other platform (and on Apple hardware/OS
	// versions that do not support MetalFX) the pass reports itself unavailable and the
	// present path is expected to fall back to bilinear upscaling.
	class metalfx_upscale_pass : public upscaler
	{
	public:
		metalfx_upscale_pass();
		~metalfx_upscale_pass() override;

		// Runtime probe. Returns true only when a MetalFX spatial scaler can actually be
		// created for the currently active render device. Cheap to call; result is cached.
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
		// Opaque Objective-C++ state kept out of this header so it can be included from plain C++.
		struct impl;
		std::unique_ptr<impl> m_impl;
	};
}
