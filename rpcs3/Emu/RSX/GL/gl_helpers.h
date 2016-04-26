#pragma once

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#include "OpenGL.h"
#include "../GCM.h"

#include "Utilities/geometry.h"

namespace gl
{
#ifdef _DEBUG
	struct __glcheck_impl_t
	{
		const char* file;
		const char* function;
		long line;

		constexpr __glcheck_impl_t(const char* file, const char* function, long line)
			: file(file)
			, function(function)
			, line(line)
		{
		}

		~__glcheck_impl_t() noexcept(false)
		{
			if (GLenum err = glGetError())
			{
				std::string error;
				switch (err)
				{
				case GL_INVALID_OPERATION:      error = "invalid operation";      break;
				case GL_INVALID_ENUM:           error = "invalid enum";           break;
				case GL_INVALID_VALUE:          error = "invalid value";          break;
				case GL_OUT_OF_MEMORY:          error = "out of memory";          break;
				case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "invalid framebuffer operation";  break;
				default: error = "unknown error"; break;
				}

				throw std::runtime_error(fmt::format("OpenGL error: %s. file '%s' function '%s' line %ld", error.c_str(), file, function, line));
			}
		}
	};
#define __glcheck gl::__glcheck_impl_t{ __FILE__, __FUNCTION__, __LINE__ },
#else
#define __glcheck
#endif

	void enable_debugging();

	class exception : public std::exception
	{
	protected:
		std::string m_what;

	public:
		const char* what() const noexcept override
		{
			return m_what.c_str();
		}
	};

	template<typename Type, uint BindId, uint GetStateId>
	class save_binding_state_base
	{
		GLint m_last_binding;

	public:
		save_binding_state_base(const Type& new_state) : save_binding_state_base()
		{
			new_state.bind();
		}

		save_binding_state_base()
		{
			glGetIntegerv(GetStateId, &m_last_binding);
		}

		~save_binding_state_base()
		{
			glBindBuffer(BindId, m_last_binding);
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
		none = 0,
		color = GL_COLOR_BUFFER_BIT,
		depth = GL_DEPTH_BUFFER_BIT,
		stencil = GL_STENCIL_BUFFER_BIT,

		color_depth = color | depth,
		color_depth_stencil = color | depth | stencil,
		color_stencil = color | stencil,

		depth_stencil = depth | stencil
	};
	
