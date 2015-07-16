#pragma once

class SPUThread;

union spu_opcode_t
{
	u32 opcode;

	struct
	{
		u32 rt : 7; // 25..31, it's actually RC in 4-op instructions
		u32 ra : 7; // 18..24
		u32 rb : 7; // 11..17
		u32 rc : 7; // 4..10, it's actually RT in 4-op instructions
	};

	struct
	{
		u32    : 14; // 18..31
		u32 i7 : 7;  // 11..17
	};

	struct
	{
		u32    : 14; // 18..31
		u32 i8 : 8;  // 10..17
	};

	struct
	{
		u32     : 14; // 18..31
		u32 i10 : 10; // 8..17
	};

	struct
	{
		u32     : 7; // 25..31
		u32 i16 : 16; // 9..24
	};

	struct
	{
		u32     : 7; // 25..31
		u32 i18 : 18; // 7..24
	};

	struct
	{
		s32     : 14; // 18..31
		s32 si7 : 7;  // 11..17
	};

	struct
	{
		s32     : 14; // 18..31
		s32 si8 : 8;  // 10..17
	};

	struct
	{
		s32      : 14; // 18..31
		s32 si10 : 10; // 8..17
	};

	struct
	{
		s32      : 7; // 25..31
		s32 si16 : 16; // 9..24
	};

	struct
	{
		s32      : 7; // 25..31
		s32 si18 : 18; // 7..24
	};

	struct
	{
		u32   : 18; // 14..31
		u32 e : 1; // 13, "enable interrupts" bit
		u32 d : 1; // 12, "disable interrupts" bit
	};
};

using spu_inter_func_t = void(*)(SPUThread& CPU, spu_opcode_t opcode);

namespace spu_interpreter
{
	void DEFAULT(SPUThread& CPU, spu_opcode_t op);

	void set_interrupt_status(SPUThread& CPU, spu_opcode_t op);

