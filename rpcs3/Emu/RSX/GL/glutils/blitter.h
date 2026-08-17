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
			areai src_rect,
			areai dst_rect,
			bool linear_interpolation,
			const rsx::typeless_xfer& xfer_info,
			const rsx::image_copy_subresource_layers& mip_layers = {});

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
