#include "stdafx.h"
#include "GLBuffers.h"

GLBufferObject::GLBufferObject()
{
}

GLBufferObject::GLBufferObject(u32 type)
{
	Create(type);
}

GLBufferObject::~GLBufferObject()
{
	Delete();
}
	
void GLBufferObject::Create(GLuint type, u32 count)
{
	if(IsCreated()) return;

	m_id.resize(count);
	glGenBuffers(count, &m_id[0]);
	m_type = type;
}

void GLBufferObject::Delete()
{
	if(!IsCreated()) return;

	glDeleteBuffers(m_id.size(), &m_id[0]);
	m_id.clear();
	m_type = 0;
}

void GLBufferObject::Bind(u32 type, u32 num)
{
	assert(num < m_id.size());
	glBindBuffer(type, m_id[num]);
}

void GLBufferObject::UnBind(u32 type)
{
	glBindBuffer(type, 0);
}

void GLBufferObject::Bind(u32 num)
{
	Bind(m_type, num);
}

void GLBufferObject::UnBind()
{
	UnBind(m_type);
}

void GLBufferObject::SetData(u32 type, const void* data, u32 size, u32 usage)
{
	glBufferData(type, size, data, usage);
}

void GLBufferObject::SetData(const void* data, u32 size, u32 usage)
{
	SetData(m_type, data, size, usage);
}

void GLBufferObject::SetAttribPointer(int location, int size, int type, GLvoid* pointer, int stride, bool normalized)
{
	if(location < 0) return;

	glVertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
	glEnableVertexAttribArray(location);
}

bool GLBufferObject::IsCreated() const
{
	return m_id.size() != 0;
}

GLvbo::GLvbo()
{
}

void GLvbo::Create(u32 count)
{
	GLBufferObject::Create(GL_ARRAY_BUFFER, count);
}

GLvao::GLvao() : m_id(0)
{
}

GLvao::~GLvao()
{
	Delete();
}

void GLvao::Create()
{
	if(!IsCreated()) glGenVertexArrays(1, &m_id);
}

void GLvao::Bind() const
{
	glBindVertexArray(m_id);
}

void GLvao::Unbind()
{
	glBindVertexArray(0);
}

void GLvao::Delete()
{
	if(!IsCreated()) return;

	Unbind();
	glDeleteVertexArrays(1, &m_id);
	m_id = 0;
}

bool GLvao::IsCreated() const
{
	return m_id != 0;
}

GLrbo::GLrbo()
{
}

GLrbo::~GLrbo()
{
}

void GLrbo::Create(u32 count)
{
	if(m_id.size() == count)
	{
		return;
	}

	Delete();

	m_id.resize(count);
	glGenRenderbuffers(count, m_id.data());
}

void GLrbo::Bind(u32 num) const
{
	assert(num < m_id.size());

	glBindRenderbuffer(GL_RENDERBUFFER, m_id[num]);
}

void GLrbo::Storage(u32 format, u32 width, u32 height)
{
	glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
}

void GLrbo::Unbind()
{
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GLrbo::Delete()
{
	if(!IsCreated())
	{
		return;
	}

	glDeleteRenderbuffers(m_id.size(), m_id.data());
	m_id.clear();
}

bool GLrbo::IsCreated() const
{
	return m_id.size();
}

u32 GLrbo::GetId(u32 num) const
{
	assert(num < m_id.size());
	return m_id[num];
}

namespace gl
{
	void fbo::create()
	{
		if (created())
		{
			clear();
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

	void fbo::clear()
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