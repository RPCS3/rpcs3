#pragma once
#include "Emu/RSX/RSXVertexProgram.h"
#include <vector>
#include <sstream>
#include "ShaderParam.h"

/**
* This class is used to translate RSX Vertex program to GLSL/HLSL code
* Backend with text based shader can subclass this class and implement :
* - virtual std::string getFloatTypeName(size_t elementCount) = 0;
* - virtual std::string getFunction(enum class FUNCTION) = 0;
* - virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) = 0;
* - virtual void insertHeader(std::stringstream &OS) = 0;
* - virtual void insertIntputs(std::stringstream &OS) = 0;
* - virtual void insertOutputs(std::stringstream &OS) = 0;
* - virtual void insertConstants(std::stringstream &OS) = 0;
* - virtual void insertMainStart(std::stringstream &OS) = 0;
* - virtual void insertMainEnd(std::stringstream &OS) = 0;
*/
struct VertexProgramDecompiler
{
	struct FuncInfo
	{
		u32 offset;
		std::string name;
	};

	struct Instruction
	{
		std::vector<std::string> body;
		int open_scopes;
		int close_scopes;
		int put_close_scopes;
		int do_count;

		void reset()
		{
			body.clear();
			put_close_scopes = open_scopes = close_scopes = do_count = 0;
		}
	};

	static const size_t m_max_instr_count = 512;
	Instruction m_instructions[m_max_instr_count];
	Instruction* m_cur_instr;
	size_t m_instr_count;

	std::set<int> m_jump_lvls;
	std::vector<std::string> m_body;
	std::vector<FuncInfo> m_funcs;

	//wxString main;

	std::vector<u32>& m_data;
	ParamArray m_parr;

	std::string GetMask(bool is_sca);
	std::string GetVecMask();
	std::string GetScaMask();
	std::string GetDST(bool is_sca = false);
	std::string GetSRC(const u32 n);
	std::string GetFunc();
	std::string GetTex();
	std::string GetCond();
	std::string AddAddrMask();
	std::string AddAddrReg();
	u32 GetAddr();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	void AddCode(const std::string& code);
	void SetDST(bool is_sca, std::string value);
	void SetDSTVec(const std::string& code);
	void SetDSTSca(const std::string& code);
	std::string BuildFuncBody(const FuncInfo& func);
	std::string BuildCode();

protected:
	/** returns the type name of float vectors.
	*/
	virtual std::string getFloatTypeName(size_t elementCount) = 0;

	/** returns string calling function where arguments are passed via
	* $0 $1 $2 substring.
	*/
	virtual std::string getFunction(FUNCTION) = 0;

	/** returns string calling comparaison function on 2 args passed as strings.
	*/
	virtual std::string compareFunction(COMPARE, const std::string &, const std::string &) = 0;

	/** Insert header of shader file (eg #version, "system constants"...)
	*/
	virtual void insertHeader(std::stringstream &OS) = 0;

	/** Insert vertex declaration.
	*/
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs) = 0;

	/** insert global declaration of vertex shader outputs.
	*/
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants) = 0;

	/** insert declaration of shader constants.
	*/
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs) = 0;

	/** insert beginning of main (signature, temporary declaration...)
	*/
	virtual void insertMainStart(std::stringstream &OS) = 0;

	/** insert end of main function (return value, output copy...)
	*/
	virtual void insertMainEnd(std::stringstream &OS) = 0;
public:
	VertexProgramDecompiler(std::vector<u32>& data);
	std::string Decompile();
};