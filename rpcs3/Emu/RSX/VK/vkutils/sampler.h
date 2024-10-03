#pragma once

#include "device.h"
#include "shared.h"

namespace vk
{
	struct border_color_t
	{
		u64 storage_key;
		VkBorderColor value;
		VkFormat format;
		VkImageCreateFlags aspect;
		color4f color_value;

		border_color_t(const color4f& color, VkFormat fmt = VK_FORMAT_UNDEFINED, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT);

		border_color_t(VkBorderColor color);

		bool operator == (const border_color_t& that) const
		{
			if (this->value != that.value)
			{
				return false;
			}

			switch (this->value)
			{
			case VK_BORDER_COLOR_FLOAT_CUSTOM_EXT:
			case VK_BORDER_COLOR_INT_CUSTOM_EXT:
				return this->color_value == that.color_value;
			default:
				return true;
			}
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
		border_color_t m_border_color;
	};

	// Caching helpers
	struct sampler_pool_key_t
	{
		u64 base_key;
		u64 border_color_key;
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
