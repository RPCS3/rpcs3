#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPCDisAsm.h"
#include "Emu/Cell/PPCThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

class PPUDisAsm
	: public PPUOpcodes
	, public PPCDisAsm
{
public:
	PPUDisAsm(CPUDisAsmMode mode) : PPCDisAsm(mode)
	{
	}

private:
	u32 DisAsmBranchTarget(const s32 imm)
	{
		return branchTarget(dump_pc, imm);
	}

private:
	void DisAsm_V4(const std::string& op, u32 v0, u32 v1, u32 v2, u32 v3)
	{
		Write(fmt::Format("%s v%d,v%d,v%d,v%d", FixOp(op).c_str(), v0, v1, v2, v3));
	}
	void DisAsm_V3_UIMM(const std::string& op, u32 v0, u32 v1, u32 v2, u32 uimm)
	{
		Write(fmt::Format("%s v%d,v%d,v%d,%u #%x", FixOp(op).c_str(), v0, v1, v2, uimm, uimm));
	}
	void DisAsm_V3(const std::string& op, u32 v0, u32 v1, u32 v2)
	{
		Write(fmt::Format("%s v%d,v%d,v%d", FixOp(op).c_str(), v0, v1, v2));
	}
	void DisAsm_V2_UIMM(const std::string& op, u32 v0, u32 v1, u32 uimm)
	{
		Write(fmt::Format("%s v%d,v%d,%u #%x", FixOp(op).c_str(), v0, v1, uimm, uimm));
	}
	void DisAsm_V2(const std::string& op, u32 v0, u32 v1)
	{
		Write(fmt::Format("%s v%d,v%d", FixOp(op).c_str(), v0, v1));
	}
	void DisAsm_V1_SIMM(const std::string& op, u32 v0, s32 simm)
	{
		Write(fmt::Format("%s v%d,%d #%x", FixOp(op).c_str(), v0, simm, simm));
	}
	void DisAsm_V1(const std::string& op, u32 v0)
	{
		Write(fmt::Format("%s v%d", FixOp(op).c_str(), v0));
	}
	void DisAsm_V1_R2(const std::string& op, u32 v0, u32 r1, u32 r2)
	{
		Write(fmt::Format("%s v%d,r%d,r%d", FixOp(op).c_str(), v0, r1, r2));
	}
	void DisAsm_CR1_F2_RC(const std::string& op, u32 cr0, u32 f0, u32 f1, bool rc)
	{
		Write(fmt::Format("%s%s cr%d,f%d,f%d", FixOp(op).c_str(), (rc ? "." : ""), cr0, f0, f1));
	}
	void DisAsm_CR1_F2(const std::string& op, u32 cr0, u32 f0, u32 f1)
	{
		DisAsm_CR1_F2_RC(op, cr0, f0, f1, false);
	}
	void DisAsm_INT1_R2(const std::string& op, u32 i0, u32 r0, u32 r1)
	{
		Write(fmt::Format("%s %d,r%d,r%d", FixOp(op).c_str(), i0, r0, r1));
	}
	void DisAsm_INT1_R1_IMM(const std::string& op, u32 i0, u32 r0, s32 imm0)
	{
		Write(fmt::Format("%s %d,r%d,%d #%x", FixOp(op).c_str(), i0, r0, imm0, imm0));
	}
	void DisAsm_INT1_R1_RC(const std::string& op, u32 i0, u32 r0, bool rc)
	{
		Write(fmt::Format("%s%s %d,r%d", FixOp(op).c_str(), (rc ? "." : ""), i0, r0));
	}
	void DisAsm_INT1_R1(const std::string& op, u32 i0, u32 r0)
	{
		DisAsm_INT1_R1_RC(op, i0, r0, false);
	}
	void DisAsm_F4_RC(const std::string& op, u32 f0, u32 f1, u32 f2, u32 f3, bool rc)
	{
		Write(fmt::Format("%s%s f%d,f%d,f%d,f%d", FixOp(op).c_str(), (rc ? "." : ""), f0, f1, f2, f3));
	}
	void DisAsm_F3_RC(const std::string& op, u32 f0, u32 f1, u32 f2, bool rc)
	{
		Write(fmt::Format("%s%s f%d,f%d,f%d", FixOp(op).c_str(), (rc ? "." : ""), f0, f1, f2));
	}
	void DisAsm_F3(const std::string& op, u32 f0, u32 f1, u32 f2)
	{
		DisAsm_F3_RC(op, f0, f1, f2, false);
	}
	void DisAsm_F2_RC(const std::string& op, u32 f0, u32 f1, bool rc)
	{
		Write(fmt::Format("%s%s f%d,f%d", FixOp(op).c_str(), (rc ? "." : ""), f0, f1));
	}
	void DisAsm_F2(const std::string& op, u32 f0, u32 f1)
	{
		DisAsm_F2_RC(op, f0, f1, false);
	}
	void DisAsm_F1_R2(const std::string& op, u32 f0, u32 r0, u32 r1)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::Format("%s f%d,r%d,r%d", FixOp(op).c_str(), f0, r0, r1));
			return;
		}

		Write(fmt::Format("%s f%d,r%d(r%d)", FixOp(op).c_str(), f0, r0, r1));
	}
	void DisAsm_F1_IMM_R1_RC(const std::string& op, u32 f0, s32 imm0, u32 r0, bool rc)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::Format("%s%s f%d,r%d,%d #%x", FixOp(op).c_str(), (rc ? "." : ""), f0, r0, imm0, imm0));
			return;
		}

		Write(fmt::Format("%s%s f%d,%d(r%d) #%x", FixOp(op).c_str(), (rc ? "." : ""), f0, imm0, r0, imm0));
	}
	void DisAsm_F1_IMM_R1(const std::string& op, u32 f0, s32 imm0, u32 r0)
	{
		DisAsm_F1_IMM_R1_RC(op, f0, imm0, r0, false);
	}
	void DisAsm_F1_RC(const std::string& op, u32 f0, bool rc)
	{
		Write(fmt::Format("%s%s f%d", FixOp(op).c_str(), (rc ? "." : ""), f0));
	}
	void DisAsm_R1_RC(const std::string& op, u32 r0, bool rc)
	{
		Write(fmt::Format("%s%s r%d", FixOp(op).c_str(), (rc ? "." : ""), r0));
	}
	void DisAsm_R1(const std::string& op, u32 r0)
	{
		DisAsm_R1_RC(op, r0, false);
	}
	void DisAsm_R2_OE_RC(const std::string& op, u32 r0, u32 r1, u32 oe, bool rc)
	{
		Write(fmt::Format("%s%s%s r%d,r%d", FixOp(op).c_str(), (oe ? "o" : ""), (rc ? "." : ""), r0, r1));
	}
	void DisAsm_R2_RC(const std::string& op, u32 r0, u32 r1, bool rc)
	{
		DisAsm_R2_OE_RC(op, r0, r1, false, rc);
	}
	void DisAsm_R2(const std::string& op, u32 r0, u32 r1)
	{
		DisAsm_R2_RC(op, r0, r1, false);
	}
	void DisAsm_R3_OE_RC(const std::string& op, u32 r0, u32 r1, u32 r2, u32 oe, bool rc)
	{
		Write(fmt::Format("%s%s%s r%d,r%d,r%d", FixOp(op).c_str(), (oe ? "o" : ""), (rc ? "." : ""), r0, r1, r2));
	}
	void DisAsm_R3_INT2_RC(const std::string& op, u32 r0, u32 r1, u32 r2, s32 i0, s32 i1, bool rc)
	{
		Write(fmt::Format("%s%s r%d,r%d,r%d,%d,%d", FixOp(op).c_str(), (rc ? "." : ""), r0, r1, r2, i0, i1));
	}
	void DisAsm_R3_RC(const std::string& op, u32 r0, u32 r1, u32 r2, bool rc)
	{
		DisAsm_R3_OE_RC(op, r0, r1, r2, false, rc);
	}
	void DisAsm_R3(const std::string& op, u32 r0, u32 r1, u32 r2)
	{
		DisAsm_R3_RC(op, r0, r1, r2, false);
	}
	void DisAsm_R2_INT3_RC(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2, bool rc)
	{
		Write(fmt::Format("%s%s r%d,r%d,%d,%d,%d", FixOp(op).c_str(), (rc ? "." : ""), r0, r1, i0, i1, i2));
	}
	void DisAsm_R2_INT3(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2)
	{
		DisAsm_R2_INT3_RC(op, r0, r1, i0, i1, i2, false);
	}
	void DisAsm_R2_INT2_RC(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, bool rc)
	{
		Write(fmt::Format("%s%s r%d,r%d,%d,%d", FixOp(op).c_str(), (rc ? "." : ""), r0, r1, i0, i1));
	}
	void DisAsm_R2_INT2(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1)
	{
		DisAsm_R2_INT2_RC(op, r0, r1, i0, i1, false);
	}
	void DisAsm_R2_INT1_RC(const std::string& op, u32 r0, u32 r1, s32 i0, bool rc)
	{
		Write(fmt::Format("%s%s r%d,r%d,%d", FixOp(op).c_str(), (rc ? "." : ""), r0, r1, i0));
	}
	void DisAsm_R2_INT1(const std::string& op, u32 r0, u32 r1, s32 i0)
	{
		DisAsm_R2_INT1_RC(op, r0, r1, i0, false);
	}
	void DisAsm_R2_IMM(const std::string& op, u32 r0, u32 r1, s32 imm0)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::Format("%s r%d,r%d,%d  #%x", FixOp(op).c_str(), r0, r1, imm0, imm0));
			return;
		}

		Write(fmt::Format("%s r%d,%d(r%d)  #%x", FixOp(op).c_str(), r0, imm0, r1, imm0));
	}
	void DisAsm_R1_IMM(const std::string& op, u32 r0, s32 imm0)
	{
		Write(fmt::Format("%s r%d,%d  #%x", FixOp(op).c_str(), r0, imm0, imm0));
	}
	void DisAsm_IMM_R1(const std::string& op, s32 imm0, u32 r0)
	{
		Write(fmt::Format("%s %d,r%d  #%x", FixOp(op).c_str(), imm0, r0, imm0));
	}
	void DisAsm_CR1_R1_IMM(const std::string& op, u32 cr0, u32 r0, s32 imm0)
	{
		Write(fmt::Format("%s cr%d,r%d,%d  #%x", FixOp(op).c_str(), cr0, r0, imm0, imm0));
	}
	void DisAsm_CR1_R2_RC(const std::string& op, u32 cr0, u32 r0, u32 r1, bool rc)
	{
		Write(fmt::Format("%s%s cr%d,r%d,r%d", FixOp(op).c_str(), (rc ? "." : ""), cr0, r0, r1));
	}
	void DisAsm_CR1_R2(const std::string& op, u32 cr0, u32 r0, u32 r1)
	{
		DisAsm_CR1_R2_RC(op, cr0, r0, r1, false);
	}
	void DisAsm_CR2(const std::string& op, u32 cr0, u32 cr1)
	{
		Write(fmt::Format("%s cr%d,cr%d", FixOp(op).c_str(), cr0, cr1));
	}
	void DisAsm_INT3(const std::string& op, const int i0, const int i1, const int i2)
	{
		Write(fmt::Format("%s %d,%d,%d", FixOp(op).c_str(), i0, i1, i2));
	}
	void DisAsm_INT1(const std::string& op, const int i0)
	{
		Write(fmt::Format("%s %d", FixOp(op).c_str(), i0));
	}
	void DisAsm_BRANCH(const std::string& op, const int pc)
	{
		Write(fmt::Format("%s 0x%x", FixOp(op).c_str(), DisAsmBranchTarget(pc)));
	}
	void DisAsm_BRANCH_A(const std::string& op, const int pc)
	{
		Write(fmt::Format("%s 0x%x", FixOp(op).c_str(), pc));
	}
	void DisAsm_B2_BRANCH(const std::string& op, u32 b0, u32 b1, const int pc)
	{
		Write(fmt::Format("%s %d,%d,0x%x ", FixOp(op).c_str(), b0, b1, DisAsmBranchTarget(pc)));
	}
	void DisAsm_CR_BRANCH(const std::string& op, u32 cr, const int pc)
	{
		Write(fmt::Format("%s cr%d,0x%x ", FixOp(op).c_str(), cr, DisAsmBranchTarget(pc)));
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

	void TDI(u32 to, u32 ra, s32 simm16)
	{
		DisAsm_INT1_R1_IMM("tdi", to, ra, simm16);
	}
	void TWI(u32 to, u32 ra, s32 simm16)
	{
		DisAsm_INT1_R1_IMM("twi", to, ra, simm16);
	}
	void MFVSCR(u32 vd)
	{
		DisAsm_V1("mfvscr", vd);
	}
	void MTVSCR(u32 vb)
	{
		DisAsm_V1("mtvscr", vb);
	}
	void VADDCUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddcuw", vd, va, vb);
	}
	void VADDFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddfp", vd, va, vb);
	}
	void VADDSBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddsbs", vd, va, vb);
	}
	void VADDSHS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddshs", vd, va, vb);
	}
	void VADDSWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddsws", vd, va, vb);
	}
	void VADDUBM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddubm", vd, va, vb);
	}
	void VADDUBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vaddubs", vd, va, vb);
	}
	void VADDUHM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vadduhm", vd, va, vb);
	}
	void VADDUHS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vadduhs", vd, va, vb);
	}
	void VADDUWM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vadduwm", vd, va, vb);
	}
	void VADDUWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vadduws", vd, va, vb);
	}
	void VAND(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vand", vd, va, vb);
	}
	void VANDC(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vandc", vd, va, vb);
	}
	void VAVGSB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavgsb", vd, va, vb);
	}
	void VAVGSH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavgsh", vd, va, vb);
	}
	void VAVGSW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavgsw", vd, va, vb);
	}
	void VAVGUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavgub", vd, va, vb);
	}
	void VAVGUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavguh", vd, va, vb);
	}
	void VAVGUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vavguw", vd, va, vb);
	}
	void VCFSX(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vcfsx", vd, vb, uimm5);
	}
	void VCFUX(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vcfux", vd, vb, uimm5);
	}
	void VCMPBFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpbfp", vd, va, vb);
	}
	void VCMPBFP_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpbfp.", vd, va, vb);
	}
	void VCMPEQFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpeqfp", vd, va, vb);
	}
	void VCMPEQFP_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpeqfp.", vd, va, vb);
	}
	void VCMPEQUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequb", vd, va, vb);
	}
	void VCMPEQUB_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequb.", vd, va, vb);
	}
	void VCMPEQUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequh", vd, va, vb);
	}
	void VCMPEQUH_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequh.", vd, va, vb);
	}
	void VCMPEQUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequw", vd, va, vb);
	}
	void VCMPEQUW_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpequw.", vd, va, vb);
	}
	void VCMPGEFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgefp", vd, va, vb);
	}
	void VCMPGEFP_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgefp.", vd, va, vb);
	}
	void VCMPGTFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtfp", vd, va, vb);
	}
	void VCMPGTFP_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtfp.", vd, va, vb);
	}
	void VCMPGTSB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsb", vd, va, vb);
	}
	void VCMPGTSB_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsb.", vd, va, vb);
	}
	void VCMPGTSH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsh", vd, va, vb);
	}
	void VCMPGTSH_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsh.", vd, va, vb);
	}
	void VCMPGTSW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsw", vd, va, vb);
	}
	void VCMPGTSW_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtsw.", vd, va, vb);
	}
	void VCMPGTUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtub", vd, va, vb);
	}
	void VCMPGTUB_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtub.", vd, va, vb);
	}
	void VCMPGTUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtuh", vd, va, vb);
	}
	void VCMPGTUH_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtuh.", vd, va, vb);
	}
	void VCMPGTUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtuw", vd, va, vb);
	}
	void VCMPGTUW_(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vcmpgtuw.", vd, va, vb);
	}
	void VCTSXS(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vctsxs", vd, vb, uimm5);
	}
	void VCTUXS(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vctuxs", vd, vb, uimm5);
	}
	void VEXPTEFP(u32 vd, u32 vb)
	{
		DisAsm_V2("vexptefp", vd, vb);
	}
	void VLOGEFP(u32 vd, u32 vb)
	{
		DisAsm_V2("vlogefp", vd, vb);
	}
	void VMADDFP(u32 vd, u32 va, u32 vc, u32 vb)
	{
		DisAsm_V4("vmaddfp", vd, va, vc, vb);
	}
	void VMAXFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxfp", vd, va, vb);
	}
	void VMAXSB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxsb", vd, va, vb);
	}
	void VMAXSH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxsh", vd, va, vb);
	}
	void VMAXSW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxsw", vd, va, vb);
	}
	void VMAXUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxub", vd, va, vb);
	}
	void VMAXUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxuh", vd, va, vb);
	}
	void VMAXUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmaxuw", vd, va, vb);
	}
	void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmhaddshs", vd, va, vb, vc);
	}
	void VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmhraddshs", vd, va, vb, vc);
	}
	void VMINFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminfp", vd, va, vb);
	}
	void VMINSB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminsb", vd, va, vb);
	}
	void VMINSH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminsh", vd, va, vb);
	}
	void VMINSW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminsw", vd, va, vb);
	}
	void VMINUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminub", vd, va, vb);
	}
	void VMINUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminuh", vd, va, vb);
	}
	void VMINUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vminuw", vd, va, vb);
	}
	void VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmladduhm", vd, va, vb, vc);
	}
	void VMRGHB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrghb", vd, va, vb);
	}
	void VMRGHH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrghh", vd, va, vb);
	}
	void VMRGHW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrghw", vd, va, vb);
	}
	void VMRGLB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrglb", vd, va, vb);
	}
	void VMRGLH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrglh", vd, va, vb);
	}
	void VMRGLW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmrglw", vd, va, vb);
	}
	void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsummbm", vd, va, vb, vc);
	}
	void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsumshm", vd, va, vb, vc);
	}
	void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsumshs", vd, va, vb, vc);
	}
	void VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsumubm", vd, va, vb, vc);
	}
	void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsumuhm", vd, va, vb, vc);
	}
	void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vmsumuhs", vd, va, vb, vc);
	}
	void VMULESB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmulesb", vd, va, vb);
	}
	void VMULESH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmulesh", vd, va, vb);
	}
	void VMULEUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmuleub", vd, va, vb);
	}
	void VMULEUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmuleuh", vd, va, vb);
	}
	void VMULOSB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmulosb", vd, va, vb);
	}
	void VMULOSH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmulosh", vd, va, vb);
	}
	void VMULOUB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmuloub", vd, va, vb);
	}
	void VMULOUH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vmulouh", vd, va, vb);
	}
	void VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb)
	{
		DisAsm_V4("vnmsubfp", vd, va, vc, vb);
	}
	void VNOR(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vnor", vd, va, vb);
	}
	void VOR(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vor", vd, va, vb);
	}
	void VPERM(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vperm", vd, va, vb, vc);
	}
	void VPKPX(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkpx", vd, va, vb);
	}
	void VPKSHSS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkshss", vd, va, vb);
	}
	void VPKSHUS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkshus", vd, va, vb);
	}
	void VPKSWSS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkswss", vd, va, vb);
	}
	void VPKSWUS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkswus", vd, va, vb);
	}
	void VPKUHUM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkuhum", vd, va, vb);
	}
	void VPKUHUS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkuhus", vd, va, vb);
	}
	void VPKUWUM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkuwum", vd, va, vb);
	}
	void VPKUWUS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vpkuwus", vd, va, vb);
	}
	void VREFP(u32 vd, u32 vb)
	{
		DisAsm_V2("vrefp", vd, vb);
	}
	void VRFIM(u32 vd, u32 vb)
	{
		DisAsm_V2("vrfim", vd, vb);
	}
	void VRFIN(u32 vd, u32 vb)
	{
		DisAsm_V2("vrfin", vd, vb);
	}
	void VRFIP(u32 vd, u32 vb)
	{
		DisAsm_V2("vrfip", vd, vb);
	}
	void VRFIZ(u32 vd, u32 vb)
	{
		DisAsm_V2("vrfiz", vd, vb);
	}
	void VRLB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vrlb", vd, va, vb);
	}
	void VRLH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vrlh", vd, va, vb);
	}
	void VRLW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vrlw", vd, va, vb);
	}
	void VRSQRTEFP(u32 vd, u32 vb)
	{
		DisAsm_V2("vrsqrtefp", vd, vb);
	}
	void VSEL(u32 vd, u32 va, u32 vb, u32 vc)
	{
		DisAsm_V4("vsel", vd, va, vb, vc);
	}
	void VSL(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsl", vd, va, vb);
	}
	void VSLB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vslb", vd, va, vb);
	}
	void VSLDOI(u32 vd, u32 va, u32 vb, u32 sh)
	{
		DisAsm_V3_UIMM("vsldoi", vd, va, vb, sh);
	}
	void VSLH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vslh", vd, va, vb);
	}
	void VSLO(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vslo", vd, va, vb);
	}
	void VSLW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vslw", vd, va, vb);
	}
	void VSPLTB(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vspltb", vd, vb, uimm5);
	}
	void VSPLTH(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vsplth", vd, vb, uimm5);
	}
	void VSPLTISB(u32 vd, s32 simm5)
	{
		DisAsm_V1_SIMM("vspltisb", vd, simm5);
	}
	void VSPLTISH(u32 vd, s32 simm5)
	{
		DisAsm_V1_SIMM("vspltish", vd, simm5);
	}
	void VSPLTISW(u32 vd, s32 simm5)
	{
		DisAsm_V1_SIMM("vspltisw", vd, simm5);
	}
	void VSPLTW(u32 vd, u32 uimm5, u32 vb)
	{
		DisAsm_V2_UIMM("vspltw", vd, vb, uimm5);
	}
	void VSR(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsr", vd, va, vb);
	}
	void VSRAB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsrab", vd, va, vb);
	}
	void VSRAH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsrah", vd, va, vb);
	}
	void VSRAW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsraw", vd, va, vb);
	}
	void VSRB(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsrb", vd, va, vb);
	}
	void VSRH(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsrh", vd, va, vb);
	}
	void VSRO(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsro", vd, va, vb);
	}
	void VSRW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsrw", vd, va, vb);
	}
	void VSUBCUW(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubcuw", vd, va, vb);
	}
	void VSUBFP(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubfp", vd, va, vb);
	}
	void VSUBSBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubsbs", vd, va, vb);
	}
	void VSUBSHS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubshs", vd, va, vb);
	}
	void VSUBSWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubsws", vd, va, vb);
	}
	void VSUBUBM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsububm", vd, va, vb);
	}
	void VSUBUBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsububs", vd, va, vb);
	}
	void VSUBUHM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubuhm", vd, va, vb);
	}
	void VSUBUHS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubuhs", vd, va, vb);
	}
	void VSUBUWM(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubuwm", vd, va, vb);
	}
	void VSUBUWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsubuws", vd, va, vb);
	}
	void VSUMSWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsumsws", vd, va, vb);
	}
	void VSUM2SWS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsum2sws", vd, va, vb);
	}
	void VSUM4SBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsum4sbs", vd, va, vb);
	}
	void VSUM4SHS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsum4shs", vd, va, vb);
	}
	void VSUM4UBS(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vsum4ubs", vd, va, vb);
	}
	void VUPKHPX(u32 vd, u32 vb)
	{
		DisAsm_V2("vupkhpx", vd, vb);
	}
	void VUPKHSB(u32 vd, u32 vb)
	{
		DisAsm_V2("vupkhsb", vd, vb);
	}
	void VUPKHSH(u32 vd, u32 vb)
	{
		DisAsm_V2("vupkhsh", vd, vb);
	}
	void VUPKLPX(u32 vd, u32 vb)
	{
		DisAsm_V2("vupklpx", vd, vb);
	}
	void VUPKLSB(u32 vd, u32 vb)
	{
		DisAsm_V2("vupklsb", vd, vb);
	}
	void VUPKLSH(u32 vd, u32 vb)
	{
		DisAsm_V2("vupklsh", vd, vb);
	}
	void VXOR(u32 vd, u32 va, u32 vb)
	{
		DisAsm_V3("vxor", vd, va, vb);
	}
	void MULLI(u32 rd, u32 ra, s32 simm16)
	{
		DisAsm_R2_IMM("mulli", rd, ra, simm16);
	}
	void SUBFIC(u32 rd, u32 ra, s32 simm16)
	{
		DisAsm_R2_IMM("subfic", rd, ra, simm16);
	}
	void CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16)
	{
		DisAsm_CR1_R1_IMM(fmt::Format("cmpl%si", (l ? "d" : "w")), crfd, ra, uimm16);
	}
	void CMPI(u32 crfd, u32 l, u32 ra, s32 simm16)
	{
		DisAsm_CR1_R1_IMM(fmt::Format("cmp%si", (l ? "d" : "w")), crfd, ra, simm16);
	}
	void ADDIC(u32 rd, u32 ra, s32 simm16)
	{
		DisAsm_R2_IMM("addic", rd, ra, simm16);
	}
	void ADDIC_(u32 rd, u32 ra, s32 simm16)
	{
		DisAsm_R2_IMM("addic.", rd, ra, simm16);
	}
	void ADDI(u32 rd, u32 ra, s32 simm16)
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
	void ADDIS(u32 rd, u32 ra, s32 simm16)
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
	void BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::Format("bc 0x%x, 0x%x, 0x%x, %d, %d", bo, bi, bd, aa, lk));
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
		
		Write(fmt::Format("bc [%x:%x:%x:%x:%x], cr%d[%x], 0x%x, %d, %d", bo0, bo1, bo2, bo3, bo4, bi/4, bi%4, bd, aa, lk));
	}
	void SC(s32 sc_code)
	{
		switch(sc_code)
		{
		case 0x1: Write("HyperCall"); break;
		case 0x2: Write("sc"); break;
		case 0x22: Write("HyperCall LV1"); break;
		default: Write(fmt::Format("Unknown sc: %x", sc_code));
		}
	}
	void B(s32 ll, u32 aa, u32 lk)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::Format("b 0x%x, %d, %d", ll, aa, lk));
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
	void MCRF(u32 crfd, u32 crfs)
	{
		DisAsm_CR2("mcrf", crfd, crfs);
	}
	void BCLR(u32 bo, u32 bi, u32 bh, u32 lk)
	{
		const u8 bo0 = (bo & 0x10) ? 1 : 0;
		const u8 bo1 = (bo & 0x08) ? 1 : 0;
		const u8 bo2 = (bo & 0x04) ? 1 : 0;
		const u8 bo3 = (bo & 0x02) ? 1 : 0;

		if(bo0 && !bo1 && bo2 && !bo3) {Write("blr"); return;}
		Write(fmt::Format("bclr [%x:%x:%x:%x], cr%d[%x], %d, %d", bo0, bo1, bo2, bo3, bi/4, bi%4, bh, lk));
	}
	void CRNOR(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crnor", bt, ba, bb);
	}
	void CRANDC(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crandc", bt, ba, bb);
	}
	void ISYNC()
	{
		Write("isync");
	}
	void CRXOR(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crxor", bt, ba, bb);
	}
	void CRNAND(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crnand", bt, ba, bb);
	}
	void CRAND(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crand", bt, ba, bb);
	}
	void CREQV(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("creqv", bt, ba, bb);
	}
	void CRORC(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("crorc", bt, ba, bb);
	}
	void CROR(u32 bt, u32 ba, u32 bb)
	{
		DisAsm_INT3("cror", bt, ba, bb);
	}
	void BCCTR(u32 bo, u32 bi, u32 bh, u32 lk)
	{
		switch(lk)
		{
			case 0: DisAsm_INT3("bcctr", bo, bi, bh); break;
			case 1: DisAsm_INT3("bcctrl", bo, bi, bh); break;
		}
	}
	void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwimi", ra, rs, sh, mb, me, rc);
	}
	void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwinm", ra, rs, sh, mb, me, rc);
	}
	void RLWNM(u32 ra, u32 rs, u32 rb, u32 MB, u32 ME, bool rc)
	{
		DisAsm_R3_INT2_RC("rlwnm", ra, rs, rb, MB, ME, rc);
	}
	void ORI(u32 rs, u32 ra, u32 uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("ori", rs, ra, uimm16);
	}
	void ORIS(u32 rs, u32 ra, u32 uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("oris", rs, ra, uimm16);
	}
	void XORI(u32 ra, u32 rs, u32 uimm16)
	{
		DisAsm_R2_IMM("xori", ra, rs, uimm16);
	}
	void XORIS(u32 ra, u32 rs, u32 uimm16)
	{
		DisAsm_R2_IMM("xoris", ra, rs, uimm16);
	}
	void ANDI_(u32 ra, u32 rs, u32 uimm16)
	{
		DisAsm_R2_IMM("andi.", ra, rs, uimm16);
	}
	void ANDIS_(u32 ra, u32 rs, u32 uimm16)
	{
		DisAsm_R2_IMM("andis.", ra, rs, uimm16);
	}
	void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
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
	void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc)
	{
		DisAsm_R2_INT2_RC("rldicr", ra, rs, sh, me, rc);
	}
	void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
	{
		DisAsm_R2_INT2_RC("rldic", ra, rs, sh, mb, rc);
	}
	void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc)
	{
		DisAsm_R2_INT2_RC("rldimi", ra, rs, sh, mb, rc);
	}
	void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc)
	{
		if (is_r)
			DisAsm_R3_INT2_RC("rldcr", ra, rs, rb, m_eb, 0, rc);
		else
			DisAsm_R3_INT2_RC("rldcl", ra, rs, rb, m_eb, 0, rc);
	}
	void CMP(u32 crfd, u32 l, u32 ra, u32 rb)
	{
		DisAsm_CR1_R2(fmt::Format("cmp%s", (l ? "d" : "w")), crfd, ra, rb);
	}
	void TW(u32 to, u32 ra, u32 rb)
	{
		DisAsm_INT1_R2("tw", to, ra, rb);
	}
	void LVSL(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvsl", vd, ra, rb);
	}
	void LVEBX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvebx", vd, ra, rb);
	}
	void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("subfc", rd, ra, rb, oe, rc);
	}
	void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("addc", rd, ra, rb, oe, rc);
	}
	void MULHDU(u32 rd, u32 ra, u32 rb, bool rc)
	{
		DisAsm_R3_RC("mulhdu", rd, ra, rb, rc);
	}
	void MULHWU(u32 rd, u32 ra, u32 rb, bool rc)
	{
		DisAsm_R3_RC("mulhwu", rd, ra, rb, rc);
	}
	void MFOCRF(u32 a, u32 rd, u32 crm)
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
	void LWARX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwarx", rd, ra, rb);
	}
	void LDX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("ldx", rd, ra, rb);
	}
	void LWZX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwzx", rd, ra, rb);
	}
	void SLW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("slw", ra, rs, rb, rc);
	}
	void CNTLZW(u32 ra, u32 rs, bool rc)
	{
		DisAsm_R2_RC("cntlzw", ra, rs, rc);
	}
	void SLD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("sld", ra, rs, rb, rc);
	}
	void AND(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("and", ra, rs, rb, rc);
	}
	void CMPL(u32 crfd, u32 l, u32 ra, u32 rb)
	{
		DisAsm_CR1_R2(fmt::Format("cmpl%s", (l ? "d" : "w")), crfd, ra, rb);
	}
	void LVSR(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvsr", vd, ra, rb);
	}
	void LVEHX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvehx", vd, ra, rb);
	}
	void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("subf", rd, ra, rb, oe, rc);
	}
	void LDUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("ldux", rd, ra, rb);
	}
	void DCBST(u32 ra, u32 rb)
	{
		DisAsm_R2("dcbst", ra, rb);
	}
	void LWZUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwzux", rd, ra, rb);
	}
	void CNTLZD(u32 ra, u32 rs, bool rc)
	{
		DisAsm_R2_RC("cntlzd", ra, rs, rc);
	}
	void ANDC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("andc", ra, rs, rb, rc);
	}
	void LVEWX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvewx", vd, ra, rb);
	}
	void MULHD(u32 rd, u32 ra, u32 rb, bool rc)
	{
		DisAsm_R3_RC("mulhd", rd, ra, rb, rc);
	}
	void MULHW(u32 rd, u32 ra, u32 rb, bool rc)
	{
		DisAsm_R3_RC("mulhw", rd, ra, rb, rc);
	}
	void LDARX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("ldarx", rd, ra, rb);
	}
	void DCBF(u32 ra, u32 rb)
	{
		DisAsm_R2("dcbf", ra, rb);
	}
	void LBZX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lbzx", rd, ra, rb);
	}
	void LVX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvx", vd, ra, rb);
	}
	void NEG(u32 rd, u32 ra, u32 oe, bool rc)
	{
		DisAsm_R2_OE_RC("neg", rd, ra, oe, rc);
	}
	void LBZUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lbzux", rd, ra, rb);
	}
	void NOR(u32 ra, u32 rs, u32 rb, bool rc)
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
	void STVEBX(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvebx", vs, ra, rb);
	}
	void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("subfe", rd, ra, rb, oe, rc);
	}
	void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("adde", rd, ra, rb, oe, rc);
	}
	void MTOCRF(u32 l, u32 crm, u32 rs)
	{
		if(l)
		{
			DisAsm_INT1_R1("mtocrf", crm, rs);
		}
		else
		{
			DisAsm_INT1_R1("mtcrf", crm, rs);
		}
	}
	void STDX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stdx.", rs, ra, rb);
	}
	void STWCX_(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stwcx.", rs, ra, rb);
	}
	void STWX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stwx", rs, ra, rb);
	}
	void STVEHX(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvehx", vs, ra, rb);
	}
	void STDUX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stdux", rs, ra, rb);
	}
	void STWUX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stwux", rs, ra, rb);
	}
	void STVEWX(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvewx", vs, ra, rb);
	}
	void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		DisAsm_R2_OE_RC("subfze", rd, ra, oe, rc);
	}
	void ADDZE(u32 rd, u32 ra, u32 oe, bool rc)
	{
		DisAsm_R2_OE_RC("addze", rd, ra, oe, rc);
	}
	void STDCX_(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stdcx.", rs, ra, rb);
	}
	void STBX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stbx", rs, ra, rb);
	}
	void STVX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvx", vd, ra, rb);
	}
	void SUBFME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		DisAsm_R2_OE_RC("subfme", rd, ra, oe, rc);
	}
	void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("mulld", rd, ra, rb, oe, rc);
	}
	void ADDME(u32 rd, u32 ra, u32 oe, bool rc)
	{
		DisAsm_R2_OE_RC("addme", rd, ra, oe, rc);
	}
	void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("mullw", rd, ra, rb, oe, rc);
	}
	void DCBTST(u32 th, u32 ra, u32 rb)
	{
		DisAsm_R3("dcbtst", th, ra, rb);
	}
	void STBUX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stbux", rs, ra, rb);
	}
	void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("add", rd, ra, rb, oe, rc);
	}
	void DCBT(u32 ra, u32 rb, u32 th)
	{
		DisAsm_R2("dcbt", ra, rb);
	}
	void LHZX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lhzx", rd, ra, rb);
	}
	void EQV(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("eqv", ra, rs, rb, rc);
	}
	void ECIWX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("eciwx", rd, ra, rb);
	}
	void LHZUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lhzux", rd, ra, rb);
	}
	void XOR(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("xor", ra, rs, rb, rc);
	}
	void MFSPR(u32 rd, u32 spr)
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
	void LWAX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwax", rd, ra, rb);
	}
	void DST(u32 ra, u32 rb, u32 strm, u32 t)
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
	void LHAX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lhax", rd, ra, rb);
	}
	void LVXL(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvxl", vd, ra, rb);
	}
	void MFTB(u32 rd, u32 spr)
	{
		const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
		switch(n)
		{
		case 268: DisAsm_R1("mftb", rd); break;
		case 269: DisAsm_R1("mftbu", rd); break;
		default: DisAsm_R1_IMM("mftb", rd, spr); break;
		}
	}
	void LWAUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwaux", rd, ra, rb);
	}
	void DSTST(u32 ra, u32 rb, u32 strm, u32 t)
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
	void LHAUX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lhaux", rd, ra, rb);
	}
	void STHX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("sthx", rs, ra, rb);
	}
	void ORC(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("orc", ra, rs, rb, rc);
	}
	void ECOWX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("ecowx", rs, ra, rb);
	}
	void STHUX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("sthux", rs, ra, rb);
	}
	void OR(u32 ra, u32 rs, u32 rb, bool rc)
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
	void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("divdu", rd, ra, rb, oe, rc);
	}
	void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("divwu", rd, ra, rb, oe, rc);
	}
	void MTSPR(u32 spr, u32 rs)
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
	void NAND(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("nand", ra, rs, rb, rc);
	}
	void STVXL(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvxl", vs, ra, rb);
	}
	void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("divd", rd, ra, rb, oe, rc);
	}
	void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc)
	{
		DisAsm_R3_OE_RC("divw", rd, ra, rb, oe, rc);
	}
	void LVLX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvlx", vd, ra, rb);
	}
	void LDBRX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("ldbrx", rd, ra, rb);
	}
	void LWBRX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lwbrx", rd, ra, rb);
	}
	void LFSX(u32 frd, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("lfsx", frd, ra, rb);
	}
	void SRW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("srw", ra, rs, rb, rc);
	}
	void SRD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("srd", ra, rs, rb, rc);
	}
	void LVRX(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvrx", vd, ra, rb);
	}
	void LSWI(u32 rd, u32 ra, u32 nb)
	{
		DisAsm_R2_INT1("lswi", rd, ra, nb);
	}
	void LFSUX(u32 frd, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("lfsux", frd, ra, rb);
	}
	void SYNC(u32 l)
	{
		DisAsm_INT1("sync", l);
	}
	void LFDX(u32 frd, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("lfdx", frd, ra, rb);
	}
	void LFDUX(u32 frd, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("lfdux", frd, ra, rb);
	}
	void STVLX(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvlx", vs, ra, rb);
	}
	void STWBRX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("stwbrx", rs, ra, rb);
	}
	void STFSX(u32 frs, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("stfsx", frs, ra, rb);
	}
	void STVRX(u32 sd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvrx", sd, ra, rb);
	}
	void STSWI(u32 rd, u32 ra, u32 nb)
	{
		DisAsm_R2_INT1("stswi", rd, ra, nb);
	}
	void STFDX(u32 frs, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("stfdx", frs, ra, rb);
	}
	void LVLXL(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvlxl", vd, ra, rb);
	}
	void LHBRX(u32 rd, u32 ra, u32 rb)
	{
		DisAsm_R3("lhbrx", rd, ra, rb);
	}
	void SRAW(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("sraw", ra, rs, rb, rc);
	}
	void SRAD(u32 ra, u32 rs, u32 rb, bool rc)
	{
		DisAsm_R3_RC("srad", ra, rs, rb, rc);
	}
	void LVRXL(u32 vd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("lvrxl", vd, ra, rb);
	}
	void DSS(u32 strm, u32 a)
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
	void SRAWI(u32 ra, u32 rs, u32 sh, bool rc)
	{
		DisAsm_R2_INT1_RC("srawi", ra, rs, sh, rc);
	}
	void SRADI1(u32 ra, u32 rs, u32 sh, bool rc)
	{
		DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
	}
	void SRADI2(u32 ra, u32 rs, u32 sh, bool rc)
	{
		DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
	}
	void EIEIO()
	{
		Write("eieio");
	}
	void STVLXL(u32 vs, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvlxl", vs, ra, rb);
	}
	void STHBRX(u32 rs, u32 ra, u32 rb)
	{
		DisAsm_R3("sthbrx", rs, ra, rb);
	}
	void EXTSH(u32 ra, u32 rs, bool rc)
	{
		DisAsm_R2_RC("extsh", ra, rs, rc);
	}
	void STVRXL(u32 sd, u32 ra, u32 rb)
	{
		DisAsm_V1_R2("stvrxl", sd, ra, rb);
	}
	void EXTSB(u32 ra, u32 rs, bool rc)
	{
		DisAsm_R2_RC("extsb", ra, rs, rc);
	}
	void STFIWX(u32 frs, u32 ra, u32 rb)
	{
		DisAsm_F1_R2("stfiwx", frs, ra, rb);
	}
	void EXTSW(u32 ra, u32 rs, bool rc)
	{
		DisAsm_R2_RC("extsw", ra, rs, rc);
	}
	/*0x3d6*///ICBI
	void DCBZ(u32 ra, u32 rs)
	{
		DisAsm_R2("dcbz", ra, rs);
	}	
	void LWZ(u32 rd, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lwz", rd, ra, d);
	}
	void LWZU(u32 rd, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lwzu", rd, ra, d);
	}
	void LBZ(u32 rd, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lbz", rd, ra, d);
	}
	void LBZU(u32 rd, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lbzu", rd, ra, d);
	}
	void STW(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("stw", rs, ra, d);
	}
	void STWU(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("stwu", rs, ra, d);
	}
	void STB(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("stb", rs, ra, d);
	}
	void STBU(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("stbu", rs, ra, d);
	}
	void LHZ(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lhz", rs, ra, d);
	}
	void LHZU(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lhzu", rs, ra, d);
	}
	void LHA(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lha", rs, ra, d);
	}
	void LHAU(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lhau", rs, ra, d);
	}
	void STH(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("sth", rs, ra, d);
	}
	void STHU(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("sthu", rs, ra, d);
	}
	void LMW(u32 rd, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("lmw", rd, ra, d);
	}
	void STMW(u32 rs, u32 ra, s32 d)
	{
		DisAsm_R2_IMM("stmw", rs, ra, d);
	}
	void LFS(u32 frd, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("lfs", frd, d, ra);
	}
	void LFSU(u32 frd, u32 ra, s32 ds)
	{
		DisAsm_F1_IMM_R1("lfsu", frd, ds, ra);
	}
	void LFD(u32 frd, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("lfd", frd, d, ra);
	}
	void LFDU(u32 frd, u32 ra, s32 ds)
	{
		DisAsm_F1_IMM_R1("lfdu", frd, ds, ra);
	}
	void STFS(u32 frs, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("stfs", frs, d, ra);
	}
	void STFSU(u32 frs, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("stfsu", frs, d, ra);
	}
	void STFD(u32 frs, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("stfd", frs, d, ra);
	}
	void STFDU(u32 frs, u32 ra, s32 d)
	{
		DisAsm_F1_IMM_R1("stfdu", frs, d, ra);
	}
	void LD(u32 rd, u32 ra, s32 ds)
	{
		DisAsm_R2_IMM("ld", rd, ra, ds);
	}
	void LDU(u32 rd, u32 ra, s32 ds)
	{
		DisAsm_R2_IMM("ldu", rd, ra, ds);
	}
	void LWA(u32 rd, u32 ra, s32 ds)
	{
		DisAsm_R2_IMM("lwa", rd, ra, ds);
	}
	void FDIVS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fdivs", frd, fra, frb, rc);
	}
	void FSUBS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fsubs", frd, fra, frb, rc);
	}
	void FADDS(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fadds", frd, fra, frb, rc);
	}
	void FSQRTS(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fsqrts", frd, frb, rc);
	}
	void FRES(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fres", frd, frb, rc);
	}
	void FMULS(u32 frd, u32 fra, u32 frc, bool rc)
	{
		DisAsm_F3_RC("fmuls", frd, fra, frc, rc);
	}
	void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fmadds", frd, fra, frc, frb, rc);
	}
	void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fmsubs", frd, fra, frc, frb, rc);
	}
	void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fnmsubs", frd, fra, frc, frb, rc);
	}
	void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fnmadds", frd, fra, frc, frb, rc);
	}
	void STD(u32 rs, u32 ra, s32 ds)
	{
		DisAsm_R2_IMM("std", rs, ra, ds);
	}
	void STDU(u32 rs, u32 ra, s32 ds)
	{
		DisAsm_R2_IMM("stdu", rs, ra, ds);
	}
	void MTFSB1(u32 bt, bool rc)
	{
		DisAsm_F1_RC("mtfsb1", bt, rc);
	}
	void MCRFS(u32 bf, u32 bfa)
	{
		DisAsm_F2("mcrfs", bf, bfa);
	}
	void MTFSB0(u32 bt, bool rc)
	{
		DisAsm_F1_RC("mtfsb0", bt, rc);
	}
	void MTFSFI(u32 crfd, u32 i, bool rc)
	{
		DisAsm_F2_RC("mtfsfi", crfd, i, rc);
	}
	void MFFS(u32 frd, bool rc)
	{
		DisAsm_F1_RC("mffs", frd, rc);
	}
	void MTFSF(u32 flm, u32 frb, bool rc)
	{
		DisAsm_F2_RC("mtfsf", flm, frb, rc);
	}
	void FCMPU(u32 crfd, u32 fra, u32 frb)
	{
		DisAsm_CR1_F2("fcmpu", crfd, fra, frb);
	}
	void FRSP(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("frsp", frd, frb, rc);
	}
	void FCTIW(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fctiw", frd, frb, rc);
	}
	void FCTIWZ(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fctiwz", frd, frb, rc);
	}
	void FDIV(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fdiv", frd, fra, frb, rc);
	}
	void FSUB(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fsub", frd, fra, frb, rc);
	}
	void FADD(u32 frd, u32 fra, u32 frb, bool rc)
	{
		DisAsm_F3_RC("fadd", frd, fra, frb, rc);
	}
	void FSQRT(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fsqrt", frd, frb, rc);
	}
	void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fsel", frd, fra, frc, frb, rc);
	}
	void FMUL(u32 frd, u32 fra, u32 frc, bool rc)
	{
		DisAsm_F3_RC("fmul", frd, fra, frc, rc);
	}
	void FRSQRTE(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("frsqrte", frd, frb, rc);
	}
	void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fmsub", frd, fra, frc, frb, rc);
	}
	void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fmadd", frd, fra, frc, frb, rc);
	}
	void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fnmsub", frd, fra, frc, frb, rc);
	}
	void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc)
	{
		DisAsm_F4_RC("fnmadd", frd, fra, frc, frb, rc);
	}
	void FCMPO(u32 crfd, u32 fra, u32 frb)
	{
		DisAsm_F3("fcmpo", crfd, fra, frb);
	}
	void FNEG(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fneg", frd, frb, rc);
	}
	void FMR(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fmr", frd, frb, rc);
	}
	void FNABS(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fnabs", frd, frb, rc);
	}
	void FABS(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fabs", frd, frb, rc);
	}
	void FCTID(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fctid", frd, frb, rc);
	}
	void FCTIDZ(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fctidz", frd, frb, rc);
	}
	void FCFID(u32 frd, u32 frb, bool rc)
	{
		DisAsm_F2_RC("fcfid", frd, frb, rc);
	}

	void UNK(const u32 code, const u32 opcode, const u32 gcode)
	{
		Write(fmt::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP
