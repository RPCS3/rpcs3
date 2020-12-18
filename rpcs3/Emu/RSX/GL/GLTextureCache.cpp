#include "stdafx.h"
#include "Emu/RSX/RSXThread.h"
#include "GLTexture.h"
#include "GLTextureCache.h"

#include "util/asm.hpp"

namespace gl
{
	void cached_texture_section::finish_flush()
	{
		// Free resources
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);

		const auto valid_range = get_confirmed_range_delta();
		const u32 valid_offset = valid_range.first;
		const u32 valid_length = valid_range.second;
		void *dst = get_ptr(get_section_base() + valid_offset);

		if (!gl::get_driver_caps().ARB_compute_shader_supported)
		{
			switch (type)
			{
			case gl::texture::type::sbyte:
			case gl::texture::type::ubyte:
			{
				// byte swapping does not work on byte types, use uint_8_8_8_8 for rgba8 instead to avoid penalty
				ensure(!pack_unpack_swap_bytes);
				break;
			}
			case gl::texture::type::uint_24_8:
			{
				// Swap bytes on D24S8 does not swap the whole dword, just shuffles the 3 bytes for D24
				// In this regard, D24S8 is the same structure on both PC and PS3, but the endianness of the whole block is reversed on PS3
				ensure(pack_unpack_swap_bytes == false);
				ensure(real_pitch == (width * 4));
				if (rsx_pitch == real_pitch) [[likely]]
				{
					stream_data_to_memory_swapped_u32<true>(dst, dst, valid_length / 4, 4);
				}
				else
				{
					const u32 num_rows = utils::align(valid_length, rsx_pitch) / rsx_pitch;
					u8* data = static_cast<u8*>(dst);
					for (u32 row = 0; row < num_rows; ++row)
					{
						stream_data_to_memory_swapped_u32<true>(data, data, width, 4);
						data += rsx_pitch;
					}
				}
				break;
			}
			default:
				break;
			}
		}

		if (is_swizzled())
		{
			// This format is completely worthless to CPU processing algorithms where cache lines on die are linear.
			// If this is happening, usually it means it was not a planned readback (e.g shared pages situation)
			rsx_log.warning("[Performance warning] CPU readback of swizzled data");

			// Read-modify-write to avoid corrupting already resident memory outside texture region
			std::vector<u8> tmp_data(rsx_pitch * height);
			std::memcpy(tmp_data.data(), dst, tmp_data.size());

			switch (type)
			{
			case gl::texture::type::uint_8_8_8_8:
			case gl::texture::type::uint_24_8:
				rsx::convert_linear_swizzle<u32, false>(tmp_data.data(), dst, width, height, rsx_pitch);
				break;
			case gl::texture::type::ushort_5_6_5:
			case gl::texture::type::ushort:
				rsx::convert_linear_swizzle<u16, false>(tmp_data.data(), dst, width, height, rsx_pitch);
				break;
			default:
				rsx_log.error("Unexpected swizzled texture format 0x%x", static_cast<u32>(format));
			}
		}

