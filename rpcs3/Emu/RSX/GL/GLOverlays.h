#pragma once

#include "util/types.hpp"
#include "../Overlays/overlays.h"
#include "GLTexture.h"
#include "Emu/RSX/rsx_utils.h"
#include <string>
#include <unordered_map>

namespace gl
{
	struct overlay_pass
	{
		std::string fs_src;
		std::string vs_src;

		gl::glsl::program program_handle;
		gl::glsl::shader vs;
		gl::glsl::shader fs;

		gl::fbo fbo;
		gl::sampler_state m_sampler;

		gl::vao m_vao;
		gl::buffer m_vertex_data_buffer;

		bool compiled = false;

		u32 num_drawable_elements = 4;
		GLenum primitives = GL_TRIANGLE_STRIP;
		GLenum input_filter = GL_NEAREST;

		struct saved_sampler_state
		{
			GLuint saved = GL_NONE;
			GLuint unit = 0;

			saved_sampler_state(GLuint _unit, const gl::sampler_state& sampler)
			{
				glActiveTexture(GL_TEXTURE0 + _unit);
				glGetIntegerv(GL_SAMPLER_BINDING, reinterpret_cast<GLint*>(&saved));

				unit = _unit;
				sampler.bind(_unit);
			}

			saved_sampler_state(const saved_sampler_state&) = delete;

			~saved_sampler_state()
			{
				glBindSampler(unit, saved);
			}
		};

		void create();
		void destroy();

		virtual void on_load() {}
		virtual void on_unload() {}

		virtual void bind_resources() {}
		virtual void cleanup_resources() {}

		template <typename T>
		void upload_vertex_data(T* data, u32 elements_count)
		{
			m_vertex_data_buffer.data(elements_count * sizeof(T), data);
		}

		virtual void emit_geometry();

		void run(const areau& region, GLuint target_texture, bool depth_target, bool use_blending = false);
	};

	struct ui_overlay_renderer : public overlay_pass
	{
		u32 num_elements = 0;
		std::vector<std::unique_ptr<gl::texture>> resources;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<gl::texture>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> temp_view_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture>> font_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> view_cache;
		rsx::overlays::primitive_type m_current_primitive_type = rsx::overlays::primitive_type::quad_list;

		ui_overlay_renderer();

		gl::texture_view* load_simple_image(rsx::overlays::image_info* desc, bool temp_resource, u32 owner_uid);

		void create();
		void destroy();

		void remove_temp_resources(u64 key);

		gl::texture_view* find_font(rsx::overlays::font* font);

		gl::texture_view* find_temp_image(rsx::overlays::image_info* desc, u32 owner_uid);

		void set_primitive_type(rsx::overlays::primitive_type type);

		void emit_geometry() override;

		void run(const areau& viewport, GLuint target, rsx::overlays::overlay& ui);
	};

	struct video_out_calibration_pass : public overlay_pass
	{
		video_out_calibration_pass();

		void run(const areau& viewport, const rsx::simple_array<GLuint>& source, f32 gamma, bool limited_rgb, bool _3d);
	};
}
