#pragma once

#include "Emu/CPU/CPUDisAsm.h"

enum class arm_encoding;

class ARMv7DisAsm final : public CPUDisAsm
{
public:
	ARMv7DisAsm(CPUDisAsmMode mode) : CPUDisAsm(mode)
	{
	}

protected:
	virtual u32 DisAsmBranchTarget(const s32 imm) override
	{
		// TODO: ARM
		return dump_pc + (true ? 4 : 8) + imm;
	}

	virtual void Write(const std::string& value) override;

private:
	template<typename... Args>
	void write(const char* fmt, const Args&... args)
	{
		Write(fmt::format(fmt, args...));
	}

public:
	void UNK(const u32 op, const u32 cond);

	template<arm_encoding type> void HACK(const u32, const u32);
	template<arm_encoding type> void MRC_(const u32, const u32);

	template<arm_encoding type> void ADC_IMM(const u32, const u32);
	template<arm_encoding type> void ADC_REG(const u32, const u32);
	template<arm_encoding type> void ADC_RSR(const u32, const u32);

	template<arm_encoding type> void ADD_IMM(const u32, const u32);
	template<arm_encoding type> void ADD_REG(const u32, const u32);
	template<arm_encoding type> void ADD_RSR(const u32, const u32);
	template<arm_encoding type> void ADD_SPI(const u32, const u32);
	template<arm_encoding type> void ADD_SPR(const u32, const u32);

	template<arm_encoding type> void ADR(const u32, const u32);

	template<arm_encoding type> void AND_IMM(const u32, const u32);
	template<arm_encoding type> void AND_REG(const u32, const u32);
	template<arm_encoding type> void AND_RSR(const u32, const u32);

	template<arm_encoding type> void ASR_IMM(const u32, const u32);
	template<arm_encoding type> void ASR_REG(const u32, const u32);

	template<arm_encoding type> void B(const u32, const u32);

	template<arm_encoding type> void BFC(const u32, const u32);
	template<arm_encoding type> void BFI(const u32, const u32);

	template<arm_encoding type> void BIC_IMM(const u32, const u32);
	template<arm_encoding type> void BIC_REG(const u32, const u32);
	template<arm_encoding type> void BIC_RSR(const u32, const u32);

	template<arm_encoding type> void BKPT(const u32, const u32);

	template<arm_encoding type> void BL(const u32, const u32);
	template<arm_encoding type> void BLX(const u32, const u32);
	template<arm_encoding type> void BX(const u32, const u32);

	template<arm_encoding type> void CB_Z(const u32, const u32);

	template<arm_encoding type> void CLZ(const u32, const u32);

	template<arm_encoding type> void CMN_IMM(const u32, const u32);
	template<arm_encoding type> void CMN_REG(const u32, const u32);
	template<arm_encoding type> void CMN_RSR(const u32, const u32);

	template<arm_encoding type> void CMP_IMM(const u32, const u32);
	template<arm_encoding type> void CMP_REG(const u32, const u32);
	template<arm_encoding type> void CMP_RSR(const u32, const u32);

	template<arm_encoding type> void DBG(const u32, const u32);
	template<arm_encoding type> void DMB(const u32, const u32);
	template<arm_encoding type> void DSB(const u32, const u32);

	template<arm_encoding type> void EOR_IMM(const u32, const u32);
	template<arm_encoding type> void EOR_REG(const u32, const u32);
	template<arm_encoding type> void EOR_RSR(const u32, const u32);

	template<arm_encoding type> void IT(const u32, const u32);

	template<arm_encoding type> void LDM(const u32, const u32);
	template<arm_encoding type> void LDMDA(const u32, const u32);
	template<arm_encoding type> void LDMDB(const u32, const u32);
	template<arm_encoding type> void LDMIB(const u32, const u32);

	template<arm_encoding type> void LDR_IMM(const u32, const u32);
	template<arm_encoding type> void LDR_LIT(const u32, const u32);
	template<arm_encoding type> void LDR_REG(const u32, const u32);

	template<arm_encoding type> void LDRB_IMM(const u32, const u32);
	template<arm_encoding type> void LDRB_LIT(const u32, const u32);
	template<arm_encoding type> void LDRB_REG(const u32, const u32);

