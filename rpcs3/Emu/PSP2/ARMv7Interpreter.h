#pragma once

#include "ARMv7Thread.h"
#include "ARMv7Opcodes.h"

struct arm_interpreter
{
	template<typename T>
	static u32 BitCount(T x, size_t len)
	{
		u32 result = 0;

		for (T mask = static_cast<T>(1) << (len - 1); mask; mask >>= 1)
		{
			if (x & mask) result++;
		}

		return result;
	}

	static u32 DecodeImmShift(u32 type, u32 imm5, u32* shift_n)
	{
		using namespace arm_code;

		SRType shift_t;

		switch (type)
		{
		case 0: shift_t = SRType_LSL; if (shift_n) *shift_n = imm5; break;
		case 1: shift_t = SRType_LSR; if (shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
		case 2: shift_t = SRType_ASR; if (shift_n) *shift_n = imm5 == 0 ? 32 : imm5; break;
		case 3:
			if (imm5 == 0)
			{
				shift_t = SRType_RRX; if (shift_n) *shift_n = 1;
			}
			else
			{
				shift_t = SRType_ROR; if (shift_n) *shift_n = imm5;
			}
			break;

		default: throw EXCEPTION("");
		}

		return shift_t;
	}

	static u32 LSL_C(u32 x, s32 shift, bool& carry_out)
	{
		EXPECTS(shift > 0);
		carry_out = shift <= 32 ? (x & (1 << (32 - shift))) != 0 : false;
		return shift < 32 ? x << shift : 0;
	}

	static u32 LSL_(u32 x, s32 shift)
	{
		EXPECTS(shift >= 0);
		return shift < 32 ? x << shift : 0;
	}

	static u32 LSR_C(u32 x, s32 shift, bool& carry_out)
	{
		EXPECTS(shift > 0);
		carry_out = shift <= 32 ? (x & (1 << (shift - 1))) != 0 : false;
		return shift < 32 ? x >> shift : 0;
	}

	static u32 LSR_(u32 x, s32 shift)
	{
		EXPECTS(shift >= 0);
		return shift < 32 ? x >> shift : 0;
	}

	static s32 ASR_C(s32 x, s32 shift, bool& carry_out)
	{
		EXPECTS(shift > 0);
		carry_out = shift <= 32 ? (x & (1 << (shift - 1))) != 0 : x < 0;
		return shift < 32 ? x >> shift : x >> 31;
	}

	static s32 ASR_(s32 x, s32 shift)
	{
		EXPECTS(shift >= 0);
		return shift < 32 ? x >> shift : x >> 31;
	}

	static u32 ROR_C(u32 x, s32 shift, bool& carry_out)
	{
		EXPECTS(shift);
		const u32 result = x >> shift | x << (32 - shift);
		carry_out = (result >> 31) != 0;
		return result;
	}

	static u32 ROR_(u32 x, s32 shift)
	{
		return x >> shift | x << (32 - shift);
	}

	static u32 RRX_C(u32 x, bool carry_in, bool& carry_out)
	{
		carry_out = x & 0x1;
		return ((u32)carry_in << 31) | (x >> 1);
	}

	static u32 RRX_(u32 x, bool carry_in)
	{
		return ((u32)carry_in << 31) | (x >> 1);
	}

	static u32 Shift_C(u32 value, u32 type, s32 amount, bool carry_in, bool& carry_out)
	{
		EXPECTS(type != arm_code::SRType_RRX || amount == 1);

		if (amount)
		{
			switch (type)
			{
			case arm_code::SRType_LSL: return LSL_C(value, amount, carry_out);
			case arm_code::SRType_LSR: return LSR_C(value, amount, carry_out);
			case arm_code::SRType_ASR: return ASR_C(value, amount, carry_out);
			case arm_code::SRType_ROR: return ROR_C(value, amount, carry_out);
			case arm_code::SRType_RRX: return RRX_C(value, carry_in, carry_out);
			default: throw EXCEPTION("");
			}
		}

		carry_out = carry_in;
		return value;
	}

	static u32 Shift(u32 value, u32 type, s32 amount, bool carry_in)
	{
		bool carry_out;
		return Shift_C(value, type, amount, carry_in, carry_out);
	}

	template<typename T> static T AddWithCarry(T x, T y, bool carry_in, bool& carry_out, bool& overflow)
	{
		const T sign_mask = (T)1 << (sizeof(T) * 8 - 1);

		T result = x + y;
		carry_out = (((x & y) | ((x ^ y) & ~result)) & sign_mask) != 0;
		overflow = ((x ^ result) & (y ^ result) & sign_mask) != 0;
		if (carry_in)
		{
			result += 1;
			carry_out ^= (result == 0);
			overflow ^= (result == sign_mask);
		}
		return result;
	}

	static bool ConditionPassed(ARMv7Thread& cpu, u32 cond)
	{
		bool result = false;

		switch (cond >> 1)
		{
		case 0: result = (cpu.APSR.Z == 1); break;
		case 1: result = (cpu.APSR.C == 1); break;
		case 2: result = (cpu.APSR.N == 1); break;
		case 3: result = (cpu.APSR.V == 1); break;
		case 4: result = (cpu.APSR.C == 1) && (cpu.APSR.Z == 0); break;
		case 5: result = (cpu.APSR.N == cpu.APSR.V); break;
		case 6: result = (cpu.APSR.N == cpu.APSR.V) && (cpu.APSR.Z == 0); break;
		case 7: return true;
		}

		if (cond & 0x1)
		{
			return !result;
		}

		return result;
	}	

	static void UNK(ARMv7Thread&, const u32 op, const u32 cond);

	template<arm_encoding type> static void HACK(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MRC_(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ADC_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADC_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADC_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ADD_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADD_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADD_RSR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADD_SPI(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ADD_SPR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ADR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void AND_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void AND_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void AND_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ASR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ASR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void B(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void BFC(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void BFI(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void BIC_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void BIC_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void BIC_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void BKPT(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void BL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void BLX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void BX(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void CB_Z(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void CLZ(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void CMN_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void CMN_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void CMN_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void CMP_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void CMP_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void CMP_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void DBG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void DMB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void DSB(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void EOR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void EOR_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void EOR_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void IT(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDMDA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDMDB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDMIB(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDR_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDRB_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRB_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRB_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDRD_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRD_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRD_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDRH_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRH_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRH_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDRSB_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRSB_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRSB_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDRSH_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRSH_LIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDRSH_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LDREX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDREXB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDREXD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LDREXH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LSL_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LSL_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void LSR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void LSR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void MLA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MLS(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void MOV_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MOV_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MOVT(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void MRS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MSR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MSR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void MUL(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void MVN_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MVN_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void MVN_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void NOP(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ORN_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ORN_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ORR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ORR_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ORR_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void PKH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void POP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void PUSH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void QADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QASX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QDADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QDSUB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QSAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QSUB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QSUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void QSUB8(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void RBIT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void REV(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void REV16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void REVSH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void ROR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void ROR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void RRX(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void RSB_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void RSB_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void RSB_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void RSC_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void RSC_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void RSC_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SASX(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SBC_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SBC_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SBC_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SBFX(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SDIV(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SEL(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SHADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SHADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SHASX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SHSAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SHSUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SHSUB8(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SMLA__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLAD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLAL__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLALD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLAW_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLSD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMLSLD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMMLA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMMLS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMMUL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMUAD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMUL__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMULL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMULW_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SMUSD(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SSAT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SSAT16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SSAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SSUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SSUB8(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STMDA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STMDB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STMIB(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STR_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STRB_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STRB_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STRD_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STRD_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STRH_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STRH_REG(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void STREX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STREXB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STREXD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void STREXH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SUB_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SUB_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SUB_RSR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SUB_SPI(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SUB_SPR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SVC(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void SXTAB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SXTAB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SXTAH(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SXTB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SXTB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void SXTH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void TB_(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void TEQ_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void TEQ_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void TEQ_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void TST_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void TST_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void TST_RSR(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void UADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UASX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UBFX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UDIV(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHASX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHSAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHSUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UHSUB8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UMAAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UMLAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UMULL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQADD16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQADD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQASX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQSAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQSUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UQSUB8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USAD8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USADA8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USAT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USAT16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USAX(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USUB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void USUB8(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTAB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTAB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTAH(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTB16(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void UXTH(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void VABA_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VABD_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VABD_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VABS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VAC__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VADD_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VADDHN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VADD_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VAND(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VBIC_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VBIC_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VB__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCEQ_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCEQ_ZERO(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCGE_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCGE_ZERO(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCGT_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCGT_ZERO(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCLE_ZERO(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCLS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCLT_ZERO(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCLZ(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCMP_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCNT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_FIA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_FIF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_FFA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_FFF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_DF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_HFA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VCVT_HFF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VDIV(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VDUP_S(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VDUP_R(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VEOR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VEXT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VHADDSUB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD__MS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD1_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD1_SAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD2_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD2_SAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD3_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD3_SAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD4_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLD4_SAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLDM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VLDR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMAXMIN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMAXMIN_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VML__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VML__FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VML__S(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_RS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_SR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_RF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_2RF(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOV_2RD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOVL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMOVN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMRS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMSR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMUL_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMUL_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMUL_S(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMVN_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VMVN_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VNEG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VNM__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VORN_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VORR_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VORR_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPADAL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPADD_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPADDL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPMAXMIN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPMAXMIN_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPOP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VPUSH(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQABS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQDML_L(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQDMULH(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQDMULL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQMOV_N(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQNEG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQRDMULH(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQRSHL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQRSHR_N(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQSHL_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQSHL_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQSHR_N(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VQSUB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRADDHN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRECPE(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRECPS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VREV__(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRHADD(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSHL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSHR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSHRN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSQRTE(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSQRTS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSRA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VRSUBHN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSHL_IMM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSHL_REG(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSHLL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSHR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSHRN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSLI(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSQRT(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSRA(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSRI(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VST__MS(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VST1_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VST2_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VST3_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VST4_SL(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSTM(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSTR(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSUB(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSUB_FP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSUBHN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSUB_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VSWP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VTB_(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VTRN(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VTST(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VUZP(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void VZIP(ARMv7Thread&, const u32, const u32);

	template<arm_encoding type> static void WFE(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void WFI(ARMv7Thread&, const u32, const u32);
	template<arm_encoding type> static void YIELD(ARMv7Thread&, const u32, const u32);
};
