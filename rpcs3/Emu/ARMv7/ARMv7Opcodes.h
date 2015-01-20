#pragma once

#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/ARMv7/ARMv7Interpreter.h"
//#include "Emu/System.h"
//#include "Utilities/Log.h"

static const char* g_arm_reg_name[16] =
{
	"r0", "r1", "r2", "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "sp", "lr", "pc",
};

using namespace ARMv7_instrs;

struct ARMv7_Instruction
{
	void(*func)(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	u8 size;
	ARMv7_encoding type;
	const char* name;
};

#if 0

#define ARMv7_OP_2(func, type) { func, 2, type, #func "_" #type }
#define ARMv7_OP_4(func, type) { func, 4, type, #func "_" #type }
#define ARMv7_NULL_OP { NULL_OP, 2, T1, "NULL_OP" }


// 0x1...
static void group_0x1(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x1_main[] =
{
	ARMv7_OP_2(ASR_IMM, T1), // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(ADD_REG, T1), // 8 0xfe00
	ARMv7_NULL_OP,           // 9
	ARMv7_OP_2(SUB_REG, T1), // A 0xfe00
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(ADD_IMM, T1), // C 0xfe00
	ARMv7_NULL_OP,           // D
	ARMv7_OP_2(SUB_IMM, T1)  // E 0xfe00
};

static const ARMv7_Instruction g_table_0x1[] =
{
	{ group_0x1 }
};

static void group_0x1(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0e00) >> 8;

	if ((thr->code.code0 & 0xf800) == 0x1000) index = 0x0;

	thr->m_last_instr_name = g_table_0x1_main[index].name;
	thr->m_last_instr_size = g_table_0x1_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x1_main[index].func(thr, g_table_0x1_main[index].type);
}

// 0x2...
static void group_0x2(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x2_main[] =
{
	ARMv7_OP_2(MOV_IMM, T1), // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(CMP_IMM, T1)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0x2[] =
{
	{ group_0x2 }
};

static void group_0x2(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x2_main[index].name;
	thr->m_last_instr_size = g_table_0x2_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x2_main[index].func(thr, g_table_0x2_main[index].type);
}

// 0x3...
static void group_0x3(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x3_main[] =
{
	ARMv7_OP_2(ADD_IMM, T2), // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(SUB_IMM, T2)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0x3[] =
{
	{ group_0x3 }
};

static void group_0x3(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x3_main[index].name;
	thr->m_last_instr_size = g_table_0x3_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x3_main[index].func(thr, g_table_0x3_main[index].type);
}

// 0x4...
static void group_0x4(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x40(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x41(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x42(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x43(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x44(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0x47(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x4[] =
{
	{ group_0x4 }
};

static const ARMv7_Instruction g_table_0x40[] =
{
	ARMv7_OP_2(AND_REG, T1), // 0 0xffc0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_2(ADC_REG, T1), // 4 0xffc0
	// ARMv7_OP_2(EOR_REG, T1), // 4 0xffc0   code(ADC_REG, T1) == code(EOR_REG, T1) ???
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(LSL_REG, T1), // 8 0xffc0
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(LSR_REG, T1)  // C 0xffc0
};

static void group_0x40(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00c0) >> 4;
	thr->m_last_instr_name = g_table_0x40[index].name;
	thr->m_last_instr_size = g_table_0x40[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x40[index].func(thr, g_table_0x40[index].type);
}

static const ARMv7_Instruction g_table_0x41[] =
{
	ARMv7_OP_2(ASR_REG, T1), // 0 0xffc0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(SBC_REG, T1), // 8 0xffc0
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(ROR_REG, T1)  // C 0xffc0
};

static void group_0x41(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00c0) >> 4;
	thr->m_last_instr_name = g_table_0x41[index].name;
	thr->m_last_instr_size = g_table_0x41[index].size;
	g_table_0x41[index].func(thr, g_table_0x41[index].type);
}

static const ARMv7_Instruction g_table_0x42[] =
{
	ARMv7_OP_2(TST_REG, T1), // 0 0xffc0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_2(RSB_IMM, T1), // 4 0xffc0
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(CMP_REG, T1), // 8 0xffc0
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(CMN_REG, T1)  // C 0xffc0
};

static void group_0x42(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00c0) >> 4;
	thr->m_last_instr_name = g_table_0x42[index].name;
	thr->m_last_instr_size = g_table_0x42[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x42[index].func(thr, g_table_0x42[index].type);
}

static const ARMv7_Instruction g_table_0x43[] =
{
	ARMv7_OP_2(ORR_REG, T1), // 0 0xffc0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_2(MUL, T1),     // 4 0xffc0
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(BIC_REG, T1), // 8 0xffc0
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(MVN_REG, T1)  // C 0xffc0
};

static void group_0x43(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00c0) >> 4;
	thr->m_last_instr_name = g_table_0x43[index].name;
	thr->m_last_instr_size = g_table_0x43[index].size;
	g_table_0x43[index].func(thr, g_table_0x43[index].type);
}

static const ARMv7_Instruction g_table_0x44[] =
{
	ARMv7_OP_2(ADD_REG, T2), // 0 0xff00
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_OP_2(ADD_SPR, T1), // 6 0xff78
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(ADD_SPR, T2)  // 8 0xff87
};

static void group_0x44(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0080) >> 4;

	if ((thr->code.code0 & 0xff00) == 0x4400) index = 0x0;
	if ((thr->code.code0 & 0xff78) == 0x4468) index = 0x6;

	thr->m_last_instr_name = g_table_0x44[index].name;
	thr->m_last_instr_size = g_table_0x44[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x44[index].func(thr, g_table_0x44[index].type);
}

static const ARMv7_Instruction g_table_0x47[] =
{
	ARMv7_OP_2(BX, T1),      // 0 0xff87
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(BLX, T1)      // 8 0xff80
};

static void group_0x47(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0080) >> 4;
	thr->m_last_instr_name = g_table_0x47[index].name;
	thr->m_last_instr_size = g_table_0x47[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x47[index].func(thr, g_table_0x47[index].type);
}

static const ARMv7_Instruction g_table_0x4_main[] =
{
	{ group_0x40 },          // 0
	{ group_0x41 },          // 1
	{ group_0x42 },          // 2
	{ group_0x43 },          // 3
	{ group_0x44 },          // 4
	ARMv7_OP_2(CMP_REG, T2), // 5 0xff00
	ARMv7_OP_2(MOV_REG, T1), // 6 0xff00
	{ group_0x47 },          // 7
	ARMv7_OP_2(LDR_LIT, T1)  // 8 0xf800
};

static void group_0x4(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0f00) >> 8;

	if ((index & 0xf800) == 0x4800) index = 0x8;

	thr->m_last_instr_name = g_table_0x4_main[index].name;
	thr->m_last_instr_size = g_table_0x4_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x4_main[index].func(thr, g_table_0x4_main[index].type);
}

// 0x5...
static void group_0x5(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x5_main[] =
{
	ARMv7_OP_2(STR_REG, T1),   // 0 0xfe00
	ARMv7_NULL_OP,             // 1
	ARMv7_OP_2(STRH_REG, T1),  // 2 0xfe00
	ARMv7_NULL_OP,             // 3
	ARMv7_OP_2(STRB_REG, T1),  // 4 0xfe00
	ARMv7_NULL_OP,             // 5
	ARMv7_OP_2(LDRSB_REG, T1), // 6 0xfe00
	ARMv7_NULL_OP,             // 7
	ARMv7_OP_2(LDR_REG, T1),   // 8 0xfe00
	ARMv7_NULL_OP,             // 9
	ARMv7_NULL_OP,             // A
	ARMv7_NULL_OP,             // B
	ARMv7_OP_2(LDRB_REG, T1),  // C 0xfe00
	ARMv7_NULL_OP,             // D
	ARMv7_OP_2(LDRSH_REG, T1)  // E 0xfe00
};

static const ARMv7_Instruction g_table_0x5[] =
{
	{ group_0x5 }
};

static void group_0x5(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0e00) >> 8;
	thr->m_last_instr_name = g_table_0x5_main[index].name;
	thr->m_last_instr_size = g_table_0x5_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x5_main[index].func(thr, g_table_0x5_main[index].type);
}

// 0x6...
static void group_0x6(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x6_main[] =
{
	ARMv7_OP_2(STR_IMM, T1), // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(LDR_IMM, T1)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0x6[] =
{
	{ group_0x6 }
};

static void group_0x6(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x6_main[index].name;
	thr->m_last_instr_size = g_table_0x6_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x6_main[index].func(thr, g_table_0x6_main[index].type);
}

// 0x7...
static void group_0x7(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x7_main[] =
{
	ARMv7_OP_2(STRB_IMM, T1), // 0 0xf800
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_2(LDRB_IMM, T1)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0x7[] =
{
	{ group_0x7 }
};

static void group_0x7(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x7_main[index].name;
	thr->m_last_instr_size = g_table_0x7_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x7_main[index].func(thr, g_table_0x7_main[index].type);
}

// 0x8...
static void group_0x8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x8_main[] =
{
	ARMv7_OP_2(STRH_IMM, T1)  // 0 0xf800
};

static const ARMv7_Instruction g_table_0x8[] =
{
	{ group_0x8 }
};

static void group_0x8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x8_main[index].name;
	thr->m_last_instr_size = g_table_0x8_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x8_main[index].func(thr, g_table_0x8_main[index].type);
}

// 0x9...
static void group_0x9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0x9_main[] =
{
	ARMv7_OP_2(STR_IMM, T2), // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(LDR_IMM, T2)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0x9[] =
{
	{ group_0x9 }
};

static void group_0x9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0x9_main[index].name;
	thr->m_last_instr_size = g_table_0x9_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0x9_main[index].func(thr, g_table_0x9_main[index].type);
}

// 0xa...
static void group_0xa(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0xa_main[] =
{
	ARMv7_OP_2(ADR, T1),     // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(ADD_SPI, T1)  // 8 0xf800
};

static const ARMv7_Instruction g_table_0xa[] =
{
	{ group_0xa }
};

static void group_0xa(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0xa_main[index].name;
	thr->m_last_instr_size = g_table_0xa_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xa_main[index].func(thr, g_table_0xa_main[index].type);
}

// 0xb...
static void group_0xb(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xb0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xba(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0xb0[] =
{
	ARMv7_OP_2(ADD_SPI, T2), // 0 0xff80
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(SUB_SPI, T1)  // 8 0xff80
};

static void group_0xb0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0080) >> 4;
	thr->m_last_instr_name = g_table_0xb0[index].name;
	thr->m_last_instr_size = g_table_0xb0[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xb0[index].func(thr, g_table_0xb0[index].type);
}

static const ARMv7_Instruction g_table_0xba[] =
{
	ARMv7_OP_2(REV, T1),     // 0 0xffc0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_2(REV16, T1),   // 4 0xffc0
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(REVSH, T1)    // C 0xffc0
};

static void group_0xba(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00c0) >> 4; // mask 0xffc0
	thr->m_last_instr_name = g_table_0xba[index].name;
	thr->m_last_instr_size = g_table_0xba[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xba[index].func(thr, g_table_0xba[index].type);
}

static const ARMv7_Instruction g_table_0xb_main[] =
{
	{ group_0xb0 },          // 0
	ARMv7_OP_2(CB_Z, T1),    // 1 0xf500
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_2(PUSH, T1),    // 4 0xfe00
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	{ group_0xba },          // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_2(POP, T1),     // C 0xfe00
	ARMv7_NULL_OP,           // D
	ARMv7_OP_2(BKPT, T1),    // E 0xff00
	ARMv7_OP_2(NOP, T1),     // F 0xffff
	ARMv7_OP_2(IT, T1),      // 10 0xff00
};

static const ARMv7_Instruction g_table_0xb[] =
{
	{ group_0xb }
};

static void group_0xb(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0e00) >> 8;

	if ((thr->code.code0 & 0xf500) == 0xb100) index = 0x1;  // CB_Z, T1
	if ((thr->code.code0 & 0xff00) == 0xbe00) index = 0xe;  // BKPT, T1
	if ((thr->code.code0 & 0xffff) == 0xbf00) index = 0xf;  // NOP, T1
	if ((thr->code.code0 & 0xff00) == 0xbf00) index = 0x10; // IT, T1

	thr->m_last_instr_name = g_table_0xb_main[index].name;
	thr->m_last_instr_size = g_table_0xb_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xb_main[index].func(thr, g_table_0xb_main[index].type);
}

// 0xc...
static void group_0xc(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0xc_main[] =
{
	ARMv7_OP_2(STM, T1),     // 0 0xf800
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_2(LDM, T1)      // 8 0xf800
};

static const ARMv7_Instruction g_table_0xc[] =
{
	{ group_0xc }
};

static void group_0xc(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x0800) >> 8;
	thr->m_last_instr_name = g_table_0xc_main[index].name;
	thr->m_last_instr_size = g_table_0xc_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xc_main[index].func(thr, g_table_0xc_main[index].type);
}

// 0xd...
static void group_0xd(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0xd_main[] =
{
	ARMv7_OP_2(B, T1),       // 0 0xf000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_NULL_OP,           // E
	ARMv7_OP_2(SVC, T1)      // F 0xff00
};

static const ARMv7_Instruction g_table_0xd[] =
{
	{ group_0xd }
};

static void group_0xd(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	//u32 index = (thr->code.code0 & 0x0f00) >> 8;
	//if ((thr->code.code0 & 0xf000) == 0xd000) index = 0;

	const u32 index = (thr->code.code0 & 0xff00) == 0xdf00 ? 0xf : 0x0; // check me
	thr->m_last_instr_name = g_table_0xd_main[index].name;
	thr->m_last_instr_size = g_table_0xd_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xd_main[index].func(thr, g_table_0xd_main[index].type);
}

// 0xe...
static void group_0xe(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xe85(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xe8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xe9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea4(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea4f(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea4f0000(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea4f0030(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xea6(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xeb(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xeb0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xeba(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);


static const ARMv7_Instruction g_table_0xe85[] =
{
	ARMv7_OP_4(LDRD_IMM, T1), // 0 0xfe50, 0x0000
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_NULL_OP,            // 8
	ARMv7_NULL_OP,            // 9
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_NULL_OP,            // C
	ARMv7_NULL_OP,            // D
	ARMv7_NULL_OP,            // E
	ARMv7_OP_4(LDRD_LIT, T1)  // F 0xfe7f, 0x0000
};

static void group_0xe85(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	//u32 index = thr->code.code0 & 0x000f;
	//if ((thr->code.code0 & 0xfe50) == 0xe850) index = 0x0;

	const u32 index = (thr->code.code0 & 0xfe7f) == 0xe85f ? 0xf : 0x0; // check me
	thr->m_last_instr_name = g_table_0xe85[index].name;
	thr->m_last_instr_size = g_table_0xe85[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xe85[index].func(thr, g_table_0xe85[index].type);
};

static const ARMv7_Instruction g_table_0xe8[] =
{
	ARMv7_NULL_OP,            // 0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_OP_4(STRD_IMM, T1), // 4 0xfe50, 0x0000
	{ group_0xe85 },          // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(STM, T2),      // 8 0xffd0, 0xa000
	ARMv7_OP_4(LDM, T2),      // 9 0xffd0, 0x2000
	ARMv7_NULL_OP,            // A
	ARMv7_OP_4(POP, T2),      // B 0xffff, 0x0000
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(TB_, T1)       // D 0xfff0, 0xffe0
};

static void group_0xe8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00f0) >> 4;

	if ((thr->code.code0 & 0xfe50) == 0xe840) index = 0x4;
	if ((thr->code.code0 & 0xffd0) == 0xe880) index = 0x8;
	if ((thr->code.code0 & 0xffd0) == 0xe890) index = 0x9;

	thr->m_last_instr_name = g_table_0xe8[index].name;
	thr->m_last_instr_size = g_table_0xe8[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xe8[index].func(thr, g_table_0xe8[index].type);
}

static const ARMv7_Instruction g_table_0xe9[] =
{
	ARMv7_OP_4(STMDB, T1),    // 0 0xffd0, 0xa000
	ARMv7_OP_4(LDMDB, T1),    // 1 0xffd0, 0x2000
	ARMv7_OP_4(PUSH, T2)      // 2 0xffff, 0x0000
};

static void group_0xe9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00d0) >> 4;

	if ((thr->code.code0 & 0xffff) == 0xe92d) index = 0x2;

	thr->m_last_instr_name = g_table_0xe9[index].name;
	thr->m_last_instr_size = g_table_0xe9[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xe9[index].func(thr, g_table_0xe9[index].type);
}

static const ARMv7_Instruction g_table_0xea4[] =
{
	ARMv7_OP_4(ORR_REG, T2), // 0 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_NULL_OP,           // E
	{ group_0xea4f }         // F
};

static void group_0xea4(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = 0x0;
	if ((thr->code.code0 & 0xffef) == 0xea4f) index = 0xf; // check me

	thr->m_last_instr_name = g_table_0xea4[index].name;
	thr->m_last_instr_size = g_table_0xea4[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea4[index].func(thr, g_table_0xea4[index].type);
}

static const ARMv7_Instruction g_table_0xea4f[] =
{
	{ group_0xea4f0000 },    // 0
	ARMv7_OP_4(ASR_IMM, T2), // 1 0xffef, 0x8030
	ARMv7_OP_4(ASR_IMM, T2), // 2 0xffef, 0x8030
	{ group_0xea4f0030 }     // 3
};

static void group_0xea4f(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code1 & 0x0030) >> 4;
	thr->m_last_instr_name = g_table_0xea4f[index].name;
	thr->m_last_instr_size = g_table_0xea4f[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea4f[index].func(thr, g_table_0xea4f[index].type);
}

static const ARMv7_Instruction g_table_0xea4f0000[] =
{
	ARMv7_OP_4(MOV_REG, T3), // 0 0xffef, 0xf0f0
	ARMv7_OP_4(LSL_IMM, T2)  // 1 0xffef, 0x8030
};

static void group_0xea4f0000(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = thr->code.code1 & 0x8030 ? 0x0 : 0x1;
	thr->m_last_instr_name = g_table_0xea4f0000[index].name;
	thr->m_last_instr_size = g_table_0xea4f0000[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea4f0000[index].func(thr, g_table_0xea4f0000[index].type);
}

static const ARMv7_Instruction g_table_0xea4f0030[] =
{
	ARMv7_OP_4(RRX, T1),     // 1 0xffef, 0xf0f0
	ARMv7_OP_4(ROR_IMM, T1)  // 2 0xffef, 0x8030
};

static void group_0xea4f0030(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = thr->code.code1 & 0x8030 ? 0x0 : 0x1;
	thr->m_last_instr_name = g_table_0xea4f0030[index].name;
	thr->m_last_instr_size = g_table_0xea4f0030[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea4f0030[index].func(thr, g_table_0xea4f0030[index].type);
}

static const ARMv7_Instruction g_table_0xea6[] =
{
	ARMv7_OP_4(ORN_REG, T1), // 0 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(MVN_REG, T2)  // F 0xffef, 0x8000
};

static void group_0xea6(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xffe08000) == 0xea600000) index = 0x0;

	thr->m_last_instr_name = g_table_0xea6[index].name;
	thr->m_last_instr_size = g_table_0xea6[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea6[index].func(thr, g_table_0xea6[index].type);
}

static const ARMv7_Instruction g_table_0xea[] =
{
	ARMv7_OP_4(AND_REG, T2),  // 0 0xffe0, 0x8000
	ARMv7_OP_4(TST_REG, T2),  // 1 0xfff0, 0x8f00
	ARMv7_OP_4(BIC_REG, T2),  // 2 0xffe0, 0x8000
	ARMv7_NULL_OP,            // 3
	{ group_0xea4 },          // 4
	ARMv7_NULL_OP,            // 5
	{ group_0xea6 },          // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(EOR_REG, T2),  // 8 0xffe0, 0x8000
	ARMv7_OP_4(TEQ_REG, T1),  // 9 0xfff0, 0x8f00
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_OP_4(PKH, T1)       // C 0xfff0, 0x8010
};

static void group_0xea(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00e0) >> 4;

	if ((thr->m_arg & 0xfff08f00) == 0xea100f00) index = 0x1;
	if ((thr->m_arg & 0xfff08f00) == 0xea900f00) index = 0x9;
	if ((thr->m_arg & 0xfff08010) == 0xeac00000) index = 0xc;

	thr->m_last_instr_name = g_table_0xea[index].name;
	thr->m_last_instr_size = g_table_0xea[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xea[index].func(thr, g_table_0xea[index].type);
}

static const ARMv7_Instruction g_table_0xeb0[] =
{
	ARMv7_OP_4(ADD_REG, T3), // 0 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(ADD_SPR, T3)  // D 0xffef, 0x8000
};

static void group_0xeb0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xffe08000) == 0xeb000000) index = 0x0;

	thr->m_last_instr_name = g_table_0xeb0[index].name;
	thr->m_last_instr_size = g_table_0xeb0[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xeb0[index].func(thr, g_table_0xeb0[index].type);
}

static const ARMv7_Instruction g_table_0xeba[] =
{
	ARMv7_OP_4(SUB_REG, T2), // 0 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(SUB_SPR, T1)  // D 0xffef, 0x8000
};

static void group_0xeba(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xffe08000) == 0xeba00000) index = 0x0;

	thr->m_last_instr_name = g_table_0xeba[index].name;
	thr->m_last_instr_size = g_table_0xeba[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xeba[index].func(thr, g_table_0xeba[index].type);
}

static const ARMv7_Instruction g_table_0xeb[] =
{
	{ group_0xeb0 },         // 0 0xffe0
	ARMv7_OP_4(CMN_REG, T2), // 1 0xfff0, 0x8f00
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_4(ADC_REG, T2), // 4 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 5
	ARMv7_OP_4(SBC_REG, T2), // 6 0xffe0, 0x8000
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	{ group_0xeba },         // A 0xffe0
	ARMv7_OP_4(CMP_REG, T3), // B 0xfff0, 0x8f00
	ARMv7_OP_4(RSB_REG, T1)  // C 0xffe0, 0x8000
};

static void group_0xeb(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00e0) >> 4;

	if ((thr->m_arg & 0xfff08f00) == 0xeb100f00) index = 0x1;
	if ((thr->m_arg & 0xfff08f00) == 0xebb00f00) index = 0xb;

	thr->m_last_instr_name = g_table_0xeb[index].name;
	thr->m_last_instr_size = g_table_0xeb[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xeb[index].func(thr, g_table_0xeb[index].type);
}

static const ARMv7_Instruction g_table_0xe_main[] =
{
	ARMv7_OP_2(B, T2),        // 0 0xf800
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	{ group_0xe8 },           // 8
	{ group_0xe9 },           // 9
	{ group_0xea },           // A
	{ group_0xeb },           // B
	ARMv7_NULL_OP,            // C
	ARMv7_NULL_OP,            // D
	ARMv7_NULL_OP,            // E
	ARMv7_NULL_OP,            // F
};

static const ARMv7_Instruction g_table_0xe[] =
{
	{ group_0xe }
};

static void group_0xe(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0f00) >> 8;

	if ((thr->code.code0 & 0xf800) == 0xe000) index = 0x0;

	thr->m_last_instr_name = g_table_0xe_main[index].name;
	thr->m_last_instr_size = g_table_0xe_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xe_main[index].func(thr, g_table_0xe_main[index].type);
}

// 0xf...
static void group_0xf(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf000(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf04(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf06(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf1(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf1a(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf10(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf20(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf2a(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf2(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf36(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf3(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf810(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf800(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf81(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf820(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf840(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf84(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf850(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf85(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf910(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf91(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf930(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf93(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xf9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xfa00(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xfa90(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
static void group_0xfa(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

static const ARMv7_Instruction g_table_0xf000[] =
{
	ARMv7_OP_4(AND_IMM, T1), // 0 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_4(B, T3),       // 8 0xf800, 0xd000
	ARMv7_OP_4(B, T4),       // 9 0xf800, 0xd000
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_OP_4(BLX, T2),     // C 0xf800, 0xc001
	ARMv7_OP_4(BL, T1)       // D 0xf800, 0xd000
};

static void group_0xf000(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0xd000) >> 12;

	if ((thr->code.code1 & 0x8000) == 0x0000) index = 0x0;
	if ((thr->code.code1 & 0xc001) == 0xc000) index = 0xc;

	thr->m_last_instr_size = g_table_0xf000[index].size;
	thr->m_last_instr_name = g_table_0xf000[index].name;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf000[index].func(thr, g_table_0xf000[index].type);
}

static const ARMv7_Instruction g_table_0xf04[] =
{
	ARMv7_OP_4(ORR_IMM, T1), // 0 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(MOV_IMM, T2)  // F 0xfbef, 0x8000
};

static void group_0xf04(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbe08000) == 0xf0400000) index = 0x0;

	thr->m_last_instr_size = g_table_0xf04[index].size;
	thr->m_last_instr_name = g_table_0xf04[index].name;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf04[index].func(thr, g_table_0xf04[index].type);
}

static const ARMv7_Instruction g_table_0xf06[] =
{
	ARMv7_OP_4(ORN_IMM, T1), // 0 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(MVN_IMM, T1)  // F 0xfbef, 0x8000
};

static void group_0xf06(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbe08000) == 0xf0600000) index = 0x0;

	thr->m_last_instr_size = g_table_0xf06[index].size;
	thr->m_last_instr_name = g_table_0xf06[index].name;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf06[index].func(thr, g_table_0xf06[index].type);
}

static const ARMv7_Instruction g_table_0xf0[] =
{
	/*
	{ group_0xf000 },        // 0 0xfbe0
	ARMv7_OP_4(TST_IMM, T1), // 1 0xfbf0, 0x8f00
	ARMv7_OP_4(BIC_IMM, T1), // 2 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 3
	{ group_0xf04 },         // 4 0xfbef
	ARMv7_NULL_OP,           // 5
	{ group_0xf06 },         // 6 0xfbef
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_4(EOR_IMM, T1), // 8 0xfbe0, 0x8000
	ARMv7_OP_4(TEQ_IMM, T1)  // 9 0xfbf0, 0x8f00
	*/

	ARMv7_OP_4(AND_IMM, T1), // f0 000  // 0
	ARMv7_OP_4(B, T3),       // f0 008  // 1
	ARMv7_OP_4(B, T4),       // f0 009  // 2
	ARMv7_OP_4(BLX, T2),     // f0 00C  // 3
	ARMv7_OP_4(BL, T1),      // f0 00D  // 4

	ARMv7_OP_4(TST_IMM, T1), // f0 1    // 5
	ARMv7_OP_4(BIC_IMM, T1), // f0 2    // 6
	ARMv7_NULL_OP,           // f0 3    // 7


	ARMv7_OP_4(ORR_IMM, T1), // f0 40   // 8
	ARMv7_OP_4(MOV_IMM, T2), // f0 4F   // 9

	ARMv7_NULL_OP,           // f0 5    // A

	ARMv7_OP_4(ORN_IMM, T1), // f0 60   // B
	ARMv7_OP_4(MVN_IMM, T1), // f0 6F   // C

	ARMv7_NULL_OP,           // f0 7    // D
	ARMv7_OP_4(EOR_IMM, T1), // f0 8    // E
	ARMv7_OP_4(TEQ_IMM, T1)  // f0 9    // F

};

static void group_0xf0(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type) // TODO: optimize this group
{
	u32 index = 0;
	if ((thr->m_arg & 0xfbe08000) == 0xf0000000) index = 0x0;
	if ((thr->m_arg & 0xf800d000) == 0xf0008000) index = 0x1;
	if ((thr->m_arg & 0xf800d000) == 0xf0009000) index = 0x2;
	if ((thr->m_arg & 0xf800c001) == 0xf000c000) index = 0x3;
	if ((thr->m_arg & 0xf800d000) == 0xf000d000) index = 0x4;
	if ((thr->m_arg & 0xfbf08f00) == 0xf0100f00) index = 0x5;
	if ((thr->m_arg & 0xfbe08000) == 0xf0200000) index = 0x6;
	if ((thr->m_arg & 0xfbe08000) == 0xf0400000) index = 0x8;
	if ((thr->m_arg & 0xfbef8000) == 0xf04f0000) index = 0x9;
	if ((thr->m_arg & 0xfbe08000) == 0xf0600000) index = 0xb;
	if ((thr->m_arg & 0xfbef8000) == 0xf06f0000) index = 0xc;
	if ((thr->m_arg & 0xfbe08000) == 0xf0800000) index = 0xe;
	if ((thr->m_arg & 0xfbf08f00) == 0xf0900f00) index = 0xf;

	/*
	u32 index = (thr->code.code0 & 0x00e0) >> 4; // 0xfbef

	if ((thr->code.code0 & 0xfbf0) == 0xf010) index = 0x1;
	if ((thr->code.code0 & 0xfbf0) == 0xf090) index = 0x9;
	*/

	thr->m_last_instr_size = g_table_0xf0[index].size;
	thr->m_last_instr_name = g_table_0xf0[index].name;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf0[index].func(thr, g_table_0xf0[index].type);
}

static const ARMv7_Instruction g_table_0xf10[] =
{
	ARMv7_OP_4(ADD_IMM, T3), // 0 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(ADD_SPI, T3)  // D 0xfbef, 0x8000
};

static void group_0xf10(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbe08000) == 0xf1000000) index = 0x0;

	thr->m_last_instr_name = g_table_0xf10[index].name;
	thr->m_last_instr_size = g_table_0xf10[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf10[index].func(thr, g_table_0xf10[index].type);
}

static const ARMv7_Instruction g_table_0xf1a[] =
{
	ARMv7_OP_4(SUB_IMM, T3), // 0 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(SUB_SPI, T2)  // D 0xfbef, 0x8000
};

static void group_0xf1a(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbe08000) == 0xf1a00000) index = 0x0;

	thr->m_last_instr_name = g_table_0xf1a[index].name;
	thr->m_last_instr_size = g_table_0xf1a[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf1a[index].func(thr, g_table_0xf1a[index].type);
}

static const ARMv7_Instruction g_table_0xf1[] =
{
	{ group_0xf10 },         // 0
	ARMv7_OP_4(CMN_IMM, T1), // 1 0xfbf0, 0x8f00
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_4(ADC_IMM, T1), // 4 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 5
	ARMv7_OP_4(SBC_IMM, T1), // 6 0xfbe0, 0x8000
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	{ group_0xf1a },         // A
	ARMv7_OP_4(CMP_IMM, T2), // B 0xfbf0, 0x8f00
	ARMv7_OP_4(RSB_IMM, T2)  // C 0xfbe0, 0x8000
};

static void group_0xf1(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00e0) >> 4;

	if ((thr->m_arg & 0xfbf08f00) == 0xf1100f00) index = 0x1;
	if ((thr->m_arg & 0xfbf08f00) == 0xf1b00f00) index = 0xb;

	thr->m_last_instr_name = g_table_0xf1[index].name;
	thr->m_last_instr_size = g_table_0xf1[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf1[index].func(thr, g_table_0xf1[index].type);
}

static const ARMv7_Instruction g_table_0xf20[] =
{
	ARMv7_OP_4(ADD_IMM, T4), // 0 0xfbf0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(ADD_SPI, T4), // D 0xfbff, 0x8000
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(ADR, T3)      // F 0xfbff, 0x8000
};

static void group_0xf20(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbf08000) == 0xf2000000) index = 0x0;

	thr->m_last_instr_name = g_table_0xf20[index].name;
	thr->m_last_instr_size = g_table_0xf20[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf20[index].func(thr, g_table_0xf20[index].type);
}

static const ARMv7_Instruction g_table_0xf2a[] =
{
	ARMv7_OP_4(SUB_IMM, T4), // 0 0xfbf0, 0x8000
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_OP_4(SUB_SPI, T3), // D 0xfbff, 0x8000
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(ADR, T2)      // F 0xfbff, 0x8000
};

static void group_0xf2a(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfbf08000) == 0xf2a00000) index = 0x0;

	thr->m_last_instr_name = g_table_0xf2a[index].name;
	thr->m_last_instr_size = g_table_0xf2a[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf2a[index].func(thr, g_table_0xf2a[index].type);
}

static const ARMv7_Instruction g_table_0xf2[] =
{
	{ group_0xf20 },         // 0 0xfbff
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_4(MOV_IMM, T3), // 4 0xfbf0, 0x8000
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	{ group_0xf2a },         // A 0xfbff
	ARMv7_NULL_OP,           // B
	ARMv7_OP_4(MOVT, T1)     // C 0xfbf0, 0x8000
};

static void group_0xf2(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00f0) >> 4; // mask 0xfbf0
	thr->m_last_instr_name = g_table_0xf2[index].name;
	thr->m_last_instr_size = g_table_0xf2[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf2[index].func(thr, g_table_0xf2[index].type);
}

static const ARMv7_Instruction g_table_0xf36[] =
{
	ARMv7_OP_4(BFI, T1),     // 0 0xfff0, 0x8020
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(BFC, T1)      // F 0xffff, 0x8020
};

static void group_0xf36(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if ((thr->m_arg & 0xfff08020) == 0xf3600000) index = 0x0;

	thr->m_last_instr_name = g_table_0xf36[index].name;
	thr->m_last_instr_size = g_table_0xf36[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf36[index].func(thr, g_table_0xf36[index].type);
}

static const ARMv7_Instruction g_table_0xf3[] =
{
	ARMv7_NULL_OP,           // 0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_OP_4(SBFX, T1),    // 4 0xfff0, 0x8020
	ARMv7_NULL_OP,           // 5
	{ group_0xf36 },         // 6 0xffff
	ARMv7_NULL_OP,           // 7
	ARMv7_OP_4(MSR_REG, T1), // 8 0xfff0, 0xf3ff
	ARMv7_NULL_OP,           // 9
	ARMv7_OP_4(NOP, T2),     // A 0xffff, 0xffff
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // D
	ARMv7_OP_4(MRS, T1),     // E 0xffff, 0xf0ff
};

static void group_0xf3(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00f0) >> 4;
	thr->m_last_instr_name = g_table_0xf3[index].name;
	thr->m_last_instr_size = g_table_0xf3[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf3[index].func(thr, g_table_0xf3[index].type);
}

static const ARMv7_Instruction g_table_0xf800[] =
{
	ARMv7_OP_4(STRB_REG, T2), // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(STRB_IMM, T3)  // 8 0xfff0, 0x0800
};

static void group_0xf800(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf800[index].name;
	thr->m_last_instr_size = g_table_0xf800[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf800[index].func(thr, g_table_0xf800[index].type);
}

static const ARMv7_Instruction g_table_0xf810[] =
{
	ARMv7_OP_4(LDRB_REG, T2), // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(LDRB_IMM, T3)  // 8 0xfff0, 0x0800
};

static void group_0xf810(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf810[index].name;
	thr->m_last_instr_size = g_table_0xf810[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf810[index].func(thr, g_table_0xf810[index].type);
}

static const ARMv7_Instruction g_table_0xf81[] =
{
	{ group_0xf810 },        // 0 0xfff0
	ARMv7_NULL_OP,           // 1
	ARMv7_NULL_OP,           // 2
	ARMv7_NULL_OP,           // 3
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	ARMv7_NULL_OP,           // 8
	ARMv7_NULL_OP,           // 9
	ARMv7_NULL_OP,           // A
	ARMv7_NULL_OP,           // B
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // C
	ARMv7_NULL_OP,           // E
	ARMv7_OP_4(LDRB_LIT, T1) // F 0xff7f, 0x0000
};

static void group_0xf81(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if (((thr->m_arg & 0xfff00fc0) == 0xf8100000) || ((thr->m_arg & 0xfff00800) == 0xf8100800)) index = 0x0;

	thr->m_last_instr_name = g_table_0xf81[index].name;
	thr->m_last_instr_size = g_table_0xf81[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf81[index].func(thr, g_table_0xf81[index].type);
}

static const ARMv7_Instruction g_table_0xf820[] =
{
	ARMv7_OP_4(STRH_REG, T2), // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(STRH_IMM, T3)  // 8 0xfff0, 0x0800
};

static void group_0xf820(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf820[index].name;
	thr->m_last_instr_size = g_table_0xf820[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf820[index].func(thr, g_table_0xf820[index].type);
}

static const ARMv7_Instruction g_table_0xf840[] =
{
	ARMv7_OP_4(STR_REG, T2),  // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(STR_IMM, T4)   // 8 0xfff0, 0x0800
};

static void group_0xf840(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf840[index].name;
	thr->m_last_instr_size = g_table_0xf840[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf840[index].func(thr, g_table_0xf840[index].type);
}

static const ARMv7_Instruction g_table_0xf84[] =
{
	{ group_0xf840 },         // 0 0xfff0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_NULL_OP,            // 8
	ARMv7_NULL_OP,            // 9
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(PUSH, T3)      // D 0xffff, 0x0fff
};

static void group_0xf84(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if (((thr->m_arg & 0xfff00fc0) == 0xf8400000) || ((thr->m_arg & 0xfff00800) == 0xf8400800)) index = 0x0;

	thr->m_last_instr_name = g_table_0xf84[index].name;
	thr->m_last_instr_size = g_table_0xf84[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf84[index].func(thr, g_table_0xf84[index].type);
}

static const ARMv7_Instruction g_table_0xf850[] =
{
	ARMv7_OP_4(LDR_REG, T2),  // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_OP_4(LDR_IMM, T4)   // 8 0xfff0, 0x0800
};

static void group_0xf850(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf850[index].name;
	thr->m_last_instr_size = g_table_0xf850[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf850[index].func(thr, g_table_0xf850[index].type);
}

static const ARMv7_Instruction g_table_0xf85[] =
{
	{ group_0xf850 },         // 0 0xfff0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_NULL_OP,            // 8
	ARMv7_NULL_OP,            // 9
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(POP, T3),      // D 0xffff, 0x0fff
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(LDR_LIT, T2)   // F 0xff7f, 0x0000
};

static void group_0xf85(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if (((thr->m_arg & 0xfff00fc0) == 0xf8500000) || ((thr->m_arg & 0xfff00800) == 0xf8500800)) index = 0x0;

	thr->m_last_instr_name = g_table_0xf85[index].name;
	thr->m_last_instr_size = g_table_0xf85[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf85[index].func(thr, g_table_0xf85[index].type);
}

static const ARMv7_Instruction g_table_0xf8[] =
{
	{ group_0xf800 },         // 0 0xfff0
	{ group_0xf81 },          // 1 0xfff0
	{ group_0xf820 },         // 2 0xfff0
	ARMv7_NULL_OP,            // 3
	{ group_0xf84 },          // 4 0xfff0
	{ group_0xf85 },          // 5 0xfff0
	ARMv7_NULL_OP,            // 6
	ARMv7_OP_4(HACK, T1),     // 7 0xffff, 0x0000
	ARMv7_OP_4(STRB_IMM, T2), // 8 0xfff0, 0x0000
	ARMv7_OP_4(LDRB_IMM, T2), // 9 0xfff0, 0x0000
	ARMv7_OP_4(STRH_IMM, T2), // A 0xfff0, 0x0000
	ARMv7_NULL_OP,            // B
	ARMv7_OP_4(STR_IMM, T3),  // C 0xfff0, 0x0000
	ARMv7_OP_4(LDR_IMM, T3)   // D 0xfff0, 0x0000
};

static void group_0xf8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code0 & 0x00f0) >> 4;
	thr->m_last_instr_name = g_table_0xf8[index].name;
	thr->m_last_instr_size = g_table_0xf8[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf8[index].func(thr, g_table_0xf8[index].type);
}

static const ARMv7_Instruction g_table_0xf910[] =
{
	ARMv7_OP_4(LDRSB_REG, T2), // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,             // 1
	ARMv7_NULL_OP,             // 2
	ARMv7_NULL_OP,             // 3
	ARMv7_NULL_OP,             // 4
	ARMv7_NULL_OP,             // 5
	ARMv7_NULL_OP,             // 6
	ARMv7_NULL_OP,             // 7
	ARMv7_OP_4(LDRSB_IMM, T2)  // 8 0xfff0, 0x0800
};

static void group_0xf910(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf910[index].name;
	thr->m_last_instr_size = g_table_0xf910[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf910[index].func(thr, g_table_0xf910[index].type);
}

static const ARMv7_Instruction g_table_0xf91[] =
{
	{ group_0xf910 },         // 0 0xfff0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_NULL_OP,            // 8
	ARMv7_NULL_OP,            // 9
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_NULL_OP,            // C
	ARMv7_NULL_OP,            // D
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(LDRSB_LIT, T1) // F 0xff7f, 0x0000
};

static void group_0xf91(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if (((thr->m_arg & 0xfff00fc0) == 0xf9100000) || ((thr->m_arg & 0xfff00800) == 0xf9100800)) index = 0x0;

	thr->m_last_instr_name = g_table_0xf91[index].name;
	thr->m_last_instr_size = g_table_0xf91[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf91[index].func(thr, g_table_0xf91[index].type);
}

static const ARMv7_Instruction g_table_0xf930[] =
{
	ARMv7_OP_4(LDRSH_REG, T2), // 0 0xfff0, 0x0fc0
	ARMv7_NULL_OP,             // 1
	ARMv7_NULL_OP,             // 2
	ARMv7_NULL_OP,             // 3
	ARMv7_NULL_OP,             // 4
	ARMv7_NULL_OP,             // 5
	ARMv7_NULL_OP,             // 6
	ARMv7_NULL_OP,             // 7
	ARMv7_OP_4(LDRSH_IMM, T2)  // 8 0xfff0, 0x0800
};

static void group_0xf930(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code1 & 0x0f00) >> 8;

	if ((thr->code.code1 & 0x0800) == 0x0800) index = 0x8;

	thr->m_last_instr_name = g_table_0xf930[index].name;
	thr->m_last_instr_size = g_table_0xf930[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf930[index].func(thr, g_table_0xf930[index].type);
}

static const ARMv7_Instruction g_table_0xf93[] =
{
	{ group_0xf930 },         // 0 0xfff0
	ARMv7_NULL_OP,            // 1
	ARMv7_NULL_OP,            // 2
	ARMv7_NULL_OP,            // 3
	ARMv7_NULL_OP,            // 4
	ARMv7_NULL_OP,            // 5
	ARMv7_NULL_OP,            // 6
	ARMv7_NULL_OP,            // 7
	ARMv7_NULL_OP,            // 8
	ARMv7_NULL_OP,            // 9
	ARMv7_NULL_OP,            // A
	ARMv7_NULL_OP,            // B
	ARMv7_NULL_OP,            // C
	ARMv7_NULL_OP,            // D
	ARMv7_NULL_OP,            // C
	ARMv7_OP_4(LDRSH_LIT, T1) // F 0xff7f, 0x0000
};

static void group_0xf93(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = thr->code.code0 & 0x000f;

	if (((thr->m_arg & 0xfff00fc0) == 0xf9300000) || ((thr->m_arg & 0xfff00800) == 0xf9300800)) index = 0x0;

	thr->m_last_instr_name = g_table_0xf93[index].name;
	thr->m_last_instr_size = g_table_0xf93[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf93[index].func(thr, g_table_0xf93[index].type);
}

static const ARMv7_Instruction g_table_0xf9[] =
{
	ARMv7_NULL_OP,             // 0
	{ group_0xf91 },           // 1 0xff7f
	ARMv7_NULL_OP,             // 2
	{ group_0xf93 },           // 3 0xff7f
	ARMv7_NULL_OP,             // 4
	ARMv7_NULL_OP,             // 5
	ARMv7_NULL_OP,             // 6
	ARMv7_NULL_OP,             // 7
	ARMv7_NULL_OP,             // 8
	ARMv7_OP_4(LDRSB_IMM, T1), // 9 0xfff0, 0x0000
	ARMv7_NULL_OP,             // A
	ARMv7_OP_4(LDRSH_IMM, T1), // B 0xfff0, 0x0000
};

static void group_0xf9(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00f0) >> 4;

	if ((thr->code.code0 & 0xff7) == 0xf91) index = 0x1; // check me
	if ((thr->code.code0 & 0xff7) == 0xf93) index = 0x3;

	thr->m_last_instr_name = g_table_0xf9[index].name;
	thr->m_last_instr_size = g_table_0xf9[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf9[index].func(thr, g_table_0xf9[index].type);
}

static const ARMv7_Instruction g_table_0xfa00[] =
{
	ARMv7_OP_4(BLX, A2),       // 0 0xfe00, 0x0000
	ARMv7_NULL_OP,             // 1
	ARMv7_NULL_OP,             // 2
	ARMv7_NULL_OP,             // 3
	ARMv7_NULL_OP,             // 4
	ARMv7_NULL_OP,             // 5
	ARMv7_NULL_OP,             // 6
	ARMv7_NULL_OP,             // 7
	ARMv7_NULL_OP,             // 8
	ARMv7_NULL_OP,             // 9
	ARMv7_NULL_OP,             // A
	ARMv7_NULL_OP,             // B
	ARMv7_NULL_OP,             // C
	ARMv7_NULL_OP,             // D
	ARMv7_NULL_OP,             // E
	ARMv7_OP_4(LSL_REG, T2)    // F 0xffe0, 0xf0f0
};

static void group_0xfa00(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code1 & 0xf0f0) == 0xf000 ? 0xf : 0x0;
	thr->m_last_instr_name = g_table_0xfa00[index].name;
	thr->m_last_instr_size = g_table_0xfa00[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xfa00[index].func(thr, g_table_0xfa00[index].type);
}

static const ARMv7_Instruction g_table_0xfa90[] =
{
	ARMv7_NULL_OP,             // 0
	ARMv7_NULL_OP,             // 1
	ARMv7_NULL_OP,             // 2
	ARMv7_NULL_OP,             // 3
	ARMv7_NULL_OP,             // 4
	ARMv7_NULL_OP,             // 5
	ARMv7_NULL_OP,             // 6
	ARMv7_NULL_OP,             // 7
	ARMv7_OP_4(REV, T2),       // 8 0xfff0, 0xf0f0
	ARMv7_OP_4(REV16, T2),     // 9 0xfff0, 0xf0f0
	ARMv7_OP_4(RBIT, T1),      // A 0xfff0, 0xf0f0
	ARMv7_OP_4(REVSH, T2)      // B 0xfff0, 0xf0f0
};

static void group_0xfa90(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	const u32 index = (thr->code.code1 & 0x00f0) >> 4;
	thr->m_last_instr_name = g_table_0xfa90[index].name;
	thr->m_last_instr_size = g_table_0xfa90[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xfa90[index].func(thr, g_table_0xfa90[index].type);
}

static const ARMv7_Instruction g_table_0xfa[] =
{
	{ group_0xfa00 },          // 0 0xffe0
	ARMv7_NULL_OP,             // 1
	ARMv7_OP_4(LSR_REG, T2),   // 2 0xffe0, 0xf0f0
	ARMv7_NULL_OP,             // 3
	ARMv7_OP_4(ASR_REG, T2),   // 4 0xffe0, 0xf0f0
	ARMv7_NULL_OP,             // 5
	ARMv7_OP_4(ROR_REG, T2),   // 6 0xffe0, 0xf0f0
	ARMv7_NULL_OP,             // 7
	ARMv7_NULL_OP,             // 8
	{ group_0xfa90 },          // 9 0xfff0
	ARMv7_OP_4(SEL, T1),       // A 0xfff0, 0xf0f0
	ARMv7_OP_4(CLZ, T1)        // B 0xfff0, 0xf0f0
};

static void group_0xfa(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x00e0) >> 4;

	switch ((thr->code.code0 & 0x00f0) >> 4)
	{
	case 0x9: index = 0x9; break;
	case 0xa: index = 0xa; break;
	case 0xb: index = 0xb; break;

	default: break;
	}

	thr->m_last_instr_name = g_table_0xfa[index].name;
	thr->m_last_instr_size = g_table_0xfa[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xfa[index].func(thr, g_table_0xfa[index].type);
}

static const ARMv7_Instruction g_table_0xf_main[] =
{
	{ group_0xf0 },          // 0 0xfbef
	{ group_0xf1 },          // 1 0xfbef
	{ group_0xf2 },          // 2 0xfbff
	{ group_0xf3 },          // 3 0xffff
	ARMv7_NULL_OP,           // 4
	ARMv7_NULL_OP,           // 5
	ARMv7_NULL_OP,           // 6
	ARMv7_NULL_OP,           // 7
	{ group_0xf8 },          // 8 0xfff0
	{ group_0xf9 },          // 9 0xfff0
	{ group_0xfa },          // A 0xfff0

};

static void group_0xf(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type)
{
	u32 index = (thr->code.code0 & 0x0b00) >> 8;

	switch ((thr->m_arg & 0x0800d000) >> 12)
	{
	case 0x8: // B, T3
	case 0x9: // B, T4
	case 0xd: index = 0x0; break; // BL, T1

	default: break;
	}

	if ((thr->m_arg & 0xf800c001) == 0xf000c000) index = 0x0; // BLX, T2

	switch ((thr->code.code0 & 0x0f00) >> 8)
	{
	case 0x3: index = 0x3; break;
	case 0x8: index = 0x8; break;
	case 0x9: index = 0x9; break;
	case 0xa: index = 0xa; break;

	default: break;
	}

	thr->m_last_instr_name = g_table_0xf_main[index].name;
	thr->m_last_instr_size = g_table_0xf_main[index].size;
	thr->code.data = thr->m_last_instr_size == 2 ? thr->code.code0 : thr->m_arg;
	g_table_0xf_main[index].func(thr, g_table_0xf_main[index].type);
}

static const ARMv7_Instruction g_table_0xf[] =
{
	{ group_0xf }
};


static void execute_main_group(ARMv7Thread* thr)
{
	switch ((thr->code.code0 & 0xf000) >> 12)
	{
	//case 0x0: (*g_table_0x0).func(thr, (*g_table_0x0).type); break; // TODO
	case 0x1: (*g_table_0x1).func(thr, (*g_table_0x1).type); break;
	case 0x2: (*g_table_0x2).func(thr, (*g_table_0x2).type); break;
	case 0x3: (*g_table_0x3).func(thr, (*g_table_0x3).type); break;
	case 0x4: (*g_table_0x4).func(thr, (*g_table_0x4).type); break;
	case 0x5: (*g_table_0x5).func(thr, (*g_table_0x5).type); break;
	case 0x6: (*g_table_0x6).func(thr, (*g_table_0x6).type); break;
	case 0x7: (*g_table_0x7).func(thr, (*g_table_0x7).type); break;
	case 0x8: (*g_table_0x8).func(thr, (*g_table_0x8).type); break;
	case 0x9: (*g_table_0x9).func(thr, (*g_table_0x9).type); break;
	case 0xa: (*g_table_0xa).func(thr, (*g_table_0xa).type); break;
	case 0xb: (*g_table_0xb).func(thr, (*g_table_0xb).type); break;
	case 0xc: (*g_table_0xc).func(thr, (*g_table_0xc).type); break;
	case 0xd: (*g_table_0xd).func(thr, (*g_table_0xd).type); break;
	case 0xe: (*g_table_0xe).func(thr, (*g_table_0xe).type); break;
	case 0xf: (*g_table_0xf).func(thr, (*g_table_0xf).type); break;

	default: LOG_ERROR(GENERAL, "ARMv7Decoder: unknown group 0x%x", (thr->code.code0 & 0xf000) >> 12); Emu.Pause(); break;
	}
}

#undef ARMv7_OP_2
#undef ARMv7_OP_4
#undef ARMv7_NULL_OP
#endif
