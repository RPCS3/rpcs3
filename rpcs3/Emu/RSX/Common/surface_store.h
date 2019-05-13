#pragma once

#include "Utilities/GSL.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
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

	template <typename surface_type>
	struct surface_overlap_info_t
	{
		surface_type surface = nullptr;
		u32 base_address = 0;
		bool is_depth = false;
		bool is_clipped = false;

		u16 src_x = 0;
		u16 src_y = 0;
		u16 dst_x = 0;
		u16 dst_y = 0;
		u16 width = 0;
		u16 height = 0;

		areai get_src_area() const
		{
			return coordi{ {src_x, src_y}, {width, height} };
		}

		areai get_dst_area() const
		{
			return coordi{ {dst_x, dst_y}, {width, height} };
		}
	};

	struct surface_format_info
	{
		u32 surface_width;
		u32 surface_height;
		u16 native_pitch;
		u16 rsx_pitch;
		u8 bpp;
	};

	template <typename surface_type>
	struct deferred_clipped_region
	{
		u16 src_x, src_y, dst_x, dst_y, width, height;
		f32 transfer_scale_x, transfer_scale_y;
		surface_type target;
		surface_type source;

		template <typename T>
		deferred_clipped_region<T> cast() const
		{
			deferred_clipped_region<T> ret;
			ret.src_x = src_x;
			ret.src_y = src_y;
			ret.dst_x = dst_x;
			ret.dst_y = dst_y;
			ret.width = width;
			ret.height = height;
			ret.transfer_scale_x = transfer_scale_x;
			ret.transfer_scale_y = transfer_scale_y;
			ret.target = (T)(target);
			ret.source = (T)(source);

			return ret;
		}

		operator bool() const
		{
			return (source != nullptr);
		}

		template <typename T>
		void init_transfer(T target_surface)
		{
			if (!width)
			{
				// Perform intersection here
				const auto region = rsx::get_transferable_region(target_surface);
				width = std::get<0>(region);
				height = std::get<1>(region);

				transfer_scale_x = f32(std::get<2>(region)) / width;
				transfer_scale_y = f32(std::get<3>(region)) / height;

				target = target_surface;
			}
		}

		areai src_rect() const
		{
			verify(HERE), width;
			return { src_x, src_y, src_x + width, src_y + height };
		}

		areai dst_rect() const
		{
			verify(HERE), width;
			return { dst_x, dst_y, dst_x + u16(width * transfer_scale_x + 0.5f), dst_y + u16(height * transfer_scale_y + 0.5f) };
		}
	};

	template <typename image_storage_type>
	struct render_target_descriptor
	{
		u64 last_use_tag = 0;         // tag indicating when this block was last confirmed to have been written to
		std::array<std::pair<u32, u64>, 5> memory_tag_samples;

		bool dirty = false;
		deferred_clipped_region<image_storage_type> old_contents{};
		rsx::surface_antialiasing read_aa_mode = rsx::surface_antialiasing::center_1_sample;

		GcmTileInfo *tile = nullptr;
		rsx::surface_antialiasing write_aa_mode = rsx::surface_antialiasing::center_1_sample;

		flags32_t usage = surface_usage_flags::unknown;

		union
		{
			rsx::surface_color_format gcm_color_format;
			rsx::surface_depth_format gcm_depth_format;
		}
		format_info;

		render_target_descriptor() {}

		virtual ~render_target_descriptor()
		{
			if (old_contents)
			{
				// Cascade resource derefs
				LOG_ERROR(RSX, "Resource was destroyed whilst holding a resource reference!");
			}
		}

		virtual image_storage_type get_surface() = 0;
		virtual u16 get_surface_width() const = 0;
		virtual u16 get_surface_height() const = 0;
		virtual u16 get_rsx_pitch() const = 0;
		virtual u16 get_native_pitch() const = 0;
		virtual bool is_depth_surface() const = 0;
		virtual void release_ref(image_storage_type) const = 0;

		u8 get_bpp() const
		{
			return u8(get_native_pitch() / get_surface_width());
		}

		void save_aa_mode()
		{
			read_aa_mode = write_aa_mode;
			write_aa_mode = rsx::surface_antialiasing::center_1_sample;
		}

		void reset_aa_mode()
		{
			write_aa_mode = read_aa_mode = rsx::surface_antialiasing::center_1_sample;
		}

		void set_format(rsx::surface_color_format format)
		{
			format_info.gcm_color_format = format;
		}

		void set_format(rsx::surface_depth_format format)
		{
			format_info.gcm_depth_format = format;
		}

		rsx::surface_color_format get_surface_color_format()
		{
			return format_info.gcm_color_format;
		}

		rsx::surface_depth_format get_surface_depth_format()
		{
			return format_info.gcm_depth_format;
		}

		bool test() const
		{
			if (dirty)
			{
				// TODO
				// Should RCB or mem-sync (inherit previous mem) to init memory
				LOG_TODO(RSX, "Resource used before memory initialization");
			}

			// Tags are tested in an X pattern
			for (const auto &tag : memory_tag_samples)
			{
				if (!tag.first)
					break;

				if (tag.second != *reinterpret_cast<u64*>(vm::g_sudo_addr + tag.first))
					return false;
			}

			return true;
		}

		void clear_rw_barrier()
		{
			release_ref(old_contents.source);
			old_contents = {};
		}

		template<typename T>
		void set_old_contents(T* other)
		{
			verify(HERE), !old_contents;

			if (!other || other->get_rsx_pitch() != this->get_rsx_pitch())
			{
				old_contents = {};
				return;
			}

			old_contents = {};
			old_contents.source = other;
			other->add_ref();
		}

		template<typename T>
		void set_old_contents_region(const T& region, bool normalized)
		{
			if (old_contents)
			{
				// This can happen when doing memory splits
				auto old_surface = static_cast<decltype(region.source)>(old_contents.source);
				if (old_surface->last_use_tag > region.source->last_use_tag)
				{
					return;
				}

				clear_rw_barrier();
			}

			// NOTE: This method will not perform pitch verification!
			verify(HERE), !old_contents, region.source, region.source != this;

			old_contents = region.template cast<image_storage_type>();
			region.source->add_ref();

			// Reverse normalization process if needed
			if (normalized)
			{
				const u16 bytes_to_texels_x = region.source->get_bpp() * (region.source->write_aa_mode == rsx::surface_antialiasing::center_1_sample? 1 : 2);
				const u16 rows_to_texels_y = (region.source->write_aa_mode > rsx::surface_antialiasing::diagonal_centered_2_samples? 2 : 1);
				old_contents.src_x /= bytes_to_texels_x;
				old_contents.src_y /= rows_to_texels_y;
				old_contents.width /= bytes_to_texels_x;
				old_contents.height /= rows_to_texels_y;

				const u16 bytes_to_texels_x2 = (get_bpp() * (write_aa_mode == rsx::surface_antialiasing::center_1_sample? 1 : 2));
				const u16 rows_to_texels_y2 = (write_aa_mode > rsx::surface_antialiasing::diagonal_centered_2_samples)? 2 : 1;
				old_contents.dst_x /= bytes_to_texels_x2;
				old_contents.dst_y /= rows_to_texels_y2;

				old_contents.transfer_scale_x = f32(bytes_to_texels_x) / bytes_to_texels_x2;
				old_contents.transfer_scale_y = f32(rows_to_texels_y) / rows_to_texels_y2;
			}

			// Apply resolution scale if needed
			if (g_cfg.video.resolution_scale_percent != 100)
			{
				auto src_width = rsx::apply_resolution_scale(old_contents.width, true, old_contents.source->width());
				auto src_height = rsx::apply_resolution_scale(old_contents.height, true, old_contents.source->height());

				auto dst_width = rsx::apply_resolution_scale(old_contents.width, true, old_contents.target->width());
				auto dst_height = rsx::apply_resolution_scale(old_contents.height, true, old_contents.target->height());

				old_contents.transfer_scale_x *= f32(dst_width) / src_width;
				old_contents.transfer_scale_y *= f32(dst_height) / src_height;

				old_contents.width = src_width;
				old_contents.height = src_height;

				old_contents.src_x = rsx::apply_resolution_scale(old_contents.src_x, false, old_contents.source->width());
				old_contents.src_y = rsx::apply_resolution_scale(old_contents.src_y, false, old_contents.source->height());
				old_contents.dst_x = rsx::apply_resolution_scale(old_contents.dst_x, false, old_contents.target->width());
				old_contents.dst_y = rsx::apply_resolution_scale(old_contents.dst_y, false, old_contents.target->height());
			}
		}

		void queue_tag(u32 address)
		{
			for (int i = 0; i < memory_tag_samples.size(); ++i)
			{
				if (LIKELY(i))
					memory_tag_samples[i].first = 0;
				else
					memory_tag_samples[i].first = address; // Top left
			}

			const u32 pitch = get_native_pitch();
			if (UNLIKELY(pitch < 16))
			{
				// Not enough area to gather samples if pitch is too small
				return;
			}

			// Top right corner
			memory_tag_samples[1].first = address + pitch - 8;

			if (const u32 h = get_surface_height(); h > 1)
			{
				// Last row
				const u32 pitch2 = get_rsx_pitch();
				const u32 last_row_offset = pitch2 * (h - 1);
				memory_tag_samples[2].first = address + last_row_offset;              // Bottom left corner
				memory_tag_samples[3].first = address + last_row_offset + pitch - 8;  // Bottom right corner

				// Centroid
				const u32 center_row_offset = pitch2 * (h / 2);
				memory_tag_samples[4].first = address + center_row_offset + pitch / 2;
			}
		}

		void sync_tag()
		{
			for (auto &tag : memory_tag_samples)
			{
				if (!tag.first)
					break;

				tag.second = *reinterpret_cast<u64*>(vm::g_sudo_addr + tag.first);
			}
		}

		void on_write(u64 write_tag = 0)
		{
			if (write_tag)
			{
				// Update use tag if requested
				last_use_tag = write_tag;
			}

			// Tag unconditionally without introducing new data
			sync_tag();

			read_aa_mode = write_aa_mode;
			dirty = false;

			if (old_contents.source)
			{
				clear_rw_barrier();
			}
		}

		// Returns the rect area occupied by this surface expressed as an 8bpp image with no AA
		areau get_normalized_memory_area() const
		{
			const u16 internal_width = get_native_pitch() * (write_aa_mode > rsx::surface_antialiasing::center_1_sample? 2: 1);
			const u16 internal_height = get_surface_height() * (write_aa_mode > rsx::surface_antialiasing::diagonal_centered_2_samples? 2: 1);

			return { 0, 0, internal_width, internal_height };
		}

		rsx::address_range get_memory_range() const
		{
			const u32 internal_height = get_surface_height() * (write_aa_mode > rsx::surface_antialiasing::diagonal_centered_2_samples? 2: 1);
			return rsx::address_range::start_length(memory_tag_samples[0].first, internal_height * get_rsx_pitch());
		}
	};

	/**
	 * Helper for surface (ie color and depth stencil render target) management.
	 * It handles surface creation and storage. Backend should only retrieve pointer to surface.
	 * It provides 2 methods get_texture_from_*_if_applicable that should be used when an app
	 * wants to sample a previous surface.
	 * Please note that the backend is still responsible for creating framebuffer/descriptors
	 * and need to inform surface_store everytime surface format/size/addresses change.
	 *
	 * Since it's a template it requires a trait with the followings:
	 * - type surface_storage_type which is a structure containing texture.
	 * - type surface_type which is a pointer to storage_type or a reference.
	 * - type command_list_type that can be void for backend without command list
	 * - type download_buffer_object used by issue_download_command and map_downloaded_buffer functions to handle sync
	 *
	 * - a member function static surface_type(const surface_storage_type&) that returns underlying surface pointer from a storage type.
	 * - 2 member functions static surface_storage_type create_new_surface(u32 address, Surface_color_format/Surface_depth_format format, size_t width, size_t height,...)
	 * used to create a new surface_storage_type holding surface from passed parameters.
	 * - a member function static prepare_rtt_for_drawing(command_list, surface_type) that makes a sampleable surface a color render target one.
	 * - a member function static prepare_rtt_for_drawing(command_list, surface_type) that makes a render target surface a sampleable one.
	 * - a member function static prepare_ds_for_drawing that does the same for depth stencil surface.
	 * - a member function static prepare_ds_for_sampling that does the same for depth stencil surface.
	 * - a member function static bool rtt_has_format_width_height(const surface_storage_type&, Surface_color_format surface_color_format, size_t width, size_t height)
	 * that checks if the given surface has the given format and size
	 * - a member function static bool ds_has_format_width_height that does the same for ds
	 * - a member function static download_buffer_object issue_download_command(surface_type, Surface_color_format color_format, size_t width, size_t height,...)
	 * that generates command to download the given surface to some mappable buffer.
	 * - a member function static issue_depth_download_command that does the same for depth surface
	 * - a member function static issue_stencil_download_command that does the same for stencil surface
	 * - a member function gsl::span<const gsl::byte> map_downloaded_buffer(download_buffer_object, ...) that maps a download_buffer_object
	 * - a member function static unmap_downloaded_buffer that unmaps it.
	 */
	template<typename Traits>
	struct surface_store
	{
		template<typename T, typename U>
		void copy_pitched_src_to_dst(gsl::span<T> dest, gsl::span<const U> src, size_t src_pitch_in_bytes, size_t width, size_t height)
		{
			for (int row = 0; row < height; row++)
			{
				for (unsigned col = 0; col < width; col++)
					dest[col] = src[col];
				src = src.subspan(src_pitch_in_bytes / sizeof(U));
				dest = dest.subspan(width);
			}
		}

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
		using download_buffer_object = typename Traits::download_buffer_object;
		using surface_overlap_info = surface_overlap_info_t<surface_type>;

	protected:
		std::unordered_map<u32, surface_storage_type> m_render_targets_storage = {};
		std::unordered_map<u32, surface_storage_type> m_depth_stencil_storage = {};

		rsx::address_range m_render_targets_memory_range;
		rsx::address_range m_depth_stencil_memory_range;

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
#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
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
				const u32 bytes_to_texels_x = (bpp * get_aa_factor_u(prev_surface->write_aa_mode));

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
				const u32 bytes_to_texels_x = (bpp * get_aa_factor_u(prev_surface->write_aa_mode));

				deferred_clipped_region<surface_type> copy;
				copy.src_x = 0;
				copy.src_y = _new.height / get_aa_factor_v(prev_surface->write_aa_mode);
				copy.dst_x = 0;
				copy.dst_y = 0;
				copy.width = std::min(_new.width, old.width) / bytes_to_texels_x;
				copy.height = (old.height - _new.height) / get_aa_factor_v(prev_surface->write_aa_mode);
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
#endif
		}

		template <bool is_depth_surface>
		void intersect_surface_region(command_list_type cmd, u32 address, surface_type new_surface, surface_type prev_surface)
		{
#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
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
						e.second->dirty)
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
				std::pair<u32, surface_type> e = { address, prev_surface };
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
				new_surface->dirty = true;
				break;
			}
