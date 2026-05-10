#include "stdafx.h"
#include "VertexProgramDecompiler.h"

#include <sstream>

std::string VertexProgramDecompiler::GetMask(bool is_sca) const
{
	std::string ret;

	if (is_sca)
	{
		if (d3.sca_writemask_x) ret += "x";
		if (d3.sca_writemask_y) ret += "y";
		if (d3.sca_writemask_z) ret += "z";
		if (d3.sca_writemask_w) ret += "w";
	}
	else
	{
		if (d3.vec_writemask_x) ret += "x";
		if (d3.vec_writemask_y) ret += "y";
		if (d3.vec_writemask_z) ret += "z";
		if (d3.vec_writemask_w) ret += "w";
	}

	return ret.empty() || ret == "xyzw" ? "" : ("." + ret);
}

std::string VertexProgramDecompiler::GetVecMask()
{
	return GetMask(false);
}

std::string VertexProgramDecompiler::GetScaMask()
{
	return GetMask(true);
}

std::string VertexProgramDecompiler::GetDST(bool is_sca)
{
	std::string ret;
	const std::string mask = GetMask(is_sca);

	// ARL writes to special integer registers
	const bool is_address_reg = !is_sca && (d1.vec_opcode == RSX_VEC_OPCODE_ARL);
	const auto tmp_index = is_sca ? d3.sca_dst_tmp : d0.dst_tmp;
	const bool is_result = is_sca ? !d0.vec_result : d0.vec_result;

	if (is_result)
	{
		// Write to output result register
		// vec_result can mask out the VEC op from writing to o[] if SCA is writing to o[]

		if (d3.dst != 0x1f)
		{
			if (d3.dst > 15)
			{
				rsx_log.error("dst index out of range: %u", d3.dst);
			}

			if (is_address_reg)
			{
				rsx_log.error("ARL opcode writing to output register!");
			}

			const auto reg_type = getFloatTypeName(4);
			const auto reg_name = std::string("dst_reg") + std::to_string(d3.dst);
			const auto default_value = reg_type + "(0.0f, 0.0f, 0.0f, 1.0f)";
			ret += m_parr.AddParam(PF_PARAM_OUT, reg_type, reg_name, default_value) + mask;
		}
	}

	if (tmp_index != 0x3f)
	{
		if (!ret.empty())
		{
			// Double assignment. Only possible for vector ops
			ensure(!is_sca);
			ret += " = ";
		}

		const std::string reg_type = (is_address_reg) ? getIntTypeName(4) : getFloatTypeName(4);
		const std::string reg_sel = (is_address_reg) ? "a" : "r";
		ret += m_parr.AddParam(PF_PARAM_NONE, reg_type, reg_sel + std::to_string(tmp_index), reg_type + "(0.)") + mask;
	}
	else if (!is_result)
	{
		// Not writing to result register, but not writing to a tmp register either
		// Write to CC instead (Far Cry 2)
		ret = AddCondReg() + mask;
	}

	return ret;
}

std::string VertexProgramDecompiler::GetSRC(const u32 n)
{
	ensure(n < 3);

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
	const auto float4 = getFloatTypeName(4);

	switch (src[n].reg_type)
	{
	case RSX_VP_REGISTER_TYPE_TEMP:
		ret += m_parr.AddParam(PF_PARAM_NONE, float4, "r" + std::to_string(src[n].tmp_src), float4 + "(0.)");
		break;
	case RSX_VP_REGISTER_TYPE_INPUT:
		if (d1.input_src < std::size(reg_table))
		{
			ret += m_parr.AddParam(PF_PARAM_IN, float4, reg_table[d1.input_src], d1.input_src);
		}
		else
		{
			rsx_log.error("Bad input src num: %d", u32{ d1.input_src });
			ret += m_parr.AddParam(PF_PARAM_IN, float4, "in_unk", d1.input_src);
		}
		break;
	case RSX_VP_REGISTER_TYPE_CONSTANT:
		m_parr.AddParam(PF_PARAM_UNIFORM, float4, std::string("vc[468]"));
		properties.has_indexed_constants |= !!d3.index_const;
		m_constant_ids.insert(static_cast<u16>(d1.const_src));
		fmt::append(ret, "_fetch_constant(%u%s)", d1.const_src, (d3.index_const ? " + " + AddAddrReg() : ""));
		break;

	default:
		rsx_log.fatal("Bad src%u reg type: %d", n, u32{ src[n].reg_type });
		break;
	}

	static const std::string f = "xyzw";

	std::string swizzle;

	swizzle += f[src[n].swz_x];
	swizzle += f[src[n].swz_y];
	swizzle += f[src[n].swz_z];
	swizzle += f[src[n].swz_w];

	if (swizzle != f) ret += '.' + swizzle;

	bool abs = false;

	switch (n)
	{
	default:
	case 0: abs = d0.src0_abs; break;
	case 1: abs = d0.src1_abs; break;
	case 2: abs = d0.src2_abs; break;
	}

	if (abs) ret = "abs(" + ret + ")";
	if (src[n].neg) ret = "-" + ret;

	return ret;
}