	template<arm_encoding type> void LDRD_IMM(const u32, const u32);
	template<arm_encoding type> void LDRD_LIT(const u32, const u32);
	template<arm_encoding type> void LDRD_REG(const u32, const u32);

	template<arm_encoding type> void LDRH_IMM(const u32, const u32);
	template<arm_encoding type> void LDRH_LIT(const u32, const u32);
	template<arm_encoding type> void LDRH_REG(const u32, const u32);

	template<arm_encoding type> void LDRSB_IMM(const u32, const u32);
	template<arm_encoding type> void LDRSB_LIT(const u32, const u32);
	template<arm_encoding type> void LDRSB_REG(const u32, const u32);

	template<arm_encoding type> void LDRSH_IMM(const u32, const u32);
	template<arm_encoding type> void LDRSH_LIT(const u32, const u32);
	template<arm_encoding type> void LDRSH_REG(const u32, const u32);

	template<arm_encoding type> void LDREX(const u32, const u32);
	template<arm_encoding type> void LDREXB(const u32, const u32);
	template<arm_encoding type> void LDREXD(const u32, const u32);
	template<arm_encoding type> void LDREXH(const u32, const u32);

	template<arm_encoding type> void LSL_IMM(const u32, const u32);
	template<arm_encoding type> void LSL_REG(const u32, const u32);

	template<arm_encoding type> void LSR_IMM(const u32, const u32);
	template<arm_encoding type> void LSR_REG(const u32, const u32);

	template<arm_encoding type> void MLA(const u32, const u32);
	template<arm_encoding type> void MLS(const u32, const u32);

	template<arm_encoding type> void MOV_IMM(const u32, const u32);
	template<arm_encoding type> void MOV_REG(const u32, const u32);
	template<arm_encoding type> void MOVT(const u32, const u32);

	template<arm_encoding type> void MRS(const u32, const u32);
	template<arm_encoding type> void MSR_IMM(const u32, const u32);
	template<arm_encoding type> void MSR_REG(const u32, const u32);

	template<arm_encoding type> void MUL(const u32, const u32);

	template<arm_encoding type> void MVN_IMM(const u32, const u32);
	template<arm_encoding type> void MVN_REG(const u32, const u32);
	template<arm_encoding type> void MVN_RSR(const u32, const u32);

	template<arm_encoding type> void NOP(const u32, const u32);

	template<arm_encoding type> void ORN_IMM(const u32, const u32);
	template<arm_encoding type> void ORN_REG(const u32, const u32);

	template<arm_encoding type> void ORR_IMM(const u32, const u32);
	template<arm_encoding type> void ORR_REG(const u32, const u32);
	template<arm_encoding type> void ORR_RSR(const u32, const u32);

	template<arm_encoding type> void PKH(const u32, const u32);

	template<arm_encoding type> void POP(const u32, const u32);
	template<arm_encoding type> void PUSH(const u32, const u32);

	template<arm_encoding type> void QADD(const u32, const u32);
	template<arm_encoding type> void QADD16(const u32, const u32);
	template<arm_encoding type> void QADD8(const u32, const u32);
	template<arm_encoding type> void QASX(const u32, const u32);
	template<arm_encoding type> void QDADD(const u32, const u32);
	template<arm_encoding type> void QDSUB(const u32, const u32);
	template<arm_encoding type> void QSAX(const u32, const u32);
	template<arm_encoding type> void QSUB(const u32, const u32);
	template<arm_encoding type> void QSUB16(const u32, const u32);
	template<arm_encoding type> void QSUB8(const u32, const u32);

	template<arm_encoding type> void RBIT(const u32, const u32);
	template<arm_encoding type> void REV(const u32, const u32);
	template<arm_encoding type> void REV16(const u32, const u32);
	template<arm_encoding type> void REVSH(const u32, const u32);

	template<arm_encoding type> void ROR_IMM(const u32, const u32);
	template<arm_encoding type> void ROR_REG(const u32, const u32);

	template<arm_encoding type> void RRX(const u32, const u32);

	template<arm_encoding type> void RSB_IMM(const u32, const u32);
	template<arm_encoding type> void RSB_REG(const u32, const u32);
	template<arm_encoding type> void RSB_RSR(const u32, const u32);

