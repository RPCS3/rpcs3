#include "stdafx.h"
#include "VKHelpers.h"

namespace vk
{
	namespace glsl
	{
		program::program()
		{
			memset(&pstate, 0, sizeof(pstate));
		}

		program::program(vk::render_device &renderer)
		{
			memset(&pstate, 0, sizeof(pstate));
			init_pipeline();
			device = &renderer;
		}

		program::program(program&& other)
		{
			//This object does not yet exist in a valid state. Clear the original
			memset(&pstate, 0, sizeof(pstate));

			pipeline_state tmp;
			memcpy(&tmp, &pstate, sizeof pstate);
			memcpy(&pstate, &other.pstate, sizeof pstate);
			memcpy(&other.pstate, &tmp, sizeof pstate);

			std::vector<program_input> tmp_uniforms = uniforms;
			uniforms = other.uniforms;
			other.uniforms = tmp_uniforms;

			vk::descriptor_pool tmp_pool;
			descriptor_pool = other.descriptor_pool;
			other.descriptor_pool = tmp_pool;

			vk::render_device *tmp_dev = device;
			device = other.device;
			other.device = tmp_dev;

			bool _uniforms_changed = uniforms_changed;
			uniforms_changed = other.uniforms_changed;
			other.uniforms_changed = _uniforms_changed;
		}

		program& program::operator = (program&& other)
		{
			pipeline_state tmp;
			memcpy(&tmp, &pstate, sizeof pstate);
			memcpy(&pstate, &other.pstate, sizeof pstate);
			memcpy(&other.pstate, &tmp, sizeof pstate);

			std::vector<program_input> tmp_uniforms = uniforms;
			uniforms = other.uniforms;
			other.uniforms = tmp_uniforms;

			vk::descriptor_pool tmp_pool;
			descriptor_pool = other.descriptor_pool;
			other.descriptor_pool = tmp_pool;

			vk::render_device *tmp_dev = device;
			device = other.device;
			other.device = tmp_dev;

			bool _uniforms_changed = uniforms_changed;
			uniforms_changed = other.uniforms_changed;
			other.uniforms_changed = _uniforms_changed;

			return *this;
		}

