#pragma once
#include "../Common/FragmentProgramDecompiler.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Utilities/Thread.h"
#include "VulkanAPI.h"
#include "../VK/VKHelpers.h"

struct VKFragmentDecompilerThread : public FragmentProgramDecompiler
{
	std::string& m_shader;
	ParamArray& m_parrDummy;
	std::vector<vk::glsl::program_input> inputs;
	class VKFragmentProgram *vk_prog;
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
	virtual std::string getFloatTypeName(size_t elementCount) override;
	virtual std::string getFunction(FUNCTION) override;
	virtual std::string saturate(const std::string &code) override;
	virtual std::string compareFunction(COMPARE, const std::string&, const std::string&) override;

	virtual void insertHeader(std::stringstream &OS) override;
	virtual void insertIntputs(std::stringstream &OS) override;
	virtual void insertOutputs(std::stringstream &OS) override;
	virtual void insertConstants(std::stringstream &OS) override;
	virtual void insertGlobalFunctions(std::stringstream &OS) override;
	virtual void insertMainStart(std::stringstream &OS) override;
	virtual void insertMainEnd(std::stringstream &OS) override;
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
	std::string shader;
	std::vector<size_t> FragmentConstantOffsetCache;

	std::vector<vk::glsl::program_input> uniforms;
	void SetInputs(std::vector<vk::glsl::program_input>& uniforms);
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
