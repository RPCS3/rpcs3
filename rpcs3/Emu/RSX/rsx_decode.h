#pragma once

#include "util/types.hpp"
#include "Utilities/BitField.h"
#include "Utilities/StrFmt.h"
#include <tuple>
#include <algorithm>
#include "gcm_enums.h"
#include "rsx_utils.h"

namespace rsx
{
	enum class boolean_to_string_t : u8 {};

	constexpr boolean_to_string_t print_boolean(bool b)
	{
		return boolean_to_string_t{static_cast<u8>(b)};
	}

template <u16 Register>
struct registers_decoder
{};

// Use the smallest type by default
template <u32 I, u32 N, typename T = get_uint_t<(N <= 8 ? 1 : (N <= 16 ? 2 : 4))>>
constexpr T bf_decoder(u32 bits)
{
	return static_cast<T>(bf_t<u32, I, N>::extract(bits));
}

template <>
struct registers_decoder<NV406E_SET_REFERENCE>
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV406E Ref: 0x%08x", decoded.value);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_HORIZONTAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 width() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: x: %u width: %u", decoded.origin_x(), decoded.width());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_VERTICAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_y() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: y: %u height: %u", decoded.origin_y(), decoded.height());
	}
};

template<>
struct registers_decoder<NV4097_SET_SCISSOR_HORIZONTAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 width() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Scissor: x: %u width: %u", decoded.origin_x(), decoded.width());
	}
};

template<>
struct registers_decoder<NV4097_SET_SCISSOR_VERTICAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_y() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Scissor: y: %u height: %u",  decoded.origin_y(), decoded.height());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_CLIP_HORIZONTAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 width() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: clip x: %u width: %u", decoded.origin_x(), decoded.width());
	}
};

template<>
struct registers_decoder< NV4097_SET_SURFACE_CLIP_VERTICAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_y() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: clip y: %u height: %u", decoded.origin_y(), decoded.height());
	}
};

template<>
struct registers_decoder<NV4097_SET_CLEAR_RECT_HORIZONTAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 width() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Clear: rect x: %u width: %u", decoded.origin_x(), decoded.width());
	}
};

template<>
struct registers_decoder<NV4097_SET_CLEAR_RECT_VERTICAL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 origin_y() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Clear: rect y: %u height: %u", decoded.origin_y(), decoded.height());
	}
};

template<>
struct registers_decoder< NV3089_CLIP_POINT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 clip_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 clip_y() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: clip x: %u y: %u", decoded.clip_x(), decoded.clip_y());
	}
};

template<>
struct registers_decoder<NV3089_CLIP_SIZE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 clip_width() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 clip_height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: clip width: %u height: %u", decoded.clip_width(), decoded.clip_height());
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_OUT_POINT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 y() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: output x: %u y: %u", decoded.x(), decoded.y());
	}
};

template<>
struct registers_decoder<NV4097_SET_WINDOW_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 window_offset_x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 window_offset_y() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Window: offset x: %u y: %u", decoded.window_offset_x(), decoded.window_offset_y());
	}
};


template<>
struct registers_decoder<NV3089_IMAGE_OUT_SIZE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 width() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: output width: %u height: %u", decoded.width(), decoded.height());
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_SIZE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 width() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 height() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: input width: %u height: %u", decoded.width(), decoded.height());
	}
};

template<>
struct registers_decoder<NV3062_SET_PITCH>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 alignment() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 pitch() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blit engine: output alignment: %u pitch: %u", decoded.alignment(), decoded.pitch());
	}
};

template<>
struct registers_decoder< NV308A_POINT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 x() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 y() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV308A: x: %u y: %u", decoded.x(), decoded.y());
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_ATTRIB_INPUT_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 mask() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		out += "Transform program enabled inputs:";
		constexpr std::string_view input_names[] =
		{
			"in_pos", "in_weight", "in_normal",
			"in_diff_color", "in_spec_color",
			"in_fog",
			"in_point_size", "in_7",
			"in_tc0", "in_tc1", "in_tc2", "in_tc3",
			"in_tc4", "in_tc5", "in_tc6", "in_tc7"
		};

		for (u32 i = 0; i < 16; i++)
		{
			if (decoded.mask() & (1 << i))
			{
				out += ' ';
				out += input_names[i];
			}
		}
	}
};

template<>
struct registers_decoder<NV4097_SET_FREQUENCY_DIVIDER_OPERATION>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 frequency_divider_operation_mask() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		out += "Frequency divider:";

		const u32 mask = decoded.frequency_divider_operation_mask();

		if (!mask)
		{
			out += " (none)";
			return;
		}

		for (u32 i = 0; i < 16; i++)
		{
			if (mask & (1 << i))
			{
				out += ' ';
				fmt::append(out, "%u", i);
			}
		}
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_TEST_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool depth_test_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: test %s", print_boolean(decoded.depth_test_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_MASK>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool depth_write_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: write: %s", print_boolean(decoded.depth_write_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_ZMIN_MAX_CONTROL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool depth_clip_enabled() const
		{
			return bf_decoder<0, 4, bool>(value);
		}

		bool depth_clamp_enabled() const
		{
			return bf_decoder<4, 4, bool>(value);
		}

		bool depth_clip_ignore_w() const
		{
			return bf_decoder<8, 4, bool>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: clip_enabled: %s, clamp: %s, ignore_w: %s", print_boolean(decoded.depth_clip_enabled()), print_boolean(decoded.depth_clamp_enabled()) , print_boolean(decoded.depth_clip_ignore_w()));
	}
};

