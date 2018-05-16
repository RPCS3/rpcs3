#pragma once

#include <d3d12.h>

/**
 * Convert GCM blend operator code to D3D12 one
 */
D3D12_BLEND_OP get_blend_op(rsx::blend_equation op);

/**
 * Convert GCM blend factor code to D3D12 one
 */
D3D12_BLEND get_blend_factor(rsx::blend_factor factor);

/**
 * Convert GCM blend factor code to D3D12 one for alpha component
 */
D3D12_BLEND get_blend_factor_alpha(rsx::blend_factor factor);

/**
* Convert GCM logic op code to D3D12 one
*/
D3D12_LOGIC_OP get_logic_op(rsx::logic_op op);

/**
 * Convert GCM stencil op code to D3D12 one
 */
D3D12_STENCIL_OP get_stencil_op(rsx::stencil_op op);

/**
 * Convert GCM comparison function code to D3D12 one.
 */
D3D12_COMPARISON_FUNC get_compare_func(rsx::comparison_function op);

/**
 * Convert GCM texture format to an equivalent one supported by D3D12.
 * Destination format may require a byte swap or data conversion.
 */
DXGI_FORMAT get_texture_format(u8 format);

/**
 * Convert texture aniso value to UINT.
 */
UINT get_texture_max_aniso(rsx::texture_max_anisotropy aniso);

/**
 * Convert texture wrap mode to D3D12_TEXTURE_ADDRESS_MODE
 */
D3D12_TEXTURE_ADDRESS_MODE get_texture_wrap_mode(rsx::texture_wrap_mode wrap);

/**
 * Convert minify and magnify filter to D3D12_FILTER
 */
D3D12_FILTER get_texture_filter(rsx::texture_minify_filter min_filter, rsx::texture_magnify_filter mag_filter);

/**
 * Convert draw mode to D3D12_PRIMITIVE_TOPOLOGY
 */
D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(rsx::primitive_type draw_mode);

/**
* Convert draw mode to D3D12_PRIMITIVE_TOPOLOGY_TYPE
*/
D3D12_PRIMITIVE_TOPOLOGY_TYPE get_primitive_topology_type(rsx::primitive_type draw_mode);

/**
 * Convert color surface format to DXGI_FORMAT
 */
DXGI_FORMAT get_color_surface_format(rsx::surface_color_format format);

/**
 * Convert depth stencil surface format to DXGI_FORMAT
 */
DXGI_FORMAT get_depth_stencil_surface_format(rsx::surface_depth_format format);

/**
 *Convert depth stencil surface format to DXGI_FORMAT suited for clear value
 */
DXGI_FORMAT get_depth_stencil_surface_clear_format(rsx::surface_depth_format format);

/**
 * Convert depth surface format to a typeless DXGI_FORMAT
 */
DXGI_FORMAT get_depth_stencil_typeless_surface_format(rsx::surface_depth_format format);

/**
* Convert depth surface format to a DXGI_FORMAT that can be depth sampled
*/
DXGI_FORMAT get_depth_samplable_surface_format(rsx::surface_depth_format format);

/**
* Get block size in bytes for a DXGI_FORMAT
*/
UCHAR get_dxgi_texel_size(DXGI_FORMAT format);

/**
 * Convert front face value to bool value telling whether front face is counterclockwise or not
 */
BOOL get_front_face_ccw(rsx::front_face set_front_face_value);

/**
* Convert cull face value to a D3D12_CULL_MODE telling whether cull face is front or back
*/
D3D12_CULL_MODE get_cull_face(rsx::cull_face set_cull_face_value);

/**
 * Convert index type to DXGI_FORMAT
 */
DXGI_FORMAT get_index_type(rsx::index_array_type index_type);

/**
 * Convert vertex attribute format and size to DXGI_FORMAT
 */
DXGI_FORMAT get_vertex_attribute_format(rsx::vertex_base_type type, u8 size);

/**
 * Convert scissor register value to D3D12_RECT
 */
D3D12_RECT get_scissor(u16 clip_origin_x, u16 clip_origin_y, u16 clip_w, u16 clip_h);
