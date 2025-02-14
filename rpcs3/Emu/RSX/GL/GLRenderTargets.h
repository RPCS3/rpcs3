#pragma once
#include "../Common/surface_store.h"
#include "../rsx_utils.h"

#include "glutils/fbo.h"

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
	gl::texture::internal_format internal_format;
	bool swap_bytes;
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
		depth_format surface_depth_format_to_gl(rsx::surface_depth_format2 depth_format);
		u8 get_pixel_size(rsx::surface_depth_format format);
	}
}

namespace gl
{
	class render_target : public viewable_image, public rsx::render_target_descriptor<texture*>
	{
		void clear_memory(gl::command_context& cmd, gl::texture* surface = nullptr);
		void load_memory(gl::command_context& cmd);
		void initialize_memory(gl::command_context& cmd, rsx::surface_access access);

		// MSAA support:
		// Get the linear resolve target bound to this surface. Initialize if none exists
		gl::viewable_image* get_resolve_target_safe(gl::command_context& cmd);
		// Resolve the planar MSAA data into a linear block
		void resolve(gl::command_context& cmd);
		// Unresolve the linear data into planar MSAA data
		void unresolve(gl::command_context& cmd);

	public:
		render_target(GLuint width, GLuint height, GLubyte samples, GLenum sized_format, rsx::format_class format_class)
			: viewable_image(GL_TEXTURE_2D, width, height, 1, 1, samples, sized_format, format_class)
		{}

		// Internal pitch is the actual row length in bytes of the openGL texture
		void set_native_pitch(u32 pitch)
		{
			native_pitch = pitch;
		}

		void set_surface_dimensions(u16 w, u16 h, u32 pitch)
		{
			surface_width = w;
			surface_height = h;
			rsx_pitch = pitch;
		}

		void set_rsx_pitch(u32 pitch)
		{
			rsx_pitch = pitch;
		}

		bool is_depth_surface() const override
		{
			return !!(aspect() & gl::image_aspect::depth);
		}

		viewable_image* get_surface(rsx::surface_access /*access_type*/) override;

		u32 raw_handle() const
		{
			return id();
		}

		bool matches_dimensions(u16 _width, u16 _height) const
		{
			//Use forward scaling to account for rounding and clamping errors
			const auto [scaled_w, scaled_h] = rsx::apply_resolution_scale<true>(_width, _height);
			return (scaled_w == width()) && (scaled_h == height());
		}

		void memory_barrier(gl::command_context& cmd, rsx::surface_access access);
		void read_barrier(gl::command_context& cmd) { memory_barrier(cmd, rsx::surface_access::shader_read); }
		void write_barrier(gl::command_context& cmd) { memory_barrier(cmd, rsx::surface_access::shader_write); }
	};

	struct framebuffer_holder : public gl::fbo, public rsx::ref_counted
	{
		using gl::fbo::fbo;
	};

	static inline gl::render_target* as_rtt(gl::texture* t)
	{
		return ensure(dynamic_cast<gl::render_target*>(t));
	}

	static inline const gl::render_target* as_rtt(const gl::texture* t)
	{
		return ensure(dynamic_cast<const gl::render_target*>(t));
	}
}

struct gl_render_target_traits
{
	using surface_storage_type = std::unique_ptr<gl::render_target>;
	using surface_type = gl::render_target*;
	using buffer_object_storage_type = std::unique_ptr<gl::buffer>;
	using buffer_object_type = gl::buffer*;
	using command_list_type = gl::command_context&;
	using download_buffer_object = std::vector<u8>;
	using barrier_descriptor_t = rsx::deferred_clipped_region<gl::render_target*>;

	static
	std::unique_ptr<gl::render_target> create_new_surface(
		u32 address,
		rsx::surface_color_format surface_color_format,
		usz width, usz height, usz pitch,
		rsx::surface_antialiasing antialias
	)
	{
		auto format = rsx::internals::surface_color_format_to_gl(surface_color_format);
		const auto [width_, height_] = rsx::apply_resolution_scale<true>(static_cast<u16>(width), static_cast<u16>(height));

		u8 samples;
		rsx::surface_sample_layout sample_layout;
		if (g_cfg.video.antialiasing_level == msaa_level::_auto)
		{
			samples = get_format_sample_count(antialias);
			sample_layout = rsx::surface_sample_layout::ps3;
		}
		else
		{
			samples = 1;
			sample_layout = rsx::surface_sample_layout::null;
		}

		std::unique_ptr<gl::render_target> result(new gl::render_target(width_, height_, samples,
			static_cast<GLenum>(format.internal_format), RSX_FORMAT_CLASS_COLOR));

		result->set_aa_mode(antialias);
		result->set_native_pitch(static_cast<u32>(width) * get_format_block_size_in_bytes(surface_color_format) * result->samples_x);
		result->set_surface_dimensions(static_cast<u16>(width), static_cast<u16>(height), static_cast<u32>(pitch));
		result->set_format(surface_color_format);

		std::array<GLenum, 4> native_layout = { static_cast<GLenum>(format.swizzle.a), static_cast<GLenum>(format.swizzle.r), static_cast<GLenum>(format.swizzle.g), static_cast<GLenum>(format.swizzle.b) };
		result->set_native_component_layout(native_layout);

		result->memory_usage_flags = rsx::surface_usage_flags::attachment;
		result->state_flags = rsx::surface_state_flags::erase_bkgnd;
		result->sample_layout = sample_layout;
		result->queue_tag(address);
		result->add_ref();
		return result;
	}

