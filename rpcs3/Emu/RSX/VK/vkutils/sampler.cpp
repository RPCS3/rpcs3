#include "memory.h"
#include "sampler.h"
#include "../../color_utils.h"
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

	border_color_t::border_color_t(const color4f& color, VkFormat fmt, VkImageAspectFlags aspect)
		: storage_key(0), format(fmt), aspect(aspect), color_value(color)
	{
		const auto encoded_color = rsx::encode_color_to_storage_key(color);
		value = vk::get_border_color(encoded_color);

		if (value != VK_BORDER_COLOR_FLOAT_CUSTOM_EXT)
		{
			// Nothing to do
			return;
		}

		if (!g_render_device->get_custom_border_color_support())
		{
			value = get_closest_border_color_enum(color_value);
			return;
		}

		if (aspect == VK_IMAGE_ASPECT_STENCIL_BIT)
		{
			// We must use INT color format for stencil
			value = VK_BORDER_COLOR_INT_CUSTOM_EXT;
			auto int_color = static_cast<s32>(color_value.z * 255.f);
			color_value = color4f(std::bit_cast<f32>(int_color));
		}

		ensure(aspect <= VK_IMAGE_ASPECT_METADATA_BIT);
		storage_key = static_cast<u64>(encoded_color)
			| (static_cast<u64>(aspect) << 32)
			| (static_cast<u64>(fmt) << 34);
	}

	border_color_t::border_color_t(VkBorderColor value)
		: storage_key(0)
		, value(value)
		, format(VK_FORMAT_UNDEFINED)
		, aspect(VK_IMAGE_ASPECT_COLOR_BIT)
		, color_value(0.f)
	{}

	sampler::sampler(const vk::render_device& dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
		VkBool32 depth_compare, VkCompareOp depth_compare_mode)
		: m_device(dev), m_border_color(border_color)
	{
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.addressModeU = clamp_u;
		info.addressModeV = clamp_v;
		info.addressModeW = clamp_w;
		info.anisotropyEnable = max_anisotropy >= 2. && dev.get_anisotropic_filtering_support();
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
		if (border_color.value >= VK_BORDER_COLOR_FLOAT_CUSTOM_EXT)
		{
			custom_color_info =
			{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
				.format = border_color.format
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
		    info.compareOp != depth_compare_mode || m_border_color != border_color)
			return false;

		return true;
	}

	sampler_pool_key_t sampler_pool_t::compute_storage_key(
		VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
		VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
		VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
		VkBool32 depth_compare, VkCompareOp depth_compare_mode)
	{
		sampler_pool_key_t key{};

		bool use_border_encoding = false;
		if (border_color.value > VK_BORDER_COLOR_INT_OPAQUE_WHITE)
		{
			// If there is no clamp to border in use, we can ignore the border color entirely
			if (clamp_u == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
				clamp_v == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
				clamp_w == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)
			{
				use_border_encoding = true;
			}
		}

		key.base_key = u16(clamp_u) | u64(clamp_v) << 3 | u64(clamp_w) << 6;
		key.base_key |= u64(unnormalized_coordinates) << 9;            // 1 bit
		key.base_key |= u64(min_filter) << 10 | u64(mag_filter) << 11; // 1 bit each
		key.base_key |= u64(mipmap_mode) << 12;   // 1 bit

		if (!use_border_encoding)
		{
			// Bits 13-16 are reserved for border color encoding
			key.base_key |= u64(border_color.value) << 13;
		}
		else
		{
			key.border_color_key = border_color.storage_key;
		}

		key.base_key |= u64(depth_compare) << 16; // 1 bit
		key.base_key |= u64(depth_compare_mode) << 17;  // 3 bits
		key.base_key |= u64(rsx::encode_fx12(min_lod)) << 20; // 12 bits
		key.base_key |= u64(rsx::encode_fx12(max_lod)) << 32; // 12 bits
		key.base_key |= u64(rsx::encode_fx12<true>(mipLodBias)) << 44; // 13 bits (fx12 + sign)
		key.base_key |= u64(max_anisotropy) << 57;                     // 4 bits

		return key;
	}

	void sampler_pool_t::clear()
	{
		m_generic_sampler_pool.clear();
		m_custom_color_sampler_pool.clear();
	}

	cached_sampler_object_t* sampler_pool_t::find(const sampler_pool_key_t& key) const
	{
		if (!key.border_color_key) [[ likely ]]
		{
			const auto found = m_generic_sampler_pool.find(key.base_key);
			return found == m_generic_sampler_pool.end() ? nullptr : found->second.get();
		}

		const auto block = m_custom_color_sampler_pool.equal_range(key.base_key);
		for (auto it = block.first; it != block.second; ++it)
		{
			if (it->second->key.border_color_key == key.border_color_key)
			{
				return it->second.get();
			}
		}

		return nullptr;
	}

	cached_sampler_object_t* sampler_pool_t::emplace(const sampler_pool_key_t& key, std::unique_ptr<cached_sampler_object_t>& object)
	{
		object->key = key;

		if (!key.border_color_key) [[ likely ]]
		{
			const auto [iterator, _unused] = m_generic_sampler_pool.emplace(key.base_key, std::move(object));
			return iterator->second.get();
		}

		const auto [iterator, _unused] = m_custom_color_sampler_pool.emplace(key.base_key, std::move(object));
		return iterator->second.get();
	}

	std::vector<std::unique_ptr<cached_sampler_object_t>> sampler_pool_t::collect(std::function<bool(const cached_sampler_object_t&)> predicate)
	{
		std::vector<std::unique_ptr<cached_sampler_object_t>> result;

		const auto collect_fn = [&](auto& container)
		{
			for (auto it = container.begin(); it != container.end();)
			{
				if (!predicate(*it->second))
				{
					it++;
					continue;
				}

				result.emplace_back(std::move(it->second));
				it = container.erase(it);
			}
		};

		collect_fn(m_generic_sampler_pool);
		collect_fn(m_custom_color_sampler_pool);

		return result;
	}
}
