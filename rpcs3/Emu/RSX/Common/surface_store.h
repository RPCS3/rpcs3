#pragma once

#include "surface_utils.h"
#include "simple_array.hpp"
#include "ranged_map.hpp"
#include "surface_cache_dma.hpp"
#include "../gcm_enums.h"
#include "../rsx_utils.h"
#include <list>

#include "util/asm.hpp"
#include "util/pair.hpp"

namespace rsx
{
	namespace utility
	{
		rsx::simple_array<u8> get_rtt_indexes(surface_target color_target);
		u8 get_mrt_buffers_count(surface_target color_target);
		usz get_aligned_pitch(surface_color_format format, u32 width);
		usz get_packed_pitch(surface_color_format format, u32 width);
	}

	template <typename Traits>
	struct surface_store
	{
		static constexpr u32 get_aa_factor_u(surface_antialiasing aa_mode)
		{
			return (aa_mode == surface_antialiasing::center_1_sample)? 1 : 2;
		}

		static constexpr u32 get_aa_factor_v(surface_antialiasing aa_mode)
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
		using surface_ranged_map = ranged_map<surface_storage_type, 0x400000>;
		using surface_cache_dma_map = surface_cache_dma<Traits, 0x400000>;

	protected:
		surface_ranged_map m_render_targets_storage = {};
		surface_ranged_map m_depth_stencil_storage = {};

		rsx::address_range32 m_render_targets_memory_range;
		rsx::address_range32 m_depth_stencil_memory_range;

		surface_cache_dma_map m_dma_block;

		bool m_invalidate_on_write = false;

		rsx::surface_raster_type m_active_raster_type = rsx::surface_raster_type::linear;

	public:
		rsx::simple_array<u8> m_bound_render_target_ids = {};
		std::array<std::pair<u32, surface_type>, 4> m_bound_render_targets = {};
		std::pair<u32, surface_type> m_bound_depth_stencil = {};

		// List of sections derived from a section that has been split and invalidated
		std::vector<std::pair<u32, surface_type>> orphaned_surfaces;

		// List of sections that have been wholly inherited and invalidated
		std::vector<surface_type> superseded_surfaces;

		std::list<surface_storage_type> invalidated_resources;
		const u64 max_invalidated_resources_count = 256ull;
		u64 cache_tag = 1ull; // Use 1 as the start since 0 is default tag on new surfaces
		u64 write_tag = 1ull;

		// Amount of virtual PS3 memory tied to allocated textures
		u64 m_active_memory_used = 0;

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
				surface_ranged_map& data)
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
						invalidate(found->second);
						data.erase(new_address);

