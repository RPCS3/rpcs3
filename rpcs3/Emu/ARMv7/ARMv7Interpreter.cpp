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

void ARMv7Interpreter::B(const u32 data, const ARMv7_encoding type)
{
	u8 cond = 0xf;
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

void ARMv7Interpreter::CBZ(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: break;
	default: throw __FUNCTION__;
	}

	if (CPU.GPR[data & 0x7] == 0)
	{
		CPU.SetBranch(CPU.PC + 2 + ((data & 0xf8) >> 2) + ((data & 0x200) >> 3));
	}
}

void ARMv7Interpreter::CBNZ(const u32 data, const ARMv7_encoding type)
{
	switch (type)
	{
	case T1: break;
	default: throw __FUNCTION__;
	}

	if (CPU.GPR[data & 0x7] != 0)
	{
		CPU.SetBranch(CPU.PC + 2 + ((data & 0xf8) >> 2) + ((data & 0x200) >> 3));
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

void ARMv7Interpreter::SUB_IMM(const u32 data, const ARMv7_encoding type)
{

}

void ARMv7Interpreter::SUB_REG(const u32 data, const ARMv7_encoding type)
{

}

void ARMv7Interpreter::SUB_RSR(const u32 data, const ARMv7_encoding type)
{

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

}

