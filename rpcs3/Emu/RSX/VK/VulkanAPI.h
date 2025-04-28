#pragma once

// Configure vulkan.h
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#elif defined(ANDROID)
#define VK_USE_PLATFORM_ANDROID_KHR
#else
#if defined(HAVE_X11)
 #define VK_USE_PLATFORM_XLIB_KHR
#endif
#if defined(HAVE_WAYLAND)
 #define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4005 )
#endif

#include <vulkan/vulkan.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Undefine header configuration variables
#undef VK_USE_PLATFORM_WIN32_KHR
#undef VK_USE_PLATFORM_MACOS_MVK
#undef VK_USE_PLATFORM_ANDROID_KHR
#undef VK_USE_PLATFORM_XLIB_KHR
#undef VK_USE_PLATFORM_WAYLAND_KHR

#include <util/types.hpp>

#if VK_HEADER_VERSION < 287
constexpr VkDriverId VK_DRIVER_ID_MESA_HONEYKRISP = static_cast<VkDriverId>(26);
#endif

#define DECLARE_VK_FUNCTION_HEADER 1
#include "VKProcTable.h"

namespace vk
{
	void init();
}
