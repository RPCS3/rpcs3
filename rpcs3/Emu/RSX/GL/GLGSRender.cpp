#include "stdafx.h"
#include "../Overlays/overlay_compile_notification.h"
#include "../Overlays/Shaders/shader_loading_dialog_native.h"
#include "GLGSRender.h"
#include "GLCompute.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/RSX/rsx_methods.h"

#include "../Program/program_state_cache2.hpp"

[[noreturn]] extern void report_fatal_error(std::string_view _text, bool is_html = false, bool include_help_text = true);

namespace
{
	void throw_fatal(const std::vector<std::string>& reasons)
	{
		const std::string delimiter = "\n- ";
		const std::string reasons_list = fmt::merge(reasons, delimiter);
		const std::string message = fmt::format(
			"OpenGL could not be initialized on this system for the following reason(s):\n"
			"\n"
			"- %s",
			reasons_list);

		Emu.BlockingCallFromMainThread([message]()
		{
			report_fatal_error(message, false, false);
		});
	}
}

u64 GLGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<GLGSRender>&>(*this));
}

GLGSRender::GLGSRender(utils::serial* ar) noexcept : GSRender(ar)
{
	m_shaders_cache = std::make_unique<gl::shader_cache>(m_prog_buffer, "opengl", "v1.94");

	if (g_cfg.video.disable_vertex_cache)
		m_vertex_cache = std::make_unique<gl::null_vertex_cache>();
	else
		m_vertex_cache = std::make_unique<gl::weak_vertex_cache>();

	backend_config.supports_hw_a2c = false;
	backend_config.supports_hw_a2one = false;
	backend_config.supports_multidraw = true;
	backend_config.supports_normalized_barycentrics = true;
}

extern CellGcmContextData current_context;

void GLGSRender::set_viewport()
{
	// NOTE: scale offset matrix already contains the viewport transformation
	const auto [clip_width, clip_height] = rsx::apply_resolution_scale<true>(
		rsx::method_registers.surface_clip_width(), rsx::method_registers.surface_clip_height());

	glViewport(0, 0, clip_width, clip_height);
}

void GLGSRender::set_scissor(bool clip_viewport)
{
	areau scissor;
	if (get_scissor(scissor, clip_viewport))
	{
		// NOTE: window origin does not affect scissor region (probably only affects viewport matrix; already applied)
		// See LIMBO [NPUB-30373] which uses shader window origin = top
		glScissor(scissor.x1, scissor.y1, scissor.width(), scissor.height());
		gl_state.enable(GL_TRUE, GL_SCISSOR_TEST);
	}
}

