#pragma once

#include "../RSXTexture.h"

#include <vector>
#include "Utilities/GSL.h"

namespace rsx
{
	enum texture_upload_context
	{
		shader_read = 0,
		blit_engine_src = 1,
		blit_engine_dst = 2,
		framebuffer_storage = 3
	};

	//Sampled image descriptor
	struct sampled_image_descriptor_base
	{
		texture_upload_context upload_context = texture_upload_context::shader_read;
		rsx::texture_dimension_extended image_type = texture_dimension_extended::texture_dimension_2d;
		bool is_depth_texture = false;
		f32 scale_x = 1.f;
		f32 scale_y = 1.f;
	};
}

struct rsx_subresource_layout
{
	gsl::span<const gsl::byte> data;
	u16 width_in_block;
	u16 height_in_block;
	u16 depth;
	u32 pitch_in_bytes;
};

/**
* Get size to store texture in a linear fashion.
* Storage is assumed to use a rowPitchAlignement boundary for every row of texture.
*/
size_t get_placed_texture_storage_size(u16 width, u16 height, u32 depth, u8 format, u16 mipmap, bool cubemap, size_t row_pitch_alignement, size_t mipmap_alignment);
size_t get_placed_texture_storage_size(const rsx::fragment_texture &texture, size_t row_pitch_alignement, size_t mipmap_alignment = 0x200);
size_t get_placed_texture_storage_size(const rsx::vertex_texture &texture, size_t row_pitch_alignement, size_t mipmap_alignment = 0x200);

/**
 * get all rsx_subresource_layout for texture.
 * The subresources are ordered per layer then per mipmap level (as in rsx memory).
 */
std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::fragment_texture &texture);
std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::vertex_texture &texture);

void upload_texture_subresource(gsl::span<gsl::byte> dst_buffer, const rsx_subresource_layout &src_layout, int format, bool is_swizzled, size_t dst_row_pitch_multiple_of);

u8 get_format_block_size_in_bytes(int format);
u8 get_format_block_size_in_texel(int format);
u8 get_format_block_size_in_bytes(rsx::surface_color_format format);

/**
* Get number of bytes occupied by texture in RSX mem
*/
size_t get_texture_size(const rsx::fragment_texture &texture);
size_t get_texture_size(const rsx::vertex_texture &texture);