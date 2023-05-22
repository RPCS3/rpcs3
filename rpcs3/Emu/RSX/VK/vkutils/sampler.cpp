#include "memory.h"
#include "sampler.h"
#include "../../rsx_utils.h"

namespace vk
{
	extern VkBorderColor get_border_color(u32);

	static VkBorderColor get_closest_border_color_enum(const color4f& color4)
	{
		if ((color4.r + color4.g + color4.b) > 1.35f)
		{
			//If color elements are brighter than roughly 0.5 average, use white border
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		}

		if (color4.a > 0.5f)
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}

	border_color_t::border_color_t(u32 encoded_color)
	{
		value = vk::get_border_color(encoded_color);

		if (value != VK_BORDER_COLOR_FLOAT_CUSTOM_EXT)
		{
			// Nothing to do
			return;
		}

		color_value = rsx::decode_border_color(encoded_color);
		if (!g_render_device->get_custom_border_color_support())
		{
			value = get_closest_border_color_enum(color_value);
		}
	}

	sampler::sampler(const vk::render_device& dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
		VkBool32 depth_compare, VkCompareOp depth_compare_mode)
		: m_device(dev)
	{
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.addressModeU = clamp_u;
		info.addressModeV = clamp_v;
		info.addressModeW = clamp_w;
		info.anisotropyEnable = dev.get_anisotropic_filtering_support();
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
		info.borderColor = border_color.value;

		VkSamplerCustomBorderColorCreateInfoEXT custom_color_info;
		if (border_color.value == VK_BORDER_COLOR_FLOAT_CUSTOM_EXT)
		{
			custom_color_info =
			{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
				.format = VK_FORMAT_UNDEFINED
			};

			std::memcpy(custom_color_info.customBorderColor.float32, border_color.color_value.rgba, sizeof(float) * 4);
			info.pNext = &custom_color_info;
		}

		CHECK_RESULT(vkCreateSampler(m_device, &info, nullptr, &value));
		vmm_notify_object_allocated(VMM_ALLOCATION_POOL_SAMPLER);
	}

	sampler::~sampler()
	{
		vkDestroySampler(m_device, value, nullptr);
		vmm_notify_object_freed(VMM_ALLOCATION_POOL_SAMPLER);
	}

	bool sampler::matches(VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
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
