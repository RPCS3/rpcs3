#pragma once
#include "../Common/surface_store.h"
#include "GLHelpers.h"
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
		::gl::texture::internal_format sized_internal_format(rsx::surface_color_format color_format);
		color_format surface_color_format_to_gl(rsx::surface_color_format color_format);
		depth_format surface_depth_format_to_gl(rsx::surface_depth_format depth_format);
		u8 get_pixel_size(rsx::surface_depth_format format);
	}
}

namespace gl
{
	class render_target : public texture
	{
		bool is_cleared = false;

		u32  rsx_pitch = 0;
		u16  native_pitch = 0;

		u16  surface_height = 0;
		u16  surface_width = 0;
		u16  surface_pixel_size = 0;

		texture::internal_format compatible_internal_format = texture::internal_format::rgba8;

	public:
		render_target *old_contents = nullptr;

		render_target() {}

		void set_cleared(bool clear=true)
		{
			is_cleared = clear;
		}

		bool cleared() const
		{
			return is_cleared;
		}

		// Internal pitch is the actual row length in bytes of the openGL texture
		void set_native_pitch(u16 pitch)
		{
			native_pitch = pitch;
		}

		u16 get_native_pitch() const
		{
			return native_pitch;
		}

		// Rsx pitch
		void set_rsx_pitch(u16 pitch)
		{
			rsx_pitch = pitch;
		}

		u16 get_rsx_pitch() const
		{
			return rsx_pitch;
		}

		std::pair<u16, u16> get_dimensions()
		{
			if (!surface_height) surface_height = height();
			if (!surface_width) surface_width = width();

			return std::make_pair(surface_width, surface_height);
		}

		void set_compatible_format(texture::internal_format format)
		{
			compatible_internal_format = format;
		}

		texture::internal_format get_compatible_internal_format()
		{
			return compatible_internal_format;
		}

		// For an address within the texture, extract this sub-section's rect origin
		// Checks whether we need to scale the subresource if it is not handled in shader
		// NOTE1: When surface->real_pitch < rsx_pitch, the surface is assumed to have been scaled to fill the rsx_region
		std::tuple<bool, u16, u16> get_texture_subresource(u32 offset, bool scale_to_fit)
		{
			if (!offset)
			{
				return std::make_tuple(true, 0, 0);
			}

			if (!surface_height) surface_height = height();
			if (!surface_width) surface_width = width();

			u32 range = rsx_pitch * surface_height;
			if (offset < range)
			{
				if (!surface_pixel_size)
					surface_pixel_size = native_pitch / surface_width;

				const u32 y = (offset / rsx_pitch);
				u32 x = (offset % rsx_pitch) / surface_pixel_size;

				if (scale_to_fit)
				{
					const f32 x_scale = (f32)rsx_pitch / native_pitch;
					x = (u32)((f32)x / x_scale);
				}

				return std::make_tuple(true, (u16)x, (u16)y);
			}
			else
				return std::make_tuple(false, 0, 0);
		}
	};
}

struct gl_render_target_traits
{
	using surface_storage_type = std::unique_ptr<gl::render_target>;
	using surface_type = gl::render_target*;
	using command_list_type = void*;
	using download_buffer_object = std::vector<u8>;

	static
	std::unique_ptr<gl::render_target> create_new_surface(
		u32 /*address*/,
		rsx::surface_color_format surface_color_format,
		size_t width,
		size_t height,
		gl::render_target* old_surface
	)
	{
		std::unique_ptr<gl::render_target> result(new gl::render_target());

		auto format = rsx::internals::surface_color_format_to_gl(surface_color_format);
		auto internal_fmt = rsx::internals::sized_internal_format(surface_color_format);

		result->recreate(gl::texture::target::texture2D);
		result->set_native_pitch((u16)width * format.channel_count * format.channel_size);
		result->set_compatible_format(internal_fmt);

		__glcheck result->config()
			.size({ (int)width, (int)height })
			.type(format.type)
			.format(format.format)
			.internal_format(internal_fmt)
			.swizzle(format.swizzle.r, format.swizzle.g, format.swizzle.b, format.swizzle.a)
			.wrap(gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border)
			.apply();

		__glcheck result->pixel_pack_settings().swap_bytes(format.swap_bytes).aligment(1);
		__glcheck result->pixel_unpack_settings().swap_bytes(format.swap_bytes).aligment(1);

		if (old_surface != nullptr && old_surface->get_compatible_internal_format() == internal_fmt)
			result->old_contents = old_surface;

		result->set_cleared();
		return result;
	}

