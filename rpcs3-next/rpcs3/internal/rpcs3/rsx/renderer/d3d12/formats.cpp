#include "stdafx_d3d12.h"
#ifdef _MSC_VER
#include "D3D12Formats.h"
#include "D3D12Utils.h"
#include "Emu/RSX/GCM.h"


D3D12_BLEND_OP get_blend_op(u16 op) noexcept
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
		unreachable("Unsupported blend op");
	}
	unreachable("Wrong blend op");
}

D3D12_BLEND get_blend_factor(u16 factor) noexcept
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
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA:
		unreachable("Unsupported blend color factor");
	}
	unreachable("Wrong blend color factor");
}

D3D12_BLEND get_blend_factor_alpha(u16 factor) noexcept
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
	case CELL_GCM_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case CELL_GCM_SRC_ALPHA_SATURATE: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_CONSTANT_COLOR:
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA:
		unreachable("Unsupported blend alpha factor");
	}
	unreachable("Wrong blend alpha factor");
}

/**
* Convert GCM logic op code to D3D12 one
*/
D3D12_LOGIC_OP get_logic_op(u32 op) noexcept
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
	unreachable("Wrong logic op");
}

/**
* Convert GCM stencil op code to D3D12 one
*/
D3D12_STENCIL_OP get_stencil_op(u32 op) noexcept
{
	switch (op)
	{
	case CELL_GCM_KEEP: return D3D12_STENCIL_OP_KEEP;
	case CELL_GCM_ZERO: return D3D12_STENCIL_OP_ZERO;
	case CELL_GCM_REPLACE: return D3D12_STENCIL_OP_REPLACE;
	case CELL_GCM_INCR: return D3D12_STENCIL_OP_INCR;
	case CELL_GCM_DECR: return D3D12_STENCIL_OP_DECR;
	case CELL_GCM_INVERT: return D3D12_STENCIL_OP_INVERT;
	case CELL_GCM_INCR_WRAP:
	case CELL_GCM_DECR_WRAP:
		LOG_WARNING(RSX, "Unsupported stencil op used %x, please report this to a developer.", op);
	}
	LOG_ERROR(RSX, "Unknow stencil op used %x, please report this to a developer.", op);
	unreachable("Wrong Stencil op");
}

D3D12_COMPARISON_FUNC get_compare_func(u32 op) noexcept
{
	switch (op)
	{
	case CELL_GCM_ZERO:
	case CELL_GCM_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case CELL_GCM_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case CELL_GCM_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case CELL_GCM_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case CELL_GCM_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case CELL_GCM_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case CELL_GCM_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case CELL_GCM_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	unreachable("Wrong compare function");
}

DXGI_FORMAT get_texture_format(int format) noexcept
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
		return DXGI_FORMAT_R8_UNORM;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		return DXGI_FORMAT_B4G4R4A4_UNORM;
	case CELL_GCM_TEXTURE_R5G6B5:
		return DXGI_FORMAT_B5G6R5_UNORM;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		return DXGI_FORMAT_BC2_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return DXGI_FORMAT_BC3_UNORM;
	case CELL_GCM_TEXTURE_G8B8:
		return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_R6G5B5:
		// Not native
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		return DXGI_FORMAT_R32_UINT;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case CELL_GCM_TEXTURE_DEPTH16:
		return DXGI_FORMAT_R16_UNORM;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case CELL_GCM_TEXTURE_X16:
		return DXGI_FORMAT_R16_UNORM;
	case CELL_GCM_TEXTURE_Y16_X16:
		return DXGI_FORMAT_R16G16_UNORM;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return DXGI_FORMAT_R8G8_B8G8_UNORM;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		unreachable( "Unimplemented Texture format");
	}
	unreachable("Wrong Texture format");
}

UINT get_texture_max_aniso(u8 aniso) noexcept
{
	switch (aniso)
	{
	case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1;
	case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2;
	case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4;
	case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6;
	case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8;
	case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10;
	case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12;
	case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16;
	}
	unreachable("Wrong Texture max aniso");
}

