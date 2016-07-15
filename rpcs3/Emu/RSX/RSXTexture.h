#pragma once
#include "GCM.h"

namespace rsx
{
	/**
	* Use an extra cubemap format
	*/
	enum class texture_dimension_extended : u8
	{
		texture_dimension_1d = 0,
		texture_dimension_2d = 1,
		texture_dimension_cubemap = 2,
		texture_dimension_3d = 3,
	};

	class texture_t
	{
	public:
		//initialize texture registers with default values
		void init();

		u32 m_offset;
		u8 m_location : 2;
		bool m_cubemap : 1;
		rsx::texture::border_type m_border_type;
		rsx::texture::dimension m_dimension : 2;
		rsx::texture::format m_format;
		rsx::texture::layout m_layout : 1;
		rsx::texture::coordinates m_normalization : 1;
		u16 m_mipmap;
		rsx::texture::wrap_mode m_wrap_r : 3;
		rsx::texture::wrap_mode m_wrap_s : 3;
		rsx::texture::wrap_mode m_wrap_t : 3;
		rsx::texture::zfunc m_zfunc : 3;
		rsx::texture::unsigned_remap m_unsigned_remap: 1;
		rsx::texture::signed_remap m_signed_remap: 1;
		u8 m_aniso_bias : 4;
		bool m_gamma_r : 1;
		bool m_gamma_g : 1;
		bool m_gamma_b : 1;
		bool m_gamma_a : 1;

		u16 m_bias : 13;
		rsx::texture::minify_filter m_min_filter : 3;
		rsx::texture::magnify_filter m_mag_filter : 2;
		u8 m_convolution_filter : 3;
		bool m_a_signed : 1;
		bool m_r_signed : 1;
		bool m_g_signed : 1;
		bool m_b_signed : 1;

		bool m_enabled : 1;

		u16 m_width;
		u16 m_height;

		u16 m_depth : 12;
		u32 m_pitch : 20;

		bool m_alpha_kill_enabled : 1;
		u16 m_min_lod : 12;
		u16 m_max_lod : 12;
		rsx::texture::max_anisotropy m_max_aniso : 3;

		// border color
		u8 m_border_color_r;
		u8 m_border_color_g;
		u8 m_border_color_b;
		u8 m_border_color_a;

		// remap
		rsx::texture::component_remap m_remap_0 : 2;
		rsx::texture::component_remap m_remap_1 : 2;
		rsx::texture::component_remap m_remap_2 : 2;
		rsx::texture::component_remap m_remap_3 : 2;

		// Offset
		u32 offset() const;

		// Format
		u8   location() const;
		bool cubemap() const;
		rsx::texture::border_type   border_type() const;
		rsx::texture::dimension   dimension() const;
		rsx::texture::coordinates normalization() const;
		rsx::texture::layout layout() const;
		/**
		 * 2d texture can be either plane or cubemap texture depending on cubemap bit.
		 * Since cubemap is a format per se in all gfx API this function directly returns
		 * cubemap as a separate dimension.
		 */
		rsx::texture_dimension_extended get_extended_texture_dimension() const;
		rsx::texture::format   format() const;
		bool is_compressed_format() const;
		u16  mipmap() const;
		/**
		 * mipmap() returns value from register which can be higher than the actual number of mipmap level.
		 * This function clamp the result with the mipmap count allowed by texture size.
		 */
		u16 get_exact_mipmap_count() const;

		// Address
		rsx::texture::wrap_mode wrap_s() const;
		rsx::texture::wrap_mode wrap_t() const;
		rsx::texture::wrap_mode wrap_r() const;
		rsx::texture::unsigned_remap unsigned_remap() const;
		rsx::texture::zfunc zfunc() const;
		u8 aniso_bias() const;
		rsx::texture::signed_remap signed_remap() const;

