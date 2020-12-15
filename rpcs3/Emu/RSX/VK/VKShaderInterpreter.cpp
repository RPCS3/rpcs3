#include "stdafx.h"
#include "VKShaderInterpreter.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKGSRender.h"

#include "../Common/GLSLCommon.h"
#include "../Common/ShaderInterpreter.h"
#include "../rsx_methods.h"

namespace vk
{
	void shader_interpreter::build_vs()
	{
		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_vertex_program;
		properties.require_lit_emulation = true;

		// TODO: Extend decompiler thread
		// TODO: Rename decompiler thread, it no longer spawns a thread
		RSXVertexProgram null_prog;
		std::string shader_str;
		ParamArray arr;
		VKVertexProgram vk_prog;
		VKVertexDecompilerThread comp(null_prog, shader_str, arr, vk_prog);

		std::stringstream builder;
		comp.insertHeader(builder);
		comp.insertConstants(builder, {});
		comp.insertInputs(builder, {});

		// Insert vp stream input
		builder << "\n"
		"layout(std140, set=0, binding=" << m_vertex_instruction_start << ") readonly restrict buffer VertexInstructionBlock\n"
		"{\n"
		"	uint base_address;\n"
		"	uint entry;\n"
		"	uint output_mask;\n"
		"	uint control;\n"
		"	uvec4 vp_instructions[];\n"
		"};\n\n";

		::glsl::insert_glsl_legacy_function(builder, properties);
		::glsl::insert_vertex_input_fetch(builder, ::glsl::glsl_rules::glsl_rules_spirv);

		builder << program_common::interpreter::get_vertex_interpreter();
		const std::string s = builder.str();

		m_vs.create(::glsl::program_domain::glsl_vertex_program, s);
		m_vs.compile();

		// Prepare input table
		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		vk::glsl::program_input in;

		in.location = binding_table.vertex_params_bind_slot;;
		in.domain = ::glsl::glsl_vertex_program;
		in.name = "VertexContextBuffer";
		in.type = vk::glsl::input_type_uniform_buffer;
		m_vs_inputs.push_back(in);

		in.location = binding_table.vertex_buffers_first_bind_slot;
		in.name = "persistent_input_stream";
		in.type = vk::glsl::input_type_texel_buffer;
		m_vs_inputs.push_back(in);

		in.location = binding_table.vertex_buffers_first_bind_slot + 1;
		in.name = "volatile_input_stream";
		in.type = vk::glsl::input_type_texel_buffer;
		m_vs_inputs.push_back(in);

		in.location = binding_table.vertex_buffers_first_bind_slot + 2;
		in.name = "vertex_layout_stream";
		in.type = vk::glsl::input_type_texel_buffer;
		m_vs_inputs.push_back(in);

		in.location = binding_table.vertex_constant_buffers_bind_slot;
		in.name = "VertexConstantsBuffer";
		in.type = vk::glsl::input_type_uniform_buffer;
		m_vs_inputs.push_back(in);

		// TODO: Bind textures if needed
	}