template<>
struct registers_decoder<NV4097_SET_ALPHA_TEST_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool alpha_test_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Alpha: test %s", print_boolean(decoded.alpha_test_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_TEST_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool stencil_test_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: test %s", print_boolean(decoded.stencil_test_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_RESTART_INDEX_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool restart_index_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Restart Index: %s", print_boolean(decoded.restart_index_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool depth_bound_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: bound test %s", print_boolean(decoded.depth_bound_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_LOGIC_OP_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool logic_op_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Logic: %s", print_boolean(decoded.logic_op_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_DITHER_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool dither_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Dither: %s", print_boolean(decoded.dither_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool blend_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend: %s", print_boolean(decoded.blend_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_LINE_SMOOTH_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool line_smooth_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Line smooth: %s", print_boolean(decoded.line_smooth_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_POINT_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool poly_offset_point_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: offset point: %s", print_boolean(decoded.poly_offset_point_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_LINE_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool poly_offset_line_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: offset line: %s", print_boolean(decoded.poly_offset_line_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_FILL_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool poly_offset_fill_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: offset fill: %s", print_boolean(decoded.poly_offset_fill_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_CULL_FACE_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool cull_face_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Cull face: %s", print_boolean(decoded.cull_face_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_SMOOTH_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool poly_smooth_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: smooth: %s", print_boolean(decoded.poly_smooth_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool two_sided_stencil_test_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: per side: %s", print_boolean(decoded.two_sided_stencil_test_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_TWO_SIDE_LIGHT_EN>
{
	struct decoded_type
	{
	private:
		u32 enabled;

	public:
		decoded_type(u32 value) : enabled(value) {}

		bool two_sided_lighting_enabled() const
		{
			return bool(enabled);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Light: per side: %s", print_boolean(decoded.two_sided_lighting_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_MIN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 depth_bound_min() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: bound min: %g", decoded.depth_bound_min());
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_MAX>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 depth_bound_max() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: bound max: %g", decoded.depth_bound_max());
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_PARAMS>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 fog_param_0() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Fog: param 0: %g", decoded.fog_param_0());
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_PARAMS + 1>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 fog_param_1() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Fog: param 1: %g", decoded.fog_param_1());
	}
};

template<>
struct registers_decoder<NV4097_SET_CLIP_MIN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 clip_min() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: clip min: %g", decoded.clip_min());
	}
};

template<>
struct registers_decoder<NV4097_SET_CLIP_MAX>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 clip_max() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: clip max: %g", decoded.clip_max());
	}
};

template<>
struct registers_decoder<NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 polygon_offset_scale_factor() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: offset scale: %g", decoded.polygon_offset_scale_factor());
	}
};

template<>
struct registers_decoder<NV4097_SET_POLYGON_OFFSET_BIAS>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 polygon_offset_scale_bias() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Polygon: offset bias: %g", decoded.polygon_offset_scale_bias());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_scale_x() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: scale x: %g", decoded.viewport_scale_x());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 1>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_scale_y() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: scale y: %g", decoded.viewport_scale_y());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 2>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_scale_z() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: scale z: %g", decoded.viewport_scale_z());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 3>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_scale_w() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: scale w: %g", decoded.viewport_scale_w());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_offset_x() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: offset x: %g", decoded.viewport_offset_x());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 1>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_offset_y() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: offset y: %g", decoded.viewport_offset_y());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 2>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_offset_z() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: offset z: %g", decoded.viewport_offset_z());
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 3>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 viewport_offset_w() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: offset w: %g", decoded.viewport_offset_w());
	}
};

template<>
struct registers_decoder<NV4097_SET_RESTART_INDEX>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 restart_index() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Restart index: %u", decoded.restart_index());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_AOFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_a_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: A offset 0x%x", decoded.surface_a_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_BOFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_b_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: B offset 0x%x", decoded.surface_b_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_COFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_c_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: C offset 0x%x", decoded.surface_c_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_DOFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_d_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: D offset 0x%x", decoded.surface_d_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_A>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_a_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: A pitch: %u", decoded.surface_a_pitch());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_B>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_b_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: B pitch: %u", decoded.surface_b_pitch());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_C>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_c_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: C pitch: %u", decoded.surface_c_pitch());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_D>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_d_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: D pitch: %u", decoded.surface_d_pitch());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_ZETA_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_z_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: Z offset: 0x%x", decoded.surface_z_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_Z>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 surface_z_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: Z pitch: %u", decoded.surface_z_pitch());
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 output_mask() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		static constexpr std::string_view output_names[] =
		{
			"diffuse_color",
			"specular_color",
			"back_diffuse_color",
			"back_specular_color",
			"fog",
			"point_size",
			"clip_distance[0]",
			"clip_distance[1]",
			"clip_distance[2]",
			"clip_distance[3]",
			"clip_distance[4]",
			"clip_distance[5]",
			"tc8",
			"tc9",
			"tc0",
			"tc1",
			"tc2",
			"tc3",
			"tc4",
			"tc5",
			"tc6",
			"tc7"
		};

		out += "Transform program outputs:";

		const u32 mask = decoded.output_mask();

		for (u32 i = 0; i < 22; i++)
		{
			if (mask & (1 << i))
			{
				out += ' ';
				out += output_names[i];
			}
		}
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_CONTROL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 shader_ctrl() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Shader control: raw_value: 0x%x reg_count: %u%s%s",
			decoded.shader_ctrl(), ((decoded.shader_ctrl() >> 24) & 0xFF), ((decoded.shader_ctrl() & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) ? " depth_replace" : ""),
			((decoded.shader_ctrl() & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? " 32b_exports" : ""));
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_PACKER>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool srgb_output_enabled() const
		{
			return !!value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Shader packer control: srgb_enabled: %s", print_boolean(decoded.srgb_output_enabled()));
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_DATA_BASE_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 vertex_data_base_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Vertex: base offset 0x%x", decoded.vertex_data_base_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_INDEX_ARRAY_ADDRESS>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 index_array_offset() const
		{
			return bf_decoder<0, 29>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Index: array offset 0x%x", decoded.index_array_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_DATA_BASE_INDEX>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 vertex_data_base_index() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Vertex: base index: %u", decoded.vertex_data_base_index());
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_PROGRAM>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 shader_program_address() const
		{
			return bf_decoder<0, 31>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		const u32 address = decoded.shader_program_address();
		fmt::append(out, "Shader: %s, offset: 0x%x", CellGcmLocation{(address & 3) - 1}, address & ~3);
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM_START>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 transform_program_start() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Transform program: start: %u", decoded.transform_program_start());
	}
};

template<>
struct registers_decoder<NV406E_SET_CONTEXT_DMA_SEMAPHORE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation context_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV406E semaphore: context: %s", decoded.context_dma());
	}
};


template<>
struct registers_decoder<NV406E_SEMAPHORE_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 semaphore_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV406E semaphore: offset: 0x%x", decoded.semaphore_offset());
	}
};

template <>
struct registers_decoder<NV406E_SEMAPHORE_RELEASE>
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV406E semaphore: release: 0x%x", decoded.value);
	}
};

template <>
struct registers_decoder<NV406E_SEMAPHORE_ACQUIRE>
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV406E semaphore: acquire: 0x%x", decoded.value);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_SEMAPHORE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation context_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV4097 semaphore: context: %s", decoded.context_dma());
	}
};

template<>
struct registers_decoder<NV4097_SET_SEMAPHORE_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 semaphore_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV4097 semaphore: offset: 0x%x", decoded.semaphore_offset());
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 input_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: input offset: 0x%x", decoded.input_offset());
	}
};

