#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "RSXThread.h"
#include "RSXTexture.h"

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

	u8 texture::dimension() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	u8 texture::format() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	u16 texture::mipmap() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u8 texture::wrap_s() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)]) & 0xf);
	}

	u8 texture::wrap_t() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 8) & 0xf);
	}

	u8 texture::wrap_r() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_ADDRESS + (m_index * 8)] >> 16) & 0xf);
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

	u8   texture::max_aniso() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 4) & 0x7);
	}

	bool texture::alpha_kill_enabled() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_CONTROL0 + (m_index * 8)] >> 2) & 0x1);
	}

	u32 texture::remap() const
	{
		return (method_registers[NV4097_SET_TEXTURE_CONTROL1 + (m_index * 8)]);
	}

	u16 texture::bias() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
	}

	u8  texture::min_filter() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 16) & 0x7);
	}

	u8  texture::mag_filter() const
	{
		return ((method_registers[NV4097_SET_TEXTURE_FILTER + (m_index * 8)] >> 24) & 0x7);
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
		return method_registers[NV4097_SET_TEXTURE_CONTROL3] >> 20;
	}

	u32 texture::pitch() const
	{
		return method_registers[NV4097_SET_TEXTURE_CONTROL3] & 0xfffff;
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

	u8 vertex_texture::dimension() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 4) & 0xf);
	}

	u8 vertex_texture::format() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 8) & 0xff);
	}

	u16 vertex_texture::mipmap() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] >> 16) & 0xffff);
	}

	u8 vertex_texture::wrap_s() const
	{
		return 1;
		//return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)]) & 0xf);
	}

	u8 vertex_texture::wrap_t() const
	{
		return 1;
		//return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 8) & 0xf);
	}

	u8 vertex_texture::wrap_r() const
	{
		return 1;
		//return ((method_registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + (m_index * 8)] >> 16) & 0xf);
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
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 31) & 0x1);
	}

	u16 vertex_texture::min_lod() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 19) & 0xfff);
	}

	u16 vertex_texture::max_lod() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 7) & 0xfff);
	}

	u8 vertex_texture::max_aniso() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 4) & 0x7);
	}

	bool vertex_texture::alpha_kill_enabled() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + (m_index * 8)] >> 2) & 0x1);
	}

	u32 vertex_texture::remap() const
	{
		return 0 | (1 << 2) | (2 << 4) | (3 << 6);//(method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL1 + (m_index * 8)]);
	}

	u16 vertex_texture::bias() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)]) & 0x1fff);
	}

	u8  vertex_texture::min_filter() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 16) & 0x7);
	}

	u8  vertex_texture::mag_filter() const
	{
		return ((method_registers[NV4097_SET_VERTEX_TEXTURE_FILTER + (m_index * 8)] >> 24) & 0x7);
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
		return method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3] >> 20;
	}

	u32 vertex_texture::pitch() const
	{
		return method_registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3] & 0xfffff;
	}
}