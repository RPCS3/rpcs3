#pragma once
#include "VKVertexProgram.h"
#include "VKFragmentProgram.h"
#include "../Common/ProgramStateCache.h"

struct VKTraits
{
	using vertex_program_type = VKVertexProgram;
	using fragment_program_type = VKFragmentProgram;
	using pipeline_storage_type = vk::glsl::program;
	using pipeline_properties = void*;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t ID)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.Compile();
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t ID)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.Compile();
	}

	static
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties &pipelineProperties)
	{
		pipeline_storage_type result(*vk::get_current_renderer());

		std::vector<vk::glsl::program_input> vertex_uniforms = vertexProgramData.uniforms;
		std::vector<vk::glsl::program_input> fragment_uniforms = fragmentProgramData.uniforms;
		
		result.attachVertexProgram(vertexProgramData.handle)
			.attachFragmentProgram(fragmentProgramData.handle)
			.load_uniforms(vk::glsl::glsl_vertex_program, vertex_uniforms)
			.load_uniforms(vk::glsl::glsl_fragment_program, fragment_uniforms)
			.make();

		return result;
	}
};

class VKProgramBuffer : public program_state_cache<VKTraits>
{
};
