#include "stdafx.h"
#include "blitter.h"
#include "state_tracker.hpp"

#include "../GLTexture.h" // TODO: This system also needs to be refactored

namespace gl
{
	blitter* g_hw_blitter = nullptr;

	void blitter::copy_image(gl::command_context&, const texture* src, const texture* dst, int src_level, int dst_level, const position3i& src_offset, const position3i& dst_offset, const size3i& size) const
	{
		ensure(src_level == 0);

		// Typeless bypass for BGRA8
		std::unique_ptr<gl::texture> temp_image;
		const texture* real_src = src;

		glCopyImageSubData(real_src->id(), static_cast<GLenum>(real_src->get_target()), src_level,
			src_offset.x, src_offset.y, src_offset.z,
			dst->id(), static_cast<GLenum>(dst->get_target()), dst_level,
			dst_offset.x, dst_offset.y, dst_offset.z, size.width, size.height, size.depth);
	}

	void blitter::scale_image(gl::command_context& cmd, const texture* src, texture* dst, areai src_rect, areai dst_rect,
		bool linear_interpolation, const rsx::typeless_xfer& xfer_info)
	{
		std::unique_ptr<texture> typeless_src;
		std::unique_ptr<texture> typeless_dst;
		const gl::texture* real_src = src;
		const gl::texture* real_dst = dst;

		// Optimization pass; check for pass-through data transfer
		if (!xfer_info.flip_horizontal && !xfer_info.flip_vertical && src_rect.height() == dst_rect.height())
		{
			auto src_w = src_rect.width();
			auto dst_w = dst_rect.width();

			if (xfer_info.src_is_typeless) src_w = static_cast<int>(src_w * xfer_info.src_scaling_hint);
			if (xfer_info.dst_is_typeless) dst_w = static_cast<int>(dst_w * xfer_info.dst_scaling_hint);

			if (src_w == dst_w)
			{
				// Final dimensions are a match
				if (xfer_info.src_is_typeless || xfer_info.dst_is_typeless)
				{
					const coord3i src_region = { { src_rect.x1, src_rect.y1, 0 }, { src_rect.width(), src_rect.height(), 1 } };
					const coord3i dst_region = { { dst_rect.x1, dst_rect.y1, 0 }, { dst_rect.width(), dst_rect.height(), 1 } };
					gl::copy_typeless(cmd, dst, src, static_cast<coord3u>(dst_region), static_cast<coord3u>(src_region));
				}
				else
				{
					copy_image(cmd, src, dst, 0, 0, position3i{ src_rect.x1, src_rect.y1, 0u }, position3i{ dst_rect.x1, dst_rect.y1, 0 }, size3i{ src_rect.width(), src_rect.height(), 1 });
				}

				return;
			}
		}

		if (xfer_info.src_is_typeless)
		{
			const auto internal_fmt = xfer_info.src_native_format_override ?
				GLenum(xfer_info.src_native_format_override) :
				get_sized_internal_format(xfer_info.src_gcm_format);

			if (static_cast<gl::texture::internal_format>(internal_fmt) != src->get_internal_format())
			{
				const u16 internal_width = static_cast<u16>(src->width() * xfer_info.src_scaling_hint);
				typeless_src = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, src->height(), 1, 1, 1, internal_fmt, RSX_FORMAT_CLASS_DONT_CARE);
				copy_typeless(cmd, typeless_src.get(), src);

				real_src = typeless_src.get();
				src_rect.x1 = static_cast<u16>(src_rect.x1 * xfer_info.src_scaling_hint);
				src_rect.x2 = static_cast<u16>(src_rect.x2 * xfer_info.src_scaling_hint);
			}
		}

		if (xfer_info.dst_is_typeless)
		{
			const auto internal_fmt = xfer_info.dst_native_format_override ?
				GLenum(xfer_info.dst_native_format_override) :
				get_sized_internal_format(xfer_info.dst_gcm_format);

			if (static_cast<gl::texture::internal_format>(internal_fmt) != dst->get_internal_format())
			{
				const auto internal_width = static_cast<u16>(dst->width() * xfer_info.dst_scaling_hint);
				typeless_dst = std::make_unique<texture>(GL_TEXTURE_2D, internal_width, dst->height(), 1, 1, 1, internal_fmt, RSX_FORMAT_CLASS_DONT_CARE);
				copy_typeless(cmd, typeless_dst.get(), dst);

				real_dst = typeless_dst.get();
				dst_rect.x1 = static_cast<u16>(dst_rect.x1 * xfer_info.dst_scaling_hint);
				dst_rect.x2 = static_cast<u16>(dst_rect.x2 * xfer_info.dst_scaling_hint);
			}
		}

