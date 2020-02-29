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
	vk::pipeline_binding_table m_binding_table{};

	struct
	{
		bool emulate_conditional_rendering;
	}
	m_device_props;

protected:
	std::string getFloatTypeName(size_t elementCount) override;
	std::string getIntTypeName(size_t elementCount) override;
	std::string getFunction(FUNCTION) override;
	std::string compareFunction(COMPARE, const std::string&, const std::string&, bool scalar) override;

	void insertHeader(std::stringstream &OS) override;
	void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs) override;
	void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants) override;
	void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs) override;
	void insertMainStart(std::stringstream &OS) override;
	void insertMainEnd(std::stringstream &OS) override;

	const RSXVertexProgram &rsx_vertex_program;
public:
	VKVertexDecompilerThread(const RSXVertexProgram &prog, std::string& shader, ParamArray&, class VKVertexProgram &dst)
		: VertexProgramDecompiler(prog)
		, m_shader(shader)
		, vk_prog(&dst)
		, rsx_vertex_program(prog)
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
