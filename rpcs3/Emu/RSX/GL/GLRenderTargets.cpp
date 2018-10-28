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
	if (m_framebuffer_state_contested && (m_framebuffer_contest_type != context))
	{
		// Clear commands affect contested memory
		m_rtts_dirty = true;
	}

	if (m_draw_fbo && !m_rtts_dirty)
	{
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
		for (u32 index = 0; index < 4; index++)
		{
			if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
			{
				surface->write_aa_mode = layout.aa_mode;
			}
		}

		if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil))
		{
			ds->write_aa_mode = layout.aa_mode;
		}

		return;
	}

	m_rtts.prepare_render_target(nullptr, layout.color_format, layout.depth_format, layout.width, layout.height,
		layout.target, layout.color_addresses, layout.zeta_address);

	bool old_format_found = false;
	gl::texture::format old_format;

	std::array<GLuint, 4> color_targets;
	GLuint depth_stencil_target;

	const auto color_offsets = get_offsets();
	const auto color_locations = get_locations();

	for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		if (m_surface_info[i].pitch && g_cfg.video.write_color_buffers)
		{
			if (!old_format_found)
			{
				old_format = rsx::internals::surface_color_format_to_gl(m_surface_info[i].color_format).format;
				old_format_found = true;
			}

			const utils::address_range surface_range = m_surface_info[i].get_memory_range();
			m_gl_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_gl_texture_cache.flush_if_cache_miss_likely(old_format, surface_range);
		}

		if (std::get<0>(m_rtts.m_bound_render_targets[i]))
		{
			auto rtt = std::get<1>(m_rtts.m_bound_render_targets[i]);
			color_targets[i] = rtt->id();

			rtt->set_rsx_pitch(layout.color_pitch[i]);
			m_surface_info[i] = { layout.color_addresses[i], layout.actual_color_pitch[i], false, layout.color_format, layout.depth_format, layout.width, layout.height };

			rtt->tile = find_tile(color_offsets[i], color_locations[i]);
			rtt->write_aa_mode = layout.aa_mode;
			m_gl_texture_cache.notify_surface_changed(m_surface_info[i].address);
			m_gl_texture_cache.tag_framebuffer(m_surface_info[i].address);
		}
		else
		{
			color_targets[i] = GL_NONE;
			m_surface_info[i] = {};
		}
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil))
	{
		if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
		{
			auto bpp = m_depth_surface_info.pitch / m_depth_surface_info.width;
			auto old_format = (bpp == 2) ? gl::texture::format::depth : gl::texture::format::depth_stencil;

			const utils::address_range surface_range = m_depth_surface_info.get_memory_range();
			m_gl_texture_cache.set_memory_read_flags(surface_range, rsx::memory_read_flags::flush_once);
			m_gl_texture_cache.flush_if_cache_miss_likely(old_format, surface_range);
		}

		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		depth_stencil_target = ds->id();

		std::get<1>(m_rtts.m_bound_depth_stencil)->set_rsx_pitch(rsx::method_registers.surface_z_pitch());
		m_depth_surface_info = { layout.zeta_address, layout.actual_zeta_pitch, true, layout.color_format, layout.depth_format, layout.width, layout.height };

		ds->write_aa_mode = layout.aa_mode;
		m_gl_texture_cache.notify_surface_changed(layout.zeta_address);
		m_gl_texture_cache.tag_framebuffer(layout.zeta_address);
	}
	else
	{
		depth_stencil_target = GL_NONE;
		m_depth_surface_info = {};
	}

	framebuffer_status_valid = false;

	for (auto &fbo : m_framebuffer_cache)
	{
		if (fbo.matches(color_targets, depth_stencil_target))
		{
			fbo.reset_refs();

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

	//Mark buffer regions as NO_ACCESS on Cell visible side
	if (g_cfg.video.write_color_buffers)
	{
		auto color_format = rsx::internals::surface_color_format_to_gl(layout.color_format);

		for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			if (!m_surface_info[i].address || !m_surface_info[i].pitch) continue;

			const utils::address_range surface_range = m_surface_info[i].get_memory_range(layout.aa_factors[1]);
			m_gl_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_render_targets[i]), surface_range, m_surface_info[i].width, m_surface_info[i].height, m_surface_info[i].pitch,
			color_format.format, color_format.type, color_format.swap_bytes);
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.address && m_depth_surface_info.pitch)
		{
			const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(layout.depth_format);
			const utils::address_range surface_range = m_depth_surface_info.get_memory_range(layout.aa_factors[1]);
			m_gl_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_depth_stencil), surface_range, m_depth_surface_info.width, m_depth_surface_info.height, m_depth_surface_info.pitch,
				depth_format_gl.format, depth_format_gl.type, true);
		}
	}

	if (m_gl_texture_cache.get_ro_tex_invalidate_intr())
	{
		//Invalidate cached sampler state
		m_samplers_dirty.store(true);
	}
}

std::array<std::vector<gsl::byte>, 4> GLGSRender::copy_render_targets_to_memory()
{
	int clip_w = rsx::method_registers.surface_clip_width();
	int clip_h = rsx::method_registers.surface_clip_height();
	return m_rtts.get_render_targets_data(rsx::method_registers.surface_color(), clip_w, clip_h);
}

std::array<std::vector<gsl::byte>, 2> GLGSRender::copy_depth_stencil_buffer_to_memory()
{
	int clip_w = rsx::method_registers.surface_clip_width();
	int clip_h = rsx::method_registers.surface_clip_height();
	return m_rtts.get_depth_stencil_data(rsx::method_registers.surface_depth_fmt(), clip_w, clip_h);
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
						m_gl_texture_cache.invalidate_range(range, rsx::invalidation_cause::read);

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
