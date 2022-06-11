#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include "../GCM.h"
#include "../Common/TextureUtils.h"
#include "../Program/GLSLTypes.h"

#include "Emu/system_config.h"
#include "Utilities/mutex.h"
#include "Utilities/geometry.h"
#include "Utilities/File.h"
#include "util/logs.hpp"
#include "util/asm.hpp"

#include "glutils/common.h"
// TODO: Include on use
#include "glutils/buffer_object.h"
#include "glutils/image.h"
#include "glutils/pixel_settings.hpp"
#include "glutils/state_tracker.hpp"

// Noop keyword outside of Windows (used in log_debug)
#if !defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY
#endif


namespace gl
{
	void enable_debugging();
	bool is_primitive_native(rsx::primitive_type in);
	GLenum draw_mode(rsx::primitive_type in);

	void flush_command_queue(fence& fence_obj);

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

	class vao;
	class attrib_t;

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

	class vao
	{
		template<buffer::target BindId, uint GetStateId>
		class entry
		{
			vao& m_parent;

		public:
			using save_binding_state = save_binding_state_base<entry, (static_cast<GLuint>(BindId)), GetStateId>;

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
		vao(const vao&) = delete;

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
			if (m_id != GL_NONE)
			{
				glDeleteVertexArrays(1, &m_id);
				m_id = GL_NONE;
			}
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

		buffer_pointer operator + (u32 offset) noexcept
		{
			return{ this, offset };
		}

		buffer_pointer operator >> (u32 stride) noexcept
		{
			return{ this, {}, stride };
		}

		operator buffer_pointer() noexcept
		{
			return{ this };
		}

		attrib_t operator [] (u32 index) const noexcept;
	};

	class attrib_t
	{
		GLint m_location;

	public:
		attrib_t(GLint location)
			: m_location(location)
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

		void operator = (buffer_pointer& pointer) const
		{
			pointer.get_vao().enable_for_attribute(m_location);
			glVertexAttribPointer(location(), pointer.size(), static_cast<GLenum>(pointer.get_type()), pointer.normalize(),
				pointer.stride(), reinterpret_cast<const void*>(u64{pointer.offset()}));
		}
	};

	class rbo
	{
		GLuint m_id = GL_NONE;

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
			glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(format), width, height);
		}

		void remove()
		{
			if (m_id != GL_NONE)
			{
				glDeleteRenderbuffers(1, &m_id);
				m_id = GL_NONE;
			}
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
			return m_id != GL_NONE;
		}

		explicit operator bool() const
		{
			return created();
		}
	};

	enum class indices_type
	{
		ubyte = GL_UNSIGNED_BYTE,
		ushort = GL_UNSIGNED_SHORT,
		uint = GL_UNSIGNED_INT
	};

	class fbo
	{
		GLuint m_id = GL_NONE;
		size2i m_size;

	protected:
		std::unordered_map<GLenum, GLuint> m_resource_bindings;

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
			bool reset = true;

		public:
			save_binding_state(const fbo& new_binding)
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_last_binding);

				if (m_last_binding + 0u != new_binding.id())
					new_binding.bind();
				else
					reset = false;
			}

			~save_binding_state()
			{
				if (reset)
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
			GLuint m_id = GL_NONE;
			fbo &m_parent;

			attachment(fbo& parent)
				: m_parent(parent)
			{}

		public:
			attachment(fbo& parent, type type)
				: m_id(static_cast<int>(type))
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

			GLuint resource_id() const
			{
				const auto found = m_parent.m_resource_bindings.find(m_id);
				if (found != m_parent.m_resource_bindings.end())
				{
					return found->second;
				}

				return 0;
			}

			void operator = (const rbo& rhs)
			{
				m_parent.m_resource_bindings[m_id] = rhs.id();
				DSA_CALL2(NamedFramebufferRenderbuffer, m_parent.id(), m_id, GL_RENDERBUFFER, rhs.id());
			}

			void operator = (const texture& rhs)
			{
				ensure(rhs.get_target() == texture::target::texture2D);
				m_parent.m_resource_bindings[m_id] = rhs.id();
				DSA_CALL2(NamedFramebufferTexture, m_parent.id(), m_id, rhs.id(), 0);
			}

			void operator = (const GLuint rhs)
			{
				m_parent.m_resource_bindings[m_id] = rhs;
				DSA_CALL2(NamedFramebufferTexture, m_parent.id(), m_id, rhs, 0);
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

		struct null_attachment : public attachment
		{
			null_attachment(fbo& parent)
				: attachment(parent)
			{}
		};

		indexed_attachment color{ *this, attachment::type::color };
		attachment depth{ *this, attachment::type::depth };
		attachment stencil{ *this, attachment::type::stencil };
		attachment depth_stencil{ *this, attachment::type::depth_stencil };
		null_attachment no_color{ *this };

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
		bool check() const;

		void recreate();
		void draw_buffer(const attachment& buffer) const;
		void draw_buffers(const std::initializer_list<attachment>& indexes) const;

		void read_buffer(const attachment& buffer) const;

		void draw_arrays(rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const buffer& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;
		void draw_arrays(const vao& buffer, rsx::primitive_type mode, GLsizei count, GLint first = 0) const;

		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, indices_type type, const GLvoid *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset = 0) const;
		void draw_elements(const buffer& buffer_, rsx::primitive_type mode, GLsizei count, indices_type type, const buffer& indices, usz indices_buffer_offset = 0) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLubyte *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLushort *indices) const;
		void draw_elements(rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;
		void draw_elements(const buffer& buffer, rsx::primitive_type mode, GLsizei count, const GLuint *indices) const;

		void clear(buffers buffers_) const;
		void clear(buffers buffers_, color4f color_value, double depth_value, u8 stencil_value) const;

		void copy_from(const void* pixels, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;
		void copy_from(const buffer& buf, const sizei& size, gl::texture::format format_, gl::texture::type type_, class pixel_unpack_settings pixel_settings = pixel_unpack_settings()) const;

		void copy_to(void* pixels, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;
		void copy_to(const buffer& buf, coordi coord, gl::texture::format format_, gl::texture::type type_, class pixel_pack_settings pixel_settings = pixel_pack_settings()) const;

		static fbo get_bound_draw_buffer();
		static fbo get_bound_read_buffer();
		static fbo get_bound_buffer();

		GLuint id() const;
		void set_id(GLuint id);

		void set_extents(const size2i& extents);
		size2i get_extents() const;

		bool matches(const std::array<GLuint, 4>& color_targets, GLuint depth_stencil_target) const;
		bool references_any(const std::vector<GLuint>& resources) const;

		explicit operator bool() const
		{
			return created();
		}
	};

	extern const fbo screen;

	namespace glsl
	{
		class shader
		{
			std::string source;
			::glsl::program_domain type;
			GLuint m_id = GL_NONE;

			fence m_compiled_fence;
			fence m_init_fence;

			shared_mutex m_compile_lock;
			atomic_t<bool> m_is_compiled{};

			void precompile()
			{
				const char* str = source.c_str();
				const GLint length = ::narrow<GLint>(source.length());

				if (g_cfg.video.log_programs)
				{
					std::string base_name;
					switch (type)
					{
					case ::glsl::program_domain::glsl_vertex_program:
						base_name = "shaderlog/VertexProgram";
						break;
					case ::glsl::program_domain::glsl_fragment_program:
						base_name = "shaderlog/FragmentProgram";
						break;
					case ::glsl::program_domain::glsl_compute_program:
						base_name = "shaderlog/ComputeProgram";
						break;
					}

					fs::file(fs::get_cache_dir() + base_name + std::to_string(m_id) + ".glsl", fs::rewrite).write(str, length);
				}

				glShaderSource(m_id, 1, &str, &length);

				m_init_fence.create();
				flush_command_queue(m_init_fence);
			}

		public:
			shader() = default;

			shader(GLuint id)
			{
				set_id(id);
			}

			shader(::glsl::program_domain type_, const std::string& src)
			{
				create(type_, src);
			}

			~shader()
			{
				if (created())
					remove();
			}

			void create(::glsl::program_domain type_, const std::string& src)
			{
				type = type_;
				source = src;

				GLenum shader_type{};
				switch (type)
				{
				case ::glsl::program_domain::glsl_vertex_program:
					shader_type = GL_VERTEX_SHADER;
					break;
				case ::glsl::program_domain::glsl_fragment_program:
					shader_type = GL_FRAGMENT_SHADER;
					break;
				case ::glsl::program_domain::glsl_compute_program:
					shader_type = GL_COMPUTE_SHADER;
					break;
				default:
					rsx_log.fatal("gl::glsl::shader::compile(): Unhandled shader type (%d)", +type_);
					return;
				}

				m_id = glCreateShader(shader_type);
				precompile();
			}

			shader& compile()
			{
				std::lock_guard lock(m_compile_lock);
				if (m_is_compiled)
				{
					// Another thread compiled this already
					return *this;
				}

				ensure(!m_init_fence.is_empty()); // Do not attempt to compile a shader_view!!
				m_init_fence.server_wait_sync();

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

					rsx_log.fatal("Compilation failed: %s\nsource: %s", error_msg, source);
				}

				m_compiled_fence.create();
				flush_command_queue(m_compiled_fence);

				m_is_compiled = true;
				return *this;
			}

			void remove()
			{
				if (m_id != GL_NONE)
				{
					glDeleteShader(m_id);
					m_id = GL_NONE;
				}
			}

			uint id() const
			{
				return m_id;
			}

			const std::string& get_source() const
			{
				return source;
			}

			fence get_compile_fence_sync() const
			{
				return m_compiled_fence;
			}

			void set_id(uint id)
			{
				m_id = id;
			}

			bool created() const
			{
				return m_id != GL_NONE;
			}

			bool compiled() const
			{
				return m_is_compiled;
			}

			explicit operator bool() const
			{
				return created();
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

		class program
		{
			GLuint m_id = GL_NONE;
			fence m_fence;

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

				void operator = (int rhs) const { glProgramUniform1i(m_program.id(), location(), rhs); }
				void operator = (unsigned rhs) const { glProgramUniform1ui(m_program.id(), location(), rhs); }
				void operator = (float rhs) const { glProgramUniform1f(m_program.id(), location(), rhs); }
				void operator = (bool rhs) const { glProgramUniform1ui(m_program.id(), location(), rhs ? 1 : 0); }
				void operator = (const color1i& rhs) const { glProgramUniform1i(m_program.id(), location(), rhs.r); }
				void operator = (const color1f& rhs) const { glProgramUniform1f(m_program.id(), location(), rhs.r); }
				void operator = (const color2i& rhs) const { glProgramUniform2i(m_program.id(), location(), rhs.r, rhs.g); }
				void operator = (const color2f& rhs) const { glProgramUniform2f(m_program.id(), location(), rhs.r, rhs.g); }
				void operator = (const color3i& rhs) const { glProgramUniform3i(m_program.id(), location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color3f& rhs) const { glProgramUniform3f(m_program.id(), location(), rhs.r, rhs.g, rhs.b); }
				void operator = (const color4i& rhs) const { glProgramUniform4i(m_program.id(), location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const color4f& rhs) const { glProgramUniform4f(m_program.id(), location(), rhs.r, rhs.g, rhs.b, rhs.a); }
				void operator = (const areaf& rhs) const { glProgramUniform4f(m_program.id(), location(), rhs.x1, rhs.y1, rhs.x2, rhs.y2); }
				void operator = (const areai& rhs) const { glProgramUniform4i(m_program.id(), location(), rhs.x1, rhs.y1, rhs.x2, rhs.y2); }
				void operator = (const std::vector<int>& rhs) const { glProgramUniform1iv(m_program.id(), location(), ::size32(rhs), rhs.data()); }
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
					auto found = locations.find(name);
					if (found != locations.end())
					{
						if (location)
						{
							*location = found->second;
						}

						return (found->second >= 0);
					}

					auto result = glGetUniformLocation(m_program.id(), name.c_str());
					locations[name] = result;

					if (location)
					{
						*location = result;
					}

					return (result >= 0);
				}

				GLint location(const std::string &name)
				{
					auto found = locations.find(name);
					if (found != locations.end())
					{
						if (found->second >= 0)
						{
							return found->second;
						}
						else
						{
							rsx_log.fatal("%s not found.", name);
							return -1;
						}
					}

					auto result = glGetUniformLocation(m_program.id(), name.c_str());

					if (result < 0)
					{
						rsx_log.fatal("%s not found.", name);
						return result;
					}

					locations[name] = result;
					return result;
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
				if (m_id != GL_NONE)
				{
					glDeleteProgram(m_id);
					m_id = GL_NONE;
				}
				uniforms.clear();
			}

			static program get_current_program()
			{
				GLint id;
				glGetIntegerv(GL_CURRENT_PROGRAM, &id);
				return{ static_cast<GLuint>(id) };
			}

			void link(std::function<void(program*)> init_func = {})
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

					rsx_log.fatal("Linkage failed: %s", error_msg);
				}
				else
				{
					if (init_func)
					{
						init_func(this);
					}

					m_fence.create();
					flush_command_queue(m_fence);
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

					rsx_log.error("Validation failed: %s", error_msg.c_str());
				}
			}

			GLuint id() const
			{
				return m_id;
			}

			void set_id(uint id)
			{
				uniforms.clear();
				m_id = id;
			}

			bool created() const
			{
				return m_id != GL_NONE;
			}

			void sync()
			{
				if (!m_fence.check_signaled())
				{
					m_fence.server_wait_sync();
				}
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
			}

			program& operator = (const program& rhs) = delete;
			program& operator = (program&& rhs)
			{
				swap(rhs);
				return *this;
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

	class blitter
	{
		struct save_binding_state
		{
			GLuint old_fbo;

			save_binding_state()
			{
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&old_fbo));
			}

			~save_binding_state()
			{
				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
			}
		};

		fbo blit_src;
		fbo blit_dst;

	public:

		void init()
		{
			blit_src.create();
			blit_dst.create();
		}

		void destroy()
		{
			blit_dst.remove();
			blit_src.remove();
		}

		void scale_image(gl::command_context& cmd, const texture* src, texture* dst, areai src_rect, areai dst_rect, bool linear_interpolation,
			const rsx::typeless_xfer& xfer_info);

		void fast_clear_image(gl::command_context& cmd, const texture* dst, const color4f& color);
		void fast_clear_image(gl::command_context& cmd, const texture* dst, float depth, u8 stencil);
	};
}
