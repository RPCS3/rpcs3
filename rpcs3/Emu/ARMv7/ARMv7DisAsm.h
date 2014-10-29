#pragma once
#include "Emu/ARMv7/ARMv7Opcodes.h"
#include "Emu/CPU/CPUDisAsm.h"

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
		return (u32)dump_pc + imm;
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

	void UNK(const u32 data);

	void NULL_OP(const u32 data, const ARMv7_encoding type);
	void NOP(const u32 data, const ARMv7_encoding type);

	void PUSH(const u32 data, const ARMv7_encoding type);
	void POP(const u32 data, const ARMv7_encoding type);

	void B(const u32 data, const ARMv7_encoding type);
	void CBZ(const u32 data, const ARMv7_encoding type);
	void CBNZ(const u32 data, const ARMv7_encoding type);
	void BL(const u32 data, const ARMv7_encoding type);
	void BLX(const u32 data, const ARMv7_encoding type);

	void SUB_IMM(const u32 data, const ARMv7_encoding type);
	void SUB_REG(const u32 data, const ARMv7_encoding type);
	void SUB_RSR(const u32 data, const ARMv7_encoding type);
	void SUB_SPI(const u32 data, const ARMv7_encoding type);
	void SUB_SPR(const u32 data, const ARMv7_encoding type);
};
