#pragma once

#include "Emu/Memory/vm.h"
#include "surface_utils.h"
#include "../GCM.h"
#include "../rsx_utils.h"
#include "Utilities/span.h"
#include <list>

namespace
{
	template <typename T>
	gsl::span<T> as_const_span(gsl::span<const std::byte> unformated_span)
	{
		return{ reinterpret_cast<T*>(unformated_span.data()), unformated_span.size_bytes() / sizeof(T) };
	}
}

namespace rsx
{
	namespace utility
	{
		std::vector<u8> get_rtt_indexes(surface_target color_target);
		size_t get_aligned_pitch(surface_color_format format, u32 width);
		size_t get_packed_pitch(surface_color_format format, u32 width);
	}

	template<typename Traits>
	struct surface_store
	{
		constexpr u32 get_aa_factor_u(surface_antialiasing aa_mode)
		{
			return (aa_mode == surface_antialiasing::center_1_sample)? 1 : 2;
		}

		constexpr u32 get_aa_factor_v(surface_antialiasing aa_mode)
		{
			switch (aa_mode)
			{
			case surface_antialiasing::center_1_sample:
			case surface_antialiasing::diagonal_centered_2_samples:
				return 1;
			default:
				return 2;
			}
		}

	public:
		using surface_storage_type = typename Traits::surface_storage_type;
		using surface_type = typename Traits::surface_type;
		using command_list_type = typename Traits::command_list_type;
		using surface_overlap_info = surface_overlap_info_t<surface_type>;

	protected:
		std::unordered_map<u32, surface_storage_type> m_render_targets_storage = {};
		std::unordered_map<u32, surface_storage_type> m_depth_stencil_storage = {};

		rsx::address_range m_render_targets_memory_range;
		rsx::address_range m_depth_stencil_memory_range;

		bool m_invalidate_on_write = false;
		bool m_skip_write_updates = false;

	public:
		std::pair<u8, u8> m_bound_render_targets_config = {};
		std::array<std::pair<u32, surface_type>, 4> m_bound_render_targets = {};
		std::pair<u32, surface_type> m_bound_depth_stencil = {};
		u8 m_bound_buffers_count = 0;

		// List of sections derived from a section that has been split and invalidated
		std::vector<surface_type> orphaned_surfaces;

		std::list<surface_storage_type> invalidated_resources;
		u64 cache_tag = 1ull; // Use 1 as the start since 0 is default tag on new surfaces
		u64 write_tag = 1ull;

		surface_store() = default;
		~surface_store() = default;
		surface_store(const surface_store&) = delete;

