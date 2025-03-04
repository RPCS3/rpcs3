#pragma once

#include <util/types.hpp>
#include <Utilities/geometry.h>
#include "gcm_enums.h"
#include "Utilities/StrFmt.h"

namespace rsx
{
	struct texture_channel_remap_t
	{
		u32 encoded = 0xDEAD;
		std::array<u8, 4> control_map;
		std::array<u8, 4> channel_map;

		template <typename T>
		std::array<T, 4> remap(const std::array<T, 4>& components, T select_zero, T select_one) const
		{
			ensure(encoded != 0xDEAD, "Channel remap was not initialized");

			std::array<T, 4> remapped{};
			for (u8 channel = 0; channel < 4; ++channel)
			{
				switch (control_map[channel])
				{
				default:
					[[fallthrough]];
				case CELL_GCM_TEXTURE_REMAP_REMAP:
					remapped[channel] = components[channel_map[channel]];
					break;
				case CELL_GCM_TEXTURE_REMAP_ZERO:
					remapped[channel] = select_zero;
					break;
				case CELL_GCM_TEXTURE_REMAP_ONE:
					remapped[channel] = select_one;
					break;
				}
			}
			return remapped;
		}

		template <typename T>
			requires std::is_integral_v<T> || std::is_floating_point_v<T>
		std::array<T, 4> remap(const std::array<T, 4>& components) const
		{
			return remap(components, static_cast<T>(0), static_cast<T>(1));
		}

		template <typename T>
		color4_base<T> remap(const color4_base<T>& components)
		{
			const std::array<T, 4> values = { components.a, components.r, components.g, components.b };
			const auto shuffled = remap(values, T{ 0 }, T{ 1 });
			return color4_base<T>(shuffled[1], shuffled[2], shuffled[3], shuffled[0]);
		}

		template <typename T>
			requires std::is_integral_v<T> || std::is_enum_v<T>
		texture_channel_remap_t with_encoding(T encoding) const
		{
			texture_channel_remap_t result = *this;
			result.encoded = encoding;
			return result;
		}
	};

	static const texture_channel_remap_t default_remap_vector =
	{
		.encoded = RSX_TEXTURE_REMAP_IDENTITY,
		.control_map = { CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP },
		.channel_map = { CELL_GCM_TEXTURE_REMAP_FROM_A, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_B }
	};

	static inline texture_channel_remap_t decode_remap_encoding(u32 remap_ctl)
	{
		// Remapping tables; format is A-R-G-B
		// Remap input table. Contains channel index to read color from
		texture_channel_remap_t result =
		{
			.encoded = remap_ctl
		};

		result.channel_map =
		{
			static_cast<u8>(remap_ctl & 0x3),
			static_cast<u8>((remap_ctl >> 2) & 0x3),
			static_cast<u8>((remap_ctl >> 4) & 0x3),
			static_cast<u8>((remap_ctl >> 6) & 0x3),
		};

		// Remap control table. Controls whether the remap value is used, or force either 0 or 1
		result.control_map =
		{
			static_cast<u8>((remap_ctl >> 8) & 0x3),
			static_cast<u8>((remap_ctl >> 10) & 0x3),
			static_cast<u8>((remap_ctl >> 12) & 0x3),
			static_cast<u8>((remap_ctl >> 14) & 0x3),
		};

		return result;
	}

	// Convert color write mask for G8B8 to R8G8
	static inline u32 get_g8b8_r8g8_clearmask(u32 mask)
	{
		u32 result = 0;
		if (mask & RSX_GCM_CLEAR_GREEN_BIT) result |= RSX_GCM_CLEAR_GREEN_BIT;
		if (mask & RSX_GCM_CLEAR_BLUE_BIT) result |= RSX_GCM_CLEAR_RED_BIT;

		return result;
	}

	static inline void get_g8b8_r8g8_colormask(bool& red, bool/*green*/, bool& blue, bool& alpha)
	{
		red = blue;
		blue = false;
		alpha = false;
	}

	static inline void get_g8b8_clear_color(u8& red, u8 /*green*/, u8 blue, u8 /*alpha*/)
	{
		red = blue;
	}

