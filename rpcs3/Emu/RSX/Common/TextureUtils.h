#pragma once

#include "../RSXTexture.h"

#include <vector>
#include "Utilities/GSL.h"

namespace rsx
{
	enum texture_upload_context : u32
	{
		shader_read = 1,
		blit_engine_src = 2,
		blit_engine_dst = 4,
		framebuffer_storage = 8
	};

	enum texture_colorspace : u32
	{
		rgb_linear = 0,
		srgb_nonlinear = 1
	};

	enum surface_usage_flags : u32
	{
		unknown = 0,
		attachment = 1,
		storage = 2,
	};

	enum surface_metrics : u32
	{
		pixels = 0,
		samples = 1,
		bytes = 2
	};

	enum surface_access : u32
	{
		read = 0,
		write = 1,
		transfer = 2
	};

	//Sampled image descriptor
	struct sampled_image_descriptor_base
	{
		texture_upload_context upload_context = texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = texture_dimension_extended::texture_dimension_2d;
		bool is_depth_texture = false;
		bool is_cyclic_reference = false;
		f32 scale_x = 1.f;
		f32 scale_y = 1.f;

		virtual ~sampled_image_descriptor_base() = default;
		virtual u32 encoded_component_map() const = 0;
	};

	struct typeless_xfer
	{
		bool src_is_typeless = false;
		bool dst_is_typeless = false;
		bool src_is_depth = false;
		bool dst_is_depth = false;
		bool flip_vertical = false;
		bool flip_horizontal = false;

		u32 src_gcm_format = 0;
		u32 dst_gcm_format = 0;
		u32 src_native_format_override = 0;
		u32 dst_native_format_override = 0;
		f32 src_scaling_hint = 1.f;
		f32 dst_scaling_hint = 1.f;
		texture_upload_context src_context = texture_upload_context::blit_engine_src;
		texture_upload_context dst_context = texture_upload_context::blit_engine_dst;

		void analyse()
		{
			if (src_is_typeless && dst_is_typeless)
			{
				if (src_scaling_hint == dst_scaling_hint &&
					src_scaling_hint != 1.f)
				{
					if (src_is_depth == dst_is_depth)
					{
						src_is_typeless = dst_is_typeless = false;
						src_scaling_hint = dst_scaling_hint = 1.f;
					}
				}
			}
		}
	};
}

struct rsx_subresource_layout
{
	gsl::span<const gsl::byte> data;
	u16 width_in_block;
	u16 height_in_block;
	u16 depth;
	u8  border;
	u8  reserved;
	u32 pitch_in_block;
};

/**
* Get size to store texture in a linear fashion.
* Storage is assumed to use a rowPitchAlignment boundary for every row of texture.
*/
size_t get_placed_texture_storage_size(u16 width, u16 height, u32 depth, u8 format, u16 mipmap, bool cubemap, size_t row_pitch_alignment, size_t mipmap_alignment);
size_t get_placed_texture_storage_size(const rsx::fragment_texture &texture, size_t row_pitch_alignment, size_t mipmap_alignment = 0x200);
size_t get_placed_texture_storage_size(const rsx::vertex_texture &texture, size_t row_pitch_alignment, size_t mipmap_alignment = 0x200);

/**
 * get all rsx_subresource_layout for texture.
 * The subresources are ordered per layer then per mipmap level (as in rsx memory).
 */
std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::fragment_texture &texture);
std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::vertex_texture &texture);

void upload_texture_subresource(gsl::span<gsl::byte> dst_buffer, const rsx_subresource_layout &src_layout, int format, bool is_swizzled, bool vtc_support, size_t dst_row_pitch_multiple_of);

u8 get_format_block_size_in_bytes(int format);
u8 get_format_block_size_in_texel(int format);
u8 get_format_block_size_in_bytes(rsx::surface_color_format format);

u8 get_format_sample_count(rsx::surface_antialiasing antialias);

/**
 * Returns number of texel rows encoded in one pitch-length line of bytes
 */
u8 get_format_texel_rows_per_line(u32 format);

/**
* Get number of bytes occupied by texture in RSX mem
*/
size_t get_texture_size(const rsx::fragment_texture &texture);
size_t get_texture_size(const rsx::vertex_texture &texture);

/**
* Get packed pitch
*/
u32 get_format_packed_pitch(u32 format, u16 width, bool border = false, bool swizzled = false);

/**
* Reverse encoding
*/
u32 get_remap_encoding(const std::pair<std::array<u8, 4>, std::array<u8, 4>>& remap);

/**
 * Get gcm texel layout. Returns <format, byteswapped>
 */
std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_color_format format);
std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_depth_format format);