	private:
		template <bool is_depth_surface>
		void split_surface_region(command_list_type cmd, u32 address, surface_type prev_surface, u16 width, u16 height, u8 bpp, rsx::surface_antialiasing aa)
		{
			auto insert_new_surface = [&](
				u32 new_address,
				deferred_clipped_region<surface_type>& region,
				std::unordered_map<u32, surface_storage_type>& data)
			{
				surface_storage_type sink;
				surface_type invalidated = 0;
				if (const auto found = data.find(new_address);
					found != data.end())
				{
					if (Traits::is_compatible_surface(Traits::get(found->second), region.source, region.width, region.height, 1))
					{
						if (found->second->last_use_tag >= prev_surface->last_use_tag)
						{
							// If memory in this block is newer, do not overwrite with stale data
							return;
						}

						// There is no need to erase due to the reinsertion below
						sink = std::move(found->second);
					}
					else
					{
						invalidated_resources.push_back(std::move(found->second));
						data.erase(new_address);

						auto &old = invalidated_resources.back();
						Traits::notify_surface_invalidated(old);

						if (Traits::surface_is_pitch_compatible(old, prev_surface->get_rsx_pitch()))
						{
							if (old->last_use_tag >= prev_surface->last_use_tag) [[unlikely]]
							{
								invalidated = Traits::get(old);
							}
						}
					}
				}

				Traits::clone_surface(cmd, sink, region.source, new_address, region);

				if (invalidated) [[unlikely]]
				{
					// Halfplement the merge by crude inheritance. Should recursively split the memory blocks instead.
					if (sink->old_contents.empty()) [[likely]]
					{
						sink->set_old_contents(invalidated);
					}
					else
					{
						const auto existing = sink->get_normalized_memory_area();
						const auto incoming = invalidated->get_normalized_memory_area();

						deferred_clipped_region<surface_type> region{};
						region.source = invalidated;
						region.target = Traits::get(sink);
						region.width = std::min(existing.x2, incoming.x2);
						region.height = std::min(existing.y2, incoming.y2);
						sink->set_old_contents_region(region, true);
					}
				}

				verify(HERE), region.target == Traits::get(sink);
				orphaned_surfaces.push_back(region.target);
				data[new_address] = std::move(sink);
			};

			// Define incoming region
			size2u old, _new;

			const auto prev_area = prev_surface->get_normalized_memory_area();
			const auto prev_bpp = prev_surface->get_bpp();
			old.width = prev_area.x2;
			old.height = prev_area.y2;

			_new.width = width * bpp * get_aa_factor_u(aa);
			_new.height = height * get_aa_factor_v(aa);

			if (old.width <= _new.width && old.height <= _new.height)
			{
				// No extra memory to be preserved
				return;
			}

			// One-time data validity test
			verify(HERE), prev_surface;
			if (prev_surface->read_barrier(cmd); !prev_surface->test())
			{
				return;
			}

			if (old.width > _new.width)
			{
				// Split in X
				const u32 baseaddr = address + _new.width;
				const u32 bytes_to_texels_x = (prev_bpp * prev_surface->samples_x);

				deferred_clipped_region<surface_type> copy;
				copy.src_x = _new.width / bytes_to_texels_x;
				copy.src_y = 0;
				copy.dst_x = 0;
				copy.dst_y = 0;
				copy.width = (old.width - _new.width) / bytes_to_texels_x;
				copy.height = prev_surface->get_surface_height();
				copy.transfer_scale_x = 1.f;
				copy.transfer_scale_y = 1.f;
				copy.target = nullptr;
				copy.source = prev_surface;

				if constexpr (is_depth_surface)
				{
					insert_new_surface(baseaddr, copy, m_depth_stencil_storage);
				}
				else
				{
					insert_new_surface(baseaddr, copy, m_render_targets_storage);
				}
			}

			if (old.height > _new.height)
			{
				// Split in Y
				const u32 baseaddr = address + (_new.height * prev_surface->get_rsx_pitch());
				const u32 bytes_to_texels_x = (prev_bpp * prev_surface->samples_x);

				deferred_clipped_region<surface_type> copy;
				copy.src_x = 0;
				copy.src_y = _new.height / prev_surface->samples_y;
				copy.dst_x = 0;
				copy.dst_y = 0;
				copy.width = std::min(_new.width, old.width) / bytes_to_texels_x;
				copy.height = (old.height - _new.height) / prev_surface->samples_y;
				copy.transfer_scale_x = 1.f;
				copy.transfer_scale_y = 1.f;
				copy.target = nullptr;
				copy.source = prev_surface;

				if constexpr (is_depth_surface)
				{
					insert_new_surface(baseaddr, copy, m_depth_stencil_storage);
				}
				else
				{
					insert_new_surface(baseaddr, copy, m_render_targets_storage);
				}
			}
		}

