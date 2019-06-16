#include "stdafx.h"
#include "../rsx_methods.h"
#include "GLGSRender.h"
#include "Emu/System.h"

color_format rsx::internals::surface_color_format_to_gl(rsx::surface_color_format color_format)
{
	//color format
	switch (color_format)
	{
	case rsx::surface_color_format::r5g6b5:
		return{ ::gl::texture::type::ushort_5_6_5, ::gl::texture::format::rgb, false, 3, 2 };

	case rsx::surface_color_format::a8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1 };

	//These formats discard their alpha component, forced to 0 or 1
	//All XBGR formats will have remapping before they can be read back in shaders as DRGB8
	//Prefix o = 1, z = 0
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::one, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::zero, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::w16z16y16x16:
		return{ ::gl::texture::type::f16, ::gl::texture::format::rgba, true, 4, 2 };

	case rsx::surface_color_format::w32z32y32x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::rgba, true, 4, 4 };

	case rsx::surface_color_format::b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::r, false, 1, 1,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::r, ::gl::texture::channel::r } };

	case rsx::surface_color_format::g8b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::rg, false, 2, 1,
		{ ::gl::texture::channel::g, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	case rsx::surface_color_format::x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::r, true, 1, 4 };

	case rsx::surface_color_format::a8b8g8r8:
		//NOTE: To sample this surface as ARGB8 (ABGR8 does not exist), swizzle will be applied by the application
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::a, ::gl::texture::channel::b, ::gl::texture::channel::g, ::gl::texture::channel::r } };

	default:
		LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", (u32)color_format);
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1 };
	}
}

