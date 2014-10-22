#ifndef PPU_LLVM_RECOMPILER_H
#define PPU_LLVM_RECOMPILER_H

#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/PassManager.h"

struct PPUState;

/// PPU recompiler that uses LLVM for code generation and optimization
class PPULLVMRecompiler : public ThreadBase, protected PPUOpcodes, protected PPCDecoder {
public:
    typedef void(*Executable)(PPUThread * ppu_state, PPUInterpreter * interpreter);

    PPULLVMRecompiler();

    PPULLVMRecompiler(const PPULLVMRecompiler & other) = delete;
    PPULLVMRecompiler(PPULLVMRecompiler && other) = delete;

    virtual ~PPULLVMRecompiler();

    PPULLVMRecompiler & operator = (const PPULLVMRecompiler & other) = delete;
    PPULLVMRecompiler & operator = (PPULLVMRecompiler && other) = delete;

    /// Get the executable for the code starting at address
    std::pair<Executable, u32> GetExecutable(u32 address);

    /// Release an executable earlier obtained through GetExecutable
    void ReleaseExecutable(u32 address, u32 revision);

    /// Request the code at the sepcified address to be compiled
    void RequestCompilation(u32 address);

    /// Get the current revision
    u32 GetCurrentRevision();

    /// Execute all tests
    void RunAllTests(PPUThread * ppu_state, PPUInterpreter * interpreter);

    void Task() override;

protected:
    void Decode(const u32 code) override;

    void NULL_OP() override;
    void NOP() override;

    void TDI(u32 to, u32 ra, s32 simm16) override;
    void TWI(u32 to, u32 ra, s32 simm16) override;

