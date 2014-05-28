#include "stdafx.h"
#include "GLFragmentProgram.h"

void GLFragmentDecompilerThread::AddCode(std::string code, bool append_mask)
{
	if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt) return;

	const std::string mask = GetMask();
	std::string cond;

	if(!src0.exec_if_gr || !src0.exec_if_lt || !src0.exec_if_eq)
	{
		static const char f[4] = {'x', 'y', 'z', 'w'};

		std::string swizzle;
		swizzle += f[src0.cond_swizzle_x];
		swizzle += f[src0.cond_swizzle_y];
		swizzle += f[src0.cond_swizzle_z];
		swizzle += f[src0.cond_swizzle_w];
		swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

		if(src0.exec_if_gr && src0.exec_if_eq)
		{
			cond = "greaterThanEqual";
		}
		else if(src0.exec_if_lt && src0.exec_if_eq)
		{
			cond = "lessThanEqual";
		}
		else if(src0.exec_if_gr && src0.exec_if_lt)
		{
			cond = "notEqual";
		}
		else if(src0.exec_if_gr)
		{
			cond = "greaterThan";
		}
		else if(src0.exec_if_lt)
		{
			cond = "lessThan";
		}
		else //if(src0.exec_if_eq)
		{
			cond = "equal";
		}

		cond = "if(all(" + cond + "(" + AddCond(dst.no_dest) + swizzle + ", vec4(0.0)))) ";
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
			ConLog.Error("Bad scale: %d", fmt::by_value(src1.scale));
			Emu.Pause();
		break;
		}
	}

	if(dst.saturate)
	{
		code = "clamp(" + code + ", 0.0, 1.0)";
	}

	std::string dest;

	if(dst.no_dest)
	{
		if(dst.set_cond)
		{
			dest = m_parr.AddParam(PARAM_NONE , "vec4", std::string(dst.fp16 ? "hc" : "rc") + std::to_string(src0.cond_reg_index));
		}
	}
	else
	{
		dest = AddReg(dst.dest_reg, dst.fp16);
	}

	code = cond + (dest.length() ? dest + mask + " = " : "") + code + (append_mask ? mask : "");

	main += "\t" + code + ";\n";
}

std::string GLFragmentDecompilerThread::GetMask()
{
	std::string ret;

	static const char dst_mask[4] =
	{
		'x', 'y', 'z', 'w',
	};

	if(dst.mask_x) ret += dst_mask[0];
	if(dst.mask_y) ret += dst_mask[1];
	if(dst.mask_z) ret += dst_mask[2];
	if(dst.mask_w) ret += dst_mask[3];

	return ret.empty() || strncmp(ret.c_str(), dst_mask, 4) == 0 ? "" : ("." + ret);
}

std::string GLFragmentDecompilerThread::AddReg(u32 index, int fp16)
{
	if(index >= 2 && index <= 4)
	{
		return m_parr.AddParam(PARAM_OUT, "vec4", std::string(fp16 ? "h" : "r") + std::to_string(index), 
							(fp16) ? -1 : (index - 1));
	}

	return m_parr.AddParam(PARAM_NONE, "vec4", std::string(fp16 ? "h" : "r") + std::to_string(index), "vec4(0.0, 0.0, 0.0, 0.0)");

	//return m_parr.AddParam((index >= 2 && index <= 4) ? PARAM_OUT : PARAM_NONE, "vec4",
	//		std::string(fp16 ? "h" : "r") + std::to_string(index), (fp16 || !index) ? -1 : ((index >= 2 && index <= 4) ? (index - 1) : -1));
}

bool GLFragmentDecompilerThread::HasReg(u32 index, int fp16)
{
	return m_parr.HasParam((index >= 2 && index <= 4) ? PARAM_OUT : PARAM_NONE, "vec4",
		std::string(fp16 ? "h" : "r") + std::to_string(index));
}

std::string GLFragmentDecompilerThread::AddCond(int fp16)
{
	return m_parr.AddParam(PARAM_NONE , "vec4", std::string(fp16 ? "hc" : "rc") + std::to_string(src0.cond_mod_reg_index));
}

std::string GLFragmentDecompilerThread::AddConst()
{
	std::string name = std::string("fc") + std::to_string(m_size + 4 * 4);
	if(m_parr.HasParam(PARAM_UNIFORM, "vec4", name))
	{
		return name;
	}

	mem32_ptr_t data(m_addr + m_size + m_offset);

	m_offset += 4 * 4;
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);
	return m_parr.AddParam(PARAM_UNIFORM, "vec4", name,
		std::string("vec4(") + std::to_string((float&)x) + ", " + std::to_string((float&)y)
		+ ", " + std::to_string((float&)z) + ", " + std::to_string((float&)w) + ")");
}

