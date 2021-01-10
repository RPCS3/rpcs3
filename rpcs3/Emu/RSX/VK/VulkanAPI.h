#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#elif HAVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#pragma warning( push )
#pragma warning( disable : 4005 )

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#pragma warning(pop)

#include <util/types.hpp>
