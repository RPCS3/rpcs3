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

	class rbo
	{
		GLuint m_id = 0;

	public:
		class save_binding_state
		{
			GLint m_old_value;

		public:
			save_binding_state(const rbo& new_state)
			{
				glGetIntegerv(GL_RENDERBUFFER_BINDING, &m_old_value);
				new_state.bind();
			}

			~save_binding_state()
			{
				glBindRenderbuffer(GL_RENDERBUFFER, m_old_value);
			}
		};

		void create()
		{
			if (created())
			{
				clear();
			}

			glGenRenderbuffers(1, &m_id);
		}

		void bind() const
		{
			glBindRenderbuffer(GL_RENDERBUFFER, m_id);
		}

		void storage(u32 format, u32 width, u32 height)
		{
			if (!created())
			{
				create();
			}

			save_binding_state save(*this);
			glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
		}

		void clear()
		{
			glDeleteRenderbuffers(1, &m_id);
		}

		uint id() const
		{
			return m_id;
		}

		bool created() const
		{
			return m_id != 0;
		}

		explicit operator bool() const
		{
			return created();
		}
	};

	//TODO
	class texture
	{
		GLuint m_id = 0;
		GLuint m_level = 0;

	public:
		enum class type
		{
			ubyte = GL_UNSIGNED_BYTE,
			ushort = GL_UNSIGNED_SHORT,
			uint = GL_UNSIGNED_INT,

			ubyte_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
			ubyte_2_3_3_rev = GL_UNSIGNED_BYTE_2_3_3_REV,

			ushort_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
			ushort_5_6_5_rev = GL_UNSIGNED_SHORT_5_6_5_REV,
			ushort_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
			ushort_4_4_4_4_rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
			ushort_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
			ushort_1_5_5_5_rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,

			uint_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
			uint_8_8_8_8_rev = GL_UNSIGNED_INT_8_8_8_8_REV,
			uint_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
			uint_2_10_10_10_rev = GL_UNSIGNED_INT_2_10_10_10_REV,

			sbyte = GL_BYTE,
			sshort = GL_SHORT,
			sint = GL_INT,
			float_ = GL_FLOAT,
			double_ = GL_DOUBLE,
		};

		enum class channel
		{
			zero = GL_ZERO,
			one = GL_ONE,
			r = GL_RED,
			g = GL_GREEN,
			b = GL_BLUE,
			a = GL_ALPHA,
		};

		enum class format
		{
			red = GL_RED,
			r = GL_R,
			rg = GL_RG,
			rgb = GL_RGB,
			rgba = GL_RGBA,

			bgr = GL_BGR,
			bgra = GL_BGRA,

			depth = GL_DEPTH_COMPONENT,
			depth_stencil = GL_DEPTH_STENCIL,

			compressed_rgb_s3tc_dxt1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt1 = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
			compressed_rgba_s3tc_dxt3 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
			compressed_rgba_s3tc_dxt5 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
		};

		enum class wrap
		{
			repeat = GL_REPEAT,
			mirrored_repeat = GL_MIRRORED_REPEAT,
			clamp_to_edge = GL_CLAMP_TO_EDGE,
			clamp_to_border = GL_CLAMP_TO_BORDER,
			mirror_clamp = GL_MIRROR_CLAMP_EXT,
			mirror_clamp_to_edge = GL_MIRROR_CLAMP_TO_EDGE,
			mirror_clamp_to_border = GL_MIRROR_CLAMP_TO_BORDER_EXT
		};

		static bool compressed_format(format format_)
		{
			switch (format_)
			{
			case format::compressed_rgb_s3tc_dxt1:
			case format::compressed_rgba_s3tc_dxt1:
			case format::compressed_rgba_s3tc_dxt3:
			case format::compressed_rgba_s3tc_dxt5:
				return true;
			}

			return false;
		}

		uint id() const
		{
			return m_id;
		}

		uint level() const
		{
			return m_level;
		}

		void create()
		{
			if (created())
				clear();

			glGenTextures(1, &m_id);
		}

		bool created() const
		{
			return m_id != 0;
		}

		void clear()
		{
			glDeleteTextures(1, &m_id);
		}

		void set_id(GLuint id)
		{
			m_id = id;
		}

		void set_level(int level)
		{
			m_level = level;
		}

		texture with_level(int level)
		{
			texture result = *this;
			result.set_level(level);

			return result;
		}

		explicit operator bool() const
		{
			return created();
		}
	};

	//TODO
	class texture1d : public texture
	{
	public:
		class save_binding_state
		{
			GLint m_last_binding;

		public:
			save_binding_state(const texture1d& new_binding)
			{
				glGetIntegerv(GL_TEXTURE_1D_BINDING_EXT, &m_last_binding);
				new_binding.bind();
			}

			~save_binding_state()
			{
				glBindTexture(GL_TEXTURE_1D, m_last_binding);
			}
		};

		void bind() const
		{
			glBindTexture(GL_TEXTURE_1D, id());
		}

	};

	//TODO
	class texture2d : public texture
	{
	public:
		class configure
		{
			texture2d &m_parent;

			texture::channel m_swizzle_r = texture::channel::r;
			texture::channel m_swizzle_g = texture::channel::g;
			texture::channel m_swizzle_b = texture::channel::b;
			texture::channel m_swizzle_a = texture::channel::a;

			texture::format m_format = texture::format::rgba;
			texture::format m_internal_format = texture::format::rgba;
			texture::type m_type = texture::type::ubyte;

			uint m_width = 0;
			uint m_height = 0;
			int m_border = 0;
			int m_level = 0;

			int m_compressed_image_size = 0;

			const void* m_pixels = nullptr;

			bool m_unpack_swap_bypes = false;
			bool m_pack_swap_bypes = false;

		public:
			configure(texture2d &parent) : m_parent(parent)
			{
			}

			~configure()
			{
				save_binding_state save(m_parent);

				glPixelStorei(GL_UNPACK_SWAP_BYTES, m_unpack_swap_bypes ? GL_TRUE : GL_FALSE);
				glPixelStorei(GL_PACK_SWAP_BYTES, m_pack_swap_bypes ? GL_TRUE : GL_FALSE);

				if (compressed_format(m_internal_format))
				{
					if (!m_compressed_image_size)
					{
						switch (m_internal_format)
						{
						case format::compressed_rgb_s3tc_dxt1:
							m_compressed_image_size = ((m_width + 2) / 3) * ((m_height + 2) / 3) * 6;
							break;

						case format::compressed_rgba_s3tc_dxt1:
							m_compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 8;
							break;

						case format::compressed_rgba_s3tc_dxt3:
						case format::compressed_rgba_s3tc_dxt5:
							m_compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 16;
							break;
						}
					}

					glCompressedTexImage2D(GL_TEXTURE_2D, m_level, (GLint)m_internal_format, m_width, m_height, m_border, m_compressed_image_size, m_pixels);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_2D, m_level, (GLint)m_internal_format, m_width, m_height, 0, (GLint)m_format, (GLint)m_type, m_pixels);
				}
			}

			configure& swizzle(
				texture::channel r = texture::channel::r,
				texture::channel g = texture::channel::g,
				texture::channel b = texture::channel::b,
				texture::channel a = texture::channel::a)
			{
				m_swizzle_r = r;
				m_swizzle_g = g;
				m_swizzle_b = b;
				m_swizzle_a = a;

				return *this;
			}

			configure& format(texture::format format)
			{
				m_format = format;

				return *this;
			}

			configure& internal_format(texture::format format)
			{
				m_internal_format = format;

				return *this;
			}

			configure& pixels(const void* pixels)
			{
				m_pixels = pixels;

				return *this;
			}

			configure& compressed_image_size(int size)
			{
				m_compressed_image_size = size;

				return *this;
			}

			configure& width(uint width)
			{
				m_width = width;

				return *this;
			}

			configure& height(uint height)
			{
				m_height = height;

				return *this;
			}

			configure& size(uint width_, uint height_)
			{
				return width(width_).height(height_);
			}

			configure& border(int size)
			{
				m_border = size;
				return *this;
			}

			configure& level(int value)
			{
				m_level = value;
				return *this;
			}
		};

		class save_binding_state
		{
			GLint m_last_binding;

		public:
			save_binding_state(const texture2d& new_binding)
			{
				glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &m_last_binding);
				new_binding.bind();
			}

			~save_binding_state()
			{
				glBindTexture(GL_TEXTURE_2D, m_last_binding);
			}
		};

		void bind() const
		{
			glBindTexture(GL_TEXTURE_2D, id());
		}
	};

	//TODO
	class texture3d : public texture
	{
	public:
		class save_binding_state
		{
			GLint m_last_binding;

		public:
			save_binding_state(const texture3d& new_binding)
			{
				glGetIntegerv(GL_TEXTURE_3D_BINDING_EXT, &m_last_binding);
				new_binding.bind();
			}

			~save_binding_state()
			{
				glBindTexture(GL_TEXTURE_3D, m_last_binding);
			}
		};

		void bind() const
		{
			glBindTexture(GL_TEXTURE_3D, id());
		}
	};

	class fbo
	{
		GLuint m_id = GL_NONE;

	public:
		class save_binding_state
		{
			GLint m_last_binding;

		public:
			save_binding_state(const fbo& new_binding)
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_last_binding);
				new_binding.bind();
			}

			~save_binding_state()
			{
				glBindFramebuffer(GL_FRAMEBUFFER, m_last_binding);
			}
		};

		class attachment
		{
		public:
			enum class type
			{
				color = GL_COLOR_ATTACHMENT0,
				depth = GL_DEPTH_ATTACHMENT,
				stencil = GL_STENCIL_ATTACHMENT,
				depth_stencil = GL_DEPTH_STENCIL_ATTACHMENT
			};

		protected:
			GLuint m_id;
			fbo &m_parent;

		public:
			attachment(fbo& parent, type type)
				: m_id((int)type)
				, m_parent(parent)
			{
			}

			void set_id(uint id)
			{
				m_id = id;
			}

			uint id() const
			{
				return m_id;
			}

			void operator = (const rbo& rhs)
			{
				save_binding_state save(m_parent);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_id, GL_RENDERBUFFER, rhs.id());
			}

			void operator = (const texture1d& rhs)
			{
				save_binding_state save(m_parent);
				glFramebufferTexture1D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_1D, rhs.id(), rhs.level());
			}

			void operator = (const texture2d& rhs)
			{
				save_binding_state save(m_parent);
				glFramebufferTexture2D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_2D, rhs.id(), rhs.level());
			}

			void operator = (const texture3d& rhs)
			{
				save_binding_state save(m_parent);
				glFramebufferTexture3D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_3D, rhs.id(), rhs.level(), 0);
			}
		};

		class indexed_attachment : public attachment
		{
		public:
			indexed_attachment(fbo& parent, type type) : attachment(parent, type)
			{
			}

			attachment operator[](int index) const
			{
				return{ m_parent, type(id() + index) };
			}

			using attachment::operator = ;
		};

		indexed_attachment color{ *this, attachment::type::color };
		attachment depth{ *this, attachment::type::depth };
		attachment stencil{ *this, attachment::type::stencil };

		enum class target
		{
			read_frame_buffer = GL_READ_FRAMEBUFFER,
			draw_frame_buffer = GL_DRAW_FRAMEBUFFER
		};

		void create();
		void bind() const;
		void blit(const fbo& dst, area src_area, area dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void clear();
		bool created() const;

		fbo() = default;

		fbo(GLuint id)
		{
			set_id(id);
		}

		static fbo get_binded_draw_buffer()
		{
			GLint value;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);

			return{ (GLuint)value };
		}

		static fbo get_binded_read_buffer()
		{
			GLint value;
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value);

			return{ (GLuint)value };
		}

		static fbo get_binded_buffer()
		{
			GLint value;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &value);

			return{ (GLuint)value };
		}

		GLuint id() const
		{
			return m_id;
		}

		void set_id(GLuint id)
		{
			m_id = id;
		}

		explicit operator bool() const
		{
			return created();
		}
	};

	static const fbo screen;
}
