#pragma once

#include "stdafx.h"
#include "GLHelpers.h"

namespace gl
{
	struct overlay_pass
	{
		std::string fs_src;
		std::string vs_src;

		gl::glsl::program program_handle;
		gl::glsl::shader vs;
		gl::glsl::shader fs;

		gl::fbo fbo;

		bool compiled = false;

		void create()
		{
			if (!compiled)
			{
				fs.create(gl::glsl::shader::type::fragment);
				fs.source(fs_src);
				fs.compile();

				vs.create(gl::glsl::shader::type::vertex);
				vs.source(vs_src);
				vs.compile();

				program_handle.create();
				program_handle.attach(vs);
				program_handle.attach(fs);
				program_handle.make();

				fbo.create();

				compiled = true;
			}
		}

		void destroy()
		{
			if (compiled)
			{
				program_handle.remove();
				vs.remove();
				fs.remove();

				fbo.remove();

				compiled = false;
			}
		}

		virtual void on_load() {}
		virtual void on_unload() {}

		virtual void bind_resources() {}
		virtual void cleanup_resources() {}

		virtual void emit_geometry()
		{
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		virtual void run(u16 w, u16 h, GLuint target_texture, bool depth_target)
		{
			if (!compiled)
			{
				LOG_ERROR(RSX, "You must initialize overlay passes with create() before calling run()");
				return;
			}

			GLint program;
			GLint old_fbo;
			GLint depth_func;
			GLint viewport[4];
			GLboolean color_writes[4];
			GLboolean depth_write;

			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo.id());

			if (depth_target)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target_texture, 0);
				glDrawBuffer(GL_NONE);
			}
			else
			{
				GLenum buffer = GL_COLOR_ATTACHMENT0;
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0);
				glDrawBuffers(1, &buffer);
			}

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
			{
				// Push rasterizer state
				glGetIntegerv(GL_VIEWPORT, viewport);
				glGetBooleanv(GL_COLOR_WRITEMASK, color_writes);
				glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write);
				glGetIntegerv(GL_CURRENT_PROGRAM, &program);
				glGetIntegerv(GL_DEPTH_FUNC, &depth_func);

				GLboolean scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);
				GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
				GLboolean cull_face_enabled = glIsEnabled(GL_CULL_FACE);
				GLboolean blend_enabled = glIsEnabled(GL_BLEND);
				GLboolean stencil_test_enabled = glIsEnabled(GL_STENCIL_TEST);

				// Set initial state
				glViewport(0, 0, w, h);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				glDepthMask(depth_target ? GL_TRUE : GL_FALSE);

				// Disabling depth test will also disable depth writes which is not desired
				glDepthFunc(GL_ALWAYS);
				glEnable(GL_DEPTH_TEST);

				if (scissor_enabled) glDisable(GL_SCISSOR_TEST);
				if (cull_face_enabled) glDisable(GL_CULL_FACE);
				if (blend_enabled) glDisable(GL_BLEND);
				if (stencil_test_enabled) glDisable(GL_STENCIL_TEST);

				// Render
				program_handle.use();
				on_load();
				bind_resources();
				emit_geometry();

				// Clean up
				if (depth_target)
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
				else
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
				glUseProgram((GLuint)program);

				glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
				glColorMask(color_writes[0], color_writes[1], color_writes[2], color_writes[3]);
				glDepthMask(depth_write);
				glDepthFunc(depth_func);

				if (!depth_test_enabled) glDisable(GL_DEPTH_TEST);
				if (scissor_enabled) glEnable(GL_SCISSOR_TEST);
				if (cull_face_enabled) glEnable(GL_CULL_FACE);
				if (blend_enabled) glEnable(GL_BLEND);
				if (stencil_test_enabled) glEnable(GL_STENCIL_TEST);
			}
			else
			{
				LOG_ERROR(RSX, "Overlay pass failed because framebuffer was not complete. Run with debug output enabled to diagnose the problem");
			}
		}
	};

	struct depth_convert_pass : public overlay_pass
	{
		depth_convert_pass()
		{
			vs_src =
			{
				"#version 420\n\n"
				"out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexID % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexID % 4];\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n\n"
				"in vec2 tc0;\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 rgba_in = texture(fs0, tc0);\n"
				"	gl_FragDepth = rgba_in.w * 0.99609 + rgba_in.x * 0.00389 + rgba_in.y * 0.00002;\n"
				"}\n"
			};
		}

		void run(u16 w, u16 h, GLuint target, GLuint source)
		{
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, source);

			overlay_pass::run(w, h, target, true);
		}
	};
}