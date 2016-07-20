#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "RSXThread.h"
#include "RSXTexture.h"
#include "rsx_methods.h"

namespace rsx
{
	void texture_t::init()
	{
		// Offset
		m_offset = 0;

		// Format
		//registers[NV4097_SET_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		m_aniso_bias = 0;
		m_unsigned_remap = rsx::texture::unsigned_remap::normal;
		m_signed_remap = rsx::texture::signed_remap::normal;
		m_gamma_a = false;
		m_gamma_b = false;
		m_gamma_r = false;
		m_gamma_g = false;
		m_zfunc = rsx::texture::zfunc::never;
		m_wrap_r = rsx::texture::wrap_mode::wrap;
		m_wrap_s = rsx::texture::wrap_mode::wrap;
		m_wrap_t = rsx::texture::wrap_mode::wrap;

		// Control0
		m_alpha_kill_enabled = false;
		m_max_aniso = rsx::texture::max_anisotropy::x1;
		m_max_lod = 3072;
		m_min_lod = 0;
		m_enabled = false;

		// Control1
		m_remap_0 = rsx::texture::component_remap::A;
		m_remap_1 = rsx::texture::component_remap::R;
		m_remap_2 = rsx::texture::component_remap::G;
		m_remap_3 = rsx::texture::component_remap::B;

		// Filter
		m_bias = 0;
		m_convolution_filter = 1;
		m_min_filter = rsx::texture::minify_filter::nearest_linear;
		m_mag_filter = rsx::texture::magnify_filter::linear;
		m_a_signed = false;
		m_r_signed = false;
		m_b_signed = false;
		m_g_signed = false;

		// Image Rect
		m_width = 1;
		m_height = 1;

		m_depth = 1;

		// Border Color
		m_border_color_a = 0;
		m_border_color_b = 0;
		m_border_color_g = 0;
		m_border_color_r = 0;
	}

	u32 texture_t::offset() const
	{
		return m_offset;
	}

	u8 texture_t::location() const
	{
		return m_location;
	}

	bool texture_t::cubemap() const
	{
		return m_cubemap;
	}

	rsx::texture::border_type texture_t::border_type() const
	{
		return m_border_type;
	}

	rsx::texture::dimension texture_t::dimension() const
	{
		return m_dimension;
	}