	template<arm_encoding type> void RSC_IMM(const u32, const u32);
	template<arm_encoding type> void RSC_REG(const u32, const u32);
	template<arm_encoding type> void RSC_RSR(const u32, const u32);

	template<arm_encoding type> void SADD16(const u32, const u32);
	template<arm_encoding type> void SADD8(const u32, const u32);
	template<arm_encoding type> void SASX(const u32, const u32);

	template<arm_encoding type> void SBC_IMM(const u32, const u32);
	template<arm_encoding type> void SBC_REG(const u32, const u32);
	template<arm_encoding type> void SBC_RSR(const u32, const u32);

	template<arm_encoding type> void SBFX(const u32, const u32);

	template<arm_encoding type> void SDIV(const u32, const u32);

	template<arm_encoding type> void SEL(const u32, const u32);

	template<arm_encoding type> void SHADD16(const u32, const u32);
	template<arm_encoding type> void SHADD8(const u32, const u32);
	template<arm_encoding type> void SHASX(const u32, const u32);
	template<arm_encoding type> void SHSAX(const u32, const u32);
	template<arm_encoding type> void SHSUB16(const u32, const u32);
	template<arm_encoding type> void SHSUB8(const u32, const u32);

	template<arm_encoding type> void SMLA__(const u32, const u32);
	template<arm_encoding type> void SMLAD(const u32, const u32);
	template<arm_encoding type> void SMLAL(const u32, const u32);
	template<arm_encoding type> void SMLAL__(const u32, const u32);
	template<arm_encoding type> void SMLALD(const u32, const u32);
	template<arm_encoding type> void SMLAW_(const u32, const u32);
	template<arm_encoding type> void SMLSD(const u32, const u32);
	template<arm_encoding type> void SMLSLD(const u32, const u32);
	template<arm_encoding type> void SMMLA(const u32, const u32);
	template<arm_encoding type> void SMMLS(const u32, const u32);
	template<arm_encoding type> void SMMUL(const u32, const u32);
	template<arm_encoding type> void SMUAD(const u32, const u32);
	template<arm_encoding type> void SMUL__(const u32, const u32);
	template<arm_encoding type> void SMULL(const u32, const u32);
	template<arm_encoding type> void SMULW_(const u32, const u32);
	template<arm_encoding type> void SMUSD(const u32, const u32);

	template<arm_encoding type> void SSAT(const u32, const u32);
	template<arm_encoding type> void SSAT16(const u32, const u32);
	template<arm_encoding type> void SSAX(const u32, const u32);
	template<arm_encoding type> void SSUB16(const u32, const u32);
	template<arm_encoding type> void SSUB8(const u32, const u32);

	template<arm_encoding type> void STM(const u32, const u32);
	template<arm_encoding type> void STMDA(const u32, const u32);
	template<arm_encoding type> void STMDB(const u32, const u32);
	template<arm_encoding type> void STMIB(const u32, const u32);

	template<arm_encoding type> void STR_IMM(const u32, const u32);
	template<arm_encoding type> void STR_REG(const u32, const u32);

	template<arm_encoding type> void STRB_IMM(const u32, const u32);
	template<arm_encoding type> void STRB_REG(const u32, const u32);

	template<arm_encoding type> void STRD_IMM(const u32, const u32);
	template<arm_encoding type> void STRD_REG(const u32, const u32);

	template<arm_encoding type> void STRH_IMM(const u32, const u32);
	template<arm_encoding type> void STRH_REG(const u32, const u32);

	template<arm_encoding type> void STREX(const u32, const u32);
	template<arm_encoding type> void STREXB(const u32, const u32);
	template<arm_encoding type> void STREXD(const u32, const u32);
	template<arm_encoding type> void STREXH(const u32, const u32);

	template<arm_encoding type> void SUB_IMM(const u32, const u32);
	template<arm_encoding type> void SUB_REG(const u32, const u32);
	template<arm_encoding type> void SUB_RSR(const u32, const u32);
	template<arm_encoding type> void SUB_SPI(const u32, const u32);
	template<arm_encoding type> void SUB_SPR(const u32, const u32);

	template<arm_encoding type> void SVC(const u32, const u32);

