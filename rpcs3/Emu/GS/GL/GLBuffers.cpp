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

	m_id.InsertRoomEnd(count);
	glGenBuffers(count, &m_id[0]);
	m_type = type;
}

void GLBufferObject::Delete()
{
	if(!IsCreated()) return;

	glDeleteBuffers(m_id.GetCount(), &m_id[0]);
	m_id.Clear();
	m_type = 0;
}

void GLBufferObject::Bind(u32 type, u32 num)
{
	assert(num < m_id.GetCount());
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
	return m_id.GetCount() != 0;
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