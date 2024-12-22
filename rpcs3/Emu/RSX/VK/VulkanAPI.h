#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#elif HAVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4005 )
#endif

#include <vulkan/vulkan.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <util/types.hpp>

#if VK_HEADER_VERSION < 287
constexpr VkDriverId VK_DRIVER_ID_MESA_HONEYKRISP = static_cast<VkDriverId>(26);
#endif
