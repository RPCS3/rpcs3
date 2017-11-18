#pragma once
#include "VKHelpers.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKRenderTargets.h"

namespace vk
{
	//TODO: Refactor text print class to inherit from this base class
	struct overlay_pass
	{
		VKVertexProgram m_vertex_shader;
		VKFragmentProgram m_fragment_shader;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		std::unordered_map<VkRenderPass, std::unique_ptr<vk::glsl::program>> m_program_cache;
		std::unique_ptr<vk::sampler> m_sampler;
		std::unique_ptr<vk::framebuffer> m_draw_fbo;
		vk::render_device* m_device = nullptr;

		std::string vs_src;
		std::string fs_src;

		struct
		{
			int color_attachments = 0;
			bool write_color = true;
			bool write_depth = true;
			bool no_depth_test = true;
		}
		renderpass_config;

		bool initialized = false;
		bool compiled = false;

		void init_descriptors()
		{
			VkDescriptorPoolSize descriptor_pools[1] =
			{
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 120 },
			};

			//Reserve descriptor pools
			m_descriptor_pool.create(*m_device, descriptor_pools, 1);

			VkDescriptorSetLayoutBinding bindings[1] = {};

			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[0].binding = 0;

			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings;
			infos.bindingCount = 1;

			CHECK_RESULT(vkCreateDescriptorSetLayout(*m_device, &infos, nullptr, &m_descriptor_layout));

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &m_descriptor_layout;

			CHECK_RESULT(vkCreatePipelineLayout(*m_device, &layout_info, nullptr, &m_pipeline_layout));
		}

		vk::glsl::program* build_pipeline(VkRenderPass render_pass)
		{
			if (!compiled)
			{
				m_vertex_shader.shader = vs_src;
				m_vertex_shader.Compile();

				m_fragment_shader.shader = fs_src;
				m_fragment_shader.Compile();

				compiled = true;
			}

			VkPipelineShaderStageCreateInfo shader_stages[2] = {};
			shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stages[0].module = m_vertex_shader.handle;
			shader_stages[0].pName = "main";

			shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stages[1].module = m_fragment_shader.handle;
			shader_stages[1].pName = "main";

			VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
			VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
			dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
			dynamic_state_info.pDynamicStates = dynamic_state_descriptors;

			VkPipelineVertexInputStateCreateInfo vi = {};
			vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			VkPipelineViewportStateCreateInfo vp = {};
			vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vp.scissorCount = 1;
			vp.viewportCount = 1;

			VkPipelineMultisampleStateCreateInfo ms = {};
			ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			ms.pSampleMask = NULL;
			ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineInputAssemblyStateCreateInfo ia = {};
			ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

			VkPipelineRasterizationStateCreateInfo rs = {};
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.lineWidth = 1.f;
			rs.polygonMode = VK_POLYGON_MODE_FILL;

			VkPipelineColorBlendAttachmentState att = {};
			if (renderpass_config.write_color)
				att.colorWriteMask = 0xf;

			VkPipelineColorBlendStateCreateInfo cs = {};
			cs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			cs.attachmentCount = renderpass_config.color_attachments;
			cs.pAttachments = &att;

			VkPipelineDepthStencilStateCreateInfo ds = {};
			ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			ds.depthWriteEnable = renderpass_config.write_depth? VK_TRUE: VK_FALSE;
			ds.depthTestEnable = VK_TRUE;
			ds.depthCompareOp = VK_COMPARE_OP_ALWAYS;

			VkPipeline pipeline;
			VkGraphicsPipelineCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			info.pVertexInputState = &vi;
			info.pInputAssemblyState = &ia;
			info.pRasterizationState = &rs;
			info.pColorBlendState = &cs;
			info.pMultisampleState = &ms;
			info.pViewportState = &vp;
			info.pDepthStencilState = &ds;
			info.stageCount = 2;
			info.pStages = shader_stages;
			info.pDynamicState = &dynamic_state_info;
			info.layout = m_pipeline_layout;
			info.basePipelineIndex = -1;
			info.basePipelineHandle = VK_NULL_HANDLE;
			info.renderPass = render_pass;

			CHECK_RESULT(vkCreateGraphicsPipelines(*m_device, nullptr, 1, &info, NULL, &pipeline));

			const std::vector<vk::glsl::program_input> unused;
			std::vector<vk::glsl::program_input> fs_inputs;
			fs_inputs.push_back({ ::glsl::program_domain::glsl_fragment_program, vk::glsl::program_input_type::input_type_texture, {}, {}, 0, "fs0"});
			auto program = std::make_unique<vk::glsl::program>(*m_device, pipeline, unused, fs_inputs);
			auto result = program.get();
			m_program_cache[render_pass] = std::move(program);

			return result;
		}

