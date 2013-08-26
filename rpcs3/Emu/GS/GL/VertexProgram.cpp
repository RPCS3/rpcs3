#include "stdafx.h"
#include "VertexProgram.h"

wxString VertexDecompilerThread::GetMask(bool is_sca)
{
	wxString ret = wxEmptyString;

	if(is_sca)
	{
		if(d3.sca_writemask_x) ret += "x";
		if(d3.sca_writemask_y) ret += "y";
		if(d3.sca_writemask_z) ret += "z";
		if(d3.sca_writemask_w) ret += "w";
	}
	else
	{
		if(d3.vec_writemask_x) ret += "x";
		if(d3.vec_writemask_y) ret += "y";
		if(d3.vec_writemask_z) ret += "z";
		if(d3.vec_writemask_w) ret += "w";
	}

	return ret.IsEmpty() || ret == "xyzw" ? wxEmptyString : ("." + ret);
}

wxString VertexDecompilerThread::GetVecMask()
{
	return GetMask(false);
}

wxString VertexDecompilerThread::GetScaMask()
{
	return GetMask(true);
}

wxString VertexDecompilerThread::GetDST(bool isSca)
{
	static const wxString reg_table[] = 
	{
		"gl_Position",
		"col0", "col1",
		"bfc0", "bfc1",
		"gl_ClipDistance[%d]",
		"gl_ClipDistance[%d]",
		"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7"
	};

	wxString ret = wxEmptyString;

	switch(isSca ? 0x1f : d3.dst)
	{
	case 0x0: case 0x5: case 0x6:
		ret += reg_table[d3.dst];
	break;

	case 0x1f:
		ret += m_parr.AddParam(PARAM_NONE, "vec4", wxString::Format("tmp%u", isSca ? d3.sca_dst_tmp : d0.dst_tmp));
	break;

	default:
		if(d3.dst < WXSIZEOF(reg_table))
		{
			ret += m_parr.AddParam(PARAM_OUT, "vec4", reg_table[d3.dst]);
		}
		else
		{
			ConLog.Error("Bad dst reg num: %d", d3.dst);
			ret += m_parr.AddParam(PARAM_OUT, "vec4", "unk");
		}
	break;
	}

	return ret;
}

