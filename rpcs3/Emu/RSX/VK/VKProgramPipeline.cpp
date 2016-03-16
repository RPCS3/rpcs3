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

		void program::init_descriptor_layout()
		{
			if (pstate.descriptor_layouts[0] != nullptr)
				throw EXCEPTION("Existing descriptors found!");

			if (descriptor_pool.valid())
				descriptor_pool.destroy();

			std::vector<VkDescriptorSetLayoutBinding> layout_bindings[2];
			std::vector<VkDescriptorPoolSize> sizes;

			program_input_type types[] = { input_type_uniform_buffer, input_type_texel_buffer, input_type_texture };
			program_domain stages[] = { glsl_vertex_program, glsl_fragment_program };

			VkDescriptorType vk_ids[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
			VkShaderStageFlags vk_stages[] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };

			for (auto &input : uniforms)
			{
				VkDescriptorSetLayoutBinding binding;
				binding.binding = input.location;
				binding.descriptorCount = 1;
				binding.descriptorType = vk_ids[(u32)input.type];
				binding.pImmutableSamplers = nullptr;
				binding.stageFlags = vk_stages[(u32)input.domain];

				layout_bindings[(u32)input.domain].push_back(binding);
			}

			for (int i = 0; i < 3; ++i)
			{
				u32 count = 0;
				for (auto &input : uniforms)
				{
					if (input.type == types[i])
						count++;
				}

				if (!count) continue;

				VkDescriptorPoolSize size;
				size.descriptorCount = count;
				size.type = vk_ids[i];

				sizes.push_back(size);
			}

			descriptor_pool.create((*device), sizes.data(), sizes.size());

			VkDescriptorSetLayoutCreateInfo infos;
			infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			infos.pNext = nullptr;
			infos.flags = 0;
			infos.pBindings = layout_bindings[0].data();
			infos.bindingCount = layout_bindings[0].size();

			CHECK_RESULT(vkCreateDescriptorSetLayout((*device), &infos, nullptr, &pstate.descriptor_layouts[0]));

			infos.pBindings = layout_bindings[1].data();
			infos.bindingCount = layout_bindings[1].size();

			CHECK_RESULT(vkCreateDescriptorSetLayout((*device), &infos, nullptr, &pstate.descriptor_layouts[1]));

			VkPipelineLayoutCreateInfo layout_info;
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.pNext = nullptr;
			layout_info.setLayoutCount = 2;
			layout_info.pSetLayouts = pstate.descriptor_layouts;
			layout_info.flags = 0;
			layout_info.pPushConstantRanges = nullptr;
			layout_info.pushConstantRangeCount = 0;

			CHECK_RESULT(vkCreatePipelineLayout((*device), &layout_info, nullptr, &pstate.pipeline_layout));

			VkDescriptorSetAllocateInfo alloc_info;
			alloc_info.descriptorPool = descriptor_pool;
			alloc_info.descriptorSetCount = 2;
			alloc_info.pNext = nullptr;
			alloc_info.pSetLayouts = pstate.descriptor_layouts;
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

			CHECK_RESULT(vkAllocateDescriptorSets((*device), &alloc_info, pstate.descriptor_sets));
		}

		void program::update_descriptors()
		{
			if (!pstate.descriptor_layouts[0])
				init_descriptor_layout();

			std::vector<VkWriteDescriptorSet> descriptor_writers;
			std::vector<VkDescriptorImageInfo> images(16);
			std::vector<VkDescriptorBufferInfo> buffers(16);
			std::vector<VkDescriptorBufferInfo> texel_buffers(16);
			std::vector<std::unique_ptr<vk::buffer_view>> texel_buffer_views(16);
			VkWriteDescriptorSet write;

			int image_index = 0;
			int buffer_index = 0;
			int texel_buffer_index = 0;

			for (auto &input : uniforms)
			{
				switch (input.type)
				{
				case input_type_texture:
				{
					auto &image = images[image_index++];
					image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					image.sampler = null_sampler();
					image.imageView = null_image_view();

					if (input.as_sampler.sampler && input.as_sampler.image_view)
					{
						image.imageView = input.as_sampler.image_view;
						image.sampler = input.as_sampler.sampler;
						image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}
					else
						LOG_ERROR(RSX, "Texture object was not bound: %s", input.name);

					memset(&write, 0, sizeof(write));
					write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					write.pImageInfo = &image;
					write.descriptorCount = 1;

					break;
				}
				case input_type_uniform_buffer:
				{
					auto &buffer = buffers[buffer_index++];
					buffer.buffer = VK_NULL_HANDLE;
					buffer.offset = 0;
					buffer.range = 0;

					if (input.as_buffer.buffer)
					{
						buffer.buffer = input.as_buffer.buffer;
						buffer.range = input.as_buffer.size;
						buffer.offset = input.as_buffer.offset;
					}
					else
					{
						LOG_ERROR(RSX, "UBO was not bound: %s", input.name);
					}

					memset(&write, 0, sizeof(write));
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					write.pBufferInfo = &buffer;
					write.descriptorCount = 1;
					break;
				}
				case input_type_texel_buffer:
				{
					auto &buffer_view = texel_buffer_views[texel_buffer_index];

					auto &buffer = texel_buffers[texel_buffer_index++];
					buffer.buffer = VK_NULL_HANDLE;
					buffer.offset = 0;
					buffer.range = 0;

					if (input.as_buffer.buffer && input.as_buffer.format != VK_FORMAT_UNDEFINED)
					{
						buffer.offset = input.as_buffer.offset;
						buffer.buffer = input.as_buffer.buffer;
						buffer.range = input.as_buffer.size;
					}
					else
					{
						LOG_ERROR(RSX, "Texel buffer was not bound: %s", input.name);
					}

					buffer_view.reset(new vk::buffer_view(*device, input.as_buffer.buffer, input.as_buffer.format, input.as_buffer.offset, input.as_buffer.size));

					memset(&write, 0, sizeof(write));
					write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					write.pTexelBufferView = &(buffer_view->value);
					write.descriptorCount = 1;
					break;
				}
				default:
					throw EXCEPTION("Unhandled input type!");
				}

				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = pstate.descriptor_sets[input.domain];
				write.pNext = nullptr;

				write.dstBinding = input.location;
				descriptor_writers.push_back(write);
			}

			if (!descriptor_writers.size()) return;
			if (descriptor_writers.size() != uniforms.size())
				throw EXCEPTION("Undefined uniform detected");

			vkUpdateDescriptorSets((*device), descriptor_writers.size(), descriptor_writers.data(), 0, nullptr);
		}

		void program::destroy_descriptors()
		{
			if (pstate.descriptor_sets[0])
				vkFreeDescriptorSets((*device), descriptor_pool, 2, pstate.descriptor_sets);

			if (pstate.pipeline_layout)
				vkDestroyPipelineLayout((*device), pstate.pipeline_layout, nullptr);

			if (pstate.descriptor_layouts[0])
				vkDestroyDescriptorSetLayout((*device), pstate.descriptor_layouts[0], nullptr);

			if (pstate.descriptor_layouts[1])
				vkDestroyDescriptorSetLayout((*device), pstate.descriptor_layouts[1], nullptr);

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
				if (item.domain != domain)
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
				update_descriptors();
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
			vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pstate.pipeline_layout, 0, 2, pstate.descriptor_sets, 0, nullptr);
		}

		bool program::has_uniform(program_domain domain, std::string uniform_name)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name &&
					uniform.domain == domain)
					return true;
			}

			return false;
		}

		bool program::bind_uniform(program_domain domain, std::string uniform_name)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name &&
					uniform.domain == domain)
				{
					uniform.as_buffer.buffer = nullptr;
					uniform.as_buffer.format = VK_FORMAT_UNDEFINED;
					uniform.as_sampler.image_view = nullptr;
					uniform.as_sampler.sampler = nullptr;

					uniforms_changed = true;
					return true;
				}
			}

			return false;
		}

		bool program::bind_uniform(program_domain domain, std::string uniform_name, vk::texture &_texture)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name &&
					uniform.domain == domain)
				{
					VkImageView view = _texture;
					VkSampler sampler = _texture;

					if (uniform.as_sampler.image_view != view ||
						uniform.as_sampler.sampler != sampler)
					{
						uniform.as_sampler.image_view = view;
						uniform.as_sampler.sampler = sampler;
						uniforms_changed = true;
					}

					uniform.type = input_type_texture;
					return true;
				}
			}

			return false;
		}

		bool program::bind_uniform(program_domain domain, std::string uniform_name, VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize size)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name &&
					uniform.domain == domain)
				{
					if (uniform.as_buffer.buffer != _buffer ||
						uniform.as_buffer.size != size ||
						uniform.as_buffer.offset != offset)
					{
						uniform.as_buffer.size = size;
						uniform.as_buffer.buffer = _buffer;
						uniform.as_buffer.format = VK_FORMAT_UNDEFINED; //UBOs cannot be viewed!
						uniform.as_buffer.offset = offset;

						uniforms_changed = true;
					}

					uniform.type = input_type_uniform_buffer;
					return true;
				}
			}
		}

		bool program::bind_uniform(program_domain domain, std::string uniform_name, VkBuffer _buffer, VkDeviceSize offset, VkDeviceSize size, VkFormat format)
		{
			for (auto &uniform : uniforms)
			{
				if (uniform.name == uniform_name &&
					uniform.domain == domain)
				{
					if (uniform.as_buffer.buffer != _buffer ||
						uniform.as_buffer.format != format ||
						uniform.as_buffer.size != size ||
						uniform.as_buffer.offset != 0)
					{
						uniform.as_buffer.size = size;
						uniform.as_buffer.buffer = _buffer;
						uniform.as_buffer.format = format;
						uniform.as_buffer.offset = offset;

						uniforms_changed = true;
					}

					uniform.type = input_type_texel_buffer;
					return true;
				}
			}

			return false;
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