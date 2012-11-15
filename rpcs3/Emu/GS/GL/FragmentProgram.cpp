#include "stdafx.h"
#include "FragmentProgram.h"

void FragmentDecompilerThread::AddCode(wxString code)
{
	if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_le) return;
	if(!src0.exec_if_eq || !src0.exec_if_gr || !src0.exec_if_le)
	{
		ConLog.Error("Bad cond! eq: %d  gr: %d  le: %d", src0.exec_if_eq, src0.exec_if_gr, src0.exec_if_le);
		Emu.Pause();
		return;
	}

	if(src1.scale)
	{
		switch(src1.scale)
		{
		case 1: code = "(" + code + ") * 2"; break;
		case 2: code = "(" + code + ") * 4"; break;
		case 3: code = "(" + code + ") * 8"; break;
		case 5: code = "(" + code + ") / 2"; break;
		case 6: code = "(" + code + ") / 4"; break;
		case 7: code = "(" + code + ") / 8"; break;

		default:
			ConLog.Error("Bad scale: %d", src1.scale);
			Emu.Pause();
		break;
		}
	}

	code = AddReg(dst.dest_reg) + GetMask() + " = " + code + GetMask();

	main += "\t" + code + ";\n";
}

wxString FragmentDecompilerThread::GetMask()
{
	wxString ret = wxEmptyString;

	if(dst.mask_x) ret += 'x';
	if(dst.mask_y) ret += 'y';
	if(dst.mask_z) ret += 'z';
	if(dst.mask_w) ret += 'w';

	return ret.IsEmpty() || ret == "xyzw" ? wxEmptyString : ("." + ret);
}

wxString FragmentDecompilerThread::AddReg(u32 index)
{
	//if(!index) return "gl_FragColor";
	return m_parr.AddParam(index ? PARAM_NONE : PARAM_OUT, "vec4", wxString::Format("r%d", index));
}

wxString FragmentDecompilerThread::AddTex()
{
	return m_parr.AddParam(PARAM_CONST, "sampler2D", wxString::Format("tex_%d", dst.tex_num));
}

template<typename T> wxString FragmentDecompilerThread::GetSRC(T src)
{
	wxString ret = wxEmptyString;

	switch(src.reg_type)
	{
	case 0: //tmp
		ret += AddReg(src.tmp_reg_index);
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

	//case 2: //imm
	//break;

	default:
		ConLog.Error("Bad src type %d", src.reg_type);
		Emu.Pause();
	break;
	}

	static const char f[4] = {'x', 'y', 'z', 'w'};
	wxString swizzle = wxEmptyString;
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if(swizzle != "xyzw") ret += "." + swizzle;

	if(src.abs) ret = "abs(" + ret + ")";
	if(src.neg) ret = "-" + ret;

	return ret;
}

wxString FragmentDecompilerThread::BuildCode()
{
	wxString p = wxEmptyString;

	for(u32 i=0; i<m_parr.params.GetCount(); ++i)
	{
		for(u32 n=0; n<m_parr.params[i].names.GetCount(); ++n)
		{
			p += m_parr.params[i].type;
			p += " ";
			p += m_parr.params[i].names[n];
			p += ";\n";
		}
	}

	static const wxString& prot = 
		"#version 330 core\n"
		"\n"
		"%s\n"
		"void main()\n{\n%s}\n";

	return wxString::Format(prot, p, main);
}

wxThread::ExitCode FragmentDecompilerThread::Entry()
{
	mem32_t data(m_addr);

	for(;;)
	{
		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		switch(dst.opcode)
		{
		case 0x00: break; //NOP
		case 0x01: AddCode(GetSRC(src0)); break; //MOV
		case 0x02: AddCode("(" + GetSRC(src0) + " * " + GetSRC(src1) + ")"); break; //MUL
		case 0x17: AddCode("texture(" + AddTex() + ", (" + GetSRC(src0) + ").xy)"); break; //TEX

		default:
			ConLog.Error("Unknown opcode 0x%x", dst.opcode);
			Emu.Pause();
		break;
		}

		m_size += 4 * 4;

		if(dst.end) break;

		data.SetOffset(4 * 4);
	}

	m_shader = BuildCode();
	main.Clear();

	return (ExitCode)0;
}

ShaderProgram::ShaderProgram() 
	: m_decompiler_thread(NULL)
	, id(0)
{
}

ShaderProgram::~ShaderProgram()
{
	if(m_decompiler_thread)
	{
		Wait();
		if(m_decompiler_thread->IsAlive()) m_decompiler_thread->Delete();
		safe_delete(m_decompiler_thread);
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
		if(m_decompiler_thread->IsAlive()) m_decompiler_thread->Delete();
		safe_delete(m_decompiler_thread);
	}

	m_decompiler_thread = new FragmentDecompilerThread(shader, parr, addr, size);
	m_decompiler_thread->Create();
	m_decompiler_thread->Run();
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
			free(buf);
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
		parr.params[i].names.Clear();
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