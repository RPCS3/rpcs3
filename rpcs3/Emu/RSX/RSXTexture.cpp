#include "stdafx.h"
#include "RSXTexture.h"

#include "rsx_methods.h"
#include "rsx_utils.h"

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

		default: ASSUME(0);
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

		return std::min(verify(HERE, mipmap()), max_mipmap_count);
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
		return static_cast<rsx::comparison_function>((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 28) & 0xf);
	}

	u8 fragment_texture::unsigned_remap() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 12) & 0xf);
	}

	u8 fragment_texture::gamma() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 20) & 0xf);
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

	std::pair<std::array<u8, 4>, std::array<u8, 4>> fragment_texture::decoded_remap() const
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
		case CELL_GCM_TEXTURE_B8:
		{
			// Low bit in remap control seems to affect whether the A component is forced to 1
			// Only seen in BLUS31604
			if (remap_override)
			{
				// Set remap lookup for A component to FORCE_ONE
				remap_ctl = (remap_ctl & ~(3 << 8)) | (1 << 8);
			}
			break;
		}
		default:
			break;
		}

		//Remapping tables; format is A-R-G-B
		//Remap input table. Contains channel index to read color from
		const std::array<u8, 4> remap_inputs =
		{
			static_cast<u8>(remap_ctl & 0x3),
			static_cast<u8>((remap_ctl >> 2) & 0x3),
			static_cast<u8>((remap_ctl >> 4) & 0x3),
			static_cast<u8>((remap_ctl >> 6) & 0x3),
		};

		//Remap control table. Controls whether the remap value is used, or force either 0 or 1
		const std::array<u8, 4> remap_lookup =
		{
			static_cast<u8>((remap_ctl >> 8) & 0x3),
			static_cast<u8>((remap_ctl >> 10) & 0x3),
			static_cast<u8>((remap_ctl >> 12) & 0x3),
			static_cast<u8>((remap_ctl >> 14) & 0x3),
		};

		return std::make_pair(remap_inputs, remap_lookup);
	}

	f32 fragment_texture::bias() const
	{
		return rsx::decode_fxp<4, 8>((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
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

		default: ASSUME(0);
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
		return std::min(verify(HERE, mipmap()), max_mipmap_count);
	}

	std::pair<std::array<u8, 4>, std::array<u8, 4>> vertex_texture::decoded_remap() const
	{
		return
		{
			{ CELL_GCM_TEXTURE_REMAP_FROM_A, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_B },
			{ CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP }
		};
	}

	u32 vertex_texture::remap() const
	{
		//disabled
		return 0xAAE4;
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
		return rsx::decode_fxp<4, 8>((registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
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

	u16 vertex_texture::depth() const
	{
		return dimension() == rsx::texture_dimension::dimension3d ? (registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] >> 20) : 1;
	}

	u32 vertex_texture::pitch() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] & 0xfffff;
	}
}
