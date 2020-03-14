#pragma once

#include "Emu/Cell/PPCDisAsm.h"
#include "Emu/Cell/PPUOpcodes.h"

class PPUDisAsm final : public PPCDisAsm
{
public:
	PPUDisAsm(CPUDisAsmMode mode) : PPCDisAsm(mode)
	{
	}

private:
	u32 DisAsmBranchTarget(const s32 imm) override
	{
		return dump_pc + (imm & ~3);
	}

	constexpr const char* get_partial_BI_field(u32 bi)
	{
		switch (bi % 4)
		{
		case 0x0: return "lt";
		case 0x1: return "gt";
		case 0x2: return "eq";
		case 0x3: return "so";
		default: ASSUME(0); return {};
		}
	}

private:
	void DisAsm_V4(const std::string& op, u32 v0, u32 v1, u32 v2, u32 v3)
	{
		Write(fmt::format("%s v%d,v%d,v%d,v%d", FixOp(op), v0, v1, v2, v3));
	}
	void DisAsm_V3_UIMM(const std::string& op, u32 v0, u32 v1, u32 v2, u32 uimm)
	{
		Write(fmt::format("%s v%d,v%d,v%d,%u #%x", FixOp(op), v0, v1, v2, uimm, uimm));
	}
	void DisAsm_V3(const std::string& op, u32 v0, u32 v1, u32 v2)
	{
		Write(fmt::format("%s v%d,v%d,v%d", FixOp(op), v0, v1, v2));
	}
	void DisAsm_V2_UIMM(const std::string& op, u32 v0, u32 v1, u32 uimm)
	{
		Write(fmt::format("%s v%d,v%d,%u #%x", FixOp(op), v0, v1, uimm, uimm));
	}
	void DisAsm_V2(const std::string& op, u32 v0, u32 v1)
	{
		Write(fmt::format("%s v%d,v%d", FixOp(op), v0, v1));
	}
	void DisAsm_V1_SIMM(const std::string& op, u32 v0, s32 simm)
	{
		Write(fmt::format("%s v%d,%d #%x", FixOp(op), v0, simm, simm));
	}
	void DisAsm_V1(const std::string& op, u32 v0)
	{
		Write(fmt::format("%s v%d", FixOp(op), v0));
	}
	void DisAsm_V1_R2(const std::string& op, u32 v0, u32 r1, u32 r2)
	{
		Write(fmt::format("%s v%d,r%d,r%d", FixOp(op), v0, r1, r2));
	}
	void DisAsm_CR1_F2_RC(const std::string& op, u32 cr0, u32 f0, u32 f1, u32 rc)
	{
		Write(fmt::format("%s cr%d,f%d,f%d", FixOp(op + (rc ? "." : "")), cr0, f0, f1));
	}
	void DisAsm_CR1_F2(const std::string& op, u32 cr0, u32 f0, u32 f1)
	{
		DisAsm_CR1_F2_RC(op, cr0, f0, f1, false);
	}
	void DisAsm_INT1_R2(const std::string& op, u32 i0, u32 r0, u32 r1)
	{
		Write(fmt::format("%s %d,r%d,r%d", FixOp(op), i0, r0, r1));
	}
	void DisAsm_INT1_R1_IMM(const std::string& op, u32 i0, u32 r0, s32 imm0)
	{
		Write(fmt::format("%s %d,r%d,%d #%x", FixOp(op), i0, r0, imm0, imm0));
	}
	void DisAsm_INT1_R1_RC(const std::string& op, u32 i0, u32 r0, u32 rc)
	{
		Write(fmt::format("%s %d,r%d", FixOp(op + (rc ? "." : "")), i0, r0));
	}
	void DisAsm_INT1_R1(const std::string& op, u32 i0, u32 r0)
	{
		DisAsm_INT1_R1_RC(op, i0, r0, false);
	}
	void DisAsm_F4_RC(const std::string& op, u32 f0, u32 f1, u32 f2, u32 f3, u32 rc)
	{
		Write(fmt::format("%s f%d,f%d,f%d,f%d", FixOp(op + (rc ? "." : "")), f0, f1, f2, f3));
	}
	void DisAsm_F3_RC(const std::string& op, u32 f0, u32 f1, u32 f2, u32 rc)
	{
		Write(fmt::format("%s f%d,f%d,f%d", FixOp(op + (rc ? "." : "")), f0, f1, f2));
	}
	void DisAsm_F3(const std::string& op, u32 f0, u32 f1, u32 f2)
	{
		DisAsm_F3_RC(op, f0, f1, f2, false);
	}
	void DisAsm_F2_RC(const std::string& op, u32 f0, u32 f1, u32 rc)
	{
		Write(fmt::format("%s f%d,f%d", FixOp(op + (rc ? "." : "")), f0, f1));
	}
	void DisAsm_F2(const std::string& op, u32 f0, u32 f1)
	{
		DisAsm_F2_RC(op, f0, f1, false);
	}
	void DisAsm_F1_R2(const std::string& op, u32 f0, u32 r0, u32 r1)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::format("%s f%d,r%d,r%d", FixOp(op), f0, r0, r1));
			return;
		}

		Write(fmt::format("%s f%d,r%d(r%d)", FixOp(op), f0, r0, r1));
	}
	void DisAsm_F1_IMM_R1_RC(const std::string& op, u32 f0, s32 imm0, u32 r0, u32 rc)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::format("%s f%d,r%d,%d #%x", FixOp(op + (rc ? "." : "")), f0, r0, imm0, imm0));
			return;
		}

		Write(fmt::format("%s f%d,%d(r%d) #%x", FixOp(op + (rc ? "." : "")), f0, imm0, r0, imm0));
	}
	void DisAsm_F1_IMM_R1(const std::string& op, u32 f0, s32 imm0, u32 r0)
	{
		DisAsm_F1_IMM_R1_RC(op, f0, imm0, r0, false);
	}
	void DisAsm_F1_RC(const std::string& op, u32 f0, u32 rc)
	{
		Write(fmt::format("%s f%d", FixOp(op + (rc ? "." : "")), f0));
	}
	void DisAsm_R1_RC(const std::string& op, u32 r0, u32 rc)
	{
		Write(fmt::format("%s r%d", FixOp(op + (rc ? "." : "")), r0));
	}
	void DisAsm_R1(const std::string& op, u32 r0)
	{
		DisAsm_R1_RC(op, r0, false);
	}
	void DisAsm_R2_OE_RC(const std::string& op, u32 r0, u32 r1, u32 _oe, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d", FixOp(op + (_oe ? "o" : "") + (rc ? "." : "")), r0, r1));
	}
	void DisAsm_R2_RC(const std::string& op, u32 r0, u32 r1, u32 rc)
	{
		DisAsm_R2_OE_RC(op, r0, r1, false, rc);
	}
	void DisAsm_R2(const std::string& op, u32 r0, u32 r1)
	{
		DisAsm_R2_RC(op, r0, r1, false);
	}
	void DisAsm_R3_OE_RC(const std::string& op, u32 r0, u32 r1, u32 r2, u32 _oe, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d,r%d", FixOp(op + (rc ? "." : "") + (_oe ? "o" : "")), r0, r1, r2));
	}
	void DisAsm_R3_INT2_RC(const std::string& op, u32 r0, u32 r1, u32 r2, s32 i0, s32 i1, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d,r%d,%d,%d", FixOp(op + (rc ? "." : "")), r0, r1, r2, i0, i1));
	}
	void DisAsm_R3_RC(const std::string& op, u32 r0, u32 r1, u32 r2, u32 rc)
	{
		DisAsm_R3_OE_RC(op, r0, r1, r2, false, rc);
	}
	void DisAsm_R3(const std::string& op, u32 r0, u32 r1, u32 r2)
	{
		DisAsm_R3_RC(op, r0, r1, r2, false);
	}
	void DisAsm_R2_INT3_RC(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d,%d,%d,%d", FixOp(op + (rc ? "." : "")), r0, r1, i0, i1, i2));
	}
	void DisAsm_R2_INT3(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, s32 i2)
	{
		DisAsm_R2_INT3_RC(op, r0, r1, i0, i1, i2, false);
	}
	void DisAsm_R2_INT2_RC(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d,%d,%d", FixOp(op + (rc ? "." : "")), r0, r1, i0, i1));
	}
	void DisAsm_R2_INT2(const std::string& op, u32 r0, u32 r1, s32 i0, s32 i1)
	{
		DisAsm_R2_INT2_RC(op, r0, r1, i0, i1, false);
	}
	void DisAsm_R2_INT1_RC(const std::string& op, u32 r0, u32 r1, s32 i0, u32 rc)
	{
		Write(fmt::format("%s r%d,r%d,%d", FixOp(op + (rc ? "." : "")), r0, r1, i0));
	}
	void DisAsm_R2_INT1(const std::string& op, u32 r0, u32 r1, s32 i0)
	{
		DisAsm_R2_INT1_RC(op, r0, r1, i0, false);
	}
	void DisAsm_R2_IMM(const std::string& op, u32 r0, u32 r1, s32 imm0)
	{
		if(m_mode == CPUDisAsm_CompilerElfMode)
		{
			Write(fmt::format("%s r%d,r%d,%d  #%x", FixOp(op), r0, r1, imm0, imm0));
			return;
		}

		Write(fmt::format("%s r%d,%d(r%d)  #%x", FixOp(op), r0, imm0, r1, imm0));
	}
	void DisAsm_R1_IMM(const std::string& op, u32 r0, s32 imm0)
	{
		Write(fmt::format("%s r%d,%d  #%x", FixOp(op), r0, imm0, imm0));
	}
	void DisAsm_IMM_R1(const std::string& op, s32 imm0, u32 r0)
	{
		Write(fmt::format("%s %d,r%d  #%x", FixOp(op), imm0, r0, imm0));
	}
	void DisAsm_CR1_R1_IMM(const std::string& op, u32 cr0, u32 r0, s32 imm0)
	{
		Write(fmt::format("%s cr%d,r%d,%d  #%x", FixOp(op), cr0, r0, imm0, imm0));
	}
	void DisAsm_CR1_R2_RC(const std::string& op, u32 cr0, u32 r0, u32 r1, u32 rc)
	{
		Write(fmt::format("%s%s cr%d,r%d,r%d", FixOp(op), (rc ? "." : ""), cr0, r0, r1));
	}
	void DisAsm_CR1_R2(const std::string& op, u32 cr0, u32 r0, u32 r1)
	{
		DisAsm_CR1_R2_RC(op, cr0, r0, r1, false);
	}
	void DisAsm_CR2(const std::string& op, u32 cr0, u32 cr1)
	{
		Write(fmt::format("%s cr%d,cr%d", FixOp(op), cr0, cr1));
	}
	void DisAsm_BI1(const std::string& op, const int i0)
	{
		Write(fmt::format("%s cr%d[%s]", FixOp(op), i0 / 4, get_partial_BI_field(i0)));
	}
	void DisAsm_BI2(const std::string& op, const int i0, const int i1)
	{
		Write(fmt::format("%s cr%d[%s],cr%d[%s]", FixOp(op), i0 / 4, get_partial_BI_field(i0), i1 / 4, get_partial_BI_field(i1)));
	}
	void DisAsm_BI3(const std::string& op, const int i0, const int i1, const int i2)
	{
		Write(fmt::format("%s cr%d[%s],cr%d[%s],cr%d[%s]", FixOp(op),
		i0 / 4, get_partial_BI_field(i0), i1 / 4, get_partial_BI_field(i1), i2 / 4, get_partial_BI_field(i2)));
	}
	void DisAsm_INT3(const std::string& op, const int i0, const int i1, const int i2)
	{
		Write(fmt::format("%s %d,%d,%d", FixOp(op), i0, i1, i2));
	}
	void DisAsm_INT1(const std::string& op, const int i0)
	{
		Write(fmt::format("%s %d", FixOp(op), i0));
	}
	void DisAsm_BRANCH(const std::string& op, const int pc)
	{
		Write(fmt::format("%s 0x%x", FixOp(op), DisAsmBranchTarget(pc)));
	}
	void DisAsm_BRANCH_A(const std::string& op, const int pc)
	{
		Write(fmt::format("%s 0x%x", FixOp(op), pc));
	}
	void DisAsm_B2_BRANCH(const std::string& op, u32 b0, u32 b1, const int pc)
	{
		Write(fmt::format("%s %d,%d,0x%x ", FixOp(op), b0, b1, DisAsmBranchTarget(pc)));
	}
	void DisAsm_CR_BRANCH(const std::string& op, u32 cr, const int pc)
	{
		Write(fmt::format("%s cr%d,0x%x ", FixOp(op), cr, DisAsmBranchTarget(pc)));
	}
	void DisAsm_CR_BRANCH_A(const std::string& op, u32 cr, const int pc)
	{
		Write(fmt::format("%s cr%d,0x%x ", FixOp(op), cr, pc));
	}
	void DisAsm_BI_BRANCH(const std::string& op, u32 bi, const int pc)
	{
		Write(fmt::format("%s cr%d[%s],0x%x ", FixOp(op), bi / 4, get_partial_BI_field(bi), DisAsmBranchTarget(pc)));
	}
	void DisAsm_BI_BRANCH_A(const std::string& op, u32 bi, const int pc)
	{
		Write(fmt::format("%s cr%d[%s],0x%x ", FixOp(op), bi / 4, get_partial_BI_field(bi), pc));
	}
	