	class pixel_pack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_aligment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_PACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_PACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_PACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_PACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_PACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_PACK_ALIGNMENT, m_aligment);
		}

		pixel_pack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_pack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_pack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_pack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_pack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_pack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_pack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_pack_settings& aligment(int value)
		{
			m_aligment = value;
			return *this;
		}
	};

	class pixel_unpack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_aligment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_UNPACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_aligment);
		}

		pixel_unpack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_unpack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_unpack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_unpack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_unpack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_unpack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_unpack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_unpack_settings& aligment(int value)
		{
			m_aligment = value;
			return *this;
		}
	};

	class vao;

	class buffer_pointer
	{
	public:
		enum class type
		{
			s8 = GL_BYTE,
			u8 = GL_UNSIGNED_BYTE,
			s16 = GL_SHORT,
			u16 = GL_UNSIGNED_SHORT,
			s32 = GL_INT,
			u32 = GL_UNSIGNED_INT,
			f16 = GL_HALF_FLOAT,
			f32 = GL_FLOAT,
			f64 = GL_DOUBLE,
			fixed = GL_FIXED,
			s32_2_10_10_10_rev = GL_INT_2_10_10_10_REV,
			u32_2_10_10_10_rev = GL_UNSIGNED_INT_2_10_10_10_REV,
			u32_10f_11f_11f_rev = GL_UNSIGNED_INT_10F_11F_11F_REV
		};

	private:
		vao* m_vao;
		u32 m_offset;
		u32 m_stride;
		u32 m_size = 4;
		type m_type = type::f32;
		bool m_normalize = false;

	public:
		buffer_pointer(vao* vao, u32 offset = 0, u32 stride = 0)
			: m_vao(vao)
			, m_offset(offset)
			, m_stride(stride)
		{
		}

		const class ::gl::vao& get_vao() const
		{
			return *m_vao;
		}

		class ::gl::vao& get_vao()
		{
			return *m_vao;
		}

		buffer_pointer& offset(u32 value)
		{
			m_offset = value;
			return *this;
		}

		u32 offset() const
		{
			return m_offset;
		}

		buffer_pointer& stride(u32 value)
		{
			m_stride = value;
			return *this;
		}

		u32 stride() const
		{
			return m_stride;
		}

		buffer_pointer& size(u32 value)
		{
			m_size = value;
			return *this;
		}

		u32 size() const
		{
			return m_size;
		}

		buffer_pointer& set_type(type value)
		{
			m_type = value;
			return *this;
		}

		type get_type() const
		{
			return m_type;
		}

		buffer_pointer& normalize(bool value)
		{
			m_normalize = value;
			return *this;
		}

		bool normalize() const
		{
			return m_normalize;
		}

		buffer_pointer& operator >> (u32 value)
		{
			return stride(value);
		}

		buffer_pointer& config(type type_ = type::f32, u32 size_ = 4, bool normalize_ = false)
		{
			return set_type(type_).size(size_).normalize(normalize_);
		}
	};

	class buffer
	{
	public:
		enum class target
		{
			pixel_pack = GL_PIXEL_PACK_BUFFER,
			pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
			array = GL_ARRAY_BUFFER,
			element_array = GL_ELEMENT_ARRAY_BUFFER
		};
		enum class access
		{
			read = GL_READ_ONLY,
			write = GL_WRITE_ONLY,
			read_write = GL_READ_WRITE
		};

	private:
		GLuint m_id = GL_NONE;
		GLsizeiptr m_size = 0;
		target m_target = target::array;

	public:
		buffer() = default;
		buffer(const buffer&) = delete;

		buffer(GLuint id)
		{
			set_id(id);
		}

		~buffer()
		{
			if (created())
				remove();
		}

		class save_binding_state
		{
			GLint m_last_binding;
			GLenum m_target;

		public:
			save_binding_state(target target_, const buffer& new_state) : save_binding_state(target_)
			{
				new_state.bind(target_);
			}

			save_binding_state(target target_)
			{
				GLenum pname;
				switch (target_)
				{
				case target::pixel_pack: pname = GL_PIXEL_PACK_BUFFER_BINDING; break;
				case target::pixel_unpack: pname = GL_PIXEL_UNPACK_BUFFER_BINDING; break;
				case target::array: pname = GL_ARRAY_BUFFER_BINDING; break;
				case target::element_array: pname = GL_ELEMENT_ARRAY_BUFFER_BINDING; break;
				}

				glGetIntegerv(pname, &m_last_binding);
				m_target = (GLenum)target_;
			}


			~save_binding_state()
			{
				glBindBuffer(m_target, m_last_binding);
			}
		};

		void recreate()
		{
			if (created())
			{
				remove();
			}

			create();
		}

		void recreate(GLsizeiptr size, const void* data = nullptr)
		{
			if (created())
			{
				remove();
			}

			create(size, data);
		}

		void create()
		{
			glGenBuffers(1, &m_id);
		}

		void create(GLsizeiptr size, const void* data_ = nullptr)
		{
			create();
			data(size, data_);
		}

		void data(GLsizeiptr size, const void* data_ = nullptr)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);
			glBufferData((GLenum)target_, size, data_, GL_STREAM_COPY);
			m_size = size;
		}

		void sub_data(GLintptr offset, GLsizeiptr size, const void* data_ = nullptr)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);
			glBufferSubData((GLenum)target_, offset, size, data_);
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

		GLsizeiptr size() const
		{
			return m_size;
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

		void map(std::function<void(GLubyte*)> impl, access access_)
		{
			target target_ = current_target();
			save_binding_state save(target_, *this);

			if (GLubyte* ptr = (GLubyte*)glMapBuffer((GLenum)target_, (GLenum)access_))
			{
				impl(ptr);
				glUnmapBuffer((GLenum)target_);
			}
		}

		class mapper
		{
			buffer *m_parent;
			GLubyte *m_data;

		public:
			mapper(buffer& parent, access access_)
			{
				m_parent = &parent;
				m_data = parent.map(access_);
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

		GLubyte* map(access access_)
		{
			bind(current_target());

			return (GLubyte*)glMapBuffer((GLenum)current_target(), (GLenum)access_);
		}

		void unmap()
		{
			glUnmapBuffer((GLenum)current_target());
		}
	};

	class vao
	{
		template<buffer::target BindId, uint GetStateId>
		class entry
		{
			vao& m_parent;

		public:
			using save_binding_state = save_binding_state_base<entry, (GLuint)BindId, GetStateId>;

			entry(vao* parent) noexcept : m_parent(*parent)
			{
			}

			entry& operator = (const buffer& buf) noexcept
			{
				m_parent.bind();
				buf.bind(BindId);

				return *this;
			}
		};

		GLuint m_id = GL_NONE;

	public:
		entry<buffer::target::pixel_pack, GL_PIXEL_PACK_BUFFER_BINDING> pixel_pack_buffer{ this };
		entry<buffer::target::pixel_unpack, GL_PIXEL_UNPACK_BUFFER_BINDING> pixel_unpack_buffer{ this };
		entry<buffer::target::array, GL_ARRAY_BUFFER_BINDING> array_buffer{ this };
		entry<buffer::target::element_array, GL_ELEMENT_ARRAY_BUFFER_BINDING> element_array_buffer{ this };

		vao() = default;
		vao(vao&) = delete;

		vao(vao&& vao_) noexcept
		{
			swap(vao_);
		}
		vao(GLuint id) noexcept
		{
			set_id(id);
		}

		~vao() noexcept
		{
			if (created())
				remove();
		}

		void swap(vao& vao_) noexcept
		{
			auto my_old_id = id();
			set_id(vao_.id());
			vao_.set_id(my_old_id);
		}

		vao& operator = (const vao& rhs) = delete;
		vao& operator = (vao&& rhs) noexcept
		{
			swap(rhs);
			return *this;
		}

		void bind() const noexcept
		{
			glBindVertexArray(m_id);
		}

		void create() noexcept
		{
			glGenVertexArrays(1, &m_id);
		}

		void remove() noexcept
		{
			glDeleteVertexArrays(1, &m_id);
			m_id = GL_NONE;
		}

		uint id() const noexcept
		{
			return m_id;
		}

		void set_id(uint id) noexcept
		{
			m_id = id;
		}

		bool created() const noexcept
		{
			return m_id != GL_NONE;
		}

		explicit operator bool() const noexcept
		{
			return created();
		}

		void enable_for_attributes(std::initializer_list<GLuint> indexes) noexcept
		{
			for (auto &index : indexes)
			{
				glEnableVertexAttribArray(index);
			}
		}

		void disable_for_attributes(std::initializer_list<GLuint> indexes) noexcept
		{
			for (auto &index : indexes)
			{
				glDisableVertexAttribArray(index);
			}
		}

		void enable_for_attribute(GLuint index) noexcept
		{
			enable_for_attributes({ index });
		}

		void disable_for_attribute(GLuint index) noexcept
		{
			disable_for_attributes({ index });
		}

		buffer_pointer operator + (u32 offset) const noexcept
		{
			return{ (vao*)this, offset };
		}

		buffer_pointer operator >> (u32 stride) const noexcept
		{
			return{ (vao*)this, {}, stride };
		}

		operator buffer_pointer() const noexcept
		{
			return{ (vao*)this };
		}
	};

	class texture_view;
	class texture
	{
		GLuint m_id = 0;
		GLuint m_level = 0;
		class pixel_pack_settings m_pixel_pack_settings;
		class pixel_unpack_settings m_pixel_unpack_settings;

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
			depth = GL_DEPTH_COMPONENT,
			depth_stencil = GL_DEPTH_STENCIL
		};

		enum class internal_format
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
			//mirror_clamp_to_edge = GL_MIRROR_CLAMP_TO_EDGE,
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

		enum class target
		{
			texture1D = GL_TEXTURE_1D,
			texture2D = GL_TEXTURE_2D,
			texture3D = GL_TEXTURE_3D,
			textureBuffer = GL_TEXTURE_BUFFER
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
			save_binding_state(const texture& new_binding) noexcept
			{
				GLenum pname;
				switch (new_binding.get_target())
				{
				case target::texture1D: pname = GL_TEXTURE_BINDING_1D; break;
				case target::texture2D: pname = GL_TEXTURE_BINDING_2D; break;
				case target::texture3D: pname = GL_TEXTURE_BINDING_3D; break;
				case target::textureBuffer: pname = GL_TEXTURE_BINDING_BUFFER; break;
				}

				glGetIntegerv(pname, &m_last_binding);
				
				new_binding.bind();
				m_target = (GLenum)new_binding.get_target();
			}

			~save_binding_state() noexcept
			{
				glBindTexture(m_target, m_last_binding);
			}
		};

		class settings;

	private:
		target m_target = target::texture2D;

	public:
		target get_target() const noexcept
		{
			return m_target;
		}

		void set_target(target target) noexcept
		{
			m_target = target;
		}

		static bool compressed_format(internal_format format_) noexcept
		{
			switch (format_)
			{
			case internal_format::compressed_rgb_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt1:
			case internal_format::compressed_rgba_s3tc_dxt3:
			case internal_format::compressed_rgba_s3tc_dxt5:
				return true;
			}

			return false;
		}

		uint id() const noexcept
		{
			return m_id;
		}

		uint level() const noexcept
		{
			return m_level;
		}

		void recreate() noexcept
		{
			if (created())
				remove();

			create();
		}

		void recreate(target target_) noexcept
		{
			if (created())
				remove();

			create(target_);
		}

		void create() noexcept
		{
			glGenTextures(1, &m_id);
		}

		void create(target target_) noexcept
		{
			set_target(target_);
			create();
		}

		bool created() const noexcept
		{
			return m_id != 0;
		}

		void remove() noexcept
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
		}

		void set_id(GLuint id) noexcept
		{
			m_id = id;
		}

		void set_level(int level) noexcept
		{
			m_level = level;
		}

		texture_view with_level(int level);

		explicit operator bool() const noexcept
		{
			return created();
		}

		void bind() const noexcept
		{
			glBindTexture((GLenum)get_target(), id());
		}

		settings config();

		void config(const settings& settings_);

		class pixel_pack_settings& pixel_pack_settings()
		{
			return m_pixel_pack_settings;
		}

		const class pixel_pack_settings& pixel_pack_settings() const
		{
			return m_pixel_pack_settings;
		}

		class pixel_unpack_settings& pixel_unpack_settings()
		{
			return m_pixel_unpack_settings;
		}

		const class pixel_unpack_settings& pixel_unpack_settings() const
		{
			return m_pixel_unpack_settings;
		}

		int width() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_WIDTH, &result);
			return (int)result;
		}

		int height() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_HEIGHT, &result);
			return (int)result;
		}

		int depth() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_DEPTH, &result);
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

		texture::format get_internal_format() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_INTERNAL_FORMAT, &result);
			return (texture::format)result;
		}

		texture::channel_type get_channel_type(texture::channel_name channel) const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), (GLenum)channel, &result);
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
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_COMPRESSED, &result);
			return result != 0;
		}

		int compressed_size() const
		{
			save_binding_state save(*this);
			GLint result;
			glGetTexLevelParameteriv((GLenum)get_target(), level(), GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &result);
			return (int)result;
		}

		texture() = default;
		texture(texture&) = delete;

		texture(texture&& texture_)
		{
			swap(texture_);
		}
		texture(target target_, GLuint id = 0)
		{
			m_target = target_;
			set_id(id);
		}

		~texture()
		{
			if (created())
				remove();
		}

		void swap(texture& texture_)
		{
			auto my_old_id = id();
			auto my_old_target = get_target();
			set_id(texture_.id());
			set_target(texture_.get_target());
			texture_.set_id(my_old_id);
			texture_.set_target(my_old_target);
		}

		texture& operator = (const texture& rhs) = delete;
		texture& operator = (texture&& rhs)
		{
			swap(rhs);
			return *this;
		}

		void copy_from(const void* src, texture::format format, texture::type type, class pixel_unpack_settings pixel_settings)
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			__glcheck glTexSubImage2D((GLenum)get_target(), level(), 0, 0, width(), height(), (GLenum)format, (GLenum)type, src);
		}

		void copy_from(buffer &buf, u32 gl_format_type, u32 offset, u32 length)
		{
			if (get_target() != target::textureBuffer)
				throw EXCEPTION("OpenGL error: texture cannot copy from buffer");

			if (!offset)
			{
				copy_from(buf, gl_format_type);
				return;
			}

			if (glTextureBufferRangeEXT == nullptr)
				throw EXCEPTION("OpenGL error: partial buffer access for textures is unsupported on your system");

			__glcheck glTextureBufferRangeEXT(id(), (GLenum)target::textureBuffer, gl_format_type, buf.id(), offset, length);
		}

		void copy_from(buffer &buf, u32 gl_format_type)
		{
			save_binding_state save(*this);
			__glcheck glTexBuffer((GLenum)target::textureBuffer, gl_format_type, buf.id());
		}

		void copy_from(const buffer& buf, texture::format format, texture::type type, class pixel_unpack_settings pixel_settings)
		{
			buffer::save_binding_state save_buffer(buffer::target::pixel_unpack, buf);
			copy_from(nullptr, format, type, pixel_settings);
		}

		void copy_from(void* dst, texture::format format, texture::type type)
		{
			copy_from(dst, format, type, pixel_unpack_settings());
		}

		void copy_from(const buffer& buf, texture::format format, texture::type type)
		{
			copy_from(buf, format, type, pixel_unpack_settings());
		}

		void copy_to(void* dst, texture::format format, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			save_binding_state save(*this);
			pixel_settings.apply();
			__glcheck glGetTexImage((GLenum)get_target(), level(), (GLenum)format, (GLenum)type, dst);
		}

		void copy_to(void* dst, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			copy_to(dst, get_internal_format(), type, pixel_settings);
		}

		void copy_to(const buffer& buf, texture::format format, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			buffer::save_binding_state save_buffer(buffer::target::pixel_pack, buf);
			copy_to(nullptr, format, type, pixel_settings);
		}

		void copy_to(const buffer& buf, texture::type type, class pixel_pack_settings pixel_settings) const
		{
			buffer::save_binding_state save_buffer(buffer::target::pixel_pack, buf);
			copy_to(nullptr, get_internal_format(), type, pixel_settings);
		}

		void copy_to(void* dst, texture::format format, texture::type type) const
		{
			copy_to(dst, format, type, pixel_pack_settings());
		}

		void copy_to(void* dst, texture::type type) const
		{
			copy_to(dst, get_internal_format(), type, pixel_pack_settings());
		}

		void copy_to(const buffer& buf, texture::format format, texture::type type) const
		{
			copy_to(buf, format, type, pixel_pack_settings());
		}

		void copy_to(const buffer& buf, texture::type type) const
		{
			copy_to(buf, get_internal_format(), type, pixel_pack_settings());
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
			if (created())
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

		void recreate()
		{
			if (created())
				remove();

			create();
		}

		void recreate(texture::format format, u32 width, u32 height)
		{
			if (created())
				remove();

			create(format, width, height);
		}

		void create()
		{
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

	class texture::settings
	{
		texture *m_parent;

		texture::channel m_swizzle_r = texture::channel::r;
		texture::channel m_swizzle_g = texture::channel::g;
		texture::channel m_swizzle_b = texture::channel::b;
		texture::channel m_swizzle_a = texture::channel::a;

		texture::format m_format = texture::format::rgba;
		texture::internal_format m_internal_format = texture::internal_format::rgba;
		texture::type m_type = texture::type::ubyte;

		gl::min_filter m_min_filter = gl::min_filter::nearest;
		gl::filter m_mag_filter = gl::filter::nearest;

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

		color4f m_border_color;

	public:
		settings(texture *parent = nullptr) : m_parent(parent)
		{
		}

		~settings()
		{
			apply();
		}

		void apply(const texture &texture) const;
		void apply();

		settings& swizzle(
			texture::channel r = texture::channel::r,
			texture::channel g = texture::channel::g,
			texture::channel b = texture::channel::b,
			texture::channel a = texture::channel::a);

		settings& format(texture::format format);
		settings& type(texture::type type);
		settings& internal_format(texture::internal_format format);
		settings& filter(min_filter min_filter, filter mag_filter);
		settings& width(uint width);
		settings& height(uint height);
		settings& size(sizei size);
		settings& level(int value);
		settings& compressed_image_size(int size);
		settings& pixels(const void* pixels);
		settings& aniso(float value);
		settings& compare_mode(texture::compare_mode value);
		settings& compare_func(texture::compare_func value);
		settings& compare(texture::compare_func func, texture::compare_mode mode);

		settings& wrap_s(texture::wrap value);
		settings& wrap_t(texture::wrap value);
		settings& wrap_r(texture::wrap value);
		settings& wrap(texture::wrap s, texture::wrap t, texture::wrap r);

		settings& max_lod(float value);
		settings& min_lod(float value);
		settings& lod(float value);
		settings& max_level(int value);
		settings& generate_mipmap(bool value);
		settings& mipmap(int level, int max_level, float lod, float min_lod, float max_lod, bool generate);

		settings& border_color(color4f value);
	};

	GLenum draw_mode(rsx::primitive_type in);

	enum class indices_type
	{
		ubyte = GL_UNSIGNED_BYTE,
		ushort = GL_UNSIGNED_SHORT,
		uint = GL_UNSIGNED_INT
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
			if (created())
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

				switch (rhs.get_target())
				{
				case texture::target::texture1D: glFramebufferTexture1D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_1D, rhs.id(), rhs.level()); break;
				case texture::target::texture2D: glFramebufferTexture2D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_2D, rhs.id(), rhs.level()); break;
				case texture::target::texture3D: glFramebufferTexture3D(GL_FRAMEBUFFER, m_id, GL_TEXTURE_3D, rhs.id(), rhs.level(), 0); break;
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

			using attachment::operator =;
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
		void blit(const fbo& dst, areai src_area, areai dst_area, buffers buffers_ = buffers::color, filter filter_ = filter::nearest) const;
		void bind_as(target target_) const;
		void remove();
		bool created() const;
		void check() const;

		void recreate();
		void draw_buffer(const attachment& buffer) const;
		void draw_buffers(const std::initializer_list<attachment>& indexes) const;
		
		void read_buffer(const attachment& buffer) const;

		void draw_arrays(rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const buffer& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const vao& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;

		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset = 0) const;
		void draw_elements(const buffer& buffer_, rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, size_t indices_buffer_offset = 0) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;

		void clear(buffers buffers_) const;
		void clear(buffers buffers_, color4f color_value, double depth_value, u8 stencil_value) const;

		void copy_from(const void* pixels, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;
		void copy_from(const buffer& buf, sizei size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;

		void copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;
		void copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;

		static fbo get_binded_draw_buffer();
		static fbo get_binded_read_buffer();
		static fbo get_binded_buffer();

		GLuint id() const;
		void set_id(GLuint id);

		explicit operator bool() const
		{
			return created();
		}
	};

	extern const fbo screen;

	namespace glsl
	{
		class compilation_exception : public exception
		{
		public:
			explicit compilation_exception(const std::string& what_arg)
			{
				m_what = "compilation failed: '" + what_arg + "'";
			}
		};

		class link_exception : public exception
		{
		public:
			explicit link_exception(const std::string& what_arg)
			{
				m_what = "linkage failed: '" + what_arg + "'";
			}
		};

		class validation_exception : public exception
		{
		public:
			explicit validation_exception(const std::string& what_arg)
			{
				m_what = "compilation failed: '" + what_arg + "'";
			}
		};

		class not_found_exception : public exception
		{
		public:
			explicit not_found_exception(const std::string& what_arg)
			{
				m_what = what_arg + " not found.";
			}
		};

		class shader
		{
			GLuint m_id = GL_NONE;

		public:
			enum class type
			{
				fragment = GL_FRAGMENT_SHADER,
				vertex = GL_VERTEX_SHADER,
				geometry = GL_GEOMETRY_SHADER
			};

			shader() = default;

			shader(GLuint id)
			{
				set_id(id);
			}

			shader(type type_)
			{
				create(type_);
			}

			shader(type type_, const std::string& src)
			{
				create(type_);
				source(src);
			}

			~shader()
			{
				if (created())
					remove();
			}

			void recreate(type type_)
			{
				if (created())
					remove();

				create(type_);
			}

			void create(type type_)
			{
				m_id = glCreateShader((GLenum)type_);
			}

			void source(const std::string& src) const
			{
				const char* str = src.c_str();
				const GLint length = (GLint)src.length();

				glShaderSource(m_id, 1, &str, &length);
			}

			shader& compile()
			{
				glCompileShader(m_id);

				GLint status = GL_FALSE;
				glGetShaderiv(m_id, GL_COMPILE_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetShaderInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					throw compilation_exception(error_msg);
				}

				return *this;
			}

			void remove()
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
				program& m_program;
				GLint m_location;

			public:
				uniform_t(program& program, GLint location)
					: m_program(program)
					, m_location(location)
				{
				}

				GLint location() const
				{
					return m_location;
				}

				void operator = (int rhs) const { m_program.use(); glUniform1i(location(), rhs); }
				void operator = (float rhs) const { m_program.use(); glUniform1f(location(), rhs); }
				//void operator = (double rhs) const { m_program.use(); glUniform1d(location(), rhs); }

				void operator = (const color1i& rhs) const { m_program.use(); glUniform1i(location(), rhs.r); }
				void operator = (const color1f& rhs) const { m_program.use(); glUniform1f(location(), rhs.r); }
				//void operator = (const color1d& rhs) const { m_program.use(); glUniform1d(location(), rhs.r); }
				void operator = (const color2i& rhs) const { m_program.use(); glUniform2i(location(), rhs.r, rhs.g); }
				void operator = (const color2f& rhs) const { m_program.use(); glUniform2f(location(), rhs.r, rhs.g); }
				//void operator = (const color2d& rhs) const { m_program.use(); glUniform2d(location(), rhs.r, rhs.g); }
				void operator = (const color3i& rhs) const { m_program.use(); glUniform3i(location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color3f& rhs) const { m_program.use(); glUniform3f(location(), rhs.r, rhs.g, rhs.b); }
				//void operator = (const color3d& rhs) const { m_program.use(); glUniform3d(location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color4i& rhs) const { m_program.use(); glUniform4i(location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4f& rhs) const { m_program.use(); glUniform4f(location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				//void operator = (const color4d& rhs) const { m_program.use(); glUniform4d(location(), rhs.r, rhs.g, rhs.b, rhs.a); }
			};

			class attrib_t
			{
				GLuint m_program;
				GLint m_location;

			public:
				attrib_t(GLuint program, GLint location)
					: m_program(program)
					, m_location(location)
				{
				}

				GLint location() const
				{
					return m_location;
				}

				void operator = (float rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1f(location(), rhs); }
				void operator = (double rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1d(location(), rhs); }

				void operator = (const color1f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1f(location(), rhs.r); }
				void operator = (const color1d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib1d(location(), rhs.r); }
				void operator = (const color2f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib2f(location(), rhs.r, rhs.g); }
				void operator = (const color2d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib2d(location(), rhs.r, rhs.g); }
				void operator = (const color3f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib3f(location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color3d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib3d(location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color4f& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib4f(location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4d& rhs) const { glDisableVertexAttribArray(location()); glVertexAttrib4d(location(), rhs.r, rhs.g, rhs.b, rhs.a); }

				void operator =(buffer_pointer& pointer) const
				{
					pointer.get_vao().enable_for_attribute(location());
					glVertexAttribPointer(location(), pointer.size(), (GLenum)pointer.get_type(), pointer.normalize(),
						pointer.stride(), (const void*)(size_t)pointer.offset());
				}
			};

			class uniforms_t
			{
				program& m_program;
				std::unordered_map<std::string, GLint> locations;
				int active_texture = 0;

			public:
				uniforms_t(program* program) : m_program(*program)
				{
				}

				void clear()
				{
					locations.clear();
					active_texture = 0;
				}

				bool has_location(const std::string &name, int *location = nullptr)
				{
					int result = glGetUniformLocation(m_program.id(), name.c_str());

					if (result < 0)
						return false;

					locations[name] = result;

					if (location)
						*location = result;

					return true;
				}

				GLint location(const std::string &name)
				{
					auto finded = locations.find(name);

					if (finded != locations.end())
					{
						return finded->second;
					}

					int result = glGetUniformLocation(m_program.id(), name.c_str());

					if (result < 0)
						throw not_found_exception(name);

					locations[name] = result;

					return result;
				}

				int texture(GLint location, int active_texture, const gl::texture& texture)
				{
					glActiveTexture(GL_TEXTURE0 + active_texture);
					texture.bind();
					(*this)[location] = active_texture;

					return active_texture;
				}

				int texture(const std::string &name, int active_texture, const gl::texture& texture_)
				{
					return texture(location(name), active_texture, texture_);
				}

				int texture(const std::string &name, const gl::texture& texture_)
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

					return texture(name, atex, texture_);
				}

				uniform_t operator[](GLint location)
				{
					return{ m_program, location };
				}

				uniform_t operator[](const std::string &name)
				{
					return{ m_program, location(name) };
				}

				void swap(uniforms_t& uniforms)
				{
					locations.swap(uniforms.locations);
					std::swap(active_texture, uniforms.active_texture);
				}
			} uniforms{ this };

			class attribs_t
			{
				program& m_program;
				std::unordered_map<std::string, GLint> m_locations;

			public:
				attribs_t(program* program) : m_program(*program)
				{
				}

				void clear()
				{
					m_locations.clear();
				}

				GLint location(const std::string &name)
				{
					auto finded = m_locations.find(name);

					if (finded != m_locations.end())
					{
						if (finded->second < 0)
							throw not_found_exception(name);

						return finded->second;
					}

					int result = glGetAttribLocation(m_program.id(), name.c_str());

					if (result < 0)
						throw not_found_exception(name);

					m_locations[name] = result;

					return result;
				}

				bool has_location(const std::string &name, int *location_ = nullptr)
				{
					auto finded = m_locations.find(name);

					if (finded != m_locations.end())
					{
						if (finded->second < 0)
							return false;

						*location_ = finded->second;
						return true;
					}

					int loc = glGetAttribLocation(m_program.id(), name.c_str());

					m_locations[name] = loc;

					if (loc < 0)
						return false;

					*location_ = loc;
					return true;
				}

				attrib_t operator[](GLint location)
				{
					return{ m_program.id(), location };
				}

				attrib_t operator[](const std::string &name)
				{
					return{ m_program.id(), location(name) };
				}

				void swap(attribs_t& attribs)
				{
					m_locations.swap(attribs.m_locations);
				}
			} attribs{ this };

			program& recreate()
			{
				if (created())
					remove();

				return create();
			}

			program& create()
			{
				m_id = glCreateProgram();
				return *this;
			}

			void remove()
			{
				glDeleteProgram(m_id);
				m_id = 0;
				uniforms.clear();
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

			void link()
			{
				glLinkProgram(m_id);

				GLint status = GL_FALSE;
				glGetProgramiv(m_id, GL_LINK_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetProgramInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					throw link_exception(error_msg);
				}
			}

			void validate()
			{
				glValidateProgram(m_id);

				GLint status = GL_FALSE;
				glGetProgramiv(m_id, GL_VALIDATE_STATUS, &status);

				if (status == GL_FALSE)
				{
					GLint length = 0;
					glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

					std::string error_msg;
					if (length)
					{
						std::unique_ptr<GLchar[]> buf(new char[length + 1]);
						glGetProgramInfoLog(m_id, length, nullptr, buf.get());
						error_msg = buf.get();
					}

					throw validation_exception(error_msg);
				}
			}

			void make()
			{
				link();
			}

			uint id() const
			{
				return m_id;
			}

			void set_id(uint id)
			{
				uniforms.clear();
				attribs.clear();
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

			program& attach(const shader& shader_)
			{
				glAttachShader(m_id, shader_.id());
				return *this;
			}

			program& bind_attribute_location(const std::string& name, int index)
			{
				glBindAttribLocation(m_id, index, name.c_str());
				return *this;
			}

			program& bind_fragment_data_location(const std::string& name, int color_number)
			{
				glBindFragDataLocation(m_id, color_number, name.c_str());
				return *this;
			}

			int attribute_location(const std::string& name)
			{
				return glGetAttribLocation(m_id, name.c_str());
			}

			int uniform_location(const std::string& name)
			{
				return glGetUniformLocation(m_id, name.c_str());
			}

			program& operator += (const shader& rhs)
			{
				return attach(rhs);
			}

			program& operator += (std::initializer_list<shader> shaders)
			{
				for (auto &shader : shaders)
					*this += shader;
				return *this;
			}

			program() = default;
			program(const program&) = delete;
			program(program&& program_)
			{
				swap(program_);
			}

			program(GLuint id)
			{
				set_id(id);
			}

			~program()
			{
				if (created())
					remove();
			}

			void swap(program& program_)
			{
				auto my_old_id = id();
				set_id(program_.id());
				program_.set_id(my_old_id);
				uniforms.swap(program_.uniforms);
				attribs.swap(program_.attribs);
			}

			program& operator = (const program& rhs) = delete;
			program& operator = (program&& rhs)
			{
				swap(rhs);
				return *this;
			}
		};

		class shader_view : public shader
		{
		public:
			shader_view(GLuint id) : shader(id)
			{
			}

			~shader_view()
			{
				set_id(0);
			}
		};

		class program_view : public program
		{
		public:
			program_view(GLuint id) : program(id)
			{
			}

			~program_view()
			{
				set_id(0);
			}
		};
	}

	class texture_view : public texture
	{
	public:
		texture_view(texture::target target_, GLuint id) : texture(target_, id)
		{
		}

		~texture_view()
		{
			set_id(0);
		}
	};

	class fbo_view : public fbo
	{
	public:
		fbo_view(GLuint id) : fbo(id)
		{
		}

		~fbo_view()
		{
			set_id(0);
		}
	};

	class rbo_view : public rbo
	{
	public:
		rbo_view(GLuint id) : rbo(id)
		{
		}

		~rbo_view()
		{
			set_id(0);
		}
	};

	class buffer_view : public buffer
	{
	public:
		buffer_view(GLuint id) : buffer(id)
		{
		}

		~buffer_view()
		{
			set_id(0);
		}
	};
}
