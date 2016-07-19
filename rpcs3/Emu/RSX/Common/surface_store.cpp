#include "stdafx.h"
#include "surface_store.h"

namespace rsx
{
	namespace utility
	{
		std::vector<u8> get_rtt_indexes(surface_target color_target)
		{
			switch (color_target)
			{
			case surface_target::none: return{};
			case surface_target::surface_a: return{ 0 };
			case surface_target::surface_b: return{ 1 };
			case surface_target::surfaces_a_b: return{ 0, 1 };
			case surface_target::surfaces_a_b_c: return{ 0, 1, 2 };
			case surface_target::surfaces_a_b_c_d: return{ 0, 1, 2, 3 };
			}
			throw EXCEPTION("Wrong color_target");
		}

		size_t get_aligned_pitch(surface_color_format format, u32 width)
		{
			switch (format)
			{
			case surface_color_format::b8: return align(width, 256);
			case surface_color_format::g8b8:
			case surface_color_format::x1r5g5b5_o1r5g5b5:
			case surface_color_format::x1r5g5b5_z1r5g5b5:
			case surface_color_format::r5g6b5: return align(width * 2, 256);
			case surface_color_format::a8b8g8r8:
			case surface_color_format::x8b8g8r8_o8b8g8r8:
			case surface_color_format::x8b8g8r8_z8b8g8r8:
			case surface_color_format::x8r8g8b8_o8r8g8b8:
			case surface_color_format::x8r8g8b8_z8r8g8b8:
			case surface_color_format::x32:
			case surface_color_format::a8r8g8b8: return align(width * 4, 256);
			case surface_color_format::w16z16y16x16: return align(width * 8, 256);
			case surface_color_format::w32z32y32x32: return align(width * 16, 256);
			}
			throw EXCEPTION("Unknown color surface format");
		}

		size_t get_packed_pitch(surface_color_format format, u32 width)
		{
			switch (format)
			{
			case surface_color_format::b8: return width;
			case surface_color_format::g8b8:
			case surface_color_format::x1r5g5b5_o1r5g5b5:
			case surface_color_format::x1r5g5b5_z1r5g5b5:
			case surface_color_format::r5g6b5: return width * 2;
			case surface_color_format::a8b8g8r8:
			case surface_color_format::x8b8g8r8_o8b8g8r8:
			case surface_color_format::x8b8g8r8_z8b8g8r8:
			case surface_color_format::x8r8g8b8_o8r8g8b8:
			case surface_color_format::x8r8g8b8_z8r8g8b8:
			case surface_color_format::x32:
			case surface_color_format::a8r8g8b8: return width * 4;
			case surface_color_format::w16z16y16x16: return width * 8;
			case surface_color_format::w32z32y32x32: return width * 16;
			}
			throw EXCEPTION("Unknown color surface format");
		}
	}
}