	void STOP(SPUThread& CPU, spu_opcode_t op);
	void LNOP(SPUThread& CPU, spu_opcode_t op);
	void SYNC(SPUThread& CPU, spu_opcode_t op);
	void DSYNC(SPUThread& CPU, spu_opcode_t op);
	void MFSPR(SPUThread& CPU, spu_opcode_t op);
	void RDCH(SPUThread& CPU, spu_opcode_t op);
	void RCHCNT(SPUThread& CPU, spu_opcode_t op);
	void SF(SPUThread& CPU, spu_opcode_t op);
	void OR(SPUThread& CPU, spu_opcode_t op);
	void BG(SPUThread& CPU, spu_opcode_t op);
	void SFH(SPUThread& CPU, spu_opcode_t op);
	void NOR(SPUThread& CPU, spu_opcode_t op);
	void ABSDB(SPUThread& CPU, spu_opcode_t op);
	void ROT(SPUThread& CPU, spu_opcode_t op);
	void ROTM(SPUThread& CPU, spu_opcode_t op);
	void ROTMA(SPUThread& CPU, spu_opcode_t op);
	void SHL(SPUThread& CPU, spu_opcode_t op);
	void ROTH(SPUThread& CPU, spu_opcode_t op);
	void ROTHM(SPUThread& CPU, spu_opcode_t op);
	void ROTMAH(SPUThread& CPU, spu_opcode_t op);
	void SHLH(SPUThread& CPU, spu_opcode_t op);
	void ROTI(SPUThread& CPU, spu_opcode_t op);
	void ROTMI(SPUThread& CPU, spu_opcode_t op);
	void ROTMAI(SPUThread& CPU, spu_opcode_t op);
	void SHLI(SPUThread& CPU, spu_opcode_t op);
	void ROTHI(SPUThread& CPU, spu_opcode_t op);
	void ROTHMI(SPUThread& CPU, spu_opcode_t op);
	void ROTMAHI(SPUThread& CPU, spu_opcode_t op);
	void SHLHI(SPUThread& CPU, spu_opcode_t op);
	void A(SPUThread& CPU, spu_opcode_t op);
	void AND(SPUThread& CPU, spu_opcode_t op);
	void CG(SPUThread& CPU, spu_opcode_t op);
	void AH(SPUThread& CPU, spu_opcode_t op);
	void NAND(SPUThread& CPU, spu_opcode_t op);
	void AVGB(SPUThread& CPU, spu_opcode_t op);
	void MTSPR(SPUThread& CPU, spu_opcode_t op);
	void WRCH(SPUThread& CPU, spu_opcode_t op);
	void BIZ(SPUThread& CPU, spu_opcode_t op);
	void BINZ(SPUThread& CPU, spu_opcode_t op);
	void BIHZ(SPUThread& CPU, spu_opcode_t op);
	void BIHNZ(SPUThread& CPU, spu_opcode_t op);
	void STOPD(SPUThread& CPU, spu_opcode_t op);
	void STQX(SPUThread& CPU, spu_opcode_t op);
	void BI(SPUThread& CPU, spu_opcode_t op);
	void BISL(SPUThread& CPU, spu_opcode_t op);
	void IRET(SPUThread& CPU, spu_opcode_t op);
	void BISLED(SPUThread& CPU, spu_opcode_t op);
	void HBR(SPUThread& CPU, spu_opcode_t op);
	void GB(SPUThread& CPU, spu_opcode_t op);
	void GBH(SPUThread& CPU, spu_opcode_t op);
	void GBB(SPUThread& CPU, spu_opcode_t op);
	void FSM(SPUThread& CPU, spu_opcode_t op);
	void FSMH(SPUThread& CPU, spu_opcode_t op);
	void FSMB(SPUThread& CPU, spu_opcode_t op);
	void FREST(SPUThread& CPU, spu_opcode_t op);
	void FRSQEST(SPUThread& CPU, spu_opcode_t op);
	void LQX(SPUThread& CPU, spu_opcode_t op);
	void ROTQBYBI(SPUThread& CPU, spu_opcode_t op);
	void ROTQMBYBI(SPUThread& CPU, spu_opcode_t op);
	void SHLQBYBI(SPUThread& CPU, spu_opcode_t op);
	void CBX(SPUThread& CPU, spu_opcode_t op);
	void CHX(SPUThread& CPU, spu_opcode_t op);
	void CWX(SPUThread& CPU, spu_opcode_t op);
	void CDX(SPUThread& CPU, spu_opcode_t op);
	void ROTQBI(SPUThread& CPU, spu_opcode_t op);
	void ROTQMBI(SPUThread& CPU, spu_opcode_t op);
	void SHLQBI(SPUThread& CPU, spu_opcode_t op);
	void ROTQBY(SPUThread& CPU, spu_opcode_t op);
	void ROTQMBY(SPUThread& CPU, spu_opcode_t op);
	void SHLQBY(SPUThread& CPU, spu_opcode_t op);
	void ORX(SPUThread& CPU, spu_opcode_t op);
	void CBD(SPUThread& CPU, spu_opcode_t op);
	void CHD(SPUThread& CPU, spu_opcode_t op);
	void CWD(SPUThread& CPU, spu_opcode_t op);
	void CDD(SPUThread& CPU, spu_opcode_t op);
	void ROTQBII(SPUThread& CPU, spu_opcode_t op);
	void ROTQMBII(SPUThread& CPU, spu_opcode_t op);
	void SHLQBII(SPUThread& CPU, spu_opcode_t op);
	void ROTQBYI(SPUThread& CPU, spu_opcode_t op);
	void ROTQMBYI(SPUThread& CPU, spu_opcode_t op);
	void SHLQBYI(SPUThread& CPU, spu_opcode_t op);
	void NOP(SPUThread& CPU, spu_opcode_t op);
	void CGT(SPUThread& CPU, spu_opcode_t op);
	void XOR(SPUThread& CPU, spu_opcode_t op);
	void CGTH(SPUThread& CPU, spu_opcode_t op);
	void EQV(SPUThread& CPU, spu_opcode_t op);
	void CGTB(SPUThread& CPU, spu_opcode_t op);
	void SUMB(SPUThread& CPU, spu_opcode_t op);
	void HGT(SPUThread& CPU, spu_opcode_t op);
	void CLZ(SPUThread& CPU, spu_opcode_t op);
	void XSWD(SPUThread& CPU, spu_opcode_t op);
	void XSHW(SPUThread& CPU, spu_opcode_t op);
	void CNTB(SPUThread& CPU, spu_opcode_t op);
	void XSBH(SPUThread& CPU, spu_opcode_t op);
	void CLGT(SPUThread& CPU, spu_opcode_t op);
	void ANDC(SPUThread& CPU, spu_opcode_t op);
	void FCGT(SPUThread& CPU, spu_opcode_t op);
	void DFCGT(SPUThread& CPU, spu_opcode_t op);
	void FA(SPUThread& CPU, spu_opcode_t op);
	void FS(SPUThread& CPU, spu_opcode_t op);
	void FM(SPUThread& CPU, spu_opcode_t op);
	void CLGTH(SPUThread& CPU, spu_opcode_t op);
	void ORC(SPUThread& CPU, spu_opcode_t op);
	void FCMGT(SPUThread& CPU, spu_opcode_t op);
	void DFCMGT(SPUThread& CPU, spu_opcode_t op);
	void DFA(SPUThread& CPU, spu_opcode_t op);
	void DFS(SPUThread& CPU, spu_opcode_t op);
	void DFM(SPUThread& CPU, spu_opcode_t op);
	void CLGTB(SPUThread& CPU, spu_opcode_t op);
	void HLGT(SPUThread& CPU, spu_opcode_t op);
	void DFMA(SPUThread& CPU, spu_opcode_t op);
	void DFMS(SPUThread& CPU, spu_opcode_t op);
	void DFNMS(SPUThread& CPU, spu_opcode_t op);
	void DFNMA(SPUThread& CPU, spu_opcode_t op);
	void CEQ(SPUThread& CPU, spu_opcode_t op);
	void MPYHHU(SPUThread& CPU, spu_opcode_t op);
	void ADDX(SPUThread& CPU, spu_opcode_t op);
	void SFX(SPUThread& CPU, spu_opcode_t op);
	void CGX(SPUThread& CPU, spu_opcode_t op);
	void BGX(SPUThread& CPU, spu_opcode_t op);
	void MPYHHA(SPUThread& CPU, spu_opcode_t op);
	void MPYHHAU(SPUThread& CPU, spu_opcode_t op);
	void FSCRRD(SPUThread& CPU, spu_opcode_t op);
	void FESD(SPUThread& CPU, spu_opcode_t op);
	void FRDS(SPUThread& CPU, spu_opcode_t op);
	void FSCRWR(SPUThread& CPU, spu_opcode_t op);
	void DFTSV(SPUThread& CPU, spu_opcode_t op);
	void FCEQ(SPUThread& CPU, spu_opcode_t op);
	void DFCEQ(SPUThread& CPU, spu_opcode_t op);
	void MPY(SPUThread& CPU, spu_opcode_t op);
	void MPYH(SPUThread& CPU, spu_opcode_t op);
	void MPYHH(SPUThread& CPU, spu_opcode_t op);
	void MPYS(SPUThread& CPU, spu_opcode_t op);
	void CEQH(SPUThread& CPU, spu_opcode_t op);
	void FCMEQ(SPUThread& CPU, spu_opcode_t op);
	void DFCMEQ(SPUThread& CPU, spu_opcode_t op);
	void MPYU(SPUThread& CPU, spu_opcode_t op);
	void CEQB(SPUThread& CPU, spu_opcode_t op);
	void FI(SPUThread& CPU, spu_opcode_t op);
	void HEQ(SPUThread& CPU, spu_opcode_t op);