template<>
struct registers_decoder<NV3062_SET_OFFSET_DESTIN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 output_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3062: output offset: 0x%x", decoded.output_offset());
	}
};

template<>
struct registers_decoder<NV309E_SET_OFFSET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV309E: offset: 0x%x", decoded.offset());
	}
};

template<>
struct registers_decoder<NV3089_DS_DX>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		// Convert signed fixed point 32-bit format
		f32 ds_dx() const
		{
			const u32 val = value;

			if (val == 0)
			{
				// Will get reported in image_in
				return 0;
			}

			return 1.f / rsx::decode_fxp<11, 20>(val);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: DS DX: %g", decoded.ds_dx());
	}
};

template<>
struct registers_decoder<NV3089_DT_DY>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		// Convert signed fixed point 32-bit format
		f32 dt_dy() const
		{
		    const u32 val = value;

			if (val == 0)
			{
				// Will get reported in image_in
				return 0.f;
			}

			return 1.f / rsx::decode_fxp<11, 20>(val);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: DT DY: %g", decoded.dt_dy());
	}
};

template<>
struct registers_decoder<NV0039_PITCH_IN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 input_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: input pitch: %u", decoded.input_pitch());
	}
};

template<>
struct registers_decoder<NV0039_PITCH_OUT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 output_pitch() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: output pitch: %u", decoded.output_pitch());
	}
};

template<>
struct registers_decoder<NV0039_LINE_LENGTH_IN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 input_line_length() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: line length input: %u", decoded.input_line_length());
	}
};

template<>
struct registers_decoder<NV0039_LINE_COUNT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 line_count() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: line count: %u", decoded.line_count());
	}
};

template<>
struct registers_decoder<NV0039_OFFSET_OUT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 output_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: output offset: 0x%x", decoded.output_offset());
	}
};

template<>
struct registers_decoder<NV0039_OFFSET_IN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 input_offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: input offset: 00x%x", decoded.input_offset());
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_FUNC>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto depth_func() const
		{
			return to_comparison_function(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth: compare_function: %s", decoded.depth_func());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto stencil_func() const
		{
			return to_comparison_function(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) compare_function: %s", decoded.stencil_func());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto back_stencil_func() const
		{
			return to_comparison_function(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: back compare_function: %s", decoded.back_stencil_func());
	}
};

template<>
struct registers_decoder<NV4097_SET_ALPHA_FUNC>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto alpha_func() const
		{
			return to_comparison_function(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Alpha: compare_function: %s", decoded.alpha_func());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_FAIL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto fail() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) fail op: %s", decoded.fail());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_ZFAIL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto zfail() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) zfail op: %s", decoded.zfail());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_ZPASS>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto zpass() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) zpass op: %s", decoded.zpass());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_FAIL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto back_fail() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) fail op: %s", decoded.back_fail());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_ZFAIL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto back_zfail() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) zfail op: %s", decoded.back_zfail());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_ZPASS>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto back_zpass() const
		{
			return to_stencil_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) zpass op: %s", decoded.back_zpass());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC_REF>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 stencil_func_ref() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) func ref: %u", decoded.stencil_func_ref());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC_REF>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 back_stencil_func_ref() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) func ref: %u", decoded.back_stencil_func_ref());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 stencil_func_mask() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) func mask: %u", decoded.stencil_func_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 back_stencil_func_mask() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) func mask: %u", decoded.back_stencil_func_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_ALPHA_REF>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 alpha_ref8() const
		{
			return bf_decoder<0, 8>(value) / 255.f;
		}

		f32 alpha_ref16() const
		{
			return rsx::decode_fp16(bf_decoder<0, 16>(value));
		}

		f32 alpha_ref32() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Alpha: ref unorm8: %g, f16: %g", decoded.alpha_ref8(), decoded.alpha_ref16());
	}
};