		void load_program(vk::command_buffer cmd, VkRenderPass pass, vk::image_view *src)
		{
			vk::glsl::program *program = nullptr;
			auto found = m_program_cache.find(pass);
			if (found != m_program_cache.end())
				program = found->second.get();
			else
				program = build_pipeline(pass);

			verify(HERE), m_used_descriptors < 120;

			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.descriptorPool = m_descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &m_descriptor_layout;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets(*m_device, &alloc_info, &m_descriptor_set));
			m_used_descriptors++;

			if (!m_sampler)
			{
				m_sampler = std::make_unique<vk::sampler>(*m_device,
					VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					VK_FALSE, 0.f, 1.f, 0.f, 0.f, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
			}

			VkDescriptorImageInfo info = { m_sampler->value, src->value, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			program->bind_uniform(info, "fs0", m_descriptor_set);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);
		}

		void create(vk::render_device &dev)
		{
			if (!initialized)
			{
				m_device = &dev;
				init_descriptors();

				initialized = true;
			}
		}

		void destroy()
		{
			if (initialized)
			{
				m_program_cache.clear();
				m_sampler.reset();

				vkDestroyDescriptorSetLayout(*m_device, m_descriptor_layout, nullptr);
				vkDestroyPipelineLayout(*m_device, m_pipeline_layout, nullptr);
				m_descriptor_pool.destroy();

				initialized = false;
			}
		}

		void free_resources()
		{
			if (m_used_descriptors == 0)
				return;

			vkResetDescriptorPool(*m_device, m_descriptor_pool, 0);
			m_used_descriptors = 0;
		}

		vk::framebuffer* get_framebuffer(vk::image* target, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			std::vector<vk::image*> test = {target};
			for (auto It = framebuffer_resources.begin(); It != framebuffer_resources.end(); It++)
			{
				auto fbo = It->get();
				if (fbo->matches(test, target->width(), target->height()))
				{
					fbo->deref_count = 0;
					return fbo;
				}
			}

			//No match, create new fbo and add to the list
			std::vector<std::unique_ptr<vk::image_view>> views;
			VkComponentMapping mapping = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
			VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			switch (target->info.format)
			{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; //We are only writing to depth
				break;
			}

			auto view = std::make_unique<vk::image_view>(*m_device, target->value, VK_IMAGE_VIEW_TYPE_2D, target->info.format, mapping, range);
			views.push_back(std::move(view));

			auto fbo = std::make_unique<vk::framebuffer_holder>(*m_device, render_pass, target->width(), target->height(), std::move(views));
			auto result = fbo.get();
			framebuffer_resources.push_back(std::move(fbo));

			return result;
		}

		void run(vk::command_buffer &cmd, u16 w, u16 h, vk::image* target, vk::image_view* src, VkRenderPass render_pass, std::list<std::unique_ptr<vk::framebuffer_holder>>& framebuffer_resources)
		{
			vk::framebuffer *fbo = get_framebuffer(target, render_pass, framebuffer_resources);

			VkViewport vp{};
			vp.width = (f32)w;
			vp.height = (f32)h;
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &vp);

			VkRect2D vs = { { 0, 0 },{ 0u + w, 0u + h } };
			vkCmdSetScissor(cmd, 0, 1, &vs);

			load_program(cmd, render_pass, src);

			VkRenderPassBeginInfo rp_begin = {};
			rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rp_begin.renderPass = render_pass;
			rp_begin.framebuffer = fbo->value;
			rp_begin.renderArea.offset.x = 0;
			rp_begin.renderArea.offset.y = 0;
			rp_begin.renderArea.extent.width = w;
			rp_begin.renderArea.extent.height = h;

			vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdDraw(cmd, 4, 1, 0, 0);
			vkCmdEndRenderPass(cmd);
		}
	};

	struct depth_convert_pass : public overlay_pass
	{
		depth_convert_pass()
		{
			vs_src =
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) out vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};\n"
				"	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};\n"
				"	gl_Position = vec4(positions[gl_VertexIndex % 4], 0., 1.);\n"
				"	tc0 = coords[gl_VertexIndex % 4];\n"
				"}\n"
			};

			fs_src =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(set=0, binding=0) uniform sampler2D fs0;\n"
				"layout(location=0) in vec2 tc0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec4 rgba_in = texture(fs0, tc0);\n"
				"	gl_FragDepth = rgba_in.w * 0.99609 + rgba_in.x * 0.00389 + rgba_in.y * 0.00002;\n"
				"}\n"
			};

			renderpass_config.write_color = false;

			m_vertex_shader.id = 100002;
			m_fragment_shader.id = 100003;
		}
	};
}