	static inline u32 get_abgr8_clearmask(u32 mask)
	{
		u32 result = 0;
		if (mask & RSX_GCM_CLEAR_RED_BIT) result |= RSX_GCM_CLEAR_BLUE_BIT;
		if (mask & RSX_GCM_CLEAR_GREEN_BIT) result |= RSX_GCM_CLEAR_GREEN_BIT;
		if (mask & RSX_GCM_CLEAR_BLUE_BIT) result |= RSX_GCM_CLEAR_RED_BIT;
		if (mask & RSX_GCM_CLEAR_ALPHA_BIT) result |= RSX_GCM_CLEAR_ALPHA_BIT;
		return result;
	}

	static inline void get_abgr8_colormask(bool& red, bool /*green*/, bool& blue, bool /*alpha*/)
	{
		std::swap(red, blue);
	}

	static inline void get_abgr8_clear_color(u8& red, u8 /*green*/, u8& blue, u8 /*alpha*/)
	{
		std::swap(red, blue);
	}

	template <typename T, typename U>
		requires std::is_integral_v<T>&& std::is_integral_v<U>
	u8 renormalize_color8(T input, U base)
	{
		// Base will be some POT-1 value
		const int value = static_cast<u8>(input & base);
		return static_cast<u8>((value * 255) / base);
	}

	static inline void get_rgb565_clear_color(u8& red, u8& green, u8& blue, u8 /*alpha*/)
	{
		// RSX clear color is just a memcpy, so in this case the input is ARGB8 so only BG have the 16-bit input
		const u16 raw_value = static_cast<u16>(green) << 8 | blue;
		blue = renormalize_color8(raw_value, 0x1f);
		green = renormalize_color8(raw_value >> 5, 0x3f);
		red = renormalize_color8(raw_value >> 11, 0x1f);
	}

	static inline void get_a1rgb555_clear_color(u8& red, u8& green, u8& blue, u8& alpha, u8 alpha_override)
	{
		// RSX clear color is just a memcpy, so in this case the input is ARGB8 so only BG have the 16-bit input
		const u16 raw_value = static_cast<u16>(green) << 8 | blue;
		blue = renormalize_color8(raw_value, 0x1f);
		green = renormalize_color8(raw_value >> 5, 0x1f);
		red = renormalize_color8(raw_value >> 10, 0x1f);

		// Alpha can technically be encoded into the clear but the format normally just injects constants.
		// Will require hardware tests when possible to determine which approach makes more sense.
		// alpha = static_cast<u8>((raw_value & (1 << 15)) ? 255 : 0);
		alpha = alpha_override;
	}

	static inline u32 get_b8_clearmask(u32 mask)
	{
		u32 result = 0;
		if (mask & RSX_GCM_CLEAR_BLUE_BIT) result |= RSX_GCM_CLEAR_RED_BIT;
		return result;
	}

	static inline void get_b8_colormask(bool& red, bool& green, bool& blue, bool& alpha)
	{
		red = blue;
		green = false;
		blue = false;
		alpha = false;
	}

	static inline void get_b8_clear_color(u8& red, u8 /*green*/, u8& blue, u8 /*alpha*/)
	{
		std::swap(red, blue);
	}

	static inline color4f decode_border_color(u32 colorref)
	{
		color4f result;
		result.b = (colorref & 0xFF) / 255.f;
		result.g = ((colorref >> 8) & 0xFF) / 255.f;
		result.r = ((colorref >> 16) & 0xFF) / 255.f;
		result.a = ((colorref >> 24) & 0xFF) / 255.f;
		return result;
	}

	static inline u32 encode_color_to_storage_key(const color4f& color)
	{
		const u32 r = static_cast<u8>(color.r * 255);
		const u32 g = static_cast<u8>(color.g * 255);
		const u32 b = static_cast<u8>(color.b * 255);
		const u32 a = static_cast<u8>(color.a * 255);

		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	static inline const std::array<bool, 4> get_write_output_mask(rsx::surface_color_format format)
	{
		constexpr std::array<bool, 4> rgba = { true, true, true, true };
		constexpr std::array<bool, 4> rgb = { true, true, true, false };
		constexpr std::array<bool, 4> rg = { true, true, false, false };
		constexpr std::array<bool, 4> r = { true, false, false, false };

		switch (format)
		{
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
			return rgba;
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
			return rgb;
		case rsx::surface_color_format::g8b8:
			return rg;
		case rsx::surface_color_format::b8:
		case rsx::surface_color_format::x32:
			return r;
		default:
			fmt::throw_exception("Unknown surface format 0x%x", static_cast<int>(format));
		}
	}
}
