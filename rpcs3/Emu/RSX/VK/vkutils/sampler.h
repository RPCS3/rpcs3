#pragma once

#include "device.h"
#include "shared.h"

namespace vk
{
	struct border_color_t
	{
		u32 storage_key;
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

	// Caching helpers
	struct sampler_pool_key_t
	{
		u64 base_key;
		u32 border_color_key;
	};

	struct cached_sampler_object_t : public vk::sampler, public rsx::ref_counted
	{
		sampler_pool_key_t key;
		using vk::sampler::sampler;
	};

	class sampler_pool_t
	{
		std::unordered_map<u64, std::unique_ptr<cached_sampler_object_t>> m_generic_sampler_pool;
		std::unordered_map<u64, std::unique_ptr<cached_sampler_object_t>> m_custom_color_sampler_pool;

	public:

		sampler_pool_key_t compute_storage_key(
			VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
			VkBool32 depth_compare, VkCompareOp depth_compare_mode);

		void clear();

		cached_sampler_object_t* find(const sampler_pool_key_t& key) const;

		cached_sampler_object_t* emplace(const sampler_pool_key_t& key, std::unique_ptr<cached_sampler_object_t>& object);

		std::vector<std::unique_ptr<cached_sampler_object_t>> collect(std::function<bool(const cached_sampler_object_t&)> predicate);
	};
}