		if (context == rsx::texture_upload_context::framebuffer_storage)
		{
			// Update memory tag
			static_cast<gl::render_target*>(vram_texture)->sync_tag();
		}
	}

	void texture_cache::copy_transfer_regions_impl(gl::command_context& cmd, gl::texture* dst_image, const std::vector<copy_region_descriptor>& sources) const
	{
		const auto dst_bpp = dst_image->pitch() / dst_image->width();
		const auto dst_aspect = dst_image->aspect();

		for (const auto &slice : sources)
		{
			if (!slice.src)
				continue;

			const bool typeless = dst_aspect != slice.src->aspect() ||
				!formats_are_bitcast_compatible(static_cast<GLenum>(slice.src->get_internal_format()), static_cast<GLenum>(dst_image->get_internal_format()));

			std::unique_ptr<gl::texture> tmp;
			auto src_image = slice.src;
			auto src_x = slice.src_x;
			auto src_y = slice.src_y;
			auto src_w = slice.src_w;
			auto src_h = slice.src_h;

			if (slice.xform == rsx::surface_transform::coordinate_transform)
			{
				// Dimensions were given in 'dst' space. Work out the real source coordinates
				const auto src_bpp = slice.src->pitch() / slice.src->width();
				src_x = (src_x * dst_bpp) / src_bpp;
				src_w = utils::aligned_div<u16>(src_w * dst_bpp, src_bpp);
			}

			if (auto surface = dynamic_cast<gl::render_target*>(slice.src))
			{
				surface->transform_samples_to_pixels(src_x, src_w, src_y, src_h);
			}

			if (typeless) [[unlikely]]
			{
				const auto src_bpp = slice.src->pitch() / slice.src->width();
				const u16 convert_w = u16(slice.src->width() * src_bpp) / dst_bpp;
				tmp = std::make_unique<texture>(GL_TEXTURE_2D, convert_w, slice.src->height(), 1, 1, static_cast<GLenum>(dst_image->get_internal_format()));

				src_image = tmp.get();

				// Compute src region in dst format layout
				const u16 src_w2 = u16(src_w * src_bpp) / dst_bpp;
				const u16 src_x2 = u16(src_x * src_bpp) / dst_bpp;

				if (src_w2 == slice.dst_w && src_h == slice.dst_h && slice.level == 0)
				{
					// Optimization, avoid typeless copy to tmp followed by data copy to dst
					// Combine the two transfers into one
					const coord3u src_region = { { src_x, src_y, 0 }, { src_w, src_h, 1 } };
					const coord3u dst_region = { { slice.dst_x, slice.dst_y, slice.dst_z }, { slice.dst_w, slice.dst_h, 1 } };
					gl::copy_typeless(dst_image, slice.src, dst_region, src_region);

					continue;
				}

				const coord3u src_region = { { src_x, src_y, 0 }, { src_w, src_h, 1 } };
				const coord3u dst_region = { { src_x2, src_y, 0 }, { src_w2, src_h, 1 } };
				gl::copy_typeless(src_image, slice.src, dst_region, src_region);

				src_x = src_x2;
				src_w = src_w2;
			}

			if (src_w == slice.dst_w && src_h == slice.dst_h)
			{
				glCopyImageSubData(src_image->id(), GL_TEXTURE_2D, 0, src_x, src_y, 0,
					dst_image->id(), static_cast<GLenum>(dst_image->get_target()), slice.level, slice.dst_x, slice.dst_y, slice.dst_z, src_w, src_h, 1);
			}
			else
			{
				ensure(dst_image->get_target() == gl::texture::target::texture2D);

				auto _blitter = gl::g_hw_blitter;
				const areai src_rect = { src_x, src_y, src_x + src_w, src_y + src_h };
				const areai dst_rect = { slice.dst_x, slice.dst_y, slice.dst_x + slice.dst_w, slice.dst_y + slice.dst_h };

				gl::texture* _dst;
				if (src_image->get_internal_format() == dst_image->get_internal_format() && slice.level == 0)
				{
					_dst = dst_image;
				}
				else
				{
					tmp = std::make_unique<texture>(GL_TEXTURE_2D, dst_rect.x2, dst_rect.y2, 1, 1, static_cast<GLenum>(slice.src->get_internal_format()));
					_dst = tmp.get();
				}

				_blitter->scale_image(cmd, src_image, _dst,
					src_rect, dst_rect, false, {});

				if (_dst != dst_image)
				{
					// Data cast comes after scaling
					glCopyImageSubData(tmp->id(), GL_TEXTURE_2D, 0, slice.dst_x, slice.dst_y, 0,
						dst_image->id(), static_cast<GLenum>(dst_image->get_target()), slice.level, slice.dst_x, slice.dst_y, slice.dst_z, slice.dst_w, slice.dst_h, 1);
				}
			}
		}
	}
}
