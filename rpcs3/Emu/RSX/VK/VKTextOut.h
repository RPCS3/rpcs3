#pragma once
#include "VKHelpers.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKRenderPass.h"
#include "../Common/TextGlyphs.h"

namespace vk
{
	class text_writer
	{
	private:
		std::unique_ptr<vk::buffer> m_vertex_buffer;
		std::unique_ptr<vk::buffer> m_uniforms_buffer;

		std::unique_ptr<vk::glsl::program> m_program;
		vk::glsl::shader m_vertex_shader;
		vk::glsl::shader m_fragment_shader;

		vk::descriptor_pool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set = nullptr;
		VkDescriptorSetLayout m_descriptor_layout = nullptr;
		VkPipelineLayout m_pipeline_layout = nullptr;
		u32 m_used_descriptors = 0;

		VkRenderPass m_render_pass;
		VkDevice device = nullptr;

		u32 m_uniform_buffer_offset = 0;
		u32 m_uniform_buffer_size = 0;

		bool initialized = false;
		std::unordered_map<u8, std::pair<u32, u32>> m_offsets;

		void init_descriptor_set(vk::render_device &dev)
		{
			VkDescriptorPoolSize descriptor_pools[1] =
			{
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 120 },
			};

			//Reserve descriptor pools
			m_descriptor_pool.create(dev, descriptor_pools, 1, 120, 2);

			VkDescriptorSetLayoutBinding bindings[1] = {};

			//Scale and offset data plus output color
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[0].binding = 0;

			VkDescriptorSetLayoutCreateInfo infos = {};
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pBindings = bindings;
			infos.bindingCount = 1;

			CHECK_RESULT(vkCreateDescriptorSetLayout(dev, &infos, nullptr, &m_descriptor_layout));

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = 1;
			layout_info.pSetLayouts = &m_descriptor_layout;

			CHECK_RESULT(vkCreatePipelineLayout(dev, &layout_info, nullptr, &m_pipeline_layout));
		}

		void init_program(vk::render_device &dev)
		{
			std::string vs =
			{
				"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=0) in vec2 pos;\n"
				"layout(std140, set=0, binding=0) uniform scale_offset_buffer\n"
				"{\n"
				"	vec4 offsets[510];\n"
				"	vec4 scale;\n"
				"	vec4 text_color;\n"
				"};\n"
				"\n"
				"layout(location=1) out vec4 draw_color;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	vec2 offset = offsets[gl_InstanceIndex].xy;\n"
				"	gl_Position = vec4(pos, 0., 1.);\n"
				"	gl_Position.xy = gl_Position.xy * scale.xy + offset;\n"
				"	draw_color = text_color;\n"
				"}\n"
			};

			std::string fs =
			{
				"#version 420\n"
				"#extension GL_ARB_separate_shader_objects : enable\n"
				"layout(location=1) in vec4 draw_color;\n"
				"layout(location=0) out vec4 col0;\n"
				"\n"
				"void main()\n"
				"{\n"
				"	col0 = draw_color;\n"
				"}\n"
			};

			m_vertex_shader.create(::glsl::program_domain::glsl_vertex_program, vs);
			m_vertex_shader.compile();

			m_fragment_shader.create(::glsl::program_domain::glsl_fragment_program, fs);
			m_fragment_shader.compile();

			VkPipelineShaderStageCreateInfo shader_stages[2] = {};
			shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stages[0].module = m_vertex_shader.get_handle();
			shader_stages[0].pName = "main";

			shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stages[1].module = m_fragment_shader.get_handle();
			shader_stages[1].pName = "main";

			VkDynamicState dynamic_state_descriptors[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
			VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
			dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamic_state_descriptors[dynamic_state_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
			dynamic_state_info.pDynamicStates = dynamic_state_descriptors;

			VkVertexInputAttributeDescription vdesc;
			VkVertexInputBindingDescription vbind;

			vdesc.binding = 0;
			vdesc.format = VK_FORMAT_R32G32_SFLOAT;
			vdesc.location = 0;
			vdesc.offset = 0;

			vbind.binding = 0;
			vbind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vbind.stride = 8;

			VkPipelineVertexInputStateCreateInfo vi = {};
			vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vi.vertexAttributeDescriptionCount = 1;
			vi.vertexBindingDescriptionCount = 1;
			vi.pVertexAttributeDescriptions = &vdesc;
			vi.pVertexBindingDescriptions = &vbind;

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
			ia.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

			VkPipelineRasterizationStateCreateInfo rs = {};
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.lineWidth = 1.f;

			VkPipelineColorBlendAttachmentState att = {};
			att.colorWriteMask = 0xf;

			VkPipelineColorBlendStateCreateInfo cs = {};
			cs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			cs.attachmentCount = 1;
			cs.pAttachments = &att;

			VkPipelineDepthStencilStateCreateInfo ds = {};
			ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

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
			info.renderPass = m_render_pass;

			CHECK_RESULT(vkCreateGraphicsPipelines(dev, nullptr, 1, &info, NULL, &pipeline));

			const std::vector<vk::glsl::program_input> unused;
			m_program = std::make_unique<vk::glsl::program>(static_cast<VkDevice>(dev), pipeline, unused, unused);
		}

		void load_program(vk::command_buffer &cmd, float scale_x, float scale_y, const float *offsets, size_t nb_offsets, std::array<float, 4> color)
		{
			verify(HERE), m_used_descriptors < 120;

			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.descriptorPool = m_descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &m_descriptor_layout;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets(device, &alloc_info, &m_descriptor_set));
			m_used_descriptors++;

			float scale[] = { scale_x, scale_y };
			float colors[] = { color[0], color[1], color[2], color[3] };
			float* dst = static_cast<float*>(m_uniforms_buffer->map(m_uniform_buffer_offset, 8192));

			//std140 spec demands that arrays be multiples of 16 bytes
			for (size_t i = 0; i < nb_offsets; ++i)
			{
				dst[i * 4] = offsets[i * 2];
				dst[i * 4 + 1] = offsets[i * 2 + 1];
			}

			memcpy(&dst[510*4], scale, 8);
			memcpy(&dst[511*4], colors, 16);

			m_uniforms_buffer->unmap();

			m_program->bind_uniform({ m_uniforms_buffer->value, m_uniform_buffer_offset, 8192 }, 0, m_descriptor_set);
			m_uniform_buffer_offset = (m_uniform_buffer_offset + 8192) % m_uniform_buffer_size;

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

			VkDeviceSize zero = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertex_buffer->value, &zero);
		}

