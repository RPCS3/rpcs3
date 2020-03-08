#include "stdafx.h"
#include "GLGSRender.h"

color_format rsx::internals::surface_color_format_to_gl(rsx::surface_color_format color_format)
{
	//color format
	switch (color_format)
	{
	case rsx::surface_color_format::r5g6b5:
		return{ ::gl::texture::type::ushort_5_6_5, ::gl::texture::format::rgb, ::gl::texture::internal_format::rgb565, true };

	case rsx::surface_color_format::a8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, ::gl::texture::internal_format::rgba8, false };

	//These formats discard their alpha component, forced to 0 or 1
	//All XBGR formats will have remapping before they can be read back in shaders as DRGB8
	//Prefix o = 1, z = 0
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		return{ ::gl::texture::type::ushort_5_5_5_1, ::gl::texture::format::rgb, ::gl::texture::internal_format::rgb5a1, true,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		return{ ::gl::texture::type::ushort_5_5_5_1, ::gl::texture::format::rgb, ::gl::texture::internal_format::rgb5a1, true,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, ::gl::texture::internal_format::rgba8, false,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::rgba, ::gl::texture::internal_format::rgba8, false,
		{ ::gl::texture::channel::one, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::rgba, ::gl::texture::internal_format::rgba8, false,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, ::gl::texture::internal_format::rgba8, false,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::w16z16y16x16:
		return{ ::gl::texture::type::f16, ::gl::texture::format::rgba, ::gl::texture::internal_format::rgba16f, true};

	case rsx::surface_color_format::w32z32y32x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::rgba, ::gl::texture::internal_format::rgba32f, true};

	case rsx::surface_color_format::b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::r, ::gl::texture::internal_format::r8, false,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::r, ::gl::texture::channel::r } };

	case rsx::surface_color_format::g8b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::rg, ::gl::texture::internal_format::rg8, false,
		{ ::gl::texture::channel::g, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::r, ::gl::texture::internal_format::r32f, true,
		{ ::gl::texture::channel::r, ::gl::texture::channel::r, ::gl::texture::channel::r, ::gl::texture::channel::r } };

	case rsx::surface_color_format::a8b8g8r8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::rgba, ::gl::texture::internal_format::rgba8, false,
		{ ::gl::texture::channel::a, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	default:
		fmt::throw_exception("Unsupported surface color format 0x%x" HERE, static_cast<u32>(color_format));
	}
}

depth_format rsx::internals::surface_depth_format_to_gl(rsx::surface_depth_format depth_format)
{
	switch (depth_format)
	{
	case rsx::surface_depth_format::z16:
		return{ ::gl::texture::type::ushort, ::gl::texture::format::depth, ::gl::texture::internal_format::depth16 };

	case rsx::surface_depth_format::z24s8:
		if (g_cfg.video.force_high_precision_z_buffer && ::gl::get_driver_caps().ARB_depth_buffer_float_supported)
			return{ ::gl::texture::type::uint_24_8, ::gl::texture::format::depth_stencil, ::gl::texture::internal_format::depth32f_stencil8 };
		else
			return{ ::gl::texture::type::uint_24_8, ::gl::texture::format::depth_stencil, ::gl::texture::internal_format::depth24_stencil8 };

	default:
		fmt::throw_exception("Unsupported depth format 0x%x" HERE, static_cast<u32>(depth_format));
	}
}

u8 rsx::internals::get_pixel_size(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return 2;
	case rsx::surface_depth_format::z24s8: return 4;
	}
	fmt::throw_exception("Unknown depth format" HERE);
}

namespace
{
	std::array<u32, 4> get_offsets()
	{
		return{
			rsx::method_registers.surface_a_offset(),
			rsx::method_registers.surface_b_offset(),
			rsx::method_registers.surface_c_offset(),
			rsx::method_registers.surface_d_offset(),
		};
	}

	std::array<u32, 4> get_locations()
	{
		return{
			rsx::method_registers.surface_a_dma(),
			rsx::method_registers.surface_b_dma(),
			rsx::method_registers.surface_c_dma(),
			rsx::method_registers.surface_d_dma(),
		};
	}

