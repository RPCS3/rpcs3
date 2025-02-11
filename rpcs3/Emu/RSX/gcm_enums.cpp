#include "gcm_enums.h"
#include "Utilities/StrFmt.h"

using namespace rsx;

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
		default: return unknown;
		}
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
		default: return unknown;
		}
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
		default: return unknown;
		}
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
		case blend_equation::subtract: return "Subtract";
		case blend_equation::reverse_subtract: return "Reverse_subtract";
		case blend_equation::min: return "Min";
		case blend_equation::max: return "Max";
		case blend_equation::add_signed: return "Add_signed";
		case blend_equation::reverse_add_signed: return "Reverse_add_signed";
		case blend_equation::reverse_subtract_signed: return "Reverse_subtract_signed";
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
	enum class boolean_to_string_t : u8;
}

template <>
void fmt_class_string<boolean_to_string_t>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](boolean_to_string_t value)
	{
		switch (value)
		{
		case boolean_to_string_t{+true}: return "true";
		case boolean_to_string_t{+false}: return "false";
		default: break; // TODO: This is technically unreachable but need needs to be reachable when value is not 1 or 0
		}

		return unknown;
	});
}
