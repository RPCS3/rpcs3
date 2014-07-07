#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GLVertexProgram.h"

std::string GLVertexDecompilerThread::GetMask(bool is_sca)
{
	std::string ret;

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

	return ret.empty() || ret == "xyzw" ? "" : ("." + ret);
}

std::string GLVertexDecompilerThread::GetVecMask()
{
	return GetMask(false);
}

std::string GLVertexDecompilerThread::GetScaMask()
{
	return GetMask(true);
}

std::string GLVertexDecompilerThread::GetDST(bool isSca)
{
	std::string ret;

	switch(d3.dst)
	{
	case 0x1f:
		ret += m_parr.AddParam(PARAM_NONE, "vec4", std::string("tmp") + std::to_string(isSca ? d3.sca_dst_tmp : d0.dst_tmp));
	break;

	default:
		if (d3.dst > 15)
			LOG_ERROR(RSX, "dst index out of range: %u", d3.dst);
		ret += m_parr.AddParam(PARAM_NONE, "vec4", std::string("dst_reg") + std::to_string(d3.dst), d3.dst == 0 ? "vec4(0.0f, 0.0f, 0.0f, 1.0f)" : "vec4(0.0)");
	break;
	}

	return ret;
}

std::string GLVertexDecompilerThread::GetSRC(const u32 n)
{
	static const std::string reg_table[] = 
	{
		"in_pos", "in_weight", "in_normal",
		"in_diff_color", "in_spec_color",
		"in_fog",
		"in_point_size", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	std::string ret;

	switch(src[n].reg_type)
	{
	case 1: //temp
		ret += m_parr.AddParam(PARAM_NONE, "vec4", "tmp" + std::to_string(src[n].tmp_src));
	break;
	case 2: //input
		if (d1.input_src < SARRSIZEOF(reg_table))
		{
			ret += m_parr.AddParam(PARAM_IN, "vec4", reg_table[d1.input_src], d1.input_src);
		}
		else
		{
			LOG_ERROR(RSX, "Bad input src num: %d", fmt::by_value(d1.input_src));
			ret += m_parr.AddParam(PARAM_IN, "vec4", "in_unk", d1.input_src);
		}
	break;
	case 3: //const
		m_parr.AddParam(PARAM_UNIFORM, "vec4", std::string("vc[468]"));
		ret += std::string("vc[") + std::to_string(d1.const_src) + (d3.index_const ? " + " + AddAddrReg() : "") + "]";
	break;

	default:
		LOG_ERROR(RSX, "Bad src%u reg type: %d", n, fmt::by_value(src[n].reg_type));
		Emu.Pause();
	break;
	}

	static const std::string f = "xyzw";

	std::string swizzle;

	swizzle += f[src[n].swz_x];
	swizzle += f[src[n].swz_y];
	swizzle += f[src[n].swz_z];
	swizzle += f[src[n].swz_w];

	if(swizzle != f) ret += '.' + swizzle;

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

void GLVertexDecompilerThread::SetDST(bool is_sca, std::string value)
{
	if(d0.cond == 0) return;

	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	std::string mask = GetMask(is_sca);

	value += mask;

	if(is_sca && d0.vec_result)
	{
		value = "vec4(" + value + ")" + mask;
	}

	if(d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	std::string dest;

	if (d0.cond_update_enable_0 && d0.cond_update_enable_1)
	{
		dest += m_parr.AddParam(PARAM_NONE, "vec4", "cc" + std::to_string(d0.cond_reg_sel_1), "vec4(0.0)") + mask  + " = ";
	}

	if (d3.dst != 0x1f || (is_sca ? d3.sca_dst_tmp != 0x3f : d0.dst_tmp != 0x3f))
	{
		dest += GetDST(is_sca) + mask + " = ";
	}

	std::string code;

	if (d0.cond_test_enable)
		code += "$ifcond ";

	code += dest + value;

	AddCode(code + ";");
}

std::string GLVertexDecompilerThread::GetFunc()
{
	std::string name = "func$a";

	for(uint i=0; i<m_funcs.size(); ++i)
	{
		if(m_funcs[i].name.compare(name) == 0)
			return name + "()";
	}

	m_funcs.emplace_back();
	FuncInfo &idx = m_funcs.back();
	idx.offset = GetAddr();
	idx.name = name;

	return name + "()";
}

std::string GLVertexDecompilerThread::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetSRC), this, 0) },
		{ "$1", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetSRC), this, 1) },
		{ "$2", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetSRC), this, 2) },
		{ "$s", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetSRC), this, 2) },
		{ "$am", std::bind(std::mem_fn(&GLVertexDecompilerThread::AddAddrMask), this) },
		{ "$a", std::bind(std::mem_fn(&GLVertexDecompilerThread::AddAddrReg), this) },

		{ "$fa", [this]()->std::string {return std::to_string(GetAddr()); } },
		{ "$f()", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetFunc), this) },
		{ "$ifcond ", [this]() -> std::string
			{
				const std::string& cond = GetCond();
				if (cond == "true") return "";
				return "if(" + cond + ") ";
			}
		},
		{ "$cond", std::bind(std::mem_fn(&GLVertexDecompilerThread::GetCond), this) }
	};

	return fmt::replace_all(code, repl_list);
}

