#include "stdafx.h"
#include "GLShaderInterpreter.h"
#include "GLGSRender.h"
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "../Common/ShaderInterpreter.h"
#include "../Common/GLSLCommon.h"

namespace gl
{
	using glsl::shader;

	namespace interpreter
	{
		void texture_pool_allocator::create(shader::type domain)
		{
			GLenum pname;
			switch (domain)
			{
			default:
				rsx_log.fatal("Unexpected program domain %d", static_cast<int>(domain));
			case shader::type::vertex:
				pname = GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS; break;
			case shader::type::fragment:
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
		texture_pools[0].create(shader::type::vertex);
		texture_pools[1].create(shader::type::fragment);

		build_vs();
		build_fs();

		program_handle.create().
			attach(vs).
			attach(fs).
			link();

		program_handle.uniforms[0] = GL_STREAM_BUFFER_START + 0;
		program_handle.uniforms[1] = GL_STREAM_BUFFER_START + 1;

		// Initialize texture bindings
		int assigned = 0;
		auto& allocator = texture_pools[1];
		const char* type_names[] = { "sampler1D_array", "sampler2D_array", "samplerCube_array", "sampler3D_array" };

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < allocator.pools[i].pool_size; ++j)
			{
				allocator.pools[i].allocate(assigned++);
			}

			program_handle.uniforms[type_names[i]] = allocator.pools[i].allocated;
		}
	}

	void shader_interpreter::destroy()
	{
		program_handle.remove();
		vs.remove();
		fs.remove();
	}

	glsl::program* shader_interpreter::get()
	{
		return &program_handle;
	}

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
		GLVertexDecompilerThread comp(null_prog, shader_str, arr);

		std::stringstream builder;
		comp.insertHeader(builder);
		comp.insertConstants(builder, {});
		comp.insertInputs(builder, {});
		comp.insertOutputs(builder, {});

		// Insert vp stream input
		builder << "\n"
		"layout(std140, binding = " << GL_INTERPRETER_VERTEX_BLOCK << ") readonly restrict buffer VertexInstructionBlock\n"
		"{\n"
		"	uint base_address;\n"
		"	uint entry;\n"
		"	uint output_mask;\n"
		"	uint reserved;\n"
		"	uvec4 vp_instructions[];\n"
		"};\n\n";

		::glsl::insert_glsl_legacy_function(builder, properties);
		::glsl::insert_vertex_input_fetch(builder, ::glsl::glsl_rules::glsl_rules_opengl4);

		builder << program_common::interpreter::get_vertex_interpreter();
		const std::string s = builder.str();

		vs.create(glsl::shader::type::vertex);
		vs.source(s);
		vs.compile();
	}

	void shader_interpreter::build_fs()
	{
		// Allocate TIUs
		auto& allocator = texture_pools[1];
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

		::glsl::shader_properties properties{};
		properties.domain = ::glsl::program_domain::glsl_fragment_program;
		properties.require_depth_conversion = true;
		properties.require_wpos = true;

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

		// Declare custom inputs
		builder <<
		"layout(location=1) in vec4 in_regs[15];\n\n";

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

		::program_common::insert_fog_declaration(builder, "vec4", "fogc", true);

		builder << program_common::interpreter::get_fragment_interpreter();
		const std::string s = builder.str();

		fs.create(glsl::shader::type::fragment);
		fs.source(s);
		fs.compile();
	}

	void shader_interpreter::update_fragment_textures(
		const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, 16>& descriptors,
		u16 reference_mask, u32* out)
	{
		if (reference_mask == 0)
		{
			return;
		}

		// Reset allocation
		auto& allocator = texture_pools[1];
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
				verify(HERE), sampler_state;

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

		if (get_driver_caps().vendor_AMD)
		{
			// AMD drivers don't like texture bindings overlapping which means workarounds are needed
			// Technically this is accurate to spec, but makes efficient usage of shader resources difficult
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
		}

		if (allocator.pools[0].flags) program_handle.uniforms["sampler1D_array"] = allocator.pools[0].allocated;
		if (allocator.pools[1].flags) program_handle.uniforms["sampler2D_array"] = allocator.pools[1].allocated;
		if (allocator.pools[2].flags) program_handle.uniforms["samplerCube_array"] = allocator.pools[2].allocated;
		if (allocator.pools[3].flags) program_handle.uniforms["sampler3D_array"] = allocator.pools[3].allocated;
	}
}
