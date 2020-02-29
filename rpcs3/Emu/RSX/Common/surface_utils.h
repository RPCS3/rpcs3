#pragma once

#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "Utilities/address_range.h"
#include "TextureUtils.h"
#include "../rsx_utils.h"

#define ENABLE_SURFACE_CACHE_DEBUG 0

namespace rsx
{
	enum surface_state_flags : u32
	{
		ready = 0,
		erase_bkgnd = 1,
		require_resolve = 2,
		require_unresolve = 4
	};

	enum surface_sample_layout : u32
	{
		null = 0,
		ps3 = 1
	};

	template <typename surface_type>
	struct surface_overlap_info_t
	{
		surface_type surface = nullptr;
		u32 base_address = 0;
		bool is_depth = false;
		bool is_clipped = false;

		coordu src_area;
		coordu dst_area;
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
			ret.target = static_cast<T>(target);
			ret.source = static_cast<T>(source);

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

				auto src_w = std::get<0>(region);
				auto src_h = std::get<1>(region);
				auto dst_w = std::get<2>(region);
				auto dst_h = std::get<3>(region);

				// Apply resolution scale if needed
				if (g_cfg.video.resolution_scale_percent != 100)
				{
					auto src = static_cast<T>(source);
					src_w = rsx::apply_resolution_scale(src_w, true, src->get_surface_width(rsx::surface_metrics::pixels));
					src_h = rsx::apply_resolution_scale(src_h, true, src->get_surface_height(rsx::surface_metrics::pixels));
					dst_w = rsx::apply_resolution_scale(dst_w, true, target_surface->get_surface_width(rsx::surface_metrics::pixels));
					dst_h = rsx::apply_resolution_scale(dst_h, true, target_surface->get_surface_height(rsx::surface_metrics::pixels));
				}

				width = src_w;
				height = src_h;
				transfer_scale_x = f32(dst_w) / src_w;
				transfer_scale_y = f32(dst_h) / src_h;

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
		u32 base_addr = 0;

#if (ENABLE_SURFACE_CACHE_DEBUG)
		u64 memory_hash = 0;
#else
		std::array<std::pair<u32, u64>, 3> memory_tag_samples;
#endif

		std::vector<deferred_clipped_region<image_storage_type>> old_contents;

		// Surface properties
		u16 rsx_pitch = 0;
		u16 native_pitch = 0;
		u16 surface_width = 0;
		u16 surface_height = 0;
		u8  spp = 1;
		u8  samples_x = 1;
		u8  samples_y = 1;

		format_type format_class = format_type::color;

		std::unique_ptr<typename std::remove_pointer<image_storage_type>::type> resolve_surface;
		surface_sample_layout sample_layout = surface_sample_layout::null;

		flags32_t memory_usage_flags = surface_usage_flags::unknown;
		flags32_t state_flags = surface_state_flags::ready;
		flags32_t msaa_flags = surface_state_flags::ready;
		flags32_t stencil_init_flags = 0;

		union
		{
			rsx::surface_color_format gcm_color_format;
			rsx::surface_depth_format gcm_depth_format;
		}
		format_info;

		render_target_descriptor() {}

		virtual ~render_target_descriptor()
		{
			if (!old_contents.empty())
			{
				// Cascade resource derefs
				rsx_log.error("Resource was destroyed whilst holding a resource reference!");
			}
		}

		virtual image_storage_type get_surface(rsx::surface_access access_type) = 0;
		virtual bool is_depth_surface() const = 0;
		virtual void release_ref(image_storage_type) const = 0;

		virtual u16 get_surface_width(rsx::surface_metrics metrics = rsx::surface_metrics::pixels) const
		{
			switch (metrics)
			{
				case rsx::surface_metrics::samples:
					return surface_width * samples_x;
				case rsx::surface_metrics::pixels:
					return surface_width;
				case rsx::surface_metrics::bytes:
					return native_pitch;
				default:
					fmt::throw_exception("Unknown surface metric %d", u32(metrics));
			}
		}

