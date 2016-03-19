#include "stdafx.h"
#include "VKFormats.h"

namespace vk
{

gpu_formats_support get_optimal_tiling_supported_formats(VkPhysicalDevice physical_device)
{
	gpu_formats_support result = {};

	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &props);

	result.d24_unorm_s8 = !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		&& !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		&& !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
		&& !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
	result.d32_sfloat_s8 = !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		&& !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		&& !!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);

	return result;
}

VkFormat get_compatible_depth_surface_format(const gpu_formats_support &support, rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return VK_FORMAT_D16_UNORM;
	case rsx::surface_depth_format::z24s8:
	{
		if (support.d24_unorm_s8) return VK_FORMAT_D24_UNORM_S8_UINT;
		if (support.d32_sfloat_s8) return VK_FORMAT_D32_SFLOAT_S8_UINT;
		throw EXCEPTION("No hardware support for z24s8");
	}
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

VkSamplerAddressMode vk_wrap_mode(u32 gcm_wrap)
{
	switch (gcm_wrap)
	{
	case CELL_GCM_TEXTURE_WRAP: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case CELL_GCM_TEXTURE_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case CELL_GCM_TEXTURE_CLAMP: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	default:
		throw EXCEPTION("unhandled texture clamp mode 0x%X", gcm_wrap);
	}
}

float max_aniso(u32 gcm_aniso)
{
	switch (gcm_aniso)
	{
	case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12.0f;
	case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16.0f;
	}

	throw EXCEPTION("Texture anisotropy error: bad max aniso (%d).", gcm_aniso);
}

}