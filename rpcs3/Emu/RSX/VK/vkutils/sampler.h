#pragma once

#include "device.h"
#include "shared.h"

namespace vk
{
	struct border_color_t
	{
		VkBorderColor value;
		color4f color_value;

		border_color_t(u32 encoded_color);

		bool operator == (const border_color_t& that) const
		{
			if (this->value != that.value)
			{
				return false;
			}

			if (this->value != VK_BORDER_COLOR_FLOAT_CUSTOM_EXT)
			{
				return true;
			}

			return this->color_value == that.color_value;
		}
	};

	struct sampler
	{
		VkSampler value;
		VkSamplerCreateInfo info = {};

		sampler(const vk::render_device& dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const border_color_t& border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER);

		~sampler();

		bool matches(VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const border_color_t& border_color,
			VkBool32 depth_compare = false, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER);

		sampler(const sampler&) = delete;
		sampler(sampler&&) = delete;
	private:
		VkDevice m_device;
	};
}