void VertexProgramDecompiler::SetDST(bool is_sca, std::string value)
{
	if (d0.cond == 0) return;

	if (is_sca)
	{
		value = getFloatTypeName(4) + "(" + value + ")";
	}

	std::string mask = GetMask(is_sca);
	value += mask;

	if (d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	std::string dest;

	if (const auto tmp_reg = is_sca? d3.sca_dst_tmp: d0.dst_tmp;
		d3.dst != 0x1f || tmp_reg != 0x3f)
	{
		dest = GetDST(is_sca);
	}
	else if (d0.cond_update_enable_0 || d0.cond_update_enable_1)
	{
		dest = AddCondReg() + mask;
	}
	else
	{
		// Broken instruction?
		rsx_log.error("Operation has no output defined! (0x%x, 0x%x, 0x%x, 0x%x)", d0.HEX, d1.HEX, d2.HEX, d3.HEX);
		dest = " //";
	}

	AddCodeCond(Format(dest), value);
}

std::string VertexProgramDecompiler::GetTex()
{
	std::string sampler;
	switch (m_prog.get_texture_dimension(d2.tex_num))
	{
	case rsx::texture_dimension_extended::texture_dimension_1d:
		sampler = "sampler1D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_2d:
		sampler = "sampler2D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_3d:
		sampler = "sampler3D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_cubemap:
		sampler = "samplerCube";
		break;
	}

	return m_parr.AddParam(PF_PARAM_UNIFORM, sampler, std::string("vtex") + std::to_string(d2.tex_num));
}

std::string VertexProgramDecompiler::Format(const std::string& code)
{
	const std::pair<std::string_view, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 0) },
		{ "$1", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 1) },
		{ "$2", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 2) },
		{ "$s", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 2) },
		{ "$a", std::bind(std::mem_fn(&VertexProgramDecompiler::AddAddrReg), this) },
		{ "$t", [this]() -> std::string { return "vtex" + std::to_string(d2.tex_num); } },
		{ "$ifcond ", [this]() -> std::string
			{
				const std::string& cond = GetCond();
				if (cond == "true") return "";
				return "if(" + cond + ") ";
			}
		},
		{ "$cond", std::bind(std::mem_fn(&VertexProgramDecompiler::GetCond), this) },
		{ "$ifbcond", std::bind(std::mem_fn(&VertexProgramDecompiler::GetOptionalBranchCond), this) },
		{ "$Ty", [this](){ return getFloatTypeName(4); } }
	};

	return fmt::replace_all(code, repl_list);
}

std::string VertexProgramDecompiler::GetRawCond()
{
	static const COMPARE cond_string_table[(lt | gt | eq) + 1] =
	{
		COMPARE::SLT, // "error"
		COMPARE::SLT,
		COMPARE::SEQ,
		COMPARE::SLE,
		COMPARE::SGT,
		COMPARE::SNE,
		COMPARE::SGE,
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;
	return compareFunction(cond_string_table[d0.cond], AddCondReg() + swizzle, getFloatTypeName(4) + "(0.)" + swizzle);
}

std::string VertexProgramDecompiler::GetCond()
{
	if (d0.cond == 0) return "false";
	if (d0.cond == (lt | gt | eq)) return "true";

	return "any(" + GetRawCond() + ")";
}

std::string VertexProgramDecompiler::GetOptionalBranchCond() const
{
	std::string cond_operator = d3.brb_cond_true ? " != " : " == ";
	std::string cond = "(transform_branch_bits & (1u << " + std::to_string(d3.branch_index) + "))" + cond_operator + "0";

	return "if (" + cond + ")";
}

void VertexProgramDecompiler::AddCodeCond(const std::string& lhs, const std::string& rhs)
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	if (!d0.cond_test_enable || d0.cond == (lt | gt | eq))
	{
		AddCode(lhs + " = " + rhs + ";");
		return;
	}

	if (d0.cond == 0)
	{
		AddCode("//" + lhs + " = " + rhs + ";");
		return;
	}

	// NOTE: x = _select(x, y, cond) is equivalent to x = cond? y : x;
	const auto dst_var = ShaderVariable(lhs);
	const auto raw_cond = dst_var.add_mask(GetRawCond());
	const auto cond = dst_var.match_size(raw_cond);
	AddCode(lhs + " = _select(" + lhs + ", " + rhs + ", " + cond + ");");
}