	static
	std::unique_ptr<gl::render_target> create_new_surface(
			u32 /*address*/,
		rsx::surface_depth_format surface_depth_format,
			size_t width,
			size_t height,
			gl::render_target* old_surface
		)
	{
		std::unique_ptr<gl::render_target> result(new gl::render_target());

		auto format = rsx::internals::surface_depth_format_to_gl(surface_depth_format);
		result->recreate(gl::texture::target::texture2D);

		__glcheck result->config()
			.size({ (int)width, (int)height })
			.type(format.type)
			.format(format.format)
			.internal_format(format.internal_format)
			.swizzle(gl::texture::channel::r, gl::texture::channel::r, gl::texture::channel::r, gl::texture::channel::r)
			.wrap(gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border, gl::texture::wrap::clamp_to_border)
			.apply();

		__glcheck result->pixel_pack_settings().aligment(1);
		__glcheck result->pixel_unpack_settings().aligment(1);

		u16 native_pitch = (u16)width * 2;
		if (surface_depth_format == rsx::surface_depth_format::z24s8)
			native_pitch *= 2;

		result->set_native_pitch(native_pitch);
		result->set_compatible_format(format.internal_format);

		if (old_surface != nullptr && old_surface->get_compatible_internal_format() == format.internal_format)
			result->old_contents = old_surface;

		return result;
	}

	static void prepare_rtt_for_drawing(void *, gl::render_target*) {}
	static void prepare_rtt_for_sampling(void *, gl::render_target*) {}
	
	static void prepare_ds_for_drawing(void *, gl::render_target*) {}
	static void prepare_ds_for_sampling(void *, gl::render_target*) {}

	static void invalidate_rtt_surface_contents(void *, gl::render_target *rtt, gl::render_target* /*old*/, bool forced) { if (forced) rtt->set_cleared(false); }
	static void invalidate_depth_surface_contents(void *, gl::render_target *ds, gl::render_target* /*old*/, bool) { ds->set_cleared(false);  }

	static
	bool rtt_has_format_width_height(const std::unique_ptr<gl::render_target> &rtt, rsx::surface_color_format format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs) //TODO
			return false;

		auto internal_fmt = rsx::internals::sized_internal_format(format);
		return rtt->get_compatible_internal_format() == internal_fmt && rtt->width() == width && rtt->height() == height;
	}

	static
	bool ds_has_format_width_height(const std::unique_ptr<gl::render_target> &rtt, rsx::surface_depth_format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs) //TODO
			return false;

		// TODO: check format
		return rtt->width() == width && rtt->height() == height;
	}

	// Note : pbo breaks fbo here so use classic texture copy
	static std::vector<u8> issue_download_command(gl::render_target* color_buffer, rsx::surface_color_format color_format, size_t width, size_t height)
	{
		auto pixel_format = rsx::internals::surface_color_format_to_gl(color_format);
		std::vector<u8> result(width * height * pixel_format.channel_count * pixel_format.channel_size);
		color_buffer->bind();
		glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)pixel_format.format, (GLenum)pixel_format.type, result.data());
		return result;
	}

	static std::vector<u8> issue_depth_download_command(gl::render_target* depth_stencil_buffer, rsx::surface_depth_format depth_format, size_t width, size_t height)
	{
		std::vector<u8> result(width * height * 4);

		auto pixel_format = rsx::internals::surface_depth_format_to_gl(depth_format);
		depth_stencil_buffer->bind();
		glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)pixel_format.format, (GLenum)pixel_format.type, result.data());
		return result;
	}

	static std::vector<u8> issue_stencil_download_command(gl::render_target*, size_t width, size_t height)
	{
		std::vector<u8> result(width * height * 4);
		return result;
	}

	static
	gsl::span<const gsl::byte> map_downloaded_buffer(const std::vector<u8> &buffer)
	{
		return{ reinterpret_cast<const gsl::byte*>(buffer.data()), ::narrow<int>(buffer.size()) };
	}

	static
	void unmap_downloaded_buffer(const std::vector<u8> &)
	{
	}

	static gl::render_target* get(const std::unique_ptr<gl::render_target> &in)
	{
		return in.get();
	}
};

struct surface_subresource
{
	gl::render_target *surface = nullptr;
	
	u16 x = 0;
	u16 y = 0;
	u16 w = 0;
	u16 h = 0;

	bool is_bound = false;
	bool is_depth_surface = false;
	bool is_clipped = false;

	surface_subresource() {}

	surface_subresource(gl::render_target *src, u16 X, u16 Y, u16 W, u16 H, bool _Bound, bool _Depth, bool _Clipped = false)
		: surface(src), x(X), y(Y), w(W), h(H), is_bound(_Bound), is_depth_surface(_Depth), is_clipped(_Clipped)
	{}
};

