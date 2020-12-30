#pragma once

#include "shared.h"

namespace vk
{
	struct sampler
	{
		VkSampler value;
		VkSamplerCreateInfo info = {};

		sampler(VkDevice dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER);

		~sampler();

		bool matches(VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER);

		sampler(const sampler&) = delete;
		sampler(sampler&&) = delete;
	private:
		VkDevice m_device;
	};
}
