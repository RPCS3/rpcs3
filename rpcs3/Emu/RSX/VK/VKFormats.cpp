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

std::tuple<VkFilter, VkSamplerMipmapMode> get_min_filter_and_mip(rsx::texture_minify_filter min_filter)
{
	switch (min_filter)
	{
	case rsx::texture_minify_filter::nearest: return std::make_tuple(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture_minify_filter::linear: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture_minify_filter::nearest_nearest: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture_minify_filter::linear_nearest: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture_minify_filter::nearest_linear: return std::make_tuple(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	case rsx::texture_minify_filter::linear_linear: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	case rsx::texture_minify_filter::convolution_min: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	}
	throw EXCEPTION("Invalid max filter");
}

VkFilter get_mag_filter(rsx::texture_magnify_filter mag_filter)
{
	switch (mag_filter)
	{
	case rsx::texture_magnify_filter::nearest: return VK_FILTER_NEAREST;
	case rsx::texture_magnify_filter::linear: return VK_FILTER_LINEAR;
	case rsx::texture_magnify_filter::convolution_mag: return VK_FILTER_LINEAR;
	}
	throw EXCEPTION("Invalid mag filter (0x%x)", mag_filter);
}

VkBorderColor get_border_color(u8 color)
{
	if ((color / 0x10) >= 0x8) 
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	else
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		
	// TODO: VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
}

VkSamplerAddressMode vk_wrap_mode(rsx::texture_wrap_mode gcm_wrap)
{
	switch (gcm_wrap)
	{
	case rsx::texture_wrap_mode::wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case rsx::texture_wrap_mode::mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case rsx::texture_wrap_mode::clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case rsx::texture_wrap_mode::border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case rsx::texture_wrap_mode::clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case rsx::texture_wrap_mode::mirror_once_border: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case rsx::texture_wrap_mode::mirror_once_clamp: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
	throw EXCEPTION("unhandled texture clamp mode");
}

float max_aniso(rsx::texture_max_anisotropy gcm_aniso)
{
	switch (gcm_aniso)
	{
	case rsx::texture_max_anisotropy::x1: return 1.0f;
	case rsx::texture_max_anisotropy::x2: return 2.0f;
	case rsx::texture_max_anisotropy::x4: return 4.0f;
	case rsx::texture_max_anisotropy::x6: return 6.0f;
	case rsx::texture_max_anisotropy::x8: return 8.0f;
	case rsx::texture_max_anisotropy::x10: return 10.0f;
	case rsx::texture_max_anisotropy::x12: return 12.0f;
	case rsx::texture_max_anisotropy::x16: return 16.0f;
	}

	throw EXCEPTION("Texture anisotropy error: bad max aniso (%d).", gcm_aniso);
}


VkComponentMapping get_component_mapping(u32 format, u8 swizzle_mask)
{
	const u8 remap_a = swizzle_mask & 0x3;
	const u8 remap_r = (swizzle_mask >> 2) & 0x3;
	const u8 remap_g = (swizzle_mask >> 4) & 0x3;
	const u8 remap_b = (swizzle_mask >> 6) & 0x3;

	switch (format)
	{
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	case CELL_GCM_TEXTURE_B8:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

	case CELL_GCM_TEXTURE_G8B8:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };

	case CELL_GCM_TEXTURE_X16:
		return { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R };

	case CELL_GCM_TEXTURE_Y16_X16:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };

	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };
		
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R };
		
	case CELL_GCM_TEXTURE_A4R4G4B4:
	{
		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		return { map_table[remap_r], map_table[remap_g], map_table[remap_b], map_table[remap_a] };
	}
		
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };

	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO };


	case CELL_GCM_TEXTURE_A8R8G8B8:
	{
		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		return { map_table[remap_r], map_table[remap_g], map_table[remap_b], map_table[remap_a] };
	}
	}
	throw EXCEPTION("Invalid or unsupported component mapping for texture format (0x%x)", format);
}

}
