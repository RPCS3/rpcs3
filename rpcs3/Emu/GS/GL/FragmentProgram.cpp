#include "stdafx.h"
#include "FragmentProgram.h"

void FragmentDecompilerThread::AddCode(wxString code)
{
	if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt) return;

	wxString cond = wxEmptyString;

	if(!src0.exec_if_gr || !src0.exec_if_lt || !src0.exec_if_eq)
	{
		if(src0.exec_if_gr)
		{
			cond = ">";
		}
		else if(src0.exec_if_lt)
		{
			cond = "<";
		}
		else if(src0.exec_if_eq)
		{
			cond = "=";
		}

		if(src0.exec_if_eq)
		{
			cond += "=";
		}
		else
		{
			if(src0.exec_if_gr && src0.exec_if_lt)
			{
				cond = "!=";
			}
		}
	}

	if(cond.Len())
	{

		static const char f[4] = {'x', 'y', 'z', 'w'};
		wxString swizzle = wxEmptyString;
		swizzle += f[src0.cond_swizzle_x];
		swizzle += f[src0.cond_swizzle_y];
		swizzle += f[src0.cond_swizzle_z];
		swizzle += f[src0.cond_swizzle_w];
		cond = wxString::Format("if(rc.%s %s 0.0) ", swizzle, cond);
		//ConLog.Error("cond! [eq: %d  gr: %d  lt: %d] (%s)", src0.exec_if_eq, src0.exec_if_gr, src0.exec_if_lt, cond);
		//Emu.Pause();
		//return;
	}

	if(src1.scale)
	{
		switch(src1.scale)
		{
		case 1: code = "(" + code + " * 2)"; break;
		case 2: code = "(" + code + " * 4)"; break;
		case 3: code = "(" + code + " * 8)"; break;
		case 5: code = "(" + code + " / 2)"; break;
		case 6: code = "(" + code + " / 4)"; break;
		case 7: code = "(" + code + " / 8)"; break;

		default:
			ConLog.Error("Bad scale: %d", src1.scale);
			Emu.Pause();
		break;
		}
	}

	if(dst.saturate)
	{
		code = "clamp(" + code + ", 0.0, 1.0)";
	}

	code = cond + (dst.set_cond ? AddCond(dst.fp16) : AddReg(dst.dest_reg, dst.fp16)) + GetMask() + " = " + code + GetMask();

	main += "\t" + code + ";\n";
}

wxString FragmentDecompilerThread::GetMask()
{
	wxString ret = wxEmptyString;

	static const char dst_mask[2][4] =
	{
		{'x', 'y', 'z', 'w'},
		{'r', 'g', 'b', 'a'}
	};

	if(dst.mask_x) ret += dst_mask[dst.dest_reg == 0][0];
	if(dst.mask_y) ret += dst_mask[dst.dest_reg == 0][1];
	if(dst.mask_z) ret += dst_mask[dst.dest_reg == 0][2];
	if(dst.mask_w) ret += dst_mask[dst.dest_reg == 0][3];

	return ret.IsEmpty() || strncmp(ret, dst_mask[dst.dest_reg == 0], 4) == 0 ? wxEmptyString : ("." + ret);
}

wxString FragmentDecompilerThread::AddReg(u32 index, int fp16)
{
	//if(!index) return "gl_FragColor";
	return m_parr.AddParam((index || fp16) ? PARAM_NONE : PARAM_OUT, "vec4",
		wxString::Format((fp16 ? "h%d" : "r%d"), index), (index || fp16) ? -1 : 0);
}

wxString FragmentDecompilerThread::AddCond(int fp16)
{
	return m_parr.AddParam(PARAM_NONE , "vec4", (fp16 ? "hc" : "rc"), -1);
}

wxString FragmentDecompilerThread::AddConst()
{
	mem32_t data(m_addr + m_size + m_offset);

	m_offset += 4 * 4;
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);
	return m_parr.AddParam("vec4", wxString::Format("fc%d", m_const_index++),
		wxString::Format("vec4(%f, %f, %f, %f)", (float&)x, (float&)y, (float&)z, (float&)w));
}

