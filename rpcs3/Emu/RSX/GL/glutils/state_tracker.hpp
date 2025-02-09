#pragma once

#include "capabilities.h"

#include "Utilities/geometry.h"
#include <unordered_map>

namespace gl
{
	class driver_state
	{
		const u32 DEPTH_BOUNDS       = 0xFFFF0001;
		const u32 CLIP_PLANES        = 0xFFFF0002;
		const u32 DEPTH_RANGE        = 0xFFFF0004;
		const u32 STENCIL_FRONT_FUNC = 0xFFFF0005;
		const u32 STENCIL_BACK_FUNC  = 0xFFFF0006;
		const u32 STENCIL_FRONT_OP   = 0xFFFF0007;
		const u32 STENCIL_BACK_OP    = 0xFFFF0008;
		const u32 STENCIL_BACK_MASK  = 0xFFFF0009;

		std::unordered_map<GLenum, u64> properties = {};
		std::unordered_map<GLenum, std::array<u64, 4>> indexed_properties = {};

		GLuint current_program = GL_NONE;
		std::array<std::unordered_map<GLenum, GLuint>, 48> bound_textures{ {} };

		bool test_and_set_property(GLenum property, u64 test)
		{
			auto found = properties.find(property);
			if (found != properties.end() && found->second == test)
				return true;

			properties[property] = test;
			return false;
		}

		bool test_and_set_property(GLenum property, u64 test, GLint index)
		{
			auto found = indexed_properties.find(property);
			if (found != indexed_properties.end())
			{
				if (found->second[index] == test)
				{
					return true;
				}

				found->second[index] = test;
				return false;
			}

			indexed_properties[property][index] = test;
			return false;
		}

	public:

		bool enable(u32 test, GLenum cap)
		{
			auto found = properties.find(cap);
			if (found != properties.end() && found->second == test)
				return !!test;

			properties[cap] = test;

			if (test)
				glEnable(cap);
			else
				glDisable(cap);

			return !!test;
		}

		bool enablei(u32 test, GLenum cap, u32 index)
		{
			auto found = indexed_properties.find(cap);
			const bool exists = found != indexed_properties.end();

			if (!exists)
			{
				indexed_properties[cap] = {};
				indexed_properties[cap][index] = test;
			}
			else
			{
				if (found->second[index] == test)
					return !!test;

				found->second[index] = test;
			}

			if (test)
				glEnablei(cap, index);
			else
				glDisablei(cap, index);

			return !!test;
		}

		bool enable(GLenum cap)
		{
			return enable(GL_TRUE, cap);
		}

		bool enablei(GLenum cap, u32 index)
		{
			return enablei(GL_TRUE, cap, index);
		}

		bool disable(GLenum cap)
		{
			return enable(GL_FALSE, cap);
		}

		bool disablei(GLenum cap, u32 index)
		{
			return enablei(GL_FALSE, cap, index);
		}

		void depth_func(GLenum func)
		{
			if (!test_and_set_property(GL_DEPTH_FUNC, func))
			{
				glDepthFunc(func);
			}
		}

		void depth_mask(GLboolean mask)
		{
			if (!test_and_set_property(GL_DEPTH_WRITEMASK, mask))
			{
				glDepthMask(mask);
			}
		}

		void clear_depth(GLfloat depth)
		{
			const u32 value = std::bit_cast<u32>(depth);
			if (!test_and_set_property(GL_DEPTH_CLEAR_VALUE, value))
			{
				glClearDepth(depth);
			}
		}

		void stencil_mask(GLuint mask)
		{
			if (!test_and_set_property(GL_STENCIL_WRITEMASK, mask))
			{
				glStencilMask(mask);
			}
		}

		void stencil_back_mask(GLuint mask)
		{
			if (!test_and_set_property(STENCIL_BACK_MASK, mask))
			{
				glStencilMaskSeparate(GL_BACK, mask);
			}
		}