std::string VertexProgramDecompiler::AddAddrReg()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	const auto mask = std::string(".") + f[d0.addr_swz];
	return m_parr.AddParam(PF_PARAM_NONE, getIntTypeName(4), "a" + std::to_string(d0.addr_reg_sel_1), getIntTypeName(4) + "(0)") + mask;
}

std::string VertexProgramDecompiler::AddCondReg()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(d0.cond_reg_sel_1), getFloatTypeName(4) + "(0.)");
}

u32 VertexProgramDecompiler::GetAddr() const
{
	return (d0.iaddrh2 << 9) | (d2.iaddrh << 3) | d3.iaddrl;
}

void VertexProgramDecompiler::AddCode(const std::string& code)
{
	m_body.push_back(Format(code) + ";");
	m_cur_instr->body.push_back(Format(code));
}

void VertexProgramDecompiler::SetDSTVec(const std::string& code)
{
	SetDST(false, code);
}

void VertexProgramDecompiler::SetDSTSca(const std::string& code)
{
	SetDST(true, code);
}

std::string VertexProgramDecompiler::NotZeroPositive(const std::string& code)
{
	return "max(" + code + ", 0.0000000001)";
}

std::string VertexProgramDecompiler::BuildCode()
{
	std::string main_body;
	for (int i = 0, lvl = 1; i < static_cast<int>(m_instr_count); i++)
	{
		lvl = std::max<int>(lvl - m_instructions[i].close_scopes, 0);

		for (int j = 0; j < m_instructions[i].put_close_scopes; ++j)
		{
			if (lvl > 1) --lvl;
			main_body.append(lvl, '\t') += "}\n";
		}

		for (int j = 0; j < m_instructions[i].do_count; ++j)
		{
			main_body.append(lvl, '\t') += "do\n";
			main_body.append(lvl, '\t') += "{\n";
			lvl++;
		}

		ensure(lvl >= 0); // Underflow of indent level will cause crashes!!

		for (const auto& instruction_body : m_instructions[i].body)
		{
			main_body.append(lvl, '\t') += instruction_body + "\n";
		}

		lvl += m_instructions[i].open_scopes;
	}

	if (const auto float4_type = getFloatTypeName(4); !m_parr.HasParam(PF_PARAM_OUT, float4_type, "dst_reg0"))
	{
		rsx_log.warning("Vertex program has no POS output, shader will be NOPed");
		main_body = "/*" + main_body + "*/";

		// Initialize vertex output register to all 0, GPU hw does not always clear position register
		m_parr.AddParam(PF_PARAM_OUT, float4_type, "dst_reg0", float4_type + "(0., 0., 0., 1.)");
	}

	if (!properties.has_indexed_constants && !m_constant_ids.empty())
	{
		// Relocate transform constants
		std::vector<std::pair<std::string, std::string>> reloc_table;
		reloc_table.reserve(m_constant_ids.size());

		// Build the string lookup table
		int offset = 0;
		for (const auto& index : m_constant_ids)
		{
			const auto i = offset++;
			if (i == index) continue; // Replace with self
			reloc_table.emplace_back(fmt::format("_fetch_constant(%d)", index), fmt::format("_fetch_constant(%d)", i));
		}

		// One-time patch
		main_body = fmt::replace_all(main_body, reloc_table);

		// Rename the array type
		auto type_list = ensure(m_parr.SearchParam(PF_PARAM_UNIFORM, getFloatTypeName(4)));
		const auto item = ParamItem(fmt::format("vc[%llu]", m_constant_ids.size()), -1);
		type_list->ReplaceOrInsert("vc[468]", item);
	}

	std::stringstream OS;
	insertHeader(OS);

	insertInputs(OS, m_parr.params[PF_PARAM_IN]);
	OS << std::endl;
	insertOutputs(OS, m_parr.params[PF_PARAM_NONE]);
	OS << std::endl;
	insertConstants(OS, m_parr.params[PF_PARAM_UNIFORM]);
	OS << std::endl;

	insertMainStart(OS);
	OS << main_body.c_str() << std::endl;
	insertMainEnd(OS);

	return OS.str();
}

