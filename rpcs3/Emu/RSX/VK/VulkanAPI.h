#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#elif HAVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "restore_new.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include "define_new_memleakdetect.h"
#include "util/types.hpp"
