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

}