template<>
struct registers_decoder<NV4097_SET_COLOR_CLEAR_VALUE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 blue() const { return bf_decoder<0, 8>(value); }
		u8 green() const { return bf_decoder<8, 8>(value); }
		u8 red() const { return bf_decoder<16, 8>(value); }
		u8 alpha() const { return bf_decoder<24, 8>(value); }
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Clear: R = %u G = %u B = %u A = %u", decoded.red(), decoded.green(), decoded.blue(), decoded.alpha());
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 stencil_mask() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (front) mask: %u", decoded.stencil_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 back_stencil_mask() const
		{
			return bf_decoder<0, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Stencil: (back) mask: %u", decoded.back_stencil_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_LOGIC_OP>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto logic_operation() const
		{
			return to_logic_op(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Logic: op: %s", decoded.logic_operation());
	}
};

template<>
struct registers_decoder<NV4097_SET_FRONT_FACE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto front_face_mode() const
		{
			return to_front_face(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Front Face: %s", decoded.front_face_mode());
	}
};


template<>
struct registers_decoder<NV4097_SET_CULL_FACE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		cull_face cull_face_mode() const
		{
			return static_cast<cull_face>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Cull Face: %s", decoded.cull_face_mode());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_TARGET>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto target() const
		{
			return to_surface_target(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: Color target(s): %s", decoded.target());
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_MODE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto fog_equation() const
		{
			return to_fog_mode(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Fog: %s", decoded.fog_equation());
	}
};

template<>
struct registers_decoder<NV4097_SET_BEGIN_END>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto primitive() const
		{
			return to_primitive_type(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Primitive: %s", decoded.primitive());
	}
};

template<>
struct registers_decoder<NV3089_SET_OPERATION>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto transfer_op() const
		{
			return blit_engine::to_transfer_operation(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: op: %s", decoded.transfer_op());
	}
};

template<>
struct registers_decoder<NV3089_SET_COLOR_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto transfer_source_fmt() const
		{
			return blit_engine::to_transfer_source_format(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: source fmt: %s", decoded.transfer_source_fmt());
	}
};

template<>
struct registers_decoder<NV3089_SET_CONTEXT_SURFACE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto ctx_surface() const
		{
			return blit_engine::to_context_surface(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: context surface: %s", decoded.ctx_surface());
	}
};

template<>
struct registers_decoder<NV3062_SET_COLOR_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto transfer_dest_fmt() const
		{
			return blit_engine::to_transfer_destination_format(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3062: output fmt: %s", decoded.transfer_dest_fmt());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_EQUATION>
{
	struct decoded_type
	{
	private:
		u32 value;

		u16 blend_rgb_raw() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 blend_a_raw() const
		{
			return bf_decoder<16, 16>(value);
		}
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto blend_rgb() const
		{
			return to_blend_equation(blend_rgb_raw());
		}

		auto blend_a() const
		{
			return to_blend_equation(blend_a_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend: equation rgb: %s a: %s", decoded.blend_rgb(), decoded.blend_a());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_FUNC_SFACTOR>
{
	struct decoded_type
	{
	private:
		u32 value;

		u16 src_blend_rgb_raw() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 src_blend_a_raw() const
		{
			return bf_decoder<16, 16>(value);
		}
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto src_blend_rgb() const
		{
			return to_blend_factor(src_blend_rgb_raw());
		}

		auto src_blend_a() const
		{
			return to_blend_factor(src_blend_a_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend: sfactor rgb: %s a: %s", decoded.src_blend_rgb(), decoded.src_blend_a());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_FUNC_DFACTOR>
{
	struct decoded_type
	{
	private:
		u32 value;

		u16 dst_blend_rgb_raw() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 dst_blend_a_raw() const
		{
			return bf_decoder<16, 16>(value);
		}
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto dst_blend_rgb() const
		{
			return to_blend_factor(dst_blend_rgb_raw());
		}

		auto dst_blend_a() const
		{
			return to_blend_factor(dst_blend_a_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend: dfactor rgb: %s a: %s", decoded.dst_blend_rgb(), decoded.dst_blend_a());
	}
};

template<>
struct registers_decoder<NV4097_SET_COLOR_MASK>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool color_b() const
		{
			return bf_decoder<0, 8, bool>(value);
		}

		bool color_g() const
		{
			return bf_decoder<8, 8, bool>(value);
		}

		bool color_r() const
		{
			return bf_decoder<16, 8, bool>(value);
		}

		bool color_a() const
		{
			return bf_decoder<24, 8, bool>(value);
		}

		bool color_write_enabled() const
		{
			return value != 0;
		}

		u32 is_invalid() const
		{
			return (value & 0xfefefefe) ? value : 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		if (u32 invalid_value = decoded.is_invalid())
		{
			fmt::append(out, "Surface: color mask: invalid = 0x%08x", invalid_value);
			return;
		}

		fmt::append(out, "Surface: color mask A = %s R = %s G = %s B = %s"
			, print_boolean(decoded.color_a()), print_boolean(decoded.color_r()), print_boolean(decoded.color_g()), print_boolean(decoded.color_b()));
	}
};

template<>
struct registers_decoder<NV4097_SET_COLOR_MASK_MRT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool color_b(int index) const
		{
			return bf_decoder<3, 1, bool>(value >> (index * 4));
		}

		bool color_g(int index) const
		{
			return bf_decoder<2, 1, bool>(value >> (index * 4));
		}

		bool color_r(int index) const
		{
			return bf_decoder<1, 1, bool>(value >> (index * 4));
		}

		bool color_a(int index) const
		{
			return bf_decoder<0, 1, bool>(value >> (index * 4));
		}

		bool color_write_enabled(int index) const
		{
			return ((value >> (index * 4)) & 0xF) != 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		out += "Color Mask MRT:\n";

		for (int index = 1; index < 4; ++index)
		{
			fmt::append(out, "Surface[%d]: A:%d R:%d G:%d B:%d\n",
				index,
				decoded.color_a(index),
				decoded.color_r(index),
				decoded.color_g(index),
				decoded.color_b(index));
		}
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_WINDOW>
{
	struct decoded_type
	{
	private:
		u32 value;

		u8 window_shader_origin_raw() const { return bf_decoder<12, 4>(value); }
		u8 window_shader_pixel_center_raw() const { return bf_decoder<16, 4>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto window_shader_origin() const
		{
			return to_window_origin(window_shader_origin_raw());
		}

		auto window_shader_pixel_center() const
		{
			return to_window_pixel_center(window_shader_pixel_center_raw());
		}

		u16 window_shader_height() const
		{
			return bf_decoder<0, 12>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Viewport: height: %u origin: %s pixel center: %s", decoded.window_shader_height()
			, decoded.window_shader_origin(), decoded.window_shader_pixel_center());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_ENABLE_MRT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool blend_surface_b() const
		{
			return bf_decoder<1, 1, bool>(value);
		}

		bool blend_surface_c() const
		{
			return bf_decoder<2, 1, bool>(value);
		}

		bool blend_surface_d() const
		{
			return bf_decoder<3, 1, bool>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend: mrt1 = %s, mrt2 = %s, mrt3 = %s", print_boolean(decoded.blend_surface_b()), print_boolean(decoded.blend_surface_c()), print_boolean(decoded.blend_surface_d()));
	}
};

template<>
struct registers_decoder<NV4097_SET_USER_CLIP_PLANE_CONTROL>
{

	struct decoded_type
	{
	private:
		u32 value;

		u8 clip_plane0_raw() const { return bf_decoder<0, 4>(value); }
		u8 clip_plane1_raw() const { return bf_decoder<4, 4>(value); }
		u8 clip_plane2_raw() const { return bf_decoder<8, 4>(value); }
		u8 clip_plane3_raw() const { return bf_decoder<12, 4>(value); }
		u8 clip_plane4_raw() const { return bf_decoder<16, 4>(value); }
		u8 clip_plane5_raw() const { return bf_decoder<20, 4>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto clip_plane0() const
		{
			return to_user_clip_plane_op(clip_plane0_raw());
		}

		auto clip_plane1() const
		{
			return to_user_clip_plane_op(clip_plane1_raw());
		}

		auto clip_plane2() const
		{
			return to_user_clip_plane_op(clip_plane2_raw());
		}

		auto clip_plane3() const
		{
			return to_user_clip_plane_op(clip_plane3_raw());
		}

		auto clip_plane4() const
		{
			return to_user_clip_plane_op(clip_plane4_raw());
		}

		auto clip_plane5() const
		{
			return to_user_clip_plane_op(clip_plane5_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "User clip: UC0: %s UC1: %s UC2: %s UC3: %s UC4: %s"
			, decoded.clip_plane0()
			, decoded.clip_plane1()
			, decoded.clip_plane2()
			, decoded.clip_plane3()
			, decoded.clip_plane4()
			, decoded.clip_plane5());
	}
};

template<>
struct registers_decoder<NV4097_SET_LINE_WIDTH>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 line_width() const
		{
			return (value >> 3) + (value & 7) / 8.f;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Line width: %g", decoded.line_width());
	}
};

template<>
struct registers_decoder<NV4097_SET_POINT_SIZE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 point_size() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Point size: %g", decoded.point_size());
	}
};

template<>
struct registers_decoder<NV4097_SET_POINT_SPRITE_CONTROL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return bf_decoder<0, 1, bool>(value);
		}

		u16 texcoord_mask() const
		{
			return bf_decoder<8, 10>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Point sprite: enabled = %s, override mask = 0x%x", print_boolean(decoded.enabled()), decoded.texcoord_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

		u8 color_fmt_raw() const { return bf_decoder<0, 5>(value); }
		u8 depth_fmt_raw() const { return bf_decoder<5, 3>(value); }
		u8 type_raw() const { return bf_decoder<8, 4>(value); }
		u8 antialias_raw() const { return bf_decoder<12, 4>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto color_fmt() const
		{
			return to_surface_color_format(color_fmt_raw());
		}

		auto depth_fmt() const
		{
			return to_surface_depth_format(depth_fmt_raw());
		}

		auto type() const
		{
			return to_surface_raster_type(type_raw());
		}

		auto antialias() const
		{
			return to_surface_antialiasing(antialias_raw());
		}

		u8 log2width() const
		{
			return bf_decoder<16, 8>(value);
		}

		u8 log2height() const
		{
			return bf_decoder<24, 8>(value);
		}

		bool is_integer_color_format() const
		{
			return color_fmt() < surface_color_format::w16z16y16x16;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: Color format: %s DepthStencil format: %s Anti aliasing: %s w: %u h: %u", decoded.color_fmt()
			, decoded.depth_fmt(), decoded.antialias(), decoded.log2width(), decoded.log2height());
	}
};

template<>
struct registers_decoder<NV4097_SET_ZSTENCIL_CLEAR_VALUE>
{
	struct decoded_type
	{
	private:
		u32 value;

		u32 clear_z16() const { return bf_decoder<0, 16, u32>(value); }
		u32 clear_z24() const { return bf_decoder<8, 24>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 clear_stencil() const
		{
			return bf_decoder<0, 8>(value);
		}

		u32 clear_z(bool is_depth_stencil) const
		{
			if (is_depth_stencil)
				return clear_z24();

			return clear_z16();
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Clear: Z24 = %u, z16 = %u, Stencil = %u", decoded.clear_z(true), decoded.clear_z(false), decoded.clear_stencil());
	}
};

template<>
struct registers_decoder<NV4097_SET_INDEX_ARRAY_DMA>
{
	struct decoded_type
	{
	private:
		u32 value;

		u8 type_raw() const { return bf_decoder<4, 8>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation index_dma() const
		{
			return CellGcmLocation{bf_decoder<0, 4>(value)};
		}

		index_array_type type() const
		{
			// Must be a valid value
			return static_cast<index_array_type>(type_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Index: type: %s dma: %s", decoded.type(), decoded.index_dma());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_A>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation dma_surface_a() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: A DMA: %s", decoded.dma_surface_a());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_B>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation dma_surface_b() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: B DMA: %s", decoded.dma_surface_b());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_C>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation dma_surface_c() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: C DMA: %s", decoded.dma_surface_c());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_D>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation dma_surface_d() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: D DMA: %s", decoded.dma_surface_d());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_ZETA>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation dma_surface_z() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Surface: Z DMA: %s", decoded.dma_surface_z());
	}
};

template<>
struct registers_decoder<NV3089_SET_CONTEXT_DMA_IMAGE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation context_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: input DMA: %s", decoded.context_dma());
	}
};

template<>
struct registers_decoder<NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation output_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3062: output DMA: %s", decoded.output_dma());
	}
};

template<>
struct registers_decoder<NV309E_SET_CONTEXT_DMA_IMAGE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation context_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV309E: output DMA: %s", decoded.context_dma());
	}
};

template<>
struct registers_decoder<NV0039_SET_CONTEXT_DMA_BUFFER_OUT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation output_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: output DMA: %s", decoded.output_dma());
	}
};

template<>
struct registers_decoder<NV0039_SET_CONTEXT_DMA_BUFFER_IN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation input_dma() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: input DMA: %s", decoded.input_dma());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_REPORT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto context_dma_report() const
		{
			return blit_engine::to_context_dma(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "REPORT: context DMA: %s", decoded.context_dma_report());
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_NOTIFIES>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation context_dma_notify() const
		{
			return CellGcmLocation{value};
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NOTIFY: context DMA: %s, index: %u", decoded.context_dma_notify(), (decoded.context_dma_notify() & 7) ^ 7);
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

		u8 transfer_origin_raw() const { return bf_decoder<16, 8>(value); }
		u8 transfer_interpolator_raw() const { return bf_decoder<24, 8>(value); }

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 format() const
		{
			return bf_decoder<0, 16>(value);
		}

		auto transfer_origin() const
		{
			return blit_engine::to_transfer_origin(transfer_origin_raw());
		}

		auto transfer_interpolator() const
		{
			return blit_engine::to_transfer_interpolator(transfer_interpolator_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: input fmt: %u origin: %s interp: %s", decoded.format()
			, decoded.transfer_origin(), decoded.transfer_interpolator());
	}
};

template<>
struct registers_decoder<NV309E_SET_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

		u32 transfer_destination_fmt() const { return bf_decoder<0, 16, u32>(value); }
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto format() const
		{
			return blit_engine::to_transfer_destination_format(transfer_destination_fmt());
		}

		u8 sw_height_log2() const
		{
			return bf_decoder<16, 8>(value);
		}

		u8 sw_width_log2() const
		{
			return bf_decoder<24, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV309E: output fmt: %s log2-width: %u log2-height: %u", decoded.format(),
			decoded.sw_width_log2(), decoded.sw_height_log2());
	}
};

template<>
struct registers_decoder<NV0039_FORMAT>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u8 input_format() const
		{
			return bf_decoder<0, 8>(value);
		}

		u8 output_format() const
		{
			return bf_decoder<8, 8>(value);
		}
	};

	static auto decode(u32 value) {
		return std::make_tuple(static_cast<u8>(value & 0xff), static_cast<u8>(value >> 8));
	}

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV0039: input format = %u, output format = %u", decoded.input_format(), decoded.output_format());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_COLOR2>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 blue() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 alpha() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend color: 16b BA = %u, %u", decoded.blue(), decoded.alpha());
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_COLOR>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 red16() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 green16() const
		{
			return bf_decoder<16, 16>(value);
		}

		u8 blue8() const
		{
			return bf_decoder<0, 8>(value);
		}

		u8 green8() const
		{
			return bf_decoder<8, 8>(value);
		}

		u8 red8() const
		{
			return bf_decoder<16, 8>(value);
		}

		u8 alpha8() const
		{
			return bf_decoder<24, 8>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Blend color: 8b BGRA = %u, %u, %u, %u 16b RG = %u , %u"
			, decoded.blue8(), decoded.green8(), decoded.red8(), decoded.alpha8(), decoded.red16(), decoded.green16());
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN>
{
	struct decoded_type
	{
	private:
		u32 value;

		u32 x_raw() const
		{
			return bf_decoder<0, 16, u32>(value);
		}

		u32 y_raw() const
		{
			return bf_decoder<16, 16, u32>(value);
		}

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		// x and y given as 16 bit fixed point

		f32 x() const
		{
			return x_raw() / 16.f;
		}

		f32 y() const
		{
			return y_raw() / 16.f;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "NV3089: in x = %u y = %u", decoded.x(), decoded.y());
	}
};

template<>
struct registers_decoder<NV4097_NO_OPERATION>
{
	struct decoded_type
	{
		decoded_type(u32) {}
	};

	static void dump(std::string& out, u32)
	{
		out += "(nop)";
	}
};

template<>
struct registers_decoder<NV4097_INVALIDATE_VERTEX_CACHE_FILE>
{
	struct decoded_type
	{
		decoded_type(u32) {}
	};

	static void dump(std::string& out, u32)
	{
		out += "(invalidate vertex cache file)";
	}
};

template<>
struct registers_decoder<NV4097_INVALIDATE_VERTEX_FILE>
{
	struct decoded_type
	{
		decoded_type(u32) {}
	};

	static void dump(std::string& out, u32)
	{
		out += "(invalidate vertex file)";
	}
};

template<>
struct registers_decoder<NV4097_SET_ANTI_ALIASING_CONTROL>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool msaa_enabled() const
		{
			return bf_decoder<0, 1, bool>(value);
		}

		bool msaa_alpha_to_coverage() const
		{
			return bf_decoder<4, 1, bool>(value);
		}

		bool msaa_alpha_to_one() const
		{
			return bf_decoder<8, 1, bool>(value);
		}

		u16 msaa_sample_mask() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Anti_aliasing: %s alpha_to_coverage: %s alpha_to_one: %s sample_mask: %u", print_boolean(decoded.msaa_enabled()), print_boolean(decoded.msaa_alpha_to_coverage()), print_boolean(decoded.msaa_alpha_to_one()), decoded.msaa_sample_mask());
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADE_MODE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto shading() const
		{
			return to_shading_mode(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Shading mode: %s", decoded.shading());
	}
};

template<>
struct registers_decoder<NV4097_SET_FRONT_POLYGON_MODE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto front_polygon_mode() const
		{
			return to_polygon_mode(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Front polygon mode: %s", decoded.front_polygon_mode());
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_POLYGON_MODE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		auto back_polygon_mode() const
		{
			return to_polygon_mode(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "back polygon mode: %s", decoded.back_polygon_mode());
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_CONSTANT_LOAD>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 transform_constant_load() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Transform constant load: %u", decoded.transform_constant_load());
	}
};

template<>
struct registers_decoder<NV4097_SET_POLYGON_STIPPLE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return value > 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "polygon_stipple: %s", print_boolean(decoded.enabled()));
	}
};

template <>
struct registers_decoder<NV4097_SET_ZCULL_EN>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return value > 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "ZCULL: %s", print_boolean(decoded.enabled()));
	}
};

template <>
struct registers_decoder<NV4097_SET_ZCULL_STATS_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return value > 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "ZCULL: stats %s", print_boolean(decoded.enabled()));
	}
};