	glsl::shader* shader_interpreter::build_fs(u64 compiler_options)
	{
		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_fragment_program;
		properties.require_depth_conversion = true;
		properties.require_wpos = true;

		u32 len;
		ParamArray arr;
		std::string shader_str;
		RSXFragmentProgram frag;
		VKFragmentProgram vk_prog;
		VKFragmentDecompilerThread comp(shader_str, arr, frag, len, vk_prog);

		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		std::stringstream builder;
		builder <<
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n\n";

		::glsl::insert_subheader_block(builder);
		comp.insertConstants(builder);

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_GE)
		{
			builder << "#define ALPHA_TEST_GEQUAL\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_G)
		{
			builder << "#define ALPHA_TEST_GREATER\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_LE)
		{
			builder << "#define ALPHA_TEST_LEQUAL\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_L)
		{
			builder << "#define ALPHA_TEST_LESS\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_EQ)
		{
			builder << "#define ALPHA_TEST_EQUAL\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_NE)
		{
			builder << "#define ALPHA_TEST_NEQUAL\n";
		}

		if (!(compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_F32_EXPORT))
		{
			builder << "#define WITH_HALF_OUTPUT_REGISTER\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_DEPTH_EXPORT)
		{
			builder << "#define WITH_DEPTH_EXPORT\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_FLOW_CTRL)
		{
			builder << "#define WITH_FLOW_CTRL\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_PACKING)
		{
			builder << "#define WITH_PACKING\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_KIL)
		{
			builder << "#define WITH_KIL\n";
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_STIPPLING)
		{
			builder << "#define WITH_STIPPLING\n";
		}

		const char* type_names[] = { "sampler1D", "sampler2D", "sampler3D", "samplerCube" };
		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES)
		{
			builder << "#define WITH_TEXTURES\n\n";

			for (int i = 0, bind_location = m_fragment_textures_start; i < 4; ++i)
			{
				builder << "layout(set=0, binding=" << bind_location++ << ") " << "uniform " << type_names[i] << " " << type_names[i] << "_array[16];\n";
			}

			builder << "\n"
				"#define IS_TEXTURE_RESIDENT(index) true\n"
				"#define SAMPLER1D(index) sampler1D_array[index]\n"
				"#define SAMPLER2D(index) sampler2D_array[index]\n"
				"#define SAMPLER3D(index) sampler3D_array[index]\n"
				"#define SAMPLERCUBE(index) samplerCube_array[index]\n\n";
		}

		builder <<
		"layout(std430, binding=" << m_fragment_instruction_start << ") readonly restrict buffer FragmentInstructionBlock\n"
		"{\n"
		"	uint shader_control;\n"
		"	uint texture_control;\n"
		"	uint reserved1;\n"
		"	uint reserved2;\n"
		"	uvec4 fp_instructions[];\n"
		"};\n\n";

		builder << program_common::interpreter::get_fragment_interpreter();
		const std::string s = builder.str();

		auto fs = new glsl::shader();
		fs->create(::glsl::program_domain::glsl_fragment_program, s);
		fs->compile();

		// Prepare input table
		vk::glsl::program_input in;
		in.location = binding_table.fragment_constant_buffers_bind_slot;
		in.domain = ::glsl::glsl_fragment_program;
		in.name = "FragmentConstantsBuffer";
		in.type = vk::glsl::input_type_uniform_buffer;
		m_fs_inputs.push_back(in);

		in.location = binding_table.fragment_state_bind_slot;
		in.name = "FragmentStateBuffer";
		m_fs_inputs.push_back(in);

		in.location = binding_table.fragment_texture_params_bind_slot;
		in.name = "TextureParametersBuffer";
		m_fs_inputs.push_back(in);

		for (int i = 0, location = m_fragment_textures_start; i < 4; ++i, ++location)
		{
			in.location = location;
			in.name = std::string(type_names[i]) + "_array[16]";
			m_fs_inputs.push_back(in);
		}

		m_fs_cache[compiler_options].reset(fs);
		return fs;
	}

	std::pair<VkDescriptorSetLayout, VkPipelineLayout> shader_interpreter::create_layout(VkDevice dev)
	{
		const auto& binding_table = vk::get_current_renderer()->get_pipeline_binding_table();
		std::vector<VkDescriptorSetLayoutBinding> bindings(binding_table.total_descriptor_bindings);

		uint32_t idx = 0;

		// Vertex stream, one stream for cacheable data, one stream for transient data. Third stream contains vertex layout info
		for (int i = 0; i < 3; i++)
		{
			bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			bindings[idx].descriptorCount = 1;
			bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[idx].binding = binding_table.vertex_buffers_first_bind_slot + i;
			idx++;
		}

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_constant_buffers_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_state_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.fragment_texture_params_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.vertex_constant_buffers_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
		bindings[idx].binding = binding_table.vertex_params_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.conditional_render_predicate_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.rasterizer_env_bind_slot;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[idx].descriptorCount = 16;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot;

		m_fragment_textures_start = bindings[idx].binding;
		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[idx].descriptorCount = 16;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 1;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[idx].descriptorCount = 16;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 2;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[idx].descriptorCount = 16;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 3;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[idx].descriptorCount = 4;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 4;

		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 5;

		m_vertex_instruction_start = bindings[idx].binding;
		idx++;

		bindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[idx].descriptorCount = 1;
		bindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[idx].binding = binding_table.textures_first_bind_slot + 6;

		m_fragment_instruction_start = bindings[idx].binding;
		idx++;
		bindings.resize(idx);

		std::array<VkPushConstantRange, 1> push_constants;
		push_constants[0].offset = 0;
		push_constants[0].size = 16;
		push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		if (vk::emulate_conditional_rendering())
		{
			// Conditional render toggle
			push_constants[0].size = 20;
		}

		VkDescriptorSetLayoutCreateInfo infos = {};
		infos.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		infos.pBindings = bindings.data();
		infos.bindingCount = static_cast<uint32_t>(bindings.size());

		VkDescriptorSetLayout set_layout;
		CHECK_RESULT(vkCreateDescriptorSetLayout(dev, &infos, nullptr, &set_layout));

		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &set_layout;
		layout_info.pushConstantRangeCount = 1;
		layout_info.pPushConstantRanges = push_constants.data();

		VkPipelineLayout result;
		CHECK_RESULT(vkCreatePipelineLayout(dev, &layout_info, nullptr, &result));
		return { set_layout, result };
	}

	void shader_interpreter::create_descriptor_pools(const vk::render_device& dev)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 6 * DESCRIPTOR_MAX_DRAW_CALLS });
		sizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER , 3 * DESCRIPTOR_MAX_DRAW_CALLS });
		sizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 68 * DESCRIPTOR_MAX_DRAW_CALLS });
		sizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 * DESCRIPTOR_MAX_DRAW_CALLS });

		m_descriptor_pool.create(dev, sizes.data(), ::size32(sizes), DESCRIPTOR_MAX_DRAW_CALLS, 2);
	}

	void shader_interpreter::init(const vk::render_device& dev)
	{
		m_device = dev;
		std::tie(m_shared_descriptor_layout, m_shared_pipeline_layout) = create_layout(dev);
		create_descriptor_pools(dev);

		build_vs();
		// TODO: Seed the cache
	}

	void shader_interpreter::destroy()
	{
		m_program_cache.clear();
		m_descriptor_pool.destroy();

		for (auto &fs : m_fs_cache)
		{
			fs.second->destroy();
		}

		m_vs.destroy();
		m_fs_cache.clear();

		if (m_shared_pipeline_layout)
		{
			vkDestroyPipelineLayout(m_device, m_shared_pipeline_layout, nullptr);
			m_shared_pipeline_layout = VK_NULL_HANDLE;
		}

		if (m_shared_descriptor_layout)
		{
			vkDestroyDescriptorSetLayout(m_device, m_shared_descriptor_layout, nullptr);
			m_shared_descriptor_layout = VK_NULL_HANDLE;
		}
	}

	glsl::program* shader_interpreter::link(const vk::pipeline_props& properties, u64 compiler_opt)
	{
		glsl::shader* fs;
		if (auto found = m_fs_cache.find(compiler_opt); found != m_fs_cache.end())
		{
			fs = found->second.get();
		}
		else
		{
			fs = build_fs(compiler_opt);
		}

		VkPipelineShaderStageCreateInfo shader_stages[2] = {};
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = m_vs.get_handle();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = fs->get_handle();
		shader_stages[1].pName = "main";

		std::vector<VkDynamicState> dynamic_state_descriptors;
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_SCISSOR);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

		if (vk::get_current_renderer()->get_depth_bounds_support())
		{
			dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
		}

		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.pDynamicStates = dynamic_state_descriptors.data();
		dynamic_state_info.dynamicStateCount = ::size32(dynamic_state_descriptors);

		VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkPipelineViewportStateCreateInfo vp = {};
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo ms = properties.state.ms;
		ensure(ms.rasterizationSamples == VkSampleCountFlagBits((properties.renderpass_key >> 16) & 0xF)); // "Multisample state mismatch!"
		if (ms.rasterizationSamples != VK_SAMPLE_COUNT_1_BIT)
		{
			// Update the sample mask pointer
			ms.pSampleMask = &properties.state.temp_storage.msaa_sample_mask;
		}

		// Rebase pointers from pipeline structure in case it is moved/copied
		VkPipelineColorBlendStateCreateInfo cs = properties.state.cs;
		cs.pAttachments = properties.state.att_state;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pVertexInputState = &vi;
		info.pInputAssemblyState = &properties.state.ia;
		info.pRasterizationState = &properties.state.rs;
		info.pColorBlendState = &cs;
		info.pMultisampleState = &ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &properties.state.ds;
		info.stageCount = 2;
		info.pStages = shader_stages;
		info.pDynamicState = &dynamic_state_info;
		info.layout = m_shared_pipeline_layout;
		info.basePipelineIndex = -1;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.renderPass = vk::get_renderpass(m_device, properties.renderpass_key);

		auto compiler = vk::get_pipe_compiler();
		auto program = compiler->compile(info, m_shared_pipeline_layout, vk::pipe_compiler::COMPILE_INLINE, {}, m_vs_inputs, m_fs_inputs);
		return program.release();
	}

	void shader_interpreter::update_fragment_textures(const std::array<VkDescriptorImageInfo, 68>& sampled_images, VkDescriptorSet descriptor_set)
	{
		const VkDescriptorImageInfo* texture_ptr = sampled_images.data();
		for (uint32_t i = 0, binding = m_fragment_textures_start; i < 4; ++i, ++binding, texture_ptr += 16)
		{
			const VkWriteDescriptorSet descriptor_writer =
			{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,    // sType
				nullptr,                                   // pNext
				descriptor_set,                            // dstSet
				binding,                                   // dstBinding
				0,                                         // dstArrayElement
				16,                                        // descriptorCount
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
				texture_ptr,                               // pImageInfo
				nullptr,                                   // pBufferInfo
				nullptr                                    // pTexelBufferView
			};

			vkUpdateDescriptorSets(m_device, 1, &descriptor_writer, 0, nullptr);
		}
	}

	VkDescriptorSet shader_interpreter::allocate_descriptor_set()
	{
		if (m_used_descriptors == DESCRIPTOR_MAX_DRAW_CALLS)
		{
			m_descriptor_pool.reset(0);
			m_used_descriptors = 0;
		}

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.descriptorPool = m_descriptor_pool;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &m_shared_descriptor_layout;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		VkDescriptorSet new_descriptor_set;
		CHECK_RESULT(vkAllocateDescriptorSets(m_device, &alloc_info, &new_descriptor_set));

		m_used_descriptors++;
		return new_descriptor_set;
	}

	glsl::program* shader_interpreter::get(const vk::pipeline_props& properties, const program_hash_util::fragment_program_utils::fragment_program_metadata& metadata)
	{
		pipeline_key key;
		key.compiler_opt = 0;
		key.properties = properties;

		if (rsx::method_registers.alpha_test_enabled()) [[unlikely]]
		{
			switch (rsx::method_registers.alpha_func())
			{
			case rsx::comparison_function::always:
				break;
			case rsx::comparison_function::never:
				return nullptr;
			case rsx::comparison_function::greater_or_equal:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_GE;
				break;
			case rsx::comparison_function::greater:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_G;
				break;
			case rsx::comparison_function::less_or_equal:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_LE;
				break;
			case rsx::comparison_function::less:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_L;
				break;
			case rsx::comparison_function::equal:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_EQ;
				break;
			case rsx::comparison_function::not_equal:
				key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_NE;
				break;
			}
		}

		if (rsx::method_registers.shader_control() & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_DEPTH_EXPORT;
		if (rsx::method_registers.shader_control() & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_F32_EXPORT;
		if (rsx::method_registers.shader_control() & RSX_SHADER_CONTROL_USES_KIL) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_KIL;
		if (metadata.referenced_textures_mask) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES;
		if (metadata.has_branch_instructions) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_FLOW_CTRL;
		if (metadata.has_pack_instructions) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_PACKING;
		if (rsx::method_registers.polygon_stipple_enabled()) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_STIPPLING;

		if (m_current_key == key) [[likely]]
		{
			return m_current_interpreter;
		}
		else
		{
			m_current_key = key;
		}

		auto found = m_program_cache.find(key);
		if (found != m_program_cache.end()) [[likely]]
		{
			m_current_interpreter = found->second.get();
			return m_current_interpreter;
		}

		m_current_interpreter = link(properties, key.compiler_opt);
		m_program_cache[key].reset(m_current_interpreter);
		return m_current_interpreter;
	}

	bool shader_interpreter::is_interpreter(const glsl::program* prog) const
	{
		return prog == m_current_interpreter;
	}

	uint32_t shader_interpreter::get_vertex_instruction_location() const
	{
		return m_vertex_instruction_start;
	}

	uint32_t shader_interpreter::get_fragment_instruction_location() const
	{
		return m_fragment_instruction_start;
	}
};
