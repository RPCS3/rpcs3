#pragma once
#include "VKHelpers.h"

namespace vk
{
	u64 get_event_id();
	u64 current_event_id();
	void on_event_completed(u64 event_id);

	class resource_manager
	{
	private:
		std::unordered_multimap<u64, std::unique_ptr<vk::sampler>> m_sampler_pool;
		std::unordered_map<u64, std::vector<std::unique_ptr<vk::buffer>>> m_buffers_pool;
		std::unordered_map<u64, rsx::simple_array<VkEvent>> m_events_pool;

		bool value_compare(const f32& a, const f32& b)
		{
			return fabsf(a - b) < 0.0000001f;
		}

	public:

		resource_manager() = default;
		~resource_manager() = default;

		void destroy()
		{
			auto& dev = *vk::get_current_renderer();

			for (auto &e : m_events_pool)
			{
				for (auto &ev : e.second)
				{
					vkDestroyEvent(dev, ev, nullptr);
				}
			}

			m_sampler_pool.clear();
			m_buffers_pool.clear();
			m_events_pool.clear();
		}

		vk::sampler* find_sampler(VkDevice dev, VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, VkBorderColor border_color,
			VkBool32 depth_compare = VK_FALSE, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER)
		{
			u64 key = u16(clamp_u) | u64(clamp_v) << 3 | u64(clamp_w) << 6;
			key |= u64(unnormalized_coordinates) << 9; // 1 bit
			key |= u64(min_filter) << 10 | u64(mag_filter) << 11; // 1 bit each
			key |= u64(mipmap_mode) << 12; // 1 bit
			key |= u64(border_color) << 13; // 3 bits
			key |= u64(depth_compare) << 16; // 1 bit
			key |= u64(depth_compare_mode) << 17; // 3 bits

			const auto found = m_sampler_pool.equal_range(key);
			for (auto It = found.first; It != found.second; ++It)
			{
				const auto& info = It->second->info;
				if (!value_compare(info.mipLodBias, mipLodBias) ||
					!value_compare(info.maxAnisotropy, max_anisotropy) ||
					!value_compare(info.minLod, min_lod) ||
					!value_compare(info.maxLod, max_lod))
				{
					continue;
				}

				return It->second.get();
			}

			auto result = std::make_unique<vk::sampler>(
				dev, clamp_u, clamp_v, clamp_w, unnormalized_coordinates,
				mipLodBias, max_anisotropy, min_lod, max_lod,
				min_filter, mag_filter, mipmap_mode, border_color,
				depth_compare, depth_compare_mode);

			auto It = m_sampler_pool.emplace(key, std::move(result));
			return It->second.get();
		}

		void dispose(std::unique_ptr<vk::buffer>& buf)
		{
			m_buffers_pool[current_event_id()].emplace_back(std::move(buf));
		}

		void dispose(VkEvent& event)
		{
			m_events_pool[current_event_id()].push_back(event);
			event = VK_NULL_HANDLE;
		}

		void eid_completed(u64 eid)
		{
			auto& dev = *vk::get_current_renderer();

			for (auto It = m_buffers_pool.begin(); It != m_buffers_pool.end();)
			{
				if (It->first <= eid)
				{
					It = m_buffers_pool.erase(It);
				}
				else
				{
					++It;
				}
			}

			for (auto It = m_events_pool.begin(); It != m_events_pool.end();)
			{
				if (It->first <= eid)
				{
					for (auto &ev : It->second)
					{
						vkDestroyEvent(dev, ev, nullptr);
					}

					It = m_events_pool.erase(It);
				}
				else
				{
					++It;
				}
			}
		}
	};

	resource_manager* get_resource_manager();
}