D3D12_TEXTURE_ADDRESS_MODE get_texture_wrap_mode(u8 wrap) noexcept
{
	switch (wrap)
	{
	case CELL_GCM_TEXTURE_WRAP: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case CELL_GCM_TEXTURE_MIRROR: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case CELL_GCM_TEXTURE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}
	unreachable("Wrong texture wrap mode");
}

namespace
{
	void get_min_filter(u8 min_filter, D3D12_FILTER_TYPE &min, D3D12_FILTER_TYPE &mip) noexcept
	{
		switch (min_filter)
		{
		case CELL_GCM_TEXTURE_NEAREST:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case CELL_GCM_TEXTURE_LINEAR:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case CELL_GCM_TEXTURE_NEAREST_NEAREST:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case CELL_GCM_TEXTURE_LINEAR_NEAREST:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		case CELL_GCM_TEXTURE_NEAREST_LINEAR:
			min = D3D12_FILTER_TYPE_POINT;
			mip = D3D12_FILTER_TYPE_LINEAR;
			return;
		case CELL_GCM_TEXTURE_LINEAR_LINEAR:
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_LINEAR;
			return;
		case CELL_GCM_TEXTURE_CONVOLUTION_MIN:
			LOG_WARNING(RSX, "CELL_GCM_TEXTURE_CONVOLUTION_MIN not supported, fallback to bilinear filtering");
			min = D3D12_FILTER_TYPE_LINEAR;
			mip = D3D12_FILTER_TYPE_POINT;
			return;
		}
		unreachable("Wrong min filter");
	}

	D3D12_FILTER_TYPE get_mag_filter(u8 mag_filter) noexcept
	{
		switch (mag_filter)
		{
		case CELL_GCM_TEXTURE_NEAREST: return D3D12_FILTER_TYPE_POINT;
		case CELL_GCM_TEXTURE_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
		}
		// Catherine uses this
		LOG_WARNING(RSX, "Unknow mag filter used %x, fallback to bilinear filtering", mag_filter);
		return D3D12_FILTER_TYPE_LINEAR;
	}
}

