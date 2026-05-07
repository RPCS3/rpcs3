#include "stdafx.h"

#include "VKShaderInterpreter.h"
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"

#include "../Overlays/Shaders/shader_loading_dialog.h"
#include "../Program/GLSLCommon.h"
#include "../Program/ShaderInterpreter.h"
#include "../rsx_methods.h"

#include <thread>

namespace vk
{
	using enum program_common::interpreter::compiler_option;
	using enum program_common::interpreter::cached_pipeline_flags;

	class async_pipe_compiler_context
	{
		vk::pipeline_props m_properties{};
		VkDevice m_device = VK_NULL_HANDLE;
		VkPipelineCache m_pipeline_cache = VK_NULL_HANDLE;
		VkShaderModule m_vs = VK_NULL_HANDLE;
		VkShaderModule m_fs = VK_NULL_HANDLE;

		VkPipelineShaderStageCreateInfo m_shader_stages[2];
		VkPipelineDynamicStateCreateInfo m_dynamic_state_info{};
		VkPipelineVertexInputStateCreateInfo m_vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		VkPipelineViewportStateCreateInfo m_vp{};
		VkPipelineMultisampleStateCreateInfo m_ms{};
		VkPipelineColorBlendStateCreateInfo m_cs{};
		VkPipelineTessellationStateCreateInfo m_ts{};

		std::vector<VkDynamicState> m_dynamic_state_descriptors
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

	public:

		async_pipe_compiler_context(
			const vk::pipeline_props& props,
			VkDevice device,
			VkPipelineCache pipeline_cache,
			VkShaderModule vs,
			VkShaderModule fs)
			: m_properties(props)
			, m_device(device)
			, m_pipeline_cache(pipeline_cache)
			, m_vs(vs)
			, m_fs(fs)
		{
		}

		VkGraphicsPipelineCreateInfo compile()
		{
			m_shader_stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			m_shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			m_shader_stages[0].module = m_vs;
			m_shader_stages[0].pName = "main";

			m_shader_stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO  };
			m_shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			m_shader_stages[1].module = m_fs;
			m_shader_stages[1].pName = "main";

			if (vk::get_current_renderer()->get_depth_bounds_support())
			{
				m_dynamic_state_descriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
			}

			m_dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			m_dynamic_state_info.pDynamicStates = m_dynamic_state_descriptors.data();
			m_dynamic_state_info.dynamicStateCount = ::size32(m_dynamic_state_descriptors);

			m_vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			m_vp.viewportCount = 1;
			m_vp.scissorCount = 1;

			m_ms = m_properties.state.ms;
			ensure(m_ms.rasterizationSamples == VkSampleCountFlagBits((m_properties.renderpass_key >> 16) & 0xF)); // "Multisample state mismatch!"
			if (m_ms.rasterizationSamples != VK_SAMPLE_COUNT_1_BIT)
			{
				// Update the sample mask pointer
				m_ms.pSampleMask = &m_properties.state.temp_storage.msaa_sample_mask;
			}

			// Rebase pointers from pipeline structure in case it is moved/copied
			m_cs = m_properties.state.cs;
			m_cs.pAttachments = m_properties.state.att_state;

			m_ts = {};
			m_ts.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

			VkGraphicsPipelineCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			info.pVertexInputState = &m_vi;
			info.pInputAssemblyState = &m_properties.state.ia;
			info.pRasterizationState = &m_properties.state.rs;
			info.pColorBlendState = &m_cs;
			info.pMultisampleState = &m_ms;
			info.pViewportState = &m_vp;
			info.pDepthStencilState = &m_properties.state.ds;
			info.pTessellationState = &m_ts;
			info.stageCount = 2;
			info.pStages = m_shader_stages;
			info.pDynamicState = &m_dynamic_state_info;
			info.layout = VK_NULL_HANDLE;
			info.basePipelineIndex = -1;
			info.basePipelineHandle = VK_NULL_HANDLE;
			info.renderPass = vk::get_renderpass(m_device, m_properties.renderpass_key);
			return info;
		}
	};

	u32 shader_interpreter::init(std::shared_ptr<VKVertexProgram>& vk_prog, u64 compiler_options) const
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

		if (compiler_options & COMPILER_OPT_ENABLE_INSTANCING)
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

	u32 shader_interpreter::init(std::shared_ptr<VKFragmentProgram>& vk_prog, u64 /*compiler_opt*/) const
	{
		std::memset(&vk_prog->binding_table, 0xff, sizeof(vk_prog->binding_table));

		vk_prog->binding_table.context_buffer_location = 0;
		vk_prog->binding_table.tex_param_location = 1;
		vk_prog->binding_table.polygon_stipple_params_location = 2;

		// Return next index
		return 3;
	}

	std::shared_ptr<VKVertexProgram> shader_interpreter::build_vs(u64 compiler_options)
	{
		compiler_options &= COMPILER_OPT_ALL_VS_MASK;
		{
			reader_lock lock(m_vs_shader_cache_lock);
			if (auto found = m_vs_shader_cache.find(compiler_options);
				found != m_vs_shader_cache.end())
			{
				return found->second;
			}
		}

		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_vertex_program;
		properties.require_lit_emulation = true;
		properties.require_clip_plane_functions = true;

		RSXVertexProgram null_prog;
		std::string shader_str;
		ParamArray arr;

		// Initialize binding layout
		auto vk_prog = std::make_shared<VKVertexProgram>();
		const u32 vertex_instruction_start = init(vk_prog, compiler_options);

		null_prog.ctrl = (compiler_options & COMPILER_OPT_ENABLE_INSTANCING)
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

		// Outputs
		builder << "layout(location=16) out flat uvec4 draw_params_payload;\n\n";

		builder <<
		"#define xform_constants_offset get_draw_params().xform_constants_offset\n"
		"#define scale_offset_mat get_vertex_context().scale_offset_mat\n"
		"#define transform_branch_bits get_vertex_context().transform_branch_bits\n"
		"#define point_size get_vertex_context().point_size\n"
		"#define z_near get_vertex_context().z_near\n"
		"#define z_far get_vertex_context().z_far\n\n";

		// Insert vp stream input
		builder << "\n"
		"layout(std140, set=0, binding=" << vertex_instruction_start << ") readonly restrict buffer VertexInstructionBlock\n"
		"{\n"
		"	uint base_address;\n"
		"	uint entry;\n"
		"	uint output_mask;\n"
		"	uint control;\n"
		"	uvec4 vp_instructions[];\n"
		"};\n\n";

		if (compiler_options & COMPILER_OPT_ENABLE_VTX_TEXTURES)
		{
			// FIXME: Unimplemented
			rsx_log.todo("Vertex textures are currently not implemented for the shader interpreter.");
		}

		if (compiler_options & COMPILER_OPT_ENABLE_INSTANCING)
		{
			builder << "#define _ENABLE_INSTANCED_CONSTANTS\n";
		}

		if (compiler_options)
		{
			builder << "\n";
		}

		::glsl::insert_glsl_legacy_function(builder, properties);
		::glsl::insert_vertex_input_fetch(builder, ::glsl::glsl_rules::glsl_rules_vulkan);
		comp.insertFSExport(builder);

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
		in.location = vertex_instruction_start;
		in.type = glsl::input_type_storage_buffer;
		in.name = "VertexInstructionBlock";
		vs_inputs.push_back(in);

		vk_prog->SetInputs(vs_inputs);

		std::lock_guard lock(m_vs_shader_cache_lock);
		m_vs_shader_cache[compiler_options] = vk_prog;
		return vk_prog;
	}

	std::shared_ptr<VKFragmentProgram> shader_interpreter::build_fs(u64 compiler_options)
	{
		compiler_options &= COMPILER_OPT_ALL_FS_MASK;
		{
			reader_lock lock(m_fs_shader_cache_lock);
			if (auto found = m_fs_shader_cache.find(compiler_options);
				found != m_fs_shader_cache.end())
			{
				return found->second;
			}
		}

		[[maybe_unused]] ::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_fragment_program;
		properties.require_depth_conversion = true;
		properties.require_wpos = true;

		u32 len;
		ParamArray arr;
		std::string shader_str;
		RSXFragmentProgram frag;

		frag.ctrl |= RSX_SHADER_CONTROL_INTERPRETER_MODEL;

		auto vk_prog = std::make_shared<VKFragmentProgram>();
		const u32 fragment_instruction_start = init(vk_prog, compiler_options);
		const u32 fragment_textures_start = fragment_instruction_start + 1;

		VKFragmentDecompilerThread comp(shader_str, arr, frag, len, *vk_prog);

		std::stringstream builder;
		builder <<
		"#version 450\n"
		"#extension GL_EXT_scalar_block_layout : require\n"
		"#extension GL_EXT_uniform_buffer_unsized_array : require\n"
		"#extension GL_ARB_separate_shader_objects : enable\n\n";

		::glsl::insert_subheader_block(builder);
		comp.insertConstants(builder);

		builder << "layout(location=16) in flat uvec4 draw_params_payload;\n\n";

		builder <<
		"#define fog_param0 fs_contexts[_fs_context_offset].fog_param0\n"
		"#define fog_param1 fs_contexts[_fs_context_offset].fog_param1\n"
		"#define fog_mode fs_contexts[_fs_context_offset].fog_mode\n"
		"#define wpos_scale fs_contexts[_fs_context_offset].wpos_scale\n"
		"#define wpos_bias fs_contexts[_fs_context_offset].wpos_bias\n\n";

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_GE)
		{
			builder << "#define ALPHA_TEST_GEQUAL\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_G)
		{
			builder << "#define ALPHA_TEST_GREATER\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_LE)
		{
			builder << "#define ALPHA_TEST_LEQUAL\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_L)
		{
			builder << "#define ALPHA_TEST_LESS\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_EQ)
		{
			builder << "#define ALPHA_TEST_EQUAL\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_ALPHA_TEST_NE)
		{
			builder << "#define ALPHA_TEST_NEQUAL\n";
		}

		if (!(compiler_options & COMPILER_OPT_ENABLE_F32_EXPORT))
		{
			builder << "#define WITH_HALF_OUTPUT_REGISTER\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_DEPTH_EXPORT)
		{
			builder << "#define WITH_DEPTH_EXPORT\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_FLOW_CTRL)
		{
			builder << "#define WITH_FLOW_CTRL\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_PACKING)
		{
			builder << "#define WITH_PACKING\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_KIL)
		{
			builder << "#define WITH_KIL\n";
		}

		if (compiler_options & COMPILER_OPT_ENABLE_STIPPLING)
		{
			builder << "#define WITH_STIPPLING\n";
		}

		const char* type_names[] = { "sampler1D", "sampler2D", "sampler3D", "samplerCube" };
		if (compiler_options & COMPILER_OPT_ENABLE_TEXTURES)
		{
			builder << "#define WITH_TEXTURES\n\n";

			for (int i = 0, bind_location = fragment_textures_start; i < 4; ++i)
			{
				builder << "layout(set=1, binding=" << bind_location++ << ") " << "uniform " << type_names[i] << " " << type_names[i] << "_array[16];\n";
			}

			builder << "\n"
				"#undef  TEX_PARAM\n"
				"#define TEX_PARAM(index) texture_parameters[index + texture_base_index]\n"
				"#define IS_TEXTURE_RESIDENT(index) true\n"
				"#define SAMPLER1D(index) sampler1D_array[index]\n"
				"#define SAMPLER2D(index) sampler2D_array[index]\n"
				"#define SAMPLER3D(index) sampler3D_array[index]\n"
				"#define SAMPLERCUBE(index) samplerCube_array[index]\n"
				"#define texture_base_index _fs_texture_base_index\n\n";
		}

		builder <<
			"layout(std430, set=1, binding=" << fragment_instruction_start << ") readonly restrict buffer FragmentInstructionBlock\n"
			"{\n"
			"	uint shader_control;\n"
			"	uint texture_control;\n"
			"	uint reserved1;\n"
			"	uint reserved2;\n"
			"	uvec4 fp_instructions[];\n"
			"};\n\n";

		builder <<
			"	uint rop_control = fs_contexts[_fs_context_offset].rop_control;\n"
			"	float alpha_ref = fs_contexts[_fs_context_offset].alpha_ref;\n\n";

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
		in.location = fragment_instruction_start;
		in.type = glsl::input_type_storage_buffer;
		in.name = "FragmentInstructionBlock";
		inputs.push_back(in);

		if (compiler_options & COMPILER_OPT_ENABLE_TEXTURES)
		{
			for (int i = 0, location = fragment_textures_start; i < 4; ++i, ++location)
			{
				in.location = location;
				in.name = std::string(type_names[i]) + "_array[16]";
				in.type = glsl::input_type_texture;
				inputs.push_back(in);
			}
		}

		vk_prog->SetInputs(inputs);

		std::lock_guard lock(m_fs_shader_cache_lock);
		m_fs_shader_cache[compiler_options] = vk_prog;
		return vk_prog;
	}

	void shader_interpreter::init(const vk::render_device& dev)
	{
		m_device = dev;

		VkPipelineCacheCreateInfo drv_cache_info{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		vkCreatePipelineCache(m_device, &drv_cache_info, nullptr, &m_driver_pipeline_cache);
	}

	void shader_interpreter::destroy()
	{
		m_current_interpreter.reset();
		m_program_cache.clear();
		m_vs_shader_cache.clear();
		m_fs_shader_cache.clear();

		if (m_driver_pipeline_cache)
		{
			vkDestroyPipelineCache(m_device, m_driver_pipeline_cache, nullptr);
			m_driver_pipeline_cache = VK_NULL_HANDLE;
		}
	}

	std::shared_ptr<glsl::program> shader_interpreter::link(const vk::pipeline_props& properties, u64 compiler_opt, bool async, async_build_fn_callback async_callback)
	{
		auto vs = build_vs(compiler_opt);
		auto fs = build_fs(compiler_opt);

		async_pipe_compiler_context context{ properties, m_device, m_driver_pipeline_cache, vs->shader.get_handle(), fs->shader.get_handle() };
		auto create_graphics_info_fn = [=]() mutable
		{
			return context.compile();
		};

		auto callback_fn = [=, this](std::unique_ptr<glsl::program>& prog)
		{
			if (!async)
			{
				return;
			}

			pipeline_key key{};
			key.compiler_opt = compiler_opt;
			key.properties = properties;

			std::shared_ptr<glsl::program> result = std::move(prog);
			std::lock_guard lock(this->m_program_cache_lock);

			pipeline_cache_entry_t cached_program
			{
				.flags = 0,
				.program = result
			};
			this->m_program_cache[key] = cached_program;

			if (async_callback)
			{
				async_callback(result);
			}
		};

		vk::pipe_compiler::op_flags flags = vk::pipe_compiler::SEPARATE_SHADER_OBJECTS;
		flags |= (async ? vk::pipe_compiler::COMPILE_DEFERRED : vk::pipe_compiler::COMPILE_INLINE);

		auto compiler = vk::get_pipe_compiler();
		auto program = compiler->compile(
			create_graphics_info_fn,
			flags,
			callback_fn,
			vs->uniforms,
			fs->uniforms);

		return program;
	}

	void shader_interpreter::update_fragment_textures(const std::array<VkDescriptorImageInfoEx, 68>& sampled_images)
	{
		// FIXME: Cannot use m_fragment_textures.start now since each interpreter has its own binding layout
		auto [set, binding] = m_current_interpreter->get_uniform_location(::glsl::glsl_fragment_program, glsl::input_type_texture, "sampler1D_array[16]");
		if (binding == umax)
		{
			return;
		}

		const VkDescriptorImageInfoEx* texture_ptr = sampled_images.data();
		for (u32 i = 0; i < 4; ++i, ++binding, texture_ptr += 16)
		{
			m_current_interpreter->bind_uniform_array({ texture_ptr, 16 }, set, binding);
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
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_GE;
				break;
			case rsx::comparison_function::greater:
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_G;
				break;
			case rsx::comparison_function::less_or_equal:
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_LE;
				break;
			case rsx::comparison_function::less:
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_L;
				break;
			case rsx::comparison_function::equal:
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_EQ;
				break;
			case rsx::comparison_function::not_equal:
				key.compiler_opt |= COMPILER_OPT_ENABLE_ALPHA_TEST_NE;
				break;
			}
		}

		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) key.compiler_opt |= COMPILER_OPT_ENABLE_DEPTH_EXPORT;
		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) key.compiler_opt |= COMPILER_OPT_ENABLE_F32_EXPORT;
		if (fp_ctrl & RSX_SHADER_CONTROL_USES_KIL) key.compiler_opt |= COMPILER_OPT_ENABLE_KIL;
		if (fp_metadata.referenced_textures_mask) key.compiler_opt |= COMPILER_OPT_ENABLE_TEXTURES;
		if (fp_metadata.has_branch_instructions) key.compiler_opt |= COMPILER_OPT_ENABLE_FLOW_CTRL;
		if (fp_metadata.has_pack_instructions) key.compiler_opt |= COMPILER_OPT_ENABLE_PACKING;
		if (rsx::method_registers.polygon_stipple_enabled()) key.compiler_opt |= COMPILER_OPT_ENABLE_STIPPLING;
		if (vp_ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS) key.compiler_opt |= COMPILER_OPT_ENABLE_INSTANCING;
		if (vp_metadata.referenced_textures_mask) key.compiler_opt |= COMPILER_OPT_ENABLE_VTX_TEXTURES;

		if (m_current_key == key) [[likely]]
		{
			return m_current_interpreter.get();
		}
		else
		{
			m_current_pipeline_info_ex = *get_pipeline_info_ex(key.compiler_opt);
			m_current_key = key;
		}

		{
			reader_lock lock(m_program_cache_lock);

			auto found = m_program_cache.find(key);
			if (found != m_program_cache.end()) [[likely]]
			{
				m_current_interpreter = found->second.program;

				if ((found->second.flags & (CACHED_PIPE_UNOPTIMIZED | CACHED_PIPE_RECOMPILING)) == CACHED_PIPE_UNOPTIMIZED)
				{
					found->second.flags |= CACHED_PIPE_RECOMPILING;
					link(properties, key.compiler_opt, true, {});
				}

				return m_current_interpreter.get();
			}
		}

		const auto start = std::chrono::steady_clock::now();

		m_current_interpreter = link(properties, key.compiler_opt);

		const auto end = std::chrono::steady_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		if (duration > std::chrono::milliseconds(1000))
		{
			rsx_log.error("Cache miss");
		}

		pipeline_cache_entry_t cache_entry
		{
			.flags = 0,
			.program = m_current_interpreter
		};

		std::lock_guard lock(m_program_cache_lock);
		m_program_cache[key] = cache_entry;
		return m_current_interpreter.get();
	}

	bool shader_interpreter::is_interpreter(const glsl::program* prog) const
	{
		return prog == m_current_interpreter.get();
	}

	u32 shader_interpreter::get_vertex_instruction_location() const
	{
		return m_current_pipeline_info_ex.vertex_instruction_location;
	}

	u32 shader_interpreter::get_fragment_instruction_location() const
	{
		return m_current_pipeline_info_ex.fragment_instruction_location;
	}

	std::pair<std::shared_ptr<VKVertexProgram>, std::shared_ptr<VKFragmentProgram>> shader_interpreter::get_shaders() const
	{
		const auto vs_opt = m_current_key.compiler_opt & COMPILER_OPT_ALL_VS_MASK;
		const auto fs_opt = m_current_key.compiler_opt & COMPILER_OPT_ALL_FS_MASK;

		std::shared_ptr<VKVertexProgram> vs;
		std::shared_ptr<VKFragmentProgram> fs;

		{
			reader_lock lock(m_vs_shader_cache_lock);
			if (auto found = m_vs_shader_cache.find(vs_opt);
				found != m_vs_shader_cache.end())
			{
				vs = found->second;
			}
		}

		{
			reader_lock lock(m_fs_shader_cache_lock);
			if (auto found = m_fs_shader_cache.find(fs_opt);
				found != m_fs_shader_cache.end())
			{
				fs = found->second;
			}
		}

		return { vs, fs };
	}

	const shader_interpreter::pipeline_info_ex_t* shader_interpreter::get_pipeline_info_ex(u64 compiler_opt)
	{
		if (auto found = m_pipeline_info_cache.find(compiler_opt); found != m_pipeline_info_cache.end())
		{
			return &found->second;
		}

		auto vs_stub = std::make_shared<VKVertexProgram>();
		auto fs_stub = std::make_shared<VKFragmentProgram>();
		const auto vi_location = init(vs_stub, compiler_opt);
		const auto fi_location = init(fs_stub, compiler_opt);

		pipeline_info_ex_t result
		{
			.vertex_instruction_location = vi_location,
			.fragment_instruction_location = fi_location,
			.fragment_textures_location = fi_location + 1
		};

		auto it = m_pipeline_info_cache.insert_or_assign(compiler_opt, result);
		return &it.first->second;
	}

	void shader_interpreter::preload(rsx::shader_loading_dialog* dlg)
	{
		dlg->create("Precompiling interpreter variants.\nPlease wait...", "Shader Compilation");

		// Create some basic pipelines that we'll use to seed the base pipeline queue
		std::vector<vk::pipeline_props> pipe_properties;
		auto pdev = vk::get_current_renderer();

		// Base pipeline - simple color
		vk::pipeline_props base_props{};
		base_props.state.set_attachment_count(1);
		base_props.state.enable_cull_face(VK_CULL_MODE_BACK_BIT);
		base_props.state.set_primitive_type(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		base_props.state.set_color_mask(0, true, true, true, true);
		base_props.state.set_attachment_count(1);
		base_props.state.enable_depth_bias(true);
		base_props.state.enable_depth_clamp(true);
		base_props.state.enable_depth_bounds_test(pdev->get_depth_bounds_support());
		base_props.renderpass_key = vk::get_renderpass_key(VK_FORMAT_B8G8R8A8_UNORM);
		pipe_properties.push_back(base_props);

		// Add in some blending
		base_props.state.enable_blend(0,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_OP_ADD, VK_BLEND_OP_ADD);
		pipe_properties.push_back(base_props);

		// Add a depth buffer
		const auto depth_format = pdev->get_formats_support().d24_unorm_s8 ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_D32_SFLOAT_S8_UINT;
		base_props.renderpass_key = vk::get_renderpass_key(VK_FORMAT_B8G8R8A8_UNORM, depth_format);
		base_props.state.enable_depth_test(VK_COMPARE_OP_LESS);
		base_props.state.set_depth_mask(true);
		pipe_properties.push_back(base_props);

		const auto variants = program_common::interpreter::get_interpreter_variants();
		const u32 limit1 = ::size32(variants.base_pipelines) * ::size32(pipe_properties);
		const u32 limit2 = ::size32(variants.pipelines) * ::size32(pipe_properties);
		dlg->set_limit(0, limit1);
		dlg->set_limit(1, limit2);

		atomic_t<u32> ctr = 0;
		for (const auto& props : pipe_properties)
		{
			for (auto& variant : variants.base_pipelines)
			{
				pipeline_key key{};
				key.properties = props;
				key.compiler_opt = variant.first | variant.second;

				link(props, variant.first | variant.second, true, [&](std::shared_ptr<glsl::program>&) { ctr++; });
			}
		}

		// Drain the queue.
		// FIXME: Since the queue is executing from the context of the pipe compiler, we cannot properly stop this process.
		do
		{
			std::this_thread::sleep_for(16ms);

			const auto completed = ctr.load();
			dlg->update_msg(0, fmt::format("Building base variant %u of %u...", completed, limit1));
			dlg->set_value(0, completed);
		}
		while (ctr.load() < limit1);

		ctr = 0;
		std::lock_guard lock(m_program_cache_lock);

		for (const auto& props : pipe_properties)
		{
			pipeline_key base_key;
			base_key.properties = props;

			for (auto& variant : variants.pipelines)
			{
				// Check if we have an exact match
				base_key.compiler_opt = variant.vs_opts.shader_opt | variant.fs_opts.shader_opt;
				if (auto found = m_program_cache.find(base_key);
					found != m_program_cache.end())
				{
					// We have a perfect match, no propagation required
					continue;
				}

				// Find a compatible pipeline
				auto compat_key = base_key;
				compat_key.compiler_opt = variant.vs_opts.compatible_shader_opts | variant.fs_opts.compatible_shader_opts;
				auto found = m_program_cache.find(compat_key);

				ensure(found != m_program_cache.end(), "Invalid interpreter configuration.");

				pipeline_cache_entry_t cache_entry
				{
					.flags = CACHED_PIPE_UNOPTIMIZED,
					.program = found->second.program
				};
				m_program_cache[base_key] = cache_entry;
			}
		}

		dlg->set_value(1, limit2);
		dlg->refresh();
	}
};
