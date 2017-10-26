#pragma once

#include "SPURecompiler.h"

#include "Utilities/JIT.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

class spu_llvm_recompiler : public spu_recompiler_base
{
public:
	spu_llvm_recompiler(SPUThread &thread);

	virtual void compile(spu_function_t& f) override;
	
private:
	void compile_llvm(spu_function_t & f);
	llvm::Function* compile_function(spu_function_t& f);

	llvm::Value* GetInt128(__m128i n);
	llvm::Value* GetFloat128(__m128 n);
	llvm::Value* GetInt128(__m128i n, llvm::Type* type);
	llvm::Value* GetVec2(__m128i n);
	llvm::Value* GetVec2Double(__m128i n);
	llvm::Value* GetVec2Double(__m128d n);
	llvm::Value* GetVec4(__m128i n);
	llvm::Value* GetVec4Float(__m128i n);
	llvm::Value* GetVec4Float(__m128 n);
	llvm::Value* GetVec8(__m128i n);
	llvm::Value* GetVec16(__m128i n);
	llvm::Value* PtrPC();
	llvm::Value* PtrGPR8(u8 reg, llvm::Value* offset);
	llvm::Value* PtrGPR8(u8 reg, s32 offset);
	llvm::Value* PtrGPR16(u8 reg, llvm::Value* offset);
	llvm::Value* PtrGPR16(u8 reg, s32 offset);
	llvm::Value* PtrGPR32(u8 reg, llvm::Value* offset);
	llvm::Value* PtrGPR32(u8 reg, s32 offset);
	llvm::Value* PtrGPR64(u8 reg, llvm::Value* offset);
	llvm::Value* PtrGPR128(u8 reg);
	llvm::Value* PtrGPR32(u8 reg);
	llvm::Value* PtrLS(llvm::Value* ls_addr);
	llvm::Value* LoadLS(llvm::Value* ls_addr, llvm::Type* type);
	void StoreLS(llvm::Value* ls_addr, llvm::Value* value);
	llvm::Value* LoadGPR8(u8 reg, u32 offset = 0);
	llvm::Value* LoadGPR16(u8 reg, u32 offset = 0);
	llvm::Value* LoadGPR32(u8 reg, u32 offset = 0);
	llvm::Value* LoadGPR128(u8 reg);
	llvm::Value* LoadGPR128(u8 reg, llvm::Type* type);
	llvm::Value* LoadGPRVec2(u8 reg);
	llvm::Value* LoadGPRVec2Double(u8 reg);
	llvm::Value* LoadGPRVec4(u8 reg);
	llvm::Value* LoadGPRVec4Float(u8 reg);
	llvm::Value* LoadGPRVec8(u8 reg);
	llvm::Value* LoadGPRVec16(u8 reg);
	void Store128(llvm::Value* ptr, llvm::Value* value, int invariant_group = -1);
	void StoreGPR128(u8 reg, __m128i data);
	void StoreGPR128(u8 reg, llvm::Value* value);
	void StoreGPR64(u8 reg, llvm::Value* data, llvm::Value* offset);
	void StoreGPR64(u8 reg, llvm::Value* data, u32 offset);
	void StoreGPR64(u8 reg, u64 data, llvm::Value * offset);
	void StoreGPR64(u8 reg, u64 data, u32 offset);
	void StoreGPR32(u8 reg, llvm::Value* value, llvm::Value* offset);
	void StoreGPR32(u8 reg, llvm::Value* value, u32 offset);
	void StoreGPR32(u8 reg, u32 data, llvm::Value* offset);
	void StoreGPR16(u8 reg, llvm::Value* data, llvm::Value* offset);
	void StoreGPR16(u8 reg, u16 data, llvm::Value* offset);
	void StoreGPR8(u8 reg, llvm::Value* data, llvm::Value* offset);
	void StoreGPR32(u8 reg, u32 data, u32 offset);
	llvm::Value* Intrinsic_minnum(llvm::Value* v0, llvm::Value* v1);
	llvm::Value* Intrinsic_maxnum(llvm::Value* v0, llvm::Value* v1);
	llvm::Value* Intrinsic_sqrt(llvm::Value* v0);
	llvm::Value* Intrinsic_pshufb(llvm::Value* v0, llvm::Value* mask);
	llvm::Value* Intrinsic_pshufb(llvm::Value* v0, std::vector<u32> mask);
	llvm::Value* Intrinsic_rcpps(llvm::Value* v0);
	llvm::Value* ZExt128(llvm::Value* va);

	void CreateJumpTableSwitch(llvm::Value* _addr);
	void FunctionCall();
	void InterpreterCall(spu_opcode_t op);

	std::unique_ptr<spu_recompiler> m_asmjit_recompiler;

	std::string m_cache_path;
	std::unique_ptr<jit_compiler> m_jit;
	
