#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

static const char* g_arm_cond_name[16] =
{
	"eq", "ne", "cs", "cc",
	"mi", "pl", "vs", "vc",
	"hi", "ls", "ge", "lt",
	"gt", "le", "al", "al",
};

class ARMv7DisAsm
	: public CPUDisAsm
	, public ARMv7Opcodes
{
public:
	ARMv7DisAsm(CPUDisAsmMode mode) : CPUDisAsm(mode)
	{
	}

protected:
	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return dump_pc + imm;
	}

	std::string GetRegsListString(u16 regs_list)
	{
		std::string regs_str;

		for(u16 mask=0x1, i=0; mask; mask <<= 1, i++)
		{
			if(regs_list & mask)
			{
				if(!regs_str.empty())
				{
					regs_str += ", ";
				}

				regs_str += g_arm_reg_name[i];
			}
		}

		return regs_str;
	}

	void NULL_OP()
	{
		Write("null");
	}

	void PUSH(u16 regs_list)
	{
		Write(fmt::Format("push {%s}", GetRegsListString(regs_list).c_str()));
	}

	void POP(u16 regs_list)
	{
		Write(fmt::Format("pop {%s}", GetRegsListString(regs_list).c_str()));
	}

	void NOP()
	{
		Write("nop");
	}

	void B(u8 cond, u32 imm, u8 intstr_size)
	{
		if((cond & 0xe) == 0xe)
		{
			Write(fmt::Format("b 0x%x", DisAsmBranchTarget(imm) + intstr_size));
		}
		else
		{
			Write(fmt::Format("b[%s] 0x%x", g_arm_cond_name[cond], DisAsmBranchTarget(imm) + intstr_size));
		}
	}

	virtual void CBZ(u8 op, u32 imm, u8 rn, u8 intstr_size)
	{
		Write(fmt::Format("cb%sz 0x%x,%s", (op ? "n" : ""), DisAsmBranchTarget(imm) + intstr_size, g_arm_reg_name[rn]));
	}

	void BL(u32 imm, u8 intstr_size)
	{
		Write(fmt::Format("bl 0x%x", DisAsmBranchTarget(imm) + intstr_size));
	}

	void UNK(const u16 code0, const u16 code1)
	{
		Write(fmt::Format("Unknown/Illegal opcode! (0x%04x : 0x%04x)", code0, code1));
	}
};