VertexProgramDecompiler::VertexProgramDecompiler(const RSXVertexProgram& prog) :
	m_prog(prog)
{
}

std::string VertexProgramDecompiler::Decompile()
{
	const auto& data = m_prog.data;
	m_instr_count = data.size() / 4;

	bool has_BRA = false;
	bool program_end = false;
	u32 i = 1;
	u32 last_label_addr = 0;

	for (auto& param : m_parr.params)
	{
		param.clear();
	}

	for (auto& instruction : m_instructions)
	{
		instruction.reset();
	}

	if (!m_prog.jump_table.empty())
	{
		last_label_addr = *m_prog.jump_table.rbegin();
	}

	auto find_jump_lvl = [this](u32 address) -> u32
	{
		u32 jump = 1;

		for (auto pos : m_prog.jump_table)
		{
			if (address == pos)
				return jump;

			++jump;
		}

		return -1;
	};

	auto do_function_call = [this, &i](const std::string& condition)
	{
		// Call function
		// NOTE: Addresses are assumed to have been patched
		m_call_stack.push(i+1);
		AddCode(condition);
		AddCode("{");
		m_cur_instr->open_scopes++;
		i = GetAddr();
	};

	auto do_function_return = [this, &i]()
	{
		if (!m_call_stack.empty())
		{
			//TODO: Conditional returns
			i = m_call_stack.top();
			m_call_stack.pop();
			m_cur_instr->close_scopes++;
			AddCode("}");
		}
		else
		{
			AddCode("$ifcond return");
		}
	};

	auto do_program_exit = [this, do_function_return, &i](bool abort)
	{
		if (abort)
		{
			AddCode("//ABORT");
		}

		while (!m_call_stack.empty())
		{
			rsx_log.error("vertex program end in subroutine call!");
			do_function_return();
		}

		if ((i + 1) < m_instr_count)
		{
			//Forcefully exit
			AddCode("return;");
		}
	};

	if (has_BRA || !m_prog.jump_table.empty())
	{
		m_cur_instr = &m_instructions[0];

		u32 jump_position = 0;
		if (m_prog.entry != m_prog.base_address)
		{
			jump_position = find_jump_lvl(m_prog.entry - m_prog.base_address);
			ensure(jump_position != umax);
		}

		AddCode(fmt::format("int jump_position = %u;", jump_position));
		AddCode("while (true)");
		AddCode("{");
		m_cur_instr->open_scopes++;

		AddCode("if (jump_position <= 0)");
		AddCode("{");
		m_cur_instr->open_scopes++;
	}

	for (i = 0; i < m_instr_count; ++i)
	{
		if (!m_prog.instruction_mask[i])
		{
			// Dead code, skip
			continue;
		}

		m_cur_instr = &m_instructions[i];

		d0.HEX = data[i * 4 + 0];
		d1.HEX = data[i * 4 + 1];
		d2.HEX = data[i * 4 + 2];
		d3.HEX = data[i * 4 + 3];

		src[0].src0l = d2.src0l;
		src[0].src0h = d1.src0h;
		src[1].src1 = d2.src1;
		src[2].src2l = d3.src2l;
		src[2].src2h = d2.src2h;

		if (m_call_stack.empty() && i)
		{
			//TODO: Subroutines can also have arbitrary jumps!
			u32 jump_position = find_jump_lvl(i);
			if (has_BRA || jump_position != umax)
			{
				m_cur_instr->close_scopes++;
				AddCode("}");
				AddCode("");

				AddCode(fmt::format("if (jump_position <= %u)", jump_position));
				AddCode("{");
				m_cur_instr->open_scopes++;
			}
		}

		if (!src[0].reg_type || !src[1].reg_type || !src[2].reg_type)
		{
			AddCode("//Src check failed. Aborting");
			program_end = true;
			d1.vec_opcode = d1.sca_opcode = 0;
		}

		switch (d1.vec_opcode)
		{
		case RSX_VEC_OPCODE_NOP: break;
		case RSX_VEC_OPCODE_MOV: SetDSTVec("$0"); break;
		case RSX_VEC_OPCODE_MUL: SetDSTVec("($0 * $1)"); break;
		case RSX_VEC_OPCODE_ADD: SetDSTVec("($0 + $2)"); break;
		case RSX_VEC_OPCODE_MAD: SetDSTVec("fma($0, $1, $2)"); break;
		case RSX_VEC_OPCODE_DP3: SetDSTVec(getFunction(FUNCTION::DP3)); break;
		case RSX_VEC_OPCODE_DPH: SetDSTVec(getFunction(FUNCTION::DPH)); break;
		case RSX_VEC_OPCODE_DP4: SetDSTVec(getFunction(FUNCTION::DP4)); break;
		case RSX_VEC_OPCODE_DST: SetDSTVec("vec4(1.0, $0.y * $1.y, $0.z, $1.w)"); break;
		case RSX_VEC_OPCODE_MIN: SetDSTVec("min($0, $1)"); break;
		case RSX_VEC_OPCODE_MAX: SetDSTVec("max($0, $1)"); break;
		case RSX_VEC_OPCODE_SLT: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SLT, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SGE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SGE, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_ARL: SetDSTVec(getIntTypeName(4) + "($0)");  break;
		case RSX_VEC_OPCODE_FRC: SetDSTVec(getFunction(FUNCTION::FRACT)); break;
		case RSX_VEC_OPCODE_FLR: SetDSTVec("floor($0)"); break;
		case RSX_VEC_OPCODE_SEQ: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SEQ, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SFL: SetDSTVec(getFunction(FUNCTION::SFL)); break;
		case RSX_VEC_OPCODE_SGT: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SGT, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SLE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SLE, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SNE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::SNE, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_STR: SetDSTVec(getFunction(FUNCTION::STR)); break;
		case RSX_VEC_OPCODE_SSG: SetDSTVec("sign($0)"); break;
		case RSX_VEC_OPCODE_TXL:
		{
			GetTex();
			const bool is_multisampled = m_prog.texture_state.multisampled_textures & (1 << d2.tex_num);

			switch (m_prog.get_texture_dimension(d2.tex_num))
			{
			case rsx::texture_dimension_extended::texture_dimension_1d:
				SetDSTVec(is_multisampled ? getFunction(FUNCTION::VERTEX_TEXTURE_FETCH2DMS) : getFunction(FUNCTION::VERTEX_TEXTURE_FETCH1D));
				break;
			case rsx::texture_dimension_extended::texture_dimension_2d:
				SetDSTVec(getFunction(is_multisampled ? FUNCTION::VERTEX_TEXTURE_FETCH2DMS : FUNCTION::VERTEX_TEXTURE_FETCH2D));
				break;
			case rsx::texture_dimension_extended::texture_dimension_3d:
				SetDSTVec(getFunction(FUNCTION::VERTEX_TEXTURE_FETCH3D));
				break;
			case rsx::texture_dimension_extended::texture_dimension_cubemap:
				SetDSTVec(getFunction(FUNCTION::VERTEX_TEXTURE_FETCHCUBE));
				break;
			}

			break;
		}
		default:
			AddCode(fmt::format("//Unknown vp opcode 0x%x", u32{ d1.vec_opcode }));
			rsx_log.error("Unknown vp opcode 0x%x", u32{ d1.vec_opcode });
			program_end = true;
			break;
		}

		//NOTE: Branch instructions have to be decoded last in case there was a dual-issued instruction!
		switch (d1.sca_opcode)
		{
		case RSX_SCA_OPCODE_NOP: break;
		case RSX_SCA_OPCODE_MOV: SetDSTSca("$s"); break;
		case RSX_SCA_OPCODE_RCP: SetDSTSca("(1.0 / $s)"); break;
		case RSX_SCA_OPCODE_RCC: SetDSTSca("clamp(1.0 / $s, 5.42101e-20, 1.884467e19)"); break;
		case RSX_SCA_OPCODE_RSQ: SetDSTSca("1. / sqrt(" + NotZeroPositive("$s.x") +").xxxx"); break;
		case RSX_SCA_OPCODE_EXP: SetDSTSca("exp($s)"); break;
		case RSX_SCA_OPCODE_LOG: SetDSTSca("log($s)"); break;
		case RSX_SCA_OPCODE_LIT:
			SetDSTSca("lit_legacy($s)");
			properties.has_lit_op = true;
			break;
		case RSX_SCA_OPCODE_BRA:
		{
			if (m_call_stack.empty())
			{
				AddCode("$ifcond //BRA");
				AddCode("{");
				m_cur_instr->open_scopes++;
				AddCode("jump_position = $a;");
				AddCode("continue;");
				m_cur_instr->close_scopes++;
				AddCode("}");
			}
			else
			{
				//TODO
				rsx_log.error("BRA opcode found in subroutine!");
			}
		}
		break;
		case RSX_SCA_OPCODE_BRI: // works differently (BRI o[1].x(TR) L0;)
		{
			if (m_call_stack.empty())
			{
				u32 jump_position = find_jump_lvl(GetAddr());

				AddCode("$ifcond //BRI");
				AddCode("{");
				m_cur_instr->open_scopes++;
				AddCode(fmt::format("jump_position = %u;", jump_position));
				AddCode("continue;");
				m_cur_instr->close_scopes++;
				AddCode("}");
			}
			else
			{
				//TODO
				rsx_log.error("BRI opcode found in subroutine!");
			}
		}
		break;
		case RSX_SCA_OPCODE_CAL:
			// works same as BRI
			AddCode("//CAL");
			do_function_call("$ifcond");
			break;
		case RSX_SCA_OPCODE_CLI:
			// works same as BRI
			rsx_log.error("Unimplemented VP opcode CLI");
			AddCode("//CLI");
			do_function_call("$ifcond");
			break;
		case RSX_SCA_OPCODE_RET:
			// works like BRI but shorter (RET o[1].x(TR);)
			do_function_return();
			break;
		case RSX_SCA_OPCODE_LG2: SetDSTSca("log2(" + NotZeroPositive("$s") + ")"); break;
		case RSX_SCA_OPCODE_EX2: SetDSTSca("exp2($s)"); break;
		case RSX_SCA_OPCODE_SIN: SetDSTSca("sin($s)"); break;
		case RSX_SCA_OPCODE_COS: SetDSTSca("cos($s)"); break;
		case RSX_SCA_OPCODE_BRB:
			// works differently (BRB o[1].x !b0, L0;)
		{
			if (m_call_stack.empty())
			{
				u32 jump_position = find_jump_lvl(GetAddr());

				AddCode("$ifbcond //BRB");
				AddCode("{");
				m_cur_instr->open_scopes++;
				AddCode(fmt::format("jump_position = %u;", jump_position));
				AddCode("continue;");
				m_cur_instr->close_scopes++;
				AddCode("}");
				AddCode("");
			}
			else
			{
				//TODO
				rsx_log.error("BRA opcode found in subroutine!");
			}

			break;
		}
		case RSX_SCA_OPCODE_CLB:
			// works same as BRB
			AddCode("//CLB");
			do_function_call("$ifbcond");
			break;
		case RSX_SCA_OPCODE_PSH:
			// works differently (PSH o[1].x A0;)
			rsx_log.error("Unimplemented sca_opcode PSH");
			break;
		case RSX_SCA_OPCODE_POP:
			// works differently (POP o[1].x;)
			rsx_log.error("Unimplemented sca_opcode POP");
			break;

		default:
			AddCode(fmt::format("//Unknown vp sca_opcode 0x%x", u32{ d1.sca_opcode }));
			rsx_log.error("Unknown vp sca_opcode 0x%x", u32{ d1.sca_opcode });
			program_end = true;
			break;
		}

		if (program_end || !!d3.end)
		{
			do_program_exit(!d3.end);

			if (i >= last_label_addr)
			{
				if ((i + 1) < m_instr_count)
				{
					// In rare cases, this might be harmless (large coalesced program blocks controlled via branches aka ubershaders)
					rsx_log.error("Vertex program block aborts prematurely. Expect glitches");
				}

				break;
			}
		}
	}

	if (has_BRA || !m_prog.jump_table.empty())
	{
		m_cur_instr = &m_instructions[m_instr_count - 1];
		m_cur_instr->close_scopes++;
		AddCode("}");
		AddCode("break;");
		m_cur_instr->close_scopes++;
		AddCode("}");
	}

	std::string result = BuildCode();

	m_body.clear();
	return result;
}
