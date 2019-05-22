﻿#pragma once
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

		u16 surface_height = 0;
		u16 surface_width = 0;
		u16 surface_pixel_size = 0;

	public:
		render_target(GLuint width, GLuint height, GLenum sized_format)
			: viewable_image(GL_TEXTURE_2D, width, height, 1, 1, sized_format)
		{}

		// Internal pitch is the actual row length in bytes of the openGL texture
		void set_native_pitch(u16 pitch)
		{
			native_pitch = pitch;
		}

		u16 get_native_pitch() const override
		{
			return native_pitch;
		}

		void set_surface_dimensions(u16 w, u16 h, u16 pitch)
		{
			surface_width = w;
			surface_height = h;
			rsx_pitch = pitch;
		}

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

		bool is_depth_surface() const override
		{
			switch (get_internal_format())
			{
			case gl::texture::internal_format::depth16:
			case gl::texture::internal_format::depth24_stencil8:
			case gl::texture::internal_format::depth32f_stencil8:
				return true;
			default:
				return false;
			}
		}

		void release_ref(texture* t) const override
		{
			static_cast<gl::render_target*>(t)->release();
		}

		texture* get_surface() override
		{
			return (gl::texture*)this;
		}

		u32 raw_handle() const
		{
			return id();
		}

		bool matches_dimensions(u16 _width, u16 _height) const
		{
			//Use forward scaling to account for rounding and clamping errors
			return (rsx::apply_resolution_scale(_width, true) == width()) && (rsx::apply_resolution_scale(_height, true) == height());
		}

		void memory_barrier(gl::command_context& cmd, bool force_init = false);
		void read_barrier(gl::command_context& cmd) { memory_barrier(cmd, true); }
		void write_barrier(gl::command_context& cmd) { memory_barrier(cmd, false); }
	};

	struct framebuffer_holder : public gl::fbo, public rsx::ref_counted
	{
		using gl::fbo::fbo;
	};

	static inline gl::render_target* as_rtt(gl::texture* t)
	{
		return reinterpret_cast<gl::render_target*>(t);
	}
}

struct gl_render_target_traits
{
	using surface_storage_type = std::unique_ptr<gl::render_target>;
	using surface_type = gl::render_target*;
	using command_list_type = gl::command_context&;
	using download_buffer_object = std::vector<u8>;
	using barrier_descriptor_t = rsx::deferred_clipped_region<gl::render_target*>;

	static
	std::unique_ptr<gl::render_target> create_new_surface(
		u32 address,
		rsx::surface_color_format surface_color_format,
		size_t width, size_t height, size_t pitch
	)
	{
		auto format = rsx::internals::surface_color_format_to_gl(surface_color_format);
		auto internal_fmt = rsx::internals::sized_internal_format(surface_color_format);

		std::unique_ptr<gl::render_target> result(new gl::render_target(rsx::apply_resolution_scale((u16)width, true),
			rsx::apply_resolution_scale((u16)height, true), (GLenum)internal_fmt));
		result->set_native_pitch((u16)width * format.channel_count * format.channel_size);
		result->set_surface_dimensions((u16)width, (u16)height, (u16)pitch);
		result->set_format(surface_color_format);

		std::array<GLenum, 4> native_layout = { (GLenum)format.swizzle.a, (GLenum)format.swizzle.r, (GLenum)format.swizzle.g, (GLenum)format.swizzle.b };
		result->set_native_component_layout(native_layout);

		result->memory_usage_flags = rsx::surface_usage_flags::attachment;
		result->state_flags = rsx::surface_state_flags::erase_bkgnd;
		result->queue_tag(address);
		result->add_ref();
		return result;
	}

	static
	std::unique_ptr<gl::render_target> create_new_surface(
			u32 address,
		rsx::surface_depth_format surface_depth_format,
			size_t width, size_t height, size_t pitch
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
		result->set_surface_dimensions((u16)width, (u16)height, (u16)pitch);
		result->set_native_component_layout(native_layout);
		result->set_format(surface_depth_format);

		result->memory_usage_flags = rsx::surface_usage_flags::attachment;
		result->state_flags = rsx::surface_state_flags::erase_bkgnd;
		result->queue_tag(address);
		result->add_ref();
		return result;
	}

