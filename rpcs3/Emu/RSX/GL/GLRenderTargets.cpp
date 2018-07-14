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
	if (draw_fbo && !m_rtts_dirty)
	{
		set_viewport();
		return;
	}

	//We are about to change buffers, flush any pending requests for the old buffers
	synchronize_buffers();

	m_rtts_dirty = false;
	zcull_surface_active = false;

	const u16 clip_horizontal = rsx::method_registers.surface_clip_width();
	const u16 clip_vertical = rsx::method_registers.surface_clip_height();
	const u16 clip_x = rsx::method_registers.surface_clip_origin_x();
	const u16 clip_y = rsx::method_registers.surface_clip_origin_y();

	framebuffer_status_valid = false;
	m_framebuffer_state_contested = false;

	if (clip_horizontal == 0 || clip_vertical == 0)
	{
		LOG_ERROR(RSX, "Invalid framebuffer setup, w=%d, h=%d", clip_horizontal, clip_vertical);
		return;
	}

	auto surface_addresses = get_color_surface_addresses();
	auto depth_address = get_zeta_surface_address();

	const auto pitchs = get_pitchs();
	const auto zeta_pitch = rsx::method_registers.surface_z_pitch();
	const auto surface_format = rsx::method_registers.surface_color();
	const auto depth_format = rsx::method_registers.surface_depth_fmt();
	const auto target = rsx::method_registers.surface_color_target();

	const auto aa_mode = rsx::method_registers.surface_antialias();
	const u32 aa_factor_u = (aa_mode == rsx::surface_antialiasing::center_1_sample) ? 1 : 2;
	const u32 aa_factor_v = (aa_mode == rsx::surface_antialiasing::center_1_sample || aa_mode == rsx::surface_antialiasing::diagonal_centered_2_samples) ? 1 : 2;

	//NOTE: Its is possible that some renders are done on a swizzled context. Pitch is meaningless in that case
	//Seen in Nier (color) and GT HD concept (z buffer)
	//Restriction is that the dimensions are powers of 2. Also, dimensions are passed via log2w and log2h entries
	const auto required_zeta_pitch = std::max<u32>((u32)(depth_format == rsx::surface_depth_format::z16 ? clip_horizontal * 2 : clip_horizontal * 4) * aa_factor_u, 64u);
	const auto required_color_pitch = std::max<u32>((u32)rsx::utility::get_packed_pitch(surface_format, clip_horizontal) * aa_factor_u, 64u);
	const bool stencil_test_enabled = depth_format == rsx::surface_depth_format::z24s8 && rsx::method_registers.stencil_test_enabled();
	const auto lg2w = rsx::method_registers.surface_log2_width();
	const auto lg2h = rsx::method_registers.surface_log2_height();
	const auto clipw_log2 = (u32)floor(log2(clip_horizontal));
	const auto cliph_log2 = (u32)floor(log2(clip_vertical));

	if (depth_address)
	{
		if (!rsx::method_registers.depth_test_enabled() &&
			!stencil_test_enabled &&
			target != rsx::surface_target::none)
		{
			//Disable depth buffer if depth testing is not enabled, unless a clear command is targeting the depth buffer
			const bool is_depth_clear = !!(context & rsx::framebuffer_creation_context::context_clear_depth);
			if (!is_depth_clear)
			{
				depth_address = 0;
				m_framebuffer_state_contested = true;
			}
		}

		if (depth_address && zeta_pitch < required_zeta_pitch)
		{
			if (lg2w < clipw_log2 || lg2h < cliph_log2)
			{
				//Cannot fit
				depth_address = 0;

				if (lg2w > 0 || lg2h > 0)
				{
					//Something was actually declared for the swizzle context dimensions
					LOG_WARNING(RSX, "Invalid swizzled context depth surface dims, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_horizontal, clip_vertical);
				}
			}
			else
			{
				LOG_TRACE(RSX, "Swizzled context depth surface, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_horizontal, clip_vertical);
			}
		}
	}

	for (const auto &index : rsx::utility::get_rtt_indexes(target))
	{
		if (pitchs[index] < required_color_pitch)
		{
			if (lg2w < clipw_log2 || lg2h < cliph_log2)
			{
				surface_addresses[index] = 0;

				if (lg2w > 0 || lg2h > 0)
				{
					//Something was actually declared for the swizzle context dimensions
					LOG_WARNING(RSX, "Invalid swizzled context color surface dims, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_horizontal, clip_vertical);
				}
			}
			else
			{
				LOG_TRACE(RSX, "Swizzled context color surface, LG2W=%d, LG2H=%d, clip_w=%d, clip_h=%d", lg2w, lg2h, clip_horizontal, clip_vertical);
			}
		}

		if (surface_addresses[index] == depth_address)
		{
			LOG_TRACE(RSX, "Framebuffer at 0x%X has aliasing color/depth targets, zeta_pitch = %d, color_pitch=%d", depth_address, zeta_pitch, pitchs[index]);
			//TODO: Research clearing both depth AND color
			//TODO: If context is creation_draw, deal with possibility of a lost buffer clear
			if (context == rsx::framebuffer_creation_context::context_clear_depth ||
				rsx::method_registers.depth_test_enabled() || stencil_test_enabled ||
				(!rsx::method_registers.color_write_enabled() && rsx::method_registers.depth_write_enabled()))
			{
				// Use address for depth data
				surface_addresses[index] = 0;
			}
			else
			{
				// Use address for color data
				depth_address = 0;
				m_framebuffer_state_contested = true;
				break;
			}
		}

		if (surface_addresses[index])
			framebuffer_status_valid = true;
	}

	if (!framebuffer_status_valid && !depth_address)
	{
		LOG_WARNING(RSX, "Framebuffer setup failed. Draw calls may have been lost");
		return;
	}

	//Window (raster) offsets
	const auto window_offset_x = rsx::method_registers.window_offset_x();
	const auto window_offset_y = rsx::method_registers.window_offset_y();
	const auto window_clip_width = rsx::method_registers.window_clip_horizontal();
	const auto window_clip_height = rsx::method_registers.window_clip_vertical();

	const auto bpp = get_format_block_size_in_bytes(surface_format);

	if (window_offset_x || window_offset_y)
	{
		//Window offset is what affects the raster position!
		//Tested with Turbo: Super stunt squad that only changes the window offset to declare new framebuffers
		//Sampling behavior clearly indicates the addresses are expected to have changed
		if (auto clip_type = rsx::method_registers.window_clip_type())
			LOG_ERROR(RSX, "Unknown window clip type 0x%X" HERE, clip_type);

		for (const auto &index : rsx::utility::get_rtt_indexes(target))
		{
			if (surface_addresses[index])
			{
				const u32 window_offset_bytes = (std::max<u32>(pitchs[index], required_color_pitch) * window_offset_y) + ((aa_factor_u * bpp) * window_offset_x);
				surface_addresses[index] += window_offset_bytes;
			}
		}

		if (depth_address)
		{
			const auto depth_bpp = depth_format == rsx::surface_depth_format::z16 ? 2 : 4;
			depth_address += (std::max<u32>(zeta_pitch, required_zeta_pitch) * window_offset_y) + ((aa_factor_u * depth_bpp) * window_offset_x);
		}
	}

	if ((window_clip_width && window_clip_width < clip_horizontal) ||
		(window_clip_height && window_clip_height < clip_vertical))
	{
		LOG_ERROR(RSX, "Unexpected window clip dimensions: window_clip=%dx%d, surface_clip=%dx%d",
			window_clip_width, window_clip_height, clip_horizontal, clip_vertical);
	}

	if (draw_fbo)
	{
		bool really_changed = false;
		auto sz = draw_fbo.get_extents();

		if (sz.width == clip_horizontal && sz.height == clip_vertical)
		{
			for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
			{
				if (m_surface_info[i].address != surface_addresses[i])
				{
					really_changed = true;
					break;
				}
			}

			if (!really_changed)
			{
				if (depth_address == m_depth_surface_info.address)
				{
					// Nothing has changed, we're still using the same framebuffer
					// Update flags to match current

					const auto aa_mode = rsx::method_registers.surface_antialias();

					for (u32 index = 0; index < 4; index++)
					{
						if (auto surface = std::get<1>(m_rtts.m_bound_render_targets[index]))
						{
							surface->write_aa_mode = aa_mode;
						}
					}

					if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil))
					{
						ds->write_aa_mode = aa_mode;
					}

					return;
				}
			}
		}
	}

	m_rtts.prepare_render_target(nullptr, surface_format, depth_format,  clip_horizontal, clip_vertical,
		target, surface_addresses, depth_address);

	draw_fbo.recreate();
	draw_fbo.bind();
	draw_fbo.set_extents({ (int)clip_horizontal, (int)clip_vertical });

	bool old_format_found = false;
	gl::texture::format old_format;

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

			m_gl_texture_cache.set_memory_read_flags(m_surface_info[i].address, m_surface_info[i].pitch * m_surface_info[i].height, rsx::memory_read_flags::flush_once);
			m_gl_texture_cache.flush_if_cache_miss_likely(old_format, m_surface_info[i].address, m_surface_info[i].pitch * m_surface_info[i].height);
		}

		if (std::get<0>(m_rtts.m_bound_render_targets[i]))
		{
			auto rtt = std::get<1>(m_rtts.m_bound_render_targets[i]);
			draw_fbo.color[i] = *rtt;

			rtt->set_rsx_pitch(pitchs[i]);
			m_surface_info[i] = { surface_addresses[i], std::max(pitchs[i], required_color_pitch), false, surface_format, depth_format, clip_horizontal, clip_vertical };

			rtt->tile = find_tile(color_offsets[i], color_locations[i]);
			rtt->write_aa_mode = aa_mode;
			m_gl_texture_cache.notify_surface_changed(surface_addresses[i]);
			m_gl_texture_cache.tag_framebuffer(surface_addresses[i]);
		}
		else
			m_surface_info[i] = {};
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil))
	{
		if (m_depth_surface_info.pitch && g_cfg.video.write_depth_buffer)
		{
			auto bpp = m_depth_surface_info.pitch / m_depth_surface_info.width;
			auto old_format = (bpp == 2) ? gl::texture::format::depth : gl::texture::format::depth_stencil;

			m_gl_texture_cache.set_memory_read_flags(m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height, rsx::memory_read_flags::flush_once);
			m_gl_texture_cache.flush_if_cache_miss_likely(old_format, m_depth_surface_info.address, m_depth_surface_info.pitch * m_depth_surface_info.height);
		}

		auto ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		if (depth_format == rsx::surface_depth_format::z24s8)
			draw_fbo.depth_stencil = *ds;
		else
			draw_fbo.depth = *ds;

		std::get<1>(m_rtts.m_bound_depth_stencil)->set_rsx_pitch(rsx::method_registers.surface_z_pitch());
		m_depth_surface_info = { depth_address, std::max(zeta_pitch, required_zeta_pitch), true, surface_format, depth_format, clip_horizontal, clip_vertical };

		ds->write_aa_mode = aa_mode;
		m_gl_texture_cache.notify_surface_changed(depth_address);

		m_gl_texture_cache.tag_framebuffer(depth_address);
	}
	else
		m_depth_surface_info = {};

	framebuffer_status_valid = draw_fbo.check();
	if (!framebuffer_status_valid) return;

	check_zcull_status(true);
	set_viewport();

	switch (rsx::method_registers.surface_color_target())
	{
	case rsx::surface_target::none: break;

	case rsx::surface_target::surface_a:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[0]);
		__glcheck draw_fbo.read_buffer(draw_fbo.color[0]);
		break;

	case rsx::surface_target::surface_b:
		__glcheck draw_fbo.draw_buffer(draw_fbo.color[1]);
		__glcheck draw_fbo.read_buffer(draw_fbo.color[1]);
		break;

	case rsx::surface_target::surfaces_a_b:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1] });
		__glcheck draw_fbo.read_buffer(draw_fbo.color[0]);
		break;

	case rsx::surface_target::surfaces_a_b_c:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2] });
		__glcheck draw_fbo.read_buffer(draw_fbo.color[0]);
		break;

	case rsx::surface_target::surfaces_a_b_c_d:
		__glcheck draw_fbo.draw_buffers({ draw_fbo.color[0], draw_fbo.color[1], draw_fbo.color[2], draw_fbo.color[3] });
		__glcheck draw_fbo.read_buffer(draw_fbo.color[0]);
		break;
	}

	m_gl_texture_cache.clear_ro_tex_invalidate_intr();

	//Mark buffer regions as NO_ACCESS on Cell visible side
	if (g_cfg.video.write_color_buffers)
	{
		auto color_format = rsx::internals::surface_color_format_to_gl(surface_format);

		for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			if (!m_surface_info[i].address || !m_surface_info[i].pitch) continue;

			const u32 range = m_surface_info[i].pitch * m_surface_info[i].height * aa_factor_v;
			m_gl_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_render_targets[i]), m_surface_info[i].address, range, m_surface_info[i].width, m_surface_info[i].height, m_surface_info[i].pitch,
			color_format.format, color_format.type, color_format.swap_bytes);
		}
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.address && m_depth_surface_info.pitch)
		{
			const auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(depth_format);
			const u32 range = m_depth_surface_info.pitch * m_depth_surface_info.height * aa_factor_v;
			m_gl_texture_cache.lock_memory_region(std::get<1>(m_rtts.m_bound_depth_stencil), m_depth_surface_info.address, range, m_depth_surface_info.width, m_depth_surface_info.height, m_depth_surface_info.pitch,
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
	if (!draw_fbo)
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

				const u32 range = pitch * height;

				rsx::tiled_region color_buffer = get_tiled_address(offset, location & 0xf);
				u32 texaddr = (u32)((u64)color_buffer.ptr - (u64)vm::base(0));

				bool success = m_gl_texture_cache.load_memory_from_cache(texaddr, pitch * height, std::get<1>(m_rtts.m_bound_render_targets[i]));

				//Fall back to slower methods if the image could not be fetched from cache.
				if (!success)
				{
					if (!color_buffer.tile)
					{
						__glcheck std::get<1>(m_rtts.m_bound_render_targets[i])->copy_from(color_buffer.ptr, color_format.format, color_format.type);
					}
					else
					{
						m_gl_texture_cache.invalidate_range(texaddr, range, false, false, true);

						std::unique_ptr<u8[]> buffer(new u8[pitch * height]);
						color_buffer.read(buffer.get(), width, height, pitch);

						__glcheck std::get<1>(m_rtts.m_bound_render_targets[i])->copy_from(buffer.get(), color_format.format, color_format.type);
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

		u32 depth_address = rsx::get_address(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());
		bool in_cache = m_gl_texture_cache.load_memory_from_cache(depth_address, pitch * height, std::get<1>(m_rtts.m_bound_depth_stencil));

		if (in_cache)
			return;

		//Read failed. Fall back to slow s/w path...

		auto depth_format = rsx::internals::surface_depth_format_to_gl(rsx::method_registers.surface_depth_fmt());
		int pixel_size    = rsx::internals::get_pixel_size(rsx::method_registers.surface_depth_fmt());
		gl::buffer pbo_depth;

		__glcheck pbo_depth.create(width * height * pixel_size);
		__glcheck pbo_depth.map([&](GLubyte* pixels)
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

		__glcheck std::get<1>(m_rtts.m_bound_depth_stencil)->copy_from(pbo_depth, depth_format.format, depth_format.type);
	}
}

void GLGSRender::write_buffers()
{
	if (!draw_fbo)
		return;

	if (g_cfg.video.write_color_buffers)
	{
		auto write_color_buffers = [&](int index, int count)
		{
			for (int i = index; i < index + count; ++i)
			{
				if (m_surface_info[i].pitch == 0)
					continue;

				/**Even tiles are loaded as whole textures during read_buffers from testing.
				* Need further evaluation to determine correct behavior. Separate paths for both show no difference,
				* but using the GPU to perform the caching is many times faster.
				*/

				const u32 range = m_surface_info[i].pitch * m_surface_info[i].height;
				m_gl_texture_cache.flush_memory_to_cache(m_surface_info[i].address, range, true, 0xFF);
			}
		};

		write_color_buffers(0, 4);
	}

	if (g_cfg.video.write_depth_buffer)
	{
		if (m_depth_surface_info.pitch == 0) return;

		const u32 range = m_depth_surface_info.pitch * m_depth_surface_info.height;
		m_gl_texture_cache.flush_memory_to_cache(m_depth_surface_info.address, range, true, 0xFF);
	}
}
