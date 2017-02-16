#include "stdafx.h"
#include "Utilities/Config.h"
#include "../rsx_methods.h"
#include "GLGSRender.h"

extern cfg::bool_entry g_cfg_rsx_write_color_buffers;
extern cfg::bool_entry g_cfg_rsx_write_depth_buffer;
extern cfg::bool_entry g_cfg_rsx_read_color_buffers;
extern cfg::bool_entry g_cfg_rsx_read_depth_buffer;

color_format rsx::internals::surface_color_format_to_gl(rsx::surface_color_format color_format)
{
	//color format
	switch (color_format)
	{
	case rsx::surface_color_format::r5g6b5:
		return{ ::gl::texture::type::ushort_5_6_5, ::gl::texture::format::rgb, false, 3, 2 };

	case rsx::surface_color_format::a8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1 };

	//These formats discard their alpha component (always 1)
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		return{ ::gl::texture::type::uint_8_8_8_8, ::gl::texture::format::bgra, false, 4, 1,
		{ ::gl::texture::channel::one, ::gl::texture::channel::r, ::gl::texture::channel::g, ::gl::texture::channel::b } };

	case rsx::surface_color_format::w16z16y16x16:
		return{ ::gl::texture::type::f16, ::gl::texture::format::rgba, true, 4, 2 };

	case rsx::surface_color_format::w32z32y32x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::rgba, true, 4, 4 };

	case rsx::surface_color_format::b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::red, false, 1, 1 };

	case rsx::surface_color_format::g8b8:
		return{ ::gl::texture::type::ubyte, ::gl::texture::format::rg, false, 2, 1 };

	case rsx::surface_color_format::x32:
		return{ ::gl::texture::type::f32, ::gl::texture::format::red, false, 1, 4 };

	default:
		LOG_ERROR(RSX, "Surface color buffer: Unsupported surface color format (0x%x)", (u32)color_format);

	case rsx::surface_color_format::a8b8g8r8:
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

