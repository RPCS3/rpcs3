#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(__APPLE__)
	using swapchain_MacOS = native_swapchain_base;
	using swapchain_NATIVE = swapchain_MacOS;

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle, WSI_config* /*config*/)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;
		VkMacOSSurfaceCreateInfoMVK createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
		createInfo.pView = window_handle;

		CHECK_RESULT(vkCreateMacOSSurfaceMVK(vk_instance, &createInfo, NULL, &result));
		return result;
	}
#endif
}
