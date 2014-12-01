#pragma once

#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/System.h"
#include "Utilities/Log.h"

static enum ARMv7_encoding
{
	T1, T2, T3, T4, A1, A2
};

static enum SRType : u32
{
	SRType_None,
	SRType_LSL,
	SRType_LSR,
	SRType_ASR,
	SRType_ROR,
	SRType_RRX
};

namespace ARMv7_instrs
{
	template<typename T>
	static bool IsZero(T x)
	{
		return x == T(0);
	}

	template<typename T>
	static bool IsOnes(T x)
	{
		return x == ~T(0);
	}

	template<typename T>
	static u8 IsZeroBit(T x)
	{
		return IsZero(x) ? 1 : 0;
	}

	template<typename T>
	static bool IsOnesBit(T x, u8 len = sizeof(T) * 8)
	{
		return IsOnes(x) ? 1 : 0;
	}

	template<typename T>
	u8 BitCount(T x, u8 len = sizeof(T) * 8);

	template<typename T>
	s8 LowestSetBit(T x, u8 len = sizeof(T) * 8);

	template<typename T>
	s8 HighestSetBit(T x, u8 len = sizeof(T) * 8);

	template<typename T>
	s8 CountLeadingZeroBits(T x, u8 len = sizeof(T) * 8);

	SRType DecodeImmShift(u32 type, u32 imm5, u32* shift_n);
	SRType DecodeRegShift(u8 type);

	u32 LSL_C(u32 x, s32 shift, bool& carry_out);
	u32 LSL(u32 x, s32 shift);
	u32 LSR_C(u32 x, s32 shift, bool& carry_out);
	u32 LSR(u32 x, s32 shift);

	s32 ASR_C(s32 x, s32 shift, bool& carry_out);
	s32 ASR(s32 x, s32 shift);

	u32 ROR_C(u32 x, s32 shift, bool& carry_out);
	u32 ROR(u32 x, s32 shift);

	u32 RRX_C(u32 x, bool carry_in, bool& carry_out);
	u32 RRX(u32 x, bool carry_in);

	template<typename T> T Shift_C(T value, SRType type, s32 amount, bool carry_in, bool& carry_out);

	template<typename T> T Shift(T value, SRType type, s32 amount, bool carry_in);

	template<typename T> T AddWithCarry(T x, T y, bool carry_in, bool& carry_out, bool& overflow);

	u32 ThumbExpandImm_C(u32 imm12, bool carry_in, bool& carry_out);
	u32 ThumbExpandImm(ARMv7Thread* CPU, u32 imm12);

	bool ConditionPassed(ARMv7Thread* CPU, u32 cond);

	// instructions
	void UNK(ARMv7Thread* thr);

	void NULL_OP(ARMv7Thread* thr, const ARMv7_encoding type);

	void HACK(ARMv7Thread* thr, const ARMv7_encoding type);