		void program::init_pipeline()
		{
			pstate.dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pstate.dynamic_state.pDynamicStates = pstate.dynamic_state_descriptors;

			pstate.pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pstate.pipeline.layout = nullptr;

			pstate.vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			pstate.ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			pstate.ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			pstate.rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			pstate.rs.polygonMode = VK_POLYGON_MODE_FILL;
			pstate.rs.cullMode = VK_CULL_MODE_NONE;
			pstate.rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			pstate.rs.depthClampEnable = VK_FALSE;
			pstate.rs.rasterizerDiscardEnable = VK_FALSE;
			pstate.rs.depthBiasEnable = VK_FALSE;

			pstate.cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			pstate.cb.attachmentCount = 1;
			pstate.cb.pAttachments = pstate.att_state;

			for (int i = 0; i < 4; ++i)
			{
				pstate.att_state[i].colorWriteMask = 0xf;
				pstate.att_state[i].blendEnable = VK_FALSE;
			}

			pstate.vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			pstate.vp.viewportCount = 1;
			pstate.dynamic_state_descriptors[pstate.dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			pstate.vp.scissorCount = 1;
			pstate.dynamic_state_descriptors[pstate.dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
			pstate.dynamic_state_descriptors[pstate.dynamic_state.dynamicStateCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;

			pstate.ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			pstate.ds.depthTestEnable = VK_FALSE;
			pstate.ds.depthWriteEnable = VK_TRUE;
			pstate.ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			pstate.ds.depthBoundsTestEnable = VK_FALSE;
			pstate.ds.back.failOp = VK_STENCIL_OP_KEEP;
			pstate.ds.back.passOp = VK_STENCIL_OP_KEEP;
			pstate.ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
			pstate.ds.stencilTestEnable = VK_FALSE;
			pstate.ds.front = pstate.ds.back;

			pstate.ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			pstate.ms.pSampleMask = NULL;
			pstate.ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			pstate.fs = nullptr;
			pstate.vs = nullptr;
			pstate.dirty = true;

			pstate.pipeline.stageCount = 2;

			pstate.shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pstate.shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			pstate.shader_stages[0].module = nullptr;
			pstate.shader_stages[0].pName = "main";

			pstate.shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pstate.shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			pstate.shader_stages[1].module = nullptr;
			pstate.shader_stages[1].pName = "main";

			pstate.pipeline_cache_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		}

		program::~program() 
		{
			LOG_ERROR(RSX, "Program destructor invoked!");
			destroy();
		}

		program& program::attach_device(vk::render_device &dev)
		{
			if (!device)
				init_pipeline();

			device = &dev;
			return *this;
		}

		program& program::attachFragmentProgram(VkShaderModule prog)
		{
			pstate.fs = prog;
			return *this;
		}

		program& program::attachVertexProgram(VkShaderModule prog)
		{
			pstate.vs = prog;
			return *this;
		}

		void program::make()
		{
			if (pstate.fs == nullptr || pstate.vs == nullptr)
				throw EXCEPTION("Missing shader stage!");

			pstate.shader_stages[0].module = pstate.vs;
			pstate.shader_stages[1].module = pstate.fs;

			CHECK_RESULT(vkCreatePipelineCache((*device), &pstate.pipeline_cache_desc, nullptr, &pstate.pipeline_cache));
		}

		void program::set_depth_compare_op(VkCompareOp op)
		{
			if (pstate.ds.depthCompareOp != op)
			{
				pstate.ds.depthCompareOp = op;
				pstate.dirty = true;
			}
		}

		void program::set_depth_write_mask(VkBool32 write_enable)
		{
			if (pstate.ds.depthWriteEnable != write_enable)
			{
				pstate.ds.depthWriteEnable = write_enable;
				pstate.dirty = true;
			}
		}

		void program::set_depth_test_enable(VkBool32 state)
		{
			if (pstate.ds.depthTestEnable != state)
			{
				pstate.ds.depthTestEnable = state;
				pstate.dirty = true;
			}
		}

		void program::set_primitive_topology(VkPrimitiveTopology topology)
		{
			if (pstate.ia.topology != topology)
			{
				pstate.ia.topology = topology;
				pstate.dirty = true;
			}
		}

		void program::set_color_mask(int num_targets, u8* targets, VkColorComponentFlags* flags)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].colorWriteMask != flags[idx])
					{
						pstate.att_state[id].colorWriteMask = flags[idx];
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_blend_state(int num_targets, u8* targets, VkBool32* enable)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].blendEnable != enable[idx])
					{
						pstate.att_state[id].blendEnable = enable[idx];
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_blend_state(int num_targets, u8 *targets, VkBool32 enable)
		{
			for (u8 idx = 0; idx < num_targets; ++idx)
			{
				u8 &id = targets[idx];
				if (pstate.att_state[id].blendEnable != enable)
				{
					pstate.att_state[id].blendEnable = enable;
					pstate.dirty = true;
				}
			}
		}

		void program::set_blend_func(int num_targets, u8* targets, VkBlendFactor* src_color, VkBlendFactor* dst_color, VkBlendFactor* src_alpha, VkBlendFactor* dst_alpha)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].srcColorBlendFactor != src_color[idx])
					{
						pstate.att_state[id].srcColorBlendFactor = src_color[idx];
						pstate.dirty = true;
					}

					if (pstate.att_state[id].dstColorBlendFactor != dst_color[idx])
					{
						pstate.att_state[id].dstColorBlendFactor = dst_color[idx];
						pstate.dirty = true;
					}

					if (pstate.att_state[id].srcAlphaBlendFactor != src_alpha[idx])
					{
						pstate.att_state[id].srcAlphaBlendFactor = src_alpha[idx];
						pstate.dirty = true;
					}

