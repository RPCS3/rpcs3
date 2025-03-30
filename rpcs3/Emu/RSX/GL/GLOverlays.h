#pragma once

#include "Emu/system_config_types.h"
#include "Emu/IdManager.h"
#include "util/types.hpp"
#include "../Common/simple_array.hpp"
#include "../Overlays/overlays.h"
#include "GLTexture.h"

#include "glutils/fbo.h"
#include "glutils/program.h"
#include "glutils/vao.hpp"

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
		gl::filter m_input_filter = gl::filter::nearest;

		u32 m_write_aspect_mask = gl::image_aspect::color | gl::image_aspect::depth;
		bool enable_depth_writes = false;
		bool enable_stencil_writes = false;

		virtual ~overlay_pass() = default;

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

		virtual void emit_geometry(gl::command_context& cmd);

		void run(gl::command_context& cmd, const areau& region, GLuint target_texture, GLuint image_aspect_bits, bool enable_blending = false);
	};

	struct ui_overlay_renderer final : public overlay_pass
	{
		u32 num_elements = 0;
		std::vector<std::unique_ptr<gl::texture>> resources;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<gl::texture>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> temp_view_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture>> font_cache;
		std::unordered_map<u64, std::unique_ptr<gl::texture_view>> view_cache;
		rsx::overlays::primitive_type m_current_primitive_type = rsx::overlays::primitive_type::quad_list;

		ui_overlay_renderer();

		gl::texture_view* load_simple_image(rsx::overlays::image_info_base* desc, bool temp_resource, u32 owner_uid);

		void create();
		void destroy();

		void remove_temp_resources(u64 key);

		gl::texture_view* find_font(rsx::overlays::font* font);

		gl::texture_view* find_temp_image(rsx::overlays::image_info_base* desc, u32 owner_uid);

		void set_primitive_type(rsx::overlays::primitive_type type);

		void emit_geometry(gl::command_context& cmd) override;

		void run(gl::command_context& cmd, const areau& viewport, GLuint target, rsx::overlays::overlay& ui);
	};

	struct video_out_calibration_pass final : public overlay_pass
	{
		video_out_calibration_pass();

		void run(gl::command_context& cmd, const areau& viewport, const rsx::simple_array<GLuint>& source, f32 gamma, bool limited_rgb, stereo_render_mode_options stereo_mode, gl::filter input_filter);
	};

	struct rp_ssbo_to_generic_texture final : public overlay_pass
	{
		rp_ssbo_to_generic_texture();
		void run(gl::command_context& cmd, const buffer* src, texture* dst, const u32 src_offset, const coordu& dst_region, const pixel_buffer_layout& layout);
		void run(gl::command_context& cmd, const buffer* src, const texture_view* dst, const u32 src_offset, const coordu& dst_region, const pixel_buffer_layout& layout);
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<gl::overlay_pass>> g_overlay_passes;

	template<class T>
	T* get_overlay_pass()
	{
		u32 index = stx::typeindex<id_manager::typeinfo, T>();
		auto &e = g_overlay_passes[index];

		if (!e)
		{
			e = std::make_unique<T>();
			e->create();
		}

		return static_cast<T*>(e.get());
	}

	void destroy_overlay_passes();
}
