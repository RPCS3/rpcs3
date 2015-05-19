#pragma once
#if defined(DX12_SUPPORT)
#include "Emu/RSX/RSXVertexProgram.h"
#include <vector>
#include <sstream>
#include "../Common/ShaderParam.h"

struct VertexDecompiler
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
	virtual void insertHeader(std::stringstream &OS);
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs);
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants);
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs);
	virtual void insertMainStart(std::stringstream &OS);
	virtual void insertMainEnd(std::stringstream &OS);
public:
	VertexDecompiler(std::vector<u32>& data);
	std::string Decompile();
};
#endif