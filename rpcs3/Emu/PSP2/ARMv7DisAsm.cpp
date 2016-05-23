#include "stdafx.h"
#include "ARMv7Opcodes.h"
#include "ARMv7DisAsm.h"

using namespace arm_code::arm_encoding_alias;

const arm_decoder<ARMv7DisAsm> s_arm_disasm;

template<arm_encoding type>
static const char* fmt_encoding()
{
	switch (type)
	{
	case T1: return "T1";
	case T2: return "T2";
	case T3: return "T3";
	case T4: return "T4";
	case A1: return "A1";
	case A2: return "A2";
	}

	return nullptr;
}

static const char* fmt_cond(u32 cond)
{
	switch (cond)
	{
	case 0: return "eq";
	case 1: return "ne";
	case 2: return "cs";
	case 3: return "cc";
	case 4: return "mi";
	case 5: return "pl";
	case 6: return "vs";
	case 7: return "vc";
	case 8: return "hi";
	case 9: return "ls";
	case 10: return "ge";
	case 11: return "lt";
	case 12: return "gt";
	case 13: return "le";
	case 14: return "al";
	case 15: return "";
	default: return "??";
	}
}

static const char* fmt_it(u32 state)
{
	switch (state & ~0x10)
	{
	case 0x8: return "";

	case 0x4: return state & 0x10 ? "e" : "t";
	case 0xc: return state & 0x10 ? "t" : "e";

	case 0x2: return state & 0x10 ? "ee" : "tt";
	case 0x6: return state & 0x10 ? "et" : "te";
	case 0xa: return state & 0x10 ? "te" : "et";
	case 0xe: return state & 0x10 ? "tt" : "ee";

	case 0x1: return state & 0x10 ? "eee" : "ttt";
	case 0x3: return state & 0x10 ? "eet" : "tte";
	case 0x5: return state & 0x10 ? "ete" : "tet";
	case 0x7: return state & 0x10 ? "ett" : "tee";
	case 0x9: return state & 0x10 ? "tee" : "ett";
	case 0xb: return state & 0x10 ? "tet" : "ete";
	case 0xd: return state & 0x10 ? "tte" : "eet";
	case 0xf: return state & 0x10 ? "ttt" : "eee";

	default: return "???";
	}
}

static const char* fmt_reg(u32 reg)
{
	switch (reg)
	{
	case 0: return "r0";
	case 1: return "r1";
	case 2: return "r2";
	case 3: return "r3";
	case 4: return "r4";
	case 5: return "r5";
	case 6: return "r6";
	case 7: return "r7";
	case 8: return "r8";
	case 9: return "r9";
	case 10: return "r10";
	case 11: return "r11";
	case 12: return "r12";
	case 13: return "sp";
	case 14: return "lr";
	case 15: return "pc";
	default: return "r???";
	}
}

static std::string fmt_shift(u32 type, u32 amount)
{
	EXPECTS(type != arm_code::SRType_RRX || amount == 1);
	EXPECTS(amount <= 32);

	if (amount)
	{
		switch (type)
		{
		case arm_code::SRType_LSL: return fmt::format(",lsl #%u", amount);
		case arm_code::SRType_LSR: return fmt::format(",lsr #%u", amount);
		case arm_code::SRType_ASR: return fmt::format(",asr #%u", amount);
		case arm_code::SRType_ROR: return fmt::format(",ror #%u", amount);
		case arm_code::SRType_RRX: return ",rrx";
		default: return ",?????";
		}
	}

	return{};
}

static std::string fmt_reg_list(u32 reg_list)
{
	std::vector<std::pair<u32, u32>> lines;

	for (u32 i = 0; i < 13; i++)
	{
		if (reg_list & (1 << i))
		{
			if (lines.size() && lines.rbegin()->second == i - 1)
			{
				lines.rbegin()->second = i;
			}
			else
			{
				lines.push_back({ i, i });
			}
		}
	}

	if (reg_list & 0x2000) lines.push_back({ 13, 13 }); // sp
	if (reg_list & 0x4000) lines.push_back({ 14, 14 }); // lr
	if (reg_list & 0x8000) lines.push_back({ 15, 15 }); // pc

	std::string result;

	if (reg_list >> 16) result = "???"; // invalid bits		

	for (auto& line : lines)
	{
		if (!result.empty())
		{
			result += ",";
		}

		if (line.first == line.second)
		{
			result += fmt_reg(line.first);
		}
		else
		{
			result += fmt_reg(line.first);
			result += '-';
			result += fmt_reg(line.second);
		}
	}

	return result;
}

static std::string fmt_mem_imm(u32 reg, u32 imm, u32 index, u32 add, u32 wback)
{
	if (index)
	{
		return fmt::format("[%s,#%s0x%X]%s", fmt_reg(reg), add ? "" : "-", imm, wback ? "!" : "");
	}
	else
	{
		return fmt::format("[%s],#%s0x%X%s", fmt_reg(reg), add ? "" : "-", imm, wback ? "" : "???");
	}
}

static std::string fmt_mem_reg(u32 n, u32 m, u32 index, u32 add, u32 wback, u32 shift_t = 0, u32 shift_n = 0)
{
	if (index)
	{
		return fmt::format("[%s,%s%s%s]%s", fmt_reg(n), add ? "" : "-", fmt_reg(m), fmt_shift(shift_t, shift_n), wback ? "!" : "");
	}
	else
	{
		return fmt::format("[%s],%s%s%s%s", fmt_reg(n), add ? "" : "-", fmt_reg(m), fmt_shift(shift_t, shift_n), wback ? "" : "???");
	}
}

u32 ARMv7DisAsm::disasm(u32 pc)
{
	const u16 op16 = *(le_t<u16>*)(offset + pc);
	const u32 cond = -1; // TODO

	if (const auto func16 = s_arm_disasm.decode_thumb(op16))
	{
		(this->*func16)(op16, cond);
		return 2;
	}
	else
	{
		const u32 op32 = (op16 << 16) | *(le_t<u16>*)(offset + pc + 2);
		(this->*s_arm_disasm.decode_thumb(op32))(op32, cond);
		return 4;
	}
}

void ARMv7DisAsm::Write(const std::string& value)
{
	switch (m_mode)
	{
	case CPUDisAsm_DumpMode:
		last_opcode = fmt::format("\t%08x:\t", dump_pc);
		break;

	case CPUDisAsm_InterpreterMode:
		last_opcode = fmt::format("[%08x]  ", dump_pc);
		break;

	case CPUDisAsm_CompilerElfMode:
		last_opcode = value + "\n";
		return;
	}

	const u16 op16 = *(le_t<u16>*)(offset + dump_pc);

	// TODO: ARM
	if (false)
	{
		const u32 op_arm = *(le_t<u32>*)(offset + dump_pc);

		last_opcode += fmt::format("%08x  ", op_arm);
	}
	else if (arm_op_thumb_is_32(op16))
	{
		const u16 op_second = *(le_t<u16>*)(offset + dump_pc + 2);

		last_opcode += fmt::format("%04x %04x ", op16, op_second);
	}
	else
	{
		last_opcode += fmt::format("%04x      ", op16);
	}

	auto str = value;
	const auto found = str.find_first_of(' ');
	if (found < 10) str.insert(str.begin() + found, 10 - found, ' ');

	switch (m_mode)
	{
	case CPUDisAsm_DumpMode:
		last_opcode += fmt::format("\t%s\n", str);
		break;

	case CPUDisAsm_InterpreterMode:
		last_opcode += fmt::format(": %s", str);
		break;
	}
}

#define ARG(arg, ...) const u32 arg = args::arg::extract(__VA_ARGS__);

void ARMv7DisAsm::UNK(const u32 op, const u32 cond)
{
	// TODO: ARM
	if (false)
	{
		write("Unknown/Illegal opcode: 0x%08X (ARM)", op);
	}
	else if (op > 0xffff)
	{
		write("Unknown/Illegal opcode: 0x%04X 0x%04X (Thumb)", op >> 16, op & 0xffff);
	}
	else
	{
		write("Unknown/Illegal opcode: 0x%04X (Thumb)", op);
	}
	
}

