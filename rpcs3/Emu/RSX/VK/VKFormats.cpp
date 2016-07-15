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

std::tuple<VkFilter, VkSamplerMipmapMode> get_min_filter_and_mip(rsx::texture::minify_filter min_filter)
{
	switch (min_filter)
	{
	case rsx::texture::minify_filter::nearest: return std::make_tuple(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture::minify_filter::linear: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture::minify_filter::nearest_nearest: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture::minify_filter::linear_nearest: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST);
	case rsx::texture::minify_filter::nearest_linear: return std::make_tuple(VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	case rsx::texture::minify_filter::linear_linear: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	case rsx::texture::minify_filter::convolution_min: return std::make_tuple(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
	}
	throw EXCEPTION("Invalid max filter");
}

VkFilter get_mag_filter(rsx::texture::magnify_filter mag_filter)
{
	switch (mag_filter)
	{
	case rsx::texture::magnify_filter::nearest: return VK_FILTER_NEAREST;
	case rsx::texture::magnify_filter::linear: return VK_FILTER_LINEAR;
	case rsx::texture::magnify_filter::convolution_mag: return VK_FILTER_LINEAR;
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

VkSamplerAddressMode vk_wrap_mode(rsx::texture::wrap_mode gcm_wrap)
{
	switch (gcm_wrap)
	{
	case rsx::texture::wrap_mode::wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case rsx::texture::wrap_mode::mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case rsx::texture::wrap_mode::clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case rsx::texture::wrap_mode::border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case rsx::texture::wrap_mode::clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case rsx::texture::wrap_mode::mirror_once_clamp_to_edge: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case rsx::texture::wrap_mode::mirror_once_border: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	case rsx::texture::wrap_mode::mirror_once_clamp: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
	throw EXCEPTION("unhandled texture clamp mode");
}

float max_aniso(rsx::texture::max_anisotropy gcm_aniso)
{
	switch (gcm_aniso)
	{
	case rsx::texture::max_anisotropy::x1: return 1.0f;
	case rsx::texture::max_anisotropy::x2: return 2.0f;
	case rsx::texture::max_anisotropy::x4: return 4.0f;
	case rsx::texture::max_anisotropy::x6: return 6.0f;
	case rsx::texture::max_anisotropy::x8: return 8.0f;
	case rsx::texture::max_anisotropy::x10: return 10.0f;
	case rsx::texture::max_anisotropy::x12: return 12.0f;
	case rsx::texture::max_anisotropy::x16: return 16.0f;
	}

	throw EXCEPTION("Texture anisotropy error: bad max aniso (%d).", gcm_aniso);
}


VkComponentMapping get_component_mapping(rsx::texture::format format, rsx::texture::component_remap remap0, rsx::texture::component_remap remap1, rsx::texture::component_remap remap2, rsx::texture::component_remap remap3)
{
	auto remap_lambda = [](rsx::texture::component_remap op) {
		switch (op)
		{
		case rsx::texture::component_remap::A: return 0;
		case rsx::texture::component_remap::R: return 1;
		case rsx::texture::component_remap::G: return 2;
		case rsx::texture::component_remap::B: return 3;
		}
		throw;
	};

	switch (format)
	{
	case rsx::texture::format::a1r5g5b5:
	case rsx::texture::format::r5g5b5a1:
	case rsx::texture::format::r6g5b5:
	case rsx::texture::format::r5g6b5:
	case rsx::texture::format::d24_8:
	case rsx::texture::format::d24_8_float:
	case rsx::texture::format::d16:
	case rsx::texture::format::d16_float:
	case rsx::texture::format::w32z32y32x32_float:
	case rsx::texture::format::compressed_dxt1:
	case rsx::texture::format::compressed_dxt23:
	case rsx::texture::format::compressed_dxt45:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	case rsx::texture::format::b8:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

	case rsx::texture::format::g8b8:
	{
		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };
		return{ map_table[remap_lambda(remap3)], map_table[remap_lambda(remap2)], map_table[remap_lambda(remap1)], map_table[remap_lambda(remap0)] };
	}

	case rsx::texture::format::x16:
		return { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R };

	case rsx::texture::format::y16x16:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

	case rsx::texture::format::y16x16_float:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };

	case rsx::texture::format::w16z16y16x16_float:
		return { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };
		
	case rsx::texture::format::x32float:
		return { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R };
		
	case rsx::texture::format::a4r4g4b4:
	{
		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		return { map_table[remap_lambda(remap3)], map_table[remap_lambda(remap2)], map_table[remap_lambda(remap1)], map_table[remap_lambda(remap0)] };
	}
		
	case rsx::texture::format::d8r8g8b8:
	case rsx::texture::format::d1r5g5b5:
		return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };

	case rsx::texture::format::compressed_hilo_8:
	case rsx::texture::format::compressed_hilo_s8:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

	case rsx::texture::format::compressed_b8r8_g8r8:
	case rsx::texture::format::compressed_r8b8_r8g8:
		return { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO };


	case rsx::texture::format::a8r8g8b8:
	{
		VkComponentSwizzle map_table[] = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
		return { map_table[remap_lambda(remap3)], map_table[remap_lambda(remap2)], map_table[remap_lambda(remap1)], map_table[remap_lambda(remap0)] };
	}
	}
	throw EXCEPTION("Invalid or unsupported component mapping for texture format (0x%x)", format);
}

}