void GLGSRender::on_init_thread()
{
	ensure(m_frame);

	// NOTES: All contexts have to be created before any is bound to a thread
	// This allows context sharing to work (both GLRCs passed to wglShareLists have to be idle or you get ERROR_BUSY)
	m_context = m_frame->make_context();

	const auto shadermode = g_cfg.video.shadermode.get();
	if (shadermode != shader_mode::recompiler)
	{
		auto context_create_func = [m_frame = m_frame]()
		{
			return m_frame->make_context();
		};

		auto context_bind_func = [m_frame = m_frame](draw_context_t ctx)
		{
			m_frame->set_current(ctx);
		};

		auto context_destroy_func = [m_frame = m_frame](draw_context_t ctx)
		{
			m_frame->delete_context(ctx);
		};

		gl::initialize_pipe_compiler(context_create_func, context_bind_func, context_destroy_func, g_cfg.video.shader_compiler_threads_count);
	}
	else
	{
		auto null_context_create_func = []() -> draw_context_t
		{
			return nullptr;
		};

		gl::initialize_pipe_compiler(null_context_create_func, {}, {}, 1);
	}

	// Bind primary context to main RSX thread
	m_frame->set_current(m_context);
	gl::set_primary_context_thread();

	zcull_ctrl.reset(static_cast<::rsx::reports::ZCULL_control*>(this));
	m_occlusion_type = g_cfg.video.precise_zpass_count ? GL_SAMPLES_PASSED : GL_ANY_SAMPLES_PASSED;

	gl::init();
	gl::set_command_context(gl_state);

	//Enable adaptive vsync if vsync is requested
	gl::set_swapinterval(g_cfg.video.vsync ? -1 : 0);

	if (g_cfg.video.debug_output)
		gl::enable_debugging();

	rsx_log.notice("GL RENDERER: %s (%s)", reinterpret_cast<const char*>(glGetString(GL_RENDERER)), reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	rsx_log.notice("GL VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	rsx_log.notice("GLSL VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

	auto& gl_caps = gl::get_driver_caps();

	std::vector<std::string> exception_reasons;
	if (!gl_caps.ARB_texture_buffer_supported)
	{
		exception_reasons.push_back("GL_ARB_texture_buffer_object is required but not supported by your GPU");
	}

	if (!gl_caps.ARB_dsa_supported && !gl_caps.EXT_dsa_supported)
	{
		exception_reasons.push_back("GL_ARB_direct_state_access or GL_EXT_direct_state_access is required but not supported by your GPU");
	}

	if (!exception_reasons.empty())
	{
		throw_fatal(exception_reasons);
	}

	if (!gl_caps.ARB_depth_buffer_float_supported && g_cfg.video.force_high_precision_z_buffer)
	{
		rsx_log.warning("High precision Z buffer requested but your GPU does not support GL_ARB_depth_buffer_float. Option ignored.");
	}

	if (!gl_caps.ARB_texture_barrier_supported && !gl_caps.NV_texture_barrier_supported && !g_cfg.video.strict_rendering_mode)
	{
		rsx_log.warning("Texture barriers are not supported by your GPU. Feedback loops will have undefined results.");
	}

	if (!gl_caps.ARB_bindless_texture_supported)
	{
		switch (shadermode)
		{
		case shader_mode::async_with_interpreter:
		case shader_mode::interpreter_only:
			rsx_log.error("Bindless texture extension required for shader interpreter is not supported on your GPU. Will use async recompiler as a fallback.");
			g_cfg.video.shadermode.set(shader_mode::async_recompiler);
			break;
		default:
			break;
		}
	}

	if (gl_caps.NV_fragment_shader_barycentric_supported &&
		gl_caps.vendor_NVIDIA &&
		g_cfg.video.shader_precision != gpu_preset_level::low)
	{
		// NVIDIA's attribute interpolation requires some workarounds
		backend_config.supports_normalized_barycentrics = false;
	}

	// Use industry standard resource alignment values as defaults
	m_uniform_buffer_offset_align = 256;
	m_min_texbuffer_alignment = 256;
	m_max_texbuffer_size = 0;

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_offset_align);
	glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &m_min_texbuffer_alignment);
	glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &m_max_texbuffer_size);
	m_vao.create();

	// Set min alignment to 16-bytes for SSE optimizations with aligned addresses to work
	m_min_texbuffer_alignment = std::max(m_min_texbuffer_alignment, 16);
	m_uniform_buffer_offset_align = std::max(m_uniform_buffer_offset_align, 16);

	rsx_log.notice("Supported texel buffer size reported: %d bytes", m_max_texbuffer_size);
	if (m_max_texbuffer_size < (16 * 0x100000))
	{
		rsx_log.error("Max texture buffer size supported is less than 16M which is useless. Expect undefined behaviour.");
		m_max_texbuffer_size = (16 * 0x100000);
	}

	// Array stream buffer
	{
		m_gl_persistent_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		gl_state.bind_texture(GL_STREAM_BUFFER_START + 0, GL_TEXTURE_BUFFER, m_gl_persistent_stream_buffer->id());
	}

	// Register stream buffer
	{
		m_gl_volatile_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		gl_state.bind_texture(GL_STREAM_BUFFER_START + 1, GL_TEXTURE_BUFFER, m_gl_volatile_stream_buffer->id());
	}

	// Fallback null texture instead of relying on texture0
	{
		std::array<u32, 8> pixeldata = { 0, 0, 0, 0, 0, 0, 0, 0 };

		// 1D
		auto tex1D = std::make_unique<gl::texture>(GL_TEXTURE_1D, 1, 1, 1, 1, GL_RGBA8);
		tex1D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		// 2D
		auto tex2D = std::make_unique<gl::texture>(GL_TEXTURE_2D, 1, 1, 1, 1, GL_RGBA8);
		tex2D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		// 3D
		auto tex3D = std::make_unique<gl::texture>(GL_TEXTURE_3D, 1, 1, 1, 1, GL_RGBA8);
		tex3D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		// CUBE
		auto texCUBE = std::make_unique<gl::texture>(GL_TEXTURE_CUBE_MAP, 1, 1, 1, 1, GL_RGBA8);
		texCUBE->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		m_null_textures[GL_TEXTURE_1D] = std::move(tex1D);
		m_null_textures[GL_TEXTURE_2D] = std::move(tex2D);
		m_null_textures[GL_TEXTURE_3D] = std::move(tex3D);
		m_null_textures[GL_TEXTURE_CUBE_MAP] = std::move(texCUBE);
	}

	if (!gl_caps.ARB_buffer_storage_supported)
	{
		rsx_log.warning("Forcing use of legacy OpenGL buffers because ARB_buffer_storage is not supported");
		// TODO: do not modify config options
		g_cfg.video.renderdoc_compatiblity.from_string("true");
	}

	if (g_cfg.video.renderdoc_compatiblity)
	{
		rsx_log.warning("Using legacy openGL buffers.");
		manually_flush_ring_buffers = true;

		m_attrib_ring_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_transform_constants_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_fragment_constants_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_fragment_env_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_vertex_env_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_texture_parameters_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_vertex_layout_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_index_ring_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_vertex_instructions_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_fragment_instructions_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_raster_env_ring_buffer = std::make_unique<gl::legacy_ring_buffer>();
	}
	else
	{
		m_attrib_ring_buffer = std::make_unique<gl::ring_buffer>();
		m_transform_constants_buffer = std::make_unique<gl::ring_buffer>();
		m_fragment_constants_buffer = std::make_unique<gl::ring_buffer>();
		m_fragment_env_buffer = std::make_unique<gl::ring_buffer>();
		m_vertex_env_buffer = std::make_unique<gl::ring_buffer>();
		m_texture_parameters_buffer = std::make_unique<gl::ring_buffer>();
		m_vertex_layout_buffer = std::make_unique<gl::ring_buffer>();
		m_index_ring_buffer = gl_caps.vendor_AMD ? std::make_unique<gl::transient_ring_buffer>() : std::make_unique<gl::ring_buffer>();
		m_vertex_instructions_buffer = std::make_unique<gl::ring_buffer>();
		m_fragment_instructions_buffer = std::make_unique<gl::ring_buffer>();
		m_raster_env_ring_buffer = std::make_unique<gl::ring_buffer>();
	}

	m_attrib_ring_buffer->create(gl::buffer::target::texture, 256 * 0x100000);
	m_index_ring_buffer->create(gl::buffer::target::element_array, 16 * 0x100000);
	m_transform_constants_buffer->create(gl::buffer::target::uniform, 64 * 0x100000);
	m_fragment_constants_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_fragment_env_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_vertex_env_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_texture_parameters_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_vertex_layout_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_raster_env_ring_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);

	if (shadermode == shader_mode::async_with_interpreter || shadermode == shader_mode::interpreter_only)
	{
		m_vertex_instructions_buffer->create(gl::buffer::target::ssbo, 16 * 0x100000);
		m_fragment_instructions_buffer->create(gl::buffer::target::ssbo, 16 * 0x100000);

		m_shader_interpreter.create();
	}

	if (gl_caps.vendor_AMD)
	{
		// Initialize with 256k identity entries
		std::vector<u32> dst(256 * 1024);
		for (u32 n = 0; n < (0x100000 >> 2); ++n)
		{
			dst[n] = n;
		}

		m_identity_index_buffer = std::make_unique<gl::buffer>();
		m_identity_index_buffer->create(gl::buffer::target::element_array,dst.size() * sizeof(u32), dst.data(), gl::buffer::memory_type::local);
	}
	else if (gl_caps.vendor_NVIDIA)
	{
		// NOTE: On NVIDIA cards going back decades (including the PS3) there is a slight normalization inaccuracy in compressed formats.
		// Confirmed in BLES01916 (The Evil Within) which uses RGB565 for some virtual texturing data.
		backend_config.supports_hw_renormalization = true;
	}

	m_persistent_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>(static_cast<u32>(m_attrib_ring_buffer->size()), m_max_texbuffer_size));
	m_volatile_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>(static_cast<u32>(m_attrib_ring_buffer->size()), m_max_texbuffer_size));
	m_gl_persistent_stream_buffer->copy_from(m_persistent_stream_view);
	m_gl_volatile_stream_buffer->copy_from(m_volatile_stream_view);

	m_vao.element_array_buffer = *m_index_ring_buffer;

	int image_unit = 0;
	for (auto &sampler : m_fs_sampler_states)
	{
		sampler.create();
		sampler.bind(image_unit++);
	}

	for (auto &sampler : m_fs_sampler_mirror_states)
	{
		sampler.create();
		sampler.apply_defaults();
		sampler.bind(image_unit++);
	}

	for (auto &sampler : m_vs_sampler_states)
	{
		sampler.create();
		sampler.bind(image_unit++);
	}

	//Occlusion query
	for (u32 i = 0; i < rsx::reports::occlusion_query_count; ++i)
	{
		GLuint handle = 0;
		auto &query = m_occlusion_query_data[i];
		glGenQueries(1, &handle);

		query.driver_handle = handle;
		query.pending = false;
		query.active = false;
		query.result = 0;
	}

	m_ui_renderer.create();
	m_video_output_pass.create();

	m_gl_texture_cache.initialize();

	m_prog_buffer.initialize
	(
		[this](void* const& props, const RSXVertexProgram& vp, const RSXFragmentProgram& fp)
		{
			// Program was linked or queued for linking
			m_shaders_cache->store(props, vp, fp);
		}
	);

	if (!m_overlay_manager)
	{
		m_frame->hide();
		m_shaders_cache->load(nullptr);
		m_frame->show();
	}
	else
	{
		rsx::shader_loading_dialog_native dlg(this);

		m_shaders_cache->load(&dlg);
	}
}


