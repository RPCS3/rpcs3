#pragma once

#include "../Overlays/overlay_controls.h"
#include "VKProgramPipeline.h"
#include "VKHelpers.h"

#include "vkutils/data_heap.h"
#include "vkutils/descriptors.hpp"
#include "vkutils/graphics_pipeline_state.hpp"

#include "Emu/IdManager.h"

#include <unordered_map>

namespace rsx
{
	namespace overlays
	{
		struct overlay;
	}
}

namespace vk
{
	struct framebuffer;
	struct sampler;
	struct image_view;
	class image;
	class viewable_image;
	class command_buffer;
	class render_target;

	namespace glsl
	{
		class program;
	}

	// TODO: Refactor text print class to inherit from this base class
	struct overlay_pass
	{
		vk::glsl::shader m_vertex_shader;
		vk::glsl::shader m_fragment_shader;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		VkFilter m_sampler_filter = VK_FILTER_LINEAR;
		u32 m_num_usable_samplers = 1;

		std::unordered_map<u64, std::unique_ptr<vk::glsl::program>> m_program_cache;
		std::unique_ptr<vk::sampler> m_sampler;
		std::unique_ptr<vk::framebuffer> m_draw_fbo;
		vk::data_heap m_vao;
		vk::data_heap m_ubo;
		const vk::render_device* m_device = nullptr;

		std::string vs_src;
		std::string fs_src;

		graphics_pipeline_state renderpass_config;
		bool multi_primitive = false;

		bool initialized = false;
		bool compiled = false;

		u32 num_drawable_elements = 4;
		u32 first_vertex = 0;

		u32 m_ubo_length = 128;
		u32 m_ubo_offset = 0;
		u32 m_vao_offset = 0;

		overlay_pass();
		~overlay_pass();

		u64 get_pipeline_key(VkRenderPass pass);

		void check_heap();

		void init_descriptors();

		virtual void update_uniforms(vk::command_buffer& /*cmd*/, vk::glsl::program* /*program*/) {}

		virtual std::vector<vk::glsl::program_input> get_vertex_inputs();
		virtual std::vector<vk::glsl::program_input> get_fragment_inputs();

		virtual void get_dynamic_state_entries(std::vector<VkDynamicState>& /*state_descriptors*/) {}

		virtual std::vector<VkPushConstantRange> get_push_constants()
		{
			return {};
		}

		template <typename T>
		void upload_vertex_data(T* data, u32 count)
		{
			check_heap();

			const auto size = count * sizeof(T);
			m_vao_offset = static_cast<u32>(m_vao.alloc<16>(size));
			auto dst = m_vao.map(m_vao_offset, size);
			std::memcpy(dst, data, size);
			m_vao.unmap();
		}

		vk::glsl::program* build_pipeline(u64 storage_key, VkRenderPass render_pass);

		void load_program(vk::command_buffer& cmd, VkRenderPass pass, const std::vector<vk::image_view*>& src);

		virtual void create(const vk::render_device& dev);
		virtual void destroy();

		void free_resources();

		vk::framebuffer* get_framebuffer(vk::image* target, VkRenderPass render_pass);

		virtual void emit_geometry(vk::command_buffer& cmd);

		virtual void set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h);

		void run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* fbo, const std::vector<vk::image_view*>& src, VkRenderPass render_pass);
		void run(vk::command_buffer& cmd, const areau& viewport, vk::image* target, const std::vector<vk::image_view*>& src, VkRenderPass render_pass);
		void run(vk::command_buffer& cmd, const areau& viewport, vk::image* target, vk::image_view* src, VkRenderPass render_pass);
	};

	struct ui_overlay_renderer : public overlay_pass
	{
		f32 m_time = 0.f;
		f32 m_blur_strength = 0.f;
		color4f m_scale_offset;
		color4f m_color;
		bool m_pulse_glow = false;
		bool m_skip_texture_read = false;
		bool m_clip_enabled = false;
		int  m_texture_type;
		areaf m_clip_region;
		coordf m_viewport;

		std::vector<std::unique_ptr<vk::image>> resources;
		std::unordered_map<u64, std::unique_ptr<vk::image>> font_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> view_cache;
		std::unordered_map<u64, std::pair<u32, std::unique_ptr<vk::image>>> temp_image_cache;
		std::unordered_map<u64, std::unique_ptr<vk::image_view>> temp_view_cache;
		rsx::overlays::primitive_type m_current_primitive_type = rsx::overlays::primitive_type::quad_list;

		ui_overlay_renderer();

		vk::image_view* upload_simple_texture(vk::render_device& dev, vk::command_buffer& cmd,
			vk::data_heap& upload_heap, u64 key, u32 w, u32 h, u32 layers, bool font, bool temp, void* pixel_src, u32 owner_uid);

		void init(vk::command_buffer& cmd, vk::data_heap& upload_heap);

		void destroy() override;

		void remove_temp_resources(u32 key);

		vk::image_view* find_font(rsx::overlays::font* font, vk::command_buffer& cmd, vk::data_heap& upload_heap);
		vk::image_view* find_temp_image(rsx::overlays::image_info* desc, vk::command_buffer& cmd, vk::data_heap& upload_heap, u32 owner_uid);

		void update_uniforms(vk::command_buffer& /*cmd*/, vk::glsl::program* /*program*/) override;

		void set_primitive_type(rsx::overlays::primitive_type type);

		void emit_geometry(vk::command_buffer& cmd) override;

		void run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* target, VkRenderPass render_pass,
				vk::data_heap& upload_heap, rsx::overlays::overlay& ui);
	};

	struct attachment_clear_pass : public overlay_pass
	{
		color4f clear_color = { 0.f, 0.f, 0.f, 0.f };
		color4f colormask = { 1.f, 1.f, 1.f, 1.f };
		VkRect2D region = {};

		attachment_clear_pass();

		std::vector<VkPushConstantRange> get_push_constants() override;

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/) override;

		void set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h) override;

		bool update_config(u32 clearmask, color4f color);

		void run(vk::command_buffer& cmd, vk::render_target* target, VkRect2D rect, VkRenderPass render_pass);
	};

	struct stencil_clear_pass : public overlay_pass
	{
		VkRect2D region = {};

		stencil_clear_pass();

		void set_up_viewport(vk::command_buffer& cmd, u32 x, u32 y, u32 w, u32 h) override;

		void run(vk::command_buffer& cmd, vk::render_target* target, VkRect2D rect, u32 stencil_clear, u32 stencil_write_mask, VkRenderPass render_pass);
	};

	struct video_out_calibration_pass : public overlay_pass
	{
		union config_t
		{
			struct
			{
				float gamma;
				int   limit_range;
				int   stereo;
				int   stereo_image_count;
			};

			float data[4];
		}
		config;

		video_out_calibration_pass();

		std::vector<VkPushConstantRange> get_push_constants() override;

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/) override;

		void run(vk::command_buffer& cmd, const areau& viewport, vk::framebuffer* target,
			const rsx::simple_array<vk::viewable_image*>& src, f32 gamma, bool limited_rgb, bool _3d, VkRenderPass render_pass);
	};

	// TODO: Replace with a proper manager
	extern std::unordered_map<u32, std::unique_ptr<vk::overlay_pass>> g_overlay_passes;

	template<class T>
	T* get_overlay_pass()
	{
		u32 index = id_manager::typeinfo::get_index<T>();
		auto& e = g_overlay_passes[index];

		if (!e)
		{
			e = std::make_unique<T>();
			e->create(*vk::get_current_renderer());
		}

		return static_cast<T*>(e.get());
	}

	void reset_overlay_passes();
}
