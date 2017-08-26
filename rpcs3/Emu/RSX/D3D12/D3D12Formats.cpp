#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12Formats.h"
#include "D3D12Utils.h"
#include "Emu/RSX/GCM.h"


D3D12_BLEND_OP get_blend_op(rsx::blend_equation op)
{
	switch (op)
	{
	case rsx::blend_equation::add: return D3D12_BLEND_OP_ADD;
	case rsx::blend_equation::substract: return D3D12_BLEND_OP_SUBTRACT;
	case rsx::blend_equation::reverse_substract: return D3D12_BLEND_OP_REV_SUBTRACT;
	case rsx::blend_equation::min: return D3D12_BLEND_OP_MIN;
	case rsx::blend_equation::max: return D3D12_BLEND_OP_MAX;
	case rsx::blend_equation::add_signed:
	case rsx::blend_equation::reverse_add_signed:
	case rsx::blend_equation::reverse_substract_signed:
		break;
	}
	fmt::throw_exception("Invalid or unsupported blend op (0x%x)" HERE, (u32)op);
}

D3D12_BLEND get_blend_factor(rsx::blend_factor factor)
{
	switch (factor)
	{
	case rsx::blend_factor::zero: return D3D12_BLEND_ZERO;
	case rsx::blend_factor::one: return D3D12_BLEND_ONE;
	case rsx::blend_factor::src_color: return D3D12_BLEND_SRC_COLOR;
	case rsx::blend_factor::one_minus_src_color: return D3D12_BLEND_INV_SRC_COLOR;
	case rsx::blend_factor::src_alpha: return D3D12_BLEND_SRC_ALPHA;
	case rsx::blend_factor::one_minus_src_alpha: return D3D12_BLEND_INV_SRC_ALPHA;
	case rsx::blend_factor::dst_alpha: return D3D12_BLEND_DEST_ALPHA;
	case rsx::blend_factor::one_minus_dst_alpha: return D3D12_BLEND_INV_DEST_ALPHA;
	case rsx::blend_factor::dst_color: return D3D12_BLEND_DEST_COLOR;
	case rsx::blend_factor::one_minus_dst_color: return D3D12_BLEND_INV_DEST_COLOR;
	case rsx::blend_factor::src_alpha_saturate: return D3D12_BLEND_SRC_ALPHA_SAT;
	case rsx::blend_factor::constant_color:
	case rsx::blend_factor::constant_alpha:
		return D3D12_BLEND_BLEND_FACTOR;
	case rsx::blend_factor::one_minus_constant_color:
	case rsx::blend_factor::one_minus_constant_alpha:
		return D3D12_BLEND_INV_BLEND_FACTOR;
	}
	fmt::throw_exception("Invalid blend factor (0x%x)" HERE, (u32)factor);
}

D3D12_BLEND get_blend_factor_alpha(rsx::blend_factor factor)
{
	switch (factor)
	{
	case rsx::blend_factor::zero: return D3D12_BLEND_ZERO;
	case rsx::blend_factor::one: return D3D12_BLEND_ONE;
	case rsx::blend_factor::src_color: return D3D12_BLEND_SRC_ALPHA;
	case rsx::blend_factor::one_minus_src_color: return D3D12_BLEND_INV_SRC_ALPHA;
	case rsx::blend_factor::src_alpha: return D3D12_BLEND_SRC_ALPHA;
	case rsx::blend_factor::one_minus_src_alpha: return D3D12_BLEND_INV_SRC_ALPHA;
	case rsx::blend_factor::dst_alpha: return D3D12_BLEND_DEST_ALPHA;
	case rsx::blend_factor::one_minus_dst_alpha: return D3D12_BLEND_INV_DEST_ALPHA;
	case rsx::blend_factor::dst_color: return D3D12_BLEND_DEST_ALPHA;
	case rsx::blend_factor::one_minus_dst_color: return D3D12_BLEND_INV_DEST_ALPHA;
	case rsx::blend_factor::src_alpha_saturate: return D3D12_BLEND_SRC_ALPHA_SAT;
	case rsx::blend_factor::constant_color:
	case rsx::blend_factor::constant_alpha:
		return D3D12_BLEND_BLEND_FACTOR;
	case rsx::blend_factor::one_minus_constant_color:
	case rsx::blend_factor::one_minus_constant_alpha:
		return D3D12_BLEND_INV_BLEND_FACTOR;
	}
	fmt::throw_exception("Invalid blend alpha factor (0x%x)" HERE, (u32)factor);
}

