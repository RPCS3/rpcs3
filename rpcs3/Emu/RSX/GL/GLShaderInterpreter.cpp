#include "stdafx.h"
#include "GLShaderInterpreter.h"
#include "GLTextureCache.h"
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "../rsx_methods.h"
#include "../Program/ShaderInterpreter.h"
#include "../Program/GLSLCommon.h"

namespace gl
{
	using glsl::shader;

	namespace interpreter
	{
		void texture_pool_allocator::create(::glsl::program_domain domain)
		{
			GLenum pname;
			switch (domain)
			{
			default:
				rsx_log.fatal("Unexpected program domain %d", static_cast<int>(domain));
				[[fallthrough]];
			case ::glsl::program_domain::glsl_vertex_program:
				pname = GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS; break;
			case ::glsl::program_domain::glsl_fragment_program:
				pname = GL_MAX_TEXTURE_IMAGE_UNITS; break;
			}

			glGetIntegerv(pname, &max_image_units);
		}

		void texture_pool_allocator::allocate(int size)
		{
			if ((used + size) > max_image_units)
			{
				rsx_log.fatal("Out of image binding slots!");
			}

			used += size;
			texture_pool pool;
			pool.pool_size = size;
			pools.push_back(pool);
		}
	}

	void shader_interpreter::create()
	{
		build_program(::program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES);
		build_program(::program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES | ::program_common::interpreter::COMPILER_OPT_ENABLE_F32_EXPORT);
	}

	void shader_interpreter::destroy()
	{
		for (auto& prog : m_program_cache)
		{
			prog.second->vertex_shader.remove();
			prog.second->fragment_shader.remove();
			prog.second->prog.remove();
		}
	}

	glsl::program* shader_interpreter::get(const interpreter::program_metadata& metadata, u32 vp_ctrl, u32 fp_ctrl)
	{
		// Build options
		u64 opt = 0;
		if (rsx::method_registers.alpha_test_enabled()) [[unlikely]]
		{
			switch (rsx::method_registers.alpha_func())
			{
			case rsx::comparison_function::always:
				break;
			case rsx::comparison_function::never:
				return nullptr;
			case rsx::comparison_function::greater_or_equal:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_GE;
				break;
			case rsx::comparison_function::greater:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_G;
				break;
			case rsx::comparison_function::less_or_equal:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_LE;
				break;
			case rsx::comparison_function::less:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_L;
				break;
			case rsx::comparison_function::equal:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_EQ;
				break;
			case rsx::comparison_function::not_equal:
				opt |= program_common::interpreter::COMPILER_OPT_ENABLE_ALPHA_TEST_NE;
				break;
			}
		}

		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_DEPTH_EXPORT;
		if (fp_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_F32_EXPORT;
		if (fp_ctrl & RSX_SHADER_CONTROL_USES_KIL) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_KIL;
		if (metadata.referenced_textures_mask) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES;
		if (metadata.has_branch_instructions) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_FLOW_CTRL;
		if (metadata.has_pack_instructions) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_PACKING;
		if (rsx::method_registers.polygon_stipple_enabled()) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_STIPPLING;
		if (vp_ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS) opt |= program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING;

		if (auto it = m_program_cache.find(opt); it != m_program_cache.end()) [[likely]]
		{
			m_current_interpreter = it->second.get();
		}
		else
		{
			m_current_interpreter = build_program(opt);
		}

		return &m_current_interpreter->prog;
	}

	void shader_interpreter::build_vs(u64 compiler_options, interpreter::cached_program& prog_data)
	{
		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_vertex_program;
		properties.require_lit_emulation = true;

		// TODO: Extend decompiler thread
		// TODO: Rename decompiler thread, it no longer spawns a thread
		RSXVertexProgram null_prog;
		std::string shader_str;
		ParamArray arr;

		null_prog.ctrl = (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING)
			? RSX_SHADER_CONTROL_INSTANCED_CONSTANTS
			: 0;
		GLVertexDecompilerThread comp(null_prog, shader_str, arr);

		// Initialize compiler properties
		comp.properties.has_indexed_constants = true;

		ParamType uniforms = { PF_PARAM_UNIFORM, "vec4" };
		uniforms.items.emplace_back("vc[468]", -1);

		std::stringstream builder;
		comp.insertHeader(builder);

		builder << "#define Z_NEGATIVE_ONE_TO_ONE\n\n";

		comp.insertConstants(builder, { uniforms });
		comp.insertInputs(builder, {});

		// Insert vp stream input
		builder << "\n"
			"layout(std140, binding = " << GL_INTERPRETER_VERTEX_BLOCK << ") readonly restrict buffer VertexInstructionBlock\n"
			"{\n"
			"	uint base_address;\n"
			"	uint entry;\n"
			"	uint output_mask;\n"
			"	uint control;\n"
			"	uvec4 vp_instructions[];\n"
			"};\n\n";

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_INSTANCING)
		{
			builder << "#define _ENABLE_INSTANCED_CONSTANTS\n";
		}

		if (compiler_options)
		{
			builder << "\n";
		}

		::glsl::insert_glsl_legacy_function(builder, properties);
		::glsl::insert_vertex_input_fetch(builder, ::glsl::glsl_rules::glsl_rules_opengl4);

		builder << program_common::interpreter::get_vertex_interpreter();
		const std::string s = builder.str();

		prog_data.vertex_shader.create(::glsl::program_domain::glsl_vertex_program, s);
		prog_data.vertex_shader.compile();
	}

