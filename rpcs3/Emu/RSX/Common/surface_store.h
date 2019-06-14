﻿#pragma once

#include "Utilities/GSL.h"
#include "Emu/Memory/vm.h"
#include "surface_utils.h"
#include "../GCM.h"
#include "../rsx_utils.h"
#include <list>

namespace
{
	template <typename T>
	gsl::span<T> as_const_span(gsl::span<const gsl::byte> unformated_span)
	{
		return{ (T*)unformated_span.data(), ::narrow<int>(unformated_span.size_bytes() / sizeof(T)) };
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

	public:
		std::pair<u8, u8> m_bound_render_targets_config = {};
		std::array<std::pair<u32, surface_type>, 4> m_bound_render_targets = {};
		std::pair<u32, surface_type> m_bound_depth_stencil = {};

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
				verify(HERE), prev_surface;
				if (prev_surface->read_barrier(cmd); !prev_surface->test())
				{
					return;
				}

				surface_storage_type sink;
				if (const auto found = data.find(new_address);
					found != data.end())
				{
					if (Traits::is_compatible_surface(Traits::get(found->second), region.source, region.width, region.height, 1))
					{
						// There is no need to erase due to the reinsertion below
						sink = std::move(found->second);
					}
					else
					{
						// TODO: Merge the 2 regions
						invalidated_resources.push_back(std::move(found->second));
						data.erase(new_address);

						auto &old = invalidated_resources.back();
						Traits::notify_surface_invalidated(old);
					}
				}

				Traits::clone_surface(cmd, sink, region.source, new_address, region);

				verify(HERE), region.target == Traits::get(sink);
				orphaned_surfaces.push_back(region.target);
				data[new_address] = std::move(sink);
			};

			// Define incoming region
			size2u old, _new;

			const auto prev_area = prev_surface->get_normalized_memory_area();
			old.width = prev_area.x2;
			old.height = prev_area.y2;

			_new.width = width * bpp * get_aa_factor_u(aa);
			_new.height = height * get_aa_factor_v(aa);

