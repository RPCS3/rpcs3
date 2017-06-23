#pragma once
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "../Common/ProgramStateCache.h"

struct GLTraits
{
	using vertex_program_type = GLVertexProgram;
	using fragment_program_type = GLFragmentProgram;
	using pipeline_storage_type = gl::glsl::program;
	using pipeline_properties = void*;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t /*ID*/)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.Compile();
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t /*ID*/)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.Compile();
	}

	static
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties&)
	{
		pipeline_storage_type result;
		__glcheck result.create()
			.attach(gl::glsl::shader_view(vertexProgramData.id))
			.attach(gl::glsl::shader_view(fragmentProgramData.id))
			.bind_fragment_data_location("ocol0", 0)
			.bind_fragment_data_location("ocol1", 1)
			.bind_fragment_data_location("ocol2", 2)
			.bind_fragment_data_location("ocol3", 3)
			.make();
		__glcheck result.use();

		//Progam locations are guaranteed to not change after linking
		//Texture locations are simply bound to the TIUs so this can be done once
		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			int location;
			if (result.uniforms.has_location("tex" + std::to_string(i), &location))
				result.uniforms[location] = i;
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			int location;
			if (result.uniforms.has_location("vtex" + std::to_string(i), &location))
				result.uniforms[location] = (i + rsx::limits::fragment_textures_count);
		}

		//We use texture buffers for vertex attributes. Bind these here as well
		//as they are guaranteed to be fixed (1 to 1 mapping)
		std::array<const char*, 16> s_reg_table =
		{
			"in_pos_buffer", "in_weight_buffer", "in_normal_buffer",
			"in_diff_color_buffer", "in_spec_color_buffer",
			"in_fog_buffer",
			"in_point_size_buffer", "in_7_buffer",
			"in_tc0_buffer", "in_tc1_buffer", "in_tc2_buffer", "in_tc3_buffer",
			"in_tc4_buffer", "in_tc5_buffer", "in_tc6_buffer", "in_tc7_buffer"
		};

		for (int i = 0; i < rsx::limits::vertex_count; ++i)
		{
			int location;
			if (result.uniforms.has_location(s_reg_table[i], &location))
				result.uniforms[location] = (i + rsx::limits::fragment_textures_count + rsx::limits::vertex_textures_count);
		}

		LOG_NOTICE(RSX, "*** prog id = %d", result.id());
		LOG_NOTICE(RSX, "*** vp id = %d", vertexProgramData.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fragmentProgramData.id);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vertexProgramData.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fragmentProgramData.shader.c_str());

		return result;
	}
};

class GLProgramBuffer : public program_state_cache<GLTraits>
{
};
