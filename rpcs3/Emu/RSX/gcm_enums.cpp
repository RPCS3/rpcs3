#include "gcm_enums.h"
#include "Utilities/StrFmt.h"

using namespace rsx;

vertex_base_type rsx::to_vertex_base_type(u8 in)
{
	switch (in)
	{
	case 0: return vertex_base_type::ub256;
	case 1: return vertex_base_type::s1;
	case 2: return vertex_base_type::f;
	case 3: return vertex_base_type::sf;
	case 4: return vertex_base_type::ub;
	case 5: return vertex_base_type::s32k;
	case 6: return vertex_base_type::cmp;
	case 7: return vertex_base_type::ub256;
	}
	fmt::throw_exception("Unknown vertex base type %d", in);
}

primitive_type rsx::to_primitive_type(u8 in)
{
	switch (in)
	{
	case CELL_GCM_PRIMITIVE_POINTS: return primitive_type::points;
	case CELL_GCM_PRIMITIVE_LINES: return primitive_type::lines;
	case CELL_GCM_PRIMITIVE_LINE_LOOP: return primitive_type::line_loop;
	case CELL_GCM_PRIMITIVE_LINE_STRIP: return primitive_type::line_strip;
	case CELL_GCM_PRIMITIVE_TRIANGLES: return primitive_type::triangles;
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP: return primitive_type::triangle_strip;
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN: return primitive_type::triangle_fan;
	case CELL_GCM_PRIMITIVE_QUADS: return primitive_type::quads;
	case CELL_GCM_PRIMITIVE_QUAD_STRIP: return primitive_type::quad_strip;
	case CELL_GCM_PRIMITIVE_POLYGON: return primitive_type::polygon;
	default: return primitive_type::invalid;
	}
}

enum
{
	CELL_GCM_WINDOW_ORIGIN_TOP = 0,
	CELL_GCM_WINDOW_ORIGIN_BOTTOM = 1,
	CELL_GCM_WINDOW_PIXEL_CENTER_HALF = 0,
	CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER = 1,
};

window_origin rsx::to_window_origin(u8 in)
{
	switch (in)
	{
	case CELL_GCM_WINDOW_ORIGIN_TOP: return window_origin::top;
	case CELL_GCM_WINDOW_ORIGIN_BOTTOM: return window_origin::bottom;
	}
	fmt::throw_exception("Unknown window origin modifier 0x%x", in);
}

window_pixel_center rsx::to_window_pixel_center(u8 in)
{
	switch (in)
	{
	case CELL_GCM_WINDOW_PIXEL_CENTER_HALF: return window_pixel_center::half;
	case CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER: return window_pixel_center::integer;
	}
	fmt::throw_exception("Unknown window pixel center 0x%x", in);
}

comparison_function rsx::to_comparison_function(u16 in)
{
	switch (in)
	{
	case CELL_GCM_TEXTURE_ZFUNC_NEVER & CELL_GCM_SCULL_SFUNC_NEVER:
	case CELL_GCM_NEVER:
		return comparison_function::never;

	case CELL_GCM_TEXTURE_ZFUNC_LESS & CELL_GCM_SCULL_SFUNC_LESS:
	case CELL_GCM_LESS:
		return comparison_function::less;

	case CELL_GCM_TEXTURE_ZFUNC_EQUAL & CELL_GCM_SCULL_SFUNC_EQUAL:
	case CELL_GCM_EQUAL:
		return comparison_function::equal;

	case CELL_GCM_TEXTURE_ZFUNC_LEQUAL & CELL_GCM_SCULL_SFUNC_LEQUAL:
	case CELL_GCM_LEQUAL:
		return comparison_function::less_or_equal;

	case CELL_GCM_TEXTURE_ZFUNC_GREATER & CELL_GCM_SCULL_SFUNC_GREATER:
	case CELL_GCM_GREATER:
		return comparison_function::greater;

	case CELL_GCM_TEXTURE_ZFUNC_NOTEQUAL & CELL_GCM_SCULL_SFUNC_NOTEQUAL:
	case CELL_GCM_NOTEQUAL:
		return comparison_function::not_equal;

	case CELL_GCM_TEXTURE_ZFUNC_GEQUAL & CELL_GCM_SCULL_SFUNC_GEQUAL:
	case CELL_GCM_GEQUAL:
		return comparison_function::greater_or_equal;

	case CELL_GCM_TEXTURE_ZFUNC_ALWAYS & CELL_GCM_SCULL_SFUNC_ALWAYS:
	case CELL_GCM_ALWAYS:
		return comparison_function::always;
	}
	fmt::throw_exception("Unknown comparison function 0x%x", in);
}

fog_mode rsx::to_fog_mode(u32 in)
{
	switch (in)
	{
	case CELL_GCM_FOG_MODE_LINEAR: return fog_mode::linear;
	case CELL_GCM_FOG_MODE_EXP: return fog_mode::exponential;
	case CELL_GCM_FOG_MODE_EXP2: return fog_mode::exponential2;
	case CELL_GCM_FOG_MODE_EXP_ABS: return fog_mode::exponential_abs;
	case CELL_GCM_FOG_MODE_EXP2_ABS: return fog_mode::exponential2_abs;
	case CELL_GCM_FOG_MODE_LINEAR_ABS: return fog_mode::linear_abs;
	}
	fmt::throw_exception("Unknown fog mode 0x%x", in);
}

texture_dimension rsx::to_texture_dimension(u8 in)
{
	switch (in)
	{
	case 1: return texture_dimension::dimension1d;
	case 2: return texture_dimension::dimension2d;
	case 3: return texture_dimension::dimension3d;
	}
	fmt::throw_exception("Unknown texture dimension %d", in);
}

