#include "stdafx.h"
#include "nv3089.h"

#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Core/RSXReservationLock.hpp"
#include "Emu/RSX/Common/tiled_dma_copy.hpp"

#include "context_accessors.define.h"

namespace rsx
{
	namespace nv3089
	{
		static std::tuple<bool, blit_src_info, blit_dst_info> decode_transfer_registers(context* ctx)
		{
			blit_src_info src_info = {};
			blit_dst_info dst_info = {};

			const rsx::blit_engine::transfer_operation operation = REGS(ctx)->blit_engine_operation();

			const u16 out_x = REGS(ctx)->blit_engine_output_x();
			const u16 out_y = REGS(ctx)->blit_engine_output_y();
			const u16 out_w = REGS(ctx)->blit_engine_output_width();
			const u16 out_h = REGS(ctx)->blit_engine_output_height();

			const u16 in_w = REGS(ctx)->blit_engine_input_width();
			const u16 in_h = REGS(ctx)->blit_engine_input_height();

			const blit_engine::transfer_origin in_origin = REGS(ctx)->blit_engine_input_origin();
			auto src_color_format = REGS(ctx)->blit_engine_src_color_format();

			const f32 scale_x = REGS(ctx)->blit_engine_ds_dx();
			const f32 scale_y = REGS(ctx)->blit_engine_dt_dy();

			// Clipping
			// Validate that clipping rect will fit onto both src and dst regions
			const u16 clip_w = std::min(REGS(ctx)->blit_engine_clip_width(), out_w);
			const u16 clip_h = std::min(REGS(ctx)->blit_engine_clip_height(), out_h);

			// Check both clip dimensions and dst dimensions
			if (clip_w == 0 || clip_h == 0)
			{
				rsx_log.warning("NV3089_IMAGE_IN: Operation NOPed out due to empty regions");
				return { false, src_info, dst_info };
			}

			if (in_w == 0 || in_h == 0)
			{
				// Input cant be an empty region
				fmt::throw_exception("NV3089_IMAGE_IN_SIZE: Invalid blit dimensions passed (in_w=%d, in_h=%d)", in_w, in_h);
			}

			u16 clip_x = REGS(ctx)->blit_engine_clip_x();
			u16 clip_y = REGS(ctx)->blit_engine_clip_y();

			//Fit onto dst
			if (clip_x && (out_x + clip_x + clip_w) > out_w) clip_x = 0;
			if (clip_y && (out_y + clip_y + clip_h) > out_h) clip_y = 0;

			u16 in_pitch = REGS(ctx)->blit_engine_input_pitch();

			switch (in_origin)
			{
			case blit_engine::transfer_origin::corner:
			case blit_engine::transfer_origin::center:
				break;
			default:
				rsx_log.warning("NV3089_IMAGE_IN_SIZE: unknown origin (%d)", static_cast<u8>(in_origin));
			}

			if (operation != rsx::blit_engine::transfer_operation::srccopy)
			{
				rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown operation (0x%x)", REGS(ctx)->registers[NV3089_SET_OPERATION]);
				RSX(ctx)->recover_fifo();
				return { false, src_info, dst_info };
			}

			if (!src_color_format)
			{
				rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown src color format (0x%x)", REGS(ctx)->registers[NV3089_SET_COLOR_FORMAT]);
				RSX(ctx)->recover_fifo();
				return { false, src_info, dst_info };
			}

			const u32 src_offset = REGS(ctx)->blit_engine_input_offset();
			const u32 src_dma = REGS(ctx)->blit_engine_input_location();

			u32 dst_offset;
			u32 dst_dma = 0;
			rsx::blit_engine::transfer_destination_format dst_color_format;
			u32 out_pitch = 0;
			[[maybe_unused]] u32 out_alignment = 64;
			bool is_block_transfer = false;

			switch (REGS(ctx)->blit_engine_context_surface())
			{
			case blit_engine::context_surface::surface2d:
			{
				dst_dma = REGS(ctx)->blit_engine_output_location_nv3062();
				dst_offset = REGS(ctx)->blit_engine_output_offset_nv3062();
				out_pitch = REGS(ctx)->blit_engine_output_pitch_nv3062();
				out_alignment = REGS(ctx)->blit_engine_output_alignment_nv3062();
				is_block_transfer = fcmp(scale_x, 1.f) && fcmp(scale_y, 1.f);

				if (auto dst_fmt = REGS(ctx)->blit_engine_nv3062_color_format(); !dst_fmt)
				{
					rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown NV3062 dst color format (0x%x)", REGS(ctx)->registers[NV3062_SET_COLOR_FORMAT]);
					RSX(ctx)->recover_fifo();
					return { false, src_info, dst_info };
				}
				else
				{
					dst_color_format = dst_fmt;
				}

				break;
			}
			case blit_engine::context_surface::swizzle2d:
			{
				dst_dma = REGS(ctx)->blit_engine_nv309E_location();
				dst_offset = REGS(ctx)->blit_engine_nv309E_offset();

				if (auto dst_fmt = REGS(ctx)->blit_engine_output_format_nv309E(); !dst_fmt)
				{
					rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown NV309E dst color format (0x%x)", REGS(ctx)->registers[NV309E_SET_FORMAT]);
					RSX(ctx)->recover_fifo();
					return { false, src_info, dst_info };
				}
				else
				{
					dst_color_format = dst_fmt;
				}

				break;
			}
			default:
				rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", static_cast<u8>(REGS(ctx)->blit_engine_context_surface()));
				return { false, src_info, dst_info };
			}

			const u32 in_bpp = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? 2 : 4; // bytes per pixel
			const u32 out_bpp = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? 2 : 4;

			if (out_pitch == 0)
			{
				out_pitch = out_bpp * out_w;
			}

			if (in_pitch == 0)
			{
				in_pitch = in_bpp * in_w;
			}

			if (in_bpp != out_bpp)
			{
				is_block_transfer = false;
			}

			u16 in_x, in_y;
			if (in_origin == blit_engine::transfer_origin::center)
			{
				// Convert to normal u,v addressing. Under this scheme offset of 1 is actually half-way inside pixel 0
				const float x = std::max(REGS(ctx)->blit_engine_in_x(), 0.5f);
				const float y = std::max(REGS(ctx)->blit_engine_in_y(), 0.5f);
				in_x = static_cast<u16>(std::floor(x - 0.5f));
				in_y = static_cast<u16>(std::floor(y - 0.5f));
			}
			else
			{
				in_x = static_cast<u16>(std::floor(REGS(ctx)->blit_engine_in_x()));
				in_y = static_cast<u16>(std::floor(REGS(ctx)->blit_engine_in_y()));
			}

			// Check for subpixel addressing
			if (scale_x < 1.f)
			{
				float dst_x = in_x * scale_x;
				in_x = static_cast<u16>(std::floor(dst_x) / scale_x);
			}

			if (scale_y < 1.f)
			{
				float dst_y = in_y * scale_y;
				in_y = static_cast<u16>(std::floor(dst_y) / scale_y);
			}

			const u32 in_offset = in_x * in_bpp + in_pitch * in_y;
			const u32 out_offset = out_x * out_bpp + out_pitch * out_y;

			const u32 src_line_length = (in_w * in_bpp);

			u32 src_address = 0;
			const u32 dst_address = get_address(dst_offset, dst_dma, 1); // TODO: Add size

			if (is_block_transfer && (clip_h == 1 || (in_pitch == out_pitch && src_line_length == in_pitch)))
			{
				const u32 nb_lines = std::min(clip_h, in_h);
				const u32 data_length = nb_lines * src_line_length;

				if (src_address = get_address(src_offset, src_dma, data_length);
					!src_address || !dst_address)
				{
					RSX(ctx)->recover_fifo();
					return { false, src_info, dst_info };
				}

				RSX(ctx)->invalidate_fragment_program(dst_dma, dst_offset, data_length);

				if (const auto result = RSX(ctx)->read_barrier(src_address, data_length, false);
					result == rsx::result_zcull_intr)
				{
					if (RSX(ctx)->copy_zcull_stats(src_address, data_length, dst_address) == data_length)
					{
						// All writes deferred
						return { false, src_info, dst_info };
					}
				}
			}
			else
			{
				const u16 read_h = std::min(static_cast<u16>(clip_h / scale_y), in_h);
				const u32 data_length = in_pitch * (read_h - 1) + src_line_length;

				if (src_address = get_address(src_offset, src_dma, data_length);
					!src_address || !dst_address)
				{
					RSX(ctx)->recover_fifo();
					return { false, src_info, dst_info };
				}

				RSX(ctx)->invalidate_fragment_program(dst_dma, dst_offset, data_length);
				RSX(ctx)->read_barrier(src_address, data_length, true);
			}

			if (src_address == dst_address &&
				in_w == clip_w && in_h == clip_h &&
				in_pitch == out_pitch &&
				rsx::fcmp(scale_x, 1.f) && rsx::fcmp(scale_y, 1.f))
			{
				// NULL operation
				rsx_log.warning("NV3089_IMAGE_IN: Operation writes memory onto itself with no modification (move-to-self). Will ignore.");
				return { false, src_info, dst_info };
			}

			u8* pixels_src = vm::_ptr<u8>(src_address + in_offset);
			u8* pixels_dst = vm::_ptr<u8>(dst_address + out_offset);

			if (dst_color_format != rsx::blit_engine::transfer_destination_format::r5g6b5 &&
				dst_color_format != rsx::blit_engine::transfer_destination_format::a8r8g8b8)
			{
				fmt::throw_exception("NV3089_IMAGE_IN_SIZE: unknown dst_color_format (%d)", static_cast<u8>(dst_color_format));
			}

			if (src_color_format != rsx::blit_engine::transfer_source_format::r5g6b5 &&
				src_color_format != rsx::blit_engine::transfer_source_format::a8r8g8b8)
			{
				// Alpha has no meaning in both formats
				if (src_color_format == rsx::blit_engine::transfer_source_format::x8r8g8b8)
				{
					src_color_format = rsx::blit_engine::transfer_source_format::a8r8g8b8;
				}
				else
				{
					// TODO: Support more formats
					fmt::throw_exception("NV3089_IMAGE_IN_SIZE: unknown src_color_format (%d)", static_cast<u8>(*src_color_format));
				}
			}

			u32 convert_w = static_cast<u32>(std::abs(scale_x) * in_w);
			u32 convert_h = static_cast<u32>(std::abs(scale_y) * in_h);

			if (convert_w == 0 || convert_h == 0)
			{
				rsx_log.error("NV3089_IMAGE_IN: Invalid dimensions or scaling factor. Request ignored (ds_dx=%f, dt_dy=%f)",
					REGS(ctx)->blit_engine_ds_dx(), REGS(ctx)->blit_engine_dt_dy());
				return { false, src_info, dst_info };
			}

			src_info.format = src_color_format;
			src_info.origin = in_origin;
			src_info.width = in_w;
			src_info.height = in_h;
			src_info.pitch = in_pitch;
			src_info.bpp = in_bpp;
			src_info.offset_x = in_x;
			src_info.offset_y = in_y;
			src_info.dma = src_dma;
			src_info.rsx_address = src_address;
			src_info.pixels = pixels_src;

			dst_info.format = dst_color_format;
			dst_info.width = convert_w;
			dst_info.height = convert_h;
			dst_info.clip_x = clip_x;
			dst_info.clip_y = clip_y;
			dst_info.clip_width = clip_w;
			dst_info.clip_height = clip_h;
			dst_info.offset_x = out_x;
			dst_info.offset_y = out_y;
			dst_info.pitch = out_pitch;
			dst_info.bpp = out_bpp;
			dst_info.scale_x = scale_x;
			dst_info.scale_y = scale_y;
			dst_info.dma = dst_dma;
			dst_info.rsx_address = dst_address;
			dst_info.pixels = pixels_dst;
			dst_info.swizzled = (REGS(ctx)->blit_engine_context_surface() == blit_engine::context_surface::swizzle2d);

			return { true, src_info, dst_info };
		}