	template<arm_encoding type> void SXTAB(const u32, const u32);
	template<arm_encoding type> void SXTAB16(const u32, const u32);
	template<arm_encoding type> void SXTAH(const u32, const u32);
	template<arm_encoding type> void SXTB(const u32, const u32);
	template<arm_encoding type> void SXTB16(const u32, const u32);
	template<arm_encoding type> void SXTH(const u32, const u32);

	template<arm_encoding type> void TB_(const u32, const u32);

	template<arm_encoding type> void TEQ_IMM(const u32, const u32);
	template<arm_encoding type> void TEQ_REG(const u32, const u32);
	template<arm_encoding type> void TEQ_RSR(const u32, const u32);

	template<arm_encoding type> void TST_IMM(const u32, const u32);
	template<arm_encoding type> void TST_REG(const u32, const u32);
	template<arm_encoding type> void TST_RSR(const u32, const u32);

	template<arm_encoding type> void UADD16(const u32, const u32);
	template<arm_encoding type> void UADD8(const u32, const u32);
	template<arm_encoding type> void UASX(const u32, const u32);
	template<arm_encoding type> void UBFX(const u32, const u32);
	template<arm_encoding type> void UDIV(const u32, const u32);
	template<arm_encoding type> void UHADD16(const u32, const u32);
	template<arm_encoding type> void UHADD8(const u32, const u32);
	template<arm_encoding type> void UHASX(const u32, const u32);
	template<arm_encoding type> void UHSAX(const u32, const u32);
	template<arm_encoding type> void UHSUB16(const u32, const u32);
	template<arm_encoding type> void UHSUB8(const u32, const u32);
	template<arm_encoding type> void UMAAL(const u32, const u32);
	template<arm_encoding type> void UMLAL(const u32, const u32);
	template<arm_encoding type> void UMULL(const u32, const u32);
	template<arm_encoding type> void UQADD16(const u32, const u32);
	template<arm_encoding type> void UQADD8(const u32, const u32);
	template<arm_encoding type> void UQASX(const u32, const u32);
	template<arm_encoding type> void UQSAX(const u32, const u32);
	template<arm_encoding type> void UQSUB16(const u32, const u32);
	template<arm_encoding type> void UQSUB8(const u32, const u32);
	template<arm_encoding type> void USAD8(const u32, const u32);
	template<arm_encoding type> void USADA8(const u32, const u32);
	template<arm_encoding type> void USAT(const u32, const u32);
	template<arm_encoding type> void USAT16(const u32, const u32);
	template<arm_encoding type> void USAX(const u32, const u32);
	template<arm_encoding type> void USUB16(const u32, const u32);
	template<arm_encoding type> void USUB8(const u32, const u32);
	template<arm_encoding type> void UXTAB(const u32, const u32);
	template<arm_encoding type> void UXTAB16(const u32, const u32);
	template<arm_encoding type> void UXTAH(const u32, const u32);
	template<arm_encoding type> void UXTB(const u32, const u32);
	template<arm_encoding type> void UXTB16(const u32, const u32);
	template<arm_encoding type> void UXTH(const u32, const u32);

