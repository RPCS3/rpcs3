#include "stdafx.h"
#include "PPUDisAsm.h"
#include "PPUFunction.h"

const ppu_decoder<PPUDisAsm> s_ppu_disasm;

u32 PPUDisAsm::disasm(u32 pc)
{
	const u32 op = *(be_t<u32>*)(offset + pc);
	(this->*(s_ppu_disasm.decode(op)))({ op });
	return 4;
}

void PPUDisAsm::MFVSCR(ppu_opcode_t op)
{
	DisAsm_V1("mfvscr", op.vd);
}

void PPUDisAsm::MTVSCR(ppu_opcode_t op)
{
	DisAsm_V1("mtvscr", op.vb);
}

void PPUDisAsm::VADDCUW(ppu_opcode_t op)
{
	DisAsm_V3("vaddcuw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDFP(ppu_opcode_t op)
{
	DisAsm_V3("vaddfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDSBS(ppu_opcode_t op)
{
	DisAsm_V3("vaddsbs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDSHS(ppu_opcode_t op)
{
	DisAsm_V3("vaddshs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDSWS(ppu_opcode_t op)
{
	DisAsm_V3("vaddsws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUBM(ppu_opcode_t op)
{
	DisAsm_V3("vaddubm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUBS(ppu_opcode_t op)
{
	DisAsm_V3("vaddubs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUHM(ppu_opcode_t op)
{
	DisAsm_V3("vadduhm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUHS(ppu_opcode_t op)
{
	DisAsm_V3("vadduhs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUWM(ppu_opcode_t op)
{
	DisAsm_V3("vadduwm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VADDUWS(ppu_opcode_t op)
{
	DisAsm_V3("vadduws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAND(ppu_opcode_t op)
{
	DisAsm_V3("vand", op.vd, op.va, op.vb);
}

void PPUDisAsm::VANDC(ppu_opcode_t op)
{
	DisAsm_V3("vandc", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGSB(ppu_opcode_t op)
{
	DisAsm_V3("vavgsb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGSH(ppu_opcode_t op)
{
	DisAsm_V3("vavgsh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGSW(ppu_opcode_t op)
{
	DisAsm_V3("vavgsw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGUB(ppu_opcode_t op)
{
	DisAsm_V3("vavgub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGUH(ppu_opcode_t op)
{
	DisAsm_V3("vavguh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VAVGUW(ppu_opcode_t op)
{
	DisAsm_V3("vavguw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCFSX(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vcfsx", op.vd, op.vb, op.vuimm);
}

void PPUDisAsm::VCFUX(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vcfux", op.vd, op.vb, op.vuimm);
}

void PPUDisAsm::VCMPBFP(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpbfp." : "vcmpbfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPEQFP(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpeqfp." : "vcmpeqfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPEQUB(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpequb." : "vcmpequb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPEQUH(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpequh." : "vcmpequh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPEQUW(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpequw." : "vcmpequw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGEFP(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgefp." : "vcmpgefp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTFP(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtfp." : "vcmpgtfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTSB(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtsb." : "vcmpgtsb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTSH(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtsh." : "vcmpgtsh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTSW(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtsw." : "vcmpgtsw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTUB(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtub." : "vcmpgtub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTUH(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtuh." : "vcmpgtuh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCMPGTUW(ppu_opcode_t op)
{
	DisAsm_V3(op.oe ? "vcmpgtuw." : "vcmpgtuw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VCTSXS(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vctsxs", op.vd, op.vb, op.vuimm);
}

void PPUDisAsm::VCTUXS(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vctuxs", op.vd, op.vb, op.vuimm);
}

void PPUDisAsm::VEXPTEFP(ppu_opcode_t op)
{
	DisAsm_V2("vexptefp", op.vd, op.vb);
}

void PPUDisAsm::VLOGEFP(ppu_opcode_t op)
{
	DisAsm_V2("vlogefp", op.vd, op.vb);
}

void PPUDisAsm::VMADDFP(ppu_opcode_t op)
{
	DisAsm_V4("vmaddfp", op.vd, op.va, op.vc, op.vb);
}

void PPUDisAsm::VMAXFP(ppu_opcode_t op)
{
	DisAsm_V3("vmaxfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXSB(ppu_opcode_t op)
{
	DisAsm_V3("vmaxsb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXSH(ppu_opcode_t op)
{
	DisAsm_V3("vmaxsh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXSW(ppu_opcode_t op)
{
	DisAsm_V3("vmaxsw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXUB(ppu_opcode_t op)
{
	DisAsm_V3("vmaxub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXUH(ppu_opcode_t op)
{
	DisAsm_V3("vmaxuh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMAXUW(ppu_opcode_t op)
{
	DisAsm_V3("vmaxuw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMHADDSHS(ppu_opcode_t op)
{
	DisAsm_V4("vmhaddshs", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMHRADDSHS(ppu_opcode_t op)
{
	DisAsm_V4("vmhraddshs", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMINFP(ppu_opcode_t op)
{
	DisAsm_V3("vminfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINSB(ppu_opcode_t op)
{
	DisAsm_V3("vminsb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINSH(ppu_opcode_t op)
{
	DisAsm_V3("vminsh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINSW(ppu_opcode_t op)
{
	DisAsm_V3("vminsw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINUB(ppu_opcode_t op)
{
	DisAsm_V3("vminub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINUH(ppu_opcode_t op)
{
	DisAsm_V3("vminuh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMINUW(ppu_opcode_t op)
{
	DisAsm_V3("vminuw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMLADDUHM(ppu_opcode_t op)
{
	DisAsm_V4("vmladduhm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMRGHB(ppu_opcode_t op)
{
	DisAsm_V3("vmrghb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMRGHH(ppu_opcode_t op)
{
	DisAsm_V3("vmrghh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMRGHW(ppu_opcode_t op)
{
	DisAsm_V3("vmrghw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMRGLB(ppu_opcode_t op)
{
	DisAsm_V3("vmrglb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMRGLH(ppu_opcode_t op)
{
	DisAsm_V3("vmrglh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMRGLW(ppu_opcode_t op)
{
	DisAsm_V3("vmrglw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMSUMMBM(ppu_opcode_t op)
{
	DisAsm_V4("vmsummbm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMSUMSHM(ppu_opcode_t op)
{
	DisAsm_V4("vmsumshm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMSUMSHS(ppu_opcode_t op)
{
	DisAsm_V4("vmsumshs", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMSUMUBM(ppu_opcode_t op)
{
	DisAsm_V4("vmsumubm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMSUMUHM(ppu_opcode_t op)
{
	DisAsm_V4("vmsumuhm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMSUMUHS(ppu_opcode_t op)
{
	DisAsm_V4("vmsumuhs", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VMULESB(ppu_opcode_t op)
{
	DisAsm_V3("vmulesb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULESH(ppu_opcode_t op)
{
	DisAsm_V3("vmulesh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULEUB(ppu_opcode_t op)
{
	DisAsm_V3("vmuleub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULEUH(ppu_opcode_t op)
{
	DisAsm_V3("vmuleuh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULOSB(ppu_opcode_t op)
{
	DisAsm_V3("vmulosb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULOSH(ppu_opcode_t op)
{
	DisAsm_V3("vmulosh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULOUB(ppu_opcode_t op)
{
	DisAsm_V3("vmuloub", op.vd, op.va, op.vb);
}

void PPUDisAsm::VMULOUH(ppu_opcode_t op)
{
	DisAsm_V3("vmulouh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VNMSUBFP(ppu_opcode_t op)
{
	DisAsm_V4("vnmsubfp", op.vd, op.va, op.vc, op.vb);
}

void PPUDisAsm::VNOR(ppu_opcode_t op)
{
	DisAsm_V3("vnor", op.vd, op.va, op.vb);
}

void PPUDisAsm::VOR(ppu_opcode_t op)
{
	DisAsm_V3("vor", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPERM(ppu_opcode_t op)
{
	DisAsm_V4("vperm", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VPKPX(ppu_opcode_t op)
{
	DisAsm_V3("vpkpx", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKSHSS(ppu_opcode_t op)
{
	DisAsm_V3("vpkshss", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKSHUS(ppu_opcode_t op)
{
	DisAsm_V3("vpkshus", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKSWSS(ppu_opcode_t op)
{
	DisAsm_V3("vpkswss", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKSWUS(ppu_opcode_t op)
{
	DisAsm_V3("vpkswus", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKUHUM(ppu_opcode_t op)
{
	DisAsm_V3("vpkuhum", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKUHUS(ppu_opcode_t op)
{
	DisAsm_V3("vpkuhus", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKUWUM(ppu_opcode_t op)
{
	DisAsm_V3("vpkuwum", op.vd, op.va, op.vb);
}

void PPUDisAsm::VPKUWUS(ppu_opcode_t op)
{
	DisAsm_V3("vpkuwus", op.vd, op.va, op.vb);
}

void PPUDisAsm::VREFP(ppu_opcode_t op)
{
	DisAsm_V2("vrefp", op.vd, op.vb);
}

void PPUDisAsm::VRFIM(ppu_opcode_t op)
{
	DisAsm_V2("vrfim", op.vd, op.vb);
}

void PPUDisAsm::VRFIN(ppu_opcode_t op)
{
	DisAsm_V2("vrfin", op.vd, op.vb);
}

void PPUDisAsm::VRFIP(ppu_opcode_t op)
{
	DisAsm_V2("vrfip", op.vd, op.vb);
}

void PPUDisAsm::VRFIZ(ppu_opcode_t op)
{
	DisAsm_V2("vrfiz", op.vd, op.vb);
}

void PPUDisAsm::VRLB(ppu_opcode_t op)
{
	DisAsm_V3("vrlb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VRLH(ppu_opcode_t op)
{
	DisAsm_V3("vrlh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VRLW(ppu_opcode_t op)
{
	DisAsm_V3("vrlw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VRSQRTEFP(ppu_opcode_t op)
{
	DisAsm_V2("vrsqrtefp", op.vd, op.vb);
}

void PPUDisAsm::VSEL(ppu_opcode_t op)
{
	DisAsm_V4("vsel", op.vd, op.va, op.vb, op.vc);
}

void PPUDisAsm::VSL(ppu_opcode_t op)
{
	DisAsm_V3("vsl", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSLB(ppu_opcode_t op)
{
	DisAsm_V3("vslb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSLDOI(ppu_opcode_t op)
{
	DisAsm_V3_UIMM("vsldoi", op.vd, op.va, op.vb, op.vsh);
}

void PPUDisAsm::VSLH(ppu_opcode_t op)
{
	DisAsm_V3("vslh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSLO(ppu_opcode_t op)
{
	DisAsm_V3("vslo", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSLW(ppu_opcode_t op)
{
	DisAsm_V3("vslw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSPLTB(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vspltb", op.vd, op.vb, op.vuimm & 0xf);
}

void PPUDisAsm::VSPLTH(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vsplth", op.vd, op.vb, op.vuimm & 0x7);
}

void PPUDisAsm::VSPLTISB(ppu_opcode_t op)
{
	DisAsm_V1_SIMM("vspltisb", op.vd, op.vsimm);
}

void PPUDisAsm::VSPLTISH(ppu_opcode_t op)
{
	DisAsm_V1_SIMM("vspltish", op.vd, op.vsimm);
}

void PPUDisAsm::VSPLTISW(ppu_opcode_t op)
{
	DisAsm_V1_SIMM("vspltisw", op.vd, op.vsimm);
}

void PPUDisAsm::VSPLTW(ppu_opcode_t op)
{
	DisAsm_V2_UIMM("vspltw", op.vd, op.vb, op.vuimm & 0x3);
}

void PPUDisAsm::VSR(ppu_opcode_t op)
{
	DisAsm_V3("vsr", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRAB(ppu_opcode_t op)
{
	DisAsm_V3("vsrab", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRAH(ppu_opcode_t op)
{
	DisAsm_V3("vsrah", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRAW(ppu_opcode_t op)
{
	DisAsm_V3("vsraw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRB(ppu_opcode_t op)
{
	DisAsm_V3("vsrb", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRH(ppu_opcode_t op)
{
	DisAsm_V3("vsrh", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRO(ppu_opcode_t op)
{
	DisAsm_V3("vsro", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSRW(ppu_opcode_t op)
{
	DisAsm_V3("vsrw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBCUW(ppu_opcode_t op)
{
	DisAsm_V3("vsubcuw", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBFP(ppu_opcode_t op)
{
	DisAsm_V3("vsubfp", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBSBS(ppu_opcode_t op)
{
	DisAsm_V3("vsubsbs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBSHS(ppu_opcode_t op)
{
	DisAsm_V3("vsubshs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBSWS(ppu_opcode_t op)
{
	DisAsm_V3("vsubsws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUBM(ppu_opcode_t op)
{
	DisAsm_V3("vsububm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUBS(ppu_opcode_t op)
{
	DisAsm_V3("vsububs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUHM(ppu_opcode_t op)
{
	DisAsm_V3("vsubuhm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUHS(ppu_opcode_t op)
{
	DisAsm_V3("vsubuhs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUWM(ppu_opcode_t op)
{
	DisAsm_V3("vsubuwm", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUBUWS(ppu_opcode_t op)
{
	DisAsm_V3("vsubuws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUMSWS(ppu_opcode_t op)
{
	DisAsm_V3("vsumsws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUM2SWS(ppu_opcode_t op)
{
	DisAsm_V3("vsum2sws", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUM4SBS(ppu_opcode_t op)
{
	DisAsm_V3("vsum4sbs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUM4SHS(ppu_opcode_t op)
{
	DisAsm_V3("vsum4shs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VSUM4UBS(ppu_opcode_t op)
{
	DisAsm_V3("vsum4ubs", op.vd, op.va, op.vb);
}

void PPUDisAsm::VUPKHPX(ppu_opcode_t op)
{
	DisAsm_V2("vupkhpx", op.vd, op.vb);
}

void PPUDisAsm::VUPKHSB(ppu_opcode_t op)
{
	DisAsm_V2("vupkhsb", op.vd, op.vb);
}

void PPUDisAsm::VUPKHSH(ppu_opcode_t op)
{
	DisAsm_V2("vupkhsh", op.vd, op.vb);
}

void PPUDisAsm::VUPKLPX(ppu_opcode_t op)
{
	DisAsm_V2("vupklpx", op.vd, op.vb);
}

void PPUDisAsm::VUPKLSB(ppu_opcode_t op)
{
	DisAsm_V2("vupklsb", op.vd, op.vb);
}

void PPUDisAsm::VUPKLSH(ppu_opcode_t op)
{
	DisAsm_V2("vupklsh", op.vd, op.vb);
}

void PPUDisAsm::VXOR(ppu_opcode_t op)
{
	DisAsm_V3("vxor", op.vd, op.va, op.vb);
}

void PPUDisAsm::TDI(ppu_opcode_t op)
{
	DisAsm_INT1_R1_IMM("tdi", op.bo, op.ra, op.simm16);
}

void PPUDisAsm::TWI(ppu_opcode_t op)
{
	DisAsm_INT1_R1_IMM("twi", op.bo, op.ra, op.simm16);
}

void PPUDisAsm::MULLI(ppu_opcode_t op)
{
	DisAsm_R2_IMM("mulli", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::SUBFIC(ppu_opcode_t op)
{
	DisAsm_R2_IMM("subfic", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::CMPLI(ppu_opcode_t op)
{
	DisAsm_CR1_R1_IMM(op.l10 ? "cmpdi" : "cmpwi", op.crfd, op.ra, op.uimm16);
}

void PPUDisAsm::CMPI(ppu_opcode_t op)
{
	DisAsm_CR1_R1_IMM(op.l10 ? "cmpdi" : "cmpwi", op.crfd, op.ra, op.simm16);
}

void PPUDisAsm::ADDIC(ppu_opcode_t op)
{
	DisAsm_R2_IMM(op.main & 1 ? "addic." : "addic", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::ADDI(ppu_opcode_t op)
{
	if (op.ra == 0)
	{
		DisAsm_R1_IMM("li", op.rd, op.simm16);
	}
	else
	{
		DisAsm_R2_IMM("addi", op.rd, op.ra, op.simm16);
	}
}

void PPUDisAsm::ADDIS(ppu_opcode_t op)
{
	if (op.ra == 0)
	{
		DisAsm_R1_IMM("lis", op.rd, op.simm16);
	}
	else
	{
		DisAsm_R2_IMM("addis", op.rd, op.ra, op.simm16);
	}
}

void PPUDisAsm::BC(ppu_opcode_t op)
{
	const u32 bo = op.bo;
	const u32 bi = op.bi;
	const s32 bd = op.ds * 4;
	const u32 aa = op.aa;
	const u32 lk = op.lk;

	if (m_mode == CPUDisAsm_CompilerElfMode)
	{
		Write(fmt::format("bc 0x%x, 0x%x, 0x%x, %d, %d", bo, bi, bd, aa, lk));
		return;
	}

	//TODO: aa lk
	const u8 bo0 = (bo & 0x10) ? 1 : 0;
	const u8 bo1 = (bo & 0x08) ? 1 : 0;
	const u8 bo2 = (bo & 0x04) ? 1 : 0;
	const u8 bo3 = (bo & 0x02) ? 1 : 0;
	const u8 bo4 = (bo & 0x01) ? 1 : 0;

	if (bo0 && !bo1 && !bo2 && bo3 && !bo4)
	{
		DisAsm_CR_BRANCH("bdz", bi / 4, bd); return;
	}
	else if (bo0 && bo1 && !bo2 && bo3 && !bo4)
	{
		DisAsm_CR_BRANCH("bdz-", bi / 4, bd); return;
	}
	else if (bo0 && bo1 && !bo2 && bo3 && bo4)
	{
		DisAsm_CR_BRANCH("bdz+", bi / 4, bd); return;
	}
	else if (bo0 && !bo1 && !bo2 && !bo3 && !bo4)
	{
		DisAsm_CR_BRANCH("bdnz", bi / 4, bd); return;
	}
	else if (bo0 && bo1 && !bo2 && !bo3 && !bo4)
	{
		DisAsm_CR_BRANCH("bdnz-", bi / 4, bd); return;
	}
	else if (bo0 && bo1 && !bo2 && !bo3 && bo4)
	{
		DisAsm_CR_BRANCH("bdnz+", bi / 4, bd); return;
	}
	else if (!bo0 && !bo1 && bo2 && !bo3 && !bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("bge", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("ble", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("bne", bi / 4, bd); return;
		}
	}
	else if (!bo0 && !bo1 && bo2 && bo3 && !bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("bge-", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("ble-", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("bne-", bi / 4, bd); return;
		}
	}
	else if (!bo0 && !bo1 && bo2 && bo3 && bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("bge+", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("ble+", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("bne+", bi / 4, bd); return;
		}
	}
	else if (!bo0 && bo1 && bo2 && !bo3 && !bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("blt", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("bgt", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("beq", bi / 4, bd); return;
		}
	}
	else if (!bo0 && bo1 && bo2 && bo3 && !bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("blt-", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("bgt-", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("beq-", bi / 4, bd); return;
		}
	}
	else if (!bo0 && bo1 && bo2 && bo3 && bo4)
	{
		switch (bi % 4)
		{
		case 0x0: DisAsm_CR_BRANCH("blt+", bi / 4, bd); return;
		case 0x1: DisAsm_CR_BRANCH("bgt+", bi / 4, bd); return;
		case 0x2: DisAsm_CR_BRANCH("beq+", bi / 4, bd); return;
		}
	}

	Write(fmt::format("bc [%x:%x:%x:%x:%x], cr%d[%x], 0x%x, %d, %d", bo0, bo1, bo2, bo3, bo4, bi / 4, bi % 4, bd, aa, lk));
}

void PPUDisAsm::SC(ppu_opcode_t op)
{
	if (op.opcode != ppu_instructions::SC(0))
	{
		return UNK(op);
	}

	Write("sc");
}

void PPUDisAsm::B(ppu_opcode_t op)
{
	const u32 ll = op.ll;
	const u32 aa = op.aa;
	const u32 lk = op.lk;

	if (m_mode == CPUDisAsm_CompilerElfMode)
	{
		Write(fmt::format("b 0x%x, %d, %d", ll, aa, lk));
		return;
	}

	switch (lk)
	{
	case 0:
		switch (aa)
		{
		case 0:	DisAsm_BRANCH("b", ll);		break;
		case 1:	DisAsm_BRANCH_A("ba", ll);	break;
		}
		break;

	case 1:
		switch (aa)
		{
		case 0: DisAsm_BRANCH("bl", ll);	break;
		case 1: DisAsm_BRANCH_A("bla", ll);	break;
		}
		break;
	}
}

void PPUDisAsm::MCRF(ppu_opcode_t op)
{
	DisAsm_CR2("mcrf", op.crfd, op.crfs);
}

void PPUDisAsm::BCLR(ppu_opcode_t op)
{
	const u32 bo = op.bo;
	const u32 bi = op.bi;

	const u8 bo0 = (bo & 0x10) ? 1 : 0;
	const u8 bo1 = (bo & 0x08) ? 1 : 0;
	const u8 bo2 = (bo & 0x04) ? 1 : 0;
	const u8 bo3 = (bo & 0x02) ? 1 : 0;

	if (bo0 && !bo1 && bo2 && !bo3) { Write("blr"); return; }
	Write(fmt::format("bclr [%x:%x:%x:%x], cr%d[%x], %d, %d", bo0, bo1, bo2, bo3, bi / 4, bi % 4, op.bh, op.lk));
}

void PPUDisAsm::CRNOR(ppu_opcode_t op)
{
	DisAsm_INT3("crnor", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CRANDC(ppu_opcode_t op)
{
	DisAsm_INT3("crandc", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::ISYNC(ppu_opcode_t op)
{
	Write("isync");
}

void PPUDisAsm::CRXOR(ppu_opcode_t op)
{
	DisAsm_INT3("crxor", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CRNAND(ppu_opcode_t op)
{
	DisAsm_INT3("crnand", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CRAND(ppu_opcode_t op)
{
	DisAsm_INT3("crand", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CREQV(ppu_opcode_t op)
{
	DisAsm_INT3("creqv", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CRORC(ppu_opcode_t op)
{
	DisAsm_INT3("crorc", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::CROR(ppu_opcode_t op)
{
	DisAsm_INT3("cror", op.crbd, op.crba, op.crbb);
}

void PPUDisAsm::BCCTR(ppu_opcode_t op)
{
	const u32 bo = op.bo;
	const u32 bi = op.bi;
	const u32 bh = op.bh;

	switch (op.lk)
	{
	case 0: DisAsm_INT3("bcctr", bo, bi, bh); break;
	case 1: DisAsm_INT3("bcctrl", bo, bi, bh); break;
	}
}

void PPUDisAsm::RLWIMI(ppu_opcode_t op)
{
	DisAsm_R2_INT3_RC("rlwimi", op.ra, op.rs, op.sh32, op.mb32, op.me32, op.rc);
}

void PPUDisAsm::RLWINM(ppu_opcode_t op)
{
	DisAsm_R2_INT3_RC("rlwinm", op.ra, op.rs, op.sh32, op.mb32, op.me32, op.rc);
}

void PPUDisAsm::RLWNM(ppu_opcode_t op)
{
	DisAsm_R3_INT2_RC("rlwnm", op.ra, op.rs, op.rb, op.mb32, op.me32, op.rc);
}

void PPUDisAsm::ORI(ppu_opcode_t op)
{
	if (op.rs == 0 && op.ra == 0 && op.uimm16 == 0) return Write("nop");
	DisAsm_R2_IMM("ori", op.rs, op.ra, op.uimm16);
}

void PPUDisAsm::ORIS(ppu_opcode_t op)
{
	if (op.rs == 0 && op.ra == 0 && op.uimm16 == 0) return Write("nop");
	DisAsm_R2_IMM("oris", op.rs, op.ra, op.uimm16);
}

void PPUDisAsm::XORI(ppu_opcode_t op)
{
	DisAsm_R2_IMM("xori", op.ra, op.rs, op.uimm16);
}

void PPUDisAsm::XORIS(ppu_opcode_t op)
{
	DisAsm_R2_IMM("xoris", op.ra, op.rs, op.uimm16);
}

void PPUDisAsm::ANDI(ppu_opcode_t op)
{
	DisAsm_R2_IMM("andi.", op.ra, op.rs, op.uimm16);
}

void PPUDisAsm::ANDIS(ppu_opcode_t op)
{
	DisAsm_R2_IMM("andis.", op.ra, op.rs, op.uimm16);
}

void PPUDisAsm::RLDICL(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;

	if (sh == 0)
	{
		DisAsm_R2_INT1_RC("clrldi", op.ra, op.rs, mb, op.rc);
	}
	else if (mb == 0)
	{
		DisAsm_R2_INT1_RC("rotldi", op.ra, op.rs, sh, op.rc);
	}
	else if (mb == 64 - sh)
	{
		DisAsm_R2_INT1_RC("srdi", op.ra, op.rs, mb, op.rc);
	}
	else
	{
		DisAsm_R2_INT2_RC("rldicl", op.ra, op.rs, sh, mb, op.rc);
	}
}

void PPUDisAsm::RLDICR(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 me = op.mbe64;

	DisAsm_R2_INT2_RC("rldicr", op.ra, op.rs, sh, me, op.rc);
}

void PPUDisAsm::RLDIC(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;

	DisAsm_R2_INT2_RC("rldic", op.ra, op.rs, sh, mb, op.rc);
}

void PPUDisAsm::RLDIMI(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;

	DisAsm_R2_INT2_RC("rldimi", op.ra, op.rs, sh, mb, op.rc);
}

void PPUDisAsm::RLDCL(ppu_opcode_t op)
{
	const u32 mb = op.mbe64;

	DisAsm_R3_INT2_RC("rldcl", op.ra, op.rs, op.rb, mb, 0, op.rc);
}

void PPUDisAsm::RLDCR(ppu_opcode_t op)
{
	const u32 me = op.mbe64;

	DisAsm_R3_INT2_RC("rldcr", op.ra, op.rs, op.rb, me, 0, op.rc);
}

void PPUDisAsm::CMP(ppu_opcode_t op)
{
	DisAsm_CR1_R2(op.l10 ? "cmpd" : "cmpw", op.crfd, op.ra, op.rb);
}

void PPUDisAsm::TW(ppu_opcode_t op)
{
	DisAsm_INT1_R2("tw", op.bo, op.ra, op.rb);
}

void PPUDisAsm::LVSL(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvsl", op.vd, op.ra, op.rb);
}

void PPUDisAsm::LVEBX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvebx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::SUBFC(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("subfc", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::ADDC(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("addc", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::MULHDU(ppu_opcode_t op)
{
	DisAsm_R3_RC("mulhdu", op.rd, op.ra, op.rb, op.rc);
}

void PPUDisAsm::MULHWU(ppu_opcode_t op)
{
	DisAsm_R3_RC("mulhwu", op.rd, op.ra, op.rb, op.rc);
}

void PPUDisAsm::MFOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		DisAsm_R1_IMM("mfocrf", op.rd, op.crm);
	}
	else
	{
		DisAsm_R1("mfcr", op.rd);
	}
}

void PPUDisAsm::LWARX(ppu_opcode_t op)
{
	DisAsm_R3("lwarx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LDX(ppu_opcode_t op)
{
	DisAsm_R3("ldx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LWZX(ppu_opcode_t op)
{
	DisAsm_R3("lwzx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::SLW(ppu_opcode_t op)
{
	DisAsm_R3_RC("slw", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::CNTLZW(ppu_opcode_t op)
{
	DisAsm_R2_RC("cntlzw", op.ra, op.rs, op.rc);
}

void PPUDisAsm::SLD(ppu_opcode_t op)
{
	DisAsm_R3_RC("sld", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::AND(ppu_opcode_t op)
{
	DisAsm_R3_RC("and", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::CMPL(ppu_opcode_t op)
{
	DisAsm_CR1_R2(op.l10 ? "cmpld" : "cmplw", op.crfd, op.ra, op.rb);
}

void PPUDisAsm::LVSR(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvsr", op.vd, op.ra, op.rb);
}

void PPUDisAsm::LVEHX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvehx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::SUBF(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("subf", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::LDUX(ppu_opcode_t op)
{
	DisAsm_R3("ldux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::DCBST(ppu_opcode_t op)
{
	DisAsm_R2("dcbst", op.ra, op.rb);
}

void PPUDisAsm::LWZUX(ppu_opcode_t op)
{
	DisAsm_R3("lwzux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::CNTLZD(ppu_opcode_t op)
{
	DisAsm_R2_RC("cntlzd", op.ra, op.rs, op.rc);
}

void PPUDisAsm::ANDC(ppu_opcode_t op)
{
	DisAsm_R3_RC("andc", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::TD(ppu_opcode_t op)
{
	DisAsm_INT1_R2("td", op.bo, op.ra, op.rb);
}

void PPUDisAsm::LVEWX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvewx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::MULHD(ppu_opcode_t op)
{
	DisAsm_R3_RC("mulhd", op.rd, op.ra, op.rb, op.rc);
}

void PPUDisAsm::MULHW(ppu_opcode_t op)
{
	DisAsm_R3_RC("mulhw", op.rd, op.ra, op.rb, op.rc);
}

void PPUDisAsm::LDARX(ppu_opcode_t op)
{
	DisAsm_R3("ldarx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::DCBF(ppu_opcode_t op)
{
	DisAsm_R2("dcbf", op.ra, op.rb);
}

void PPUDisAsm::LBZX(ppu_opcode_t op)
{
	DisAsm_R3("lbzx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LVX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::NEG(ppu_opcode_t op)
{
	DisAsm_R2_OE_RC("neg", op.rd, op.ra, op.oe, op.rc);
}

void PPUDisAsm::LBZUX(ppu_opcode_t op)
{
	DisAsm_R3("lbzux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::NOR(ppu_opcode_t op)
{
	if (op.rs == op.rb)
	{
		DisAsm_R2_RC("not", op.ra, op.rs, op.rc);
	}
	else
	{
		DisAsm_R3_RC("nor", op.ra, op.rs, op.rb, op.rc);
	}
}

void PPUDisAsm::STVEBX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvebx", op.vs, op.ra, op.rb);
}

void PPUDisAsm::SUBFE(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("subfe", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::ADDE(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("adde", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::MTOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		DisAsm_INT1_R1("mtocrf", op.crm, op.rs);
	}
	else
	{
		DisAsm_INT1_R1("mtcrf", op.crm, op.rs);
	}
}

void PPUDisAsm::STDX(ppu_opcode_t op)
{
	DisAsm_R3("stdx.", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STWCX(ppu_opcode_t op)
{
	DisAsm_R3("stwcx.", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STWX(ppu_opcode_t op)
{
	DisAsm_R3("stwx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STVEHX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvehx", op.vs, op.ra, op.rb);
}

void PPUDisAsm::STDUX(ppu_opcode_t op)
{
	DisAsm_R3("stdux", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STWUX(ppu_opcode_t op)
{
	DisAsm_R3("stwux", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STVEWX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvewx", op.vs, op.ra, op.rb);
}

void PPUDisAsm::SUBFZE(ppu_opcode_t op)
{
	DisAsm_R2_OE_RC("subfze", op.rd, op.ra, op.oe, op.rc);
}

void PPUDisAsm::ADDZE(ppu_opcode_t op)
{
	DisAsm_R2_OE_RC("addze", op.rd, op.ra, op.oe, op.rc);
}

void PPUDisAsm::STDCX(ppu_opcode_t op)
{
	DisAsm_R3("stdcx.", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STBX(ppu_opcode_t op)
{
	DisAsm_R3("stbx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STVX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::SUBFME(ppu_opcode_t op)
{
	DisAsm_R2_OE_RC("subfme", op.rd, op.ra, op.oe, op.rc);
}

void PPUDisAsm::MULLD(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("mulld", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::ADDME(ppu_opcode_t op)
{
	DisAsm_R2_OE_RC("addme", op.rd, op.ra, op.oe, op.rc);
}

void PPUDisAsm::MULLW(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("mullw", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::DCBTST(ppu_opcode_t op)
{
	DisAsm_R3("dcbtst", op.ra, op.rb, op.bo);
}

void PPUDisAsm::STBUX(ppu_opcode_t op)
{
	DisAsm_R3("stbux", op.rs, op.ra, op.rb);
}

void PPUDisAsm::ADD(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("add", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::DCBT(ppu_opcode_t op)
{
	DisAsm_R2("dcbt", op.ra, op.rb);
}

void PPUDisAsm::LHZX(ppu_opcode_t op)
{
	DisAsm_R3("lhzx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::EQV(ppu_opcode_t op)
{
	DisAsm_R3_RC("eqv", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::ECIWX(ppu_opcode_t op)
{
	DisAsm_R3("eciwx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LHZUX(ppu_opcode_t op)
{
	DisAsm_R3("lhzux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::XOR(ppu_opcode_t op)
{
	DisAsm_R3_RC("xor", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::MFSPR(ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);
	switch (n)
	{
	case 0x001: DisAsm_R1("mfxer", op.rd); break;
	case 0x008: DisAsm_R1("mflr", op.rd); break;
	case 0x009: DisAsm_R1("mfctr", op.rd); break;
	default: DisAsm_R1_IMM("mfspr", op.rd, op.spr); break;
	}
}

void PPUDisAsm::LWAX(ppu_opcode_t op)
{
	DisAsm_R3("lwax", op.rd, op.ra, op.rb);
}

void PPUDisAsm::DST(ppu_opcode_t op)
{
	DisAsm_R2("dst(t)", op.ra, op.rb);
}

void PPUDisAsm::LHAX(ppu_opcode_t op)
{
	DisAsm_R3("lhax", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LVXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvxl", op.vd, op.ra, op.rb);
}

void PPUDisAsm::MFTB(ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);
	switch (n)
	{
	case 268: DisAsm_R1("mftb", op.rd); break;
	case 269: DisAsm_R1("mftbu", op.rd); break;
	default: DisAsm_R1_IMM("mftb", op.rd, op.spr); break;
	}
}

void PPUDisAsm::LWAUX(ppu_opcode_t op)
{
	DisAsm_R3("lwaux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::DSTST(ppu_opcode_t op)
{
	DisAsm_R2("dstst(t)", op.ra, op.rb);
}

void PPUDisAsm::LHAUX(ppu_opcode_t op)
{
	DisAsm_R3("lhaux", op.rd, op.ra, op.rb);
}

void PPUDisAsm::STHX(ppu_opcode_t op)
{
	DisAsm_R3("sthx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::ORC(ppu_opcode_t op)
{
	DisAsm_R3_RC("orc", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::ECOWX(ppu_opcode_t op)
{
	DisAsm_R3("ecowx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STHUX(ppu_opcode_t op)
{
	DisAsm_R3("sthux", op.rs, op.ra, op.rb);
}

void PPUDisAsm::OR(ppu_opcode_t op)
{
	if (op.rs == op.rb)
	{
		switch (op.opcode)
		{	
		case 0x7f9ce378: return Write("db8cyc");
		case 0x7fbdeb78: return Write("db10cyc");
		case 0x7fdef378: return Write("db12cyc");
		case 0x7ffffb78: return Write("db16cyc");
		default : DisAsm_R2_RC("mr", op.ra, op.rb, op.rc);
		}
	}
	else
	{
		DisAsm_R3_RC("or", op.ra, op.rs, op.rb, op.rc);
	}
}

void PPUDisAsm::DIVDU(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("divdu", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::DIVWU(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("divwu", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::MTSPR(ppu_opcode_t op)
{
	const u32 n = (op.spr & 0x1f) + ((op.spr >> 5) & 0x1f);

	switch (n)
	{
	case 0x001: DisAsm_R1("mtxer", op.rs); break;
	case 0x008: DisAsm_R1("mtlr", op.rs); break;
	case 0x009: DisAsm_R1("mtctr", op.rs); break;
	default: DisAsm_IMM_R1("mtspr", op.spr, op.rs); break;
	}
}

void PPUDisAsm::DCBI(ppu_opcode_t op)
{
	DisAsm_R2("dcbi", op.ra, op.rb);
}

void PPUDisAsm::NAND(ppu_opcode_t op)
{
	DisAsm_R3_RC("nand", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::STVXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvxl", op.vs, op.ra, op.rb);
}

void PPUDisAsm::DIVD(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("divd", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::DIVW(ppu_opcode_t op)
{
	DisAsm_R3_OE_RC("divw", op.rd, op.ra, op.rb, op.oe, op.rc);
}

void PPUDisAsm::LVLX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvlx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::LDBRX(ppu_opcode_t op)
{
	DisAsm_R3("ldbrx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LSWX(ppu_opcode_t op)
{
	DisAsm_R3("lswx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LWBRX(ppu_opcode_t op)
{
	DisAsm_R3("lwbrx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LFSX(ppu_opcode_t op)
{
	DisAsm_F1_R2("lfsx", op.frd, op.ra, op.rb);
}

void PPUDisAsm::SRW(ppu_opcode_t op)
{
	DisAsm_R3_RC("srw", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::SRD(ppu_opcode_t op)
{
	DisAsm_R3_RC("srd", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::LVRX(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvrx", op.vd, op.ra, op.rb);
}

void PPUDisAsm::LSWI(ppu_opcode_t op)
{
	DisAsm_R2_INT1("lswi", op.rd, op.ra, op.rb);
}

void PPUDisAsm::LFSUX(ppu_opcode_t op)
{
	DisAsm_F1_R2("lfsux", op.frd, op.ra, op.rb);
}

void PPUDisAsm::SYNC(ppu_opcode_t op)
{
	Write("sync");
}

void PPUDisAsm::LFDX(ppu_opcode_t op)
{
	DisAsm_F1_R2("lfdx", op.frd, op.ra, op.rb);
}

void PPUDisAsm::LFDUX(ppu_opcode_t op)
{
	DisAsm_F1_R2("lfdux", op.frd, op.ra, op.rb);
}

void PPUDisAsm::STVLX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvlx", op.vs, op.ra, op.rb);
}

void PPUDisAsm::STDBRX(ppu_opcode_t op)
{
	DisAsm_R3("stdbrx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STSWX(ppu_opcode_t op)
{
	DisAsm_R3("swswx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STWBRX(ppu_opcode_t op)
{
	DisAsm_R3("stwbrx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::STFSX(ppu_opcode_t op)
{
	DisAsm_F1_R2("stfsx", op.frs, op.ra, op.rb);
}

void PPUDisAsm::STVRX(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvrx", op.vs, op.ra, op.rb);
}

void PPUDisAsm::STFSUX(ppu_opcode_t op)
{
	DisAsm_F1_R2("stfsux", op.frs, op.ra, op.rb);
}

void PPUDisAsm::STSWI(ppu_opcode_t op)
{
	DisAsm_R2_INT1("stswi", op.rd, op.ra, op.rb);
}

void PPUDisAsm::STFDX(ppu_opcode_t op)
{
	DisAsm_F1_R2("stfdx", op.frs, op.ra, op.rb);
}

void PPUDisAsm::STFDUX(ppu_opcode_t op)
{
	DisAsm_F1_R2("stfdux", op.frs, op.ra, op.rb);
}

void PPUDisAsm::LVLXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvlxl", op.vd, op.ra, op.rb);
}

void PPUDisAsm::LHBRX(ppu_opcode_t op)
{
	DisAsm_R3("lhbrx", op.rd, op.ra, op.rb);
}

void PPUDisAsm::SRAW(ppu_opcode_t op)
{
	DisAsm_R3_RC("sraw", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::SRAD(ppu_opcode_t op)
{
	DisAsm_R3_RC("srad", op.ra, op.rs, op.rb, op.rc);
}

void PPUDisAsm::LVRXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("lvrxl", op.vd, op.ra, op.rb);
}

void PPUDisAsm::DSS(ppu_opcode_t op)
{
	Write("dss()");
}

void PPUDisAsm::SRAWI(ppu_opcode_t op)
{
	DisAsm_R2_INT1_RC("srawi", op.ra, op.rs, op.sh32, op.rc);
}

void PPUDisAsm::SRADI(ppu_opcode_t op)
{
	DisAsm_R2_INT1_RC("sradi", op.ra, op.rs, op.sh64, op.rc);
}

void PPUDisAsm::EIEIO(ppu_opcode_t op)
{
	Write("eieio");
}

void PPUDisAsm::STVLXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvlxl", op.vs, op.ra, op.rb);
}

void PPUDisAsm::STHBRX(ppu_opcode_t op)
{
	DisAsm_R3("sthbrx", op.rs, op.ra, op.rb);
}

void PPUDisAsm::EXTSH(ppu_opcode_t op)
{
	DisAsm_R2_RC("extsh", op.ra, op.rs, op.rc);
}

void PPUDisAsm::STVRXL(ppu_opcode_t op)
{
	DisAsm_V1_R2("stvrxl", op.vs, op.ra, op.rb);
}

void PPUDisAsm::EXTSB(ppu_opcode_t op)
{
	DisAsm_R2_RC("extsb", op.ra, op.rs, op.rc);
}

void PPUDisAsm::STFIWX(ppu_opcode_t op)
{
	DisAsm_F1_R2("stfiwx", op.frs, op.ra, op.rb);
}

void PPUDisAsm::EXTSW(ppu_opcode_t op)
{
	DisAsm_R2_RC("extsw", op.ra, op.rs, op.rc);
}

void PPUDisAsm::ICBI(ppu_opcode_t op)
{
	DisAsm_R2("icbi", op.ra, op.rb);
}

void PPUDisAsm::DCBZ(ppu_opcode_t op)
{
	DisAsm_R2("dcbz", op.ra, op.rb);
}

void PPUDisAsm::LWZ(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lwz", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::LWZU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lwzu", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::LBZ(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lbz", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::LBZU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lbzu", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::STW(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stw", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::STWU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stwu", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::STB(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stb", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::STBU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stbu", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LHZ(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lhz", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LHZU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lhzu", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LHA(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lha", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LHAU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lhau", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::STH(ppu_opcode_t op)
{
	DisAsm_R2_IMM("sth", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::STHU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("sthu", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LMW(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lmw", op.rd, op.ra, op.simm16);
}

void PPUDisAsm::STMW(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stmw", op.rs, op.ra, op.simm16);
}

void PPUDisAsm::LFS(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("lfs", op.frd, op.simm16, op.ra);
}

void PPUDisAsm::LFSU(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("lfsu", op.frd, op.simm16, op.ra);
}

void PPUDisAsm::LFD(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("lfd", op.frd, op.simm16, op.ra);
}

void PPUDisAsm::LFDU(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("lfdu", op.frd, op.simm16, op.ra);
}

void PPUDisAsm::STFS(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("stfs", op.frs, op.simm16, op.ra);
}

void PPUDisAsm::STFSU(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("stfsu", op.frs, op.simm16, op.ra);
}

void PPUDisAsm::STFD(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("stfd", op.frs, op.simm16, op.ra);
}

void PPUDisAsm::STFDU(ppu_opcode_t op)
{
	DisAsm_F1_IMM_R1("stfdu", op.frs, op.simm16, op.ra);
}

void PPUDisAsm::LD(ppu_opcode_t op)
{
	DisAsm_R2_IMM("ld", op.rd, op.ra, op.ds * 4);
}

void PPUDisAsm::LDU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("ldu", op.rd, op.ra, op.ds * 4);
}

void PPUDisAsm::LWA(ppu_opcode_t op)
{
	DisAsm_R2_IMM("lwa", op.rd, op.ra, op.ds * 4);
}

void PPUDisAsm::STD(ppu_opcode_t op)
{
	DisAsm_R2_IMM("std", op.rs, op.ra, op.ds * 4);
}

void PPUDisAsm::STDU(ppu_opcode_t op)
{
	DisAsm_R2_IMM("stdu", op.rs, op.ra, op.ds * 4);
}

void PPUDisAsm::FDIVS(ppu_opcode_t op)
{
	DisAsm_F3_RC("fdivs", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FSUBS(ppu_opcode_t op)
{
	DisAsm_F3_RC("fsubs", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FADDS(ppu_opcode_t op)
{
	DisAsm_F3_RC("fadds", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FSQRTS(ppu_opcode_t op)
{
	DisAsm_F2_RC("fsqrts", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FRES(ppu_opcode_t op)
{
	DisAsm_F2_RC("fres", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FMULS(ppu_opcode_t op)
{
	DisAsm_F3_RC("fmuls", op.frd, op.fra, op.frc, op.rc);
}

void PPUDisAsm::FMADDS(ppu_opcode_t op)
{
	DisAsm_F4_RC("fmadds", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FMSUBS(ppu_opcode_t op)
{
	DisAsm_F4_RC("fmsubs", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FNMSUBS(ppu_opcode_t op)
{
	DisAsm_F4_RC("fnmsubs", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FNMADDS(ppu_opcode_t op)
{
	DisAsm_F4_RC("fnmadds", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::MTFSB1(ppu_opcode_t op)
{
	Write(fmt::format("mtfsb1%s %d", op.rc ? "." : "", op.crbd));
}

void PPUDisAsm::MCRFS(ppu_opcode_t op)
{
	DisAsm_CR2("mcrfs", op.crfd, op.crfs);
}

void PPUDisAsm::MTFSB0(ppu_opcode_t op)
{
	Write(fmt::format("mtfsb0%s %d", op.rc ? "." : "", op.crbd));
}

void PPUDisAsm::MTFSFI(ppu_opcode_t op)
{
	Write(fmt::format("mtfsfi%s cr%d,%d,%d", op.rc ? "." : "", op.crfd, op.i, op.l15));
}

void PPUDisAsm::MFFS(ppu_opcode_t op)
{
	DisAsm_F1_RC("mffs", op.frd, op.rc);
}

void PPUDisAsm::MTFSF(ppu_opcode_t op)
{
	Write(fmt::format("mtfsf%s %d,f%d,%d,%d", op.rc ? "." : "", op.rc, op.flm, op.frb, op.l6, op.l15));
}

void PPUDisAsm::FCMPU(ppu_opcode_t op)
{
	DisAsm_CR1_F2("fcmpu", op.crfd, op.fra, op.frb);
}

void PPUDisAsm::FRSP(ppu_opcode_t op)
{
	DisAsm_F2_RC("frsp", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FCTIW(ppu_opcode_t op)
{
	DisAsm_F2_RC("fctiw", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FCTIWZ(ppu_opcode_t op)
{
	DisAsm_F2_RC("fctiwz", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FDIV(ppu_opcode_t op)
{
	DisAsm_F3_RC("fdiv", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FSUB(ppu_opcode_t op)
{
	DisAsm_F3_RC("fsub", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FADD(ppu_opcode_t op)
{
	DisAsm_F3_RC("fadd", op.frd, op.fra, op.frb, op.rc);
}

void PPUDisAsm::FSQRT(ppu_opcode_t op)
{
	DisAsm_F2_RC("fsqrt", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FSEL(ppu_opcode_t op)
{
	DisAsm_F4_RC("fsel", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FMUL(ppu_opcode_t op)
{
	DisAsm_F3_RC("fmul", op.frd, op.fra, op.frc, op.rc);
}

void PPUDisAsm::FRSQRTE(ppu_opcode_t op)
{
	DisAsm_F2_RC("frsqrte", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FMSUB(ppu_opcode_t op)
{
	DisAsm_F4_RC("fmsub", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FMADD(ppu_opcode_t op)
{
	DisAsm_F4_RC("fmadd", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FNMSUB(ppu_opcode_t op)
{
	DisAsm_F4_RC("fnmsub", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FNMADD(ppu_opcode_t op)
{
	DisAsm_F4_RC("fnmadd", op.frd, op.fra, op.frc, op.frb, op.rc);
}

void PPUDisAsm::FCMPO(ppu_opcode_t op)
{
	DisAsm_F3("fcmpo", op.crfd, op.fra, op.frb);
}

void PPUDisAsm::FNEG(ppu_opcode_t op)
{
	DisAsm_F2_RC("fneg", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FMR(ppu_opcode_t op)
{
	DisAsm_F2_RC("fmr", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FNABS(ppu_opcode_t op)
{
	DisAsm_F2_RC("fnabs", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FABS(ppu_opcode_t op)
{
	DisAsm_F2_RC("fabs", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FCTID(ppu_opcode_t op)
{
	DisAsm_F2_RC("fctid", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FCTIDZ(ppu_opcode_t op)
{
	DisAsm_F2_RC("fctidz", op.frd, op.frb, op.rc);
}

void PPUDisAsm::FCFID(ppu_opcode_t op)
{
	DisAsm_F2_RC("fcfid", op.frd, op.frb, op.rc);
}

extern std::vector<std::string> g_ppu_function_names;

void PPUDisAsm::UNK(ppu_opcode_t op)
{
	if (op.opcode == dump_pc && ppu_function_manager::addr)
	{
		// HLE function index
		const u32 index = (dump_pc - ppu_function_manager::addr) / 8;

		if (index < ppu_function_manager::get().size())
		{
			Write(fmt::format("Function : %s (index %u)", index < g_ppu_function_names.size() ? g_ppu_function_names[index].c_str() : "?", index));
			return;
		}
	}

	Write(fmt::format("Unknown/Illegal opcode! (0x%08x)", op.opcode));
}
