#pragma once
#include "GLShaderParam.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include <set>

struct GLVertexDecompilerThread : public ThreadBase
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
	std::string& m_shader;
	u32 *m_data;
	u32 m_start;
	GLParamArray& m_parr;

	GLVertexDecompilerThread(u32 start, u32* data, std::string& shader, GLParamArray& parr)
		: ThreadBase("Vertex Shader Decompiler Thread")
		, m_data(data)
		, m_start(start)
		, m_shader(shader)
		, m_parr(parr)
	{
		m_funcs.emplace_back();
		m_funcs[0].offset = 0;
		m_funcs[0].name = "main";
		m_funcs.emplace_back();
		m_funcs[1].offset = 0;
		m_funcs[1].name = "func0";
		//m_cur_func->body = "\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
	}

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

	virtual void Task();
};

class GLVertexProgram
{ 
public:
	GLVertexProgram();
	~GLVertexProgram();

	GLParamArray parr;
	u32 id;
	std::string shader;

	void Decompile(u32 start, u32 *prog);
	void DecompileAsync(u32 start, u32 *prog);
	void Wait();
	void Compile();

private:
	GLVertexDecompilerThread* m_decompiler_thread;
	void Delete();
};
