#pragma once
#include "ShaderParam.h"

struct FragmentDecompilerThread : public ThreadBase
{
	union OPDEST
	{
		u32 HEX;

		struct
		{
			u32 end					: 1;
			u32 dest_reg			: 6;
			u32 fp16				: 1;
			u32 set_cond			: 1;
			u32 mask_x				: 1;
			u32 mask_y				: 1;
			u32 mask_z				: 1;
			u32 mask_w				: 1;
			u32 src_attr_reg_num	: 4;
			u32 tex_num				: 4;
			u32 exp_tex				: 1;
			u32 prec 				: 2;
			u32 opcode 				: 6;
			u32 no_dest 			: 1;
			u32 saturate 			: 1;
		};
	} dst;

	union SRC0
	{
		u32 HEX;

		struct
		{
			u32 reg_type			: 2;
			u32 tmp_reg_index		: 6;
			u32 fp16				: 1;
			u32 swizzle_x			: 2;
			u32 swizzle_y			: 2;
			u32 swizzle_z			: 2;
			u32 swizzle_w			: 2;
			u32 neg					: 1;
			u32 exec_if_lt			: 1;
			u32 exec_if_eq			: 1;
			u32 exec_if_gr			: 1;
			u32 cond_swizzle_x		: 2;
			u32 cond_swizzle_y		: 2;
			u32 cond_swizzle_z		: 2;
			u32 cond_swizzle_w		: 2;
			u32 abs					: 1;
			u32 cond_mod_reg_index	: 1;
			u32 cond_reg_index		: 1;
		};
	} src0;

	union SRC1
	{
		u32 HEX;

		struct
		{
			u32 reg_type			: 2;
			u32 tmp_reg_index		: 6;
			u32 fp16				: 1;
			u32 swizzle_x			: 2;
			u32 swizzle_y			: 2;
			u32 swizzle_z			: 2;
			u32 swizzle_w			: 2;
			u32 neg					: 1;
			u32 abs					: 1;
			u32 input_mod_src0		: 3;
			u32						: 6;
			u32 scale				: 3;
			u32 opcode_is_branch	: 1;
		};
	} src1;

	union SRC2
	{
		u32 HEX;

		struct
		{
			u32 reg_type		: 2;
			u32 tmp_reg_index	: 6;
			u32 fp16			: 1;
			u32 swizzle_x		: 2;
			u32 swizzle_y		: 2;
			u32 swizzle_z		: 2;
			u32 swizzle_w		: 2;
			u32 neg				: 1;
			u32 abs				: 1;
			u32 addr_reg		: 11;
			u32 use_index_reg	: 1;
			u32 perspective_corr: 1;
		};
	} src2;

	wxString main;
	wxString& m_shader;
	ParamArray& m_parr;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;

	FragmentDecompilerThread(wxString& shader, ParamArray& parr, u32 addr, u32& size)
		: ThreadBase(false, "Fragment Shader Decompiler Thread")
		, m_shader(shader)
		, m_parr(parr)
		, m_addr(addr)
		, m_size(size) 
		, m_const_index(0)
	{
		m_size = 0;
	}

	wxString GetMask();

	void AddCode(wxString code, bool append_mask = true);
	wxString AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	wxString AddCond(int fp16);
	wxString AddConst();
	wxString AddTex();

	template<typename T> wxString GetSRC(T src);
	wxString BuildCode();

	virtual void Task();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
};

struct ShaderProgram
{
	ShaderProgram();
	~ShaderProgram();

	FragmentDecompilerThread* m_decompiler_thread;

	ParamArray parr;

	u32 size;
	u32 addr;
	u32 offset;
	wxString shader;

	u32 id;
	
	void Wait()
	{
		if(m_decompiler_thread && m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Wait();
		}
	}
	void Decompile();
	void Compile();

	void Delete();
};