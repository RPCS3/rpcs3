#pragma once

#include "swapchain_core.h"
#include "metal_layer.h"

namespace vk
{
#if defined(__APPLE__)
	using swapchain_MacOS = native_swapchain_base;
	using swapchain_NATIVE = swapchain_MacOS;

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle, WSI_config* /*config*/)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;
		VkMetalSurfaceCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		createInfo.pLayer = GetCAMetalLayerFromMetalView(window_handle);

		CHECK_RESULT(vkCreateMetalSurfaceEXT(vk_instance, &createInfo, NULL, &result));
		return result;
	}
#endif
}