		template <bool is_depth_surface>
		void intersect_surface_region(command_list_type cmd, u32 address, surface_type new_surface, surface_type prev_surface)
		{
			auto scan_list = [&new_surface, address](const rsx::address_range& mem_range,
				std::unordered_map<u32, surface_storage_type>& data) -> std::vector<std::pair<u32, surface_type>>
			{
				std::vector<std::pair<u32, surface_type>> result;
				for (const auto &e : data)
				{
					auto surface = Traits::get(e.second);

					if (new_surface->last_use_tag >= surface->last_use_tag ||
						new_surface == surface ||
						address == e.first)
					{
						// Do not bother synchronizing with uninitialized data
						continue;
					}

					// Memory partition check
					if (mem_range.start >= constants::local_mem_base)
					{
						if (e.first < constants::local_mem_base) continue;
					}
					else
					{
						if (e.first >= constants::local_mem_base) continue;
					}

					// Pitch check
					if (!rsx::pitch_compatible(surface, new_surface))
					{
						continue;
					}

					// Range check
					const rsx::address_range this_range = surface->get_memory_range();
					if (!this_range.overlaps(mem_range))
					{
						continue;
					}

					result.push_back({ e.first, surface });
				}

				return result;
			};

			const rsx::address_range mem_range = new_surface->get_memory_range();
			auto list1 = scan_list(mem_range, m_render_targets_storage);
			auto list2 = scan_list(mem_range, m_depth_stencil_storage);

			if (prev_surface)
			{
				// Append the previous removed surface to the intersection list
				if constexpr (is_depth_surface)
				{
					list2.push_back({ address, prev_surface });
				}
				else
				{
					list1.push_back({ address, prev_surface });
				}
			}
			else
			{
				if (list1.empty() && list2.empty())
				{
					return;
				}
			}

			std::vector<std::pair<u32, surface_type>> surface_info;
			if (list1.empty())
			{
				surface_info = std::move(list2);
			}
			else if (list2.empty())
			{
				surface_info = std::move(list1);
			}
			else
			{
				const auto reserve = list1.size() + list2.size();
				surface_info = std::move(list1);
				surface_info.reserve(reserve);

				for (const auto& e : list2) surface_info.push_back(e);
			}

			// TODO: Modify deferred_clip_region::direct_copy() to take a few more things into account!
			const areau child_region = new_surface->get_normalized_memory_area();
			const auto child_w = child_region.width();
			const auto child_h = child_region.height();

			const auto pitch = new_surface->get_rsx_pitch();
			for (const auto &e: surface_info)
			{
				auto this_address = e.first;
				auto surface = e.second;

				if (surface->old_contents.size() == 1) [[unlikely]]
				{
					// Dirty zombies are possible with unused pixel storage subslices and are valid
					// Avoid double transfer if possible
					// This is an optional optimization that can be safely disabled
					surface = static_cast<decltype(surface)>(surface->old_contents[0].source);

					// Ignore self-reference
					if (new_surface == surface)
					{
						continue;
					}

					// If this surface has already been added via another descendant, just ignore it
					bool ignore = false;
					for (auto &slice : new_surface->old_contents)
					{
						if (slice.source == surface)
						{
							ignore = true;
							break;
						}
					}

					if (ignore) continue;

					this_address = surface->base_addr;
					verify(HERE), this_address;
				}

				const auto parent_region = surface->get_normalized_memory_area();
				const auto parent_w = parent_region.width();
				const auto parent_h = parent_region.height();
				const auto rect = rsx::intersect_region(this_address, parent_w, parent_h, 1, address, child_w, child_h, 1, pitch);

				const auto src_offset = std::get<0>(rect);
				const auto dst_offset = std::get<1>(rect);
				const auto size = std::get<2>(rect);

				if (src_offset.x >= parent_w || src_offset.y >= parent_h)
				{
					continue;
				}

				if (dst_offset.x >= child_w || dst_offset.y >= child_h)
				{
					continue;
				}

				// TODO: Eventually need to stack all the overlapping regions, but for now just do the latest rect in the space
				deferred_clipped_region<surface_type> region;
				region.src_x = src_offset.x;
				region.src_y = src_offset.y;
				region.dst_x = dst_offset.x;
				region.dst_y = dst_offset.y;
				region.width = size.width;
				region.height = size.height;
				region.source = surface;
				region.target = new_surface;

				new_surface->set_old_contents_region(region, true);

				if (surface->memory_usage_flags == surface_usage_flags::storage &&
					region.width == parent_w &&
					region.height == parent_h &&
					surface != prev_surface &&
					surface == e.second)
				{
					// This has been 'swallowed' by the new surface and can be safely freed
					auto &storage = surface->is_depth_surface() ? m_depth_stencil_storage : m_render_targets_storage;
					auto &object = storage[e.first];

					verify(HERE), !src_offset.x, !src_offset.y, object;
					if (!surface->old_contents.empty()) [[unlikely]]
					{
						surface->read_barrier(cmd);
					}

					Traits::notify_surface_invalidated(object);
					invalidated_resources.push_back(std::move(object));
					storage.erase(e.first);
				}
			}
		}