	public:

		text_writer() = default;
		~text_writer()
		{
			if (initialized)
			{
				m_vertex_shader.destroy();
				m_fragment_shader.destroy();

				vkDestroyDescriptorSetLayout(device, m_descriptor_layout, nullptr);
				vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
				m_descriptor_pool.destroy();
			}
		}

		void init(vk::render_device &dev, VkRenderPass render_pass)
		{
			verify(HERE), render_pass != VK_NULL_HANDLE;

			//At worst case, 1 char = 16*16*8 bytes (average about 24*8), so ~256K for 128 chars. Allocating 512k for verts
			//uniform params are 8k in size, allocating for 120 lines (max lines at 4k, one column per row. Can be expanded
			m_vertex_buffer = std::make_unique<vk::buffer>(dev, 524288, dev.get_memory_mapping().host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
			m_uniforms_buffer = std::make_unique<vk::buffer>(dev, 983040, dev.get_memory_mapping().host_visible_coherent, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0);

			m_render_pass = render_pass;
			m_uniform_buffer_size = 983040;

			init_descriptor_set(dev);
			init_program(dev);

			GlyphManager glyph_source;
			auto points = glyph_source.generate_point_map();
			const size_t buffer_size = points.size() * sizeof(GlyphManager::glyph_point);

			u8* dst = static_cast<u8*>(m_vertex_buffer->map(0, buffer_size));
			memcpy(dst, points.data(), buffer_size);
			m_vertex_buffer->unmap();

			m_offsets = glyph_source.get_glyph_offsets();

			device = dev;
			initialized = true;
		}

		void print_text(vk::command_buffer &cmd, vk::framebuffer &target, int x, int y, int target_w, int target_h, const std::string &text, std::array<float, 4> color = { 0.3f, 1.f, 0.3f, 1.f })
		{
			std::vector<u32> offsets;
			std::vector<u32> counts;
			std::vector<float> shader_offsets;
			char *s = const_cast<char *>(text.c_str());

			//Y is in raster coordinates: convert to bottom-left origin
			y = (target_h - y - 16);

			//Compress [0, w] and [0, h] into range [-1, 1]
			//Flip Y scaling
			float scale_x = +2.f / target_w;
			float scale_y = -2.f / target_h;

			float base_offset = 0.f;
			shader_offsets.reserve(text.length() * 2);

			while (*s)
			{
				u8 offset = static_cast<u8>(*s);
				bool to_draw = false;	//Can be false for space or unsupported characters

				auto o = m_offsets.find(offset);
				if (o != m_offsets.end())
				{
					if (o->second.second > 0)
					{
						to_draw = true;
						offsets.push_back(o->second.first);
						counts.push_back(o->second.second);
					}
				}

				if (to_draw)
				{
					//Generate a scale_offset pair for this entry
					float offset_x = scale_x * (x + base_offset);
					offset_x -= 1.f;

					float offset_y = scale_y * y;
					offset_y += 1.f;

					shader_offsets.push_back(offset_x);
					shader_offsets.push_back(offset_y);
				}

				base_offset += 9.f;
				s++;
			}

			VkViewport vp{};
			vp.width = static_cast<f32>(target_w);
			vp.height = static_cast<f32>(target_h);
			vp.minDepth = 0.f;
			vp.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &vp);

			VkRect2D vs = { {0, 0}, {0u+target_w, 0u+target_h} };
			vkCmdSetScissor(cmd, 0, 1, &vs);

			//TODO: Add drop shadow if deemed necessary for visibility
			load_program(cmd, scale_x, scale_y, shader_offsets.data(), counts.size(), color);

			const coordu viewport = { positionu{0u, 0u}, sizeu{target.width(), target.height() } };
			vk::begin_renderpass(cmd, m_render_pass, target.value, viewport);

			for (uint i = 0; i < counts.size(); ++i)
			{
				vkCmdDraw(cmd, counts[i], 1, offsets[i], i);
			}
		}

		void reset_descriptors()
		{
			if (m_used_descriptors == 0)
				return;

			m_descriptor_pool.reset(0);
			m_used_descriptors = 0;
		}
	};
}
