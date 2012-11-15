#pragma once
#include "ShaderParam.h"

struct VertexDecompilerThread : public wxThread
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
			u32 sca_dst			: 5;
			u32 sca_result		: 1;
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
		u16 HEX;

		struct
		{
			u16 reg_type	: 2;
			u16 tmp_src		: 5;
			u16 swz_w		: 2;
			u16 swz_z		: 2;
			u16 swz_y		: 2;
			u16 swz_x		: 2;
			u16 neg			: 1;
		};
	} src[3];

	wxString main;
	wxString& m_shader;
	Array<u32>& m_data;
	ParamArray& m_parr;

	VertexDecompilerThread(Array<u32>& data, wxString& shader, ParamArray& parr)
		: wxThread(wxTHREAD_JOINABLE)
		, m_data(data)
		, m_shader(shader)
		, m_parr(parr)
	{
	}

	wxString GetMask();
	wxString GetDST();
	wxString GetSRC(const u32 n);
	void AddCode(wxString code, bool src_mask = true);
	wxString BuildCode();

	ExitCode Entry();
};

struct VertexProgram
{
	wxString shader;
	u32 id;
	VertexDecompilerThread* m_decompiler_thread;

	VertexProgram();
	~VertexProgram();

	struct Constant4
	{
		u32 id;
		float x, y, z, w;
	};

	Array<Constant4> constants4;

	Array<u32> data;
	ParamArray parr;

	void Wait() { if(m_decompiler_thread && m_decompiler_thread->IsRunning()) m_decompiler_thread->Wait(); }
	void Decompile();
	void Compile();
	void Delete();
};

struct VertexData
{
	u32 index;
	u32 frequency;
	u32 stride;
	u32 size;
	u32 type;
	u32 addr;

	Array<u8> data;

	VertexData() { memset(this, 0, sizeof(*this)); }
	~VertexData() { data.Clear(); }

	bool IsEnabled() { return size > 0; }

	void Load(u32 start, u32 count);

	u32 GetTypeSize();
};