		template <bool depth, typename format_type, typename ...Args>
		surface_type bind_surface_address(
			command_list_type command_list,
			u32 address,
			format_type format,
			surface_antialiasing antialias,
			size_t width, size_t height, size_t pitch,
			u8 bpp,
			Args&&... extra_params)
		{
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			bool do_intersection_test = true;
			bool store = true;

			address_range *storage_bounds;
			std::unordered_map<u32, surface_storage_type> *primary_storage, *secondary_storage;
			if constexpr (depth)
			{
				primary_storage = &m_depth_stencil_storage;
				secondary_storage = &m_render_targets_storage;
				storage_bounds = &m_depth_stencil_memory_range;
			}
			else
			{
				primary_storage = &m_render_targets_storage;
				secondary_storage = &m_depth_stencil_storage;
				storage_bounds = &m_render_targets_memory_range;
			}

			// Check if render target already exists
			auto It = primary_storage->find(address);
			if (It != primary_storage->end())
			{
				surface_storage_type &surface = It->second;
				const bool pitch_compatible = Traits::surface_is_pitch_compatible(surface, pitch);

				if (Traits::surface_matches_properties(surface, format, width, height, antialias))
				{
					if (pitch_compatible)
						Traits::notify_surface_persist(surface);
					else
						Traits::invalidate_surface_contents(command_list, Traits::get(surface), address, pitch);

					Traits::prepare_surface_for_drawing(command_list, Traits::get(surface));
					new_surface = Traits::get(surface);
					store = false;
				}
				else
				{
					if (pitch_compatible)
					{
						// Preserve memory outside the area to be inherited if needed
						split_surface_region<depth>(command_list, address, Traits::get(surface), static_cast<u16>(width), static_cast<u16>(height), bpp, antialias);
						old_surface = Traits::get(surface);
					}

					// This will be unconditionally moved to invalidated list shortly
					Traits::notify_surface_invalidated(surface);
					old_surface_storage = std::move(surface);

					primary_storage->erase(It);
				}
			}

			if (!new_surface)
			{
				// Range test
				const auto aa_factor_v = get_aa_factor_v(antialias);
				rsx::address_range range = rsx::address_range::start_length(address, static_cast<u32>(pitch * height * aa_factor_v));
				*storage_bounds = range.get_min_max(*storage_bounds);

				// Search invalidated resources for a suitable surface
				for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
				{
					auto &surface = *It;
					if (Traits::surface_matches_properties(surface, format, width, height, antialias, true))
					{
						new_surface_storage = std::move(surface);
						Traits::notify_surface_reused(new_surface_storage);

						if (old_surface_storage)
						{
							// Exchange this surface with the invalidated one
							surface = std::move(old_surface_storage);
						}
						else
						{
							// Iterator is now empty - erase it
							invalidated_resources.erase(It);
						}

						new_surface = Traits::get(new_surface_storage);
						Traits::invalidate_surface_contents(command_list, new_surface, address, pitch);
						Traits::prepare_surface_for_drawing(command_list, new_surface);
						break;
					}
				}
			}

			// Check for stale storage
			if (old_surface_storage)
			{
				// This was already determined to be invalid and is excluded from testing above
				invalidated_resources.push_back(std::move(old_surface_storage));
			}

			if (!new_surface)
			{
				verify(HERE), store;
				new_surface_storage = Traits::create_new_surface(address, format, width, height, pitch, antialias, std::forward<Args>(extra_params)...);
				new_surface = Traits::get(new_surface_storage);
			}

			// Remove and preserve if possible any overlapping/replaced surface from the other pool
			auto aliased_surface = secondary_storage->find(address);
			if (aliased_surface != secondary_storage->end())
			{
				if (Traits::surface_is_pitch_compatible(aliased_surface->second, pitch))
				{
					auto surface = Traits::get(aliased_surface->second);
					split_surface_region<!depth>(command_list, address, surface, static_cast<u16>(width), static_cast<u16>(height), bpp, antialias);

					if (!old_surface || old_surface->last_use_tag < surface->last_use_tag)
					{
						// TODO: This can leak data outside inherited bounds
						old_surface = surface;
					}
				}

				Traits::notify_surface_invalidated(aliased_surface->second);
				invalidated_resources.push_back(std::move(aliased_surface->second));
				secondary_storage->erase(aliased_surface);
			}

			// Check if old_surface is 'new' and hopefully avoid intersection
			if (old_surface)
			{
				if (old_surface->last_use_tag < new_surface->last_use_tag)
				{
					// Can happen if aliasing occurs; a probable condition due to memory splitting
					// This is highly unlikely but is possible in theory
					old_surface = nullptr;
				}
				else if (old_surface->last_use_tag >= write_tag)
				{
					const auto new_area = new_surface->get_normalized_memory_area();
					const auto old_area = old_surface->get_normalized_memory_area();

					if (new_area.x2 <= old_area.x2 && new_area.y2 <= old_area.y2)
					{
						do_intersection_test = false;
						new_surface->set_old_contents(old_surface);
					}
				}
			}

			if (do_intersection_test)
			{
				intersect_surface_region<depth>(command_list, address, new_surface, old_surface);
			}

			if (store)
			{
				// New surface was found among invalidated surfaces or created from scratch
				(*primary_storage)[address] = std::move(new_surface_storage);
			}

			verify(HERE), !old_surface_storage, new_surface->get_spp() == get_format_sample_count(antialias);
			return new_surface;
		}