	void shader_interpreter::build_fs(u64 compiler_options, interpreter::cached_program& prog_data)
	{
		// Allocate TIUs
		auto& allocator = prog_data.allocator;
		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES)
		{
			allocator.create(::glsl::program_domain::glsl_fragment_program);
			if (allocator.max_image_units >= 32)
			{
				// 16 + 4 + 4 + 4
				allocator.allocate(4);   // 1D
				allocator.allocate(16);  // 2D
				allocator.allocate(4);   // CUBE
				allocator.allocate(4);   // 3D
			}
			else if (allocator.max_image_units >= 24)
			{
				// 16 + 4 + 2 + 2
				allocator.allocate(2);   // 1D
				allocator.allocate(16);  // 2D
				allocator.allocate(2);   // CUBE
				allocator.allocate(4);   // 3D
			}
			else if (allocator.max_image_units >= 16)
			{
				// 10 + 2 + 2 + 2
				allocator.allocate(2);   // 1D
				allocator.allocate(10);  // 2D
				allocator.allocate(2);   // CUBE
				allocator.allocate(2);   // 3D
			}
			else
			{
				// Unusable
				rsx_log.fatal("Failed to allocate enough TIUs for shader interpreter.");
			}
		}

		u32 len;
		ParamArray arr;
		std::string shader_str;
		RSXFragmentProgram frag;
		GLFragmentDecompilerThread comp(shader_str, arr, frag, len);

