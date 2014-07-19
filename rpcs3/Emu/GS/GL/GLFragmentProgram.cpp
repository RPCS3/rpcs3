#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLFragmentProgram.h"

void GLFragmentDecompilerThread::SetDst(std::string code, bool append_mask)
{
	if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt) return;

	switch(src1.scale)
	{
	case 0: break;
	case 1: code = "(" + code + " * 2.0)"; break;
	case 2: code = "(" + code + " * 4.0)"; break;
	case 3: code = "(" + code + " * 8.0)"; break;
	case 5: code = "(" + code + " / 2.0)"; break;
	case 6: code = "(" + code + " / 4.0)"; break;
	case 7: code = "(" + code + " / 8.0)"; break;

	default:
		LOG_ERROR(RSX, "Bad scale: %d", fmt::by_value(src1.scale));
		Emu.Pause();
	break;
	}

	if(dst.saturate)
	{
		code = "clamp(" + code + ", 0.0, 1.0)";
	}

	std::string dest;

	if (dst.set_cond)
	{
		dest += m_parr.AddParam(PARAM_NONE, "vec4", "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = ";
	}

	if (!dst.no_dest)
	{
		dest += AddReg(dst.dest_reg, dst.fp16) + "$m = ";
	}

	AddCode("$ifcond " + dest + code + (append_mask ? "$m;" : ";"));
}

void GLFragmentDecompilerThread::AddCode(const std::string& code)
{
	main.append(m_code_level, '\t') += Format(code) + "\n";
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
	return m_parr.AddParam(PARAM_NONE, "vec4", std::string(fp16 ? "h" : "r") + std::to_string(index), "vec4(0.0, 0.0, 0.0, 0.0)");
}

bool GLFragmentDecompilerThread::HasReg(u32 index, int fp16)
{
	return m_parr.HasParam(PARAM_NONE, "vec4",
		std::string(fp16 ? "h" : "r") + std::to_string(index));
}

std::string GLFragmentDecompilerThread::AddCond()
{
	return m_parr.AddParam(PARAM_NONE , "vec4", "cc" + std::to_string(src0.cond_reg_index));
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

std::string GLFragmentDecompilerThread::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC0>), this, src0) },
		{ "$1", std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC1>), this, src1) },
		{ "$2", std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC2>), this, src2) },
		{ "$t", std::bind(std::mem_fn(&GLFragmentDecompilerThread::AddTex), this) },
		{ "$m", std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetMask), this) },
		{ "$ifcond ", [this]() -> std::string
			{
				const std::string& cond = GetCond();
				if (cond == "true") return "";
				return "if(" + cond + ") ";
			}
		},
		{ "$cond", std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetCond), this) },
		{ "$c", std::bind(std::mem_fn(&GLFragmentDecompilerThread::AddConst), this) }
	};

	return fmt::replace_all(code, repl_list);
}

std::string GLFragmentDecompilerThread::GetCond()
{
	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		return "true";
	}
	else if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		return "false";
	}

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle, cond;
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];
	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	if (src0.exec_if_gr && src0.exec_if_eq)
	{
		cond = "greaterThanEqual";
	}
	else if (src0.exec_if_lt && src0.exec_if_eq)
	{
		cond = "lessThanEqual";
	}
	else if (src0.exec_if_gr && src0.exec_if_lt)
	{
		cond = "notEqual";
	}
	else if (src0.exec_if_gr)
	{
		cond = "greaterThan";
	}
	else if (src0.exec_if_lt)
	{
		cond = "lessThan";
	}
	else //if(src0.exec_if_eq)
	{
		cond = "equal";
	}

	return "any(" + cond + "(" + AddCond() + swizzle + ", vec4(0.0)))";
}