D3D12_FILTER get_texture_filter(u8 min_filter, u8 mag_filter) noexcept
{
	D3D12_FILTER_TYPE min, mip;
	get_min_filter(min_filter, min, mip);
	D3D12_FILTER_TYPE mag = get_mag_filter(mag_filter);
	return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(u8 draw_mode) noexcept
{
	switch (draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POINTS: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case CELL_GCM_PRIMITIVE_LINES: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case CELL_GCM_PRIMITIVE_LINE_LOOP: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
	case CELL_GCM_PRIMITIVE_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case CELL_GCM_PRIMITIVE_TRIANGLES: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		// Emulated
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
	case CELL_GCM_PRIMITIVE_QUADS: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	unreachable("Wrong draw mode");
}
D3D12_PRIMITIVE_TOPOLOGY_TYPE get_primitive_topology_type(u8 draw_mode) noexcept
{
	switch (draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POINTS: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case CELL_GCM_PRIMITIVE_QUADS:
		// unsupported
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
	unreachable("Wrong draw mode");
}

DXGI_FORMAT get_color_surface_format(u8 format) noexcept
{
	switch (format)
	{
	case CELL_GCM_SURFACE_R5G6B5: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_SURFACE_A8R8G8B8: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case CELL_GCM_SURFACE_F_X32: return DXGI_FORMAT_R32_FLOAT;
	}
	unreachable("Wrong color surface format");
}

DXGI_FORMAT get_depth_stencil_surface_format(u8 format) noexcept
{
	switch (format)
	{
	case CELL_GCM_SURFACE_Z16: return DXGI_FORMAT_D16_UNORM;
	case CELL_GCM_SURFACE_Z24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	unreachable("Wrong depth stencil surface format");
}

DXGI_FORMAT get_depth_stencil_surface_clear_format(u8 format) noexcept
{
	switch (format)
	{
	case CELL_GCM_SURFACE_Z16: return DXGI_FORMAT_D16_UNORM;
	case CELL_GCM_SURFACE_Z24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	unreachable("Wrong depth stencil surface format");
}

DXGI_FORMAT get_depth_stencil_typeless_surface_format(u8 format) noexcept
{
	switch (format)
	{
	case CELL_GCM_SURFACE_Z16: return DXGI_FORMAT_R16_TYPELESS;
	case CELL_GCM_SURFACE_Z24S8: return DXGI_FORMAT_R24G8_TYPELESS;
	}
	unreachable("Wrong depth stencil surface format");
}

DXGI_FORMAT get_depth_samplable_surface_format(u8 format) noexcept
{
	switch (format)
	{
	case CELL_GCM_SURFACE_Z16: return DXGI_FORMAT_R16_FLOAT;
	case CELL_GCM_SURFACE_Z24S8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	}
	unreachable("Wrong depth stencil surface format");
}

BOOL get_front_face_ccw(u32 set_front_face_value) noexcept
{
	switch (set_front_face_value)
	{
	default: // Disgaea 3 pass some garbage value at startup, this is needed to survive.
	case CELL_GCM_CW: return FALSE;
	case CELL_GCM_CCW: return TRUE;
	}
	unreachable("Wrong front face value");
}

DXGI_FORMAT get_index_type(u8 index_type) noexcept
{
	switch (index_type)
	{
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16: return DXGI_FORMAT_R16_UINT;
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32: return DXGI_FORMAT_R32_UINT;
	}
	unreachable("Wrong index type");
}

DXGI_FORMAT get_vertex_attribute_format(u8 type, u8 size) noexcept
{
	switch (type)
	{
	case CELL_GCM_VERTEX_S1:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_SNORM;
		case 2: return DXGI_FORMAT_R16G16_SNORM;
		case 3: return DXGI_FORMAT_R16G16B16A16_SNORM; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_SNORM;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_F:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R32_FLOAT;
		case 2: return DXGI_FORMAT_R32G32_FLOAT;
		case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_SF:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_FLOAT;
		case 2: return DXGI_FORMAT_R16G16_FLOAT;
		case 3: return DXGI_FORMAT_R16G16B16A16_FLOAT; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_UB:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R8_UNORM;
		case 2: return DXGI_FORMAT_R8G8_UNORM;
		case 3: return DXGI_FORMAT_R8G8B8A8_UNORM; // No 3 channel type
		case 4: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_S32K:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R16_SINT;
		case 2: return DXGI_FORMAT_R16G16_SINT;
		case 3: return DXGI_FORMAT_R16G16B16A16_SINT; // No 3 channel type
		case 4: return DXGI_FORMAT_R16G16B16A16_SINT;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_CMP:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R32_FLOAT;
		case 2: return DXGI_FORMAT_R32G32_FLOAT;
		case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		unreachable("Wrong type size");
	}
	case CELL_GCM_VERTEX_UB256:
	{
		switch (size)
		{
		case 1: return DXGI_FORMAT_R8_UINT;
		case 2: return DXGI_FORMAT_R8G8_UINT;
		case 3: return DXGI_FORMAT_R8G8B8A8_UINT; // No 3 channel type
		case 4: return DXGI_FORMAT_R8G8B8A8_UINT;
		}
		unreachable("Wrong type size");
	}
	}
	unreachable("Wrong type");
}

D3D12_RECT get_scissor(u32 horizontal, u32 vertical) noexcept
{
	return{
		horizontal & 0xFFFF,
		vertical & 0xFFFF,
		(horizontal & 0xFFFF) + (horizontal >> 16),
		(vertical & 0xFFFF) + (vertical >> 16)
	};
}
#endif
