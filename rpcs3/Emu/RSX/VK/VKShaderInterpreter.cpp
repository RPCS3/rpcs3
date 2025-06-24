#include "stdafx.h"

#include "VKShaderInterpreter.h"
#include "VKCommonPipelineLayout.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "../Program/GLSLCommon.h"
#include "../Program/ShaderInterpreter.h"
#include "../rsx_methods.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"

namespace vk
{
	u32 shader_interpreter::init(VKVertexProgram* vk_prog, u64 compiler_options) const
	{
		std::memset(&vk_prog->binding_table, 0xff, sizeof(vk_prog->binding_table));

		u32 location = 0;
		vk_prog->binding_table.vertex_buffers_location = location;
		location += 3;

		vk_prog->binding_table.context_buffer_location = location++;

		if (vk::emulate_conditional_rendering())
		{
			vk_prog->binding_table.cr_pred_buffer_location = location++;
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING)
		{
			vk_prog->binding_table.instanced_lut_buffer_location = location++;
			vk_prog->binding_table.instanced_cbuf_location = location++;
		}
		else
		{
			vk_prog->binding_table.cbuf_location = location++;
		}

		if (vk::emulate_conditional_rendering())
		{
			vk_prog->binding_table.cr_pred_buffer_location = location++;
		}

		// Return next index
		return location;
	}

	u32 shader_interpreter::init(VKFragmentProgram* vk_prog, u64 /*compiler_opt*/) const
	{
		std::memset(&vk_prog->binding_table, 0xff, sizeof(vk_prog->binding_table));

		vk_prog->binding_table.context_buffer_location = 0;
		vk_prog->binding_table.tex_param_location = 1;
		vk_prog->binding_table.polygon_stipple_params_location = 2;

		// Return next index
		return 3;
	}