	protected:
		/**
		* If render target already exists at address, issue state change operation on cmdList.
		* Otherwise create one with width, height, clearColor info.
		* returns the corresponding render target resource.
		*/
		template <typename ...Args>
		surface_type bind_address_as_render_targets(
			command_list_type command_list,
			u32 address,
			surface_color_format color_format,
			surface_antialiasing antialias,
			size_t width, size_t height, size_t pitch,
			Args&&... extra_params)
		{
			return bind_surface_address<false>(
				command_list, address, color_format, antialias,
				width, height, pitch, get_format_block_size_in_bytes(color_format),
				std::forward<Args>(extra_params)...);
		}

		template <typename ...Args>
		surface_type bind_address_as_depth_stencil(
			command_list_type command_list,
			u32 address,
			surface_depth_format depth_format,
			surface_antialiasing antialias,
			size_t width, size_t height, size_t pitch,
			Args&&... extra_params)
		{
			return bind_surface_address<true>(
				command_list, address, depth_format, antialias,
				width, height, pitch,
				depth_format == rsx::surface_depth_format::z16? 2 : 4,
				std::forward<Args>(extra_params)...);
		}
	public:
		/**
		 * Update bound color and depth surface.
		 * Must be called everytime surface format, clip, or addresses changes.
		 */
		template <typename ...Args>
		void prepare_render_target(
			command_list_type command_list,
			surface_color_format color_format, surface_depth_format depth_format,
			u32 clip_horizontal_reg, u32 clip_vertical_reg,
			surface_target set_surface_target,
			surface_antialiasing antialias,
			const std::array<u32, 4> &surface_addresses, u32 address_z,
			const std::array<u32, 4> &surface_pitch, u32 zeta_pitch,
			Args&&... extra_params)
		{
			u32 clip_width = clip_horizontal_reg;
			u32 clip_height = clip_vertical_reg;

			cache_tag = rsx::get_shared_tag();
			m_invalidate_on_write = (antialias != rsx::surface_antialiasing::center_1_sample);
			m_bound_buffers_count = 0;

			// Make previous RTTs sampleable
			for (int i = m_bound_render_targets_config.first, count = 0;
				count < m_bound_render_targets_config.second;
				++i, ++count)
			{
				auto &rtt = m_bound_render_targets[i];
				Traits::prepare_surface_for_sampling(command_list, std::get<1>(rtt));
				rtt = std::make_pair(0, nullptr);
			}

			const auto rtt_indices = utility::get_rtt_indexes(set_surface_target);
			if (!rtt_indices.empty()) [[likely]]
			{
				m_bound_render_targets_config = { rtt_indices.front(), 0 };

				// Create/Reuse requested rtts
				for (u8 surface_index : rtt_indices)
				{
					if (surface_addresses[surface_index] == 0)
						continue;

					m_bound_render_targets[surface_index] = std::make_pair(surface_addresses[surface_index],
						bind_address_as_render_targets(command_list, surface_addresses[surface_index], color_format, antialias,
							clip_width, clip_height, surface_pitch[surface_index], std::forward<Args>(extra_params)...));

					++m_bound_render_targets_config.second;
					++m_bound_buffers_count;
				}
			}
			else
			{
				m_bound_render_targets_config = { 0, 0 };
			}

			// Same for depth buffer
			if (std::get<1>(m_bound_depth_stencil) != nullptr)
			{
				Traits::prepare_surface_for_sampling(command_list, std::get<1>(m_bound_depth_stencil));
			}

			if (address_z) [[likely]]
			{
				m_bound_depth_stencil = std::make_pair(address_z,
					bind_address_as_depth_stencil(command_list, address_z, depth_format, antialias,
						clip_width, clip_height, zeta_pitch, std::forward<Args>(extra_params)...));

				++m_bound_buffers_count;
			}
			else
			{
				m_bound_depth_stencil = std::make_pair(0, nullptr);
			}
		}