	llvm::LLVMContext m_context;
	std::unique_ptr<llvm::Module> m_module;

	llvm::Function* m_spufunccall;
	llvm::Function* m_spuintcall;
	llvm::Function* m_llvm_func;

	llvm::IRBuilder<>* m_ir;
	llvm::BasicBlock* m_body;
	llvm::BasicBlock* m_end;

	llvm::Argument* m_cpu;
	llvm::Argument* m_ls;
	llvm::Value* m_addr;

	llvm::BasicBlock** m_blocks;

public:

	void STOP(spu_opcode_t op);
	void LNOP(spu_opcode_t op);
	void SYNC(spu_opcode_t op);
	void DSYNC(spu_opcode_t op);
	void MFSPR(spu_opcode_t op);
	void RDCH(spu_opcode_t op);
	void RCHCNT(spu_opcode_t op);
	void SF(spu_opcode_t op);
	void OR(spu_opcode_t op);
	void BG(spu_opcode_t op);
	void SFH(spu_opcode_t op);
	void NOR(spu_opcode_t op);		
	void ABSDB(spu_opcode_t op);
	void ROT(spu_opcode_t op);
	void ROTM(spu_opcode_t op);
	void ROTMA(spu_opcode_t op);
	void SHL(spu_opcode_t op);
	void ROTH(spu_opcode_t op);
	void ROTHM(spu_opcode_t op);
	void ROTMAH(spu_opcode_t op);
	void SHLH(spu_opcode_t op);
	void ROTI(spu_opcode_t op);
	void ROTMI(spu_opcode_t op);
	void ROTMAI(spu_opcode_t op);
	void SHLI(spu_opcode_t op);
	void ROTHI(spu_opcode_t op);
	void ROTHMI(spu_opcode_t op);
	void ROTMAHI(spu_opcode_t op);
	void SHLHI(spu_opcode_t op);
	void A(spu_opcode_t op);
	void AND(spu_opcode_t op);
	void CG(spu_opcode_t op);
	void AH(spu_opcode_t op);
	void NAND(spu_opcode_t op);
	void AVGB(spu_opcode_t op);
	void MTSPR(spu_opcode_t op);
	void WRCH(spu_opcode_t op);
	void BIZ(spu_opcode_t op);
	void BINZ(spu_opcode_t op);
	void BIHZ(spu_opcode_t op);
	void BIHNZ(spu_opcode_t op);
	void STOPD(spu_opcode_t op);
	void STQX(spu_opcode_t op);
	void BI(spu_opcode_t op);
	void BISL(spu_opcode_t op);
	void IRET(spu_opcode_t op);
	void BISLED(spu_opcode_t op);
	void HBR(spu_opcode_t op);
	void GB(spu_opcode_t op);
	void GBH(spu_opcode_t op);
	void GBB(spu_opcode_t op);
	void FSM(spu_opcode_t op);
	void FSMH(spu_opcode_t op);
	void FSMB(spu_opcode_t op);
	void FREST(spu_opcode_t op);
	void FRSQEST(spu_opcode_t op);
	void LQX(spu_opcode_t op);
	void ROTQBYBI(spu_opcode_t op);
	void ROTQMBYBI(spu_opcode_t op);
	void SHLQBYBI(spu_opcode_t op);
	void CBX(spu_opcode_t op);
	void CHX(spu_opcode_t op);
	void CWX(spu_opcode_t op);
	void CDX(spu_opcode_t op);
	void ROTQBI(spu_opcode_t op);
	void ROTQMBI(spu_opcode_t op);
	void SHLQBI(spu_opcode_t op);
	void ROTQBY(spu_opcode_t op);
	void ROTQMBY(spu_opcode_t op);
	void SHLQBY(spu_opcode_t op);
	void ORX(spu_opcode_t op);
	void CBD(spu_opcode_t op);
	void CHD(spu_opcode_t op);
	void CWD(spu_opcode_t op);
	void CDD(spu_opcode_t op);
	void ROTQBII(spu_opcode_t op);
	void ROTQMBII(spu_opcode_t op);
	void SHLQBII(spu_opcode_t op);
	void ROTQBYI(spu_opcode_t op);
	void ROTQMBYI(spu_opcode_t op);
	void SHLQBYI(spu_opcode_t op);
	void NOP(spu_opcode_t op);
	void CGT(spu_opcode_t op);
	void XOR(spu_opcode_t op);
	void CGTH(spu_opcode_t op);
	void EQV(spu_opcode_t op);
	void CGTB(spu_opcode_t op);
	void SUMB(spu_opcode_t op);
	void HGT(spu_opcode_t op);
	void CLZ(spu_opcode_t op);
	void XSWD(spu_opcode_t op);
	void XSHW(spu_opcode_t op);
	void CNTB(spu_opcode_t op);
	void XSBH(spu_opcode_t op);
	void CLGT(spu_opcode_t op);
	void ANDC(spu_opcode_t op);
	void FCGT(spu_opcode_t op);
	void DFCGT(spu_opcode_t op);
	void FA(spu_opcode_t op);
	void FS(spu_opcode_t op);
	void FM(spu_opcode_t op);
	void CLGTH(spu_opcode_t op);
	void ORC(spu_opcode_t op);
	void FCMGT(spu_opcode_t op);
	void DFCMGT(spu_opcode_t op);
	void DFA(spu_opcode_t op);
	void DFS(spu_opcode_t op);
	void DFM(spu_opcode_t op);
	void CLGTB(spu_opcode_t op);
	void HLGT(spu_opcode_t op);
	void DFMA(spu_opcode_t op);
	void DFMS(spu_opcode_t op);
	void DFNMS(spu_opcode_t op);
	void DFNMA(spu_opcode_t op);
	void CEQ(spu_opcode_t op);
	void MPYHHU(spu_opcode_t op);
	void ADDX(spu_opcode_t op);
	void SFX(spu_opcode_t op);
	void CGX(spu_opcode_t op);
	void BGX(spu_opcode_t op);
	void MPYHHA(spu_opcode_t op);
	void MPYHHAU(spu_opcode_t op);
	void FSCRRD(spu_opcode_t op);
	void FESD(spu_opcode_t op);
	void FRDS(spu_opcode_t op);
	void FSCRWR(spu_opcode_t op);
	void DFTSV(spu_opcode_t op);
	void FCEQ(spu_opcode_t op);
	void DFCEQ(spu_opcode_t op);
	void MPY(spu_opcode_t op);
	void MPYH(spu_opcode_t op);
	void MPYHH(spu_opcode_t op);
	void MPYS(spu_opcode_t op);
	void CEQH(spu_opcode_t op);
	void FCMEQ(spu_opcode_t op);
	void DFCMEQ(spu_opcode_t op);
	void MPYU(spu_opcode_t op);
	void CEQB(spu_opcode_t op);
	void FI(spu_opcode_t op);
	void HEQ(spu_opcode_t op);
	void CFLTS(spu_opcode_t op);
	void CFLTU(spu_opcode_t op);
	void CSFLT(spu_opcode_t op);
	void CUFLT(spu_opcode_t op);
	void BRZ(spu_opcode_t op);
	void STQA(spu_opcode_t op);
	void BRNZ(spu_opcode_t op);
	void BRHZ(spu_opcode_t op);
	void BRHNZ(spu_opcode_t op);
	void STQR(spu_opcode_t op);
	void BRA(spu_opcode_t op);
	void LQA(spu_opcode_t op);
	void BRASL(spu_opcode_t op);
	void BR(spu_opcode_t op);
	void FSMBI(spu_opcode_t op);
	void BRSL(spu_opcode_t op);
	void LQR(spu_opcode_t op);
	void IL(spu_opcode_t op);
	void ILHU(spu_opcode_t op);
	void ILH(spu_opcode_t op);
	void IOHL(spu_opcode_t op);
	void ORI(spu_opcode_t op);
	void ORHI(spu_opcode_t op);
	void ORBI(spu_opcode_t op);
	void SFI(spu_opcode_t op);
	void SFHI(spu_opcode_t op);
	void ANDI(spu_opcode_t op);
	void ANDHI(spu_opcode_t op);
	void ANDBI(spu_opcode_t op);
	void AI(spu_opcode_t op);
	void AHI(spu_opcode_t op);
	void STQD(spu_opcode_t op);
	void LQD(spu_opcode_t op);
	void XORI(spu_opcode_t op);
	void XORHI(spu_opcode_t op);
	void XORBI(spu_opcode_t op);
	void CGTI(spu_opcode_t op);
	void CGTHI(spu_opcode_t op);
	void CGTBI(spu_opcode_t op);
	void HGTI(spu_opcode_t op);
	void CLGTI(spu_opcode_t op);
	void CLGTHI(spu_opcode_t op);
	void CLGTBI(spu_opcode_t op);
	void HLGTI(spu_opcode_t op);
	void MPYI(spu_opcode_t op);
	void MPYUI(spu_opcode_t op);
	void CEQI(spu_opcode_t op);
	void CEQHI(spu_opcode_t op);
	void CEQBI(spu_opcode_t op);
	void HEQI(spu_opcode_t op);
	void HBRA(spu_opcode_t op);
	void HBRR(spu_opcode_t op);
	void ILA(spu_opcode_t op);
	void SELB(spu_opcode_t op);
	void SHUFB(spu_opcode_t op);
	void MPYA(spu_opcode_t op);
	void FNMS(spu_opcode_t op);
	void FMA(spu_opcode_t op);
	void FMS(spu_opcode_t op);

	void UNK(spu_opcode_t op);
};