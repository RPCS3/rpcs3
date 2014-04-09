#pragma once
#include "OpenGL.h"

struct GLBufferObject
{
protected:
	std::vector<GLuint> m_id;
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
	static void Unbind();
	void Delete();
	bool IsCreated() const;
};

class GLrbo
{
protected:
	std::vector<GLuint> m_id;

public:
	GLrbo();
	~GLrbo();

	void Create(u32 count = 1);
	void Bind(u32 num = 0) const;
	void Storage(u32 format, u32 width, u32 height);
	static void Unbind();
	void Delete();
	bool IsCreated() const;
	u32 GetId(u32 num = 0) const;
};

class GLfbo
{
protected:
	GLuint m_id;
	GLuint m_type;

public:
	GLfbo();
	~GLfbo();

	void Create();
	static void Bind(u32 type, int id);
	void Bind(u32 type = GL_FRAMEBUFFER);
	void Texture1D(u32 attachment, u32 texture, int level = 0);
	void Texture2D(u32 attachment, u32 texture, int level = 0);
	void Texture3D(u32 attachment, u32 texture, int zoffset = 0, int level = 0);
	void Renderbuffer(u32 attachment, u32 renderbuffer);
	static void Blit(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, u32 mask, u32 filter);
	void Unbind();
	static void Unbind(u32 type);
	void Delete();
	bool IsCreated() const;
};