	void CFLTS(SPUThread& CPU, spu_opcode_t op);
	void CFLTU(SPUThread& CPU, spu_opcode_t op);
	void CSFLT(SPUThread& CPU, spu_opcode_t op);
	void CUFLT(SPUThread& CPU, spu_opcode_t op);

	void BRZ(SPUThread& CPU, spu_opcode_t op);
	void STQA(SPUThread& CPU, spu_opcode_t op);
	void BRNZ(SPUThread& CPU, spu_opcode_t op);
	void BRHZ(SPUThread& CPU, spu_opcode_t op);
	void BRHNZ(SPUThread& CPU, spu_opcode_t op);
	void STQR(SPUThread& CPU, spu_opcode_t op);
	void BRA(SPUThread& CPU, spu_opcode_t op);
	void LQA(SPUThread& CPU, spu_opcode_t op);
	void BRASL(SPUThread& CPU, spu_opcode_t op);
	void BR(SPUThread& CPU, spu_opcode_t op);
	void FSMBI(SPUThread& CPU, spu_opcode_t op);
	void BRSL(SPUThread& CPU, spu_opcode_t op);
	void LQR(SPUThread& CPU, spu_opcode_t op);
	void IL(SPUThread& CPU, spu_opcode_t op);
	void ILHU(SPUThread& CPU, spu_opcode_t op);
	void ILH(SPUThread& CPU, spu_opcode_t op);
	void IOHL(SPUThread& CPU, spu_opcode_t op);

