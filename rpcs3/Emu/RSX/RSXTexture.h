#pragma once

namespace rsx
{
	class texture
	{
	protected:
		u8 m_index;

	public:
		//initialize texture registers with default values
		void init(u8 index);

		// Offset
		u32 offset() const;

		// Format
		u8   location() const;
		bool cubemap() const;
		u8   border_type() const;
		u8   dimension() const;
		u8   format() const;
		u16  mipmap() const;

		// Address
		u8 wrap_s() const;
		u8 wrap_t() const;
		u8 wrap_r() const;
		u8 unsigned_remap() const;
		u8 zfunc() const;
		u8 gamma() const;
		u8 aniso_bias() const;
		u8 signed_remap() const;

		// Control0
		bool enabled() const;
		u16  min_lod() const;
		u16  max_lod() const;
		u8   max_aniso() const;
		bool alpha_kill_enabled() const;

		// Control1
		u32 remap() const;

		// Filter
		u16 bias() const;
		u8  min_filter() const;
		u8  mag_filter() const;
		u8  convolution_filter() const;
		bool a_signed() const;
		bool r_signed() const;
		bool g_signed() const;
		bool b_signed() const;

		// Image Rect
		u16 width() const;
		u16 height() const;

		// Border Color
		u32 border_color() const;
		u16 depth() const;
		u32 pitch() const;

		//custom info
		u8 index() const;
	};

	class vertex_texture
	{
	protected:
		u8 m_index;

	public:
		//initialize texture registers with default values
		void init(u8 index);

		// Offset
		u32 offset() const;

		// Format
		u8   location() const;
		bool cubemap() const;
		u8   border_type() const;
		u8   dimension() const;
		u8   format() const;
		u16  mipmap() const;

		// Address
		u8 wrap_s() const;
		u8 wrap_t() const;
		u8 wrap_r() const;
		u8 unsigned_remap() const;
		u8 zfunc() const;
		u8 gamma() const;
		u8 aniso_bias() const;
		u8 signed_remap() const;

		// Control0
		bool enabled() const;
		u16  min_lod() const;
		u16  max_lod() const;
		u8   max_aniso() const;
		bool alpha_kill_enabled() const;

		// Control1
		u32 remap() const;

		// Filter
		u16 bias() const;
		u8  min_filter() const;
		u8  mag_filter() const;
		u8  convolution_filter() const;
		bool a_signed() const;
		bool r_signed() const;
		bool g_signed() const;
		bool b_signed() const;

		// Image Rect
		u16 width() const;
		u16 height() const;

		// Border Color
		u32 border_color() const;
		u16 depth() const;
		u32 pitch() const;

		//custom info
		u8 index() const;
	};
}
