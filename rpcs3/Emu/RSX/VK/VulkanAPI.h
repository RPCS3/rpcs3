#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include "Utilities/types.h"

namespace vk
{
	void init();
}
