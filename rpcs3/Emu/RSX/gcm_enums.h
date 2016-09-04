#pragma once
#include "Utilities/types.h"

namespace rsx
{
	enum class vertex_base_type : u8
	{
		s1, ///< signed byte
		f, ///< float
		sf, ///< half float
		ub, ///< unsigned byte interpreted as 0.f and 1.f
		s32k, ///< signed 32bits int
		cmp, ///< compressed aka X11G11Z10 and always 1. W.
		ub256, ///< unsigned byte interpreted as between 0 and 255.
	};

	vertex_base_type to_vertex_base_type(u8 in);

	enum class index_array_type : u8
	{
		u32,
		u16,
	};

	index_array_type to_index_array_type(u8 in);

	enum class primitive_type : u8
	{
		invalid,
		points,
		lines,
		line_loop, // line strip with last end being joined with first end.
		line_strip,
		triangles,
		triangle_strip,
		triangle_fan, // like strip except that every triangle share the first vertex and one instead of 2 from previous triangle.
		quads,
		quad_strip,
		polygon, // convex polygon
	};

	primitive_type to_primitive_type(u8 in);

	enum class surface_target : u8
	{
		none,
		surface_a,
		surface_b,
		surfaces_a_b,
		surfaces_a_b_c,
		surfaces_a_b_c_d,
	};

	surface_target to_surface_target(u8 in);

	enum class surface_depth_format : u8
	{
		z16, // unsigned 16 bits depth
		z24s8, // unsigned 24 bits depth + 8 bits stencil
	};

	surface_depth_format to_surface_depth_format(u8 in);

	enum class surface_antialiasing : u8
	{
		center_1_sample,
		diagonal_centered_2_samples,
		square_centered_4_samples,
		square_rotated_4_samples,
	};

	surface_antialiasing to_surface_antialiasing(u8 in);

	enum class surface_color_format : u8
	{
		x1r5g5b5_z1r5g5b5,
		x1r5g5b5_o1r5g5b5,
		r5g6b5,
		x8r8g8b8_z8r8g8b8,
		x8r8g8b8_o8r8g8b8,
		a8r8g8b8,
		b8,
		g8b8,
		w16z16y16x16,
		w32z32y32x32,
		x32,
		x8b8g8r8_z8b8g8r8,
		x8b8g8r8_o8b8g8r8,
		a8b8g8r8,
	};

	surface_color_format to_surface_color_format(u8 in);

	enum class window_origin : u8
	{
		top,
		bottom
	};

	window_origin to_window_origin(u8 in);

	enum class window_pixel_center : u8
	{
		half,
		integer
	};

	window_pixel_center to_window_pixel_center(u8 in);

	enum class comparison_function : u8
	{
		never,
		less,
		equal,
		less_or_equal,
		greater,
		not_equal,
		greater_or_equal,
		always
	};

	comparison_function to_comparison_function(u16 in);

	enum class fog_mode : u8
	{
		linear,
		exponential,
		exponential2,
		exponential_abs,
		exponential2_abs,
		linear_abs
	};

	fog_mode to_fog_mode(u32 in);

	enum class texture_dimension : u8
	{
		dimension1d,
		dimension2d,
		dimension3d,
	};

	texture_dimension to_texture_dimension(u8 in);

	enum class texture_wrap_mode : u8
	{
		wrap,
		mirror,
		clamp_to_edge,
		border,
		clamp,
		mirror_once_clamp_to_edge,
		mirror_once_border,
		mirror_once_clamp,
	};

	texture_wrap_mode to_texture_wrap_mode(u8 in);

	enum class texture_max_anisotropy : u8
	{
		x1,
		x2,
		x4,
		x6,
		x8,
		x10,
		x12,
		x16,
	};

	texture_max_anisotropy to_texture_max_anisotropy(u8 in);

	enum class texture_minify_filter : u8
	{
		nearest, ///< no filtering, mipmap base level
		linear, ///< linear filtering, mipmap base level
		nearest_nearest, ///< no filtering, closest mipmap level
		linear_nearest, ///< linear filtering, closest mipmap level
		nearest_linear, ///< no filtering, linear mix between closest mipmap levels
		linear_linear, ///< linear filtering, linear mix between closest mipmap levels
		convolution_min, ///< Unknown mode but looks close to linear_linear
	};