	VKVertexProgram* shader_interpreter::build_vs(u64 compiler_options)
	{
		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_vertex_program;
		properties.require_lit_emulation = true;

		RSXVertexProgram null_prog;
		std::string shader_str;
		ParamArray arr;

		// Initialize binding layout
		auto vk_prog = std::make_unique<VKVertexProgram>();
		m_vertex_instruction_start = init(vk_prog.get(), compiler_options);

		null_prog.ctrl = (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING)
			? RSX_SHADER_CONTROL_INSTANCED_CONSTANTS
			: 0;
		VKVertexDecompilerThread comp(null_prog, shader_str, arr, *vk_prog);

		// Initialize compiler properties
		comp.properties.has_indexed_constants = true;

		ParamType uniforms = { PF_PARAM_UNIFORM, "vec4" };
		uniforms.items.emplace_back("vc[468]", -1);

		std::stringstream builder;
		comp.insertHeader(builder);
		comp.insertConstants(builder, { uniforms });
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

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_VTX_TEXTURES)
		{
			// FIXME: Unimplemented
			rsx_log.todo("Vertex textures are currently not implemented for the shader interpreter.");
		}

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING)
		{
			builder << "#define _ENABLE_INSTANCED_CONSTANTS\n";
		}

		if (compiler_options)
		{
			builder << "\n";
		}

		::glsl::insert_glsl_legacy_function(builder, properties);
		::glsl::insert_vertex_input_fetch(builder, ::glsl::glsl_rules::glsl_rules_vulkan);

		builder << program_common::interpreter::get_vertex_interpreter();
		const std::string s = builder.str();

		auto vs = &vk_prog->shader;
		vs->create(::glsl::program_domain::glsl_vertex_program, s);
		vs->compile();

		// Declare local inputs
		auto vs_inputs = comp.get_inputs();

		vk::glsl::program_input in;
		in.set = 0;
		in.domain = ::glsl::glsl_vertex_program;
		in.location = m_vertex_instruction_start;
		in.type = glsl::input_type_storage_buffer;
		in.name = "VertexInstructionBlock";
		vs_inputs.push_back(in);

		vk_prog->SetInputs(vs_inputs);

		auto ret = vk_prog.get();
		m_shader_cache[compiler_options].m_vs = std::move(vk_prog);
		return ret;
	}

	VKFragmentProgram* shader_interpreter::build_fs(u64 compiler_options)
	{
		[[maybe_unused]] ::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_fragment_program;
		properties.require_depth_conversion = true;
		properties.require_wpos = true;

		u32 len;
		ParamArray arr;
		std::string shader_str;
		RSXFragmentProgram frag;

		auto vk_prog = std::make_unique<VKFragmentProgram>();
		m_fragment_instruction_start = init(vk_prog.get(), compiler_options);
		m_fragment_textures_start = m_fragment_instruction_start + 1;

		VKFragmentDecompilerThread comp(shader_str, arr, frag, len, *vk_prog);

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
				builder << "layout(set=1, binding=" << bind_location++ << ") " << "uniform " << type_names[i] << " " << type_names[i] << "_array[16];\n";
			}

			builder << "\n"
				"#define IS_TEXTURE_RESIDENT(index) true\n"
				"#define SAMPLER1D(index) sampler1D_array[index]\n"
				"#define SAMPLER2D(index) sampler2D_array[index]\n"
				"#define SAMPLER3D(index) sampler3D_array[index]\n"
				"#define SAMPLERCUBE(index) samplerCube_array[index]\n\n";
		}

		builder <<
		"layout(std430, set=1, binding=" << m_fragment_instruction_start << ") readonly restrict buffer FragmentInstructionBlock\n"
		"{\n"
		"	uint shader_control;\n"
		"	uint texture_control;\n"
		"	uint reserved1;\n"
		"	uint reserved2;\n"
		"	uvec4 fp_instructions[];\n"
		"};\n\n";

		builder << program_common::interpreter::get_fragment_interpreter();
		const std::string s = builder.str();

		auto fs = &vk_prog->shader;
		fs->create(::glsl::program_domain::glsl_fragment_program, s);
		fs->compile();

		// Declare local inputs
		auto inputs = comp.get_inputs();

		vk::glsl::program_input in;
		in.set = 1;
		in.domain = ::glsl::glsl_fragment_program;
		in.location = m_fragment_instruction_start;
		in.type = glsl::input_type_storage_buffer;
		in.name = "FragmentInstructionBlock";
		inputs.push_back(in);

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES)
		{
			for (int i = 0, location = m_fragment_textures_start; i < 4; ++i, ++location)
			{
				in.location = location;
				in.name = std::string(type_names[i]) + "_array[16]";
				in.type = glsl::input_type_texture;
				inputs.push_back(in);
			}
		}

		vk_prog->SetInputs(inputs);

		auto ret = vk_prog.get();
		m_shader_cache[compiler_options].m_fs = std::move(vk_prog);
		return ret;
	}

	void shader_interpreter::init(const vk::render_device& dev)
	{
		m_device = dev;
	}

	void shader_interpreter::destroy()
	{
		m_program_cache.clear();
		m_shader_cache.clear();
	}

	glsl::program* shader_interpreter::link(const vk::pipeline_props& properties, u64 compiler_opt)
	{
		VKVertexProgram* vs;
		VKFragmentProgram* fs;

		if (auto found = m_shader_cache.find(compiler_opt); found != m_shader_cache.end())
		{
			fs = found->second.m_fs.get();
			vs = found->second.m_vs.get();
		}
		else
		{
			fs = build_fs(compiler_opt);
			vs = build_vs(compiler_opt);
		}

		VkPipelineShaderStageCreateInfo shader_stages[2] = {};
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = vs->shader.get_handle();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = fs->shader.get_handle();
		shader_stages[1].pName = "main";

		std::vector<VkDynamicState> dynamic_state_descriptors =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
			VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE,
			VK_DYNAMIC_STATE_DEPTH_BIAS
		};

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

		VkPipelineTessellationStateCreateInfo ts = {};
		ts.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.pVertexInputState = &vi;
		info.pInputAssemblyState = &properties.state.ia;
		info.pRasterizationState = &properties.state.rs;
		info.pColorBlendState = &cs;
		info.pMultisampleState = &ms;
		info.pViewportState = &vp;
		info.pDepthStencilState = &properties.state.ds;
		info.pTessellationState = &ts;
		info.stageCount = 2;
		info.pStages = shader_stages;
		info.pDynamicState = &dynamic_state_info;
		info.layout = VK_NULL_HANDLE;
		info.basePipelineIndex = -1;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.renderPass = vk::get_renderpass(m_device, properties.renderpass_key);

		auto compiler = vk::get_pipe_compiler();
		auto program = compiler->compile(
			info,
			vk::pipe_compiler::COMPILE_INLINE | vk::pipe_compiler::SEPARATE_SHADER_OBJECTS,
			{},
			vs->uniforms,
			fs->uniforms);

		return program.release();
	}

	void shader_interpreter::update_fragment_textures(const std::array<VkDescriptorImageInfo, 68>& sampled_images)
	{
		// FIXME: Cannot use m_fragment_textures.start now since each interpreter has its own binding layout
		auto [set, binding] = m_current_interpreter->get_uniform_location(::glsl::glsl_fragment_program, glsl::input_type_texture, "sampler1D_array[16]");
		if (binding == umax)
		{
			return;
		}

		const VkDescriptorImageInfo* texture_ptr = sampled_images.data();
		for (u32 i = 0; i < 4; ++i, ++binding, texture_ptr += 16)
		{
			m_current_interpreter->bind_uniform_array(texture_ptr, 16, set, binding);
		}
	}

	glsl::program* shader_interpreter::get(
		const vk::pipeline_props& properties,
		const program_hash_util::fragment_program_utils::fragment_program_metadata& fp_metadata,
		const program_hash_util::vertex_program_utils::vertex_program_metadata& vp_metadata,
		u32 vp_ctrl,
		u32 fp_ctrl)
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

		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_DEPTH_EXPORT;
		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_F32_EXPORT;
		if (fp_ctrl & RSX_SHADER_CONTROL_USES_KIL) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_KIL;
		if (fp_metadata.referenced_textures_mask) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES;
		if (fp_metadata.has_branch_instructions) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_FLOW_CTRL;
		if (fp_metadata.has_pack_instructions) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_PACKING;
		if (rsx::method_registers.polygon_stipple_enabled()) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_STIPPLING;
		if (vp_ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING;
		if (vp_metadata.referenced_textures_mask) key.compiler_opt |= program_common::interpreter::COMPILER_OPT_ENABLE_VTX_TEXTURES;

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

	u32 shader_interpreter::get_vertex_instruction_location() const
	{
		return m_vertex_instruction_start;
	}

	u32 shader_interpreter::get_fragment_instruction_location() const
	{
		return m_fragment_instruction_start;
	}

	std::pair<VKVertexProgram*, VKFragmentProgram*> shader_interpreter::get_shaders() const
	{
		if (auto found = m_shader_cache.find(m_current_key.compiler_opt); found != m_shader_cache.end())
		{
			auto fs = found->second.m_fs.get();
			auto vs = found->second.m_vs.get();
			return { vs, fs };
		}

		return { nullptr, nullptr };
	}
};
