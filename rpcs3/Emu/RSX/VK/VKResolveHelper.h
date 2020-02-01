#pragma once

#include "VKHelpers.h"
#include "VKCompute.h"
#include "VKOverlays.h"

namespace vk
{
	struct cs_resolve_base : compute_task
	{
		vk::viewable_image* multisampled;
		vk::viewable_image* resolve;

		u32 cs_wave_x = 1;
		u32 cs_wave_y = 1;

		cs_resolve_base()
		{}

		virtual ~cs_resolve_base()
		{}

		void build(const std::string& kernel, const std::string& format_prefix, int direction)
		{
			create();

			// TODO: Tweak occupancy
			switch (optimal_group_size)
			{
			default:
			case 64:
				cs_wave_x = 8;
				cs_wave_y = 8;
				break;
			case 32:
				cs_wave_x = 8;
				cs_wave_y = 4;
				break;
			}

			const std::pair<std::string, std::string> syntax_replace[] =
			{
				{ "%wx", std::to_string(cs_wave_x) },
				{ "%wy", std::to_string(cs_wave_y) },
			};

			m_src =
			"#version 430\n"
			"layout(local_size_x=%wx, local_size_y=%wy, local_size_z=1) in;\n"
			"\n";

			m_src = fmt::replace_all(m_src, syntax_replace);

			if (direction == 0)
			{
				m_src +=
				"layout(set=0, binding=0, " + format_prefix + ") uniform readonly restrict image2DMS multisampled;\n"
				"layout(set=0, binding=1) uniform writeonly restrict image2D resolve;\n";
			}
			else
			{
				m_src +=
				"layout(set=0, binding=0) uniform writeonly restrict image2DMS multisampled;\n"
				"layout(set=0, binding=1, " + format_prefix + ") uniform readonly restrict image2D resolve;\n";
			}

			m_src +=
			"\n"
			"void main()\n"
			"{\n"
			"	ivec2 resolve_size = imageSize(resolve);\n"
			"	ivec2 aa_size = imageSize(multisampled);\n"
			"	ivec2 sample_count = resolve_size / aa_size;\n"
			"\n"
			"	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, uvec2(resolve_size)))) return;"
			"\n"
			"	ivec2 resolve_coords = ivec2(gl_GlobalInvocationID.xy);\n"
			"	ivec2 aa_coords = resolve_coords / sample_count;\n"
			"	ivec2 sample_loc = ivec2(resolve_coords % sample_count);\n"
			"	int sample_index = sample_loc.x + (sample_loc.y * sample_count.y);\n"
			+ kernel +
			"}\n";

			rsx_log.notice("Compute shader:\n%s", m_src);
		}

		std::vector<std::pair<VkDescriptorType, u8>> get_descriptor_layout() override
		{
			return
			{
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 }
			};
		}

		void declare_inputs() override
		{
			std::vector<vk::glsl::program_input> inputs =
			{
				{
					::glsl::program_domain::glsl_compute_program,
					vk::glsl::program_input_type::input_type_texture,
					{}, {},
					0,
					"multisampled"
				},
				{
					::glsl::program_domain::glsl_compute_program,
					vk::glsl::program_input_type::input_type_texture,
					{}, {},
					1,
					"resolve"
				}
			};

			m_program->load_uniforms(inputs);
		}

