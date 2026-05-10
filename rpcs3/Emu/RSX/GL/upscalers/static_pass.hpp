#pragma once

#include "../glutils/fbo.h"

#include "upscaling.h"

namespace gl
{
	template<gl::filter Filter>
	class static_upscale_pass : public upscaler
	{
	public:
		static_upscale_pass() = default;
		~static_upscale_pass()
		{
			if (m_flip_fbo)
			{
				m_flip_fbo.remove();
			}
		}

		gl::texture* scale_output(
			gl::command_context& /*cmd*/,           // State
			gl::texture*   src,                     // Source input
			const areai& src_region,                // Scaling request information
			const areai& dst_region,                // Ditto
			gl::flags32_t mode                      // Mode
		) override
		{
			if (mode & UPSCALE_AND_COMMIT)
			{
				m_flip_fbo.recreate();
				m_flip_fbo.bind();
				m_flip_fbo.color = src->id();
				m_flip_fbo.read_buffer(m_flip_fbo.color);
				m_flip_fbo.draw_buffer(m_flip_fbo.color);
				m_flip_fbo.blit(gl::screen, src_region, dst_region, gl::buffers::color, Filter);
				return nullptr;
			}

			// Upscaling source only is unsupported
			return src;
		}

	private:
		gl::fbo m_flip_fbo;
	};
}
