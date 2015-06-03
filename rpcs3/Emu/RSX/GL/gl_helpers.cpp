#include "stdafx.h"
#include "gl_helpers.h"

namespace gl
{
	void fbo::create()
	{
		if (created())
		{
			remove();
		}

		glGenFramebuffers(1, &m_id);
	}

	void fbo::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_id);
	}

	void fbo::blit(const fbo& dst, area src_area, area dst_area, buffers buffers_, filter filter_) const
	{
		bind_as(target::read_frame_buffer);
		dst.bind_as(target::draw_frame_buffer);
		glBlitFramebuffer(
			src_area.x1, src_area.y1, src_area.x2, src_area.y2,
			dst_area.x1, dst_area.y1, dst_area.x2, dst_area.y2,
			(GLbitfield)buffers_, (GLenum)filter_);
	}

	void fbo::bind_as(target target_) const
	{
		glBindFramebuffer((int)target_, id());
	}

	void fbo::remove()
	{
		if (!created())
		{
			return;
		}

		glDeleteFramebuffers(1, &m_id);
		m_id = 0;
	}

	bool fbo::created() const
	{
		return m_id != 0;
	}
}