#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "restore_new.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include "define_new_memleakdetect.h"
#include "Utilities/types.h"

// TODO: Remove when packages catch up, ubuntu is stuck at 1.1.73 (bionic) and 1.1.82 (cosmic)
// Do we still use libvulkan-dev package on travis??????
#if VK_HEADER_VERSION < 95

typedef struct VkPhysicalDeviceFloat16Int8FeaturesKHR {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           shaderFloat16;
    VkBool32           shaderInt8;
} VkPhysicalDeviceFloat16Int8FeaturesKHR;

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR VkStructureType(1000082000)

#endif

namespace vk
{
	void init();
}
