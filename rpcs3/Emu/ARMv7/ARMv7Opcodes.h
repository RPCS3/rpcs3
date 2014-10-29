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

enum ARMv7_encoding
{
	T1,
	T2,
	T3,
	T4,
	A1,
	A2,
};

class ARMv7Opcodes
{
public:
	virtual void UNK(const u32 data) = 0;

	virtual void NULL_OP(const u32 data, const ARMv7_encoding type) = 0;
	virtual void NOP(const u32 data, const ARMv7_encoding type) = 0;

	virtual void PUSH(const u32 data, const ARMv7_encoding type) = 0;
	virtual void POP(const u32 data, const ARMv7_encoding type) = 0;

	virtual void B(const u32 data, const ARMv7_encoding type) = 0;
	virtual void CBZ(const u32 data, const ARMv7_encoding type) = 0;
	virtual void CBNZ(const u32 data, const ARMv7_encoding type) = 0;
	virtual void BL(const u32 data, const ARMv7_encoding type) = 0;
	virtual void BLX(const u32 data, const ARMv7_encoding type) = 0;

	virtual void ADC_IMM(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADC_REG(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADC_RSR(const u32 data, const ARMv7_encoding type) = 0;

	virtual void ADD_IMM(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADD_REG(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADD_RSR(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADD_SPI(const u32 data, const ARMv7_encoding type) = 0;
	virtual void ADD_SPR(const u32 data, const ARMv7_encoding type) = 0;

	virtual void MOV_IMM(const u32 data, const ARMv7_encoding type) = 0;
	virtual void MOV_REG(const u32 data, const ARMv7_encoding type) = 0;
	virtual void MOVT(const u32 data, const ARMv7_encoding type) = 0;

	virtual void SUB_IMM(const u32 data, const ARMv7_encoding type) = 0;
	virtual void SUB_REG(const u32 data, const ARMv7_encoding type) = 0;
	virtual void SUB_RSR(const u32 data, const ARMv7_encoding type) = 0;
	virtual void SUB_SPI(const u32 data, const ARMv7_encoding type) = 0;
	virtual void SUB_SPR(const u32 data, const ARMv7_encoding type) = 0;

	virtual void STR_IMM(const u32 data, const ARMv7_encoding type) = 0;
	virtual void STR_REG(const u32 data, const ARMv7_encoding type) = 0;
};

struct ARMv7_opcode_t
{
	u32 mask;
	u32 code;
	u32 length; // 2 or 4
	const char* name;
	ARMv7_encoding type;
	void (ARMv7Opcodes::*func)(const u32 data, const ARMv7_encoding type);
};

// single 16-bit value
#define ARMv7_OP2(mask, code, type, name) { (u32)((mask) << 16), (u32)((code) << 16), 2, #name "_" #type, type, &ARMv7Opcodes::name }
// two 16-bit values
#define ARMv7_OP4(mask0, mask1, code0, code1, type, name) { (u32)((mask0) << 16) | (mask1), (u32)((code0) << 16) | (code1), 4, #name "_" #type, type, &ARMv7Opcodes::name }

static const ARMv7_opcode_t ARMv7_opcode_table[] = 
{
	ARMv7_OP2(0xffff, 0x0000, T1, NULL_OP),
	ARMv7_OP2(0xffff, 0xbf00, T1, NOP),
	ARMv7_OP4(0xffff, 0xffff, 0xf3af, 0x8000, T2, NOP),
	ARMv7_OP4(0x0fff, 0xffff, 0x0320, 0xf000, A1, NOP),
	ARMv7_OP2(0xfe00, 0xb400, T1, PUSH),
	ARMv7_OP4(0xffff, 0x0000, 0xe92d, 0x0000, T2, PUSH), // had an error in arch ref
	ARMv7_OP4(0xffff, 0x0fff, 0xf84d, 0x0d04, T3, PUSH),
	ARMv7_OP4(0x0fff, 0x0000, 0x092d, 0x0000, A1, PUSH),
	ARMv7_OP4(0x0fff, 0x0fff, 0x052d, 0x0004, A2, PUSH),
	ARMv7_OP2(0xfe00, 0xbc00, T1, POP),
	ARMv7_OP4(0xffff, 0x0000, 0xe8bd, 0x0000, T2, POP),
	ARMv7_OP4(0xffff, 0x0fff, 0xf85d, 0x0b04, T3, POP),
	ARMv7_OP4(0x0fff, 0x0000, 0x08bd, 0x0000, A1, POP),
	ARMv7_OP4(0x0fff, 0x0fff, 0x049d, 0x0004, A2, POP),
	ARMv7_OP2(0xf000, 0xd000, T1, B),
	ARMv7_OP2(0xf800, 0xe000, T2, B),
	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0x8000, T3, B),
	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0x9000, T4, B),
	ARMv7_OP4(0x0f00, 0x0000, 0x0a00, 0x0000, A1, B),
	ARMv7_OP2(0xfd00, 0xb100, T1, CBZ),
	ARMv7_OP2(0xfd00, 0xb900, T1, CBNZ),
	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0xd000, T1, BL),
	ARMv7_OP4(0x0f00, 0x0000, 0x0b00, 0x0000, A1, BL),
	ARMv7_OP2(0xff80, 0x4780, T1, BLX),
	ARMv7_OP4(0xf800, 0xc001, 0xf000, 0xc000, T2, BLX),
	ARMv7_OP4(0x0fff, 0xfff0, 0x012f, 0xff30, A1, BLX),
	ARMv7_OP4(0xfe00, 0x0000, 0xfa00, 0x0000, A2, BLX),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf140, 0x0000, T1, ADC_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x02a0, 0x0000, A1, ADC_IMM),
	ARMv7_OP2(0xffc0, 0x4040, T1, ADC_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeb40, 0x0000, T2, ADC_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x00a0, 0x0000, A1, ADC_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x00a0, 0x0010, A1, ADC_RSR),

	ARMv7_OP2(0xfe00, 0x1c00, T1, ADD_IMM),
	ARMv7_OP2(0xf800, 0x3000, T2, ADD_IMM),
	ARMv7_OP4(0xfbe0, 0x8000, 0xf100, 0x0000, T3, ADD_IMM),
	ARMv7_OP4(0xfbf0, 0x8000, 0xf200, 0x0000, T4, ADD_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0280, 0x0000, A1, ADD_IMM),
	ARMv7_OP2(0xfe00, 0x1800, T1, ADD_REG),
	ARMv7_OP2(0xff00, 0x4400, T2, ADD_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeb00, 0x0000, T3, ADD_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0080, 0x0000, A1, ADD_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0080, 0x0010, A1, ADD_RSR),
	ARMv7_OP2(0xf800, 0xa800, T1, ADD_SPI),
	ARMv7_OP2(0xff80, 0xb000, T2, ADD_SPI),
	ARMv7_OP4(0xfbef, 0x8000, 0xf10d, 0x0000, T3, ADD_SPI),
	ARMv7_OP4(0xfbff, 0x8000, 0xf20d, 0x0000, T4, ADD_SPI),
	ARMv7_OP4(0x0fef, 0x0000, 0x028d, 0x0000, A1, ADD_SPI),
	ARMv7_OP2(0xff78, 0x4468, T1, ADD_SPR),
	ARMv7_OP2(0xff87, 0x4485, T2, ADD_SPR),
	ARMv7_OP4(0xffef, 0x8000, 0xeb0d, 0x0000, T3, ADD_SPR),
	ARMv7_OP4(0x0fef, 0x0010, 0x008d, 0x0000, A1, ADD_SPR),

	ARMv7_OP2(0xf800, 0x2000, T1, MOV_IMM),
	ARMv7_OP4(0xfbef, 0x8000, 0xf04f, 0x0000, T2, MOV_IMM),
	ARMv7_OP4(0xfbf0, 0x8000, 0xf240, 0x0000, T3, MOV_IMM),
	ARMv7_OP4(0x0fef, 0x0000, 0x03a0, 0x0000, A1, MOV_IMM),
	ARMv7_OP4(0x0ff0, 0x0000, 0x0300, 0x0000, A2, MOV_IMM),
	ARMv7_OP2(0xff00, 0x4600, T1, MOV_REG),
	ARMv7_OP2(0xffc0, 0x0000, T2, MOV_REG),
	ARMv7_OP4(0xffef, 0xf0f0, 0xea4f, 0x0000, T3, MOV_REG),
	ARMv7_OP4(0x0fef, 0x0ff0, 0x01a0, 0x0000, A1, MOV_REG),
	ARMv7_OP4(0xfbf0, 0x8000, 0xf2c0, 0x0000, T1, MOVT),
	ARMv7_OP4(0x0ff0, 0x0000, 0x0340, 0x0000, A1, MOVT),

	ARMv7_OP2(0xfe00, 0x1e00, T1, SUB_IMM),
	ARMv7_OP2(0xf800, 0x3800, T2, SUB_IMM),
	ARMv7_OP4(0xfbe0, 0x8000, 0xf1a0, 0x0000, T3, SUB_IMM),
	ARMv7_OP4(0xfbf0, 0x8000, 0xf2a0, 0x0000, T4, SUB_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0240, 0x0000, A1, SUB_IMM),
	ARMv7_OP2(0xfe00, 0x1a00, T1, SUB_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeba0, 0x0000, T2, SUB_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0040, 0x0000, A1, SUB_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0040, 0x0010, A1, SUB_RSR),
	ARMv7_OP2(0xff80, 0xb080, T1, SUB_SPI),
	ARMv7_OP4(0xfbef, 0x8000, 0xf1ad, 0x0000, T2, SUB_SPI),
	ARMv7_OP4(0xfbff, 0x8000, 0xf2ad, 0x0000, T3, SUB_SPI),
	ARMv7_OP4(0x0fef, 0x0000, 0x024d, 0x0000, A1, SUB_SPI),
	ARMv7_OP4(0xffef, 0x8000, 0xebad, 0x0000, T1, SUB_SPR),
	ARMv7_OP4(0x0fef, 0x0010, 0x004d, 0x0000, A1, SUB_SPR),

	ARMv7_OP2(0xf800, 0x6000, T1, STR_IMM),
	ARMv7_OP2(0xf800, 0x9000, T2, STR_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf8c0, 0x0000, T3, STR_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf840, 0x0800, T4, STR_IMM),
	ARMv7_OP4(0x0e50, 0x0000, 0x0400, 0x0000, A1, STR_IMM),
	ARMv7_OP2(0xfe00, 0x5000, T1, STR_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf840, 0x0000, T2, STR_REG),
	ARMv7_OP4(0x0e50, 0x0010, 0x0600, 0x0000, A1, STR_REG),
};

#undef ARMv7_OP
#undef ARMv7_OPP