	std::array<u32, 4> get_pitchs()
	{
		return{
			rsx::method_registers.surface_a_pitch(),
			rsx::method_registers.surface_b_pitch(),
			rsx::method_registers.surface_c_pitch(),
			rsx::method_registers.surface_d_pitch(),
		};
	}
}

void GLGSRender::init_buffers(rsx::framebuffer_creation_context context, bool skip_reading)
{
	const bool clipped_scissor = (context == rsx::framebuffer_creation_context::context_draw);
	if (m_current_framebuffer_context == context && !m_rtts_dirty && m_draw_fbo)
	{
		// Fast path
		// Framebuffer usage has not changed, framebuffer exists and config regs have not changed
		set_scissor(clipped_scissor);
		return;
	}

	m_rtts_dirty = false;
	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	get_framebuffer_layout(context, m_framebuffer_layout);
	if (!framebuffer_status_valid)
	{
		return;
	}

	if (m_draw_fbo && m_framebuffer_layout.ignore_change)
	{
		// Nothing has changed, we're still using the same framebuffer
		// Update flags to match current
		m_draw_fbo->bind();
		set_viewport();
		set_scissor(clipped_scissor);

		return;
	}

	gl::command_context cmd{ gl_state };
	m_rtts.prepare_render_target(cmd,
		m_framebuffer_layout.color_format, m_framebuffer_layout.depth_format,
		m_framebuffer_layout.width, m_framebuffer_layout.height,
		m_framebuffer_layout.target, m_framebuffer_layout.aa_mode,
		m_framebuffer_layout.color_addresses, m_framebuffer_layout.zeta_address,
		m_framebuffer_layout.actual_color_pitch, m_framebuffer_layout.actual_zeta_pitch);

	std::array<GLuint, 4> color_targets;
	GLuint depth_stencil_target;

	const u8 color_bpp = get_format_block_size_in_bytes(m_framebuffer_layout.color_format);
	const auto samples = get_format_sample_count(m_framebuffer_layout.aa_mode);

	for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		if (m_surface_info[i].pitch && g_cfg.video.write_color_buffers)
		{
			const utils::address_range surface_range = m_surface_info[i].get_memory_range();
			m_gl_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_gl_texture_cache.flush_if_cache_miss_likely(cmd, surface_range);
		}

		if (std::get<0>(m_rtts.m_bound_render_targets[i]))
		{
			auto rtt = std::get<1>(m_rtts.m_bound_render_targets[i]);
			color_targets[i] = rtt->id();

			verify("Pitch mismatch!" HERE), rtt->get_rsx_pitch() == m_framebuffer_layout.actual_color_pitch[i];
			m_surface_info[i].address = m_framebuffer_layout.color_addresses[i];
			m_surface_info[i].pitch = m_framebuffer_layout.actual_color_pitch[i];
			m_surface_info[i].width = m_framebuffer_layout.width;
			m_surface_info[i].height = m_framebuffer_layout.height;
			m_surface_info[i].color_format = m_framebuffer_layout.color_format;
			m_surface_info[i].bpp = color_bpp;
			m_surface_info[i].samples = samples;
			m_gl_texture_cache.notify_surface_changed(m_surface_info[i].get_memory_range(m_framebuffer_layout.aa_factors));
		}
		else
		{
			color_targets[i] = GL_NONE;
			m_surface_info[i] = {};
		}
	}

	if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
	{
		const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
		m_gl_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
		m_gl_texture_cache.flush_if_cache_miss_likely(cmd, surface_range);
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil))
	{
		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		depth_stencil_target = ds->id();
		ds->set_depth_render_mode(!m_framebuffer_layout.depth_float);

		verify("Pitch mismatch!" HERE), std::get<1>(m_rtts.m_bound_depth_stencil)->get_rsx_pitch() == m_framebuffer_layout.actual_zeta_pitch;

		m_depth_surface_info.address = m_framebuffer_layout.zeta_address;
		m_depth_surface_info.pitch = m_framebuffer_layout.actual_zeta_pitch;
		m_depth_surface_info.width = m_framebuffer_layout.width;
		m_depth_surface_info.height = m_framebuffer_layout.height;
		m_depth_surface_info.depth_format = m_framebuffer_layout.depth_format;
		m_depth_surface_info.depth_buffer_float = m_framebuffer_layout.depth_float;
		m_depth_surface_info.bpp = (m_framebuffer_layout.depth_format == rsx::surface_depth_format::z16? 2 : 4);
		m_depth_surface_info.samples = samples;

		m_gl_texture_cache.notify_surface_changed(m_depth_surface_info.get_memory_range(m_framebuffer_layout.aa_factors));
	}
	else
	{
		depth_stencil_target = GL_NONE;
		m_depth_surface_info = {};
	}

	framebuffer_status_valid = false;

	if (m_draw_fbo)
	{
		// Release resource
		static_cast<gl::framebuffer_holder*>(m_draw_fbo)->release();
	}

	for (auto &fbo : m_framebuffer_cache)
	{
		if (fbo.matches(color_targets, depth_stencil_target))
		{
			fbo.add_ref();

			m_draw_fbo = &fbo;
			m_draw_fbo->bind();
			m_draw_fbo->set_extents({ m_framebuffer_layout.width, m_framebuffer_layout.height });

			framebuffer_status_valid = true;
			break;
		}
	}

	if (!framebuffer_status_valid)
	{
		m_framebuffer_cache.emplace_back();
		m_framebuffer_cache.back().add_ref();

		m_draw_fbo = &m_framebuffer_cache.back();
		m_draw_fbo->create();
		m_draw_fbo->bind();
		m_draw_fbo->set_extents({ m_framebuffer_layout.width, m_framebuffer_layout.height });

		for (int i = 0; i < 4; ++i)
		{
			if (color_targets[i])
			{
				m_draw_fbo->color[i] = color_targets[i];
			}
		}

		if (depth_stencil_target)
		{
			if (m_framebuffer_layout.depth_format == rsx::surface_depth_format::z24s8)
			{
				m_draw_fbo->depth_stencil = depth_stencil_target;
			}
			else
			{
				m_draw_fbo->depth = depth_stencil_target;
			}
		}
	}

	switch (rsx::method_registers.surface_color_target())
	{
	case rsx::surface_target::none: break;

	case rsx::surface_target::surface_a:
		m_draw_fbo->draw_buffer(m_draw_fbo->color[0]);
		m_draw_fbo->read_buffer(m_draw_fbo->color[0]);
		break;

	case rsx::surface_target::surface_b:
		m_draw_fbo->draw_buffer(m_draw_fbo->color[1]);
		m_draw_fbo->read_buffer(m_draw_fbo->color[1]);
		break;

	case rsx::surface_target::surfaces_a_b:
		m_draw_fbo->draw_buffers({ m_draw_fbo->color[0], m_draw_fbo->color[1] });
		m_draw_fbo->read_buffer(m_draw_fbo->color[0]);
		break;

	case rsx::surface_target::surfaces_a_b_c:
		m_draw_fbo->draw_buffers({ m_draw_fbo->color[0], m_draw_fbo->color[1], m_draw_fbo->color[2] });
		m_draw_fbo->read_buffer(m_draw_fbo->color[0]);
		break;

	case rsx::surface_target::surfaces_a_b_c_d:
		m_draw_fbo->draw_buffers({ m_draw_fbo->color[0], m_draw_fbo->color[1], m_draw_fbo->color[2], m_draw_fbo->color[3] });
		m_draw_fbo->read_buffer(m_draw_fbo->color[0]);
		break;
	}

	framebuffer_status_valid = m_draw_fbo->check();
	if (!framebuffer_status_valid) return;

	check_zcull_status(true);
	set_viewport();
	set_scissor(clipped_scissor);

	m_gl_texture_cache.clear_ro_tex_invalidate_intr();

	const auto color_format = rsx::internals::surface_color_format_to_gl(m_framebuffer_layout.color_format);
	for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		if (!m_surface_info[i].address || !m_surface_info[i].pitch) continue;

		const auto surface_range = m_surface_info[i].get_memory_range();
		if (g_cfg.video.write_color_buffers)
		{
			// Mark buffer regions as NO_ACCESS on Cell-visible side
			m_gl_texture_cache.lock_memory_region(
				cmd, m_rtts.m_bound_render_targets[i].second, surface_range, true,
				m_surface_info[i].width, m_surface_info[i].height, m_surface_info[i].pitch,
				color_format.format, color_format.type, color_format.swap_bytes);
		}
		else
		{
			m_gl_texture_cache.commit_framebuffer_memory_region(cmd, surface_range);
		}
	}

	if (m_depth_surface_info.address && m_depth_surface_info.pitch)
	{
		const auto surface_range = m_depth_surface_info.get_memory_range();
		if (g_cfg.video.write_depth_buffer)
		{
			const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(m_framebuffer_layout.depth_format);
			m_gl_texture_cache.lock_memory_region(
				cmd, m_rtts.m_bound_depth_stencil.second, surface_range, true,
				m_depth_surface_info.width, m_depth_surface_info.height, m_depth_surface_info.pitch,
				depth_format_gl.format, depth_format_gl.type, true);
		}
		else
		{
			m_gl_texture_cache.commit_framebuffer_memory_region(cmd, surface_range);
		}
	}

	if (!m_rtts.orphaned_surfaces.empty())
	{
		gl::texture::format format;
		gl::texture::type type;
		bool swap_bytes;

		for (auto& surface : m_rtts.orphaned_surfaces)
		{
			const bool lock = surface->is_depth_surface() ? !!g_cfg.video.write_depth_buffer :
				!!g_cfg.video.write_color_buffers;

			if (!lock) [[likely]]
			{
				m_gl_texture_cache.commit_framebuffer_memory_region(cmd, surface->get_memory_range());
				continue;
			}

			if (surface->is_depth_surface())
			{
				const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(surface->get_surface_depth_format());
				format = depth_format_gl.format;
				type = depth_format_gl.type;
				swap_bytes = (type != gl::texture::type::uint_24_8);
			}
			else
			{
				const auto color_format_gl = rsx::internals::surface_color_format_to_gl(surface->get_surface_color_format());
				format = color_format_gl.format;
				type = color_format_gl.type;
				swap_bytes = color_format_gl.swap_bytes;
			}

			m_gl_texture_cache.lock_memory_region(
				cmd, surface, surface->get_memory_range(), false,
				surface->get_surface_width(rsx::surface_metrics::pixels), surface->get_surface_height(rsx::surface_metrics::pixels), surface->get_rsx_pitch(),
				format, type, swap_bytes);
		}

		m_rtts.orphaned_surfaces.clear();
	}

	if (m_gl_texture_cache.get_ro_tex_invalidate_intr())
	{
		// Invalidate cached sampler state
		m_samplers_dirty.store(true);
	}
}