		void linear_copy(
			const blit_dst_info& dst,
			const blit_src_info& src,
			u16 out_w,
			u16 out_h,
			u32 slice_h,
			AVPixelFormat ffmpeg_src_format,
			AVPixelFormat ffmpeg_dst_format,
			bool need_convert,
			bool need_clip,
			bool src_is_modified,
			bool interpolate)
		{
			std::vector<u8> temp2;

			if (!need_convert) [[ likely ]]
			{
				const bool is_overlapping = !src_is_modified && dst.dma == src.dma && [&]() -> bool
				{
					const auto src_range = utils::address_range32::start_length(src.rsx_address, src.pitch * (src.height - 1) + (src.bpp * src.width));
					const auto dst_range = utils::address_range32::start_length(dst.rsx_address, dst.pitch * (dst.clip_height - 1) + (dst.bpp * dst.clip_width));
					return src_range.overlaps(dst_range);
				}();

					if (is_overlapping) [[ unlikely ]]
					{
						if (need_clip)
						{
							temp2.resize(dst.pitch * dst.clip_height);
							clip_image_may_overlap(dst.pixels, src.pixels, dst.clip_x, dst.clip_y, dst.clip_width, dst.clip_height, dst.bpp, src.pitch, dst.pitch, temp2.data());
							return;
						}

						if (dst.pitch != src.pitch || dst.pitch != dst.bpp * out_w)
						{
							const u32 buffer_pitch = dst.bpp * out_w;
							temp2.resize(buffer_pitch * out_h);
							std::add_pointer_t<u8> buf = temp2.data(), pixels = src.pixels;

							// Read the whole buffer from source
							for (u32 y = 0; y < out_h; ++y)
							{
								std::memcpy(buf, pixels, buffer_pitch);
								pixels += src.pitch;
								buf += buffer_pitch;
							}

							buf = temp2.data(), pixels = dst.pixels;

							// Write to destination
							for (u32 y = 0; y < out_h; ++y)
							{
								std::memcpy(pixels, buf, buffer_pitch);
								pixels += dst.pitch;
								buf += buffer_pitch;
							}

							return;
						}

						std::memmove(dst.pixels, src.pixels, dst.pitch * out_h);
						return;
					}

					if (need_clip) [[ unlikely ]]
					{
						clip_image(dst.pixels, src.pixels, dst.clip_x, dst.clip_y, dst.clip_width, dst.clip_height, dst.bpp, src.pitch, dst.pitch);
						return;
					}

					if (dst.pitch != src.pitch || dst.pitch != dst.bpp * out_w) [[ unlikely ]]
					{
						u8* dst_pixels = dst.pixels, * src_pixels = src.pixels;

						for (u32 y = 0; y < out_h; ++y)
						{
							std::memcpy(dst_pixels, src_pixels, out_w * dst.bpp);
							dst_pixels += dst.pitch;
							src_pixels += src.pitch;
						}

						return;
					}

					std::memcpy(dst.pixels, src.pixels, dst.pitch * out_h);
					return;
			}

			if (need_clip) [[ unlikely ]]
			{
				temp2.resize(dst.pitch * std::max<u32>(dst.height, dst.clip_height));

				convert_scale_image(temp2.data(), ffmpeg_dst_format, dst.width, dst.height, dst.pitch,
					src.pixels, ffmpeg_src_format, src.width, src.height, src.pitch, slice_h, interpolate);

				clip_image(dst.pixels, temp2.data(), dst.clip_x, dst.clip_y, dst.clip_width, dst.clip_height, dst.bpp, dst.pitch, dst.pitch);
				return;
			}

			convert_scale_image(dst.pixels, ffmpeg_dst_format, out_w, out_h, dst.pitch,
				src.pixels, ffmpeg_src_format, src.width, src.height, src.pitch, slice_h,
				interpolate);
		}