	void ADC_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADC_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADC_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void ADD_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADD_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADD_RSR(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADD_SPI(ARMv7Thread* thr, const ARMv7_encoding type);
	void ADD_SPR(ARMv7Thread* thr, const ARMv7_encoding type);

	void ADR(ARMv7Thread* thr, const ARMv7_encoding type);

	void AND_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void AND_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void AND_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void ASR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ASR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void B(ARMv7Thread* thr, const ARMv7_encoding type);

	void BFC(ARMv7Thread* thr, const ARMv7_encoding type);
	void BFI(ARMv7Thread* thr, const ARMv7_encoding type);

	void BIC_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void BIC_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void BIC_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void BKPT(ARMv7Thread* thr, const ARMv7_encoding type);

	void BL(ARMv7Thread* thr, const ARMv7_encoding type);
	void BLX(ARMv7Thread* thr, const ARMv7_encoding type);
	void BX(ARMv7Thread* thr, const ARMv7_encoding type);

	void CB_Z(ARMv7Thread* thr, const ARMv7_encoding type);

	void CLZ(ARMv7Thread* thr, const ARMv7_encoding type);

	void CMN_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void CMN_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void CMN_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void CMP_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void CMP_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void CMP_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void EOR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void EOR_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void EOR_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void IT(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDMDA(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDMDB(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDMIB(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDR_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDRB_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRB_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRB_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDRD_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRD_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRD_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDRH_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRH_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRH_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDRSB_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRSB_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRSB_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LDRSH_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRSH_LIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void LDRSH_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LSL_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LSL_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void LSR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void LSR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void MLA(ARMv7Thread* thr, const ARMv7_encoding type);
	void MLS(ARMv7Thread* thr, const ARMv7_encoding type);

	void MOV_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void MOV_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void MOVT(ARMv7Thread* thr, const ARMv7_encoding type);

	void MRS(ARMv7Thread* thr, const ARMv7_encoding type);
	void MSR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void MSR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void MUL(ARMv7Thread* thr, const ARMv7_encoding type);

	void MVN_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void MVN_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void MVN_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void NOP(ARMv7Thread* thr, const ARMv7_encoding type);

	void ORN_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ORN_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void ORR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ORR_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void ORR_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void PKH(ARMv7Thread* thr, const ARMv7_encoding type);

	void POP(ARMv7Thread* thr, const ARMv7_encoding type);
	void PUSH(ARMv7Thread* thr, const ARMv7_encoding type);

	void QADD(ARMv7Thread* thr, const ARMv7_encoding type);
	void QADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void QADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void QASX(ARMv7Thread* thr, const ARMv7_encoding type);
	void QDADD(ARMv7Thread* thr, const ARMv7_encoding type);
	void QDSUB(ARMv7Thread* thr, const ARMv7_encoding type);
	void QSAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void QSUB(ARMv7Thread* thr, const ARMv7_encoding type);
	void QSUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void QSUB8(ARMv7Thread* thr, const ARMv7_encoding type);

	void RBIT(ARMv7Thread* thr, const ARMv7_encoding type);
	void REV(ARMv7Thread* thr, const ARMv7_encoding type);
	void REV16(ARMv7Thread* thr, const ARMv7_encoding type);
	void REVSH(ARMv7Thread* thr, const ARMv7_encoding type);

	void ROR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void ROR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void RRX(ARMv7Thread* thr, const ARMv7_encoding type);

	void RSB_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void RSB_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void RSB_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void RSC_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void RSC_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void RSC_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void SADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void SASX(ARMv7Thread* thr, const ARMv7_encoding type);

	void SBC_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void SBC_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void SBC_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void SBFX(ARMv7Thread* thr, const ARMv7_encoding type);

	void SDIV(ARMv7Thread* thr, const ARMv7_encoding type);

	void SEL(ARMv7Thread* thr, const ARMv7_encoding type);

	void SHADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SHADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void SHASX(ARMv7Thread* thr, const ARMv7_encoding type);
	void SHSAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void SHSUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SHSUB8(ARMv7Thread* thr, const ARMv7_encoding type);

	void SMLA__(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLAD(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLAL(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLAL__(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLALD(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLAW_(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLSD(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMLSLD(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMMLA(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMMLS(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMMUL(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMUAD(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMUL__(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMULL(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMULW_(ARMv7Thread* thr, const ARMv7_encoding type);
	void SMUSD(ARMv7Thread* thr, const ARMv7_encoding type);

	void SSAT(ARMv7Thread* thr, const ARMv7_encoding type);
	void SSAT16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SSAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void SSUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SSUB8(ARMv7Thread* thr, const ARMv7_encoding type);

	void STM(ARMv7Thread* thr, const ARMv7_encoding type);
	void STMDA(ARMv7Thread* thr, const ARMv7_encoding type);
	void STMDB(ARMv7Thread* thr, const ARMv7_encoding type);
	void STMIB(ARMv7Thread* thr, const ARMv7_encoding type);

	void STR_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void STR_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void STRB_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void STRB_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void STRD_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void STRD_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void STRH_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void STRH_REG(ARMv7Thread* thr, const ARMv7_encoding type);

	void SUB_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void SUB_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void SUB_RSR(ARMv7Thread* thr, const ARMv7_encoding type);
	void SUB_SPI(ARMv7Thread* thr, const ARMv7_encoding type);
	void SUB_SPR(ARMv7Thread* thr, const ARMv7_encoding type);

	void SVC(ARMv7Thread* thr, const ARMv7_encoding type);

	void SXTAB(ARMv7Thread* thr, const ARMv7_encoding type);
	void SXTAB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SXTAH(ARMv7Thread* thr, const ARMv7_encoding type);
	void SXTB(ARMv7Thread* thr, const ARMv7_encoding type);
	void SXTB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void SXTH(ARMv7Thread* thr, const ARMv7_encoding type);

	void TB_(ARMv7Thread* thr, const ARMv7_encoding type);

	void TEQ_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void TEQ_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void TEQ_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void TST_IMM(ARMv7Thread* thr, const ARMv7_encoding type);
	void TST_REG(ARMv7Thread* thr, const ARMv7_encoding type);
	void TST_RSR(ARMv7Thread* thr, const ARMv7_encoding type);

	void UADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void UASX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UBFX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UDIV(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHASX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHSAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHSUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UHSUB8(ARMv7Thread* thr, const ARMv7_encoding type);
	void UMAAL(ARMv7Thread* thr, const ARMv7_encoding type);
	void UMLAL(ARMv7Thread* thr, const ARMv7_encoding type);
	void UMULL(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQADD16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQADD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQASX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQSAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQSUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UQSUB8(ARMv7Thread* thr, const ARMv7_encoding type);
	void USAD8(ARMv7Thread* thr, const ARMv7_encoding type);
	void USADA8(ARMv7Thread* thr, const ARMv7_encoding type);
	void USAT(ARMv7Thread* thr, const ARMv7_encoding type);
	void USAT16(ARMv7Thread* thr, const ARMv7_encoding type);
	void USAX(ARMv7Thread* thr, const ARMv7_encoding type);
	void USUB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void USUB8(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTAB(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTAB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTAH(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTB(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTB16(ARMv7Thread* thr, const ARMv7_encoding type);
	void UXTH(ARMv7Thread* thr, const ARMv7_encoding type);
};


// old instructions table for debugging and information, delete this later
using namespace ARMv7_instrs;

struct ARMv7_opcode_t
{
	u32 mask;
	u32 code;
	u32 length; // 2 or 4
	const char* name;
	ARMv7_encoding type;
	void(*func)(ARMv7Thread* thr, const ARMv7_encoding type);
};

// single 16-bit value
#define ARMv7_OP2(mask, code, type, name) { (u32)((mask) << 16), (u32)((code) << 16), 2, #name "_" #type, type, name }
// two 16-bit values
#define ARMv7_OP4(mask0, mask1, code0, code1, type, name) { (u32)((mask0) << 16) | (mask1), (u32)((code0) << 16) | (code1), 4, #name "_" #type, type, name }

static const ARMv7_opcode_t ARMv7_opcode_table[] =
{
	ARMv7_OP2(0xffff, 0x0000, T1, NULL_OP), // ???

	ARMv7_OP4(0xffff, 0x0000, 0xf870, 0x0000, T1, HACK), // "Undefined" Thumb opcode
	ARMv7_OP4(0x0ff0, 0x00f0, 0x0070, 0x0090, A1, HACK), // "Undefined" ARM opcode

	ARMv7_OP4(0xfbe0, 0x8000, 0xf140, 0x0000, T1, ADC_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x02a0, 0x0000, A1, ADC_IMM),
	ARMv7_OP2(0xffc0, 0x4040, T1, ADC_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeb40, 0x0000, T2, ADC_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x00a0, 0x0000, A1, ADC_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x00a0, 0x0010, A1, ADC_RSR),

	ARMv7_OP2(0xf800, 0xa800, T1, ADD_SPI),
	ARMv7_OP2(0xff80, 0xb000, T2, ADD_SPI),
	ARMv7_OP4(0xfbef, 0x8000, 0xf10d, 0x0000, T3, ADD_SPI),
	ARMv7_OP4(0xfbff, 0x8000, 0xf20d, 0x0000, T4, ADD_SPI),
	ARMv7_OP4(0x0fef, 0x0000, 0x028d, 0x0000, A1, ADD_SPI),
	ARMv7_OP2(0xff78, 0x4468, T1, ADD_SPR),
	ARMv7_OP2(0xff87, 0x4485, T2, ADD_SPR),
	ARMv7_OP4(0xffef, 0x8000, 0xeb0d, 0x0000, T3, ADD_SPR),
	ARMv7_OP4(0x0fef, 0x0010, 0x008d, 0x0000, A1, ADD_SPR),
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

	ARMv7_OP2(0xf800, 0xa000, T1, ADR),
	ARMv7_OP4(0xfbff, 0x8000, 0xf2af, 0x0000, T2, ADR),
	ARMv7_OP4(0xfbff, 0x8000, 0xf20f, 0x0000, T3, ADR),
	ARMv7_OP4(0x0fff, 0x0000, 0x028f, 0x0000, A1, ADR),
	ARMv7_OP4(0x0fff, 0x0000, 0x024f, 0x0000, A2, ADR),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf000, 0x0000, T1, AND_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0200, 0x0000, A1, AND_IMM),
	ARMv7_OP2(0xffc0, 0x4000, T1, AND_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xea00, 0x0000, T2, AND_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0000, 0x0000, A1, AND_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0000, 0x0010, A1, AND_RSR),

	ARMv7_OP2(0xf800, 0x1000, T1, ASR_IMM),
	ARMv7_OP4(0xffef, 0x8030, 0xea4f, 0x0020, T2, ASR_IMM),
	ARMv7_OP4(0x0fef, 0x0070, 0x01a0, 0x0040, A1, ASR_IMM),
	ARMv7_OP2(0xffc0, 0x4100, T1, ASR_REG),
	ARMv7_OP4(0xffe0, 0xf0f0, 0xfa40, 0xf000, T2, ASR_REG),
	ARMv7_OP4(0x0fef, 0x00f0, 0x01a0, 0x0050, A1, ASR_REG),

	ARMv7_OP2(0xf000, 0xd000, T1, B),
	ARMv7_OP2(0xf800, 0xe000, T2, B),
	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0x8000, T3, B),
	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0x9000, T4, B),
	ARMv7_OP4(0x0f00, 0x0000, 0x0a00, 0x0000, A1, B),

	ARMv7_OP4(0xffff, 0x8020, 0xf36f, 0x0000, T1, BFC),
	ARMv7_OP4(0x0fe0, 0x007f, 0x07c0, 0x001f, A1, BFC),
	ARMv7_OP4(0xfff0, 0x8020, 0xf360, 0x0000, T1, BFI),
	ARMv7_OP4(0x0fe0, 0x0070, 0x07c0, 0x0010, A1, BFI),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf020, 0x0000, T1, BIC_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x03c0, 0x0000, A1, BIC_IMM),
	ARMv7_OP2(0xffc0, 0x4380, T1, BIC_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xea20, 0x0000, T2, BIC_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x01c0, 0x0000, A1, BIC_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x01c0, 0x0010, A1, BIC_RSR),

	ARMv7_OP2(0xff00, 0xbe00, T1, BKPT),
	ARMv7_OP4(0x0ff0, 0x00f0, 0x0120, 0x0070, A1, BKPT),

	ARMv7_OP4(0xf800, 0xd000, 0xf000, 0xd000, T1, BL),
	ARMv7_OP4(0x0f00, 0x0000, 0x0b00, 0x0000, A1, BL),
	ARMv7_OP2(0xff80, 0x4780, T1, BLX),
	ARMv7_OP4(0xf800, 0xc001, 0xf000, 0xc000, T2, BLX),
	ARMv7_OP4(0x0fff, 0xfff0, 0x012f, 0xff30, A1, BLX),
	ARMv7_OP4(0xfe00, 0x0000, 0xfa00, 0x0000, A2, BLX),

	ARMv7_OP2(0xff87, 0x4700, T1, BX),
	ARMv7_OP4(0x0fff, 0xfff0, 0x012f, 0xff10, A1, BX),

	ARMv7_OP2(0xf500, 0xb100, T1, CB_Z),

	ARMv7_OP4(0xfff0, 0xf0f0, 0xfab0, 0xf080, T1, CLZ),
	ARMv7_OP4(0x0fff, 0x0ff0, 0x016f, 0x0f10, A1, CLZ),

	ARMv7_OP4(0xfbf0, 0x8f00, 0xf110, 0x0f00, T1, CMN_IMM),
	ARMv7_OP4(0x0ff0, 0xf000, 0x0370, 0x0000, A1, CMN_IMM),
	ARMv7_OP2(0xffc0, 0x42c0, T1, CMN_REG),
	ARMv7_OP4(0xfff0, 0x8f00, 0xeb10, 0x0f00, T2, CMN_REG),
	ARMv7_OP4(0x0ff0, 0xf010, 0x0170, 0x0000, A1, CMN_REG),
	ARMv7_OP4(0x0ff0, 0xf090, 0x0170, 0x0010, A1, CMN_RSR),

	ARMv7_OP2(0xf800, 0x2800, T1, CMP_IMM),
	ARMv7_OP4(0xfbf0, 0x8f00, 0xf1b0, 0x0f00, T2, CMP_IMM),
	ARMv7_OP4(0x0ff0, 0xf000, 0x0350, 0x0000, A1, CMP_IMM),
	ARMv7_OP2(0xffc0, 0x4280, T1, CMP_REG),
	ARMv7_OP2(0xff00, 0x4500, T2, CMP_REG),
	ARMv7_OP4(0xfff0, 0x8f00, 0xebb0, 0x0f00, T3, CMP_REG),
	ARMv7_OP4(0x0ff0, 0xf010, 0x0150, 0x0000, A1, CMP_REG),
	ARMv7_OP4(0x0ff0, 0xf090, 0x0150, 0x0010, A1, CMP_RSR),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf080, 0x0000, T1, EOR_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0220, 0x0000, A1, EOR_IMM),
	ARMv7_OP2(0xffc0, 0x4040, T1, EOR_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xea80, 0x0000, T2, EOR_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0020, 0x0000, A1, EOR_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0020, 0x0010, A1, EOR_RSR),

	ARMv7_OP2(0xff00, 0xbf00, T1, IT),

	ARMv7_OP2(0xf800, 0xc800, T1, LDM),
	ARMv7_OP4(0xffd0, 0x2000, 0xe890, 0x0000, T2, LDM),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0890, 0x0000, A1, LDM),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0810, 0x0000, A1, LDMDA),
	ARMv7_OP4(0xffd0, 0x2000, 0xe910, 0x0000, T1, LDMDB),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0910, 0x0000, A1, LDMDB),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0990, 0x0000, A1, LDMIB),

	ARMv7_OP2(0xf800, 0x6800, T1, LDR_IMM),
	ARMv7_OP2(0xf800, 0x9800, T2, LDR_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf8d0, 0x0000, T3, LDR_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf850, 0x0800, T4, LDR_IMM),
	ARMv7_OP4(0x0e50, 0x0000, 0x0410, 0x0000, A1, LDR_IMM),
	ARMv7_OP2(0xf800, 0x4800, T1, LDR_LIT),
	ARMv7_OP4(0xff7f, 0x0000, 0xf85f, 0x0000, T2, LDR_LIT),
	ARMv7_OP4(0x0f7f, 0x0000, 0x051f, 0x0000, A1, LDR_LIT),
	ARMv7_OP2(0xfe00, 0x5800, T1, LDR_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf850, 0x0000, T2, LDR_REG),
	ARMv7_OP4(0x0e50, 0x0010, 0x0610, 0x0000, A1, LDR_REG),

	ARMv7_OP2(0xf800, 0x7800, T1, LDRB_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf890, 0x0000, T2, LDRB_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf810, 0x0800, T3, LDRB_IMM),
	ARMv7_OP4(0x0e50, 0x0000, 0x0450, 0x0000, A1, LDRB_IMM),
	ARMv7_OP4(0xff7f, 0x0000, 0xf81f, 0x0000, T1, LDRB_LIT),
	ARMv7_OP4(0x0f7f, 0x0000, 0x055f, 0x0000, A1, LDRB_LIT),
	ARMv7_OP2(0xfe00, 0x5c00, T1, LDRB_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf810, 0x0000, T2, LDRB_REG),
	ARMv7_OP4(0x0e50, 0x0010, 0x0650, 0x0000, A1, LDRB_REG),

	ARMv7_OP4(0xfe50, 0x0000, 0xe850, 0x0000, T1, LDRD_IMM),
	ARMv7_OP4(0x0e50, 0x00f0, 0x0040, 0x00d0, A1, LDRD_IMM),
	ARMv7_OP4(0xfe7f, 0x0000, 0xe85f, 0x0000, T1, LDRD_LIT),
	ARMv7_OP4(0x0f7f, 0x00f0, 0x014f, 0x00d0, A1, LDRD_LIT),
	ARMv7_OP4(0x0e50, 0x0ff0, 0x0000, 0x00d0, A1, LDRD_REG),

	ARMv7_OP4(0xfff0, 0x0000, 0xf990, 0x0000, T1, LDRSB_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf910, 0x0800, T2, LDRSB_IMM),
	ARMv7_OP4(0x0e50, 0x00f0, 0x0050, 0x00d0, A1, LDRSB_IMM),
	ARMv7_OP4(0xff7f, 0x0000, 0xf91f, 0x0000, T1, LDRSB_LIT),
	ARMv7_OP4(0x0f7f, 0x00f0, 0x015f, 0x00d0, A1, LDRSB_LIT),
	ARMv7_OP2(0xfe00, 0x5600, T1, LDRSB_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf910, 0x0000, T2, LDRSB_REG),
	ARMv7_OP4(0x0e50, 0x0ff0, 0x0010, 0x00d0, A1, LDRSB_REG),

	ARMv7_OP4(0xfff0, 0x0000, 0xf9b0, 0x0000, T1, LDRSH_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf930, 0x0800, T2, LDRSH_IMM),
	ARMv7_OP4(0x0e50, 0x00f0, 0x0050, 0x00f0, A1, LDRSH_IMM),
	ARMv7_OP4(0xff7f, 0x0000, 0xf93f, 0x0000, T1, LDRSH_LIT),
	ARMv7_OP4(0x0f7f, 0x00f0, 0x015f, 0x00f0, A1, LDRSH_LIT),
	ARMv7_OP2(0xfe00, 0x5e00, T1, LDRSH_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf930, 0x0000, T2, LDRSH_REG),
	ARMv7_OP4(0x0e50, 0x0ff0, 0x0010, 0x00f0, A1, LDRSH_REG),

	ARMv7_OP2(0xf800, 0x0000, T1, LSL_IMM),
	ARMv7_OP4(0xffef, 0x8030, 0xea4f, 0x0000, T2, LSL_IMM),
	ARMv7_OP4(0x0fef, 0x0070, 0x01a0, 0x0000, A1, LSL_IMM),
	ARMv7_OP2(0xffc0, 0x4080, T1, LSL_REG),
	ARMv7_OP4(0xffe0, 0xf0f0, 0xfa00, 0xf000, T2, LSL_REG),
	ARMv7_OP4(0x0fef, 0x00f0, 0x01a0, 0x0010, A1, LSL_REG),

	ARMv7_OP2(0xf800, 0x0800, T1, LSR_IMM),
	ARMv7_OP4(0xffef, 0x8030, 0xea4f, 0x0010, T2, LSR_IMM),
	ARMv7_OP4(0x0fef, 0x0030, 0x01a0, 0x0020, A1, LSR_IMM),
	ARMv7_OP2(0xffc0, 0x40c0, T1, LSR_REG),
	ARMv7_OP4(0xffe0, 0xf0f0, 0xfa20, 0xf000, T2, LSR_REG),
	ARMv7_OP4(0x0fef, 0x00f0, 0x01a0, 0x0030, A1, LSR_REG),

	ARMv7_OP4(0xfff0, 0x00f0, 0xfb00, 0x0000, T1, MLA),
	ARMv7_OP4(0x0fe0, 0x00f0, 0x0020, 0x0090, A1, MLA),

	ARMv7_OP4(0xfff0, 0x00f0, 0xfb00, 0x0010, T1, MLS),
	ARMv7_OP4(0x0ff0, 0x00f0, 0x0060, 0x0090, A1, MLS),

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

	ARMv7_OP4(0xffff, 0xf0ff, 0xf3ef, 0x8000, T1, MRS),
	ARMv7_OP4(0x0fff, 0x0fff, 0x010f, 0x0000, A1, MRS),
	ARMv7_OP4(0x0ff3, 0xf000, 0x0320, 0xf000, A1, MSR_IMM),
	ARMv7_OP4(0xfff0, 0xf3ff, 0xf380, 0x8000, T1, MSR_REG),
	ARMv7_OP4(0x0ff3, 0xfff0, 0x0120, 0xf000, A1, MSR_REG),

	ARMv7_OP2(0xffc0, 0x4340, T1, MUL),
	ARMv7_OP4(0xfff0, 0xf0f0, 0xfb00, 0xf000, T2, MUL),
	ARMv7_OP4(0x0fe0, 0xf0f0, 0x0000, 0x0090, A1, MUL),

	ARMv7_OP4(0xfbef, 0x8000, 0xf06f, 0x0000, T1, MVN_IMM),
	ARMv7_OP4(0x0fef, 0x0000, 0x03e0, 0x0000, A1, MVN_IMM),
	ARMv7_OP2(0xffc0, 0x43c0, T1, MVN_REG),
	ARMv7_OP4(0xffef, 0x8000, 0xea6f, 0x0000, T2, MVN_REG),
	ARMv7_OP4(0xffef, 0x0010, 0x01e0, 0x0000, A1, MVN_REG),
	ARMv7_OP4(0x0fef, 0x0090, 0x01e0, 0x0010, A1, MVN_RSR),

	ARMv7_OP2(0xffff, 0xbf00, T1, NOP),
	ARMv7_OP4(0xffff, 0xffff, 0xf3af, 0x8000, T2, NOP),
	ARMv7_OP4(0x0fff, 0xffff, 0x0320, 0xf000, A1, NOP),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf060, 0x0000, T1, ORN_IMM),
	ARMv7_OP4(0xffe0, 0x8000, 0xea60, 0x0000, T1, ORN_REG),

	ARMv7_OP4(0xfbe0, 0x8000, 0xf040, 0x0000, T1, ORR_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0380, 0x0000, A1, ORR_IMM),
	ARMv7_OP2(0xffc0, 0x4300, T1, ORR_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xea40, 0x0000, T2, ORR_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0180, 0x0000, A1, ORR_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0180, 0x0010, A1, ORR_RSR),

	ARMv7_OP4(0xfff0, 0x8010, 0xeac0, 0x0000, T1, PKH),
	ARMv7_OP4(0x0ff0, 0x0030, 0x0680, 0x0010, A1, PKH),

	ARMv7_OP2(0xfe00, 0xbc00, T1, POP),
	ARMv7_OP4(0xffff, 0x0000, 0xe8bd, 0x0000, T2, POP),
	ARMv7_OP4(0xffff, 0x0fff, 0xf85d, 0x0b04, T3, POP),
	ARMv7_OP4(0x0fff, 0x0000, 0x08bd, 0x0000, A1, POP),
	ARMv7_OP4(0x0fff, 0x0fff, 0x049d, 0x0004, A2, POP),

	ARMv7_OP2(0xfe00, 0xb400, T1, PUSH),
	ARMv7_OP4(0xffff, 0x0000, 0xe92d, 0x0000, T2, PUSH), // had an error in arch ref
	ARMv7_OP4(0xffff, 0x0fff, 0xf84d, 0x0d04, T3, PUSH),
	ARMv7_OP4(0x0fff, 0x0000, 0x092d, 0x0000, A1, PUSH),
	ARMv7_OP4(0x0fff, 0x0fff, 0x052d, 0x0004, A2, PUSH),

	// TODO (Q*...)

	ARMv7_OP4(0xfff0, 0xf0f0, 0xfa90, 0xf0a0, T1, RBIT),
	ARMv7_OP4(0x0fff, 0x0ff0, 0x06ff, 0x0f30, A1, RBIT),

	ARMv7_OP2(0xffc0, 0xba00, T1, REV),
	ARMv7_OP4(0xfff0, 0xf0f0, 0xfa90, 0xf080, T2, REV),
	ARMv7_OP4(0x0fff, 0x0ff0, 0x06bf, 0x0f30, A1, REV),
	ARMv7_OP2(0xffc0, 0xba40, T1, REV16),
	ARMv7_OP4(0xfff0, 0xf0f0, 0xfa90, 0xf090, T2, REV16),
	ARMv7_OP4(0x0fff, 0x0ff0, 0x06bf, 0x0fb0, A1, REV16),
	ARMv7_OP2(0xffc0, 0xbac0, T1, REVSH),
	ARMv7_OP4(0xfff0, 0xf0f0, 0xfa90, 0xf0b0, T2, REVSH),
	ARMv7_OP4(0x0fff, 0x0ff0, 0x06ff, 0x0fb0, A1, REVSH),

	ARMv7_OP4(0xffef, 0x8030, 0xea4f, 0x0030, T1, ROR_IMM),
	ARMv7_OP4(0x0fef, 0x0070, 0x01a0, 0x0060, A1, ROR_IMM),
	ARMv7_OP2(0xffc0, 0x41c0, T1, ROR_REG),
	ARMv7_OP4(0xffe0, 0xf0f0, 0xfa60, 0xf000, T2, ROR_REG),
	ARMv7_OP4(0x0fef, 0x00f0, 0x01a0, 0x0070, A1, ROR_REG),
	ARMv7_OP4(0xffef, 0xf0f0, 0xea4f, 0x0030, T1, RRX),
	ARMv7_OP4(0x0fef, 0x0ff0, 0x01a0, 0x0060, A1, RRX),

	ARMv7_OP2(0xffc0, 0x4240, T1, RSB_IMM),
	ARMv7_OP4(0xfbe0, 0x8000, 0xf1c0, 0x0000, T2, RSB_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0260, 0x0000, A1, RSB_IMM),
	ARMv7_OP4(0xffe0, 0x8000, 0xebc0, 0x0000, T1, RSB_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0060, 0x0000, A1, RSB_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0060, 0x0010, A1, RSB_RSR),

	ARMv7_OP4(0x0fe0, 0x0000, 0x02e0, 0x0000, A1, RSC_IMM),
	ARMv7_OP4(0x0fe0, 0x0010, 0x00e0, 0x0000, A1, RSC_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x00e0, 0x0010, A1, RSC_RSR),

	// TODO (SADD16, SADD8, SASX)

	ARMv7_OP4(0xfbe0, 0x8000, 0xf160, 0x0000, T1, SBC_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x02c0, 0x0000, A1, SBC_IMM),
	ARMv7_OP2(0xffc0, 0x4180, T1, SBC_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeb60, 0x0000, T2, SBC_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x00c0, 0x0000, A1, SBC_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x00c0, 0x0010, A1, SBC_RSR),

	ARMv7_OP4(0xfff0, 0x8020, 0xf340, 0x0000, T1, SBFX),
	ARMv7_OP4(0x0fe0, 0x0070, 0x07a0, 0x0050, A1, SBFX),

	ARMv7_OP4(0xfff0, 0xf0f0, 0xfb90, 0xf0f0, T1, SDIV), // ???

	ARMv7_OP4(0xfff0, 0xf0f0, 0xfaa0, 0xf080, T1, SEL),
	ARMv7_OP4(0x0ff0, 0x0ff0, 0x0680, 0x0fb0, A1, SEL),

	// TODO (SH*, SM*, SS*)

	ARMv7_OP2(0xf800, 0xc000, T1, STM),
	ARMv7_OP4(0xffd0, 0xa000, 0xe880, 0x0000, T2, STM),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0880, 0x0000, A1, STM),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0800, 0x0000, A1, STMDA),
	ARMv7_OP4(0xffd0, 0xa000, 0xe900, 0x0000, T1, STMDB),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0900, 0x0000, A1, STMDB),
	ARMv7_OP4(0x0fd0, 0x0000, 0x0980, 0x0000, A1, STMIB),

	ARMv7_OP2(0xf800, 0x6000, T1, STR_IMM),
	ARMv7_OP2(0xf800, 0x9000, T2, STR_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf8c0, 0x0000, T3, STR_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf840, 0x0800, T4, STR_IMM),
	ARMv7_OP4(0x0e50, 0x0000, 0x0400, 0x0000, A1, STR_IMM),
	ARMv7_OP2(0xfe00, 0x5000, T1, STR_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf840, 0x0000, T2, STR_REG),
	ARMv7_OP4(0x0e50, 0x0010, 0x0600, 0x0000, A1, STR_REG),

	ARMv7_OP2(0xf800, 0x7000, T1, STRB_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf880, 0x0000, T2, STRB_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf800, 0x0800, T3, STRB_IMM),
	ARMv7_OP4(0x0e50, 0x0000, 0x0440, 0x0000, A1, STRB_IMM),
	ARMv7_OP2(0xfe00, 0x5400, T1, STRB_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf800, 0x0000, T2, STRB_REG),
	ARMv7_OP4(0x0e50, 0x0010, 0x0640, 0x0000, A1, STRB_REG),

	ARMv7_OP4(0xfe50, 0x0000, 0xe840, 0x0000, T1, STRD_IMM),
	ARMv7_OP4(0x0e50, 0x00f0, 0x0040, 0x00f0, A1, STRD_IMM),
	ARMv7_OP4(0x0e50, 0x0ff0, 0x0000, 0x00f0, A1, STRD_REG),

	ARMv7_OP2(0xf800, 0x8000, T1, STRH_IMM),
	ARMv7_OP4(0xfff0, 0x0000, 0xf8a0, 0x0000, T2, STRH_IMM),
	ARMv7_OP4(0xfff0, 0x0800, 0xf820, 0x0800, T3, STRH_IMM),
	ARMv7_OP4(0x0e50, 0x00f0, 0x0040, 0x00b0, A1, STRH_IMM),
	ARMv7_OP2(0xfe00, 0x5200, T1, STRH_REG),
	ARMv7_OP4(0xfff0, 0x0fc0, 0xf820, 0x0000, T2, STRH_REG),
	ARMv7_OP4(0x0e50, 0x0ff0, 0x0000, 0x00b0, A1, STRH_REG),

	ARMv7_OP2(0xff80, 0xb080, T1, SUB_SPI),
	ARMv7_OP4(0xfbef, 0x8000, 0xf1ad, 0x0000, T2, SUB_SPI),
	ARMv7_OP4(0xfbff, 0x8000, 0xf2ad, 0x0000, T3, SUB_SPI),
	ARMv7_OP4(0x0fef, 0x0000, 0x024d, 0x0000, A1, SUB_SPI),
	ARMv7_OP4(0xffef, 0x8000, 0xebad, 0x0000, T1, SUB_SPR),
	ARMv7_OP4(0x0fef, 0x0010, 0x004d, 0x0000, A1, SUB_SPR),
	ARMv7_OP2(0xfe00, 0x1e00, T1, SUB_IMM),
	ARMv7_OP2(0xf800, 0x3800, T2, SUB_IMM),
	ARMv7_OP4(0xfbe0, 0x8000, 0xf1a0, 0x0000, T3, SUB_IMM),
	ARMv7_OP4(0xfbf0, 0x8000, 0xf2a0, 0x0000, T4, SUB_IMM),
	ARMv7_OP4(0x0fe0, 0x0000, 0x0240, 0x0000, A1, SUB_IMM),
	ARMv7_OP2(0xfe00, 0x1a00, T1, SUB_REG),
	ARMv7_OP4(0xffe0, 0x8000, 0xeba0, 0x0000, T2, SUB_REG),
	ARMv7_OP4(0x0fe0, 0x0010, 0x0040, 0x0000, A1, SUB_REG),
	ARMv7_OP4(0x0fe0, 0x0090, 0x0040, 0x0010, A1, SUB_RSR),

	ARMv7_OP2(0xff00, 0xdf00, T1, SVC),
	ARMv7_OP4(0x0f00, 0x0000, 0x0f00, 0x0000, A1, SVC),

	// TODO (SX*)

	ARMv7_OP4(0xfff0, 0xffe0, 0xe8d0, 0xf000, T1, TB_),

	ARMv7_OP4(0xfbf0, 0x8f00, 0xf090, 0x0f00, T1, TEQ_IMM),
	ARMv7_OP4(0x0ff0, 0xf000, 0x0330, 0x0000, A1, TEQ_IMM),
	ARMv7_OP4(0xfff0, 0x8f00, 0xea90, 0x0f00, T1, TEQ_REG),
	ARMv7_OP4(0x0ff0, 0xf010, 0x0130, 0x0000, A1, TEQ_REG),
	ARMv7_OP4(0x0ff0, 0xf090, 0x0130, 0x0010, A1, TEQ_RSR),

	ARMv7_OP4(0xfbf0, 0x8f00, 0xf010, 0x0f00, T1, TST_IMM),
	ARMv7_OP4(0x0ff0, 0xf000, 0x0310, 0x0000, A1, TST_IMM),
	ARMv7_OP2(0xffc0, 0x4200, T1, TST_REG),
	ARMv7_OP4(0xfff0, 0x8f00, 0xea10, 0x0f00, T2, TST_REG),
	ARMv7_OP4(0x0ff0, 0xf010, 0x0110, 0x0000, A1, TST_REG),
	ARMv7_OP4(0x0ff0, 0xf090, 0x0110, 0x0010, A1, TST_RSR)

	// TODO (U*, V*)
};

#undef ARMv7_OP2
#undef ARMv7_OP4