#endif
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
			// TODO: Fix corner cases
			// This doesn't take overlapping surface(s) into account.
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			bool store = true;

			// Check if render target already exists
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
			{
				surface_storage_type &rtt = It->second;
				const bool pitch_compatible = Traits::surface_is_pitch_compatible(rtt, pitch);

				if (pitch_compatible)
				{
					// Preserve memory outside the area to be inherited if needed
					const u8 bpp = get_format_block_size_in_bytes(color_format);
					split_surface_region<false>(command_list, address, Traits::get(rtt), (u16)width, (u16)height, bpp, antialias);
				}

				if (Traits::rtt_has_format_width_height(rtt, color_format, width, height))
				{
					if (pitch_compatible)
						Traits::notify_surface_persist(rtt);
					else
						Traits::invalidate_surface_contents(command_list, Traits::get(rtt), address, pitch);

					Traits::prepare_rtt_for_drawing(command_list, Traits::get(rtt));
					new_surface = Traits::get(rtt);
					store = false;
				}
				else
				{
					old_surface = Traits::get(rtt);
					old_surface_storage = std::move(rtt);
					m_render_targets_storage.erase(address);
				}
			}

			if (!new_surface)
			{
				// Range test
				const auto aa_factor_v = get_aa_factor_v(antialias);
				rsx::address_range range = rsx::address_range::start_length(address, u32(pitch * height * aa_factor_v));
				m_render_targets_memory_range = range.get_min_max(m_render_targets_memory_range);

				// Search invalidated resources for a suitable surface
				for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
				{
					auto &rtt = *It;
					if (Traits::rtt_has_format_width_height(rtt, color_format, width, height, true))
					{
						new_surface_storage = std::move(rtt);
#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
						Traits::notify_surface_reused(new_surface_storage);
#endif
						if (old_surface)
						{
							// Exchange this surface with the invalidated one
							Traits::notify_surface_invalidated(old_surface_storage);
							rtt = std::move(old_surface_storage);
						}
						else
						{
							// rtt is now empty - erase it
							invalidated_resources.erase(It);
						}

						new_surface = Traits::get(new_surface_storage);
						Traits::invalidate_surface_contents(command_list, new_surface, address, pitch);
						Traits::prepare_rtt_for_drawing(command_list, new_surface);
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
				new_surface_storage = Traits::create_new_surface(address, color_format, width, height, pitch, std::forward<Args>(extra_params)...);
				new_surface = Traits::get(new_surface_storage);
			}

#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
			// Check if old_surface is 'new' and avoid intersection
			if (old_surface && old_surface->last_use_tag >= write_tag)
			{
				new_surface->set_old_contents(old_surface);
			}
			else
#endif
			{
				intersect_surface_region<false>(command_list, address, new_surface, old_surface);
			}

			if (store)
			{
				// New surface was found among invalidated surfaces
				m_render_targets_storage[address] = std::move(new_surface_storage);
			}

			// Remove and preserve if possible any overlapping/replaced depth surface
			auto aliased_depth_surface = m_depth_stencil_storage.find(address);
			if (aliased_depth_surface != m_depth_stencil_storage.end())
			{
				if (Traits::surface_is_pitch_compatible(aliased_depth_surface->second, pitch))
				{
					// Preserve memory outside the area to be inherited if needed
					const u8 bpp = get_format_block_size_in_bytes(color_format);
					split_surface_region<true>(command_list, address, Traits::get(aliased_depth_surface->second), (u16)width, (u16)height, bpp, antialias);
				}

				Traits::notify_surface_invalidated(aliased_depth_surface->second);
				invalidated_resources.push_back(std::move(aliased_depth_surface->second));
				m_depth_stencil_storage.erase(aliased_depth_surface);
			}

			return new_surface;
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
			surface_storage_type old_surface_storage;
			surface_storage_type new_surface_storage;
			surface_type old_surface = nullptr;
			surface_type new_surface = nullptr;
			bool store = true;

			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
			{
				surface_storage_type &ds = It->second;
				const bool pitch_compatible = Traits::surface_is_pitch_compatible(ds, pitch);

				if (pitch_compatible)
				{
					const u8 bpp = (depth_format == rsx::surface_depth_format::z16)? 2 : 4;
					split_surface_region<true>(command_list, address, Traits::get(ds), (u16)width, (u16)height, bpp, antialias);
				}

				if (Traits::ds_has_format_width_height(ds, depth_format, width, height))
				{
					if (pitch_compatible)
						Traits::notify_surface_persist(ds);
					else
						Traits::invalidate_surface_contents(command_list, Traits::get(ds), address, pitch);

					Traits::prepare_ds_for_drawing(command_list, Traits::get(ds));
					new_surface = Traits::get(ds);
					store = false;
				}
				else
				{
					old_surface = Traits::get(ds);
					old_surface_storage = std::move(ds);
					m_depth_stencil_storage.erase(address);
				}
			}

			if (!new_surface)
			{
				// Range test
				const auto aa_factor_v = get_aa_factor_v(antialias);
				rsx::address_range range = rsx::address_range::start_length(address, u32(pitch * height * aa_factor_v));
				m_depth_stencil_memory_range = range.get_min_max(m_depth_stencil_memory_range);

				//Search invalidated resources for a suitable surface
				for (auto It = invalidated_resources.begin(); It != invalidated_resources.end(); It++)
				{
					auto &ds = *It;
					if (Traits::ds_has_format_width_height(ds, depth_format, width, height, true))
					{
						new_surface_storage = std::move(ds);
#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
						Traits::notify_surface_reused(new_surface_storage);
#endif
						if (old_surface)
						{
							//Exchange this surface with the invalidated one
							Traits::notify_surface_invalidated(old_surface_storage);
							ds = std::move(old_surface_storage);
						}
						else
							invalidated_resources.erase(It);

						new_surface = Traits::get(new_surface_storage);
						Traits::prepare_ds_for_drawing(command_list, new_surface);
						Traits::invalidate_surface_contents(command_list, new_surface, address, pitch);
						break;
					}
				}
			}

			if (old_surface != nullptr && new_surface == nullptr)
			{
				// This was already determined to be invalid and is excluded from testing above
				Traits::notify_surface_invalidated(old_surface_storage);
				invalidated_resources.push_back(std::move(old_surface_storage));
			}

			if (!new_surface)
			{
				verify(HERE), store;
				new_surface_storage = Traits::create_new_surface(address, depth_format, width, height, pitch, std::forward<Args>(extra_params)...);
				new_surface = Traits::get(new_surface_storage);
			}

#ifndef INCOMPLETE_SURFACE_CACHE_IMPL
			// Check if old_surface is 'new' and avoid intersection
			if (old_surface && old_surface->last_use_tag >= write_tag)
			{
				new_surface->set_old_contents(old_surface);
			}
			else
#endif
			{
				intersect_surface_region<true>(command_list, address, new_surface, old_surface);
			}

			if (store)
			{
				// New surface was found among invalidated surfaces
				m_depth_stencil_storage[address] = std::move(new_surface_storage);
			}

			// Remove and preserve if possible any overlapping/replaced color surface
			auto aliased_rtt_surface = m_render_targets_storage.find(address);
			if (aliased_rtt_surface != m_render_targets_storage.end())
			{
				if (Traits::surface_is_pitch_compatible(aliased_rtt_surface->second, pitch))
				{
					const u8 bpp = (depth_format == rsx::surface_depth_format::z16) ? 2 : 4;
					split_surface_region<false>(command_list, address, Traits::get(aliased_rtt_surface->second), (u16)width, (u16)height, bpp, antialias);
				}

				Traits::notify_surface_invalidated(aliased_rtt_surface->second);
				invalidated_resources.push_back(std::move(aliased_rtt_surface->second));
				m_render_targets_storage.erase(aliased_rtt_surface);
			}

			return new_surface;
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
//			u32 clip_x = clip_horizontal_reg;
//			u32 clip_y = clip_vertical_reg;

			cache_tag = rsx::get_shared_tag();

			// Make previous RTTs sampleable
			for (int i = m_bound_render_targets_config.first, count = 0;
				count < m_bound_render_targets_config.second;
				++i, ++count)
			{
				auto &rtt = m_bound_render_targets[i];
				Traits::prepare_rtt_for_sampling(command_list, std::get<1>(rtt));
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
				Traits::prepare_ds_for_sampling(command_list, std::get<1>(m_bound_depth_stencil));

			m_bound_depth_stencil = std::make_pair(0, nullptr);

			if (!address_z)
				return;

			m_bound_depth_stencil = std::make_pair(address_z,
				bind_address_as_depth_stencil(command_list, address_z, depth_format, antialias,
					clip_width, clip_height, zeta_pitch, std::forward<Args>(extra_params)...));
		}

		/**
		 * Search for given address in stored color surface
		 * Return an empty surface_type otherwise.
		 */
		surface_type get_texture_from_render_target_if_applicable(u32 address)
		{
			auto It = m_render_targets_storage.find(address);
			if (It != m_render_targets_storage.end())
				return Traits::get(It->second);
			return surface_type();
		}

		/**
		* Search for given address in stored depth stencil surface
		* Return an empty surface_type otherwise.
		*/
		surface_type get_texture_from_depth_stencil_if_applicable(u32 address)
		{
			auto It = m_depth_stencil_storage.find(address);
			if (It != m_depth_stencil_storage.end())
				return Traits::get(It->second);
			return surface_type();
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

		/**
		 * Get bound color surface raw data.
		 */
		template <typename... Args>
		std::array<std::vector<gsl::byte>, 4> get_render_targets_data(
			surface_color_format color_format, size_t width, size_t height,
			Args&& ...args
			)
		{
			std::array<download_buffer_object, 4> download_data = {};

			// Issue download commands
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				surface_type surface_resource = std::get<1>(m_bound_render_targets[i]);
				download_data[i] = std::move(
					Traits::issue_download_command(surface_resource, color_format, width, height, std::forward<Args&&>(args)...)
					);
			}

			std::array<std::vector<gsl::byte>, 4> result = {};

			// Sync and copy data
			for (int i = 0; i < 4; i++)
			{
				if (std::get<0>(m_bound_render_targets[i]) == 0)
					continue;

				gsl::span<const gsl::byte> raw_src = Traits::map_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);

				size_t src_pitch = utility::get_aligned_pitch(color_format, ::narrow<u32>(width));
				size_t dst_pitch = utility::get_packed_pitch(color_format, ::narrow<u32>(width));

				result[i].resize(dst_pitch * height);

				// Note: MSVC + GSL doesn't support span<byte> -> span<T> for non const span atm
				// thus manual conversion
				switch (color_format)
				{
				case surface_color_format::a8b8g8r8:
				case surface_color_format::x8b8g8r8_o8b8g8r8:
				case surface_color_format::x8b8g8r8_z8b8g8r8:
				case surface_color_format::a8r8g8b8:
				case surface_color_format::x8r8g8b8_o8r8g8b8:
				case surface_color_format::x8r8g8b8_z8r8g8b8:
				case surface_color_format::x32:
				{
					gsl::span<be_t<u32>> dst_span{ (be_t<u32>*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(be_t<u32>)) };
					copy_pitched_src_to_dst(dst_span, as_const_span<const u32>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::b8:
				{
					gsl::span<u8> dst_span{ (u8*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u8)) };
					copy_pitched_src_to_dst(dst_span, as_const_span<const u8>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::g8b8:
				case surface_color_format::r5g6b5:
				case surface_color_format::x1r5g5b5_o1r5g5b5:
				case surface_color_format::x1r5g5b5_z1r5g5b5:
				{
					gsl::span<be_t<u16>> dst_span{ (be_t<u16>*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(be_t<u16>)) };
					copy_pitched_src_to_dst(dst_span, as_const_span<const u16>(raw_src), src_pitch, width, height);
					break;
				}
				// Note : may require some big endian swap
				case surface_color_format::w32z32y32x32:
				{
					gsl::span<u128> dst_span{ (u128*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u128)) };
					copy_pitched_src_to_dst(dst_span, as_const_span<const u128>(raw_src), src_pitch, width, height);
					break;
				}
				case surface_color_format::w16z16y16x16:
				{
					gsl::span<u64> dst_span{ (u64*)result[i].data(), ::narrow<int>(dst_pitch * height / sizeof(u64)) };
					copy_pitched_src_to_dst(dst_span, as_const_span<const u64>(raw_src), src_pitch, width, height);
					break;
				}

				}
				Traits::unmap_downloaded_buffer(download_data[i], std::forward<Args&&>(args)...);
			}
			return result;
		}

		/**
		 * Get bound color surface raw data.
		 */
		template <typename... Args>
		std::array<std::vector<gsl::byte>, 2> get_depth_stencil_data(
			surface_depth_format depth_format, size_t width, size_t height,
			Args&& ...args
			)
		{
			std::array<std::vector<gsl::byte>, 2> result = {};
			if (std::get<0>(m_bound_depth_stencil) == 0)
				return result;
			size_t row_pitch = align(width * 4, 256);

			download_buffer_object stencil_data = {};
			download_buffer_object depth_data = Traits::issue_depth_download_command(std::get<1>(m_bound_depth_stencil), depth_format, width, height, std::forward<Args&&>(args)...);
			if (depth_format == surface_depth_format::z24s8)
				stencil_data = std::move(Traits::issue_stencil_download_command(std::get<1>(m_bound_depth_stencil), width, height, std::forward<Args&&>(args)...));

			gsl::span<const gsl::byte> depth_buffer_raw_src = Traits::map_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);
			if (depth_format == surface_depth_format::z16)
			{
				result[0].resize(width * height * 2);
				gsl::span<u16> dest{ (u16*)result[0].data(), ::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, as_const_span<const u16>(depth_buffer_raw_src), row_pitch, width, height);
			}
			if (depth_format == surface_depth_format::z24s8)
			{
				result[0].resize(width * height * 4);
				gsl::span<u32> dest{ (u32*)result[0].data(), ::narrow<int>(width * height) };
				copy_pitched_src_to_dst(dest, as_const_span<const u32>(depth_buffer_raw_src), row_pitch, width, height);
			}
			Traits::unmap_downloaded_buffer(depth_data, std::forward<Args&&>(args)...);

			if (depth_format == surface_depth_format::z16)
				return result;

			gsl::span<const gsl::byte> stencil_buffer_raw_src = Traits::map_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			result[1].resize(width * height);
			gsl::span<u8> dest{ (u8*)result[1].data(), ::narrow<int>(width * height) };
			copy_pitched_src_to_dst(dest, as_const_span<const u8>(stencil_buffer_raw_src), align(width, 256), width, height);
			Traits::unmap_downloaded_buffer(stencil_data, std::forward<Args&&>(args)...);
			return result;
		}

		/**
		 * Moves a single surface from surface storage to invalidated surface store.
		 * Can be triggered by the texture cache's blit functionality when formats do not match
		 */
		void invalidate_single_surface(surface_type surface, bool depth)
		{
			if (!depth)
			{
				for (auto It = m_render_targets_storage.begin(); It != m_render_targets_storage.end(); It++)
				{
					const auto address = It->first;
					const auto ref = Traits::get(It->second);

					if (surface == ref)
					{
						Traits::notify_surface_invalidated(It->second);
						invalidated_resources.push_back(std::move(It->second));
						m_render_targets_storage.erase(It);

						cache_tag = rsx::get_shared_tag();
						return;
					}
				}
			}
			else
			{
				for (auto It = m_depth_stencil_storage.begin(); It != m_depth_stencil_storage.end(); It++)
				{
					const auto address = It->first;
					const auto ref = Traits::get(It->second);

					if (surface == ref)
					{
						Traits::notify_surface_invalidated(It->second);
						invalidated_resources.push_back(std::move(It->second));
						m_depth_stencil_storage.erase(It);

						cache_tag = rsx::get_shared_tag();
						return;
					}
				}
			}
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
					auto this_address = std::get<0>(tex_info);
					if (this_address >= limit)
						continue;

					auto surface = std::get<1>(tex_info).get();
					const auto pitch = surface->get_rsx_pitch();
					if (!rsx::pitch_compatible(surface, required_pitch, required_height))
						continue;

					const u16 scale_x = surface->read_aa_mode > rsx::surface_antialiasing::center_1_sample? 2 : 1;
					const u16 scale_y = surface->read_aa_mode > rsx::surface_antialiasing::diagonal_centered_2_samples? 2 : 1;
					const auto texture_size = pitch * surface->get_surface_height() * scale_y;

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

					surface_format_info surface_info{};
					Traits::get_surface_info(surface, &surface_info);

					const auto normalized_surface_width = (surface_info.surface_width * scale_x * surface_info.bpp) / required_bpp;
					const auto normalized_surface_height = surface_info.surface_height * scale_y;

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

					if (UNLIKELY(surface_info.bpp != required_bpp))
					{
						// Width is calculated in the coordinate-space of the requester; normalize
						info.src_x = (info.src_x * required_bpp) / surface_info.bpp;
						info.width = (info.width * required_bpp) / surface_info.bpp;
					}

					if (UNLIKELY(scale_x > 1))
					{
						info.src_x /= scale_x;
						info.dst_x /= scale_x;
						info.width /= scale_x;
						info.src_y /= scale_y;
						info.dst_y /= scale_y;
						info.height /= scale_y;
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
					// Nothing to do
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
