#include "stdafx.h"

#include "Emu/RSX/RSXThread.h"
#include "Utilities/address_range.h"
#include "util/fnv_hash.hpp"

#include "texture_cache_utils.h"
#include "texture_cache_helpers.h"

namespace rsx
{
	constexpr u32 min_lockable_data_size = 4096; // Increasing this value has worse results even on systems with pages > 4k

	namespace helpers = rsx::texture_cache_helpers;

	void buffered_section::init_lockable_range(const address_range32& range)
	{
		locked_range = range.to_page_range();
		AUDIT((locked_range.start == page_start(range.start)) || (locked_range.start == next_page(range.start)));
		AUDIT(locked_range.end <= page_end(range.end));
		ensure(locked_range.is_page_range());
	}

	void buffered_section::reset(const address_range32& memory_range)
	{
		ensure(memory_range.valid() && locked == false);

		cpu_range = address_range32(memory_range);
		confirmed_range.invalidate();
		locked_range.invalidate();

		protection = utils::protection::rw;
		protection_strat = section_protection_strategy::lock;
		locked = false;

		init_lockable_range(cpu_range);

		if (memory_range.length() < min_lockable_data_size)
		{
			protection_strat = section_protection_strategy::hash;
			mem_hash = 0;
		}
	}

	void buffered_section::invalidate_range()
	{
		ensure(!locked);

		cpu_range.invalidate();
		confirmed_range.invalidate();
		locked_range.invalidate();
	}

	void buffered_section::protect(utils::protection new_prot, bool force)
	{
		if (new_prot == protection && !force) return;

		ensure(locked_range.is_page_range());
		AUDIT(!confirmed_range.valid() || confirmed_range.inside(cpu_range));

#ifdef TEXTURE_CACHE_DEBUG
		if (new_prot != protection || force)
		{
			if (locked && !force) // When force=true, it is the responsibility of the caller to remove this section from the checker refcounting
				tex_cache_checker.remove(locked_range, protection);
			if (new_prot != utils::protection::rw)
				tex_cache_checker.add(locked_range, new_prot);
		}
#endif // TEXTURE_CACHE_DEBUG

		if (new_prot == utils::protection::no)
		{
			// Override
			protection_strat = section_protection_strategy::lock;
		}

		if (protection_strat == section_protection_strategy::lock)
		{
			rsx::memory_protect(locked_range, new_prot);
		}
		else if (new_prot != utils::protection::rw)
		{
			mem_hash = fast_hash_internal();
		}

		protection = new_prot;
		locked = (protection != utils::protection::rw);

		if (!locked)
		{
			// Unprotect range also invalidates secured range
			confirmed_range.invalidate();
		}
	}

	void buffered_section::protect(utils::protection prot, const std::pair<u32, u32>& new_confirm)
	{
		// new_confirm.first is an offset after cpu_range.start
		// new_confirm.second is the length (after cpu_range.start + new_confirm.first)

#ifdef TEXTURE_CACHE_DEBUG
		// We need to remove the lockable range from page_info as we will be re-protecting with force==true
		if (locked)
			tex_cache_checker.remove(locked_range, protection);
#endif

		// Save previous state to compare for changes
		const auto prev_confirmed_range = confirmed_range;

		if (prot != utils::protection::rw)
		{
			if (confirmed_range.valid())
			{
				confirmed_range.start = std::min(confirmed_range.start, cpu_range.start + new_confirm.first);
				confirmed_range.end = std::max(confirmed_range.end, cpu_range.start + new_confirm.first + new_confirm.second - 1);
			}
			else
			{
				confirmed_range = address_range32::start_length(cpu_range.start + new_confirm.first, new_confirm.second);
				ensure(!locked || locked_range.inside(confirmed_range.to_page_range()));
			}

			ensure(confirmed_range.inside(cpu_range));
			init_lockable_range(confirmed_range);
		}

		protect(prot, confirmed_range != prev_confirmed_range);
	}

	void buffered_section::unprotect()
	{
		AUDIT(protection != utils::protection::rw);
		protect(utils::protection::rw);
	}

	void buffered_section::discard()
	{
#ifdef TEXTURE_CACHE_DEBUG
		if (locked)
			tex_cache_checker.remove(locked_range, protection);
#endif

		protection = utils::protection::rw;
		confirmed_range.invalidate();
		locked = false;
	}