/**
* Convert GCM logic op code to D3D12 one
*/
D3D12_LOGIC_OP get_logic_op(rsx::logic_op op)
{
	switch (op)
	{
	case rsx::logic_op::logic_clear: return D3D12_LOGIC_OP_CLEAR;
	case rsx::logic_op::logic_and: return D3D12_LOGIC_OP_AND;
	case rsx::logic_op::logic_and_reverse: return D3D12_LOGIC_OP_AND_REVERSE;
	case rsx::logic_op::logic_copy: return D3D12_LOGIC_OP_COPY;
	case rsx::logic_op::logic_and_inverted: return D3D12_LOGIC_OP_AND_INVERTED;
	case rsx::logic_op::logic_noop: return D3D12_LOGIC_OP_NOOP;
	case rsx::logic_op::logic_xor: return D3D12_LOGIC_OP_XOR;
	case rsx::logic_op::logic_or: return D3D12_LOGIC_OP_OR;
	case rsx::logic_op::logic_nor: return D3D12_LOGIC_OP_NOR;
	case rsx::logic_op::logic_equiv: return D3D12_LOGIC_OP_EQUIV;
	case rsx::logic_op::logic_invert: return D3D12_LOGIC_OP_INVERT;
	case rsx::logic_op::logic_or_reverse: return D3D12_LOGIC_OP_OR_REVERSE;
	case rsx::logic_op::logic_copy_inverted: return D3D12_LOGIC_OP_COPY_INVERTED;
	case rsx::logic_op::logic_or_inverted: return D3D12_LOGIC_OP_OR_INVERTED;
	case rsx::logic_op::logic_nand: return D3D12_LOGIC_OP_NAND;
	case rsx::logic_op::logic_set: return D3D12_LOGIC_OP_SET;
	}
	fmt::throw_exception("Invalid logic op (0x%x)" HERE, (u32)op);
}

/**
* Convert GCM stencil op code to D3D12 one
*/
D3D12_STENCIL_OP get_stencil_op(rsx::stencil_op op)
{
	switch (op)
	{
	case rsx::stencil_op::keep: return D3D12_STENCIL_OP_KEEP;
	case rsx::stencil_op::zero: return D3D12_STENCIL_OP_ZERO;
	case rsx::stencil_op::replace: return D3D12_STENCIL_OP_REPLACE;
	case rsx::stencil_op::incr: return D3D12_STENCIL_OP_INCR_SAT;
	case rsx::stencil_op::decr: return D3D12_STENCIL_OP_DECR_SAT;
	case rsx::stencil_op::invert: return D3D12_STENCIL_OP_INVERT;
	case rsx::stencil_op::incr_wrap: return D3D12_STENCIL_OP_INCR;
	case rsx::stencil_op::decr_wrap: return D3D12_STENCIL_OP_DECR;
	}
	fmt::throw_exception("Invalid stencil op (0x%x)" HERE, (u32)op);
}

D3D12_COMPARISON_FUNC get_compare_func(rsx::comparison_function op)
{
	switch (op)
	{
	case rsx::comparison_function::never: return D3D12_COMPARISON_FUNC_NEVER;
	case rsx::comparison_function::less: return D3D12_COMPARISON_FUNC_LESS;
	case rsx::comparison_function::equal: return D3D12_COMPARISON_FUNC_EQUAL;
	case rsx::comparison_function::less_or_equal: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case rsx::comparison_function::greater: return D3D12_COMPARISON_FUNC_GREATER;
	case rsx::comparison_function::not_equal: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case rsx::comparison_function::greater_or_equal: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case rsx::comparison_function::always: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	fmt::throw_exception("Invalid compare func (0x%x)" HERE, (u32)op);
}

DXGI_FORMAT get_texture_format(u8 format)
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8: return DXGI_FORMAT_R8_UNORM;
	case CELL_GCM_TEXTURE_A1R5G5B5: return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_A4R4G4B4: return DXGI_FORMAT_B4G4R4A4_UNORM;
	case CELL_GCM_TEXTURE_R5G6B5: return DXGI_FORMAT_B5G6R5_UNORM;
	case CELL_GCM_TEXTURE_A8R8G8B8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return DXGI_FORMAT_BC1_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return DXGI_FORMAT_BC2_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return DXGI_FORMAT_BC3_UNORM;
	case CELL_GCM_TEXTURE_G8B8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_R6G5B5: return DXGI_FORMAT_B5G6R5_UNORM;
	case CELL_GCM_TEXTURE_DEPTH24_D8: return DXGI_FORMAT_R32_UINT; // Untested
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:	return DXGI_FORMAT_R32_FLOAT; // Untested
	case CELL_GCM_TEXTURE_DEPTH16: return DXGI_FORMAT_R16_UINT; // Untested
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return DXGI_FORMAT_R16_FLOAT; // Untested
	case CELL_GCM_TEXTURE_X16: return DXGI_FORMAT_R16_UNORM;
	case CELL_GCM_TEXTURE_Y16_X16: return DXGI_FORMAT_R16G16_UNORM;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
	case CELL_GCM_TEXTURE_X32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
	case CELL_GCM_TEXTURE_R5G5B5A1: return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case CELL_GCM_TEXTURE_D1R5G5B5: return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_D8R8G8B8: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return DXGI_FORMAT_R8G8_B8G8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return DXGI_FORMAT_R8G8_SNORM;
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return DXGI_FORMAT_R8G8_B8G8_UNORM;
	}
	fmt::throw_exception("Invalid texture format (0x%x)" HERE, (u32)format);
}