	void ORI(SPUThread& CPU, spu_opcode_t op);
	void ORHI(SPUThread& CPU, spu_opcode_t op);
	void ORBI(SPUThread& CPU, spu_opcode_t op);
	void SFI(SPUThread& CPU, spu_opcode_t op);
	void SFHI(SPUThread& CPU, spu_opcode_t op);
	void ANDI(SPUThread& CPU, spu_opcode_t op);
	void ANDHI(SPUThread& CPU, spu_opcode_t op);
	void ANDBI(SPUThread& CPU, spu_opcode_t op);
	void AI(SPUThread& CPU, spu_opcode_t op);
	void AHI(SPUThread& CPU, spu_opcode_t op);
	void STQD(SPUThread& CPU, spu_opcode_t op);
	void LQD(SPUThread& CPU, spu_opcode_t op);
	void XORI(SPUThread& CPU, spu_opcode_t op);
	void XORHI(SPUThread& CPU, spu_opcode_t op);
	void XORBI(SPUThread& CPU, spu_opcode_t op);
	void CGTI(SPUThread& CPU, spu_opcode_t op);
	void CGTHI(SPUThread& CPU, spu_opcode_t op);
	void CGTBI(SPUThread& CPU, spu_opcode_t op);
	void HGTI(SPUThread& CPU, spu_opcode_t op);
	void CLGTI(SPUThread& CPU, spu_opcode_t op);
	void CLGTHI(SPUThread& CPU, spu_opcode_t op);
	void CLGTBI(SPUThread& CPU, spu_opcode_t op);
	void HLGTI(SPUThread& CPU, spu_opcode_t op);
	void MPYI(SPUThread& CPU, spu_opcode_t op);
	void MPYUI(SPUThread& CPU, spu_opcode_t op);
	void CEQI(SPUThread& CPU, spu_opcode_t op);
	void CEQHI(SPUThread& CPU, spu_opcode_t op);
	void CEQBI(SPUThread& CPU, spu_opcode_t op);
	void HEQI(SPUThread& CPU, spu_opcode_t op);

	void HBRA(SPUThread& CPU, spu_opcode_t op);
	void HBRR(SPUThread& CPU, spu_opcode_t op);
	void ILA(SPUThread& CPU, spu_opcode_t op);

	void SELB(SPUThread& CPU, spu_opcode_t op);
	void SHUFB(SPUThread& CPU, spu_opcode_t op);
	void MPYA(SPUThread& CPU, spu_opcode_t op);
	void FNMS(SPUThread& CPU, spu_opcode_t op);
	void FMA(SPUThread& CPU, spu_opcode_t op);
	void FMS(SPUThread& CPU, spu_opcode_t op);

	void UNK(SPUThread& CPU, spu_opcode_t op);
}

class SPUInterpreter2 : public SPUOpcodes
{
public:
	virtual ~SPUInterpreter2() {}

	spu_inter_func_t func;