	texture_minify_filter to_texture_minify_filter(u8 in);

	enum class texture_magnify_filter : u8
	{
		nearest, ///< no filtering
		linear, ///< linear filtering
		convolution_mag, ///< Unknown mode but looks close to linear
	};

	texture_magnify_filter to_texture_magnify_filter(u8 in);

	enum class stencil_op : u8
	{
		keep,
		zero,
		replace,
		incr,
		decr,
		invert,
		incr_wrap,
		decr_wrap,
	};

	stencil_op to_stencil_op(u16 in);

	enum class blend_equation : u8
	{
		add,
		min,
		max,
		substract,
		reverse_substract,
		reverse_substract_signed,
		add_signed,
		reverse_add_signed,
	};

	blend_equation to_blend_equation(u16 in);

	enum class blend_factor : u8
	{
		zero,
		one,
		src_color,
		one_minus_src_color,
		dst_color,
		one_minus_dst_color,
		src_alpha,
		one_minus_src_alpha,
		dst_alpha,
		one_minus_dst_alpha,
		src_alpha_saturate,
		constant_color,
		one_minus_constant_color,
		constant_alpha,
		one_minus_constant_alpha,
	};

	blend_factor to_blend_factor(u16 in);

	enum class logic_op : u8
	{
		logic_clear,
		logic_and,
		logic_and_reverse,
		logic_copy,
		logic_and_inverted,
		logic_noop,
		logic_xor,
		logic_or,
		logic_nor,
		logic_equiv,
		logic_invert,
		logic_or_reverse,
		logic_copy_inverted,
		logic_or_inverted,
		logic_nand,
		logic_set,
	};

	logic_op to_logic_op(u16 in);

	enum class front_face : u8
	{
		cw, /// clockwise
		ccw /// counter clockwise
	};

	front_face to_front_face(u16 in);

	enum class cull_face : u8
	{
		front,
		back,
		front_and_back,
	};

	cull_face to_cull_face(u16 in);

	enum class user_clip_plane_op : u8
	{
		disable,
		less_than,
		greather_or_equal,
	};

	user_clip_plane_op to_user_clip_plane_op(u8 in);

	enum class shading_mode : u8
	{
		smooth,
		flat,
	};

	shading_mode to_shading_mode(u32 in);

	enum class polygon_mode : u8
	{
		point,
		line,
		fill,
	};

	polygon_mode to_polygon_mode(u32 in);

	namespace blit_engine
	{
		enum class transfer_origin : u8
		{
			center,
			corner,
		};

		transfer_origin to_transfer_origin(u8 in);

		enum class transfer_interpolator : u8
		{
			zoh,
			foh,
		};

		transfer_interpolator to_transfer_interpolator(u8 in);

		enum class transfer_operation : u8
		{
			srccopy_and,
			rop_and,
			blend_and,
			srccopy,
			srccopy_premult,
			blend_premult,
		};

		transfer_operation to_transfer_operation(u8 in);

		enum class transfer_source_format : u8
		{
			a1r5g5b5,
			x1r5g5b5,
			a8r8g8b8,
			x8r8g8b8,
			cr8yb8cb8ya8,
			yb8cr8ya8cb8,
			r5g6b5,
			y8,
			ay8,
			eyb8ecr8eya8ecb8,
			ecr8eyb8ecb8eya8,
			a8b8g8r8,
			x8b8g8r8,
		};

		transfer_source_format to_transfer_source_format(u8 in);

		enum class transfer_destination_format : u8
		{
			r5g6b5,
			a8r8g8b8,
			y32,
		};

		transfer_destination_format to_transfer_destination_format(u8 in);

		enum class context_surface : u8
		{
			surface2d,
			swizzle2d,
		};

		context_surface to_context_surface(u32 in);

		enum class context_dma : u8
		{
			to_memory_get_report,
			report_location_main,
		};

		context_dma to_context_dma(u32 in);
	}
}
