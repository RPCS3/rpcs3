#pragma once
#include "../Common/surface_store.h"
#include "gl_helpers.h"
#include "stdafx.h"
#include "../RSXThread.h"

struct color_swizzle
{
	gl::texture::channel a = gl::texture::channel::a;
	gl::texture::channel r = gl::texture::channel::r;
	gl::texture::channel g = gl::texture::channel::g;
	gl::texture::channel b = gl::texture::channel::b;

	color_swizzle() = default;
	color_swizzle(gl::texture::channel a, gl::texture::channel r, gl::texture::channel g, gl::texture::channel b)
		: a(a), r(r), g(g), b(b)
	{
	}
};

struct color_format
{
	gl::texture::type type;
	gl::texture::format format;
	bool swap_bytes;
	int channel_count;
	int channel_size;
	color_swizzle swizzle;
};

struct depth_format
{
	gl::texture::type type;
	gl::texture::format format;
	gl::texture::internal_format internal_format;
};

namespace rsx
{
	namespace internals
	{
		color_format surface_color_format_to_gl(rsx::surface_color_format color_format);
		depth_format surface_depth_format_to_gl(rsx::surface_depth_format depth_format);
		u8 get_pixel_size(rsx::surface_depth_format format);

		const u32 mr_color_offset[rsx::limits::color_buffers_count] =
		{
			NV4097_SET_SURFACE_COLOR_AOFFSET,
			NV4097_SET_SURFACE_COLOR_BOFFSET,
			NV4097_SET_SURFACE_COLOR_COFFSET,
			NV4097_SET_SURFACE_COLOR_DOFFSET
		};

		const u32 mr_color_dma[rsx::limits::color_buffers_count] =
		{
			NV4097_SET_CONTEXT_DMA_COLOR_A,
			NV4097_SET_CONTEXT_DMA_COLOR_B,
			NV4097_SET_CONTEXT_DMA_COLOR_C,
			NV4097_SET_CONTEXT_DMA_COLOR_D
		};

		const u32 mr_color_pitch[rsx::limits::color_buffers_count] =
		{
			NV4097_SET_SURFACE_PITCH_A,
			NV4097_SET_SURFACE_PITCH_B,
			NV4097_SET_SURFACE_PITCH_C,
			NV4097_SET_SURFACE_PITCH_D
		};
	}
}

struct gl_render_target_traits
{
	using surface_storage_type = std::unique_ptr<gl::texture>;
	using surface_type = gl::texture*;
	using command_list_type = void*;
	using download_buffer_object = std::vector<u8>;

	static
	std::unique_ptr<gl::texture> create_new_surface(
		u32 address,
		rsx::surface_color_format surface_color_format,
		size_t width,
		size_t height
	)
	{
		std::unique_ptr<gl::texture> result(new gl::texture());

		auto format = rsx::internals::surface_color_format_to_gl(surface_color_format);
		result->recreate(gl::texture::target::texture2D);

		__glcheck result->config()
			.size({ (int)width, (int)height })
			.type(format.type)
			.format(format.format)
			.swizzle(format.swizzle.r, format.swizzle.g, format.swizzle.b, format.swizzle.a)
			.wrap(gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border)
			.apply();

		__glcheck result->pixel_pack_settings().swap_bytes(format.swap_bytes).aligment(1);
		__glcheck result->pixel_unpack_settings().swap_bytes(format.swap_bytes).aligment(1);

		return result;
	}

	static
	std::unique_ptr<gl::texture> create_new_surface(
			u32 address,
		rsx::surface_depth_format surface_depth_format,
			size_t width,
			size_t height
		)
	{
		std::unique_ptr<gl::texture> result(new gl::texture());

		auto format = rsx::internals::surface_depth_format_to_gl(surface_depth_format);
		result->recreate(gl::texture::target::texture2D);

		__glcheck result->config()
			.size({ (int)width, (int)height })
			.type(format.type)
			.format(format.format)
			.internal_format(format.internal_format)
			.wrap(gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border)
			.apply();

		__glcheck result->pixel_pack_settings().aligment(1);
		__glcheck result->pixel_unpack_settings().aligment(1);

		return result;
	}

	static void prepare_rtt_for_drawing(void *, gl::texture*) {}
	static void prepare_rtt_for_sampling(void *, gl::texture*) {}
	static void prepare_ds_for_drawing(void *, gl::texture*) {}
	static void prepare_ds_for_sampling(void *, gl::texture*) {}

	static
	bool rtt_has_format_width_height(const std::unique_ptr<gl::texture> &rtt, rsx::surface_color_format surface_color_format, size_t width, size_t height)
	{
		// TODO: check format
		return rtt->width() == width && rtt->height() == height;
	}

	static
		bool ds_has_format_width_height(const std::unique_ptr<gl::texture> &rtt, rsx::surface_depth_format surface_depth_stencil_format, size_t width, size_t height)
	{
		// TODO: check format
		return rtt->width() == width && rtt->height() == height;
	}

	// Note : pbo breaks fbo here so use classic texture copy
	static std::vector<u8> issue_download_command(gl::texture* color_buffer, rsx::surface_color_format color_format, size_t width, size_t height)
	{
		auto pixel_format = rsx::internals::surface_color_format_to_gl(color_format);
		std::vector<u8> result(width * height * pixel_format.channel_count * pixel_format.channel_size);
		color_buffer->bind();
		glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)pixel_format.format, (GLenum)pixel_format.type, result.data());
		return result;
	}

	static std::vector<u8> issue_depth_download_command(gl::texture* depth_stencil_buffer, rsx::surface_depth_format depth_format, size_t width, size_t height)
	{
		std::vector<u8> result(width * height * 4);

		auto pixel_format = rsx::internals::surface_depth_format_to_gl(depth_format);
		depth_stencil_buffer->bind();
		glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)pixel_format.format, (GLenum)pixel_format.type, result.data());
		return result;
	}

	static std::vector<u8> issue_stencil_download_command(gl::texture* depth_stencil_buffer, size_t width, size_t height)
	{
		std::vector<u8> result(width * height * 4);
		return result;
	}

	static
	gsl::span<const gsl::byte> map_downloaded_buffer(const std::vector<u8> &buffer)
	{
		return{ reinterpret_cast<const gsl::byte*>(buffer.data()), gsl::narrow<int>(buffer.size()) };
	}

	static
	void unmap_downloaded_buffer(const std::vector<u8> &)
	{
	}

	static gl::texture* get(const std::unique_ptr<gl::texture> &in)
	{
		return in.get();
	}
};


struct gl_render_targets : public rsx::surface_store<gl_render_target_traits>
{
};
