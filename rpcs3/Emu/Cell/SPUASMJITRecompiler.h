#pragma once

#include "SPURecompiler.h"

namespace asmjit
{
	struct JitRuntime;
	struct CodeHolder;
	struct X86Compiler;
	struct X86Gp;
	struct X86Xmm;
	struct X86Mem;
	struct Label;
}

// SPU ASMJIT Recompiler
class spu_recompiler : public spu_recompiler_base
{
	const std::shared_ptr<asmjit::JitRuntime> m_jit;

public:
	spu_recompiler();

	virtual void compile(spu_function_t& f) override;

private:
	// emitter:
	asmjit::X86Compiler* c;
	asmjit::CodeHolder* codeHolder;

	// input:
	asmjit::X86Gp* cpu;
	asmjit::X86Gp* ls;

	// temporary:
	asmjit::X86Gp* addr;
	asmjit::X86Gp* qw0;
	asmjit::X86Gp* qw1;
	asmjit::X86Gp* qw2;
	std::array<asmjit::X86Xmm*, 6> vec;

	// labels:
	asmjit::Label* labels; // array[0x10000]
	asmjit::Label* jt; // jump table resolver (uses *addr)
	asmjit::Label* end; // function end (return *addr)

	class XmmLink
	{
		asmjit::X86Xmm* m_var;

	public:
		XmmLink(asmjit::X86Xmm*& xmm_var)
			: m_var(xmm_var)
		{
			xmm_var = nullptr;
		}

		XmmLink(XmmLink&&) = default; // MoveConstructible + delete copy constructor and copy/move operators

		operator asmjit::X86Xmm&() const
		{
			return *m_var;
		}
	};

	enum class XmmType
	{
		Int,
		Float,
		Double,
	};

	XmmLink XmmAlloc();
	XmmLink XmmGet(s8 reg, XmmType type);

	asmjit::X86Mem XmmConst(v128 data);
	asmjit::X86Mem XmmConst(__m128 data);
	asmjit::X86Mem XmmConst(__m128i data);

public:
	void CheckInterruptStatus(spu_opcode_t op);
	void InterpreterCall(spu_opcode_t op);
	void FunctionCall();

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