std::array<std::vector<std::byte>, 4> GLGSRender::copy_render_targets_to_memory()
{
	return {};
}

std::array<std::vector<std::byte>, 2> GLGSRender::copy_depth_stencil_buffer_to_memory()
{
	return {};
}

// Render target helpers
void gl::render_target::clear_memory(gl::command_context& cmd)
{
	if (aspect() & gl::image_aspect::depth)
	{
		gl::g_hw_blitter->fast_clear_image(cmd, this, 1.f, 255);
	}
	else
	{
		gl::g_hw_blitter->fast_clear_image(cmd, this, {});
	}

	state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
}

void gl::render_target::load_memory(gl::command_context& cmd)
{
	const u32 gcm_format = is_depth_surface() ?
		get_compatible_gcm_format(format_info.gcm_depth_format).first :
		get_compatible_gcm_format(format_info.gcm_color_format).first;

	rsx_subresource_layout subres{};
	subres.width_in_block = subres.width_in_texel = surface_width * samples_x;
	subres.height_in_block = subres.height_in_texel = surface_height * samples_y;
	subres.pitch_in_block = rsx_pitch / get_bpp();
	subres.depth = 1;
	subres.data = { vm::get_super_ptr<const std::byte>(base_addr), static_cast<gsl::span<const std::byte>::index_type>(rsx_pitch * surface_height * samples_y) };

	// TODO: MSAA support
	if (g_cfg.video.resolution_scale_percent == 100 && spp == 1) [[likely]]
	{
		gl::upload_texture(id(), gcm_format, surface_width, surface_height, 1, 1,
			false, rsx::texture_dimension_extended::texture_dimension_2d, { subres });
	}
	else
	{
		auto tmp = std::make_unique<gl::texture>(GL_TEXTURE_2D, subres.width_in_block, subres.height_in_block, 1, 1, static_cast<GLenum>(get_internal_format()));
		gl::upload_texture(tmp->id(), gcm_format, surface_width, surface_height, 1, 1,
			false, rsx::texture_dimension_extended::texture_dimension_2d, { subres });

		gl::g_hw_blitter->scale_image(cmd, tmp.get(), this,
			{ 0, 0, subres.width_in_block, subres.height_in_block },
			{ 0, 0, static_cast<int>(width()), static_cast<int>(height()) },
			!is_depth_surface(),
			{});
	}
}