		std::vector<u8> swizzled_copy_1(
			const blit_dst_info& dst,
			const blit_src_info& src,
			u16 out_w,
			u16 out_h,
			u32 slice_h,
			AVPixelFormat ffmpeg_src_format,
			AVPixelFormat ffmpeg_dst_format,
			bool need_convert,
			bool need_clip,
			bool interpolate)
		{
			std::vector<u8> temp2, temp3;

			if (need_clip)
			{
				temp3.resize(dst.pitch * dst.clip_height);

				if (need_convert)
				{
					temp2.resize(dst.pitch * std::max<u32>(dst.height, dst.clip_height));

					convert_scale_image(temp2.data(), ffmpeg_dst_format, dst.width, dst.height, dst.pitch,
						src.pixels, ffmpeg_src_format, src.width, src.height, src.pitch, slice_h,
						interpolate);

					clip_image(temp3.data(), temp2.data(), dst.clip_x, dst.clip_y, dst.clip_width, dst.clip_height, dst.bpp, dst.pitch, dst.pitch);
					return temp3;
				}

				clip_image(temp3.data(), src.pixels, dst.clip_x, dst.clip_y, dst.clip_width, dst.clip_height, dst.bpp, src.pitch, dst.pitch);
				return temp3;
			}

			if (need_convert)
			{
				temp3.resize(dst.pitch * out_h);

				convert_scale_image(temp3.data(), ffmpeg_dst_format, out_w, out_h, dst.pitch,
					src.pixels, ffmpeg_src_format, src.width, src.height, src.pitch, slice_h,
					interpolate);

				return temp3;
			}

			return {};
		}