	static
	std::unique_ptr<gl::render_target> create_new_surface(
			u32 address,
			rsx::surface_depth_format2 surface_depth_format,
			usz width, usz height, usz pitch,
			rsx::surface_antialiasing antialias
		)
	{
		auto format = rsx::internals::surface_depth_format_to_gl(surface_depth_format);
		const auto [width_, height_] = rsx::apply_resolution_scale<true>(static_cast<u16>(width), static_cast<u16>(height));

		u8 samples;
		rsx::surface_sample_layout sample_layout;
		if (g_cfg.video.antialiasing_level == msaa_level::_auto)
		{
			samples = get_format_sample_count(antialias);
			sample_layout = rsx::surface_sample_layout::ps3;
		}
		else
		{
			samples = 1;
			sample_layout = rsx::surface_sample_layout::null;
		}

		std::unique_ptr<gl::render_target> result(new gl::render_target(width_, height_, samples,
			static_cast<GLenum>(format.internal_format), rsx::classify_format(surface_depth_format)));

		result->set_aa_mode(antialias);
		result->set_surface_dimensions(static_cast<u16>(width), static_cast<u16>(height), static_cast<u32>(pitch));
		result->set_format(surface_depth_format);
		result->set_native_pitch(static_cast<u32>(width) * get_format_block_size_in_bytes(surface_depth_format) * result->samples_x);

		std::array<GLenum, 4> native_layout = { GL_RED, GL_RED, GL_RED, GL_RED };
		result->set_native_component_layout(native_layout);

		result->memory_usage_flags = rsx::surface_usage_flags::attachment;
		result->state_flags = rsx::surface_state_flags::erase_bkgnd;
		result->sample_layout = sample_layout;
		result->queue_tag(address);
		result->add_ref();
		return result;
	}

	static
	void clone_surface(
		gl::command_context& cmd,
		std::unique_ptr<gl::render_target>& sink, gl::render_target* ref,
		u32 address, barrier_descriptor_t& prev)
	{
		if (!sink)
		{
			auto internal_format = static_cast<GLenum>(ref->get_internal_format());
			const auto [new_w, new_h] = rsx::apply_resolution_scale<true>(prev.width, prev.height,
				ref->get_surface_width<rsx::surface_metrics::pixels>(), ref->get_surface_height<rsx::surface_metrics::pixels>());

			sink = std::make_unique<gl::render_target>(new_w, new_h, ref->samples(), internal_format, ref->format_class());
			sink->add_ref();

			sink->memory_usage_flags = rsx::surface_usage_flags::storage;
			sink->state_flags = rsx::surface_state_flags::erase_bkgnd;
			sink->format_info = ref->format_info;

			sink->set_spp(ref->get_spp());
			sink->set_native_pitch(static_cast<u32>(prev.width) * ref->get_bpp() * ref->samples_x);
			sink->set_rsx_pitch(ref->get_rsx_pitch());
			sink->set_surface_dimensions(prev.width, prev.height, ref->get_rsx_pitch());
			sink->set_native_component_layout(ref->get_native_component_layout());
			sink->queue_tag(address);
		}

		sink->on_clone_from(ref);

		if (!sink->old_contents.empty())
		{
			// Deal with this, likely only needs to clear
			if (sink->surface_width > prev.width || sink->surface_height > prev.height)
			{
				sink->write_barrier(cmd);
			}
			else
			{
				sink->clear_rw_barrier();
			}
		}

		prev.target = sink.get();
		sink->set_old_contents_region(prev, false);
	}

	static
	std::unique_ptr<gl::render_target> convert_pitch(
		gl::command_context& /*cmd*/,
		std::unique_ptr<gl::render_target>& src,
		usz /*out_pitch*/)
	{
		// TODO
		src->state_flags = rsx::surface_state_flags::erase_bkgnd;
		return {};
	}

	static
	bool is_compatible_surface(const gl::render_target* surface, const gl::render_target* ref, u16 width, u16 height, u8 sample_count)
	{
		return (surface->get_internal_format() == ref->get_internal_format() &&
				surface->get_spp() == sample_count &&
				surface->get_surface_width<rsx::surface_metrics::pixels>() == width &&
				surface->get_surface_height<rsx::surface_metrics::pixels>() == height);
	}