		virtual u16 get_surface_height(rsx::surface_metrics metrics = rsx::surface_metrics::pixels) const
		{
			switch (metrics)
			{
				case rsx::surface_metrics::samples:
				case rsx::surface_metrics::bytes:
					return surface_height * samples_y;
				case rsx::surface_metrics::pixels:
					return surface_height;
				default:
					fmt::throw_exception("Unknown surface metric %d", u32(metrics));
			}
		}

		virtual u16 get_rsx_pitch() const
		{
			return rsx_pitch;
		}

		virtual u16 get_native_pitch() const
		{
			return native_pitch;
		}

		u8 get_bpp() const
		{
			return u8(get_native_pitch() / get_surface_width(rsx::surface_metrics::samples));
		}

		u8 get_spp() const
		{
			return spp;
		}

		void set_aa_mode(rsx::surface_antialiasing aa)
		{
			switch (aa)
			{
				case rsx::surface_antialiasing::center_1_sample:
					samples_x = samples_y = spp = 1;
					break;
				case rsx::surface_antialiasing::diagonal_centered_2_samples:
					samples_x = spp = 2;
					samples_y = 1;
					break;
				case rsx::surface_antialiasing::square_centered_4_samples:
				case rsx::surface_antialiasing::square_rotated_4_samples:
					samples_x = samples_y = 2;
					spp = 4;
					break;
				default:
					fmt::throw_exception("Unknown AA mode 0x%x", static_cast<u8>(aa));
			}
		}

		void set_spp(u8 count)
		{
			switch (count)
			{
			case 1:
				samples_x = samples_y = spp = 1;
				break;
			case 2:
				samples_x = spp = 2;
				samples_y = 1;
				break;
			case 4:
				samples_x = samples_y = 2;
				spp = 4;
				break;
			default:
				fmt::throw_exception("Unexpected sample count 0x%x", count);
			}
		}

		void set_format(rsx::surface_color_format format)
		{
			format_info.gcm_color_format = format;
		}

		void set_format(rsx::surface_depth_format format)
		{
			format_info.gcm_depth_format = format;
		}

		void set_depth_render_mode(bool integer)
		{
			if (is_depth_surface())
			{
				format_class = (integer) ? format_type::depth_uint : format_type::depth_float;
			}
		}

		rsx::surface_color_format get_surface_color_format() const
		{
			return format_info.gcm_color_format;
		}

		rsx::surface_depth_format get_surface_depth_format() const
		{
			return format_info.gcm_depth_format;
		}

		rsx::format_type get_format_type() const
		{
			return format_class;
		}

		bool dirty() const
		{
			return (state_flags != rsx::surface_state_flags::ready) || !old_contents.empty();
		}

		bool write_through() const
		{
			return (state_flags & rsx::surface_state_flags::erase_bkgnd) && old_contents.empty();
		}

#if (ENABLE_SURFACE_CACHE_DEBUG)
		u64 hash_block() const
		{
			const auto padding = (rsx_pitch - native_pitch) / 8;
			const auto row_length = (native_pitch) / 8;
			auto num_rows = (surface_height * samples_y);
			auto ptr = reinterpret_cast<u64*>(vm::g_sudo_addr + base_addr);

			auto col = row_length;
			u64 result = 0;

			while (num_rows--)
			{
				while (col--)
				{
					result ^= *ptr++;
				}

				ptr += padding;
				col = row_length;
			}

			return result;
		}

		void queue_tag(u32 address)
		{
			base_addr = address;
		}

		void sync_tag()
		{
			memory_hash = hash_block();
		}

		void shuffle_tag()
		{
			memory_hash = ~memory_hash;
		}

