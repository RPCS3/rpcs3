#include "GLOverlays.h"

namespace gl
{
	// Lame
	std::unordered_map<u32, std::unique_ptr<gl::overlay_pass>> g_overlay_passes;

	void destroy_overlay_passes()
	{
		for (auto& [key, prog] : g_overlay_passes)
		{
			prog->destroy();
		}

		g_overlay_passes.clear();
	}

	void overlay_pass::create()
	{
		if (!compiled)
		{
			fs.create(::glsl::program_domain::glsl_fragment_program, fs_src);
			fs.compile();

			vs.create(::glsl::program_domain::glsl_vertex_program, vs_src);
			vs.compile();

			program_handle.create();
			program_handle.attach(vs);
			program_handle.attach(fs);
			program_handle.link();

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

	void overlay_pass::destroy()
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

	void overlay_pass::emit_geometry()
	{
		int old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		m_vao.bind();
		glDrawArrays(primitives, 0, num_drawable_elements);

		glBindVertexArray(old_vao);
	}

	void overlay_pass::run(gl::command_context& cmd, const areau& region, GLuint target_texture, bool depth_target, bool use_blending)
	{
		if (!compiled)
		{
			rsx_log.error("You must initialize overlay passes with create() before calling run()");
			return;
		}

		GLint viewport[4];
		std::unique_ptr<fbo::save_binding_state> save_fbo;

		if (target_texture)
		{
			save_fbo = std::make_unique<fbo::save_binding_state>(fbo);

			if (depth_target)
			{
				fbo.draw_buffer(fbo.no_color);
				fbo.depth_stencil = target_texture;
			}
			else
			{
				fbo.color[0] = target_texture;
				fbo.draw_buffer(fbo.color[0]);
			}
		}

		if (!target_texture || fbo.check())
		{
			// Save state (TODO)
			glGetIntegerv(GL_VIEWPORT, viewport);

			// Set initial state
			glViewport(region.x1, region.y1, region.width(), region.height());
			cmd->color_maski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			cmd->depth_mask(depth_target ? GL_TRUE : GL_FALSE);

			// Disabling depth test will also disable depth writes which is not desired
			cmd->depth_func(GL_ALWAYS);
			cmd->enable(GL_DEPTH_TEST);

			cmd->disable(GL_SCISSOR_TEST);
			cmd->disable(GL_CULL_FACE);
			cmd->disable(GL_STENCIL_TEST);

			if (use_blending)
			{
				cmd->enablei(GL_BLEND, 0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBlendEquation(GL_FUNC_ADD);
			}
			else
			{
				cmd->disablei(GL_BLEND, 0);
			}

			// Render
			cmd->use_program(program_handle.id());
			on_load();
			bind_resources();
			emit_geometry();

			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

			if (target_texture)
			{
				fbo.color[0] = GL_NONE;
				fbo.depth_stencil = GL_NONE;
			}
		}
		else
		{
			rsx_log.error("Overlay pass failed because framebuffer was not complete. Run with debug output enabled to diagnose the problem");
		}
	}

	ui_overlay_renderer::ui_overlay_renderer()
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

	gl::texture_view* ui_overlay_renderer::load_simple_image(rsx::overlays::image_info* desc, bool temp_resource, u32 owner_uid)
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

	void ui_overlay_renderer::create()
	{
		overlay_pass::create();

		rsx::overlays::resource_config configuration;
		configuration.load_files();

		for (const auto &res : configuration.texture_raw_data)
		{
			load_simple_image(res.get(), false, -1);
		}

		configuration.free_resources();
	}

	void ui_overlay_renderer::destroy()
	{
		temp_image_cache.clear();
		temp_view_cache.clear();
		resources.clear();
		font_cache.clear();
		view_cache.clear();
		overlay_pass::destroy();
	}

	void ui_overlay_renderer::remove_temp_resources(u64 key)
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

	gl::texture_view* ui_overlay_renderer::find_font(rsx::overlays::font* font)
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

	gl::texture_view* ui_overlay_renderer::find_temp_image(rsx::overlays::image_info* desc, u32 owner_uid)
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

	void ui_overlay_renderer::set_primitive_type(rsx::overlays::primitive_type type)
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
				fmt::throw_exception("Unexpected primitive type %d", static_cast<s32>(type));
		}
	}

	void ui_overlay_renderer::emit_geometry()
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

	void ui_overlay_renderer::run(gl::command_context& cmd_, const areau& viewport, GLuint target, rsx::overlays::overlay& ui)
	{
		program_handle.uniforms["viewport"] = color4f(static_cast<f32>(viewport.width()), static_cast<f32>(viewport.height()), static_cast<f32>(viewport.x1), static_cast<f32>(viewport.y1));
		program_handle.uniforms["ui_scale"] = color4f(static_cast<f32>(ui.virtual_width), static_cast<f32>(ui.virtual_height), 1.f, 1.f);

		saved_sampler_state save_30(30, m_sampler);
		saved_sampler_state save_31(31, m_sampler);

		for (auto& cmd : ui.get_compiled().draw_commands)
		{
			set_primitive_type(cmd.config.primitives);
			upload_vertex_data(cmd.verts.data(), ::size32(cmd.verts));
			num_drawable_elements = ::size32(cmd.verts);
			GLint texture_read = GL_TRUE;

			switch (cmd.config.texture_ref)
			{
			case rsx::overlays::image_resource_id::game_icon:
			case rsx::overlays::image_resource_id::backbuffer:
				// TODO
			case rsx::overlays::image_resource_id::none:
			{
				texture_read = GL_FALSE;
				cmd_->bind_texture(31, GL_TEXTURE_2D, GL_NONE);
				break;
			}
			case rsx::overlays::image_resource_id::raw_image:
			{
				cmd_->bind_texture(31, GL_TEXTURE_2D, find_temp_image(static_cast<rsx::overlays::image_info*>(cmd.config.external_data_ref), ui.uid)->id());
				break;
			}
			case rsx::overlays::image_resource_id::font_file:
			{
				texture_read = (GL_TRUE + 1);
				cmd_->bind_texture(30, GL_TEXTURE_2D_ARRAY, find_font(cmd.config.font_ref)->id());
				break;
			}
			default:
			{
				cmd_->bind_texture(31, GL_TEXTURE_2D, view_cache[cmd.config.texture_ref - 1]->id());
				break;
			}
			}

			program_handle.uniforms["time"] = cmd.config.get_sinus_value();
			program_handle.uniforms["color"] = cmd.config.color;
			program_handle.uniforms["sampler_mode"] = texture_read;
			program_handle.uniforms["pulse_glow"] = static_cast<s32>(cmd.config.pulse_glow);
			program_handle.uniforms["blur_strength"] = static_cast<s32>(cmd.config.blur_strength);
			program_handle.uniforms["clip_region"] = static_cast<s32>(cmd.config.clip_region);
			program_handle.uniforms["clip_bounds"] = cmd.config.clip_rect;
			overlay_pass::run(cmd_, viewport, target, false, true);
		}

		ui.update();
	}

	video_out_calibration_pass::video_out_calibration_pass()
	{
		vs_src =
		#include "../Program/GLSLSnippets/GenericVSPassthrough.glsl"
		;

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

	void video_out_calibration_pass::run(gl::command_context& cmd, const areau& viewport, const rsx::simple_array<GLuint>& source, f32 gamma, bool limited_rgb, bool _3d)
	{
		program_handle.uniforms["gamma"] = gamma;
		program_handle.uniforms["limit_range"] = limited_rgb + 0;
		program_handle.uniforms["stereo"] = _3d + 0;
		program_handle.uniforms["stereo_image_count"] = (source[1] == GL_NONE? 1 : 2);

		saved_sampler_state saved(31, m_sampler);
		cmd->bind_texture(31, GL_TEXTURE_2D, source[0]);

		saved_sampler_state saved2(30, m_sampler);
		cmd->bind_texture(30, GL_TEXTURE_2D, source[1]);

		overlay_pass::run(cmd, viewport, GL_NONE, false, false);
	}

	rp_ssbo_to_d24x8_texture::rp_ssbo_to_d24x8_texture()
	{
		vs_src =
		#include "../Program/GLSLSnippets/GenericVSPassthrough.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/CopyBufferToD24x8.glsl"
		;

		std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%set, ", "" },
			{ "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%push_block", fmt::format("binding=%d, std140", GL_COMPUTE_BUFFER_SLOT(1)) }
		};

		fs_src = fmt::replace_all(fs_src, repl_list);
	}

	void rp_ssbo_to_d24x8_texture::run(gl::command_context& cmd,
		const buffer* src, const texture* dst,
		const u32 src_offset, const coordu& dst_region,
		const pixel_unpack_settings& settings)
	{
		const u32 row_length = settings.get_row_length() ? settings.get_row_length() : static_cast<u32>(dst_region.width);
		program_handle.uniforms["src_pitch"] = row_length;
		program_handle.uniforms["swap_bytes"] = settings.get_swap_bytes();
		src->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), src_offset, row_length * 4 * dst_region.height);

		cmd->stencil_mask(0xFF);

		overlay_pass::run(cmd, dst_region, dst->id(), true);
	}
}
