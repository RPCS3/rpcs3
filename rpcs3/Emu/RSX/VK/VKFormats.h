#pragma once
#include "VulkanAPI.h"
#include "../gcm_enums.h"

namespace vk
{
	class image;
	struct gpu_formats_support;

	struct minification_filter
	{
		VkFilter filter;
		VkSamplerMipmapMode mipmap_mode;
		bool sample_mipmaps;
	};

	VkBorderColor get_border_color(u32 color);

	VkFormat get_compatible_depth_surface_format(const gpu_formats_support& support, rsx::surface_depth_format2 format);
	VkFormat get_compatible_sampler_format(const gpu_formats_support& support, u32 format);
	VkFormat get_compatible_srgb_format(VkFormat rgb_format);
	u8 get_format_texel_width(VkFormat format);
	std::pair<u8, u8> get_format_element_size(VkFormat format);
	std::pair<bool, u32> get_format_convert_flags(VkFormat format);
	bool formats_are_bitcast_compatible(image* image1, image* image2);

	minification_filter get_min_filter(rsx::texture_minify_filter min_filter);
	VkFilter get_mag_filter(rsx::texture_magnify_filter mag_filter);
	VkSamplerAddressMode vk_wrap_mode(rsx::texture_wrap_mode gcm_wrap);
	float max_aniso(rsx::texture_max_anisotropy gcm_aniso);
	std::array<VkComponentSwizzle, 4> get_component_mapping(u32 format);
	std::pair<VkPrimitiveTopology, bool> get_appropriate_topology(rsx::primitive_type mode);
}
