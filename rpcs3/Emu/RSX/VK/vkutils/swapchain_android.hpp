#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(ANDROID)
	using swapchain_ANDROID = native_swapchain_base;
	using swapchain_NATIVE = swapchain_ANDROID;

	// TODO: Implement this
	static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle)
	{
		return VK_NULL_HANDLE;
	}
#endif
}

