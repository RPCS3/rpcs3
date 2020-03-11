#pragma once
#include "VKHelpers.h"

namespace vk
{
	u64 get_event_id();
	u64 current_event_id();
	void on_event_completed(u64 event_id);

	struct eid_scope_t
	{
		u64 eid;
		const vk::render_device* m_device;
		std::vector<std::unique_ptr<vk::buffer>> m_disposed_buffers;
		std::vector<std::unique_ptr<vk::image_view>> m_disposed_image_views;
		std::vector<std::unique_ptr<vk::image>> m_disposed_images;
		std::vector<std::unique_ptr<vk::event>> m_disposed_events;

		eid_scope_t(u64 _eid):
			eid(_eid), m_device(vk::get_current_renderer())
		{}

		~eid_scope_t()
		{
			discard();
		}

		void discard()
		{
			m_disposed_buffers.clear();
			m_disposed_events.clear();
			m_disposed_image_views.clear();
			m_disposed_images.clear();
		}
	};

	class resource_manager
	{
	private:
		std::unordered_map<u64, std::unique_ptr<vk::sampler>> m_sampler_pool;
		std::deque<eid_scope_t> m_eid_map;

		eid_scope_t& get_current_eid_scope()
		{
			const auto eid = current_event_id();
			if (!m_eid_map.empty())
			{
				// Elements are insterted in order, so just check the last entry for a match
				if (auto &old = m_eid_map.back(); old.eid == eid)
				{
					return old;
				}
			}

			m_eid_map.emplace_back(eid);
			return m_eid_map.back();
		}

		template<bool _signed = false>
		u16 encode_fxp(f32 value)
		{
			u16 raw = u16(std::abs(value) * 256.);

			if constexpr (!_signed)
			{
				return raw;
			}
			else
			{
				if (value >= 0.f) [[likely]]
				{
					return raw;
				}
				else
				{
					return u16(0 - raw) & 0x1fff;
				}
			}
		}

	public:

		resource_manager() = default;
		~resource_manager() = default;

		void destroy()
		{
			m_eid_map.clear();
			m_sampler_pool.clear();
		}

		vk::sampler* find_sampler(VkDevice dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = VK_FALSE, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER)
		{
			u64 key = u16(clamp_u) | u64(clamp_v) << 3 | u64(clamp_w) << 6;
			key |= u64(unnormalized_coordinates) << 9;            // 1 bit
			key |= u64(min_filter) << 10 | u64(mag_filter) << 11; // 1 bit each
			key |= u64(mipmap_mode) << 12;   // 1 bit
			key |= u64(border_color) << 13;  // 3 bits
			key |= u64(depth_compare) << 16; // 1 bit
			key |= u64(depth_compare_mode) << 17;  // 3 bits
			key |= u64(encode_fxp(min_lod)) << 20; // 12 bits
			key |= u64(encode_fxp(max_lod)) << 32; // 12 bits
			key |= u64(encode_fxp<true>(mipLodBias)) << 44; // 13 bits
			key |= u64(max_anisotropy) << 57;               // 4 bits

			if (const auto found = m_sampler_pool.find(key);
				found != m_sampler_pool.end())
			{
				return found->second.get();
			}

			auto result = std::make_unique<vk::sampler>(
				dev, clamp_u, clamp_v, clamp_w, unnormalized_coordinates,
				mipLodBias, max_anisotropy, min_lod, max_lod,
				min_filter, mag_filter, mipmap_mode, border_color,
				depth_compare, depth_compare_mode);

			auto It = m_sampler_pool.emplace(key, std::move(result));
			return It.first->second.get();
		}

		void dispose(std::unique_ptr<vk::buffer>& buf)
		{
			get_current_eid_scope().m_disposed_buffers.emplace_back(std::move(buf));
		}

		void dispose(std::unique_ptr<vk::image_view>& view)
		{
			get_current_eid_scope().m_disposed_image_views.emplace_back(std::move(view));
		}

		void dispose(std::unique_ptr<vk::image>& img)
		{
			get_current_eid_scope().m_disposed_images.emplace_back(std::move(img));
		}

		void dispose(std::unique_ptr<vk::viewable_image>& img)
		{
			get_current_eid_scope().m_disposed_images.emplace_back(std::move(img));
		}

		void dispose(std::unique_ptr<vk::event>& event)
		{
			get_current_eid_scope().m_disposed_events.emplace_back(std::move(event));
			event = VK_NULL_HANDLE;
		}

		void eid_completed(u64 eid)
		{
			while (!m_eid_map.empty())
			{
				auto& scope = m_eid_map.front();
				if (scope.eid > eid)
				{
					break;
				}
				else
				{
					m_eid_map.pop_front();
				}
			}
		}
	};

	struct vmm_allocation_t
	{
		u64 size;
		u32 type_index;
	};

	resource_manager* get_resource_manager();
}
