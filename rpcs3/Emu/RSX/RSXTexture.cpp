#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "RSXThread.h"
#include "RSXTexture.h"
#include "rsx_methods.h"

namespace rsx
{
	void fragment_texture::init()
	{
		// Offset
		registers[NV4097_SET_TEXTURE_OFFSET + (m_index * 8)] = 0;

		// Format
		registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] =
			((/*wraps*/1) | ((/*anisoBias*/0) << 4) | ((/*wrapt*/1) << 8) | ((/*unsignedRemap*/0) << 12) |
			((/*wrapr*/3) << 16) | ((/*gamma*/0) << 20) | ((/*signedRemap*/0) << 24) | ((/*zfunc*/0) << 28));

		// Control0
		registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] =
			(((/*alphakill*/0) << 2) | (/*maxaniso*/0) << 4) | ((/*maxlod*/0xc00) << 7) | ((/*minlod*/0) << 19) | ((/*enable*/0) << 31);

		// Control1
		registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)] = 0xAAE4;

		// Filter
		registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] =
			((/*bias*/0) | ((/*conv*/1) << 13) | ((/*min*/5) << 16) | ((/*mag*/2) << 24)
			| ((/*as*/0) << 28) | ((/*rs*/0) << 29) | ((/*gs*/0) << 30) | ((/*bs*/0) << 31));

		// Image Rect
		registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)] = (/*height*/1) | ((/*width*/1) << 16);

		// Border Color
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index * 8)] = 0;
	}

	u32 fragment_texture::offset() const
	{
		return registers[NV4097_SET_TEXTURE_OFFSET + (m_index * 8)];
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
		u16 max_mipmap_count = 1;
		if (is_compressed_format())
		{
			// OpenGL considers that highest mipmap level for DXTC format is when either width or height is 1
			// not both. Assume it's the same for others backend.
			max_mipmap_count = static_cast<u16>(floor(log2(std::min(width() / 4, height() / 4))) + 1);
		}
		else
			max_mipmap_count = static_cast<u16>(floor(log2(std::max(width(), height()))) + 1);
		
		max_mipmap_count = std::min(mipmap(), max_mipmap_count);
		return (max_mipmap_count > 0) ? max_mipmap_count : 1;
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

	u8 fragment_texture::unsigned_remap() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 12) & 0xf);
	}

	u8 fragment_texture::zfunc() const
	{
		return ((registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 28) & 0xf);
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
		return location() <= 1 && ((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	u16 fragment_texture::min_lod() const
	{
		return ((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	u16 fragment_texture::max_lod() const
	{
		return ((registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
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
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		{
			//Low bit in remap control affects whether the G component should match R and B components
			//Components are usually interleaved R-G-R-G unless flag is set, then its R-R-R-G (Virtua Fighter 5)
			//NOTE: The remap vector can also read from B-A-B-A in some cases (Mass Effect 3)
			if (remap_override)
			{
				auto r_component = (remap_ctl >> 2) & 3;
				remap_ctl = remap_ctl & ~(3 << 4) | r_component << 4;
			}

			remap_ctl &= 0xFFFF;
			break;
		}
		case CELL_GCM_TEXTURE_B8:
		{
			//Low bit in remap control seems to affect whether the A component is forced to 1
			//Only seen in BLUS31604
			//TODO: Verify with a hardware test
			if (remap_override)
			{
				//Set remap lookup for A component to FORCE_ONE
				remap_ctl = remap_ctl & ~(3 << 8) | (1 << 8);
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

	float fragment_texture::bias() const
	{
		return float(f16((registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff));
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
		return ((registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff);
	}

	u32 fragment_texture::border_color() const
	{
		return registers[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	u16 fragment_texture::depth() const
	{
		return registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] >> 20;
	}

	u32 fragment_texture::pitch() const
	{
		return registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] & 0xfffff;
	}

	void vertex_texture::init()
	{
		// Offset
		registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + (m_index * 8)] = 0;

		// Format
		registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] =
			((/*wraps*/1) | ((/*anisoBias*/0) << 4) | ((/*wrapt*/1) << 8) | ((/*unsignedRemap*/0) << 12) |
			((/*wrapr*/3) << 16) | ((/*gamma*/0) << 20) | ((/*signedRemap*/0) << 24) | ((/*zfunc*/0) << 28));

		// Control0
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] =
			(((/*alphakill*/0) << 2) | (/*maxaniso*/0) << 4) | ((/*maxlod*/0xc00) << 7) | ((/*minlod*/0) << 19) | ((/*enable*/0) << 31);

		// Control1
		//registers[NV4097_SET_VERTEX_TEXTURE_CONTROL1 + (m_index * 8)] = 0xE4;

		// Filter
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] =
			((/*bias*/0) | ((/*conv*/1) << 13) | ((/*min*/5) << 16) | ((/*mag*/2) << 24)
			| ((/*as*/0) << 28) | ((/*rs*/0) << 29) | ((/*gs*/0) << 30) | ((/*bs*/0) << 31));

		// Image Rect
		registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)] = (/*height*/1) | ((/*width*/1) << 16);

		// Border Color
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (m_index * 8)] = 0;
	}

	u32 vertex_texture::offset() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + (m_index * 8)];
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
		return ((registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 3) & 0x1);
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
		u16 max_mipmap_count = static_cast<u16>(floor(log2(std::max(width(), height()))) + 1);
		max_mipmap_count = std::min(mipmap(), max_mipmap_count);

		return (max_mipmap_count > 0) ? max_mipmap_count : 1;
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
		return location() <= 1 && ((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	u16 vertex_texture::min_lod() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	u16 vertex_texture::max_lod() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	u16 vertex_texture::bias() const
	{
		return ((registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
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
		return ((registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff);
	}

	u32 vertex_texture::border_color() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	u16 vertex_texture::depth() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] >> 20;
	}

	u32 vertex_texture::pitch() const
	{
		return registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] & 0xfffff;
	}
}
