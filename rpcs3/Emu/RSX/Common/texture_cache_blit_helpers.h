#pragma once

#include "texture_cache_utils.h"

namespace rsx
{
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
			u8 src_bpp);

		// Given a surface's actual bpp, decide whether the blit side must be typeless and pick its format.
		void configure_typeless_format(
			/*out*/ bool& is_typeless,
			/*out*/ f32& scaling_hint,
			/*out*/ u32& gcm_format,
			u8 surface_bpp,
			u8 blit_bpp,
			bool is_argb8,
			bool surface_is_depth,
			bool is_format_convert);

		// Whether a cached surface's format is an acceptable source for a blit.
		// Only accept 16-bit data for 16-bit transfers and 32-bit for 32-bit transfers.
		// NOTE: some formats are copy-compatible but not scaling-compatible (hence the is_copy_op / dst_is_render_target checks).
		bool is_blit_source_format_compatible(u32 gcm_format, bool src_is_argb8, bool is_copy_op, bool dst_is_render_target);

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
			u32 dst_address);

		// Verifies that the memory range defined by [base_address, write_end] actually exists in VM
		// The heurestic-defined block_end input is speculative. e.g We may be rendering to one corner of a larger image.
		// Returns true if we can use the entire range, false if we can only use the [base_address, write_end] area
		bool validate_memory_range(u32 base_address, u32 write_end, u32 block_end);

		// Reverse projects output clip rect back onto the source input.
		// This cuts out a post-transfer clip operation at the cost of possible edge bleed.
		void reproject_clip_offsets(
			rsx::blit_dst_info& dst_info,
			areai& src_area,
			u16 max_dst_width,
			u16 max_dst_height,
			f32 scale_x,
			f32 scale_y,
			const typeless_xfer& typeless_info);

		// Reconcile cross-aspect transfers. Transfers cannot go across aspects due to API limitations.
		void reconcile_depth_color_typeless(
			typeless_xfer& typeless_info,
			bool src_is_argb8,
			bool dst_is_argb8);
	}
}
