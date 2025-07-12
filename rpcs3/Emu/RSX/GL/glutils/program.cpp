#include "stdafx.h"
#include "program.h"
#include "state_tracker.hpp"

#include "Emu/system_config.h"

namespace gl
{
	namespace glsl
	{
		void patch_macros_INTEL(std::string& source)
		{
			auto read_token = [&source](size_t start) -> std::tuple<size_t, size_t>
			{
				size_t string_begin = std::string::npos, i = start;
				for (size_t count = 0; i < source.length(); ++i)
				{
					const auto& c = source[i];
					const auto is_space = std::isspace(c);

					if (string_begin == std::string::npos)
					{
						if (c == '\n') break;
						if (is_space) continue;

						string_begin = i;
					}

					if (is_space)
					{
						if (!count) break;
					}
					else if (c == '(')
					{
						count++;
					}
					else if (c == ')')
					{
						count--;
					}
				}

				return std::make_tuple(string_begin, i - 1);
			};

			auto is_exempt = [&source](const std::string_view& token) -> bool
			{
				const char* handled_keywords[] =
				{
					"SSBO_LOCATION(x)",
					"UBO_LOCATION(x)",
					"IMAGE_LOCATION(x)"
				};

				for (const auto& keyword : handled_keywords)
				{
					if (token.starts_with(keyword))
					{
						return false;
					}
				}

				return true;
			};

			size_t prev_loc = 0;
			while (true)
			{
				// Find macro define blocks and remove the outer-most brackets around the expression part
				const auto next_loc = source.find("#define", prev_loc);
				if (next_loc == std::string::npos)
				{
					break;
				}

				prev_loc = next_loc + 1;

				const auto [name_start, name_end] = read_token(next_loc + ("#define"sv).length());
				if (name_start == std::string::npos)
				{
					break;
				}

				const auto macro_name = std::string_view(source.data() + name_start, (name_end - name_start) + 1);
				if (is_exempt(macro_name))
				{
					continue;
				}

				const auto [expr_start, expr_end] = read_token(name_end + 1);
				if (expr_start == std::string::npos)
				{
					continue;
				}

				if (source[expr_start] == '(' && source[expr_end] == ')')
				{
					rsx_log.notice("[Compiler warning] We'll remove brackets around the expression named '%s'. Add it to exclusion list if this is not desired.", macro_name);

					source[expr_start] = ' ';
					source[expr_end] = ' ';
				}
			}
		}

		void shader::precompile()
		{
			if (gl::get_driver_caps().vendor_INTEL)
			{
				// Workaround for broken macro expansion.
				patch_macros_INTEL(source);
			}

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
				default:
					fmt::throw_exception("Unexpected program type %d", static_cast<int>(type));
				}

				fs::write_file(fs::get_cache_dir() + base_name + std::to_string(m_id) + ".glsl", fs::rewrite, str, length);
			}

			glShaderSource(m_id, 1, &str, &length);

			m_init_fence.create();
			flush_command_queue(m_init_fence);
		}

		void shader::create(::glsl::program_domain type_, const std::string & src)
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

		shader& shader::compile()
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

		bool program::uniforms_t::has_location(const std::string & name, int* location)
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

		GLint program::uniforms_t::location(const std::string& name)
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

		void program::link(std::function<void(program*)> init_func)
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

		void program::validate()
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
	}
}
