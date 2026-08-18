#pragma once

#include "common.h"
#include "fbo.h"

namespace gl
{
	class command_context;
	class texture;

	class blitter
	{
		struct save_binding_state
		{
			GLuint old_fbo;

			save_binding_state()
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&old_fbo));
			}

			~save_binding_state()
			{
				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
			}
		};

		fbo blit_src;
		fbo blit_dst;

	public:

		void init();

		void destroy();

		void scale_image(
			gl::command_context& cmd,
			const texture* src,
			texture* dst,
			const coord3i& src_rect,         //<- Note, negative w/h/d is allowed for flipping
			const coord3i& dst_rect,         //<- Note, negative w/h/d is allowed for flipping
			bool linear_interpolation,
			const rsx::typeless_xfer& xfer_info,
			const rsx::image_copy_subresource_layers& mip_layers = {});

		void scale_image(
			gl::command_context& cmd,
			const texture* src,
			texture* dst,
			areai src_rect,
			areai dst_rect,
			bool linear_interpolation,
			const rsx::typeless_xfer& xfer_info,
			const rsx::image_copy_subresource_layers& mip_layers = {})
		{
			const auto src_region = coord3i{ src_rect.x1, src_rect.y1, 0, src_rect.x2 - src_rect.x1, src_rect.y2 - src_rect.y1, 1 };
			const auto dst_region = coord3i{ dst_rect.x1, dst_rect.y1, 0, dst_rect.x2 - dst_rect.x1, dst_rect.y2 - dst_rect.y1, 1 };
			scale_image(cmd, src, dst, src_region, dst_region, linear_interpolation, xfer_info, mip_layers);
		}

		void copy_image(
			gl::command_context& cmd,
			const texture* src,
			const texture* dst,
			const position3i& src_offset,
			const position3i& dst_offset,
			const size3i& size,
			const rsx::image_copy_subresource_layers& mip_layers = {}) const;

		void fast_clear_image(gl::command_context& cmd, const texture* dst, const color4f& color);
		void fast_clear_image(gl::command_context& cmd, const texture* dst, float depth, u8 stencil);

		void copy_image(
			gl::command_context& cmd,
			const texture* src,
			const texture* dst,
			const position3u& src_offset,
			const position3u& dst_offset,
			const size3u& size,
			const rsx::image_copy_subresource_layers& mip_layers = {}) const
		{
			copy_image(cmd, src, dst, static_cast<position3i>(src_offset), static_cast<position3i>(dst_offset), static_cast<size3i>(size), mip_layers);
		}
	};

	extern blitter* g_hw_blitter;
}
