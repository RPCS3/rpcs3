#pragma once

static const char* g_arm_reg_name[16] =
{
	"r0", "r1", "r2", "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "sp", "lr", "pc",
};

namespace ARMv7_opcodes
{
	enum ARMv7_T1Opcodes
	{
		T1_CBZ = 0xb,
		T1_B = 0xd,
		T1_PUSH = 0x1d,
		T1_POP = 0x5e,
		T1_NOP = 0xBF00,
	};

	enum ARMv7_T2Opcodes
	{
		T2_B = 0x1c,
		T2_PUSH = 0xe92d,
		T2_POP = 0xe8bd,
	};

	enum ARMv7_T3Opcodes
	{
		T3_B = 0x1e,
	};
}

class ARMv7Opcodes
{
public:
	virtual void NULL_OP() = 0;
	virtual void NOP() = 0;

	virtual void PUSH(u16 regs_list) = 0;
	virtual void POP(u16 regs_list) = 0;

	virtual void B(u8 cond, u32 imm, u8 intstr_size) = 0;
	virtual void CBZ(u8 op, u32 imm, u8 rn, u8 intstr_size) = 0;
	virtual void BL(u32 imm, u8 intstr_size)=0;
	
	virtual void UNK(const u16 code0, const u16 code1) = 0;
};