		u8 get_color_surface_count() const
		{
			return m_bound_render_targets_config.second;
		}

		surface_type get_surface_at(u32 address)
		{
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return Traits::get(It->second);

			auto _It = m_depth_stencil_storage.find(address);
			if (_It != m_depth_stencil_storage.end())
				return Traits::get(_It->second);

			fmt::throw_exception("Unreachable" HERE);
		}

		surface_type get_color_surface_at(u32 address)
		{
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return Traits::get(It->second);

			return nullptr;
		}

		surface_type get_depth_stencil_surface_at(u32 address)
		{
			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
				return Traits::get(It->second);

			return nullptr;
		}

		/**
		 * Invalidates surface that exists at an address
		 */
		void invalidate_surface_address(u32 addr, bool depth)
		{
			if (address_is_bound(addr))
			{
				rsx_log.error("Cannot invalidate a currently bound render target!");
				return;
			}

			if (!depth)
			{
				auto It = m_render_targets_storage.find(addr);
				if (It != m_render_targets_storage.end())
				{
					Traits::notify_surface_invalidated(It->second);
					invalidated_resources.push_back(std::move(It->second));
					m_render_targets_storage.erase(It);

					cache_tag = rsx::get_shared_tag();
					return;
				}
			}
			else
			{
				auto It = m_depth_stencil_storage.find(addr);
				if (It != m_depth_stencil_storage.end())
				{
					Traits::notify_surface_invalidated(It->second);
					invalidated_resources.push_back(std::move(It->second));
					m_depth_stencil_storage.erase(It);

					cache_tag = rsx::get_shared_tag();
					return;
				}
			}
		}