		std::stringstream builder;
		builder <<
		"#version 450\n"
		"#extension GL_ARB_bindless_texture : require\n\n";

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

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES)
		{
			builder << "#define WITH_TEXTURES\n\n";

			const char* type_names[] = { "sampler1D", "sampler2D", "samplerCube", "sampler3D" };
			for (int i = 0; i < 4; ++i)
			{
				builder << "uniform " << type_names[i] << " " << type_names[i] << "_array[" << allocator.pools[i].pool_size << "];\n";
			}

			builder << "\n"
				"#define IS_TEXTURE_RESIDENT(index) (texture_handles[index] < 0xFF)\n"
				"#define SAMPLER1D(index) sampler1D_array[texture_handles[index]]\n"
				"#define SAMPLER2D(index) sampler2D_array[texture_handles[index]]\n"
				"#define SAMPLER3D(index) sampler3D_array[texture_handles[index]]\n"
				"#define SAMPLERCUBE(index) samplerCube_array[texture_handles[index]]\n\n";
		}
		else if (compiler_options)
		{
			builder << "\n";
		}

		builder <<
			"layout(std430, binding =" << GL_INTERPRETER_FRAGMENT_BLOCK << ") readonly restrict buffer FragmentInstructionBlock\n"
			"{\n"
			"	uint shader_control;\n"
			"	uint texture_control;\n"
			"	uint reserved1;\n"
			"	uint reserved2;\n"
			"	uint texture_handles[16];\n"
			"	uvec4 fp_instructions[];\n"
			"};\n\n";

		builder << program_common::interpreter::get_fragment_interpreter();
		const std::string s = builder.str();

		prog_data.fragment_shader.create(::glsl::program_domain::glsl_fragment_program, s);
		prog_data.fragment_shader.compile();
	}

	interpreter::cached_program* shader_interpreter::build_program(u64 compiler_options)
	{
		auto data = new interpreter::cached_program();
		build_fs(compiler_options, *data);
		build_vs(compiler_options, *data);

		data->prog.create().
			attach(data->vertex_shader).
			attach(data->fragment_shader).
			link();

		data->prog.uniforms[0] = GL_STREAM_BUFFER_START + 0;
		data->prog.uniforms[1] = GL_STREAM_BUFFER_START + 1;

		if (compiler_options & program_common::interpreter::COMPILER_OPT_ENABLE_TEXTURES)
		{
			// Initialize texture bindings
			int assigned = 0;
			auto& allocator = data->allocator;
			const char* type_names[] = { "sampler1D_array", "sampler2D_array", "samplerCube_array", "sampler3D_array" };

			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < allocator.pools[i].pool_size; ++j)
				{
					allocator.pools[i].allocate(assigned++);
				}

				data->prog.uniforms[type_names[i]] = allocator.pools[i].allocated;
			}
		}

		m_program_cache[compiler_options].reset(data);
		return data;
	}

	bool shader_interpreter::is_interpreter(const glsl::program* program) const
	{
		return (program == &m_current_interpreter->prog);
	}

	void shader_interpreter::update_fragment_textures(
		const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, 16>& descriptors,
		u16 reference_mask, u32* out)
	{
		if (reference_mask == 0 || !m_current_interpreter)
		{
			return;
		}

		// Reset allocation
		auto& allocator = m_current_interpreter->allocator;
		for (unsigned i = 0; i < 4; ++i)
		{
			allocator.pools[i].num_used = 0;
			allocator.pools[i].flags = 0;
		}

		rsx::simple_array<std::pair<int, int>> replacement_map;
		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			if (reference_mask & (1 << i))
			{
				auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(descriptors[i].get());
				ensure(sampler_state);

				int pool_id = static_cast<int>(sampler_state->image_type);
				auto& pool = allocator.pools[pool_id];

				const int old = pool.allocated[pool.num_used];
				if (!pool.allocate(i))
				{
					rsx_log.error("Could not allocate texture resource for shader interpreter.");
					break;
				}

				out[i] = (pool.num_used - 1);
				if (old != i)
				{
					// Check if the candidate target has also been replaced
					bool found = false;
					for (auto& e : replacement_map)
					{
						if (e.second == old)
						{
							// This replacement consumed this 'old' value
							e.second = i;
							found = true;
							break;
						}
					}

					if (!found)
					{
						replacement_map.push_back({ old, i });
					}
				}
			}
			else
			{
				out[i] = 0xFF;
			}
		}

		// Bind TIU locations
		if (replacement_map.empty()) [[likely]]
		{
			return;
		}

		// Overlapping texture bindings are trouble. Cannot bind one TIU to two types of samplers simultaneously
		for (unsigned i = 0; i < replacement_map.size(); ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				auto& pool = allocator.pools[j];
				for (int k = pool.num_used; k < pool.pool_size; ++k)
				{
					if (pool.allocated[k] == replacement_map[i].second)
					{
						pool.allocated[k] = replacement_map[i].first;
						pool.flags |= static_cast<u32>(interpreter::texture_pool_flags::dirty);

						// Exit nested loop
						j = 4;
						break;
					}
				}
			}
		}

		if (allocator.pools[0].flags) m_current_interpreter->prog.uniforms["sampler1D_array"] = allocator.pools[0].allocated;
		if (allocator.pools[1].flags) m_current_interpreter->prog.uniforms["sampler2D_array"] = allocator.pools[1].allocated;
		if (allocator.pools[2].flags) m_current_interpreter->prog.uniforms["samplerCube_array"] = allocator.pools[2].allocated;
		if (allocator.pools[3].flags) m_current_interpreter->prog.uniforms["sampler3D_array"] = allocator.pools[3].allocated;
	}
}