    void MFVSCR(u32 vd) override;
    void MTVSCR(u32 vb) override;
    void VADDCUW(u32 vd, u32 va, u32 vb) override;
    void VADDFP(u32 vd, u32 va, u32 vb) override;
    void VADDSBS(u32 vd, u32 va, u32 vb) override;
    void VADDSHS(u32 vd, u32 va, u32 vb) override;
    void VADDSWS(u32 vd, u32 va, u32 vb) override;
    void VADDUBM(u32 vd, u32 va, u32 vb) override;
    void VADDUBS(u32 vd, u32 va, u32 vb) override;
    void VADDUHM(u32 vd, u32 va, u32 vb) override;
    void VADDUHS(u32 vd, u32 va, u32 vb) override;
    void VADDUWM(u32 vd, u32 va, u32 vb) override;
    void VADDUWS(u32 vd, u32 va, u32 vb) override;
    void VAND(u32 vd, u32 va, u32 vb) override;
    void VANDC(u32 vd, u32 va, u32 vb) override;
    void VAVGSB(u32 vd, u32 va, u32 vb) override;
    void VAVGSH(u32 vd, u32 va, u32 vb) override;
    void VAVGSW(u32 vd, u32 va, u32 vb) override;
    void VAVGUB(u32 vd, u32 va, u32 vb) override;
    void VAVGUH(u32 vd, u32 va, u32 vb) override;
    void VAVGUW(u32 vd, u32 va, u32 vb) override;
    void VCFSX(u32 vd, u32 uimm5, u32 vb) override;
    void VCFUX(u32 vd, u32 uimm5, u32 vb) override;
    void VCMPBFP(u32 vd, u32 va, u32 vb) override;
    void VCMPBFP_(u32 vd, u32 va, u32 vb) override;
    void VCMPEQFP(u32 vd, u32 va, u32 vb) override;
    void VCMPEQFP_(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUB(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUB_(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUH(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUH_(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUW(u32 vd, u32 va, u32 vb) override;
    void VCMPEQUW_(u32 vd, u32 va, u32 vb) override;
    void VCMPGEFP(u32 vd, u32 va, u32 vb) override;
    void VCMPGEFP_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTFP(u32 vd, u32 va, u32 vb) override;
    void VCMPGTFP_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSB(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSB_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSH(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSH_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSW(u32 vd, u32 va, u32 vb) override;
    void VCMPGTSW_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUB(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUB_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUH(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUH_(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUW(u32 vd, u32 va, u32 vb) override;
    void VCMPGTUW_(u32 vd, u32 va, u32 vb) override;
    void VCTSXS(u32 vd, u32 uimm5, u32 vb) override;
    void VCTUXS(u32 vd, u32 uimm5, u32 vb) override;
    void VEXPTEFP(u32 vd, u32 vb) override;
    void VLOGEFP(u32 vd, u32 vb) override;
    void VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) override;
    void VMAXFP(u32 vd, u32 va, u32 vb) override;
    void VMAXSB(u32 vd, u32 va, u32 vb) override;
    void VMAXSH(u32 vd, u32 va, u32 vb) override;
    void VMAXSW(u32 vd, u32 va, u32 vb) override;
    void VMAXUB(u32 vd, u32 va, u32 vb) override;
    void VMAXUH(u32 vd, u32 va, u32 vb) override;
    void VMAXUW(u32 vd, u32 va, u32 vb) override;
    void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMINFP(u32 vd, u32 va, u32 vb) override;
    void VMINSB(u32 vd, u32 va, u32 vb) override;
    void VMINSH(u32 vd, u32 va, u32 vb) override;
    void VMINSW(u32 vd, u32 va, u32 vb) override;
    void VMINUB(u32 vd, u32 va, u32 vb) override;
    void VMINUH(u32 vd, u32 va, u32 vb) override;
    void VMINUW(u32 vd, u32 va, u32 vb) override;
    void VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMRGHB(u32 vd, u32 va, u32 vb) override;
    void VMRGHH(u32 vd, u32 va, u32 vb) override;
    void VMRGHW(u32 vd, u32 va, u32 vb) override;
    void VMRGLB(u32 vd, u32 va, u32 vb) override;
    void VMRGLH(u32 vd, u32 va, u32 vb) override;
    void VMRGLW(u32 vd, u32 va, u32 vb) override;
    void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VMULESB(u32 vd, u32 va, u32 vb) override;
    void VMULESH(u32 vd, u32 va, u32 vb) override;
    void VMULEUB(u32 vd, u32 va, u32 vb) override;
    void VMULEUH(u32 vd, u32 va, u32 vb) override;
    void VMULOSB(u32 vd, u32 va, u32 vb) override;
    void VMULOSH(u32 vd, u32 va, u32 vb) override;
    void VMULOUB(u32 vd, u32 va, u32 vb) override;
    void VMULOUH(u32 vd, u32 va, u32 vb) override;
    void VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) override;
    void VNOR(u32 vd, u32 va, u32 vb) override;
    void VOR(u32 vd, u32 va, u32 vb) override;
    void VPERM(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VPKPX(u32 vd, u32 va, u32 vb) override;
    void VPKSHSS(u32 vd, u32 va, u32 vb) override;
    void VPKSHUS(u32 vd, u32 va, u32 vb) override;
    void VPKSWSS(u32 vd, u32 va, u32 vb) override;
    void VPKSWUS(u32 vd, u32 va, u32 vb) override;
    void VPKUHUM(u32 vd, u32 va, u32 vb) override;
    void VPKUHUS(u32 vd, u32 va, u32 vb) override;
    void VPKUWUM(u32 vd, u32 va, u32 vb) override;
    void VPKUWUS(u32 vd, u32 va, u32 vb) override;
    void VREFP(u32 vd, u32 vb) override;
    void VRFIM(u32 vd, u32 vb) override;
    void VRFIN(u32 vd, u32 vb) override;
    void VRFIP(u32 vd, u32 vb) override;
    void VRFIZ(u32 vd, u32 vb) override;
    void VRLB(u32 vd, u32 va, u32 vb) override;
    void VRLH(u32 vd, u32 va, u32 vb) override;
    void VRLW(u32 vd, u32 va, u32 vb) override;
    void VRSQRTEFP(u32 vd, u32 vb) override;
    void VSEL(u32 vd, u32 va, u32 vb, u32 vc) override;
    void VSL(u32 vd, u32 va, u32 vb) override;
    void VSLB(u32 vd, u32 va, u32 vb) override;
    void VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) override;
    void VSLH(u32 vd, u32 va, u32 vb) override;
    void VSLO(u32 vd, u32 va, u32 vb) override;
    void VSLW(u32 vd, u32 va, u32 vb) override;
    void VSPLTB(u32 vd, u32 uimm5, u32 vb) override;
    void VSPLTH(u32 vd, u32 uimm5, u32 vb) override;
    void VSPLTISB(u32 vd, s32 simm5) override;
    void VSPLTISH(u32 vd, s32 simm5) override;
    void VSPLTISW(u32 vd, s32 simm5) override;
    void VSPLTW(u32 vd, u32 uimm5, u32 vb) override;
    void VSR(u32 vd, u32 va, u32 vb) override;
    void VSRAB(u32 vd, u32 va, u32 vb) override;
    void VSRAH(u32 vd, u32 va, u32 vb) override;
    void VSRAW(u32 vd, u32 va, u32 vb) override;
    void VSRB(u32 vd, u32 va, u32 vb) override;
    void VSRH(u32 vd, u32 va, u32 vb) override;
    void VSRO(u32 vd, u32 va, u32 vb) override;
    void VSRW(u32 vd, u32 va, u32 vb) override;
    void VSUBCUW(u32 vd, u32 va, u32 vb) override;
    void VSUBFP(u32 vd, u32 va, u32 vb) override;
    void VSUBSBS(u32 vd, u32 va, u32 vb) override;
    void VSUBSHS(u32 vd, u32 va, u32 vb) override;
    void VSUBSWS(u32 vd, u32 va, u32 vb) override;
    void VSUBUBM(u32 vd, u32 va, u32 vb) override;
    void VSUBUBS(u32 vd, u32 va, u32 vb) override;
    void VSUBUHM(u32 vd, u32 va, u32 vb) override;
    void VSUBUHS(u32 vd, u32 va, u32 vb) override;
    void VSUBUWM(u32 vd, u32 va, u32 vb) override;
    void VSUBUWS(u32 vd, u32 va, u32 vb) override;
    void VSUMSWS(u32 vd, u32 va, u32 vb) override;
    void VSUM2SWS(u32 vd, u32 va, u32 vb) override;
    void VSUM4SBS(u32 vd, u32 va, u32 vb) override;
    void VSUM4SHS(u32 vd, u32 va, u32 vb) override;
    void VSUM4UBS(u32 vd, u32 va, u32 vb) override;
    void VUPKHPX(u32 vd, u32 vb) override;
    void VUPKHSB(u32 vd, u32 vb) override;
    void VUPKHSH(u32 vd, u32 vb) override;
    void VUPKLPX(u32 vd, u32 vb) override;
    void VUPKLSB(u32 vd, u32 vb) override;
    void VUPKLSH(u32 vd, u32 vb) override;
    void VXOR(u32 vd, u32 va, u32 vb) override;
    void MULLI(u32 rd, u32 ra, s32 simm16) override;
    void SUBFIC(u32 rd, u32 ra, s32 simm16) override;
    void CMPLI(u32 bf, u32 l, u32 ra, u32 uimm16) override;
    void CMPI(u32 bf, u32 l, u32 ra, s32 simm16) override;
    void ADDIC(u32 rd, u32 ra, s32 simm16) override;
    void ADDIC_(u32 rd, u32 ra, s32 simm16) override;
    void ADDI(u32 rd, u32 ra, s32 simm16) override;
    void ADDIS(u32 rd, u32 ra, s32 simm16) override;
    void BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) override;
    void SC(u32 sc_code) override;
    void B(s32 ll, u32 aa, u32 lk) override;
    void MCRF(u32 crfd, u32 crfs) override;
    void BCLR(u32 bo, u32 bi, u32 bh, u32 lk) override;
    void CRNOR(u32 bt, u32 ba, u32 bb) override;
    void CRANDC(u32 bt, u32 ba, u32 bb) override;
    void ISYNC() override;
    void CRXOR(u32 bt, u32 ba, u32 bb) override;
    void CRNAND(u32 bt, u32 ba, u32 bb) override;
    void CRAND(u32 bt, u32 ba, u32 bb) override;
    void CREQV(u32 bt, u32 ba, u32 bb) override;
    void CRORC(u32 bt, u32 ba, u32 bb) override;
    void CROR(u32 bt, u32 ba, u32 bb) override;
    void BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) override;
    void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) override;
    void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) override;
    void RLWNM(u32 ra, u32 rs, u32 rb, u32 MB, u32 ME, bool rc) override;
    void ORI(u32 rs, u32 ra, u32 uimm16) override;
    void ORIS(u32 rs, u32 ra, u32 uimm16) override;
    void XORI(u32 ra, u32 rs, u32 uimm16) override;
    void XORIS(u32 ra, u32 rs, u32 uimm16) override;
    void ANDI_(u32 ra, u32 rs, u32 uimm16) override;
    void ANDIS_(u32 ra, u32 rs, u32 uimm16) override;
    void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
    void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) override;
    void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
    void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
    void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) override;
    void CMP(u32 crfd, u32 l, u32 ra, u32 rb) override;
    void TW(u32 to, u32 ra, u32 rb) override;
    void LVSL(u32 vd, u32 ra, u32 rb) override;
    void LVEBX(u32 vd, u32 ra, u32 rb) override;
    void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void MULHDU(u32 rd, u32 ra, u32 rb, bool rc) override;
    void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void MULHWU(u32 rd, u32 ra, u32 rb, bool rc) override;
    void MFOCRF(u32 a, u32 rd, u32 crm) override;
    void LWARX(u32 rd, u32 ra, u32 rb) override;
    void LDX(u32 ra, u32 rs, u32 rb) override;
    void LWZX(u32 rd, u32 ra, u32 rb) override;
    void SLW(u32 ra, u32 rs, u32 rb, bool rc) override;
    void CNTLZW(u32 ra, u32 rs, bool rc) override;
    void SLD(u32 ra, u32 rs, u32 rb, bool rc) override;
    void AND(u32 ra, u32 rs, u32 rb, bool rc) override;
    void CMPL(u32 bf, u32 l, u32 ra, u32 rb) override;
    void LVSR(u32 vd, u32 ra, u32 rb) override;
    void LVEHX(u32 vd, u32 ra, u32 rb) override;
    void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void LDUX(u32 rd, u32 ra, u32 rb) override;
    void DCBST(u32 ra, u32 rb) override;
    void LWZUX(u32 rd, u32 ra, u32 rb) override;
    void CNTLZD(u32 ra, u32 rs, bool rc) override;
    void ANDC(u32 ra, u32 rs, u32 rb, bool rc) override;
    void TD(u32 to, u32 ra, u32 rb) override;
    void LVEWX(u32 vd, u32 ra, u32 rb) override;
    void MULHD(u32 rd, u32 ra, u32 rb, bool rc) override;
    void MULHW(u32 rd, u32 ra, u32 rb, bool rc) override;
    void LDARX(u32 rd, u32 ra, u32 rb) override;
    void DCBF(u32 ra, u32 rb) override;
    void LBZX(u32 rd, u32 ra, u32 rb) override;
    void LVX(u32 vd, u32 ra, u32 rb) override;
    void NEG(u32 rd, u32 ra, u32 oe, bool rc) override;
    void LBZUX(u32 rd, u32 ra, u32 rb) override;
    void NOR(u32 ra, u32 rs, u32 rb, bool rc) override;
    void STVEBX(u32 vs, u32 ra, u32 rb) override;
    void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void MTOCRF(u32 l, u32 crm, u32 rs) override;
    void STDX(u32 rs, u32 ra, u32 rb) override;
    void STWCX_(u32 rs, u32 ra, u32 rb) override;
    void STWX(u32 rs, u32 ra, u32 rb) override;
    void STVEHX(u32 vs, u32 ra, u32 rb) override;
    void STDUX(u32 rs, u32 ra, u32 rb) override;
    void STWUX(u32 rs, u32 ra, u32 rb) override;
    void STVEWX(u32 vs, u32 ra, u32 rb) override;
    void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) override;
    void ADDZE(u32 rd, u32 ra, u32 oe, bool rc) override;
    void STDCX_(u32 rs, u32 ra, u32 rb) override;
    void STBX(u32 rs, u32 ra, u32 rb) override;
    void STVX(u32 vs, u32 ra, u32 rb) override;
    void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void SUBFME(u32 rd, u32 ra, u32 oe, bool rc) override;
    void ADDME(u32 rd, u32 ra, u32 oe, bool rc) override;
    void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void DCBTST(u32 ra, u32 rb, u32 th) override;
    void STBUX(u32 rs, u32 ra, u32 rb) override;
    void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void DCBT(u32 ra, u32 rb, u32 th) override;
    void LHZX(u32 rd, u32 ra, u32 rb) override;
    void EQV(u32 ra, u32 rs, u32 rb, bool rc) override;
    void ECIWX(u32 rd, u32 ra, u32 rb) override;
    void LHZUX(u32 rd, u32 ra, u32 rb) override;
    void XOR(u32 rs, u32 ra, u32 rb, bool rc) override;
    void MFSPR(u32 rd, u32 spr) override;
    void LWAX(u32 rd, u32 ra, u32 rb) override;
    void DST(u32 ra, u32 rb, u32 strm, u32 t) override;
    void LHAX(u32 rd, u32 ra, u32 rb) override;
    void LVXL(u32 vd, u32 ra, u32 rb) override;
    void MFTB(u32 rd, u32 spr) override;
    void LWAUX(u32 rd, u32 ra, u32 rb) override;
    void DSTST(u32 ra, u32 rb, u32 strm, u32 t) override;
    void LHAUX(u32 rd, u32 ra, u32 rb) override;
    void STHX(u32 rs, u32 ra, u32 rb) override;
    void ORC(u32 rs, u32 ra, u32 rb, bool rc) override;
    void ECOWX(u32 rs, u32 ra, u32 rb) override;
    void STHUX(u32 rs, u32 ra, u32 rb) override;
    void OR(u32 ra, u32 rs, u32 rb, bool rc) override;
    void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void MTSPR(u32 spr, u32 rs) override;
    //DCBI
    void NAND(u32 ra, u32 rs, u32 rb, bool rc) override;
    void STVXL(u32 vs, u32 ra, u32 rb) override;
    void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
    void LVLX(u32 vd, u32 ra, u32 rb) override;
    void LDBRX(u32 rd, u32 ra, u32 rb) override;
    void LSWX(u32 rd, u32 ra, u32 rb) override;
    void LWBRX(u32 rd, u32 ra, u32 rb) override;
    void LFSX(u32 frd, u32 ra, u32 rb) override;
    void SRW(u32 ra, u32 rs, u32 rb, bool rc) override;
    void SRD(u32 ra, u32 rs, u32 rb, bool rc) override;
    void LVRX(u32 vd, u32 ra, u32 rb) override;
    void LSWI(u32 rd, u32 ra, u32 nb) override;
    void LFSUX(u32 frd, u32 ra, u32 rb) override;
    void SYNC(u32 l) override;
    void LFDX(u32 frd, u32 ra, u32 rb) override;
    void LFDUX(u32 frd, u32 ra, u32 rb) override;
    void STVLX(u32 vs, u32 ra, u32 rb) override;
    void STSWX(u32 rs, u32 ra, u32 rb) override;
    void STWBRX(u32 rs, u32 ra, u32 rb) override;
    void STFSX(u32 frs, u32 ra, u32 rb) override;
    void STVRX(u32 vs, u32 ra, u32 rb) override;
    void STFSUX(u32 frs, u32 ra, u32 rb) override;
    void STSWI(u32 rd, u32 ra, u32 nb) override;
    void STFDX(u32 frs, u32 ra, u32 rb) override;
    void STFDUX(u32 frs, u32 ra, u32 rb) override;
    void LVLXL(u32 vd, u32 ra, u32 rb) override;
    void LHBRX(u32 rd, u32 ra, u32 rb) override;
    void SRAW(u32 ra, u32 rs, u32 rb, bool rc) override;
    void SRAD(u32 ra, u32 rs, u32 rb, bool rc) override;
    void LVRXL(u32 vd, u32 ra, u32 rb) override;
    void DSS(u32 strm, u32 a) override;
    void SRAWI(u32 ra, u32 rs, u32 sh, bool rc) override;
    void SRADI1(u32 ra, u32 rs, u32 sh, bool rc) override;
    void SRADI2(u32 ra, u32 rs, u32 sh, bool rc) override;
    void EIEIO() override;
    void STVLXL(u32 vs, u32 ra, u32 rb) override;
    void STHBRX(u32 rs, u32 ra, u32 rb) override;
    void EXTSH(u32 ra, u32 rs, bool rc) override;
    void STVRXL(u32 sd, u32 ra, u32 rb) override;
    void EXTSB(u32 ra, u32 rs, bool rc) override;
    void STFIWX(u32 frs, u32 ra, u32 rb) override;
    void EXTSW(u32 ra, u32 rs, bool rc) override;
    void ICBI(u32 ra, u32 rb) override;
    void DCBZ(u32 ra, u32 rb) override;
    void LWZ(u32 rd, u32 ra, s32 d) override;
    void LWZU(u32 rd, u32 ra, s32 d) override;
    void LBZ(u32 rd, u32 ra, s32 d) override;
    void LBZU(u32 rd, u32 ra, s32 d) override;
    void STW(u32 rs, u32 ra, s32 d) override;
    void STWU(u32 rs, u32 ra, s32 d) override;
    void STB(u32 rs, u32 ra, s32 d) override;
    void STBU(u32 rs, u32 ra, s32 d) override;
    void LHZ(u32 rd, u32 ra, s32 d) override;
    void LHZU(u32 rd, u32 ra, s32 d) override;
    void LHA(u32 rs, u32 ra, s32 d) override;
    void LHAU(u32 rs, u32 ra, s32 d) override;
    void STH(u32 rs, u32 ra, s32 d) override;
    void STHU(u32 rs, u32 ra, s32 d) override;
    void LMW(u32 rd, u32 ra, s32 d) override;
    void STMW(u32 rs, u32 ra, s32 d) override;
    void LFS(u32 frd, u32 ra, s32 d) override;
    void LFSU(u32 frd, u32 ra, s32 d) override;
    void LFD(u32 frd, u32 ra, s32 d) override;
    void LFDU(u32 frd, u32 ra, s32 d) override;
    void STFS(u32 frs, u32 ra, s32 d) override;
    void STFSU(u32 frs, u32 ra, s32 d) override;
    void STFD(u32 frs, u32 ra, s32 d) override;
    void STFDU(u32 frs, u32 ra, s32 d) override;
    void LD(u32 rd, u32 ra, s32 ds) override;
    void LDU(u32 rd, u32 ra, s32 ds) override;
    void LWA(u32 rd, u32 ra, s32 ds) override;
    void FDIVS(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FSUBS(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FADDS(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FSQRTS(u32 frd, u32 frb, bool rc) override;
    void FRES(u32 frd, u32 frb, bool rc) override;
    void FMULS(u32 frd, u32 fra, u32 frc, bool rc) override;
    void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void STD(u32 rs, u32 ra, s32 ds) override;
    void STDU(u32 rs, u32 ra, s32 ds) override;
    void MTFSB1(u32 bt, bool rc) override;
    void MCRFS(u32 bf, u32 bfa) override;
    void MTFSB0(u32 bt, bool rc) override;
    void MTFSFI(u32 crfd, u32 i, bool rc) override;
    void MFFS(u32 frd, bool rc) override;
    void MTFSF(u32 flm, u32 frb, bool rc) override;

    void FCMPU(u32 bf, u32 fra, u32 frb) override;
    void FRSP(u32 frd, u32 frb, bool rc) override;
    void FCTIW(u32 frd, u32 frb, bool rc) override;
    void FCTIWZ(u32 frd, u32 frb, bool rc) override;
    void FDIV(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FSUB(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FADD(u32 frd, u32 fra, u32 frb, bool rc) override;
    void FSQRT(u32 frd, u32 frb, bool rc) override;
    void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FMUL(u32 frd, u32 fra, u32 frc, bool rc) override;
    void FRSQRTE(u32 frd, u32 frb, bool rc) override;
    void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
    void FCMPO(u32 crfd, u32 fra, u32 frb) override;
    void FNEG(u32 frd, u32 frb, bool rc) override;
    void FMR(u32 frd, u32 frb, bool rc) override;
    void FNABS(u32 frd, u32 frb, bool rc) override;
    void FABS(u32 frd, u32 frb, bool rc) override;
    void FCTID(u32 frd, u32 frb, bool rc) override;
    void FCTIDZ(u32 frd, u32 frb, bool rc) override;
    void FCFID(u32 frd, u32 frb, bool rc) override;

    void UNK(const u32 code, const u32 opcode, const u32 gcode) override;

private:
    struct ExecutableInfo {
        /// Pointer to the executable
        Executable executable;

        /// Size of the executable
        size_t size;

        /// Number of PPU instructions compiled into this executable
        u32 num_instructions;

        /// List of blocks that this executable refers to that have not been hit yet
        std::list<u32> unhit_blocks_list;

        /// LLVM function corresponding to the executable
        llvm::Function * llvm_function;
    };

    /// Lock for accessing m_compiled_shared
    // TODO: Use a RW lock
    std::mutex m_compiled_shared_lock;

    /// Sections that have been compiled. This data store is shared with the execution threads.
    /// Keys are starting address of the section and ~revision. Data is pointer to the executable and its reference count.
    std::map<std::pair<u32, u32>, std::pair<Executable, u32>> m_compiled_shared;

    /// Lock for accessing m_uncompiled_shared
    std::mutex m_uncompiled_shared_lock;

    /// Current revision. This is incremented everytime a section is compiled.
    std::atomic<u32> m_revision;

    /// Sections that have not been compiled yet. This data store is shared with the execution threads.
    std::list<u32> m_uncompiled_shared;

    /// Set of all blocks that have been hit
    std::set<u32> m_hit_blocks;

    /// Sections that have been compiled. Keys are starting address of the section and ~revision.
    std::map<std::pair<u32, u32>, ExecutableInfo> m_compiled;

    /// LLVM context
    llvm::LLVMContext * m_llvm_context;

    /// LLVM IR builder
    llvm::IRBuilder<> * m_ir_builder;

    /// Module to which all generated code is output to
    llvm::Module * m_module;

    /// JIT execution engine
    llvm::ExecutionEngine * m_execution_engine;

    /// Function pass manager
    llvm::FunctionPassManager * m_fpm;

    /// A flag used to detect branch instructions.
    /// This is set to false at the start of compilation of a block.
    /// When a branch instruction is encountered, this is set to true by the decode function.
    bool m_hit_branch_instruction;

    /// The function being compiled
    llvm::Function * m_current_function;

    /// List of blocks to be compiled in the current function being compiled
    std::list<u32> m_current_function_uncompiled_blocks_list;

    /// List of blocks that the current function refers to but have not been hit yet
    std::list<u32> m_current_function_unhit_blocks_list;

    /// Address of the current instruction
    u32 m_current_instruction_address;

    /// Number of instructions in this section
    u32 m_num_instructions;

    /// Time spent building the LLVM IR
    std::chrono::nanoseconds m_ir_build_time;

    /// Time spent optimizing
    std::chrono::nanoseconds m_optimizing_time;

    /// Time spent translating LLVM IR to machine code
    std::chrono::nanoseconds m_translation_time;

    /// Time spent compiling
    std::chrono::nanoseconds m_compilation_time;

    /// Time spent idling
    std::chrono::nanoseconds m_idling_time;

    /// Total time
    std::chrono::nanoseconds m_total_time;

    /// Contains the number of times the interpreter fallback was used
    std::map<std::string, u64> m_interpreter_fallback_stats;

    /// Get the block in function for the instruction at the specified address.
    llvm::BasicBlock * GetBlockInFunction(u32 address, llvm::Function * function, bool create_if_not_exist = false);

    /// Compile the section startin at address
    void Compile(u32 address);

    /// Remove old versions of executables that are no longer used by any execution thread
    void RemoveUnusedOldVersions();

    /// Test whether the blocks needs to be compiled
    bool NeedsCompiling(u32 address);

    /// Get PPU state pointer
    llvm::Value * GetPPUState();

    /// Get interpreter pointer
    llvm::Value * GetInterpreter();

    /// Get a bit
    llvm::Value * GetBit(llvm::Value * val, u32 n);

    /// Clear a bit
    llvm::Value * ClrBit(llvm::Value * val, u32 n);

    /// Set a bit
    llvm::Value * SetBit(llvm::Value * val, u32 n, llvm::Value * bit, bool doClear = true);

    /// Get a nibble
    llvm::Value * GetNibble(llvm::Value * val, u32 n);

    /// Clear a nibble
    llvm::Value * ClrNibble(llvm::Value * val, u32 n);

    /// Set a nibble
    llvm::Value * SetNibble(llvm::Value * val, u32 n, llvm::Value * nibble, bool doClear = true);

    /// Set a nibble
    llvm::Value * SetNibble(llvm::Value * val, u32 n, llvm::Value * b0, llvm::Value * b1, llvm::Value * b2, llvm::Value * b3, bool doClear = true);

    /// Load PC
    llvm::Value * GetPc();

    /// Set PC
    void SetPc(llvm::Value * val_ix);

    /// Load GPR
    llvm::Value * GetGpr(u32 r, u32 num_bits = 64);

    /// Set GPR
    void SetGpr(u32 r, llvm::Value * val_x64);

    /// Load CR
    llvm::Value * GetCr();

    /// Load CR and get field CRn
    llvm::Value * GetCrField(u32 n);

    /// Set CR
    void SetCr(llvm::Value * val_x32);

    /// Set CR field
    void SetCrField(u32 n, llvm::Value * field);

    /// Set CR field
    void SetCrField(u32 n, llvm::Value * b0, llvm::Value * b1, llvm::Value * b2, llvm::Value * b3);

    /// Set CR field based on signed comparison
    void SetCrFieldSignedCmp(u32 n, llvm::Value * a, llvm::Value * b);

    /// Set CR field based on unsigned comparison
    void SetCrFieldUnsignedCmp(u32 n, llvm::Value * a, llvm::Value * b);

    /// Set CR6 based on the result of the vector compare instruction
    void SetCr6AfterVectorCompare(u32 vr);

    /// Get LR
    llvm::Value * GetLr();

    /// Set LR
    void SetLr(llvm::Value * val_x64);

    /// Get CTR
    llvm::Value * GetCtr();

    /// Set CTR
    void SetCtr(llvm::Value * val_x64);

    /// Load XER and convert it to an i64
    llvm::Value * GetXer();

    /// Load XER and return the CA bit
    llvm::Value * GetXerCa();

    /// Load XER and return the SO bit
    llvm::Value * GetXerSo();

    /// Set XER
    void SetXer(llvm::Value * val_x64);

    /// Set the CA bit of XER
    void SetXerCa(llvm::Value * ca);

    /// Set the SO bit of XER
    void SetXerSo(llvm::Value * so);

    /// Get USPRG0
    llvm::Value * GetUsprg0();

    /// Set USPRG0
    void SetUsprg0(llvm::Value * val_x64);

    /// Get FPR
    llvm::Value * GetFpr(u32 r, u32 bits = 64, bool as_int = false);

    /// Set FPR
    void SetFpr(u32 r, llvm::Value * val);

    /// Load VSCR
    llvm::Value * GetVscr();

    /// Set VSCR
    void SetVscr(llvm::Value * val_x32);

    /// Load VR
    llvm::Value * GetVr(u32 vr);

    /// Load VR and convert it to an integer vector
    llvm::Value * GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits);

    /// Load VR and convert it to a float vector with 4 elements
    llvm::Value * GetVrAsFloatVec(u32 vr);

    /// Load VR and convert it to a double vector with 2 elements
    llvm::Value * GetVrAsDoubleVec(u32 vr);

    /// Set VR to the specified value
    void SetVr(u32 vr, llvm::Value * val_x128);

    /// Check condition for branch instructions
    llvm::Value * CheckBranchCondition(u32 bo, u32 bi);

    /// Create IR for a branch instruction
    void CreateBranch(llvm::Value * cmp_i1, llvm::Value * target_i64, bool lk);

    /// Read from memory
    llvm::Value * ReadMemory(llvm::Value * addr_i64, u32 bits, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

    /// Write to memory
    void WriteMemory(llvm::Value * addr_i64, llvm::Value * val_ix, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

    /// Call an interpreter function
    template<class Func, class... Args>
    llvm::Value * InterpreterCall(const char * name, Func function, Args... args);

    /// Convert a C++ type to an LLVM type
    template<class T>
    llvm::Type * CppToLlvmType();

    /// Call a function
    template<class ReturnType, class Func, class... Args>
    llvm::Value * Call(const char * name, Func function, Args... args);

    /// Test an instruction against the interpreter
    template <class PPULLVMRecompilerFn, class PPUInterpreterFn, class... Args>
    void VerifyInstructionAgainstInterpreter(const char * name, PPULLVMRecompilerFn recomp_fn, PPUInterpreterFn interp_fn, PPUState & input_state, Args... args);

    /// Excute a test
    void RunTest(const char * name, std::function<void()> test_case, std::function<void()> input, std::function<bool(std::string & msg)> check_result);

    /// A mask used in rotate instructions
    static u64 s_rotate_mask[64][64];

    /// A flag indicating whether s_rotate_mask has been initialised or not
    static bool s_rotate_mask_inited;

    /// Initialse s_rotate_mask
    static void InitRotateMask();
};

/// PPU emulator that uses LLVM to convert PPU instructions to host CPU instructions
class PPULLVMEmulator : public CPUDecoder {
public:
    PPULLVMEmulator(PPUThread & ppu);
    PPULLVMEmulator() = delete;

    PPULLVMEmulator(const PPULLVMEmulator & other) = delete;
    PPULLVMEmulator(PPULLVMEmulator && other) = delete;

    virtual ~PPULLVMEmulator();

    PPULLVMEmulator & operator = (const PPULLVMEmulator & other) = delete;
    PPULLVMEmulator & operator = (PPULLVMEmulator && other) = delete;

    u8 DecodeMemory(const u32 address) override;

private:
    struct ExecutableInfo {
        /// Pointer to the executable
        PPULLVMRecompiler::Executable executable;

        /// The revision of the executable
        u32 revision;

        /// Number of times the executable was hit
        u32 num_hits;
    };

    /// PPU processor context
    PPUThread & m_ppu;

    /// PPU Interpreter
    PPUInterpreter * m_interpreter;

    /// PPU instruction Decoder
    PPUDecoder m_decoder;

    /// Set to true if the last executed instruction was a branch
    bool m_last_instr_was_branch;

    /// The time at which the m_address_to_executable cache was last cleared
    std::chrono::high_resolution_clock::time_point m_last_cache_clear_time;

    /// The revision of the recompiler to which this thread is synced
    u32 m_recompiler_revision;

    /// Address to executable map. Key is address.
    std::unordered_map<u32, ExecutableInfo> m_address_to_executable;

    /// Sections that have not been compiled yet. Key is starting address of the section.
    std::unordered_map<u32, u64> m_uncompiled;

    /// Number of instances of this class
    static u32 s_num_instances;

    /// Mutex used prevent multiple instances of the recompiler from being created
    static std::mutex s_recompiler_mutex;

    /// PPU to LLVM recompiler
    static PPULLVMRecompiler * s_recompiler;
};

#endif // PPU_LLVM_RECOMPILER_H