void GLGSRender::on_exit()
{
	// Destroy internal RSX state, may call upon this->do_local_task
	GSRender::on_exit();

	// Globals
	// TODO: Move these
	gl::destroy_compute_tasks();
	gl::destroy_overlay_passes();

	gl::destroy_global_texture_resources();

	gl::debug::g_vis_texture.reset(); // TODO

	gl::destroy_pipe_compiler();

	m_prog_buffer.clear();
	m_rtts.destroy();

	for (auto &fbo : m_framebuffer_cache)
	{
		fbo.remove();
	}

	m_framebuffer_cache.clear();

	if (m_flip_fbo)
	{
		m_flip_fbo.remove();
	}

	if (m_flip_tex_color)
	{
		m_flip_tex_color.reset();
	}

	if (m_vao)
	{
		m_vao.remove();
	}

	m_gl_persistent_stream_buffer.reset();
	m_gl_volatile_stream_buffer.reset();

	for (auto &sampler : m_fs_sampler_states)
	{
		sampler.remove();
	}

	for (auto &sampler : m_fs_sampler_mirror_states)
	{
		sampler.remove();
	}

	for (auto &sampler : m_vs_sampler_states)
	{
		sampler.remove();
	}

	if (m_attrib_ring_buffer)
	{
		m_attrib_ring_buffer->remove();
	}

	if (m_transform_constants_buffer)
	{
		m_transform_constants_buffer->remove();
	}

	if (m_fragment_constants_buffer)
	{
		m_fragment_constants_buffer->remove();
	}

	if (m_fragment_env_buffer)
	{
		m_fragment_env_buffer->remove();
	}

	if (m_vertex_env_buffer)
	{
		m_vertex_env_buffer->remove();
	}

	if (m_texture_parameters_buffer)
	{
		m_texture_parameters_buffer->remove();
	}

	if (m_vertex_layout_buffer)
	{
		m_vertex_layout_buffer->remove();
	}

	if (m_index_ring_buffer)
	{
		m_index_ring_buffer->remove();
	}

	if (m_identity_index_buffer)
	{
		m_identity_index_buffer->remove();
	}

	if (m_vertex_instructions_buffer)
	{
		m_vertex_instructions_buffer->remove();
	}

	if (m_fragment_instructions_buffer)
	{
		m_fragment_instructions_buffer->remove();
	}

	if (m_raster_env_ring_buffer)
	{
		m_raster_env_ring_buffer->remove();
	}

	m_null_textures.clear();
	m_text_printer.close();
	m_gl_texture_cache.destroy();
	m_ui_renderer.destroy();
	m_video_output_pass.destroy();

	m_shader_interpreter.destroy();

	for (u32 i = 0; i < rsx::reports::occlusion_query_count; ++i)
	{
		auto &query = m_occlusion_query_data[i];
		query.active = false;
		query.pending = false;

		GLuint handle = query.driver_handle;
		glDeleteQueries(1, &handle);
		query.driver_handle = 0;
	}

	zcull_ctrl.release();

	gl::set_primary_context_thread(false);
}

