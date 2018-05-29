#pragma once

#include "stdafx.h"
#include "GLHelpers.h"
#include "../Overlays/overlays.h"

extern u64 get_system_time();

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

		gl::vao m_vao;
		gl::buffer m_vertex_data_buffer;

		bool compiled = false;

		u32 num_drawable_elements = 4;
		GLenum primitives = GL_TRIANGLE_STRIP;

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

				m_vertex_data_buffer.create();

				int old_vao;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

				m_vao.create();
				m_vao.bind();

				m_vao.array_buffer = m_vertex_data_buffer;
				auto ptr = buffer_pointer(&m_vao);
				m_vao[0] = ptr;

				glBindVertexArray(old_vao);

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
				m_vao.remove();
				m_vertex_data_buffer.remove();

				compiled = false;
			}
		}

		virtual void on_load() {}
		virtual void on_unload() {}

		virtual void bind_resources() {}
		virtual void cleanup_resources() {}

		virtual void upload_vertex_data(f32* data, u32 elements_count)
		{
			elements_count <<= 2;
			m_vertex_data_buffer.data(elements_count, data);
		}

		virtual void emit_geometry()
		{
			int old_vao;
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

			m_vao.bind();
			glDrawArrays(primitives, 0, num_drawable_elements);

			glBindVertexArray(old_vao);
		}

		virtual void run(u16 w, u16 h, GLuint target_texture, bool depth_target, bool use_blending = false)
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

			GLint blend_src_rgb;
			GLint blend_src_a;
			GLint blend_dst_rgb;
			GLint blend_dst_a;
			GLint blend_eq_a;
			GLint blend_eq_rgb;

			if (target_texture)
			{
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
			}

			if (!target_texture || glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
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
				GLboolean blend_enabled = glIsEnabledi(GL_BLEND, 0);
				GLboolean stencil_test_enabled = glIsEnabled(GL_STENCIL_TEST);

				if (use_blending)
				{
					glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_rgb);
					glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_a);
					glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_rgb);
					glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_a);
					glGetIntegerv(GL_BLEND_EQUATION_RGB, &blend_eq_rgb);
					glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blend_eq_a);
				}

				// Set initial state
				glViewport(0, 0, w, h);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				glDepthMask(depth_target ? GL_TRUE : GL_FALSE);

				// Disabling depth test will also disable depth writes which is not desired
				glDepthFunc(GL_ALWAYS);
				glEnable(GL_DEPTH_TEST);

				if (scissor_enabled) glDisable(GL_SCISSOR_TEST);
				if (cull_face_enabled) glDisable(GL_CULL_FACE);
				if (stencil_test_enabled) glDisable(GL_STENCIL_TEST);

				if (use_blending)
				{
					if (!blend_enabled)
						glEnablei(GL_BLEND, 0);

					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBlendEquation(GL_FUNC_ADD);
				}
				else if (blend_enabled)
				{
					glDisablei(GL_BLEND, 0);
				}

				// Render
				program_handle.use();
				on_load();
				bind_resources();
				emit_geometry();

				// Clean up
				if (target_texture)
				{
					if (depth_target)
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
					else
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

					glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
				}

				glUseProgram((GLuint)program);

				glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
				glColorMask(color_writes[0], color_writes[1], color_writes[2], color_writes[3]);
				glDepthMask(depth_write);
				glDepthFunc(depth_func);

				if (!depth_test_enabled) glDisable(GL_DEPTH_TEST);
				if (scissor_enabled) glEnable(GL_SCISSOR_TEST);
				if (cull_face_enabled) glEnable(GL_CULL_FACE);
				if (stencil_test_enabled) glEnable(GL_STENCIL_TEST);

				if (use_blending)
				{
					if (!blend_enabled)
						glDisablei(GL_BLEND, 0);

					glBlendFuncSeparate(blend_src_rgb, blend_dst_rgb, blend_src_a, blend_dst_a);
					glBlendEquationSeparate(blend_eq_rgb, blend_eq_a);
				}
				else if (blend_enabled)
				{
					 glEnablei(GL_BLEND, 0);
				}
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

	struct rgba8_unorm_rg16_sfloat_convert_pass : public overlay_pass
	{
		//Not really needed since directly copying data via ARB_copy_image works out fine
		rgba8_unorm_rg16_sfloat_convert_pass()
		{
			vs_src =
			{
				"#version 420\n\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexID % 4], 0., 1.);\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint value = packUnorm4x8(texelFetch(fs0, ivec2(gl_FragCoord.xy), 0).zyxw);\n"
				"	ocol.xy = unpackHalf2x16(value);\n"
				"}\n"
			};
		}

		void run(u16 w, u16 h, GLuint target, GLuint source)
		{
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, source);

			overlay_pass::run(w, h, target, false);
		}
	};

	struct ui_overlay_renderer : public overlay_pass
	{
		u32 num_elements = 0;
		std::vector<std::unique_ptr<gl::texture>> resources;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<gl::texture>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> temp_view_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture>> font_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> view_cache;
		bool is_font_draw = false;

		ui_overlay_renderer()
		{
			vs_src =
			{
				"#version 420\n\n"
				"layout(location=0) in vec4 in_pos;\n"
				"layout(location=0) out vec2 tc0;\n"
				"layout(location=1) out vec4 clip_rect;\n"
				"uniform vec4 ui_scale;\n"
				"uniform vec4 clip_bounds;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	tc0.xy = in_pos.zw;\n"
				"	clip_rect = (clip_bounds * ui_scale.zwzw);\n"
				"	clip_rect.yw = ui_scale.yy - clip_rect.wy; //invert y axis\n"
				"	vec4 pos = vec4((in_pos.xy * ui_scale.zw) / ui_scale.xy, 0., 1.);\n"
				"	pos.y = (1. - pos.y); //invert y axis\n"
				"	gl_Position = (pos + pos) - 1.;\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) in vec4 clip_rect;\n"
				"layout(location=0) out vec4 ocol;\n"
				"uniform vec4 color;\n"
				"uniform float time;\n"
				"uniform int read_texture;\n"
				"uniform int pulse_glow;\n"
				"uniform int clip_region;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	if (clip_region != 0)\n"
				"	{"
				"		if (gl_FragCoord.x < clip_rect.x || gl_FragCoord.x > clip_rect.z ||\n"
				"			gl_FragCoord.y < clip_rect.y || gl_FragCoord.y > clip_rect.w)\n"
				"		{\n"
				"			discard;\n"
				"			return;\n"
				"		}\n"
				"	}\n"
				"\n"
				"	vec4 diff_color = color;\n"
				"	if (pulse_glow != 0)\n"
				"		diff_color.a *= (sin(time) + 1.f) * 0.5f;\n"
				"\n"
				"	if (read_texture != 0)\n"
				"		ocol = texture(fs0, tc0) * diff_color;\n"
				"	else\n"
				"		ocol = diff_color;\n"
				"}\n"
			};
		}

		gl::texture_view* load_simple_image(rsx::overlays::image_info* desc, bool temp_resource, u32 owner_uid)
		{
			auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D, desc->w, desc->h, 1, 1, GL_RGBA8);
			tex->copy_from(desc->data, gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8);

			GLenum remap[] = { GL_RED, GL_ALPHA, GL_BLUE, GL_GREEN };
			auto view = std::make_unique<gl::texture_view>(tex.get(), remap);

			auto result = view.get();
			if (!temp_resource)
			{
				resources.push_back(std::move(tex));
				view_cache[view_cache.size()] = std::move(view);
			}
			else
			{
				u64 key = (u64)desc;
				temp_image_cache[key] = std::move(std::make_pair(owner_uid, std::move(tex)));
				temp_view_cache[key] = std::move(view);
			}

			return result;
		}

		void create()
		{
			overlay_pass::create();

			rsx::overlays::resource_config configuration;
			configuration.load_files();

			for (const auto &res : configuration.texture_raw_data)
			{
				load_simple_image(res.get(), false, UINT32_MAX);
			}

			configuration.free_resources();
		}

		void destroy()
		{
			temp_image_cache.clear();
			resources.clear();
			font_cache.clear();
			overlay_pass::destroy();
		}

		void remove_temp_resources(u64 key)
		{
			std::vector<u64> keys_to_remove;
			for (auto It = temp_image_cache.begin(); It != temp_image_cache.end(); ++It)
			{
				if (It->second.first == key)
				{
					keys_to_remove.push_back(It->first);
				}
			}

			for (const auto& _key : keys_to_remove)
			{
				temp_image_cache.erase(_key);
				temp_view_cache.erase(_key);
			}
		}

		gl::texture_view* find_font(rsx::overlays::font *font)
		{
			u64 key = (u64)font;
			auto found = view_cache.find(key);
			if (found != view_cache.end())
				return found->second.get();

			//Create font file
			auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D, (int)font->width, (int)font->height, 1, 1, GL_R8);
			tex->copy_from(font->glyph_data.data(), gl::texture::format::r, gl::texture::type::ubyte);

			GLenum remap[] = { GL_RED, GL_RED, GL_RED, GL_RED };
			auto view = std::make_unique<gl::texture_view>(tex.get(), remap);

			auto result = view.get();
			font_cache[key] = std::move(tex);
			view_cache[key] = std::move(view);

			return result;
		}

		gl::texture_view* find_temp_image(rsx::overlays::image_info *desc, u32 owner_uid)
		{
			auto key = (u64)desc;
			auto cached = temp_view_cache.find(key);
			if (cached != temp_view_cache.end())
			{
				return cached->second.get();
			}
			else
			{
				return load_simple_image(desc, true, owner_uid);
			}
		}

		void emit_geometry() override
		{
			if (!is_font_draw)
			{
				overlay_pass::emit_geometry();
			}
			else
			{
				int num_quads = num_drawable_elements / 4;
				std::vector<GLint> firsts;
				std::vector<GLsizei> counts;

				firsts.resize(num_quads);
				counts.resize(num_quads);

				for (int n = 0; n < num_quads; ++n)
				{
					firsts[n] = (n * 4);
					counts[n] = 4;
				}

				int old_vao;
				glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

				m_vao.bind();
				glMultiDrawArrays(GL_TRIANGLE_STRIP, firsts.data(), counts.data(), num_quads);

				glBindVertexArray(old_vao);
			}
		}

		void run(u16 w, u16 h, GLuint target, rsx::overlays::overlay& ui)
		{
			program_handle.uniforms["ui_scale"] = color4f((f32)ui.virtual_width, (f32)ui.virtual_height, 1.f, 1.f);
			program_handle.uniforms["time"] = (f32)(get_system_time() / 1000) * 0.005f;
			for (auto &cmd : ui.get_compiled().draw_commands)
			{
				upload_vertex_data((f32*)cmd.second.data(), (u32)cmd.second.size() * 4u);
				num_drawable_elements = (u32)cmd.second.size();
				is_font_draw = false;
				GLint texture_exists = GL_TRUE;

				glActiveTexture(GL_TEXTURE31);
				switch (cmd.first.texture_ref)
				{
				case rsx::overlays::image_resource_id::game_icon:
				case rsx::overlays::image_resource_id::backbuffer:
					//TODO
				case rsx::overlays::image_resource_id::none:
				{
					texture_exists = GL_FALSE;
					glBindTexture(GL_TEXTURE_2D, GL_NONE);
					break;
				}
				case rsx::overlays::image_resource_id::raw_image:
				{
					glBindTexture(GL_TEXTURE_2D, find_temp_image((rsx::overlays::image_info*)cmd.first.external_data_ref, ui.uid)->id());
					break;
				}
				case rsx::overlays::image_resource_id::font_file:
				{
					is_font_draw = true;
					glBindTexture(GL_TEXTURE_2D, find_font(cmd.first.font_ref)->id());
					break;
				}
				default:
				{
					glBindTexture(GL_TEXTURE_2D, view_cache[cmd.first.texture_ref - 1]->id());
					break;
				}
				}

				program_handle.uniforms["color"] = cmd.first.color;
				program_handle.uniforms["read_texture"] = texture_exists;
				program_handle.uniforms["pulse_glow"] = (s32)cmd.first.pulse_glow;
				program_handle.uniforms["clip_region"] = (s32)cmd.first.clip_region;
				program_handle.uniforms["clip_bounds"] = cmd.first.clip_rect;
				overlay_pass::run(w, h, target, false, true);
			}

			ui.update();
		}
	};

	struct video_out_calibration_pass : public overlay_pass
	{
		video_out_calibration_pass()
		{
			vs_src =
			{
				"#version 420\n\n"
				"layout(location=0) out vec2 tc0;\n"
				"uniform float x_scale;\n"
				"uniform float y_scale;\n"
				"uniform float x_offset;\n"
				"uniform float y_offset;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 1.), vec2(1., 1.), vec2(0., 0.), vec2(1., 0.)};\n"
				"	tc0 = coords[gl_VertexID % 4];\n"
				"	vec2 pos = positions[gl_VertexID % 4] * vec2(x_scale, y_scale) + (2. * vec2(x_offset, y_offset));\n"
				"	gl_Position = vec4(pos, 0., 1.);\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"uniform float gamma;\n"
				"uniform int limit_range;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 color = texture(fs0, tc0);\n"
				"	color.rgb = pow(color.rgb, vec3(gamma));\n"
				"	if (limit_range > 0)\n"
				"		ocol = ((color * 220.) + 16.) / 255.;\n"
				"	else\n"
				"		ocol = color;\n"
				"}\n"
			};
		}

		void run(u16 w, u16 h, GLuint source, const areai& region, f32 gamma, bool limited_rgb)
		{
			const f32 x_scale = (f32)(region.x2 - region.x1) / w;
			const f32 y_scale = (f32)(region.y2 - region.y1) / h;
			const f32 x_offset = (f32)(region.x1) / w;
			const f32 y_offset = (f32)(region.y1) / h;

			program_handle.uniforms["x_scale"] = x_scale;
			program_handle.uniforms["y_scale"] = y_scale;
			program_handle.uniforms["x_offset"] = x_offset;
			program_handle.uniforms["y_offset"] = y_offset;
			program_handle.uniforms["gamma"] = gamma;
			program_handle.uniforms["limit_range"] = (int)limited_rgb;

			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, source);
			overlay_pass::run(w, h, GL_NONE, false, false);
		}
	};
}
