#pragma once
#include "OpenGL.h"

struct GLBufferObject
{
protected:
	Array<GLuint> m_id;
	GLuint m_type;
	
public:
	GLBufferObject();
	GLBufferObject(u32 type);

	~GLBufferObject();

	void Create(GLuint type, u32 count = 1);
	void Delete();
	void Bind(u32 type, u32 num);
	void UnBind(u32 type);
	void Bind(u32 num = 0);
	void UnBind();
	void SetData(u32 type, const void* data, u32 size, u32 usage = GL_DYNAMIC_DRAW);
	void SetData(const void* data, u32 size, u32 usage = GL_DYNAMIC_DRAW);
	void SetAttribPointer(int location, int size, int type, int pointer, int stride, bool normalized = false);
	bool IsCreated() const;
};

struct GLvbo : public GLBufferObject
{
	GLvbo();

	void Create(u32 count = 1);
};

class GLvao
{
protected:
	GLuint m_id;

public:
	GLvao();
	~GLvao();

	void Create();
	void Bind() const;
	void Delete();
	bool IsCreated() const;
};