std::string GLVertexDecompilerThread::GetCond()
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	if (d0.cond == 0) return "false";
	if (d0.cond == (lt | gt | eq)) return "true";

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

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	return fmt::Format("any(%s(cc%d%s, vec4(0.0)%s))", cond_string_table[d0.cond], d0.cond_reg_sel_1, swizzle.c_str(), swizzle.c_str());
}


std::string GLVertexDecompilerThread::AddAddrMask()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return std::string(".") + f[d0.addr_swz];
}

std::string GLVertexDecompilerThread::AddAddrReg()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return m_parr.AddParam(PARAM_NONE, "ivec4", "a" + std::to_string(d0.addr_reg_sel_1), "ivec4(0)") + AddAddrMask();
}

u32 GLVertexDecompilerThread::GetAddr()
{
	return (d2.iaddrh << 3) | d3.iaddrl;
}

void GLVertexDecompilerThread::AddCode(const std::string& code)
{
	m_body.push_back(Format(code) + ";");
	m_cur_instr->body.push_back(Format(code));
}

void GLVertexDecompilerThread::SetDSTVec(const std::string& code)
{
	SetDST(false, code);
}

void GLVertexDecompilerThread::SetDSTSca(const std::string& code)
{
	SetDST(true, code);
}

std::string GLVertexDecompilerThread::BuildFuncBody(const FuncInfo& func)
{
	std::string result;

	for(uint i=func.offset; i<m_body.size(); ++i)
	{
		if(i != func.offset)
		{
			uint call_func = -1;
			for(uint j=0; j<m_funcs.size(); ++j)
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

std::string GLVertexDecompilerThread::BuildCode()
{
	struct reg_info
	{
		std::string name;
		bool need_declare;
		std::string src_reg;
		std::string src_reg_mask;
		bool need_cast;
	};

	static const reg_info reg_table[] =
	{
		{ "gl_Position", false, "dst_reg0", "", false },
		{ "diff_color", true, "dst_reg1", "", false },
		{ "spec_color", true, "dst_reg2", "", false },
		{ "front_diff_color", true, "dst_reg3", "", false },
		{ "front_spec_color", true, "dst_reg4", "", false },
		{ "fogc", true, "dst_reg5", ".x", true },
		{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
		{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
		{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
		{ "gl_PointSize", false, "dst_reg6", ".x", false },
		{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
		{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
		{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
		{ "tc0", true, "dst_reg7", "", false },
		{ "tc1", true, "dst_reg8", "", false },
		{ "tc2", true, "dst_reg9", "", false },
		{ "tc3", true, "dst_reg10", "", false },
		{ "tc4", true, "dst_reg11", "", false },
		{ "tc5", true, "dst_reg12", "", false },
		{ "tc6", true, "dst_reg13", "", false },
		{ "tc7", true, "dst_reg14", "", false },
		{ "tc8", true, "dst_reg15", "", false },
		{ "tc9", true, "dst_reg6", "", false }
	};

	std::string f;

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PARAM_NONE, "vec4", i.src_reg))
		{
			if (i.need_declare)
			{
				m_parr.AddParam(PARAM_OUT, "vec4", i.name);
			}

			if (i.need_cast)
			{
				f += "\t" + i.name + " = vec4(" + i.src_reg + i.src_reg_mask + ");\n";
			}
			else
			{
				f += "\t" + i.name + " = " + i.src_reg + i.src_reg_mask + ";\n";
			}
		}
	}

	std::string p;

	for (u32 i = 0; i<m_parr.params.size(); ++i)
	{
		p += m_parr.params[i].Format();
	}

	std::string fp;

	for (int i = m_funcs.size() - 1; i>0; --i)
	{
		fp += fmt::Format("void %s();\n", m_funcs[i].name.c_str());
	}

	f = fmt::Format("void %s()\n{\n\t%s();\n%s\tgl_Position = gl_Position * scaleOffsetMat;\n}\n",
		m_funcs[0].name.c_str(), m_funcs[1].name.c_str(), f.c_str());

	std::string main_body;
	for (uint i = 0, lvl = 1; i < m_instr_count; i++)
	{
		lvl -= m_instructions[i].close_scopes;
		if (lvl < 1) lvl = 1;
		//assert(lvl >= 1);
		for (uint j = 0; j < m_instructions[i].put_close_scopes; ++j)
		{
			--lvl;
			if (lvl < 1) lvl = 1;
			main_body.append(lvl, '\t') += "}\n";
		}

		for (uint j = 0; j < m_instructions[i].do_count; ++j)
		{
			main_body.append(lvl, '\t') += "do\n";
			main_body.append(lvl, '\t') += "{\n";
			lvl++;
		}

		for (uint j = 0; j < m_instructions[i].body.size(); ++j)
		{
			main_body.append(lvl, '\t') += m_instructions[i].body[j] + "\n";
		}

		lvl += m_instructions[i].open_scopes;
	}

	f += fmt::Format("\nvoid %s()\n{\n%s}\n", m_funcs[1].name.c_str(), main_body.c_str());

	for(uint i=2; i<m_funcs.size(); ++i)
	{
		f += fmt::Format("\nvoid %s()\n{\n%s}\n", m_funcs[i].name.c_str(), BuildFuncBody(m_funcs[i]).c_str());
	}

	static const std::string& prot =
		"#version 330\n"
		"\n"
		"uniform mat4 scaleOffsetMat = mat4(1.0);\n"
		"%s\n"
		"%s\n"
		"%s";

	return fmt::Format(prot, p.c_str(), fp.c_str(), f.c_str());
}

void GLVertexDecompilerThread::Task()
{
	m_parr.params.clear();
	m_instr_count = 0;

	for (int i = 0; i < m_max_instr_count; ++i)
		m_instructions[i].reset();

	for (u32 i = 0; m_instr_count < m_max_instr_count; m_instr_count++)
	{
		m_cur_instr = &m_instructions[m_instr_count];

		d0.HEX = m_data[i++];
		d1.HEX = m_data[i++];
		d2.HEX = m_data[i++];
		d3.HEX = m_data[i++];

		src[0].src0l = d2.src0l;
		src[0].src0h = d1.src0h;
		src[1].src1 = d2.src1;
		src[2].src2l = d3.src2l;
		src[2].src2h = d2.src2h;

		if(!d1.sca_opcode && !d1.vec_opcode)
		{
			m_body.push_back("//nop");
		}

		switch(d1.sca_opcode)
		{
		case 0x00: break; // NOP
		case 0x01: SetDSTSca("$s"); break; // MOV
		case 0x02: SetDSTSca("(1.0 / $s)"); break; // RCP
		case 0x03: SetDSTSca("clamp(1.0 / $s, 5.42101e-20, 1.884467e19)"); break; // RCC
		case 0x04: SetDSTSca("inversesqrt(abs($s))"); break; // RSQ
		case 0x05: SetDSTSca("exp($s)"); break; // EXP
		case 0x06: SetDSTSca("log($s)"); break; // LOG
		case 0x07: SetDSTSca("vec4(1.0, $s.x, ($s.x > 0 ? exp2($s.w * log2($s.y)) : 0.0), 1.0)"); break; // LIT
		//case 0x08: break; // BRA
		case 0x09: // BRI : works differently (BRI o[1].x(TR) L0;)
			//AddCode("$ifcond { $f(); return; }");
			if (GetAddr() > m_instr_count)
			{
				AddCode("if(!$cond)");
				AddCode("{");
				m_cur_instr->open_scopes++;
				m_instructions[GetAddr()].put_close_scopes++;
			}
			else
			{
				AddCode("} while ($cond);");
				m_cur_instr->close_scopes++;
				m_instructions[GetAddr()].do_count++;
			}
		break;
		//case 0x0a: AddCode("$ifcond $f(); //CAL"); break; // CAL : works same as BRI
		case 0x0b: AddCode("$ifcond $f(); //CLI"); break; // CLI : works same as BRI
		case 0x0c: AddCode("$ifcond return;"); break; // RET : works like BRI but shorter (RET o[1].x(TR);)
		case 0x0d: SetDSTSca("log2($s)"); break; // LG2
		case 0x0e: SetDSTSca("exp2($s)"); break; // EX2
		case 0x0f: SetDSTSca("sin($s)"); break; // SIN
		case 0x10: SetDSTSca("cos($s)"); break; // COS
		//case 0x11: break; // BRB : works differently (BRB o[1].x !b0, L0;)
		//case 0x12: break; // CLB : works same as BRB
		//case 0x13: break; // PSH : works differently (PSH o[1].x A0;)
		//case 0x14: break; // POP : works differently (POP o[1].x;)

		default:
			m_body.push_back(fmt::Format("//Unknown vp sca_opcode 0x%x", fmt::by_value(d1.sca_opcode)));
			LOG_ERROR(RSX, "Unknown vp sca_opcode 0x%x", fmt::by_value(d1.sca_opcode));
			Emu.Pause();
		break;
		}

		switch(d1.vec_opcode)
		{
		case 0x00: break; //NOP
		case 0x01: SetDSTVec("$0"); break; //MOV
		case 0x02: SetDSTVec("($0 * $1)"); break; //MUL
		case 0x03: SetDSTVec("($0 + $2)"); break; //ADD
		case 0x04: SetDSTVec("($0 * $1 + $2)"); break; //MAD
		case 0x05: SetDSTVec("vec4(dot($0.xyz, $1.xyz))"); break; //DP3
		case 0x06: SetDSTVec("vec4(dot(vec4($0.xyz, 1.0), $1))"); break; //DPH
		case 0x07: SetDSTVec("vec4(dot($0, $1))"); break; //DP4
		case 0x08: SetDSTVec("vec4(distance($0, $1))"); break; //DST
		case 0x09: SetDSTVec("min($0, $1)"); break; //MIN
		case 0x0a: SetDSTVec("max($0, $1)"); break; //MAX
		case 0x0b: SetDSTVec("vec4(lessThan($0, $1))"); break; //SLT
		case 0x0c: SetDSTVec("vec4(greaterThanEqual($0, $1))"); break; //SGE
		case 0x0d: AddCode("$ifcond $a = ivec4($0)$am;");  break; //ARL
		case 0x0e: SetDSTVec("fract($0)"); break; //FRC
		case 0x0f: SetDSTVec("floor($0)"); break; //FLR
		case 0x10: SetDSTVec("vec4(equal($0, $1))"); break; //SEQ
		case 0x11: SetDSTVec("vec4(equal($0, vec4(0.0)))"); break; //SFL
		case 0x12: SetDSTVec("vec4(greaterThan($0, $1))"); break; //SGT
		case 0x13: SetDSTVec("vec4(lessThanEqual($0, $1))"); break; //SLE
		case 0x14: SetDSTVec("vec4(notEqual($0, $1))"); break; //SNE
		case 0x15: SetDSTVec("vec4(equal($0, vec4(1.0)))"); break; //STR
		case 0x16: SetDSTVec("sign($0)"); break; //SSG

		default:
			m_body.push_back(fmt::Format("//Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
			LOG_ERROR(RSX, "Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode));
			Emu.Pause();
		break;
		}

		if(d3.end)
		{
			m_instr_count++;

			if(i < m_data.size())
				LOG_ERROR(RSX, "Program end before buffer end.");

			break;
		}
	}

	m_shader = BuildCode();

	m_body.clear();
	if (m_funcs.size() > 2)
	{
		m_funcs.erase(m_funcs.begin()+2, m_funcs.end());
	}
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

void GLVertexProgram::Wait()
{
	if(m_decompiler_thread && m_decompiler_thread->IsAlive())
	{
		m_decompiler_thread->Join();
	}
}

void GLVertexProgram::Decompile(RSXVertexProgram& prog)
{
	GLVertexDecompilerThread decompiler(prog.data, shader, parr);
	decompiler.Task();
}

void GLVertexProgram::DecompileAsync(RSXVertexProgram& prog)
{
	if (m_decompiler_thread)
	{
		Wait();
		if (m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	m_decompiler_thread = new GLVertexDecompilerThread(prog.data, shader, parr);
	m_decompiler_thread->Start();
}

void GLVertexProgram::Compile()
{
	if(id) glDeleteShader(id);

	id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.c_str();
	const int strlen = shader.length();

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
			LOG_ERROR(RSX, "Failed to compile vertex shader: %s", buf);
			delete[] buf;
		}

		LOG_NOTICE(RSX, shader);
		Emu.Pause();
	}
	//else LOG_WARNING(RSX, "Vertex shader compiled successfully!");

}

void GLVertexProgram::Delete()
{
	parr.params.clear();
	shader.clear();

	if(id)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLVertexProgram::Delete(): glDeleteShader(%d) avoided", id);
		}
		else
		{
			glDeleteShader(id);
		}
		id = 0;
	}
}
