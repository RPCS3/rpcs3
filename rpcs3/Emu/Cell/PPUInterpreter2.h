#pragma once
#include "PPUOpcodes.h"

class PPUThread;

union ppu_opcode_t
{
	u32 opcode;

	struct
	{
		u32 rc    : 1; // 31
		u32 shh   : 1; // 30
		u32       : 3; // 27..29
		u32 mbmeh : 1; // 26
		u32 mbmel : 5; // 21..25
		u32 shl   : 5; // 16..20
		u32 vuimm : 5; // 11..15
		u32 vs    : 5; // 6..10
		u32       : 6;
	};

	struct
	{
		u32     : 6;  // 26..31
	    u32 vsh : 4;  // 22..25
		u32 oe  : 1;  // 21
		u32 spr : 10; // 11..20
		u32     : 11;
	};

	struct
	{
		u32    : 6; // 26..31
		u32 vc : 5; // 21..25
		u32 vb : 5; // 16..20
		u32 va : 5; // 11..15
		u32 vd : 5; // 6..10
		u32    : 6;
	};

	struct
	{
		u32 lk : 1; // 31
		u32 aa : 1; // 30
		u32    : 4; // 26..29
		u32    : 5; // 21..25
		u32 rb : 5; // 16..20
		u32 ra : 5; // 11..15
		u32 rd : 5; // 6..10
		u32    : 6;
	};

	struct
	{
		u32 uimm16 : 16; // 16..31
		u32        : 4;  // 12..15
		u32 l11    : 1;  // 11
		u32 rs     : 5;  // 6..10
		u32        : 6;
	};

	struct
	{
		s32 simm16 : 16; // 16..31
		s32 vsimm  : 5;  // 11..15
		s32        : 11;
	};

	struct
	{
		s32 ll : 26; // 6..31
		s32    : 6;
	};

	struct
	{
	    u32      : 5; // 27..31
	    u32 lev  : 7; // 20..26
		u32 i    : 4; // 16..19
		u32      : 2; // 14..15
		u32 crfs : 3; // 11..13
		u32 l10  : 1; // 10
		u32      : 1; // 9
		u32 crfd : 3; // 6..8
		u32      : 6;
	};

	struct
	{
		u32      : 1; // 31
		u32      : 1; // 30
		u32      : 4; // 26..29
		u32      : 5; // 21..25
		u32 crbb : 5; // 16..20
		u32 crba : 5; // 11..15
		u32 crbd : 5; // 6..10
		u32      : 6;
	};

	struct
	{
		u32 rc : 1; // 31
		u32 me : 5; // 26..30
		u32 mb : 5; // 21..25
		u32 sh : 5; // 16..20
		u32 bi : 5; // 11..15
		u32 bo : 5; // 6..10
		u32    : 6;
	};

	struct
	{
		u32     : 6; // 26..31
		u32 frc : 5; // 21..25
		u32 frb : 5; // 16..20
		u32 fra : 5; // 11..15
		u32 frd : 5; // 6..10
		u32     : 6;
	};

	struct
	{
		u32     : 12; // 20..31
		u32 crm : 8;  // 12..19
		u32     : 1;  // 11
		u32 frs : 5;  // 6..10
		u32     : 6;
	};

	struct
	{
		u32     : 17; // 15..31
		u32 flm : 8;  // 7..14
		u32     : 7;
	};
};

using ppu_inter_func_t = void(*)(PPUThread& CPU, ppu_opcode_t opcode);

namespace ppu_interpreter
{
	void NULL_OP(PPUThread& CPU, ppu_opcode_t op);
	void NOP(PPUThread& CPU, ppu_opcode_t op);

	void TDI(PPUThread& CPU, ppu_opcode_t op);
	void TWI(PPUThread& CPU, ppu_opcode_t op);