void GLGSRender::clear_surface(u32 arg)
{
	if (skip_current_frame) return;

	// If stencil write mask is disabled, remove clear_stencil bit
	if (!rsx::method_registers.stencil_mask()) arg &= ~RSX_GCM_CLEAR_STENCIL_BIT;

	// Ignore invalid clear flags
	if ((arg & RSX_GCM_CLEAR_ANY_MASK) == 0) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (arg & RSX_GCM_CLEAR_COLOR_RGBA_MASK) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (arg & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK) ctx |= rsx::framebuffer_creation_context::context_clear_depth;

	init_buffers(static_cast<rsx::framebuffer_creation_context>(ctx), true);

	if (!m_graphics_state.test(rsx::rtt_config_valid)) return;

	gl::clear_cmd_info clear_cmd{};

	gl::command_context cmd{ gl_state };
	const bool full_frame =
		rsx::method_registers.scissor_origin_x() == 0 &&
		rsx::method_registers.scissor_origin_y() == 0 &&
		rsx::method_registers.scissor_width() >= rsx::method_registers.surface_clip_width() &&
		rsx::method_registers.scissor_height() >= rsx::method_registers.surface_clip_height();

	bool update_color = false, update_z = false;
	rsx::surface_depth_format2 surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil); arg & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK)
	{
		if (arg & RSX_GCM_CLEAR_DEPTH_BIT)
		{
			u32 max_depth_value = get_max_depth_value(surface_depth_format);
			u32 clear_depth = rsx::method_registers.z_clear_value(is_depth_stencil_format(surface_depth_format));

			clear_cmd.clear_depth.value = f32(clear_depth) / max_depth_value;
			clear_cmd.aspect_mask |= gl::image_aspect::depth;
		}

		if (is_depth_stencil_format(surface_depth_format))
		{
			if (arg & RSX_GCM_CLEAR_STENCIL_BIT)
			{
				clear_cmd.clear_stencil.mask = rsx::method_registers.stencil_mask();
				clear_cmd.clear_stencil.value = rsx::method_registers.stencil_clear_value();
				clear_cmd.aspect_mask |= gl::image_aspect::stencil;
			}

			if (const auto ds_mask = (arg & RSX_GCM_CLEAR_DEPTH_STENCIL_MASK);
				ds_mask != RSX_GCM_CLEAR_DEPTH_STENCIL_MASK || !full_frame)
			{
				ensure(clear_cmd.aspect_mask);

				if (ds->state_flags & rsx::surface_state_flags::erase_bkgnd &&  // Needs initialization
					ds->old_contents.empty() && !g_cfg.video.read_depth_buffer) // No way to load data from memory, so no initialization given
				{
					// Only one aspect was cleared. Make sure to memory initialize the other before removing dirty flag
					if (ds_mask == RSX_GCM_CLEAR_DEPTH_BIT)
					{
						// Depth was cleared, initialize stencil
						clear_cmd.clear_stencil.mask = 0xff;
						clear_cmd.clear_stencil.value = 0xff;
						clear_cmd.aspect_mask |= gl::image_aspect::stencil;
					}
					else if (ds_mask == RSX_GCM_CLEAR_STENCIL_BIT)
					{
						// Stencil was cleared, initialize depth
						clear_cmd.clear_depth.value = 1.f;
						clear_cmd.aspect_mask |= gl::image_aspect::depth;
					}
				}
				else
				{
					ds->write_barrier(cmd);
				}
			}
		}

		if (clear_cmd.aspect_mask)
		{
			// Memory has been initialized
			update_z = true;
		}
	}

	if (auto colormask = (arg & 0xf0))
	{
		u8 clear_a = rsx::method_registers.clear_color_a();
		u8 clear_r = rsx::method_registers.clear_color_r();
		u8 clear_g = rsx::method_registers.clear_color_g();
		u8 clear_b = rsx::method_registers.clear_color_b();

		switch (rsx::method_registers.surface_color())
		{
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
		{
			// Nop
			colormask = 0;
			break;
		}
		case rsx::surface_color_format::b8:
		{
			rsx::get_b8_clear_color(clear_r, clear_g, clear_b, clear_a);
			colormask = rsx::get_b8_clearmask(colormask);
			break;
		}
		case rsx::surface_color_format::g8b8:
		{
			rsx::get_g8b8_clear_color(clear_r, clear_g, clear_b, clear_a);
			colormask = rsx::get_g8b8_r8g8_clearmask(colormask);
			break;
		}
		case rsx::surface_color_format::r5g6b5:
		{
			rsx::get_rgb565_clear_color(clear_r, clear_g, clear_b, clear_a);
			break;
		}
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		{
			rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 255);
			break;
		}
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		{
			rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 0);
			break;
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			rsx::get_abgr8_clear_color(clear_r, clear_g, clear_b, clear_a);
			colormask = rsx::get_abgr8_clearmask(colormask);
			break;
		}
		default:
		{
			break;
		}
		}

		if (colormask)
		{
			clear_cmd.clear_color.mask = colormask;
			clear_cmd.clear_color.attachment_count = static_cast<u8>(m_rtts.m_bound_render_target_ids.size());
			clear_cmd.clear_color.r = clear_r;
			clear_cmd.clear_color.g = clear_g;
			clear_cmd.clear_color.b = clear_b;
			clear_cmd.clear_color.a = clear_a;
			clear_cmd.aspect_mask |= gl::image_aspect::color;

			if (!full_frame)
			{
				for (const auto& index : m_rtts.m_bound_render_target_ids)
				{
					m_rtts.m_bound_render_targets[index].second->write_barrier(cmd);
				}
			}

			update_color = true;
		}
	}

	if (update_color || update_z)
	{
		m_rtts.on_write({ update_color, update_color, update_color, update_color }, update_z);
	}

	if (!full_frame)
	{
		gl_state.enable(GL_SCISSOR_TEST);
	}

	gl::clear_attachments(cmd, clear_cmd);
}