	const address_range32& buffered_section::get_bounds(section_bounds bounds) const
	{
		switch (bounds)
		{
		case section_bounds::full_range:
			return cpu_range;
		case section_bounds::locked_range:
			return locked_range;
		case section_bounds::confirmed_range:
			return confirmed_range.valid() ? confirmed_range : cpu_range;
		default:
			fmt::throw_exception("Unreachable");
		}
	}

	u64 buffered_section::fast_hash_internal() const
	{
		const auto hash_range = confirmed_range.valid() ? confirmed_range : cpu_range;
		const auto hash_length = hash_range.length();
		const auto cycles = hash_length / 8;
		auto rem = hash_length % 8;

		auto src = get_ptr<const char>(hash_range.start);
		auto data64 = reinterpret_cast<const u64*>(src);

		usz hash = rpcs3::fnv_seed;
		for (unsigned i = 0; i < cycles; ++i)
		{
			hash = rpcs3::hash64(hash, data64[i]);
		}

		if (rem) [[unlikely]] // Data often aligned to some power of 2
		{
			src += hash_length - rem;

			if (rem > 4)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u32*>(src));
				src += 4;
			}

			if (rem > 2)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u16*>(src));
				src += 2;
			}

			while (rem--)
			{
				hash = rpcs3::hash64(hash, *reinterpret_cast<const u8*>(src));
				src++;
			}
		}

		return hash;
	}

	bool buffered_section::is_locked(bool actual_page_flags) const
	{
		if (!actual_page_flags || !locked)
		{
			return locked;
		}

		return (protection_strat == section_protection_strategy::lock);
	}

	bool buffered_section::sync() const
	{
		if (protection_strat == section_protection_strategy::lock || !locked)
		{
			return true;
		}

		return (fast_hash_internal() == mem_hash);
	}

	namespace blit_engine_helpers
	{
		// Result of the CPU-vs-GPU transfer heuristic for a main-memory destination.
		struct transfer_configuration
		{
			bool fall_back_to_cpu = false;          // Abort the GPU path and let the caller memcpy instead
			bool discard_dst_render_target = false; // dst was a render target but should be treated as plain memory
		};

		// Applies reverse-scan subpixel correction and computes the mirror/flip adjustments for the source.
		void apply_pixel_transforms(
			rsx::blit_src_info& src,
			const rsx::blit_dst_info& dst,
			typeless_xfer& typeless_info,
			u32& src_address,
			u16& src_w,
			u16& src_h,
			u8 src_bpp)
		{
			if (true) // This block is a debug/sanity check and should be optionally disabled with a config option
			{
				// Do subpixel correction in the special case of reverse scanning
				// When reverse scanning, pixel0 is at offset = (dimension - 1)
				if (dst.scale_y < 0.f && src.offset_y)
				{
					if (src.offset_y = (src.height - src.offset_y);
						src.offset_y == 1)
					{
						src.offset_y = 0;
					}
				}

				if (dst.scale_x < 0.f && src.offset_x)
				{
					if (src.offset_x = (src.width - src.offset_x);
						src.offset_x == 1)
					{
						src.offset_x = 0;
					}
				}

				if ((src_h + src.offset_y) > src.height) [[unlikely]]
				{
					// TODO: Special case that needs wrapping around (custom blit)
					rsx_log.error("Transfer cropped in Y, src_h=%d, offset_y=%d, block_h=%d", src_h, src.offset_y, src.height);
					src_h = src.height - src.offset_y;
				}

				if ((src_w + src.offset_x) > src.width) [[unlikely]]
				{
					// TODO: Special case that needs wrapping around (custom blit)
					rsx_log.error("Transfer cropped in X, src_w=%d, offset_x=%d, block_w=%d", src_w, src.offset_x, src.width);
					src_w = src.width - src.offset_x;
				}
			}

			if (dst.scale_y < 0.f)
			{
				typeless_info.flip_vertical = true;
				src_address -= (src.pitch * (src_h - 1));
			}

			if (dst.scale_x < 0.f)
			{
				typeless_info.flip_horizontal = true;
				src_address += (src.width - src_w) * src_bpp;
			}
		}

		// Given a surface's actual bpp, decide whether the blit side must be typeless and pick its format.
		void configure_typeless_format(
			/*out*/ bool& is_typeless,
			/*out*/ f32& scaling_hint,
			/*out*/ u32& gcm_format,
			u8 surface_bpp,
			u8 blit_bpp,
			bool is_argb8,
			bool surface_is_depth,
			bool is_format_convert)
		{
			const bool typeless = (surface_bpp != blit_bpp || is_format_convert);

			if (!typeless) [[likely]]
			{
				// Use format as-is
				gcm_format = helpers::get_sized_blit_format(is_argb8, surface_is_depth, false);
			}
			else
			{
				// Enable type scaling
				is_typeless = true;
				scaling_hint = static_cast<f32>(surface_bpp) / blit_bpp;
				gcm_format = helpers::get_sized_blit_format(is_argb8, false, is_format_convert);
			}
		}

		// Whether a cached surface's format is an acceptable source for a blit.
		// Only accept 16-bit data for 16-bit transfers and 32-bit for 32-bit transfers.
		// NOTE: some formats are copy-compatible but not scaling-compatible (hence the is_copy_op / dst_is_render_target checks).
		bool is_blit_source_format_compatible(u32 gcm_format, bool src_is_argb8, bool is_copy_op, bool dst_is_render_target)
		{
			switch (gcm_format)
			{
			case CELL_GCM_TEXTURE_X32_FLOAT:
			case CELL_GCM_TEXTURE_Y16_X16:
			case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
				// Should be copy compatible but not scaling compatible
				return src_is_argb8 && (is_copy_op || dst_is_render_target);
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
				// Should be copy compatible but not scaling compatible
				return src_is_argb8 && (is_copy_op || !dst_is_render_target);
			case CELL_GCM_TEXTURE_A8R8G8B8:
			case CELL_GCM_TEXTURE_D8R8G8B8:
				// Perfect match
				return src_is_argb8;
			case CELL_GCM_TEXTURE_X16:
			case CELL_GCM_TEXTURE_G8B8:
			case CELL_GCM_TEXTURE_A1R5G5B5:
			case CELL_GCM_TEXTURE_A4R4G4B4:
			case CELL_GCM_TEXTURE_D1R5G5B5:
			case CELL_GCM_TEXTURE_R5G5B5A1:
				// Copy compatible
				return !src_is_argb8 && (is_copy_op || dst_is_render_target);
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
				// Copy compatible
				return !src_is_argb8 && (is_copy_op || !dst_is_render_target);
			case CELL_GCM_TEXTURE_R5G6B5:
				// Perfect match
				return !src_is_argb8;
			default:
				return false;
			}
		}

		// Decides whether a transfer into main memory should run on the GPU or fall back to a CPU memcpy.
		// Pure heuristic: the caller applies the returned effects (early-out, rtt discard, collision handling).
		transfer_configuration configure_transfer_mode(
			const rsx::blit_src_info& src,
			const rsx::blit_dst_info& dst,
			bool dst_is_render_target,
			bool is_copy_op,
			bool is_format_convert,
			bool dst_is_tiled,
			bool src_is_tiled,
			u16 src_w,
			u16 src_h,
			u16 dst_w,
			u16 dst_h,
			u32 dst_address)
		{
			transfer_configuration config{};

			// Determine whether to perform this transfer on CPU or GPU (src data may not be graphical)
			const bool is_trivial_copy = is_copy_op && !is_format_convert && !dst.swizzled && !dst_is_tiled && !src_is_tiled;
			const bool is_block_transfer = (dst_w == src_w && dst_h == src_h && (src.pitch == dst.pitch || src_h == 1));
			const bool is_mirror_op = (dst.scale_x < 0.f || dst.scale_y < 0.f);

			// Always use GPU blit if src or dst is in the surface store
			if (dst_is_render_target)
			{
				if (is_trivial_copy && src_h == 1)
				{
					dst_is_render_target = false;
					config.discard_dst_render_target = true;
				}
				else
				{
					return config;
				}
			}

			if (is_trivial_copy)
			{
				// Check if trivial memcpy can perform the same task
				// Used to copy programs and arbitrary data to the GPU in some cases
				// NOTE: This case overrides the GPU texture scaling option
				if (is_block_transfer && !is_mirror_op)
				{
					config.fall_back_to_cpu = true;
					return config;
				}
			}

			if (!g_cfg.video.use_gpu_texture_scaling && !dst_is_tiled && !src_is_tiled)
			{
				if (dst.swizzled)
				{
					// Swizzle operation requested. Use fallback
					config.fall_back_to_cpu = true;
					return config;
				}

				if (is_trivial_copy && get_location(dst_address) != CELL_GCM_LOCATION_LOCAL)
				{
					// Trivial copy and the destination is in XDR memory
					config.fall_back_to_cpu = true;
					return config;
				}
			}

			return config;
		}

		// Verifies that the memory range defined by [base_address, write_end] actually exists in VM
		// The heurestic-defined block_end input is speculative. e.g We may be rendering to one corner of a larger image.
		// Returns true if we can use the entire range, false if we can only use the [base_address, write_end] area
		bool validate_memory_range(u32 base_address, u32 write_end, u32 block_end)
		{
			if (block_end <= write_end)
			{
				return true;
			}

			// Confirm if the pages actually exist in vm
			if (get_location(base_address) == CELL_GCM_LOCATION_LOCAL)
			{
				const auto vram_end = rsx::get_current_renderer()->local_mem_size + rsx::constants::local_mem_base;
				return block_end <= vram_end;
			}

			// Main memory block validation
			return vm::check_addr(write_end, vm::page_readable, (block_end - write_end));
		}

		// Reverse projects output clip rect back onto the source input.
		// This cuts out a post-transfer clip operation at the cost of possible edge bleed.
		void reproject_clip_offsets(
			rsx::blit_dst_info& dst_info,
			areai& src_area,
			u16 max_dst_width,
			u16 max_dst_height,
			f32 scale_x,
			f32 scale_y,
			const typeless_xfer& typeless_info)
		{
			// Validate clipping region
			if ((dst_info.offset_x + dst_info.clip_x + dst_info.clip_width) > max_dst_width) dst_info.clip_x = 0;
			if ((dst_info.offset_y + dst_info.clip_y + dst_info.clip_height) > max_dst_height) dst_info.clip_y = 0;

			// Reproject clip offsets onto source to simplify blit
			if (dst_info.clip_x || dst_info.clip_y)
			{
				const u16 scaled_clip_offset_x = static_cast<u16>(dst_info.clip_x / (scale_x * typeless_info.src_scaling_hint));
				const u16 scaled_clip_offset_y = static_cast<u16>(dst_info.clip_y / scale_y);

				src_area.x1 += scaled_clip_offset_x;
				src_area.x2 += scaled_clip_offset_x;
				src_area.y1 += scaled_clip_offset_y;
				src_area.y2 += scaled_clip_offset_y;
			}
		}

		// Reconcile cross-aspect transfers. Transfers cannot go across aspects due to API limitations.
		void reconcile_depth_color_typeless(
			typeless_xfer& typeless_info,
			bool src_is_argb8,
			bool dst_is_argb8)
		{
			if (typeless_info.src_gcm_format == typeless_info.dst_gcm_format) [[ likely ]]
			{
				return;
			}

			const auto src_is_depth = texture_cache_helpers::is_gcm_depth_format(typeless_info.src_gcm_format);
			const auto dst_is_depth = helpers::is_gcm_depth_format(typeless_info.dst_gcm_format);

			if (src_is_depth == dst_is_depth)
			{
				return;
			}

			// Make the depth side typeless because the other side is guaranteed to be color
			if (src_is_depth)
			{
				// SRC is depth, transfer must be done typelessly
				if (!typeless_info.src_is_typeless)
				{
					typeless_info.src_is_typeless = true;
					typeless_info.src_gcm_format = helpers::get_sized_blit_format(src_is_argb8, false, false);
				}

				return;
			}

			// DST is depth, transfer must be done typelessly
			if (!typeless_info.dst_is_typeless)
			{
				typeless_info.dst_is_typeless = true;
				typeless_info.dst_gcm_format = helpers::get_sized_blit_format(dst_is_argb8, false, false);
			}
		}
	}
}