		bool address_is_bound(u32 address) const
		{
			for (int i = m_bound_render_targets_config.first, count = 0;
				count < m_bound_render_targets_config.second;
				++i, ++count)
			{
				if (m_bound_render_targets[i].first == address)
					return true;
			}

			return (m_bound_depth_stencil.first == address);
		}

		template <typename commandbuffer_type>
		std::vector<surface_overlap_info> get_merged_texture_memory_region(commandbuffer_type& cmd, u32 texaddr, u32 required_width, u32 required_height, u32 required_pitch, u8 required_bpp, rsx::surface_access access)
		{
			std::vector<surface_overlap_info> result;
			std::vector<std::pair<u32, bool>> dirty;

			const auto surface_internal_pitch = (required_width * required_bpp);

			// Sanity check
			if (surface_internal_pitch > required_pitch) [[unlikely]]
			{
				rsx_log.warning("Invalid 2D region descriptor. w=%d, h=%d, bpp=%d, pitch=%d",
							required_width, required_height, required_bpp, required_pitch);
				return {};
			}

			const auto test_range = utils::address_range::start_length(texaddr, (required_pitch * required_height) - (required_pitch - surface_internal_pitch));

			auto process_list_function = [&](std::unordered_map<u32, surface_storage_type>& data, bool is_depth)
			{
				for (auto& tex_info : data)
				{
					const auto range = tex_info.second->get_memory_range();
					if (!range.overlaps(test_range))
						continue;

					auto surface = tex_info.second.get();
					if (access == rsx::surface_access::transfer && surface->write_through())
						continue;

					if (!rsx::pitch_compatible(surface, required_pitch, required_height))
						continue;

					surface_overlap_info info;
					u32 width, height;
					info.surface = surface;
					info.base_address = range.start;
					info.is_depth = is_depth;

					const u32 normalized_surface_width = surface->get_surface_width(rsx::surface_metrics::bytes) / required_bpp;
					const u32 normalized_surface_height = surface->get_surface_height(rsx::surface_metrics::samples);

					if (range.start >= texaddr) [[likely]]
					{
						const auto offset = range.start - texaddr;
						info.dst_area.y = (offset / required_pitch);
						info.dst_area.x = (offset % required_pitch) / required_bpp;

						if (info.dst_area.x >= required_width || info.dst_area.y >= required_height) [[unlikely]]
						{
							// Out of bounds
							continue;
						}

						info.src_area.x = 0;
						info.src_area.y = 0;
						width = std::min<u32>(normalized_surface_width, required_width - info.dst_area.x);
						height = std::min<u32>(normalized_surface_height, required_height - info.dst_area.y);
					}
					else
					{
						const auto pitch = surface->get_rsx_pitch();
						const auto offset = texaddr - range.start;
						info.src_area.y = (offset / pitch);
						info.src_area.x = (offset % pitch) / required_bpp;

						if (info.src_area.x >= normalized_surface_width || info.src_area.y >= normalized_surface_height) [[unlikely]]
						{
							// Region lies outside the actual texture area, but inside the 'tile'
							// In this case, a small region lies to the top-left corner, partially occupying the  target
							continue;
						}

						info.dst_area.x = 0;
						info.dst_area.y = 0;
						width = std::min<u32>(required_width, normalized_surface_width - info.src_area.x);
						height = std::min<u32>(required_height, normalized_surface_height - info.src_area.y);
					}

					// Delay this as much as possible to avoid side-effects of spamming barrier
					if (surface->memory_barrier(cmd, access); !surface->test())
					{
						dirty.emplace_back(range.start, is_depth);
						continue;
					}

					info.is_clipped = (width < required_width || height < required_height);
					info.src_area.height = info.dst_area.height = height;
					info.dst_area.width = width;

					if (auto surface_bpp = surface->get_bpp(); surface_bpp != required_bpp) [[unlikely]]
					{
						// Width is calculated in the coordinate-space of the requester; normalize
						info.src_area.x = (info.src_area.x * required_bpp) / surface_bpp;
						info.src_area.width = align(width * required_bpp, surface_bpp) / surface_bpp;
					}
					else
					{
						info.src_area.width = width;
					}

					result.push_back(info);
				}
			};

			// Range test helper to quickly discard blocks
			// Fortunately, render targets tend to be clustered anyway
			if (test_range.overlaps(m_render_targets_memory_range))
			{
				process_list_function(m_render_targets_storage, false);
			}

			if (test_range.overlaps(m_depth_stencil_memory_range))
			{
				process_list_function(m_depth_stencil_storage, true);
			}

			if (!dirty.empty())
			{
				for (const auto& p : dirty)
				{
					invalidate_surface_address(p.first, p.second);
				}
			}

			if (result.size() > 1)
			{
				std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
				{
					if (a.surface->last_use_tag == b.surface->last_use_tag)
					{
						const auto area_a = a.dst_area.width * a.dst_area.height;
						const auto area_b = b.dst_area.width * b.dst_area.height;

						return area_a < area_b;
					}

					return a.surface->last_use_tag < b.surface->last_use_tag;
				});
			}