void gl::render_target::initialize_memory(gl::command_context& cmd, bool /*read_access*/)
{
	const bool memory_load = is_depth_surface() ?
		!!g_cfg.video.read_depth_buffer :
		!!g_cfg.video.read_color_buffers;

	if (!memory_load)
	{
		clear_memory(cmd);
	}
	else
	{
		load_memory(cmd);
	}
}

void gl::render_target::memory_barrier(gl::command_context& cmd, rsx::surface_access access)
{
	const bool read_access = (access != rsx::surface_access::write);

	if (old_contents.empty())
	{
		// No memory to inherit
		if (dirty() && (read_access || state_flags & rsx::surface_state_flags::erase_bkgnd))
		{
			// Initialize memory contents if we did not find anything usable
			initialize_memory(cmd, true);
			on_write();
		}

		return;
	}

	const bool dst_is_depth = !!(aspect() & gl::image_aspect::depth);
	const auto dst_bpp = get_bpp();
	unsigned first = prepare_rw_barrier_for_transfer(this);
	u64 newest_tag = 0;

	for (auto i = first; i < old_contents.size(); ++i)
	{
		auto &section = old_contents[i];
		auto src_texture = gl::as_rtt(section.source);
		const auto src_bpp = src_texture->get_bpp();
		rsx::typeless_xfer typeless_info{};

		if (get_internal_format() == src_texture->get_internal_format())
		{
			// Copy data from old contents onto this one
			verify(HERE), src_bpp == dst_bpp;
		}
		else
		{
			// Mem cast, generate typeless xfer info
			if (!formats_are_bitcast_compatible(static_cast<GLenum>(get_internal_format()), static_cast<GLenum>(src_texture->get_internal_format())) ||
				aspect() != src_texture->aspect())
			{
				typeless_info.src_is_typeless = true;
				typeless_info.src_context = rsx::texture_upload_context::framebuffer_storage;
				typeless_info.src_native_format_override = static_cast<u32>(get_internal_format());
				typeless_info.src_scaling_hint = static_cast<f32>(src_bpp) / dst_bpp;
			}
		}

		section.init_transfer(this);

		if (state_flags & rsx::surface_state_flags::erase_bkgnd)
		{
			const auto area = section.dst_rect();
			if (area.x1 > 0 || area.y1 > 0 || unsigned(area.x2) < width() || unsigned(area.y2) < height())
			{
				initialize_memory(cmd, false);
			}
			else
			{
				state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
			}
		}

		gl::g_hw_blitter->scale_image(cmd, section.source, this,
			section.src_rect(),
			section.dst_rect(),
			!dst_is_depth, typeless_info);

		newest_tag = src_texture->last_use_tag;
	}

	// Memory has been transferred, discard old contents and update memory flags
	// TODO: Preserve memory outside surface clip region
	on_write(newest_tag);
}
