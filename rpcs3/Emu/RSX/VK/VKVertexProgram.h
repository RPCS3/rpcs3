#pragma once
#include "../Common/VertexProgramDecompiler.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include "VulkanAPI.h"
#include "../VK/VKHelpers.h"

struct VKVertexDecompilerThread : public VertexProgramDecompiler
{
	std::string &m_shader;
	std::vector<vk::glsl::program_input> inputs;
	class VKVertexProgram *vk_prog;
protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	std::string getIntTypeName(size_t elementCount) override;
	virtual std::string getFunction(FUNCTION) override;
	virtual std::string compareFunction(COMPARE, const std::string&, const std::string&, bool scalar) override;

	virtual void insertHeader(std::stringstream &OS) override;
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs) override;
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants) override;
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs) override;
	virtual void insertMainStart(std::stringstream &OS) override;
	virtual void insertMainEnd(std::stringstream &OS) override;

	const RSXVertexProgram &rsx_vertex_program;
public:
	VKVertexDecompilerThread(const RSXVertexProgram &prog, std::string& shader, ParamArray&, class VKVertexProgram &dst)
		: VertexProgramDecompiler(prog)
		, m_shader(shader)
		, rsx_vertex_program(prog)
		, vk_prog(&dst)
	{
	}

	void Task();
	const std::vector<vk::glsl::program_input>& get_inputs() { return inputs; }
};

class VKVertexProgram
{ 
public:
	VKVertexProgram();
	~VKVertexProgram();

	ParamArray parr;
	VkShaderModule handle = nullptr;
	u32 id;
	vk::glsl::shader shader;
	std::vector<vk::glsl::program_input> uniforms;

	void Decompile(const RSXVertexProgram& prog);
	void Compile();
	void SetInputs(std::vector<vk::glsl::program_input>& inputs);

private:
	void Delete();
};
