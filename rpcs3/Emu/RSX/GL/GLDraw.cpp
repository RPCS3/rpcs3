#include "stdafx.h"
#include "GLGSRender.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "../Program/GLSLCommon.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace gl
{
	extern GLenum tex_min_filter(rsx::texture_minify_filter min_filter);

	inline GLenum comparison_op(rsx::comparison_function op)
	{
		return static_cast<GLenum>(op);
	}

	inline GLenum stencil_op(rsx::stencil_op op)
	{
		return static_cast<GLenum>(op);
	}

	GLenum blend_equation(rsx::blend_equation op)
	{
		switch (op)
		{
			// Note : maybe add is signed on gl
		case rsx::blend_equation::add_signed:
			rsx_log.trace("blend equation add_signed used. Emulating using FUNC_ADD");
			[[fallthrough]];
		case rsx::blend_equation::add: return GL_FUNC_ADD;
		case rsx::blend_equation::min: return GL_MIN;
		case rsx::blend_equation::max: return GL_MAX;
		case rsx::blend_equation::subtract: return GL_FUNC_SUBTRACT;
		case rsx::blend_equation::reverse_subtract_signed:
			rsx_log.trace("blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
			[[fallthrough]];
		case rsx::blend_equation::reverse_subtract: return GL_FUNC_REVERSE_SUBTRACT;
		case rsx::blend_equation::reverse_add_signed:
		default:
			rsx_log.error("Blend equation 0x%X is unimplemented!", static_cast<u32>(op));
			return GL_FUNC_ADD;
		}
	}

	inline GLenum blend_factor(rsx::blend_factor op)
	{
		return static_cast<GLenum>(op);
	}

	inline GLenum logic_op(rsx::logic_op op)
	{
		return static_cast<GLenum>(op);
	}

	inline GLenum front_face(rsx::front_face op)
	{
		// NOTE: RSX face winding is always based off of upper-left corner like vulkan, but GL is bottom left
		// shader_window_origin register does not affect this
		// verified with Outrun Online Arcade (window_origin::top) and DS2 (window_origin::bottom)
		// correctness of face winding checked using stencil test (GOW collection shadows)
		return static_cast<GLenum>(op) ^ 1u;
	}

	inline GLenum cull_face(rsx::cull_face op)
	{
		return static_cast<GLenum>(op);
	}

	inline GLenum polygon_mode(rsx::polygon_mode mode)
	{
		return static_cast<GLenum>(mode);
	}
}

void GLGSRender::update_draw_state()
{
	m_profiler.start();

	const auto& raster = m_ctx->draw_state.raster;
	const auto& depth_stencil = m_ctx->draw_state.depth_stencil;
	const auto& blend = m_ctx->draw_state.blend;
	const auto& multisample = m_ctx->draw_state.multisample;

	gl_state.enable(GL_SCISSOR_TEST);
	gl_state.enable(blend.dither_enabled, GL_DITHER);

	if (m_rtts.m_bound_depth_stencil.first)
	{
		// Z-buffer is active.
		gl_state.depth_mask(depth_stencil.depth_write_enabled);
		const bool emulate_depth_range = requires_depth_range_emulation();
		if (!emulate_depth_range) [[ likely ]]
		{
			gl_state.enable(depth_stencil.depth_clamp_enabled || !depth_stencil.depth_clip_enabled, GL_DEPTH_CLAMP);
		}
		else
		{
			// Depth clip and clamp are emulated in the fragment shader
			gl_state.enable(GL_DEPTH_CLAMP);
		}

		if (gl_state.enable(depth_stencil.depth_test_enabled, GL_DEPTH_TEST))
		{
			gl_state.depth_func(gl::comparison_op(depth_stencil.depth_func));
		}

		if (gl::get_driver_caps().EXT_depth_bounds_test_supported &&
			(gl_state.enable(depth_stencil.depth_bounds_test_enabled, GL_DEPTH_BOUNDS_TEST_EXT)))
		{
			f32 bounds_min = depth_stencil.depth_bounds_min;
			f32 bounds_max = depth_stencil.depth_bounds_max;

			if (emulate_depth_range) [[ unlikely ]]
			{
				bounds_min = rsx::encode_emulated_depth(bounds_min);
				bounds_max = rsx::encode_emulated_depth(bounds_max);
			}

			gl_state.depth_bounds(bounds_min, bounds_max);
		}

		if (gl::get_driver_caps().NV_depth_buffer_float_supported)
		{
			gl_state.depth_range(m_ctx->draw_state.viewport.z_clip_min, m_ctx->draw_state.viewport.z_clip_max);
		}

		if (gl_state.enable(depth_stencil.stencil_test_enabled, GL_STENCIL_TEST))
		{
			gl_state.stencil_front_mask(depth_stencil.front.write_mask);

			gl_state.stencil_front_func(
				gl::comparison_op(depth_stencil.front.func),
				depth_stencil.front.func_ref,
				depth_stencil.front.func_mask);

			gl_state.stencil_front_op(
				gl::stencil_op(depth_stencil.front.op_fail),
				gl::stencil_op(depth_stencil.front.op_zfail),
				gl::stencil_op(depth_stencil.front.op_zpass));

			if (depth_stencil.two_sided_stencil_test_enabled) [[ unlikely ]]
			{
				gl_state.stencil_back_mask(depth_stencil.back.write_mask);

				gl_state.stencil_back_func(gl::comparison_op(depth_stencil.back.func),
					depth_stencil.back.func_ref, depth_stencil.back.func_mask);

				gl_state.stencil_back_op(gl::stencil_op(depth_stencil.back.op_fail),
					gl::stencil_op(depth_stencil.back.op_zfail), gl::stencil_op(depth_stencil.back.op_zpass));
			}
			else
			{
				// Reuse front values
				gl_state.stencil_back_mask(depth_stencil.front.write_mask);

				gl_state.stencil_back_func(
					gl::comparison_op(depth_stencil.front.func),
					depth_stencil.front.func_ref,
					depth_stencil.front.func_mask);

				gl_state.stencil_back_op(
					gl::stencil_op(depth_stencil.front.op_fail),
					gl::stencil_op(depth_stencil.front.op_zfail),
					gl::stencil_op(depth_stencil.front.op_zpass));
			}
		}
	}

	if (m_rtts.get_color_surface_count())
	{
		// Color buffer is active
		for (int index = 0; index < m_rtts.get_color_surface_count(); ++index)
		{
			const auto& color_mask = blend.color_mask[index];
			gl_state.color_maski(index, color_mask.r, color_mask.g, color_mask.b, color_mask.a);
		}

		// LogicOp and Blend are mutually exclusive. If both are enabled, LogicOp takes precedence.
		// In OpenGL, this behavior is enforced in spec, but let's enforce it at renderer level as well.
		if (gl_state.enable(blend.logic_op_enabled, GL_COLOR_LOGIC_OP))
		{
			gl_state.logic_op(gl::logic_op(blend.logic_operation));

			gl_state.enablei(GL_FALSE, GL_BLEND, 0);
			gl_state.enablei(GL_FALSE, GL_BLEND, 1);
			gl_state.enablei(GL_FALSE, GL_BLEND, 2);
			gl_state.enablei(GL_FALSE, GL_BLEND, 3);
		}
		else
		{
			const bool mrt_blend_enabled[] =
			{
				!!(blend.enable_mask & 1),
				!!(blend.enable_mask & 2),
				!!(blend.enable_mask & 4),
				!!(blend.enable_mask & 8)
			};

			if (blend.enable_mask)
			{
				glBlendFuncSeparate(gl::blend_factor(blend.sfactor_rgb),
					gl::blend_factor(blend.dfactor_rgb),
					gl::blend_factor(blend.sfactor_a),
					gl::blend_factor(blend.dfactor_a));

				const auto& blend_colors = blend.constant_color;
				glBlendColor(blend_colors[0], blend_colors[1], blend_colors[2], blend_colors[3]);

				glBlendEquationSeparate(gl::blend_equation(blend.equation_rgb),
					gl::blend_equation(blend.equation_a));
			}

			gl_state.enablei(mrt_blend_enabled[0], GL_BLEND, 0);
			gl_state.enablei(mrt_blend_enabled[1], GL_BLEND, 1);
			gl_state.enablei(mrt_blend_enabled[2], GL_BLEND, 2);
			gl_state.enablei(mrt_blend_enabled[3], GL_BLEND, 3);
		}

		// Antialias control
		if (m_config.supports_hw_msaa)
		{
			gl_state.enable(/*REGS(m_ctx)->msaa_enabled()*/GL_MULTISAMPLE);

			gl_state.enable(GL_SAMPLE_MASK);
			gl_state.sample_mask(multisample.sample_mask);

			gl_state.enable(GL_SAMPLE_SHADING);
			gl_state.min_sample_shading_rate(1.f);

			gl_state.enable(GL_SAMPLE_COVERAGE);
			gl_state.sample_coverage(1.f);
		}

		if (m_config.supports_hw_a2c)
		{
			const bool hw_enable = m_config.supports_hw_a2c_1spp || multisample.antialias != rsx::surface_antialiasing::center_1_sample;
			gl_state.enable(hw_enable && multisample.alpha_to_coverage_enabled, GL_SAMPLE_ALPHA_TO_COVERAGE);
		}

		if (m_config.supports_hw_a2one)
		{
			gl_state.enable(multisample.alpha_to_one_enabled, GL_SAMPLE_ALPHA_TO_ONE);
		}
	}

	switch (REGS(m_ctx)->current_draw_clause.primitive)
	{
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::line_strip:
		gl_state.line_width(raster.line_width * resolution_scaling_config.scale_factor());
		gl_state.enable(raster.line_smooth_enabled, GL_LINE_SMOOTH);
		break;
	default:
		gl_state.enable(raster.poly_offset_point_enabled, GL_POLYGON_OFFSET_POINT);
		gl_state.enable(raster.poly_offset_line_enabled, GL_POLYGON_OFFSET_LINE);
		gl_state.enable(raster.poly_offset_fill_enabled, GL_POLYGON_OFFSET_FILL);

		// offset_bias is the constant factor, multiplied by the implementation factor R
		// offset_scale is the slope factor, multiplied by the triangle slope factor M
		const auto poly_offset_scale = raster.poly_offset_scale;
		auto poly_offset_bias = raster.poly_offset_bias;

		if (auto ds = m_rtts.m_bound_depth_stencil.second;
			ds && ds->get_internal_format() == gl::texture::internal_format::depth24_stencil8)
		{
			// Check details in VKDraw.cpp about behaviour of RSX vs desktop D24X8 implementations
			// TLDR, RSX expects R = 16,777,215 (2^24 - 1)
			const auto& caps = gl::get_driver_caps();
			if (caps.vendor_NVIDIA || caps.vendor_MESA)
			{
				// R derived to be 8388607 (2^23 - 1)
				poly_offset_bias *= 0.5f;
			}
			else if (caps.vendor_AMD)
			{
				// R derived to be 4194303 (2^22 - 1)
				poly_offset_bias *= 0.25f;
			}
		}
		gl_state.polygon_offset(poly_offset_scale, poly_offset_bias);

		if (gl_state.enable(raster.cull_face_enabled, GL_CULL_FACE))
		{
			gl_state.cull_face(gl::cull_face(raster.cull_face_mode));
		}

		gl_state.front_face(gl::front_face(raster.front_face_mode));
		break;
	}

	// Clip planes
	gl_state.clip_planes((m_ctx->program.vertex_program.output_mask >> CELL_GCM_ATTRIB_OUTPUT_UC0) & 0x3F);

	// Polygon mode. We can only have one polygon mode active at one time, so we need to determine if any face is currently culled.
	const bool show_back = raster.cull_face_enabled && raster.cull_face_mode == rsx::cull_face::front;
	gl_state.polygon_mode(gl::polygon_mode(show_back ? raster.polygon_mode_back : raster.polygon_mode_front));

	//TODO
	//NV4097_SET_ANISO_SPREAD
	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG
	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	m_ctx->stats.setup_time += m_profiler.duration();
}

rsx::flags32_t GLGSRender::load_texture_env()
{
	// Load textures
	rsx::flags32_t invalidated_programs = 0;
	gl::command_context cmd{ gl_state };
	std::lock_guard lock(m_sampler_mutex);

	for (u32 textures_ref = m_ctx->program.fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
		{
			continue;
		}

		if (!m_ctx->textures.fs_sampler_state[i])
		{
			m_ctx->textures.fs_sampler_state[i] = std::make_unique<gl::texture_cache::sampled_image_descriptor>();
		}

		auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(m_ctx->textures.fs_sampler_state[i].get());
		const auto& tex = REGS(m_ctx)->fragment_textures[i];
		const auto previous_format_class = sampler_state->format_class;

		if (!m_samplers_dirty &&
			!(m_ctx->textures.fragment_textures_dirty & (1u << i)) &&
			!m_gl_texture_cache.test_if_descriptor_expired(cmd, m_rtts, sampler_state, tex))
		{
			continue;
		}

		const bool is_sampler_dirty = !!(m_ctx->textures.fragment_textures_dirty & (1u << i));

		if (!tex.enabled())
		{
			*sampler_state = {};
			continue;
		}

		*sampler_state = m_gl_texture_cache.upload_texture(cmd, tex, m_rtts);
		if (!sampler_state->validate())
		{
			continue;
		}

		if (!is_sampler_dirty)
		{
			if (sampler_state->format_class != previous_format_class)
			{
				// Host details changed but RSX is not aware
				invalidated_programs |= rsx::fragment_program_state_dirty;
			}

			if (sampler_state->format_ex)
			{
				// Nothing to change, use cached sampler
				continue;
			}
		}

		sampler_state->format_ex = tex.format_ex();

		if (sampler_state->format_ex.texel_remap_control &&
			sampler_state->image_handle &&
			sampler_state->upload_context == rsx::texture_upload_context::shader_read &&
			(m_ctx->program.fp_metadata.bx2_texture_reads_mask & (1u << i)) == 0 &&
			!g_cfg.video.disable_hardware_texel_remapping) [[ unlikely ]]
		{
			// Check if we need to override the view format
			const auto gl_format = sampler_state->image_handle->view_format();
			GLenum format_override = gl_format;
			rsx::flags32_t flags_to_erase = 0u;
			rsx::flags32_t host_flags_to_set = 0u;

			if (sampler_state->format_ex.hw_SNORM_possible())
			{
				format_override = gl::get_compatible_snorm_format(gl_format);
				flags_to_erase = rsx::texture_control_bits::SEXT_MASK;
				host_flags_to_set = rsx::RSX_HOST_FORMAT_FEATURE_SNORM;
			}
			else if (sampler_state->format_ex.hw_SRGB_possible())
			{
				format_override = gl::get_compatible_srgb_format(gl_format);
				flags_to_erase = rsx::texture_control_bits::GAMMA_CTRL_MASK;
				host_flags_to_set = rsx::RSX_HOST_FORMAT_FEATURE_SRGB;
			}

			if (format_override != GL_NONE && format_override != gl_format)
			{
				sampler_state->image_handle = sampler_state->image_handle->as(format_override);
				sampler_state->format_ex.texel_remap_control &= (~flags_to_erase);
				sampler_state->format_ex.host_features |= host_flags_to_set;
			}
		}

		u32 actual_mipcount = 1;
		if (sampler_state->upload_context == rsx::texture_upload_context::shader_read)
		{
			actual_mipcount = tex.get_exact_mipmap_count();
		}
		else if (sampler_state->external_subresource_desc.op != rsx::deferred_request_command::nop)
		{
			actual_mipcount = sampler_state->external_subresource_desc.exact_mip_count();
		}

		m_fs_sampler_states[i].apply(tex, m_ctx->textures.fs_sampler_state[i].get(), actual_mipcount > 1);

		const auto texture_format = sampler_state->format_ex.format();
		// Depth format redirected to BGRA8 resample stage. Do not filter to avoid bits leaking.
		// If accurate graphics are desired, force a bitcast to COLOR as a workaround.
		const bool is_depth_reconstructed = sampler_state->format_class != rsx::classify_format(texture_format) &&
			(texture_format == CELL_GCM_TEXTURE_A8R8G8B8 || texture_format == CELL_GCM_TEXTURE_D8R8G8B8);
		// SNORM conversion required in shader. Do not interpolate to avoid introducing discontinuities due to how negative numbers work
		const bool is_snorm = (sampler_state->format_ex.texel_remap_control & rsx::texture_control_bits::SEXT_MASK) != 0;

		if (is_depth_reconstructed || is_snorm)
		{
			GLint adjusted_min_filter = GL_NEAREST;
			if (!is_depth_reconstructed) [[ unlikely ]]
			{
				switch (gl::tex_min_filter(tex.min_filter()))
				{
				default:
					break;
				case GL_LINEAR_MIPMAP_LINEAR:
				case GL_LINEAR_MIPMAP_NEAREST:
				case GL_NEAREST_MIPMAP_LINEAR:
				case GL_NEAREST_MIPMAP_NEAREST:
					// This is a hack and an unfortunate one at that as there is no feasible workaround.
					// Doing full trilinear filtering in a shader is just dumb, approximate it instead with NEAREST_NEAREST.
					adjusted_min_filter = GL_NEAREST_MIPMAP_NEAREST;
					break;
				}
			}

			// Depth format redirected to BGRA8 resample stage. Do not filter to avoid bits leaking.
			m_fs_sampler_states[i].set_parameteri(GL_TEXTURE_MIN_FILTER, adjusted_min_filter);
			m_fs_sampler_states[i].set_parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	for (u32 textures_ref = m_ctx->program.vp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
		{
			continue;
		}

		if (!m_ctx->textures.vs_sampler_state[i])
		{
			m_ctx->textures.vs_sampler_state[i] = std::make_unique<gl::texture_cache::sampled_image_descriptor>();
		}

		auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(m_ctx->textures.vs_sampler_state[i].get());
		const auto& tex = REGS(m_ctx)->vertex_textures[i];
		const auto previous_format_class = sampler_state->format_class;

		if (!m_samplers_dirty &&
			!(m_ctx->textures.vertex_textures_dirty & (1u << i)) &&
			!m_gl_texture_cache.test_if_descriptor_expired(cmd, m_rtts, sampler_state, tex))
		{
			continue;
		}

		const bool is_sampler_dirty = !!(m_ctx->textures.vertex_textures_dirty & (1u << i));

		if (!tex.enabled())
		{
			*sampler_state = {};
			continue;
		}

		*sampler_state = m_gl_texture_cache.upload_texture(cmd, REGS(m_ctx)->vertex_textures[i], m_rtts);

		if (!sampler_state->validate())
		{
			continue;
		}

		if (is_sampler_dirty)
		{
			m_vs_sampler_states[i].apply(tex, m_ctx->textures.vs_sampler_state[i].get());
		}
		else if (sampler_state->format_class != previous_format_class)
		{
			invalidated_programs |= rsx::vertex_program_state_dirty;
		}
	}

	m_samplers_dirty.store(false);
	return invalidated_programs;
}

void GLGSRender::bind_texture_env()
{
	// Bind textures and resolve external copy operations
	gl::command_context cmd{ gl_state };
	const bool is_interpreter = m_shader_interpreter.is_interpreter(m_program);

	auto decay_view_for_interpreter = [&](
		const rsx::image_section_attributes_t& attr,
		gl::texture_cache::sampled_image_descriptor* desc,
		gl::texture_view* base,
		const rsx::texture_channel_remap_t& decoded_remap,
		bool is_msaa,
		bool is_redirected) -> gl::texture_view*
	{
		if (!is_msaa && !is_redirected)
		{
			return base;
		}

		if (is_redirected && desc->image_type > rsx::texture_dimension_extended::texture_dimension_2d)
		{
			// Cannot handle redirect on 3D or cubemap with the interpreter.
			const auto target = gl::get_target(desc->image_type);
			return m_null_textures[target]->get_view(rsx::default_remap_vector);
		}

		using deferred_subresource_t = gl::texture_cache::deferred_subresource;
		auto image = static_cast<gl::viewable_image*>(base->image());
		auto rtt = gl::try_as_rtt(base->image());

		if (is_msaa)
		{
			// MSAA resolve
			ensure(rtt);
			rtt->memory_barrier(cmd, rsx::surface_access::transfer_read);
			image = rtt->get_surface(rsx::surface_access::transfer_read);
		}

		if (is_redirected)
		{
			// Force bitcast
			rsx::image_section_attributes_t flatten_attrs{};
			flatten_attrs.address = desc->ref_address;
			flatten_attrs.gcm_format = desc->format_ex.format();
			flatten_attrs.width = image->width();
			flatten_attrs.height = image->height();
			flatten_attrs.depth = 1;

			const coord3u flatten_rect = { 0, 0, 0, flatten_attrs.width, flatten_attrs.height, 1 };
			auto flatten_op = deferred_subresource_t::create_copy(
				image, flatten_attrs, flatten_rect, rsx::surface_transform::identity, decoded_remap, desc->is_cyclic_reference);

			ensure(desc->ref_address);
			flatten_op.cache_range = rtt
				? rtt->get_memory_range()
				: utils::address_range32::start_length(desc->ref_address, attr.pitch * attr.height);

			return m_gl_texture_cache.create_temporary_subresource(cmd, flatten_op);
		}

		return image->get_view(decoded_remap, base->aspect());
	};

	for (u32 textures_ref = m_ctx->program.fp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
		{
			continue;
		}

		gl::texture_view* primary_view = nullptr;
		gl::texture_view* stencil_mirror = nullptr;
		auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(m_ctx->textures.fs_sampler_state[i].get());

		auto& tex = REGS(m_ctx)->fragment_textures[i];
		if (tex.enabled() && sampler_state->validate())
		{
			if (primary_view = sampler_state->image_handle; !primary_view) [[unlikely]]
			{
				primary_view = m_gl_texture_cache.create_temporary_subresource(cmd, sampler_state->external_subresource_desc);
			}
		}

		if (const bool is_redirected = (m_ctx->program.fragment_program.texture_state.redirected_textures & (1 << i));
			!primary_view)
		{
			const auto target = gl::get_target(m_ctx->program.fragment_program.get_texture_dimension(i));
			primary_view = m_null_textures[target]->get_view(rsx::default_remap_vector);

			if (is_redirected)
			{
				stencil_mirror = primary_view;
			}
		}
		else if (is_redirected)
		{
			auto root_texture = static_cast<gl::viewable_image*>(primary_view->image());
			stencil_mirror = root_texture->get_view(rsx::default_remap_vector.with_encoding(gl::GL_REMAP_IDENTITY), gl::image_aspect::stencil);
		}

		if (is_interpreter) [[ unlikely ]]
		{
			// Interpreter does not support MSAA or DEPTH->RGBA conversion a.k.a aspect redirection
			auto view = primary_view;
			if (primary_view->aspect() != gl::image_aspect::color || primary_view->image()->samples() != 1)
			{
				const auto mask = (1u << i);
				const bool is_redirected = !!(m_ctx->program.fragment_program.texture_state.redirected_textures & mask);
				const bool is_msaa = !!(m_ctx->program.fragment_program.texture_state.multisampled_textures & mask);
				if (is_redirected || is_msaa)
				{
					view = decay_view_for_interpreter(
						tex.attributes(),
						sampler_state,
						primary_view,
						tex.decoded_remap(),
						is_msaa,
						is_redirected);
				}
			}

			m_shader_interpreter.bind_fragment_texture(i, view->handle(), *sampler_state);
			continue;
		}

		primary_view->bind(cmd, GL_FRAGMENT_TEXTURES_START + i);

		if (stencil_mirror)
		{
			stencil_mirror->bind(cmd, GL_STENCIL_MIRRORS_START + i);
		}
	}

	for (u32 textures_ref = m_ctx->program.vp_metadata.referenced_textures_mask, i = 0; textures_ref; textures_ref >>= 1, ++i)
	{
		if (!(textures_ref & 1))
		{
			continue;
		}

		auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(m_ctx->textures.vs_sampler_state[i].get());
		gl::texture_view* view = nullptr;

		if (REGS(m_ctx)->vertex_textures[i].enabled() &&
			sampler_state->validate())
		{
			if (view = sampler_state->image_handle; !view)
			{
				view = m_gl_texture_cache.create_temporary_subresource(cmd, sampler_state->external_subresource_desc);
			}
		}

		if (view) [[likely]]
		{
			view->bind(cmd, GL_VERTEX_TEXTURES_START + i);
		}
		else
		{
			cmd->bind_texture(GL_VERTEX_TEXTURES_START + i, GL_TEXTURE_2D, GL_NONE);
		}
	}

	if (is_interpreter)
	{
		if (m_ctx->program.fp_metadata.referenced_textures_mask)
		{
			m_shader_interpreter.flush_fragment_texture_bindings();
		}

		if (m_ctx->program.vp_metadata.referenced_textures_mask)
		{
			m_shader_interpreter.flush_vertex_texture_bindings();
		}
	}

	if (m_ctx->program.fragment_program.ctrl & RSX_SHADER_CONTROL_EMULATE_DEPTH_COMPARE)
	{
		ensure(m_rtts.m_bound_depth_stencil.first, "Invalid FBO setup");
		gl::insert_texture_barrier();

		auto view = m_rtts.m_bound_depth_stencil.second->get_view(rsx::default_remap_vector, gl::image_aspect::depth);
		view->bind(cmd, GL_TEMP_IMAGE_SLOT(0));
	}
}

void GLGSRender::emit_geometry(u32 sub_index)
{
	const auto do_heap_cleanup = [this]()
	{
		if (manually_flush_ring_buffers)
		{
			m_attrib_ring_buffer->unmap();
			m_index_ring_buffer->unmap();
		}
		else
		{
			//DMA push; not needed with MAP_COHERENT
			//glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		}
	};

	m_profiler.start();

	auto& draw_call = REGS(m_ctx)->current_draw_clause;
	const rsx::flags32_t vertex_state_mask = rsx::vertex_base_changed | rsx::vertex_arrays_changed;
	const rsx::flags32_t vertex_state = (sub_index == 0) ? rsx::vertex_arrays_changed : draw_call.execute_pipeline_dependencies(m_ctx) & vertex_state_mask;

	if (vertex_state & rsx::vertex_arrays_changed)
	{
		m_draw_processor.analyse_inputs_interleaved(m_vertex_layout, m_ctx->program.vp_metadata);
	}
	else if (vertex_state & rsx::vertex_base_changed)
	{
		// Rebase vertex bases instead of
		for (auto& info : m_vertex_layout.interleaved_blocks)
		{
			info->vertex_range.second = 0;
			const auto vertex_base_offset = REGS(m_ctx)->vertex_data_base_offset();
			info->real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(vertex_base_offset, info->base_offset), info->memory_location);
		}
	}
	else
	{
		// Discard cached results
		for (auto& info : m_vertex_layout.interleaved_blocks)
		{
			info->vertex_range.second = 0;
		}
	}

	if (vertex_state && !m_vertex_layout.validate())
	{
		// No vertex inputs enabled
		// Execute remaining pipeline barriers with NOP draw
		do
		{
			draw_call.execute_pipeline_dependencies(m_ctx);
		} while (draw_call.next());

		draw_call.end();
		return;
	}

	if (manually_flush_ring_buffers)
	{
		//Use approximations to reserve space. This path is mostly for debug purposes anyway
		u32 approx_vertex_count = draw_call.get_elements_count();
		u32 approx_working_buffer_size = approx_vertex_count * 256;

		//Allocate 256K heap if we have no approximation at this time (inlined array)
		m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
		m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
	}

	// Do vertex upload before RTT prep / texture lookups to give the driver time to push data
	auto upload_info = set_vertex_buffer();
	do_heap_cleanup();

	if (upload_info.vertex_draw_count == 0)
	{
		// Malformed vertex setup; abort
		return;
	}

	const GLenum draw_mode = gl::draw_mode(draw_call.primitive);
	update_vertex_env(upload_info);

	m_ctx->stats.vertex_upload_time += m_profiler.duration();

	gl_state.use_program(m_program->id());

	if (!upload_info.index_info)
	{
		if (draw_call.is_trivial_instanced_draw && m_config.supports_hw_instanced_rendering)
		{
			glDrawArraysInstanced(draw_mode, 0, upload_info.vertex_draw_count, draw_call.pass_count());
		}
		else if (draw_call.is_single_draw())
		{
			glDrawArrays(draw_mode, 0, upload_info.vertex_draw_count);
		}
		else
		{
			const auto& subranges = draw_call.get_subranges();
			const auto draw_count = subranges.size();
			const auto& driver_caps = gl::get_driver_caps();
			bool use_draw_arrays_fallback = false;

			m_scratch_buffer.resize(draw_count * 24);
			GLint* firsts = reinterpret_cast<GLint*>(m_scratch_buffer.data());
			GLsizei* counts = (firsts + draw_count);
			const GLvoid** offsets = utils::bless<const GLvoid*>(counts + draw_count);

			u32 first = 0;
			u32 dst_index = 0;
			for (const auto& range : subranges)
			{
				firsts[dst_index] = first;
				counts[dst_index] = range.count;
				offsets[dst_index++] = reinterpret_cast<const GLvoid*>(u64{first << 2});

				if (driver_caps.vendor_AMD && (first + range.count) > (0x100000 >> 2))
				{
					// Unlikely, but added here in case the identity buffer is not large enough somehow
					use_draw_arrays_fallback = true;
					break;
				}

				first += range.count;
			}

			if (use_draw_arrays_fallback)
			{
				// MultiDrawArrays is broken on some primitive types using AMD. One known type is GL_TRIANGLE_STRIP but there could be more
				for (u32 n = 0; n < draw_count; ++n)
				{
					glDrawArrays(draw_mode, firsts[n], counts[n]);
				}
			}
			else if (driver_caps.vendor_AMD)
			{
				// Use identity index buffer to fix broken vertexID on AMD
				m_identity_index_buffer->bind();
				glMultiDrawElements(draw_mode, counts, GL_UNSIGNED_INT, offsets, static_cast<GLsizei>(draw_count));
			}
			else
			{
				// Normal render
				glMultiDrawArrays(draw_mode, firsts, counts, static_cast<GLsizei>(draw_count));
			}
		}
	}
	else
	{
		const GLenum index_type = std::get<0>(*upload_info.index_info);
		const u32 index_offset = std::get<1>(*upload_info.index_info);
		const bool restarts_valid = gl::is_primitive_native(draw_call.primitive) && !draw_call.is_disjoint_primitive;

		if (gl_state.enable(restarts_valid && m_ctx->draw_state.input_assembly.restart_index_enabled, GL_PRIMITIVE_RESTART))
		{
			glPrimitiveRestartIndex((index_type == GL_UNSIGNED_SHORT) ? 0xffff : 0xffffffff);
		}

		m_index_ring_buffer->bind();

		if (draw_call.is_trivial_instanced_draw && m_config.supports_hw_instanced_rendering)
		{
			glDrawElementsInstanced(draw_mode, upload_info.vertex_draw_count, index_type, reinterpret_cast<GLvoid*>(u64{ index_offset }), draw_call.pass_count());
		}
		else if (draw_call.is_single_draw())
		{
			glDrawElements(draw_mode, upload_info.vertex_draw_count, index_type, reinterpret_cast<GLvoid*>(u64{index_offset}));
		}
		else
		{
			const auto& subranges = draw_call.get_subranges();
			const auto draw_count = subranges.size();
			const u32 type_scale = (index_type == GL_UNSIGNED_SHORT) ? 1 : 2;
			uptr index_ptr = index_offset;
			m_scratch_buffer.resize(draw_count * 16);

			GLsizei *counts = reinterpret_cast<GLsizei*>(m_scratch_buffer.data());
			const GLvoid** offsets = utils::bless<const GLvoid*>(counts + draw_count);
			int dst_index = 0;

			for (const auto& range : subranges)
			{
				const auto index_size = get_index_count(draw_call.primitive, range.count);
				counts[dst_index] = index_size;
				offsets[dst_index++] = reinterpret_cast<const GLvoid*>(index_ptr);

				index_ptr += (index_size << type_scale);
			}

			glMultiDrawElements(draw_mode, counts, index_type, offsets, static_cast<GLsizei>(draw_count));
		}
	}

	m_ctx->stats.draw_exec_time += m_profiler.duration();
}

void GLGSRender::begin_draw()
{
	set_scissor();

	if (m_ctx->draw_state.dirty.test(rsx::draw_state_t::group::programs))
	{
		// Shaders need to be reloaded.
		m_prev_program = m_program;
		m_program = nullptr;
	}
}

bool GLGSRender::resolve_sampler_state(rsx::flags32_t& invalidated_programs)
{
	invalidated_programs = load_texture_env();
	return true;
}

bool GLGSRender::end_draw()
{
	// Groups changed since the last executed draw. Loading an interpreter program adds both programs.
	m_interpreter_state = m_ctx->draw_state.dirty;

	// NOTE: Due to common OpenGL driver architecture, vertex data has to be uploaded as far away from the draw as possible
	// TODO: Implement shaders cache prediction to avoid uploading vertex data if draw is going to skip
	if (!load_program())
	{
		// Program is not ready, skip drawing this
		std::this_thread::yield();
		// m_rtts.on_write(); - breaks games for obvious reasons
		return false;
	}

	// Load program execution environment
	load_program_env();
	m_ctx->stats.setup_time += m_profiler.duration();

	bind_texture_env();
	m_gl_texture_cache.release_uncached_temporary_subresources();
	m_ctx->stats.textures_upload_time += m_profiler.duration();

	gl::command_context cmd{ gl_state };
	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil))
	{
		ds->write_barrier(cmd);
	}

	for (auto &rtt : m_rtts.m_bound_render_targets)
	{
		if (auto surface = std::get<1>(rtt))
		{
			surface->write_barrier(cmd);
		}
	}

	update_draw_state();

	if (g_cfg.video.debug_output)
	{
		m_program->validate();
	}

	auto& draw_call = REGS(m_ctx)->current_draw_clause;
	draw_call.begin();
	u32 subdraw = 0u;
	do
	{
		emit_geometry(subdraw++);

		if (draw_call.is_trivial_instanced_draw && m_config.supports_hw_instanced_rendering)
		{
			// We already completed. End the draw.
			draw_call.end();
		}
	}
	while (draw_call.next());

	m_rtts.on_write(m_ctx->fb.layout.color_write_enabled, m_ctx->fb.layout.zeta_write_enabled);

	m_attrib_ring_buffer->notify();
	m_index_ring_buffer->notify();
	m_fragment_env_buffer->notify();
	m_vertex_env_buffer->notify();
	m_texture_parameters_buffer->notify();
	m_vertex_layout_buffer->notify();
	m_fragment_constants_buffer->notify();
	m_transform_constants_buffer->notify();
	m_instancing_ring_buffer->notify();

	m_ctx->stats.setup_time += m_profiler.duration();

	return true;
}
