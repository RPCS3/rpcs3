#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "RSXThread.h"
#include "RSXTexture.h"
#include "rsx_methods.h"

namespace rsx
{
	void texture::init(u8 index)
	{
		m_index = index;

		// Offset
		method_registers[NV4097_SET_TEXTURE_OFFSET + (m_index * 8)] = 0;

		// Format
		method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] =
			((/*wraps*/1) | ((/*anisoBias*/0) << 4) | ((/*wrapt*/1) << 8) | ((/*unsignedRemap*/0) << 12) |
			((/*wrapr*/3) << 16) | ((/*gamma*/0) << 20) | ((/*signedRemap*/0) << 24) | ((/*zfunc*/0) << 28));

		// Control0
		method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] =
			(((/*alphakill*/0) << 2) | (/*maxaniso*/0) << 4) | ((/*maxlod*/0xc00) << 7) | ((/*minlod*/0) << 19) | ((/*enable*/0) << 31);

		// Control1
		method_registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)] = 0xE4;

		// Filter
		method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] =
			((/*bias*/0) | ((/*conv*/1) << 13) | ((/*min*/5) << 16) | ((/*mag*/2) << 24)
			| ((/*as*/0) << 28) | ((/*rs*/0) << 29) | ((/*gs*/0) << 30) | ((/*bs*/0) << 31));

		// Image Rect
		method_registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)] = (/*height*/1) | ((/*width*/1) << 16);

		// Border Color
		method_registers[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index * 8)] = 0;
	}

	u32 texture::offset() const
	{
		return method_registers[NV4097_SET_TEXTURE_OFFSET + (m_index * 8)];
	}

	u8 texture::location() const
	{
		return (method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] & 0x3) - 1;
	}

	bool texture::cubemap() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 2) & 0x1);
	}

	u8 texture::border_type() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 3) & 0x1);
	}

	rsx::texture_dimension texture::dimension() const
	{
		return rsx::to_texture_dimension((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	rsx::texture_dimension_extended texture::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture_dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture_dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_2d;
		case rsx::texture_dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: ASSUME(0);
		}
	}

	u8 texture::format() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	bool texture::is_compressed_format() const
	{
		int texture_format = format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		if (texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT1 ||
			texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT23 ||
			texture_format == CELL_GCM_TEXTURE_COMPRESSED_DXT45)
			return true;
		return false;
	}

	u16 texture::mipmap() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 texture::get_exact_mipmap_count() const
	{
		if (is_compressed_format())
		{
			// OpenGL considers that highest mipmap level for DXTC format is when either width or height is 1
			// not both. Assume it's the same for others backend.
			u16 max_mipmap_count = static_cast<u16>(floor(log2(std::min(width() / 4, height() / 4))) + 1);
			return std::min(mipmap(), max_mipmap_count);
		}
		u16 max_mipmap_count = static_cast<u16>(floor(log2(std::max(width(), height()))) + 1);
		return std::min(mipmap(), max_mipmap_count);
	}

	rsx::texture_wrap_mode texture::wrap_s() const
	{
		return rsx::to_texture_wrap_mode((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)]) & 0xf);
	}

	rsx::texture_wrap_mode texture::wrap_t() const
	{
		return rsx::to_texture_wrap_mode((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 8) & 0xf);
	}

	rsx::texture_wrap_mode texture::wrap_r() const
	{
		return rsx::to_texture_wrap_mode((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 16) & 0xf);
	}

	u8 texture::unsigned_remap() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 12) & 0xf);
	}

	u8 texture::zfunc() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 28) & 0xf);
	}

	u8 texture::gamma() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 20) & 0xf);
	}

	u8 texture::aniso_bias() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 4) & 0xf);
	}

	u8 texture::signed_remap() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 24) & 0xf);
	}

	bool texture::enabled() const
	{
		return location() <= 1 && ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	u16  texture::min_lod() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	u16  texture::max_lod() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	rsx::texture_max_anisotropy   texture::max_aniso() const
	{
		return rsx::to_texture_max_anisotropy((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 4) & 0x7);
	}

	bool texture::alpha_kill_enabled() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 2) & 0x1);
	}

	u32 texture::remap() const
	{
		return (method_registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)]);
	}

	float texture::bias() const
	{
		return float(f16((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff));
	}

	rsx::texture_minify_filter texture::min_filter() const
	{
		return rsx::to_texture_minify_filter((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 16) & 0x7);
	}

	rsx::texture_magnify_filter texture::mag_filter() const
	{
		return rsx::to_texture_magnify_filter((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 24) & 0x7);
	}

	u8 texture::convolution_filter() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 13) & 0xf);
	}

	bool texture::a_signed() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 28) & 0x1);
	}

	bool texture::r_signed() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 29) & 0x1);
	}

	bool texture::g_signed() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 30) & 0x1);
	}

	bool texture::b_signed() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 31) & 0x1);
	}

	u16 texture::width() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 texture::height() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff);
	}

	u32 texture::border_color() const
	{
		return method_registers[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	u16 texture::depth() const
	{
		return method_registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] >> 20;
	}

	u32 texture::pitch() const
	{
		return method_registers[NV4097_SET_TEXTURE_CONTROL3 + m_index] & 0xfffff;
	}

	void vertex_texture::init(u8 index)
	{
		m_index = index;

		// Offset
		method_registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + (m_index * 8)] = 0;

		// Format
		method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] =
			((/*wraps*/1) | ((/*anisoBias*/0) << 4) | ((/*wrapt*/1) << 8) | ((/*unsignedRemap*/0) << 12) |
			((/*wrapr*/3) << 16) | ((/*gamma*/0) << 20) | ((/*signedRemap*/0) << 24) | ((/*zfunc*/0) << 28));

		// Control0
		method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] =
			(((/*alphakill*/0) << 2) | (/*maxaniso*/0) << 4) | ((/*maxlod*/0xc00) << 7) | ((/*minlod*/0) << 19) | ((/*enable*/0) << 31);

		// Control1
		//method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL1 + (m_index * 8)] = 0xE4;

		// Filter
		method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] =
			((/*bias*/0) | ((/*conv*/1) << 13) | ((/*min*/5) << 16) | ((/*mag*/2) << 24)
			| ((/*as*/0) << 28) | ((/*rs*/0) << 29) | ((/*gs*/0) << 30) | ((/*bs*/0) << 31));

		// Image Rect
		method_registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)] = (/*height*/1) | ((/*width*/1) << 16);

		// Border Color
		method_registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (m_index * 8)] = 0;
	}

	u32 vertex_texture::offset() const
	{
		return method_registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + (m_index * 8)];
	}

	u8 vertex_texture::location() const
	{
		return (method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] & 0x3) - 1;
	}

	bool vertex_texture::cubemap() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 2) & 0x1);
	}

	u8 vertex_texture::border_type() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 3) & 0x1);
	}

	rsx::texture_dimension vertex_texture::dimension() const
	{
		return rsx::to_texture_dimension((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	rsx::texture_dimension_extended vertex_texture::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture_dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture_dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_2d;
		case rsx::texture_dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: ASSUME(0);
		}
	}

	u8 vertex_texture::format() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	u16 vertex_texture::mipmap() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 vertex_texture::get_exact_mipmap_count() const
	{
		u16 max_mipmap_count = static_cast<u16>(floor(log2(std::max(width(), height()))) + 1);
		return std::min(mipmap(), max_mipmap_count);
	}

	u8 vertex_texture::unsigned_remap() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 12) & 0xf);
	}

	u8 vertex_texture::zfunc() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 28) & 0xf);
	}

	u8 vertex_texture::gamma() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 20) & 0xf);
	}

	u8 vertex_texture::aniso_bias() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 4) & 0xf);
	}

	u8 vertex_texture::signed_remap() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 24) & 0xf);
	}

	bool vertex_texture::enabled() const
	{
		return location() <= 1 && ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	u16 vertex_texture::min_lod() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	u16 vertex_texture::max_lod() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	rsx::texture_max_anisotropy vertex_texture::max_aniso() const
	{
		return rsx::to_texture_max_anisotropy((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 4) & 0x7);
	}

	bool vertex_texture::alpha_kill_enabled() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 2) & 0x1);
	}

	u16 vertex_texture::bias() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
	}

	rsx::texture_minify_filter vertex_texture::min_filter() const
	{
		return rsx::to_texture_minify_filter((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 16) & 0x7);
	}

	rsx::texture_magnify_filter vertex_texture::mag_filter() const
	{
		return rsx::to_texture_magnify_filter((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 24) & 0x7);
	}

	u8 vertex_texture::convolution_filter() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 13) & 0xf);
	}

	bool vertex_texture::a_signed() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 28) & 0x1);
	}

	bool vertex_texture::r_signed() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 29) & 0x1);
	}

	bool vertex_texture::g_signed() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 30) & 0x1);
	}

	bool vertex_texture::b_signed() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 31) & 0x1);
	}

	u16 vertex_texture::width() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)] >> 16) & 0xffff);
	}

	u16 vertex_texture::height() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + (m_index * 8)]) & 0xffff);
	}

	u32 vertex_texture::border_color() const
	{
		return method_registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + (m_index * 8)];
	}

	u16 vertex_texture::depth() const
	{
		return method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] >> 20;
	}

	u32 vertex_texture::pitch() const
	{
		return method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + (m_index * 8)] & 0xfffff;
	}
}