			if (old.width > _new.width)
			{
				// Split in X
				const u32 baseaddr = address + _new.width;
				const u32 bytes_to_texels_x = (bpp * prev_surface->samples_x);

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
				const u32 bytes_to_texels_x = (bpp * prev_surface->samples_x);

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
			auto scan_list = [&new_surface, address](const rsx::address_range& mem_range, u64 timestamp_check,
				std::unordered_map<u32, surface_storage_type>& data) -> std::vector<std::pair<u32, surface_type>>
			{
				std::vector<std::pair<u32, surface_type>> result;
				for (const auto &e : data)
				{
					auto surface = Traits::get(e.second);

					if (e.second->last_use_tag <= timestamp_check ||
						new_surface == surface ||
						address == e.first ||
						e.second->dirty())
					{
						// Do not bother synchronizing with uninitialized data
						continue;
					}

					// Memory partition check
					if (mem_range.start >= 0xc0000000)
					{
						if (e.first < 0xc0000000) continue;
					}
					else
					{
						if (e.first >= 0xc0000000) continue;
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
			const u64 timestamp_check = prev_surface ? prev_surface->last_use_tag : new_surface->last_use_tag;

			auto list1 = scan_list(mem_range, timestamp_check, m_render_targets_storage);
			auto list2 = scan_list(mem_range, timestamp_check, m_depth_stencil_storage);

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
				surface_info = std::move(list1);
				surface_info.reserve(list1.size() + list2.size());

				for (const auto& e : list2) surface_info.push_back(e);
			}

			if (UNLIKELY(surface_info.size() > 1))
			{
				// Sort with newest first for early exit
				std::sort(surface_info.begin(), surface_info.end(), [](const auto& a, const auto& b)
				{
					return (a.second->last_use_tag > b.second->last_use_tag);
				});
			}

			// TODO: Modify deferred_clip_region::direct_copy() to take a few more things into account!
			const areau child_region = new_surface->get_normalized_memory_area();
			const auto child_w = child_region.width();
			const auto child_h = child_region.height();

			const auto pitch = new_surface->get_rsx_pitch();
			for (const auto &e: surface_info)
			{
				const auto parent_region = e.second->get_normalized_memory_area();
				const auto parent_w = parent_region.width();
				const auto parent_h = parent_region.height();
				const auto rect = rsx::intersect_region(e.first, parent_w, parent_h, 1, address, child_w, child_h, 1, pitch);

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
				region.source = e.second;
				region.target = new_surface;

				new_surface->set_old_contents_region(region, true);
				break;
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

				if (pitch_compatible)
				{
					// Preserve memory outside the area to be inherited if needed
					split_surface_region<depth>(command_list, address, Traits::get(surface), (u16)width, (u16)height, bpp, antialias);
				}

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
					old_surface = Traits::get(surface);
					old_surface_storage = std::move(surface);
					primary_storage->erase(It);
				}
			}

			if (!new_surface)
			{
				// Range test
				const auto aa_factor_v = get_aa_factor_v(antialias);
				rsx::address_range range = rsx::address_range::start_length(address, u32(pitch * height * aa_factor_v));
				*storage_bounds = range.get_min_max(*storage_bounds);

				// Search invalidated resources for a suitable surface
				for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
				{
					auto &surface = *It;
					if (Traits::surface_matches_properties(surface, format, width, height, antialias, true))
					{
						new_surface_storage = std::move(surface);
						Traits::notify_surface_reused(new_surface_storage);

						if (old_surface)
						{
							// Exchange this surface with the invalidated one
							Traits::notify_surface_invalidated(old_surface_storage);
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
			if (old_surface != nullptr && new_surface == nullptr)
			{
				// This was already determined to be invalid and is excluded from testing above
				Traits::notify_surface_invalidated(old_surface_storage);
				invalidated_resources.push_back(std::move(old_surface_storage));
			}

			if (!new_surface)
			{
				verify(HERE), store;
				new_surface_storage = Traits::create_new_surface(address, format, width, height, pitch, antialias, std::forward<Args>(extra_params)...);
				new_surface = Traits::get(new_surface_storage);
			}

			if (!old_surface)
			{
				// Remove and preserve if possible any overlapping/replaced surface from the other pool
				auto aliased_surface = secondary_storage->find(address);
				if (aliased_surface != secondary_storage->end())
				{
					old_surface = Traits::get(aliased_surface->second);

					Traits::notify_surface_invalidated(aliased_surface->second);
					invalidated_resources.push_back(std::move(aliased_surface->second));
					secondary_storage->erase(aliased_surface);
				}
			}

			// Check if old_surface is 'new' and avoid intersection
			if (old_surface && old_surface->last_use_tag >= write_tag)
			{
				new_surface->set_old_contents(old_surface);
			}
			else
			{
				intersect_surface_region<depth>(command_list, address, new_surface, old_surface);
			}

			if (store)
			{
				// New surface was found among invalidated surfaces or created from scratch
				(*primary_storage)[address] = std::move(new_surface_storage);
			}

			verify(HERE), new_surface->get_spp() == get_format_sample_count(antialias);
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
			if (LIKELY(!rtt_indices.empty()))
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

					m_bound_render_targets_config.second++;
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

			if (LIKELY(address_z))
			{
				m_bound_depth_stencil = std::make_pair(address_z,
					bind_address_as_depth_stencil(command_list, address_z, depth_format, antialias,
						clip_width, clip_height, zeta_pitch, std::forward<Args>(extra_params)...));
			}
			else
			{
				m_bound_depth_stencil = std::make_pair(0, nullptr);
			}
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
				LOG_ERROR(RSX, "Cannot invalidate a currently bound render target!");
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
		std::vector<surface_overlap_info> get_merged_texture_memory_region(commandbuffer_type& cmd, u32 texaddr, u32 required_width, u32 required_height, u32 required_pitch, u8 required_bpp)
		{
			std::vector<surface_overlap_info> result;
			std::vector<std::pair<u32, bool>> dirty;
			const u32 limit = texaddr + (required_pitch * required_height);

			auto process_list_function = [&](std::unordered_map<u32, surface_storage_type>& data, bool is_depth)
			{
				for (auto &tex_info : data)
				{
					const auto this_address = tex_info.first;
					if (this_address >= limit)
						continue;

					auto surface = tex_info.second.get();
					const auto pitch = surface->get_rsx_pitch();
					if (!rsx::pitch_compatible(surface, required_pitch, required_height))
						continue;

					const auto texture_size = pitch * surface->get_surface_height(rsx::surface_metrics::samples);
					if ((this_address + texture_size) <= texaddr)
						continue;

					if (surface->read_barrier(cmd); !surface->test())
					{
						dirty.emplace_back(this_address, is_depth);
						continue;
					}

					surface_overlap_info info;
					info.surface = surface;
					info.base_address = this_address;
					info.is_depth = is_depth;

					const auto normalized_surface_width = surface->get_surface_width(rsx::surface_metrics::bytes) / required_bpp;
					const auto normalized_surface_height = surface->get_surface_height(rsx::surface_metrics::samples);

					if (LIKELY(this_address >= texaddr))
					{
						const auto offset = this_address - texaddr;
						info.dst_y = (offset / required_pitch);
						info.dst_x = (offset % required_pitch) / required_bpp;

						if (UNLIKELY(info.dst_x >= required_width || info.dst_y >= required_height))
						{
							// Out of bounds
							continue;
						}

						info.src_x = 0;
						info.src_y = 0;
						info.width = std::min<u32>(normalized_surface_width, required_width - info.dst_x);
						info.height = std::min<u32>(normalized_surface_height, required_height - info.dst_y);
					}
					else
					{
						const auto offset = texaddr - this_address;
						info.src_y = (offset / required_pitch);
						info.src_x = (offset % required_pitch) / required_bpp;

						if (UNLIKELY(info.src_x >= normalized_surface_width || info.src_y >= normalized_surface_height))
						{
							// Region lies outside the actual texture area, but inside the 'tile'
							// In this case, a small region lies to the top-left corner, partially occupying the  target
							continue;
						}

						info.dst_x = 0;
						info.dst_y = 0;
						info.width = std::min<u32>(required_width, normalized_surface_width - info.src_x);
						info.height = std::min<u32>(required_height, normalized_surface_height - info.src_y);
					}

					info.is_clipped = (info.width < required_width || info.height < required_height);

					if (auto surface_bpp = surface->get_bpp(); UNLIKELY(surface_bpp != required_bpp))
					{
						// Width is calculated in the coordinate-space of the requester; normalize
						info.src_x = (info.src_x * required_bpp) / surface_bpp;
						info.width = (info.width * required_bpp) / surface_bpp;
					}

					result.push_back(info);
				}
			};

			// Range test helper to quickly discard blocks
			// Fortunately, render targets tend to be clustered anyway
			rsx::address_range test = rsx::address_range::start_end(texaddr, limit-1);

			if (test.overlaps(m_render_targets_memory_range))
			{
				process_list_function(m_render_targets_storage, false);
			}

			if (test.overlaps(m_depth_stencil_memory_range))
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
						const auto area_a = a.width * a.height;
						const auto area_b = b.width * b.height;

						return area_a < area_b;
					}

					return a.surface->last_use_tag < b.surface->last_use_tag;
				});
			}

			return result;
		}

		void on_write(u32 address = 0)
		{
			if (!address)
			{
				if (write_tag == cache_tag)
				{
					if (m_invalidate_on_write)
					{
						for (int i = m_bound_render_targets_config.first, count = 0;
							count < m_bound_render_targets_config.second;
							++i, ++count)
						{
							m_bound_render_targets[i].second->on_invalidate_children();
						}

						if (m_bound_depth_stencil.first)
						{
							m_bound_depth_stencil.second->on_invalidate_children();
						}
					}

					return;
				}
				else
				{
					write_tag = cache_tag;
				}

				// Tag all available surfaces
				for (int i = m_bound_render_targets_config.first, count = 0;
					count < m_bound_render_targets_config.second;
					++i, ++count)
				{
					m_bound_render_targets[i].second->on_write(write_tag);
				}

				if (m_bound_depth_stencil.first)
				{
					m_bound_depth_stencil.second->on_write(write_tag);
				}
			}
			else
			{
				for (int i = m_bound_render_targets_config.first, count = 0;
					count < m_bound_render_targets_config.second;
					++i, ++count)
				{
					if (m_bound_render_targets[i].first != address)
					{
						continue;
					}

					m_bound_render_targets[i].second->on_write(write_tag);
				}

				if (m_bound_depth_stencil.first == address)
				{
					m_bound_depth_stencil.second->on_write(write_tag);
				}
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
	};
}
