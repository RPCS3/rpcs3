#pragma once

#include "VKCompute.h"
#include "VKOverlays.h"

#include "vkutils/image.h"

namespace vk
{
	struct cs_resolve_base : compute_task
	{
		vk::viewable_image* multisampled = nullptr;
		vk::viewable_image* resolve = nullptr;

		u32 cs_wave_x = 1;
		u32 cs_wave_y = 1;

		cs_resolve_base()
		{
			ssbo_count = 0;
		}

		virtual ~cs_resolve_base()
		{}

		void build(const std::string& format_prefix, bool unresolve, bool bgra_swap);

		std::vector<glsl::program_input> get_inputs() override
		{
			std::vector<vk::glsl::program_input> inputs =
			{
				glsl::program_input::make(
					::glsl::program_domain::glsl_compute_program,
					"multisampled",
					glsl::input_type_storage_texture,
					0,
					0
				),

				glsl::program_input::make(
					::glsl::program_domain::glsl_compute_program,
					"resolve",
					glsl::input_type_storage_texture,
					0,
					1
				),
			};

			auto result = compute_task::get_inputs();
			result.insert(result.end(), inputs.begin(), inputs.end());
			return result;
		}

		void bind_resources(const vk::command_buffer& /*cmd*/) override
		{
			auto msaa_view = multisampled->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_VIEW_MULTISAMPLED));
			auto resolved_view = resolve->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY));
			m_program->bind_uniform({ VK_NULL_HANDLE, msaa_view->value, multisampled->current_layout }, 0, 0);
			m_program->bind_uniform({ VK_NULL_HANDLE, resolved_view->value, resolve->current_layout }, 0, 1);
		}

		void run(const vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image)
		{
			ensure(msaa_image->samples() > 1);
			ensure(resolve_image->samples() == 1);

			multisampled = msaa_image;
			resolve = resolve_image;

			const u32 invocations_x = utils::align(resolve_image->width(), cs_wave_x) / cs_wave_x;
			const u32 invocations_y = utils::align(resolve_image->height(), cs_wave_y) / cs_wave_y;

			compute_task::run(cmd, invocations_x, invocations_y, 1);
		}
	};

	struct cs_resolve_task : cs_resolve_base
	{
		cs_resolve_task(const std::string& format_prefix, bool bgra_swap = false)
		{
			// BGRA-swap flag is a workaround to swap channels for old GeForce cards with broken compute image handling
			build(format_prefix, false, bgra_swap);
		}
	};

	struct cs_unresolve_task : cs_resolve_base
	{
		cs_unresolve_task(const std::string& format_prefix, bool bgra_swap = false)
		{
			// BGRA-swap flag is a workaround to swap channels for old GeForce cards with broken compute image handling
			build(format_prefix, true, bgra_swap);
		}
	};

	struct depth_resolve_base : public overlay_pass
	{
		u8 samples_x = 1;
		u8 samples_y = 1;
		s32 static_parameters[4];
		s32 static_parameters_width = 2;

		depth_resolve_base()
		{
			renderpass_config.set_depth_mask(true);
			renderpass_config.enable_depth_test(VK_COMPARE_OP_ALWAYS);

			// Depth-stencil buffers are almost never filterable, and we do not need it here (1:1 mapping)
			m_sampler_filter = VK_FILTER_NEAREST;

			// Do not use UBOs
			m_num_uniform_buffers = 0;
		}

		void build(bool resolve_depth, bool resolve_stencil, bool unresolve);

		std::vector<glsl::program_input> get_fragment_inputs() override
		{
			auto result = overlay_pass::get_fragment_inputs();
			result.push_back(glsl::program_input::make(
				::glsl::glsl_fragment_program,
				"push_constants",
				glsl::input_type_push_constant,
				0,
				umax,
				glsl::push_constant_ref{ .size = 16 }
			));
			return result;
		}

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* program) override
		{
			vkCmdPushConstants(cmd, program->layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, static_parameters_width * 4, static_parameters);
		}

		void update_sample_configuration(vk::image* msaa_image)
		{
			switch (msaa_image->samples())
			{
			case 1:
				fmt::throw_exception("MSAA input not multisampled!");
			case 2:
				samples_x = 2;
				samples_y = 1;
				break;
			case 4:
				samples_x = samples_y = 2;
				break;
			default:
				fmt::throw_exception("Unsupported sample count %d", msaa_image->samples());
			}

			static_parameters[0] = samples_x;
			static_parameters[1] = samples_y;
		}
	};

	struct depthonly_resolve : depth_resolve_base
	{
		depthonly_resolve()
		{
			build(true, false, false);
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto src_view = msaa_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_VIEW_MULTISAMPLED));

			overlay_pass::run(
				cmd,
				{ 0, 0, resolve_image->width(), resolve_image->height() },
				resolve_image, src_view,
				render_pass);
		}
	};

	struct depthonly_unresolve : depth_resolve_base
	{
		depthonly_unresolve()
		{
			build(true, false, true);
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto src_view = resolve_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY));

			overlay_pass::run(
				cmd,
				{ 0, 0, msaa_image->width(), msaa_image->height() },
				msaa_image, src_view,
				render_pass);
		}
	};

	struct stencilonly_resolve : depth_resolve_base
	{
		VkClearRect region{};
		VkClearAttachment clear_info{};

		stencilonly_resolve()
		{
			renderpass_config.enable_stencil_test(
				VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE,  // Always replace
				VK_COMPARE_OP_ALWAYS,                                                 // Always pass
				0xFF,                                                                 // Full write-through
				0xFF);                                                                // Write active bit

			renderpass_config.set_stencil_mask(0xFF);
			renderpass_config.set_depth_mask(false);

			clear_info.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			region.baseArrayLayer = 0;
			region.layerCount = 1;

			static_parameters_width = 3;

			build(false, true, false);
		}

		void get_dynamic_state_entries(std::vector<VkDynamicState>& state_descriptors) override
		{
			state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
		}

		void emit_geometry(vk::command_buffer& cmd, glsl::program* program) override
		{
			vkCmdClearAttachments(cmd, 1, &clear_info, 1, &region);

			for (s32 write_mask = 0x1; write_mask <= 0x80; write_mask <<= 1)
			{
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, write_mask);
				vkCmdPushConstants(cmd, program->layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, 4, &write_mask);

				overlay_pass::emit_geometry(cmd, program);
			}
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto stencil_view = msaa_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_VIEW_MULTISAMPLED), VK_IMAGE_ASPECT_STENCIL_BIT);

			region.rect.extent.width = resolve_image->width();
			region.rect.extent.height = resolve_image->height();

			overlay_pass::run(
				cmd,
				{ 0, 0, resolve_image->width(), resolve_image->height() },
				resolve_image, stencil_view,
				render_pass);
		}
	};

	struct stencilonly_unresolve : depth_resolve_base
	{
		VkClearRect clear_region{};
		VkClearAttachment clear_info{};

		stencilonly_unresolve()
		{
			renderpass_config.enable_stencil_test(
				VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE,  // Always replace
				VK_COMPARE_OP_ALWAYS,                                                 // Always pass
				0xFF,                                                                 // Full write-through
				0xFF);                                                                // Write active bit

			renderpass_config.set_stencil_mask(0xFF);
			renderpass_config.set_depth_mask(false);

			clear_info.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			clear_region.baseArrayLayer = 0;
			clear_region.layerCount = 1;

			static_parameters_width = 3;

			build(false, true, true);
		}

		void get_dynamic_state_entries(std::vector<VkDynamicState>& state_descriptors) override
		{
			state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
		}

		void emit_geometry(vk::command_buffer& cmd, glsl::program* program) override
		{
			vkCmdClearAttachments(cmd, 1, &clear_info, 1, &clear_region);

			for (s32 write_mask = 0x1; write_mask <= 0x80; write_mask <<= 1)
			{
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, write_mask);
				vkCmdPushConstants(cmd, program->layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 8, 4, &write_mask);

				overlay_pass::emit_geometry(cmd, program);
			}
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto stencil_view = resolve_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY), VK_IMAGE_ASPECT_STENCIL_BIT);

			clear_region.rect.extent.width = msaa_image->width();
			clear_region.rect.extent.height = msaa_image->height();

			overlay_pass::run(
				cmd,
				{ 0, 0, msaa_image->width(), msaa_image->height() },
				msaa_image, stencil_view,
				render_pass);
		}
	};

	struct depthstencil_resolve_EXT : depth_resolve_base
	{
		depthstencil_resolve_EXT()
		{
			renderpass_config.enable_stencil_test(
				VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE,  // Always replace
				VK_COMPARE_OP_ALWAYS,                                                 // Always pass
				0xFF,                                                                 // Full write-through
				0);                                                                   // Unused

			renderpass_config.set_stencil_mask(0xFF);
			m_num_usable_samplers = 2;

			build(true, true, false);
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto depth_view = msaa_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_VIEW_MULTISAMPLED), VK_IMAGE_ASPECT_DEPTH_BIT);
			auto stencil_view = msaa_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_VIEW_MULTISAMPLED), VK_IMAGE_ASPECT_STENCIL_BIT);

			overlay_pass::run(
				cmd,
				{ 0, 0, resolve_image->width(), resolve_image->height() },
				resolve_image, { depth_view, stencil_view },
				render_pass);
		}
	};

	struct depthstencil_unresolve_EXT : depth_resolve_base
	{
		depthstencil_unresolve_EXT()
		{
			renderpass_config.enable_stencil_test(
				VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE,  // Always replace
				VK_COMPARE_OP_ALWAYS,                                                 // Always pass
				0xFF,                                                                 // Full write-through
				0);                                                                   // Unused

			renderpass_config.set_stencil_mask(0xFF);
			m_num_usable_samplers = 2;

			build(true, true, true);
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto depth_view = resolve_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY), VK_IMAGE_ASPECT_DEPTH_BIT);
			auto stencil_view = resolve_image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY), VK_IMAGE_ASPECT_STENCIL_BIT);

			overlay_pass::run(
				cmd,
				{ 0, 0, msaa_image->width(), msaa_image->height() },
				msaa_image, { depth_view, stencil_view },
				render_pass);
		}
	};

	//void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	//void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void reset_resolve_resources();
	void clear_resolve_helpers();
}
