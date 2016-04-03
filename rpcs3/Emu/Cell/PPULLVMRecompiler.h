#ifndef PPU_LLVM_RECOMPILER_H
#define PPU_LLVM_RECOMPILER_H

#ifdef LLVM_AVAILABLE
#define PPU_LLVM_RECOMPILER 1

#include <list>
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUInterpreter.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ppu_recompiler_llvm {
	enum ExecutionStatus
	{
		ExecutionStatusReturn = 0, ///< Block has hit a return, caller can continue execution
		ExecutionStatusBlockEnded, ///< Block has been executed but no return was hit, at least another block must be executed before caller can continue
		ExecutionStatusPropagateException, ///< an exception was thrown
	};

	class Compiler;
	class RecompilationEngine;
	class ExecutionEngine;
	struct PPUState;

	enum class BranchType {
		NonBranch,
		LocalBranch,
		FunctionCall,
		Return,
	};

	/// Pointer to an executable
	typedef u32(*Executable)(PPUThread * ppu_state, u64 context);

	/// Parses PPU opcodes and translate them into llvm ir.
	class Compiler : protected PPUOpcodes, protected PPCDecoder {
	public:
		Compiler(llvm::LLVMContext *context, llvm::IRBuilder<> *builder, std::unordered_map<std::string, void*> &function_ptrs);

		Compiler(const Compiler&) = delete; // Delete copy/move constructors and copy/move operators

		virtual ~Compiler();

		/// Create a module setting target triples and some callbacks
		static std::unique_ptr<llvm::Module> create_module(llvm::LLVMContext &llvm_context);

		/// Create a function called name in module and populates it by translating block at start_address with instruction_count length.
		void translate_to_llvm_ir(llvm::Module *module, const std::string & name, u32 start_address, u32 instruction_count);

		static void optimise_module(llvm::Module *module);

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
		void HACK(u32 id) override;
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
		void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, u32 rc) override;
		void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, u32 rc) override;
		void RLWNM(u32 ra, u32 rs, u32 rb, u32 MB, u32 ME, u32 rc) override;
		void ORI(u32 rs, u32 ra, u32 uimm16) override;
		void ORIS(u32 rs, u32 ra, u32 uimm16) override;
		void XORI(u32 ra, u32 rs, u32 uimm16) override;
		void XORIS(u32 ra, u32 rs, u32 uimm16) override;
		void ANDI_(u32 ra, u32 rs, u32 uimm16) override;
		void ANDIS_(u32 ra, u32 rs, u32 uimm16) override;
		void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, u32 rc) override;
		void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, u32 rc) override;
		void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, u32 rc) override;
		void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 rc) override;
		void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, u32 is_r, u32 rc) override;
		void CMP(u32 crfd, u32 l, u32 ra, u32 rb) override;
		void TW(u32 to, u32 ra, u32 rb) override;
		void LVSL(u32 vd, u32 ra, u32 rb) override;
		void LVEBX(u32 vd, u32 ra, u32 rb) override;
		void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void MULHDU(u32 rd, u32 ra, u32 rb, u32 rc) override;
		void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void MULHWU(u32 rd, u32 ra, u32 rb, u32 rc) override;
		void MFOCRF(u32 a, u32 rd, u32 crm) override;
		void LWARX(u32 rd, u32 ra, u32 rb) override;
		void LDX(u32 ra, u32 rs, u32 rb) override;
		void LWZX(u32 rd, u32 ra, u32 rb) override;
		void SLW(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void CNTLZW(u32 ra, u32 rs, u32 rc) override;
		void SLD(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void AND(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void CMPL(u32 bf, u32 l, u32 ra, u32 rb) override;
		void LVSR(u32 vd, u32 ra, u32 rb) override;
		void LVEHX(u32 vd, u32 ra, u32 rb) override;
		void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void LDUX(u32 rd, u32 ra, u32 rb) override;
		void DCBST(u32 ra, u32 rb) override;
		void LWZUX(u32 rd, u32 ra, u32 rb) override;
		void CNTLZD(u32 ra, u32 rs, u32 rc) override;
		void ANDC(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void TD(u32 to, u32 ra, u32 rb) override;
		void LVEWX(u32 vd, u32 ra, u32 rb) override;
		void MULHD(u32 rd, u32 ra, u32 rb, u32 rc) override;
		void MULHW(u32 rd, u32 ra, u32 rb, u32 rc) override;
		void LDARX(u32 rd, u32 ra, u32 rb) override;
		void DCBF(u32 ra, u32 rb) override;
		void LBZX(u32 rd, u32 ra, u32 rb) override;
		void LVX(u32 vd, u32 ra, u32 rb) override;
		void NEG(u32 rd, u32 ra, u32 oe, u32 rc) override;
		void LBZUX(u32 rd, u32 ra, u32 rb) override;
		void NOR(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void STVEBX(u32 vs, u32 ra, u32 rb) override;
		void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void MTOCRF(u32 l, u32 crm, u32 rs) override;
		void STDX(u32 rs, u32 ra, u32 rb) override;
		void STWCX_(u32 rs, u32 ra, u32 rb) override;
		void STWX(u32 rs, u32 ra, u32 rb) override;
		void STVEHX(u32 vs, u32 ra, u32 rb) override;
		void STDUX(u32 rs, u32 ra, u32 rb) override;
		void STWUX(u32 rs, u32 ra, u32 rb) override;
		void STVEWX(u32 vs, u32 ra, u32 rb) override;
		void SUBFZE(u32 rd, u32 ra, u32 oe, u32 rc) override;
		void ADDZE(u32 rd, u32 ra, u32 oe, u32 rc) override;
		void STDCX_(u32 rs, u32 ra, u32 rb) override;
		void STBX(u32 rs, u32 ra, u32 rb) override;
		void STVX(u32 vs, u32 ra, u32 rb) override;
		void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void SUBFME(u32 rd, u32 ra, u32 oe, u32 rc) override;
		void ADDME(u32 rd, u32 ra, u32 oe, u32 rc) override;
		void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void DCBTST(u32 ra, u32 rb, u32 th) override;
		void STBUX(u32 rs, u32 ra, u32 rb) override;
		void ADD(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void DCBT(u32 ra, u32 rb, u32 th) override;
		void LHZX(u32 rd, u32 ra, u32 rb) override;
		void EQV(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void ECIWX(u32 rd, u32 ra, u32 rb) override;
		void LHZUX(u32 rd, u32 ra, u32 rb) override;
		void XOR(u32 rs, u32 ra, u32 rb, u32 rc) override;
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
		void ORC(u32 rs, u32 ra, u32 rb, u32 rc) override;
		void ECOWX(u32 rs, u32 ra, u32 rb) override;
		void STHUX(u32 rs, u32 ra, u32 rb) override;
		void OR(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void MTSPR(u32 spr, u32 rs) override;
		void DCBI(u32 ra, u32 rb) override;
		void NAND(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void STVXL(u32 vs, u32 ra, u32 rb) override;
		void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, u32 rc) override;
		void LVLX(u32 vd, u32 ra, u32 rb) override;
		void LDBRX(u32 rd, u32 ra, u32 rb) override;
		void LSWX(u32 rd, u32 ra, u32 rb) override;
		void LWBRX(u32 rd, u32 ra, u32 rb) override;
		void LFSX(u32 frd, u32 ra, u32 rb) override;
		void SRW(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void SRD(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void LVRX(u32 vd, u32 ra, u32 rb) override;
		void LSWI(u32 rd, u32 ra, u32 nb) override;
		void LFSUX(u32 frd, u32 ra, u32 rb) override;
		void SYNC(u32 l) override;
		void LFDX(u32 frd, u32 ra, u32 rb) override;
		void LFDUX(u32 frd, u32 ra, u32 rb) override;
		void STVLX(u32 vs, u32 ra, u32 rb) override;
		void STDBRX(u32 rd, u32 ra, u32 rb) override;
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
		void SRAW(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void SRAD(u32 ra, u32 rs, u32 rb, u32 rc) override;
		void LVRXL(u32 vd, u32 ra, u32 rb) override;
		void DSS(u32 strm, u32 a) override;
		void SRAWI(u32 ra, u32 rs, u32 sh, u32 rc) override;
		void SRADI1(u32 ra, u32 rs, u32 sh, u32 rc) override;
		void SRADI2(u32 ra, u32 rs, u32 sh, u32 rc) override;
		void EIEIO() override;
		void STVLXL(u32 vs, u32 ra, u32 rb) override;
		void STHBRX(u32 rs, u32 ra, u32 rb) override;
		void EXTSH(u32 ra, u32 rs, u32 rc) override;
		void STVRXL(u32 sd, u32 ra, u32 rb) override;
		void EXTSB(u32 ra, u32 rs, u32 rc) override;
		void STFIWX(u32 frs, u32 ra, u32 rb) override;
		void EXTSW(u32 ra, u32 rs, u32 rc) override;
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
		void FDIVS(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FSUBS(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FADDS(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FSQRTS(u32 frd, u32 frb, u32 rc) override;
		void FRES(u32 frd, u32 frb, u32 rc) override;
		void FMULS(u32 frd, u32 fra, u32 frc, u32 rc) override;
		void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void STD(u32 rs, u32 ra, s32 ds) override;
		void STDU(u32 rs, u32 ra, s32 ds) override;
		void MTFSB1(u32 bt, u32 rc) override;
		void MCRFS(u32 bf, u32 bfa) override;
		void MTFSB0(u32 bt, u32 rc) override;
		void MTFSFI(u32 crfd, u32 i, u32 rc) override;
		void MFFS(u32 frd, u32 rc) override;
		void MTFSF(u32 flm, u32 frb, u32 rc) override;

		void FCMPU(u32 bf, u32 fra, u32 frb) override;
		void FRSP(u32 frd, u32 frb, u32 rc) override;
		void FCTIW(u32 frd, u32 frb, u32 rc) override;
		void FCTIWZ(u32 frd, u32 frb, u32 rc) override;
		void FDIV(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FSUB(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FADD(u32 frd, u32 fra, u32 frb, u32 rc) override;
		void FSQRT(u32 frd, u32 frb, u32 rc) override;
		void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FMUL(u32 frd, u32 fra, u32 frc, u32 rc) override;
		void FRSQRTE(u32 frd, u32 frb, u32 rc) override;
		void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, u32 rc) override;
		void FCMPO(u32 crfd, u32 fra, u32 frb) override;
		void FNEG(u32 frd, u32 frb, u32 rc) override;
		void FMR(u32 frd, u32 frb, u32 rc) override;
		void FNABS(u32 frd, u32 frb, u32 rc) override;
		void FABS(u32 frd, u32 frb, u32 rc) override;
		void FCTID(u32 frd, u32 frb, u32 rc) override;
		void FCTIDZ(u32 frd, u32 frb, u32 rc) override;
		void FCFID(u32 frd, u32 frb, u32 rc) override;

		void UNK(const u32 code, const u32 opcode, const u32 gcode) override;

		/// Utility function creating a function called name with Executable signature
		void initiate_function(const std::string &name);

	protected:
		/// State of a compilation task
		struct CompileTaskState {
			enum Args {
				State,
				Context,
				MaxArgs,
			};

			/// The LLVM function for the compilation task
			llvm::Function * function;

			/// Args of the LLVM function
			llvm::Value * args[MaxArgs];

			/// Address of the current instruction being compiled
			u32 current_instruction_address;

			/// A flag used to detect branch instructions.
			/// This is set to false at the start of compilation of an instruction.
			/// If a branch instruction is encountered, this is set to true by the decode function.
			bool hit_branch_instruction;
		};

		/// The function that will be called to execute unknown functions
		llvm::Function * m_execute_unknown_function;

		/// The executable that will be called to execute unknown blocks
		llvm::Function *  m_execute_unknown_block;

		/// Maps function name to executable memory pointer
		std::unordered_map<std::string, void*> &m_executable_map;

		/// LLVM context
		llvm::LLVMContext * m_llvm_context;

		/// LLVM IR builder
		llvm::IRBuilder<> * m_ir_builder;

		/// Module to which all generated code is output to
		llvm::Module * m_module;

		/// LLVM type of the functions genreated by the compiler
		llvm::FunctionType * m_compiled_function_type;

		/// State of the current compilation task
		CompileTaskState m_state;

		/// Get the name of the basic block for the specified address
		std::string GetBasicBlockNameFromAddress(u32 address, const std::string & suffix = "") const;

		/// Get the address of a basic block from its name
		u32 GetAddressFromBasicBlockName(const std::string & name) const;

		/// Get the basic block in for the specified address.
		llvm::BasicBlock * GetBasicBlockFromAddress(u32 address, const std::string & suffix = "", bool create_if_not_exist = true);

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

		/// Get VRSAVE
		llvm::Value * GetVrsave();

		/// Set VRSAVE
		void SetVrsave(llvm::Value * val_x64);

		/// Load FPSCR
		llvm::Value * GetFpscr();

		/// Set FPSCR
		void SetFpscr(llvm::Value * val_x32);

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
		void CreateBranch(llvm::Value * cmp_i1, llvm::Value * target_i32, bool lk, bool target_is_lr = false);

		/// Read from memory
		llvm::Value * ReadMemory(llvm::Value * addr_i64, u32 bits, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

		/// Write to memory
		void WriteMemory(llvm::Value * addr_i64, llvm::Value * val_ix, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

		/// Convert a C++ type to an LLVM type
		template<class T>
		llvm::Type * CppToLlvmType() {
			if (std::is_void<T>::value) {
				return m_ir_builder->getVoidTy();
			}
			else if (std::is_same<T, long long>::value || std::is_same<T, unsigned long long>::value) {
				return m_ir_builder->getInt64Ty();
			}
			else if (std::is_same<T, int>::value || std::is_same<T, unsigned int>::value) {
				return m_ir_builder->getInt32Ty();
			}
			else if (std::is_same<T, short>::value || std::is_same<T, unsigned short>::value) {
				return m_ir_builder->getInt16Ty();
			}
			else if (std::is_same<T, char>::value || std::is_same<T, unsigned char>::value) {
				return m_ir_builder->getInt8Ty();
			}
			else if (std::is_same<T, float>::value) {
				return m_ir_builder->getFloatTy();
			}
			else if (std::is_same<T, double>::value) {
				return m_ir_builder->getDoubleTy();
			}
			else if (std::is_same<T, bool>::value) {
				return m_ir_builder->getInt1Ty();
			}
			else if (std::is_pointer<T>::value) {
				return m_ir_builder->getInt8PtrTy();
			}
			else {
				assert(0);
			}

			return nullptr;
		}

		/// Call a function
		template<class ReturnType, class... Args>
		llvm::Value * Call(const char * name, Args... args) {
			auto fn = m_module->getFunction(name);
			if (!fn) {
				std::vector<llvm::Type *> fn_args_type = { args->getType()... };
				auto fn_type = llvm::FunctionType::get(CppToLlvmType<ReturnType>(), fn_args_type, false);
				fn = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, fn_type));
				fn->setCallingConv(llvm::CallingConv::X86_64_Win64);
				// Create an entry in m_executable_map that will be populated outside of compiler
				(void)m_executable_map[name];
			}

			std::vector<llvm::Value *> fn_args = { args... };
			return m_ir_builder->CreateCall(fn, fn_args);
		}

		/// Handle compilation errors
		void CompilationError(const std::string & error);

		/// A mask used in rotate instructions
		static u64 s_rotate_mask[64][64];

		/// A flag indicating whether s_rotate_mask has been initialised or not
		static bool s_rotate_mask_inited;

		/// Initialse s_rotate_mask
		static void InitRotateMask();
	};

	/**
	 * Manages block compilation.
	 * PPUInterpreter1 execution is traced (using Tracer class)
	 * Periodically RecompilationEngine process traces result to find blocks
	 * whose compilation can improve performances.
	 * It then builds them asynchroneously and update the executable mapping
	 * using atomic based locks to avoid undefined behavior.
	 **/
	class RecompilationEngine final : public named_thread_t {
		friend class CPUHybridDecoderRecompiler;
	public:
		virtual ~RecompilationEngine() override;

		/**
		 * Get the executable for the specified address if a compiled version is
		 * available, otherwise returns nullptr.
		 **/
		const Executable GetCompiledExecutableIfAvailable(u32 address) const;

		/// Notify the recompilation engine about a newly detected block start.
		void NotifyBlockStart(u32 address);

		/// Log
		llvm::raw_fd_ostream & Log();

		std::string get_name() const override { return "PPU Recompilation Engine"; }

		void on_task() override;

		/// Get a pointer to the instance of this class
		static std::shared_ptr<RecompilationEngine> GetInstance();

	private:
		/// An entry in the block table
		struct BlockEntry {
			/// Start address
			u32 address;

			/// Number of times this block was hit
			u32 num_hits;

			/// Indicates whether this function has been analysed or not
			bool is_analysed;

			/// Indicates whether the block has been compiled or not
			bool is_compiled;

			/// Indicate wheter the block is a function that can be completly compiled
			/// that is, that has a clear "return" semantic and no indirect branch
			bool is_compilable_function;

			/// If the analysis was successfull, how long the block is.
			u32 instructionCount;

			/// If the analysis was successfull, which function does it call.
			std::set<u32> calledFunctions;

			BlockEntry(u32 start_address)
				: num_hits(0)
				, address(start_address)
				, is_compiled(false)
				, is_analysed(false)
				, is_compilable_function(false)
				, instructionCount(0) {
			}

			std::string ToString() const {
				return fmt::format("0x%08X: NumHits=%u, IsCompiled=%c",
					address, num_hits, is_compiled ? 'Y' : 'N');
			}

			bool operator == (const BlockEntry & other) const {
				return address == other.address;
			}
		};

		/// Log
		llvm::raw_fd_ostream * m_log;

		/// Lock for accessing m_pending_address_start. TODO: Eliminate this and use a lock-free queue.
		std::mutex m_pending_address_start_lock;

		/// Queue of block start address to process
		std::list<u32> m_pending_address_start;

		/// Block table
		std::unordered_map<u32, BlockEntry> m_block_table;

		int m_currentId;

		/// (function, id).
		typedef std::pair<Executable, u32> ExecutableStorageType;

		/// Virtual memory allocated array.
		/// Store pointer to every compiled function/block and a unique Id.
		/// We need to map every instruction in PS3 Ram so it's a big table
		/// But a lot of it won't be accessed. Fortunatly virtual memory help here...
		ExecutableStorageType* FunctionCache;

		// Bitfield recording page status in FunctionCache reserved memory.
		char *FunctionCachePagesCommited;

		bool isAddressCommited(u32) const;
		void commitAddress(u32);

		/// vector storing all exec engine
		std::vector<std::unique_ptr<llvm::ExecutionEngine> > m_executable_storage;


		/// LLVM context
		llvm::LLVMContext &m_llvm_context;

		/// LLVM IR builder
		llvm::IRBuilder<> m_ir_builder;

		/**
		* Compile a code fragment described by a cfg and return an executable and the ExecutionEngine storing it
		* Pointer to function can be retrieved with getPointerToFunction
		*/
		std::pair<Executable, llvm::ExecutionEngine *> compile(const std::string & name, u32 start_address, u32 instruction_count);

		/// The time at which the m_address_to_ordinal cache was last cleared
		std::chrono::high_resolution_clock::time_point m_last_cache_clear_time;

		RecompilationEngine();

		RecompilationEngine(const RecompilationEngine&) = delete; // Delete copy/move constructors and copy/move operators

		/// Increase usage counter for block starting at addr and compile it if threshold was reached.
		/// Returns true if block was compiled
		bool IncreaseHitCounterAndBuild(u32 addr);

		/**
		* Analyse block to get useful info (function called, has indirect branch...)
		* This code is inspired from Dolphin PPC Analyst
		* Return true if analysis is successful.
		*/
		bool AnalyseBlock(BlockEntry &functionData, size_t maxSize = 10000);

		/// Compile a block
		void CompileBlock(BlockEntry & block_entry);

		/// Mutex used to prevent multiple creation
		static std::mutex s_mutex;

		/// The instance
		static std::shared_ptr<RecompilationEngine> s_the_instance;
	};

	/**
	 * PPU execution engine
	 * Relies on PPUInterpreter1 to execute uncompiled code.
	 * Traces execution to determine which block to compile.
	 * Use LLVM to compile block into native code.
	 */
	class CPUHybridDecoderRecompiler : public CPUDecoder {
		friend class RecompilationEngine;
		friend class Compiler;
	public:
		CPUHybridDecoderRecompiler(PPUThread & ppu);

		CPUHybridDecoderRecompiler(const CPUHybridDecoderRecompiler&) = delete; // Delete copy/move constructors and copy/move operators

		virtual ~CPUHybridDecoderRecompiler();

		u32 DecodeMemory(const u32 address) override;

	private:
		/// PPU processor context
		PPUThread & m_ppu;

		/// PPU Interpreter
		PPUInterpreter * m_interpreter;

		/// PPU instruction Decoder
		PPUDecoder m_decoder;

		/// Recompilation engine
		std::shared_ptr<RecompilationEngine> m_recompilation_engine;

		/// Execute a function
		static u32 ExecuteFunction(PPUThread * ppu_state, u64 context);

		/// Execute till the current function returns
		static u32 ExecuteTillReturn(PPUThread * ppu_state, u64 context);

		/// Check thread status. Returns true if the thread must exit.
		static bool PollStatus(PPUThread * ppu_state);
	};

	class CustomSectionMemoryManager : public llvm::SectionMemoryManager {
	private:
		std::unordered_map<std::string, void*> &executableMap;
	public:
		CustomSectionMemoryManager(std::unordered_map<std::string, void*> &map) :
			executableMap(map)
		{}
		~CustomSectionMemoryManager() override {}

		virtual uint64_t getSymbolAddress(const std::string &Name) override
		{
			std::unordered_map<std::string, void*>::const_iterator It = executableMap.find(Name);
			if (It != executableMap.end())
				return (uint64_t)It->second;
			return getSymbolAddressInProcess(Name);
		}
	};
}

#endif // LLVM_AVAILABLE
#endif // PPU_LLVM_RECOMPILER_H
