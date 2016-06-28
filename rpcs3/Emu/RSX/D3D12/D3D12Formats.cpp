#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12Formats.h"
#include "D3D12Utils.h"
#include "Emu/RSX/GCM.h"


D3D12_BLEND_OP get_blend_op(u16 op)
{
	switch (op)
	{
	case CELL_GCM_FUNC_ADD: return D3D12_BLEND_OP_ADD;
	case CELL_GCM_FUNC_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
	case CELL_GCM_FUNC_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
	case CELL_GCM_MIN: return D3D12_BLEND_OP_MIN;
	case CELL_GCM_MAX: return D3D12_BLEND_OP_MAX;
	case CELL_GCM_FUNC_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED:
		break;
	}
	throw EXCEPTION("Invalid or unsupported blend op (0x%x)", op);
}

D3D12_BLEND get_blend_factor(u16 factor)
{
	switch (factor)
	{
	case CELL_GCM_ZERO: return D3D12_BLEND_ZERO;
	case CELL_GCM_ONE: return D3D12_BLEND_ONE;
	case CELL_GCM_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case CELL_GCM_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case CELL_GCM_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case CELL_GCM_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case CELL_GCM_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case CELL_GCM_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case CELL_GCM_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case CELL_GCM_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	case CELL_GCM_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	{
		LOG_ERROR(RSX,"Constant blend factor not supported. Using ONE instead");
		return D3D12_BLEND_ONE;
	}
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA: 
	{
		LOG_ERROR(RSX,"Inv Constant blend factor not supported. Using ZERO instead");
		return D3D12_BLEND_ZERO;
	}
	}
	throw EXCEPTION("Invalid blend factor (0x%x)", factor);
}

D3D12_BLEND get_blend_factor_alpha(u16 factor)
{
	switch (factor)
	{
	case CELL_GCM_ZERO: return D3D12_BLEND_ZERO;
	case CELL_GCM_ONE: return D3D12_BLEND_ONE;
	case CELL_GCM_SRC_COLOR: return D3D12_BLEND_SRC_ALPHA;
	case CELL_GCM_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_ALPHA;
	case CELL_GCM_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case CELL_GCM_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case CELL_GCM_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case CELL_GCM_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_DST_COLOR: return D3D12_BLEND_DEST_ALPHA;
	case CELL_GCM_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	case CELL_GCM_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	{
		LOG_ERROR(RSX,"Constant blend factor not supported. Using ONE instead");
		return D3D12_BLEND_ONE;
	}
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA: 
	{
		LOG_ERROR(RSX,"Inv Constant blend factor not supported. Using ZERO instead");
		return D3D12_BLEND_ZERO;
	}
	}
	throw EXCEPTION("Invalid blend alpha factor (0x%x)", factor);
}

/**
* Convert GCM logic op code to D3D12 one
*/
D3D12_LOGIC_OP get_logic_op(u32 op)
{
	switch (op)
	{
	case CELL_GCM_CLEAR: return D3D12_LOGIC_OP_CLEAR;
	case CELL_GCM_AND: return D3D12_LOGIC_OP_AND;
	case CELL_GCM_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
	case CELL_GCM_COPY: return D3D12_LOGIC_OP_COPY;
	case CELL_GCM_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
	case CELL_GCM_NOOP: return D3D12_LOGIC_OP_NOOP;
	case CELL_GCM_XOR: return D3D12_LOGIC_OP_XOR;
	case CELL_GCM_OR: return D3D12_LOGIC_OP_OR;
	case CELL_GCM_NOR: return D3D12_LOGIC_OP_NOR;
	case CELL_GCM_EQUIV: return D3D12_LOGIC_OP_EQUIV;
	case CELL_GCM_INVERT: return D3D12_LOGIC_OP_INVERT;
	case CELL_GCM_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
	case CELL_GCM_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
	case CELL_GCM_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
	case CELL_GCM_NAND: return D3D12_LOGIC_OP_NAND;
	}
	throw EXCEPTION("Invalid logic op (0x%x)", op);
}

/**
* Convert GCM stencil op code to D3D12 one
*/
D3D12_STENCIL_OP get_stencil_op(u32 op)
{
	switch (op)
	{
	case CELL_GCM_KEEP: return D3D12_STENCIL_OP_KEEP;
	case CELL_GCM_ZERO: return D3D12_STENCIL_OP_ZERO;
	case CELL_GCM_REPLACE: return D3D12_STENCIL_OP_REPLACE;
	case CELL_GCM_INCR: return D3D12_STENCIL_OP_INCR_SAT;
	case CELL_GCM_DECR: return D3D12_STENCIL_OP_DECR_SAT;
	case CELL_GCM_INVERT: return D3D12_STENCIL_OP_INVERT;
	case CELL_GCM_INCR_WRAP: return D3D12_STENCIL_OP_INCR;
	case CELL_GCM_DECR_WRAP: return D3D12_STENCIL_OP_DECR;
	}
	throw EXCEPTION("Invalid stencil op (0x%x)", op);
}

