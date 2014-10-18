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

	auto data = vm::ptr<u32>::make(m_addr + m_size + m_offset);

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
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & 0x40 ? "r0" : "h0" },
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h2" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h4" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h6" },
		{ "ocol4", m_ctrl & 0x40 ? "r5" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PARAM_NONE, "vec4", table[i].second))
			AddCode(m_parr.AddParam(PARAM_OUT, "vec4", table[i].first, i) + " = " + table[i].second + ";");
	}

	if (m_ctrl & 0xe) main += "\tgl_FragDepth = r1.z;\n";

	std::string p;

	for (auto& param : m_parr.params) {
		p += param.Format();
	}

	return std::string("#version 330\n"
		"\n"
		+ p + "\n"
		"void main()\n{\n" + main + "}\n");
}

void GLFragmentDecompilerThread::Task()
{
	auto data = vm::ptr<u32>::make(m_addr);
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

		m_offset = 4 * sizeof(u32);

		const u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

		switch(opcode)
		{
		case RSX_FP_OPCODE_NOP: break;
		case RSX_FP_OPCODE_MOV: SetDst("$0"); break;
		case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); break;
		case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); break;
		case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); break;
		case RSX_FP_OPCODE_DP3: SetDst("vec4(dot($0.xyz, $1.xyz))"); break;
		case RSX_FP_OPCODE_DP4: SetDst("vec4(dot($0, $1))"); break;
		case RSX_FP_OPCODE_DST: SetDst("vec4(distance($0, $1))"); break;
		case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)"); break;
		case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)"); break;
		case RSX_FP_OPCODE_SLT: SetDst("vec4(lessThan($0, $1))"); break;
		case RSX_FP_OPCODE_SGE: SetDst("vec4(greaterThanEqual($0, $1))"); break;
		case RSX_FP_OPCODE_SLE: SetDst("vec4(lessThanEqual($0, $1))"); break;
		case RSX_FP_OPCODE_SGT: SetDst("vec4(greaterThan($0, $1))"); break;
		case RSX_FP_OPCODE_SNE: SetDst("vec4(notEqual($0, $1))"); break;
		case RSX_FP_OPCODE_SEQ: SetDst("vec4(equal($0, $1))"); break;

		case RSX_FP_OPCODE_FRC: SetDst("fract($0)"); break;
		case RSX_FP_OPCODE_FLR: SetDst("floor($0)"); break;
		case RSX_FP_OPCODE_KIL: SetDst("discard", false); break;
		//case RSX_FP_OPCODE_PK4: break;
		//case RSX_FP_OPCODE_UP4: break;
		case RSX_FP_OPCODE_DDX: SetDst("dFdx($0)"); break;
		case RSX_FP_OPCODE_DDY: SetDst("dFdy($0)"); break;
		case RSX_FP_OPCODE_TEX: SetDst("texture($t, $0.xy)"); break;
		//case RSX_FP_OPCODE_TXP: break;
		//case RSX_FP_OPCODE_TXD: break;
		case RSX_FP_OPCODE_RCP: SetDst("(1 / $0)"); break;
		case RSX_FP_OPCODE_RSQ: SetDst("inversesqrt(abs($0))"); break;
		case RSX_FP_OPCODE_EX2: SetDst("exp2($0)"); break;
		case RSX_FP_OPCODE_LG2: SetDst("log2($0)"); break;
		case RSX_FP_OPCODE_LIT: SetDst("vec4(1.0, $0.x, ($0.x > 0 ? exp2($0.w * log2($0.y)) : 0.0), 1.0)"); break;
		case RSX_FP_OPCODE_LRP: SetDst("($0 * ($1 - $2) + $2)"); break;
		
		case RSX_FP_OPCODE_STR: SetDst("vec4(equal($0, vec4(1.0)))"); break;
		case RSX_FP_OPCODE_SFL: SetDst("vec4(equal($0, vec4(0.0)))"); break;
		case RSX_FP_OPCODE_COS: SetDst("cos($0)"); break;
		case RSX_FP_OPCODE_SIN: SetDst("sin($0)"); break;
		//case RSX_FP_OPCODE_PK2: break;
		//case RSX_FP_OPCODE_UP2: break;
		case RSX_FP_OPCODE_POW: SetDst("pow($0, $1)"); break;
		//case RSX_FP_OPCODE_PKB: break;
		//case RSX_FP_OPCODE_UPB: break;
		//case RSX_FP_OPCODE_PK16: break;
		//case RSX_FP_OPCODE_UP16: break;
		//case RSX_FP_OPCODE_BEM: break;
		//case RSX_FP_OPCODE_PKG: break;
		//case RSX_FP_OPCODE_UPG: break;
		case RSX_FP_OPCODE_DP2A: SetDst("($0.x * $1.x + $0.y * $1.y + $2.x)"); break;
		//case RSX_FP_OPCODE_TXL: break;
 		
		//case RSX_FP_OPCODE_TXB: break;
		//case RSX_FP_OPCODE_TEXBEM: break;
		//case RSX_FP_OPCODE_TXPBEM: break;
		//case RSX_FP_OPCODE_BEMLUM: break;
		case RSX_FP_OPCODE_REFL: SetDst("($0 - 2.0 * $1 * dot($0, $1))"); break;
		//case RSX_FP_OPCODE_TIMESWTEX: break;
		case RSX_FP_OPCODE_DP2: SetDst("vec4(dot($0.xy, $1.xy))"); break;
		case RSX_FP_OPCODE_NRM: SetDst("normalize($0.xyz)"); break;
		case RSX_FP_OPCODE_DIV: SetDst("($0 / $1)"); break;
		case RSX_FP_OPCODE_DIVSQ: SetDst("($0 / sqrt($1))"); break;
		case RSX_FP_OPCODE_LIF: SetDst("vec4(1.0, $0.y, ($0.y > 0 ? pow(2.0, $0.w) : 0.0), 1.0)"); break;
		case RSX_FP_OPCODE_FENCT: break;
		case RSX_FP_OPCODE_FENCB: break;

		case RSX_FP_OPCODE_BRK: SetDst("break"); break;
		//case RSX_FP_OPCODE_CAL: break;
		case RSX_FP_OPCODE_IFE:
			AddCode("if($cond)");
			m_else_offsets.push_back(src1.else_offset << 2);
			m_end_offsets.push_back(src2.end_offset << 2);
			AddCode("{");
			m_code_level++;
		break;

		case RSX_FP_OPCODE_LOOP:
			if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //LOOP",
					m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
			}
			else
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) //LOOP",
					m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment));
				m_loop_count++;
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
			}
		break;
		case RSX_FP_OPCODE_REP:
			if(!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
			{
				AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //REP",
					m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
			}
			else
			{
				AddCode(fmt::Format("if($cond) for(int i%u = %u; i%u < %u; i%u += %u) //REP",
					m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment));
				m_loop_count++;
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
			}
		break;
		//case RSX_FP_OPCODE_RET: SetDst("return"); break;

		default:
			LOG_ERROR(RSX, "Unknown fp opcode 0x%x (inst %d)", opcode, m_size / (4 * 4));
			//Emu.Pause();
		break;
		}

		m_size += m_offset;

		if(dst.end) break;

		assert(m_offset % sizeof(u32) == 0);
		data += m_offset / sizeof(u32);
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
	for (auto& param : m_parr.params) {
		param.items.clear();
		param.type.clear();
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