	template<arm_encoding type> void VABA_(const u32, const u32);
	template<arm_encoding type> void VABD_(const u32, const u32);
	template<arm_encoding type> void VABD_FP(const u32, const u32);
	template<arm_encoding type> void VABS(const u32, const u32);
	template<arm_encoding type> void VAC__(const u32, const u32);
	template<arm_encoding type> void VADD(const u32, const u32);
	template<arm_encoding type> void VADD_FP(const u32, const u32);
	template<arm_encoding type> void VADDHN(const u32, const u32);
	template<arm_encoding type> void VADD_(const u32, const u32);
	template<arm_encoding type> void VAND(const u32, const u32);
	template<arm_encoding type> void VBIC_IMM(const u32, const u32);
	template<arm_encoding type> void VBIC_REG(const u32, const u32);
	template<arm_encoding type> void VB__(const u32, const u32);
	template<arm_encoding type> void VCEQ_REG(const u32, const u32);
	template<arm_encoding type> void VCEQ_ZERO(const u32, const u32);
	template<arm_encoding type> void VCGE_REG(const u32, const u32);
	template<arm_encoding type> void VCGE_ZERO(const u32, const u32);
	template<arm_encoding type> void VCGT_REG(const u32, const u32);
	template<arm_encoding type> void VCGT_ZERO(const u32, const u32);
	template<arm_encoding type> void VCLE_ZERO(const u32, const u32);
	template<arm_encoding type> void VCLS(const u32, const u32);
	template<arm_encoding type> void VCLT_ZERO(const u32, const u32);
	template<arm_encoding type> void VCLZ(const u32, const u32);
	template<arm_encoding type> void VCMP_(const u32, const u32);
	template<arm_encoding type> void VCNT(const u32, const u32);
	template<arm_encoding type> void VCVT_FIA(const u32, const u32);
	template<arm_encoding type> void VCVT_FIF(const u32, const u32);
	template<arm_encoding type> void VCVT_FFA(const u32, const u32);
	template<arm_encoding type> void VCVT_FFF(const u32, const u32);
	template<arm_encoding type> void VCVT_DF(const u32, const u32);
	template<arm_encoding type> void VCVT_HFA(const u32, const u32);
	template<arm_encoding type> void VCVT_HFF(const u32, const u32);
	template<arm_encoding type> void VDIV(const u32, const u32);
	template<arm_encoding type> void VDUP_S(const u32, const u32);
	template<arm_encoding type> void VDUP_R(const u32, const u32);
	template<arm_encoding type> void VEOR(const u32, const u32);
	template<arm_encoding type> void VEXT(const u32, const u32);
	template<arm_encoding type> void VHADDSUB(const u32, const u32);
	template<arm_encoding type> void VLD__MS(const u32, const u32);
	template<arm_encoding type> void VLD1_SL(const u32, const u32);
	template<arm_encoding type> void VLD1_SAL(const u32, const u32);
	template<arm_encoding type> void VLD2_SL(const u32, const u32);
	template<arm_encoding type> void VLD2_SAL(const u32, const u32);
	template<arm_encoding type> void VLD3_SL(const u32, const u32);
	template<arm_encoding type> void VLD3_SAL(const u32, const u32);
	template<arm_encoding type> void VLD4_SL(const u32, const u32);
	template<arm_encoding type> void VLD4_SAL(const u32, const u32);
	template<arm_encoding type> void VLDM(const u32, const u32);
	template<arm_encoding type> void VLDR(const u32, const u32);
	template<arm_encoding type> void VMAXMIN(const u32, const u32);
	template<arm_encoding type> void VMAXMIN_FP(const u32, const u32);
	template<arm_encoding type> void VML__(const u32, const u32);
	template<arm_encoding type> void VML__FP(const u32, const u32);
	template<arm_encoding type> void VML__S(const u32, const u32);
	template<arm_encoding type> void VMOV_IMM(const u32, const u32);
	template<arm_encoding type> void VMOV_REG(const u32, const u32);
	template<arm_encoding type> void VMOV_RS(const u32, const u32);
	template<arm_encoding type> void VMOV_SR(const u32, const u32);
	template<arm_encoding type> void VMOV_RF(const u32, const u32);
	template<arm_encoding type> void VMOV_2RF(const u32, const u32);
	template<arm_encoding type> void VMOV_2RD(const u32, const u32);
	template<arm_encoding type> void VMOVL(const u32, const u32);
	template<arm_encoding type> void VMOVN(const u32, const u32);
	template<arm_encoding type> void VMRS(const u32, const u32);
	template<arm_encoding type> void VMSR(const u32, const u32);
	template<arm_encoding type> void VMUL_(const u32, const u32);
	template<arm_encoding type> void VMUL_FP(const u32, const u32);
	template<arm_encoding type> void VMUL_S(const u32, const u32);
	template<arm_encoding type> void VMVN_IMM(const u32, const u32);
	template<arm_encoding type> void VMVN_REG(const u32, const u32);
	template<arm_encoding type> void VNEG(const u32, const u32);
	template<arm_encoding type> void VNM__(const u32, const u32);
	template<arm_encoding type> void VORN_REG(const u32, const u32);
	template<arm_encoding type> void VORR_IMM(const u32, const u32);
	template<arm_encoding type> void VORR_REG(const u32, const u32);
	template<arm_encoding type> void VPADAL(const u32, const u32);
	template<arm_encoding type> void VPADD(const u32, const u32);
	template<arm_encoding type> void VPADD_FP(const u32, const u32);
	template<arm_encoding type> void VPADDL(const u32, const u32);
	template<arm_encoding type> void VPMAXMIN(const u32, const u32);
	template<arm_encoding type> void VPMAXMIN_FP(const u32, const u32);
	template<arm_encoding type> void VPOP(const u32, const u32);
	template<arm_encoding type> void VPUSH(const u32, const u32);
	template<arm_encoding type> void VQABS(const u32, const u32);
	template<arm_encoding type> void VQADD(const u32, const u32);
	template<arm_encoding type> void VQDML_L(const u32, const u32);
	template<arm_encoding type> void VQDMULH(const u32, const u32);
	template<arm_encoding type> void VQDMULL(const u32, const u32);
	template<arm_encoding type> void VQMOV_N(const u32, const u32);
	template<arm_encoding type> void VQNEG(const u32, const u32);
	template<arm_encoding type> void VQRDMULH(const u32, const u32);
	template<arm_encoding type> void VQRSHL(const u32, const u32);
	template<arm_encoding type> void VQRSHR_N(const u32, const u32);
	template<arm_encoding type> void VQSHL_REG(const u32, const u32);
	template<arm_encoding type> void VQSHL_IMM(const u32, const u32);
	template<arm_encoding type> void VQSHR_N(const u32, const u32);
	template<arm_encoding type> void VQSUB(const u32, const u32);
	template<arm_encoding type> void VRADDHN(const u32, const u32);
	template<arm_encoding type> void VRECPE(const u32, const u32);
	template<arm_encoding type> void VRECPS(const u32, const u32);
	template<arm_encoding type> void VREV__(const u32, const u32);
	template<arm_encoding type> void VRHADD(const u32, const u32);
	template<arm_encoding type> void VRSHL(const u32, const u32);
	template<arm_encoding type> void VRSHR(const u32, const u32);
	template<arm_encoding type> void VRSHRN(const u32, const u32);
	template<arm_encoding type> void VRSQRTE(const u32, const u32);
	template<arm_encoding type> void VRSQRTS(const u32, const u32);
	template<arm_encoding type> void VRSRA(const u32, const u32);
	template<arm_encoding type> void VRSUBHN(const u32, const u32);
	template<arm_encoding type> void VSHL_IMM(const u32, const u32);
	template<arm_encoding type> void VSHL_REG(const u32, const u32);
	template<arm_encoding type> void VSHLL(const u32, const u32);
	template<arm_encoding type> void VSHR(const u32, const u32);
	template<arm_encoding type> void VSHRN(const u32, const u32);
	template<arm_encoding type> void VSLI(const u32, const u32);
	template<arm_encoding type> void VSQRT(const u32, const u32);
	template<arm_encoding type> void VSRA(const u32, const u32);
	template<arm_encoding type> void VSRI(const u32, const u32);
	template<arm_encoding type> void VST__MS(const u32, const u32);
	template<arm_encoding type> void VST1_SL(const u32, const u32);
	template<arm_encoding type> void VST2_SL(const u32, const u32);
	template<arm_encoding type> void VST3_SL(const u32, const u32);
	template<arm_encoding type> void VST4_SL(const u32, const u32);
	template<arm_encoding type> void VSTM(const u32, const u32);
	template<arm_encoding type> void VSTR(const u32, const u32);
	template<arm_encoding type> void VSUB(const u32, const u32);
	template<arm_encoding type> void VSUB_FP(const u32, const u32);
	template<arm_encoding type> void VSUBHN(const u32, const u32);
	template<arm_encoding type> void VSUB_(const u32, const u32);
	template<arm_encoding type> void VSWP(const u32, const u32);
	template<arm_encoding type> void VTB_(const u32, const u32);
	template<arm_encoding type> void VTRN(const u32, const u32);
	template<arm_encoding type> void VTST(const u32, const u32);
	template<arm_encoding type> void VUZP(const u32, const u32);
	template<arm_encoding type> void VZIP(const u32, const u32);

	template<arm_encoding type> void WFE(const u32, const u32);
	template<arm_encoding type> void WFI(const u32, const u32);
	template<arm_encoding type> void YIELD(const u32, const u32);

public:
	u32 disasm(u32 pc) override;
};
