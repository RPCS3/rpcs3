#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/CPU/CPUDecoder.h"
#include "ARMv7Thread.h"
#include "ARMv7Interpreter.h"

void ARMv7Interpreter::UNK(const u32 data)
{
	LOG_ERROR(HLE, "Unknown/illegal opcode! (0x%04x : 0x%04x)", data >> 16, data & 0xffff);
	Emu.Pause();
}

void ARMv7Interpreter::NULL_OP(const u32 data, const ARMv7_encoding type)
{
	LOG_ERROR(HLE, "Null opcode found");
	Emu.Pause();
}

void ARMv7Interpreter::ADC_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADC_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADC_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ADD_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADD_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADD_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADD_SPI(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ADD_SPR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ADR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::AND_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::AND_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::AND_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ASR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ASR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::B(const u32 data, const ARMv7_encoding type)
{
	u32 cond = 0xf;
	u32 jump = 0; // jump = instr_size + imm32

	switch (type)
	{
	case T1:
	{
		jump = 2 + sign<9, u32>((data & 0xff) << 1);
		cond = (data >> 8) & 0xf; if (cond == 0xf) throw "SVC";
		break;
	}
	case T2:
	{
		jump = 2 + sign<12, u32>((data & 0x7ff) << 1);
		break;
	}
	case T3:
	{
		u32 s = (data >> 26) & 0x1;
		u32 j1 = (data >> 13) & 0x1;
		u32 j2 = (data >> 11) & 0x1;
		jump = 4 + sign<21, u32>(s << 20 | j2 << 19 | j1 << 18 | (data & 0x3f0000) >> 4 | (data & 0x7ff) << 1);
		cond = (data >> 6) & 0xf; if (cond >= 0xe) throw "Related encodings";
		break;
	}
	case T4:
	{
		u32 s = (data >> 26) & 0x1;
		u32 i1 = (data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (data >> 11) & 0x1 ^ s ^ 1;
		jump = 4 + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (data & 0x3ff0000) >> 4 | (data & 0x7ff) << 1);
		break;
	}
	case A1:
	{
		jump = 1 + 4 + sign<26, u32>((data & 0xffffff) << 2);
		cond = (data >> 28) & 0xf;
		break;
	}
	}

	if (ConditionPassed(cond))
	{
		CPU.SetBranch(CPU.PC + jump);
	}
}


void ARMv7Interpreter::BFC(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::BFI(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::BIC_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::BIC_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::BIC_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::BKPT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::BL(const u32 data, const ARMv7_encoding type)
{
	u32 jump = 0;

	switch (type)
	{
	case T1:
	{
		u32 s = (data >> 26) & 0x1;
		u32 i1 = (data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (data >> 11) & 0x1 ^ s ^ 1;
		jump = 4 + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (data & 0x3ff0000) >> 4 | (data & 0x7ff) << 1);
		break;
	}
	case A1:
	{
		jump = 4 + sign<26, u32>((data & 0xffffff) << 2);
		break;
	}
	default: throw __FUNCTION__;
	}

	CPU.LR = (CPU.PC & 1) ? CPU.PC - 4 : CPU.PC;
	CPU.SetBranch(CPU.PC + jump);
}

void ARMv7Interpreter::BLX(const u32 data, const ARMv7_encoding type)
{
	u32 target;

	switch (type)
	{
	case T1:
	{
		target = CPU.GPR[(data >> 3) & 0xf];
		break;
	}
	case T2:
	{
		u32 s = (data >> 26) & 0x1;
		u32 i1 = (data >> 13) & 0x1 ^ s ^ 1;
		u32 i2 = (data >> 11) & 0x1 ^ s ^ 1;
		target = CPU.PC + 4 + sign<25, u32>(s << 24 | i2 << 23 | i1 << 22 | (data & 0x3ff0000) >> 4 | (data & 0x7ff) << 1) & ~1;
		break;
	}
	case A1:
	{
		target = CPU.GPR[data & 0xf];
		if (!ConditionPassed(data >> 28)) return;
		break;
	}
	case A2:
	{
		target = CPU.PC + 5 + sign<25, u32>((data & 0xffffff) << 2 | (data & 0x1000000) >> 23);
		break;
	}
	default: throw __FUNCTION__;
	}

	CPU.LR = (CPU.PC & 1) ? CPU.PC - (type == T1 ? 2 : 4) : CPU.PC - 4; // ???
	CPU.SetBranch(target);
}

void ARMv7Interpreter::BX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::CB_Z(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: break;
	default: throw __FUNCTION__;
	}

	if ((CPU.GPR[data & 0x7] == 0) ^ (data & 0x800))
	{
		CPU.SetBranch(CPU.PC + 2 + ((data & 0xf8) >> 2) + ((data & 0x200) >> 3));
	}
}


void ARMv7Interpreter::CLZ(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::CMN_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::CMN_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::CMN_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::CMP_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::CMP_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::CMP_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::EOR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::EOR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::EOR_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::IT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDMDA(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDMDB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDMIB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDR_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDRB_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRB_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRB_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDRD_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRD_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRD_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDRH_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRH_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRH_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDRSB_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRSB_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRSB_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LDRSH_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRSH_LIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LDRSH_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LSL_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LSL_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::LSR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::LSR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::MLA(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MLS(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::MOV_IMM(const u32 data, const ARMv7_encoding type)
{
	u32 d;
	u32 imm;

	switch (type)
	{
	case T1: d = (data >> 8) & 0x7; imm = sign<8, u32>(data & 0xff); break;
	default: throw __FUNCTION__;
	}

	CPU.write_gpr(d, imm);
}

void ARMv7Interpreter::MOV_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MOVT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::MRS(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MSR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MSR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::MUL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::MVN_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MVN_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::MVN_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::NOP(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: break;
	case T2: break;
	case A1: break;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ORN_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ORN_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ORR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ORR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ORR_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::PKH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::POP(const u32 data, const ARMv7_encoding type)
{
	u16 reg_list = 0;

	switch (type)
	{
	case T1:
	{
		reg_list = ((data & 0x100) << 6) | (data & 0xff);
		break;
	}
	case T2:
	{
		reg_list = data & 0xdfff;
		break;
	}
	case T3:
	{
		reg_list = 1 << (data >> 12);
		break;
	}
	case A1:
	{
		reg_list = data & 0xffff; if (BitCount(reg_list) < 2) throw "LDM / LDMIA / LDMFD";
		if (!ConditionPassed(data >> 28)) return;
		break;
	}
	case A2:
	{
		reg_list = 1 << ((data >> 12) & 0xf);
		if (!ConditionPassed(data >> 28)) return;
		break;
	}
	default: throw __FUNCTION__;
	}

	for (u16 mask = 1, i = 0; mask; mask <<= 1, i++)
	{
		if (reg_list & mask)
		{
			CPU.write_gpr(i, vm::psv::read32(CPU.SP));
			CPU.SP += 4;
		}
	}
}

void ARMv7Interpreter::PUSH(const u32 data, const ARMv7_encoding type)
{
	u16 reg_list = 0;

	switch (type)
	{
	case T1:
	{
		reg_list = ((data & 0x100) << 6) | (data & 0xff);
		break;
	}
	case T2:
	{
		reg_list = data & 0x5fff;
		break;
	}
	case T3:
	{
		reg_list = 1 << (data >> 12);
		break;
	}
	case A1:
	{
		reg_list = data & 0xffff; if (BitCount(reg_list) < 2) throw "STMDB / STMFD";
		if (!ConditionPassed(data >> 28)) return;
		break;
	}
	case A2:
	{
		reg_list = 1 << ((data >> 12) & 0xf);
		if (!ConditionPassed(data >> 28)) return;
		break;
	}
	default: throw __FUNCTION__;
	}

	for (u16 mask = 1 << 15, i = 15; mask; mask >>= 1, i--)
	{
		if (reg_list & mask)
		{
			CPU.SP -= 4;
			vm::psv::write32(CPU.SP, CPU.read_gpr(i));
		}
	}
}


void ARMv7Interpreter::QADD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QDADD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QDSUB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QSAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QSUB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QSUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::QSUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::RBIT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::REV(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::REV16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::REVSH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::ROR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::ROR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::RRX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::RSB_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::RSB_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::RSB_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::RSC_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::RSC_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::RSC_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SBC_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SBC_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SBC_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SBFX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SDIV(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SEL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SHADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SHADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SHASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SHSAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SHSUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SHSUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SMLA__(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLAD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLAL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLAL__(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLALD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLAW_(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLSD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMLSLD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMMLA(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMMLS(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMMUL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMUAD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMUL__(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMULL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMULW_(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SMUSD(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SSAT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SSAT16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SSAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SSUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SSUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::STM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STMDA(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STMDB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STMIB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::STR_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STR_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::STRB_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STRB_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::STRD_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STRD_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::STRH_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::STRH_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SUB_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SUB_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SUB_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SUB_SPI(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: CPU.SP -= (data & 0x7f) << 2; return;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SUB_SPR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SVC(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SWP_(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::SXTAB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SXTAB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SXTAH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SXTB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SXTB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::SXTH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::TB_(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::TEQ_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::TEQ_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::TEQ_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::TST_IMM(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::TST_REG(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::TST_RSR(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}


void ARMv7Interpreter::UADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UBFX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UDIV(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHSAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHSUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UHSUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UMAAL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UMLAL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UMULL(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQADD16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQADD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQASX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQSAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQSUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UQSUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USAD8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USADA8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USAT(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USAT16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USAX(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USUB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::USUB8(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTAB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTAB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTAH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTB(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTB16(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}

void ARMv7Interpreter::UXTH(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case A1: throw __FUNCTION__;
	default: throw __FUNCTION__;
	}
}
