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

	virtual void UNK(const u32 data);

	virtual void NULL_OP(const u32 data, const ARMv7_encoding type);
	virtual void NOP(const u32 data, const ARMv7_encoding type);

	virtual void PUSH(const u32 data, const ARMv7_encoding type);
	virtual void POP(const u32 data, const ARMv7_encoding type);

	virtual void B(const u32 data, const ARMv7_encoding type);
	virtual void CBZ(const u32 data, const ARMv7_encoding type);
	virtual void CBNZ(const u32 data, const ARMv7_encoding type);
	virtual void BL(const u32 data, const ARMv7_encoding type);
	virtual void BLX(const u32 data, const ARMv7_encoding type);

	virtual void ADC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADC_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void ADD_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADD_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADD_RSR(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPI(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPR(const u32 data, const ARMv7_encoding type);

	virtual void MOV_IMM(const u32 data, const ARMv7_encoding type);
	virtual void MOV_REG(const u32 data, const ARMv7_encoding type);
	virtual void MOVT(const u32 data, const ARMv7_encoding type);

	virtual void SUB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void SUB_REG(const u32 data, const ARMv7_encoding type);
	virtual void SUB_RSR(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPI(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPR(const u32 data, const ARMv7_encoding type);

	virtual void STR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STR_REG(const u32 data, const ARMv7_encoding type);
};
