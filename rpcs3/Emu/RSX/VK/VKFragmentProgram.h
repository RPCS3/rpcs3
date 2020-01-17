﻿#pragma once
#include "../Common/FragmentProgramDecompiler.h"
#include "../Common/GLSLTypes.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include "VulkanAPI.h"
#include "VKHelpers.h"

struct VKFragmentDecompilerThread : public FragmentProgramDecompiler
{
	std::string& m_shader;
	ParamArray& m_parrDummy;
	std::vector<vk::glsl::program_input> inputs;
	class VKFragmentProgram *vk_prog;
	glsl::shader_properties m_shader_props{};
	vk::pipeline_binding_table m_binding_table{};

public:
	VKFragmentDecompilerThread(std::string& shader, ParamArray& parr, const RSXFragmentProgram &prog, u32& size, class VKFragmentProgram& dst)
		: FragmentProgramDecompiler(prog, size)
		, m_shader(shader)
		, m_parrDummy(parr)
		, vk_prog(&dst)
	{
	}

	void Task();
	const std::vector<vk::glsl::program_input>& get_inputs() { return inputs; }
protected:
	std::string getFloatTypeName(size_t elementCount) override;
	std::string getHalfTypeName(size_t elementCount) override;
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
class VKFragmentProgram
{
public:
	VKFragmentProgram();
	~VKFragmentProgram();

	ParamArray parr;
	VkShaderModule handle = nullptr;
	u32 id;
	vk::glsl::shader shader;
	std::vector<size_t> FragmentConstantOffsetCache;

	std::array<u32, 4> output_color_masks{ {} };

	std::vector<vk::glsl::program_input> uniforms;
	void SetInputs(std::vector<vk::glsl::program_input>& inputs);
	/**
	 * Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	 * @param prog RSXShaderProgram specifying the location and size of the shader in memory
	 * @param td texture dimensions of input textures
	 */
	void Decompile(const RSXFragmentProgram& prog);

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile();

private:
	/** Deletes the shader and any stored information */
	void Delete();
};
