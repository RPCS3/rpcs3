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
#include <vulkan/vk_sdk_platform.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <util/types.hpp>

#if VK_HEADER_VERSION < 224

#define VK_EXT_attachment_feedback_loop_layout 1
#define VK_EXT_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_EXTENSION_NAME "VK_EXT_attachment_feedback_loop_layout"
#define VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT static_cast<VkImageLayout>(1000339000)
#define VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT 0x00080000
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT static_cast<VkStructureType>(1000339000)

typedef struct VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT {
	VkStructureType    sType;
	void*              pNext;
	VkBool32           attachmentFeedbackLoopLayout;
} VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT;

#endif
