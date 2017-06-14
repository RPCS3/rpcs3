#include "stdafx.h"
#include "VKHelpers.h"

namespace vk
{
	context* g_current_vulkan_ctx = nullptr;
	render_device g_current_renderer;

	texture g_null_texture;

	VkSampler g_null_sampler      = nullptr;
	VkImageView g_null_image_view = nullptr;

	bool g_cb_no_interrupt_flag = false;

	VKAPI_ATTR void* VKAPI_CALL mem_realloc(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
#ifdef _MSC_VER
		return _aligned_realloc(pOriginal, size, alignment);
#elif _WIN32
		return __mingw_aligned_realloc(pOriginal, size, alignment);
#else
		std::abort();
#endif
	}

	VKAPI_ATTR void* VKAPI_CALL mem_alloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
#ifdef _MSC_VER
		return _aligned_malloc(size, alignment);
#elif _WIN32
		return __mingw_aligned_malloc(size, alignment);
#else
		std::abort();
#endif
	}

	VKAPI_ATTR void VKAPI_CALL mem_free(void* pUserData, void* pMemory)
	{
#ifdef _MSC_VER
		_aligned_free(pMemory);
#elif _WIN32
		__mingw_aligned_free(pMemory);
#else
		std::abort();
#endif
	}

	memory_type_mapping get_memory_mapping(VkPhysicalDevice pdev)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

		memory_type_mapping result;
		result.device_local = VK_MAX_MEMORY_TYPES;
		result.host_visible_coherent = VK_MAX_MEMORY_TYPES;

		bool host_visible_cached = false;
		VkDeviceSize  host_visible_vram_size = 0;
		VkDeviceSize  device_local_vram_size = 0;

		for (u32 i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			VkMemoryHeap &heap = memory_properties.memoryHeaps[memory_properties.memoryTypes[i].heapIndex];
			
			bool is_device_local = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (is_device_local)
			{
				if (device_local_vram_size < heap.size)
				{
					result.device_local = i;
					device_local_vram_size = heap.size;
				}
			}

			bool is_host_visible = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			bool is_host_coherent = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			bool is_cached = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			
			if (is_host_coherent && is_host_visible)
			{
				if ((is_cached && !host_visible_cached) ||
					(host_visible_vram_size < heap.size))
				{
					result.host_visible_coherent = i;
					host_visible_vram_size = heap.size;
					host_visible_cached = is_cached;
				}
			}
		}

		if (result.device_local == VK_MAX_MEMORY_TYPES) fmt::throw_exception("GPU doesn't support device local memory" HERE);
		if (result.host_visible_coherent == VK_MAX_MEMORY_TYPES) fmt::throw_exception("GPU doesn't support host coherent device local memory" HERE);
		return result;
	}

	VkFormat get_compatible_sampler_format(u32 format)
	{
		switch (format)
		{
		case CELL_GCM_TEXTURE_B8: return VK_FORMAT_R8_UNORM;
		case CELL_GCM_TEXTURE_A1R5G5B5: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A4R4G4B4: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case CELL_GCM_TEXTURE_R5G6B5: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A8R8G8B8: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return VK_FORMAT_BC2_UNORM_BLOCK;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return VK_FORMAT_BC3_UNORM_BLOCK;
		case CELL_GCM_TEXTURE_G8B8: return VK_FORMAT_R8G8_UNORM;
		case CELL_GCM_TEXTURE_R6G5B5: return VK_FORMAT_R5G6B5_UNORM_PACK16; // Expand, discard high bit?
		case CELL_GCM_TEXTURE_DEPTH24_D8: return VK_FORMAT_R32_UINT;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:	return VK_FORMAT_R32_SFLOAT;
		case CELL_GCM_TEXTURE_DEPTH16: return VK_FORMAT_R16_UINT;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return VK_FORMAT_R16_SFLOAT;
		case CELL_GCM_TEXTURE_X16: return VK_FORMAT_R16_UNORM;
		case CELL_GCM_TEXTURE_Y16_X16: return VK_FORMAT_R16G16_UNORM;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
		case CELL_GCM_TEXTURE_R5G5B5A1: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case CELL_GCM_TEXTURE_X32_FLOAT: return VK_FORMAT_R32_SFLOAT;
		case CELL_GCM_TEXTURE_D1R5G5B5: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_D8R8G8B8: return VK_FORMAT_B8G8R8A8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;	// Expand
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return VK_FORMAT_R8G8B8A8_UNORM; // Expand
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return VK_FORMAT_R8G8_UNORM;
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return VK_FORMAT_R8G8_SNORM;
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return VK_FORMAT_R8G8_UNORM; // Not right
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return VK_FORMAT_R8G8_UNORM; // Not right
		}
		fmt::throw_exception("Invalid or unsupported sampler format for texture format (0x%x)" HERE, format);
	}

	VkAllocationCallbacks default_callbacks()
	{
		VkAllocationCallbacks callbacks;
		callbacks.pfnAllocation = vk::mem_alloc;
		callbacks.pfnFree = vk::mem_free;
		callbacks.pfnReallocation = vk::mem_realloc;

		return callbacks;
	}

	VkSampler null_sampler()
	{
		if (g_null_sampler)
			return g_null_sampler;

		VkSamplerCreateInfo sampler_info = {};

		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.anisotropyEnable = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.mipLodBias = 0;
		sampler_info.maxAnisotropy = 0;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.compareOp = VK_COMPARE_OP_NEVER;
		sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		vkCreateSampler(g_current_renderer, &sampler_info, nullptr, &g_null_sampler);
		return g_null_sampler;
	}

	VkImageView null_image_view()
	{
		if (g_null_image_view)
			return g_null_image_view;

		g_null_texture.create(g_current_renderer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, 4, 4);
		g_null_image_view = g_null_texture;
		return g_null_image_view;
	}

	void destroy_global_resources()
	{
		g_null_texture.destroy();

		if (g_null_sampler)
			vkDestroySampler(g_current_renderer, g_null_sampler, nullptr);

		g_null_sampler = nullptr;
		g_null_image_view = nullptr;
	}

	void set_current_thread_ctx(const vk::context &ctx)
	{
		g_current_vulkan_ctx = (vk::context *)&ctx;
	}

	context *get_current_thread_ctx()
	{
		return g_current_vulkan_ctx;
	}

	vk::render_device *get_current_renderer()
	{
		return &g_current_renderer;
	}

	void set_current_renderer(const vk::render_device &device)
	{
		g_current_renderer = device;
	}

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageSubresourceRange range)
	{
		//Prepare an image to match the new layout..
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.newLayout = new_layout;
		barrier.oldLayout = current_layout;
		barrier.image = image;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = range;

		switch (new_layout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; break;
		}

		switch (current_layout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; break;
		}

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void change_image_layout(VkCommandBuffer cmd, vk::image *image, VkImageLayout new_layout, VkImageSubresourceRange range)
	{
		if (image->current_layout == new_layout) return;

		change_image_layout(cmd, image->value, image->current_layout, new_layout, range);
		image->current_layout = new_layout;
	}

	void enter_uninterruptible()
	{
		g_cb_no_interrupt_flag = true;
	}

	void leave_uninterruptible()
	{
		g_cb_no_interrupt_flag = false;
	}

	bool is_uninterruptible()
	{
		return g_cb_no_interrupt_flag;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
											uint64_t srcObject, size_t location, int32_t msgCode,
											const char *pLayerPrefix, const char *pMsg, void *pUserData)
	{
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			LOG_ERROR(RSX, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			LOG_WARNING(RSX, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
		}
		else
		{
			return false;
		}

		//Let the app crash..
		return false;
	}

	VkBool32 BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
							uint64_t srcObject, size_t location, int32_t msgCode,
							const char *pLayerPrefix, const char *pMsg, void *pUserData)
	{
#ifdef _WIN32
		DebugBreak();
#endif

		return false;
	}
}