UCHAR get_dxgi_texel_size(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8_UNORM:
		return 1;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_TYPELESS:
		return 2;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_R24G8_TYPELESS:
		return 4;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return 8;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 16;
	}

	fmt::throw_exception("Unsupported DXGI format 0x%X" HERE, (u32)format);
}

UINT get_texture_max_aniso(rsx::texture_max_anisotropy aniso)
{
	switch (aniso)
	{
	case rsx::texture_max_anisotropy::x1: return 1;
	case rsx::texture_max_anisotropy::x2: return 2;
	case rsx::texture_max_anisotropy::x4: return 4;
	case rsx::texture_max_anisotropy::x6: return 6;
	case rsx::texture_max_anisotropy::x8: return 8;
	case rsx::texture_max_anisotropy::x10: return 10;
	case rsx::texture_max_anisotropy::x12: return 12;
	case rsx::texture_max_anisotropy::x16: return 16;
	}
	fmt::throw_exception("Invalid texture max aniso (0x%x)" HERE, (u32)aniso);
}

D3D12_TEXTURE_ADDRESS_MODE get_texture_wrap_mode(rsx::texture_wrap_mode wrap)
{
	switch (wrap)
	{
	case rsx::texture_wrap_mode::wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case rsx::texture_wrap_mode::mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case rsx::texture_wrap_mode::clamp_to_edge: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case rsx::texture_wrap_mode::border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case rsx::texture_wrap_mode::clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case rsx::texture_wrap_mode::mirror_once_clamp_to_edge: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case rsx::texture_wrap_mode::mirror_once_border: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case rsx::texture_wrap_mode::mirror_once_clamp: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}
	fmt::throw_exception("Invalid texture wrap mode (0x%x)" HERE, (u32)wrap);
}

namespace
{
	void get_min_filter(rsx::texture_minify_filter min_filter, D3D12_FILTER_TYPE &min, D3D12_FILTER_TYPE &mip)
	{
		switch (min_filter)
		{
		case rsx::texture_minify_filter::nearest:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case rsx::texture_minify_filter::linear:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case rsx::texture_minify_filter::nearest_nearest:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case rsx::texture_minify_filter::linear_nearest:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case rsx::texture_minify_filter::nearest_linear:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_LINEAR;
			return;
		case rsx::texture_minify_filter::linear_linear:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_LINEAR;
			return;
		case rsx::texture_minify_filter::convolution_min:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		}
		fmt::throw_exception("Invalid max filter" HERE);
	}

	D3D12_FILTER_TYPE get_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return D3D12_FILTER_TYPE_POINT;
		case rsx::texture_magnify_filter::linear: return D3D12_FILTER_TYPE_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return D3D12_FILTER_TYPE_LINEAR;
		}
		fmt::throw_exception("Invalid mag filter" HERE);
	}
}

D3D12_FILTER get_texture_filter(rsx::texture_minify_filter min_filter, rsx::texture_magnify_filter mag_filter)
{
	D3D12_FILTER_TYPE min, mip;
	get_min_filter(min_filter, min, mip);
	D3D12_FILTER_TYPE mag = get_mag_filter(mag_filter);
	return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(rsx::primitive_type draw_mode)
{
	switch (draw_mode)
	{
	case rsx::primitive_type::points: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case rsx::primitive_type::lines: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case rsx::primitive_type::line_loop: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
	case rsx::primitive_type::line_strip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case rsx::primitive_type::triangles: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case rsx::primitive_type::triangle_strip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case rsx::primitive_type::triangle_fan: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case rsx::primitive_type::quads: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case rsx::primitive_type::quad_strip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case rsx::primitive_type::polygon: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	fmt::throw_exception("Invalid draw mode (0x%x)" HERE, (u32)draw_mode);
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE get_primitive_topology_type(rsx::primitive_type draw_mode)
{
	switch (draw_mode)
	{
	case rsx::primitive_type::points: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case rsx::primitive_type::lines: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case rsx::primitive_type::line_strip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case rsx::primitive_type::triangles: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::triangle_strip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::triangle_fan: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::quads: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::quad_strip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::polygon: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case rsx::primitive_type::line_loop: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	}
	fmt::throw_exception("Invalid draw mode (0x%x)" HERE, (u32)draw_mode);
}

DXGI_FORMAT get_color_surface_format(rsx::surface_color_format format)
{
	switch (format)
	{
	case rsx::surface_color_format::x1r5g5b5_z1r5g5b5: 
	case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case rsx::surface_color_format::r5g6b5: return DXGI_FORMAT_B5G6R5_UNORM;
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case rsx::surface_color_format::a8b8g8r8: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
	case rsx::surface_color_format::a8r8g8b8: 
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case rsx::surface_color_format::b8: return DXGI_FORMAT_R8_UNORM;
	case rsx::surface_color_format::g8b8: return DXGI_FORMAT_R8G8_UNORM;
	case rsx::surface_color_format::w16z16y16x16: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case rsx::surface_color_format::w32z32y32x32: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case rsx::surface_color_format::x32: return DXGI_FORMAT_R32_FLOAT;
	}
	fmt::throw_exception("Invalid format (0x%x)" HERE, (u32)format);
}

DXGI_FORMAT get_depth_stencil_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_D16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	fmt::throw_exception("Invalid format (0x%x)" HERE, (u32)format);
}

DXGI_FORMAT get_depth_stencil_surface_clear_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_D16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	fmt::throw_exception("Invalid format (0x%x)" HERE, (u32)format);
}

DXGI_FORMAT get_depth_stencil_typeless_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_R16_TYPELESS;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_R24G8_TYPELESS;
	}
	fmt::throw_exception("Invalid format (0x%x)" HERE, (u32)format);
}

