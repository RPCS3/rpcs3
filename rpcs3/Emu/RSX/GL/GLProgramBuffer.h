#pragma once
#include "GLProgram.h"
#include "../Common/ProgramStateCache.h"
#include "Utilities/File.h"

struct GLTraits
{
	typedef GLVertexProgram VertexProgramData;
	typedef GLFragmentProgram FragmentProgramData;
	typedef gl::glsl::program PipelineData;
	typedef void* PipelineProperties;
	typedef void* ExtraData;

	static
	void RecompileFragmentProgram(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID)
	{
		fragmentProgramData.Decompile(*RSXFP, RSXFP->texture_dimensions);
		fragmentProgramData.Compile();
		//checkForGlError("m_fragment_prog.Compile");

		// TODO: This shouldn't use current dir
		fs::file("./FragmentProgram.txt", fom::rewrite) << fragmentProgramData.shader;
	}

	static
	void RecompileVertexProgram(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID)
	{
		vertexProgramData.Decompile(*RSXVP);
		vertexProgramData.Compile();
		//checkForGlError("m_vertex_prog.Compile");

		// TODO: This shouldn't use current dir
		fs::file("./VertexProgram.txt", fom::rewrite) << vertexProgramData.shader;
	}

	static
	PipelineData *BuildProgram(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData)
	{
		PipelineData *result = new PipelineData();
		__glcheck result->create()
			.attach(gl::glsl::shader_view(vertexProgramData.id))
			.attach(gl::glsl::shader_view(fragmentProgramData.id))
			.bind_fragment_data_location("ocol0", 0)
			.bind_fragment_data_location("ocol1", 1)
			.bind_fragment_data_location("ocol2", 2)
			.bind_fragment_data_location("ocol3", 3)
			.make();
		__glcheck result->use();

		LOG_NOTICE(RSX, "*** prog id = %d", result->id());
		LOG_NOTICE(RSX, "*** vp id = %d", vertexProgramData.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fragmentProgramData.id);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vertexProgramData.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fragmentProgramData.shader.c_str());

		return result;
	}

	static
	void DeleteProgram(PipelineData *ptr)
	{
		ptr->remove();
	}
};

class GLProgramBuffer : public ProgramStateCache<GLTraits>
{
};