		void clear_stencil(GLint stencil)
		{
			const u32 value = std::bit_cast<u32>(stencil);
			if (!test_and_set_property(GL_STENCIL_CLEAR_VALUE, value))
			{
				glClearStencil(stencil);
			}
		}

		void stencil_func(GLenum func, GLint ref, GLuint mask)
		{
			const u32 value = func | ref << 16u | mask << 24;
			if (!test_and_set_property(STENCIL_FRONT_FUNC, value))
			{
				glStencilFunc(func, ref, mask);
			}
		}

		void stencil_back_func(GLenum func, GLint ref, GLuint mask)
		{
			const u32 value = func | ref << 16u | mask << 24;
			if (!test_and_set_property(STENCIL_BACK_FUNC, value))
			{
				glStencilFunc(func, ref, mask);
			}
		}

		void stencil_op(GLenum fail, GLenum zfail, GLenum zpass)
		{
			const u64 value = static_cast<u64>(fail) << 32 | static_cast<u64>(zfail) << 16 | static_cast<u64>(zpass);
			if (!test_and_set_property(STENCIL_FRONT_OP, value))
			{
				glStencilOp(fail, zfail, zpass);
			}
		}

		void stencil_back_op(GLenum fail, GLenum zfail, GLenum zpass)
		{
			const u64 value = static_cast<u64>(fail) << 32 | static_cast<u64>(zfail) << 16 | static_cast<u64>(zpass);
			if (!test_and_set_property(STENCIL_BACK_OP, value))
			{
				glStencilOpSeparate(GL_BACK, fail, zfail, zpass);
			}
		}

		void color_maski(GLint index, u32 mask)
		{
			if (!test_and_set_property(GL_COLOR_WRITEMASK, mask, index))
			{
				glColorMaski(index, ((mask & 0x10) ? 1 : 0), ((mask & 0x20) ? 1 : 0), ((mask & 0x40) ? 1 : 0), ((mask & 0x80) ? 1 : 0));
			}
		}

		void color_maski(GLint index, bool r, bool g, bool b, bool a)
		{
			u32 mask = 0;
			if (r) mask |= 0x10;
			if (g) mask |= 0x20;
			if (b) mask |= 0x40;
			if (a) mask |= 0x80;

			color_maski(index, mask);
		}

		void clear_color(u8 r, u8 g, u8 b, u8 a)
		{
			const u32 value = u32{ r } | u32{ g } << 8 | u32{ b } << 16 | u32{ a } << 24;
			if (!test_and_set_property(GL_COLOR_CLEAR_VALUE, value))
			{
				glClearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
			}
		}

		void clear_color(const color4f& color)
		{
			clear_color(static_cast<u8>(color.r * 255), static_cast<u8>(color.g * 255), static_cast<u8>(color.b * 255), static_cast<u8>(color.a * 255));
		}

		void depth_bounds(float min, float max)
		{
			const u64 value = (static_cast<u64>(std::bit_cast<u32>(max)) << 32) | std::bit_cast<u32>(min);
			if (!test_and_set_property(DEPTH_BOUNDS, value))
			{
				if (get_driver_caps().NV_depth_buffer_float_supported)
				{
					glDepthBoundsdNV(min, max);
				}
				else
				{
					glDepthBoundsEXT(min, max);
				}
			}
		}

		void depth_range(float min, float max)
		{
			const u64 value = (static_cast<u64>(std::bit_cast<u32>(max)) << 32) | std::bit_cast<u32>(min);
			if (!test_and_set_property(DEPTH_RANGE, value))
			{
				if (get_driver_caps().NV_depth_buffer_float_supported)
				{
					glDepthRangedNV(min, max);
				}
				else
				{
					glDepthRange(min, max);
				}
			}
		}

		void logic_op(GLenum op)
		{
			if (!test_and_set_property(GL_COLOR_LOGIC_OP, op))
			{
				glLogicOp(op);
			}
		}