bool GLGSRender::load_program()
{
	const auto shadermode = g_cfg.video.shadermode.get();

	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		ensure(current_fragment_program.valid);

		get_current_vertex_program(vs_sampler_state);
	}
	else if (m_program)
	{
		if (!m_shader_interpreter.is_interpreter(m_program)) [[likely]]
		{
			return true;
		}

		if (shadermode == shader_mode::interpreter_only)
		{
			m_program = m_shader_interpreter.get(current_fp_metadata);
			return true;
		}
	}

	const bool was_interpreter = m_shader_interpreter.is_interpreter(m_program);
	m_vertex_prog = nullptr;
	m_fragment_prog = nullptr;

	if (shadermode != shader_mode::interpreter_only) [[likely]]
	{
		void* pipeline_properties = nullptr;
		std::tie(m_program, m_vertex_prog, m_fragment_prog) = m_prog_buffer.get_graphics_pipeline(current_vertex_program, current_fragment_program, pipeline_properties,
			shadermode != shader_mode::recompiler, true);

		if (m_prog_buffer.check_cache_missed())
		{
			// Notify the user with HUD notification
			if (g_cfg.misc.show_shader_compilation_hint)
			{
				rsx::overlays::show_shader_compile_notification();
			}
		}
		else
		{
			ensure(m_program);
			m_program->sync();
		}
	}
	else
	{
		m_program = nullptr;
	}

	if (!m_program && (shadermode == shader_mode::async_with_interpreter || shadermode == shader_mode::interpreter_only))
	{
		// Fall back to interpreter
		m_program = m_shader_interpreter.get(current_fp_metadata);
		if (was_interpreter != m_shader_interpreter.is_interpreter(m_program))
		{
			// Program has changed, reupload
			m_interpreter_state = rsx::invalidate_pipeline_bits;
		}
	}

	return m_program != nullptr;
}