public:
	u32 disasm(u32 pc) override;

	void MFVSCR(ppu_opcode_t op);
	void MTVSCR(ppu_opcode_t op);
	void VADDCUW(ppu_opcode_t op);
	void VADDFP(ppu_opcode_t op);
	void VADDSBS(ppu_opcode_t op);
	void VADDSHS(ppu_opcode_t op);
	void VADDSWS(ppu_opcode_t op);
	void VADDUBM(ppu_opcode_t op);
	void VADDUBS(ppu_opcode_t op);
	void VADDUHM(ppu_opcode_t op);
	void VADDUHS(ppu_opcode_t op);
	void VADDUWM(ppu_opcode_t op);
	void VADDUWS(ppu_opcode_t op);
	void VAND(ppu_opcode_t op);
	void VANDC(ppu_opcode_t op);
	void VAVGSB(ppu_opcode_t op);
	void VAVGSH(ppu_opcode_t op);
	void VAVGSW(ppu_opcode_t op);
	void VAVGUB(ppu_opcode_t op);
	void VAVGUH(ppu_opcode_t op);
	void VAVGUW(ppu_opcode_t op);
	void VCFSX(ppu_opcode_t op);
	void VCFUX(ppu_opcode_t op);
	void VCMPBFP(ppu_opcode_t op);
	void VCMPEQFP(ppu_opcode_t op);
	void VCMPEQUB(ppu_opcode_t op);
	void VCMPEQUH(ppu_opcode_t op);
	void VCMPEQUW(ppu_opcode_t op);
	void VCMPGEFP(ppu_opcode_t op);
	void VCMPGTFP(ppu_opcode_t op);
	void VCMPGTSB(ppu_opcode_t op);
	void VCMPGTSH(ppu_opcode_t op);
	void VCMPGTSW(ppu_opcode_t op);
	void VCMPGTUB(ppu_opcode_t op);
	void VCMPGTUH(ppu_opcode_t op);
	void VCMPGTUW(ppu_opcode_t op);
	void VCTSXS(ppu_opcode_t op);
	void VCTUXS(ppu_opcode_t op);
	void VEXPTEFP(ppu_opcode_t op);
	void VLOGEFP(ppu_opcode_t op);
	void VMADDFP(ppu_opcode_t op);
	void VMAXFP(ppu_opcode_t op);
	void VMAXSB(ppu_opcode_t op);
	void VMAXSH(ppu_opcode_t op);
	void VMAXSW(ppu_opcode_t op);
	void VMAXUB(ppu_opcode_t op);
	void VMAXUH(ppu_opcode_t op);
	void VMAXUW(ppu_opcode_t op);
	void VMHADDSHS(ppu_opcode_t op);
	void VMHRADDSHS(ppu_opcode_t op);
	void VMINFP(ppu_opcode_t op);
	void VMINSB(ppu_opcode_t op);
	void VMINSH(ppu_opcode_t op);
	void VMINSW(ppu_opcode_t op);
	void VMINUB(ppu_opcode_t op);
	void VMINUH(ppu_opcode_t op);
	void VMINUW(ppu_opcode_t op);
	void VMLADDUHM(ppu_opcode_t op);
	void VMRGHB(ppu_opcode_t op);
	void VMRGHH(ppu_opcode_t op);
	void VMRGHW(ppu_opcode_t op);
	void VMRGLB(ppu_opcode_t op);
	void VMRGLH(ppu_opcode_t op);
	void VMRGLW(ppu_opcode_t op);
	void VMSUMMBM(ppu_opcode_t op);
	void VMSUMSHM(ppu_opcode_t op);
	void VMSUMSHS(ppu_opcode_t op);
	void VMSUMUBM(ppu_opcode_t op);
	void VMSUMUHM(ppu_opcode_t op);
	void VMSUMUHS(ppu_opcode_t op);
	void VMULESB(ppu_opcode_t op);
	void VMULESH(ppu_opcode_t op);
	void VMULEUB(ppu_opcode_t op);
	void VMULEUH(ppu_opcode_t op);
	void VMULOSB(ppu_opcode_t op);
	void VMULOSH(ppu_opcode_t op);
	void VMULOUB(ppu_opcode_t op);
	void VMULOUH(ppu_opcode_t op);
	void VNMSUBFP(ppu_opcode_t op);
	void VNOR(ppu_opcode_t op);
	void VOR(ppu_opcode_t op);
	void VPERM(ppu_opcode_t op);
	void VPKPX(ppu_opcode_t op);
	void VPKSHSS(ppu_opcode_t op);
	void VPKSHUS(ppu_opcode_t op);
	void VPKSWSS(ppu_opcode_t op);
	void VPKSWUS(ppu_opcode_t op);
	void VPKUHUM(ppu_opcode_t op);
	void VPKUHUS(ppu_opcode_t op);
	void VPKUWUM(ppu_opcode_t op);
	void VPKUWUS(ppu_opcode_t op);
	void VREFP(ppu_opcode_t op);
	void VRFIM(ppu_opcode_t op);
	void VRFIN(ppu_opcode_t op);
	void VRFIP(ppu_opcode_t op);
	void VRFIZ(ppu_opcode_t op);
	void VRLB(ppu_opcode_t op);
	void VRLH(ppu_opcode_t op);
	void VRLW(ppu_opcode_t op);
	void VRSQRTEFP(ppu_opcode_t op);
	void VSEL(ppu_opcode_t op);
	void VSL(ppu_opcode_t op);
	void VSLB(ppu_opcode_t op);
	void VSLDOI(ppu_opcode_t op);
	void VSLH(ppu_opcode_t op);
	void VSLO(ppu_opcode_t op);
	void VSLW(ppu_opcode_t op);
	void VSPLTB(ppu_opcode_t op);
	void VSPLTH(ppu_opcode_t op);
	void VSPLTISB(ppu_opcode_t op);
	void VSPLTISH(ppu_opcode_t op);
	void VSPLTISW(ppu_opcode_t op);
	void VSPLTW(ppu_opcode_t op);
	void VSR(ppu_opcode_t op);
	void VSRAB(ppu_opcode_t op);
	void VSRAH(ppu_opcode_t op);
	void VSRAW(ppu_opcode_t op);
	void VSRB(ppu_opcode_t op);
	void VSRH(ppu_opcode_t op);
	void VSRO(ppu_opcode_t op);
	void VSRW(ppu_opcode_t op);
	void VSUBCUW(ppu_opcode_t op);
	void VSUBFP(ppu_opcode_t op);
	void VSUBSBS(ppu_opcode_t op);
	void VSUBSHS(ppu_opcode_t op);
	void VSUBSWS(ppu_opcode_t op);
	void VSUBUBM(ppu_opcode_t op);
	void VSUBUBS(ppu_opcode_t op);
	void VSUBUHM(ppu_opcode_t op);
	void VSUBUHS(ppu_opcode_t op);
	void VSUBUWM(ppu_opcode_t op);
	void VSUBUWS(ppu_opcode_t op);
	void VSUMSWS(ppu_opcode_t op);
	void VSUM2SWS(ppu_opcode_t op);
	void VSUM4SBS(ppu_opcode_t op);
	void VSUM4SHS(ppu_opcode_t op);
	void VSUM4UBS(ppu_opcode_t op);
	void VUPKHPX(ppu_opcode_t op);
	void VUPKHSB(ppu_opcode_t op);
	void VUPKHSH(ppu_opcode_t op);
	void VUPKLPX(ppu_opcode_t op);
	void VUPKLSB(ppu_opcode_t op);
	void VUPKLSH(ppu_opcode_t op);
	void VXOR(ppu_opcode_t op);
	void TDI(ppu_opcode_t op);
	void TWI(ppu_opcode_t op);
	void MULLI(ppu_opcode_t op);
	void SUBFIC(ppu_opcode_t op);
	void CMPLI(ppu_opcode_t op);
	void CMPI(ppu_opcode_t op);
	void ADDIC(ppu_opcode_t op);
	void ADDI(ppu_opcode_t op);
	void ADDIS(ppu_opcode_t op);
	void BC(ppu_opcode_t op);
	void SC(ppu_opcode_t op);
	void B(ppu_opcode_t op);
	void MCRF(ppu_opcode_t op);
	void BCLR(ppu_opcode_t op);
	void CRNOR(ppu_opcode_t op);
	void CRANDC(ppu_opcode_t op);
	void ISYNC(ppu_opcode_t op);
	void CRXOR(ppu_opcode_t op);
	void CRNAND(ppu_opcode_t op);
	void CRAND(ppu_opcode_t op);
	void CREQV(ppu_opcode_t op);
	void CRORC(ppu_opcode_t op);
	void CROR(ppu_opcode_t op);
	void BCCTR(ppu_opcode_t op);
	void RLWIMI(ppu_opcode_t op);
	void RLWINM(ppu_opcode_t op);
	void RLWNM(ppu_opcode_t op);
	void ORI(ppu_opcode_t op);
	void ORIS(ppu_opcode_t op);
	void XORI(ppu_opcode_t op);
	void XORIS(ppu_opcode_t op);
	void ANDI(ppu_opcode_t op);
	void ANDIS(ppu_opcode_t op);
	void RLDICL(ppu_opcode_t op);
	void RLDICR(ppu_opcode_t op);
	void RLDIC(ppu_opcode_t op);
	void RLDIMI(ppu_opcode_t op);
	void RLDCL(ppu_opcode_t op);
	void RLDCR(ppu_opcode_t op);
	void CMP(ppu_opcode_t op);
	void TW(ppu_opcode_t op);
	void LVSL(ppu_opcode_t op);
	void LVEBX(ppu_opcode_t op);
	void SUBFC(ppu_opcode_t op);
	void ADDC(ppu_opcode_t op);
	void MULHDU(ppu_opcode_t op);
	void MULHWU(ppu_opcode_t op);
	void MFOCRF(ppu_opcode_t op);
	void LWARX(ppu_opcode_t op);
	void LDX(ppu_opcode_t op);
	void LWZX(ppu_opcode_t op);
	void SLW(ppu_opcode_t op);
	void CNTLZW(ppu_opcode_t op);
	void SLD(ppu_opcode_t op);
	void AND(ppu_opcode_t op);
	void CMPL(ppu_opcode_t op);
	void LVSR(ppu_opcode_t op);
	void LVEHX(ppu_opcode_t op);
	void SUBF(ppu_opcode_t op);
	void LDUX(ppu_opcode_t op);
	void DCBST(ppu_opcode_t op);
	void LWZUX(ppu_opcode_t op);
	void CNTLZD(ppu_opcode_t op);
	void ANDC(ppu_opcode_t op);
	void TD(ppu_opcode_t op);
	void LVEWX(ppu_opcode_t op);
	void MULHD(ppu_opcode_t op);
	void MULHW(ppu_opcode_t op);
	void LDARX(ppu_opcode_t op);
	void DCBF(ppu_opcode_t op);
	void LBZX(ppu_opcode_t op);
	void LVX(ppu_opcode_t op);
	void NEG(ppu_opcode_t op);
	void LBZUX(ppu_opcode_t op);
	void NOR(ppu_opcode_t op);
	void STVEBX(ppu_opcode_t op);
	void SUBFE(ppu_opcode_t op);
	void ADDE(ppu_opcode_t op);
	void MTOCRF(ppu_opcode_t op);
	void STDX(ppu_opcode_t op);
	void STWCX(ppu_opcode_t op);
	void STWX(ppu_opcode_t op);
	void STVEHX(ppu_opcode_t op);
	void STDUX(ppu_opcode_t op);
	void STWUX(ppu_opcode_t op);
	void STVEWX(ppu_opcode_t op);
	void SUBFZE(ppu_opcode_t op);
	void ADDZE(ppu_opcode_t op);
	void STDCX(ppu_opcode_t op);
	void STBX(ppu_opcode_t op);
	void STVX(ppu_opcode_t op);
	void SUBFME(ppu_opcode_t op);
	void MULLD(ppu_opcode_t op);
	void ADDME(ppu_opcode_t op);
	void MULLW(ppu_opcode_t op);
	void DCBTST(ppu_opcode_t op);
	void STBUX(ppu_opcode_t op);
	void ADD(ppu_opcode_t op);
	void DCBT(ppu_opcode_t op);
	void LHZX(ppu_opcode_t op);
	void EQV(ppu_opcode_t op);
	void ECIWX(ppu_opcode_t op);
	void LHZUX(ppu_opcode_t op);
	void XOR(ppu_opcode_t op);
	void MFSPR(ppu_opcode_t op);
	void LWAX(ppu_opcode_t op);
	void DST(ppu_opcode_t op);
	void LHAX(ppu_opcode_t op);
	void LVXL(ppu_opcode_t op);
	void MFTB(ppu_opcode_t op);
	void LWAUX(ppu_opcode_t op);
	void DSTST(ppu_opcode_t op);
	void LHAUX(ppu_opcode_t op);
	void STHX(ppu_opcode_t op);
	void ORC(ppu_opcode_t op);
	void ECOWX(ppu_opcode_t op);
	void STHUX(ppu_opcode_t op);
	void OR(ppu_opcode_t op);
	void DIVDU(ppu_opcode_t op);
	void DIVWU(ppu_opcode_t op);
	void MTSPR(ppu_opcode_t op);
	void DCBI(ppu_opcode_t op);
	void NAND(ppu_opcode_t op);
	void STVXL(ppu_opcode_t op);
	void DIVD(ppu_opcode_t op);
	void DIVW(ppu_opcode_t op);
	void LVLX(ppu_opcode_t op);
	void LDBRX(ppu_opcode_t op);
	void LSWX(ppu_opcode_t op);
	void LWBRX(ppu_opcode_t op);
	void LFSX(ppu_opcode_t op);
	void SRW(ppu_opcode_t op);
	void SRD(ppu_opcode_t op);
	void LVRX(ppu_opcode_t op);
	void LSWI(ppu_opcode_t op);
	void LFSUX(ppu_opcode_t op);
	void SYNC(ppu_opcode_t op);
	void LFDX(ppu_opcode_t op);
	void LFDUX(ppu_opcode_t op);
	void STVLX(ppu_opcode_t op);
	void STDBRX(ppu_opcode_t op);
	void STSWX(ppu_opcode_t op);
	void STWBRX(ppu_opcode_t op);
	void STFSX(ppu_opcode_t op);
	void STVRX(ppu_opcode_t op);
	void STFSUX(ppu_opcode_t op);
	void STSWI(ppu_opcode_t op);
	void STFDX(ppu_opcode_t op);
	void STFDUX(ppu_opcode_t op);
	void LVLXL(ppu_opcode_t op);
	void LHBRX(ppu_opcode_t op);
	void SRAW(ppu_opcode_t op);
	void SRAD(ppu_opcode_t op);
	void LVRXL(ppu_opcode_t op);
	void DSS(ppu_opcode_t op);
	void SRAWI(ppu_opcode_t op);
	void SRADI(ppu_opcode_t op);
	void EIEIO(ppu_opcode_t op);
	void STVLXL(ppu_opcode_t op);
	void STHBRX(ppu_opcode_t op);
	void EXTSH(ppu_opcode_t op);
	void STVRXL(ppu_opcode_t op);
	void EXTSB(ppu_opcode_t op);
	void STFIWX(ppu_opcode_t op);
	void EXTSW(ppu_opcode_t op);
	void ICBI(ppu_opcode_t op);
	void DCBZ(ppu_opcode_t op);
	void LWZ(ppu_opcode_t op);
	void LWZU(ppu_opcode_t op);
	void LBZ(ppu_opcode_t op);
	void LBZU(ppu_opcode_t op);
	void STW(ppu_opcode_t op);
	void STWU(ppu_opcode_t op);
	void STB(ppu_opcode_t op);
	void STBU(ppu_opcode_t op);
	void LHZ(ppu_opcode_t op);
	void LHZU(ppu_opcode_t op);
	void LHA(ppu_opcode_t op);
	void LHAU(ppu_opcode_t op);
	void STH(ppu_opcode_t op);
	void STHU(ppu_opcode_t op);
	void LMW(ppu_opcode_t op);
	void STMW(ppu_opcode_t op);
	void LFS(ppu_opcode_t op);
	void LFSU(ppu_opcode_t op);
	void LFD(ppu_opcode_t op);
	void LFDU(ppu_opcode_t op);
	void STFS(ppu_opcode_t op);
	void STFSU(ppu_opcode_t op);
	void STFD(ppu_opcode_t op);
	void STFDU(ppu_opcode_t op);
	void LD(ppu_opcode_t op);
	void LDU(ppu_opcode_t op);
	void LWA(ppu_opcode_t op);
	void STD(ppu_opcode_t op);
	void STDU(ppu_opcode_t op);
	void FDIVS(ppu_opcode_t op);
	void FSUBS(ppu_opcode_t op);
	void FADDS(ppu_opcode_t op);
	void FSQRTS(ppu_opcode_t op);
	void FRES(ppu_opcode_t op);
	void FMULS(ppu_opcode_t op);
	void FMADDS(ppu_opcode_t op);
	void FMSUBS(ppu_opcode_t op);
	void FNMSUBS(ppu_opcode_t op);
	void FNMADDS(ppu_opcode_t op);
	void MTFSB1(ppu_opcode_t op);
	void MCRFS(ppu_opcode_t op);
	void MTFSB0(ppu_opcode_t op);
	void MTFSFI(ppu_opcode_t op);
	void MFFS(ppu_opcode_t op);
	void MTFSF(ppu_opcode_t op);
	void FCMPU(ppu_opcode_t op);
	void FRSP(ppu_opcode_t op);
	void FCTIW(ppu_opcode_t op);
	void FCTIWZ(ppu_opcode_t op);
	void FDIV(ppu_opcode_t op);
	void FSUB(ppu_opcode_t op);
	void FADD(ppu_opcode_t op);
	void FSQRT(ppu_opcode_t op);
	void FSEL(ppu_opcode_t op);
	void FMUL(ppu_opcode_t op);
	void FRSQRTE(ppu_opcode_t op);
	void FMSUB(ppu_opcode_t op);
	void FMADD(ppu_opcode_t op);
	void FNMSUB(ppu_opcode_t op);
	void FNMADD(ppu_opcode_t op);
	void FCMPO(ppu_opcode_t op);
	void FNEG(ppu_opcode_t op);
	void FMR(ppu_opcode_t op);
	void FNABS(ppu_opcode_t op);
	void FABS(ppu_opcode_t op);
	void FCTID(ppu_opcode_t op);
	void FCTIDZ(ppu_opcode_t op);
	void FCFID(ppu_opcode_t op);

	void UNK(ppu_opcode_t op);
};