template<arm_encoding type>
void ARMv7DisAsm::HACK(const u32 op, const u32 cond)
{
	using args = arm_code::hack<type>;
	ARG(index, op);

	write("hack%s %d", fmt_cond(cond), index);
}

template<arm_encoding type>
void ARMv7DisAsm::MRC_(const u32 op, const u32 cond)
{
	using args = arm_code::mrc<type>;
	ARG(t, op);
	ARG(cp, op);
	ARG(opc1, op);
	ARG(opc2, op);
	ARG(cn, op);
	ARG(cm, op);

	write("mrc%s p%d,%d,r%d,c%d,c%d,%d", fmt_cond(cond), cp, opc1, t, cn, cm, opc2);
}


template<arm_encoding type>
void ARMv7DisAsm::ADC_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::adc_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("adc%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::ADC_REG(const u32 op, const u32 cond)
{
	using args = arm_code::adc_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("adc%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::ADC_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::ADD_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::add_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("add%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::ADD_REG(const u32 op, const u32 cond)
{
	using args = arm_code::add_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("add%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::ADD_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::ADD_SPI(const u32 op, const u32 cond)
{
	using args = arm_code::add_spi<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("add%s%s %s,sp,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::ADD_SPR(const u32 op, const u32 cond)
{
	using args = arm_code::add_spr<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("add%s%s %s,sp,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m), fmt_shift(shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::ADR(const u32 op, const u32 cond)
{
	using args = arm_code::adr<type>;
	ARG(d, op);
	ARG(i, op);

	write("adr%s r%d, 0x%08X", fmt_cond(cond), d, (DisAsmBranchTarget(0) & ~3) + i);
}


template<arm_encoding type>
void ARMv7DisAsm::AND_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::and_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("and%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::AND_REG(const u32 op, const u32 cond)
{
	using args = arm_code::and_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("and%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::AND_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::ASR_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::ASR_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::B(const u32 op, const u32 cond)
{
	using args = arm_code::b<type>;
	ARG(imm32, op);

	write("b%s 0x%08X", fmt_cond(cond), DisAsmBranchTarget(imm32));
}


template<arm_encoding type>
void ARMv7DisAsm::BFC(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::BFI(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::BIC_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::bic_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("bic%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::BIC_REG(const u32 op, const u32 cond)
{
	using args = arm_code::bic_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("bic%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::BIC_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::BKPT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::BL(const u32 op, const u32 cond)
{
	using args = arm_code::bl<type>;
	ARG(imm32, op);
	ARG(to_arm);

	write("bl%s%s 0x%08X", (to_arm != 0) != (type >= A1) ? "x" : "", fmt_cond(cond), DisAsmBranchTarget(imm32));
}

template<arm_encoding type>
void ARMv7DisAsm::BLX(const u32 op, const u32 cond)
{
	using args = arm_code::blx<type>;
	ARG(m, op);

	write("blx%s %s", fmt_cond(cond), fmt_reg(m));
}

template<arm_encoding type>
void ARMv7DisAsm::BX(const u32 op, const u32 cond)
{
	using args = arm_code::bx<type>;
	ARG(m, op);

	write("bx%s %s", fmt_cond(cond), fmt_reg(m));
}


template<arm_encoding type>
void ARMv7DisAsm::CB_Z(const u32 op, const u32 cond)
{
	using args = arm_code::cb_z<type>;
	ARG(n, op);
	ARG(imm32, op);
	ARG(nonzero, op);

	write("cb%sz 0x%08X", nonzero ? "n" : "", DisAsmBranchTarget(imm32));
}


template<arm_encoding type>
void ARMv7DisAsm::CLZ(const u32 op, const u32 cond)
{
	using args = arm_code::clz<type>;
	ARG(d, op);
	ARG(m, op);

	write("clz%s %s,%s", fmt_cond(cond), fmt_reg(d), fmt_reg(m));
}


template<arm_encoding type>
void ARMv7DisAsm::CMN_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::CMN_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::CMN_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::CMP_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::cmp_imm<type>;
	ARG(n, op);
	ARG(imm32, op);

	write("cmp%s %s,#0x%X", fmt_cond(cond), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::CMP_REG(const u32 op, const u32 cond)
{
	using args = arm_code::cmp_reg<type>;
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);

	write("cmp%s %s,%s%s", fmt_cond(cond), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::CMP_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::DBG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::DMB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::DSB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::EOR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::eor_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("eor%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::EOR_REG(const u32 op, const u32 cond)
{
	using args = arm_code::eor_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("eor%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::EOR_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::IT(const u32 op, const u32 cond)
{
	static_assert(type == T1, "IT");

	const u32 mask = (op & 0xf);
	const u32 first = (op & 0xf0) >> 4;

	write("IT%s %s", fmt_it(mask), fmt_cond(first));
}


template<arm_encoding type>
void ARMv7DisAsm::LDM(const u32 op, const u32 cond)
{
	using args = arm_code::ldm<type>;
	ARG(n, op);
	ARG(registers, op);
	ARG(wback, op);

	write("ldm%s %s%s,{%s}", fmt_cond(cond), fmt_reg(n), wback ? "!" : "", fmt_reg_list(registers));
}

template<arm_encoding type>
void ARMv7DisAsm::LDMDA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDMDB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDMIB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LDR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ldr_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldr%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::LDR_LIT(const u32 op, const u32 cond)
{
	using args = arm_code::ldr_lit<type>;
	ARG(t, op);
	ARG(imm32, op);
	ARG(add, op);

	const u32 base = DisAsmBranchTarget(0) & ~3;
	const u32 addr = add ? base + imm32 : base - imm32;

	write("ldr%s %s,0x%08X", fmt_cond(cond), fmt_reg(t), addr);
}

template<arm_encoding type>
void ARMv7DisAsm::LDR_REG(const u32 op, const u32 cond)
{
	using args = arm_code::ldr_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldr%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_reg(n, m, index, add, wback, shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::LDRB_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ldrb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldrb%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::LDRB_LIT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDRB_REG(const u32 op, const u32 cond)
{
	using args = arm_code::ldrb_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldrb%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_reg(n, m, index, add, wback, shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::LDRD_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ldrd_imm<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldrd%s %s,%s,%s", fmt_cond(cond), fmt_reg(t), fmt_reg(t2), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::LDRD_LIT(const u32 op, const u32 cond)
{
	using args = arm_code::ldrd_lit<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(imm32, op);
	ARG(add, op);

	const u32 base = DisAsmBranchTarget(0) & ~3;
	const u32 addr = add ? base + imm32 : base - imm32;

	write("ldrd%s %s,%s,0x%08X", fmt_cond(cond), fmt_reg(t), fmt_reg(t2), addr);
}

template<arm_encoding type>
void ARMv7DisAsm::LDRD_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LDRH_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ldrh_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldrh%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::LDRH_LIT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDRH_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LDRSB_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ldrsb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("ldrsb%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::LDRSB_LIT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDRSB_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LDRSH_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDRSH_LIT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDRSH_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LDREX(const u32 op, const u32 cond)
{
	using args = arm_code::ldrex<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);

	write("ldrex%s %s,[%s,#0x%X]", fmt_cond(cond), fmt_reg(t), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::LDREXB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDREXD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::LDREXH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::LSL_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::lsl_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("lsl%s%s %s,%s,#%d", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m), shift_n);
}

template<arm_encoding type>
void ARMv7DisAsm::LSL_REG(const u32 op, const u32 cond)
{
	using args = arm_code::lsl_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	write("lsl%s%s %s,%s,%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m));
}


template<arm_encoding type>
void ARMv7DisAsm::LSR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::lsr_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("lsr%s%s %s,%s,#%d", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m), shift_n);
}

template<arm_encoding type>
void ARMv7DisAsm::LSR_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::MLA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::MLS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::MOV_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::mov_imm<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	//switch (type)
	//{
	//case T3:
	//case A2: write("movw%s%s %s,#0x%04X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), imm32); break;
	//default: write("mov%s%s %s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), imm32);
	//}
}

template<arm_encoding type>
void ARMv7DisAsm::MOV_REG(const u32 op, const u32 cond)
{
	using args = arm_code::mov_reg<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	write("mov%s%s %s,%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m));
}

template<arm_encoding type>
void ARMv7DisAsm::MOVT(const u32 op, const u32 cond)
{
	using args = arm_code::movt<type>;
	ARG(d, op);
	ARG(imm16, op);

	write("movt%s %s,#0x%04X", fmt_cond(cond), fmt_reg(d), imm16);
}


template<arm_encoding type>
void ARMv7DisAsm::MRS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::MSR_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::MSR_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::MUL(const u32 op, const u32 cond)
{
	using args = arm_code::mul<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	write("mul%s%s %s,%s,%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m));
}


template<arm_encoding type>
void ARMv7DisAsm::MVN_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::mvn_imm<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("mvn%s%s %s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::MVN_REG(const u32 op, const u32 cond)
{
	using args = arm_code::mvn_reg<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("mvn%s%s %s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::MVN_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::NOP(const u32 op, const u32 cond)
{
	write("nop%s", fmt_cond(cond));
}


template<arm_encoding type>
void ARMv7DisAsm::ORN_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::ORN_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::ORR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::orr_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("orr%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::ORR_REG(const u32 op, const u32 cond)
{
	using args = arm_code::orr_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("orr%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::ORR_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::PKH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::POP(const u32 op, const u32 cond)
{
	const u32 registers = arm_code::pop<type>::registers::extract(op);

	write("pop%s {%s}", fmt_cond(cond), fmt_reg_list(registers));
}

template<arm_encoding type>
void ARMv7DisAsm::PUSH(const u32 op, const u32 cond)
{
	const u32 registers = arm_code::push<type>::registers::extract(op);

	write("push%s {%s}", fmt_cond(cond), fmt_reg_list(registers));
}


template<arm_encoding type>
void ARMv7DisAsm::QADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QDADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QDSUB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QSAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QSUB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QSUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::QSUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::RBIT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::REV(const u32 op, const u32 cond)
{
	using args = arm_code::rev<type>;
	ARG(d, op);
	ARG(m, op);

	write("rev%s %s,%s", fmt_cond(cond), fmt_reg(d), fmt_reg(m));
}

template<arm_encoding type>
void ARMv7DisAsm::REV16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::REVSH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::ROR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::ror_imm<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("ror%s%s %s,%s,#%d", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(m), shift_n);
}

template<arm_encoding type>
void ARMv7DisAsm::ROR_REG(const u32 op, const u32 cond)
{
	using args = arm_code::ror_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	write("ror%s%s %s,%s,%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m));
}


template<arm_encoding type>
void ARMv7DisAsm::RRX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::RSB_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::rsb_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("rsb%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::RSB_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::RSB_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::RSC_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::RSC_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::RSC_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SBC_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SBC_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SBC_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SBFX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SDIV(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SEL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SHADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SHADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SHASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SHSAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SHSUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SHSUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SMLA__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLAD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLAL__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLALD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLAW_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLSD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMLSLD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMMLA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMMLS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMMUL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMUAD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMUL__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMULL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMULW_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SMUSD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SSAT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SSAT16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SSAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SSUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SSUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::STM(const u32 op, const u32 cond)
{
	using args = arm_code::stm<type>;
	ARG(n, op);
	ARG(registers, op);
	ARG(wback, op);

	write("stm%s %s%s,{%s}", fmt_cond(cond), fmt_reg(n), wback ? "!" : "", fmt_reg_list(registers));
}

template<arm_encoding type>
void ARMv7DisAsm::STMDA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::STMDB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::STMIB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::STR_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::str_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("str%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::STR_REG(const u32 op, const u32 cond)
{
	using args = arm_code::str_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("str%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_reg(n, m, index, add, wback, shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::STRB_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::strb_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("strb%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::STRB_REG(const u32 op, const u32 cond)
{
	using args = arm_code::strb_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("strb%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_reg(n, m, index, add, wback, shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::STRD_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::strd_imm<type>;
	ARG(t, op);
	ARG(t2, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("strd%s %s,%s,%s", fmt_cond(cond), fmt_reg(t), fmt_reg(t2), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::STRD_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::STRH_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::strh_imm<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("strh%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_imm(n, imm32, index, add, wback));
}

template<arm_encoding type>
void ARMv7DisAsm::STRH_REG(const u32 op, const u32 cond)
{
	using args = arm_code::strh_reg<type>;
	ARG(t, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(index, op);
	ARG(add, op);
	ARG(wback, op);

	write("strh%s %s,%s", fmt_cond(cond), fmt_reg(t), fmt_mem_reg(n, m, index, add, wback, shift_t, shift_n));
}


template<arm_encoding type>
void ARMv7DisAsm::STREX(const u32 op, const u32 cond)
{
	using args = arm_code::strex<type>;
	ARG(d, op);
	ARG(t, op);
	ARG(n, op);
	ARG(imm32, op);

	write("strex%s %s,%s,[%s,#0x%x]", fmt_cond(cond), fmt_reg(d), fmt_reg(t), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::STREXB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::STREXD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::STREXH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SUB_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::sub_imm<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("sub%s%s %s,%s,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::SUB_REG(const u32 op, const u32 cond)
{
	using args = arm_code::sub_reg<type>;
	ARG(d, op);
	ARG(n, op);
	ARG(m, op);
	ARG(shift_t, op);
	ARG(shift_n, op);
	ARG(set_flags, op, cond);

	write("sub%s%s %s,%s,%s%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), fmt_reg(n), fmt_reg(m), fmt_shift(shift_t, shift_n));
}

template<arm_encoding type>
void ARMv7DisAsm::SUB_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SUB_SPI(const u32 op, const u32 cond)
{
	using args = arm_code::sub_spi<type>;
	ARG(d, op);
	ARG(imm32, op);
	ARG(set_flags, op, cond);

	write("sub%s%s %s,sp,#0x%X", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::SUB_SPR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SVC(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::SXTAB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SXTAB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SXTAH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SXTB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SXTB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::SXTH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::TB_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::TEQ_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::TEQ_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::TEQ_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::TST_IMM(const u32 op, const u32 cond)
{
	using args = arm_code::tst_imm<type>;
	ARG(n, op);
	ARG(imm32, op);

	write("tst%s %s,#0x%X", fmt_cond(cond), fmt_reg(n), imm32);
}

template<arm_encoding type>
void ARMv7DisAsm::TST_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::TST_RSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::UADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UBFX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UDIV(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHSAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHSUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UHSUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UMAAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UMLAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UMULL(const u32 op, const u32 cond)
{
	using args = arm_code::umull<type>;
	ARG(d0, op);
	ARG(d1, op);
	ARG(n, op);
	ARG(m, op);
	ARG(set_flags, op, cond);

	write("umull%s%s %s,%s,%s,%s", set_flags ? "s" : "", fmt_cond(cond), fmt_reg(d0), fmt_reg(d1), fmt_reg(n), fmt_reg(m));
}

template<arm_encoding type>
void ARMv7DisAsm::UQADD16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UQADD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UQASX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UQSAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UQSUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UQSUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USAD8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USADA8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USAT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USAT16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USAX(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USUB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::USUB8(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UXTAB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UXTAB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UXTAH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UXTB(const u32 op, const u32 cond)
{
	using args = arm_code::uxtb<type>;
	ARG(d, op);
	ARG(m, op);
	ARG(rotation, op);

	write("uxtb%s %s,%s%s", fmt_cond(cond), fmt_reg(d), fmt_reg(m), fmt_shift(arm_code::SRType_ROR, rotation));
}

template<arm_encoding type>
void ARMv7DisAsm::UXTB16(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::UXTH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::VABA_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VABD_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VABD_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VABS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VAC__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VADD_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VADDHN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VADD_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VAND(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VBIC_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VBIC_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VB__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCEQ_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCEQ_ZERO(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCGE_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCGE_ZERO(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCGT_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCGT_ZERO(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCLE_ZERO(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCLS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCLT_ZERO(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCLZ(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCMP_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCNT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_FIA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_FIF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_FFA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_FFF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_DF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_HFA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VCVT_HFF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VDIV(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VDUP_S(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VDUP_R(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VEOR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VEXT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VHADDSUB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD__MS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD1_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD1_SAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD2_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD2_SAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD3_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD3_SAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD4_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLD4_SAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLDM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VLDR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMAXMIN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMAXMIN_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VML__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VML__FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VML__S(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_RS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_SR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_RF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_2RF(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOV_2RD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOVL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMOVN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMRS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMSR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMUL_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMUL_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMUL_S(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMVN_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VMVN_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VNEG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VNM__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VORN_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VORR_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VORR_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPADAL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPADD_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPADDL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPMAXMIN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPMAXMIN_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPOP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VPUSH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQABS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQDML_L(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQDMULH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQDMULL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQMOV_N(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQNEG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQRDMULH(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQRSHL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQRSHR_N(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQSHL_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQSHL_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQSHR_N(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VQSUB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRADDHN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRECPE(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRECPS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VREV__(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRHADD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSHL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSHR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSHRN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSQRTE(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSQRTS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSRA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VRSUBHN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSHL_IMM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSHL_REG(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSHLL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSHR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSHRN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSLI(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSQRT(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSRA(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSRI(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VST__MS(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VST1_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VST2_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VST3_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VST4_SL(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSTM(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSTR(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSUB(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSUB_FP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSUBHN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSUB_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VSWP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VTB_(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VTRN(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VTST(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VUZP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::VZIP(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template<arm_encoding type>
void ARMv7DisAsm::WFE(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::WFI(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}

template<arm_encoding type>
void ARMv7DisAsm::YIELD(const u32 op, const u32 cond)
{
	write("%s<%s>", __func__, fmt_encoding<type>());
}


template void ARMv7DisAsm::HACK<T1>(const u32, const u32);
template void ARMv7DisAsm::HACK<A1>(const u32, const u32);
template void ARMv7DisAsm::ADC_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ADC_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::ADC_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ADC_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::ADC_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::ADC_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::ADD_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ADD_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::ADD_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::ADD_IMM<T4>(const u32, const u32);
template void ARMv7DisAsm::ADD_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::ADD_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ADD_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::ADD_REG<T3>(const u32, const u32);
template void ARMv7DisAsm::ADD_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::ADD_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPI<T1>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPI<T2>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPI<T3>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPI<T4>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPI<A1>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPR<T1>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPR<T2>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPR<T3>(const u32, const u32);
template void ARMv7DisAsm::ADD_SPR<A1>(const u32, const u32);
template void ARMv7DisAsm::ADR<T1>(const u32, const u32);
template void ARMv7DisAsm::ADR<T2>(const u32, const u32);
template void ARMv7DisAsm::ADR<T3>(const u32, const u32);
template void ARMv7DisAsm::ADR<A1>(const u32, const u32);
template void ARMv7DisAsm::ADR<A2>(const u32, const u32);
template void ARMv7DisAsm::AND_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::AND_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::AND_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::AND_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::AND_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::AND_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::ASR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ASR_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::ASR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::ASR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ASR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::ASR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::B<T1>(const u32, const u32);
template void ARMv7DisAsm::B<T2>(const u32, const u32);
template void ARMv7DisAsm::B<T3>(const u32, const u32);
template void ARMv7DisAsm::B<T4>(const u32, const u32);
template void ARMv7DisAsm::B<A1>(const u32, const u32);
template void ARMv7DisAsm::BFC<T1>(const u32, const u32);
template void ARMv7DisAsm::BFC<A1>(const u32, const u32);
template void ARMv7DisAsm::BFI<T1>(const u32, const u32);
template void ARMv7DisAsm::BFI<A1>(const u32, const u32);
template void ARMv7DisAsm::BIC_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::BIC_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::BIC_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::BIC_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::BIC_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::BIC_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::BKPT<T1>(const u32, const u32);
template void ARMv7DisAsm::BKPT<A1>(const u32, const u32);
template void ARMv7DisAsm::BL<T1>(const u32, const u32);
template void ARMv7DisAsm::BL<T2>(const u32, const u32);
template void ARMv7DisAsm::BL<A1>(const u32, const u32);
template void ARMv7DisAsm::BL<A2>(const u32, const u32);
template void ARMv7DisAsm::BLX<T1>(const u32, const u32);
template void ARMv7DisAsm::BLX<A1>(const u32, const u32);
template void ARMv7DisAsm::BX<T1>(const u32, const u32);
template void ARMv7DisAsm::BX<A1>(const u32, const u32);
template void ARMv7DisAsm::CB_Z<T1>(const u32, const u32);
template void ARMv7DisAsm::CLZ<T1>(const u32, const u32);
template void ARMv7DisAsm::CLZ<A1>(const u32, const u32);
template void ARMv7DisAsm::CMN_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::CMN_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::CMN_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::CMN_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::CMN_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::CMN_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::CMP_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::CMP_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::CMP_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::CMP_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::CMP_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::CMP_REG<T3>(const u32, const u32);
template void ARMv7DisAsm::CMP_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::CMP_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::DBG<T1>(const u32, const u32);
template void ARMv7DisAsm::DBG<A1>(const u32, const u32);
template void ARMv7DisAsm::DMB<T1>(const u32, const u32);
template void ARMv7DisAsm::DMB<A1>(const u32, const u32);
template void ARMv7DisAsm::DSB<T1>(const u32, const u32);
template void ARMv7DisAsm::DSB<A1>(const u32, const u32);
template void ARMv7DisAsm::EOR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::EOR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::EOR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::EOR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::EOR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::EOR_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::IT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDMDA<A1>(const u32, const u32);
template void ARMv7DisAsm::LDMDB<T1>(const u32, const u32);
template void ARMv7DisAsm::LDMDB<A1>(const u32, const u32);
template void ARMv7DisAsm::LDMIB<A1>(const u32, const u32);
template void ARMv7DisAsm::LDR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDR_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDR_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::LDR_IMM<T4>(const u32, const u32);
template void ARMv7DisAsm::LDR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDR_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDR_LIT<T2>(const u32, const u32);
template void ARMv7DisAsm::LDR_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LDR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LDR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRB_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::LDRB_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRB_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRB_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRD_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRD_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRD_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRD_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRD_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LDREX<T1>(const u32, const u32);
template void ARMv7DisAsm::LDREX<A1>(const u32, const u32);
template void ARMv7DisAsm::LDREXB<T1>(const u32, const u32);
template void ARMv7DisAsm::LDREXB<A1>(const u32, const u32);
template void ARMv7DisAsm::LDREXD<T1>(const u32, const u32);
template void ARMv7DisAsm::LDREXD<A1>(const u32, const u32);
template void ARMv7DisAsm::LDREXH<T1>(const u32, const u32);
template void ARMv7DisAsm::LDREXH<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRH_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::LDRH_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRH_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRH_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRSB_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_LIT<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_LIT<A1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LDRSH_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LSL_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LSL_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LSL_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LSL_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LSL_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LSL_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::LSR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::LSR_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::LSR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::LSR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::LSR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::LSR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::MLA<T1>(const u32, const u32);
template void ARMv7DisAsm::MLA<A1>(const u32, const u32);
template void ARMv7DisAsm::MLS<T1>(const u32, const u32);
template void ARMv7DisAsm::MLS<A1>(const u32, const u32);
template void ARMv7DisAsm::MOV_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::MOV_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::MOV_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::MOV_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::MOV_IMM<A2>(const u32, const u32);
template void ARMv7DisAsm::MOV_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::MOV_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::MOV_REG<T3>(const u32, const u32);
template void ARMv7DisAsm::MOV_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::MOVT<T1>(const u32, const u32);
template void ARMv7DisAsm::MOVT<A1>(const u32, const u32);
template void ARMv7DisAsm::MRC_<T1>(const u32, const u32);
template void ARMv7DisAsm::MRC_<A1>(const u32, const u32);
template void ARMv7DisAsm::MRC_<T2>(const u32, const u32);
template void ARMv7DisAsm::MRC_<A2>(const u32, const u32);
template void ARMv7DisAsm::MRS<T1>(const u32, const u32);
template void ARMv7DisAsm::MRS<A1>(const u32, const u32);
template void ARMv7DisAsm::MSR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::MSR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::MSR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::MUL<T1>(const u32, const u32);
template void ARMv7DisAsm::MUL<T2>(const u32, const u32);
template void ARMv7DisAsm::MUL<A1>(const u32, const u32);
template void ARMv7DisAsm::MVN_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::MVN_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::MVN_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::MVN_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::MVN_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::MVN_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::NOP<T1>(const u32, const u32);
template void ARMv7DisAsm::NOP<T2>(const u32, const u32);
template void ARMv7DisAsm::NOP<A1>(const u32, const u32);
template void ARMv7DisAsm::ORN_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ORN_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ORR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ORR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::ORR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ORR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::ORR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::ORR_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::PKH<T1>(const u32, const u32);
template void ARMv7DisAsm::PKH<A1>(const u32, const u32);
template void ARMv7DisAsm::POP<T1>(const u32, const u32);
template void ARMv7DisAsm::POP<T2>(const u32, const u32);
template void ARMv7DisAsm::POP<T3>(const u32, const u32);
template void ARMv7DisAsm::POP<A1>(const u32, const u32);
template void ARMv7DisAsm::POP<A2>(const u32, const u32);
template void ARMv7DisAsm::PUSH<T1>(const u32, const u32);
template void ARMv7DisAsm::PUSH<T2>(const u32, const u32);
template void ARMv7DisAsm::PUSH<T3>(const u32, const u32);
template void ARMv7DisAsm::PUSH<A1>(const u32, const u32);
template void ARMv7DisAsm::PUSH<A2>(const u32, const u32);
template void ARMv7DisAsm::QADD<T1>(const u32, const u32);
template void ARMv7DisAsm::QADD<A1>(const u32, const u32);
template void ARMv7DisAsm::QADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::QADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::QADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::QADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::QASX<T1>(const u32, const u32);
template void ARMv7DisAsm::QASX<A1>(const u32, const u32);
template void ARMv7DisAsm::QDADD<T1>(const u32, const u32);
template void ARMv7DisAsm::QDADD<A1>(const u32, const u32);
template void ARMv7DisAsm::QDSUB<T1>(const u32, const u32);
template void ARMv7DisAsm::QDSUB<A1>(const u32, const u32);
template void ARMv7DisAsm::QSAX<T1>(const u32, const u32);
template void ARMv7DisAsm::QSAX<A1>(const u32, const u32);
template void ARMv7DisAsm::QSUB<T1>(const u32, const u32);
template void ARMv7DisAsm::QSUB<A1>(const u32, const u32);
template void ARMv7DisAsm::QSUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::QSUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::QSUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::QSUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::RBIT<T1>(const u32, const u32);
template void ARMv7DisAsm::RBIT<A1>(const u32, const u32);
template void ARMv7DisAsm::REV<T1>(const u32, const u32);
template void ARMv7DisAsm::REV<T2>(const u32, const u32);
template void ARMv7DisAsm::REV<A1>(const u32, const u32);
template void ARMv7DisAsm::REV16<T1>(const u32, const u32);
template void ARMv7DisAsm::REV16<T2>(const u32, const u32);
template void ARMv7DisAsm::REV16<A1>(const u32, const u32);
template void ARMv7DisAsm::REVSH<T1>(const u32, const u32);
template void ARMv7DisAsm::REVSH<T2>(const u32, const u32);
template void ARMv7DisAsm::REVSH<A1>(const u32, const u32);
template void ARMv7DisAsm::ROR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::ROR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::ROR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::ROR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::ROR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::RRX<T1>(const u32, const u32);
template void ARMv7DisAsm::RRX<A1>(const u32, const u32);
template void ARMv7DisAsm::RSB_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::RSB_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::RSB_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::RSB_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::RSB_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::RSB_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::RSC_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::RSC_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::RSC_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::SADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::SADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::SADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::SADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::SASX<T1>(const u32, const u32);
template void ARMv7DisAsm::SASX<A1>(const u32, const u32);
template void ARMv7DisAsm::SBC_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::SBC_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::SBC_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::SBC_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::SBC_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::SBC_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::SBFX<T1>(const u32, const u32);
template void ARMv7DisAsm::SBFX<A1>(const u32, const u32);
template void ARMv7DisAsm::SDIV<T1>(const u32, const u32);
template void ARMv7DisAsm::SEL<T1>(const u32, const u32);
template void ARMv7DisAsm::SEL<A1>(const u32, const u32);
template void ARMv7DisAsm::SHADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::SHADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::SHADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::SHADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::SHASX<T1>(const u32, const u32);
template void ARMv7DisAsm::SHASX<A1>(const u32, const u32);
template void ARMv7DisAsm::SHSAX<T1>(const u32, const u32);
template void ARMv7DisAsm::SHSAX<A1>(const u32, const u32);
template void ARMv7DisAsm::SHSUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::SHSUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::SHSUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::SHSUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLA__<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLA__<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLAD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLAD<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLAL<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLAL<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLAL__<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLAL__<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLALD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLALD<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLAW_<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLAW_<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLSD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLSD<A1>(const u32, const u32);
template void ARMv7DisAsm::SMLSLD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMLSLD<A1>(const u32, const u32);
template void ARMv7DisAsm::SMMLA<T1>(const u32, const u32);
template void ARMv7DisAsm::SMMLA<A1>(const u32, const u32);
template void ARMv7DisAsm::SMMLS<T1>(const u32, const u32);
template void ARMv7DisAsm::SMMLS<A1>(const u32, const u32);
template void ARMv7DisAsm::SMMUL<T1>(const u32, const u32);
template void ARMv7DisAsm::SMMUL<A1>(const u32, const u32);
template void ARMv7DisAsm::SMUAD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMUAD<A1>(const u32, const u32);
template void ARMv7DisAsm::SMUL__<T1>(const u32, const u32);
template void ARMv7DisAsm::SMUL__<A1>(const u32, const u32);
template void ARMv7DisAsm::SMULL<T1>(const u32, const u32);
template void ARMv7DisAsm::SMULL<A1>(const u32, const u32);
template void ARMv7DisAsm::SMULW_<T1>(const u32, const u32);
template void ARMv7DisAsm::SMULW_<A1>(const u32, const u32);
template void ARMv7DisAsm::SMUSD<T1>(const u32, const u32);
template void ARMv7DisAsm::SMUSD<A1>(const u32, const u32);
template void ARMv7DisAsm::SSAT<T1>(const u32, const u32);
template void ARMv7DisAsm::SSAT<A1>(const u32, const u32);
template void ARMv7DisAsm::SSAT16<T1>(const u32, const u32);
template void ARMv7DisAsm::SSAT16<A1>(const u32, const u32);
template void ARMv7DisAsm::SSAX<T1>(const u32, const u32);
template void ARMv7DisAsm::SSAX<A1>(const u32, const u32);
template void ARMv7DisAsm::SSUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::SSUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::SSUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::SSUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::STM<T1>(const u32, const u32);
template void ARMv7DisAsm::STM<T2>(const u32, const u32);
template void ARMv7DisAsm::STM<A1>(const u32, const u32);
template void ARMv7DisAsm::STMDA<A1>(const u32, const u32);
template void ARMv7DisAsm::STMDB<T1>(const u32, const u32);
template void ARMv7DisAsm::STMDB<A1>(const u32, const u32);
template void ARMv7DisAsm::STMIB<A1>(const u32, const u32);
template void ARMv7DisAsm::STR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::STR_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::STR_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::STR_IMM<T4>(const u32, const u32);
template void ARMv7DisAsm::STR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::STR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::STR_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::STR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::STRB_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::STRB_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::STRB_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::STRB_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::STRB_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::STRB_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::STRB_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::STRD_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::STRD_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::STRD_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::STREX<T1>(const u32, const u32);
template void ARMv7DisAsm::STREX<A1>(const u32, const u32);
template void ARMv7DisAsm::STREXB<T1>(const u32, const u32);
template void ARMv7DisAsm::STREXB<A1>(const u32, const u32);
template void ARMv7DisAsm::STREXD<T1>(const u32, const u32);
template void ARMv7DisAsm::STREXD<A1>(const u32, const u32);
template void ARMv7DisAsm::STREXH<T1>(const u32, const u32);
template void ARMv7DisAsm::STREXH<A1>(const u32, const u32);
template void ARMv7DisAsm::STRH_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::STRH_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::STRH_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::STRH_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::STRH_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::STRH_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::STRH_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::SUB_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::SUB_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::SUB_IMM<T3>(const u32, const u32);
template void ARMv7DisAsm::SUB_IMM<T4>(const u32, const u32);
template void ARMv7DisAsm::SUB_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::SUB_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::SUB_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::SUB_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::SUB_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPI<T1>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPI<T2>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPI<T3>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPI<A1>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPR<T1>(const u32, const u32);
template void ARMv7DisAsm::SUB_SPR<A1>(const u32, const u32);
template void ARMv7DisAsm::SVC<T1>(const u32, const u32);
template void ARMv7DisAsm::SVC<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTAB<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTAB<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTAB16<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTAB16<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTAH<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTAH<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTB<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTB<T2>(const u32, const u32);
template void ARMv7DisAsm::SXTB<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTB16<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTB16<A1>(const u32, const u32);
template void ARMv7DisAsm::SXTH<T1>(const u32, const u32);
template void ARMv7DisAsm::SXTH<T2>(const u32, const u32);
template void ARMv7DisAsm::SXTH<A1>(const u32, const u32);
template void ARMv7DisAsm::TB_<T1>(const u32, const u32);
template void ARMv7DisAsm::TEQ_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::TEQ_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::TEQ_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::TEQ_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::TEQ_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::TST_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::TST_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::TST_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::TST_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::TST_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::TST_RSR<A1>(const u32, const u32);
template void ARMv7DisAsm::UADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::UADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::UADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::UADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::UASX<T1>(const u32, const u32);
template void ARMv7DisAsm::UASX<A1>(const u32, const u32);
template void ARMv7DisAsm::UBFX<T1>(const u32, const u32);
template void ARMv7DisAsm::UBFX<A1>(const u32, const u32);
template void ARMv7DisAsm::UDIV<T1>(const u32, const u32);
template void ARMv7DisAsm::UHADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::UHADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::UHADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::UHADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::UHASX<T1>(const u32, const u32);
template void ARMv7DisAsm::UHASX<A1>(const u32, const u32);
template void ARMv7DisAsm::UHSAX<T1>(const u32, const u32);
template void ARMv7DisAsm::UHSAX<A1>(const u32, const u32);
template void ARMv7DisAsm::UHSUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::UHSUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::UHSUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::UHSUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::UMAAL<T1>(const u32, const u32);
template void ARMv7DisAsm::UMAAL<A1>(const u32, const u32);
template void ARMv7DisAsm::UMLAL<T1>(const u32, const u32);
template void ARMv7DisAsm::UMLAL<A1>(const u32, const u32);
template void ARMv7DisAsm::UMULL<T1>(const u32, const u32);
template void ARMv7DisAsm::UMULL<A1>(const u32, const u32);
template void ARMv7DisAsm::UQADD16<T1>(const u32, const u32);
template void ARMv7DisAsm::UQADD16<A1>(const u32, const u32);
template void ARMv7DisAsm::UQADD8<T1>(const u32, const u32);
template void ARMv7DisAsm::UQADD8<A1>(const u32, const u32);
template void ARMv7DisAsm::UQASX<T1>(const u32, const u32);
template void ARMv7DisAsm::UQASX<A1>(const u32, const u32);
template void ARMv7DisAsm::UQSAX<T1>(const u32, const u32);
template void ARMv7DisAsm::UQSAX<A1>(const u32, const u32);
template void ARMv7DisAsm::UQSUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::UQSUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::UQSUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::UQSUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::USAD8<T1>(const u32, const u32);
template void ARMv7DisAsm::USAD8<A1>(const u32, const u32);
template void ARMv7DisAsm::USADA8<T1>(const u32, const u32);
template void ARMv7DisAsm::USADA8<A1>(const u32, const u32);
template void ARMv7DisAsm::USAT<T1>(const u32, const u32);
template void ARMv7DisAsm::USAT<A1>(const u32, const u32);
template void ARMv7DisAsm::USAT16<T1>(const u32, const u32);
template void ARMv7DisAsm::USAT16<A1>(const u32, const u32);
template void ARMv7DisAsm::USAX<T1>(const u32, const u32);
template void ARMv7DisAsm::USAX<A1>(const u32, const u32);
template void ARMv7DisAsm::USUB16<T1>(const u32, const u32);
template void ARMv7DisAsm::USUB16<A1>(const u32, const u32);
template void ARMv7DisAsm::USUB8<T1>(const u32, const u32);
template void ARMv7DisAsm::USUB8<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTAB<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTAB<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTAB16<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTAB16<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTAH<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTAH<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTB<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTB<T2>(const u32, const u32);
template void ARMv7DisAsm::UXTB<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTB16<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTB16<A1>(const u32, const u32);
template void ARMv7DisAsm::UXTH<T1>(const u32, const u32);
template void ARMv7DisAsm::UXTH<T2>(const u32, const u32);
template void ARMv7DisAsm::UXTH<A1>(const u32, const u32);
template void ARMv7DisAsm::VABA_<T1>(const u32, const u32);
template void ARMv7DisAsm::VABA_<A1>(const u32, const u32);
template void ARMv7DisAsm::VABA_<T2>(const u32, const u32);
template void ARMv7DisAsm::VABA_<A2>(const u32, const u32);
template void ARMv7DisAsm::VABD_<T1>(const u32, const u32);
template void ARMv7DisAsm::VABD_<A1>(const u32, const u32);
template void ARMv7DisAsm::VABD_<T2>(const u32, const u32);
template void ARMv7DisAsm::VABD_<A2>(const u32, const u32);
template void ARMv7DisAsm::VABD_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VABD_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VABS<T1>(const u32, const u32);
template void ARMv7DisAsm::VABS<A1>(const u32, const u32);
template void ARMv7DisAsm::VABS<T2>(const u32, const u32);
template void ARMv7DisAsm::VABS<A2>(const u32, const u32);
template void ARMv7DisAsm::VAC__<T1>(const u32, const u32);
template void ARMv7DisAsm::VAC__<A1>(const u32, const u32);
template void ARMv7DisAsm::VADD<T1>(const u32, const u32);
template void ARMv7DisAsm::VADD<A1>(const u32, const u32);
template void ARMv7DisAsm::VADD_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VADD_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VADD_FP<T2>(const u32, const u32);
template void ARMv7DisAsm::VADD_FP<A2>(const u32, const u32);
template void ARMv7DisAsm::VADDHN<T1>(const u32, const u32);
template void ARMv7DisAsm::VADDHN<A1>(const u32, const u32);
template void ARMv7DisAsm::VADD_<T1>(const u32, const u32);
template void ARMv7DisAsm::VADD_<A1>(const u32, const u32);
template void ARMv7DisAsm::VAND<T1>(const u32, const u32);
template void ARMv7DisAsm::VAND<A1>(const u32, const u32);
template void ARMv7DisAsm::VBIC_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VBIC_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VBIC_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VBIC_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VB__<T1>(const u32, const u32);
template void ARMv7DisAsm::VB__<A1>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_REG<A2>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_ZERO<T1>(const u32, const u32);
template void ARMv7DisAsm::VCEQ_ZERO<A1>(const u32, const u32);
template void ARMv7DisAsm::VCGE_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VCGE_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VCGE_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::VCGE_REG<A2>(const u32, const u32);
template void ARMv7DisAsm::VCGE_ZERO<T1>(const u32, const u32);
template void ARMv7DisAsm::VCGE_ZERO<A1>(const u32, const u32);
template void ARMv7DisAsm::VCGT_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VCGT_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VCGT_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::VCGT_REG<A2>(const u32, const u32);
template void ARMv7DisAsm::VCGT_ZERO<T1>(const u32, const u32);
template void ARMv7DisAsm::VCGT_ZERO<A1>(const u32, const u32);
template void ARMv7DisAsm::VCLE_ZERO<T1>(const u32, const u32);
template void ARMv7DisAsm::VCLE_ZERO<A1>(const u32, const u32);
template void ARMv7DisAsm::VCLS<T1>(const u32, const u32);
template void ARMv7DisAsm::VCLS<A1>(const u32, const u32);
template void ARMv7DisAsm::VCLT_ZERO<T1>(const u32, const u32);
template void ARMv7DisAsm::VCLT_ZERO<A1>(const u32, const u32);
template void ARMv7DisAsm::VCLZ<T1>(const u32, const u32);
template void ARMv7DisAsm::VCLZ<A1>(const u32, const u32);
template void ARMv7DisAsm::VCMP_<T1>(const u32, const u32);
template void ARMv7DisAsm::VCMP_<A1>(const u32, const u32);
template void ARMv7DisAsm::VCMP_<T2>(const u32, const u32);
template void ARMv7DisAsm::VCMP_<A2>(const u32, const u32);
template void ARMv7DisAsm::VCNT<T1>(const u32, const u32);
template void ARMv7DisAsm::VCNT<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FIA<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FIA<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FIF<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FIF<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FFA<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FFA<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FFF<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_FFF<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_DF<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_DF<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_HFA<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_HFA<A1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_HFF<T1>(const u32, const u32);
template void ARMv7DisAsm::VCVT_HFF<A1>(const u32, const u32);
template void ARMv7DisAsm::VDIV<T1>(const u32, const u32);
template void ARMv7DisAsm::VDIV<A1>(const u32, const u32);
template void ARMv7DisAsm::VDUP_S<T1>(const u32, const u32);
template void ARMv7DisAsm::VDUP_S<A1>(const u32, const u32);
template void ARMv7DisAsm::VDUP_R<T1>(const u32, const u32);
template void ARMv7DisAsm::VDUP_R<A1>(const u32, const u32);
template void ARMv7DisAsm::VEOR<T1>(const u32, const u32);
template void ARMv7DisAsm::VEOR<A1>(const u32, const u32);
template void ARMv7DisAsm::VEXT<T1>(const u32, const u32);
template void ARMv7DisAsm::VEXT<A1>(const u32, const u32);
template void ARMv7DisAsm::VHADDSUB<T1>(const u32, const u32);
template void ARMv7DisAsm::VHADDSUB<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD__MS<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD__MS<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD1_SAL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD1_SAL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD1_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD1_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD2_SAL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD2_SAL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD2_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD2_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD3_SAL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD3_SAL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD3_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD3_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD4_SAL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD4_SAL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLD4_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VLD4_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VLDM<T1>(const u32, const u32);
template void ARMv7DisAsm::VLDM<A1>(const u32, const u32);
template void ARMv7DisAsm::VLDM<T2>(const u32, const u32);
template void ARMv7DisAsm::VLDM<A2>(const u32, const u32);
template void ARMv7DisAsm::VLDR<T1>(const u32, const u32);
template void ARMv7DisAsm::VLDR<A1>(const u32, const u32);
template void ARMv7DisAsm::VLDR<T2>(const u32, const u32);
template void ARMv7DisAsm::VLDR<A2>(const u32, const u32);
template void ARMv7DisAsm::VMAXMIN<T1>(const u32, const u32);
template void ARMv7DisAsm::VMAXMIN<A1>(const u32, const u32);
template void ARMv7DisAsm::VMAXMIN_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VMAXMIN_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VML__<T1>(const u32, const u32);
template void ARMv7DisAsm::VML__<A1>(const u32, const u32);
template void ARMv7DisAsm::VML__<T2>(const u32, const u32);
template void ARMv7DisAsm::VML__<A2>(const u32, const u32);
template void ARMv7DisAsm::VML__FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VML__FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VML__FP<T2>(const u32, const u32);
template void ARMv7DisAsm::VML__FP<A2>(const u32, const u32);
template void ARMv7DisAsm::VML__S<T1>(const u32, const u32);
template void ARMv7DisAsm::VML__S<A1>(const u32, const u32);
template void ARMv7DisAsm::VML__S<T2>(const u32, const u32);
template void ARMv7DisAsm::VML__S<A2>(const u32, const u32);
template void ARMv7DisAsm::VMOV_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_IMM<T2>(const u32, const u32);
template void ARMv7DisAsm::VMOV_IMM<A2>(const u32, const u32);
template void ARMv7DisAsm::VMOV_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_REG<T2>(const u32, const u32);
template void ARMv7DisAsm::VMOV_REG<A2>(const u32, const u32);
template void ARMv7DisAsm::VMOV_RS<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_RS<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_SR<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_SR<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_RF<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_RF<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_2RF<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_2RF<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_2RD<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOV_2RD<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOVL<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOVL<A1>(const u32, const u32);
template void ARMv7DisAsm::VMOVN<T1>(const u32, const u32);
template void ARMv7DisAsm::VMOVN<A1>(const u32, const u32);
template void ARMv7DisAsm::VMRS<T1>(const u32, const u32);
template void ARMv7DisAsm::VMRS<A1>(const u32, const u32);
template void ARMv7DisAsm::VMSR<T1>(const u32, const u32);
template void ARMv7DisAsm::VMSR<A1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_<T1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_<A1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_<T2>(const u32, const u32);
template void ARMv7DisAsm::VMUL_<A2>(const u32, const u32);
template void ARMv7DisAsm::VMUL_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_FP<T2>(const u32, const u32);
template void ARMv7DisAsm::VMUL_FP<A2>(const u32, const u32);
template void ARMv7DisAsm::VMUL_S<T1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_S<A1>(const u32, const u32);
template void ARMv7DisAsm::VMUL_S<T2>(const u32, const u32);
template void ARMv7DisAsm::VMUL_S<A2>(const u32, const u32);
template void ARMv7DisAsm::VMVN_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VMVN_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VMVN_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VMVN_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VNEG<T1>(const u32, const u32);
template void ARMv7DisAsm::VNEG<A1>(const u32, const u32);
template void ARMv7DisAsm::VNEG<T2>(const u32, const u32);
template void ARMv7DisAsm::VNEG<A2>(const u32, const u32);
template void ARMv7DisAsm::VNM__<T1>(const u32, const u32);
template void ARMv7DisAsm::VNM__<A1>(const u32, const u32);
template void ARMv7DisAsm::VNM__<T2>(const u32, const u32);
template void ARMv7DisAsm::VNM__<A2>(const u32, const u32);
template void ARMv7DisAsm::VORN_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VORN_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VORR_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VORR_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VORR_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VORR_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VPADAL<T1>(const u32, const u32);
template void ARMv7DisAsm::VPADAL<A1>(const u32, const u32);
template void ARMv7DisAsm::VPADD<T1>(const u32, const u32);
template void ARMv7DisAsm::VPADD<A1>(const u32, const u32);
template void ARMv7DisAsm::VPADD_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VPADD_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VPADDL<T1>(const u32, const u32);
template void ARMv7DisAsm::VPADDL<A1>(const u32, const u32);
template void ARMv7DisAsm::VPMAXMIN<T1>(const u32, const u32);
template void ARMv7DisAsm::VPMAXMIN<A1>(const u32, const u32);
template void ARMv7DisAsm::VPMAXMIN_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VPMAXMIN_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VPOP<T1>(const u32, const u32);
template void ARMv7DisAsm::VPOP<A1>(const u32, const u32);
template void ARMv7DisAsm::VPOP<T2>(const u32, const u32);
template void ARMv7DisAsm::VPOP<A2>(const u32, const u32);
template void ARMv7DisAsm::VPUSH<T1>(const u32, const u32);
template void ARMv7DisAsm::VPUSH<A1>(const u32, const u32);
template void ARMv7DisAsm::VPUSH<T2>(const u32, const u32);
template void ARMv7DisAsm::VPUSH<A2>(const u32, const u32);
template void ARMv7DisAsm::VQABS<T1>(const u32, const u32);
template void ARMv7DisAsm::VQABS<A1>(const u32, const u32);
template void ARMv7DisAsm::VQADD<T1>(const u32, const u32);
template void ARMv7DisAsm::VQADD<A1>(const u32, const u32);
template void ARMv7DisAsm::VQDML_L<T1>(const u32, const u32);
template void ARMv7DisAsm::VQDML_L<A1>(const u32, const u32);
template void ARMv7DisAsm::VQDML_L<T2>(const u32, const u32);
template void ARMv7DisAsm::VQDML_L<A2>(const u32, const u32);
template void ARMv7DisAsm::VQDMULH<T1>(const u32, const u32);
template void ARMv7DisAsm::VQDMULH<A1>(const u32, const u32);
template void ARMv7DisAsm::VQDMULH<T2>(const u32, const u32);
template void ARMv7DisAsm::VQDMULH<A2>(const u32, const u32);
template void ARMv7DisAsm::VQDMULL<T1>(const u32, const u32);
template void ARMv7DisAsm::VQDMULL<A1>(const u32, const u32);
template void ARMv7DisAsm::VQDMULL<T2>(const u32, const u32);
template void ARMv7DisAsm::VQDMULL<A2>(const u32, const u32);
template void ARMv7DisAsm::VQMOV_N<T1>(const u32, const u32);
template void ARMv7DisAsm::VQMOV_N<A1>(const u32, const u32);
template void ARMv7DisAsm::VQNEG<T1>(const u32, const u32);
template void ARMv7DisAsm::VQNEG<A1>(const u32, const u32);
template void ARMv7DisAsm::VQRDMULH<T1>(const u32, const u32);
template void ARMv7DisAsm::VQRDMULH<A1>(const u32, const u32);
template void ARMv7DisAsm::VQRDMULH<T2>(const u32, const u32);
template void ARMv7DisAsm::VQRDMULH<A2>(const u32, const u32);
template void ARMv7DisAsm::VQRSHL<T1>(const u32, const u32);
template void ARMv7DisAsm::VQRSHL<A1>(const u32, const u32);
template void ARMv7DisAsm::VQRSHR_N<T1>(const u32, const u32);
template void ARMv7DisAsm::VQRSHR_N<A1>(const u32, const u32);
template void ARMv7DisAsm::VQSHL_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VQSHL_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VQSHL_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VQSHL_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VQSHR_N<T1>(const u32, const u32);
template void ARMv7DisAsm::VQSHR_N<A1>(const u32, const u32);
template void ARMv7DisAsm::VQSUB<T1>(const u32, const u32);
template void ARMv7DisAsm::VQSUB<A1>(const u32, const u32);
template void ARMv7DisAsm::VRADDHN<T1>(const u32, const u32);
template void ARMv7DisAsm::VRADDHN<A1>(const u32, const u32);
template void ARMv7DisAsm::VRECPE<T1>(const u32, const u32);
template void ARMv7DisAsm::VRECPE<A1>(const u32, const u32);
template void ARMv7DisAsm::VRECPS<T1>(const u32, const u32);
template void ARMv7DisAsm::VRECPS<A1>(const u32, const u32);
template void ARMv7DisAsm::VREV__<T1>(const u32, const u32);
template void ARMv7DisAsm::VREV__<A1>(const u32, const u32);
template void ARMv7DisAsm::VRHADD<T1>(const u32, const u32);
template void ARMv7DisAsm::VRHADD<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSHL<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSHL<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSHR<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSHR<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSHRN<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSHRN<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSQRTE<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSQRTE<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSQRTS<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSQRTS<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSRA<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSRA<A1>(const u32, const u32);
template void ARMv7DisAsm::VRSUBHN<T1>(const u32, const u32);
template void ARMv7DisAsm::VRSUBHN<A1>(const u32, const u32);
template void ARMv7DisAsm::VSHL_IMM<T1>(const u32, const u32);
template void ARMv7DisAsm::VSHL_IMM<A1>(const u32, const u32);
template void ARMv7DisAsm::VSHL_REG<T1>(const u32, const u32);
template void ARMv7DisAsm::VSHL_REG<A1>(const u32, const u32);
template void ARMv7DisAsm::VSHLL<T1>(const u32, const u32);
template void ARMv7DisAsm::VSHLL<A1>(const u32, const u32);
template void ARMv7DisAsm::VSHLL<T2>(const u32, const u32);
template void ARMv7DisAsm::VSHLL<A2>(const u32, const u32);
template void ARMv7DisAsm::VSHR<T1>(const u32, const u32);
template void ARMv7DisAsm::VSHR<A1>(const u32, const u32);
template void ARMv7DisAsm::VSHRN<T1>(const u32, const u32);
template void ARMv7DisAsm::VSHRN<A1>(const u32, const u32);
template void ARMv7DisAsm::VSLI<T1>(const u32, const u32);
template void ARMv7DisAsm::VSLI<A1>(const u32, const u32);
template void ARMv7DisAsm::VSQRT<T1>(const u32, const u32);
template void ARMv7DisAsm::VSQRT<A1>(const u32, const u32);
template void ARMv7DisAsm::VSRA<T1>(const u32, const u32);
template void ARMv7DisAsm::VSRA<A1>(const u32, const u32);
template void ARMv7DisAsm::VSRI<T1>(const u32, const u32);
template void ARMv7DisAsm::VSRI<A1>(const u32, const u32);
template void ARMv7DisAsm::VST__MS<T1>(const u32, const u32);
template void ARMv7DisAsm::VST__MS<A1>(const u32, const u32);
template void ARMv7DisAsm::VST1_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VST1_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VST2_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VST2_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VST3_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VST3_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VST4_SL<T1>(const u32, const u32);
template void ARMv7DisAsm::VST4_SL<A1>(const u32, const u32);
template void ARMv7DisAsm::VSTM<T1>(const u32, const u32);
template void ARMv7DisAsm::VSTM<A1>(const u32, const u32);
template void ARMv7DisAsm::VSTM<T2>(const u32, const u32);
template void ARMv7DisAsm::VSTM<A2>(const u32, const u32);
template void ARMv7DisAsm::VSTR<T1>(const u32, const u32);
template void ARMv7DisAsm::VSTR<A1>(const u32, const u32);
template void ARMv7DisAsm::VSTR<T2>(const u32, const u32);
template void ARMv7DisAsm::VSTR<A2>(const u32, const u32);
template void ARMv7DisAsm::VSUB<T1>(const u32, const u32);
template void ARMv7DisAsm::VSUB<A1>(const u32, const u32);
template void ARMv7DisAsm::VSUB_FP<T1>(const u32, const u32);
template void ARMv7DisAsm::VSUB_FP<A1>(const u32, const u32);
template void ARMv7DisAsm::VSUB_FP<T2>(const u32, const u32);
template void ARMv7DisAsm::VSUB_FP<A2>(const u32, const u32);
template void ARMv7DisAsm::VSUBHN<T1>(const u32, const u32);
template void ARMv7DisAsm::VSUBHN<A1>(const u32, const u32);
template void ARMv7DisAsm::VSUB_<T1>(const u32, const u32);
template void ARMv7DisAsm::VSUB_<A1>(const u32, const u32);
template void ARMv7DisAsm::VSWP<T1>(const u32, const u32);
template void ARMv7DisAsm::VSWP<A1>(const u32, const u32);
template void ARMv7DisAsm::VTB_<T1>(const u32, const u32);
template void ARMv7DisAsm::VTB_<A1>(const u32, const u32);
template void ARMv7DisAsm::VTRN<T1>(const u32, const u32);
template void ARMv7DisAsm::VTRN<A1>(const u32, const u32);
template void ARMv7DisAsm::VTST<T1>(const u32, const u32);
template void ARMv7DisAsm::VTST<A1>(const u32, const u32);
template void ARMv7DisAsm::VUZP<T1>(const u32, const u32);
template void ARMv7DisAsm::VUZP<A1>(const u32, const u32);
template void ARMv7DisAsm::VZIP<T1>(const u32, const u32);
template void ARMv7DisAsm::VZIP<A1>(const u32, const u32);
template void ARMv7DisAsm::WFE<T1>(const u32, const u32);
template void ARMv7DisAsm::WFE<T2>(const u32, const u32);
template void ARMv7DisAsm::WFE<A1>(const u32, const u32);
template void ARMv7DisAsm::WFI<T1>(const u32, const u32);
template void ARMv7DisAsm::WFI<T2>(const u32, const u32);
template void ARMv7DisAsm::WFI<A1>(const u32, const u32);
template void ARMv7DisAsm::YIELD<T1>(const u32, const u32);
template void ARMv7DisAsm::YIELD<T2>(const u32, const u32);
template void ARMv7DisAsm::YIELD<A1>(const u32, const u32);
