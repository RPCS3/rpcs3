#pragma once
#include "RSXVertexProgram.h"
#include <vector>
#include <stack>
#include "ShaderParam.h"

/**
* This class is used to translate RSX Vertex program to GLSL/HLSL code
* Backend with text based shader can subclass this class and implement :
* - virtual std::string getFloatTypeName(usz elementCount) = 0;
* - virtual std::string getFunction(enum class FUNCTION) = 0;
* - virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) = 0;
* - virtual void insertHeader(std::stringstream &OS) = 0;
* - virtual void insertInputs(std::stringstream &OS) = 0;
* - virtual void insertOutputs(std::stringstream &OS) = 0;
* - virtual void insertConstants(std::stringstream &OS) = 0;
* - virtual void insertMainStart(std::stringstream &OS) = 0;
* - virtual void insertMainEnd(std::stringstream &OS) = 0;
*/
struct VertexProgramDecompiler
{
	D0 d0;
	D1 d1;
	D2 d2;
	D3 d3;
	SRC src[3];

	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

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

	Instruction m_instructions[rsx::max_vertex_program_instructions];
	Instruction* m_cur_instr;
	usz m_instr_count;

	std::vector<std::string> m_body;
	std::stack<u32> m_call_stack;

	const RSXVertexProgram& m_prog;
	ParamArray m_parr;

	std::set<u16> m_constant_ids;

	static std::string NotZeroPositive(const std::string& code);
	std::string GetMask(bool is_sca) const;
	std::string GetVecMask();
	std::string GetScaMask();
	std::string GetDST(bool is_sca = false);
	std::string GetSRC(u32 n);
	std::string GetTex();
	std::string GetRawCond();
	std::string GetCond();
	std::string GetOptionalBranchCond() const;	//Conditional branch expression modified externally at runtime
	std::string AddAddrReg();
	std::string AddCondReg();
	u32 GetAddr() const;
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& lhs, const std::string& rhs);
	void AddCode(const std::string& code);
	void SetDST(bool is_sca, std::string value);
	void SetDSTVec(const std::string& code);
	void SetDSTSca(const std::string& code);
	std::string BuildCode();

protected:
	/** returns the type name of float vectors.
	*/
	virtual std::string getFloatTypeName(usz elementCount) = 0;

	/** returns the type name of int vectors.
	*/
	virtual std::string getIntTypeName(usz elementCount) = 0;

	/** returns string calling function where arguments are passed via
	* $0 $1 $2 substring.
	*/
	virtual std::string getFunction(FUNCTION) = 0;

	/** returns string calling comparison function on 2 args passed as strings.
	*/
	virtual std::string compareFunction(COMPARE, const std::string &, const std::string &, bool scalar = false) = 0;

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
	struct
	{
		// Configuration properties (in)
		// None

		// Decoded properties (out)
		bool has_lit_op = false;
		bool has_indexed_constants = false;
	}
	properties;

	VertexProgramDecompiler(const RSXVertexProgram& prog);
	std::string Decompile();
};
