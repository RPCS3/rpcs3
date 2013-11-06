#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"

class ARMv7Interpreter : public ARMv7Opcodes
{
	ARMv7Thread& CPU;

public:
	ARMv7Interpreter(ARMv7Thread& cpu) : CPU(cpu)
	{
	}

	bool ConditionPassed(u8 cond)
	{
		bool result;

		switch(cond >> 1)
		{
		case 0: result = CPU.APSR.Z == 1; break;
		case 1: result = CPU.APSR.C == 1; break;
		case 2: result = CPU.APSR.N == 1; break;
		case 3: result = CPU.APSR.V == 1; break;
		case 4: result = CPU.APSR.C == 1 && CPU.APSR.Z == 0; break;
		case 5: result = CPU.APSR.N == CPU.APSR.V; break;
		case 6: result = CPU.APSR.N == CPU.APSR.V && CPU.APSR.Z == 0; break;
		case 7: return true;
		}

		if(cond & 0x1)
		{
			return !result;
		}

		return result;
	}

protected:
	void NULL_OP()
	{
		ConLog.Error("null");
		Emu.Pause();
	}

	void NOP()
	{
	}

	void PUSH(u16 regs_list)
	{
		for(u16 mask=0x1, i=0; mask; mask <<= 1, i++)
		{
			if(regs_list & mask)
			{
				Memory.Write32(CPU.SP, CPU.read_gpr(i));
				CPU.SP += 4;
			}
		}
	}

	void POP(u16 regs_list)
	{
		for(u16 mask=(0x1 << 15), i=15; mask; mask >>= 1, i++)
		{
			if(regs_list & mask)
			{
				CPU.SP -= 4;
				CPU.write_gpr(i, Memory.Read32(CPU.SP));
			}
		}
	}

	void B(u8 cond, u32 imm, u8 intstr_size)
	{
		if(ConditionPassed(cond))
		{
			CPU.SetBranch(CPU.PC + intstr_size + imm);
		}
	}

	void CBZ(u8 op, u32 imm, u8 rn, u8 intstr_size)
	{
		if((CPU.GPR[rn] == 0) ^ op)
		{
			CPU.SetBranch(CPU.PC + intstr_size + imm);
		}
	}

	void BL(u32 imm, u8 intstr_size)
	{
		CPU.LR = (CPU.PC + intstr_size) | 1;
		CPU.SetBranch(CPU.PC + intstr_size + imm);
	}

	void UNK(const u16 code0, const u16 code1)
	{
		ConLog.Error("Unknown/Illegal opcode! (0x%04x : 0x%04x)", code0, code1);
		Emu.Pause();
	}
};