		bool test() const
		{
			return hash_block() == memory_hash;
		}

#else
		void queue_tag(u32 address)
		{
			verify(HERE), native_pitch, rsx_pitch;

			base_addr = address;

			const u32 size_x = (native_pitch > 8)? (native_pitch - 8) : 0u;
			const u32 size_y = u32(surface_height * samples_y) - 1u;
			const position2u samples[] =
			{
				// NOTE: Sorted by probability to catch dirty flag
				{0, 0},
				{size_x, size_y},
				{size_x / 2, size_y / 2},

				// Auxilliary, highly unlikely to ever catch anything
				// NOTE: Currently unused as length of samples is truncated to 3
				{size_x, 0},
				{0, size_y},
			};

			for (uint n = 0; n < memory_tag_samples.size(); ++n)
			{
				const auto sample_offset = (samples[n].y * rsx_pitch) + samples[n].x;
				memory_tag_samples[n].first = (sample_offset + base_addr);
			}
		}

		void sync_tag()
		{
			for (auto &e : memory_tag_samples)
			{
				e.second = *reinterpret_cast<u64*>(vm::g_sudo_addr + e.first);
			}
		}

		void shuffle_tag()
		{
			memory_tag_samples[0].second = ~memory_tag_samples[0].second;
		}

		bool test()
		{
			for (auto &e : memory_tag_samples)
			{
				if (e.second != *reinterpret_cast<u64*>(vm::g_sudo_addr + e.first))
					return false;
			}

			return true;
		}
#endif

		void clear_rw_barrier()
		{
			for (auto &e : old_contents)
			{
				release_ref(e.source);
			}

			old_contents.clear();
		}

		template <typename T>
		u32 prepare_rw_barrier_for_transfer(T *target)
		{
			if (old_contents.size() <= 1)
				return 0;

			// Sort here before doing transfers since surfaces may have been updated in the meantime
			std::sort(old_contents.begin(), old_contents.end(), [](auto& a, auto &b)
			{
				auto _a = static_cast<T*>(a.source);
				auto _b = static_cast<T*>(b.source);
				return (_a->last_use_tag < _b->last_use_tag);
			});

			// Try and optimize by omitting possible overlapped transfers
			for (size_t i = old_contents.size() - 1; i > 0 /* Intentional */; i--)
			{
				old_contents[i].init_transfer(target);

				const auto dst_area = old_contents[i].dst_rect();
				if (unsigned(dst_area.x2) == target->width() && unsigned(dst_area.y2) == target->height() &&
					!dst_area.x1 && !dst_area.y1)
				{
					// This transfer will overwrite everything older
					return u32(i);
				}
			}

			return 0;
		}

		template<typename T>
		void set_old_contents(T* other)
		{
			verify(HERE), old_contents.empty();

			if (!other || other->get_rsx_pitch() != this->get_rsx_pitch())
			{
				return;
			}

			old_contents.emplace_back();
			old_contents.back().source = other;
			other->add_ref();
		}

		template<typename T>
		void set_old_contents_region(const T& region, bool normalized)
		{
			// NOTE: This method will not perform pitch verification!
			verify(HERE), region.source, region.source != static_cast<decltype(region.source)>(this);

			old_contents.push_back(region.template cast<image_storage_type>());
			auto &slice = old_contents.back();
			region.source->add_ref();

			// Reverse normalization process if needed
			if (normalized)
			{
				const u16 bytes_to_texels_x = region.source->get_bpp() * region.source->samples_x;
				const u16 rows_to_texels_y = region.source->samples_y;
				slice.src_x /= bytes_to_texels_x;
				slice.src_y /= rows_to_texels_y;
				slice.width /= bytes_to_texels_x;
				slice.height /= rows_to_texels_y;

				const u16 bytes_to_texels_x2 = (get_bpp() * samples_x);
				const u16 rows_to_texels_y2 = samples_y;
				slice.dst_x /= bytes_to_texels_x2;
				slice.dst_y /= rows_to_texels_y2;

				slice.transfer_scale_x = f32(bytes_to_texels_x) / bytes_to_texels_x2;
				slice.transfer_scale_y = f32(rows_to_texels_y) / rows_to_texels_y2;
			}

			// Apply resolution scale if needed
			if (g_cfg.video.resolution_scale_percent != 100)
			{
				auto src_width = rsx::apply_resolution_scale(slice.width, true, slice.source->width());
				auto src_height = rsx::apply_resolution_scale(slice.height, true, slice.source->height());

				auto dst_width = rsx::apply_resolution_scale(slice.width, true, slice.target->width());
				auto dst_height = rsx::apply_resolution_scale(slice.height, true, slice.target->height());

				slice.transfer_scale_x *= f32(dst_width) / src_width;
				slice.transfer_scale_y *= f32(dst_height) / src_height;

				slice.width = src_width;
				slice.height = src_height;

				slice.src_x = rsx::apply_resolution_scale(slice.src_x, false, slice.source->width());
				slice.src_y = rsx::apply_resolution_scale(slice.src_y, false, slice.source->height());
				slice.dst_x = rsx::apply_resolution_scale(slice.dst_x, false, slice.target->width());
				slice.dst_y = rsx::apply_resolution_scale(slice.dst_y, false, slice.target->height());
			}
		}

