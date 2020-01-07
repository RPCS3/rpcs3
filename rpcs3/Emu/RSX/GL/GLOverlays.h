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
		gl::sampler_state m_sampler;

		gl::vao m_vao;
		gl::buffer m_vertex_data_buffer;

		bool compiled = false;

		u32 num_drawable_elements = 4;
		GLenum primitives = GL_TRIANGLE_STRIP;
		GLenum input_filter = GL_NEAREST;

		struct saved_sampler_state
		{
			GLuint saved = GL_NONE;
			GLuint unit = 0;

			saved_sampler_state(GLuint _unit, const gl::sampler_state& sampler)
			{
				glActiveTexture(GL_TEXTURE0 + _unit);
				glGetIntegerv(GL_SAMPLER_BINDING, reinterpret_cast<GLint*>(&saved));

				unit = _unit;
				sampler.bind(_unit);
			}

			saved_sampler_state(const saved_sampler_state&) = delete;

			~saved_sampler_state()
			{
				glBindSampler(unit, saved);
			}
		};

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

				m_sampler.create();
				m_sampler.apply_defaults(input_filter);

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

				m_sampler.remove();

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

		void run(const areau& region, GLuint target_texture, bool depth_target, bool use_blending = false)
		{
			if (!compiled)
			{
				rsx_log.error("You must initialize overlay passes with create() before calling run()");
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
				glViewport(region.x1, region.y1, region.width(), region.height());
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

				glUseProgram(program);

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
				rsx_log.error("Overlay pass failed because framebuffer was not complete. Run with debug output enabled to diagnose the problem");
			}
		}
	};

	struct depth_convert_pass : public overlay_pass
	{
		depth_convert_pass()
		{
			vs_src =
				"#version 420\n\n"
				"uniform vec2 tex_scale;\n"
				"out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexID % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexID % 4] * tex_scale;\n"
				"}\n";

			fs_src =
				"#version 420\n\n"
				"in vec2 tc0;\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 rgba_in = texture(fs0, tc0);\n"
				"	gl_FragDepth = rgba_in.w * 0.99609 + rgba_in.x * 0.00389 + rgba_in.y * 0.00002;\n"
				"}\n";
		}

		void run(const areai& src_area, const areai& dst_area, gl::texture* source, gl::texture* target)
		{
			const auto src_ratio_x = f32(src_area.x2) / source->width();
			const auto src_ratio_y = f32(src_area.y2) / source->height();

			program_handle.uniforms["tex_scale"] = color2f(src_ratio_x, src_ratio_y);

			saved_sampler_state saved(31, m_sampler);
			glBindTexture(GL_TEXTURE_2D, source->id());

			overlay_pass::run(static_cast<areau>(dst_area), target->id(), true);
		}
	};

	struct rgba8_unorm_rg16_sfloat_convert_pass : public overlay_pass
	{
		//Not really needed since directly copying data via ARB_copy_image works out fine
		rgba8_unorm_rg16_sfloat_convert_pass()
		{
			vs_src =
				"#version 420\n\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexID % 4], 0., 1.);\n"
				"}\n";

			fs_src =
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	uint value = packUnorm4x8(texelFetch(fs0, ivec2(gl_FragCoord.xy), 0).zyxw);\n"
				"	ocol.xy = unpackHalf2x16(value);\n"
				"}\n";
		}

		void run(const areau& viewport, GLuint target, GLuint source)
		{
			saved_sampler_state saved(31, m_sampler);
			glBindTexture(GL_TEXTURE_2D, source);

			overlay_pass::run(viewport, target, false);
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
		rsx::overlays::primitive_type m_current_primitive_type = rsx::overlays::primitive_type::quad_list;

		ui_overlay_renderer()
		{
			vs_src =
				"#version 420\n\n"
				"layout(location=0) in vec4 in_pos;\n"
				"layout(location=0) out vec2 tc0;\n"
				"layout(location=1) flat out vec4 clip_rect;\n"
				"uniform vec4 ui_scale;\n"
				"uniform vec4 viewport;\n"
				"uniform vec4 clip_bounds;\n"
				"\n"
				"vec2 snap_to_grid(vec2 normalized)\n"
				"{\n"
				"	return (floor(normalized * viewport.xy) + 0.5) / viewport.xy;\n"
				"}\n"
				"\n"
				"vec4 clip_to_ndc(const in vec4 coord)\n"
				"{\n"
				"	vec4 ret = (coord * ui_scale.zwzw) / ui_scale.xyxy;\n"
				"	ret.yw = 1. - ret.yw;\n"
				"	return ret;\n"
				"}\n"
				"\n"
				"vec4 ndc_to_window(const in vec4 coord)\n"
				"{\n"
				"	return fma(coord, viewport.xyxy, viewport.zwzw);\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	tc0.xy = in_pos.zw;\n"
				"	clip_rect = ndc_to_window(clip_to_ndc(clip_bounds)).xwzy; // Swap y1 and y2 due to flipped origin!\n"
				"	vec4 pos = vec4(clip_to_ndc(in_pos).xy, 0.5, 1.);\n"
				"	pos.xy = snap_to_grid(pos.xy);\n"
				"	gl_Position = (pos + pos) - 1.;\n"
				"}\n";

			fs_src =
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(binding=30) uniform sampler2DArray fs1;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=1) flat in vec4 clip_rect;\n"
				"layout(location=0) out vec4 ocol;\n"
				"uniform vec4 color;\n"
				"uniform float time;\n"
				"uniform int sampler_mode;\n"
				"uniform int pulse_glow;\n"
				"uniform int clip_region;\n"
				"uniform int blur_strength;\n"
				"\n"
				"vec4 blur_sample(sampler2D tex, vec2 coord, vec2 tex_offset)\n"
				"{\n"
				"	vec2 coords[9];\n"
				"	coords[0] = coord - tex_offset\n;"
				"	coords[1] = coord + vec2(0., -tex_offset.y);\n"
				"	coords[2] = coord + vec2(tex_offset.x, -tex_offset.y);\n"
				"	coords[3] = coord + vec2(-tex_offset.x, 0.);\n"
				"	coords[4] = coord;\n"
				"	coords[5] = coord + vec2(tex_offset.x, 0.);\n"
				"	coords[6] = coord + vec2(-tex_offset.x, tex_offset.y);\n"
				"	coords[7] = coord + vec2(0., tex_offset.y);\n"
				"	coords[8] = coord + tex_offset;\n"
				"\n"
				"	float weights[9] =\n"
				"	{\n"
				"		1., 2., 1.,\n"
				"		2., 4., 2.,\n"
				"		1., 2., 1.\n"
				"	};\n"
				"\n"
				"	vec4 blurred = vec4(0.);\n"
				"	for (int n = 0; n < 9; ++n)\n"
				"	{\n"
				"		blurred += texture(tex, coords[n]) * weights[n];\n"
				"	}\n"
				"\n"
				"	return blurred / 16.f;\n"
				"}\n"
				"\n"
				"vec4 sample_image(sampler2D tex, vec2 coord)\n"
				"{\n"
				"	vec4 original = texture(tex, coord);\n"
				"	if (blur_strength == 0) return original;\n"
				"	\n"
				"	vec2 constraints = 1.f / vec2(640, 360);\n"
				"	vec2 res_offset = 1.f / textureSize(fs0, 0);\n"
				"	vec2 tex_offset = max(res_offset, constraints);\n"
				"\n"
				"	// Sample triangle pattern and average\n"
				"	// TODO: Nicer looking gaussian blur with less sampling\n"
				"	vec4 blur0 = blur_sample(tex, coord + vec2(-res_offset.x, 0.), tex_offset);\n"
				"	vec4 blur1 = blur_sample(tex, coord + vec2(res_offset.x, 0.), tex_offset);\n"
				"	vec4 blur2 = blur_sample(tex, coord + vec2(0., res_offset.y), tex_offset);\n"
				"\n"
				"	vec4 blurred = blur0 + blur1 + blur2;\n"
				"	blurred /= 3.;\n"
				"	return mix(original, blurred, float(blur_strength) / 100.);\n"
				"}\n"
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
				"	switch (sampler_mode)\n"
				"	{\n"
				"	case 1:\n"
				"		ocol = sample_image(fs0, tc0) * diff_color;\n"
				"		break;\n"
				"	case 2:\n"
				"		ocol = texture(fs1, vec3(tc0.x, fract(tc0.y), trunc(tc0.y))) * diff_color;\n"
				"		break;\n"
				"	default:\n"
				"		ocol = diff_color;\n"
				"		break;\n"
				"	}\n"
				"}\n";

			// Smooth filtering required for inputs
			input_filter = GL_LINEAR;
		}

		gl::texture_view* load_simple_image(rsx::overlays::image_info* desc, bool temp_resource, u32 owner_uid)
		{
			auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D, desc->w, desc->h, 1, 1, GL_RGBA8);
			tex->copy_from(desc->data, gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

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
				u64 key = reinterpret_cast<u64>(desc);
				temp_image_cache[key] = std::make_pair(owner_uid, std::move(tex));
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
			for (const auto& temp_image : temp_image_cache)
			{
				if (temp_image.second.first == key)
				{
					keys_to_remove.push_back(temp_image.first);
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
			const auto font_size = font->get_glyph_data_dimensions();

			u64 key = reinterpret_cast<u64>(font);
			auto found = view_cache.find(key);
			if (found != view_cache.end())
			{
				if (const auto this_size = found->second->image()->size3D();
					font_size.width == this_size.width &&
					font_size.height == this_size.height &&
					font_size.depth == this_size.depth)
				{
					return found->second.get();
				}
			}

			// Create font file
			std::vector<u8> glyph_data;
			font->get_glyph_data(glyph_data);

			auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D_ARRAY, font_size.width, font_size.height, font_size.depth, 1, GL_R8);
			tex->copy_from(glyph_data.data(), gl::texture::format::r, gl::texture::type::ubyte, {});

			GLenum remap[] = { GL_RED, GL_RED, GL_RED, GL_RED };
			auto view = std::make_unique<gl::texture_view>(tex.get(), remap);

			auto result = view.get();
			font_cache[key] = std::move(tex);
			view_cache[key] = std::move(view);

			return result;
		}

		gl::texture_view* find_temp_image(rsx::overlays::image_info *desc, u32 owner_uid)
		{
			auto key = reinterpret_cast<u64>(desc);
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

		void set_primitive_type(rsx::overlays::primitive_type type)
		{
			m_current_primitive_type = type;

			switch (type)
			{
				case rsx::overlays::primitive_type::quad_list:
				case rsx::overlays::primitive_type::triangle_strip:
					primitives = GL_TRIANGLE_STRIP;
					break;
				case rsx::overlays::primitive_type::line_list:
					primitives = GL_LINES;
					break;
				case rsx::overlays::primitive_type::line_strip:
					primitives = GL_LINE_STRIP;
					break;
				default:
					fmt::throw_exception("Unexpected primitive type %d" HERE, static_cast<s32>(type));
			}
		}

		void emit_geometry() override
		{
			if (m_current_primitive_type == rsx::overlays::primitive_type::quad_list)
			{
				// Emulate quads with disjointed triangle strips
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
			else
			{
				overlay_pass::emit_geometry();
			}
		}

		void run(const areau& viewport, GLuint target, rsx::overlays::overlay& ui)
		{
			program_handle.uniforms["viewport"] = color4f(static_cast<f32>(viewport.width()), static_cast<f32>(viewport.height()), static_cast<f32>(viewport.x1), static_cast<f32>(viewport.y1));
			program_handle.uniforms["ui_scale"] = color4f(static_cast<f32>(ui.virtual_width), static_cast<f32>(ui.virtual_height), 1.f, 1.f);
			program_handle.uniforms["time"] = static_cast<f32>(get_system_time() / 1000) * 0.005f;

			saved_sampler_state save_30(30, m_sampler);
			saved_sampler_state save_31(31, m_sampler);

			for (auto &cmd : ui.get_compiled().draw_commands)
			{
				set_primitive_type(cmd.config.primitives);
				upload_vertex_data(reinterpret_cast<f32*>(cmd.verts.data()), ::size32(cmd.verts) * 4u);
				num_drawable_elements = ::size32(cmd.verts);
				GLint texture_read = GL_TRUE;

				switch (cmd.config.texture_ref)
				{
				case rsx::overlays::image_resource_id::game_icon:
				case rsx::overlays::image_resource_id::backbuffer:
					//TODO
				case rsx::overlays::image_resource_id::none:
				{
					texture_read = GL_FALSE;
					glBindTexture(GL_TEXTURE_2D, GL_NONE);
					break;
				}
				case rsx::overlays::image_resource_id::raw_image:
				{
					glBindTexture(GL_TEXTURE_2D, find_temp_image(static_cast<rsx::overlays::image_info*>(cmd.config.external_data_ref), ui.uid)->id());
					break;
				}
				case rsx::overlays::image_resource_id::font_file:
				{
					texture_read = (GL_TRUE + 1);
					glActiveTexture(GL_TEXTURE0 + 30);
					glBindTexture(GL_TEXTURE_2D_ARRAY, find_font(cmd.config.font_ref)->id());
					glActiveTexture(GL_TEXTURE0 + 31);
					break;
				}
				default:
				{
					glBindTexture(GL_TEXTURE_2D, view_cache[cmd.config.texture_ref - 1]->id());
					break;
				}
				}

				program_handle.uniforms["color"] = cmd.config.color;
				program_handle.uniforms["sampler_mode"] = texture_read;
				program_handle.uniforms["pulse_glow"] = static_cast<s32>(cmd.config.pulse_glow);
				program_handle.uniforms["blur_strength"] = static_cast<s32>(cmd.config.blur_strength);
				program_handle.uniforms["clip_region"] = static_cast<s32>(cmd.config.clip_region);
				program_handle.uniforms["clip_bounds"] = cmd.config.clip_rect;
				overlay_pass::run(viewport, target, false, true);
			}

			ui.update();
		}
	};

	struct video_out_calibration_pass : public overlay_pass
	{
		video_out_calibration_pass()
		{
			vs_src =
				"#version 420\n\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 1.), vec2(1., 1.), vec2(0., 0.), vec2(1., 0.)};\n"
				"	tc0 = coords[gl_VertexID % 4];\n"
				"	vec2 pos = positions[gl_VertexID % 4];\n"
				"	gl_Position = vec4(pos, 0., 1.);\n"
				"}\n";

			fs_src =
				"#version 420\n\n"
				"layout(binding=31) uniform sampler2D fs0;\n"
				"layout(binding=30) uniform sampler2D fs1;\n"
				"layout(location=0) in vec2 tc0;\n"
				"layout(location=0) out vec4 ocol;\n"
				"\n"
				"uniform float gamma;\n"
				"uniform int limit_range;\n"
				"uniform int stereo;\n"
				"uniform int stereo_image_count;\n"
				"\n"
				"vec4 read_source()\n"
				"{\n"
				"	if (stereo == 0) return texture(fs0, tc0);\n"
				"\n"
				"	vec4 left, right;\n"
				"	if (stereo_image_count == 2)\n"
				"	{\n"
				"		left = texture(fs0, tc0);\n"
				"		right = texture(fs1, tc0);\n"
				"	}\n"
				"	else\n"
				"	{\n"
				"		vec2 coord_left = tc0 * vec2(1.f, 0.4898f);\n"
				"		vec2 coord_right = coord_left + vec2(0.f, 0.510204f);\n"
				"		left = texture(fs0, coord_left);\n"
				"		right = texture(fs0, coord_right);\n"
				"	}\n"
				"\n"
				"	return vec4(left.r, right.g, right.b, 1.);\n"
				"}\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 color = read_source();\n"
				"	color.rgb = pow(color.rgb, vec3(gamma));\n"
				"	if (limit_range > 0)\n"
				"		ocol = ((color * 220.) + 16.) / 255.;\n"
				"	else\n"
				"		ocol = color;\n"
				"}\n";

			input_filter = GL_LINEAR;
		}

		void run(const areau& viewport, const rsx::simple_array<GLuint>& source, f32 gamma, bool limited_rgb, bool _3d)
		{
			program_handle.uniforms["gamma"] = gamma;
			program_handle.uniforms["limit_range"] = limited_rgb + 0;
			program_handle.uniforms["stereo"] = _3d + 0;
			program_handle.uniforms["stereo_image_count"] = (source[1] == GL_NONE? 1 : 2);

			saved_sampler_state saved(31, m_sampler);
			glBindTexture(GL_TEXTURE_2D, source[0]);

			saved_sampler_state saved2(30, m_sampler);
			glBindTexture(GL_TEXTURE_2D, source[1]);

			overlay_pass::run(viewport, GL_NONE, false, false);
		}
	};
}