			return result;
		}

		void on_write(const bool* color, bool z)
		{
			if (write_tag == cache_tag && m_skip_write_updates)
			{
				// Nothing to do
				verify(HERE), !m_invalidate_on_write;
				return;
			}

			write_tag = cache_tag;
			m_skip_write_updates = false;
			int tagged = 0;

			// Tag surfaces
			if (color)
			{
				for (int i = m_bound_render_targets_config.first, count = 0;
					count < m_bound_render_targets_config.second;
					++i, ++count)
				{
					if (!color[i])
						continue;

					auto& surface = m_bound_render_targets[i].second;
					if (surface->last_use_tag != write_tag)
					{
						m_bound_render_targets[i].second->on_write(write_tag);
					}
					else if (m_invalidate_on_write)
					{
						m_bound_render_targets[i].second->on_invalidate_children();
					}

					++tagged;
				}
			}

			if (z && m_bound_depth_stencil.first)
			{
				auto& surface = m_bound_depth_stencil.second;
				if (surface->last_use_tag != write_tag)
				{
					m_bound_depth_stencil.second->on_write(write_tag);
				}
				else if (m_invalidate_on_write)
				{
					m_bound_depth_stencil.second->on_invalidate_children();
				}

				++tagged;
			}

			if (!m_invalidate_on_write && tagged == m_bound_buffers_count)
			{
				// Skip any further updates as all active surfaces have been updated
				m_skip_write_updates = true;
			}
		}

		void notify_memory_structure_changed()
		{
			cache_tag = rsx::get_shared_tag();
		}

		void invalidate_all()
		{
			// Unbind and invalidate all resources
			auto free_resource_list = [&](auto &data)
			{
				for (auto &e : data)
				{
					Traits::notify_surface_invalidated(e.second);
					invalidated_resources.push_back(std::move(e.second));
				}

				data.clear();
			};

			free_resource_list(m_render_targets_storage);
			free_resource_list(m_depth_stencil_storage);

			m_bound_depth_stencil = std::make_pair(0, nullptr);
			m_bound_render_targets_config = { 0, 0 };
			for (auto &rtt : m_bound_render_targets)
			{
				rtt = std::make_pair(0, nullptr);
			}
		}

		void invalidate_range(const rsx::address_range& range)
		{
			for (auto &rtt : m_render_targets_storage)
			{
				if (range.overlaps(rtt.second->get_memory_range()))
				{
					rtt.second->clear_rw_barrier();
					rtt.second->state_flags |= rsx::surface_state_flags::erase_bkgnd;
				}
			}

			for (auto &ds : m_depth_stencil_storage)
			{
				if (range.overlaps(ds.second->get_memory_range()))
				{
					ds.second->clear_rw_barrier();
					ds.second->state_flags |= rsx::surface_state_flags::erase_bkgnd;
				}
			}
		}
	};
}
