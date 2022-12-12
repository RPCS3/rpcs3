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

		void init()
		{
			blit_src.create();
			blit_dst.create();
		}

		void destroy()
		{
			blit_dst.remove();
			blit_src.remove();
		}

		void scale_image(gl::command_context& cmd, const texture* src, texture* dst, areai src_rect, areai dst_rect, bool linear_interpolation,
			const rsx::typeless_xfer& xfer_info);

		void copy_image(gl::command_context& cmd, const texture* src, const texture* dst, int src_level, int dst_level, const position3i& src_offset, const position3i& dst_offset, const size3i& size) const;

		void fast_clear_image(gl::command_context& cmd, const texture* dst, const color4f& color);
		void fast_clear_image(gl::command_context& cmd, const texture* dst, float depth, u8 stencil);

		void copy_image(gl::command_context& cmd, const texture* src, const texture* dst, int src_level, int dst_level, const position3u& src_offset, const position3u& dst_offset, const size3u& size) const
		{
			copy_image(cmd, src, dst, src_level, dst_level, static_cast<position3i>(src_offset), static_cast<position3i>(dst_offset), static_cast<size3i>(size));
		}
	};

	extern blitter* g_hw_blitter;
}
