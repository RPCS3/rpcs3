#include "stdafx.h"
#include "ARMv7DisAsm.h"

void ARMv7DisAsm::UNK(const u32 data)
{
	Write("Unknown/illegal opcode");
}

void ARMv7DisAsm::NULL_OP(const u32 data, const ARMv7_encoding type)
{
	Write("NULL");
}

void ARMv7DisAsm::PUSH(const u32 data, const ARMv7_encoding type)
{
	Write("PUSH...");
	//Write(fmt::Format("push {%s}", GetRegsListString(regs_list).c_str()));
}

void ARMv7DisAsm::POP(const u32 data, const ARMv7_encoding type)
{
	Write("POP...");
	//Write(fmt::Format("pop {%s}", GetRegsListString(regs_list).c_str()));
}

void ARMv7DisAsm::NOP(const u32 data, const ARMv7_encoding type)
{
	Write("NOP");
}

void ARMv7DisAsm::B(const u32 data, const ARMv7_encoding type)
{
	Write("B...");
	//if ((cond & 0xe) == 0xe)
	//{
	//	Write(fmt::Format("b 0x%x", DisAsmBranchTarget(imm) + intstr_size));
	//}
	//else
	//{
	//	Write(fmt::Format("b[%s] 0x%x", g_arm_cond_name[cond], DisAsmBranchTarget(imm) + intstr_size));
	//}
}

void ARMv7DisAsm::CBZ(const u32 data, const ARMv7_encoding type)
{
	Write("CBZ...");
	//Write(fmt::Format("cbz 0x%x,%s", DisAsmBranchTarget(imm) + intstr_size, g_arm_reg_name[rn]));
}

void ARMv7DisAsm::CBNZ(const u32 data, const ARMv7_encoding type)
{
	Write("CBNZ...");
	//Write(fmt::Format("cbnz 0x%x,%s", DisAsmBranchTarget(imm) + intstr_size, g_arm_reg_name[rn]));
}

void ARMv7DisAsm::BL(const u32 data, const ARMv7_encoding type)
{
	Write("BL...");
	//Write(fmt::Format("bl 0x%x", DisAsmBranchTarget(imm) + intstr_size));
}

void ARMv7DisAsm::BLX(const u32 data, const ARMv7_encoding type)
{
	Write("BLX...");
	//Write(fmt::Format("bl 0x%x", DisAsmBranchTarget(imm) + intstr_size));
}

void ARMv7DisAsm::ADC_IMM(const u32 data, const ARMv7_encoding type)
{
	Write("ADC...");
}

void ARMv7DisAsm::ADC_REG(const u32 data, const ARMv7_encoding type)
{
	Write("ADC...");
}

void ARMv7DisAsm::ADC_RSR(const u32 data, const ARMv7_encoding type)
{
	Write("ADC...");
}

void ARMv7DisAsm::ADD_IMM(const u32 data, const ARMv7_encoding type)
{
	Write("ADD...");
}

void ARMv7DisAsm::ADD_REG(const u32 data, const ARMv7_encoding type)
{
	Write("ADD...");
}

void ARMv7DisAsm::ADD_RSR(const u32 data, const ARMv7_encoding type)
{
	Write("ADD...");
}

void ARMv7DisAsm::ADD_SPI(const u32 data, const ARMv7_encoding type)
{
	Write("ADD SP...");
}

void ARMv7DisAsm::ADD_SPR(const u32 data, const ARMv7_encoding type)
{
	Write("ADD SP...");
}

void ARMv7DisAsm::MOV_IMM(const u32 data, const ARMv7_encoding type)
{
	Write("MOV...");
}

void ARMv7DisAsm::MOV_REG(const u32 data, const ARMv7_encoding type)
{
	Write("MOV...");
}

void ARMv7DisAsm::MOVT(const u32 data, const ARMv7_encoding type)
{
	Write("MOVT...");
}

void ARMv7DisAsm::SUB_IMM(const u32 data, const ARMv7_encoding type)
{
	Write("SUB...");
}

void ARMv7DisAsm::SUB_REG(const u32 data, const ARMv7_encoding type)
{
	Write("SUB...");
}

void ARMv7DisAsm::SUB_RSR(const u32 data, const ARMv7_encoding type)
{
	Write("SUB...");
}

void ARMv7DisAsm::SUB_SPI(const u32 data, const ARMv7_encoding type)
{
	Write("SUB SP...");
}

void ARMv7DisAsm::SUB_SPR(const u32 data, const ARMv7_encoding type)
{
	Write("SUB SP...");
}

void ARMv7DisAsm::STR_IMM(const u32 data, const ARMv7_encoding type)
{
	Write("STR...");
}

void ARMv7DisAsm::STR_REG(const u32 data, const ARMv7_encoding type)
{
	Write("STR...");
}
