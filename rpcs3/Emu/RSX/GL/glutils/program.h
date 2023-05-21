#pragma once

#include "common.h"
#include "sync.hpp"

#include "Emu/RSX/Program/GLSLTypes.h"
#include "Utilities/geometry.h"
#include "Utilities/mutex.h"

namespace gl
{
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

			void precompile();

		public:
			shader() = default;

			shader(::glsl::program_domain type_, const std::string& src)
			{
				create(type_, src);
			}

			~shader()
			{
				remove();
			}

			void remove()
			{
				if (m_id)
				{
					glDeleteShader(m_id);
					m_id = GL_NONE;
				}
			}

			void create(::glsl::program_domain type_, const std::string& src);

			shader& compile();

			uint id() const { return m_id; }

			const std::string& get_source() const { return source; }

			fence get_compile_fence_sync() const { return m_compiled_fence; }

			bool created() const { return m_id != GL_NONE; }

			bool compiled() const { return m_is_compiled; }

			explicit operator bool() const { return created(); }
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

			public:
				uniforms_t(program* program)
					: m_program(*program)
				{}

				void clear() { locations.clear(); }

				bool has_location(const std::string& name, int* location = nullptr);

				GLint location(const std::string& name);

				uniform_t operator[](GLint location) { return{ m_program, location }; }

				uniform_t operator[](const std::string& name) { return{ m_program, location(name) }; }

			} uniforms{ this };

		public:
			program() = default;
			program(const program&) = delete;

			~program()
			{
				if (created())
				{
					remove();
				}
			}

			program& recreate()
			{
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
				m_id = GL_NONE;
				uniforms.clear();
			}

			void link(std::function<void(program*)> init_func = {});

			void validate();

			void sync()
			{
				if (!m_fence.check_signaled())
				{
					m_fence.server_wait_sync();
				}
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

			GLuint id() const { return m_id; }

			bool created() const { return m_id != GL_NONE; }

			explicit operator bool() const { return created(); }
		};
	}
}
