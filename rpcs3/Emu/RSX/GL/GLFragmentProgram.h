#pragma once
#include "../Program/FragmentProgramDecompiler.h"
#include "../Program/GLSLTypes.h"
#include "glutils/program.h"

namespace glsl
{
	struct shader_properties;
}

namespace gl
{
	class shader_interpreter;
}

struct GLFragmentDecompilerThread : public FragmentProgramDecompiler
{
	friend class gl::shader_interpreter;

	std::string& m_shader;
	ParamArray& m_parrDummy;
	glsl::shader_properties m_shader_props{};

public:
	GLFragmentDecompilerThread(std::string& shader, ParamArray& parr, const RSXFragmentProgram &prog, u32& size)
		: FragmentProgramDecompiler(prog, size)
		, m_shader(shader)
		, m_parrDummy(parr)
	{
	}

	void Task();

protected:
	std::string getFloatTypeName(usz elementCount) override;
	std::string getHalfTypeName(usz elementCount) override;
	std::string getFunction(FUNCTION) override;
	std::string compareFunction(COMPARE, const std::string&, const std::string&) override;

	void insertHeader(std::stringstream &OS) override;
	void insertInputs(std::stringstream &OS) override;
	void insertOutputs(std::stringstream &OS) override;
	void insertConstants(std::stringstream &OS) override;
	void insertGlobalFunctions(std::stringstream &OS) override;
	void insertMainStart(std::stringstream &OS) override;
	void insertMainEnd(std::stringstream &OS) override;
};

/** Storage for an Fragment Program in the process of of recompilation.
 *  This class calls OpenGL functions and should only be used from the RSX/Graphics thread.
 */
class GLFragmentProgram
{
public:
	GLFragmentProgram();
	~GLFragmentProgram();

	ParamArray parr;
	u32 id;
	gl::glsl::shader shader;
	std::vector<usz> FragmentConstantOffsetCache;

	/**
	 * Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	 * @param prog RSXShaderProgram specifying the location and size of the shader in memory
	 */
	void Decompile(const RSXFragmentProgram& prog);

private:
	/** Deletes the shader and any stored information */
	void Delete();
};
