#pragma once

union ARMv7Code
{
	struct
	{
		u16 code0;
		u16 code1;
	};

	u32 data;
};

enum ARMv7_encoding
{
	T1, T2, T3, T4, A1, A2
};

enum SRType : u32
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
	u32 LSL_(u32 x, s32 shift);
	u32 LSR_C(u32 x, s32 shift, bool& carry_out);
	u32 LSR_(u32 x, s32 shift);

	s32 ASR_C(s32 x, s32 shift, bool& carry_out);
	s32 ASR_(s32 x, s32 shift);

	u32 ROR_C(u32 x, s32 shift, bool& carry_out);
	u32 ROR_(u32 x, s32 shift);

	u32 RRX_C(u32 x, bool carry_in, bool& carry_out);
	u32 RRX_(u32 x, bool carry_in);

	template<typename T> T Shift_C(T value, SRType type, s32 amount, bool carry_in, bool& carry_out);

	template<typename T> T Shift(T value, SRType type, s32 amount, bool carry_in);

	template<typename T> T AddWithCarry(T x, T y, bool carry_in, bool& carry_out, bool& overflow);

	u32 ThumbExpandImm_C(u32 imm12, bool carry_in, bool& carry_out);
	u32 ThumbExpandImm(ARMv7Context& context, u32 imm12);

	bool ConditionPassed(ARMv7Context& context, u32 cond);

	// instructions
	void UNK(ARMv7Context& context, const ARMv7Code code);

	void NULL_OP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void HACK(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ADC_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADC_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADC_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ADD_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADD_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADD_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADD_SPI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ADD_SPR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ADR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void AND_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void AND_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void AND_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ASR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ASR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void B(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void BFC(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void BFI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void BIC_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void BIC_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void BIC_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void BKPT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void BL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void BLX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void BX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void CB_Z(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void CLZ(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void CMN_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void CMN_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void CMN_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void CMP_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void CMP_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void CMP_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void EOR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void EOR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void EOR_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void IT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDMDA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDMDB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDMIB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDR_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDRB_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRB_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRB_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDRD_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRD_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRD_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDRH_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRH_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRH_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDRSB_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRSB_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRSB_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDRSH_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRSH_LIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDRSH_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LDREX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDREXB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDREXD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LDREXH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LSL_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LSL_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void LSR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void LSR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MLA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MLS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MOV_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MOV_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MOVT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MRS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MSR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MSR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MVN_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MVN_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void MVN_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void NOP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ORN_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ORN_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ORR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ORR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ORR_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void PKH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void POP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void PUSH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void QADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QDADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QDSUB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QSAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QSUB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QSUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void QSUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void RBIT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void REV(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void REV16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void REVSH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void ROR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void ROR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void RRX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void RSB_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void RSB_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void RSB_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void RSC_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void RSC_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void RSC_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SBC_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SBC_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SBC_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SBFX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SDIV(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SEL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SHADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SHADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SHASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SHSAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SHSUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SHSUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SMLA__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLAD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLAL__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLALD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLAW_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLSD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMLSLD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMMLA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMMLS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMMUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMUAD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMUL__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMULL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMULW_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SMUSD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SSAT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SSAT16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SSAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SSUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SSUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STMDA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STMDB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STMIB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STRB_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STRB_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STRD_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STRD_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STRH_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STRH_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void STREX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STREXB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STREXD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void STREXH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SUB_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SUB_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SUB_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SUB_SPI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SUB_SPR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SVC(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void SXTAB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SXTAB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SXTAH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SXTB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SXTB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void SXTH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void TB_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void TEQ_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void TEQ_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void TEQ_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void TST_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void TST_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void TST_RSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void UADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UBFX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UDIV(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHSAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHSUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UHSUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UMAAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UMLAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UMULL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQADD16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQADD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQASX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQSAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQSUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UQSUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USAD8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USADA8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USAT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USAT16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USAX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USUB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void USUB8(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTAB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTAB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTAH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTB16(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void UXTH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void VABA_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VABD_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VABD_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VABS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VAC__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VADD_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VADDHN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VADD_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VAND(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VBIC_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VBIC_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VB__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCEQ_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCEQ_ZERO(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCGE_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCGE_ZERO(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCGT_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCGT_ZERO(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCLE_ZERO(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCLS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCLT_ZERO(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCLZ(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCMP_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCNT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_FIA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_FIF(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_FFA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_FFF(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_DF(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_HFA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VCVT_HFF(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VDIV(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VDUP_S(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VDUP_R(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VEOR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VEXT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VHADDSUB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD1_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD1_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD1_SAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD2_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD2_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD2_SAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD3_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD3_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD3_SAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD4_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD4_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLD4_SAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLDM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VLDR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMAXMIN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMAXMIN_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VML__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VML_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VML__S(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_RS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_SR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_RF(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_RF2(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOV_RD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOVL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMOVN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMRS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMSR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMUL_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMUL_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMUL_S(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMVN_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VMVN_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VNEG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VNM__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VORN_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VORN_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VORR_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VORR_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPADAL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPADD_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPADDL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPMAXMIN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPMAXMIN_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPOP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VPUSH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQABS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQDML_L(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQDMULH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQDMULL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQMOV_N(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQNEG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQRDMULH(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQRSHL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQRSHR_N(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQSHL_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQSHL_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQSHR_N(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VQSUB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRADDHN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRECPE(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRECPS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VREV__(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRHADD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSHL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSHR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSHRN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSQRTE(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSQRTS(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSRA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VRSUBHN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSHL_IMM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSHL_REG(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSHLL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSHR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSHRN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSLI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSQRT(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSRA(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSRI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST1_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST1_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST2_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST2_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST3_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST3_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST4_MUL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VST4_SL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSTM(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSTR(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSUB(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSUB_FP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSUBHN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSUB_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VSWP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VTBL(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VTBX(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VTRN(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VTST(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VUZP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void VZIP(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void WFE(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void WFI(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
	void YIELD(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);

	void MRC_(ARMv7Context& context, const ARMv7Code code, const ARMv7_encoding type);
};
