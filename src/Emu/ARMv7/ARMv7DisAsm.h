#pragma once
#include "Emu/CPU/CPUDisAsm.h"

static const char* g_arm_cond_name[16] =
{
	"eq", "ne", "cs", "cc",
	"mi", "pl", "vs", "vc",
	"hi", "ls", "ge", "lt",
	"gt", "le", "al", "al",
};

static const char* g_arm_reg_name[16] =
{
	"r0", "r1", "r2", "r3",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "sp", "lr", "pc",
};

class ARMv7DisAsm
	: public CPUDisAsm
{
public:
	ARMv7DisAsm() : CPUDisAsm(CPUDisAsm_InterpreterMode)
	{
	}

protected:
	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return (u32)dump_pc + imm;
	}

#if 0
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

	virtual void HACK(const u32 data, const ARMv7_encoding type);

	virtual void ADC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADC_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void ADD_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ADD_REG(const u32 data, const ARMv7_encoding type);
	virtual void ADD_RSR(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPI(const u32 data, const ARMv7_encoding type);
	virtual void ADD_SPR(const u32 data, const ARMv7_encoding type);

	virtual void ADR(const u32 data, const ARMv7_encoding type);

	virtual void AND_IMM(const u32 data, const ARMv7_encoding type);
	virtual void AND_REG(const u32 data, const ARMv7_encoding type);
	virtual void AND_RSR(const u32 data, const ARMv7_encoding type);

	virtual void ASR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ASR_REG(const u32 data, const ARMv7_encoding type);

	virtual void B(const u32 data, const ARMv7_encoding type);

	virtual void BFC(const u32 data, const ARMv7_encoding type);
	virtual void BFI(const u32 data, const ARMv7_encoding type);

	virtual void BIC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void BIC_REG(const u32 data, const ARMv7_encoding type);
	virtual void BIC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void BKPT(const u32 data, const ARMv7_encoding type);

	virtual void BL(const u32 data, const ARMv7_encoding type);
	virtual void BLX(const u32 data, const ARMv7_encoding type);
	virtual void BX(const u32 data, const ARMv7_encoding type);

	virtual void CB_Z(const u32 data, const ARMv7_encoding type);

	virtual void CLZ(const u32 data, const ARMv7_encoding type);

	virtual void CMN_IMM(const u32 data, const ARMv7_encoding type);
	virtual void CMN_REG(const u32 data, const ARMv7_encoding type);
	virtual void CMN_RSR(const u32 data, const ARMv7_encoding type);

	virtual void CMP_IMM(const u32 data, const ARMv7_encoding type);
	virtual void CMP_REG(const u32 data, const ARMv7_encoding type);
	virtual void CMP_RSR(const u32 data, const ARMv7_encoding type);

	virtual void EOR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void EOR_REG(const u32 data, const ARMv7_encoding type);
	virtual void EOR_RSR(const u32 data, const ARMv7_encoding type);

	virtual void IT(const u32 data, const ARMv7_encoding type);

	virtual void LDM(const u32 data, const ARMv7_encoding type);
	virtual void LDMDA(const u32 data, const ARMv7_encoding type);
	virtual void LDMDB(const u32 data, const ARMv7_encoding type);
	virtual void LDMIB(const u32 data, const ARMv7_encoding type);

	virtual void LDR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDR_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDR_REG(const u32 data, const ARMv7_encoding type);

	virtual void LDRB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDRB_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDRB_REG(const u32 data, const ARMv7_encoding type);

	virtual void LDRD_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDRD_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDRD_REG(const u32 data, const ARMv7_encoding type);

	virtual void LDRH_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDRH_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDRH_REG(const u32 data, const ARMv7_encoding type);

	virtual void LDRSB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDRSB_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDRSB_REG(const u32 data, const ARMv7_encoding type);

	virtual void LDRSH_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LDRSH_LIT(const u32 data, const ARMv7_encoding type);
	virtual void LDRSH_REG(const u32 data, const ARMv7_encoding type);

	virtual void LSL_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LSL_REG(const u32 data, const ARMv7_encoding type);

	virtual void LSR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void LSR_REG(const u32 data, const ARMv7_encoding type);

	virtual void MLA(const u32 data, const ARMv7_encoding type);
	virtual void MLS(const u32 data, const ARMv7_encoding type);

	virtual void MOV_IMM(const u32 data, const ARMv7_encoding type);
	virtual void MOV_REG(const u32 data, const ARMv7_encoding type);
	virtual void MOVT(const u32 data, const ARMv7_encoding type);

	virtual void MRS(const u32 data, const ARMv7_encoding type);
	virtual void MSR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void MSR_REG(const u32 data, const ARMv7_encoding type);

	virtual void MUL(const u32 data, const ARMv7_encoding type);

	virtual void MVN_IMM(const u32 data, const ARMv7_encoding type);
	virtual void MVN_REG(const u32 data, const ARMv7_encoding type);
	virtual void MVN_RSR(const u32 data, const ARMv7_encoding type);

	virtual void NOP(const u32 data, const ARMv7_encoding type);

	virtual void ORN_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ORN_REG(const u32 data, const ARMv7_encoding type);

	virtual void ORR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ORR_REG(const u32 data, const ARMv7_encoding type);
	virtual void ORR_RSR(const u32 data, const ARMv7_encoding type);

	virtual void PKH(const u32 data, const ARMv7_encoding type);

	virtual void POP(const u32 data, const ARMv7_encoding type);
	virtual void PUSH(const u32 data, const ARMv7_encoding type);

	virtual void QADD(const u32 data, const ARMv7_encoding type);
	virtual void QADD16(const u32 data, const ARMv7_encoding type);
	virtual void QADD8(const u32 data, const ARMv7_encoding type);
	virtual void QASX(const u32 data, const ARMv7_encoding type);
	virtual void QDADD(const u32 data, const ARMv7_encoding type);
	virtual void QDSUB(const u32 data, const ARMv7_encoding type);
	virtual void QSAX(const u32 data, const ARMv7_encoding type);
	virtual void QSUB(const u32 data, const ARMv7_encoding type);
	virtual void QSUB16(const u32 data, const ARMv7_encoding type);
	virtual void QSUB8(const u32 data, const ARMv7_encoding type);

	virtual void RBIT(const u32 data, const ARMv7_encoding type);
	virtual void REV(const u32 data, const ARMv7_encoding type);
	virtual void REV16(const u32 data, const ARMv7_encoding type);
	virtual void REVSH(const u32 data, const ARMv7_encoding type);

	virtual void ROR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void ROR_REG(const u32 data, const ARMv7_encoding type);

	virtual void RRX(const u32 data, const ARMv7_encoding type);

	virtual void RSB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void RSB_REG(const u32 data, const ARMv7_encoding type);
	virtual void RSB_RSR(const u32 data, const ARMv7_encoding type);

	virtual void RSC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void RSC_REG(const u32 data, const ARMv7_encoding type);
	virtual void RSC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void SADD16(const u32 data, const ARMv7_encoding type);
	virtual void SADD8(const u32 data, const ARMv7_encoding type);
	virtual void SASX(const u32 data, const ARMv7_encoding type);

	virtual void SBC_IMM(const u32 data, const ARMv7_encoding type);
	virtual void SBC_REG(const u32 data, const ARMv7_encoding type);
	virtual void SBC_RSR(const u32 data, const ARMv7_encoding type);

	virtual void SBFX(const u32 data, const ARMv7_encoding type);

	virtual void SDIV(const u32 data, const ARMv7_encoding type);

	virtual void SEL(const u32 data, const ARMv7_encoding type);

	virtual void SHADD16(const u32 data, const ARMv7_encoding type);
	virtual void SHADD8(const u32 data, const ARMv7_encoding type);
	virtual void SHASX(const u32 data, const ARMv7_encoding type);
	virtual void SHSAX(const u32 data, const ARMv7_encoding type);
	virtual void SHSUB16(const u32 data, const ARMv7_encoding type);
	virtual void SHSUB8(const u32 data, const ARMv7_encoding type);

	virtual void SMLA__(const u32 data, const ARMv7_encoding type);
	virtual void SMLAD(const u32 data, const ARMv7_encoding type);
	virtual void SMLAL(const u32 data, const ARMv7_encoding type);
	virtual void SMLAL__(const u32 data, const ARMv7_encoding type);
	virtual void SMLALD(const u32 data, const ARMv7_encoding type);
	virtual void SMLAW_(const u32 data, const ARMv7_encoding type);
	virtual void SMLSD(const u32 data, const ARMv7_encoding type);
	virtual void SMLSLD(const u32 data, const ARMv7_encoding type);
	virtual void SMMLA(const u32 data, const ARMv7_encoding type);
	virtual void SMMLS(const u32 data, const ARMv7_encoding type);
	virtual void SMMUL(const u32 data, const ARMv7_encoding type);
	virtual void SMUAD(const u32 data, const ARMv7_encoding type);
	virtual void SMUL__(const u32 data, const ARMv7_encoding type);
	virtual void SMULL(const u32 data, const ARMv7_encoding type);
	virtual void SMULW_(const u32 data, const ARMv7_encoding type);
	virtual void SMUSD(const u32 data, const ARMv7_encoding type);

	virtual void SSAT(const u32 data, const ARMv7_encoding type);
	virtual void SSAT16(const u32 data, const ARMv7_encoding type);
	virtual void SSAX(const u32 data, const ARMv7_encoding type);
	virtual void SSUB16(const u32 data, const ARMv7_encoding type);
	virtual void SSUB8(const u32 data, const ARMv7_encoding type);

	virtual void STM(const u32 data, const ARMv7_encoding type);
	virtual void STMDA(const u32 data, const ARMv7_encoding type);
	virtual void STMDB(const u32 data, const ARMv7_encoding type);
	virtual void STMIB(const u32 data, const ARMv7_encoding type);

	virtual void STR_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STR_REG(const u32 data, const ARMv7_encoding type);

	virtual void STRB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STRB_REG(const u32 data, const ARMv7_encoding type);

	virtual void STRD_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STRD_REG(const u32 data, const ARMv7_encoding type);

	virtual void STRH_IMM(const u32 data, const ARMv7_encoding type);
	virtual void STRH_REG(const u32 data, const ARMv7_encoding type);

	virtual void SUB_IMM(const u32 data, const ARMv7_encoding type);
	virtual void SUB_REG(const u32 data, const ARMv7_encoding type);
	virtual void SUB_RSR(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPI(const u32 data, const ARMv7_encoding type);
	virtual void SUB_SPR(const u32 data, const ARMv7_encoding type);

	virtual void SVC(const u32 data, const ARMv7_encoding type);

	virtual void SXTAB(const u32 data, const ARMv7_encoding type);
	virtual void SXTAB16(const u32 data, const ARMv7_encoding type);
	virtual void SXTAH(const u32 data, const ARMv7_encoding type);
	virtual void SXTB(const u32 data, const ARMv7_encoding type);
	virtual void SXTB16(const u32 data, const ARMv7_encoding type);
	virtual void SXTH(const u32 data, const ARMv7_encoding type);

	virtual void TB_(const u32 data, const ARMv7_encoding type);

	virtual void TEQ_IMM(const u32 data, const ARMv7_encoding type);
	virtual void TEQ_REG(const u32 data, const ARMv7_encoding type);
	virtual void TEQ_RSR(const u32 data, const ARMv7_encoding type);

	virtual void TST_IMM(const u32 data, const ARMv7_encoding type);
	virtual void TST_REG(const u32 data, const ARMv7_encoding type);
	virtual void TST_RSR(const u32 data, const ARMv7_encoding type);

	virtual void UADD16(const u32 data, const ARMv7_encoding type);
	virtual void UADD8(const u32 data, const ARMv7_encoding type);
	virtual void UASX(const u32 data, const ARMv7_encoding type);
	virtual void UBFX(const u32 data, const ARMv7_encoding type);
	virtual void UDIV(const u32 data, const ARMv7_encoding type);
	virtual void UHADD16(const u32 data, const ARMv7_encoding type);
	virtual void UHADD8(const u32 data, const ARMv7_encoding type);
	virtual void UHASX(const u32 data, const ARMv7_encoding type);
	virtual void UHSAX(const u32 data, const ARMv7_encoding type);
	virtual void UHSUB16(const u32 data, const ARMv7_encoding type);
	virtual void UHSUB8(const u32 data, const ARMv7_encoding type);
	virtual void UMAAL(const u32 data, const ARMv7_encoding type);
	virtual void UMLAL(const u32 data, const ARMv7_encoding type);
	virtual void UMULL(const u32 data, const ARMv7_encoding type);
	virtual void UQADD16(const u32 data, const ARMv7_encoding type);
	virtual void UQADD8(const u32 data, const ARMv7_encoding type);
	virtual void UQASX(const u32 data, const ARMv7_encoding type);
	virtual void UQSAX(const u32 data, const ARMv7_encoding type);
	virtual void UQSUB16(const u32 data, const ARMv7_encoding type);
	virtual void UQSUB8(const u32 data, const ARMv7_encoding type);
	virtual void USAD8(const u32 data, const ARMv7_encoding type);
	virtual void USADA8(const u32 data, const ARMv7_encoding type);
	virtual void USAT(const u32 data, const ARMv7_encoding type);
	virtual void USAT16(const u32 data, const ARMv7_encoding type);
	virtual void USAX(const u32 data, const ARMv7_encoding type);
	virtual void USUB16(const u32 data, const ARMv7_encoding type);
	virtual void USUB8(const u32 data, const ARMv7_encoding type);
	virtual void UXTAB(const u32 data, const ARMv7_encoding type);
	virtual void UXTAB16(const u32 data, const ARMv7_encoding type);
	virtual void UXTAH(const u32 data, const ARMv7_encoding type);
	virtual void UXTB(const u32 data, const ARMv7_encoding type);
	virtual void UXTB16(const u32 data, const ARMv7_encoding type);
	virtual void UXTH(const u32 data, const ARMv7_encoding type);
#endif
};
