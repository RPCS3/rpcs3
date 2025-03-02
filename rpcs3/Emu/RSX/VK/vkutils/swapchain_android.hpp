#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(ANDROID)
	using swapchain_ANDROID = native_swapchain_base;
	using swapchain_NATIVE = swapchain_ANDROID;

	// TODO: Implement this
	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle, WSI_config* /*config*/)
	{
		return VK_NULL_HANDLE;
	}
#endif
}