		ensure(real_src->aspect() == real_dst->aspect());

		if (xfer_info.flip_horizontal)
		{
			src_rect.flip_horizontal();
		}

		if (xfer_info.flip_vertical)
		{
			src_rect.flip_vertical();
		}

		if (src_rect.width() == dst_rect.width() &&
			src_rect.height() == dst_rect.height() &&
			!src_rect.is_flipped() && !dst_rect.is_flipped())
		{
			copy_image(cmd, real_src, real_dst, 0, 0, position3i{ src_rect.x1, src_rect.y1, 0 }, position3i{ dst_rect.x1, dst_rect.y1, 0 }, size3i{ src_rect.width(), src_rect.height(), 1 });
		}
		else
		{
			const bool is_depth_copy = (real_src->aspect() != image_aspect::color);
			const filter interp = (linear_interpolation && !is_depth_copy) ? filter::linear : filter::nearest;
			gl::fbo::attachment::type attachment;
			gl::buffers target;

			if (is_depth_copy)
			{
				if (real_dst->aspect() & gl::image_aspect::stencil)
				{
					attachment = fbo::attachment::type::depth_stencil;
					target = gl::buffers::depth_stencil;
				}
				else
				{
					attachment = fbo::attachment::type::depth;
					target = gl::buffers::depth;
				}
			}
			else
			{
				attachment = fbo::attachment::type::color;
				target = gl::buffers::color;
			}

			cmd->disable(GL_SCISSOR_TEST);

			save_binding_state saved;

			gl::fbo::attachment src_att{ blit_src, static_cast<fbo::attachment::type>(attachment) };
			src_att = *real_src;

			gl::fbo::attachment dst_att{ blit_dst, static_cast<fbo::attachment::type>(attachment) };
			dst_att = *real_dst;

			blit_src.blit(blit_dst, src_rect, dst_rect, target, interp);

			// Release the attachments explicitly (not doing so causes glitches, e.g Journey Menu)
			src_att = GL_NONE;
			dst_att = GL_NONE;
		}

		if (xfer_info.dst_is_typeless)
		{
			// Transfer contents from typeless dst back to original dst
			copy_typeless(cmd, dst, typeless_dst.get());
		}
	}

	void blitter::fast_clear_image(gl::command_context& cmd, const texture* dst, const color4f& color)
	{
		save_binding_state saved;

		blit_dst.bind();
		blit_dst.color[0] = *dst;
		blit_dst.check();

		cmd->clear_color(color);
		cmd->color_maski(0, true, true, true, true);

		glClear(GL_COLOR_BUFFER_BIT);
		blit_dst.color[0] = GL_NONE;
	}

	void blitter::fast_clear_image(gl::command_context& cmd, const texture* dst, float /*depth*/, u8 /*stencil*/)
	{
		fbo::attachment::type attachment;
		GLbitfield clear_mask;

		switch (const auto fmt = dst->get_internal_format())
		{
		case texture::internal_format::depth16:
		case texture::internal_format::depth32f:
			clear_mask = GL_DEPTH_BUFFER_BIT;
			attachment = fbo::attachment::type::depth;
			break;
		case texture::internal_format::depth24_stencil8:
		case texture::internal_format::depth32f_stencil8:
			clear_mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
			attachment = fbo::attachment::type::depth_stencil;
			break;
		default:
			fmt::throw_exception("Invalid texture passed to clear depth function, format=0x%x", static_cast<u32>(fmt));
		}

		save_binding_state saved;
		fbo::attachment attach_point{ blit_dst, attachment };

		blit_dst.bind();
		attach_point = *dst;
		blit_dst.check();

		cmd->depth_mask(GL_TRUE);
		cmd->stencil_mask(0xFF);

		glClear(clear_mask);
		attach_point = GL_NONE;
	}
}
