#pragma once
#include "../Program/VertexProgramDecompiler.h"
#include "VKProgramPipeline.h"
#include "vkutils/pipeline_binding_table.h"

namespace vk
{
	class shader_interpreter;
}

struct VKVertexDecompilerThread : public VertexProgramDecompiler
{
	friend class vk::shader_interpreter;

	std::string &m_shader;
	std::vector<vk::glsl::program_input> inputs;
	class VKVertexProgram *vk_prog;

	struct
	{
		bool emulate_conditional_rendering{false};
	}
	m_device_props;

protected:
	std::string getFloatTypeName(usz elementCount) override;
	std::string getIntTypeName(usz elementCount) override;
	std::string getFunction(FUNCTION) override;
	std::string compareFunction(COMPARE, const std::string&, const std::string&, bool scalar) override;

	void insertHeader(std::stringstream &OS) override;
	void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs) override;
	void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants) override;
	void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs) override;
	void insertMainStart(std::stringstream &OS) override;
	void insertMainEnd(std::stringstream &OS) override;

	void prepareBindingTable();

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

class VKVertexProgram : public rsx::VertexProgramBase
{
public:
	VKVertexProgram();
	~VKVertexProgram();

	ParamArray parr;
	VkShaderModule handle = nullptr;
	vk::glsl::shader shader;
	std::vector<vk::glsl::program_input> uniforms;

	// Quick attribute indices
	struct
	{
		u32 context_buffer_location = umax;        // Vertex program context
		u32 cr_pred_buffer_location = umax;        // Conditional rendering predicate
		u32 vertex_buffers_location = umax;        // Vertex input streams (3)
		u32 cbuf_location = umax;                  // Vertex program constants register file
		u32 instanced_lut_buffer_location = umax;  // Instancing redirection table
		u32 instanced_cbuf_location = umax;        // Instancing constants register file
		u32 vtex_location[4];                      // Vertex textures (inf)

	} binding_table;

	void Decompile(const RSXVertexProgram& prog);
	void Compile();
	void SetInputs(std::vector<vk::glsl::program_input>& inputs);

private:
	void Delete();
};