		void bind_resources() override
		{
			auto msaa_view = multisampled->get_view(VK_REMAP_VIEW_MULTISAMPLED, rsx::default_remap_vector);
			auto resolved_view = resolve->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector);
			m_program->bind_uniform({ VK_NULL_HANDLE, msaa_view->value, multisampled->current_layout }, "multisampled", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_descriptor_set);
			m_program->bind_uniform({ VK_NULL_HANDLE, resolved_view->value, resolve->current_layout }, "resolve", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_descriptor_set);
		}

		void run(VkCommandBuffer cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image)
		{
			verify(HERE), msaa_image->samples() > 1, resolve_image->samples() == 1;

			multisampled = msaa_image;
			resolve = resolve_image;

			const u32 invocations_x = align(resolve_image->width(), cs_wave_x) / cs_wave_x;
			const u32 invocations_y = align(resolve_image->height(), cs_wave_y) / cs_wave_y;

			compute_task::run(cmd, invocations_x, invocations_y, 1);
		}
	};

	struct cs_resolve_task : cs_resolve_base
	{
		cs_resolve_task(const std::string& format_prefix, bool bgra_swap = false)
		{
			// Allow rgba->bgra transformation for old GeForce cards
			const std::string swizzle = bgra_swap? ".bgra" : "";

			std::string kernel =
			"	vec4 aa_sample = imageLoad(multisampled, aa_coords, sample_index);\n"
			"	imageStore(resolve, resolve_coords, aa_sample" + swizzle + ");\n";

			build(kernel, format_prefix, 0);
		}
	};

	struct cs_unresolve_task : cs_resolve_base
	{
		cs_unresolve_task(const std::string& format_prefix, bool bgra_swap = false)
		{
			// Allow rgba->bgra transformation for old GeForce cards
			const std::string swizzle = bgra_swap? ".bgra" : "";

			std::string kernel =
			"	vec4 resolved_sample = imageLoad(resolve, resolve_coords);\n"
			"	imageStore(multisampled, aa_coords, sample_index, resolved_sample" + swizzle + ");\n";

			build(kernel, format_prefix, 1);
		}
	};

	struct depth_resolve_base : public overlay_pass
	{
		u8 samples_x = 1;
		u8 samples_y = 1;
		s32 static_parameters[4];

		depth_resolve_base()
		{
			renderpass_config.set_depth_mask(true);
			renderpass_config.enable_depth_test(VK_COMPARE_OP_ALWAYS);
		}

		void build(const std::string& kernel, const std::string& extensions, const std::vector<const char*>& inputs)
		{
			vs_src =
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"}\n";

			fs_src =
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n";
			fs_src += extensions +
				"\n"
				"layout(push_constant) uniform static_data{ ivec4 regs[1]; };\n";

				int binding = 1;
				for (const auto& input : inputs)
				{
					fs_src += "layout(set=0, binding=" + std::to_string(binding++) + ") uniform " + input + ";\n";
				}

				fs_src +=
				"//layout(pixel_center_integer) in vec4 gl_FragCoord;\n"
				"\n"
				"void main()\n"
				"{\n";
					fs_src += kernel +
				"}\n";

			rsx_log.notice("Resolve shader:\n%s", fs_src);
		}

		std::vector<VkPushConstantRange> get_push_constants() override
		{
			VkPushConstantRange constant;
			constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			constant.offset = 0;
			constant.size = 16;

			return { constant };
		}

		void update_uniforms(vk::command_buffer& cmd, vk::glsl::program* /*program*/) override
		{
			vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8, static_parameters);
		}

		void update_sample_configuration(vk::image* msaa_image)
		{
			switch (msaa_image->samples())
			{
			case 1:
				fmt::throw_exception("MSAA input not multisampled!" HERE);
			case 2:
				samples_x = 2;
				samples_y = 1;
				break;
			case 4:
				samples_x = samples_y = 2;
				break;
			default:
				fmt::throw_exception("Unsupported sample count %d" HERE, msaa_image->samples());
			}

			static_parameters[0] = samples_x;
			static_parameters[1] = samples_y;
		}
	};

	struct depthonly_resolve : depth_resolve_base
	{
		depthonly_resolve()
		{
			build(
				"	ivec2 out_coord = ivec2(gl_FragCoord.xy);\n"
				"	ivec2 in_coord = (out_coord / regs[0].xy);\n"
				"	ivec2 sample_loc = out_coord % regs[0].xy;\n"
				"	int sample_index = sample_loc.x + (sample_loc.y * regs[0].y);\n"
				"	float frag_depth = texelFetch(fs0, in_coord, sample_index).x;\n"
				"	gl_FragDepth = frag_depth;\n",
				"",
				{ "sampler2DMS fs0" });
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto src_view = msaa_image->get_view(VK_REMAP_VIEW_MULTISAMPLED, rsx::default_remap_vector);

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
			build(
				"	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);\n"
				"	pixel_coord *= regs[0].xy;\n"
				"	pixel_coord.x += (gl_SampleID % regs[0].x);\n"
				"	pixel_coord.y += (gl_SampleID / regs[0].x);\n"
				"	float frag_depth = texelFetch(fs0, pixel_coord, 0).x;\n"
				"	gl_FragDepth = frag_depth;\n",
				"",
				{ "sampler2D fs0" });
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto src_view = resolve_image->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector);

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

			build(
				"	ivec2 out_coord = ivec2(gl_FragCoord.xy);\n"
				"	ivec2 in_coord = (out_coord / regs[0].xy);\n"
				"	ivec2 sample_loc = out_coord % regs[0].xy;\n"
				"	int sample_index = sample_loc.x + (sample_loc.y * regs[0].y);\n"
				"	uint frag_stencil = texelFetch(fs0, in_coord, sample_index).x;\n"
				"	if ((frag_stencil & uint(regs[0].z)) == 0) discard;\n",
				"",
				{"usampler2DMS fs0"});
		}

		void get_dynamic_state_entries(VkDynamicState* state_descriptors, VkPipelineDynamicStateCreateInfo& info) override
		{
			state_descriptors[info.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
		}

		void emit_geometry(vk::command_buffer& cmd) override
		{
			vkCmdClearAttachments(cmd, 1, &clear_info, 1, &region);

			for (s32 write_mask = 0x1; write_mask <= 0x80; write_mask <<= 1)
			{
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, write_mask);
				vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 8, 4, &write_mask);

				overlay_pass::emit_geometry(cmd);
			}
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto stencil_view = msaa_image->get_view(VK_REMAP_VIEW_MULTISAMPLED, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

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
		VkClearRect region{};
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
			region.baseArrayLayer = 0;
			region.layerCount = 1;

			build(
				"	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);\n"
				"	pixel_coord *= regs[0].xy;\n"
				"	pixel_coord.x += (gl_SampleID % regs[0].x);\n"
				"	pixel_coord.y += (gl_SampleID / regs[0].x);\n"
				"	uint frag_stencil = texelFetch(fs0, pixel_coord, 0).x;\n"
				"	if ((frag_stencil & uint(regs[0].z)) == 0) discard;\n",
				"",
				{ "usampler2D fs0" });
		}

		void get_dynamic_state_entries(VkDynamicState* state_descriptors, VkPipelineDynamicStateCreateInfo& info) override
		{
			state_descriptors[info.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
		}

		void emit_geometry(vk::command_buffer& cmd) override
		{
			vkCmdClearAttachments(cmd, 1, &clear_info, 1, &region);

			for (s32 write_mask = 0x1; write_mask <= 0x80; write_mask <<= 1)
			{
				vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FRONT_AND_BACK, write_mask);
				vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 8, 4, &write_mask);

				overlay_pass::emit_geometry(cmd);
			}
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto stencil_view = resolve_image->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

			region.rect.extent.width = resolve_image->width();
			region.rect.extent.height = resolve_image->height();

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

			build(
				"	ivec2 out_coord = ivec2(gl_FragCoord.xy);\n"
				"	ivec2 in_coord = (out_coord / regs[0].xy);\n"
				"	ivec2 sample_loc = out_coord % ivec2(regs[0].xy);\n"
				"	int sample_index = sample_loc.x + (sample_loc.y * regs[0].y);\n"
				"	float frag_depth = texelFetch(fs0, in_coord, sample_index).x;\n"
				"	uint frag_stencil = texelFetch(fs1, in_coord, sample_index).x;\n"
				"	gl_FragDepth = frag_depth;\n"
				"	gl_FragStencilRefARB = int(frag_stencil);\n",

				"#extension GL_ARB_shader_stencil_export : enable\n",

				{ "sampler2DMS fs0", "usampler2DMS fs1" });
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			update_sample_configuration(msaa_image);
			auto depth_view = msaa_image->get_view(VK_REMAP_VIEW_MULTISAMPLED, rsx::default_remap_vector, VK_IMAGE_ASPECT_DEPTH_BIT);
			auto stencil_view = msaa_image->get_view(VK_REMAP_VIEW_MULTISAMPLED, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

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

			build(
				"	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);\n"
				"	pixel_coord *= regs[0].xy;\n"
				"	pixel_coord.x += (gl_SampleID % regs[0].x);\n"
				"	pixel_coord.y += (gl_SampleID / regs[0].x);\n"
				"	float frag_depth = texelFetch(fs0, pixel_coord, 0).x;\n"
				"	uint frag_stencil = texelFetch(fs1, pixel_coord, 0).x;\n"
				"	gl_FragDepth = frag_depth;\n"
				"	gl_FragStencilRefARB = int(frag_stencil);\n",

				"#extension GL_ARB_shader_stencil_export : enable\n",

				{ "sampler2D fs0", "usampler2D fs1" });
		}

		void run(vk::command_buffer& cmd, vk::viewable_image* msaa_image, vk::viewable_image* resolve_image, VkRenderPass render_pass)
		{
			renderpass_config.set_multisample_state(msaa_image->samples(), 0xFFFF, true, false, false);
			renderpass_config.set_multisample_shading_rate(1.f);
			update_sample_configuration(msaa_image);

			auto depth_view = resolve_image->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector, VK_IMAGE_ASPECT_DEPTH_BIT);
			auto stencil_view = resolve_image->get_view(VK_REMAP_IDENTITY, rsx::default_remap_vector, VK_IMAGE_ASPECT_STENCIL_BIT);

			overlay_pass::run(
				cmd,
				{ 0, 0, msaa_image->width(), msaa_image->height() },
				msaa_image, { depth_view, stencil_view },
				render_pass);
		}
	};

	void resolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void unresolve_image(vk::command_buffer& cmd, vk::viewable_image* dst, vk::viewable_image* src);
	void reset_resolve_resources();
	void clear_resolve_helpers();
}