void GLGSRender::init_buffers(bool skip_reading)
{
	//NOTE 1: Sometimes, we process clear before sync flushing rsx buffers. Leads to downloading of blank data
	//Clearing of surfaces is deferred to handle this
	//NOTE 2: It is possible for a game to do:
	//1. Bind buffer 1
	//2. Clear
	//3. Bind buffer 2 without touching 1
	//4. Clear
	//5. Bind buffer 1
	//6. Draw without clear

	if (draw_fbo && !m_rtts_dirty)
	{
		set_viewport();
		return;
	}

	//We are about to change buffers, flush any pending requests for the old buffers
	//LOG_WARNING(RSX, "Render targets have changed; checking for sync points (EID=%d)", m_draw_calls);
	synchronize_buffers();

	//If the old buffers were dirty, clear them before we bind new buffers
	if (surface_clear_flags)
	{
		clear_surface(surface_clear_flags);
		surface_clear_flags = 0;
	}

	m_rtts_dirty = false;

	const u16 clip_horizontal = rsx::method_registers.surface_clip_width();
	const u16 clip_vertical = rsx::method_registers.surface_clip_height();

	const auto pitchs = get_pitchs();
	const auto surface_format = rsx::method_registers.surface_color();
	const auto depth_format = rsx::method_registers.surface_depth_fmt();

	const auto surface_addresses = get_color_surface_addresses();
	const auto depth_address = get_zeta_surface_address();

	m_rtts.prepare_render_target(nullptr, surface_format, depth_format,  clip_horizontal, clip_vertical,
		rsx::method_registers.surface_color_target(),
		surface_addresses, depth_address);

	draw_fbo.recreate();

	for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
	{
		if (std::get<0>(m_rtts.m_bound_render_targets[i]))
		{
			__glcheck draw_fbo.color[i] = *std::get<1>(m_rtts.m_bound_render_targets[i]);

			std::get<1>(m_rtts.m_bound_render_targets[i])->set_rsx_pitch(pitchs[i]);
			surface_info[i] = { surface_addresses[i], pitchs[i], false, surface_format, depth_format, clip_horizontal, clip_vertical };
		}
		else
			surface_info[i] = {};
	}

	if (std::get<0>(m_rtts.m_bound_depth_stencil))
	{
		__glcheck draw_fbo.depth = *std::get<1>(m_rtts.m_bound_depth_stencil);

		std::get<1>(m_rtts.m_bound_depth_stencil)->set_rsx_pitch(rsx::method_registers.surface_z_pitch());
		depth_surface_info = { depth_address, rsx::method_registers.surface_z_pitch(), true, surface_format, depth_format, clip_horizontal, clip_vertical };
	}
	else
		depth_surface_info = {};

	if (!draw_fbo.check())
		return;

	draw_fbo.bind();
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

	//Mark buffer regions as NO_ACCESS on Cell visible side
	if (g_cfg_rsx_write_color_buffers)
	{
		auto color_format = rsx::internals::surface_color_format_to_gl(surface_format);

		for (u8 i = 0; i < rsx::limits::color_buffers_count; ++i)
		{
			if (!surface_info[i].address || pitchs[i] <= 64) continue;

			const u32 range = surface_info[i].pitch * surface_info[i].height;
			m_gl_texture_cache.lock_rtt_region(surface_info[i].address, range, surface_info[i].width, surface_info[i].height, surface_info[i].pitch,
				color_format.format, color_format.type, *std::get<1>(m_rtts.m_bound_render_targets[i]));
		}
	}

	if (g_cfg_rsx_write_depth_buffer)
	{
		if (depth_surface_info.address && rsx::method_registers.surface_z_pitch() > 64)
		{
			auto depth_format_gl = rsx::internals::surface_depth_format_to_gl(depth_format);

			u32 pitch = depth_surface_info.width * 2;
			if (depth_surface_info.depth_format != rsx::surface_depth_format::z16) pitch *= 2;

			const u32 range = pitch * depth_surface_info.height;

			//TODO: Verify that depth surface pitch variance affects results
			if (pitch != depth_surface_info.pitch)
				LOG_WARNING(RSX, "Depth surface pitch does not match computed pitch, %d vs %d", depth_surface_info.pitch, pitch);

			m_gl_texture_cache.lock_rtt_region(depth_surface_info.address, range, depth_surface_info.width, depth_surface_info.height, pitch,
				depth_format_gl.format, depth_format_gl.type, *std::get<1>(m_rtts.m_bound_depth_stencil));
		}
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

	if (g_cfg_rsx_read_color_buffers)
	{
		auto color_format = rsx::internals::surface_color_format_to_gl(rsx::method_registers.surface_color());

		auto read_color_buffers = [&](int index, int count)
		{
			u32 width = rsx::method_registers.surface_clip_width();
			u32 height = rsx::method_registers.surface_clip_height();

			const std::array<u32, 4> offsets = get_offsets();
			const std::array<u32, 4 > locations = get_locations();
			const std::array<u32, 4 > pitchs = get_pitchs();

			for (int i = index; i < index + count; ++i)
			{
				u32 offset = offsets[i];
				u32 location = locations[i];
				u32 pitch = pitchs[i];

				if (pitch <= 64)
					continue;

				rsx::tiled_region color_buffer = get_tiled_address(offset, location & 0xf);
				u32 texaddr = (u32)((u64)color_buffer.ptr - (u64)vm::base(0));

				bool success = m_gl_texture_cache.load_rtt((*std::get<1>(m_rtts.m_bound_render_targets[i])), texaddr, pitch);

				//Fall back to slower methods if the image could not be fetched from cache.
				if (!success)
				{
					if (!color_buffer.tile)
					{
						__glcheck std::get<1>(m_rtts.m_bound_render_targets[i])->copy_from(color_buffer.ptr, color_format.format, color_format.type);
					}
					else
					{
						u32 range = pitch * height;
						m_gl_texture_cache.invalidate_range(texaddr, range);

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

	if (g_cfg_rsx_read_depth_buffer)
	{
		//TODO: use pitch
		u32 pitch = rsx::method_registers.surface_z_pitch();

		if (pitch <= 64)
			return;

		u32 depth_address = rsx::get_address(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());
		bool in_cache = m_gl_texture_cache.load_rtt((*std::get<1>(m_rtts.m_bound_depth_stencil)), depth_address, pitch);

		if (in_cache)
			return;

		//Read failed. Fall back to slow s/w path...

		auto depth_format = rsx::internals::surface_depth_format_to_gl(rsx::method_registers.surface_depth_fmt());
		int pixel_size    = rsx::internals::get_pixel_size(rsx::method_registers.surface_depth_fmt());
		gl::buffer pbo_depth;

		__glcheck pbo_depth.create(rsx::method_registers.surface_clip_width() * rsx::method_registers.surface_clip_height() * pixel_size);
		__glcheck pbo_depth.map([&](GLubyte* pixels)
		{
			u32 depth_address = rsx::get_address(rsx::method_registers.surface_z_offset(), rsx::method_registers.surface_z_dma());

			if (rsx::method_registers.surface_depth_fmt() == rsx::surface_depth_format::z16)
			{
				u16 *dst = (u16*)pixels;
				const be_t<u16>* src = vm::ps3::_ptr<u16>(depth_address);
				for (int i = 0, end = std::get<1>(m_rtts.m_bound_depth_stencil)->width() * std::get<1>(m_rtts.m_bound_depth_stencil)->height(); i < end; ++i)
				{
					dst[i] = src[i];
				}
			}
			else
			{
				u32 *dst = (u32*)pixels;
				const be_t<u32>* src = vm::ps3::_ptr<u32>(depth_address);
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

	if (g_cfg_rsx_write_color_buffers)
	{
		auto write_color_buffers = [&](int index, int count)
		{
			for (int i = index; i < index + count; ++i)
			{
				if (surface_info[i].address == 0 || surface_info[i].pitch <= 64)
					continue;

				/**Even tiles are loaded as whole textures during read_buffers from testing.
				* Need further evaluation to determine correct behavior. Separate paths for both show no difference,
				* but using the GPU to perform the caching is many times faster.
				*/

				const u32 range = surface_info[i].pitch * surface_info[i].height;
				__glcheck m_gl_texture_cache.save_rtt(surface_info[i].address, range);
			}
		};

		write_color_buffers(0, 4);
	}

	if (g_cfg_rsx_write_depth_buffer)
	{
		//TODO: use pitch
		if (!depth_surface_info.address || depth_surface_info.pitch <= 64) return;

		u32 range = depth_surface_info.width * depth_surface_info.height * 2;
		if (depth_surface_info.depth_format != rsx::surface_depth_format::z16) range *= 2;

		m_gl_texture_cache.save_rtt(depth_surface_info.address, range);
	}
}