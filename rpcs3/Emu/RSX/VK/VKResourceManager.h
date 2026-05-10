#pragma once
#include "Emu/RSX/VK/vkutils/sync.h"
#include "vkutils/garbage_collector.h"
#include "vkutils/query_pool.hpp"
#include "vkutils/sampler.h"

#include "Utilities/mutex.h"

#include <deque>
#include <memory>

namespace vk
{
	u64 get_event_id();
	u64 current_event_id();
	u64 last_completed_event_id();
	void on_event_completed(u64 event_id, bool flush = false);

	struct eid_scope_t
	{
		u64 eid;
		const vk::render_device* m_device;
		std::vector<disposable_t> m_disposables;
		std::vector<std::unique_ptr<gpu_debug_marker>> m_debug_markers;

		eid_scope_t(u64 _eid):
			eid(_eid), m_device(g_render_device)
		{}

		~eid_scope_t()
		{
			discard();
		}

		void swap(eid_scope_t& other)
		{
			std::swap(eid, other.eid);
			std::swap(m_device, other.m_device);
			std::swap(m_disposables, other.m_disposables);
			std::swap(m_debug_markers, other.m_debug_markers);
		}

		void discard()
		{
			m_disposables.clear();
			m_debug_markers.clear();
		}
	};

	class resource_manager : public garbage_collector
	{
	private:
		sampler_pool_t m_sampler_pool;

		std::deque<eid_scope_t> m_eid_map;
		shared_mutex m_eid_map_lock;

		std::vector<std::function<void()>> m_exit_handlers;

		inline eid_scope_t& get_current_eid_scope()
		{
			const auto eid = current_event_id();
			{
				std::lock_guard lock(m_eid_map_lock);
				if (m_eid_map.empty() || m_eid_map.back().eid != eid)
				{
					m_eid_map.emplace_back(eid);
				}
			}
			return m_eid_map.back();
		}

	public:

		resource_manager() = default;
		~resource_manager() = default;

		void destroy()
		{
			flush();

			// Run the on-exit callbacks
			for (const auto& callback : m_exit_handlers)
			{
				callback();
			}
		}

		void flush()
		{
			m_eid_map.clear();
			m_sampler_pool.clear();
		}

		vk::sampler* get_sampler(const vk::render_device& dev, vk::sampler* previous,
			VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
			VkBool32 unnormalized_coordinates, float mipLodBias, float max_anisotropy, float min_lod, float max_lod,
			VkFilter min_filter, VkFilter mag_filter, VkSamplerMipmapMode mipmap_mode, const vk::border_color_t& border_color,
			VkBool32 depth_compare = VK_FALSE, VkCompareOp depth_compare_mode = VK_COMPARE_OP_NEVER)
		{
			const auto key = m_sampler_pool.compute_storage_key(
				clamp_u, clamp_v, clamp_w,
				unnormalized_coordinates, mipLodBias, max_anisotropy, min_lod, max_lod,
				min_filter, mag_filter, mipmap_mode, border_color,
				depth_compare, depth_compare_mode);

			if (previous)
			{
				auto as_cached_object = static_cast<cached_sampler_object_t*>(previous);
				ensure(as_cached_object->has_refs());
				as_cached_object->release();
			}

			if (const auto found = m_sampler_pool.find(key))
			{
				found->add_ref();
				return found;
			}

			auto result = std::make_unique<cached_sampler_object_t>(
				dev, clamp_u, clamp_v, clamp_w, unnormalized_coordinates,
				mipLodBias, max_anisotropy, min_lod, max_lod,
				min_filter, mag_filter, mipmap_mode, border_color,
				depth_compare, depth_compare_mode);

			auto ret = m_sampler_pool.emplace(key, result);
			ret->add_ref();
			return ret;
		}

		void add_exit_callback(std::function<void()> callback) override
		{
			m_exit_handlers.push_back(callback);
		}

		void dispose(vk::disposable_t& disposable) override
		{
			get_current_eid_scope().m_disposables.emplace_back(std::move(disposable));
		}

		inline void dispose(std::unique_ptr<vk::gpu_debug_marker>& object)
		{
			// Special case as we may need to read these out.
			// FIXME: We can manage these markers better and remove this exception.
			get_current_eid_scope().m_debug_markers.emplace_back(std::move(object));
		}

		template<typename T>
		inline void dispose(std::unique_ptr<T>& object)
		{
			auto ptr = vk::disposable_t::make(object.release());
			dispose(ptr);
		}

		void push_down_current_scope()
		{
			get_current_eid_scope().eid++;
		}

		void eid_completed(u64 eid)
		{
			while (!m_eid_map.empty())
			{
				const auto& scope = m_eid_map.front();
				if (scope.eid > eid)
				{
					break;
				}

				eid_scope_t tmp(0);
				{
					std::lock_guard lock(m_eid_map_lock);
					m_eid_map.front().swap(tmp);
					m_eid_map.pop_front();
				}
			}
		}

		void trim();

		std::vector<const gpu_debug_marker*> gather_debug_markers() const
		{
			std::vector<const gpu_debug_marker*> result;
			for (const auto& scope : m_eid_map)
			{
				for (const auto& item : scope.m_debug_markers)
				{
					result.push_back(item.get());
				}
			}
			return result;
		}
	};

	struct vmm_allocation_t
	{
		u64 size;
		u32 type_index;
		vmm_allocation_pool pool;
	};

	resource_manager* get_resource_manager();
}