	rsx::texture_dimension_extended texture_t::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture::dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture::dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_2d;
		case rsx::texture::dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: ASSUME(0);
		}
	}

	rsx::texture::format texture_t::format() const
	{
		return m_format;
	}

	rsx::texture::layout texture_t::layout() const
	{
		return m_layout;
	}

	rsx::texture::coordinates texture_t::normalization() const
	{
		return m_normalization;
	}

	bool texture_t::is_compressed_format() const
	{
		if (format() == rsx::texture::format::compressed_dxt1 ||
			format() == rsx::texture::format::compressed_dxt23 ||
			format() == rsx::texture::format::compressed_dxt45)
			return true;
		return false;
	}

	u16 texture_t::mipmap() const
	{
		return m_mipmap;
	}

	u16 texture_t::get_exact_mipmap_count() const
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

	rsx::texture::wrap_mode texture_t::wrap_s() const
	{
		return m_wrap_s;
	}

	rsx::texture::wrap_mode texture_t::wrap_t() const
	{
		return m_wrap_t;
	}

	rsx::texture::wrap_mode texture_t::wrap_r() const
	{
		return m_wrap_r;
	}

	rsx::texture::unsigned_remap texture_t::unsigned_remap() const
	{
		return m_unsigned_remap;
	}

	rsx::texture::zfunc texture_t::zfunc() const
	{
		return m_zfunc;
	}

	u8 texture_t::aniso_bias() const
	{
		return m_aniso_bias;
	}

	rsx::texture::signed_remap texture_t::signed_remap() const
	{
		return m_signed_remap;
	}

	bool texture_t::enabled() const
	{
		return m_enabled;
	}

	u16  texture_t::min_lod() const
	{
		return m_min_lod;
	}

	u16  texture_t::max_lod() const
	{
		return m_max_lod;
	}

	rsx::texture::max_anisotropy texture_t::max_aniso() const
	{
		return m_max_aniso;
	}

	bool texture_t::alpha_kill_enabled() const
	{
		return m_alpha_kill_enabled;
	}

	rsx::texture::component_remap texture_t::remap_0() const
	{
		return m_remap_0;
	}

	rsx::texture::component_remap texture_t::remap_1() const
	{
		return m_remap_1;
	}

	rsx::texture::component_remap texture_t::remap_2() const
	{
		return m_remap_2;
	}

	rsx::texture::component_remap texture_t::remap_3() const
	{
		return m_remap_3;
	}

	float texture_t::bias() const
	{
		return float(f16(m_bias));
	}

	rsx::texture::minify_filter texture_t::min_filter() const
	{
		return m_min_filter;
	}

	rsx::texture::magnify_filter texture_t::mag_filter() const
	{
		return m_mag_filter;
	}

	u8 texture_t::convolution_filter() const
	{
		return m_convolution_filter;
	}

	bool texture_t::a_signed() const
	{
		return m_a_signed;
	}

	bool texture_t::r_signed() const
	{
		return m_r_signed;
	}

	bool texture_t::g_signed() const
	{
		return m_g_signed;
	}

	bool texture_t::b_signed() const
	{
		return m_b_signed;
	}

	u16 texture_t::width() const
	{
		return m_width;
	}

	u16 texture_t::height() const
	{
		return m_height;
	}

	u8 texture_t::border_color_a() const
	{
		return m_border_color_a;
	}

	u8 texture_t::border_color_r() const
	{
		return m_border_color_r;
	}

	u8 texture_t::border_color_g() const
	{
		return m_border_color_g;
	}

	u8 texture_t::border_color_b() const
	{
		return m_border_color_b;
	}

	u16 texture_t::depth() const
	{
		return m_depth;
	}

	u32 texture_t::pitch() const
	{
		return m_pitch;
	}

	vertex_texture_t::vertex_texture_t()
	{

	}

	void vertex_texture_t::init()
	{
		// Offset
		m_offset = 0;

		// Format
		//registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (m_index * 8)] = 0;

		// Address
		m_aniso_bias = 0;
		m_unsigned_remap = rsx::texture::unsigned_remap::normal;
		m_signed_remap = rsx::texture::signed_remap::normal;
		m_gamma_a = false;
		m_gamma_b = false;
		m_gamma_r = false;
		m_gamma_g = false;
		m_zfunc = rsx::texture::zfunc::never;

		// Control0
		m_alpha_kill_enabled = false;
		m_max_aniso = rsx::texture::max_anisotropy::x1;
		m_max_lod = 3072;
		m_min_lod = 0;
		m_enabled = false;

		// Control1
		m_remap_0 = rsx::texture::component_remap::A;
		m_remap_1 = rsx::texture::component_remap::R;
		m_remap_2 = rsx::texture::component_remap::G;
		m_remap_3 = rsx::texture::component_remap::B;

		// Filter
		m_bias = 0;
		m_convolution_filter = 1;
		m_min_filter = rsx::texture::minify_filter::nearest_linear;
		m_mag_filter = rsx::texture::magnify_filter::linear;
		m_a_signed = false;
		m_r_signed = false;
		m_b_signed = false;
		m_g_signed = false;

		// Image Rect
		m_width = 1;
		m_height = 1;
		m_depth = 1;

		// Border Color
		m_border_color_a = 0;
		m_border_color_b = 0;
		m_border_color_g = 0;
		m_border_color_r = 0;
	}

	u32 vertex_texture_t::offset() const
	{
		return m_offset;
	}

	u8 vertex_texture_t::location() const
	{
		return m_location;
	}

	bool vertex_texture_t::cubemap() const
	{
		return m_cubemap;
	}

	rsx::texture::border_type vertex_texture_t::border_type() const
	{
		return m_border_type;
	}

	rsx::texture::dimension vertex_texture_t::dimension() const
	{
		return m_dimension;
	}

	rsx::texture_dimension_extended vertex_texture_t::get_extended_texture_dimension() const
	{
		switch (dimension())
		{
		case rsx::texture::dimension::dimension1d: return rsx::texture_dimension_extended::texture_dimension_1d;
		case rsx::texture::dimension::dimension3d: return rsx::texture_dimension_extended::texture_dimension_2d;
		case rsx::texture::dimension::dimension2d: return cubemap() ? rsx::texture_dimension_extended::texture_dimension_cubemap : rsx::texture_dimension_extended::texture_dimension_2d;

		default: ASSUME(0);
		}
	}

	rsx::texture::format vertex_texture_t::format() const
	{
		return m_format;
	}

	rsx::texture::layout vertex_texture_t::layout() const
	{
		return m_layout;
	}

	rsx::texture::coordinates vertex_texture_t::normalization() const
	{
		return rsx::texture::coordinates();
	}

	u16 vertex_texture_t::mipmap() const
	{
		return m_mipmap;
	}

	u16 vertex_texture_t::get_exact_mipmap_count() const
	{
		u16 max_mipmap_count = static_cast<u16>(floor(log2(std::max(width(), height()))) + 1);
		return std::min(mipmap(), max_mipmap_count);
	}

	rsx::texture::unsigned_remap vertex_texture_t::unsigned_remap() const
	{
		return m_unsigned_remap;
	}

	rsx::texture::zfunc vertex_texture_t::zfunc() const
	{
		return m_zfunc;
	}

	u8 vertex_texture_t::gamma() const
	{
		return m_gamma_a;
	}

	u8 vertex_texture_t::aniso_bias() const
	{
		return m_aniso_bias;
	}

	rsx::texture::signed_remap vertex_texture_t::signed_remap() const
	{
		return m_signed_remap;
	}

	bool vertex_texture_t::enabled() const
	{
		return m_enabled;
	}

	u16 vertex_texture_t::min_lod() const
	{
		return m_min_lod;
	}

	u16 vertex_texture_t::max_lod() const
	{
		return m_max_lod;
	}

	rsx::texture::max_anisotropy vertex_texture_t::max_aniso() const
	{
		return m_max_aniso;
	}

	bool vertex_texture_t::alpha_kill_enabled() const
	{
		return m_alpha_kill_enabled;
	}

	u16 vertex_texture_t::bias() const
	{
		return m_bias;
	}

	rsx::texture::minify_filter vertex_texture_t::min_filter() const
	{
		return m_min_filter;
	}

	rsx::texture::magnify_filter vertex_texture_t::mag_filter() const
	{
		return m_mag_filter;
	}

	u8 vertex_texture_t::convolution_filter() const
	{
		return m_convolution_filter;
	}

	bool vertex_texture_t::a_signed() const
	{
		return m_a_signed;
	}

	bool vertex_texture_t::r_signed() const
	{
		return m_r_signed;
	}

	bool vertex_texture_t::g_signed() const
	{
		return m_g_signed;
	}

	bool vertex_texture_t::b_signed() const
	{
		return m_b_signed;
	}

	u16 vertex_texture_t::width() const
	{
		return m_width;
	}

	u16 vertex_texture_t::height() const
	{
		return m_height;
	}

	u32 vertex_texture_t::border_color() const
	{
		return m_border_color_a;
	}

	u16 vertex_texture_t::depth() const
	{
		return m_depth;
	}

	u32 vertex_texture_t::pitch() const
	{
		return m_pitch;
	}
}