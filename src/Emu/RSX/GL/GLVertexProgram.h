#pragma once
#include "../Common/VertexProgramDecompiler.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include "OpenGL.h"

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
	GLVertexDecompilerThread(const RSXVertexProgram &prog, std::string& shader, ParamArray& parr)
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