						auto &old = invalidated_resources.back();
						if (Traits::surface_is_pitch_compatible(old, prev_surface->get_rsx_pitch()))
						{
							if (old->last_use_tag >= prev_surface->last_use_tag) [[unlikely]]
							{
								invalidated = Traits::get(old);
							}
						}
					}
				}

				if (sink)
				{
					// Memory requirements can be altered when cloning
					free_rsx_memory(Traits::get(sink));
				}

				Traits::clone_surface(cmd, sink, region.source, new_address, region);
				allocate_rsx_memory(Traits::get(sink));

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

				ensure(region.target == Traits::get(sink));
				orphaned_surfaces.push_back({ address, region.target });
				data.emplace(region.target->get_memory_range(), std::move(sink));
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
			ensure(prev_surface);
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
				copy.width = std::max<u16>((old.width - _new.width) / bytes_to_texels_x, 1);
				copy.height = prev_surface->template get_surface_height<>();
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
				copy.width = std::max<u16>(std::min(_new.width, old.width) / bytes_to_texels_x, 1);
				copy.height = std::max<u16>((old.height - _new.height) / prev_surface->samples_y, 1);
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
			auto scan_list = [&new_surface, address](const rsx::address_range32& mem_range, surface_ranged_map& data)
			{
				rsx::simple_array<utils::pair<u32, surface_type>> result;
				for (auto it = data.begin_range(mem_range); it != data.end(); ++it)
				{
					auto surface = Traits::get(it->second);

					if (new_surface->last_use_tag >= surface->last_use_tag ||
						new_surface == surface ||
						address == it->first)
					{
						// Do not bother synchronizing with uninitialized data
						continue;
					}

					// Memory partition check
					if (mem_range.start >= constants::local_mem_base)
					{
						if (it->first < constants::local_mem_base) continue;
					}
					else
					{
						if (it->first >= constants::local_mem_base) continue;
					}

					// Pitch check
					if (!rsx::pitch_compatible(surface, new_surface))
					{
						continue;
					}

					// Range check
					const rsx::address_range32 this_range = surface->get_memory_range();
					if (!this_range.overlaps(mem_range))
					{
						continue;
					}

					result.push_back({ it->first, surface });
					ensure(it->first == surface->base_addr);
				}

				return result;
			};

			const rsx::address_range32 mem_range = new_surface->get_memory_range();
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

			rsx::simple_array<utils::pair<u32, surface_type>> surface_info;
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
					for (const auto& slice : new_surface->old_contents)
					{
						if (slice.source == surface)
						{
							ignore = true;
							break;
						}
					}

					if (ignore) continue;

					this_address = surface->base_addr;
					ensure(this_address);
				}

				if (new_surface->inherit_surface_contents(surface) == surface_inheritance_result::full &&
					surface->memory_usage_flags == surface_usage_flags::storage &&
					surface != prev_surface &&
					surface == e.second)
				{
					// This has been 'swallowed' by the new surface and can be safely freed
					auto& storage = surface->is_depth_surface() ? m_depth_stencil_storage : m_render_targets_storage;
					auto& object = ::at32(storage, e.first);

					ensure(object);

					if (!surface->old_contents.empty()) [[unlikely]]
					{
						surface->read_barrier(cmd);
					}

					invalidate(object);
					storage.erase(e.first);
					superseded_surfaces.push_back(surface);
				}
			}
		}

		template <bool depth, typename format_type, typename ...Args>
		surface_type bind_surface_address(
			command_list_type command_list,
			u32 address,
			format_type format,
			surface_antialiasing antialias,
			usz width, usz height, usz pitch,
			u8 bpp,
			Args&&... extra_params)
		{
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			bool do_intersection_test = true;
			bool store = true;

			// Workaround. Preserve new surface tag value because pitch convert is unimplemented
			u64 new_content_tag = 0;

			address_range32* storage_bounds;
			surface_ranged_map* primary_storage;
			surface_ranged_map* secondary_storage;
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
				surface_storage_type& surface = It->second;
				const bool pitch_compatible = Traits::surface_is_pitch_compatible(surface, pitch);

				if (!pitch_compatible)
				{
					// This object should be pitch-converted and re-intersected with
					if (old_surface_storage = Traits::convert_pitch(command_list, surface, pitch); old_surface_storage)
					{
						old_surface = Traits::get(old_surface_storage);
					}
					else
					{
						// Preserve content age. This is hacky, but matches previous behavior
						// TODO: Remove when pitch convert is implemented
						new_content_tag = Traits::get(surface)->last_use_tag;
					}
				}

				if (Traits::surface_matches_properties(surface, format, width, height, antialias))
				{
					if (!pitch_compatible)
					{
						Traits::invalidate_surface_contents(command_list, Traits::get(surface), format, address, pitch);
					}

					Traits::notify_surface_persist(surface);
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
					free_rsx_memory(Traits::get(surface));
					Traits::notify_surface_invalidated(surface);

					if (old_surface_storage)
					{
						// Pitch-converted data. Send to invalidated pool immediately.
						invalidated_resources.push_back(std::move(old_surface_storage));
					}

					old_surface_storage = std::move(surface);
					primary_storage->erase(It);
				}
			}

			if (!new_surface)
			{
				// Range test
				const auto aa_factor_v = get_aa_factor_v(antialias);
				rsx::address_range32 range = rsx::address_range32::start_length(address, static_cast<u32>(pitch * height * aa_factor_v));
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
							old_surface_storage = {};
						}
						else
						{
							// Iterator is now empty - erase it
							invalidated_resources.erase(It);
						}

						new_surface = Traits::get(new_surface_storage);
						Traits::invalidate_surface_contents(command_list, new_surface, format, address, pitch);
						Traits::prepare_surface_for_drawing(command_list, new_surface);
						allocate_rsx_memory(new_surface);
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
				ensure(store);
				new_surface_storage = Traits::create_new_surface(address, format, width, height, pitch, antialias, std::forward<Args>(extra_params)...);
				new_surface = Traits::get(new_surface_storage);
				Traits::prepare_surface_for_drawing(command_list, new_surface);
				allocate_rsx_memory(new_surface);
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

				invalidate(aliased_surface->second);
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
				if (new_content_tag)
				{
					new_surface->last_use_tag = new_content_tag;
				}

				intersect_surface_region<depth>(command_list, address, new_surface, old_surface);
			}

			if (store)
			{
				// New surface was found among invalidated surfaces or created from scratch
				primary_storage->emplace(new_surface->get_memory_range(), std::move(new_surface_storage));
			}

			ensure(!old_surface_storage);
			ensure(new_surface->get_spp() == get_format_sample_count(antialias));
			return new_surface;
		}

		void allocate_rsx_memory(surface_type surface)
		{
			const auto memory_size = surface->get_memory_range().length();
			m_active_memory_used += memory_size;
		}

		void free_rsx_memory(surface_type surface)
		{
			ensure(surface->has_refs()); // "Surface memory double free"

			if (const auto memory_size = surface->get_memory_range().length();
				m_active_memory_used >= memory_size) [[likely]]
			{
				m_active_memory_used -= memory_size;
			}
			else
			{
				rsx_log.error("Memory allocation underflow!");
				m_active_memory_used = 0;
			}
		}

		inline void invalidate(surface_storage_type& storage)
		{
			free_rsx_memory(Traits::get(storage));
			Traits::notify_surface_invalidated(storage);
			invalidated_resources.push_back(std::move(storage));
		}

		int remove_duplicates_fast_impl(rsx::simple_array<surface_overlap_info>& sections, const rsx::address_range32& range)
		{
			// Range tests to check for gaps
			std::list<utils::address_range32> m_ranges;
			bool invalidate_sections = false;
			int removed_count = 0;

			for (auto it = sections.crbegin(); it != sections.crend(); ++it)
			{
				auto this_range = it->surface->get_memory_range();
				if (invalidate_sections)
				{
					if (this_range.inside(range))
					{
						invalidate_surface_address(it->base_address, it->is_depth);
						removed_count++;
					}
					continue;
				}

				if (it->surface->get_rsx_pitch() != it->surface->get_native_pitch() &&
					it->surface->template get_surface_height<>() != 1)
				{
					// Memory gap in descriptor
					continue;
				}

				// Insert the range, respecting sort order
				bool inserted = false;
				for (auto iter = m_ranges.begin(); iter != m_ranges.end(); ++iter)
				{
					if (this_range.start < iter->start)
					{
						// This range slots in here. Test ranges after this one to find the end position
						auto pos = iter;
						for (auto _p = ++iter; _p != m_ranges.end();)
						{
							if (_p->start > (this_range.end + 1))
							{
								// Gap
								break;
							}

							// Consume
							this_range.end = std::max(this_range.end, _p->end);
							_p = m_ranges.erase(_p);
						}

						m_ranges.insert(pos, this_range);
						inserted = true;
						break;
					}
				}

				if (!inserted)
				{
					m_ranges.push_back(this_range);
				}
				else if (m_ranges.size() == 1 && range.inside(m_ranges.front()))
				{
					invalidate_sections = true;
				}
			}

			rsx_log.notice("rsx::surface_cache::check_for_duplicates_fast analysed %u overlapping sections and removed %u", ::size32(sections), removed_count);
			return removed_count;
		}

		void remove_duplicates_fallback_impl(rsx::simple_array<surface_overlap_info>& sections, const rsx::address_range32& range)
		{
			// Originally used to debug crashes but this function breaks often enough that I'll leave the checks in for now.
			// Safe to remove after some time if no asserts are reported.
			constexpr u32 overrun_cookie_value = 0xCAFEBABEu;

			// Generic painter's algorithm to detect obsolete sections
			ensure(range.length() < 64 * 0x100000);
			std::vector<u8> marker(range.length() + sizeof(overrun_cookie_value), 0);

			// Tag end
			write_to_ptr(marker, range.length(), overrun_cookie_value);

			u32 removed_count = 0;

			auto compare_and_tag_row = [&](const u32 offset, u32 length) -> bool
			{
				u64 mask = 0;
				u8* dst_ptr = marker.data() + offset;

				while (length >= 8)
				{
					const u64 value = read_from_ptr<u64>(dst_ptr);
					const u64 block_mask = ~value;              // If the value is not all 1s, set valid to true
					mask |= block_mask;
					write_to_ptr<u64>(dst_ptr, umax);

					dst_ptr += 8;
					length -= 8;
				}

				if (length >= 4)
				{
					const u32 value = read_from_ptr<u32>(dst_ptr);
					const u32 block_mask = ~value;
					mask |= block_mask;
					write_to_ptr<u32>(dst_ptr, umax);

					dst_ptr += 4;
					length -= 4;
				}

				if (length >= 2)
				{
					const u16 value = read_from_ptr<u16>(dst_ptr);
					const u16 block_mask = ~value;
					mask |= block_mask;
					write_to_ptr<u16>(dst_ptr, umax);

					dst_ptr += 2;
					length -= 2;
				}

				if (length)
				{
					const u8 value = *dst_ptr;
					const u8 block_mask = ~value;
					mask |= block_mask;
					*dst_ptr = umax;
				}

				return !!mask;
			};

			for (auto it = sections.crbegin(); it != sections.crend(); ++it)
			{
				auto this_range = it->surface->get_memory_range();
				ensure(this_range.overlaps(range));

				const auto native_pitch = it->surface->template get_surface_width<rsx::surface_metrics::bytes>();
				const auto rsx_pitch = it->surface->get_rsx_pitch();
				auto num_rows = it->surface->template get_surface_height<rsx::surface_metrics::samples>();
				bool valid = false;

				if (this_range.start < range.start)
				{
					// Starts outside bounds
					const auto internal_offset = (range.start - this_range.start);
					const auto row_num = internal_offset / rsx_pitch;
					const auto row_offset = internal_offset % rsx_pitch;

					// This section is unconditionally valid
					valid = true;

					if (row_offset < native_pitch)
					{
						compare_and_tag_row(0, std::min(native_pitch - row_offset, range.length()));
					}

					// Jump to next row...
					this_range.start = this_range.start + (row_num + 1) * rsx_pitch;
				}

				if (this_range.end > range.end)
				{
					// Unconditionally valid
					valid = true;
					this_range.end = range.end;
				}

				if (valid)
				{
					if (this_range.start >= this_range.end)
					{
						continue;
					}

					num_rows = utils::aligned_div(this_range.length(), rsx_pitch);
				}

				for (u32 row = 0, offset = (this_range.start - range.start), section_len = (this_range.end - range.start + 1);
					row < num_rows;
					++row, offset += rsx_pitch)
				{
					valid |= compare_and_tag_row(offset, std::min<u32>(native_pitch, (section_len - offset)));
				}

				if (!valid)
				{
					removed_count++;
					rsx_log.warning("Stale surface at address 0x%x will be deleted", it->base_address);
					invalidate_surface_address(it->base_address, it->is_depth);
				}
			}

			// Notify
			rsx_log.notice("rsx::surface_cache::check_for_duplicates_fallback analysed %u overlapping sections and removed %u", ::size32(sections), removed_count);

			// Verify no OOB
			ensure(read_from_ptr<u32>(marker, range.length()) == overrun_cookie_value);
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
			usz width, usz height, usz pitch,
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
			surface_depth_format2 depth_format,
			surface_antialiasing antialias,
			usz width, usz height, usz pitch,
			Args&&... extra_params)
		{
			return bind_surface_address<true>(
				command_list, address, depth_format, antialias,
				width, height, pitch,
				get_format_block_size_in_bytes(depth_format),
				std::forward<Args>(extra_params)...);
		}

		std::tuple<rsx::simple_array<surface_type>, rsx::simple_array<surface_type>>
		find_overlapping_set(const utils::address_range32& range) const
		{
			rsx::simple_array<surface_type> color_result, depth_result;
			utils::address_range32 result_range;

			if (m_render_targets_memory_range.valid() &&
				range.overlaps(m_render_targets_memory_range))
			{
				for (auto it = m_render_targets_storage.begin_range(range); it != m_render_targets_storage.end(); ++it)
				{
					auto surface = Traits::get(it->second);
					const auto surface_range = surface->get_memory_range();
					if (!range.overlaps(surface_range))
						continue;

					color_result.push_back(surface);
				}
			}

			if (m_depth_stencil_memory_range.valid() &&
				range.overlaps(m_depth_stencil_memory_range))
			{
				for (auto it = m_depth_stencil_storage.begin_range(range); it != m_depth_stencil_storage.end(); ++it)
				{
					auto surface = Traits::get(it->second);
					const auto surface_range = surface->get_memory_range();
					if (!range.overlaps(surface_range))
						continue;

					depth_result.push_back(surface);
				}
			}

			return { color_result, depth_result, result_range };
		}

		void write_to_dma_buffers(
			command_list_type command_list,
			const utils::address_range32& range)
		{
			auto block_range = m_dma_block.to_block_range(range);
			auto [color_data, depth_stencil_data] = find_overlapping_set(block_range);
			auto [bo, offset, bo_timestamp] = m_dma_block
				.with_range(command_list, block_range)
				.get(block_range.start);

			u64 src_offset, dst_offset, write_length;
			auto block_length = block_range.length();

			auto& all_data = color_data;
			all_data += depth_stencil_data;

			if (all_data.size() > 1)
			{
				std::sort(all_data.begin(), all_data.end(), [](const auto& a, const auto& b)
				{
					return a->last_use_tag < b->last_use_tag;
				});
			}

			for (const auto& surface : all_data)
			{
				if (surface->last_use_tag <= bo_timestamp)
				{
					continue;
				}

				const auto this_range = surface->get_memory_range();
				const auto max_length = this_range.length();
				if (this_range.start < block_range.start)
				{
					src_offset = block_range.start - this_range.start;
					dst_offset = 0;
				}
				else
				{
					src_offset = 0;
					dst_offset = this_range.start - block_range.start;
				}

				write_length = std::min(max_length, block_length - dst_offset);
				Traits::write_render_target_to_memory(command_list, bo, surface, dst_offset, src_offset, write_length);
			}

			m_dma_block.touch(block_range);
		}

	public:
		/**
		 * Update bound color and depth surface.
		 * Must be called everytime surface format, clip, or addresses changes.
		 */
		template <typename ...Args>
		void prepare_render_target(
			command_list_type command_list,
			surface_color_format color_format, surface_depth_format2 depth_format,
			u32 clip_horizontal_reg, u32 clip_vertical_reg,
			surface_target set_surface_target,
			surface_antialiasing antialias,
			surface_raster_type raster_type,
			const std::array<u32, 4> &surface_addresses, u32 address_z,
			const std::array<u32, 4> &surface_pitch, u32 zeta_pitch,
			Args&&... extra_params)
		{
			u32 clip_width = clip_horizontal_reg;
			u32 clip_height = clip_vertical_reg;

			cache_tag = rsx::get_shared_tag();
			m_invalidate_on_write = (antialias != rsx::surface_antialiasing::center_1_sample);
			m_active_raster_type = raster_type;

			// Make previous RTTs sampleable
			for (const auto& i : m_bound_render_target_ids)
			{
				auto &rtt = m_bound_render_targets[i];
				Traits::prepare_surface_for_sampling(command_list, std::get<1>(rtt));
				rtt = std::make_pair(0, nullptr);
			}

			m_bound_render_target_ids.clear();
			if (const auto rtt_indices = utility::get_rtt_indexes(set_surface_target);
				!rtt_indices.empty()) [[likely]]
			{
				// Create/Reuse requested rtts
				for (u8 surface_index : rtt_indices)
				{
					if (surface_addresses[surface_index] == 0)
						continue;

					m_bound_render_targets[surface_index] = std::make_pair(surface_addresses[surface_index],
						bind_address_as_render_targets(command_list, surface_addresses[surface_index], color_format, antialias,
							clip_width, clip_height, surface_pitch[surface_index], std::forward<Args>(extra_params)...));

					m_bound_render_target_ids.push_back(surface_index);
				}
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
			}
			else
			{
				m_bound_depth_stencil = std::make_pair(0, nullptr);
			}
		}

		u8 get_color_surface_count() const
		{
			return static_cast<u8>(m_bound_render_target_ids.size());
		}

		surface_type get_surface_at(u32 address)
		{
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return Traits::get(It->second);

			auto _It = m_depth_stencil_storage.find(address);
			if (_It != m_depth_stencil_storage.end())
				return Traits::get(_It->second);

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
					invalidate(It->second);
					m_render_targets_storage.erase(It);
					return;
				}
			}
			else
			{
				auto It = m_depth_stencil_storage.find(addr);
				if (It != m_depth_stencil_storage.end())
				{
					invalidate(It->second);
					m_depth_stencil_storage.erase(It);
					return;
				}
			}
		}

		inline bool address_is_bound(u32 address) const
		{
			ensure(address);
			for (int i = 0; i < 4; ++i)
			{
				if (m_bound_render_targets[i].first == address)
				{
					return true;
				}
			}

			return (m_bound_depth_stencil.first == address);
		}

		template <typename commandbuffer_type>
		rsx::simple_array<surface_overlap_info> get_merged_texture_memory_region(commandbuffer_type& cmd, u32 texaddr, u32 required_width, u32 required_height, u32 required_pitch, u8 required_bpp, rsx::surface_access access)
		{
			rsx::simple_array<surface_overlap_info> result;
			rsx::simple_array<utils::pair<u32, bool>> dirty;

			const auto surface_internal_pitch = (required_width * required_bpp);

			// Sanity check
			if (surface_internal_pitch > required_pitch) [[unlikely]]
			{
				rsx_log.warning("Invalid 2D region descriptor. w=%d, h=%d, bpp=%d, pitch=%d",
							required_width, required_height, required_bpp, required_pitch);
				return {};
			}

			const auto test_range = utils::address_range32::start_length(texaddr, (required_pitch * required_height) - (required_pitch - surface_internal_pitch));

			auto process_list_function = [&](surface_ranged_map& data, bool is_depth)
			{
				for (auto it = data.begin_range(test_range); it != data.end(); ++it)
				{
					const auto range = it->second->get_memory_range();
					if (!range.overlaps(test_range))
						continue;

					auto surface = it->second.get();
					if (access.is_transfer() && access.is_read() && surface->write_through())
					{
						// The surface has no data other than what can be loaded from CPU
						continue;
					}

					if (!rsx::pitch_compatible(surface, required_pitch, required_height))
						continue;

					surface_overlap_info info;
					u32 width, height;
					info.surface = surface;
					info.base_address = range.start;
					info.is_depth = is_depth;

					const u32 normalized_surface_width = surface->template get_surface_width<rsx::surface_metrics::bytes>() / required_bpp;
					const u32 normalized_surface_height = surface->template get_surface_height<rsx::surface_metrics::samples>();

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
						info.src_area.width = utils::align(width * required_bpp, surface_bpp) / surface_bpp;
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
			if (m_render_targets_memory_range.valid() &&
				test_range.overlaps(m_render_targets_memory_range))
			{
				process_list_function(m_render_targets_storage, false);
			}

			if (m_depth_stencil_memory_range.valid() &&
				test_range.overlaps(m_depth_stencil_memory_range))
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

		void check_for_duplicates(rsx::simple_array<surface_overlap_info>& sections)
		{
			utils::address_range32 test_range;
			for (const auto& section : sections)
			{
				const auto range = section.surface->get_memory_range();
				test_range.start = std::min(test_range.start, range.start);
				test_range.end = std::max(test_range.end, range.end);
			}

			if (!remove_duplicates_fast_impl(sections, test_range))
			{
				remove_duplicates_fallback_impl(sections, test_range);
			}
		}

		void on_write(const std::array<bool, 4>& color_mrt_writes_enabled, const bool depth_stencil_writes_enabled)
		{
			if (write_tag >= cache_tag && !m_invalidate_on_write)
			{
				return;
			}

			// TODO: Take WCB/WDB into account. Should speed this up a bit by skipping sync_tag calls
			write_tag = rsx::get_shared_tag();

			for (const auto& i : m_bound_render_target_ids)
			{
				if (color_mrt_writes_enabled[i])
				{
					auto surface = m_bound_render_targets[i].second;
					if (surface->last_use_tag > cache_tag) [[ likely ]]
					{
						surface->on_write_fast(write_tag);
					}
					else
					{
						surface->on_write(write_tag, rsx::surface_state_flags::require_resolve, m_active_raster_type);
					}
				}
			}

			if (auto zsurface = m_bound_depth_stencil.second;
				zsurface && depth_stencil_writes_enabled)
			{
				if (zsurface->last_use_tag > cache_tag) [[ likely ]]
				{
					zsurface->on_write_fast(write_tag);
				}
				else
				{
					zsurface->on_write(write_tag, rsx::surface_state_flags::require_resolve, m_active_raster_type);
				}
			}
		}

		void invalidate_all()
		{
			// Unbind and invalidate all resources
			auto free_resource_list = [&](auto &data, const utils::address_range32& range)
			{
				for (auto it = data.begin_range(range); it != data.end(); ++it)
				{
					invalidate(it->second);
				}

				data.clear();
			};

			free_resource_list(m_render_targets_storage, m_render_targets_memory_range);
			free_resource_list(m_depth_stencil_storage, m_depth_stencil_memory_range);

			ensure(m_active_memory_used == 0);

			m_bound_depth_stencil = std::make_pair(0, nullptr);
			m_bound_render_target_ids.clear();
			for (auto &rtt : m_bound_render_targets)
			{
				rtt = std::make_pair(0, nullptr);
			}
		}

		void invalidate_range(const rsx::address_range32& range)
		{
			for (auto it = m_render_targets_storage.begin_range(range); it != m_render_targets_storage.end(); ++it)
			{
				auto& rtt = it->second;
				if (range.overlaps(rtt->get_memory_range()))
				{
					rtt->clear_rw_barrier();
					rtt->state_flags |= rsx::surface_state_flags::erase_bkgnd;
				}
			}

			for (auto it = m_depth_stencil_storage.begin_range(range); it != m_depth_stencil_storage.end(); ++it)
			{
				auto& ds = it->second;
				if (range.overlaps(ds->get_memory_range()))
				{
					ds->clear_rw_barrier();
					ds->state_flags |= rsx::surface_state_flags::erase_bkgnd;
				}
			}
		}

		bool check_memory_usage(u64 max_safe_memory) const
		{
			if (m_active_memory_used <= max_safe_memory) [[likely]]
			{
				return false;
			}
			else if (m_active_memory_used > (max_safe_memory * 3) / 2)
			{
				rsx_log.warning("Surface cache is using too much memory! (%dM)", m_active_memory_used / 0x100000);
			}
			else
			{
				rsx_log.trace("Surface cache is using too much memory! (%dM)", m_active_memory_used / 0x100000);
			}

			return true;
		}

		virtual bool can_collapse_surface(const surface_storage_type&, problem_severity)
		{
			return true;
		}

		void trim_invalidated_resources(command_list_type cmd, problem_severity severity)
		{
			// It is possible to have stale invalidated resources holding references to other invalidated resources.
			// This can bloat the VRAM usage significantly especially if the references are never collapsed.
			for (auto& surface : invalidated_resources)
			{
				if (!surface->has_refs() || surface->old_contents.empty())
				{
					continue;
				}

				if (can_collapse_surface(surface, severity))
				{
					surface->memory_barrier(cmd, rsx::surface_access::transfer_read);
				}
			}
		}

		void collapse_dirty_surfaces(command_list_type cmd, problem_severity severity)
		{
			auto process_list_function = [&](surface_ranged_map& data, const utils::address_range32& range)
			{
				for (auto It = data.begin_range(range); It != data.end();)
				{
					auto surface = Traits::get(It->second);
					if (surface->dirty())
					{
						// Force memory barrier to release some resources
						if (can_collapse_surface(It->second, severity))
						{
							// NOTE: Do not call memory_barrier under fatal conditions as it can create allocations!
							// It would be safer to leave the resources hanging around and spill them instead
							surface->memory_barrier(cmd, rsx::surface_access::memory_read);
						}
					}
					else if (!surface->test())
					{
						// Remove this
						invalidate(It->second);
						It = data.erase(It);
						continue;
					}

					++It;
				}
			};

			process_list_function(m_render_targets_storage, m_render_targets_memory_range);
			process_list_function(m_depth_stencil_storage, m_depth_stencil_memory_range);
		}

		virtual bool handle_memory_pressure(command_list_type cmd, problem_severity severity)
		{
			ensure(severity >= rsx::problem_severity::moderate);
			const auto old_usage = m_active_memory_used;

			// Try and find old surfaces to remove
			collapse_dirty_surfaces(cmd, severity);

			// Check invalidated resources as they can have long dependency chains
			if (invalidated_resources.size() > max_invalidated_resources_count ||
				severity >= rsx::problem_severity::severe)
			{
				trim_invalidated_resources(cmd, severity);
			}

			return (m_active_memory_used < old_usage);
		}

		void run_cleanup_internal(
			command_list_type cmd,
			rsx::problem_severity memory_pressure,
			u32 max_surface_store_memory_mb,
			std::function<void(command_list_type)> pre_task_callback)
		{
			if (check_memory_usage(max_surface_store_memory_mb * 0x100000))
			{
				pre_task_callback(cmd);

				const auto severity = std::max(memory_pressure, rsx::problem_severity::moderate);
				handle_memory_pressure(cmd, severity);
			}
			else if (invalidated_resources.size() > max_invalidated_resources_count)
			{
				pre_task_callback(cmd);

				rsx_log.warning("[PERFORMANCE WARNING] Invalidated resource pool has exceeded the desired limit. A trim will now be attempted. Current=%u, Limit=%u",
					invalidated_resources.size(), max_invalidated_resources_count);

				// Check invalidated resources as they can have long dependency chains
				trim_invalidated_resources(cmd, memory_pressure);

				if ((invalidated_resources.size() + 16u) > max_invalidated_resources_count)
				{
					// We didn't release enough resources, scan the active RTTs as well
					collapse_dirty_surfaces(cmd, memory_pressure);
				}
			}
		}
	};
}
