#pragma once
#include "../Common/FragmentProgramDecompiler.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Utilities/Thread.h"
#include "OpenGL.h"

struct GLFragmentDecompilerThread : public ThreadBase, public FragmentProgramDecompiler
{
	std::string& m_shader;
	ParamArray& m_parrDummy;
public:
	GLFragmentDecompilerThread(std::string& shader, ParamArray& parr, u32 addr, u32& size, u32 ctrl)
		: ThreadBase("Fragment Shader Decompiler Thread"), FragmentProgramDecompiler(addr, size, ctrl)
		, m_shader(shader)
		, m_parrDummy(parr)
	{}

	void Task();

protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	virtual std::string getFunction(FUNCTION) override;
	virtual std::string saturate(const std::string &code) override;
	virtual std::string compareFunction(COMPARE, const std::string&, const std::string&) override;

	virtual void insertHeader(std::stringstream &OS) override;
	virtual void insertIntputs(std::stringstream &OS) override;
	virtual void insertOutputs(std::stringstream &OS) override;
	virtual void insertConstants(std::stringstream &OS) override;
	virtual void insertMainStart(std::stringstream &OS) override;
	virtual void insertMainEnd(std::stringstream &OS) override;
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
	std::string shader;
	std::vector<size_t> FragmentConstantOffsetCache;

	/**
	 * Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	 * @param prog RSXShaderProgram specifying the location and size of the shader in memory
	 */
	void Decompile(RSXFragmentProgram& prog);

	/**
	* Asynchronously decompile a fragment shader located in the PS3's Memory.
	* When this function is called you must call Wait() before GetShaderText() will return valid data.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	void DecompileAsync(RSXFragmentProgram& prog);

	/** Wait for the decompiler task to complete decompilation. */
	void Wait();

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile();

private:
	/** Threaded fragment shader decompiler responsible for decompiling this program */
	GLFragmentDecompilerThread* m_decompiler_thread;

	/** Deletes the shader and any stored information */
	void Delete();
};