template <>
struct registers_decoder<NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return value > 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "ZCULL: pixel count %s", print_boolean(decoded.enabled()));
	}
};

#define EXPAND_RANGE_1(index, MACRO) \
	MACRO(index)

#define EXPAND_RANGE_2(index, MACRO) \
	EXPAND_RANGE_1((index), MACRO) \
	EXPAND_RANGE_1((index) + 1, MACRO)

#define EXPAND_RANGE_4(index, MACRO) \
	EXPAND_RANGE_2((index), MACRO) \
	EXPAND_RANGE_2((index) + 2, MACRO)

#define EXPAND_RANGE_8(index, MACRO) \
	EXPAND_RANGE_4((index), MACRO) \
	EXPAND_RANGE_4((index) + 4, MACRO)

#define EXPAND_RANGE_16(index, MACRO) \
	EXPAND_RANGE_8((index), MACRO) \
	EXPAND_RANGE_8((index) + 8, MACRO)

#define EXPAND_RANGE_32(index, MACRO) \
	EXPAND_RANGE_16((index), MACRO) \
	EXPAND_RANGE_16((index) + 16, MACRO)

#define EXPAND_RANGE_64(index, MACRO) \
	EXPAND_RANGE_32((index), MACRO) \
	EXPAND_RANGE_32((index) + 32, MACRO)

