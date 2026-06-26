#pragma once

// The Vulkan specific part of openxr_frame_share.h, used by the OpenXR renderer and the Vulkan backend of RSX

#include "openxr_frame_share.h"
#include "Emu/RSX/VK/VulkanAPI.h"

namespace openxr
{
#ifdef _WIN32
	constexpr VkExternalMemoryHandleTypeFlagBits shared_memory_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	constexpr const char* shared_memory_extension_name = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
	constexpr VkExternalSemaphoreHandleTypeFlagBits shared_semaphore_handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	constexpr const char* shared_semaphore_extension_name = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
#else
	constexpr VkExternalMemoryHandleTypeFlagBits shared_memory_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
	constexpr const char* shared_memory_extension_name = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
	constexpr VkExternalSemaphoreHandleTypeFlagBits shared_semaphore_handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
	constexpr const char* shared_semaphore_extension_name = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
#endif

	// Both devices have to create the shared images with the exact same parameters
	inline VkImageCreateInfo get_shared_image_info(VkFormat format, const VkExternalMemoryImageCreateInfo* external_info)
	{
		VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		info.pNext = external_info;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = format;
		info.extent = { shared_frames::image_width, shared_frames::image_height, 1 };
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		// OpenGL drivers expect images that can be rendered to and sampled when they import them
		info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		return info;
	}

	// The Vulkan backend leaves the images in this layout when it releases them to OpenXR.
	// The OpenGL backend hands them over in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL (GL_LAYOUT_TRANSFER_SRC_EXT) through the semaphore.
	constexpr VkImageLayout shared_image_written_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	constexpr VkImageLayout shared_image_released_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	static_assert(vk_format_r8g8b8a8_unorm == VK_FORMAT_R8G8B8A8_UNORM);
}
