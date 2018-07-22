#pragma once
#include "../Common/surface_store.h"
#include "GLHelpers.h"
#include "stdafx.h"
#include "../RSXThread.h"
#include "../rsx_utils.h"

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
	class render_target : public viewable_image, public rsx::ref_counted, public rsx::render_target_descriptor<texture*>
	{
		u32 rsx_pitch = 0;
		u16 native_pitch = 0;

		u16 internal_width = 0;
		u16 internal_height = 0;
		u16 surface_height = 0;
		u16 surface_width = 0;
		u16 surface_pixel_size = 0;

	public:
		render_target(GLuint width, GLuint height, GLenum sized_format)
			: viewable_image(GL_TEXTURE_2D, width, height, 1, 1, sized_format)
		{}

		void set_cleared(bool clear=true)
		{
			dirty = !clear;
		}

		bool cleared() const
		{
			return !dirty;
		}

		// Internal pitch is the actual row length in bytes of the openGL texture
		void set_native_pitch(u16 pitch)
		{
			native_pitch = pitch;
		}

		u16 get_native_pitch() const override
		{
			return native_pitch;
		}

		// Rsx pitch
		void set_rsx_pitch(u16 pitch)
		{
			rsx_pitch = pitch;
		}

		u16 get_rsx_pitch() const override
		{
			return rsx_pitch;
		}

		u16 get_surface_width() const override
		{
			return surface_width;
		}

		u16 get_surface_height() const override
		{
			return surface_height;
		}

		texture* get_surface() override
		{
			return (gl::texture*)this;
		}

		u32 raw_handle() const
		{
			return id();
		}

		void update_surface()
		{
			internal_width = width();
			internal_height = height();
			surface_width = rsx::apply_inverse_resolution_scale(internal_width, true);
			surface_height = rsx::apply_inverse_resolution_scale(internal_height, true);
		}

		bool matches_dimensions(u16 _width, u16 _height) const
		{
			//Use forward scaling to account for rounding and clamping errors
			return (rsx::apply_resolution_scale(_width, true) == internal_width) && (rsx::apply_resolution_scale(_height, true) == internal_height);
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
		auto format = rsx::internals::surface_color_format_to_gl(surface_color_format);
		auto internal_fmt = rsx::internals::sized_internal_format(surface_color_format);

		std::unique_ptr<gl::render_target> result(new gl::render_target(rsx::apply_resolution_scale((u16)width, true),
			rsx::apply_resolution_scale((u16)height, true), (GLenum)internal_fmt));
		result->set_native_pitch((u16)width * format.channel_count * format.channel_size);

		std::array<GLenum, 4> native_layout = { (GLenum)format.swizzle.a, (GLenum)format.swizzle.r, (GLenum)format.swizzle.g, (GLenum)format.swizzle.b };
		result->set_native_component_layout(native_layout);
		result->old_contents = old_surface;

		result->set_cleared(false);
		result->update_surface();
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
		auto format = rsx::internals::surface_depth_format_to_gl(surface_depth_format);
		std::unique_ptr<gl::render_target> result(new gl::render_target(rsx::apply_resolution_scale((u16)width, true),
				rsx::apply_resolution_scale((u16)height, true), (GLenum)format.internal_format));

		u16 native_pitch = (u16)width * 2;
		if (surface_depth_format == rsx::surface_depth_format::z24s8)
			native_pitch *= 2;

		std::array<GLenum, 4> native_layout = { GL_RED, GL_RED, GL_RED, GL_RED };
		result->set_native_pitch(native_pitch);
		result->set_native_component_layout(native_layout);
		result->old_contents = old_surface;

		result->set_cleared(false);
		result->update_surface();
		return result;
	}

	static
	void get_surface_info(gl::render_target *surface, rsx::surface_format_info *info)
	{
		info->rsx_pitch = surface->get_rsx_pitch();
		info->native_pitch = surface->get_native_pitch();
		info->surface_width = surface->get_surface_width();
		info->surface_height = surface->get_surface_height();
		info->bpp = static_cast<u8>(info->native_pitch / info->surface_width);
	}

	static void prepare_rtt_for_drawing(void *, gl::render_target *rtt) { rtt->reset_refs(); }
	static void prepare_rtt_for_sampling(void *, gl::render_target*) {}
	
	static void prepare_ds_for_drawing(void *, gl::render_target *ds) { ds->reset_refs(); }
	static void prepare_ds_for_sampling(void *, gl::render_target*) {}

	static
	void invalidate_surface_contents(void *, gl::render_target *surface, gl::render_target* old_surface)
	{
		surface->set_cleared(false);
		surface->old_contents = old_surface;
		surface->reset_aa_mode();
	}

	static
	void notify_surface_invalidated(const std::unique_ptr<gl::render_target>&)
	{}

	static
	void notify_surface_persist(const std::unique_ptr<gl::render_target>& surface)
	{
		surface->save_aa_mode();
	}

	static
	bool rtt_has_format_width_height(const std::unique_ptr<gl::render_target> &rtt, rsx::surface_color_format format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs) //TODO
			return false;

		auto internal_fmt = rsx::internals::sized_internal_format(format);
		return rtt->get_internal_format() == internal_fmt && rtt->matches_dimensions((u16)width, (u16)height);
	}

	static
	bool ds_has_format_width_height(const std::unique_ptr<gl::render_target> &rtt, rsx::surface_depth_format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs) //TODO
			return false;

		// TODO: check format
		return rtt->matches_dimensions((u16)width, (u16)height);
	}

	// Note : pbo breaks fbo here so use classic texture copy
	static std::vector<u8> issue_download_command(gl::render_target* color_buffer, rsx::surface_color_format color_format, size_t width, size_t height)
	{
		auto pixel_format = rsx::internals::surface_color_format_to_gl(color_format);
		std::vector<u8> result(width * height * pixel_format.channel_count * pixel_format.channel_size);
		glBindTexture(GL_TEXTURE_2D, color_buffer->id());
		glGetTexImage(GL_TEXTURE_2D, 0, (GLenum)pixel_format.format, (GLenum)pixel_format.type, result.data());
		return result;
	}

	static std::vector<u8> issue_depth_download_command(gl::render_target* depth_stencil_buffer, rsx::surface_depth_format depth_format, size_t width, size_t height)
	{
		std::vector<u8> result(width * height * 4);

		auto pixel_format = rsx::internals::surface_depth_format_to_gl(depth_format);
		glBindTexture(GL_TEXTURE_2D, depth_stencil_buffer->id());
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

struct gl_render_targets : public rsx::surface_store<gl_render_target_traits>
{
	void free_invalidated()
	{
		invalidated_resources.remove_if([&](auto &rtt)
		{
			if (rtt->deref_count >= 2)
				return true;

			rtt->deref_count++;
			return false;
		});
	}
};