	static
	void prepare_surface_for_drawing(gl::command_context& cmd, gl::render_target* surface)
	{
		surface->memory_barrier(cmd, rsx::surface_access::gpu_reference);
		surface->memory_usage_flags |= rsx::surface_usage_flags::attachment;
	}

	static
	void prepare_surface_for_sampling(gl::command_context&, gl::render_target*)
	{}

	static
	bool surface_is_pitch_compatible(const std::unique_ptr<gl::render_target> &surface, usz pitch)
	{
		return surface->get_rsx_pitch() == pitch;
	}

	static
	void int_invalidate_surface_contents(gl::command_context&, gl::render_target *surface, u32 address, usz pitch)
	{
		surface->set_rsx_pitch(static_cast<u32>(pitch));
		surface->queue_tag(address);
		surface->last_use_tag = 0;
		surface->stencil_init_flags = 0;
		surface->memory_usage_flags = rsx::surface_usage_flags::unknown;
		surface->raster_type = rsx::surface_raster_type::linear;
	}

	static
	void invalidate_surface_contents(
		gl::command_context& cmd,
		gl::render_target* surface,
		rsx::surface_color_format format,
		u32 address,
		usz pitch)
	{
		auto fmt = rsx::internals::surface_color_format_to_gl(format);
		std::array<GLenum, 4> native_layout = { static_cast<GLenum>(fmt.swizzle.a), static_cast<GLenum>(fmt.swizzle.r), static_cast<GLenum>(fmt.swizzle.g), static_cast<GLenum>(fmt.swizzle.b) };
		surface->set_native_component_layout(native_layout);
		surface->set_format(format);

		int_invalidate_surface_contents(cmd, surface, address, pitch);
	}

	static
	void invalidate_surface_contents(
		gl::command_context& cmd,
		gl::render_target* surface,
		rsx::surface_depth_format2 format,
		u32 address,
		usz pitch)
	{
		surface->set_format(format);
		int_invalidate_surface_contents(cmd, surface, address, pitch);
	}

	static
	void notify_surface_invalidated(const std::unique_ptr<gl::render_target>& surface)
	{
		if (!surface->old_contents.empty())
		{
			// TODO: Retire the deferred writes
			surface->clear_rw_barrier();
		}

		surface->release();
	}

	static
	void notify_surface_persist(const std::unique_ptr<gl::render_target>& /*surface*/)
	{}

	static
	void notify_surface_reused(const std::unique_ptr<gl::render_target>& surface)
	{
		surface->state_flags |= rsx::surface_state_flags::erase_bkgnd;
		surface->add_ref();
	}

	static
	bool int_surface_matches_properties(
		const std::unique_ptr<gl::render_target> &surface,
		gl::texture::internal_format format,
		usz width, usz height,
		rsx::surface_antialiasing antialias,
		bool check_refs = false)
	{
		if (check_refs && surface->has_refs())
			return false;

		return surface->get_internal_format() == format &&
			surface->get_spp() == get_format_sample_count(antialias) &&
			surface->matches_dimensions(static_cast<u16>(width), static_cast<u16>(height));
	}

	static
	bool surface_matches_properties(
		const std::unique_ptr<gl::render_target> &surface,
		rsx::surface_color_format format,
		usz width, usz height,
		rsx::surface_antialiasing antialias,
		bool check_refs=false)
	{
		const auto internal_fmt = rsx::internals::surface_color_format_to_gl(format).internal_format;
		return int_surface_matches_properties(surface, internal_fmt, width, height, antialias, check_refs);
	}

	static
	bool surface_matches_properties(
		const std::unique_ptr<gl::render_target> &surface,
		rsx::surface_depth_format2 format,
		usz width, usz height,
		rsx::surface_antialiasing antialias,
		bool check_refs = false)
	{
		const auto internal_fmt = rsx::internals::surface_depth_format_to_gl(format).internal_format;
		return int_surface_matches_properties(surface, internal_fmt, width, height, antialias, check_refs);
	}

	static
	void spill_buffer(std::unique_ptr<gl::buffer>& /*bo*/)
	{
		// TODO
	}

	static
	void unspill_buffer(std::unique_ptr<gl::buffer>& /*bo*/)
	{
		// TODO
	}

	static
	void write_render_target_to_memory(
		gl::command_context&,
		gl::buffer*,
		gl::render_target*,
		u64, u64, u64)
	{
		// TODO
	}

	template <int BlockSize>
	static
	gl::buffer* merge_bo_list(gl::command_context&, const std::vector<gl::buffer*>& /*list*/)
	{
		// TODO
		return nullptr;
	}

	template <typename T>
	static
	T* get(const std::unique_ptr<T> &in)
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

	std::vector<GLuint> trim(gl::command_context& cmd)
	{
		run_cleanup_internal(cmd, rsx::problem_severity::moderate, 256, [](gl::command_context&) {});

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
