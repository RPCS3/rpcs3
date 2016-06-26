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
	VkFormat get_compatible_depth_surface_format(const gpu_formats_support &support, rsx::surface_depth_format format);
	VkStencilOp get_stencil_op(u32 op);
	VkLogicOp get_logic_op(u32 op);
	VkFrontFace get_front_face_ccw(u32 ffv);
	VkBorderColor get_border_color(u8 color);

	std::tuple<VkFilter, VkSamplerMipmapMode> get_min_filter_and_mip(rsx::texture_minify_filter min_filter);
	VkFilter get_mag_filter(rsx::texture_magnify_filter mag_filter);
	VkSamplerAddressMode vk_wrap_mode(rsx::texture_wrap_mode gcm_wrap);
	float max_aniso(rsx::texture_max_anisotropy gcm_aniso);
	VkComponentMapping get_component_mapping(u32 format, u8 swizzle_mask);
	VkPrimitiveTopology get_appropriate_topology(rsx::primitive_type& mode, bool &requires_modification);
}