void GLGSRender::load_program_env()
{
	if (!m_program)
	{
		fmt::throw_exception("Unreachable right now");
	}

	const u32 fragment_constants_size = current_fp_metadata.program_constants_buffer_length;

	const bool update_transform_constants = m_graphics_state & rsx::pipeline_state::transform_constants_dirty;
	const bool update_fragment_constants = (m_graphics_state & rsx::pipeline_state::fragment_constants_dirty) && fragment_constants_size;
	const bool update_vertex_env = m_graphics_state & rsx::pipeline_state::vertex_state_dirty;
	const bool update_fragment_env = m_graphics_state & rsx::pipeline_state::fragment_state_dirty;
	const bool update_fragment_texture_env = m_graphics_state & rsx::pipeline_state::fragment_texture_state_dirty;
	const bool update_instruction_buffers = !!m_interpreter_state && m_shader_interpreter.is_interpreter(m_program);
	const bool update_raster_env = rsx::method_registers.polygon_stipple_enabled() && (m_graphics_state & rsx::pipeline_state::polygon_stipple_pattern_dirty);

	if (manually_flush_ring_buffers)
	{
		if (update_fragment_env) m_fragment_env_buffer->reserve_storage_on_heap(128);
		if (update_vertex_env) m_vertex_env_buffer->reserve_storage_on_heap(256);
		if (update_fragment_texture_env) m_texture_parameters_buffer->reserve_storage_on_heap(256);
		if (update_fragment_constants) m_fragment_constants_buffer->reserve_storage_on_heap(utils::align(fragment_constants_size, 256));
		if (update_transform_constants) m_transform_constants_buffer->reserve_storage_on_heap(8192);
		if (update_raster_env) m_raster_env_ring_buffer->reserve_storage_on_heap(128);

		if (update_instruction_buffers)
		{
			m_vertex_instructions_buffer->reserve_storage_on_heap(513 * 16);
			m_fragment_instructions_buffer->reserve_storage_on_heap(current_fp_metadata.program_ucode_length);
		}
	}

	if (update_vertex_env)
	{
		// Vertex state
		auto mapping = m_vertex_env_buffer->alloc_from_heap(144, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_scale_offset_data(buf, false);
		fill_user_clip_data(buf + 64);
		*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
		*(reinterpret_cast<f32*>(buf + 132)) = rsx::method_registers.point_size() * rsx::get_resolution_scale();
		*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.clip_min();
		*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_max();

		m_vertex_env_buffer->bind_range(GL_VERTEX_PARAMS_BIND_SLOT, mapping.second, 144);
	}

	if (update_transform_constants)
	{
		// Vertex constants
		const usz transform_constants_size = (!m_vertex_prog || m_vertex_prog->has_indexed_constants) ? 8192 : m_vertex_prog->constant_ids.size() * 16;
		if (transform_constants_size)
		{
			auto mapping = m_transform_constants_buffer->alloc_from_heap(static_cast<u32>(transform_constants_size), m_uniform_buffer_offset_align);
			auto buf = static_cast<u8*>(mapping.first);

			const auto constant_ids = (transform_constants_size == 8192)
				? std::span<const u16>{}
				: std::span<const u16>(m_vertex_prog->constant_ids);
			fill_vertex_program_constants_data(buf, constant_ids);

			m_transform_constants_buffer->bind_range(GL_VERTEX_CONSTANT_BUFFERS_BIND_SLOT, mapping.second, static_cast<u32>(transform_constants_size));
		}
	}

	if (update_fragment_constants && !update_instruction_buffers)
	{
		// Fragment constants
		auto mapping = m_fragment_constants_buffer->alloc_from_heap(fragment_constants_size, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);

		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), fragment_constants_size },
			*ensure(m_fragment_prog), current_fragment_program, true);

		m_fragment_constants_buffer->bind_range(GL_FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, mapping.second, fragment_constants_size);
	}

	if (update_fragment_env)
	{
		// Fragment state
		auto mapping = m_fragment_env_buffer->alloc_from_heap(32, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_fragment_state_buffer(buf, current_fragment_program);

		m_fragment_env_buffer->bind_range(GL_FRAGMENT_STATE_BIND_SLOT, mapping.second, 32);
	}

	if (update_fragment_texture_env)
	{
		// Fragment texture parameters
		auto mapping = m_texture_parameters_buffer->alloc_from_heap(768, m_uniform_buffer_offset_align);
		current_fragment_program.texture_params.write_to(mapping.first, current_fp_metadata.referenced_textures_mask);

		m_texture_parameters_buffer->bind_range(GL_FRAGMENT_TEXTURE_PARAMS_BIND_SLOT, mapping.second, 768);
	}

	if (update_raster_env)
	{
		auto mapping = m_raster_env_ring_buffer->alloc_from_heap(128, m_uniform_buffer_offset_align);

		std::memcpy(mapping.first, rsx::method_registers.polygon_stipple_pattern(), 128);
		m_raster_env_ring_buffer->bind_range(GL_RASTERIZER_STATE_BIND_SLOT, mapping.second, 128);

		m_graphics_state.clear(rsx::pipeline_state::polygon_stipple_pattern_dirty);
	}

	if (update_instruction_buffers)
	{
		if (m_interpreter_state & rsx::vertex_program_dirty)
		{
			// Attach vertex buffer data
			const auto vp_block_length = current_vp_metadata.ucode_length + 16;
			auto vp_mapping = m_vertex_instructions_buffer->alloc_from_heap(vp_block_length, 16);
			auto vp_buf = static_cast<u8*>(vp_mapping.first);

			auto vp_config = reinterpret_cast<u32*>(vp_buf);
			vp_config[0] = current_vertex_program.base_address;
			vp_config[1] = current_vertex_program.entry;
			vp_config[2] = current_vertex_program.output_mask;
			vp_config[3] = rsx::method_registers.two_side_light_en() ? 1u : 0u;

			std::memcpy(vp_buf + 16, current_vertex_program.data.data(), current_vp_metadata.ucode_length);

			m_vertex_instructions_buffer->bind_range(GL_INTERPRETER_VERTEX_BLOCK, vp_mapping.second, vp_block_length);
			m_vertex_instructions_buffer->notify();
		}

		if (m_interpreter_state & rsx::fragment_program_dirty)
		{
			// Attach fragment buffer data
			const auto fp_block_length = current_fp_metadata.program_ucode_length + 80;
			auto fp_mapping = m_fragment_instructions_buffer->alloc_from_heap(fp_block_length, 16);
			auto fp_buf = static_cast<u8*>(fp_mapping.first);

			// Control mask
			const auto control_masks = reinterpret_cast<u32*>(fp_buf);
			control_masks[0] = rsx::method_registers.shader_control();
			control_masks[1] = current_fragment_program.texture_state.texture_dimensions;

			// Bind textures
			m_shader_interpreter.update_fragment_textures(fs_sampler_state, current_fp_metadata.referenced_textures_mask, reinterpret_cast<u32*>(fp_buf + 16));

			std::memcpy(fp_buf + 80, current_fragment_program.get_data(), current_fragment_program.ucode_length);

			m_fragment_instructions_buffer->bind_range(GL_INTERPRETER_FRAGMENT_BLOCK, fp_mapping.second, fp_block_length);
			m_fragment_instructions_buffer->notify();
		}
	}

	if (manually_flush_ring_buffers)
	{
		if (update_fragment_env) m_fragment_env_buffer->unmap();
		if (update_vertex_env) m_vertex_env_buffer->unmap();
		if (update_fragment_texture_env) m_texture_parameters_buffer->unmap();
		if (update_fragment_constants) m_fragment_constants_buffer->unmap();
		if (update_transform_constants) m_transform_constants_buffer->unmap();
		if (update_raster_env) m_raster_env_ring_buffer->unmap();

		if (update_instruction_buffers)
		{
			m_vertex_instructions_buffer->unmap();
			m_fragment_instructions_buffer->unmap();
		}
	}

	m_graphics_state.clear(
		rsx::pipeline_state::fragment_state_dirty |
		rsx::pipeline_state::vertex_state_dirty |
		rsx::pipeline_state::transform_constants_dirty |
		rsx::pipeline_state::fragment_constants_dirty |
		rsx::pipeline_state::fragment_texture_state_dirty);
}