					if (pstate.att_state[id].dstAlphaBlendFactor != dst_alpha[idx])
					{
						pstate.att_state[id].dstAlphaBlendFactor = dst_alpha[idx];
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_blend_func(int num_targets, u8* targets, VkBlendFactor src_color, VkBlendFactor dst_color, VkBlendFactor src_alpha, VkBlendFactor dst_alpha)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].srcColorBlendFactor != src_color)
					{
						pstate.att_state[id].srcColorBlendFactor = src_color;
						pstate.dirty = true;
					}

					if (pstate.att_state[id].dstColorBlendFactor != dst_color)
					{
						pstate.att_state[id].dstColorBlendFactor = dst_color;
						pstate.dirty = true;
					}

					if (pstate.att_state[id].srcAlphaBlendFactor != src_alpha)
					{
						pstate.att_state[id].srcAlphaBlendFactor = src_alpha;
						pstate.dirty = true;
					}

					if (pstate.att_state[id].dstAlphaBlendFactor != dst_alpha)
					{
						pstate.att_state[id].dstAlphaBlendFactor = dst_alpha;
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_blend_op(int num_targets, u8* targets, VkBlendOp* color_ops, VkBlendOp* alpha_ops)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].colorBlendOp != color_ops[idx])
					{
						pstate.att_state[id].colorBlendOp = color_ops[idx];
						pstate.dirty = true;
					}

					if (pstate.att_state[id].alphaBlendOp != alpha_ops[idx])
					{
						pstate.att_state[id].alphaBlendOp = alpha_ops[idx];
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_blend_op(int num_targets, u8* targets, VkBlendOp color_op, VkBlendOp alpha_op)
		{
			if (num_targets)
			{
				for (u8 idx = 0; idx < num_targets; ++idx)
				{
					u8 &id = targets[idx];
					if (pstate.att_state[id].colorBlendOp != color_op)
					{
						pstate.att_state[id].colorBlendOp = color_op;
						pstate.dirty = true;
					}

					if (pstate.att_state[id].alphaBlendOp != alpha_op)
					{
						pstate.att_state[id].alphaBlendOp = alpha_op;
						pstate.dirty = true;
					}
				}
			}
		}

		void program::set_primitive_restart(VkBool32 state)
		{
			if (pstate.ia.primitiveRestartEnable != state)
			{
				pstate.ia.primitiveRestartEnable = state;
				pstate.dirty = true;
			}
		}

		namespace
		{
			std::tuple<VkPipelineLayout, VkDescriptorSetLayout> get_shared_pipeline_layout(VkDevice dev)
			{
				std::array<VkDescriptorSetLayoutBinding, 35> bindings = {};

				size_t idx = 0;
				// Vertex buffer
				for (int i = 0; i < 16; i++)
				{
					bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					bindings[idx].descriptorCount = 1;
					bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
					bindings[idx].binding = VERTEX_BUFFERS_FIRST_BIND_SLOT + i;
					idx++;
				}

				bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				bindings[idx].descriptorCount = 1;
				bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				bindings[idx].binding = FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT;

				idx++;

				bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				bindings[idx].descriptorCount = 1;
				bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				bindings[idx].binding = VERTEX_CONSTANT_BUFFERS_BIND_SLOT;

				idx++;

				for (int i = 0; i < 16; i++)
				{
					bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					bindings[idx].descriptorCount = 1;
					bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					bindings[idx].binding = TEXTURES_FIRST_BIND_SLOT + i;
					idx++;
				}

				bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				bindings[idx].descriptorCount = 1;
				bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
				bindings[idx].binding = SCALE_OFFSET_BIND_SLOT;

				VkDescriptorSetLayoutCreateInfo infos = {};
				infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				infos.pBindings = bindings.data();
				infos.bindingCount = bindings.size();

				VkDescriptorSetLayout set_layout;
				CHECK_RESULT(vkCreateDescriptorSetLayout(dev, &infos, nullptr, &set_layout));

				VkPipelineLayoutCreateInfo layout_info = {};
				layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				layout_info.setLayoutCount = 1;
				layout_info.pSetLayouts = &set_layout;

				VkPipelineLayout result;
				CHECK_RESULT(vkCreatePipelineLayout(dev, &layout_info, nullptr, &result));
				return std::make_tuple(result, set_layout);
			}
		}

		void program::init_descriptor_layout()
		{
			if (descriptor_pool.valid())
				descriptor_pool.destroy();

			std::tie(pstate.pipeline_layout, pstate.descriptor_layouts) = get_shared_pipeline_layout(*device);

			VkDescriptorPoolSize uniform_buffer_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 3};
			VkDescriptorPoolSize uniform_texel_pool = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 16};
			VkDescriptorPoolSize texture_pool = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 16 };

			std::vector<VkDescriptorPoolSize> sizes{ uniform_buffer_pool, uniform_texel_pool, texture_pool };

			descriptor_pool.create((*device), sizes.data(), sizes.size());

			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.descriptorPool = descriptor_pool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &pstate.descriptor_layouts;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets((*device), &alloc_info, &pstate.descriptor_sets));
		}

		void program::destroy_descriptors()
		{
			if (pstate.descriptor_sets)
				vkFreeDescriptorSets((*device), descriptor_pool, 1, &pstate.descriptor_sets);

			if (pstate.pipeline_layout)
				vkDestroyPipelineLayout((*device), pstate.pipeline_layout, nullptr);

			if (pstate.descriptor_layouts)
				vkDestroyDescriptorSetLayout((*device), pstate.descriptor_layouts, nullptr);

			descriptor_pool.destroy();
		}

		void program::set_draw_buffer_count(u8 draw_buffers)
		{
			if (pstate.num_targets != draw_buffers)
			{
				pstate.num_targets = draw_buffers;
				pstate.dirty = true;
			}
		}

		program& program::load_uniforms(program_domain domain, std::vector<program_input>& inputs)
		{
			std::vector<program_input> store = uniforms;
			uniforms.resize(0);

			for (auto &item : store)
			{
				uniforms.push_back(item);
			}

			for (auto &item : inputs)
				uniforms.push_back(item);

			return *this;
		}

		void program::use(vk::command_buffer& commands, VkRenderPass pass, u32 subpass)
		{
			if (/*uniforms_changed*/true)
			{
				uniforms_changed = false;
			}

			if (pstate.dirty)
			{
				if (pstate.pipeline_handle)
					vkDestroyPipeline((*device), pstate.pipeline_handle, nullptr);

				pstate.dynamic_state.pDynamicStates = pstate.dynamic_state_descriptors;
				pstate.cb.pAttachments = pstate.att_state;
				pstate.cb.attachmentCount = pstate.num_targets;

				//Reconfigure this..
				pstate.pipeline.pVertexInputState = &pstate.vi;
				pstate.pipeline.pInputAssemblyState = &pstate.ia;
				pstate.pipeline.pRasterizationState = &pstate.rs;
				pstate.pipeline.pColorBlendState = &pstate.cb;
				pstate.pipeline.pMultisampleState = &pstate.ms;
				pstate.pipeline.pViewportState = &pstate.vp;
				pstate.pipeline.pDepthStencilState = &pstate.ds;
				pstate.pipeline.pStages = pstate.shader_stages;
				pstate.pipeline.pDynamicState = &pstate.dynamic_state;
				pstate.pipeline.layout = pstate.pipeline_layout;
				pstate.pipeline.basePipelineIndex = -1;
				pstate.pipeline.basePipelineHandle = VK_NULL_HANDLE;

				pstate.pipeline.renderPass = pass;

				CHECK_RESULT(vkCreateGraphicsPipelines((*device), nullptr, 1, &pstate.pipeline, NULL, &pstate.pipeline_handle));
				pstate.dirty = false;
			}

			vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pstate.pipeline_handle);
			vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pstate.pipeline_layout, 0, 1, &pstate.descriptor_sets, 0, nullptr);
		}

		bool program::has_uniform(std::string uniform_name)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name)
					return true;
			}

			return false;
		}

		void program::bind_uniform(VkDescriptorImageInfo image_descriptor, std::string uniform_name)
		{
			if (!pstate.descriptor_layouts)
				init_descriptor_layout();
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name)
				{
					VkWriteDescriptorSet descriptor_writer = {};
					descriptor_writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptor_writer.dstSet = pstate.descriptor_sets;
					descriptor_writer.descriptorCount = 1;
					descriptor_writer.pImageInfo = &image_descriptor;
					descriptor_writer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptor_writer.dstArrayElement = 0;
					descriptor_writer.dstBinding = uniform.location + TEXTURES_FIRST_BIND_SLOT;

					vkUpdateDescriptorSets((*device), 1, &descriptor_writer, 0, nullptr);
					return;
				}
			}

			throw EXCEPTION("texture not found");
		}

		void program::bind_uniform(VkDescriptorBufferInfo buffer_descriptor, uint32_t binding_point)
		{
			if (!pstate.descriptor_layouts)
				init_descriptor_layout();
			VkWriteDescriptorSet descriptor_writer = {};
			descriptor_writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writer.dstSet = pstate.descriptor_sets;
			descriptor_writer.descriptorCount = 1;
			descriptor_writer.pBufferInfo = &buffer_descriptor;
			descriptor_writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writer.dstArrayElement = 0;
			descriptor_writer.dstBinding = binding_point;

			vkUpdateDescriptorSets((*device), 1, &descriptor_writer, 0, nullptr);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, const std::string &binding_name)
		{
			if (!pstate.descriptor_layouts)
				init_descriptor_layout();

			for (auto &uniform : uniforms)
			{
				if (uniform.name == binding_name)
				{
					VkWriteDescriptorSet descriptor_writer = {};
					descriptor_writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptor_writer.dstSet = pstate.descriptor_sets;
					descriptor_writer.descriptorCount = 1;
					descriptor_writer.pTexelBufferView = &buffer_view;
					descriptor_writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					descriptor_writer.dstArrayElement = 0;
					descriptor_writer.dstBinding = uniform.location + VERTEX_BUFFERS_FIRST_BIND_SLOT;

					vkUpdateDescriptorSets((*device), 1, &descriptor_writer, 0, nullptr);
					return;
				}
			}
			throw EXCEPTION("vertex buffer not found");
		}

		void program::destroy()
		{
			if (device)
			{
				destroy_descriptors();
				uniforms.resize(0);

				if (pstate.pipeline_handle)
					vkDestroyPipeline((*device), pstate.pipeline_handle, nullptr);

				if (pstate.pipeline_cache)
					vkDestroyPipelineCache((*device), pstate.pipeline_cache, nullptr);
			}

			memset(&pstate, 0, sizeof pstate);
			device = nullptr;
		}
	}
}