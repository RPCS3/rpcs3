#pragma once
#include "ShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include <sstream>

/**
 * This class is used to translate RSX Fragment program to GLSL/HLSL code
 * Backend with text based shader can subclass this class and implement :
 * - virtual std::string getFloatTypeName(size_t elementCount) = 0;
 * - virtual std::string getFunction(enum class FUNCTION) = 0;
 * - virtual std::string saturate(const std::string &code) = 0;
 * - virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) = 0;
 * - virtual void insertHeader(std::stringstream &OS) = 0;
 * - virtual void insertIntputs(std::stringstream &OS) = 0;
 * - virtual void insertOutputs(std::stringstream &OS) = 0;
 * - virtual void insertConstants(std::stringstream &OS) = 0;
 * - virtual void insertMainStart(std::stringstream &OS) = 0;
 * - virtual void insertMainEnd(std::stringstream &OS) = 0;
 */
class FragmentProgramDecompiler
{
	std::string main;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;

	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

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

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }

	/**
	 * Emits code if opcode is an SCT one and returns true,
	 * otherwise do nothing and return false.
	 * NOTE: What does SCT means ???
	 */
	bool handle_sct(u32 opcode);

	/**
	* Emits code if opcode is an SCB one and returns true,
	* otherwise do nothing and return false.
	* NOTE: What does SCB means ???
	*/
	bool handle_scb(u32 opcode);

	/**
	* Emits code if opcode is an TEX SRB one and returns true,
	* otherwise do nothing and return false.
	* NOTE: What does TEX SRB means ???
	*/
	bool handle_tex_srb(u32 opcode);
protected:
	u32 m_ctrl;
	/** returns the type name of float vectors.
	 */
	virtual std::string getFloatTypeName(size_t elementCount) = 0;

	/** returns string calling function where arguments are passed via
	 * $0 $1 $2 substring.
	 */
	virtual std::string getFunction(FUNCTION) = 0;

	/** returns string calling saturate function.
	*/
	virtual std::string saturate(const std::string &code) = 0;
	/** returns string calling comparaison function on 2 args passed as strings.
	 */
	virtual std::string compareFunction(COMPARE, const std::string &, const std::string &) = 0;

	/** Insert header of shader file (eg #version, "system constants"...)
	 */
	virtual void insertHeader(std::stringstream &OS) = 0;
	/** Insert global declaration of fragments inputs.
	 */
	virtual void insertIntputs(std::stringstream &OS) = 0;
	/** insert global declaration of fragments outputs.
	*/
	virtual void insertOutputs(std::stringstream &OS) = 0;
	/** insert declaration of shader constants.
	*/
	virtual void insertConstants(std::stringstream &OS) = 0;
	/** insert beginning of main (signature, temporary declaration...)
	*/
	virtual void insertMainStart(std::stringstream &OS) = 0;
	/** insert end of main function (return value, output copy...)
	 */
	virtual void insertMainEnd(std::stringstream &OS) = 0;
public:
	ParamArray m_parr;
	FragmentProgramDecompiler(u32 addr, u32& size, u32 ctrl);
	std::string Decompile();
};
