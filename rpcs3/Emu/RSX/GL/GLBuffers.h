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
	void SetAttribPointer(int location, int size, int type, GLvoid* pointer, int stride, bool normalized = false);
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

namespace gl
{
	enum class filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR
	};

	enum class buffers
	{
		color = GL_COLOR_BUFFER_BIT,
		depth = GL_DEPTH_BUFFER_BIT,
		stencil = GL_STENCIL_BUFFER_BIT,

		color_depth = color | depth,
		color_depth_stencil = color | depth | stencil,
		color_stencil = color | stencil,

		depth_stencil = depth | stencil
	};

	class fbo
	{
		GLuint m_id = GL_NONE;

	public:
		enum class target
		{
			read_frame_buffer = GL_READ_FRAMEBUFFER,
			draw_frame_buffer = GL_DRAW_FRAMEBUFFER
		};
		void create();
		void bind() const;
		void texture1D(u32 attachment, u32 texture, int level = 0) const;
		void texture2D(u32 attachment, u32 texture, int level = 0) const;
		void texture3D(u32 attachment, u32 texture, int zoffset = 0, int level = 0) const;
		void renderbuffer(u32 attachment, u32 renderbuffer) const;
		void blit(const fbo& dst, area src_area, area dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void clear();
		bool is_created() const;

		GLuint id() const
		{
			return m_id;
		}

		void set_id(GLuint id)
		{
			m_id = id;
		}

		operator bool() const
		{
			return is_created();
		}
	};

	static const fbo screen;
}
