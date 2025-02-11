#pragma once

#include "OpenGL.h"
#include "../Common/TextureUtils.h"
#include "GLHelpers.h"

namespace rsx
{
	class vertex_texture;
	class fragment_texture;
}

namespace gl
{
	struct pixel_buffer_layout
	{
		GLenum format;
		GLenum type;
		u8     size;
		bool   swap_bytes;
		u8     alignment;
	};

	struct image_memory_requirements
	{
		u64 image_size_in_texels;
		u64 image_size_in_bytes;
		u64 memory_required;
	};

	struct clear_cmd_info
	{
		GLbitfield aspect_mask = 0;

		struct
		{
			f32 value;
		}
		clear_depth{};

		struct
		{
			u8 mask;
			u8 value;
		}
		clear_stencil{};

		struct
		{
			u32 mask;
			u8 attachment_count;
			u8 r;
			u8 g;
			u8 b;
			u8 a;
		}
		clear_color{};
	};

	GLenum get_target(rsx::texture_dimension_extended type);
	GLenum get_sized_internal_format(u32 texture_format);
	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format);
	pixel_buffer_layout get_format_type(texture::internal_format format);
	std::array<GLenum, 4> get_swizzle_remap(u32 texture_format);

	viewable_image* create_texture(u32 gcm_format, u16 width, u16 height, u16 depth, u16 mipmaps, rsx::texture_dimension_extended type);

	bool formats_are_bitcast_compatible(const texture* texture1, const texture* texture2);
	void copy_typeless(gl::command_context& cmd, texture* dst, const texture* src, const coord3u& dst_region, const coord3u& src_region);
	void copy_typeless(gl::command_context& cmd, texture* dst, const texture* src);

	void* copy_image_to_buffer(gl::command_context& cmd, const pixel_buffer_layout& pack_info, const gl::texture* src, gl::buffer* dst,
		u32 dst_offset, const int src_level, const coord3u& src_region, image_memory_requirements* mem_info);

	void copy_buffer_to_image(gl::command_context& cmd, const pixel_buffer_layout& unpack_info, gl::buffer* src, gl::texture* dst,
		const void* src_offset, const int dst_level, const coord3u& dst_region, image_memory_requirements* mem_info);

	void upload_texture(gl::command_context& cmd, texture* dst, u32 gcm_format, bool is_swizzled, const std::vector<rsx::subresource_layout>& subresources_layout);

	void clear_attachments(gl::command_context& cmd, const clear_cmd_info& info);

	namespace debug
	{
		extern std::unique_ptr<texture> g_vis_texture;
	}

	void destroy_global_texture_resources();
}
