#pragma once
#include "VKHelpers.h"
#include <tuple>

namespace vk
{
	struct gpu_formats_support
	{
		bool d24_unorm_s8 : 1;
		bool d32_sfloat_s8 : 1;
	};

	gpu_formats_support get_optimal_tiling_supported_formats(VkPhysicalDevice physical_device);
	VkBorderColor get_border_color(u8 color);

	VkFormat get_compatible_depth_surface_format(const gpu_formats_support &support, rsx::surface_depth_format format);
	VkFormat get_compatible_sampler_format(u32 format);
	VkFormat get_compatible_srgb_format(VkFormat rgb_format);
	u8 get_format_texel_width(VkFormat format);
	std::pair<u8, u8> get_format_element_size(VkFormat format);

	std::tuple<VkFilter, VkSamplerMipmapMode> get_min_filter_and_mip(rsx::texture_minify_filter min_filter);
	VkFilter get_mag_filter(rsx::texture_magnify_filter mag_filter);
	VkSamplerAddressMode vk_wrap_mode(rsx::texture_wrap_mode gcm_wrap);
	float max_aniso(rsx::texture_max_anisotropy gcm_aniso);
	std::array<VkComponentSwizzle, 4> get_component_mapping(u32 format);
	VkPrimitiveTopology get_appropriate_topology(rsx::primitive_type& mode, bool &requires_modification);
}