DXGI_FORMAT get_depth_samplable_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_R16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	fmt::throw_exception("Invalid format (0x%x)" HERE, (u32)format);
}

BOOL get_front_face_ccw(rsx::front_face ffv)
{
	switch (ffv)
	{
	case rsx::front_face::cw: return FALSE;
	case rsx::front_face::ccw: return TRUE;
	}
	fmt::throw_exception("Invalid front face value (0x%x)" HERE, (u32)ffv);
}

D3D12_CULL_MODE get_cull_face(rsx::cull_face cfv)
{
	switch (cfv)
	{
		case rsx::cull_face::front: return D3D12_CULL_MODE_FRONT;
		case rsx::cull_face::back: return D3D12_CULL_MODE_BACK;
		case rsx::cull_face::front_and_back: return D3D12_CULL_MODE_NONE;
	}
	fmt::throw_exception("Invalid cull face value (0x%x)" HERE, (u32)cfv);
}

DXGI_FORMAT get_index_type(rsx::index_array_type index_type)
{
	switch (index_type)
	{
	case rsx::index_array_type::u16: return DXGI_FORMAT_R16_UINT;
	case rsx::index_array_type::u32: return DXGI_FORMAT_R32_UINT;
	}
	fmt::throw_exception("Invalid index_type (0x%x)" HERE, (u32)index_type);
}

DXGI_FORMAT get_vertex_attribute_format(rsx::vertex_base_type type, u8 size)
{
	switch (type)
	{
	case rsx::vertex_base_type::s1:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_SNORM;
		case 2: return DXGI_FORMAT_R16G16_SNORM;
		case 3: return DXGI_FORMAT_R16G16B16A16_SNORM; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_SNORM;
		}
		break;
	}
	case rsx::vertex_base_type::f:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R32_FLOAT;
		case 2: return DXGI_FORMAT_R32G32_FLOAT;
		case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		break;
	}
	case rsx::vertex_base_type::sf:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_FLOAT;
		case 2: return DXGI_FORMAT_R16G16_FLOAT;
		case 3: return DXGI_FORMAT_R16G16B16A16_FLOAT; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		break;
	}
	case rsx::vertex_base_type::ub:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R8_UNORM;
		case 2: return DXGI_FORMAT_R8G8_UNORM;
		case 3: return DXGI_FORMAT_R8G8B8A8_UNORM; // No 3 channel type
		case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		break;
	}
	case rsx::vertex_base_type::s32k:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_SINT;
		case 2: return DXGI_FORMAT_R16G16_SINT;
		case 3: return DXGI_FORMAT_R16G16B16A16_SINT; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
		}
		break;
	}
	case rsx::vertex_base_type::cmp:
	{
		return DXGI_FORMAT_R16G16B16A16_SNORM;
	}
	case rsx::vertex_base_type::ub256:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R8_UINT;
		case 2: return DXGI_FORMAT_R8G8_UINT;
		case 3: return DXGI_FORMAT_R8G8B8A8_UINT; // No 3 channel type
		case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
		}
		break;
	}
	}

	fmt::throw_exception("Invalid or unsupported type or size (type=0x%x, size=0x%x)" HERE, (u32)type, size);
}

D3D12_RECT get_scissor(u16 clip_origin_x, u16 clip_origin_y, u16 clip_w, u16 clip_h)
{
	return{
		clip_origin_x,
		clip_origin_y,
		clip_origin_x + clip_w,
		clip_origin_y + clip_h,
	};
}
#endif
