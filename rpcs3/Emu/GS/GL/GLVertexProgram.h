#pragma once
#include "GLShaderParam.h"
#include "Emu/GS/RSXVertexProgram.h"

struct GLVertexDecompilerThread : public ThreadBase
{
	union D0
	{
		u32 HEX;

		struct
		{
			u32 addr_swz				: 2;
			u32 mask_w					: 2;
			u32 mask_z					: 2;
			u32 mask_y					: 2;
			u32 mask_x					: 2;
			u32 cond					: 3;
			u32 cond_test_enable		: 1;
			u32 cond_update_enable_0	: 1;
			u32 dst_tmp					: 6;
			u32 src0_abs				: 1;
			u32 src1_abs				: 1;
			u32 src2_abs				: 1;
			u32 addr_reg_sel_1			: 1;
			u32 cond_reg_sel_1			: 1;
			u32 staturate				: 1;
			u32 index_input				: 1;
			u32							: 1;
			u32 cond_update_enable_1	: 1;
			u32 vec_result				: 1;
			u32							: 1;
		};
	} d0;

	union D1
	{
		u32 HEX;

		struct
		{
			u32 src0h		: 8;
			u32 input_src	: 4;
			u32 const_src	: 10;
			u32 vec_opcode	: 5;
			u32 sca_opcode	: 5;
		};
	} d1;

	union D2
	{
		u32 HEX;

		struct
		{
			u32 src2h	: 6;
			u32 src1	: 17;
			u32 src0l	: 9;
		};
		struct
		{
			u32 iaddrh	: 6;
			u32			: 26;
		};
	} d2;

	union D3
	{
		u32 HEX;

		struct
		{
			u32	end				: 1;
			u32 index_const		: 1;
			u32 dst				: 5;
			u32 sca_dst_tmp		: 6;
			u32 vec_writemask_w : 1;
			u32 vec_writemask_z : 1;
			u32 vec_writemask_y : 1;
			u32 vec_writemask_x : 1;
			u32 sca_writemask_w : 1;
			u32 sca_writemask_z : 1;
			u32 sca_writemask_y : 1;
			u32 sca_writemask_x : 1;
			u32 src2l			: 11;
		};
		struct
		{
			u32					: 29;
			u32 iaddrl			: 3;
		};
	} d3;

	union SRC
	{
		union
		{
			u32 HEX;

			struct
			{
				u32 src0l		: 9;
				u32 src0h		: 8;
			};

			struct
			{
				u32 src1 : 17;
			};

			struct
			{
				u32 src2l		: 11;
				u32 src2h		: 6;
			};
		};

		struct
		{
			u32 reg_type	: 2;
			u32 tmp_src		: 6;
			u32 swz_w		: 2;
			u32 swz_z		: 2;
			u32 swz_y		: 2;
			u32 swz_x		: 2;
			u32 neg			: 1;
		};
	} src[3];

	struct FuncInfo
	{
		u32 offset;
		std::string name;
	};

	std::vector<std::string> m_body;

	ArrayF<FuncInfo> m_funcs;

	//wxString main;
	std::string& m_shader;
	Array<u32>& m_data;
	GLParamArray& m_parr;

	GLVertexDecompilerThread(Array<u32>& data, std::string& shader, GLParamArray& parr)
		: ThreadBase("Vertex Shader Decompiler Thread")
		, m_data(data)
		, m_shader(shader)
		, m_parr(parr)
	{
		m_funcs.Add(new FuncInfo());
		m_funcs[0].offset = 0;
		m_funcs[0].name = "main";
		m_funcs.Add(new FuncInfo());
		m_funcs[1].offset = 0;
		m_funcs[1].name = "func0";
		//m_cur_func->body = "\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
	}

	std::string GetMask(bool is_sca);
	std::string GetVecMask();
	std::string GetScaMask();
	std::string GetDST(bool is_sca = false);
	std::string GetSRC(const u32 n, bool is_sca = false);
	std::string GetFunc();
	void AddCode(bool is_sca, const std::string& code, bool src_mask = true, bool set_dst = true, bool set_cond = true);
	void AddVecCode(const std::string& code, bool src_mask = true, bool set_dst = true);
	void AddScaCode(const std::string& code, bool set_dst = true, bool set_cond = true);
	std::string BuildFuncBody(const FuncInfo& func);
	std::string BuildCode();

	virtual void Task();
};

struct GLVertexProgram
{ 
	GLVertexDecompilerThread* m_decompiler_thread;

	GLVertexProgram();
	~GLVertexProgram();

	GLParamArray parr;
	u32 id;
	std::string shader;

	void Wait()
	{
		if(m_decompiler_thread && m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Join();
		}
	}

	void Decompile(RSXVertexProgram& prog);
	void Compile();
	void Delete();
};
