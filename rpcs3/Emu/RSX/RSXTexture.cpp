#include "stdafx.h"
#include "RSXTexture.h"

#include "rsx_utils.h"

#include "Emu/system_config.h"

namespace rsx
{
	u32 fragment_texture::offset() const
	{
		return registers[NV4097_SET_TEXTURE_OFFSET + (m_index * 8)] & 0x7FFFFFFF;
	}

	u8 fragment_texture::location() const
	{
		return (registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] & 0x3) - 1;
	}

	bool fragment_texture::cubemap() const
	{
		return ((registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 2) & 0x1);
	}

	u8 fragment_texture::border_type() const
	{
		return ((registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 3) & 0x1);
	}

	rsx::texture_dimension fragment_texture::dimension() const
	{
		return rsx::to_texture_dimension((registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	rsx::texture_dimension_extended fragment_texture::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture_dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture_dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_3d;
		case rsx::texture_dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: fmt::throw_exception("Unreachable");
		}
	}

	u8 fragment_texture::format() const
	{
		return ((registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	bool fragment_texture::is_compressed_format() const
	{
		int texture_format = format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		if (texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 ||
			texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 ||
			texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT45)
			return true;
		return false;
	}

	u16 fragment_texture::mipmap() const
	{
		return ((registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 fragment_texture::get_exact_mipmap_count() const
	{
		u16 max_mipmap_count;
		if (is_compressed_format())
		{
			// OpenGL considers that highest mipmap level for DXTC format is when either width or height is 1
			// not both. Assume it's the same for others backend.
			max_mipmap_count = floor_log2(static_cast<u32>(std::min(width() / 4, height() / 4))) + 1;
		}
		else
			max_mipmap_count = floor_log2(static_cast<u32>(std::max(width(), height()))) + 1;

		return std::min(ensure(mipmap()), max_mipmap_count);
	}

	rsx::texture_wrap_mode fragment_texture::wrap_s() const
	{
		return rsx::to_texture_wrap_mode((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)]) & 0xf);
	}

	rsx::texture_wrap_mode fragment_texture::wrap_t() const
	{
		return rsx::to_texture_wrap_mode((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 8) & 0xf);
	}

	rsx::texture_wrap_mode fragment_texture::wrap_r() const
	{
		return rsx::to_texture_wrap_mode((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 16) & 0xf);
	}

	rsx::comparison_function fragment_texture::zfunc() const
	{
		return rsx::to_comparison_function((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 28) & 0xf);
	}

	u8 fragment_texture::unsigned_remap() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 12) & 0xf);
	}

	u8 fragment_texture::gamma() const
	{
		// Converts gamma mask from RGBA to ARGB for compatibility with other per-channel mask registers
		const u32 rgba8_ctrl = ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 20) & 0xf);
		return ((rgba8_ctrl << 1) & 0xF) | (rgba8_ctrl >> 3);
	}

	u8 fragment_texture::aniso_bias() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 4) & 0xf);
	}

	u8 fragment_texture::signed_remap() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 24) & 0xf);
	}

	bool fragment_texture::enabled() const
	{
		return ((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	f32 fragment_texture::min_lod() const
	{
		return rsx::decode_fxp<4, 8, false>((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	f32 fragment_texture::max_lod() const
	{
		return rsx::decode_fxp<4, 8, false>((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	rsx::texture_max_anisotropy fragment_texture::max_aniso() const
	{
		switch (g_cfg.video.strict_rendering_mode ? 0 : g_cfg.video.anisotropic_level_override)
		{
		case 1: return rsx::texture_max_anisotropy::x1;
		case 2: return rsx::texture_max_anisotropy::x2;
		case 4: return rsx::texture_max_anisotropy::x4;
		case 6: return rsx::texture_max_anisotropy::x6;
		case 8: return rsx::texture_max_anisotropy::x8;
		case 10: return rsx::texture_max_anisotropy::x10;
		case 12: return rsx::texture_max_anisotropy::x12;
		case 16: return rsx::texture_max_anisotropy::x16;
		default: break;
		}

		return rsx::to_texture_max_anisotropy((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 4) & 0x7);
	}

	bool fragment_texture::alpha_kill_enabled() const
	{
		return ((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 2) & 0x1);
	}

	u32 fragment_texture::remap() const
	{
		return (registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)]);
	}

	rsx::texture_channel_remap_t fragment_texture::decoded_remap() const
	{
		u32 remap_ctl = registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)];
		u32 remap_override = (remap_ctl >> 16) & 0xFFFF;

		switch (format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN))
		{
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		{
			// Floating point textures cannot be remapped on realhw, throws error 261
			remap_ctl &= ~(0xFF);
			remap_ctl |= 0xE4;
			break;
		}
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		{
			// Floating point textures cannot be remapped on realhw, throws error 261
			// High word of remap ctrl remaps ARGB to YXXX
			const u32 lo_word = (remap_override) ? 0x56 : 0x66;
			remap_ctl &= ~(0xFF);
			remap_ctl |= lo_word;
			break;
		}
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		{
			// These are special formats whose remap encoding is in 16-bit blocks
			// The first channel is encoded as 0x4 (combination of 0 and 1) and the second channel is 0xE (combination of 2 and 3)
			// There are only 2 valid combinations - 0xE4 or 0x4E, any attempts to mess with this will crash the system
			// This means only R and G channels exist for these formats
			// Note that for the X16 format, 0xE refers to the "second" channel of a Y16_X16 format. 0xE is actually the existing data in this case

			// Low bit in remap override (high word) affects whether the G component should match R and B components
			// Components are usually interleaved R-G-R-G unless flag is set, then its R-R-R-G (Virtua Fighter 5)
			// NOTE: The remap vector can also read from B-A-B-A in some cases (Mass Effect 3)

			u32 lo_word = remap_ctl & 0xFF;
			remap_ctl &= 0xFF00;

			switch (lo_word)
			{
			case 0xE4:
				lo_word = (remap_override) ? 0x56 : 0x66;
				break;
			case 0x4E:
				lo_word = (remap_override) ? 0xA9 : 0x99;
				break;
			case 0xEE:
				lo_word = 0xAA;
				break;
			case 0x44:
				lo_word = 0x55;
				break;
			}

			remap_ctl |= lo_word;
			break;
		}
		default:
			break;
		}

		return decode_remap_encoding(remap_ctl);
	}

	f32 fragment_texture::bias() const
	{
		const f32 bias = rsx::decode_fxp<4, 8>((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
		return std::clamp<f32>(bias + static_cast<f32>(g_cfg.video.texture_lod_bias.get()), -16.f, 16.f - 1.f / 256);
	}

	rsx::texture_minify_filter fragment_texture::min_filter() const
	{
		return rsx::to_texture_minify_filter((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 16) & 0x7);
	}

	rsx::texture_magnify_filter fragment_texture::mag_filter() const
	{
		return rsx::to_texture_magnify_filter((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 24) & 0x7);
	}

	u8 fragment_texture::convolution_filter() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 13) & 0xf);
	}

	u8 fragment_texture::argb_signed() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 28) & 0xf);
	}

	bool fragment_texture::a_signed() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 28) & 0x1);
	}

	bool fragment_texture::r_signed() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 29) & 0x1);
	}

	bool fragment_texture::g_signed() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 30) & 0x1);
	}

	bool fragment_texture::b_signed() const
	{
		return ((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 31) & 0x1);
	}

	u16 fragment_texture::width() const
	{
		return ((registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 fragment_texture::height() const
	{
		return dimension() != rsx::texture_dimension::dimension1d ? ((registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff) : 1;
	}

	u32 fragment_texture::border_color() const
	{
		return registers[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	color4f fragment_texture::remapped_border_color() const
	{
		color4f base_color = rsx::decode_border_color(border_color());
		if (remap() == RSX_TEXTURE_REMAP_IDENTITY)
		{
			return base_color;
		}
		return decoded_remap().remap(base_color);
	}

	u16 fragment_texture::depth() const
	{
		return dimension() == rsx::texture_dimension::dimension3d ? (registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] >> 20) : 1;
	}

	u32 fragment_texture::pitch() const
	{
		return registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] & 0xfffff;
	}

	u32 vertex_texture::offset() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + (m_index * 8)] & 0x7FFFFFFF;
	}

	u8 vertex_texture::location() const
	{
		return (registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] & 0x3) - 1;
	}

	bool vertex_texture::cubemap() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 2) & 0x1);
	}

	u8 vertex_texture::border_type() const
	{
		// Border bit has no effect on vertex textures, it is always zero
		return 1;
	}

	rsx::texture_dimension vertex_texture::dimension() const
	{
		return rsx::to_texture_dimension((registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	rsx::texture_dimension_extended vertex_texture::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture_dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture_dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_3d;
		case rsx::texture_dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: fmt::throw_exception("Unreachable");
		}
	}

	u8 vertex_texture::format() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	u16 vertex_texture::mipmap() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 vertex_texture::get_exact_mipmap_count() const
	{
		const u16 max_mipmap_count = floor_log2(static_cast<u32>(std::max(width(), height()))) + 1;
		return std::min(ensure(mipmap()), max_mipmap_count);
	}

	rsx::texture_channel_remap_t vertex_texture::decoded_remap() const
	{
		return rsx::default_remap_vector;
	}

	u32 vertex_texture::remap() const
	{
		// disabled
		return RSX_TEXTURE_REMAP_IDENTITY;
	}

	bool vertex_texture::enabled() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	f32 vertex_texture::min_lod() const
	{
		return rsx::decode_fxp<4, 8, false>((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	f32 vertex_texture::max_lod() const
	{
		return rsx::decode_fxp<4, 8, false>((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	f32 vertex_texture::bias() const
	{
		const f32 bias = rsx::decode_fxp<4, 8>((registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
		return std::clamp<f32>(bias + static_cast<f32>(g_cfg.video.texture_lod_bias.get()), -16.f, 16.f - 1.f / 256);
	}

	rsx::texture_minify_filter vertex_texture::min_filter() const
	{
		return rsx::texture_minify_filter::nearest;
	}

	rsx::texture_magnify_filter vertex_texture::mag_filter() const
	{
		return rsx::texture_magnify_filter::nearest;
	}

	rsx::texture_wrap_mode vertex_texture::wrap_s() const
	{
		return rsx::to_texture_wrap_mode((registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)]) & 0xf);
	}

	rsx::texture_wrap_mode vertex_texture::wrap_t() const
	{
		return rsx::to_texture_wrap_mode((registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 8) & 0xf);
	}

	rsx::texture_wrap_mode vertex_texture::wrap_r() const
	{
		return rsx::texture_wrap_mode::wrap;
	}

	u16 vertex_texture::width() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 vertex_texture::height() const
	{
		return dimension() != rsx::texture_dimension::dimension1d ? ((registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff) : 1;
	}

	u32 vertex_texture::border_color() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	color4f vertex_texture::remapped_border_color() const
	{
		return rsx::decode_border_color(border_color());
	}

	u16 vertex_texture::depth() const
	{
		return dimension() == rsx::texture_dimension::dimension3d ? (registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] >> 20) : 1;
	}

	u32 vertex_texture::pitch() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] & 0xfffff;
	}
}