void GLGSRender::update_vertex_env(const gl::vertex_upload_info& upload_info)
{
	if (manually_flush_ring_buffers)
	{
		m_vertex_layout_buffer->reserve_storage_on_heap(128 + 16);
	}

	// Vertex layout state
	auto mapping = m_vertex_layout_buffer->alloc_from_heap(128 + 16, m_uniform_buffer_offset_align);
	auto buf = static_cast<u32*>(mapping.first);

	buf[0] = upload_info.vertex_index_base;
	buf[1] = upload_info.vertex_index_offset;
	buf += 4;

	fill_vertex_layout_state(m_vertex_layout, upload_info.first_vertex, upload_info.allocated_vertex_count, reinterpret_cast<s32*>(buf), upload_info.persistent_mapping_offset, upload_info.volatile_mapping_offset);

	m_vertex_layout_buffer->bind_range(GL_VERTEX_LAYOUT_BIND_SLOT, mapping.second, 128 + 16);

	if (manually_flush_ring_buffers)
	{
		m_vertex_layout_buffer->unmap();
	}
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	const bool can_flush = is_current_thread();
	const rsx::invalidation_cause cause =
		is_writing ? (can_flush ? rsx::invalidation_cause::write : rsx::invalidation_cause::deferred_write)
		           : (can_flush ? rsx::invalidation_cause::read  : rsx::invalidation_cause::deferred_read);

	auto cmd = can_flush ? gl::command_context{ gl_state } : gl::command_context{};
	auto result = m_gl_texture_cache.invalidate_address(cmd, address, cause);

	if (result.invalidate_samplers)
	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (!result.violation_handled)
	{
		return zcull_ctrl->on_access_violation(address);
	}

	if (result.num_flushable > 0)
	{
		auto &task = post_flush_request(address, result);

		m_eng_interrupt_mask |= rsx::backend_interrupt;
		vm::temporary_unlock();
		task.producer_wait();
	}

	return true;
}

