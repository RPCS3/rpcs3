#include "sampler.h"
#include "../../rsx_utils.h"

namespace vk
{
	sampler::sampler(VkDevice dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
		VkBool32 depth_compare, VkCompareOp depth_compare_mode)
		: m_device(dev)
	{
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.addressModeU = clamp_u;
		info.addressModeV = clamp_v;
		info.addressModeW = clamp_w;
		info.anisotropyEnable = VK_TRUE;
		info.compareEnable = depth_compare;
		info.unnormalizedCoordinates = unnormalized_coordinates;
		info.mipLodBias = mipLodBias;
		info.maxAnisotropy = max_anisotropy;
		info.maxLod = max_lod;
		info.minLod = min_lod;
		info.magFilter = mag_filter;
		info.minFilter = min_filter;
		info.mipmapMode = mipmap_mode;
		info.compareOp = depth_compare_mode;
		info.borderColor = border_color;

		CHECK_RESULT(vkCreateSampler(m_device, &info, nullptr, &value));
	}

	sampler::~sampler()
	{
		vkDestroySampler(m_device, value, nullptr);
	}

	bool sampler::matches(VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
		VkBool32 depth_compare, VkCompareOp depth_compare_mode)
	{
		if (info.magFilter != mag_filter || info.minFilter != min_filter || info.mipmapMode != mipmap_mode ||
		    info.addressModeU != clamp_u || info.addressModeV != clamp_v || info.addressModeW != clamp_w ||
		    info.compareEnable != depth_compare || info.unnormalizedCoordinates != unnormalized_coordinates ||
		    !rsx::fcmp(info.maxLod, max_lod) || !rsx::fcmp(info.mipLodBias, mipLodBias) || !rsx::fcmp(info.minLod, min_lod) ||
		    !rsx::fcmp(info.maxAnisotropy, max_anisotropy) ||
		    info.compareOp != depth_compare_mode || info.borderColor != border_color)
			return false;

		return true;
	}
}