	static
	void clone_surface(
		gl::command_context&,
		std::unique_ptr<gl::render_target>& sink, gl::render_target* ref,
		u32 address, barrier_descriptor_t& prev)
	{
		if (!sink)
		{
			auto internal_format = (GLenum)ref->get_internal_format();
			const auto new_w = rsx::apply_resolution_scale(prev.width, true, ref->get_surface_width());
			const auto new_h = rsx::apply_resolution_scale(prev.height, true, ref->get_surface_height());

			sink.reset(new gl::render_target(new_w, new_h, internal_format));
			sink->add_ref();

			sink->memory_usage_flags = rsx::surface_usage_flags::storage;
			sink->state_flags = rsx::surface_state_flags::erase_bkgnd;
			sink->format_info = ref->format_info;

			sink->set_native_pitch(prev.width * ref->get_bpp());
			sink->set_surface_dimensions(prev.width, prev.height, ref->get_rsx_pitch());
			sink->set_native_component_layout(ref->get_native_component_layout());
			sink->queue_tag(address);
		}
		else
		{
			sink->set_rsx_pitch(ref->get_rsx_pitch());
		}

		prev.target = sink.get();

		sink->sync_tag();
		sink->set_old_contents_region(prev, false);
		sink->last_use_tag = ref->last_use_tag;
	}

	static
	bool is_compatible_surface(const gl::render_target* surface, const gl::render_target* ref, u16 width, u16 height, u8 /*sample_count*/)
	{
		return (surface->get_internal_format() == ref->get_internal_format() &&
				surface->get_surface_width() >= width &&
				surface->get_surface_height() >= height);
	}

	static
	void get_surface_info(gl::render_target *surface, rsx::surface_format_info *info)
	{
		info->rsx_pitch = surface->get_rsx_pitch();
		info->native_pitch = surface->get_native_pitch();
		info->surface_width = surface->get_surface_width();
		info->surface_height = surface->get_surface_height();
		info->bpp = surface->get_bpp();
	}

	static void prepare_rtt_for_drawing(gl::command_context&, gl::render_target* rtt)
	{
		rtt->memory_usage_flags |= rsx::surface_usage_flags::attachment;
	}

	static void prepare_ds_for_drawing(gl::command_context&, gl::render_target* ds)
	{
		ds->memory_usage_flags |= rsx::surface_usage_flags::attachment;
	}

	static void prepare_rtt_for_sampling(gl::command_context&, gl::render_target*) {}
	static void prepare_ds_for_sampling(gl::command_context&, gl::render_target*) {}

	static
	bool surface_is_pitch_compatible(const std::unique_ptr<gl::render_target> &surface, size_t pitch)
	{
		return surface->get_rsx_pitch() == pitch;
	}

	static
	void invalidate_surface_contents(gl::command_context&, gl::render_target *surface, u32 address, size_t pitch)
	{
		surface->set_rsx_pitch((u16)pitch);
		surface->reset_aa_mode();
		surface->queue_tag(address);
		surface->last_use_tag = 0;
		surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
	}

	static
	void notify_surface_invalidated(const std::unique_ptr<gl::render_target>& surface)
	{
		if (surface->old_contents)
		{
			// TODO: Retire the deferred writes
			surface->clear_rw_barrier();
		}

		surface->release();
	}

	static
	void notify_surface_persist(const std::unique_ptr<gl::render_target>& surface)
	{
		surface->save_aa_mode();
	}

	static
	void notify_surface_reused(const std::unique_ptr<gl::render_target>& surface)
	{
		surface->state_flags |= rsx::surface_state_flags::erase_bkgnd;
		surface->add_ref();
	}

	static
	bool rtt_has_format_width_height(const std::unique_ptr<gl::render_target> &rtt, rsx::surface_color_format format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs && rtt->has_refs())
			return false;

		const auto internal_fmt = rsx::internals::sized_internal_format(format);
		return rtt->get_internal_format() == internal_fmt && rtt->matches_dimensions((u16)width, (u16)height);
	}

	static
	bool ds_has_format_width_height(const std::unique_ptr<gl::render_target> &ds, rsx::surface_depth_format format, size_t width, size_t height, bool check_refs=false)
	{
		if (check_refs && ds->has_refs())
			return false;

		const auto internal_fmt = rsx::internals::surface_depth_format_to_gl(format).internal_format;
		return ds->get_internal_format() == internal_fmt && ds->matches_dimensions((u16)width, (u16)height);
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
	void destroy()
	{
		invalidate_all();
		invalidated_resources.clear();
	}

	std::vector<GLuint> free_invalidated()
	{
		std::vector<GLuint> removed;
		invalidated_resources.remove_if([&](auto &rtt)
		{
			if (rtt->unused_check_count() >= 2)
			{
				removed.push_back(rtt->id());
				return true;
			}

			return false;
		});

		return removed;
	}
};
