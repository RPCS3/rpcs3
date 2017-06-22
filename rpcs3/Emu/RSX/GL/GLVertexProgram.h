#pragma once
#include "../Common/VertexProgramDecompiler.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include "OpenGL.h"

enum
{
	GL_VP_FORCE_ATTRIB_SCALING = 1,	//Scale vertex read result
	GL_VP_ATTRIB_S16_INT = (1 << 1),	//Attrib is a signed 16-bit integer
	GL_VP_ATTRIB_S32_INT = (1 << 2),	//Attrib is a signed 32-bit integer

	GL_VP_SINT_MASK = (GL_VP_ATTRIB_S16_INT|GL_VP_ATTRIB_S32_INT)
};

struct GLVertexDecompilerThread : public VertexProgramDecompiler
{
	std::string &m_shader;
protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	std::string getIntTypeName(size_t elementCount) override;
	virtual std::string getFunction(FUNCTION) override;
	virtual std::string compareFunction(COMPARE, const std::string&, const std::string&) override;

	virtual void insertHeader(std::stringstream &OS) override;
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs) override;
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants) override;
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs) override;
	virtual void insertMainStart(std::stringstream &OS) override;
	virtual void insertMainEnd(std::stringstream &OS) override;

	const RSXVertexProgram &rsx_vertex_program;
public:
	GLVertexDecompilerThread(const RSXVertexProgram &prog, std::string& shader, ParamArray&)
		: VertexProgramDecompiler(prog)
		, m_shader(shader)
		, rsx_vertex_program(prog)
	{
	}

	void Task();
};

class GLVertexProgram
{ 
public:
	GLVertexProgram();
	~GLVertexProgram();

	ParamArray parr;
	u32 id = 0;
	std::string shader;

	void Decompile(const RSXVertexProgram& prog);
	void Compile();

private:
	void Delete();
};
