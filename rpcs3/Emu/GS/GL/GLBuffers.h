#pragma once
#include "OpenGL.h"

struct BufferObject
{
protected:
	u32 m_id;
	u32 m_type;
	
public:
	BufferObject();
	BufferObject(u32 type);
	
	void Create(u32 type);
	void Delete();
	void Bind();
	void UnBind();
	void SetData(const void* data, u32 size, u32 type = GL_DYNAMIC_DRAW);
};

struct VBO : public BufferObject
{
	void Create();
};