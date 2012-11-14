#pragma once
#include "Program.h"

struct BufferInfo
{
	u32 prog_id;
	u32 fp_id;
	u32 vp_id;
	Array<u8> fp_data;
	Array<u32> vp_data;
	wxString fp_shader;
	wxString vp_shader;
};

struct ProgramBuffer
{
	Array<BufferInfo> m_buf;

	int SearchFp(ShaderProgram& fp);
	int SearchVp(VertexProgram& vp);

	bool CmpVP(const u32 a, const u32 b) const;
	bool CmpFP(const u32 a, const u32 b) const;

	u32 GetProg(u32 fp, u32 vp) const;

	void Add(Program& prog, ShaderProgram& fp, VertexProgram& vp);
	void Clear();
};