		void swizzled_copy_2(
			const u8* linear_pixels,
			u8* swizzled_pixels,
			u32 linear_pitch,
			u16 out_w,
			u16 out_h,
			u8 out_bpp)
		{
			// TODO: Validate these claims. Are the registers always correctly initialized? Should we trust them at all?
			// It looks like rsx may ignore the requested swizzle size and just always
			// round up to nearest power of 2
			/*
			u8 sw_width_log2 = REGS(ctx)->nv309e_sw_width_log2();
			u8 sw_height_log2 = REGS(ctx)->nv309e_sw_height_log2();

			// 0 indicates height of 1 pixel
			sw_height_log2 = sw_height_log2 == 0 ? 1 : sw_height_log2;

			// swizzle based on destination size
			const u16 sw_width = 1 << sw_width_log2;
			const u16 sw_height = 1 << sw_height_log2;
			*/

			std::vector<u8> sw_temp;

			const u32 sw_width = next_pow2(out_w);
			const u32 sw_height = next_pow2(out_h);

			// Check and pad texture out if we are given non power of 2 output
			if (sw_width != out_w || sw_height != out_h)
			{
				sw_temp.resize(out_bpp * sw_width * sw_height);

				switch (out_bpp)
				{
				case 1:
					pad_texture<u8>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
					break;
				case 2:
					pad_texture<u16>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
					break;
				case 4:
					pad_texture<u32>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
					break;
				}

				linear_pixels = sw_temp.data();
			}

			switch (out_bpp)
			{
			case 1:
				convert_linear_swizzle<u8, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, linear_pitch);
				break;
			case 2:
				convert_linear_swizzle<u16, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, linear_pitch);
				break;
			case 4:
				convert_linear_swizzle<u32, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, linear_pitch);
				break;
			}
		}

		std::vector<u8> _mirror_transform(const blit_src_info& src, bool flip_x, bool flip_y)
		{
			std::vector<u8> temp1;
			if (!flip_x && !flip_y)
			{
				return temp1;
			}

			const u32 packed_pitch = src.width * src.bpp;
			temp1.resize(packed_pitch * src.height);

			const s32 stride_y = (flip_y ? -1 : 1) * static_cast<s32>(src.pitch);

			for (u32 y = 0; y < src.height; ++y)
			{
				u8* dst_pixels = temp1.data() + (packed_pitch * y);
				u8* src_pixels = src.pixels + (static_cast<s32>(y) * stride_y);

				if (flip_x)
				{
					if (src.bpp == 4) [[ likely ]]
					{
						rsx::memcpy_r<u32>(dst_pixels, src_pixels, src.width);
						continue;
					}

					rsx::memcpy_r<u16>(dst_pixels, src_pixels, src.width);
					continue;
				}

				std::memcpy(dst_pixels, src_pixels, packed_pitch);
			}

			return temp1;
		}

		void image_in(context* ctx, u32 /*reg*/, u32 /*arg*/)
		{
			auto [success, src, dst] = decode_transfer_registers(ctx);
			if (!success)
			{
				return;
			}

			// Decode extra params before locking
			const blit_engine::transfer_interpolator in_inter = REGS(ctx)->blit_engine_input_inter();
			const u16 out_w = REGS(ctx)->blit_engine_output_width();
			const u16 out_h = REGS(ctx)->blit_engine_output_height();

			// Lock here. RSX cannot execute any locking operations from this point, including ZCULL read barriers
			auto res = ::rsx::reservation_lock<true>(
				dst.rsx_address, dst.pitch * dst.clip_height,
				src.rsx_address, src.pitch * src.height);

			if (!g_cfg.video.force_cpu_blit_processing &&
				(dst.dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER || src.dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER) &&
				RSX(ctx)->scaled_image_from_memory(src, dst, in_inter == blit_engine::transfer_interpolator::foh))
			{
				// HW-accelerated blit
				return;
			}

			std::vector<u8> mirror_tmp;
			bool src_is_temp = false;

			// Flip source if needed
			if (dst.scale_y < 0 || dst.scale_x < 0)
			{
				mirror_tmp = _mirror_transform(src, dst.scale_x < 0, dst.scale_y < 0);
				src.pixels = mirror_tmp.data();
				src.pitch = src.width * src.bpp;
				src_is_temp = true;
			}

			const AVPixelFormat in_format = (src.format == rsx::blit_engine::transfer_source_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;
			const AVPixelFormat out_format = (dst.format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;

			const bool need_clip =
				dst.clip_width != src.width ||
				dst.clip_height != src.height ||
				dst.clip_x > 0 || dst.clip_y > 0 ||
				dst.width != out_w || dst.height != out_h;

			const bool need_convert = out_format != in_format || !rsx::fcmp(fabsf(dst.scale_x), 1.f) || !rsx::fcmp(fabsf(dst.scale_y), 1.f);
			const u32 slice_h = static_cast<u32>(std::ceil(static_cast<f32>(dst.clip_height + dst.clip_y) / dst.scale_y));
			const bool interpolate = in_inter == blit_engine::transfer_interpolator::foh;

			auto real_dst = dst.pixels;
			const auto tiled_region = RSX(ctx)->get_tiled_memory_region(utils::address_range32::start_length(dst.rsx_address, dst.pitch * dst.clip_height));
			std::vector<u8> tmp;

			if (tiled_region)
			{
				tmp.resize(tiled_region.tile->size);
				real_dst = dst.pixels;
				dst.pixels = tmp.data();
			}

			if (REGS(ctx)->blit_engine_context_surface() != blit_engine::context_surface::swizzle2d)
			{
				linear_copy(dst, src, out_w, out_h, slice_h, in_format, out_format, need_convert, need_clip, src_is_temp, interpolate);
			}
			else
			{
				// Swizzle_copy_1 prepares usable output buffer from our original source. It mostly deals with cropping and scaling the input pixels so that the final swizzle does not need to apply that.
				const auto swz_temp = swizzled_copy_1(dst, src, out_w, out_h, slice_h, in_format, out_format, need_convert, need_clip, interpolate);
				const u8* pixels_src = src.pixels;
				auto src_pitch = src.pitch;

				// NOTE: Swizzled copy routine creates temp output buffer that uses dst pitch, not source pitch. We need to account for this if using that output as intermediary buffer.
				if (!swz_temp.empty())
				{
					pixels_src = swz_temp.data();
					src_pitch = dst.pitch;
				}

				// Swizzle_copy_2 only pads the data and encodes it as a swizzled output. Transformation (scaling, rotation, etc) is done in swizzle_copy_1
				swizzled_copy_2(pixels_src, dst.pixels, src_pitch, out_w, out_h, dst.bpp);
			}

			if (tiled_region)
			{
				const auto tile_func = dst.bpp == 4
					? rsx::tile_texel_data32
					: rsx::tile_texel_data16;

				tile_func(
					real_dst,
					dst.pixels,
					tiled_region.base_address,
					dst.rsx_address - tiled_region.base_address,
					tiled_region.tile->size,
					tiled_region.tile->bank,
					tiled_region.tile->pitch,
					dst.clip_width,
					dst.clip_height
				);
			}
		}
	}
}