#define EXPAND_RANGE_128(index, MACRO) \
	EXPAND_RANGE_64((index), MACRO) \
	EXPAND_RANGE_64((index) + 64, MACRO)

#define EXPAND_RANGE_256(index, MACRO) \
	EXPAND_RANGE_128((index), MACRO) \
	EXPAND_RANGE_128((index) + 128, MACRO)

#define EXPAND_RANGE_512(index, MACRO) \
	EXPAND_RANGE_256((index), MACRO) \
	EXPAND_RANGE_256((index) + 256, MACRO)

template<u32 index>
struct transform_constant_helper
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		f32 constant_value() const
		{
			return std::bit_cast<f32>(value);
		}
	};

	static constexpr u32 reg = index / 4;
	static constexpr u8 subreg = index % 4;

	static void dump(std::string& out, const decoded_type& decoded)
	{
		auto get_subreg_name = [](u8 subreg) -> std::string_view
		{
			return subreg == 0 ? "x"sv :
				subreg == 1 ? "y"sv :
				subreg == 2 ? "z"sv :
				"w"sv;
		};

		fmt::append(out, "TransformConstant[%u].%s: %g (0x%08x)", reg, get_subreg_name(subreg), decoded.constant_value(), std::bit_cast<u32>(decoded.constant_value()));
	}
};

#define TRANSFORM_CONSTANT(index) template<> struct registers_decoder<NV4097_SET_TRANSFORM_CONSTANT + index> : public transform_constant_helper<index> {};
#define DECLARE_TRANSFORM_CONSTANT(index) NV4097_SET_TRANSFORM_CONSTANT + index,

EXPAND_RANGE_32(0, TRANSFORM_CONSTANT)

