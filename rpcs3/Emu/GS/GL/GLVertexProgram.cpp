#include "stdafx.h"
#include "GLVertexProgram.h"

wxString GLVertexDecompilerThread::GetMask(bool is_sca)
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

	return ret.IsEmpty() || ret == "xyzw" ? wxString(wxEmptyString) : ("." + ret);
}

wxString GLVertexDecompilerThread::GetVecMask()
{
	return GetMask(false);
}

wxString GLVertexDecompilerThread::GetScaMask()
{
	return GetMask(true);
}

wxString GLVertexDecompilerThread::GetDST(bool isSca)
{
	static const std::string reg_table[] = 
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
		ret += m_parr.AddParam(PARAM_NONE, "vec4", std::string("tmp") + std::to_string(isSca ? d3.sca_dst_tmp : d0.dst_tmp));
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

wxString GLVertexDecompilerThread::GetSRC(const u32 n, bool isSca)
{
	static const std::string reg_table[] = 
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
		ret += m_parr.AddParam(PARAM_NONE, "vec4", std::string("tmp") + std::to_string(src[n].tmp_src));
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
		ret += m_parr.AddParam(PARAM_UNIFORM, "vec4", std::string("vc") + std::to_string(d1.const_src));
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

void GLVertexDecompilerThread::AddCode(bool is_sca, wxString code, bool src_mask, bool set_dst, bool set_cond)
{
	if(d0.cond == 0) return;
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	static const char* cond_string_table[(lt | gt | eq) + 1] =
	{
		"error",
		"lessThan",
		"equal",
		"lessThanEqual",
		"greaterThan",
		"notEqual",
		"greaterThanEqual",
		"error"
	};

	wxString cond;

	if((set_cond || d0.cond_test_enable) && d0.cond != (lt | gt | eq))
	{
		static const char f[4] = {'x', 'y', 'z', 'w'};

		std::string swizzle;
		swizzle += f[d0.mask_x];
		swizzle += f[d0.mask_y];
		swizzle += f[d0.mask_z];
		swizzle += f[d0.mask_w];

		swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

		cond = wxString::Format("if(all(%s(rc%s, vec4(0.0)%s))) ", wxString(cond_string_table[d0.cond]).wx_str(), wxString(swizzle).wx_str(), wxString(swizzle).wx_str());
	}

	wxString mask = GetMask(is_sca);
	wxString value = src_mask ? code + mask : code;

	if(is_sca && d0.vec_result)
	{
		value = "vec4(" + value + ")" + mask;
	}

	if(d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	if(set_dst)
	{
		wxString dest;
		if(d0.cond_update_enable_0)
		{
			dest = m_parr.AddParam(PARAM_NONE, "vec4", "rc", "vec4(0.0)") + mask;
		}
		else if(d3.dst == 5 || d3.dst == 6)
		{
			if(d3.vec_writemask_x)
			{
				dest = m_parr.AddParam(PARAM_OUT, "vec4", "fogc") + mask;
			}
			else
			{
				int num = d3.dst == 5 ? 0 : 3;

				//if(d3.vec_writemask_y) num += 0;
				if(d3.vec_writemask_z) num += 1;
				else if(d3.vec_writemask_w) num += 2;

				dest = wxString::Format(GetDST(is_sca) + "/*" + mask + "*/", num);
			}
		}
		else
		{
			dest = GetDST(is_sca) + mask;
		}

		code = cond + dest + " = " + value;
	}
	else
	{
		code = cond + value;
	}

	m_body.Add(code + ";");
}

wxString GLVertexDecompilerThread::GetFunc()
{
	u32 offset = (d2.iaddrh << 3) | d3.iaddrl;
	wxString name = wxString::Format("func%u", offset);

	for(uint i=0; i<m_funcs.GetCount(); ++i)
	{
		if(m_funcs[i].name.Cmp(name) == 0)
			return name + "()";
	}

	uint idx = m_funcs.Add(new FuncInfo());
	m_funcs[idx].offset = offset;
	m_funcs[idx].name = name;

	return name + "()";
}

void GLVertexDecompilerThread::AddVecCode(const wxString& code, bool src_mask, bool set_dst)
{
	AddCode(false, code, src_mask, set_dst);
}

void GLVertexDecompilerThread::AddScaCode(const wxString& code, bool set_dst, bool set_cond)
{
	AddCode(true, code, false, set_dst, set_cond);
}

wxString GLVertexDecompilerThread::BuildFuncBody(const FuncInfo& func)
{
	wxString result;

	for(uint i=func.offset; i<m_body.GetCount(); ++i)
	{
		if(i != func.offset)
		{
			uint call_func = -1;
			for(uint j=0; j<m_funcs.GetCount(); ++j)
			{
				if(m_funcs[j].offset == i)
				{
					call_func = j;
					break;
				}
			}

			if(call_func != -1)
			{
				result += '\t' + m_funcs[call_func].name + "();\n";
				break;
			}
		}

		result += '\t' + m_body[i] + '\n';
	}

	return result;
}

wxString GLVertexDecompilerThread::BuildCode()
{
	wxString p = wxEmptyString;

	for(u32 i=0; i<m_parr.params.GetCount(); ++i)
	{
		p += m_parr.params[i].Format();
	}

	wxString fp = wxEmptyString;

	for(int i=m_funcs.GetCount() - 1; i>0; --i)
	{
		fp += wxString::Format("void %s();\n", m_funcs[i].name.wx_str());
	}

	wxString f = wxEmptyString;

	f += wxString::Format("void %s()\n{\n\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n%sgl_Position = gl_Position * scaleOffsetMat;\n}\n", m_funcs[0].name.wx_str(), BuildFuncBody(m_funcs[0]).wx_str());

	for(uint i=1; i<m_funcs.GetCount(); ++i)
	{
		f += wxString::Format("\nvoid %s()\n{\n%s}\n", m_funcs[i].name.wx_str(), BuildFuncBody(m_funcs[i]).wx_str());
	}

	static const wxString& prot = 
		"#version 330\n"
		"\n"
		"uniform mat4 scaleOffsetMat = mat4(1.0);\n"
		"%s\n"
		"%s\n"
		"%s";

	return wxString::Format(prot, p.wx_str(), fp.wx_str(), f.wx_str());
}

void GLVertexDecompilerThread::Task()
{
	m_parr.params.Clear();

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
		case 0x02: AddScaCode("1.0 / " + GetSRC(2, true)); break; // RCP
		case 0x03: AddScaCode("clamp(1.0 / " + GetSRC(2, true) + ", 5.42101e-20, 1.884467e19)"); break; // RCC
		case 0x04: AddScaCode("inversesqrt(" + GetSRC(2, true) + ")"); break; // RSQ
		case 0x05: AddScaCode("exp(" + GetSRC(2, true) + ")"); break; // EXP
		case 0x06: AddScaCode("log(" + GetSRC(2, true) + ")"); break; // LOG
		//case 0x07: break; // LIT
		case 0x08: AddScaCode("{ /*BRA*/ " + GetFunc() + "; return; }", false, true); break; // BRA
		case 0x09: AddScaCode("{ " + GetFunc() + "; return; }", false, true); break; // BRI : works differently (BRI o[1].x(TR) L0;)
		case 0x0a: AddScaCode("/*CAL*/ " + GetFunc(), false, true); break; // CAL : works same as BRI
		case 0x0b: AddScaCode("/*CLI*/ " + GetFunc(), false, true); break; // CLI : works same as BRI
		case 0x0c: AddScaCode("return", false, true); break; // RET : works like BRI but shorter (RET o[1].x(TR);)
		case 0x0d: AddScaCode("log2(" + GetSRC(2, true) + ")"); break; // LG2
		case 0x0e: AddScaCode("exp2(" + GetSRC(2, true) + ")"); break; // EX2
		case 0x0f: AddScaCode("sin(" + GetSRC(2, true) + ")"); break; // SIN
		case 0x10: AddScaCode("cos(" + GetSRC(2, true) + ")"); break; // COS
		//case 0x11: break; // BRB : works differently (BRB o[1].x !b0, L0;)
		//case 0x12: break; // CLB : works same as BRB
		//case 0x13: break; // PSH : works differently (PSH o[1].x A0;)
		//case 0x14: break; // POP : works differently (POP o[1].x;)

		default:
			m_body.Add(wxString::Format("//Unknown vp sca_opcode 0x%x", d1.sca_opcode));
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
		case 0x05: AddVecCode("vec2(dot(" + GetSRC(0) + ".xyz, " + GetSRC(1) + ".xyz), 0).xxxx"); break; //DP3
		case 0x06: AddVecCode("vec2(dot(vec4(" + GetSRC(0) + ".xyz, 1), " + GetSRC(1) + "), 0).xxxx"); break; //DPH
		case 0x07: AddVecCode("vec2(dot(" + GetSRC(0) + ", " + GetSRC(1) + "), 0).xxxx"); break; //DP4
		case 0x08: AddVecCode("vec2(distance(" + GetSRC(0) + ", " + GetSRC(1) + "), 0).xxxx"); break; //DST
		case 0x09: AddVecCode("min(" + GetSRC(0) + ", " + GetSRC(1) + ")"); break; //MIN
		case 0x0a: AddVecCode("max(" + GetSRC(0) + ", " + GetSRC(1) + ")"); break; //MAX
		case 0x0b: AddVecCode("vec4(lessThan(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SLT
		case 0x0c: AddVecCode("vec4(greaterThanEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SGE
		case 0x0e: AddVecCode("fract(" + GetSRC(0) + ")"); break; //FRC
		case 0x0f: AddVecCode("floor(" + GetSRC(0) + ")"); break; //FLR
		case 0x10: AddVecCode("vec4(equal(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SEQ
		//case 0x11: AddVecCode("vec4(equal(" + GetSRC(0) + ", vec4(0, 0, 0, 0)))"); break; //SFL
		case 0x12: AddVecCode("vec4(greaterThan(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SGT
		case 0x13: AddVecCode("vec4(lessThanEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SLE
		case 0x14: AddVecCode("vec4(notEqual(" + GetSRC(0) + ", " + GetSRC(1) + "))"); break; //SNE
		//case 0x15: AddVecCode("vec4(notEqual(" + GetSRC(0) + ", vec4(0, 0, 0, 0)))"); break; //STR
		case 0x16: AddVecCode("sign(" + GetSRC(0) + ")"); break; //SSG

		default:
			m_body.Add(wxString::Format("//Unknown vp opcode 0x%x", d1.vec_opcode));
			ConLog.Error("Unknown vp opcode 0x%x", d1.vec_opcode);
			Emu.Pause();
		break;
		}

		if(d3.end)
		{
			if(i < m_data.GetCount())
				ConLog.Error("Program end before buffer end.");

			break;
		}
	}

	m_shader = BuildCode();

	m_body.Clear();
	m_funcs.RemoveAt(1, m_funcs.GetCount() - 1);
}

GLVertexProgram::GLVertexProgram()
	: m_decompiler_thread(nullptr)
	, id(0)
{
}

GLVertexProgram::~GLVertexProgram()
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

void GLVertexProgram::Decompile(RSXVertexProgram& prog)
{
#if 0
	GLVertexDecompilerThread(data, shader, parr).Entry();
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

	m_decompiler_thread = new GLVertexDecompilerThread(prog.data, shader, parr);
	m_decompiler_thread->Start();
#endif
}

void GLVertexProgram::Compile()
{
	if(id) glDeleteShader(id);

	id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.mb_str();
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
			ConLog.Error("Failed to compile vertex shader: %s", wxString(buf).wx_str());
			delete[] buf;
		}

		ConLog.Write(shader);
		Emu.Pause();
	}
	//else ConLog.Write("Vertex shader compiled successfully!");

}

void GLVertexProgram::Delete()
{
	parr.params.Clear();
	shader.Clear();

	if(id)
	{
		glDeleteShader(id);
		id = 0;
	}
}