wxString FragmentDecompilerThread::AddTex()
{
	return m_parr.AddParam(PARAM_UNIFORM, "sampler2D", wxString::Format("tex%d", dst.tex_num));
}

template<typename T> wxString FragmentDecompilerThread::GetSRC(T src)
{
	wxString ret = wxEmptyString;

	bool is_color = src.reg_type == 0 || (src.reg_type == 1 && dst.src_attr_reg_num >= 1 && dst.src_attr_reg_num <= 3);

	switch(src.reg_type)
	{
	case 0: //tmp
		ret += AddReg(src.tmp_reg_index, src.fp16);
	break;

	case 1: //input
	{
		static const wxString reg_table[] =
		{
			"gl_Position",
			"col0", "col1",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7"
		};

		switch(dst.src_attr_reg_num)
		{
		case 0x00: ret += reg_table[0]; break;
		default:
			if(dst.src_attr_reg_num < WXSIZEOF(reg_table))
			{
				ret += m_parr.AddParam(PARAM_IN, "vec4", reg_table[dst.src_attr_reg_num]);
			}
			else
			{
				ConLog.Error("Bad src reg num: %d", dst.src_attr_reg_num);
				ret += m_parr.AddParam(PARAM_IN, "vec4", "unk");
				Emu.Pause();
			}
		break;
		}
	}
	break;

	case 2: //const
		ret += AddConst();
	break;

	default:
		ConLog.Error("Bad src type %d", src.reg_type);
		Emu.Pause();
	break;
	}

	static const char f_pos[4] = {'x', 'y', 'z', 'w'};
	static const char f_col[4] = {'r', 'g', 'b', 'a'};
	const char *f = is_color ? f_col : f_pos;

	wxString swizzle = wxEmptyString;
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if(strncmp(swizzle, f, 4) != 0) ret += "." + swizzle;

	if(src.abs) ret = "abs(" + ret + ")";
	if(src.neg) ret = "-" + ret;

	return ret;
}

wxString FragmentDecompilerThread::BuildCode()
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
		"void main()\n{\n%s}\n";

	return wxString::Format(prot, p, main);
}

