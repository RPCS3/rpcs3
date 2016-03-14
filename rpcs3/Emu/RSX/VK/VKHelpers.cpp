#include "stdafx.h"
#include "VKHelpers.h"

namespace vk
{
	context *g_current_vulkan_ctx = nullptr;
	render_device g_current_renderer;

	buffer g_null_buffer;
	texture g_null_texture;

	VkSampler g_null_sampler = nullptr;
	VkImageView g_null_image_view = nullptr;

	VKAPI_ATTR void *VKAPI_CALL mem_realloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
		return realloc(pOriginal, size);
	}

	VKAPI_ATTR void *VKAPI_CALL mem_alloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
	{
#ifdef _WIN32
		return _aligned_malloc(size, alignment);
#else
		return malloc(size);
#endif
	}

	VKAPI_ATTR void VKAPI_CALL mem_free(void *pUserData, void *pMemory)
	{
#ifdef _WIN32
		_aligned_free(pMemory);
#else
		free(pMemory);
#endif
	}

	memory_type_mapping get_memory_mapping(VkPhysicalDevice pdev)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(pdev, &memory_properties);

		memory_type_mapping result;
		result.device_local = VK_MAX_MEMORY_TYPES;
		result.host_visible_coherent = VK_MAX_MEMORY_TYPES;

		for (int i = 0; i < VK_MAX_MEMORY_TYPES; i++)
		{
			bool is_device_local = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (is_device_local)
				result.device_local = i;
			bool is_host_visible = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			bool is_host_coherent = !!(memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (is_host_coherent && is_host_visible)
				result.host_visible_coherent = i;
		}

		if (result.device_local == VK_MAX_MEMORY_TYPES) throw EXCEPTION("GPU doesn't support device local memory");
		if (result.host_visible_coherent == VK_MAX_MEMORY_TYPES) throw EXCEPTION("GPU doesn't support host coherent device local memory");
		return result;
	}

	VkFormat get_compatible_sampler_format(u32 format, VkComponentMapping& swizzle, u8 swizzle_mask)
	{
		u8 remap_a = swizzle_mask & 0x3;
		u8 remap_r = (swizzle_mask >> 2) & 0x3;
		u8 remap_g = (swizzle_mask >> 4) & 0x3;
		u8 remap_b = (swizzle_mask >> 6) & 0x3;

		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		
		VkComponentMapping remapped;
		remapped.a = map_table[remap_a];
		remapped.b = map_table[remap_b];
		remapped.g = map_table[remap_g];
		remapped.r = map_table[remap_r];

		swizzle = default_component_map();

		switch (format)
		{
		case CELL_GCM_TEXTURE_B8:
		{
			swizzle = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };
			return VK_FORMAT_R8_UNORM;
		}
		case CELL_GCM_TEXTURE_A1R5G5B5: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A4R4G4B4: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
		case CELL_GCM_TEXTURE_R5G6B5: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case CELL_GCM_TEXTURE_A8R8G8B8:
		{
			swizzle = remapped;
			return VK_FORMAT_B8G8R8A8_UNORM;
		}
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return VK_FORMAT_BC2_UNORM_BLOCK;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: 
		{
			return VK_FORMAT_BC3_UNORM_BLOCK;
		}
		case CELL_GCM_TEXTURE_G8B8:
		{
			swizzle = { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
			return VK_FORMAT_R8G8_UNORM;
		}
		case CELL_GCM_TEXTURE_R6G5B5: return VK_FORMAT_R5G6B5_UNORM_PACK16;					//Expand, discard high bit?
		case CELL_GCM_TEXTURE_DEPTH24_D8: return VK_FORMAT_R32_UINT;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:	return VK_FORMAT_R32_SFLOAT;
		case CELL_GCM_TEXTURE_DEPTH16: return VK_FORMAT_R16_UNORM;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return VK_FORMAT_R16_SFLOAT;
		case CELL_GCM_TEXTURE_X16: return VK_FORMAT_R16_UNORM;
		case CELL_GCM_TEXTURE_Y16_X16: return VK_FORMAT_R16G16_UNORM;
		case CELL_GCM_TEXTURE_R5G5B5A1: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case CELL_GCM_TEXTURE_X32_FLOAT: return VK_FORMAT_R32_SFLOAT;
		case CELL_GCM_TEXTURE_D1R5G5B5:
		{
			swizzle.a = VK_COMPONENT_SWIZZLE_ONE;
			return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		}
		case CELL_GCM_TEXTURE_D8R8G8B8:
		{
			swizzle = remapped;
			swizzle.a = VK_COMPONENT_SWIZZLE_ONE;
			return VK_FORMAT_B8G8R8A8_UNORM;
		}
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;	//Expand
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return VK_FORMAT_R8G8B8A8_UNORM;		//Expand
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			break;
		}
		throw EXCEPTION("Invalid or unsupported texture format (0x%x)", format);
	}

	VkAllocationCallbacks default_callbacks()
	{
		VkAllocationCallbacks callbacks;
		callbacks.pfnAllocation = vk::mem_alloc;
		callbacks.pfnFree = vk::mem_free;
		callbacks.pfnReallocation = vk::mem_realloc;

		return callbacks;
	}

	VkBuffer null_buffer()
	{
		if (g_null_buffer.size())
			return g_null_buffer;

		g_null_buffer.create(g_current_renderer, 32, VK_FORMAT_R32_SFLOAT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		return g_null_buffer;
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

	VkBufferView null_buffer_view()
	{
		if (g_null_buffer.size())
			return g_null_buffer;

		g_null_buffer.create(g_current_renderer, 32, VK_FORMAT_R32_SFLOAT, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		return g_null_buffer;
	}

	void destroy_global_resources()
	{
		g_null_buffer.destroy();
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

	void change_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_flags)
	{
		//Prepare an image to match the new layout..
		VkImageSubresourceRange range = default_image_subresource_range();
		range.aspectMask = aspect_flags;

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