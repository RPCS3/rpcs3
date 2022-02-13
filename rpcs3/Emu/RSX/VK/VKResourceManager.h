#pragma once
#include "vkutils/image.h"
#include "vkutils/query_pool.hpp"
#include "vkutils/sampler.h"

#include "Utilities/mutex.h"

#include <unordered_map>
#include <deque>
#include <memory>

namespace vk
{
	u64 get_event_id();
	u64 current_event_id();
	u64 last_completed_event_id();
	void on_event_completed(u64 event_id, bool flush = false);

	class disposable_t
	{
		void* ptr;
		std::function<void(void*)> deleter;

		disposable_t(void* ptr_, std::function<void(void*)> deleter_) :
			ptr(ptr_), deleter(deleter_) {}
	public:

		disposable_t() = delete;
		disposable_t(const disposable_t&) = delete;

		disposable_t(disposable_t&& other):
			ptr(std::exchange(other.ptr, nullptr)),
			deleter(other.deleter)
		{}

		~disposable_t()
		{
			if (ptr)
			{
				deleter(ptr);
				ptr = nullptr;
			}
		}

		template <typename T>
		static disposable_t make(T* raw)
		{
			return disposable_t(raw, [](void *raw)
			{
				delete static_cast<T*>(raw);
			});
		}
	};

	struct eid_scope_t
	{
		u64 eid;
		const vk::render_device* m_device;
		std::vector<disposable_t> m_disposables;

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
		}

		void discard()
		{
			m_disposables.clear();
		}
	};

	class resource_manager
	{
	private:
		struct cached_sampler_object_t : public vk::sampler, public rsx::ref_counted
		{
			using vk::sampler::sampler;
		};

		std::unordered_map<u64, std::unique_ptr<cached_sampler_object_t>> m_sampler_pool;
		std::deque<eid_scope_t> m_eid_map;
		shared_mutex m_eid_map_lock;

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

		vk::sampler* get_sampler(const vk::render_device& dev, vk::sampler* previous,
			VkSamplerAddressMode clamp_u, VkSamplerAddressMode clamp_v, VkSamplerAddressMode clamp_w,
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

			if (previous)
			{
				auto as_cached_object = static_cast<cached_sampler_object_t*>(previous);
				ensure(as_cached_object->has_refs());
				as_cached_object->release();
			}

			if (const auto found = m_sampler_pool.find(key);
				found != m_sampler_pool.end())
			{
				found->second->add_ref();
				return found->second.get();
			}

			auto result = std::make_unique<cached_sampler_object_t>(
				dev, clamp_u, clamp_v, clamp_w, unnormalized_coordinates,
				mipLodBias, max_anisotropy, min_lod, max_lod,
				min_filter, mag_filter, mipmap_mode, border_color,
				depth_compare, depth_compare_mode);

			auto It = m_sampler_pool.emplace(key, std::move(result));
			auto ret = It.first->second.get();
			ret->add_ref();
			return ret;
		}

		inline void dispose(vk::disposable_t& disposable)
		{
			get_current_eid_scope().m_disposables.emplace_back(std::move(disposable));
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
				auto& scope = m_eid_map.front();
				if (scope.eid > eid)
				{
					break;
				}
				else
				{
					eid_scope_t tmp(0);
					{
						std::lock_guard lock(m_eid_map_lock);
						m_eid_map.front().swap(tmp);
						m_eid_map.pop_front();
					}
				}
			}
		}

		void trim();
	};

	struct vmm_allocation_t
	{
		u64 size;
		u32 type_index;
		vmm_allocation_pool pool;
	};

	resource_manager* get_resource_manager();
}