void FragmentDecompilerThread::Task()
{
	mem32_t data(m_addr);
	m_size = 0;

	while(true)
	{
		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		m_offset = 4 * 4;

		switch(dst.opcode)
		{
		case 0x00: break; //NOP
		case 0x01: AddCode(GetSRC(src0)); break; //MOV
		case 0x02: AddCode("(" + GetSRC(src0) + " * " + GetSRC(src1) + ")"); break; //MUL
		case 0x03: AddCode("(" + GetSRC(src0) + " + " + GetSRC(src1) + ")"); break; //ADD
		case 0x04: AddCode("(" + GetSRC(src0) + " * " + GetSRC(src1) + " + " + GetSRC(src2) + ")"); break; //MAD
		case 0x05: AddCode("dot(" + GetSRC(src0) + ".xyz, " + GetSRC(src1) + ".xyz)"); break; // DP3
		case 0x06: AddCode("dot(" + GetSRC(src0) + ", " + GetSRC(src1) + ")"); break; // DP4
		case 0x07: AddCode("distance(" + GetSRC(src0) + ", " + GetSRC(src1) + ")"); break; // DST
		case 0x08: AddCode("min(" + GetSRC(src0) + ", " + GetSRC(src1) + ")"); break; // MIN
		case 0x09: AddCode("max(" + GetSRC(src0) + ", " + GetSRC(src1) + ")"); break; // MAX
		case 0x0a: AddCode("vec4(lessThan(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SLT
		case 0x0b: AddCode("vec4(greaterThanEqual(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SGE
		case 0x0c: AddCode("vec4(lessThanEqual(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SLE
		case 0x0d: AddCode("vec4(greaterThan(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SGT
		case 0x0e: AddCode("vec4(notEqual(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SNE
		case 0x0f: AddCode("vec4(equal(" + GetSRC(src0) + ", " + GetSRC(src1) + "))"); break; // SEQ

		case 0x10: AddCode("fract(" + GetSRC(src0) + ")"); break; // FRC
		case 0x11: AddCode("floor(" + GetSRC(src0) + ")"); break; // FLR
		//case 0x12: break; // KIL
		//case 0x13: break; // PK4
		//case 0x14: break; // UP4
		case 0x15: AddCode("ddx(" + GetSRC(src0) + ")"); break; // DDX
		case 0x16: AddCode("ddy(" + GetSRC(src0) + ")"); break; // DDY
		case 0x17: AddCode("texture(" + AddTex() + ", " + GetSRC(src0) + ".xy)"); break; //TEX
		//case 0x18: break; // TXP
		//case 0x19: break; // TXD
		case 0x1a: AddCode("1 / (" + GetSRC(src0) + ")"); break; // RCP
		case 0x1b: AddCode("inversesqrt(" + GetSRC(src0) + ")"); break; // RSQ
		case 0x1c: AddCode("exp2(" + GetSRC(src0) + ")"); break; // EX2
		case 0x1d: AddCode("log2(" + GetSRC(src0) + ")"); break; // LG2
		//case 0x1e: break; // LIT
		//case 0x1f: break; // LRP

		//case 0x20: break; // STR
		//case 0x21: break; // SFL
		case 0x22: AddCode("cos(" + GetSRC(src0) + ")"); break; // COS
		case 0x23: AddCode("sin(" + GetSRC(src0) + ")"); break; // SIN
		//case 0x24: break; // PK2
		//case 0x25: break; // UP2
		case 0x26: AddCode("pow(" + GetSRC(src0) + ", " + GetSRC(src1) +")"); break; // POW
		//case 0x27: break; // PKB
		//case 0x28: break; // UPB
		//case 0x29: break; // PK16
		//case 0x2a: break; // UP16
		//case 0x2b: break; // BEM
		//case 0x2c: break; // PKG
		//case 0x2d: break; // UPG
		//case 0x2e: break; // DP2A
		//case 0x2f: break; // TXL

		//case 0x31: break; // TXB
		//case 0x33: break; // TEXBEM
		//case 0x34: break; // TXPBEM
		//case 0x35: break; // BEMLUM
		//case 0x36: break; // REFL
		//case 0x37: break; // TIMESWTEX
		//case 0x38: break; // DP2
		//case 0x39: break; // NRM
		case 0x3a: AddCode("(" + GetSRC(src0) + " / " + GetSRC(src1) + ")"); break; // DIV
		case 0x3b: AddCode("(" + GetSRC(src0) + " / " + GetSRC(src1) + ")"); break; // DIVSQ
		//case 0x3c: break; // LIF
		case 0x3d: break; // FENCT
		case 0x3e: break; // FENCB

		default:
			ConLog.Error("Unknown opcode 0x%x (inst %d)", dst.opcode, m_size / (4 * 4));
			Emu.Pause();
		break;
		}

		m_size += m_offset;

		if(dst.end) break;

		data.SetOffset(m_offset);
	}

	m_shader = BuildCode();
	main.Clear();
}

ShaderProgram::ShaderProgram() 
	: m_decompiler_thread(nullptr)
	, id(0)
{
}

ShaderProgram::~ShaderProgram()
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

void ShaderProgram::Decompile()
{
#if 0
	FragmentDecompilerThread(shader, parr, addr).Entry();
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

	m_decompiler_thread = new FragmentDecompilerThread(shader, parr, addr, size);
	m_decompiler_thread->Start();
#endif
}

void ShaderProgram::Compile()
{
	if(id) glDeleteShader(id);

	id = glCreateShader(GL_FRAGMENT_SHADER);

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
			ConLog.Error("Failed to compile shader: %s", buf);
			delete[] buf;
		}

		ConLog.Write(shader);
		Emu.Pause();
	}
	//else ConLog.Write("Shader compiled successfully!");
}

void ShaderProgram::Delete()
{
	for(u32 i=0; i<parr.params.GetCount(); ++i)
	{
		parr.params[i].items.Clear();
		parr.params[i].type.Clear();
	}

	parr.params.Clear();
	shader.Clear();

	if(id)
	{
		glDeleteShader(id);
		id = 0;
	}
}