D3D12_COMPARISON_FUNC get_compare_func(u32 op)
{
	switch (op)
	{
	case CELL_GCM_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case CELL_GCM_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case CELL_GCM_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case CELL_GCM_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case CELL_GCM_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case CELL_GCM_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case CELL_GCM_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case CELL_GCM_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	throw EXCEPTION("Invalid or unsupported compare func (0x%x)", op);
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
	case CELL_GCM_TEXTURE_D8R8G8B8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return DXGI_FORMAT_R8G8_B8G8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return DXGI_FORMAT_R8G8_SNORM;
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8: return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return DXGI_FORMAT_R8G8_B8G8_UNORM;
		break;
	}
	throw EXCEPTION("Invalid or unsupported texture format (0x%x)", format);
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
	throw EXCEPTION("Invalid texture max aniso (0x%x)", aniso);
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
	throw EXCEPTION("Invalid texture wrap mode (0x%x)", wrap);
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
		throw EXCEPTION("Invalid max filter");
	}

	D3D12_FILTER_TYPE get_mag_filter(rsx::texture_magnify_filter mag_filter)
	{
		switch (mag_filter)
		{
		case rsx::texture_magnify_filter::nearest: return D3D12_FILTER_TYPE_POINT;
		case rsx::texture_magnify_filter::linear: return D3D12_FILTER_TYPE_LINEAR;
		case rsx::texture_magnify_filter::convolution_mag: return D3D12_FILTER_TYPE_LINEAR;
		}
		throw EXCEPTION("Invalid mag filter");
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
	case rsx::primitive_type::quad_strip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case rsx::primitive_type::polygon: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	throw EXCEPTION("Invalid draw mode (0x%x)", draw_mode);
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
	throw EXCEPTION("Invalid or unsupported draw mode (0x%x)", draw_mode);
}

DXGI_FORMAT get_color_surface_format(rsx::surface_color_format format)
{
	switch (format)
	{
	case rsx::surface_color_format::r5g6b5: return DXGI_FORMAT_B5G6R5_UNORM;
	case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
	case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case rsx::surface_color_format::a8b8g8r8: return DXGI_FORMAT_B8G8R8A8_UNORM;
	case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
	case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
	case rsx::surface_color_format::a8r8g8b8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case rsx::surface_color_format::b8: return DXGI_FORMAT_R8_UNORM;
	case rsx::surface_color_format::g8b8: return DXGI_FORMAT_R8G8_UNORM;
	case rsx::surface_color_format::w16z16y16x16: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case rsx::surface_color_format::w32z32y32x32: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case rsx::surface_color_format::x32: return DXGI_FORMAT_R32_FLOAT;
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

DXGI_FORMAT get_depth_stencil_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_D16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

DXGI_FORMAT get_depth_stencil_surface_clear_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_D16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

DXGI_FORMAT get_depth_stencil_typeless_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_R16_TYPELESS;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_R24G8_TYPELESS;
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

DXGI_FORMAT get_depth_samplable_surface_format(rsx::surface_depth_format format)
{
	switch (format)
	{
	case rsx::surface_depth_format::z16: return DXGI_FORMAT_R16_UNORM;
	case rsx::surface_depth_format::z24s8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	throw EXCEPTION("Invalid format (0x%x)", format);
}

BOOL get_front_face_ccw(u32 ffv)
{
	switch (ffv)
	{
	default: // Disgaea 3 pass some garbage value at startup, this is needed to survive.
	case CELL_GCM_CW: return FALSE;
	case CELL_GCM_CCW: return TRUE;
	}
	throw EXCEPTION("Invalid front face value (0x%x)", ffv);
}

DXGI_FORMAT get_index_type(rsx::index_array_type index_type)
{
	switch (index_type)
	{
	case rsx::index_array_type::u16: return DXGI_FORMAT_R16_UINT;
	case rsx::index_array_type::u32: return DXGI_FORMAT_R32_UINT;
	}
	throw EXCEPTION("Invalid index_type (0x%x)", index_type);
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
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16G16B16A16_SNORM;
		case 2:
		case 3:
		case 4: throw EXCEPTION("Unsupported CMP vertex format with size > 1");
		}
		break;
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

	throw EXCEPTION("Invalid or unsupported type or size (type=0x%x, size=0x%x)", type, size);
}

D3D12_RECT get_scissor(u32 horizontal, u32 vertical)
{
	return{
		horizontal & 0xFFFF,
		vertical & 0xFFFF,
		(horizontal & 0xFFFF) + (horizontal >> 16),
		(vertical & 0xFFFF) + (vertical >> 16)
	};
}
#endif
