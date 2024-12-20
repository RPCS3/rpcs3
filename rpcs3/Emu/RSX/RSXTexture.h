#pragma once
#include "gcm_enums.h"
#include "color_utils.h"

namespace rsx
{
	class fragment_texture
	{
	protected:
		const u8 m_index;
		std::array<u32, 0x10000 / 4>& registers;

	public:
		fragment_texture(u8 idx, std::array<u32, 0x10000 / 4>& r)
			: m_index(idx)
			, registers(r)
		{
		}

		fragment_texture() = delete;

		// Offset
		u32 offset() const;

		// Format
		u8 location() const;
		bool cubemap() const;
		u8 border_type() const;
		rsx::texture_dimension dimension() const;

		// 2D texture can be either plane or cubemap texture depending on cubemap bit.
		// Since cubemap is a format per se in all gfx API this function directly returns
		// cubemap as a separate dimension.
		rsx::texture_dimension_extended get_extended_texture_dimension() const;
		u8 format() const;
		bool is_compressed_format() const;
		u16 mipmap() const;

		// mipmap() returns value from register which can be higher than the actual number of mipmap level.
		// This function clamp the result with the mipmap count allowed by texture size.
		u16 get_exact_mipmap_count() const;

		// Address
		rsx::texture_wrap_mode wrap_s() const;
		rsx::texture_wrap_mode wrap_t() const;
		rsx::texture_wrap_mode wrap_r() const;
		rsx::comparison_function zfunc() const;
		u8 unsigned_remap() const;
		u8 gamma() const;
		u8 aniso_bias() const;
		u8 signed_remap() const;

		// Control0
		bool enabled() const;
		f32 min_lod() const;
		f32 max_lod() const;
		rsx::texture_max_anisotropy max_aniso() const;
		bool alpha_kill_enabled() const;

		// Control1
		u32 remap() const;
		rsx::texture_channel_remap_t decoded_remap() const;

		// Filter
		f32 bias() const;
		rsx::texture_minify_filter min_filter() const;
		rsx::texture_magnify_filter mag_filter() const;
		u8 convolution_filter() const;
		u8 argb_signed() const;
		bool a_signed() const;
		bool r_signed() const;
		bool g_signed() const;
		bool b_signed() const;

		// Image Rect
		u16 width() const;
		u16 height() const;

		// Border Color
		u32 border_color() const;
		color4f remapped_border_color() const;

		u16 depth() const;
		u32 pitch() const;
	};

	class vertex_texture
	{
	protected:
		const u8 m_index;
		std::array<u32, 0x10000 / 4>& registers;

	public:
		vertex_texture(u8 idx, std::array<u32, 0x10000 / 4> &r)
			: m_index(idx)
			, registers(r)
		{
		}

		vertex_texture() = delete;

		// Offset
		u32 offset() const;

		// Format
		u8 location() const;
		bool cubemap() const;
		u8 border_type() const;
		rsx::texture_dimension dimension() const;
		u8 format() const;
		u16 mipmap() const;

		// Address
		rsx::texture_wrap_mode wrap_s() const;
		rsx::texture_wrap_mode wrap_t() const;
		rsx::texture_wrap_mode wrap_r() const;

		rsx::texture_channel_remap_t decoded_remap() const;
		u32 remap() const;

		// Control0
		bool enabled() const;
		f32 min_lod() const;
		f32 max_lod() const;

		// Filter
		f32 bias() const;
		rsx::texture_minify_filter min_filter() const;
		rsx::texture_magnify_filter mag_filter() const;

		// Image Rect
		u16 width() const;
		u16 height() const;

		// Border Color
		u32 border_color() const;
		color4f remapped_border_color() const;

		u16 depth() const;
		u32 pitch() const;

		rsx::texture_dimension_extended get_extended_texture_dimension() const;
		u16 get_exact_mipmap_count() const;
	};

	template<typename T>
	concept Texture = std::is_same_v<T, fragment_texture> || std::is_same_v<T, vertex_texture>;
}
