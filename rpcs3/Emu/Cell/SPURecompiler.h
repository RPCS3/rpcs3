#pragma once
#include "Emu/CPU/CPUDecoder.h"
#include "Emu/Cell/SPUOpcodes.h"

namespace asmjit
{
	struct JitRuntime;
	struct X86Compiler;
	struct X86GpVar;
	struct X86XmmVar;
	struct X86Mem;
}

class SPURecompiler;
class SPUInterpreter;

class SPURecompilerCore : public CPUDecoder
{
	std::unique_ptr<SPURecompiler> m_enc;
	std::unique_ptr<SPUInterpreter> m_int;
	std::unique_ptr<asmjit::JitRuntime> m_jit;
	SPUThread& CPU;

public:
	bool first = true;
	bool need_check = false;

	struct SPURecEntry
	{
		u32 count; // count of instructions compiled from current point (and to be checked)
		u32 valid; // copy of valid opcode for validation
		void* pointer; // pointer to executable memory object
	};

	std::array<SPURecEntry, 0x10000> entry = {};

	std::vector<u128> imm_table;

	SPURecompilerCore(SPUThread& cpu);

	void Compile(u16 pos);

	virtual void Decode(const u32 code);

	virtual u32 DecodeMemory(const u32 address);
};

class SPURecompiler : public SPUOpcodes
{
private:
	SPUThread& CPU;
	SPURecompilerCore& rec;

public:
	asmjit::X86Compiler* compiler;
	bool do_finalize;
	// input:
	asmjit::X86GpVar* cpu_var;
	asmjit::X86GpVar* ls_var;
	asmjit::X86GpVar* imm_var;
	asmjit::X86GpVar* g_imm_var;
	// output:
	asmjit::X86GpVar* pos_var;
	// temporary:
	asmjit::X86GpVar* addr;
	asmjit::X86GpVar* qw0;
	asmjit::X86GpVar* qw1;
	asmjit::X86GpVar* qw2;

	struct XmmLink
	{
		asmjit::X86XmmVar* data;
		s8 reg;
		bool taken;
		mutable bool got;
		mutable u32 access;

		XmmLink()
			: data(nullptr)
			, reg(-1)
			, taken(false)
			, got(false)
			, access(0)
		{
		}

		const asmjit::X86XmmVar& get() const
		{
			assert(data);
			assert(taken);
			if (!taken) throw "XmmLink::get(): wrong use";
			got = true;
			return *data;
		}

		const asmjit::X86XmmVar& read() const
		{
			assert(data);
			return *data;
		}
	}
	xmm_var[16];

	SPURecompiler(SPUThread& cpu, SPURecompilerCore& rec)
		: CPU(cpu)
		, rec(rec)
		, compiler(nullptr)
	{
	}

	const XmmLink& XmmAlloc(s8 pref = -1);
	const XmmLink* XmmRead(const s8 reg) const;
	const XmmLink& XmmGet(s8 reg, s8 target = -1);
	const XmmLink& XmmCopy(const XmmLink& from, s8 pref = -1);
	void XmmInvalidate(const s8 reg);
	void XmmFinalize(const XmmLink& var, s8 reg = -1);
	void XmmRelease();
	asmjit::X86Mem XmmConst(u128 data);

private:

	//0 - 10
	virtual void STOP(u32 code) override;
	virtual void LNOP() override;
	virtual void SYNC(u32 Cbit) override;
	virtual void DSYNC() override;
	virtual void MFSPR(u32  rt, u32  sa) override;
	virtual void RDCH(u32  rt, u32  ra) override;
	virtual void RCHCNT(u32  rt, u32  ra) override;
	virtual void SF(u32  rt, u32  ra, u32  rb) override;
	virtual void OR(u32  rt, u32  ra, u32  rb) override;
	virtual void BG(u32  rt, u32  ra, u32  rb) override;
	virtual void SFH(u32  rt, u32  ra, u32  rb) override;
	virtual void NOR(u32  rt, u32  ra, u32  rb) override;
	virtual void ABSDB(u32  rt, u32  ra, u32  rb) override;
	virtual void ROT(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTM(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTMA(u32  rt, u32  ra, u32  rb) override;
	virtual void SHL(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTH(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTHM(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTMAH(u32  rt, u32  ra, u32  rb) override;
	virtual void SHLH(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTMI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTMAI(u32  rt, u32  ra, s32 i7) override;
	virtual void SHLI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTHI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTHMI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTMAHI(u32  rt, u32  ra, s32 i7) override;
	virtual void SHLHI(u32  rt, u32  ra, s32 i7) override;
	virtual void A(u32  rt, u32  ra, u32  rb) override;
	virtual void AND(u32  rt, u32  ra, u32  rb) override;
	virtual void CG(u32  rt, u32  ra, u32  rb) override;
	virtual void AH(u32  rt, u32  ra, u32  rb) override;
	virtual void NAND(u32  rt, u32  ra, u32  rb) override;
	virtual void AVGB(u32  rt, u32  ra, u32  rb) override;
	virtual void MTSPR(u32  rt, u32  sa) override;
	virtual void WRCH(u32  ra, u32  rt) override;
	virtual void BIZ(u32 intr, u32  rt, u32  ra) override;
	virtual void BINZ(u32 intr, u32  rt, u32  ra) override;
	virtual void BIHZ(u32 intr, u32  rt, u32  ra) override;
	virtual void BIHNZ(u32 intr, u32  rt, u32  ra) override;
	virtual void STOPD(u32  rc, u32  ra, u32  rb) override;
	virtual void STQX(u32  rt, u32  ra, u32  rb) override;
	virtual void BI(u32 intr, u32  ra) override;
	virtual void BISL(u32 intr, u32  rt, u32  ra) override;
	virtual void IRET(u32  ra) override;
	virtual void BISLED(u32 intr, u32  rt, u32  ra) override;
	virtual void HBR(u32  p, u32  ro, u32  ra) override;
	virtual void GB(u32  rt, u32  ra) override;
	virtual void GBH(u32  rt, u32  ra) override;
	virtual void GBB(u32  rt, u32  ra) override;
	virtual void FSM(u32  rt, u32  ra) override;
	virtual void FSMH(u32  rt, u32  ra) override;
	virtual void FSMB(u32  rt, u32  ra) override;
	virtual void FREST(u32  rt, u32  ra) override;
	virtual void FRSQEST(u32  rt, u32  ra) override;
	virtual void LQX(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQBYBI(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQMBYBI(u32  rt, u32  ra, u32  rb) override;
	virtual void SHLQBYBI(u32  rt, u32  ra, u32  rb) override;
	virtual void CBX(u32  rt, u32  ra, u32  rb) override;
	virtual void CHX(u32  rt, u32  ra, u32  rb) override;
	virtual void CWX(u32  rt, u32  ra, u32  rb) override;
	virtual void CDX(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQBI(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQMBI(u32  rt, u32  ra, u32  rb) override;
	virtual void SHLQBI(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQBY(u32  rt, u32  ra, u32  rb) override;
	virtual void ROTQMBY(u32  rt, u32  ra, u32  rb) override;
	virtual void SHLQBY(u32  rt, u32  ra, u32  rb) override;
	virtual void ORX(u32  rt, u32  ra) override;
	virtual void CBD(u32  rt, u32  ra, s32 i7) override;
	virtual void CHD(u32  rt, u32  ra, s32 i7) override;
	virtual void CWD(u32  rt, u32  ra, s32 i7) override;
	virtual void CDD(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTQBII(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTQMBII(u32  rt, u32  ra, s32 i7) override;
	virtual void SHLQBII(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTQBYI(u32  rt, u32  ra, s32 i7) override;
	virtual void ROTQMBYI(u32  rt, u32  ra, s32 i7) override;
	virtual void SHLQBYI(u32  rt, u32  ra, s32 i7) override;
	virtual void NOP(u32  rt) override;
	virtual void CGT(u32  rt, u32  ra, u32  rb) override;
	virtual void XOR(u32  rt, u32  ra, u32  rb) override;
	virtual void CGTH(u32  rt, u32  ra, u32  rb) override;
	virtual void EQV(u32  rt, u32  ra, u32  rb) override;
	virtual void CGTB(u32  rt, u32  ra, u32  rb) override;
	virtual void SUMB(u32  rt, u32  ra, u32  rb) override;
	virtual void HGT(u32  rt, s32  ra, s32  rb) override;
	virtual void CLZ(u32  rt, u32  ra) override;
	virtual void XSWD(u32  rt, u32  ra) override;
	virtual void XSHW(u32  rt, u32  ra) override;
	virtual void CNTB(u32  rt, u32  ra) override;
	virtual void XSBH(u32  rt, u32  ra) override;
	virtual void CLGT(u32  rt, u32  ra, u32  rb) override;
	virtual void ANDC(u32  rt, u32  ra, u32  rb) override;
	virtual void FCGT(u32  rt, u32  ra, u32  rb) override;
	virtual void DFCGT(u32  rt, u32  ra, u32  rb) override;
	virtual void FA(u32  rt, u32  ra, u32  rb) override;
	virtual void FS(u32  rt, u32  ra, u32  rb) override;
	virtual void FM(u32  rt, u32  ra, u32  rb) override;
	virtual void CLGTH(u32  rt, u32  ra, u32  rb) override;
	virtual void ORC(u32  rt, u32  ra, u32  rb) override;
	virtual void FCMGT(u32  rt, u32  ra, u32  rb) override;
	virtual void DFCMGT(u32  rt, u32  ra, u32  rb) override;
	virtual void DFA(u32  rt, u32  ra, u32  rb) override;
	virtual void DFS(u32  rt, u32  ra, u32  rb) override;
	virtual void DFM(u32  rt, u32  ra, u32  rb) override;
	virtual void CLGTB(u32  rt, u32  ra, u32  rb) override;
	virtual void HLGT(u32  rt, u32  ra, u32  rb) override;
	virtual void DFMA(u32  rt, u32  ra, u32  rb) override;
	virtual void DFMS(u32  rt, u32  ra, u32  rb) override;
	virtual void DFNMS(u32  rt, u32  ra, u32  rb) override;
	virtual void DFNMA(u32  rt, u32  ra, u32  rb) override;
	virtual void CEQ(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYHHU(u32  rt, u32  ra, u32  rb) override;
	virtual void ADDX(u32  rt, u32  ra, u32  rb) override;
	virtual void SFX(u32  rt, u32  ra, u32  rb) override;
	virtual void CGX(u32  rt, u32  ra, u32  rb) override;
	virtual void BGX(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYHHA(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYHHAU(u32  rt, u32  ra, u32  rb) override;
	virtual void FSCRRD(u32  rt) override;
	virtual void FESD(u32  rt, u32  ra) override;
	virtual void FRDS(u32  rt, u32  ra) override;
	virtual void FSCRWR(u32  rt, u32  ra) override;
	virtual void DFTSV(u32  rt, u32  ra, s32 i7) override;
	virtual void FCEQ(u32  rt, u32  ra, u32  rb) override;
	virtual void DFCEQ(u32  rt, u32  ra, u32  rb) override;
	virtual void MPY(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYH(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYHH(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYS(u32  rt, u32  ra, u32  rb) override;
	virtual void CEQH(u32  rt, u32  ra, u32  rb) override;
	virtual void FCMEQ(u32  rt, u32  ra, u32  rb) override;
	virtual void DFCMEQ(u32  rt, u32  ra, u32  rb) override;
	virtual void MPYU(u32  rt, u32  ra, u32  rb) override;
	virtual void CEQB(u32  rt, u32  ra, u32  rb) override;
	virtual void FI(u32  rt, u32  ra, u32  rb) override;
	virtual void HEQ(u32  rt, u32  ra, u32  rb) override;

	//0 - 9
	virtual void CFLTS(u32  rt, u32  ra, s32 i8) override;
	virtual void CFLTU(u32  rt, u32  ra, s32 i8) override;
	virtual void CSFLT(u32  rt, u32  ra, s32 i8) override;
	virtual void CUFLT(u32  rt, u32  ra, s32 i8) override;

	//0 - 8
	virtual void BRZ(u32  rt, s32 i16) override;
	virtual void STQA(u32  rt, s32 i16) override;
	virtual void BRNZ(u32  rt, s32 i16) override;
	virtual void BRHZ(u32  rt, s32 i16) override;
	virtual void BRHNZ(u32  rt, s32 i16) override;
	virtual void STQR(u32  rt, s32 i16) override;
	virtual void BRA(s32 i16) override;
	virtual void LQA(u32  rt, s32 i16) override;
	virtual void BRASL(u32  rt, s32 i16) override;
	virtual void BR(s32 i16) override;
	virtual void FSMBI(u32  rt, s32 i16) override;
	virtual void BRSL(u32  rt, s32 i16) override;
	virtual void LQR(u32  rt, s32 i16) override;
	virtual void IL(u32  rt, s32 i16) override;
	virtual void ILHU(u32  rt, s32 i16) override;
	virtual void ILH(u32  rt, s32 i16) override;
	virtual void IOHL(u32  rt, s32 i16) override;

	//0 - 7
	virtual void ORI(u32  rt, u32  ra, s32 i10) override;
	virtual void ORHI(u32  rt, u32  ra, s32 i10) override;
	virtual void ORBI(u32  rt, u32  ra, s32 i10) override;
	virtual void SFI(u32  rt, u32  ra, s32 i10) override;
	virtual void SFHI(u32  rt, u32  ra, s32 i10) override;
	virtual void ANDI(u32  rt, u32  ra, s32 i10) override;
	virtual void ANDHI(u32  rt, u32  ra, s32 i10) override;
	virtual void ANDBI(u32  rt, u32  ra, s32 i10) override;
	virtual void AI(u32  rt, u32  ra, s32 i10) override;
	virtual void AHI(u32  rt, u32  ra, s32 i10) override;
	virtual void STQD(u32  rt, s32 i10, u32  ra) override;
	virtual void LQD(u32  rt, s32 i10, u32  ra) override;
	virtual void XORI(u32  rt, u32  ra, s32 i10) override;
	virtual void XORHI(u32  rt, u32  ra, s32 i10) override;
	virtual void XORBI(u32  rt, u32  ra, s32 i10) override;
	virtual void CGTI(u32  rt, u32  ra, s32 i10) override;
	virtual void CGTHI(u32  rt, u32  ra, s32 i10) override;
	virtual void CGTBI(u32  rt, u32  ra, s32 i10) override;
	virtual void HGTI(u32  rt, u32  ra, s32 i10) override;
	virtual void CLGTI(u32  rt, u32  ra, s32 i10) override;
	virtual void CLGTHI(u32  rt, u32  ra, s32 i10) override;
	virtual void CLGTBI(u32  rt, u32  ra, s32 i10) override;
	virtual void HLGTI(u32  rt, u32  ra, s32 i10) override;
	virtual void MPYI(u32  rt, u32  ra, s32 i10) override;
	virtual void MPYUI(u32  rt, u32  ra, s32 i10) override;
	virtual void CEQI(u32  rt, u32  ra, s32 i10) override;
	virtual void CEQHI(u32  rt, u32  ra, s32 i10) override;
	virtual void CEQBI(u32  rt, u32  ra, s32 i10) override;
	virtual void HEQI(u32  rt, u32  ra, s32 i10) override;

	//0 - 6
	virtual void HBRA(s32 ro, s32 i16) override;
	virtual void HBRR(s32 ro, s32 i16) override;
	virtual void ILA(u32  rt, u32 i18) override;

	//0 - 3
	virtual void SELB(u32  rc, u32  ra, u32  rb, u32  rt) override;
	virtual void SHUFB(u32  rc, u32  ra, u32  rb, u32  rt) override;
	virtual void MPYA(u32  rc, u32  ra, u32  rb, u32  rt) override;
	virtual void FNMS(u32  rc, u32  ra, u32  rb, u32  rt) override;
	virtual void FMA(u32  rc, u32  ra, u32  rb, u32  rt) override;
	virtual void FMS(u32  rc, u32  ra, u32  rb, u32  rt) override;

	virtual void UNK(u32 code, u32 opcode, u32 gcode) override;
	virtual void UNK(const std::string& error);
};
