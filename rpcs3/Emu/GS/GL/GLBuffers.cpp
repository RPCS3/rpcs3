#include "stdafx.h"
#include "GLBuffers.h"
#include "GLGSRender.h"

BufferObject::BufferObject() 
	: m_id(0)
	, m_type(0)
{
}

BufferObject::BufferObject(u32 type) 
	: m_id(0)
	, m_type(0)
{
	Create(type);
}

void BufferObject::Create(u32 type)
{
	if(m_id) return;
	glGenBuffers(1, &m_id);
	m_type = type;
}

void BufferObject::Delete()
{
	if(!m_id) return;
	glDeleteBuffers(1, &m_id);
	m_id = 0;
	m_type = 0;
}

void BufferObject::Bind()
{
	if(m_id) glBindBuffer(m_type, m_id);
}

void BufferObject::UnBind()
{
	glBindBuffer(m_type, 0);
}

void BufferObject::SetData(const void* data, u32 size, u32 type)
{
	if(!m_type) return;
	Bind();
	glBufferData(m_type, size, data, type);
}

void VBO::Create()
{
	BufferObject::Create(GL_ARRAY_BUFFER);
}