std::string GLFragmentDecompilerThread::AddTex()
{
	return m_parr.AddParam(PARAM_UNIFORM, "sampler2D", std::string("tex") + std::to_string(dst.tex_num));
}

template<typename T> std::string GLFragmentDecompilerThread::GetSRC(T src)
{
	std::string ret = "";

	switch(src.reg_type)
	{
	case 0: //tmp
		ret += AddReg(src.tmp_reg_index, src.fp16);
	break;

	case 1: //input
	{
		static const std::string reg_table[] =
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
			if(dst.src_attr_reg_num < sizeof(reg_table)/sizeof(reg_table[0]))
			{
				ret += m_parr.AddParam(PARAM_IN, "vec4", reg_table[dst.src_attr_reg_num]);
			}
			else
			{
				ConLog.Error("Bad src reg num: %d", fmt::by_value(dst.src_attr_reg_num));
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
		ConLog.Error("Bad src type %d", fmt::by_value(src.reg_type));
		Emu.Pause();
	break;
	}

	static const char f[4] = {'x', 'y', 'z', 'w'};

	std::string swizzle = "";
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if(strncmp(swizzle.c_str(), f, 4) != 0) ret += "." + swizzle;

	if(src.abs) ret = "abs(" + ret + ")";
	if(src.neg) ret = "-" + ret;

	return ret;
}

std::string GLFragmentDecompilerThread::BuildCode()
{
	//main += fmt::Format("\tgl_FragColor = %c0;\n", m_ctrl & 0x40 ? 'r' : 'h');
	main += "\t" + m_parr.AddParam(PARAM_OUT, "vec4", "ocol", 0) + " = " + (m_ctrl & 0x40 ? "r0" : "h0") + ";\n";
	if(m_ctrl & 0xe) main += "\tgl_FragDepth = r1.z;\n";

	std::string p;

	for(u32 i=0; i<m_parr.params.size(); ++i)
	{
		p += m_parr.params[i].Format();
	}

	return std::string("#version 330\n"
					   "\n"
					   + p + "\n"
					   "void main()\n{\n" + main + "}\n");
}

void GLFragmentDecompilerThread::Task()
{
	mem32_ptr_t data(m_addr);
	m_size = 0;
	m_location = 0;

	while(true)
	{
		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		m_offset = 4 * 4;

		const u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

		switch(opcode)
		{
		case 0x00: break; //NOP
		case 0x01: AddCode(GetSRC(src0)); break; //MOV
		case 0x02: AddCode("(" + GetSRC(src0) + " * " + GetSRC(src1) + ")"); break; //MUL
		case 0x03: AddCode("(" + GetSRC(src0) + " + " + GetSRC(src1) + ")"); break; //ADD
		case 0x04: AddCode("(" + GetSRC(src0) + " * " + GetSRC(src1) + " + " + GetSRC(src2) + ")"); break; //MAD
		case 0x05: AddCode("vec2(dot(" + GetSRC(src0) + ".xyz, " + GetSRC(src1) + ".xyz), 0).xxxx"); break; // DP3
		case 0x06: AddCode("vec2(dot(" + GetSRC(src0) + ", " + GetSRC(src1) + "), 0).xxxx"); break; // DP4
		case 0x07: AddCode("vec2(distance(" + GetSRC(src0) + ", " + GetSRC(src1) + "), 0).xxxx"); break; // DST
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
		case 0x12: AddCode("if (" + GetSRC(src0) + " < 0 ) exit()"); break; // KIL (kill fragment (if (A<0) exit())
		//case 0x13: break; // PK4 (pack four signed 8-bit values)
		//case 0x14: break; // UP4 (unpack four signed 8-bit values)
		case 0x15: AddCode("dFdx(" + GetSRC(src0) + ")"); break; // DDX
		case 0x16: AddCode("dFdy(" + GetSRC(src0) + ")"); break; // DDY
		case 0x17: AddCode("texture(" + AddTex() + ", " + GetSRC(src0) + ".xy)"); break; // TEX (texture lookup)
		//case 0x18: break; // TXP (projective texture lookup)
		//case 0x19: break; // TXD (texture lookup w/partials)
		case 0x1a: AddCode("1 / (" + GetSRC(src0) + ")"); break; // RCP
		case 0x1b: AddCode("inversesqrt(" + GetSRC(src0) + ")"); break; // RSQ
		case 0x1c: AddCode("exp2(" + GetSRC(src0) + ")"); break; // EX2
		case 0x1d: AddCode("log2(" + GetSRC(src0) + ")"); break; // LG2
		//case 0x1e: break; // LIT (compute light coefficients)
		case 0x1f: AddCode("(" + GetSRC(src0) + " * (" + GetSRC(src1) + " - " + GetSRC(src2) + ") +" + GetSRC(src2) + ")"); break; // LRP (linear interpolation)
		//case 0x20: break; // STR (set on true)
		//case 0x21: break; // SFL (set on false)
		case 0x22: AddCode("cos(" + GetSRC(src0) + ")"); break; // COS
		case 0x23: AddCode("sin(" + GetSRC(src0) + ")"); break; // SIN
		//case 0x24: break; // PK2 (pack two 16-bit floats)
		//case 0x25: break; // UP2 (unpack two 16-bit floats)
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
		case 0x38: AddCode("vec2(dot(" + GetSRC(src0) + ".xy, " + GetSRC(src1) + ".xy)).xxxx"); break; // DP2
		case 0x39: AddCode("normalize(" + GetSRC(src0) + ".xyz)"); break; // NRM
		case 0x3a: AddCode("(" + GetSRC(src0) + " / " + GetSRC(src1) + ")"); break; // DIV
		case 0x3b: AddCode("(" + GetSRC(src0) + " / sqrt(" + GetSRC(src1) + "))"); break; // DIVSQ
		//case 0x3c: break; // LIF
		case 0x3d: break; // FENCT
		case 0x3e: break; // FENCB

		default:
			ConLog.Error("Unknown opcode 0x%x (inst %d)", opcode, m_size / (4 * 4));
			//Emu.Pause();
		break;
		}

		m_size += m_offset;

		if(dst.end) break;

		data.Skip(m_offset);
	}

	m_shader = BuildCode();
	main.clear();
	m_parr.params.clear();
}

GLShaderProgram::GLShaderProgram() 
	: m_decompiler_thread(nullptr)
	, m_id(0)
{
}

GLShaderProgram::~GLShaderProgram()
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

void GLShaderProgram::Wait()
{
	if(m_decompiler_thread && m_decompiler_thread->IsAlive())
	{
		m_decompiler_thread->Join();
	}
}

void GLShaderProgram::Decompile(RSXShaderProgram& prog)
{
	GLFragmentDecompilerThread decompiler(m_shader, m_parr, prog.addr, prog.size, prog.ctrl);
	decompiler.Task();
}

void GLShaderProgram::DecompileAsync(RSXShaderProgram& prog)
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

	m_decompiler_thread = new GLFragmentDecompilerThread(m_shader, m_parr, prog.addr, prog.size, prog.ctrl);
	m_decompiler_thread->Start();
}

void GLShaderProgram::Compile()
{
	if (m_id) glDeleteShader(m_id);

	m_id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* str = m_shader.c_str();
	const int strlen = m_shader.length();

	glShaderSource(m_id, 1, &str, &strlen);
	glCompileShader(m_id);

	GLint compileStatus = GL_FALSE;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &compileStatus); // Determine the result of the glCompileShader call
	if (compileStatus != GL_TRUE) // If the shader failed to compile...
	{
		GLint infoLength;
		glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLength); // Retrieve the length in bytes (including trailing NULL) of the shader info log

		if (infoLength > 0)
		{
			GLsizei len;
			char* buf = new char[infoLength]; // Buffer to store infoLog

			glGetShaderInfoLog(m_id, infoLength, &len, buf); // Retrieve the shader info log into our buffer
			ConLog.Error("Failed to compile shader: %s", buf); // Write log to the console

			delete[] buf;
		}

		ConLog.Write(m_shader.c_str()); // Log the text of the shader that failed to compile
		Emu.Pause(); // Pause the emulator, we can't really continue from here
	}
}

void GLShaderProgram::Delete()
{
	for (u32 i = 0; i<m_parr.params.size(); ++i)
	{
		m_parr.params[i].items.clear();
		m_parr.params[i].type.clear();
	}

	m_parr.params.clear();
	m_shader.clear();

	if (m_id)
	{
		glDeleteShader(m_id);
		m_id = 0;
	}
}
