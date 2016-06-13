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
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t ID)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.Compile();
		//checkForGlError("m_fragment_prog.Compile");

		fs::file(fs::get_config_dir() + "shaderlog/FragmentProgram.glsl", fs::rewrite).write(fragmentProgramData.shader);
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t ID)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.Compile();
		//checkForGlError("m_vertex_prog.Compile");

		fs::file(fs::get_config_dir() + "shaderlog/VertexProgram.glsl", fs::rewrite).write(vertexProgramData.shader);
	}

	static
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties &pipelineProperties)
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
