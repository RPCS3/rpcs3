#pragma once
#include "GLProgram.h"
#include "../Common/ProgramStateCache.h"
#include "Utilities/File.h"

struct GLTraits
{
	typedef GLVertexProgram VertexProgramData;
	typedef GLFragmentProgram FragmentProgramData;
	typedef GLProgram PipelineData;
	typedef void* PipelineProperties;
	typedef void* ExtraData;

	static
	void RecompileFragmentProgram(RSXFragmentProgram *RSXFP, FragmentProgramData& fragmentProgramData, size_t ID)
	{
		fragmentProgramData.Decompile(*RSXFP);
		fragmentProgramData.Compile();
		//checkForGlError("m_fragment_prog.Compile");

		// TODO: This shouldn't use current dir
		fs::file("./FragmentProgram.txt", o_write | o_create | o_trunc).write(fragmentProgramData.shader.c_str(), fragmentProgramData.shader.size());
	}

	static
	void RecompileVertexProgram(RSXVertexProgram *RSXVP, VertexProgramData& vertexProgramData, size_t ID)
	{
		vertexProgramData.Decompile(*RSXVP);
		vertexProgramData.Compile();
		//checkForGlError("m_vertex_prog.Compile");

		// TODO: This shouldn't use current dir
		fs::file("./VertexProgram.txt", o_write | o_create | o_trunc).write(vertexProgramData.shader.c_str(), vertexProgramData.shader.size());
	}

	static
	PipelineData *BuildProgram(VertexProgramData &vertexProgramData, FragmentProgramData &fragmentProgramData, const PipelineProperties &pipelineProperties, const ExtraData& extraData)
	{
		GLProgram *result = new GLProgram();
		result->Create(vertexProgramData.id, fragmentProgramData.id);
		//checkForGlError("m_program.Create");
		result->Use();

		LOG_NOTICE(RSX, "*** prog id = %d", result->id);
		LOG_NOTICE(RSX, "*** vp id = %d", vertexProgramData.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fragmentProgramData.id);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vertexProgramData.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fragmentProgramData.shader.c_str());

		return result;
	}

	static
	void DeleteProgram(PipelineData *ptr)
	{
		ptr->Delete();
	}
};

class GLProgramBuffer : public ProgramStateCache<GLTraits>
{
};
