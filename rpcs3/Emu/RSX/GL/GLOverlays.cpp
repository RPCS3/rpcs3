#include "GLOverlays.h"

#include "Utilities/StrUtil.h"
#include "../Program/RSXOverlay.h"
#include "Emu/Cell/timers.hpp"

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
			ensure(!fs_src.empty() && !vs_src.empty(), "Shaders have not been initialized.");

			fs.create(::glsl::program_domain::glsl_fragment_program, fs_src);
			fs.compile();

			vs.create(::glsl::program_domain::glsl_vertex_program, vs_src);
			vs.compile();

			program_handle.create();
			program_handle.attach(vs);
			program_handle.attach(fs);
			program_handle.link();

			ensure(program_handle.id());

			fbo.create();

			m_sampler.create();
			m_sampler.apply_defaults(static_cast<GLenum>(m_input_filter));

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

	void overlay_pass::emit_geometry(gl::command_context& /*cmd*/)
	{
		int old_vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

		m_vao.bind();
		glDrawArrays(primitives, 0, num_drawable_elements);

		glBindVertexArray(old_vao);
	}

	void overlay_pass::run(gl::command_context& cmd, const areau& region, GLuint target_texture, GLuint image_aspect_bits, bool enable_blending)
	{
		ensure(compiled && program_handle.id() != GL_NONE, "You must initialize overlay passes with create() before calling run()");

		GLint viewport[4];
		std::unique_ptr<fbo::save_binding_state> save_fbo;

		if (target_texture)
		{
			save_fbo = std::make_unique<fbo::save_binding_state>(fbo);

			switch (image_aspect_bits)
			{
			case gl::image_aspect::color:
				fbo.color[0] = target_texture;
				fbo.draw_buffer(fbo.color[0]);
				break;
			case gl::image_aspect::depth:
				fbo.draw_buffer(fbo.no_color);
				fbo.depth = target_texture;
				break;
			case gl::image_aspect::stencil:
				fbo.draw_buffer(fbo.no_color);
				fbo.depth_stencil = target_texture;
				break;
			case gl::image_aspect::depth | gl::image_aspect::stencil:
				fbo.draw_buffer(fbo.no_color);
				fbo.depth_stencil = target_texture;
				break;
			default:
				fmt::throw_exception("Unsupported image aspect combination 0x%x", image_aspect_bits);
			}

			enable_depth_writes = (image_aspect_bits & m_write_aspect_mask) & gl::image_aspect::depth;
			enable_stencil_writes = (image_aspect_bits & m_write_aspect_mask) & gl::image_aspect::stencil;
		}

		if (!target_texture || fbo.check())
		{
			// Save state (TODO)
			glGetIntegerv(GL_VIEWPORT, viewport);

			// Set initial state
			glViewport(region.x1, region.y1, region.width(), region.height());
			cmd->color_maski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			cmd->depth_mask(image_aspect_bits == gl::image_aspect::color ? GL_FALSE : GL_TRUE);

			cmd->disable(GL_CULL_FACE);
			cmd->disable(GL_SCISSOR_TEST);
			cmd->clip_planes(GL_NONE);

			if (enable_depth_writes)
			{
				// Disabling depth test will also disable depth writes which is not desired
				cmd->depth_func(GL_ALWAYS);
				cmd->enable(GL_DEPTH_TEST);
			}
			else
			{
				cmd->disable(GL_DEPTH_TEST);
			}

			if (enable_stencil_writes)
			{
				// Disabling stencil test also disables stencil writes.
				cmd->enable(GL_STENCIL_TEST);
				cmd->stencil_mask(0xFF);
				cmd->stencil_func(GL_ALWAYS, 0xFF, 0xFF);
				cmd->stencil_op(GL_KEEP, GL_KEEP, GL_REPLACE);
			}
			else
			{
				cmd->disable(GL_STENCIL_TEST);
			}

			if (enable_blending)
			{
				cmd->enablei(GL_BLEND, 0);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
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
			emit_geometry(cmd);

			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

			if (target_texture)
			{
				fbo.color[0] = GL_NONE;
				fbo.depth = GL_NONE;
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
		#include "../Program/GLSLSnippets/OverlayRenderVS.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/OverlayRenderFS.glsl"
		;

		vs_src = fmt::replace_all(vs_src,
		{
			{ "#version 450", "#version 420" },
			{ "%preprocessor", "// %preprocessor" }
		});
		fs_src = fmt::replace_all(fs_src, "%preprocessor", "// %preprocessor");

		// Smooth filtering required for inputs
		m_input_filter = gl::filter::linear;
	}

	gl::texture_view* ui_overlay_renderer::load_simple_image(rsx::overlays::image_info_base* desc, bool temp_resource, u32 owner_uid)
	{
		auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D, desc->w, desc->h, 1, 1, 1, GL_RGBA8, RSX_FORMAT_CLASS_COLOR);
		tex->copy_from(desc->get_data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		const GLenum remap[] = { GL_RED, GL_ALPHA, GL_BLUE, GL_GREEN };
		auto view = std::make_unique<gl::texture_view>(tex.get(), remap);

		auto result = view.get();
		if (!temp_resource)
		{
			resources.push_back(std::move(tex));
			view_cache[view_cache.size()] = std::move(view);
		}
		else
		{
			const u64 key = reinterpret_cast<u64>(desc);
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

		for (const auto& res : configuration.texture_raw_data)
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
		const std::vector<u8>& glyph_data = font->get_glyph_data();

		auto tex = std::make_unique<gl::texture>(GL_TEXTURE_2D_ARRAY, font_size.width, font_size.height, font_size.depth, 1, 1, GL_R8, RSX_FORMAT_CLASS_COLOR);
		tex->copy_from(glyph_data.data(), gl::texture::format::r, gl::texture::type::ubyte, {});

		GLenum remap[] = { GL_RED, GL_RED, GL_RED, GL_RED };
		auto view = std::make_unique<gl::texture_view>(tex.get(), remap);

		auto result = view.get();
		font_cache[key] = std::move(tex);
		view_cache[key] = std::move(view);

		return result;
	}

	gl::texture_view* ui_overlay_renderer::find_temp_image(rsx::overlays::image_info_base* desc, u32 owner_uid)
	{
		const bool dirty = std::exchange(desc->dirty, false);
		const u64 key = reinterpret_cast<u64>(desc);

		auto cached = temp_view_cache.find(key);
		if (cached != temp_view_cache.end())
		{
			gl::texture_view* view = cached->second.get();

			if (dirty)
			{
				view->image()->copy_from(desc->get_data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});
			}

			return view;
		}

		return load_simple_image(desc, true, owner_uid);
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
			case rsx::overlays::primitive_type::triangle_fan:
				primitives = GL_TRIANGLE_FAN;
				break;
			default:
				fmt::throw_exception("Unexpected primitive type %d", static_cast<s32>(type));
		}
	}

	void ui_overlay_renderer::emit_geometry(gl::command_context& cmd)
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
			overlay_pass::emit_geometry(cmd);
		}
	}

	void ui_overlay_renderer::run(gl::command_context& cmd_, const areau& viewport, GLuint target, rsx::overlays::overlay& ui)
	{
		program_handle.uniforms["viewport"] = color4f(static_cast<f32>(viewport.width()), static_cast<f32>(viewport.height()), static_cast<f32>(viewport.x1), static_cast<f32>(viewport.y1));
		program_handle.uniforms["ui_scale"] = color4f(static_cast<f32>(ui.virtual_width), static_cast<f32>(ui.virtual_height), 1.f, 1.f);

		saved_sampler_state save_30(30, m_sampler);
		saved_sampler_state save_31(31, m_sampler);

		if (ui.status_flags & rsx::overlays::status_bits::invalidate_image_cache)
		{
			remove_temp_resources(ui.uid);
			ui.status_flags.clear(rsx::overlays::status_bits::invalidate_image_cache);
		}

		for (auto& cmd : ui.get_compiled().draw_commands)
		{
			set_primitive_type(cmd.config.primitives);
			upload_vertex_data(cmd.verts.data(), ::size32(cmd.verts));
			num_drawable_elements = ::size32(cmd.verts);
			auto texture_mode = rsx::overlays::texture_sampling_mode::texture2D;

			switch (cmd.config.texture_ref)
			{
			case rsx::overlays::image_resource_id::game_icon:
			case rsx::overlays::image_resource_id::backbuffer:
				// TODO
			case rsx::overlays::image_resource_id::none:
			{
				texture_mode = rsx::overlays::texture_sampling_mode::none;
				cmd_->bind_texture(31, GL_TEXTURE_2D, GL_NONE);
				break;
			}
			case rsx::overlays::image_resource_id::raw_image:
			{
				cmd_->bind_texture(31, GL_TEXTURE_2D, find_temp_image(static_cast<rsx::overlays::image_info_base*>(cmd.config.external_data_ref), ui.uid)->id());
				break;
			}
			case rsx::overlays::image_resource_id::font_file:
			{
				texture_mode = rsx::overlays::texture_sampling_mode::font3D;
				cmd_->bind_texture(30, GL_TEXTURE_2D_ARRAY, find_font(cmd.config.font_ref)->id());
				break;
			}
			default:
			{
				cmd_->bind_texture(31, GL_TEXTURE_2D, view_cache[cmd.config.texture_ref - 1]->id());
				break;
			}
			}

			rsx::overlays::vertex_options vert_opts;
			program_handle.uniforms["vertex_config"] = vert_opts
				.disable_vertex_snap(cmd.config.disable_vertex_snap)
				.get();

			rsx::overlays::fragment_options draw_opts;
			program_handle.uniforms["fragment_config"] = draw_opts
				.texture_mode(texture_mode)
				.clip_fragments(cmd.config.clip_region)
				.pulse_glow(cmd.config.pulse_glow)
				.get();

			program_handle.uniforms["timestamp"] = cmd.config.get_sinus_value();
			program_handle.uniforms["albedo"] = cmd.config.color;
			program_handle.uniforms["clip_bounds"] = cmd.config.clip_rect;
			program_handle.uniforms["blur_intensity"] = static_cast<f32>(cmd.config.blur_strength);
			overlay_pass::run(cmd_, viewport, target, gl::image_aspect::color, true);
		}

		ui.update(get_system_time());
	}

	video_out_calibration_pass::video_out_calibration_pass()
	{
		vs_src =
		#include "../Program/GLSLSnippets/GenericVSPassthrough.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/VideoOutCalibrationPass.glsl"
		;

		std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%sampler_binding", fmt::format("(%d - x)", GL_TEMP_IMAGE_SLOT(0)) },
			{ "%set_decorator, ", "" },
		};
		fs_src = fmt::replace_all(fs_src, repl_list);

		m_input_filter = gl::filter::linear;
	}

	void video_out_calibration_pass::run(gl::command_context& cmd, const areau& viewport, const rsx::simple_array<GLuint>& source, f32 gamma, bool limited_rgb, stereo_render_mode_options stereo_mode, gl::filter input_filter)
	{
		if (m_input_filter != input_filter)
		{
			m_input_filter = input_filter;
			m_sampler.set_parameteri(GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(m_input_filter));
			m_sampler.set_parameteri(GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(m_input_filter));
		}

		program_handle.uniforms["gamma"] = gamma;
		program_handle.uniforms["limit_range"] = limited_rgb + 0;
		program_handle.uniforms["stereo_display_mode"] = static_cast<u8>(stereo_mode);
		program_handle.uniforms["stereo_image_count"] = (source[1] == GL_NONE? 1 : 2);

		saved_sampler_state saved(GL_TEMP_IMAGE_SLOT(0), m_sampler);
		cmd->bind_texture(GL_TEMP_IMAGE_SLOT(0), GL_TEXTURE_2D, source[0]);

		saved_sampler_state saved2(GL_TEMP_IMAGE_SLOT(1), m_sampler);
		cmd->bind_texture(GL_TEMP_IMAGE_SLOT(1), GL_TEXTURE_2D, source[1]);

		overlay_pass::run(cmd, viewport, GL_NONE, gl::image_aspect::color, false);
	}

	rp_ssbo_to_generic_texture::rp_ssbo_to_generic_texture()
	{
		vs_src =
		#include "../Program/GLSLSnippets/GenericVSPassthrough.glsl"
		;

		fs_src =
		#include "../Program/GLSLSnippets/CopyBufferToGenericImage.glsl"
		;

		const auto& caps = gl::get_driver_caps();
		const bool stencil_export_supported = caps.ARB_shader_stencil_export_supported;
		const bool legacy_format_support = caps.subvendor_ATI;

		std::pair<std::string_view, std::string> repl_list[] =
		{
			{ "%set, ", "" },
			{ "%loc", std::to_string(GL_COMPUTE_BUFFER_SLOT(0)) },
			{ "%push_block", fmt::format("binding=%d, std140", GL_COMPUTE_BUFFER_SLOT(1)) },
			{ "%stencil_export_supported", stencil_export_supported ? "1" : "0" },
			{ "%legacy_format_support", legacy_format_support ? "1" : "0" }
		};

		fs_src = fmt::replace_all(fs_src, repl_list);

		if (stencil_export_supported)
		{
			m_write_aspect_mask |= gl::image_aspect::stencil;
		}
	}

	void rp_ssbo_to_generic_texture::run(gl::command_context& cmd,
		const buffer* src, const texture_view* dst,
		const u32 src_offset, const coordu& dst_region,
		const pixel_buffer_layout& layout)
	{
		const u32 bpp = dst->image()->pitch() / dst->image()->width();
		const u32 row_length = utils::align(dst_region.width * bpp, std::max<int>(layout.alignment, 1)) / bpp;

		program_handle.uniforms["src_pitch"] = row_length;
		program_handle.uniforms["swap_bytes"] = layout.swap_bytes;
		program_handle.uniforms["format"] = static_cast<GLenum>(dst->image()->get_internal_format());
		src->bind_range(gl::buffer::target::ssbo, GL_COMPUTE_BUFFER_SLOT(0), src_offset, row_length * bpp * dst_region.height);

		cmd->stencil_mask(0xFF);

		overlay_pass::run(cmd, dst_region, dst->id(), dst->aspect());
	}

	void rp_ssbo_to_generic_texture::run(gl::command_context& cmd,
		const buffer* src, texture* dst,
		const u32 src_offset, const coordu& dst_region,
		const pixel_buffer_layout& layout)
	{
		gl::nil_texture_view view(dst);
		run(cmd, src, &view, src_offset, dst_region, layout);
	}
}