	void MFVSCR(PPUThread& CPU, ppu_opcode_t op);
	void MTVSCR(PPUThread& CPU, ppu_opcode_t op);
	void VADDCUW(PPUThread& CPU, ppu_opcode_t op);
	void VADDFP(PPUThread& CPU, ppu_opcode_t op);
	void VADDSBS(PPUThread& CPU, ppu_opcode_t op);
	void VADDSHS(PPUThread& CPU, ppu_opcode_t op);
	void VADDSWS(PPUThread& CPU, ppu_opcode_t op);
	void VADDUBM(PPUThread& CPU, ppu_opcode_t op);
	void VADDUBS(PPUThread& CPU, ppu_opcode_t op);
	void VADDUHM(PPUThread& CPU, ppu_opcode_t op);
	void VADDUHS(PPUThread& CPU, ppu_opcode_t op);
	void VADDUWM(PPUThread& CPU, ppu_opcode_t op);
	void VADDUWS(PPUThread& CPU, ppu_opcode_t op);
	void VAND(PPUThread& CPU, ppu_opcode_t op);
	void VANDC(PPUThread& CPU, ppu_opcode_t op);
	void VAVGSB(PPUThread& CPU, ppu_opcode_t op);
	void VAVGSH(PPUThread& CPU, ppu_opcode_t op);
	void VAVGSW(PPUThread& CPU, ppu_opcode_t op);
	void VAVGUB(PPUThread& CPU, ppu_opcode_t op);
	void VAVGUH(PPUThread& CPU, ppu_opcode_t op);
	void VAVGUW(PPUThread& CPU, ppu_opcode_t op);
	void VCFSX(PPUThread& CPU, ppu_opcode_t op);
	void VCFUX(PPUThread& CPU, ppu_opcode_t op);
	void VCMPBFP(PPUThread& CPU, ppu_opcode_t op);
	void VCMPBFP_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQFP(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQFP_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUB(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUB_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUH(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUH_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUW(PPUThread& CPU, ppu_opcode_t op);
	void VCMPEQUW_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGEFP(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGEFP_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTFP(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTFP_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSB(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSB_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSH(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSH_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSW(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTSW_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUB(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUB_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUH(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUH_(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUW(PPUThread& CPU, ppu_opcode_t op);
	void VCMPGTUW_(PPUThread& CPU, ppu_opcode_t op);
	void VCTSXS(PPUThread& CPU, ppu_opcode_t op);
	void VCTUXS(PPUThread& CPU, ppu_opcode_t op);
	void VEXPTEFP(PPUThread& CPU, ppu_opcode_t op);
	void VLOGEFP(PPUThread& CPU, ppu_opcode_t op);
	void VMADDFP(PPUThread& CPU, ppu_opcode_t op);
	void VMAXFP(PPUThread& CPU, ppu_opcode_t op);
	void VMAXSB(PPUThread& CPU, ppu_opcode_t op);
	void VMAXSH(PPUThread& CPU, ppu_opcode_t op);
	void VMAXSW(PPUThread& CPU, ppu_opcode_t op);
	void VMAXUB(PPUThread& CPU, ppu_opcode_t op);
	void VMAXUH(PPUThread& CPU, ppu_opcode_t op);
	void VMAXUW(PPUThread& CPU, ppu_opcode_t op);
	void VMHADDSHS(PPUThread& CPU, ppu_opcode_t op);
	void VMHRADDSHS(PPUThread& CPU, ppu_opcode_t op);
	void VMINFP(PPUThread& CPU, ppu_opcode_t op);
	void VMINSB(PPUThread& CPU, ppu_opcode_t op);
	void VMINSH(PPUThread& CPU, ppu_opcode_t op);
	void VMINSW(PPUThread& CPU, ppu_opcode_t op);
	void VMINUB(PPUThread& CPU, ppu_opcode_t op);
	void VMINUH(PPUThread& CPU, ppu_opcode_t op);
	void VMINUW(PPUThread& CPU, ppu_opcode_t op);
	void VMLADDUHM(PPUThread& CPU, ppu_opcode_t op);
	void VMRGHB(PPUThread& CPU, ppu_opcode_t op);
	void VMRGHH(PPUThread& CPU, ppu_opcode_t op);
	void VMRGHW(PPUThread& CPU, ppu_opcode_t op);
	void VMRGLB(PPUThread& CPU, ppu_opcode_t op);
	void VMRGLH(PPUThread& CPU, ppu_opcode_t op);
	void VMRGLW(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMMBM(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMSHM(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMSHS(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMUBM(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMUHM(PPUThread& CPU, ppu_opcode_t op);
	void VMSUMUHS(PPUThread& CPU, ppu_opcode_t op);
	void VMULESB(PPUThread& CPU, ppu_opcode_t op);
	void VMULESH(PPUThread& CPU, ppu_opcode_t op);
	void VMULEUB(PPUThread& CPU, ppu_opcode_t op);
	void VMULEUH(PPUThread& CPU, ppu_opcode_t op);
	void VMULOSB(PPUThread& CPU, ppu_opcode_t op);
	void VMULOSH(PPUThread& CPU, ppu_opcode_t op);
	void VMULOUB(PPUThread& CPU, ppu_opcode_t op);
	void VMULOUH(PPUThread& CPU, ppu_opcode_t op);
	void VNMSUBFP(PPUThread& CPU, ppu_opcode_t op);
	void VNOR(PPUThread& CPU, ppu_opcode_t op);
	void VOR(PPUThread& CPU, ppu_opcode_t op);
	void VPERM(PPUThread& CPU, ppu_opcode_t op);
	void VPKPX(PPUThread& CPU, ppu_opcode_t op);
	void VPKSHSS(PPUThread& CPU, ppu_opcode_t op);
	void VPKSHUS(PPUThread& CPU, ppu_opcode_t op);
	void VPKSWSS(PPUThread& CPU, ppu_opcode_t op);
	void VPKSWUS(PPUThread& CPU, ppu_opcode_t op);
	void VPKUHUM(PPUThread& CPU, ppu_opcode_t op);
	void VPKUHUS(PPUThread& CPU, ppu_opcode_t op);
	void VPKUWUM(PPUThread& CPU, ppu_opcode_t op);
	void VPKUWUS(PPUThread& CPU, ppu_opcode_t op);
	void VREFP(PPUThread& CPU, ppu_opcode_t op);
	void VRFIM(PPUThread& CPU, ppu_opcode_t op);
	void VRFIN(PPUThread& CPU, ppu_opcode_t op);
	void VRFIP(PPUThread& CPU, ppu_opcode_t op);
	void VRFIZ(PPUThread& CPU, ppu_opcode_t op);
	void VRLB(PPUThread& CPU, ppu_opcode_t op);
	void VRLH(PPUThread& CPU, ppu_opcode_t op);
	void VRLW(PPUThread& CPU, ppu_opcode_t op);
	void VRSQRTEFP(PPUThread& CPU, ppu_opcode_t op);
	void VSEL(PPUThread& CPU, ppu_opcode_t op);
	void VSL(PPUThread& CPU, ppu_opcode_t op);
	void VSLB(PPUThread& CPU, ppu_opcode_t op);
	void VSLDOI(PPUThread& CPU, ppu_opcode_t op);
	void VSLH(PPUThread& CPU, ppu_opcode_t op);
	void VSLO(PPUThread& CPU, ppu_opcode_t op);
	void VSLW(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTB(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTH(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTISB(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTISH(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTISW(PPUThread& CPU, ppu_opcode_t op);
	void VSPLTW(PPUThread& CPU, ppu_opcode_t op);
	void VSR(PPUThread& CPU, ppu_opcode_t op);
	void VSRAB(PPUThread& CPU, ppu_opcode_t op);
	void VSRAH(PPUThread& CPU, ppu_opcode_t op);
	void VSRAW(PPUThread& CPU, ppu_opcode_t op);
	void VSRB(PPUThread& CPU, ppu_opcode_t op);
	void VSRH(PPUThread& CPU, ppu_opcode_t op);
	void VSRO(PPUThread& CPU, ppu_opcode_t op);
	void VSRW(PPUThread& CPU, ppu_opcode_t op);
	void VSUBCUW(PPUThread& CPU, ppu_opcode_t op);
	void VSUBFP(PPUThread& CPU, ppu_opcode_t op);
	void VSUBSBS(PPUThread& CPU, ppu_opcode_t op);
	void VSUBSHS(PPUThread& CPU, ppu_opcode_t op);
	void VSUBSWS(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUBM(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUBS(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUHM(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUHS(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUWM(PPUThread& CPU, ppu_opcode_t op);
	void VSUBUWS(PPUThread& CPU, ppu_opcode_t op);
	void VSUMSWS(PPUThread& CPU, ppu_opcode_t op);
	void VSUM2SWS(PPUThread& CPU, ppu_opcode_t op);
	void VSUM4SBS(PPUThread& CPU, ppu_opcode_t op);
	void VSUM4SHS(PPUThread& CPU, ppu_opcode_t op);
	void VSUM4UBS(PPUThread& CPU, ppu_opcode_t op);
	void VUPKHPX(PPUThread& CPU, ppu_opcode_t op);
	void VUPKHSB(PPUThread& CPU, ppu_opcode_t op);
	void VUPKHSH(PPUThread& CPU, ppu_opcode_t op);
	void VUPKLPX(PPUThread& CPU, ppu_opcode_t op);
	void VUPKLSB(PPUThread& CPU, ppu_opcode_t op);
	void VUPKLSH(PPUThread& CPU, ppu_opcode_t op);
	void VXOR(PPUThread& CPU, ppu_opcode_t op);
	void MULLI(PPUThread& CPU, ppu_opcode_t op);
	void SUBFIC(PPUThread& CPU, ppu_opcode_t op);
	void CMPLI(PPUThread& CPU, ppu_opcode_t op);
	void CMPI(PPUThread& CPU, ppu_opcode_t op);
	void ADDIC(PPUThread& CPU, ppu_opcode_t op);
	void ADDIC_(PPUThread& CPU, ppu_opcode_t op);
	void ADDI(PPUThread& CPU, ppu_opcode_t op);
	void ADDIS(PPUThread& CPU, ppu_opcode_t op);
	void BC(PPUThread& CPU, ppu_opcode_t op);
	void HACK(PPUThread& CPU, ppu_opcode_t op);
	void SC(PPUThread& CPU, ppu_opcode_t op);
	void B(PPUThread& CPU, ppu_opcode_t op);
	void MCRF(PPUThread& CPU, ppu_opcode_t op);
	void BCLR(PPUThread& CPU, ppu_opcode_t op);
	void CRNOR(PPUThread& CPU, ppu_opcode_t op);
	void CRANDC(PPUThread& CPU, ppu_opcode_t op);
	void ISYNC(PPUThread& CPU, ppu_opcode_t op);
	void CRXOR(PPUThread& CPU, ppu_opcode_t op);
	void CRNAND(PPUThread& CPU, ppu_opcode_t op);
	void CRAND(PPUThread& CPU, ppu_opcode_t op);
	void CREQV(PPUThread& CPU, ppu_opcode_t op);
	void CRORC(PPUThread& CPU, ppu_opcode_t op);
	void CROR(PPUThread& CPU, ppu_opcode_t op);
	void BCCTR(PPUThread& CPU, ppu_opcode_t op);
	void RLWIMI(PPUThread& CPU, ppu_opcode_t op);
	void RLWINM(PPUThread& CPU, ppu_opcode_t op);
	void RLWNM(PPUThread& CPU, ppu_opcode_t op);
	void ORI(PPUThread& CPU, ppu_opcode_t op);
	void ORIS(PPUThread& CPU, ppu_opcode_t op);
	void XORI(PPUThread& CPU, ppu_opcode_t op);
	void XORIS(PPUThread& CPU, ppu_opcode_t op);
	void ANDI_(PPUThread& CPU, ppu_opcode_t op);
	void ANDIS_(PPUThread& CPU, ppu_opcode_t op);
	void RLDICL(PPUThread& CPU, ppu_opcode_t op);
	void RLDICR(PPUThread& CPU, ppu_opcode_t op);
	void RLDIC(PPUThread& CPU, ppu_opcode_t op);
	void RLDIMI(PPUThread& CPU, ppu_opcode_t op);
	void RLDC_LR(PPUThread& CPU, ppu_opcode_t op);
	void CMP(PPUThread& CPU, ppu_opcode_t op);
	void TW(PPUThread& CPU, ppu_opcode_t op);
	void LVSL(PPUThread& CPU, ppu_opcode_t op);
	void LVEBX(PPUThread& CPU, ppu_opcode_t op);
	void SUBFC(PPUThread& CPU, ppu_opcode_t op);
	void MULHDU(PPUThread& CPU, ppu_opcode_t op);
	void ADDC(PPUThread& CPU, ppu_opcode_t op);
	void MULHWU(PPUThread& CPU, ppu_opcode_t op);
	void MFOCRF(PPUThread& CPU, ppu_opcode_t op);
	void LWARX(PPUThread& CPU, ppu_opcode_t op);
	void LDX(PPUThread& CPU, ppu_opcode_t op);
	void LWZX(PPUThread& CPU, ppu_opcode_t op);
	void SLW(PPUThread& CPU, ppu_opcode_t op);
	void CNTLZW(PPUThread& CPU, ppu_opcode_t op);
	void SLD(PPUThread& CPU, ppu_opcode_t op);
	void AND(PPUThread& CPU, ppu_opcode_t op);
	void CMPL(PPUThread& CPU, ppu_opcode_t op);
	void LVSR(PPUThread& CPU, ppu_opcode_t op);
	void LVEHX(PPUThread& CPU, ppu_opcode_t op);
	void SUBF(PPUThread& CPU, ppu_opcode_t op);
	void LDUX(PPUThread& CPU, ppu_opcode_t op);
	void DCBST(PPUThread& CPU, ppu_opcode_t op);
	void LWZUX(PPUThread& CPU, ppu_opcode_t op);
	void CNTLZD(PPUThread& CPU, ppu_opcode_t op);
	void ANDC(PPUThread& CPU, ppu_opcode_t op);
	void TD(PPUThread& CPU, ppu_opcode_t op);
	void LVEWX(PPUThread& CPU, ppu_opcode_t op);
	void MULHD(PPUThread& CPU, ppu_opcode_t op);
	void MULHW(PPUThread& CPU, ppu_opcode_t op);
	void LDARX(PPUThread& CPU, ppu_opcode_t op);
	void DCBF(PPUThread& CPU, ppu_opcode_t op);
	void LBZX(PPUThread& CPU, ppu_opcode_t op);
	void LVX(PPUThread& CPU, ppu_opcode_t op);
	void NEG(PPUThread& CPU, ppu_opcode_t op);
	void LBZUX(PPUThread& CPU, ppu_opcode_t op);
	void NOR(PPUThread& CPU, ppu_opcode_t op);
	void STVEBX(PPUThread& CPU, ppu_opcode_t op);
	void SUBFE(PPUThread& CPU, ppu_opcode_t op);
	void ADDE(PPUThread& CPU, ppu_opcode_t op);
	void MTOCRF(PPUThread& CPU, ppu_opcode_t op);
	void STDX(PPUThread& CPU, ppu_opcode_t op);
	void STWCX_(PPUThread& CPU, ppu_opcode_t op);
	void STWX(PPUThread& CPU, ppu_opcode_t op);
	void STVEHX(PPUThread& CPU, ppu_opcode_t op);
	void STDUX(PPUThread& CPU, ppu_opcode_t op);
	void STWUX(PPUThread& CPU, ppu_opcode_t op);
	void STVEWX(PPUThread& CPU, ppu_opcode_t op);
	void SUBFZE(PPUThread& CPU, ppu_opcode_t op);
	void ADDZE(PPUThread& CPU, ppu_opcode_t op);
	void STDCX_(PPUThread& CPU, ppu_opcode_t op);
	void STBX(PPUThread& CPU, ppu_opcode_t op);
	void STVX(PPUThread& CPU, ppu_opcode_t op);
	void MULLD(PPUThread& CPU, ppu_opcode_t op);
	void SUBFME(PPUThread& CPU, ppu_opcode_t op);
	void ADDME(PPUThread& CPU, ppu_opcode_t op);
	void MULLW(PPUThread& CPU, ppu_opcode_t op);
	void DCBTST(PPUThread& CPU, ppu_opcode_t op);
	void STBUX(PPUThread& CPU, ppu_opcode_t op);
	void ADD(PPUThread& CPU, ppu_opcode_t op);
	void DCBT(PPUThread& CPU, ppu_opcode_t op);
	void LHZX(PPUThread& CPU, ppu_opcode_t op);
	void EQV(PPUThread& CPU, ppu_opcode_t op);
	void ECIWX(PPUThread& CPU, ppu_opcode_t op);
	void LHZUX(PPUThread& CPU, ppu_opcode_t op);
	void XOR(PPUThread& CPU, ppu_opcode_t op);
	void MFSPR(PPUThread& CPU, ppu_opcode_t op);
	void LWAX(PPUThread& CPU, ppu_opcode_t op);
	void DST(PPUThread& CPU, ppu_opcode_t op);
	void LHAX(PPUThread& CPU, ppu_opcode_t op);
	void LVXL(PPUThread& CPU, ppu_opcode_t op);
	void MFTB(PPUThread& CPU, ppu_opcode_t op);
	void LWAUX(PPUThread& CPU, ppu_opcode_t op);
	void DSTST(PPUThread& CPU, ppu_opcode_t op);
	void LHAUX(PPUThread& CPU, ppu_opcode_t op);
	void STHX(PPUThread& CPU, ppu_opcode_t op);
	void ORC(PPUThread& CPU, ppu_opcode_t op);
	void ECOWX(PPUThread& CPU, ppu_opcode_t op);
	void STHUX(PPUThread& CPU, ppu_opcode_t op);
	void OR(PPUThread& CPU, ppu_opcode_t op);
	void DIVDU(PPUThread& CPU, ppu_opcode_t op);
	void DIVWU(PPUThread& CPU, ppu_opcode_t op);
	void MTSPR(PPUThread& CPU, ppu_opcode_t op);
	void DCBI(PPUThread& CPU, ppu_opcode_t op);
	void NAND(PPUThread& CPU, ppu_opcode_t op);
	void STVXL(PPUThread& CPU, ppu_opcode_t op);
	void DIVD(PPUThread& CPU, ppu_opcode_t op);
	void DIVW(PPUThread& CPU, ppu_opcode_t op);
	void LVLX(PPUThread& CPU, ppu_opcode_t op);
	void LDBRX(PPUThread& CPU, ppu_opcode_t op);
	void LSWX(PPUThread& CPU, ppu_opcode_t op);
	void LWBRX(PPUThread& CPU, ppu_opcode_t op);
	void LFSX(PPUThread& CPU, ppu_opcode_t op);
	void SRW(PPUThread& CPU, ppu_opcode_t op);
	void SRD(PPUThread& CPU, ppu_opcode_t op);
	void LVRX(PPUThread& CPU, ppu_opcode_t op);
	void LSWI(PPUThread& CPU, ppu_opcode_t op);
	void LFSUX(PPUThread& CPU, ppu_opcode_t op);
	void SYNC(PPUThread& CPU, ppu_opcode_t op);
	void LFDX(PPUThread& CPU, ppu_opcode_t op);
	void LFDUX(PPUThread& CPU, ppu_opcode_t op);
	void STVLX(PPUThread& CPU, ppu_opcode_t op);
	void STDBRX(PPUThread& CPU, ppu_opcode_t op);
	void STSWX(PPUThread& CPU, ppu_opcode_t op);
	void STWBRX(PPUThread& CPU, ppu_opcode_t op);
	void STFSX(PPUThread& CPU, ppu_opcode_t op);
	void STVRX(PPUThread& CPU, ppu_opcode_t op);
	void STFSUX(PPUThread& CPU, ppu_opcode_t op);
	void STSWI(PPUThread& CPU, ppu_opcode_t op);
	void STFDX(PPUThread& CPU, ppu_opcode_t op);
	void STFDUX(PPUThread& CPU, ppu_opcode_t op);
	void LVLXL(PPUThread& CPU, ppu_opcode_t op);
	void LHBRX(PPUThread& CPU, ppu_opcode_t op);
	void SRAW(PPUThread& CPU, ppu_opcode_t op);
	void SRAD(PPUThread& CPU, ppu_opcode_t op);
	void LVRXL(PPUThread& CPU, ppu_opcode_t op);
	void DSS(PPUThread& CPU, ppu_opcode_t op);
	void SRAWI(PPUThread& CPU, ppu_opcode_t op);
	void SRADI(PPUThread& CPU, ppu_opcode_t op);
	void EIEIO(PPUThread& CPU, ppu_opcode_t op);
	void STVLXL(PPUThread& CPU, ppu_opcode_t op);
	void STHBRX(PPUThread& CPU, ppu_opcode_t op);
	void EXTSH(PPUThread& CPU, ppu_opcode_t op);
	void STVRXL(PPUThread& CPU, ppu_opcode_t op);
	void EXTSB(PPUThread& CPU, ppu_opcode_t op);
	void STFIWX(PPUThread& CPU, ppu_opcode_t op);
	void EXTSW(PPUThread& CPU, ppu_opcode_t op);
	void ICBI(PPUThread& CPU, ppu_opcode_t op);
	void DCBZ(PPUThread& CPU, ppu_opcode_t op);
	void LWZ(PPUThread& CPU, ppu_opcode_t op);
	void LWZU(PPUThread& CPU, ppu_opcode_t op);
	void LBZ(PPUThread& CPU, ppu_opcode_t op);
	void LBZU(PPUThread& CPU, ppu_opcode_t op);
	void STW(PPUThread& CPU, ppu_opcode_t op);
	void STWU(PPUThread& CPU, ppu_opcode_t op);
	void STB(PPUThread& CPU, ppu_opcode_t op);
	void STBU(PPUThread& CPU, ppu_opcode_t op);
	void LHZ(PPUThread& CPU, ppu_opcode_t op);
	void LHZU(PPUThread& CPU, ppu_opcode_t op);
	void LHA(PPUThread& CPU, ppu_opcode_t op);
	void LHAU(PPUThread& CPU, ppu_opcode_t op);
	void STH(PPUThread& CPU, ppu_opcode_t op);
	void STHU(PPUThread& CPU, ppu_opcode_t op);
	void LMW(PPUThread& CPU, ppu_opcode_t op);
	void STMW(PPUThread& CPU, ppu_opcode_t op);
	void LFS(PPUThread& CPU, ppu_opcode_t op);
	void LFSU(PPUThread& CPU, ppu_opcode_t op);
	void LFD(PPUThread& CPU, ppu_opcode_t op);
	void LFDU(PPUThread& CPU, ppu_opcode_t op);
	void STFS(PPUThread& CPU, ppu_opcode_t op);
	void STFSU(PPUThread& CPU, ppu_opcode_t op);
	void STFD(PPUThread& CPU, ppu_opcode_t op);
	void STFDU(PPUThread& CPU, ppu_opcode_t op);
	void LD(PPUThread& CPU, ppu_opcode_t op);
	void LDU(PPUThread& CPU, ppu_opcode_t op);
	void LWA(PPUThread& CPU, ppu_opcode_t op);
	void FDIVS(PPUThread& CPU, ppu_opcode_t op);
	void FSUBS(PPUThread& CPU, ppu_opcode_t op);
	void FADDS(PPUThread& CPU, ppu_opcode_t op);
	void FSQRTS(PPUThread& CPU, ppu_opcode_t op);
	void FRES(PPUThread& CPU, ppu_opcode_t op);
	void FMULS(PPUThread& CPU, ppu_opcode_t op);
	void FMADDS(PPUThread& CPU, ppu_opcode_t op);
	void FMSUBS(PPUThread& CPU, ppu_opcode_t op);
	void FNMSUBS(PPUThread& CPU, ppu_opcode_t op);
	void FNMADDS(PPUThread& CPU, ppu_opcode_t op);
	void STD(PPUThread& CPU, ppu_opcode_t op);
	void STDU(PPUThread& CPU, ppu_opcode_t op);
	void MTFSB1(PPUThread& CPU, ppu_opcode_t op);
	void MCRFS(PPUThread& CPU, ppu_opcode_t op);
	void MTFSB0(PPUThread& CPU, ppu_opcode_t op);
	void MTFSFI(PPUThread& CPU, ppu_opcode_t op);
	void MFFS(PPUThread& CPU, ppu_opcode_t op);
	void MTFSF(PPUThread& CPU, ppu_opcode_t op);

	void FCMPU(PPUThread& CPU, ppu_opcode_t op);
	void FRSP(PPUThread& CPU, ppu_opcode_t op);
	void FCTIW(PPUThread& CPU, ppu_opcode_t op);
	void FCTIWZ(PPUThread& CPU, ppu_opcode_t op);
	void FDIV(PPUThread& CPU, ppu_opcode_t op);
	void FSUB(PPUThread& CPU, ppu_opcode_t op);
	void FADD(PPUThread& CPU, ppu_opcode_t op);
	void FSQRT(PPUThread& CPU, ppu_opcode_t op);
	void FSEL(PPUThread& CPU, ppu_opcode_t op);
	void FMUL(PPUThread& CPU, ppu_opcode_t op);
	void FRSQRTE(PPUThread& CPU, ppu_opcode_t op);
	void FMSUB(PPUThread& CPU, ppu_opcode_t op);
	void FMADD(PPUThread& CPU, ppu_opcode_t op);
	void FNMSUB(PPUThread& CPU, ppu_opcode_t op);
	void FNMADD(PPUThread& CPU, ppu_opcode_t op);
	void FCMPO(PPUThread& CPU, ppu_opcode_t op);
	void FNEG(PPUThread& CPU, ppu_opcode_t op);
	void FMR(PPUThread& CPU, ppu_opcode_t op);
	void FNABS(PPUThread& CPU, ppu_opcode_t op);
	void FABS(PPUThread& CPU, ppu_opcode_t op);
	void FCTID(PPUThread& CPU, ppu_opcode_t op);
	void FCTIDZ(PPUThread& CPU, ppu_opcode_t op);
	void FCFID(PPUThread& CPU, ppu_opcode_t op);

	void UNK(PPUThread& CPU, ppu_opcode_t op);
}

class PPUInterpreter2 : public PPUOpcodes
{
public:
	virtual ~PPUInterpreter2() {}

	ppu_inter_func_t func;

	virtual void NULL_OP() { func = ppu_interpreter::NULL_OP; }
	virtual void NOP() { func = ppu_interpreter::NOP; }

	virtual void TDI(u32 to, u32 ra, s32 simm16) { func = ppu_interpreter::TDI; }
	virtual void TWI(u32 to, u32 ra, s32 simm16) { func = ppu_interpreter::TWI; }

	virtual void MFVSCR(u32 vd) { func = ppu_interpreter::MFVSCR; }
	virtual void MTVSCR(u32 vb) { func = ppu_interpreter::MTVSCR; }
	virtual void VADDCUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDCUW; }
	virtual void VADDFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDFP; }
	virtual void VADDSBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDSBS; }
	virtual void VADDSHS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDSHS; }
	virtual void VADDSWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDSWS; }
	virtual void VADDUBM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUBM; }
	virtual void VADDUBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUBS; }
	virtual void VADDUHM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUHM; }
	virtual void VADDUHS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUHS; }
	virtual void VADDUWM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUWM; }
	virtual void VADDUWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VADDUWS; }
	virtual void VAND(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAND; }
	virtual void VANDC(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VANDC; }
	virtual void VAVGSB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGSB; }
	virtual void VAVGSH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGSH; }
	virtual void VAVGSW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGSW; }
	virtual void VAVGUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGUB; }
	virtual void VAVGUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGUH; }
	virtual void VAVGUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VAVGUW; }
	virtual void VCFSX(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VCFSX; }
	virtual void VCFUX(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VCFUX; }
	virtual void VCMPBFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPBFP; }
	virtual void VCMPBFP_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPBFP_; }
	virtual void VCMPEQFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQFP; }
	virtual void VCMPEQFP_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQFP_; }
	virtual void VCMPEQUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUB; }
	virtual void VCMPEQUB_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUB_; }
	virtual void VCMPEQUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUH; }
	virtual void VCMPEQUH_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUH_; }
	virtual void VCMPEQUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUW; }
	virtual void VCMPEQUW_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPEQUW_; }
	virtual void VCMPGEFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGEFP; }
	virtual void VCMPGEFP_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGEFP_; }
	virtual void VCMPGTFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTFP; }
	virtual void VCMPGTFP_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTFP_; }
	virtual void VCMPGTSB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSB; }
	virtual void VCMPGTSB_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSB_; }
	virtual void VCMPGTSH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSH; }
	virtual void VCMPGTSH_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSH_; }
	virtual void VCMPGTSW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSW; }
	virtual void VCMPGTSW_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTSW_; }
	virtual void VCMPGTUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUB; }
	virtual void VCMPGTUB_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUB_; }
	virtual void VCMPGTUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUH; }
	virtual void VCMPGTUH_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUH_; }
	virtual void VCMPGTUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUW; }
	virtual void VCMPGTUW_(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VCMPGTUW_; }
	virtual void VCTSXS(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VCTSXS; }
	virtual void VCTUXS(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VCTUXS; }
	virtual void VEXPTEFP(u32 vd, u32 vb) { func = ppu_interpreter::VEXPTEFP; }
	virtual void VLOGEFP(u32 vd, u32 vb) { func = ppu_interpreter::VLOGEFP; }
	virtual void VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) { func = ppu_interpreter::VMADDFP; }
	virtual void VMAXFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXFP; }
	virtual void VMAXSB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXSB; }
	virtual void VMAXSH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXSH; }
	virtual void VMAXSW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXSW; }
	virtual void VMAXUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXUB; }
	virtual void VMAXUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXUH; }
	virtual void VMAXUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMAXUW; }
	virtual void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMHADDSHS; }
	virtual void VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMHRADDSHS; }
	virtual void VMINFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINFP; }
	virtual void VMINSB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINSB; }
	virtual void VMINSH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINSH; }
	virtual void VMINSW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINSW; }
	virtual void VMINUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINUB; }
	virtual void VMINUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINUH; }
	virtual void VMINUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMINUW; }
	virtual void VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMLADDUHM; }
	virtual void VMRGHB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGHB; }
	virtual void VMRGHH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGHH; }
	virtual void VMRGHW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGHW; }
	virtual void VMRGLB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGLB; }
	virtual void VMRGLH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGLH; }
	virtual void VMRGLW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMRGLW; }
	virtual void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMMBM; }
	virtual void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMSHM; }
	virtual void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMSHS; }
	virtual void VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMUBM; }
	virtual void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMUHM; }
	virtual void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VMSUMUHS; }
	virtual void VMULESB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULESB; }
	virtual void VMULESH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULESH; }
	virtual void VMULEUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULEUB; }
	virtual void VMULEUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULEUH; }
	virtual void VMULOSB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULOSB; }
	virtual void VMULOSH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULOSH; }
	virtual void VMULOUB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULOUB; }
	virtual void VMULOUH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VMULOUH; }
	virtual void VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) { func = ppu_interpreter::VNMSUBFP; }
	virtual void VNOR(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VNOR; }
	virtual void VOR(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VOR; }
	virtual void VPERM(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VPERM; }
	virtual void VPKPX(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKPX; }
	virtual void VPKSHSS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKSHSS; }
	virtual void VPKSHUS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKSHUS; }
	virtual void VPKSWSS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKSWSS; }
	virtual void VPKSWUS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKSWUS; }
	virtual void VPKUHUM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKUHUM; }
	virtual void VPKUHUS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKUHUS; }
	virtual void VPKUWUM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKUWUM; }
	virtual void VPKUWUS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VPKUWUS; }
	virtual void VREFP(u32 vd, u32 vb) { func = ppu_interpreter::VREFP; }
	virtual void VRFIM(u32 vd, u32 vb) { func = ppu_interpreter::VRFIM; }
	virtual void VRFIN(u32 vd, u32 vb) { func = ppu_interpreter::VRFIN; }
	virtual void VRFIP(u32 vd, u32 vb) { func = ppu_interpreter::VRFIP; }
	virtual void VRFIZ(u32 vd, u32 vb) { func = ppu_interpreter::VRFIZ; }
	virtual void VRLB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VRLB; }
	virtual void VRLH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VRLH; }
	virtual void VRLW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VRLW; }
	virtual void VRSQRTEFP(u32 vd, u32 vb) { func = ppu_interpreter::VRSQRTEFP; }
	virtual void VSEL(u32 vd, u32 va, u32 vb, u32 vc) { func = ppu_interpreter::VSEL; }
	virtual void VSL(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSL; }
	virtual void VSLB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSLB; }
	virtual void VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) { func = ppu_interpreter::VSLDOI; }
	virtual void VSLH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSLH; }
	virtual void VSLO(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSLO; }
	virtual void VSLW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSLW; }
	virtual void VSPLTB(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VSPLTB; }
	virtual void VSPLTH(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VSPLTH; }
	virtual void VSPLTISB(u32 vd, s32 simm5) { func = ppu_interpreter::VSPLTISB; }
	virtual void VSPLTISH(u32 vd, s32 simm5) { func = ppu_interpreter::VSPLTISH; }
	virtual void VSPLTISW(u32 vd, s32 simm5) { func = ppu_interpreter::VSPLTISW; }
	virtual void VSPLTW(u32 vd, u32 uimm5, u32 vb) { func = ppu_interpreter::VSPLTW; }
	virtual void VSR(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSR; }
	virtual void VSRAB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRAB; }
	virtual void VSRAH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRAH; }
	virtual void VSRAW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRAW; }
	virtual void VSRB(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRB; }
	virtual void VSRH(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRH; }
	virtual void VSRO(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRO; }
	virtual void VSRW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSRW; }
	virtual void VSUBCUW(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBCUW; }
	virtual void VSUBFP(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBFP; }
	virtual void VSUBSBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBSBS; }
	virtual void VSUBSHS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBSHS; }
	virtual void VSUBSWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBSWS; }
	virtual void VSUBUBM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUBM; }
	virtual void VSUBUBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUBS; }
	virtual void VSUBUHM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUHM; }
	virtual void VSUBUHS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUHS; }
	virtual void VSUBUWM(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUWM; }
	virtual void VSUBUWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUBUWS; }
	virtual void VSUMSWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUMSWS; }
	virtual void VSUM2SWS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUM2SWS; }
	virtual void VSUM4SBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUM4SBS; }
	virtual void VSUM4SHS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUM4SHS; }
	virtual void VSUM4UBS(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VSUM4UBS; }
	virtual void VUPKHPX(u32 vd, u32 vb) { func = ppu_interpreter::VUPKHPX; }
	virtual void VUPKHSB(u32 vd, u32 vb) { func = ppu_interpreter::VUPKHSB; }
	virtual void VUPKHSH(u32 vd, u32 vb) { func = ppu_interpreter::VUPKHSH; }
	virtual void VUPKLPX(u32 vd, u32 vb) { func = ppu_interpreter::VUPKLPX; }
	virtual void VUPKLSB(u32 vd, u32 vb) { func = ppu_interpreter::VUPKLSB; }
	virtual void VUPKLSH(u32 vd, u32 vb) { func = ppu_interpreter::VUPKLSH; }
	virtual void VXOR(u32 vd, u32 va, u32 vb) { func = ppu_interpreter::VXOR; }
	virtual void MULLI(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::MULLI; }
	virtual void SUBFIC(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::SUBFIC; }
	virtual void CMPLI(u32 bf, u32 l, u32 ra, u32 uimm16) { func = ppu_interpreter::CMPLI; }
	virtual void CMPI(u32 bf, u32 l, u32 ra, s32 simm16) { func = ppu_interpreter::CMPI; }
	virtual void ADDIC(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::ADDIC; }
	virtual void ADDIC_(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::ADDIC_; }
	virtual void ADDI(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::ADDI; }
	virtual void ADDIS(u32 rd, u32 ra, s32 simm16) { func = ppu_interpreter::ADDIS; }
	virtual void BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) { func = ppu_interpreter::BC; }
	virtual void HACK(u32 index) { func = ppu_interpreter::HACK; }
	virtual void SC(u32 lev) { func = ppu_interpreter::SC; }
	virtual void B(s32 ll, u32 aa, u32 lk) { func = ppu_interpreter::B; }
	virtual void MCRF(u32 crfd, u32 crfs) { func = ppu_interpreter::MCRF; }
	virtual void BCLR(u32 bo, u32 bi, u32 bh, u32 lk) { func = ppu_interpreter::BCLR; }
	virtual void CRNOR(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRNOR; }
	virtual void CRANDC(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRANDC; }
	virtual void ISYNC() { func = ppu_interpreter::ISYNC; }
	virtual void CRXOR(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRXOR; }
	virtual void CRNAND(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRNAND; }
	virtual void CRAND(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRAND; }
	virtual void CREQV(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CREQV; }
	virtual void CRORC(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CRORC; }
	virtual void CROR(u32 bt, u32 ba, u32 bb) { func = ppu_interpreter::CROR; }
	virtual void BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) { func = ppu_interpreter::BCCTR; }
	virtual void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) { func = ppu_interpreter::RLWIMI; }
	virtual void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) { func = ppu_interpreter::RLWINM; }
	virtual void RLWNM(u32 ra, u32 rs, u32 rb, u32 MB, u32 ME, bool rc) { func = ppu_interpreter::RLWNM; }
	virtual void ORI(u32 rs, u32 ra, u32 uimm16) { func = ppu_interpreter::ORI; }
	virtual void ORIS(u32 rs, u32 ra, u32 uimm16) { func = ppu_interpreter::ORIS; }
	virtual void XORI(u32 ra, u32 rs, u32 uimm16) { func = ppu_interpreter::XORI; }
	virtual void XORIS(u32 ra, u32 rs, u32 uimm16) { func = ppu_interpreter::XORIS; }
	virtual void ANDI_(u32 ra, u32 rs, u32 uimm16) { func = ppu_interpreter::ANDI_; }
	virtual void ANDIS_(u32 ra, u32 rs, u32 uimm16) { func = ppu_interpreter::ANDIS_; }
	virtual void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) { func = ppu_interpreter::RLDICL; }
	virtual void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) { func = ppu_interpreter::RLDICR; }
	virtual void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) { func = ppu_interpreter::RLDIC; }
	virtual void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) { func = ppu_interpreter::RLDIMI; }
	virtual void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) { func = ppu_interpreter::RLDC_LR; }
	virtual void CMP(u32 crfd, u32 l, u32 ra, u32 rb) { func = ppu_interpreter::CMP; }
	virtual void TW(u32 to, u32 ra, u32 rb) { func = ppu_interpreter::TW; }
	virtual void LVSL(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVSL; }
	virtual void LVEBX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVEBX; }
	virtual void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::SUBFC; }
	virtual void MULHDU(u32 rd, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::MULHDU; }
	virtual void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::ADDC; }
	virtual void MULHWU(u32 rd, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::MULHWU; }
	virtual void MFOCRF(u32 a, u32 rd, u32 crm) { func = ppu_interpreter::MFOCRF; }
	virtual void LWARX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWARX; }
	virtual void LDX(u32 ra, u32 rs, u32 rb) { func = ppu_interpreter::LDX; }
	virtual void LWZX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWZX; }
	virtual void SLW(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SLW; }
	virtual void CNTLZW(u32 ra, u32 rs, bool rc) { func = ppu_interpreter::CNTLZW; }
	virtual void SLD(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SLD; }
	virtual void AND(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::AND; }
	virtual void CMPL(u32 bf, u32 l, u32 ra, u32 rb) { func = ppu_interpreter::CMPL; }
	virtual void LVSR(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVSR; }
	virtual void LVEHX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVEHX; }
	virtual void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::SUBF; }
	virtual void LDUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LDUX; }
	virtual void DCBST(u32 ra, u32 rb) { func = ppu_interpreter::DCBST; }
	virtual void LWZUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWZUX; }
	virtual void CNTLZD(u32 ra, u32 rs, bool rc) { func = ppu_interpreter::CNTLZD; }
	virtual void ANDC(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::ANDC; }
	virtual void TD(u32 to, u32 ra, u32 rb) { func = ppu_interpreter::TD; }
	virtual void LVEWX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVEWX; }
	virtual void MULHD(u32 rd, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::MULHD; }
	virtual void MULHW(u32 rd, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::MULHW; }
	virtual void LDARX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LDARX; }
	virtual void DCBF(u32 ra, u32 rb) { func = ppu_interpreter::DCBF; }
	virtual void LBZX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LBZX; }
	virtual void LVX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVX; }
	virtual void NEG(u32 rd, u32 ra, u32 oe, bool rc) { func = ppu_interpreter::NEG; }
	virtual void LBZUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LBZUX; }
	virtual void NOR(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::NOR; }
	virtual void STVEBX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVEBX; }
	virtual void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::SUBFE; }
	virtual void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::ADDE; }
	virtual void MTOCRF(u32 l, u32 crm, u32 rs) { func = ppu_interpreter::MTOCRF; }
	virtual void STDX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STDX; }
	virtual void STWCX_(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STWCX_; }
	virtual void STWX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STWX; }
	virtual void STVEHX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVEHX; }
	virtual void STDUX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STDUX; }
	virtual void STWUX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STWUX; }
	virtual void STVEWX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVEWX; }
	virtual void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) { func = ppu_interpreter::SUBFZE; }
	virtual void ADDZE(u32 rd, u32 ra, u32 oe, bool rc) { func = ppu_interpreter::ADDZE; }
	virtual void STDCX_(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STDCX_; }
	virtual void STBX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STBX; }
	virtual void STVX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVX; }
	virtual void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::MULLD; }
	virtual void SUBFME(u32 rd, u32 ra, u32 oe, bool rc) { func = ppu_interpreter::SUBFME; }
	virtual void ADDME(u32 rd, u32 ra, u32 oe, bool rc) { func = ppu_interpreter::ADDME; }
	virtual void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::MULLW; }
	virtual void DCBTST(u32 ra, u32 rb, u32 th) { func = ppu_interpreter::DCBTST; }
	virtual void STBUX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STBUX; }
	virtual void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::ADD; }
	virtual void DCBT(u32 ra, u32 rb, u32 th) { func = ppu_interpreter::DCBT; }
	virtual void LHZX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LHZX; }
	virtual void EQV(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::EQV; }
	virtual void ECIWX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::ECIWX; }
	virtual void LHZUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LHZUX; }
	virtual void XOR(u32 rs, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::XOR; }
	virtual void MFSPR(u32 rd, u32 spr) { func = ppu_interpreter::MFSPR; }
	virtual void LWAX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWAX; }
	virtual void DST(u32 ra, u32 rb, u32 strm, u32 t) { func = ppu_interpreter::DST; }
	virtual void LHAX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LHAX; }
	virtual void LVXL(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVXL; }
	virtual void MFTB(u32 rd, u32 spr) { func = ppu_interpreter::MFTB; }
	virtual void LWAUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWAUX; }
	virtual void DSTST(u32 ra, u32 rb, u32 strm, u32 t) { func = ppu_interpreter::DSTST; }
	virtual void LHAUX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LHAUX; }
	virtual void STHX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STHX; }
	virtual void ORC(u32 rs, u32 ra, u32 rb, bool rc) { func = ppu_interpreter::ORC; }
	virtual void ECOWX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::ECOWX; }
	virtual void STHUX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STHUX; }
	virtual void OR(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::OR; }
	virtual void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::DIVDU; }
	virtual void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::DIVWU; }
	virtual void MTSPR(u32 spr, u32 rs) { func = ppu_interpreter::MTSPR; }
	virtual void DCBI(u32 ra, u32 rb) { func = ppu_interpreter::DCBI; }
	virtual void NAND(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::NAND; }
	virtual void STVXL(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVXL; }
	virtual void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::DIVD; }
	virtual void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) { func = ppu_interpreter::DIVW; }
	virtual void LVLX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVLX; }
	virtual void LDBRX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LDBRX; }
	virtual void LSWX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LSWX; }
	virtual void LWBRX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LWBRX; }
	virtual void LFSX(u32 frd, u32 ra, u32 rb) { func = ppu_interpreter::LFSX; }
	virtual void SRW(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SRW; }
	virtual void SRD(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SRD; }
	virtual void LVRX(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVRX; }
	virtual void LSWI(u32 rd, u32 ra, u32 nb) { func = ppu_interpreter::LSWI; }
	virtual void LFSUX(u32 frd, u32 ra, u32 rb) { func = ppu_interpreter::LFSUX; }
	virtual void SYNC(u32 l) { func = ppu_interpreter::SYNC; }
	virtual void LFDX(u32 frd, u32 ra, u32 rb) { func = ppu_interpreter::LFDX; }
	virtual void LFDUX(u32 frd, u32 ra, u32 rb) { func = ppu_interpreter::LFDUX; }
	virtual void STVLX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVLX; }
	virtual void STDBRX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STDBRX; }
	virtual void STSWX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STSWX; }
	virtual void STWBRX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STWBRX; }
	virtual void STFSX(u32 frs, u32 ra, u32 rb) { func = ppu_interpreter::STFSX; }
	virtual void STVRX(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVRX; }
	virtual void STFSUX(u32 frs, u32 ra, u32 rb) { func = ppu_interpreter::STFSUX; }
	virtual void STSWI(u32 rd, u32 ra, u32 nb) { func = ppu_interpreter::STSWI; }
	virtual void STFDX(u32 frs, u32 ra, u32 rb) { func = ppu_interpreter::STFDX; }
	virtual void STFDUX(u32 frs, u32 ra, u32 rb) { func = ppu_interpreter::STFDUX; }
	virtual void LVLXL(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVLXL; }
	virtual void LHBRX(u32 rd, u32 ra, u32 rb) { func = ppu_interpreter::LHBRX; }
	virtual void SRAW(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SRAW; }
	virtual void SRAD(u32 ra, u32 rs, u32 rb, bool rc) { func = ppu_interpreter::SRAD; }
	virtual void LVRXL(u32 vd, u32 ra, u32 rb) { func = ppu_interpreter::LVRXL; }
	virtual void DSS(u32 strm, u32 a) { func = ppu_interpreter::DSS; }
	virtual void SRAWI(u32 ra, u32 rs, u32 sh, bool rc) { func = ppu_interpreter::SRAWI; }
	virtual void SRADI1(u32 ra, u32 rs, u32 sh, bool rc) { func = ppu_interpreter::SRADI; }
	virtual void SRADI2(u32 ra, u32 rs, u32 sh, bool rc) { func = ppu_interpreter::SRADI; }
	virtual void EIEIO() { func = ppu_interpreter::EIEIO; }
	virtual void STVLXL(u32 vs, u32 ra, u32 rb) { func = ppu_interpreter::STVLXL; }
	virtual void STHBRX(u32 rs, u32 ra, u32 rb) { func = ppu_interpreter::STHBRX; }
	virtual void EXTSH(u32 ra, u32 rs, bool rc) { func = ppu_interpreter::EXTSH; }
	virtual void STVRXL(u32 sd, u32 ra, u32 rb) { func = ppu_interpreter::STVRXL; }
	virtual void EXTSB(u32 ra, u32 rs, bool rc) { func = ppu_interpreter::EXTSB; }
	virtual void STFIWX(u32 frs, u32 ra, u32 rb) { func = ppu_interpreter::STFIWX; }
	virtual void EXTSW(u32 ra, u32 rs, bool rc) { func = ppu_interpreter::EXTSW; }
	virtual void ICBI(u32 ra, u32 rb) { func = ppu_interpreter::ICBI; }
	virtual void DCBZ(u32 ra, u32 rb) { func = ppu_interpreter::DCBZ; }
	virtual void LWZ(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LWZ; }
	virtual void LWZU(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LWZU; }
	virtual void LBZ(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LBZ; }
	virtual void LBZU(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LBZU; }
	virtual void STW(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STW; }
	virtual void STWU(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STWU; }
	virtual void STB(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STB; }
	virtual void STBU(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STBU; }
	virtual void LHZ(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LHZ; }
	virtual void LHZU(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LHZU; }
	virtual void LHA(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::LHA; }
	virtual void LHAU(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::LHAU; }
	virtual void STH(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STH; }
	virtual void STHU(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STHU; }
	virtual void LMW(u32 rd, u32 ra, s32 d) { func = ppu_interpreter::LMW; }
	virtual void STMW(u32 rs, u32 ra, s32 d) { func = ppu_interpreter::STMW; }
	virtual void LFS(u32 frd, u32 ra, s32 d) { func = ppu_interpreter::LFS; }
	virtual void LFSU(u32 frd, u32 ra, s32 d) { func = ppu_interpreter::LFSU; }
	virtual void LFD(u32 frd, u32 ra, s32 d) { func = ppu_interpreter::LFD; }
	virtual void LFDU(u32 frd, u32 ra, s32 d) { func = ppu_interpreter::LFDU; }
	virtual void STFS(u32 frs, u32 ra, s32 d) { func = ppu_interpreter::STFS; }
	virtual void STFSU(u32 frs, u32 ra, s32 d) { func = ppu_interpreter::STFSU; }
	virtual void STFD(u32 frs, u32 ra, s32 d) { func = ppu_interpreter::STFD; }
	virtual void STFDU(u32 frs, u32 ra, s32 d) { func = ppu_interpreter::STFDU; }
	virtual void LD(u32 rd, u32 ra, s32 ds) { func = ppu_interpreter::LD; }
	virtual void LDU(u32 rd, u32 ra, s32 ds) { func = ppu_interpreter::LDU; }
	virtual void LWA(u32 rd, u32 ra, s32 ds) { func = ppu_interpreter::LWA; }
	virtual void FDIVS(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FDIVS; }
	virtual void FSUBS(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FSUBS; }
	virtual void FADDS(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FADDS; }
	virtual void FSQRTS(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FSQRTS; }
	virtual void FRES(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FRES; }
	virtual void FMULS(u32 frd, u32 fra, u32 frc, bool rc) { func = ppu_interpreter::FMULS; }
	virtual void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FMADDS; }
	virtual void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FMSUBS; }
	virtual void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FNMSUBS; }
	virtual void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FNMADDS; }
	virtual void STD(u32 rs, u32 ra, s32 ds) { func = ppu_interpreter::STD; }
	virtual void STDU(u32 rs, u32 ra, s32 ds) { func = ppu_interpreter::STDU; }
	virtual void MTFSB1(u32 bt, bool rc) { func = ppu_interpreter::MTFSB1; }
	virtual void MCRFS(u32 bf, u32 bfa) { func = ppu_interpreter::MCRFS; }
	virtual void MTFSB0(u32 bt, bool rc) { func = ppu_interpreter::MTFSB0; }
	virtual void MTFSFI(u32 crfd, u32 i, bool rc) { func = ppu_interpreter::MTFSFI; }
	virtual void MFFS(u32 frd, bool rc) { func = ppu_interpreter::MFFS; }
	virtual void MTFSF(u32 flm, u32 frb, bool rc) { func = ppu_interpreter::MTFSF; }

	virtual void FCMPU(u32 bf, u32 fra, u32 frb) { func = ppu_interpreter::FCMPU; }
	virtual void FRSP(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FRSP; }
	virtual void FCTIW(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FCTIW; }
	virtual void FCTIWZ(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FCTIWZ; }
	virtual void FDIV(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FDIV; }
	virtual void FSUB(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FSUB; }
	virtual void FADD(u32 frd, u32 fra, u32 frb, bool rc) { func = ppu_interpreter::FADD; }
	virtual void FSQRT(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FSQRT; }
	virtual void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FSEL; }
	virtual void FMUL(u32 frd, u32 fra, u32 frc, bool rc) { func = ppu_interpreter::FMUL; }
	virtual void FRSQRTE(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FRSQRTE; }
	virtual void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FMSUB; }
	virtual void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FMADD; }
	virtual void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FNMSUB; }
	virtual void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) { func = ppu_interpreter::FNMADD; }
	virtual void FCMPO(u32 crfd, u32 fra, u32 frb) { func = ppu_interpreter::FCMPO; }
	virtual void FNEG(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FNEG; }
	virtual void FMR(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FMR; }
	virtual void FNABS(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FNABS; }
	virtual void FABS(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FABS; }
	virtual void FCTID(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FCTID; }
	virtual void FCTIDZ(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FCTIDZ; }
	virtual void FCFID(u32 frd, u32 frb, bool rc) { func = ppu_interpreter::FCFID; }

	virtual void UNK(const u32 code, const u32 opcode, const u32 gcode) { func = ppu_interpreter::UNK; }
};