template<u32 index>
struct transform_program_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Transform Program (%u): 0x%08x", index, decoded.value);
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM_LOAD>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 transform_program_load() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Transform Program Load: %u", decoded.transform_program_load());
	}
};

template <>
struct registers_decoder<NV4097_DRAW_ARRAYS>
{
	struct decoded_type
	{
	private:
		u32 value;

		u16 count_raw() const
		{
			return bf_decoder<24, 8>(value);
		}

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 start() const
		{
			return bf_decoder<0, 24>(value);
		}

		u16 count() const
		{
			return count_raw() + 1;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Draw vertexes range [%u, %u]", decoded.start(), decoded.start() + decoded.count());
	}
};

template <>
struct registers_decoder<NV4097_DRAW_INDEX_ARRAY>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 start() const
		{
			return bf_decoder<0, 24>(value);
		}

		u16 count() const
		{
			return static_cast<u16>(bf_decoder<24, 8>(value) + 1);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Draw vertexes range {IdxArray[%u], IdxArray[%u]}", decoded.start(), decoded.start() + decoded.count());
	}
};

template <>
struct registers_decoder<NV4097_SET_CONTROL0>
{
	struct decoded_type
	{
	private:
		u32 value;

	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool depth_float() const
		{
			return bf_decoder<12, 1>(value) != 0;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Depth float: %s", print_boolean(decoded.depth_float()));
	}
};

#define TRANSFORM_PROGRAM(index) template<> struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM + index> : public transform_program_helper<index> {};
#define DECLARE_TRANSFORM_PROGRAM(index) NV4097_SET_TRANSFORM_PROGRAM + index,
EXPAND_RANGE_32(0, TRANSFORM_PROGRAM)

template<u32 index>
struct vertex_array_helper
{
	struct decoded_type
	{
	private:
		u32 value;

		u8 type_raw() const
		{
			return bf_decoder<0, 3>(value);
		}
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 frequency() const
		{
			return bf_decoder<16, 16>(value);
		}

		u8 stride() const
		{
			return bf_decoder<8, 8>(value);
		}

		u8 size() const
		{
			return bf_decoder<4, 4>(value);
		}

		rsx::vertex_base_type type() const
		{
			return rsx::to_vertex_base_type(type_raw());
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		auto print_vertex_attribute_format = [](rsx::vertex_base_type type) -> std::string_view
		{
			switch (type)
			{
			case rsx::vertex_base_type::s1: return "Signed short normalized";
			case rsx::vertex_base_type::f: return "Float";
			case rsx::vertex_base_type::sf: return "Half float";
			case rsx::vertex_base_type::ub: return "Unsigned byte normalized";
			case rsx::vertex_base_type::s32k: return "Signed short unormalized";
			case rsx::vertex_base_type::cmp: return "CMP";
			case rsx::vertex_base_type::ub256: return "Unsigned byte unormalized";
			}
			fmt::throw_exception("Unexpected enum found");
		};

		fmt::append(out, "Vertex Data Array %u%s: Type: %s, size: %u, stride: %u, frequency: %u", index, decoded.size() ? "" : " (disabled)", print_vertex_attribute_format(decoded.type()), decoded.size(), decoded.stride(), decoded.frequency());
	}
};

#define VERTEX_DATA_ARRAY_FORMAT(index) template<> struct registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index> : public vertex_array_helper<index> {};
#define DECLARE_VERTEX_DATA_ARRAY_FORMAT(index) NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index,

EXPAND_RANGE_16(0, VERTEX_DATA_ARRAY_FORMAT)

template<u32 index>
struct vertex_array_offset_helper
{
	struct decoded_type
	{
	private:
		u32 value;
	public:
		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Vertex Array %u: Offset: 0x%x", index, decoded.offset());
	}
};

#define VERTEX_DATA_ARRAY_OFFSET(index) template<> struct registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index> : public vertex_array_offset_helper<index> {};
#define DECLARE_VERTEX_DATA_ARRAY_OFFSET(index) NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index,

EXPAND_RANGE_16(0, VERTEX_DATA_ARRAY_OFFSET)

template<typename type, int count>
struct register_vertex_printer;

template<int count>
struct register_vertex_printer<f32, count>
{
	static std::string type()
	{
		return fmt::format("float%u", count);
	}

	static std::string value(u32 v)
	{
		return fmt::format("%g", std::bit_cast<f32>(v));
	}
};

template<int count>
struct register_vertex_printer<u16, count>
{
	static std::string type()
	{
		return fmt::format("short%u", count);
	}

	static std::string value(u32 v)
	{
		return fmt::format("%u %u", (v & 0xffff), (v >> 16));
	}
};

template<>
struct register_vertex_printer<u8, 4>
{
	static std::string_view type()
	{
		return "uchar4";
	}

	static std::string value(u32 v)
	{
		return fmt::format("%u %u %u %u", (v & 0xff), ((v >> 8) & 0xff), ((v >> 16) & 0xff), ((v >> 24) & 0xff));
	}
};

template<u32 index, typename type, int count>
struct register_vertex_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}
	};

	static constexpr usz increment_per_array_index = (count * sizeof(type)) / sizeof(u32);
	static constexpr usz attribute_index = index / increment_per_array_index;
	static constexpr usz vertex_subreg = index % increment_per_array_index;

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "register vertex: %u as %u: %s", attribute_index, register_vertex_printer<type, count>::type(), register_vertex_printer<type, count>::value(decoded.value));
	}
};

#define VERTEX_DATA4UB(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4UB_M + index> : public register_vertex_helper<index, u8, 4> {};
#define VERTEX_DATA1F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA1F_M + index> : public register_vertex_helper<index, f32, 1> {};
#define VERTEX_DATA2F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA2F_M + index> : public register_vertex_helper<index, f32, 2> {};
#define VERTEX_DATA3F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA3F_M + index> : public register_vertex_helper<index, f32, 3> {};
#define VERTEX_DATA4F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4F_M + index> : public register_vertex_helper<index, f32, 4> {};
#define VERTEX_DATA2S(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA2S_M + index> : public register_vertex_helper<index, u16, 2> {};
#define VERTEX_DATA4S(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4S_M + index> : public register_vertex_helper<index, u16, 4> {};

#define DECLARE_VERTEX_DATA4UB(index) \
	NV4097_SET_VERTEX_DATA4UB_M + index,
#define DECLARE_VERTEX_DATA1F(index) \
	NV4097_SET_VERTEX_DATA1F_M + index,
#define DECLARE_VERTEX_DATA2F(index) \
	NV4097_SET_VERTEX_DATA2F_M + index,
