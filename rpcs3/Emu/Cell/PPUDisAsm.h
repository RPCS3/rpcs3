#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPCDisAsm.h"
#include "Emu/Cell/PPCThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

#define START_OPCODES_GROUP(x) /*x*/
#define END_OPCODES_GROUP(x) /*x*/

class PPU_DisAsm
	: public PPU_Opcodes
	, public PPC_DisAsm
{
public:
	PPCThread& CPU;

	PPU_DisAsm()
		: PPC_DisAsm(*(PPCThread*)NULL, DumpMode)
		, CPU(*(PPCThread*)NULL)
	{
	}

	PPU_DisAsm(PPCThread& cpu, DisAsmModes mode = NormalMode)
		: PPC_DisAsm(cpu, mode)
		, CPU(cpu)
	{
	}

	~PPU_DisAsm()
	{
	}

private:
	void Exit()
	{
		if(m_mode == NormalMode && !disasm_frame->exit)
		{
			disasm_frame->Close();
		}
	}

	u32 DisAsmBranchTarget(const s32 imm)
	{
		return branchTarget(m_mode == NormalMode ? CPU.PC : dump_pc, imm);
	}
	
private:
	void NULL_OP()
	{
		Write( "null" );
	}

	void NOP()
	{
		Write( "nop" );
	}

	void TDI(OP_REG to, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_INT1_R1_IMM("tdi", to, ra, simm16);
	}
	void TWI(OP_REG to, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_INT1_R1_IMM("twi", to, ra, simm16);
	}

	START_OPCODES_GROUP(G_04)
		void MFVSCR(OP_REG vd)
		{
			DisAsm_V1("mfvscr", vd);
		}
		void MTVSCR(OP_REG vb)
		{
			DisAsm_V1("mtvscr", vb);
		}
		void VADDCUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddcuw", vd, va, vb);
		}
		void VADDFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddfp", vd, va, vb);
		}
		void VADDSBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddsbs", vd, va, vb);
		}
		void VADDSHS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddshs", vd, va, vb);
		}
		void VADDSWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddsws", vd, va, vb);
		}
		void VADDUBM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddubm", vd, va, vb);
		}
		void VADDUBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vaddubs", vd, va, vb);
		}
		void VADDUHM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vadduhm", vd, va, vb);
		}
		void VADDUHS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vadduhs", vd, va, vb);
		}
		void VADDUWM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vadduwm", vd, va, vb);
		}
		void VADDUWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vadduws", vd, va, vb);
		}
		void VAND(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vand", vd, va, vb);
		}
		void VANDC(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vandc", vd, va, vb);
		}
		void VAVGSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavgsb", vd, va, vb);
		}
		void VAVGSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavgsh", vd, va, vb);
		}
		void VAVGSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavgsw", vd, va, vb);
		}
		void VAVGUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavgub", vd, va, vb);
		}
		void VAVGUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavguh", vd, va, vb);
		}
		void VAVGUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vavguw", vd, va, vb);
		}
		void VCFSX(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vcfsx", vd, vb, uimm5);
		}
		void VCFUX(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vcfux", vd, vb, uimm5);
		}
		void VCMPBFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpbfp", vd, va, vb);
		}
		void VCMPBFP_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpbfp.", vd, va, vb);
		}
		void VCMPEQFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpeqfp", vd, va, vb);
		}
		void VCMPEQFP_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpeqfp.", vd, va, vb);
		}
		void VCMPEQUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequb", vd, va, vb);
		}
		void VCMPEQUB_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequb.", vd, va, vb);
		}
		void VCMPEQUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequh", vd, va, vb);
		}
		void VCMPEQUH_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequh.", vd, va, vb);
		}
		void VCMPEQUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequw", vd, va, vb);
		}
		void VCMPEQUW_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpequw.", vd, va, vb);
		}
		void VCMPGEFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgefp", vd, va, vb);
		}
		void VCMPGEFP_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgefp.", vd, va, vb);
		}
		void VCMPGTFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtfp", vd, va, vb);
		}
		void VCMPGTFP_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtfp.", vd, va, vb);
		}
		void VCMPGTSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsb", vd, va, vb);
		}
		void VCMPGTSB_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsb.", vd, va, vb);
		}
		void VCMPGTSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsh", vd, va, vb);
		}
		void VCMPGTSH_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsh.", vd, va, vb);
		}
		void VCMPGTSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsw", vd, va, vb);
		}
		void VCMPGTSW_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtsw.", vd, va, vb);
		}
		void VCMPGTUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtub", vd, va, vb);
		}
		void VCMPGTUB_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtub.", vd, va, vb);
		}
		void VCMPGTUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtuh", vd, va, vb);
		}
		void VCMPGTUH_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtuh.", vd, va, vb);
		}
		void VCMPGTUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtuw", vd, va, vb);
		}
		void VCMPGTUW_(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vcmpgtuw.", vd, va, vb);
		}
		void VCTSXS(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vctsxs", vd, vb, uimm5);
		}
		void VCTUXS(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vctuxs", vd, vb, uimm5);
		}
		void VEXPTEFP(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vexptefp", vd, vb);
		}
		void VLOGEFP(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vlogefp", vd, vb);
		}
		void VMADDFP(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmaddfp", vd, va, vb, vc);
		}
		void VMAXFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxfp", vd, va, vb);
		}
		void VMAXSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxsb", vd, va, vb);
		}
		void VMAXSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxsh", vd, va, vb);
		}
		void VMAXSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxsw", vd, va, vb);
		}
		void VMAXUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxub", vd, va, vb);
		}
		void VMAXUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxuh", vd, va, vb);
		}
		void VMAXUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmaxuw", vd, va, vb);
		}
		void VMHADDSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmhaddshs", vd, va, vb, vc);
		}
		void VMHRADDSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmhraddshs", vd, va, vb, vc);
		}
		void VMINFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminfp", vd, va, vb);
		}
		void VMINSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminsb", vd, va, vb);
		}
		void VMINSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminsh", vd, va, vb);
		}
		void VMINSW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminsw", vd, va, vb);
		}
		void VMINUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminub", vd, va, vb);
		}
		void VMINUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminuh", vd, va, vb);
		}
		void VMINUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vminuw", vd, va, vb);
		}
		void VMLADDUHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmladduhm", vd, va, vb, vc);
		}
		void VMRGHB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrghb", vd, va, vb);
		}
		void VMRGHH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrghh", vd, va, vb);
		}
		void VMRGHW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrghw", vd, va, vb);
		}
		void VMRGLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrglb", vd, va, vb);
		}
		void VMRGLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrglh", vd, va, vb);
		}
		void VMRGLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmrglw", vd, va, vb);
		}
		void VMSUMMBM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsummbm", vd, va, vb, vc);
		}
		void VMSUMSHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsumshm", vd, va, vb, vc);
		}
		void VMSUMSHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsumshs", vd, va, vb, vc);
		}
		void VMSUMUBM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsumubm", vd, va, vb, vc);
		}
		void VMSUMUHM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsumuhm", vd, va, vb, vc);
		}
		void VMSUMUHS(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vmsumuhs", vd, va, vb, vc);
		}
		void VMULESB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmulesb", vd, va, vb);
		}
		void VMULESH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmulesh", vd, va, vb);
		}
		void VMULEUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmuleub", vd, va, vb);
		}
		void VMULEUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmuleuh", vd, va, vb);
		}
		void VMULOSB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmulosb", vd, va, vb);
		}
		void VMULOSH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmulosh", vd, va, vb);
		}
		void VMULOUB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmuloub", vd, va, vb);
		}
		void VMULOUH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vmulouh", vd, va, vb);
		}
		void VNMSUBFP(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vnmsubfp", vd, va, vb, vc);
		}
		void VNOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vnor", vd, va, vb);
		}
		void VOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vor", vd, va, vb);
		}
		void VPERM(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vperm", vd, va, vb, vc);
		}
		void VPKPX(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkpx", vd, va, vb);
		}
		void VPKSHSS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkshss", vd, va, vb);
		}
		void VPKSHUS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkshus", vd, va, vb);
		}
		void VPKSWSS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkswss", vd, va, vb);
		}
		void VPKSWUS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkswus", vd, va, vb);
		}
		void VPKUHUM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkuhum", vd, va, vb);
		}
		void VPKUHUS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkuhus", vd, va, vb);
		}
		void VPKUWUM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkuwum", vd, va, vb);
		}
		void VPKUWUS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vpkuwus", vd, va, vb);
		}
		void VREFP(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrefp", vd, vb);
		}
		void VRFIM(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrfim", vd, vb);
		}
		void VRFIN(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrfin", vd, vb);
		}
		void VRFIP(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrfip", vd, vb);
		}
		void VRFIZ(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrfiz", vd, vb);
		}
		void VRLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vrlb", vd, va, vb);
		}
		void VRLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vrlh", vd, va, vb);
		}
		void VRLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vrlw", vd, va, vb);
		}
		void VRSQRTEFP(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vrsqrtefp", vd, vb);
		}
		void VSEL(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc)
		{
			DisAsm_V4("vsel", vd, va, vb, vc);
		}
		void VSL(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsl", vd, va, vb);
		}
		void VSLB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vslb", vd, va, vb);
		}
		void VSLDOI(OP_REG vd, OP_REG va, OP_REG vb, OP_uIMM sh)
		{
			DisAsm_V3_UIMM("vsldoi", vd, va, vb, sh);
		}
		void VSLH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vslh", vd, va, vb);
		}
		void VSLO(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vslo", vd, va, vb);
		}
		void VSLW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vslw", vd, va, vb);
		}
		void VSPLTB(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vspltb", vd, vb, uimm5);
		}
		void VSPLTH(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vsplth", vd, vb, uimm5);
		}
		void VSPLTISB(OP_REG vd, OP_sIMM simm5)
		{
			DisAsm_V1_SIMM("vspltisb", vd, simm5);
		}
		void VSPLTISH(OP_REG vd, OP_sIMM simm5)
		{
			DisAsm_V1_SIMM("vspltish", vd, simm5);
		}
		void VSPLTISW(OP_REG vd, OP_sIMM simm5)
		{
			DisAsm_V1_SIMM("vspltisw", vd, simm5);
		}
		void VSPLTW(OP_REG vd, OP_uIMM uimm5, OP_REG vb)
		{
			DisAsm_V2_UIMM("vspltw", vd, vb, uimm5);
		}
		void VSR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsr", vd, va, vb);
		}
		void VSRAB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsrab", vd, va, vb);
		}
		void VSRAH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsrah", vd, va, vb);
		}
		void VSRAW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsraw", vd, va, vb);
		}
		void VSRB(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsrb", vd, va, vb);
		}
		void VSRH(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsrh", vd, va, vb);
		}
		void VSRO(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsro", vd, va, vb);
		}
		void VSRW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsrw", vd, va, vb);
		}
		void VSUBCUW(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubcuw", vd, va, vb);
		}
		void VSUBFP(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubfp", vd, va, vb);
		}
		void VSUBSBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubsbs", vd, va, vb);
		}
		void VSUBSHS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubshs", vd, va, vb);
		}
		void VSUBSWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubsws", vd, va, vb);
		}
		void VSUBUBM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsububm", vd, va, vb);
		}
		void VSUBUBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsububs", vd, va, vb);
		}
		void VSUBUHM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubuhm", vd, va, vb);
		}
		void VSUBUHS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubuhs", vd, va, vb);
		}
		void VSUBUWM(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubuwm", vd, va, vb);
		}
		void VSUBUWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsubuws", vd, va, vb);
		}
		void VSUMSWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsumsws", vd, va, vb);
		}
		void VSUM2SWS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsum2sws", vd, va, vb);
		}
		void VSUM4SBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsum4sbs", vd, va, vb);
		}
		void VSUM4SHS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsum4shs", vd, va, vb);
		}
		void VSUM4UBS(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vsum4ubs", vd, va, vb);
		}
		void VUPKHPX(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupkhpx", vd, vb);
		}
		void VUPKHSB(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupkhsb", vd, vb);
		}
		void VUPKHSH(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupkhsh", vd, vb);
		}
		void VUPKLPX(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupklpx", vd, vb);
		}
		void VUPKLSB(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupklsb", vd, vb);
		}
		void VUPKLSH(OP_REG vd, OP_REG vb)
		{
			DisAsm_V2("vupklsh", vd, vb);
		}
		void VXOR(OP_REG vd, OP_REG va, OP_REG vb)
		{
			DisAsm_V3("vxor", vd, va, vb);
		}
	END_OPCODES_GROUP(G_04);
	
	void MULLI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("mulli", rd, ra, simm16);
	}
	void SUBFIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("subfic", rd, ra, simm16);
	}
	void CMPLI(OP_REG crfd, OP_REG l, OP_REG ra, OP_uIMM uimm16)
	{
		DisAsm_CR1_R1_IMM(wxString::Format("cmpl%si", l ? "d" : "w"), crfd, ra, uimm16);
	}
	void CMPI(OP_REG crfd, OP_REG l, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_CR1_R1_IMM(wxString::Format("cmp%si", l ? "d" : "w"), crfd, ra, simm16);
	}
	void ADDIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("addic", rd, ra, simm16);
	}
	void ADDIC_(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("addic.", rd, ra, simm16);
	}
	void ADDI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		if(ra == 0)
		{
			DisAsm_R1_IMM("li", rd, simm16);
		}
		else
		{
			DisAsm_R2_IMM("addi", rd, ra, simm16);
		}
	}
	void ADDIS(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		if(ra == 0)
		{
			DisAsm_R1_IMM("lis", rd, simm16);
		}
		else
		{
			DisAsm_R2_IMM("addis", rd, ra, simm16);
		}
	}

	void BC(OP_REG bo, OP_REG bi, OP_sIMM bd, OP_REG aa, OP_REG lk)
	{
		if(m_mode == CompilerElfMode)
		{
			Write(wxString::Format("bc 0x%x, 0x%x, 0x%x, %d, %d", bo, bi, bd, aa, lk));
			return;
		}

		//TODO: aa lk
		const u8 bo0 = (bo & 0x10) ? 1 : 0;
		const u8 bo1 = (bo & 0x08) ? 1 : 0;
		const u8 bo2 = (bo & 0x04) ? 1 : 0;
		const u8 bo3 = (bo & 0x02) ? 1 : 0;
		const u8 bo4 = (bo & 0x01) ? 1 : 0;

		if(bo0 && !bo1 && !bo2 && bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdz", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdz-", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && bo3 && bo4)
		{
			DisAsm_CR_BRANCH("bdz+", bi/4, bd); return;
		}
		else if(bo0 && !bo1 && !bo2 && !bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdnz", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && !bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdnz-", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && !bo3 && bo4)
		{
			DisAsm_CR_BRANCH("bdnz+", bi/4, bd); return;
		}
		else if(!bo0 && !bo1 && bo2 && !bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne", bi/4, bd); return;
			}
		}
		else if(!bo0 && !bo1 && bo2 && bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge-", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble-", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne-", bi/4, bd); return;
			}
		}
		else if(!bo0 && !bo1 && bo2 && bo3 && bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge+", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble+", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne+", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && !bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt-", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt-", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq-", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && bo3 && bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt+", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt+", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq+", bi/4, bd); return;
			}
		}
		
		Write(wxString::Format("bc [%x:%x:%x:%x:%x], cr%d[%x], 0x%x, %d, %d", bo0, bo1, bo2, bo3, bo4, bi/4, bi%4, bd, aa, lk));
	}
	void SC(OP_sIMM sc_code)
	{
		switch(sc_code)
		{
		case 0x1: Write("HyperCall"); break;
		case 0x2: Write("sc"); break;
		case 0x22: Write("HyperCall LV1"); break;
		default: Write(wxString::Format("Unknown sc: %x", sc_code));
		}
	}
	void B(OP_sIMM ll, OP_REG aa, OP_REG lk)
	{
		if(m_mode == CompilerElfMode)
		{
			Write(wxString::Format("b 0x%x, %d, %d", ll, aa, lk));
			return;
		}

		switch(lk)
		{
			case 0:
				switch(aa)
				{
					case 0:	DisAsm_BRANCH("b", ll);		break;
					case 1:	DisAsm_BRANCH_A("ba", ll);	break;
				}
			break;
			
			case 1:
				switch(aa)
				{
					case 0: DisAsm_BRANCH("bl", ll);	break;
					case 1: DisAsm_BRANCH_A("bla", ll);	break;
				}
			break;
		}
	}
	
	START_OPCODES_GROUP(G_13)
		void MCRF(OP_REG crfd, OP_REG crfs)
		{
			DisAsm_CR2("mcrf", crfd, crfs);
		}
		void BCLR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			const u8 bo0 = (bo & 0x10) ? 1 : 0;
			const u8 bo1 = (bo & 0x08) ? 1 : 0;
			const u8 bo2 = (bo & 0x04) ? 1 : 0;
			const u8 bo3 = (bo & 0x02) ? 1 : 0;

			if(bo0 && !bo1 && bo2 && !bo3) {Write("blr"); return;}
			Write(wxString::Format("bclr [%x:%x:%x:%x], cr%d[%x], %d, %d", bo0, bo1, bo2, bo3, bi/4, bi%4, bh, lk));
		}
		void CRNOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crnor", bt, ba, bb);
		}
		void CRANDC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crandc", bt, ba, bb);
		}
		void ISYNC()
		{
			Write("isync");
		}
		void CRXOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crxor", bt, ba, bb);
		}
		void CRNAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crnand", bt, ba, bb);
		}
		void CRAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crand", bt, ba, bb);
		}
		void CREQV(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("creqv", bt, ba, bb);
		}
		void CRORC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crorc", bt, ba, bb);
		}
		void CROR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("cror", bt, ba, bb);
		}
		void BCCTR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			switch(lk)
			{
				case 0: DisAsm_INT3("bcctr", bo, bi, bh); break;
				case 1: DisAsm_INT3("bcctrl", bo, bi, bh); break;
			}
		}
	END_OPCODES_GROUP(G_13);
	
	
	void RLWIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwimi", ra, rs, sh, mb, me, rc);
	}
	void RLWINM(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwinm", ra, rs, sh, mb, me, rc);
	}
	void RLWNM(OP_REG ra, OP_REG rs, OP_REG rb, OP_REG MB, OP_REG ME, bool rc)
	{
		DisAsm_R3_INT2_RC("rlwnm", ra, rs, rb, MB, ME, rc);
	}
	void ORI(OP_REG rs, OP_REG ra, OP_uIMM uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("ori", rs, ra, uimm16);
	}
	void ORIS(OP_REG rs, OP_REG ra, OP_uIMM uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("oris", rs, ra, uimm16);
	}
	void XORI(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("xori", ra, rs, uimm16);
	}
	void XORIS(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("xoris", ra, rs, uimm16);
	}
	void ANDI_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("andi.", ra, rs, uimm16);
	}
	void ANDIS_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("andis.", ra, rs, uimm16);
	}

	START_OPCODES_GROUP(G_1e)
		void RLDICL(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			if(sh == 0)
			{
				DisAsm_R2_INT1_RC("clrldi", ra, rs, mb, rc);
			}
			else if(mb == 0)
			{
				DisAsm_R2_INT1_RC("rotldi", ra, rs, sh, rc);
			}
			else if(mb == 64 - sh)
			{
				DisAsm_R2_INT1_RC("srdi", ra, rs, mb, rc);
			}
			else
			{
				DisAsm_R2_INT2_RC("rldicl", ra, rs, sh, mb, rc);
			}
		}
		void RLDICR(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG me, bool rc)
		{
			DisAsm_R2_INT2_RC("rldicr", ra, rs, sh, me, rc);
		}
		void RLDIC(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			DisAsm_R2_INT2_RC("rldic", ra, rs, sh, mb, rc);
		}
		void RLDIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			DisAsm_R2_INT2_RC("rldimi", ra, rs, sh, mb, rc);
		}
	END_OPCODES_GROUP(G_1e);
	
	START_OPCODES_GROUP(G_1f)
		void CMP(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			DisAsm_CR1_R2(wxString::Format("cmp%s", l ? "d" : "w"), crfd, ra, rb);
		}
		void TW(OP_REG to, OP_REG ra, OP_REG rb)
		{
			DisAsm_INT1_R2("tw", to, ra, rb);
		}
		void LVSL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvsl", vd, ra, rb);
		}
		void LVEBX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvebx", vd, ra, rb);
		}
		void SUBFC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("subfc", rd, ra, rb, oe, rc);
		}
		void ADDC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("addc", rd, ra, rb, oe, rc);
		}
		void MULHDU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhdu", rd, ra, rb, rc);
		}
		void MULHWU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhwu", rd, ra, rb, rc);
		}
		void MFOCRF(OP_uIMM a, OP_REG rd, OP_uIMM crm)
		{
			if(a)
			{
				DisAsm_R1_IMM("mfocrf", rd, crm);
			}
			else
			{
				DisAsm_R1("mfcr", rd);
			}
		}
		void LWARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwarx", rd, ra, rb);
		}
		void LDX(OP_REG ra, OP_REG rs, OP_REG rb)
		{
			DisAsm_R3("ldx", ra, rs, rb);
		}
		void LWZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwzx", rd, ra, rb);
		}
		void SLW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("slw", ra, rs, rb, rc);
		}
		void CNTLZW(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("cntlzw", ra, rs, rc);
		}
		void SLD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("sld", ra, rs, rb, rc);
		}
		void AND(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("and", ra, rs, rb, rc);
		}
		void CMPL(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			DisAsm_CR1_R2(wxString::Format("cmpl%s", l ? "d" : "w"), crfd, ra, rb);
		}
		void LVSR(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvsr", vd, ra, rb);
		}
		void LVEHX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvehx", vd, ra, rb);
		}
		void SUBF(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			 DisAsm_R3_OE_RC("subf", rd, ra, rb, oe, rc);
		}
		void LDUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ldux", rd, ra, rb);
		}
		void DCBST(OP_REG ra, OP_REG rb)
		{
			DisAsm_R2("dcbst", ra, rb);
		}
		void CNTLZD(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("cntlzd", ra, rs, rc);
		}
		void ANDC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("andc", ra, rs, rb, rc);
		}
		void LVEWX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvewx", vd, ra, rb);
		}
		void MULHD(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhd", rd, ra, rb, rc);
		}
		void MULHW(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhw", rd, ra, rb, rc);
		}
		void LDARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ldarx", rd, ra, rb);
		}
		void DCBF(OP_REG ra, OP_REG rb)
		{
			DisAsm_R2("dcbf", ra, rb);
		}
		void LBZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lbzx", rd, ra, rb);
		}
		void LVX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvx", vd, ra, rb);
		}
		void NEG(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("neg", rd, ra, oe, rc);
		}
		void LBZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lbzux", rd, ra, rb);
		}
		void NOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			if(rs == rb)
			{
				DisAsm_R2_RC("not", ra, rs, rc);
			}
			else
			{
				DisAsm_R3_RC("nor", ra, rs, rb, rc);
			}
		}
		void STVEBX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvebx", vs, ra, rb);
		}
		void SUBFE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("subfe", rd, ra, rb, oe, rc);
		}
		void ADDE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("adde", rd, ra, rb, oe, rc);
		}
		void MTOCRF(OP_REG crm, OP_REG rs)
		{
			DisAsm_INT1_R1("mtocrf", crm, rs);
		}
		void STDX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdx.", rs, ra, rb);
		}
		void STWCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stwcx.", rs, ra, rb);
		}
		void STWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stwx", rs, ra, rb);
		}
		void STVEHX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvehx", vs, ra, rb);
		}
		void STDUX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdux", rs, ra, rb);
		}
		void STVEWX(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvewx", vs, ra, rb);
		}
		void ADDZE(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("addze", rd, ra, oe, rc);
		}
		void STDCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdcx.", rs, ra, rb);
		}
		void STBX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stbx", rs, ra, rb);
		}
		void STVX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvx", vd, ra, rb);
		}
		void MULLD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("mulld", rd, ra, rb, oe, rc);
		}
		void ADDME(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("addme", rd, ra, oe, rc);
		}
		void MULLW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("mullw", rd, ra, rb, oe, rc);
		}
		void DCBTST(OP_REG th, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("dcbtst", th, ra, rb);
		}
		void ADD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("add", rd, ra, rb, oe, rc);
		}
		void DCBT(OP_REG ra, OP_REG rb, OP_REG th)
		{
			DisAsm_R2("dcbt", ra, rb);
		}
		void LHZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhzx", rd, ra, rb);
		}
		void EQV(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("eqv", ra, rs, rb, rc);
		}
		void ECIWX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("eciwx", rd, ra, rb);
		}
		void LHZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhzux", rd, ra, rb);
		}
		void XOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("xor", ra, rs, rb, rc);
		}
		void MFSPR(OP_REG rd, OP_REG spr)
		{
			const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
			switch(n)
			{
			case 0x001: DisAsm_R1("mfxer", rd); break;
			case 0x008: DisAsm_R1("mflr", rd); break;
			case 0x009: DisAsm_R1("mfctr", rd); break;
			default: DisAsm_R1_IMM("mfspr", rd, spr); break;
			}
		}
		void DST(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t)
		{
			if(t)
			{
				DisAsm_R2_INT1("dstt", ra, rb, strm);
			}
			else
			{
				DisAsm_R2_INT1("dst", ra, rb, strm);
			}
		}
		void LHAX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhax", rd, ra, rb);
		}
		void LVXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvxl", vd, ra, rb);
		}
		void ABS(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("abs", rd, ra, oe, rc);
		}
		void MFTB(OP_REG rd, OP_REG spr)
		{
			const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
			switch(n)
			{
			case 268: DisAsm_R1("mftb", rd); break;
			case 269: DisAsm_R1("mftbu", rd); break;
			default: DisAsm_R1_IMM("mftb", rd, spr); break;
			}
		}
		void DSTST(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t)
		{
			if(t)
			{
				DisAsm_R2_INT1("dststt", ra, rb, strm);
			}
			else
			{
				DisAsm_R2_INT1("dstst", ra, rb, strm);
			}
		}
		void LHAUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhaux", rd, ra, rb);
		}
		void STHX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("sthx", rs, ra, rb);
		}
		void ORC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("orc", ra, rs, rb, rc);
		}
		void ECOWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ecowx", rs, ra, rb);
		}
		void OR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			if(rs==rb)
			{
				DisAsm_R2_RC("mr", ra, rb, rc);
			}
			else
			{
				DisAsm_R3_RC("or", ra, rs, rb, rc);
			}
		}
		void DIVDU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divdu", rd, ra, rb, oe, rc);
		}
		void DIVWU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divwu", rd, ra, rb, oe, rc);
		}
		void MTSPR(OP_REG spr, OP_REG rs)
		{
			const u32 n = (spr & 0x1f) + ((spr >> 5) & 0x1f);

			switch(n)
			{
			case 0x001: DisAsm_R1("mtxer", rs); break;
			case 0x008: DisAsm_R1("mtlr", rs); break;
			case 0x009: DisAsm_R1("mtctr", rs); break;
			default: DisAsm_IMM_R1("mtspr", spr, rs); break;
			}
		}
		/*0x1d6*///DCBI
		void NAND(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("nand", ra, rs, rb, rc);
		}
		void STVXL(OP_REG vs, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvxl", vs, ra, rb);
		}
		void DIVD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divd", rd, ra, rb, oe, rc);
		}
		void DIVW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divw", rd, ra, rb, oe, rc);
		}
		void LVLX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvlx", vd, ra, rb);
		}
		void LWBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwbrx", rd, ra, rb);
		}
		void LFSX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfsx", frd, ra, rb);
		}
		void SRW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srw", ra, rs, rb, rc);
		}
		void SRD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srd", ra, rs, rb, rc);
		}
		void LVRX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvrx", vd, ra, rb);
		}
		void LFSUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfsux", frd, ra, rb);
		}
		void SYNC(OP_uIMM l)
		{
			DisAsm_INT1("sync", l);
		}
		void LFDX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfdx", frd, ra, rb);
		}
		void LFDUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfdux", frd, ra, rb);
		}
		void STVLX(OP_REG sd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvlx", sd, ra, rb);
		}
		void STFSX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("stfsx", frs, ra, rb);
		}
		void STVRX(OP_REG sd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvrx", sd, ra, rb);
		}
		void STFDX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("stfdx", frs, ra, rb);
		}
		void LVLXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvlxl", vd, ra, rb);
		}
		void LHBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhbrx", rd, ra, rb);
		}
		void SRAW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("sraw", ra, rs, rb, rc);
		}
		void SRAD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srad", ra, rs, rb, rc);
		}
		void LVRXL(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvrxl", vd, ra, rb);
		}
		void DSS(OP_uIMM strm, OP_uIMM a)
		{
			if(a)
			{
				Write("dssall");
			}
			else
			{
				DisAsm_INT1("dss", strm);
			}
		}
		void SRAWI(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("srawi", ra, rs, sh, rc);
		}
		void SRADI1(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
		}
		void SRADI2(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
		}
		void EIEIO()
		{
			Write("eieio");
		}
		void STVLXL(OP_REG sd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvlxl", sd, ra, rb);
		}
		void EXTSH(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsh", ra, rs, rc);
		}
		void STVRXL(OP_REG sd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvrxl", sd, ra, rb);
		}
		void EXTSB(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsb", ra, rs, rc);
		}
		void STFIWX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("stfiwx", frs, ra, rb);
		}
		void EXTSW(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsw", ra, rs, rc);
		}
		/*0x3d6*///ICBI
		void DCBZ(OP_REG ra, OP_REG rs)
		{
			DisAsm_R2("dcbz", ra, rs);
		}
	END_OPCODES_GROUP(G_1f);
	
	void LWZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lwz", rd, ra, d);
	}
	void LWZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lwzu", rd, ra, d);
	}
	void LBZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lbz", rd, ra, d);
	}
	void LBZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lbzu", rd, ra, d);
	}
	void STW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stw", rs, ra, d);
	}
	void STWU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stwu", rs, ra, d);
	}
	void STB(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stb", rs, ra, d);
	}
	void STBU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stbu", rs, ra, d);
	}
	void LHZ(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lhz", rs, ra, d);
	}
	void LHZU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lhzu", rs, ra, d);
	}
	void STH(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("sth", rs, ra, d);
	}
	void STHU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("sthu", rs, ra, d);
	}
	void LMW(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lmw", rd, ra, d);
	}
	void STMW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stmw", rs, ra, d);
	}
	void LFS(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("lfs", frd, d, ra);
	}
	void LFSU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		DisAsm_F1_IMM_R1("lfsu", frd, ds, ra);
	}
	void LFD(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("lfd", frd, d, ra);
	}
	void LFDU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		DisAsm_F1_IMM_R1("lfdu", frd, ds, ra);
	}
	void STFS(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfs", frs, d, ra);
	}
	void STFSU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfsu", frs, d, ra);
	}
	void STFD(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfd", frs, d, ra);
	}
	void STFDU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfdu", frs, d, ra);
	}
	
	START_OPCODES_GROUP(G_3a)
		void LD(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("ld", rd, ra, ds);
		}
		void LDU(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("ldu", rd, ra, ds);
		}
	END_OPCODES_GROUP(G_3a);

	START_OPCODES_GROUP(G_3b)
		void FDIVS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fdivs", frd, fra, frb, rc);
		}
		void FSUBS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fsubs", frd, fra, frb, rc);
		}
		void FADDS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fadds", frd, fra, frb, rc);
		}
		void FSQRTS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fsqrts", frd, frb, rc);
		}
		void FRES(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fres", frd, frb, rc);
		}
		void FMULS(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			DisAsm_F3_RC("fmuls", frd, fra, frc, rc);
		}
		void FMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmadds", frd, fra, frc, frb, rc);
		}
		void FMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmsubs", frd, fra, frc, frb, rc);
		}
		void FNMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmsubs", frd, fra, frc, frb, rc);
		}
		void FNMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmadds", frd, fra, frc, frb, rc);
		}
	END_OPCODES_GROUP(G_3b);
	
	START_OPCODES_GROUP(G_3e)
		void STD(OP_REG rs, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("std", rs, ra, ds);
		}
		void STDU(OP_REG rs, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("stdu", rs, ra, ds);
		}
	END_OPCODES_GROUP(G_3e);

	START_OPCODES_GROUP(G_3f)
		void MTFSB1(OP_REG bt, bool rc)
		{
			DisAsm_F1_RC("mtfsb1", bt, rc);
		}
		void MCRFS(OP_REG bf, OP_REG bfa)
		{
			DisAsm_F2("mcrfs", bf, bfa);
		}
		void MTFSB0(OP_REG bt, bool rc)
		{
			DisAsm_F1_RC("mtfsb0", bt, rc);
		}
		void MTFSFI(OP_REG crfd, OP_REG i, bool rc)
		{
			DisAsm_F2_RC("mtfsfi", crfd, i, rc);
		}
		void MFFS(OP_REG frd, bool rc)
		{
			DisAsm_F1_RC("mffs", frd, rc);
		}
		void MTFSF(OP_REG flm, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("mtfsf", flm, frb, rc);
		}
		void FCMPU(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			DisAsm_CR1_F2("fcmpu", crfd, fra, frb);
		}
		void FRSP(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("frsp", frd, frb, rc);
		}
		void FCTIW(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctiw", frd, frb, rc);
		}
		void FCTIWZ(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctiwz", frd, frb, rc);
		}
		void FDIV(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fdiv", frd, fra, frb, rc);
		}
		void FSUB(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fsub", frd, fra, frb, rc);
		}
		void FADD(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fadd", frd, fra, frb, rc);
		}
		void FSQRT(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fsqrt", frd, frb, rc);
		}
		void FSEL(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fsel", frd, fra, frc, frb, rc);
		}
		void FMUL(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			DisAsm_F3_RC("fmul", frd, fra, frc, rc);
		}
		void FRSQRTE(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("frsqrte", frd, frb, rc);
		}
		void FMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmsub", frd, fra, frc, frb, rc);
		}
		void FMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmadd", frd, fra, frc, frb, rc);
		}
		void FNMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmsub", frd, fra, frc, frb, rc);
		}
		void FNMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmadd", frd, fra, frc, frb, rc);
		}
		void FCMPO(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			DisAsm_F3("fcmpo", crfd, fra, frb);
		}
		void FNEG(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fneg", frd, frb, rc);
		}
		void FMR(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fmr", frd, frb, rc);
		}
		void FNABS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fnabs", frd, frb, rc);
		}
		void FABS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fabs", frd, frb, rc);
		}
		void FCTID(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctid", frd, frb, rc);
		}
		void FCTIDZ(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctidz", frd, frb, rc);
		}
		void FCFID(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fcfid", frd, frb, rc);
		}
	END_OPCODES_GROUP(G_3f);

	void UNK(const u32 code, const u32 opcode, const u32 gcode)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP