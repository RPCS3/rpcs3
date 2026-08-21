#include "stdafx.h"
#include "blitter.h"
#include "state_tracker.hpp"

#include "../GLTexture.h" // TODO: This system also needs to be refactored

namespace gl
{
	blitter* g_hw_blitter = nullptr;

	// Max operation on the magnitude of value while preserving the sign
	// Clamp_val must be >= 0
	static int mag_max(int value, int clamp_val)
	{
		if (value >= 0)
		{
			return std::max(value, clamp_val);
		}

		return -std::max(-value, clamp_val);
	}

	// Shrink a blit rectangle to describe the same window one mipmap level down.
	static void increment_mip_level(areai& rect)
	{
		const int mip_w = (rect.x2 - rect.x1) / 2;
		const int mip_h = (rect.y2 - rect.y1) / 2;

		rect.x1 /= 2;
		rect.y1 /= 2;
		rect.x2 = rect.x1 + mag_max(mip_w, 1);
		rect.y2 = rect.y1 + mag_max(mip_h, 1);
	}

	// Targets whose slices have to be selected explicitly. A flat attach binds every slice of these at once.
	static bool is_layered_target(const texture* tex)
	{
		switch (tex->get_target())
		{
		case texture::target::texture3D:
		case texture::target::texture2DArray:
		case texture::target::textureCUBE:
			return true;
		default:
			return false;
		}
	}

	void blitter::init()
	{
		blit_src.create();
		blit_dst.create();
	}

	void blitter::destroy()
	{
		blit_dst.remove();
		blit_src.remove();
	}

	void blitter::copy_image(
		gl::command_context& /*cmd*/,
		const texture* src, const texture* dst,
		const position3i& src_offset,
		const position3i& dst_offset,
		const size3i& size,
		const rsx::image_copy_subresource_layers& mip_layers) const
	{
		// Sanity check - disallow layered volumes. Either set the layer parameters or the Z parameters, not both.
		ensure(!src_offset.z || !mip_layers.src_layer);
		ensure(!dst_offset.z || !mip_layers.dst_layer);
		ensure(size.depth == 1 || mip_layers.layer_count == 1);

		// Sanity check - 3D textures can only copy 1 mip level ata a time
		if (src->get_target() == texture::target::texture3D || dst->get_target() == texture::target::texture3D )
		{
			ensure(mip_layers.mipmap_count == 1);
		}

		auto src_pos = src_offset;
		auto dst_pos = dst_offset;
		auto extents = size;

		// Wrap the mess of layers and Z to match OpenGL's dumb specification. OGL only provides X/Y/Z to mean both volume and layers.
		if (mip_layers.src_layer) src_pos.z = mip_layers.src_layer;
		if (mip_layers.dst_layer) dst_pos.z = mip_layers.dst_layer;
		if (mip_layers.layer_count > 1) extents.depth = mip_layers.layer_count;

		int src_level = mip_layers.src_mip_level;
		int dst_level = mip_layers.dst_mip_level;

		for (u32 remaining_levels = mip_layers.mipmap_count; remaining_levels > 0; --remaining_levels)
		{
			glCopyImageSubData(src->id(), static_cast<GLenum>(src->get_target()), src_level,
				src_pos.x, src_pos.y, src_pos.z,
				dst->id(), static_cast<GLenum>(dst->get_target()), dst_level,
				dst_pos.x, dst_pos.y, dst_pos.z, extents.width, extents.height, extents.depth);

			if (remaining_levels > 1)
			{
				// NOTE: We don't touch Z here, it's a layer index not a depth slice.
				src_pos.x /= 2;
				src_pos.y /= 2;
				dst_pos.x /= 2;
				dst_pos.y /= 2;

				extents.width = std::max(extents.width / 2, 1);
				extents.height = std::max(extents.height / 2, 1);

				src_level++;
				dst_level++;
			}
		}
	}

	void blitter::scale_image(gl::command_context& cmd,
		const texture* src, texture* dst,
		const coord3i& src_rect, const coord3i& dst_rect,
		bool linear_interpolation, const rsx::typeless_xfer& xfer_info,
		const rsx::image_copy_subresource_layers& mip_layers)
	{
		std::unique_ptr<texture> typeless_src;
		std::unique_ptr<texture> typeless_dst;
		const gl::texture* real_src = src;
		const gl::texture* real_dst = dst;

		const bool targets_subresource =
			mip_layers.src_mip_level || mip_layers.dst_mip_level || mip_layers.mipmap_count > 1 ||
			mip_layers.src_layer || mip_layers.dst_layer || mip_layers.layer_count > 1;

		// Typeless scratch images are allocated as flat level-0 2D surfaces sized from the base mip, so they cannot carry a subresource selection.
		ensure(!targets_subresource || (!xfer_info.src_is_typeless && !xfer_info.dst_is_typeless));

		// Optimization pass; check for pass-through data transfer
		if (!xfer_info.flip_horizontal && !xfer_info.flip_vertical &&
			!src_rect.is_flipped() && !dst_rect.is_flipped() &&
			src_rect.height == dst_rect.height && src_rect.depth == dst_rect.depth)
		{
			auto src_w = src_rect.width;
			auto dst_w = dst_rect.width;

			if (xfer_info.src_is_typeless) src_w = static_cast<int>(src_w * xfer_info.src_scaling_hint);
			if (xfer_info.dst_is_typeless) dst_w = static_cast<int>(dst_w * xfer_info.dst_scaling_hint);

			if (src_w == dst_w)
			{
				// Final dimensions are a match
				if (xfer_info.src_is_typeless || xfer_info.dst_is_typeless)
				{
					gl::copy_typeless(cmd, dst, src, static_cast<coord3u>(dst_rect), static_cast<coord3u>(src_rect), mip_layers);
				}
				else
				{
					copy_image(cmd, src, dst, src_rect.position, dst_rect.position, src_rect.size, mip_layers);
				}

				return;
			}
		}

		auto src_rect_ = src_rect;
		auto dst_rect_ = dst_rect;

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
				src_rect_.x = static_cast<int>(src_rect.x * xfer_info.src_scaling_hint);
				src_rect_.width = static_cast<int>(src_rect.width * xfer_info.src_scaling_hint);
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
				dst_rect_.x = static_cast<int>(dst_rect_.x * xfer_info.dst_scaling_hint);
				dst_rect_.width = static_cast<int>(dst_rect_.width * xfer_info.dst_scaling_hint);
			}
		}

		ensure(real_src->aspect() == real_dst->aspect());

		if (xfer_info.flip_horizontal)
		{
			src_rect_.flip_horizontal();
		}

		if (xfer_info.flip_vertical)
		{
			src_rect_.flip_vertical();
		}

		if (src_rect_.width == dst_rect_.width &&
			src_rect_.height == dst_rect_.height &&
			src_rect_.depth == dst_rect_.depth &&
			!src_rect_.is_flipped() && !dst_rect_.is_flipped())
		{
			copy_image(cmd, real_src, real_dst, src_rect_.position, dst_rect_.position, src_rect_.size, mip_layers);
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

			// Bulk layer transfers are not doable in OpenGL. We can only process one layer at a time.
			ensure(mip_layers.layer_count == 1);

			cmd->disable(GL_SCISSOR_TEST);

			save_binding_state saved;

			gl::fbo::attachment src_att{ blit_src, static_cast<fbo::attachment::type>(attachment) };
			gl::fbo::attachment dst_att{ blit_dst, static_cast<fbo::attachment::type>(attachment) };

			auto attach_slice = [](fbo::attachment& att, const texture* tex, int level, int layer)
			{
				if (is_layered_target(tex)) [[ unlikely ]]
				{
					att.bind_layer(*tex, level, layer);
					return;
				}

				ensure(!layer);
				att.bind(*tex, level);
			};

			// Pick volume or layered transfer, not both.
			ensure(!mip_layers.src_layer || !src_rect_.z);
			ensure(!mip_layers.dst_layer || !dst_rect_.z);
			const int src_layer = mip_layers.src_layer + src_rect_.z;
			const int dst_layer = mip_layers.dst_layer + dst_rect_.z;

			areai src_area = src_rect_.to_area();
			areai dst_area = dst_rect_.to_area();

			int src_level = mip_layers.src_mip_level;
			int dst_level = mip_layers.dst_mip_level;

			for (u32 remaining_levels = mip_layers.mipmap_count; remaining_levels > 0; --remaining_levels)
			{
				attach_slice(src_att, real_src, src_level, src_layer);
				attach_slice(dst_att, real_dst, dst_level, dst_layer);

				blit_src.check();
				blit_dst.check();

				blit_src.blit(blit_dst, src_area, dst_area, target, interp);

				if (remaining_levels > 1)
				{
					increment_mip_level(src_area);
					increment_mip_level(dst_area);

					src_level++;
					dst_level++;
				}
			}

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