#define DECLARE_VERTEX_DATA3F(index) \
	NV4097_SET_VERTEX_DATA3F_M + index,
#define DECLARE_VERTEX_DATA4F(index) \
	NV4097_SET_VERTEX_DATA4F_M + index,
#define DECLARE_VERTEX_DATA2S(index) \
	NV4097_SET_VERTEX_DATA2S_M + index,
#define DECLARE_VERTEX_DATA4S(index) \
	NV4097_SET_VERTEX_DATA4S_M + index,

EXPAND_RANGE_16(0, VERTEX_DATA4UB)
EXPAND_RANGE_16(0, VERTEX_DATA1F)
EXPAND_RANGE_16(0, VERTEX_DATA2F)
EXPAND_RANGE_16(0, VERTEX_DATA3F)
EXPAND_RANGE_16(0, VERTEX_DATA4F)
EXPAND_RANGE_16(0, VERTEX_DATA2S)
EXPAND_RANGE_16(0, VERTEX_DATA4S)

template <u32 index>
struct texture_offset_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u32 offset() const
		{
			return value;
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Texture %u: Offset: 0x%x", index, decoded.offset());
	}
};

template <u32 index>
struct texture_format_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		CellGcmLocation location() const
		{
			return CellGcmLocation{(value & 3) - 1};
		}

		bool cubemap() const
		{
			return bf_decoder<2, 1, bool>(value);
		}

		u8 border_type() const
		{
			return bf_decoder<3, 1>(value);
		}

		texture_dimension dimension() const
		{
			// Hack: avoid debugger crash on not-written value (needs checking on realhw)
			// This is not the function RSX uses so it's safe
			return rsx::to_texture_dimension(std::clamp<u8>(bf_decoder<4, 4>(value), 1, 3));
		}

		CellGcmTexture format() const
		{
			return CellGcmTexture{bf_decoder<8, 8>(value)};
		}

		u16 mipmap() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Texture %u: %s, Cubemap: %s, %s, %s, Mipmap: %u", index,
				decoded.location(), decoded.cubemap(), decoded.dimension(), decoded.format(), decoded.mipmap());
	}
};

template <u32 index>
struct texture_image_rect_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 height() const
		{
			return bf_decoder<0, 16>(value);
		}

		u16 width() const
		{
			return bf_decoder<16, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Texture %u: W: %u, H: %u", index, decoded.width(), decoded.height());
	}
};

template <u32 index>
struct texture_control0_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return bf_decoder<31, 1, bool>(value);
		}

		f32 min_lod() const
		{
			return rsx::decode_fxp<4, 8, false>(bf_decoder<19, 12>(value));
		}

		f32 max_lod() const
		{
			return rsx::decode_fxp<4, 8, false>(bf_decoder<7, 12>(value));
		}

		texture_max_anisotropy max_aniso() const
		{
			return rsx::to_texture_max_anisotropy(bf_decoder<4, 3>(value));
		}

		bool alpha_kill_enabled() const
		{
			return bf_decoder<2, 1, bool>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Texture %u: %s, Min/Max LOD: %g/%g, Max Aniso: %s, AKill: %s", index, print_boolean(decoded.enabled())
			, decoded.min_lod(), decoded.max_lod(), decoded.max_aniso(), print_boolean(decoded.alpha_kill_enabled()));
	}
};

template <u32 index>
struct texture_control3_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		u16 depth() const
		{
			return bf_decoder<20, 12>(value);
		}

		u32 pitch() const
		{
			return bf_decoder<0, 16>(value);
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "Texture %u: Pitch: %u, Depth: %u", index, decoded.pitch(), decoded.depth());
	}
};

#define TEXTURE_OFFSET(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_OFFSET + ((index) * 8)> : public texture_offset_helper<index> {};
#define TEXTURE_FORMAT(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_FORMAT + ((index) * 8)> : public texture_format_helper<index> {};
#define TEXTURE_IMAGE_RECT(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_IMAGE_RECT + ((index) * 8)> : public texture_image_rect_helper<index> {};
#define TEXTURE_CONTROL0(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_CONTROL0 + ((index) * 8)> : public texture_control0_helper<index> {};
#define TEXTURE_CONTROL3(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_CONTROL3 + index> : public texture_control3_helper<index> {};

#define DECLARE_TEXTURE_OFFSET(index) \
	NV4097_SET_TEXTURE_OFFSET + ((index) * 8),
#define DECLARE_TEXTURE_FORMAT(index) \
	NV4097_SET_TEXTURE_FORMAT + ((index) * 8),
#define DECLARE_TEXTURE_IMAGE_RECT(index) \
	NV4097_SET_TEXTURE_IMAGE_RECT + ((index) * 8),
#define DECLARE_TEXTURE_CONTROL0(index) \
	NV4097_SET_TEXTURE_CONTROL0 + ((index) * 8),
#define DECLARE_TEXTURE_CONTROL3(index) \
	NV4097_SET_TEXTURE_CONTROL3 + index,

EXPAND_RANGE_16(0, TEXTURE_OFFSET)
EXPAND_RANGE_16(0, TEXTURE_FORMAT)
EXPAND_RANGE_16(0, TEXTURE_IMAGE_RECT)
EXPAND_RANGE_16(0, TEXTURE_CONTROL0)
EXPAND_RANGE_16(0, TEXTURE_CONTROL3)

template <u32 index>
struct vertex_texture_control0_helper
{
	struct decoded_type
	{
		const u32 value;

		constexpr decoded_type(u32 value) noexcept : value(value) {}

		bool enabled() const
		{
			return bf_decoder<31, 1, bool>(value);
		}

		f32 min_lod() const
		{
			return rsx::decode_fxp<4, 8, false>(bf_decoder<19, 12>(value));
		}

		f32 max_lod() const
		{
			return rsx::decode_fxp<4, 8, false>(bf_decoder<7, 12>(value));
		}
	};

	static void dump(std::string& out, const decoded_type& decoded)
	{
		fmt::append(out, "VTexture %u: %s, Min/Max LOD: %g/%g", index, print_boolean(decoded.enabled())
			, decoded.min_lod(), decoded.max_lod());
	}
};

#define VERTEX_TEXTURE_CONTROL0(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_CONTROL0 + ((index) * 8)> : public vertex_texture_control0_helper<index> {};

#define DECLARE_VERTEX_TEXTURE_CONTROL0(index) \
	NV4097_SET_VERTEX_TEXTURE_CONTROL0 + ((index) * 8),

EXPAND_RANGE_4(0, VERTEX_TEXTURE_CONTROL0)

} // end namespace rsx
