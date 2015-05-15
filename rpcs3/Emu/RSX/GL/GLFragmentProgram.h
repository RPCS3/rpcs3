#pragma once
#include "GLShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Utilities/Thread.h"

struct GLFragmentDecompilerThread : public ThreadBase
{
	std::string main;
	std::string& m_shader;
	GLParamArray& m_parr;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;
	u32 m_ctrl;
	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	GLFragmentDecompilerThread(std::string& shader, GLParamArray& parr, u32 addr, u32& size, u32 ctrl)
		: ThreadBase("Fragment Shader Decompiler Thread")
		, m_shader(shader)
		, m_parr(parr)
		, m_addr(addr)
		, m_size(size) 
		, m_const_index(0)
		, m_location(0)
		, m_ctrl(ctrl)
	{
		m_size = 0;
	}

	std::string GetMask();

	void SetDst(std::string code, bool append_mask = true);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	virtual void Task();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
};

/** Storage for an Fragment Program in the process of of recompilation.
 *  This class calls OpenGL functions and should only be used from the RSX/Graphics thread.
 */
class GLFragmentProgram
{
public:
	GLFragmentProgram();
	~GLFragmentProgram();

	GLParamArray parr;
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