void GLGSRender::on_invalidate_memory_range(const utils::address_range &range, rsx::invalidation_cause cause)
{
	gl::command_context cmd{ gl_state };
	auto data = m_gl_texture_cache.invalidate_range(cmd, range, cause);
	AUDIT(data.empty());

	if (cause == rsx::invalidation_cause::unmap && data.violation_handled)
	{
		m_gl_texture_cache.purge_unreleased_sections();
		{
			std::lock_guard lock(m_sampler_mutex);
			m_samplers_dirty.store(true);
		}
	}
}

void GLGSRender::on_semaphore_acquire_wait()
{
	if (!work_queue.empty() ||
		(async_flip_requested & flip_request::emu_requested))
	{
		do_local_task(rsx::FIFO::state::lock_wait);
	}
}

void GLGSRender::do_local_task(rsx::FIFO::state state)
{
	if (!work_queue.empty())
	{
		std::lock_guard lock(queue_guard);

		work_queue.remove_if([](auto &q) { return q.received; });

		for (auto& q : work_queue)
		{
			if (q.processed) continue;

			gl::command_context cmd{ gl_state };
			q.result = m_gl_texture_cache.flush_all(cmd, q.section_data);
			q.processed = true;
		}
	}
	else if (!in_begin_end && state != rsx::FIFO::state::lock_wait)
	{
		if (m_graphics_state & rsx::pipeline_state::framebuffer_reads_dirty)
		{
			//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
			//Only call when there are no waiters
			m_gl_texture_cache.do_update();
			m_graphics_state.clear(rsx::pipeline_state::framebuffer_reads_dirty);
		}
	}

	rsx::thread::do_local_task(state);

	if (state == rsx::FIFO::state::lock_wait)
	{
		// Critical check finished
		return;
	}

	if (m_overlay_manager)
	{
		if (!in_begin_end && async_flip_requested & flip_request::native_ui && !is_stopped())
		{
			rsx::display_flip_info_t info{};
			info.buffer = current_display_buffer;
			flip(info);
		}
	}
}

gl::work_item& GLGSRender::post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data)
{
	std::lock_guard lock(queue_guard);

	auto &result = work_queue.emplace_back();
	result.address_to_flush = address;
	result.section_data = std::move(flush_data);
	return result;
}

bool GLGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	gl::command_context cmd{ gl_state };
	if (m_gl_texture_cache.blit(cmd, src, dst, interpolate, m_rtts))
	{
		m_samplers_dirty.store(true);
		return true;
	}

	return false;
}

void GLGSRender::notify_tile_unbound(u32 tile)
{
	// TODO: Handle texture writeback
	if (false)
	{
		u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location);
		on_notify_memory_unmapped(addr, tiles[tile].size);
		m_rtts.invalidate_surface_address(addr, false);
	}

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void GLGSRender::begin_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	query->result = 0;
	glBeginQuery(m_occlusion_type, query->driver_handle);
}

void GLGSRender::end_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	ensure(query->active);
	glEndQuery(m_occlusion_type);
}

bool GLGSRender::check_occlusion_query_status(rsx::reports::occlusion_query_info* query)
{
	if (!query->num_draws)
		return true;

	GLint status = GL_TRUE;
	glGetQueryObjectiv(query->driver_handle, GL_QUERY_RESULT_AVAILABLE, &status);

	return status != GL_FALSE;
}

void GLGSRender::get_occlusion_query_result(rsx::reports::occlusion_query_info* query)
{
	if (query->num_draws)
	{
		GLint result = 0;
		glGetQueryObjectiv(query->driver_handle, GL_QUERY_RESULT, &result);

		query->result += result;
	}
}

void GLGSRender::discard_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	if (query->active)
	{
		//Discard is being called on an active query, close it
		glEndQuery(m_occlusion_type);
	}
}