template<typename T> std::string GLFragmentDecompilerThread::GetSRC(T src)
{
	std::string ret;

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
			"diff_color", "spec_color",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
			"ssa"
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
				LOG_ERROR(RSX, "Bad src reg num: %d", fmt::by_value(dst.src_attr_reg_num));
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
		LOG_ERROR(RSX, "Bad src type %d", fmt::by_value(src.reg_type));
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
	AddCode(m_parr.AddParam(PARAM_OUT, "vec4", "ocol0", 0) + " = " + (m_ctrl & 0x40 ? "r0" : "h0") + ";\n");
	static const std::pair<std::string, std::string> table[] =
	{
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h2" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h4" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h6" },
		{ "ocol4", m_ctrl & 0x40 ? "r5" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PARAM_NONE, "vec4", table[i].second))
			AddCode(m_parr.AddParam(PARAM_OUT, "vec4", table[i].first, i+1) + " = " + table[i].second + ";");
	}

	if (m_ctrl & 0xe) main += "\tgl_FragDepth = r1.z;\n";

	std::string p;

	for (u32 i = 0; i<m_parr.params.size(); ++i)
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
	m_loop_count = 0;
	m_code_level = 1;

	while(true)
	{
		for (auto finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
			finded != m_end_offsets.end();
			finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(finded);
			m_code_level--;
			AddCode("}");
			m_loop_count--;
		}

		for (auto finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
			finded != m_else_offsets.end();
			finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size))
		{
			m_else_offsets.erase(finded);
			m_code_level--;
			AddCode("}");
			AddCode("else");
			AddCode("{");
			m_code_level++;
		}

		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		m_offset = 4 * 4;

		const u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

		switch(opcode)
		{
		case 0x00: break; //NOP
		case 0x01: SetDst("$0"); break; //MOV
		case 0x02: SetDst("($0 * $1)"); break; //MUL
		case 0x03: SetDst("($0 + $1)"); break; //ADD
		case 0x04: SetDst("($0 * $1 + $2)"); break; //MAD
		case 0x05: SetDst("vec4(dot($0.xyz, $1.xyz))"); break; // DP3
		case 0x06: SetDst("vec4(dot($0, $1))"); break; // DP4
		case 0x07: SetDst("vec4(distance($0, $1))"); break; // DST
		case 0x08: SetDst("min($0, $1)"); break; // MIN
		case 0x09: SetDst("max($0, $1)"); break; // MAX
		case 0x0a: SetDst("vec4(lessThan($0, $1))"); break; // SLT
		case 0x0b: SetDst("vec4(greaterThanEqual($0, $1))"); break; // SGE
		case 0x0c: SetDst("vec4(lessThanEqual($0, $1))"); break; // SLE
		case 0x0d: SetDst("vec4(greaterThan($0, $1))"); break; // SGT
		case 0x0e: SetDst("vec4(notEqual($0, $1))"); break; // SNE
		case 0x0f: SetDst("vec4(equal($0, $1))"); break; // SEQ

		case 0x10: SetDst("fract($0)"); break; // FRC
		case 0x11: SetDst("floor($0)"); break; // FLR
		case 0x12: SetDst("discard", false); break; // KIL (kill fragment)
		//case 0x13: break; // PK4 (pack four signed 8-bit values)
		//case 0x14: break; // UP4 (unpack four signed 8-bit values)
		case 0x15: SetDst("dFdx($0)"); break; // DDX
		case 0x16: SetDst("dFdy($0)"); break; // DDY
		case 0x17: SetDst("texture($t, $0.xy)"); break; // TEX (texture lookup)
		//case 0x18: break; // TXP (projective texture lookup)
		//case 0x19: break; // TXD (texture lookup with derivatives)
		case 0x1a: SetDst("(1 / $0)"); break; // RCP
		case 0x1b: SetDst("inversesqrt(abs($0))"); break; // RSQ
		case 0x1c: SetDst("exp2($0)"); break; // EX2
		case 0x1d: SetDst("log2($0)"); break; // LG2
		case 0x1e: SetDst("vec4(1.0, $0.x, ($0.x > 0 ? exp2($0.w * log2($0.y)) : 0.0), 1.0)"); break; // LIT (compute light coefficients)
		case 0x1f: SetDst("($0 * ($1 - $2) + $2)"); break; // LRP (linear interpolation)
		
		case 0x20: SetDst("vec4(equal($0, vec4(1.0)))"); break; // STR (set on true)
		case 0x21: SetDst("vec4(equal($0, vec4(0.0)))"); break; // SFL (set on false)
		case 0x22: SetDst("cos($0)"); break; // COS
		case 0x23: SetDst("sin($0)"); break; // SIN
		//case 0x24: break; // PK2 (pack two 16-bit floats)
		//case 0x25: break; // UP2 (unpack two 16-bit floats)
		case 0x26: SetDst("pow($0, $1)"); break; // POW
		//case 0x27: break; // PKB
		//case 0x28: break; // UPB
		//case 0x29: break; // PK16
		//case 0x2a: break; // UP16
		//case 0x2b: break; // BEM
		//case 0x2c: break; // PKG
		//case 0x2d: break; // UPG
		case 0x2e: SetDst("($0.x * $1.x + $0.y * $1.y + $2.x)");  break; // DP2A (2-component dot product and add)
		//case 0x2f: break; // TXL (texture lookup with LOD)
 		
 		//case 0x30: break;
		//case 0x31: break; // TXB (texture lookup with bias)
		//case 0x33: break; // TEXBEM
		//case 0x34: break; // TXPBEM
		//case 0x35: break; // BEMLUM
		case 0x36: SetDst("($0 - 2.0 * $1 * dot($0, $1))"); break; // RFL (reflection vector)
		//case 0x37: break; // TIMESWTEX
		case 0x38: SetDst("vec4(dot($0.xy, $1.xy))"); break; // DP2
		case 0x39: SetDst("normalize($0.xyz)"); break; // NRM
		case 0x3a: SetDst("($0 / $1)"); break; // DIV
		case 0x3b: SetDst("($0 / sqrt($1))"); break; // DIVSQ
		case 0x3c: SetDst("vec4(1.0, $0.y, ($0.y > 0 ? pow(2.0, $0.w) : 0.0), 1.0)"); break; // LIF
		case 0x3d: break; // FENCT
		case 0x3e: break; // FENCB

		case 0x40: SetDst("break"); break; //BRK
		//case 0x41: break; //CAL
		case 0x42:
			AddCode("if($cond)"); //IF
			m_else_offsets.push_back(src1.else_offset << 2);
			m_end_offsets.push_back(src2.end_offset << 2);
			AddCode("{");
			m_code_level++;
		break;

		case 0x43: //LOOP
			if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //LOOP",
					m_loop_count, src1.rep2, m_loop_count, src1.rep1, m_loop_count, src1.rep3, src2.end_offset));
			}
			else
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) //LOOP",
					m_loop_count, src1.rep2, m_loop_count, src1.rep1, m_loop_count, src1.rep3));
				m_loop_count++;
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
			}
		break;
		case 0x44: //REP
			if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //REP",
					m_loop_count, src1.rep2, m_loop_count, src1.rep1, m_loop_count, src1.rep3, src2.end_offset));
			}
			else
			{
				AddCode(fmt::Format("if($cond) for(int i%u = %u; i%u < %u; i%u += %u) //REP",
					m_loop_count, src1.rep2, m_loop_count, src1.rep1, m_loop_count, src1.rep3));
				m_loop_count++;
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
			}
		break;
		//case 0x45: SetDst("return"); break; //RET

		default:
			LOG_ERROR(RSX, "Unknown fp opcode 0x%x (inst %d)", opcode, m_size / (4 * 4));
			//Emu.Pause();
		break;
		}

		m_size += m_offset;

		if(dst.end) break;

		data.Skip(m_offset);
	}

	// flush m_code_level
	m_code_level = 1;
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
			LOG_ERROR(RSX, "Failed to compile shader: %s", buf); // Write log to the console

			delete[] buf;
		}

		LOG_NOTICE(RSX, m_shader.c_str()); // Log the text of the shader that failed to compile
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
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLShaderProgram::Delete(): glDeleteShader(%d) avoided", m_id);
		}
		else
		{
			glDeleteShader(m_id);
		}
		m_id = 0;
	}
}