		void on_write(u64 write_tag = 0, rsx::surface_state_flags resolve_flags = surface_state_flags::require_resolve)
		{
			if (write_tag)
			{
				// Update use tag if requested
				last_use_tag = write_tag;
			}

			// Tag unconditionally without introducing new data
			sync_tag();

			// HACK!! This should be cleared through memory barriers only
			state_flags = rsx::surface_state_flags::ready;

			if (spp > 1 && sample_layout != surface_sample_layout::null)
			{
				msaa_flags = resolve_flags;
			}

			if (!old_contents.empty())
			{
				clear_rw_barrier();
			}
		}

		void on_write_copy(u64 write_tag = 0, bool keep_optimizations = false)
		{
			on_write(write_tag, rsx::surface_state_flags::require_unresolve);

			if (!keep_optimizations && is_depth_surface())
			{
				// A successful write-copy occured, cannot guarantee flat contents in stencil area
				stencil_init_flags |= (1 << 9);
			}
		}

		void on_invalidate_children()
		{
			if (resolve_surface)
			{
				msaa_flags = rsx::surface_state_flags::require_resolve;
			}
		}

		// Returns the rect area occupied by this surface expressed as an 8bpp image with no AA
		areau get_normalized_memory_area() const
		{
			const u16 internal_width = get_surface_width(rsx::surface_metrics::bytes);
			const u16 internal_height = get_surface_height(rsx::surface_metrics::bytes);

			return { 0, 0, internal_width, internal_height };
		}

		rsx::address_range get_memory_range() const
		{
			const u32 internal_height = get_surface_height(rsx::surface_metrics::samples);
			const u32 excess = (rsx_pitch - native_pitch);
			return rsx::address_range::start_length(base_addr, internal_height * rsx_pitch - excess);
		}

		template <typename T>
		void transform_samples_to_pixels(area_base<T>& area)
		{
			if (spp == 1) [[likely]] return;

			area.x1 /= samples_x;
			area.x2 /= samples_x;
			area.y1 /= samples_y;
			area.y2 /= samples_y;
		}

		template <typename T>
		void transform_pixels_to_samples(area_base<T>& area)
		{
			if (spp == 1) [[likely]] return;

			area.x1 *= samples_x;
			area.x2 *= samples_x;
			area.y1 *= samples_y;
			area.y2 *= samples_y;
		}

		template <typename T>
		void transform_samples_to_pixels(T& x1, T& x2, T& y1, T& y2)
		{
			if (spp == 1) [[likely]] return;

			x1 /= samples_x;
			x2 /= samples_x;
			y1 /= samples_y;
			y2 /= samples_y;
		}

		template <typename T>
		void transform_pixels_to_samples(T& x1, T& x2, T& y1, T& y2)
		{
			if (spp == 1) [[likely]] return;

			x1 *= samples_x;
			x2 *= samples_x;
			y1 *= samples_y;
			y2 *= samples_y;
		}

		template<typename T>
		void transform_blit_coordinates(rsx::surface_access access_type, area_base<T>& region)
		{
			if (spp == 1 || sample_layout == rsx::surface_sample_layout::ps3)
				return;

			verify(HERE), access_type != rsx::surface_access::write;
			transform_samples_to_pixels(region);
		}
	};
}