class gl_render_targets : public rsx::surface_store<gl_render_target_traits>
{
private:
	bool surface_overlaps(gl::render_target *surface, u32 surface_address, u32 texaddr, u16 *x, u16 *y, bool scale_to_fit)
	{
		bool is_subslice = false;
		u16  x_offset = 0;
		u16  y_offset = 0;

		if (surface_address > texaddr)
			return false;

		u32 offset = texaddr - surface_address;
		if (offset >= 0)
		{
			std::tie(is_subslice, x_offset, y_offset) = surface->get_texture_subresource(offset, scale_to_fit);
			if (is_subslice)
			{
				*x = x_offset;
				*y = y_offset;

				return true;
			}
		}

		return false;
	}

	bool is_bound(u32 address, bool is_depth)
	{
		if (is_depth)
		{
			const u32 bound_depth_address = std::get<0>(m_bound_depth_stencil);
			return (bound_depth_address == address);
		}

		for (auto &surface: m_bound_render_targets)
		{
			const u32 bound_address = std::get<0>(surface);
			if (bound_address == address)
				return true;
		}

		return false;
	}

	bool fits(gl::render_target*, std::pair<u16, u16> &dims, u16 x_offset, u16 y_offset, u16 width, u16 height) const
	{
		if ((x_offset + width) > dims.first) return false;
		if ((y_offset + height) > dims.second) return false;

		return true;
	}

public:
	surface_subresource get_surface_subresource_if_applicable(u32 texaddr, u16 requested_width, u16 requested_height, u16 requested_pitch, bool scale_to_fit =false, bool crop=false)
	{
		gl::render_target *surface = nullptr;
		u16  x_offset = 0;
		u16  y_offset = 0;

		for (auto &tex_info : m_render_targets_storage)
		{
			u32 this_address = std::get<0>(tex_info);
			surface = std::get<1>(tex_info).get();

			if (surface_overlaps(surface, this_address, texaddr, &x_offset, &y_offset, scale_to_fit))
			{
				if (surface->get_rsx_pitch() != requested_pitch)
					continue;

				auto dims = surface->get_dimensions();

				if (scale_to_fit)
				{
					f32  pitch_scaling = (f32)requested_pitch / surface->get_native_pitch();
					requested_width = (u16)((f32)requested_width / pitch_scaling);
				}

				if (fits(surface, dims, x_offset, y_offset, requested_width, requested_height))
					return{ surface, x_offset, y_offset, requested_width, requested_height, is_bound(this_address, false), false };
				else
				{
					if (crop) //Forcefully fit the requested region by clipping and scaling
					{
						u16 remaining_width = dims.first - x_offset;
						u16 remaining_height = dims.second - y_offset;

						return{ surface, x_offset, y_offset, remaining_width, remaining_height, is_bound(this_address, false), false, true };
					}

					if (dims.first >= requested_width && dims.second >= requested_height)
					{
						LOG_WARNING(RSX, "Overlapping surface exceeds bounds; returning full surface region");
						return{ surface, 0, 0, requested_width, requested_height, is_bound(this_address, false), false, true };
					}
				}
			}
		}

		//Check depth surfaces for overlap
		for (auto &tex_info : m_depth_stencil_storage)
		{
			u32 this_address = std::get<0>(tex_info);
			surface = std::get<1>(tex_info).get();

			if (surface_overlaps(surface, this_address, texaddr, &x_offset, &y_offset, scale_to_fit))
			{
				if (surface->get_rsx_pitch() != requested_pitch)
					continue;

				auto dims = surface->get_dimensions();
				
				if (scale_to_fit)
				{
					f32  pitch_scaling = (f32)requested_pitch / surface->get_native_pitch();
					requested_width = (u16)((f32)requested_width / pitch_scaling);
				}

				if (fits(surface, dims, x_offset, y_offset, requested_width, requested_height))
					return{ surface, x_offset, y_offset, requested_width, requested_height, is_bound(this_address, true), true };
				else
				{
					if (crop) //Forcefully fit the requested region by clipping and scaling
					{
						u16 remaining_width = dims.first - x_offset;
						u16 remaining_height = dims.second - y_offset;

						return{ surface, x_offset, y_offset, remaining_width, remaining_height, is_bound(this_address, true), true, true };
					}

					if (dims.first >= requested_width && dims.second >= requested_height)
					{
						LOG_WARNING(RSX, "Overlapping depth surface exceeds bounds; returning full surface region");
						return{ surface, 0, 0, requested_width, requested_height, is_bound(this_address, true), true, true };
					}
				}
			}
		}

		return {};
	}
};
