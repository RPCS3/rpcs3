#pragma once
#include "OpenGL.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void printGlError(GLenum err, const char* situation);
void printGlError(GLenum err, const std::string& situation);
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);
extern GLenum g_last_gl_error;
#define checkForGlError(sit) if((g_last_gl_error = glGetError()) != GL_NO_ERROR) printGlError(g_last_gl_error, sit)


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
	class exception : public std::exception
	{
	protected:
		std::string m_what;

	public:
		const char* what() const override
		{
			return m_what.c_str();
		}
	};

	enum class filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR
	};

	enum class min_filter
	{
		nearest = GL_NEAREST,
		linear = GL_LINEAR,
		nearest_mipmap_nearest = GL_NEAREST_MIPMAP_NEAREST,
		nearest_mipmap_linear = GL_NEAREST_MIPMAP_LINEAR,
		linear_mipmap_nearest = GL_LINEAR_MIPMAP_NEAREST,
		linear_mipmap_linear = GL_LINEAR_MIPMAP_LINEAR
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

	class pixel_settings
	{
		bool m_pack_swap_bytes = false;
		bool m_pack_lsb_first = false;
		int m_pack_row_length = 0;
		int m_pack_image_height = 0;
		int m_pack_skip_rows = 0;
		int m_pack_skip_pixels = 0;
		int m_pack_skip_images = 0;
		int m_pack_aligment = 4;
		bool m_unpack_swap_bytes = false;
		bool m_unpack_lsb_first = false;
		int m_unpack_row_length = 0;
		int m_unpack_image_height = 0;
		int m_unpack_skip_rows = 0;
		int m_unpack_skip_pixels = 0;
		int m_unpack_skip_images = 0;
		int m_unpack_aligment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_PACK_SWAP_BYTES, m_pack_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_LSB_FIRST, m_pack_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_ROW_LENGTH, m_pack_row_length);
			glPixelStorei(GL_PACK_IMAGE_HEIGHT, m_pack_image_height);
			glPixelStorei(GL_PACK_SKIP_ROWS, m_pack_skip_rows);
			glPixelStorei(GL_PACK_SKIP_PIXELS, m_pack_skip_pixels);
			glPixelStorei(GL_PACK_SKIP_IMAGES, m_pack_skip_images);
			glPixelStorei(GL_PACK_ALIGNMENT, m_pack_aligment);

			glPixelStorei(GL_UNPACK_SWAP_BYTES, m_unpack_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_LSB_FIRST, m_unpack_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, m_unpack_row_length);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, m_unpack_image_height);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, m_unpack_skip_rows);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, m_unpack_skip_pixels);
			glPixelStorei(GL_UNPACK_SKIP_IMAGES, m_unpack_skip_images);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_unpack_aligment);
		}

		pixel_settings& pack_swap_bytes(bool value = true)
		{
			m_pack_swap_bytes = value;
			return *this;
		}
		pixel_settings& pack_lsb_first(bool value = true)
		{
			m_pack_lsb_first = value;
			return *this;
		}
		pixel_settings& pack_row_length(int value)
		{
			m_pack_row_length = value;
			return *this;
		}
		pixel_settings& pack_image_height(int value)
		{
			m_pack_image_height = value;
			return *this;
		}
		pixel_settings& pack_skip_rows(int value)
		{
			m_pack_skip_rows = value;
			return *this;
		}
		pixel_settings& pack_skip_pixels(int value)
		{
			m_pack_skip_pixels = value;
			return *this;
		}
		pixel_settings& pack_skip_images(int value)
		{
			m_pack_skip_images = value;
			return *this;
		}
		pixel_settings& pack_aligment(int value)
		{
			m_pack_aligment = value;
			return *this;
		}
		pixel_settings& unpack_swap_bytes(bool value = true)
		{
			m_unpack_swap_bytes = value;
			return *this;
		}
		pixel_settings& unpack_lsb_first(bool value = true)
		{
			m_unpack_lsb_first = value;
			return *this;
		}
		pixel_settings& unpack_row_length(int value)
		{
			m_unpack_row_length = value;
			return *this;
		}
		pixel_settings& unpack_image_height(int value)
		{
			m_unpack_image_height = value;
			return *this;
		}
		pixel_settings& unpack_skip_rows(int value)
		{
			m_unpack_skip_rows = value;
			return *this;
		}
		pixel_settings& unpack_skip_pixels(int value)
		{
			m_unpack_skip_pixels = value;
			return *this;
		}
		pixel_settings& unpack_skip_images(int value)
		{
			m_unpack_skip_images = value;
			return *this;
		}
		pixel_settings& unpack_aligment(int value)
		{
			m_unpack_aligment = value;
			return *this;
		}

		pixel_settings& swap_bytes(bool value = true)
		{
			return pack_swap_bytes(value).unpack_swap_bytes(value);
		}
		pixel_settings& lsb_first(bool value = true)
		{
			return pack_lsb_first(value).unpack_lsb_first(value);
		}
		pixel_settings& row_length(int value)
		{
			return pack_row_length(value).unpack_row_length(value);
		}
		pixel_settings& image_height(int value)
		{
			return pack_image_height(value).unpack_image_height(value);
		}
		pixel_settings& skip_rows(int value)
		{
			return pack_skip_rows(value).unpack_skip_rows(value);
		}
		pixel_settings& skip_pixels(int value)
		{
			return pack_skip_pixels(value).unpack_skip_pixels(value);
		}
		pixel_settings& skip_images(int value)
		{
			return pack_skip_images(value).unpack_skip_images(value);
		}
		pixel_settings& aligment(int value)
		{
			return pack_aligment(value).unpack_aligment(value);
		}
	};

	class pbo
	{
	public:
		enum class target
		{
			pack = GL_PIXEL_PACK_BUFFER,
			unpack = GL_PIXEL_UNPACK_BUFFER
		};
		enum class access
		{
			read = GL_READ_ONLY,
			write = GL_WRITE_ONLY,
			read_write = GL_READ_WRITE
		};

	private:
		GLuint m_id = GL_NONE;
		target m_target = target::unpack;

	public:
		pbo() = default;

		pbo(GLuint id)
		{
			set_id(id);
		}

		~pbo()
		{
			remove();
		}

		class save_binding_state
		{
			GLint m_last_binding;
			GLenum m_target;

		public:
			save_binding_state(target target_, const pbo& new_state) : save_binding_state(target_)
			{
				new_state.bind(target_);
			}

			save_binding_state(target target_)
			{
				GLenum pname;
				switch (target_)
				{
				case target::pack: pname = GL_PIXEL_PACK_BUFFER_BINDING; break;
				case target::unpack: pname = GL_PIXEL_UNPACK_BUFFER_BINDING; break;
				}

				glGetIntegerv(pname, &m_last_binding);
				m_target = (GLenum)target_;
			}


			~save_binding_state()
			{
				glBindBuffer(m_target, m_last_binding);
			}
		};

		void create()
		{
			if (created())
			{
				remove();
			}

			glGenBuffers(1, &m_id);
		}

		void create(GLsizeiptr size)
		{
			create();
			data(size);
		}

		void data(GLsizeiptr size)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);
			glBufferData((GLenum)target_, size, 0, GL_STREAM_COPY);
		}

		void bind(target target_) const
		{
			glBindBuffer((GLenum)target_, m_id);
		}

		target current_target() const
		{
			return m_target;
		}

		void remove()
		{
			glDeleteBuffers(1, &m_id);
			m_id = 0;
		}

		uint id() const
		{
			return m_id;
		}

		void set_id(uint id)
		{
			m_id = id;
		}

		bool created() const
		{
			return m_id != 0;
		}

		explicit operator bool() const
		{
			return created();
		}

		void map(std::function<void(GLubyte*)> impl, access access_, GLsizeiptr data_size = 0)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);

			if (data_size != 0)
			{
				GLenum usage;

				switch (access_)
				{
				case access::read: usage = GL_STREAM_READ; break;
				case access::write: usage = GL_STREAM_DRAW; break;
				case access::read_write: usage = GL_STREAM_DRAW; break;
				}

				glBufferData((GLenum)target_, data_size, 0, usage);
			}

			if (GLubyte* ptr = (GLubyte*)glMapBuffer((GLenum)target_, (GLenum)access_))
			{
				impl(ptr);
				glUnmapBuffer((GLenum)target_);
			}
		}

		class mapper
		{
			pbo *m_parent;
			GLubyte *m_data;

		public:
			mapper(pbo& parent, access access_, GLsizeiptr data_size = 0)
			{
				m_parent = &parent;
				m_data = parent.map(access_, data_size);
			}

			~mapper()
			{
				m_parent->unmap();
			}

			GLubyte* get() const
			{
				return m_data;
			}
		};

		GLubyte* map(access access_, GLsizeiptr data_size = 0)
		{
			bind(current_target());

			if (data_size != 0)
			{
				GLenum usage;

				switch (access_)
				{
				case access::read: usage = GL_STREAM_READ; break;
				case access::write: usage = GL_STREAM_DRAW; break;
				case access::read_write: usage = GL_STREAM_DRAW; break;
				}

				glBufferData((GLenum)current_target(), data_size, 0, usage);
			}

			return (GLubyte*)glMapBuffer((GLenum)current_target(), (GLenum)access_);
		}

		void unmap()
		{
			glUnmapBuffer((GLenum)current_target());
		}
	};

	class texture
	{
		GLuint m_id = 0;
		GLuint m_level = 0;
		gl::pixel_settings m_pixel_settings;

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
			uint_24_8 = GL_UNSIGNED_INT_24_8,

			sbyte = GL_BYTE,
			sshort = GL_SHORT,
			sint = GL_INT,
			f16 = GL_HALF_FLOAT,
			f32 = GL_FLOAT,
			f64 = GL_DOUBLE,
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

			stencil = GL_STENCIL_INDEX,
			stencil8 = GL_STENCIL_INDEX8,
			depth = GL_DEPTH_COMPONENT,
			depth16 = GL_DEPTH_COMPONENT16,
			depth_stencil = GL_DEPTH_STENCIL,
			depth24_stencil8 = GL_DEPTH24_STENCIL8,

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

		enum class compare_mode
		{
			none = GL_NONE,
			ref_to_texture = GL_COMPARE_REF_TO_TEXTURE
		};

		enum class compare_func
		{
			never = GL_NEVER,
			less = GL_LESS,
			equal = GL_EQUAL,
			lequal = GL_LEQUAL,
			greater = GL_GREATER,
			notequal = GL_NOTEQUAL,
			gequal = GL_GEQUAL,
			always = GL_ALWAYS
		};

		enum class texture_target
		{
			texture1D = GL_TEXTURE_1D,
			texture2D = GL_TEXTURE_2D,
			texture3D = GL_TEXTURE_3D
		};

		enum class channel_type
		{
			none = GL_NONE,
			signed_normalized = GL_SIGNED_NORMALIZED,
			unsigned_normalized = GL_UNSIGNED_NORMALIZED,
			float_ = GL_FLOAT,
			int_ = GL_INT,
			uint_ = GL_UNSIGNED_INT
		};

		enum class channel_name
		{
			red = GL_TEXTURE_RED_TYPE,
			green = GL_TEXTURE_GREEN_TYPE,
			blue = GL_TEXTURE_BLUE_TYPE,
			alpha = GL_TEXTURE_ALPHA_TYPE,
			depth = GL_TEXTURE_DEPTH_TYPE
		};

		class save_binding_state
		{
			GLint m_last_binding;
			GLenum m_target;

		public:
			save_binding_state(const texture& new_binding)
			{
				GLenum pname;
				switch (new_binding.target())
				{
				case texture_target::texture1D: pname = GL_TEXTURE_1D_BINDING_EXT; break;
				case texture_target::texture2D: pname = GL_TEXTURE_2D_BINDING_EXT; break;
				case texture_target::texture3D: pname = GL_TEXTURE_3D_BINDING_EXT; break;
				}

				glGetIntegerv(pname, &m_last_binding);
				
				checkForGlError("glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &m_last_binding);");
				new_binding.bind();
				m_target = (GLenum)new_binding.target();
			}

			~save_binding_state()
			{
				glBindTexture(m_target, m_last_binding);
				checkForGlError(fmt::format("glBindTexture(GL_TEXTURE_2D, m_last_binding=0x%x)", m_last_binding));
			}
		};

		class settings
		{
			texture *m_parent;

			texture::channel m_swizzle_r = texture::channel::r;
			texture::channel m_swizzle_g = texture::channel::g;
			texture::channel m_swizzle_b = texture::channel::b;
			texture::channel m_swizzle_a = texture::channel::a;

			texture::format m_format = texture::format::rgba;
			texture::format m_internal_format = texture::format::rgba;
			texture::type m_type = texture::type::ubyte;

			min_filter m_min_filter = min_filter::nearest;
			filter m_mag_filter = filter::nearest;

			uint m_width = 0;
			uint m_height = 0;
			int m_level = 0;

			int m_compressed_image_size = 0;

			const void* m_pixels = nullptr;
			float m_aniso = 1.f;
			texture::compare_mode m_compare_mode = texture::compare_mode::none;
			texture::compare_func m_compare_func = texture::compare_func::greater;

			texture::wrap m_wrap_s = texture::wrap::repeat;
			texture::wrap m_wrap_t = texture::wrap::repeat;
			texture::wrap m_wrap_r = texture::wrap::repeat;

			float m_max_lod = 1000.f;
			float m_min_lod = -1000.f;
			float m_lod = 0.f;
			int m_max_level = 1000;
			bool m_generate_mipmap = false;

			color m_border_color;

		public:
			settings(texture *parent = nullptr) : m_parent(parent)
			{
			}

			~settings()
			{
				apply();
			}

			void apply(const texture &texture) const
			{
				save_binding_state save(texture);

				texture.pixel_settings().apply();

				if (compressed_format(m_internal_format))
				{
					int compressed_image_size = m_compressed_image_size;
					if (!compressed_image_size)
					{
						switch (m_internal_format)
						{
						case format::compressed_rgb_s3tc_dxt1:
							compressed_image_size = ((m_width + 2) / 3) * ((m_height + 2) / 3) * 6;
							break;

						case format::compressed_rgba_s3tc_dxt1:
							compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 8;
							break;

						case format::compressed_rgba_s3tc_dxt3:
						case format::compressed_rgba_s3tc_dxt5:
							compressed_image_size = ((m_width + 3) / 4) * ((m_height + 3) / 4) * 16;
							break;
						}
					}

					glCompressedTexImage2D((GLenum)m_parent->target(), m_level, (GLint)m_internal_format, m_width, m_height, 0, compressed_image_size, m_pixels);
					checkForGlError("glCompressedTexImage2D");
				}
				else
				{
					glTexImage2D((GLenum)m_parent->target(), m_level, (GLint)m_internal_format, m_width, m_height, 0, (GLint)m_format, (GLint)m_type, m_pixels);
					checkForGlError("glTexImage2D");
				}

				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_MAX_LEVEL, m_max_level);
				glTexParameteri((GLenum)m_parent->target(), GL_GENERATE_MIPMAP, m_generate_mipmap ? GL_TRUE : GL_FALSE);

				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_WRAP_S, (GLint)m_wrap_s);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_WRAP_T, (GLint)m_wrap_t);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_WRAP_R, (GLint)m_wrap_r);

				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_COMPARE_MODE, (GLint)m_compare_mode);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_COMPARE_FUNC, (GLint)m_compare_func);

				glTexParameterf((GLenum)m_parent->target(), GL_TEXTURE_MIN_LOD, m_max_lod);
				glTexParameterf((GLenum)m_parent->target(), GL_TEXTURE_MAX_LOD, m_min_lod);
				glTexParameterf((GLenum)m_parent->target(), GL_TEXTURE_LOD_BIAS, m_lod);

				glTexParameterfv((GLenum)m_parent->target(), GL_TEXTURE_BORDER_COLOR, m_border_color.rgba);

				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_MIN_FILTER, (GLint)m_min_filter);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_MAG_FILTER, (GLint)m_mag_filter);

				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_SWIZZLE_R, (GLint)m_swizzle_r);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_SWIZZLE_G, (GLint)m_swizzle_g);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_SWIZZLE_B, (GLint)m_swizzle_b);
				glTexParameteri((GLenum)m_parent->target(), GL_TEXTURE_SWIZZLE_A, (GLint)m_swizzle_a);

				glTexParameterf((GLenum)m_parent->target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, m_aniso);
			}

			void apply()
			{
				if (m_parent)
				{
					apply(*m_parent);
					m_parent = nullptr;
				}
			}

			settings& swizzle(
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

			settings& format(texture::format format)
			{
				m_format = format;
				return *this;
			}

			settings& type(texture::type type)
			{
				m_type = type;
				return *this;
			}

			settings& internal_format(texture::format format)
			{
				m_internal_format = format;
				return *this;
			}

			settings& filter(min_filter min_filter, filter mag_filter)
			{
				m_min_filter = min_filter;
				m_mag_filter = mag_filter;
			}

			settings& width(uint width)
			{
				m_width = width;
				return *this;
			}

			settings& height(uint height)
			{
				m_height = height;
				return *this;
			}

			settings& size(uint width_, uint height_)
			{
				return width(width_).height(height_);
			}

			settings& level(int value)
			{
				m_level = value;
				return *this;
			}

			settings& compressed_image_size(int size)
			{
				m_compressed_image_size = size;
				return *this;
			}

			settings& pixels(const void* pixels)
			{
				m_pixels = pixels;
				return *this;
			}

			settings& aniso(float value)
			{
				m_aniso = value;
				return *this;
			}

			settings& compare_mode(texture::compare_mode value)
			{
				m_compare_mode = value;
				return *this;
			}
			settings& compare_func(texture::compare_func value)
			{
				m_compare_func = value;
				return *this;
			}
			settings& compare(texture::compare_func func, texture::compare_mode mode)
			{
				return compare_func(func).compare_mode(mode);
			}

			settings& wrap_s(texture::wrap value)
			{
				m_wrap_s = value;
				return *this;
			}
			settings& wrap_t(texture::wrap value)
			{
				m_wrap_t = value;
				return *this;
			}
			settings& wrap_r(texture::wrap value)
			{
				m_wrap_r = value;
				return *this;
			}
			settings& wrap(texture::wrap s, texture::wrap t, texture::wrap r)
			{
				return wrap_s(s).wrap_t(t).wrap_r(r);
			}

			settings& max_lod(float value)
			{
				m_max_lod = value;
				return *this;
			}
			settings& min_lod(float value)
			{
				m_min_lod = value;
				return *this;
			}
			settings& lod(float value)
			{
				m_lod = value;
				return *this;
			}
			settings& max_level(int value)
			{
				m_max_level = value;
				return *this;
			}
			settings& generate_mipmap(bool value)
			{
				m_generate_mipmap = value;
				return *this;
			}
			settings& mipmap(int level, int max_level, float lod, float min_lod, float max_lod, bool generate)
			{
				return this->level(level).max_level(max_level).lod(lod).min_lod(min_lod).max_lod(max_lod).generate_mipmap(generate);
			}

			settings& border_color(color value)
			{
				m_border_color = value;
				return *this;
			}
		};

	private:
		texture_target m_target = texture_target::texture2D;

	public:
		texture_target target() const
		{
			return m_target;
		}

		void set_target(texture_target target)
		{
			m_target = target;
		}

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
				remove();

			glGenTextures(1, &m_id);
		}

		void create(texture_target target_)
		{
			set_target(target_);
			create();
		}

		bool created() const
		{
			return m_id != 0;
		}

		void remove()
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
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

		void bind() const
		{
			glBindTexture((GLenum)target(), id());
		}

		settings config()
		{
			return{ this };
		}

		void config(const settings& settings_)
		{
			settings_.apply(*this);
		}

		gl::pixel_settings& pixel_settings()
		{
			return m_pixel_settings;
		}

		const gl::pixel_settings& pixel_settings() const
		{
			return m_pixel_settings;
		}

		int width() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_WIDTH, &result);
			return (int)result;
		}

		int height() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_HEIGHT, &result);
			return (int)result;
		}

		int depth() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_DEPTH, &result);
			return (int)result;
		}

		sizei size() const
		{
			return{ width(), height() };
		}

		size3i size3d() const
		{
			return{ width(), height(), depth() };
		}

		texture::format internal_format() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_INTERNAL_FORMAT, &result);
			return (texture::format)result;
		}

		texture::channel_type get_channel_type(texture::channel_name channel) const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), (GLenum)channel, &result);
			return (texture::channel_type)result;
		}

		int get_channel_count() const
		{
			int result = 0;

			if (get_channel_type(channel_name::red) != channel_type::none)
				result++;
			if (get_channel_type(channel_name::green) != channel_type::none)
				result++;
			if (get_channel_type(channel_name::blue) != channel_type::none)
				result++;
			if (get_channel_type(channel_name::alpha) != channel_type::none)
				result++;
			if (get_channel_type(channel_name::depth) != channel_type::none)
				result++;

			return result;
		}

		bool compressed() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_COMPRESSED, &result);
			return (bool)result;
		}

		int compressed_size() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)target(), level(), GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &result);
			return (int)result;
		}

		texture() = default;

		texture(texture_target target, GLuint id = 0)
		{
			m_target = target;
			set_id(id);
		}

		void copy_from(const void* src, texture::format format, texture::type type, gl::pixel_settings pixel_settings)
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			glTexSubImage2D((GLenum)target(), level(), 0, 0, width(), height(), (GLenum)format, (GLenum)type, src);
		}

		void copy_from(const pbo& pbo_, texture::format format, texture::type type, gl::pixel_settings pixel_settings)
		{
			pbo::save_binding_state save_pbo(pbo::target::unpack, pbo_);
			copy_from(nullptr, format, type, pixel_settings);
		}

		void copy_from(void* dst, texture::format format, texture::type type)
		{
			copy_from(dst, format, type, pixel_settings());
		}

		void copy_from(const pbo& pbo_, texture::format format, texture::type type)
		{
			copy_from(pbo_, format, type, pixel_settings());
		}

		void copy_to(void* dst, texture::format format, texture::type type, gl::pixel_settings pixel_settings)
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			glGetTexImage((GLenum)target(), level(), (GLenum)format, (GLenum)type, dst);
		}

		void copy_to(const pbo& pbo_, texture::format format, texture::type type, gl::pixel_settings pixel_settings)
		{
			pbo::save_binding_state save_pbo(pbo::target::pack, pbo_);
			copy_to(nullptr, format, type, pixel_settings);
		}

		void copy_to(void* dst, texture::format format, texture::type type)
		{
			copy_to(dst, format, type, pixel_settings());
		}

		void copy_to(const pbo& pbo_, texture::format format, texture::type type)
		{
			copy_to(pbo_, format, type, pixel_settings());
		}
	};

	class rbo
	{
		GLuint m_id = 0;

	public:
		rbo() = default;

		rbo(GLuint id)
		{
			set_id(id);
		}

		~rbo()
		{
			remove();
		}

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
				remove();
			}

			glGenRenderbuffers(1, &m_id);
		}

		void create(texture::format format, u32 width, u32 height)
		{
			create();
			storage(format, width, height);
		}

		void bind() const
		{
			glBindRenderbuffer(GL_RENDERBUFFER, m_id);
		}

		void storage(texture::format format, u32 width, u32 height)
		{
			save_binding_state save(*this);
			glRenderbufferStorage(GL_RENDERBUFFER, (GLenum)format, width, height);
		}

		void remove()
		{
			glDeleteRenderbuffers(1, &m_id);
			m_id = 0;
		}

		uint id() const
		{
			return m_id;
		}

		void set_id(uint id)
		{
			m_id = id;
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

	class fbo
	{
		GLuint m_id = GL_NONE;

	public:
		fbo() = default;

		fbo(GLuint id)
		{
			set_id(id);
		}

		~fbo()
		{
			remove();
		}

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

			void operator = (const texture& rhs)
			{
				save_binding_state save(m_parent);

				switch (rhs.target())
				{
				case texture::texture_target::texture1D: glFramebufferTexture1D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_1D, rhs.id(), rhs.level()); break;
				case texture::texture_target::texture2D: glFramebufferTexture2D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_2D, rhs.id(), rhs.level()); break;
				case texture::texture_target::texture3D: glFramebufferTexture3D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_3D, rhs.id(), rhs.level(), 0); break;
				}
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

			std::vector<attachment> range(int from, int count) const
			{
				std::vector<attachment> result;

				for (int i = from; i < from + count; ++i)
					result.push_back((*this)[i]);

				return result;
			}

			using attachment::operator = ;
		};

		indexed_attachment color{ *this, attachment::type::color };
		attachment depth{ *this, attachment::type::depth };
		attachment stencil{ *this, attachment::type::stencil };
		attachment depth_stencil{ *this, attachment::type::depth_stencil };

		enum class target
		{
			read_frame_buffer = GL_READ_FRAMEBUFFER,
			draw_frame_buffer = GL_DRAW_FRAMEBUFFER
		};

		void create();
		void bind() const;
		void blit(const fbo& dst, area src_area, area dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void remove();
		bool created() const;

		void draw(const attachment& buffer)
		{
			save_binding_state save(*this);
			GLenum buf = buffer.id();
			glDrawBuffers(1, &buf);
		}

		void draw(const std::vector<attachment>& indexes)
		{
			save_binding_state save(*this);
			std::vector<GLenum> ids;

			for (auto &index : indexes)
				ids.push_back(index.id());

			glDrawBuffers(ids.size(), ids.data());
		}

		void clear(buffers buffers_) const
		{
			save_binding_state save(*this);
			glClear((GLbitfield)buffers_);
		}

		void draw_pixels(const void* pixels, sizei size, gl::texture::format format_, gl::texture::type type_, gl::pixel_settings pixel_settings = gl::pixel_settings()) const
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			glDrawPixels(size.width, size.height, (GLenum)format_, (GLenum)type_, pixels);
		}

		void draw_pixels(const pbo& pbo_, sizei size, gl::texture::format format_, gl::texture::type type_, gl::pixel_settings pixel_settings = gl::pixel_settings()) const
		{
			save_binding_state save(*this);
			pbo::save_binding_state save_pbo(pbo::target::unpack, pbo_);
			pixel_settings.apply();
			glDrawPixels(size.width, size.height, (GLenum)format_, (GLenum)type_, nullptr);
		}

		void read_pixels(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, gl::pixel_settings pixel_settings = gl::pixel_settings()) const
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			glReadPixels(coord.x, coord.y, coord.width, coord.height, (GLenum)format_, (GLenum)type_, pixels);
		}

		void read_pixels(const pbo& pbo_, coordi coord, gl::texture::format format_, gl::texture::type type_, gl::pixel_settings pixel_settings = gl::pixel_settings()) const
		{
			save_binding_state save(*this);
			pbo::save_binding_state save_pbo(pbo::target::pack, pbo_);
			pixel_settings.apply();
			glReadPixels(coord.x, coord.y, coord.width, coord.height, (GLenum)format_, (GLenum)type_, nullptr);
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

	static const fbo screen{};

	class vertex_attrib_array
	{
	public:
		class entry
		{
			bool m_normalize = false;
			int m_stride = 0;
			std::vector<u8> m_data;
			int m_element_size = 0;

		public:
			void set_data(const void *data, int element_size, int size, bool normalize = false, int stride = 0)
			{
				m_data.resize(size);
				memcpy(m_data.data(), data, size);

				m_normalize = normalize;
				m_stride = stride;
				m_element_size = element_size;
			}

			bool normalize() const
			{
				return m_normalize;
			}

			void set_normalize(bool value)
			{
				m_normalize = value;
			}

			int element_size() const
			{
				return m_element_size;
			}

			size_t size() const
			{
				return m_data.size();
			}

			int stride() const
			{
				return m_stride;
			}

			void set_stride(int value)
			{
				m_stride = value;
			}

			const void* data() const
			{
				return m_data.data();
			}

			template<typename T>
			void operator += (const T& value)
			{
				if (m_element_size && m_element_size != sizeof(T))
					throw std::runtime_error("redefinition vertex_attrib_array element size");

				m_element_size = sizeof(T);
				size_t position = m_data.size();
				m_data.resize(position + sizeof(T));
				(T&)m_data[position] = value;
			}

			void clear()
			{
				m_data.clear();
			}
		};

	private:
		mutable std::unordered_map<int, entry> m_entries;

	public:
		entry operator[](int index) const
		{
			return m_entries[index];
		}

		std::unordered_map<int, entry>& entries() const
		{
			return m_entries;
		}

		void clear()
		{
			m_entries.clear();
		}
	};

	//TODO
	class vbo
	{
	public:
		enum class usage
		{
			static_draw = GL_STATIC_DRAW,
			dynamic_draw = GL_DYNAMIC_DRAW
		};

		enum class target
		{
			array_buffer = GL_ARRAY_BUFFER,
			element_array_buffer = GL_ELEMENT_ARRAY_BUFFER
		};

		class save_binding_state
		{
			GLint m_last_binding;
			target m_target;

		public:
			save_binding_state(const vbo& new_binding) : save_binding_state(new_binding.get_target())
			{
				new_binding.bind();
			}

			save_binding_state(target target_)
			{
				GLenum pname;
				switch (m_target = target_)
				{
				case target::array_buffer: pname = GL_ARRAY_BUFFER_BINDING; break;
				case target::element_array_buffer: pname = GL_ELEMENT_ARRAY_BUFFER_BINDING; break;
				}

				glGetIntegerv(pname, &m_last_binding);
			}

			~save_binding_state()
			{
				glBindBuffer((GLenum)m_target, m_last_binding);
			}
		};

	private:
		GLuint m_id = GL_NONE;
		target m_target = target::array_buffer;

	public:
		vbo() = default;
		vbo(GLint id)
		{
			set_id(id);
		}

		vbo(const vertex_attrib_array& array)
		{
			std::unordered_map<int, int> index_offset;
			std::vector<u8> data;

			for (auto &entry : array.entries())
			{
				int index = entry.first;
				size_t offset = data.size();
				index_offset[index] = offset;
				size_t size = entry.second.size();
				data.resize(offset + size);

				if (entry.second.stride() == 0)
				{
					memcpy(&data[offset], entry.second.data(), size);
				}
				else
				{
					for (size_t i = 0, loffset = 0, end = data.size(), step = entry.second.stride() ? entry.second.stride() : entry.second.element_size();
						i < end;
						i += step, loffset += entry.second.element_size())
					{
						memcpy(&data[offset + loffset], (u8*)entry.second.data() + i, entry.second.element_size());
					}
				}
			}

			create(target::array_buffer, data.data(), data.size());
		}

		~vbo()
		{
			remove();
		}

		target get_target() const
		{
			return m_target;
		}

		void set_data(const void* data, size_t size, usage usage_ = usage::dynamic_draw)
		{
			glBufferData((GLenum)m_target, size, data, (GLenum)usage_);
		}

		void bind() const
		{
			glBindBuffer((GLenum)m_target, id());
		}

		void create(target target_)
		{
			if (created())
			{
				remove();
			}

			m_target = target_;
			glGenBuffers(1, &m_id);
		}

		void create(target target_, const void* data, size_t size, usage usage_ = usage::dynamic_draw)
		{
			create(target_);
			set_data(data, size, usage_);
		}

		void remove()
		{
			glDeleteBuffers(1, &m_id);
			m_id = GL_NONE;
		}

		GLuint id() const
		{
			return m_id;
		}

		void set_id(GLuint id)
		{
			m_id = id;
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

	namespace glsl
	{
		class link_exception : public exception
		{
		public:
			explicit link_exception(const std::string& what_arg)
			{
				m_what = "linkage failed: '" + what_arg + "'";
			}
		};

		class compile_exception : public exception
		{
		public:
			explicit compile_exception(const std::string& what_arg)
			{
				m_what = "compilation failed: '" + what_arg + "'";
			}
		};

		class shader
		{
			GLuint m_id = 0;

		public:
			enum class type
			{
				fragment = GL_FRAGMENT_SHADER,
				vertext = GL_VERTEX_SHADER,
				geometry = GL_GEOMETRY_SHADER
			};

			shader() = default;

			shader(type type_)
			{
				create(type_);
			}

			shader(type type_, const std::string& src)
			{
				create(type_);
				source(src);
			}

			void create(type type_)
			{
				m_id = glCreateShader((GLenum)type_);
			}

			void source(const std::string& src) const
			{
				const char* str = src.c_str();
				const GLint length = src.length();

				glShaderSource(m_id, 1, &str, &length);
			}

			void clear()
			{
				glDeleteShader(m_id);
				m_id = 0;
			}

			uint id() const
			{
				return m_id;
			}

			void set_id(uint id)
			{
				m_id = id;
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

		class program
		{
			GLuint m_id = 0;

		public:
			class uniform_t
			{
				GLuint program;
				GLint location;

			public:
				uniform_t(GLuint program, GLint location)
					: program(program)
					, location(location)
				{
				}

				void operator = (int rhs) const { glProgramUniform1i(program, location, rhs); }
				void operator = (float rhs) const { glProgramUniform1f(program, location, rhs); }
				void operator = (double rhs) const { glProgramUniform1d(program, location, rhs); }

				void operator = (const color1i& rhs) const { glProgramUniform1i(program, location, rhs.r); }
				void operator = (const color1f& rhs) const { glProgramUniform1f(program, location, rhs.r); }
				void operator = (const color1d& rhs) const { glProgramUniform1d(program, location, rhs.r); }
				void operator = (const color2i& rhs) const { glProgramUniform2i(program, location, rhs.r, rhs.g); }
				void operator = (const color2f& rhs) const { glProgramUniform2f(program, location, rhs.r, rhs.g); }
				void operator = (const color2d& rhs) const { glProgramUniform2d(program, location, rhs.r, rhs.g); }
				void operator = (const color3i& rhs) const { glProgramUniform3i(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const color3f& rhs) const { glProgramUniform3f(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const color3d& rhs) const { glProgramUniform3d(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const color4i& rhs) const { glProgramUniform4i(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4f& rhs) const { glProgramUniform4f(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4d& rhs) const { glProgramUniform4d(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }

				void operator = (const glm::ivec2& rhs) const { glProgramUniform2f(program, location, rhs.r, rhs.g); }
				void operator = (const glm::vec2& rhs) const { glProgramUniform2f(program, location, rhs.r, rhs.g); }
				void operator = (const glm::dvec2& rhs) const { glProgramUniform2d(program, location, rhs.r, rhs.g); }
				void operator = (const glm::ivec3& rhs) const { glProgramUniform3i(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const glm::vec3& rhs) const { glProgramUniform3f(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const glm::dvec3& rhs) const { glProgramUniform3d(program, location, rhs.r, rhs.g, rhs.b); }
				void operator = (const glm::ivec4& rhs) const { glProgramUniform4i(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const glm::vec4& rhs) const { glProgramUniform4f(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const glm::dvec4& rhs) const { glProgramUniform4d(program, location, rhs.r, rhs.g, rhs.b, rhs.a); }

				void operator = (const glm::mat2& rhs) const { glProgramUniformMatrix2fv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
				void operator = (const glm::dmat2& rhs) const { glProgramUniformMatrix2dv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
				void operator = (const glm::mat3& rhs) const { glProgramUniformMatrix3fv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
				void operator = (const glm::dmat3& rhs) const { glProgramUniformMatrix3dv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
				void operator = (const glm::mat4& rhs) const { glProgramUniformMatrix4fv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
				void operator = (const glm::dmat4& rhs) const { glProgramUniformMatrix4dv(program, location, 1, GL_FALSE, glm::value_ptr(rhs)); }
			};

			class uniforms_t
			{
				GLuint program;
				std::unordered_map<std::string, GLint> locations;
				int active_texture = 0;

			public:
				uniforms_t(GLuint program) : program(program)
				{
				}

				GLint location(const std::string &name)
				{
					auto finded = locations.find(name);

					if (finded != locations.end())
					{
						return finded->second;
					}

					int result = glGetUniformLocation(program, name.c_str());
					locations[name] = result;

					return result;
				}

				int texture(const std::string &name, int active_texture, const ::gl::texture& texture)
				{
					glActiveTexture(GL_TEXTURE0 + active_texture);
					texture.bind();
					(*this)[name] = active_texture;

					return active_texture;
				}

				int texture(const std::string &name, const ::gl::texture& tex)
				{
					int atex;
					auto finded = locations.find(name);

					if (finded != locations.end())
					{
						atex = finded->second;
					}
					else
					{
						atex = active_texture++;
					}

					return texture(name, atex, tex);
				}

				uniform_t operator[](int location)
				{
					return{ program, location };
				}

				uniform_t operator[](const std::string &name)
				{
					return{ program, location(name) };
				}
			};

			class attribs_t
			{
				GLuint program;

			public:
				void operator = (const vertex_attrib_array& array) const
				{

				}
			};

			program() = default;

			program(GLuint id)
			{
				set_id(id);
			}

			~program()
			{
				remove();
			}

			void create()
			{
				m_id = glCreateProgram();
			}

			void remove()
			{
				glDeleteProgram(m_id);
				m_id = 0;
			}

			static program get_current_program()
			{
				GLint id;
				glGetIntegerv(GL_CURRENT_PROGRAM, &id);
				return{ (GLuint)id };
			}

			void use()
			{
				glUseProgram(m_id);
			}

			void compile()
			{
				glCompileShader(m_id);
			}

			void link()
			{
				glLinkProgram(m_id);
			}

			void make()
			{
				compile();
				link();
			}

			uint id() const
			{
				return m_id;
			}

			void set_id(uint id)
			{
				m_id = id;
			}

			bool created() const
			{
				return m_id != 0;
			}

			explicit operator bool() const
			{
				return created();
			}

			void operator += (const shader& rhs)
			{
				glAttachShader(m_id, rhs.id());
			}
		};
	};
}