	virtual void STOP(u32 code) { func = spu_interpreter::STOP; }
	virtual void LNOP() { func = spu_interpreter::LNOP; }
	virtual void SYNC(u32 Cbit) { func = spu_interpreter::SYNC; }
	virtual void DSYNC() { func = spu_interpreter::DSYNC; }
	virtual void MFSPR(u32 rt, u32 sa) { func = spu_interpreter::MFSPR; }
	virtual void RDCH(u32 rt, u32 ra) { func = spu_interpreter::RDCH; }
	virtual void RCHCNT(u32 rt, u32 ra) { func = spu_interpreter::RCHCNT; }
	virtual void SF(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SF; }
	virtual void OR(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::OR; }
	virtual void BG(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::BG; }
	virtual void SFH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SFH; }
	virtual void NOR(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::NOR; }
	virtual void ABSDB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ABSDB; }
	virtual void ROT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROT; }
	virtual void ROTM(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTM; }
	virtual void ROTMA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTMA; }
	virtual void SHL(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SHL; }
	virtual void ROTH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTH; }
	virtual void ROTHM(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTHM; }
	virtual void ROTMAH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTMAH; }
	virtual void SHLH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SHLH; }
	virtual void ROTI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTI; }
	virtual void ROTMI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTMI; }
	virtual void ROTMAI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTMAI; }
	virtual void SHLI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::SHLI; }
	virtual void ROTHI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTHI; }
	virtual void ROTHMI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTHMI; }
	virtual void ROTMAHI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTMAHI; }
	virtual void SHLHI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::SHLHI; }
	virtual void A(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::A; }
	virtual void AND(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::AND; }
	virtual void CG(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CG; }
	virtual void AH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::AH; }
	virtual void NAND(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::NAND; }
	virtual void AVGB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::AVGB; }
	virtual void MTSPR(u32 rt, u32 sa) { func = spu_interpreter::MTSPR; }
	virtual void WRCH(u32 ra, u32 rt) { func = spu_interpreter::WRCH; }
	virtual void BIZ(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BIZ; }
	virtual void BINZ(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BINZ; }
	virtual void BIHZ(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BIHZ; }
	virtual void BIHNZ(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BIHNZ; }
	virtual void STOPD(u32 rc, u32 ra, u32 rb) { func = spu_interpreter::STOPD; }
	virtual void STQX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::STQX; }
	virtual void BI(u32 intr, u32 ra) { func = spu_interpreter::BI; }
	virtual void BISL(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BISL; }
	virtual void IRET(u32 ra) { func = spu_interpreter::IRET; }
	virtual void BISLED(u32 intr, u32 rt, u32 ra) { func = spu_interpreter::BISLED; }
	virtual void HBR(u32 p, u32 ro, u32 ra) { func = spu_interpreter::HBR; }
	virtual void GB(u32 rt, u32 ra) { func = spu_interpreter::GB; }
	virtual void GBH(u32 rt, u32 ra) { func = spu_interpreter::GBH; }
	virtual void GBB(u32 rt, u32 ra) { func = spu_interpreter::GBB; }
	virtual void FSM(u32 rt, u32 ra) { func = spu_interpreter::FSM; }
	virtual void FSMH(u32 rt, u32 ra) { func = spu_interpreter::FSMH; }
	virtual void FSMB(u32 rt, u32 ra) { func = spu_interpreter::FSMB; }
	virtual void FREST(u32 rt, u32 ra) { func = spu_interpreter::FREST; }
	virtual void FRSQEST(u32 rt, u32 ra) { func = spu_interpreter::FRSQEST; }
	virtual void LQX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::LQX; }
	virtual void ROTQBYBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQBYBI; }
	virtual void ROTQMBYBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQMBYBI; }
	virtual void SHLQBYBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SHLQBYBI; }
	virtual void CBX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CBX; }
	virtual void CHX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CHX; }
	virtual void CWX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CWX; }
	virtual void CDX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CDX; }
	virtual void ROTQBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQBI; }
	virtual void ROTQMBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQMBI; }
	virtual void SHLQBI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SHLQBI; }
	virtual void ROTQBY(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQBY; }
	virtual void ROTQMBY(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ROTQMBY; }
	virtual void SHLQBY(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SHLQBY; }
	virtual void ORX(u32 rt, u32 ra) { func = spu_interpreter::ORX; }
	virtual void CBD(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::CBD; }
	virtual void CHD(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::CHD; }
	virtual void CWD(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::CWD; }
	virtual void CDD(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::CDD; }
	virtual void ROTQBII(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTQBII; }
	virtual void ROTQMBII(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTQMBII; }
	virtual void SHLQBII(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::SHLQBII; }
	virtual void ROTQBYI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTQBYI; }
	virtual void ROTQMBYI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::ROTQMBYI; }
	virtual void SHLQBYI(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::SHLQBYI; }
	virtual void NOP(u32 rt) { func = spu_interpreter::NOP; }
	virtual void CGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CGT; }
	virtual void XOR(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::XOR; }
	virtual void CGTH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CGTH; }
	virtual void EQV(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::EQV; }
	virtual void CGTB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CGTB; }
	virtual void SUMB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SUMB; }
	virtual void HGT(u32 rt, s32 ra, s32 rb) { func = spu_interpreter::HGT; }
	virtual void CLZ(u32 rt, u32 ra) { func = spu_interpreter::CLZ; }
	virtual void XSWD(u32 rt, u32 ra) { func = spu_interpreter::XSWD; }
	virtual void XSHW(u32 rt, u32 ra) { func = spu_interpreter::XSHW; }
	virtual void CNTB(u32 rt, u32 ra) { func = spu_interpreter::CNTB; }
	virtual void XSBH(u32 rt, u32 ra) { func = spu_interpreter::XSBH; }
	virtual void CLGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CLGT; }
	virtual void ANDC(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ANDC; }
	virtual void FCGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FCGT; }
	virtual void DFCGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFCGT; }
	virtual void FA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FA; }
	virtual void FS(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FS; }
	virtual void FM(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FM; }
	virtual void CLGTH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CLGTH; }
	virtual void ORC(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ORC; }
	virtual void FCMGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FCMGT; }
	virtual void DFCMGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFCMGT; }
	virtual void DFA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFA; }
	virtual void DFS(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFS; }
	virtual void DFM(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFM; }
	virtual void CLGTB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CLGTB; }
	virtual void HLGT(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::HLGT; }
	virtual void DFMA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFMA; }
	virtual void DFMS(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFMS; }
	virtual void DFNMS(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFNMS; }
	virtual void DFNMA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFNMA; }
	virtual void CEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CEQ; }
	virtual void MPYHHU(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYHHU; }
	virtual void ADDX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::ADDX; }
	virtual void SFX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::SFX; }
	virtual void CGX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CGX; }
	virtual void BGX(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::BGX; }
	virtual void MPYHHA(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYHHA; }
	virtual void MPYHHAU(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYHHAU; }
	virtual void FSCRRD(u32 rt) { func = spu_interpreter::FSCRRD; }
	virtual void FESD(u32 rt, u32 ra) { func = spu_interpreter::FESD; }
	virtual void FRDS(u32 rt, u32 ra) { func = spu_interpreter::FRDS; }
	virtual void FSCRWR(u32 rt, u32 ra) { func = spu_interpreter::FSCRWR; }
	virtual void DFTSV(u32 rt, u32 ra, s32 i7) { func = spu_interpreter::DFTSV; }
	virtual void FCEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FCEQ; }
	virtual void DFCEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFCEQ; }
	virtual void MPY(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPY; }
	virtual void MPYH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYH; }
	virtual void MPYHH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYHH; }
	virtual void MPYS(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYS; }
	virtual void CEQH(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CEQH; }
	virtual void FCMEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FCMEQ; }
	virtual void DFCMEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::DFCMEQ; }
	virtual void MPYU(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::MPYU; }
	virtual void CEQB(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::CEQB; }
	virtual void FI(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::FI; }
	virtual void HEQ(u32 rt, u32 ra, u32 rb) { func = spu_interpreter::HEQ; }

	virtual void CFLTS(u32 rt, u32 ra, s32 i8) { func = spu_interpreter::CFLTS; }
	virtual void CFLTU(u32 rt, u32 ra, s32 i8) { func = spu_interpreter::CFLTU; }
	virtual void CSFLT(u32 rt, u32 ra, s32 i8) { func = spu_interpreter::CSFLT; }
	virtual void CUFLT(u32 rt, u32 ra, s32 i8) { func = spu_interpreter::CUFLT; }

	virtual void BRZ(u32 rt, s32 i16) { func = spu_interpreter::BRZ; }
	virtual void STQA(u32 rt, s32 i16) { func = spu_interpreter::STQA; }
	virtual void BRNZ(u32 rt, s32 i16) { func = spu_interpreter::BRNZ; }
	virtual void BRHZ(u32 rt, s32 i16) { func = spu_interpreter::BRHZ; }
	virtual void BRHNZ(u32 rt, s32 i16) { func = spu_interpreter::BRHNZ; }
	virtual void STQR(u32 rt, s32 i16) { func = spu_interpreter::STQR; }
	virtual void BRA(s32 i16) { func = spu_interpreter::BRA; }
	virtual void LQA(u32 rt, s32 i16) { func = spu_interpreter::LQA; }
	virtual void BRASL(u32 rt, s32 i16) { func = spu_interpreter::BRASL; }
	virtual void BR(s32 i16) { func = spu_interpreter::BR; }
	virtual void FSMBI(u32 rt, s32 i16) { func = spu_interpreter::FSMBI; }
	virtual void BRSL(u32 rt, s32 i16) { func = spu_interpreter::BRSL; }
	virtual void LQR(u32 rt, s32 i16) { func = spu_interpreter::LQR; }
	virtual void IL(u32 rt, s32 i16) { func = spu_interpreter::IL; }
	virtual void ILHU(u32 rt, s32 i16) { func = spu_interpreter::ILHU; }
	virtual void ILH(u32 rt, s32 i16) { func = spu_interpreter::ILH; }
	virtual void IOHL(u32 rt, s32 i16) { func = spu_interpreter::IOHL; }

	virtual void ORI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ORI; }
	virtual void ORHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ORHI; }
	virtual void ORBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ORBI; }
	virtual void SFI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::SFI; }
	virtual void SFHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::SFHI; }
	virtual void ANDI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ANDI; }
	virtual void ANDHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ANDHI; }
	virtual void ANDBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::ANDBI; }
	virtual void AI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::AI; }
	virtual void AHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::AHI; }
	virtual void STQD(u32 rt, s32 i10, u32 ra) { func = spu_interpreter::STQD; }
	virtual void LQD(u32 rt, s32 i10, u32 ra) { func = spu_interpreter::LQD; }
	virtual void XORI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::XORI; }
	virtual void XORHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::XORHI; }
	virtual void XORBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::XORBI; }
	virtual void CGTI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CGTI; }
	virtual void CGTHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CGTHI; }
	virtual void CGTBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CGTBI; }
	virtual void HGTI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::HGTI; }
	virtual void CLGTI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CLGTI; }
	virtual void CLGTHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CLGTHI; }
	virtual void CLGTBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CLGTBI; }
	virtual void HLGTI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::HLGTI; }
	virtual void MPYI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::MPYI; }
	virtual void MPYUI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::MPYUI; }
	virtual void CEQI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CEQI; }
	virtual void CEQHI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CEQHI; }
	virtual void CEQBI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::CEQBI; }
	virtual void HEQI(u32 rt, u32 ra, s32 i10) { func = spu_interpreter::HEQI; }

	virtual void HBRA(s32 ro, s32 i16) { func = spu_interpreter::HBRA; }
	virtual void HBRR(s32 ro, s32 i16) { func = spu_interpreter::HBRR; }
	virtual void ILA(u32 rt, u32 i18) { func = spu_interpreter::ILA; }

	virtual void SELB(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::SELB; }
	virtual void SHUFB(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::SHUFB; }
	virtual void MPYA(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::MPYA; }
	virtual void FNMS(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::FNMS; }
	virtual void FMA(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::FMA; }
	virtual void FMS(u32 rc, u32 ra, u32 rb, u32 rt) { func = spu_interpreter::FMS; }

	virtual void UNK(u32 code, u32 opcode, u32 gcode) { func = spu_interpreter::UNK; }
};