template <>
void fmt_class_string<CellGcmLocation>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellGcmLocation value)
	{
		switch (value)
		{
		case CELL_GCM_LOCATION_LOCAL: return "Local";
		case CELL_GCM_LOCATION_MAIN: return "Main";

		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER: return "Local-Buffer"; // Local memory for DMA operations
		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER: return "Main-Buffer"; // Main memory for DMA operations
		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL: return "Report Local";
		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN: return  "Report Main";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0: return "_Notify0";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_1: return "_Notify1";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_2: return "_Notify2";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_3: return "_Notify3";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_4: return "_Notify4";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_5: return "_Notify5";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_6: return "_Notify6";
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_7: return "_Notify7";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0: return "_Get-Notify0";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY1: return "_Get-Notify1";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY2: return "_Get-Notify2";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY3: return "_Get-Notify3";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY4: return "_Get-Notify4";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY5: return "_Get-Notify5";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY6: return "_Get-Notify6";
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY7: return "_Get-Notify7";
		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW: return "SEMA-RW";
		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_R: return "SEMA-R";
		case CELL_GCM_CONTEXT_DMA_DEVICE_RW: return "DEVICE-RW";
		case CELL_GCM_CONTEXT_DMA_DEVICE_R: return "DEVICE-R";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellGcmTexture>::format(std::string& out, u64 arg)
{
	switch (static_cast<u32>(arg) & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN))
	{
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8: out += "COMPRESSED_HILO8"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: out += "COMPRESSED_HILO_S8"; break;
	case CELL_GCM_TEXTURE_B8: out += "B8"; break;
	case CELL_GCM_TEXTURE_A1R5G5B5: out += "A1R5G5B5"; break;
	case CELL_GCM_TEXTURE_A4R4G4B4: out += "A4R4G4B4"; break;
	case CELL_GCM_TEXTURE_R5G6B5: out += "R5G6B5"; break;
	case CELL_GCM_TEXTURE_A8R8G8B8: out += "A8R8G8B8"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1: out += "COMPRESSED_DXT1"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23: out += "COMPRESSED_DXT23"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: out += "COMPRESSED_DXT45"; break;
	case CELL_GCM_TEXTURE_G8B8: out += "G8B8"; break;
	case CELL_GCM_TEXTURE_R6G5B5: out += "R6G5B5"; break;
	case CELL_GCM_TEXTURE_DEPTH24_D8: out += "DEPTH24_D8"; break;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: out += "DEPTH24_D8_FLOAT"; break;
	case CELL_GCM_TEXTURE_DEPTH16: out += "DEPTH16"; break;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT: out += "DEPTH16_FLOAT"; break;
	case CELL_GCM_TEXTURE_X16: out += "X16"; break;
	case CELL_GCM_TEXTURE_Y16_X16: out += "Y16_X16"; break;
	case CELL_GCM_TEXTURE_R5G5B5A1: out += "R5G5B5A1"; break;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: out += "W16_Z16_Y16_X16_FLOAT"; break;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: out += "W32_Z32_Y32_X32_FLOAT"; break;
	case CELL_GCM_TEXTURE_X32_FLOAT: out += "X32_FLOAT"; break;
	case CELL_GCM_TEXTURE_D1R5G5B5: out += "D1R5G5B5"; break;
	case CELL_GCM_TEXTURE_D8R8G8B8: out += "D8R8G8B8"; break;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT: out += "Y16_X16_FLOAT"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: out += "COMPRESSED_B8R8_G8R8"; break;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: out += "COMPRESSED_R8B8_R8G8"; break;
	default: fmt::append(out, "%s", arg); return;
	}

	switch (arg & (CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN))
	{
	case CELL_GCM_TEXTURE_LN: out += "-LN"; break;
	case CELL_GCM_TEXTURE_UN: out += "-UN"; break;
	case CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN: out += "-LN-UN"; break;
	default: break;
	}
}

template <>
void fmt_class_string<comparison_function>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](comparison_function value)
	{
		switch (value)
		{
		case comparison_function::never: return "Never";
		case comparison_function::less: return "Less";
		case comparison_function::equal: return "Equal";
		case comparison_function::less_or_equal: return "Less_equal";
		case comparison_function::greater: return "Greater";
		case comparison_function::not_equal: return "Not_equal";
		case comparison_function::greater_or_equal: return "Greater_equal";
		case comparison_function::always: return "Always";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<stencil_op>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](stencil_op value)
	{
		switch (value)
		{
		case stencil_op::keep: return "Keep";
		case stencil_op::zero: return "Zero";
		case stencil_op::replace: return "Replace";
		case stencil_op::incr: return "Incr";
		case stencil_op::decr: return "Decr";
		case stencil_op::incr_wrap: return "Incr_wrap";
		case stencil_op::decr_wrap: return "Decr_wrap";
		case stencil_op::invert: return "Invert";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<fog_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](fog_mode value)
	{
		switch (value)
		{
		case fog_mode::exponential: return "exponential";
		case fog_mode::exponential2: return "exponential2";
		case fog_mode::exponential2_abs: return "exponential2(abs)";
		case fog_mode::exponential_abs: return "exponential(abs)";
		case fog_mode::linear: return "linear";
		case fog_mode::linear_abs: return "linear(abs)";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<logic_op>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](logic_op value)
	{
		switch (value)
		{
		case logic_op::logic_clear: return "Clear";
		case logic_op::logic_and: return "And";
		case logic_op::logic_set: return "Set";
		case logic_op::logic_and_reverse: return "And_reverse";
		case logic_op::logic_copy: return "Copy";
		case logic_op::logic_and_inverted: return "And_inverted";
		case logic_op::logic_noop: return "Noop";
		case logic_op::logic_xor: return "Xor";
		case logic_op::logic_or: return "Or";
		case logic_op::logic_nor: return "Nor";
		case logic_op::logic_equiv: return "Equiv";
		case logic_op::logic_invert: return "Invert";
		case logic_op::logic_or_reverse: return "Or_reverse";
		case logic_op::logic_copy_inverted: return "Copy_inverted";
		case logic_op::logic_or_inverted: return "Or_inverted";
		case logic_op::logic_nand: return "Nand";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<front_face>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](front_face value)
	{
		switch (value)
		{
		case front_face::ccw: return "counter clock wise";
		case front_face::cw: return "clock wise";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<cull_face>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cull_face value)
	{
		switch (value)
		{
		case cull_face::back: return "back";
		case cull_face::front: return "front";
		case cull_face::front_and_back: return "front and back";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<surface_target>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](surface_target value)
	{
		switch (value)
		{
		case surface_target::none: return "none";
		case surface_target::surface_a: return "surface A";
		case surface_target::surface_b: return "surface B";
		case surface_target::surfaces_a_b: return "surfaces A and B";
		case surface_target::surfaces_a_b_c: return "surfaces A, B and C";
		case surface_target::surfaces_a_b_c_d: return "surfaces A,B, C and D";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<primitive_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](primitive_type value)
	{
		switch (value)
		{
		case primitive_type::invalid: return "";
		case primitive_type::points: return "Points";
		case primitive_type::lines: return "Lines";
		case primitive_type::line_loop: return "Line_loop";
		case primitive_type::line_strip: return "Line_strip";
		case primitive_type::triangles: return "Triangles";
		case primitive_type::triangle_strip: return "Triangle_strip";
		case primitive_type::triangle_fan: return "Triangle_fan";
		case primitive_type::quads: return "Quads";
		case primitive_type::quad_strip: return "Quad_strip";
		case primitive_type::polygon: return "Polygon";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::transfer_operation>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::transfer_operation value)
	{
		switch (value)
		{
		case blit_engine::transfer_operation::blend_and: return "blend and";
		case blit_engine::transfer_operation::blend_premult: return "blend premult";
		case blit_engine::transfer_operation::rop_and: return "rop and";
		case blit_engine::transfer_operation::srccopy: return "srccopy";
		case blit_engine::transfer_operation::srccopy_and: return "srccopy_and";
		case blit_engine::transfer_operation::srccopy_premult: return "srccopy_premult";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::transfer_source_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::transfer_source_format value)
	{
		switch (value)
		{
		case blit_engine::transfer_source_format::a1r5g5b5: return "a1r5g5b5";
		case blit_engine::transfer_source_format::a8b8g8r8: return "a8b8g8r8";
		case blit_engine::transfer_source_format::a8r8g8b8: return "a8r8g8b8";
		case blit_engine::transfer_source_format::ay8: return "ay8";
		case blit_engine::transfer_source_format::cr8yb8cb8ya8: return "cr8yb8cb8ya8";
		case blit_engine::transfer_source_format::ecr8eyb8ecb8eya8: return "ecr8eyb8ecb8eya8";
		case blit_engine::transfer_source_format::eyb8ecr8eya8ecb8: return "eyb8ecr8eya8ecb8";
		case blit_engine::transfer_source_format::r5g6b5: return "r5g6b5";
		case blit_engine::transfer_source_format::x1r5g5b5: return "x1r5g5b5";
		case blit_engine::transfer_source_format::x8b8g8r8: return "x8b8g8r8";
		case blit_engine::transfer_source_format::x8r8g8b8: return "x8r8g8b8";
		case blit_engine::transfer_source_format::y8: return "y8";
		case blit_engine::transfer_source_format::yb8cr8ya8cb8: return "yb8cr8ya8cb8";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::context_surface>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::context_surface value)
	{
		switch (value)
		{
		case blit_engine::context_surface::surface2d: return "surface 2d";
		case blit_engine::context_surface::swizzle2d: return "swizzle 2d";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::transfer_destination_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::transfer_destination_format value)
	{
		switch (value)
		{
		case blit_engine::transfer_destination_format::a8r8g8b8: return "a8r8g8b8";
		case blit_engine::transfer_destination_format::r5g6b5: return "r5g6b5";
		case blit_engine::transfer_destination_format::y32: return "y32";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<index_array_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](index_array_type value)
	{
		switch (value)
		{
		case index_array_type::u16: return "u16";
		case index_array_type::u32: return "u32";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<polygon_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](polygon_mode value)
	{
		switch (value)
		{
		case polygon_mode::fill: return "fill";
		case polygon_mode::line: return "line";
		case polygon_mode::point: return "point";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<surface_color_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](surface_color_format value)
	{
		switch (value)
		{
		case surface_color_format::x1r5g5b5_z1r5g5b5: return "X1R5G5B5_Z1R5G5B5";
		case surface_color_format::x1r5g5b5_o1r5g5b5: return "X1R5G5B5_O1R5G5B5";
		case surface_color_format::r5g6b5: return "R5G6B5";
		case surface_color_format::x8r8g8b8_z8r8g8b8: return "X8R8G8B8_Z8R8G8B8";
		case surface_color_format::x8r8g8b8_o8r8g8b8: return "X8R8G8B8_O8R8G8B8";
		case surface_color_format::a8r8g8b8: return "A8R8G8B8";
		case surface_color_format::b8: return "B8";
		case surface_color_format::g8b8: return "G8B8";
		case surface_color_format::w16z16y16x16: return "F_W16Z16Y16X16";
		case surface_color_format::w32z32y32x32: return "F_W32Z32Y32X32";
		case surface_color_format::x32: return "F_X32";
		case surface_color_format::x8b8g8r8_z8b8g8r8: return "X8B8G8R8_Z8B8G8R8";
		case surface_color_format::x8b8g8r8_o8b8g8r8: return "X8B8G8R8_O8B8G8R8";
		case surface_color_format::a8b8g8r8: return "A8B8G8R8";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<surface_antialiasing>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](surface_antialiasing value)
	{
		switch (value)
		{
		case surface_antialiasing::center_1_sample: return "1 sample centered";
		case surface_antialiasing::diagonal_centered_2_samples: return "2 samples diagonal centered";
		case surface_antialiasing::square_centered_4_samples: return "4 samples square centered";
		case surface_antialiasing::square_rotated_4_samples: return "4 samples diagonal rotated";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blend_equation>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blend_equation value)
	{
		switch (value)
		{
		case blend_equation::add: return "Add";
		case blend_equation::substract: return "Substract";
		case blend_equation::reverse_substract: return "Reverse_substract";
		case blend_equation::min: return "Min";
		case blend_equation::max: return "Max";
		case blend_equation::add_signed: return "Add_signed";
		case blend_equation::reverse_add_signed: return "Reverse_add_signed";
		case blend_equation::reverse_substract_signed: return "Reverse_substract_signed";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blend_factor>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blend_factor value)
	{
		switch (value)
		{
		case blend_factor::zero: return "0";
		case blend_factor::one: return "1";
		case blend_factor::src_color: return "src.rgb";
		case blend_factor::one_minus_src_color: return "(1 - src.rgb)";
		case blend_factor::src_alpha: return "src.a";
		case blend_factor::one_minus_src_alpha: return "(1 - src.a)";
		case blend_factor::dst_alpha: return "dst.a";
		case blend_factor::one_minus_dst_alpha: return "(1 - dst.a)";
		case blend_factor::dst_color: return "dst.rgb";
		case blend_factor::one_minus_dst_color: return "(1 - dst.rgb)";
		case blend_factor::src_alpha_saturate: return "sat(src.a)";
		case blend_factor::constant_color: return "const.rgb";
		case blend_factor::one_minus_constant_color: return "(1 - const.rgb)";
		case blend_factor::constant_alpha: return "const.a";
		case blend_factor::one_minus_constant_alpha: return "(1 - const.a)";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<window_origin>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](window_origin value)
	{
		switch (value)
		{
		case window_origin::bottom: return "bottom";
		case window_origin::top: return "top";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<window_pixel_center>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](window_pixel_center value)
	{
		switch (value)
		{
		case window_pixel_center::half: return "half";
		case window_pixel_center::integer: return "integer";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<user_clip_plane_op>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](user_clip_plane_op value)
	{
		switch (value)
		{
		case user_clip_plane_op::disable: return "disabled";
		case user_clip_plane_op::greater_or_equal: return "greater or equal";
		case user_clip_plane_op::less_than: return "less than";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::context_dma>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::context_dma value)
	{
		switch (value)
		{
		case blit_engine::context_dma::report_location_main: return "report location main";
		case blit_engine::context_dma::to_memory_get_report: return "to memory get report";
		case blit_engine::context_dma::memory_host_buffer: return "memory host buffer";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<blit_engine::transfer_origin>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::transfer_origin value)
	{
		switch (value)
		{
		case blit_engine::transfer_origin::center: return "center";
		case blit_engine::transfer_origin::corner: return "corner";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<shading_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](shading_mode value)
	{
		switch (value)
		{
		case shading_mode::flat: return "flat";
		case shading_mode::smooth: return "smooth";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<surface_depth_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](surface_depth_format value)
	{
		switch (value)
		{
		case surface_depth_format::z16: return "Z16";
		case surface_depth_format::z24s8: return "Z24S8";
		}

		return unknown;
	});
}


template <>
void fmt_class_string<blit_engine::transfer_interpolator>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](blit_engine::transfer_interpolator value)
	{
		switch (value)
		{
		case blit_engine::transfer_interpolator::foh: return "foh";
		case blit_engine::transfer_interpolator::zoh: return "zoh";
		}

		return unknown;
	});
}


template <>
void fmt_class_string<texture_dimension>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](texture_dimension value)
	{
		switch (value)
		{
		case texture_dimension::dimension1d: return "1D";
		case texture_dimension::dimension2d: return "2D";
		case texture_dimension::dimension3d: return "3D";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<texture_max_anisotropy>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](texture_max_anisotropy value)
	{
		switch (value)
		{
		case texture_max_anisotropy::x1: return "1";
		case texture_max_anisotropy::x2: return "2";
		case texture_max_anisotropy::x4: return "4";
		case texture_max_anisotropy::x6: return "6";
		case texture_max_anisotropy::x8: return "8";
		case texture_max_anisotropy::x10: return "10";
		case texture_max_anisotropy::x12: return "12";
		case texture_max_anisotropy::x16: return "16";
		}

		return unknown;
	});
}

namespace rsx
{
	std::string print_boolean(bool b)
	{
		return b ? "enabled" : "disabled";
	}
} // end namespace rsx

enum
{
	// Surface Target
	CELL_GCM_SURFACE_TARGET_NONE = 0,
	CELL_GCM_SURFACE_TARGET_0 = 1,
	CELL_GCM_SURFACE_TARGET_1 = 2,
	CELL_GCM_SURFACE_TARGET_MRT1 = 0x13,
	CELL_GCM_SURFACE_TARGET_MRT2 = 0x17,
	CELL_GCM_SURFACE_TARGET_MRT3 = 0x1f,

	// Surface Depth
	CELL_GCM_SURFACE_Z16 = 1,
	CELL_GCM_SURFACE_Z24S8 = 2,

	// Surface Antialias
	CELL_GCM_SURFACE_CENTER_1 = 0,
	CELL_GCM_SURFACE_DIAGONAL_CENTERED_2 = 3,
	CELL_GCM_SURFACE_SQUARE_CENTERED_4 = 4,
	CELL_GCM_SURFACE_SQUARE_ROTATED_4 = 5,

	// Surface format
	CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5 = 1,
	CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5 = 2,
	CELL_GCM_SURFACE_R5G6B5 = 3,
	CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8 = 4,
	CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8 = 5,
	CELL_GCM_SURFACE_A8R8G8B8 = 8,
	CELL_GCM_SURFACE_B8 = 9,
	CELL_GCM_SURFACE_G8B8 = 10,
	CELL_GCM_SURFACE_F_W16Z16Y16X16 = 11,
	CELL_GCM_SURFACE_F_W32Z32Y32X32 = 12,
	CELL_GCM_SURFACE_F_X32 = 13,
	CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8 = 14,
	CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8 = 15,
	CELL_GCM_SURFACE_A8B8G8R8 = 16,

	// Wrap
	CELL_GCM_TEXTURE_WRAP = 1,
	CELL_GCM_TEXTURE_MIRROR = 2,
	CELL_GCM_TEXTURE_CLAMP_TO_EDGE = 3,
	CELL_GCM_TEXTURE_BORDER = 4,
	CELL_GCM_TEXTURE_CLAMP = 5,
	CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE = 6,
	CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER = 7,
	CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP = 8,

	// Max Anisotropy
	CELL_GCM_TEXTURE_MAX_ANISO_1 = 0,
	CELL_GCM_TEXTURE_MAX_ANISO_2 = 1,
	CELL_GCM_TEXTURE_MAX_ANISO_4 = 2,
	CELL_GCM_TEXTURE_MAX_ANISO_6 = 3,
	CELL_GCM_TEXTURE_MAX_ANISO_8 = 4,
	CELL_GCM_TEXTURE_MAX_ANISO_10 = 5,
	CELL_GCM_TEXTURE_MAX_ANISO_12 = 6,
	CELL_GCM_TEXTURE_MAX_ANISO_16 = 7,

	// Texture Filter
	CELL_GCM_TEXTURE_NEAREST = 1,
	CELL_GCM_TEXTURE_LINEAR = 2,
	CELL_GCM_TEXTURE_NEAREST_NEAREST = 3,
	CELL_GCM_TEXTURE_LINEAR_NEAREST = 4,
	CELL_GCM_TEXTURE_NEAREST_LINEAR = 5,
	CELL_GCM_TEXTURE_LINEAR_LINEAR = 6,
	CELL_GCM_TEXTURE_CONVOLUTION_MIN = 7,
	CELL_GCM_TEXTURE_CONVOLUTION_MAG = 4,
};

texture_wrap_mode rsx::to_texture_wrap_mode(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TEXTURE_WRAP: return texture_wrap_mode::wrap;
	case CELL_GCM_TEXTURE_MIRROR: return texture_wrap_mode::mirror;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return texture_wrap_mode::clamp_to_edge;
	case CELL_GCM_TEXTURE_BORDER: return texture_wrap_mode::border;
	case CELL_GCM_TEXTURE_CLAMP: return texture_wrap_mode::clamp;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return texture_wrap_mode::mirror_once_clamp_to_edge;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return texture_wrap_mode::mirror_once_border;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return texture_wrap_mode::mirror_once_clamp;
	}
	fmt::throw_exception("Unknown wrap mode 0x%x", in);
}

texture_max_anisotropy rsx::to_texture_max_anisotropy(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TEXTURE_MAX_ANISO_1: return texture_max_anisotropy::x1;
	case CELL_GCM_TEXTURE_MAX_ANISO_2: return texture_max_anisotropy::x2;
	case CELL_GCM_TEXTURE_MAX_ANISO_4: return texture_max_anisotropy::x4;
	case CELL_GCM_TEXTURE_MAX_ANISO_6: return texture_max_anisotropy::x6;
	case CELL_GCM_TEXTURE_MAX_ANISO_8: return texture_max_anisotropy::x8;
	case CELL_GCM_TEXTURE_MAX_ANISO_10: return texture_max_anisotropy::x10;
	case CELL_GCM_TEXTURE_MAX_ANISO_12: return texture_max_anisotropy::x12;
	case CELL_GCM_TEXTURE_MAX_ANISO_16: return texture_max_anisotropy::x16;
	}
	fmt::throw_exception("Unknown anisotropy max mode 0x%x", in);
}

texture_minify_filter rsx::to_texture_minify_filter(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TEXTURE_NEAREST: return texture_minify_filter::nearest;
	case CELL_GCM_TEXTURE_LINEAR: return texture_minify_filter::linear;
	case CELL_GCM_TEXTURE_NEAREST_NEAREST: return texture_minify_filter::nearest_nearest;
	case CELL_GCM_TEXTURE_LINEAR_NEAREST: return texture_minify_filter::linear_nearest;
	case CELL_GCM_TEXTURE_NEAREST_LINEAR: return texture_minify_filter::nearest_linear;
	case CELL_GCM_TEXTURE_LINEAR_LINEAR: return texture_minify_filter::linear_linear;
	case CELL_GCM_TEXTURE_CONVOLUTION_MIN: return texture_minify_filter::linear_linear;
	}
	fmt::throw_exception("Unknown minify filter 0x%x", in);
}


texture_magnify_filter rsx::to_texture_magnify_filter(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TEXTURE_NEAREST: return texture_magnify_filter::nearest;
	case CELL_GCM_TEXTURE_LINEAR: return texture_magnify_filter::linear;
	case CELL_GCM_TEXTURE_CONVOLUTION_MAG: return texture_magnify_filter::convolution_mag;
	}
	fmt::throw_exception("Unknown magnify filter 0x%x", in);
}

surface_target rsx::to_surface_target(u8 in)
{
	switch (in)
	{
	case CELL_GCM_SURFACE_TARGET_NONE: return surface_target::none;
	case CELL_GCM_SURFACE_TARGET_0: return surface_target::surface_a;
	case CELL_GCM_SURFACE_TARGET_1: return surface_target::surface_b;
	case CELL_GCM_SURFACE_TARGET_MRT1: return surface_target::surfaces_a_b;
	case CELL_GCM_SURFACE_TARGET_MRT2: return surface_target::surfaces_a_b_c;
	case CELL_GCM_SURFACE_TARGET_MRT3: return surface_target::surfaces_a_b_c_d;
	}
	fmt::throw_exception("Unknown surface target 0x%x", in);
}

surface_depth_format rsx::to_surface_depth_format(u8 in)
{
	switch (in)
	{
	case CELL_GCM_SURFACE_Z16: return surface_depth_format::z16;
	case CELL_GCM_SURFACE_Z24S8: return surface_depth_format::z24s8;
	}
	fmt::throw_exception("Unknown surface depth format 0x%x", in);
}

surface_antialiasing rsx::to_surface_antialiasing(u8 in)
{
	switch (in)
	{
	case CELL_GCM_SURFACE_CENTER_1: return surface_antialiasing::center_1_sample;
	case CELL_GCM_SURFACE_DIAGONAL_CENTERED_2: return surface_antialiasing::diagonal_centered_2_samples;
	case CELL_GCM_SURFACE_SQUARE_CENTERED_4: return surface_antialiasing::square_centered_4_samples;
	case CELL_GCM_SURFACE_SQUARE_ROTATED_4: return surface_antialiasing::square_rotated_4_samples;
	}
	fmt::throw_exception("Unknown surface antialiasing format 0x%x", in);
}

surface_color_format rsx::to_surface_color_format(u8 in)
{
	switch (in)
	{
	case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5: return surface_color_format::x1r5g5b5_z1r5g5b5;
	case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5: return surface_color_format::x1r5g5b5_o1r5g5b5;
	case CELL_GCM_SURFACE_R5G6B5: return surface_color_format::r5g6b5;
	case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8: return surface_color_format::x8r8g8b8_z8r8g8b8;
	case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8: return surface_color_format::x8r8g8b8_o8r8g8b8;
	case CELL_GCM_SURFACE_A8R8G8B8: return surface_color_format::a8r8g8b8;
	case CELL_GCM_SURFACE_B8: return surface_color_format::b8;
	case CELL_GCM_SURFACE_G8B8: return surface_color_format::g8b8;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16: return surface_color_format::w16z16y16x16;
	case CELL_GCM_SURFACE_F_W32Z32Y32X32: return surface_color_format::w32z32y32x32;
	case CELL_GCM_SURFACE_F_X32: return surface_color_format::x32;
	case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8: return surface_color_format::x8b8g8r8_z8b8g8r8;
	case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8: return surface_color_format::x8b8g8r8_o8b8g8r8;
	case CELL_GCM_SURFACE_A8B8G8R8: return surface_color_format::a8b8g8r8;
	}
	fmt::throw_exception("Unknown surface color format 0x%x", in);
}

stencil_op rsx::to_stencil_op(u16 in)
{
	switch (in)
	{
	case CELL_GCM_INVERT: return stencil_op::invert;
	case CELL_GCM_KEEP: return stencil_op::keep;
	case CELL_GCM_REPLACE: return stencil_op::replace;
	case CELL_GCM_INCR: return stencil_op::incr;
	case CELL_GCM_DECR: return stencil_op::decr;
	case CELL_GCM_INCR_WRAP: return stencil_op::incr_wrap;
	case CELL_GCM_DECR_WRAP: return stencil_op::decr_wrap;
	case CELL_GCM_ZERO: return stencil_op::zero;
	}
	fmt::throw_exception("Unknown stencil op 0x%x", in);
}

blend_equation rsx::to_blend_equation(u16 in)
{
	switch (in)
	{
	case CELL_GCM_FUNC_ADD: return blend_equation::add;
	case CELL_GCM_MIN: return blend_equation::min;
	case CELL_GCM_MAX: return blend_equation::max;
	case CELL_GCM_FUNC_SUBTRACT: return blend_equation::substract;
	case CELL_GCM_FUNC_REVERSE_SUBTRACT: return blend_equation::reverse_substract;
	case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED: return blend_equation::reverse_substract_signed;
	case CELL_GCM_FUNC_ADD_SIGNED: return blend_equation::add_signed;
	case CELL_GCM_FUNC_REVERSE_ADD_SIGNED: return blend_equation::reverse_add_signed;
	}
	fmt::throw_exception("Unknown blend eq 0x%x", in);
}

blend_factor rsx::to_blend_factor(u16 in)
{
	switch (in)
	{
	case CELL_GCM_ZERO: return blend_factor::zero;
	case CELL_GCM_ONE: return blend_factor::one;
	case CELL_GCM_SRC_COLOR: return blend_factor::src_color;
	case CELL_GCM_ONE_MINUS_SRC_COLOR: return blend_factor::one_minus_src_color;
	case CELL_GCM_SRC_ALPHA: return blend_factor::src_alpha;
	case CELL_GCM_ONE_MINUS_SRC_ALPHA: return blend_factor::one_minus_src_alpha;
	case CELL_GCM_DST_ALPHA: return blend_factor::dst_alpha;
	case CELL_GCM_ONE_MINUS_DST_ALPHA: return blend_factor::one_minus_dst_alpha;
	case CELL_GCM_DST_COLOR: return blend_factor::dst_color;
	case CELL_GCM_ONE_MINUS_DST_COLOR: return blend_factor::one_minus_dst_color;
	case CELL_GCM_SRC_ALPHA_SATURATE: return blend_factor::src_alpha_saturate;
	case CELL_GCM_CONSTANT_COLOR: return blend_factor::constant_color;
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR: return blend_factor::one_minus_constant_color;
	case CELL_GCM_CONSTANT_ALPHA: return blend_factor::constant_alpha;
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA: return blend_factor::one_minus_constant_alpha;
	}
	fmt::throw_exception("Unknown blend factor 0x%x", in);
}

enum
{
	CELL_GCM_CLEAR = 0x1500,
	CELL_GCM_AND = 0x1501,
	CELL_GCM_AND_REVERSE = 0x1502,
	CELL_GCM_COPY = 0x1503,
	CELL_GCM_AND_INVERTED = 0x1504,
	CELL_GCM_NOOP = 0x1505,
	CELL_GCM_XOR = 0x1506,
	CELL_GCM_OR = 0x1507,
	CELL_GCM_NOR = 0x1508,
	CELL_GCM_EQUIV = 0x1509,
	CELL_GCM_OR_REVERSE = 0x150B,
	CELL_GCM_COPY_INVERTED = 0x150C,
	CELL_GCM_OR_INVERTED = 0x150D,
	CELL_GCM_NAND = 0x150E,
	CELL_GCM_SET = 0x150F,
};

logic_op rsx::to_logic_op(u16 in)
{
	switch (in)
	{
	case CELL_GCM_CLEAR: return logic_op::logic_clear;
	case CELL_GCM_AND: return logic_op::logic_and;
	case CELL_GCM_AND_REVERSE: return logic_op::logic_and_reverse;
	case CELL_GCM_COPY: return logic_op::logic_copy;
	case CELL_GCM_AND_INVERTED: return logic_op::logic_and_inverted;
	case CELL_GCM_NOOP: return logic_op::logic_noop;
	case CELL_GCM_XOR: return logic_op::logic_xor;
	case CELL_GCM_OR: return logic_op::logic_or;
	case CELL_GCM_NOR: return logic_op::logic_nor;
	case CELL_GCM_EQUIV: return logic_op::logic_equiv;
	case CELL_GCM_INVERT: return logic_op::logic_invert;
	case CELL_GCM_OR_REVERSE: return logic_op::logic_or_reverse;
	case CELL_GCM_COPY_INVERTED: return logic_op::logic_copy_inverted;
	case CELL_GCM_OR_INVERTED: return logic_op::logic_or_inverted;
	case CELL_GCM_NAND: return logic_op::logic_nand;
	case CELL_GCM_SET: return logic_op::logic_set;
	}
	fmt::throw_exception("Unknown logic op 0x%x", in);
}

front_face rsx::to_front_face(u16 in)
{
	switch (in)
	{
	default: // Disgaea 3 pass some garbage value at startup, this is needed to survive.
	case CELL_GCM_CW: return front_face::cw;
	case CELL_GCM_CCW: return front_face::ccw;
	}
	fmt::throw_exception("Unknown front face 0x%x", in);
}

enum
{
	CELL_GCM_TRANSFER_ORIGIN_CENTER = 1,
	CELL_GCM_TRANSFER_ORIGIN_CORNER = 2,

	CELL_GCM_TRANSFER_INTERPOLATOR_ZOH = 0,
	CELL_GCM_TRANSFER_INTERPOLATOR_FOH = 1,
};

blit_engine::transfer_origin blit_engine::to_transfer_origin(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TRANSFER_ORIGIN_CENTER: return blit_engine::transfer_origin::center;
	case CELL_GCM_TRANSFER_ORIGIN_CORNER: return blit_engine::transfer_origin::corner;
	}
	fmt::throw_exception("Unknown transfer origin 0x%x", in);
}

blit_engine::transfer_interpolator blit_engine::to_transfer_interpolator(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TRANSFER_INTERPOLATOR_ZOH: return blit_engine::transfer_interpolator::zoh;
	case CELL_GCM_TRANSFER_INTERPOLATOR_FOH: return blit_engine::transfer_interpolator::foh;
	}
	fmt::throw_exception("Unknown transfer interpolator 0x%x", in);
}

enum
{
	CELL_GCM_TRANSFER_OPERATION_SRCCOPY_AND = 0,
	CELL_GCM_TRANSFER_OPERATION_ROP_AND = 1,
	CELL_GCM_TRANSFER_OPERATION_BLEND_AND = 2,
	CELL_GCM_TRANSFER_OPERATION_SRCCOPY = 3,
	CELL_GCM_TRANSFER_OPERATION_SRCCOPY_PREMULT = 4,
	CELL_GCM_TRANSFER_OPERATION_BLEND_PREMULT = 5,
};

blit_engine::transfer_operation blit_engine::to_transfer_operation(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TRANSFER_OPERATION_SRCCOPY_AND: return blit_engine::transfer_operation::srccopy_and;
	case CELL_GCM_TRANSFER_OPERATION_ROP_AND: return blit_engine::transfer_operation::rop_and;
	case CELL_GCM_TRANSFER_OPERATION_BLEND_AND: return blit_engine::transfer_operation::blend_and;
	case CELL_GCM_TRANSFER_OPERATION_SRCCOPY: return blit_engine::transfer_operation::srccopy;
	case CELL_GCM_TRANSFER_OPERATION_SRCCOPY_PREMULT: return blit_engine::transfer_operation::srccopy_premult;
	case CELL_GCM_TRANSFER_OPERATION_BLEND_PREMULT: return blit_engine::transfer_operation::blend_premult;
	default: return blit_engine::transfer_operation::invalid;
	}
}

enum
{
	CELL_GCM_TRANSFER_SCALE_FORMAT_A1R5G5B5 = 1,
	CELL_GCM_TRANSFER_SCALE_FORMAT_X1R5G5B5 = 2,
	CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8 = 3,
	CELL_GCM_TRANSFER_SCALE_FORMAT_X8R8G8B8 = 4,
	CELL_GCM_TRANSFER_SCALE_FORMAT_CR8YB8CB8YA8 = 5,
	CELL_GCM_TRANSFER_SCALE_FORMAT_YB8CR8YA8CB8 = 6,
	CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 = 7,
	CELL_GCM_TRANSFER_SCALE_FORMAT_Y8 = 8,
	CELL_GCM_TRANSFER_SCALE_FORMAT_AY8 = 9,
	CELL_GCM_TRANSFER_SCALE_FORMAT_EYB8ECR8EYA8ECB8 = 10,
	CELL_GCM_TRANSFER_SCALE_FORMAT_ECR8EYB8ECB8EYA8 = 11,
	CELL_GCM_TRANSFER_SCALE_FORMAT_A8B8G8R8 = 12,
	CELL_GCM_TRANSFER_SCALE_FORMAT_X8B8G8R8 = 13,
};

blit_engine::transfer_source_format blit_engine::to_transfer_source_format(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TRANSFER_SCALE_FORMAT_A1R5G5B5: return blit_engine::transfer_source_format::a1r5g5b5;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_X1R5G5B5: return blit_engine::transfer_source_format::x1r5g5b5;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8: return blit_engine::transfer_source_format::a8r8g8b8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_X8R8G8B8: return blit_engine::transfer_source_format::x8r8g8b8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_CR8YB8CB8YA8: return blit_engine::transfer_source_format::cr8yb8cb8ya8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_YB8CR8YA8CB8: return blit_engine::transfer_source_format::yb8cr8ya8cb8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5: return blit_engine::transfer_source_format::r5g6b5;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_Y8: return blit_engine::transfer_source_format::y8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_AY8: return blit_engine::transfer_source_format::ay8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_EYB8ECR8EYA8ECB8: return blit_engine::transfer_source_format::eyb8ecr8eya8ecb8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_ECR8EYB8ECB8EYA8: return blit_engine::transfer_source_format::ecr8eyb8ecb8eya8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_A8B8G8R8: return blit_engine::transfer_source_format::a8b8g8r8;
	case CELL_GCM_TRANSFER_SCALE_FORMAT_X8B8G8R8: return blit_engine::transfer_source_format::x8b8g8r8;
	default: return blit_engine::transfer_source_format::invalid;
	}
}

enum
{
	// Destination Format conversions
	CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 = 4,
	CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 = 10,
	CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32 = 11,
};

blit_engine::transfer_destination_format blit_engine::to_transfer_destination_format(u8 in)
{
	switch (in)
	{
	case CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5: return blit_engine::transfer_destination_format::r5g6b5;
	case CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8: return blit_engine::transfer_destination_format::a8r8g8b8;
	case CELL_GCM_TRANSFER_SURFACE_FORMAT_Y32: return blit_engine::transfer_destination_format::y32;
	default: return blit_engine::transfer_destination_format::invalid;
	}
}

enum
{
	CELL_GCM_CONTEXT_SURFACE2D = 0x313371C3,
	CELL_GCM_CONTEXT_SWIZZLE2D = 0x31337A73,
};

blit_engine::context_surface blit_engine::to_context_surface(u32 in)
{
	switch (in)
	{
	case CELL_GCM_CONTEXT_SURFACE2D: return blit_engine::context_surface::surface2d;
	case CELL_GCM_CONTEXT_SWIZZLE2D: return blit_engine::context_surface::swizzle2d;
	}
	fmt::throw_exception("Unknown context surface 0x%x", in);
}

blit_engine::context_dma blit_engine::to_context_dma(u32 in)
{
	switch (in)
	{
	case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL: return blit_engine::context_dma::to_memory_get_report;
	case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN: return blit_engine::context_dma::report_location_main;
	case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER: return blit_engine::context_dma::memory_host_buffer;
	}
	fmt::throw_exception("Unknown context dma 0x%x", in);
}

enum
{
	CELL_GCM_USER_CLIP_PLANE_DISABLE = 0,
	CELL_GCM_USER_CLIP_PLANE_ENABLE_LT = 1,
	CELL_GCM_USER_CLIP_PLANE_ENABLE_GE = 2,
};

user_clip_plane_op rsx::to_user_clip_plane_op(u8 in)
{
	switch (in)
	{
	case CELL_GCM_USER_CLIP_PLANE_DISABLE: return user_clip_plane_op::disable;
	case CELL_GCM_USER_CLIP_PLANE_ENABLE_LT: return user_clip_plane_op::less_than;
	case CELL_GCM_USER_CLIP_PLANE_ENABLE_GE: return user_clip_plane_op::greater_or_equal;
	}
	fmt::throw_exception("Unknown user clip plane 0x%x", in);
}

enum
{
	CELL_GCM_FLAT = 0x1D00,
	CELL_GCM_SMOOTH = 0x1D01,
};

shading_mode rsx::to_shading_mode(u32 in)
{
	switch (in)
	{
	case CELL_GCM_FLAT: return shading_mode::flat;
	case CELL_GCM_SMOOTH: return shading_mode::smooth;
	}
	fmt::throw_exception("Unknown shading mode 0x%x", in);
}

enum
{
	CELL_GCM_POLYGON_MODE_POINT = 0x1B00,
	CELL_GCM_POLYGON_MODE_LINE = 0x1B01,
	CELL_GCM_POLYGON_MODE_FILL = 0x1B02,
};

polygon_mode rsx::to_polygon_mode(u32 in)
{
	switch (in)
	{
	case CELL_GCM_POLYGON_MODE_POINT: return polygon_mode::point;
	case CELL_GCM_POLYGON_MODE_LINE: return polygon_mode::line;
	case CELL_GCM_POLYGON_MODE_FILL: return polygon_mode::fill;
	}
	fmt::throw_exception("Unknown polygon mode 0x%x", in);
}
