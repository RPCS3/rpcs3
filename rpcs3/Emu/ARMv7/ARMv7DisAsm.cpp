#include "stdafx.h"
#if 0
#include "ARMv7DisAsm.h"

void ARMv7DisAsm::UNK(const u32 data)
{
	Write("Unknown/illegal opcode");
}

void ARMv7DisAsm::NULL_OP(const u32 data, const ARMv7_encoding type)
{
	Write("Illegal opcode (null)");
}

void ARMv7DisAsm::HACK(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADC_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADC_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADC_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ADD_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADD_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADD_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADD_SPI(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ADD_SPR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ADR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::AND_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::AND_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::AND_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ASR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ASR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::B(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//if ((cond & 0xe) == 0xe)
	//{
	//	Write(fmt::format("b 0x%x", DisAsmBranchTarget(imm) + intstr_size));
	//}
	//else
	//{
	//	Write(fmt::format("b[%s] 0x%x", g_arm_cond_name[cond], DisAsmBranchTarget(imm) + intstr_size));
	//}
}


void ARMv7DisAsm::BFC(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::BFI(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::BIC_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::BIC_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::BIC_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::BKPT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::BL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//Write(fmt::format("bl 0x%x", DisAsmBranchTarget(imm) + intstr_size));
}

void ARMv7DisAsm::BLX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//Write(fmt::format("bl 0x%x", DisAsmBranchTarget(imm) + intstr_size));
}

void ARMv7DisAsm::BX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::CB_Z(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//Write(fmt::format("cbz 0x%x,%s", DisAsmBranchTarget(imm) + intstr_size, g_arm_reg_name[rn]));
	//Write(fmt::format("cbnz 0x%x,%s", DisAsmBranchTarget(imm) + intstr_size, g_arm_reg_name[rn]));
}


void ARMv7DisAsm::CLZ(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::CMN_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::CMN_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::CMN_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::CMP_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::CMP_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::CMP_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::EOR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::EOR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::EOR_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::IT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDMDA(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDMDB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDMIB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDR_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDRB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRB_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRB_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDRD_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRD_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRD_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDRH_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRH_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRH_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDRSB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRSB_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRSB_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LDRSH_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRSH_LIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LDRSH_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LSL_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LSL_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::LSR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::LSR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::MLA(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MLS(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::MOV_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MOV_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MOVT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::MRS(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MSR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MSR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::MUL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::MVN_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MVN_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::MVN_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::NOP(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ORN_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ORN_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ORR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ORR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ORR_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::PKH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::POP(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//Write(fmt::format("pop {%s}", GetRegsListString(regs_list).c_str()));
}

void ARMv7DisAsm::PUSH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
	//Write(fmt::format("push {%s}", GetRegsListString(regs_list).c_str()));
}


void ARMv7DisAsm::QADD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QDADD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QDSUB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QSAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QSUB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QSUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::QSUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::RBIT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::REV(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::REV16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::REVSH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::ROR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::ROR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::RRX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::RSB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::RSB_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::RSB_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::RSC_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::RSC_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::RSC_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SBC_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SBC_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SBC_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SBFX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SDIV(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SEL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SHADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SHADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SHASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SHSAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SHSUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SHSUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SMLA__(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLAD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLAL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLAL__(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLALD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLAW_(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLSD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMLSLD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMMLA(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMMLS(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMMUL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMUAD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMUL__(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMULL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMULW_(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SMUSD(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SSAT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SSAT16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SSAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SSUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SSUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::STM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STMDA(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STMDB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STMIB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::STR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STR_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::STRB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STRB_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::STRD_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STRD_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::STRH_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::STRH_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SUB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SUB_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SUB_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SUB_SPI(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SUB_SPR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SVC(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::SXTAB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SXTAB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SXTAH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SXTB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SXTB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::SXTH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::TB_(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::TEQ_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::TEQ_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::TEQ_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::TST_IMM(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::TST_REG(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::TST_RSR(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}


void ARMv7DisAsm::UADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UBFX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UDIV(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHSAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHSUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UHSUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UMAAL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UMLAL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UMULL(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQADD16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQADD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQASX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQSAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQSUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UQSUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USAD8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USADA8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USAT(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USAT16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USAX(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USUB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::USUB8(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTAB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTAB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTAH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTB(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTB16(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}

void ARMv7DisAsm::UXTH(const u32 data, const ARMv7_encoding type)
{
	Write(__FUNCTION__);
}
#endif
