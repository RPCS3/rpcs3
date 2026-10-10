#pragma once

#include "../glutils/fbo.h"
#include "../glutils/image.h"

#include "upscaling.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace gl
{
	// Sharp bilinear scaling. The image is first scaled up by an integer factor using nearest neighbor filtering
	// and then scaled to the final size using bilinear filtering.
	// This keeps pixels crisp while avoiding the uneven pixel sizes of plain nearest neighbor scaling at non-integer ratios.
	class sharp_bilinear_upscale_pass : public upscaler
	{
	public:
		sharp_bilinear_upscale_pass() = default;

		~sharp_bilinear_upscale_pass()
		{
			dispose_fbos();
		}

		gl::texture* scale_output(
			gl::command_context& /*cmd*/,           // State
			gl::texture* src,                       // Source input
			const areai& src_region,                // Scaling request information
			const areai& dst_region,                // Ditto
			gl::flags32_t mode                      // Mode
		) override
		{
			if (!(mode & UPSCALE_AND_COMMIT))
			{
				// Upscaling source only is unsupported
				return src;
			}

			const int src_w = std::abs(src_region.x2 - src_region.x1);
			const int src_h = std::abs(src_region.y2 - src_region.y1);
			const int dst_w = std::abs(dst_region.x2 - dst_region.x1);
			const int dst_h = std::abs(dst_region.y2 - dst_region.y1);

			// Integer prescale factors. Only useful when upscaling.
			const int scale_x = (src_w > 0 && dst_w > src_w) ? ((dst_w + src_w - 1) / src_w) : 1;
			const int scale_y = (src_h > 0 && dst_h > src_h) ? ((dst_h + src_h - 1) / src_h) : 1;

			m_src_fbo.recreate();
			m_src_fbo.bind();
			m_src_fbo.color = src->id();
			m_src_fbo.read_buffer(m_src_fbo.color);
			m_src_fbo.draw_buffer(m_src_fbo.color);

			if (scale_x > 1 || scale_y > 1)
			{
				const u32 prescaled_w = static_cast<u32>(src_w * scale_x);
				const u32 prescaled_h = static_cast<u32>(src_h * scale_y);

				if (!m_prescaled || m_prescaled->width() != prescaled_w || m_prescaled->height() != prescaled_h)
				{
					m_prescaled = std::make_unique<gl::viewable_image>(
						GL_TEXTURE_2D,
						prescaled_w, prescaled_h, 1, 1, 1,
						GL_RGBA8, RSX_FORMAT_CLASS_COLOR);
				}

				m_prescaled_fbo.recreate();
				m_prescaled_fbo.bind();
				m_prescaled_fbo.color = m_prescaled->id();
				m_prescaled_fbo.read_buffer(m_prescaled_fbo.color);
				m_prescaled_fbo.draw_buffer(m_prescaled_fbo.color);

				// Pass 1: Integer upscale with nearest filtering. Mirroring is preserved by the source region.
				m_src_fbo.blit(m_prescaled_fbo, src_region, { 0, 0, static_cast<int>(prescaled_w), static_cast<int>(prescaled_h) }, gl::buffers::color, gl::filter::nearest);

				// Pass 2: Scale to the final size with bilinear filtering
				m_prescaled_fbo.blit(gl::screen, { 0, 0, static_cast<int>(prescaled_w), static_cast<int>(prescaled_h) }, dst_region, gl::buffers::color, gl::filter::linear);
				return nullptr;
			}

			// Not an upscale, plain bilinear is all that is needed
			m_src_fbo.blit(gl::screen, src_region, dst_region, gl::buffers::color, gl::filter::linear);
			return nullptr;
		}

	private:
		void dispose_fbos()
		{
			if (m_src_fbo)
			{
				m_src_fbo.remove();
			}

			if (m_prescaled_fbo)
			{
				m_prescaled_fbo.remove();
			}
		}

		gl::fbo m_src_fbo;
		gl::fbo m_prescaled_fbo;
		std::unique_ptr<gl::viewable_image> m_prescaled;
	};
}