		// Control0
		bool enabled() const;
		u16  min_lod() const;
		u16  max_lod() const;
		rsx::texture::max_anisotropy   max_aniso() const;
		bool alpha_kill_enabled() const;

		// Control1
		rsx::texture::component_remap remap_0() const;
		rsx::texture::component_remap remap_1() const;
		rsx::texture::component_remap remap_2() const;
		rsx::texture::component_remap remap_3() const;

		// Filter
		float bias() const;
		rsx::texture::minify_filter  min_filter() const;
		rsx::texture::magnify_filter  mag_filter() const;
		u8  convolution_filter() const;
		bool a_signed() const;
		bool r_signed() const;
		bool g_signed() const;
		bool b_signed() const;

		// Image Rect
		u16 width() const;
		u16 height() const;

		// Border Color
		u8 border_color_a() const;
		u8 border_color_r() const;
		u8 border_color_g() const;
		u8 border_color_b() const;

		u16 depth() const;
		u32 pitch() const;
	};

	class vertex_texture_t
	{
	public:
		vertex_texture_t();

		//initialize texture registers with default values
		void init();

		u32 m_offset;
		u8 m_location : 2;

		bool m_enabled : 1;

		u16 m_mipmap;
		rsx::texture::zfunc m_zfunc : 3;
		rsx::texture::unsigned_remap m_unsigned_remap : 1;
		rsx::texture::signed_remap m_signed_remap : 1;
		u8 m_aniso_bias : 4;
		bool m_gamma_r : 1;
		bool m_gamma_g : 1;
		bool m_gamma_b : 1;
		bool m_gamma_a : 1;

		u16 m_bias : 13;
		rsx::texture::minify_filter m_min_filter : 3;
		rsx::texture::magnify_filter m_mag_filter : 2;
		u8 m_convolution_filter : 3;
		bool m_a_signed : 1;
		bool m_r_signed : 1;
		bool m_g_signed : 1;
		bool m_b_signed : 1;

		bool m_cubemap : 1;
		rsx::texture::border_type m_border_type;
		rsx::texture::dimension m_dimension : 2;
		rsx::texture::format m_format;
		rsx::texture::layout m_layout;

		u16 m_width;
		u16 m_height;

		u16 m_depth : 12;
		u32 m_pitch : 20;

		bool m_alpha_kill_enabled : 1;
		u16 m_min_lod : 12;
		u16 m_max_lod : 12;
		rsx::texture::max_anisotropy m_max_aniso : 3;

		// border color
		u8 m_border_color_r;
		u8 m_border_color_g;
		u8 m_border_color_b;
		u8 m_border_color_a;

		// remap
		rsx::texture::component_remap m_remap_0 : 2;
		rsx::texture::component_remap m_remap_1 : 2;
		rsx::texture::component_remap m_remap_2 : 2;
		rsx::texture::component_remap m_remap_3 : 2;

		// Offset
		u32 offset() const;

		// Format
		u8 location() const;
		bool cubemap() const;
		rsx::texture::border_type border_type() const;
		rsx::texture::dimension dimension() const;
		rsx::texture::format format() const;
		rsx::texture::layout layout() const;
		rsx::texture::coordinates normalization() const;
		u16 mipmap() const;

		// Address
		rsx::texture::unsigned_remap unsigned_remap() const;
		rsx::texture::zfunc zfunc() const;
		u8 gamma() const;
		u8 aniso_bias() const;
		rsx::texture::signed_remap signed_remap() const;

		// Control0
		bool enabled() const;
		u16 min_lod() const;
		u16 max_lod() const;
		rsx::texture::max_anisotropy max_aniso() const;
		bool alpha_kill_enabled() const;

		// Filter
		u16 bias() const;
		rsx::texture::minify_filter min_filter() const;
		rsx::texture::magnify_filter mag_filter() const;
		u8 convolution_filter() const;
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

		rsx::texture_dimension_extended get_extended_texture_dimension() const;
		u16 get_exact_mipmap_count() const;
	};
}