		void line_width(GLfloat width)
		{
			u32 value = std::bit_cast<u32>(width);
			if (!test_and_set_property(GL_LINE_WIDTH, value))
			{
				glLineWidth(width);
			}
		}

		void front_face(GLenum face)
		{
			if (!test_and_set_property(GL_FRONT_FACE, face))
			{
				glFrontFace(face);
			}
		}

		void cull_face(GLenum mode)
		{
			if (!test_and_set_property(GL_CULL_FACE_MODE, mode))
			{
				glCullFace(mode);
			}
		}

		void polygon_offset(float factor, float units)
		{
			const u64 value = (static_cast<u64>(std::bit_cast<u32>(units)) << 32) | std::bit_cast<u32>(factor);
			if (!test_and_set_property(GL_POLYGON_OFFSET_FILL, value))
			{
				glPolygonOffset(factor, units);
			}
		}

		void sample_mask(GLbitfield mask)
		{
			if (!test_and_set_property(GL_SAMPLE_MASK_VALUE, mask))
			{
				glSampleMaski(0, mask);
			}
		}

		void sample_coverage(GLclampf coverage)
		{
			const u32 value = std::bit_cast<u32>(coverage);
			if (!test_and_set_property(GL_SAMPLE_COVERAGE_VALUE, value))
			{
				glSampleCoverage(coverage, GL_FALSE);
			}
		}

		void min_sample_shading_rate(GLclampf rate)
		{
			const u32 value = std::bit_cast<u32>(rate);
			if (!test_and_set_property(GL_MIN_SAMPLE_SHADING_VALUE, value))
			{
				glMinSampleShading(rate);
			}
		}

		void clip_planes(GLuint mask)
		{
			if (!test_and_set_property(CLIP_PLANES, mask))
			{
				for (u32 i = 0; i < 6; ++i)
				{
					if (mask & (1 << i))
					{
						glEnable(GL_CLIP_DISTANCE0 + i);
					}
					else
					{
						glDisable(GL_CLIP_DISTANCE0 + i);
					}
				}
			}
		}

		void use_program(GLuint program)
		{
			if (current_program == program)
			{
				return;
			}

			current_program = program;
			glUseProgram(program);
		}

		GLuint get_bound_texture(GLuint layer, GLenum target)
		{
			ensure(layer < 48);
			return bound_textures[layer][target];
		}

		void bind_texture(GLuint layer, GLenum target, GLuint name, GLboolean force = GL_FALSE)
		{
			ensure(layer < 48);

			auto& bound = bound_textures[layer][target];
			if (bound != name || force)
			{
				glActiveTexture(GL_TEXTURE0 + layer);
				glBindTexture(target, name);

				bound = name;
			}
		}

		void unbind_texture(GLenum target, GLuint name)
		{
			// To be called with glDeleteTextures.
			// OpenGL internally unbinds the texture on delete, but then reuses the same ID when GenTextures is called again!
			// This can also be avoided using unique internal names, such as 64-bit handles, but that involves changing a lot of code for little benefit
			for (auto& layer : bound_textures)
			{
				if (layer.empty())
				{
					continue;
				}

				if (auto found = layer.find(target);
					found != layer.end() && found->second == name)
				{
					// Actually still bound!
					found->second = GL_NONE;
					return;
				}
			}
		}
	};

	class command_context
	{
		driver_state* drv;

	public:
		command_context()
			: drv(nullptr)
		{}

		command_context(driver_state& drv_)
			: drv(&drv_)
		{}

		driver_state* operator -> () {
			return drv;
		}
	};

	void set_command_context(gl::command_context& ctx);
	void set_command_context(gl::driver_state& ctx);
	gl::command_context get_command_context();

	void set_primary_context_thread(bool = true);
	bool is_primary_context_thread();

	class fence;
	void flush_command_queue(fence& fence_obj);
}
