#pragma once

#include "swapchain_core.h"

namespace vk
{
#if defined(ANDROID)
	using swapchain_ANDROID = native_swapchain_base;
	using swapchain_NATIVE = swapchain_ANDROID;

	[[maybe_unused]] static
	VkSurfaceKHR make_WSI_surface(VkInstance vk_instance, display_handle_t window_handle, WSI_config* /*config*/)
	{
		VkSurfaceKHR result = VK_NULL_HANDLE;

		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
		createInfo.window = std::get<ANativeWindow *>(window_handle);

		CHECK_RESULT(vkCreateAndroidSurfaceKHR(this->m_instance, &createInfo, nullptr, &result));
		return result;
	}
#endif
}