wxString VertexDecompilerThread::GetSRC(const u32 n, bool isSca)
{
	static const wxString reg_table[] = 
	{
		"in_pos", "in_weight", "in_normal",
		"in_col0", "in_col1",
		"in_fogc",
		"in_6", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	wxString ret = wxEmptyString;

	switch(src[n].reg_type)
	{
	case 1: //temp
		ret += m_parr.AddParam(PARAM_NONE, "vec4", wxString::Format("tmp%u", src[n].tmp_src));
	break;
	case 2: //input
		if(d1.input_src < WXSIZEOF(reg_table))
		{
			ret += m_parr.AddParam(PARAM_IN, "vec4", reg_table[d1.input_src], d1.input_src);
		}
		else
		{
			ConLog.Error("Bad input src num: %d", d1.input_src);
			ret += m_parr.AddParam(PARAM_IN, "vec4", "in_unk", d1.input_src);
		}
	break;
	case 3: //const
		ret += m_parr.AddParam(PARAM_UNIFORM, "vec4", wxString::Format("vc%u", d1.const_src));
	break;

	default:
		ConLog.Error("Bad src%u reg type: %d", n, src[n].reg_type);
		Emu.Pause();
	break;
	}

	static const wxString f = "xyzw";

	if (isSca)
	{
		assert(src[n].swz_x == src[n].swz_y);
		assert(src[n].swz_z == src[n].swz_w);
		assert(src[n].swz_x == src[n].swz_z);

		ret += '.';
		ret += f[src[n].swz_x];
	}
	else
	{
		wxString swizzle = wxEmptyString;

		swizzle += f[src[n].swz_x];
		swizzle += f[src[n].swz_y];
		swizzle += f[src[n].swz_z];
		swizzle += f[src[n].swz_w];

		if(swizzle != f) ret += '.' + swizzle;
	}

	bool abs;
	
	switch(n)
	{
	case 0: abs = d0.src0_abs; break;
	case 1: abs = d0.src1_abs; break;
	case 2: abs = d0.src2_abs; break;
	}
	
	if(abs) ret = "abs(" + ret + ")";
	if(src[n].neg) ret = "-" + ret;

	return ret;
}

void VertexDecompilerThread::AddCode(bool is_sca, wxString code, bool src_mask)
{
	if(d0.cond == 0) return;
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	wxString cond;

	if(d0.cond != (lt | gt | eq))
	{
		if((d0.cond & (lt | gt)) == (lt | gt))
		{
			cond = "!=";
		}
		else
		{
			if(d0.cond & lt) cond = "<";
			else if(d0.cond & gt) cond = ">";
			else if(d0.cond & eq) cond = "=";

			if(d0.cond & eq) cond += "=";
		}

		//ConLog.Error("cond! %d (%d %s %d %d)", d0.cond, d0.dst_tmp, cond, d1.input_src, d1.const_src);
		cond = wxString::Format("if(tmp%d.x %s 0) ", d0.dst_tmp, cond);
	}

	wxString value = src_mask ? code + GetMask(is_sca) : code;

	if(d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	wxString dest;
	if(d3.dst == 5 || d3.dst == 6)
	{
		int num = d3.dst == 5 ? 0 : 3;

		if(d3.vec_writemask_x)
		{
			ConLog.Error("Bad clip mask.");
		}

		//if(d3.vec_writemask_y) num += 0;
		if(d3.vec_writemask_z) num += 1;
		else if(d3.vec_writemask_w) num += 2;

		dest = wxString::Format(GetDST(is_sca), num);
	}
	else
	{
		dest = GetDST(is_sca) + GetMask(is_sca);
	}

	code = cond + dest + " = " + value;

	main += "\t" + code + ";\n";
}

void VertexDecompilerThread::AddVecCode(const wxString& code, bool src_mask)
{
	AddCode(false, code, src_mask);
}

void VertexDecompilerThread::AddScaCode(const wxString& code)
{
	AddCode(true, code, false);
}

wxString VertexDecompilerThread::BuildCode()
{
	wxString p = wxEmptyString;

	for(u32 i=0; i<m_parr.params.GetCount(); ++i)
	{
		p += m_parr.params[i].Format();
	}
		
	static const wxString& prot = 
		"#version 330\n"
		"\n"
		"%s\n"
		"void main()\n{\n\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n%s}\n";

	return wxString::Format(prot, p, main);
}

void VertexDecompilerThread::Task()
{
	for(u32 i=0;;)
	{
		d0.HEX = m_data[i++];
		d1.HEX = m_data[i++];
		d2.HEX = m_data[i++];
		d3.HEX = m_data[i++];

		src[0].src0l = d2.src0l;
		src[0].src0h = d1.src0h;
		src[1].src1 = d2.src1;
		src[2].src2l = d3.src2l;
		src[2].src2h = d2.src2h;

		switch(d1.sca_opcode)
		{
		case 0x00: break; // NOP
		case 0x01: AddScaCode(GetSRC(2, true)); break; // MOV
		case 0x02: AddScaCode("1 / (" + GetSRC(2, true) + ")"); break; // RCP
		case 0x03: AddScaCode("clamp(1 / (" + GetSRC(2, true) + "), 5.42101e-20, 1.884467e19)"); break; // RCC
		case 0x04: AddScaCode("inversesqrt(" + GetSRC(2, true) + ")"); break; // RSQ
		case 0x05: AddScaCode("exp(" + GetSRC(2, true) + ")"); break; // EXP
		case 0x06: AddScaCode("log(" + GetSRC(2, true) + ")"); break; // LOG
		//case 0x07: break; // LIT
		//case 0x08: break; // BRA
		//case 0x09: break; // BRI : works differently (BRI o[1].x(TR) L0;)
		//case 0x0a: break; // CAL : works same as BRI
		//case 0x0b: break; // CLI : works same as BRI
		//case 0x0c: break; // RET : works like BRI but shorter (RET o[1].x(TR);)
		case 0x0d: AddScaCode("log2(" + GetSRC(2, true) + ")"); break; // LG2
		case 0x0e: AddScaCode("exp2(" + GetSRC(2, true) + ")"); break; // EX2
		case 0x0f: AddScaCode("sin(" + GetSRC(2, true) + ")"); break; // SIN
		case 0x10: AddScaCode("cos(" + GetSRC(2, true) + ")"); break; // COS
		//case 0x11: break; // BRB : works differently (BRB o[1].x !b0, L0;)
		//case 0x12: break; // CLB : works same as BRB
		//case 0x13: break; // PSH : works differently (PSH o[1].x A0;)
		//case 0x14: break; // POP : works differently (POP o[1].x;)

		default:
			ConLog.Error("Unknown vp sca_opcode 0x%x", d1.sca_opcode);
			Emu.Pause();
		break;
		}

		switch(d1.vec_opcode)
		{
		case 0x00: break; //NOP
		case 0x01: AddVecCode(GetSRC(0)); break; //MOV
		case 0x02: AddVecCode("(" + GetSRC(0) + " * " + GetSRC(1) + ")"); break; //MUL
		case 0x03: AddVecCode("(" + GetSRC(0) + " + " + GetSRC(2) + ")"); break; //ADD
		case 0x04: AddVecCode("(" + GetSRC(0) + " * " + GetSRC(1) + " + " + GetSRC(2) + ")"); break; //MAD
		case 0x05: AddVecCode("vec4(dot(" + GetSRC(0) + ".xyz, " + GetSRC(1) + ".xyz)).xxxx"); break; //DP3
		case 0x06: AddVecCode("vec4(dot(vec4(" + GetSRC(0) + ".xyz, 1), " + GetSRC(1) + ")).xxxx"); break; //DPH
		case 0x07: AddVecCode("vec4(dot(" + GetSRC(0) + ", " + GetSRC(1) + ")).xxxx"); break; //DP4
		case 0x08: AddVecCode("vec4(distance(" + GetSRC(0) + ", " + GetSRC(1) + ")).xxxx"); break; //DST
		case 0x09: AddVecCode("min(" + GetSRC(0) + ", " + GetSRC(1) + ")"); break; //MIN
		case 0x0a: AddVecCode("max(" + GetSRC(0) + ", " + GetSRC(1) + ")"); break; //MAX
		case 0x0b: AddVecCode("vec4(lessThan(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SLT
		case 0x0c: AddVecCode("vec4(greaterThanEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SGE
		case 0x0e: AddVecCode("fract(" + GetSRC(0) + ")"); break; //FRC
		case 0x0f: AddVecCode("floor(" + GetSRC(0) + ")"); break; //FLR
		case 0x10: AddVecCode("vec4(equal(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SEQ
		//case 0x11: AddVecCode("vec4(0.0f, 0.0f, 0.0f, 0.0f)"); break; //SFL
		case 0x12: AddVecCode("vec4(greaterThan(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SGT
		case 0x13: AddVecCode("vec4(lessThanEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SLE
		case 0x14: AddVecCode("vec4(notEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SNE
		//case 0x15: AddVecCode("vec4(1.0f, 1.0f, 1.0f, 1.0f)"); break; //STR
		case 0x16: AddVecCode("sign(" + GetSRC(0) + ")"); break; //SSG

		default:
			ConLog.Error("Unknown vp opcode 0x%x", d1.vec_opcode);
			Emu.Pause();
		break;
		}

		if(d3.end) break;
	}

	m_shader = BuildCode();

	main = wxEmptyString;
}

VertexProgram::VertexProgram()
	: m_decompiler_thread(nullptr)
	, id(0)
{
}

VertexProgram::~VertexProgram()
{
	if(m_decompiler_thread)
	{
		Wait();
		if(m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	Delete();
}

void VertexProgram::Decompile()
{
#if 0
	VertexDecompilerThread(data, shader, parr).Entry();
#else
	if(m_decompiler_thread)
	{
		Wait();
		if(m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	m_decompiler_thread = new VertexDecompilerThread(data, shader, parr);
	m_decompiler_thread->Start();
#endif
}

void VertexProgram::Compile()
{
	if(id) glDeleteShader(id);

	id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.c_str();
	const int strlen = shader.Len();

	glShaderSource(id, 1, &str, &strlen);
	glCompileShader(id);
	
	GLint r = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &r);
	if(r != GL_TRUE)
	{
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &r);

		if(r)
		{
			char* buf = new char[r+1];
			GLsizei len;
			memset(buf, 0, r+1);
			glGetShaderInfoLog(id, r, &len, buf);
			ConLog.Error("Failed to compile vertex shader: %s", buf);
			delete[] buf;
		}

		ConLog.Write(shader);
		Emu.Pause();
	}
	//else ConLog.Write("Vertex shader compiled successfully!");

}

void VertexProgram::Delete()
{
	data.Clear();
	parr.params.Clear();
	shader.Clear();

	if(id)
	{
		glDeleteShader(id);
		id = 0;
	}
}

VertexData::VertexData()
	: frequency(0)
	, stride(0)
	, size(0)
	, type(0)
	, addr(0)
	, data()
{
}

void VertexData::Reset()
{
	frequency = 0;
	stride = 0;
	size = 0;
	type = 0;
	addr = 0;
	data.ClearF();
}

void VertexData::Load(u32 start, u32 count)
{
	if(!addr) return;

	const u32 tsize = GetTypeSize();

	data.SetCount((start + count) * tsize * size);

	for(u32 i=start; i<start + count; ++i)
	{
		const u8* src = Memory.GetMemFromAddr(addr) + stride * i;
		u8* dst = &data[i * tsize * size];

		switch(tsize)
		{
		case 1:
		{
			memcpy(dst, src, size);
		}
		break;

		case 2:
		{
			const u16* c_src = (const u16*)src;
			u16* c_dst = (u16*)dst;
			for(u32 j=0; j<size; ++j) *c_dst++ = re(*c_src++);
		}
		break;

		case 4:
		{
			const u32* c_src = (const u32*)src;
			u32* c_dst = (u32*)dst;
			for(u32 j=0; j<size; ++j) *c_dst++ = re(*c_src++);
		}
		break;
		}
	}
}

u32 VertexData::GetTypeSize()
{
	switch (type)
	{
	case 1: return 2;
	case 2: return 4;
	case 3: return 2;
	case 4: return 1;
	case 5: return 2;
	case 7: return 1;
	}

	ConLog.Error("Bad vertex data type! %d", type);
	return 1;
}