depth_format rsx::internals::surface_depth_format_to_gl(rsx::surface_depth_format depth_format)
{
	switch (depth_format)
	{
	case rsx::surface_depth_format::z16:
		return{ ::gl::texture::type::ushort, ::gl::texture::format::depth, ::gl::texture::internal_format::depth16 };

	default:
		LOG_ERROR(RSX, "Surface depth buffer: Unsupported surface depth format (0x%x)", (u32)depth_format);
	case rsx::surface_depth_format::z24s8:
		if (g_cfg.video.force_high_precision_z_buffer && ::gl::get_driver_caps().ARB_depth_buffer_float_supported)
			return{ ::gl::texture::type::uint_24_8, ::gl::texture::format::depth_stencil, ::gl::texture::internal_format::depth32f_stencil8 };
		else
			return{ ::gl::texture::type::uint_24_8, ::gl::texture::format::depth_stencil, ::gl::texture::internal_format::depth24_stencil8 };
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

::gl::texture::internal_format rsx::internals::sized_internal_format(rsx::surface_color_format color_format)
{
	switch (color_format)
	{
	case rsx::surface_color_format::r5g6b5:
		return ::gl::texture::internal_format::r5g6b5;

	case rsx::surface_color_format::a8r8g8b8:
		return ::gl::texture::internal_format::rgba8;

	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		return ::gl::texture::internal_format::rgba8;

	case rsx::surface_color_format::w16z16y16x16:
		return ::gl::texture::internal_format::rgba16f;

	case rsx::surface_color_format::w32z32y32x32:
		return ::gl::texture::internal_format::rgba32f;

	case rsx::surface_color_format::b8:
		return ::gl::texture::internal_format::r8;

	case rsx::surface_color_format::g8b8:
		return ::gl::texture::internal_format::rg8;

	case rsx::surface_color_format::x32:
		return ::gl::texture::internal_format::r32f;

	case rsx::surface_color_format::a8b8g8r8:
		return ::gl::texture::internal_format::rgba8;

	default:
		LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", (u32)color_format);
		return ::gl::texture::internal_format::rgba8;
	}
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
	if (m_current_framebuffer_context == context && !m_rtts_dirty && m_draw_fbo)
	{
		// Fast path
		// Framebuffer usage has not changed, framebuffer exists and config regs have not changed
		set_scissor();
		return;
	}

	m_rtts_dirty = false;
	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	const auto layout = get_framebuffer_layout(context);
	if (!framebuffer_status_valid)
	{
		return;
	}

	if (m_draw_fbo && layout.ignore_change)
	{
		// Nothing has changed, we're still using the same framebuffer
		// Update flags to match current
		m_draw_fbo->bind();
		set_viewport();
		set_scissor();

		return;
	}

	gl::command_context cmd{ gl_state };
	m_rtts.prepare_render_target(cmd,
		layout.color_format, layout.depth_format,
		layout.width, layout.height,
		layout.target, layout.aa_mode,
		layout.color_addresses, layout.zeta_address,
		layout.actual_color_pitch, layout.actual_zeta_pitch);

	std::array<GLuint, 4> color_targets;
	GLuint depth_stencil_target;

	const u8 color_bpp = get_format_block_size_in_bytes(layout.color_format);
	const auto samples = get_format_sample_count(layout.aa_mode);

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

			verify("Pitch mismatch!" HERE), rtt->get_rsx_pitch() == layout.actual_color_pitch[i];
			m_surface_info[i].address = layout.color_addresses[i];
			m_surface_info[i].pitch = layout.actual_color_pitch[i];
			m_surface_info[i].width = layout.width;
			m_surface_info[i].height = layout.height;
			m_surface_info[i].color_format = layout.color_format;
			m_surface_info[i].bpp = color_bpp;
			m_surface_info[i].samples = samples;
			m_gl_texture_cache.notify_surface_changed(m_surface_info[i].get_memory_range(layout.aa_factors));
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

		verify("Pitch mismatch!" HERE), std::get<1>(m_rtts.m_bound_depth_stencil)->get_rsx_pitch() == layout.actual_zeta_pitch;

		m_depth_surface_info.address = layout.zeta_address;
		m_depth_surface_info.pitch = layout.actual_zeta_pitch;
		m_depth_surface_info.width = layout.width;
		m_depth_surface_info.height = layout.height;
		m_depth_surface_info.depth_format = layout.depth_format;
		m_depth_surface_info.bpp = (layout.depth_format == rsx::surface_depth_format::z16? 2 : 4);
		m_depth_surface_info.samples = samples;
		m_gl_texture_cache.notify_surface_changed(m_depth_surface_info.get_memory_range(layout.aa_factors));
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
			m_draw_fbo->set_extents({ (int)layout.width, (int)layout.height });

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
		m_draw_fbo->set_extents({ (int)layout.width, (int)layout.height });

		for (int i = 0; i < 4; ++i)
		{
			if (color_targets[i])
			{
				m_draw_fbo->color[i] = color_targets[i];
			}
		}

		if (depth_stencil_target)
		{
			if (layout.depth_format == rsx::surface_depth_format::z24s8)
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
	set_scissor();

	m_gl_texture_cache.clear_ro_tex_invalidate_intr();

	const auto color_format = rsx::internals::surface_color_format_to_gl(layout.color_format);
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
			const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(layout.depth_format);
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
		if (g_cfg.video.write_color_buffers || g_cfg.video.write_depth_buffer)
		{
			gl::texture::format format;
			gl::texture::type type;
			bool swap_bytes;

			for (auto& surface : m_rtts.orphaned_surfaces)
			{
				if (surface->is_depth_surface())
				{
					if (!g_cfg.video.write_depth_buffer) continue;

					const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(surface->get_surface_depth_format());
					format = depth_format_gl.format;
					type = depth_format_gl.type;
					swap_bytes = true;
				}
				else
				{
					if (!g_cfg.video.write_color_buffers) continue;

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
		}

		m_rtts.orphaned_surfaces.clear();
	}

	if (m_gl_texture_cache.get_ro_tex_invalidate_intr())
	{
		// Invalidate cached sampler state
		m_samplers_dirty.store(true);
	}
}

std::array<std::vector<gsl::byte>, 4> GLGSRender::copy_render_targets_to_memory()
{
	return {};
}

std::array<std::vector<gsl::byte>, 2> GLGSRender::copy_depth_stencil_buffer_to_memory()
{
	return {};
}

void GLGSRender::read_buffers()
{
	if (!m_draw_fbo)
		return;

	glDisable(GL_STENCIL_TEST);

	if (g_cfg.video.read_color_buffers)
	{
		auto color_format = rsx::internals::surface_color_format_to_gl(rsx::method_registers.surface_color());

		auto read_color_buffers = [&](int index, int count)
		{
			const u32 width = rsx::method_registers.surface_clip_width();
			const u32 height = rsx::method_registers.surface_clip_height();

			const std::array<u32, 4> offsets = get_offsets();
			const std::array<u32, 4 > locations = get_locations();
			const std::array<u32, 4 > pitchs = get_pitchs();

			for (int i = index; i < index + count; ++i)
			{
				const u32 offset = offsets[i];
				const u32 location = locations[i];
				const u32 pitch = pitchs[i];

				if (!m_surface_info[i].pitch)
					continue;

				rsx::tiled_region color_buffer = get_tiled_address(offset, location & 0xf);
				u32 texaddr = (u32)((u64)color_buffer.ptr - (u64)vm::base(0));

				const utils::address_range range = utils::address_range::start_length(texaddr, pitch * height);
				bool success = m_gl_texture_cache.load_memory_from_cache(range, std::get<1>(m_rtts.m_bound_render_targets[i]));

				//Fall back to slower methods if the image could not be fetched from cache.
				if (!success)
				{
					if (!color_buffer.tile)
					{
						std::get<1>(m_rtts.m_bound_render_targets[i])->copy_from(color_buffer.ptr, color_format.format, color_format.type);
					}
					else
					{
						std::unique_ptr<u8[]> buffer(new u8[pitch * height]);
						color_buffer.read(buffer.get(), width, height, pitch);

						std::get<1>(m_rtts.m_bound_render_targets[i])->copy_from(buffer.get(), color_format.format, color_format.type);
					}
				}
			}
		};

		switch (rsx::method_registers.surface_color_target())
		{
		case rsx::surface_target::none:
			break;

		case rsx::surface_target::surface_a:
			read_color_buffers(0, 1);
			break;

		case rsx::surface_target::surface_b:
			read_color_buffers(1, 1);
			break;

		case rsx::surface_target::surfaces_a_b:
			read_color_buffers(0, 2);
			break;

		case rsx::surface_target::surfaces_a_b_c:
			read_color_buffers(0, 3);
			break;

		case rsx::surface_target::surfaces_a_b_c_d:
			read_color_buffers(0, 4);
			break;
		}
	}

	if (g_cfg.video.read_depth_buffer)
	{
		//TODO: use pitch
		const u32 pitch = m_depth_surface_info.pitch;
		const u32 width = rsx::method_registers.surface_clip_width();
		const u32 height = rsx::method_registers.surface_clip_height();

		if (!pitch)
			return;

		const u32 depth_address = rsx::get_address(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());
		const utils::address_range range = utils::address_range::start_length(depth_address, pitch * height);
		bool in_cache = m_gl_texture_cache.load_memory_from_cache(range, std::get<1>(m_rtts.m_bound_depth_stencil));

		if (in_cache)
			return;

		//Read failed. Fall back to slow s/w path...

		auto depth_format = rsx::internals::surface_depth_format_to_gl(rsx::method_registers.surface_depth_fmt());
		int pixel_size    = rsx::internals::get_pixel_size(rsx::method_registers.surface_depth_fmt());
		gl::buffer pbo_depth;

		pbo_depth.create(width * height * pixel_size);
		pbo_depth.map([&](GLubyte* pixels)
		{
			u32 depth_address = rsx::get_address(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());

			if (rsx::method_registers.surface_depth_fmt() == rsx::surface_depth_format::z16)
			{
				u16 *dst = (u16*)pixels;
				const be_t<u16>* src = vm::_ptr<u16>(depth_address);
				for (int i = 0, end = std::get<1>(m_rtts.m_bound_depth_stencil)->width() * std::get<1>(m_rtts.m_bound_depth_stencil)->height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				u32 *dst = (u32*)pixels;
				const be_t<u32>* src = vm::_ptr<u32>(depth_address);
				for (int i = 0, end = std::get<1>(m_rtts.m_bound_depth_stencil)->width() * std::get<1>(m_rtts.m_bound_depth_stencil)->height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
		}, gl::buffer::access::write);

		std::get<1>(m_rtts.m_bound_depth_stencil)->copy_from(pbo_depth, depth_format.format, depth_format.type);
	}
}

void gl::render_target::memory_barrier(gl::command_context& cmd, bool force_init)
{
	auto clear_surface_impl = [&]()
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
	};

	if (!old_contents)
	{
		// No memory to inherit
		if (dirty() && (force_init || state_flags & rsx::surface_state_flags::erase_bkgnd))
		{
			// Initialize memory contents if we did not find anything usable
			// TODO: Properly sync with Cell
			clear_surface_impl();
			on_write();
		}

		return;
	}

	auto src_texture = gl::as_rtt(old_contents.source);
	if (!rsx::pitch_compatible(this, src_texture))
	{
		LOG_TRACE(RSX, "Pitch mismatch, could not transfer inherited memory");

		clear_rw_barrier();
		return;
	}

	const auto src_bpp = src_texture->get_bpp();
	const auto dst_bpp = get_bpp();
	rsx::typeless_xfer typeless_info{};

	if (get_internal_format() == src_texture->get_internal_format())
	{
		// Copy data from old contents onto this one
		verify(HERE), src_bpp == dst_bpp;
	}
	else
	{
		// Mem cast, generate typeless xfer info
		if (!formats_are_bitcast_compatible((GLenum)get_internal_format(), (GLenum)src_texture->get_internal_format()) ||
			aspect() != src_texture->aspect())
		{
			typeless_info.src_is_typeless = true;
			typeless_info.src_context = rsx::texture_upload_context::framebuffer_storage;
			typeless_info.src_native_format_override = (u32)get_internal_format();
			typeless_info.src_is_depth = !!(src_texture->aspect() & gl::image_aspect::depth);
			typeless_info.src_scaling_hint = f32(src_bpp) / dst_bpp;
		}
	}

	const bool dst_is_depth = !!(aspect() & gl::image_aspect::depth);
	old_contents.init_transfer(this);

	if (state_flags & rsx::surface_state_flags::erase_bkgnd)
	{
		const auto area = old_contents.dst_rect();
		if (area.x1 > 0 || area.y1 > 0 || unsigned(area.x2) < width() || unsigned(area.y2) < height())
		{
			clear_surface_impl();
		}
		else
		{
			state_flags &= ~rsx::surface_state_flags::erase_bkgnd;
		}
	}

	gl::g_hw_blitter->scale_image(cmd, old_contents.source, this,
		old_contents.src_rect(),
		old_contents.dst_rect(),
		!dst_is_depth, dst_is_depth, typeless_info);

	// Memory has been transferred, discard old contents and update memory flags
	// TODO: Preserve memory outside surface clip region
	on_write();
}
