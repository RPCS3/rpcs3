#include "stdafx.h"
#include "GLBuffers.h"
#include "GLGSRender.h"

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

void GLBufferObject::SetAttribPointer(int location, int size, int type, int pointer, int stride, bool normalized)
{
	if(location < 0) return;

	glVertexAttribPointer(location, size, type, normalized ? GL_TRUE : GL_FALSE, stride, (const GLvoid*)pointer);
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

GLfbo::GLfbo() : m_id(0)
{
}

GLfbo::~GLfbo()
{
}

void GLfbo::Create()
{
	if(IsCreated())
	{
		return;
	}

	glGenFramebuffers(1, &m_id);
}

void GLfbo::Bind(u32 type, int id)
{
	glBindFramebuffer(type, id);
}

void GLfbo::Bind(u32 type)
{
	assert(IsCreated());

	m_type = type;
	Bind(type, m_id);
}

void GLfbo::Texture1D(u32 attachment, u32 texture, int level)
{
	glFramebufferTexture1D(m_type, attachment, GL_TEXTURE_1D, texture, level);
}

void GLfbo::Texture2D(u32 attachment, u32 texture, int level)
{
	glFramebufferTexture2D(m_type, attachment, GL_TEXTURE_2D, texture, level);
}

void GLfbo::Texture3D(u32 attachment, u32 texture, int zoffset, int level)
{
	glFramebufferTexture3D(m_type, attachment, GL_TEXTURE_3D, texture, level, zoffset);
}

void GLfbo::Renderbuffer(u32 attachment, u32 renderbuffer)
{
	glFramebufferRenderbuffer(m_type, attachment, GL_RENDERBUFFER, renderbuffer);
}

void GLfbo::Blit(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, u32 mask, u32 filter)
{
	glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GLfbo::Unbind()
{
	Unbind(m_type);
}

void GLfbo::Unbind(u32 type)
{
	glBindFramebuffer(type, 0);
}

void GLfbo::Delete()
{
	if(!IsCreated())
	{
		return;
	}

	glDeleteFramebuffers(1, &m_id);
	m_id = 0;
}

bool GLfbo::IsCreated() const
{
	return m_id != 0;
}
