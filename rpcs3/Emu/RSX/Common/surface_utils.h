#pragma once

#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "Utilities/address_range.h"
#include "TextureUtils.h"
#include "../rsx_utils.h"

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
		std::array<std::pair<u32, u64>, 5> memory_tag_samples;

		// Obsolete, requires updating
		deferred_clipped_region<image_storage_type> old_contents{};

		// Surface properties
		u16 rsx_pitch = 0;
		u16 native_pitch = 0;
		u16 surface_width = 0;
		u16 surface_height = 0;
		u8  spp = 1;
		u8  samples_x = 1;
		u8  samples_y = 1;

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
			if (old_contents)
			{
				// Cascade resource derefs
				LOG_ERROR(RSX, "Resource was destroyed whilst holding a resource reference!");
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
					fmt::throw_exception("Unknown AA mode 0x%x", (u32)aa);
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

		rsx::surface_color_format get_surface_color_format()
		{
			return format_info.gcm_color_format;
		}

		rsx::surface_depth_format get_surface_depth_format()
		{
			return format_info.gcm_depth_format;
		}

		bool dirty() const
		{
			return (state_flags != rsx::surface_state_flags::ready) || old_contents;
		}

		bool test() const
		{
			if (dirty())
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
				const u16 bytes_to_texels_x = region.source->get_bpp() * region.source->samples_x;
				const u16 rows_to_texels_y = region.source->samples_y;
				old_contents.src_x /= bytes_to_texels_x;
				old_contents.src_y /= rows_to_texels_y;
				old_contents.width /= bytes_to_texels_x;
				old_contents.height /= rows_to_texels_y;

				const u16 bytes_to_texels_x2 = (get_bpp() * samples_x);
				const u16 rows_to_texels_y2 = samples_y;
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
			for (unsigned i = 0; i < memory_tag_samples.size(); ++i)
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

			if (old_contents.source)
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
			return rsx::address_range::start_length(memory_tag_samples[0].first, internal_height * get_rsx_pitch());
		}

		template <typename T>
		void transform_samples_to_pixels(area_base<T>& area)
		{
			if (LIKELY(spp == 1)) return;

			area.x1 /= samples_x;
			area.x2 /= samples_x;
			area.y1 /= samples_y;
			area.y2 /= samples_y;
		}

		template <typename T>
		void transform_pixels_to_samples(area_base<T>& area)
		{
			if (LIKELY(spp == 1)) return;

			area.x1 *= samples_x;
			area.x2 *= samples_x;
			area.y1 *= samples_y;
			area.y2 *= samples_y;
		}

		template <typename T>
		void transform_samples_to_pixels(T& x1, T& x2, T& y1, T& y2)
		{
			if (LIKELY(spp == 1)) return;

			x1 /= samples_x;
			x2 /= samples_x;
			y1 /= samples_y;
			y2 /= samples_y;
		}

		template <typename T>
		void transform_pixels_to_samples(T& x1, T& x2, T& y1, T& y2)
		{
			if (LIKELY(spp == 1)) return;

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