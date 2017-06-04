#pragma once

#include "SPUOpcodes.h"

class SPUThread;

using spu_inter_func_t = void(*)(SPUThread& spu, spu_opcode_t opcode);

namespace spu_interpreter
{
	namespace fast
	{
		extern const spu_opcode_table_t<spu_inter_func_t> g_spu_opcode_table;
	}

	namespace precise
	{
		extern const spu_opcode_table_t<spu_inter_func_t> g_spu_opcode_table;
	}

	void default_function(SPUThread& spu, spu_opcode_t op);
	void set_interrupt_status(SPUThread& spu, spu_opcode_t op);

	void STOP(SPUThread& spu, spu_opcode_t op);
	void LNOP(SPUThread& spu, spu_opcode_t op);
	void SYNC(SPUThread& spu, spu_opcode_t op);
	void DSYNC(SPUThread& spu, spu_opcode_t op);
	void MFSPR(SPUThread& spu, spu_opcode_t op);
	void RDCH(SPUThread& spu, spu_opcode_t op);
	void RCHCNT(SPUThread& spu, spu_opcode_t op);
	void SF(SPUThread& spu, spu_opcode_t op);
	void OR(SPUThread& spu, spu_opcode_t op);
	void BG(SPUThread& spu, spu_opcode_t op);
	void SFH(SPUThread& spu, spu_opcode_t op);
	void NOR(SPUThread& spu, spu_opcode_t op);
	void ABSDB(SPUThread& spu, spu_opcode_t op);
	void ROT(SPUThread& spu, spu_opcode_t op);
	void ROTM(SPUThread& spu, spu_opcode_t op);
	void ROTMA(SPUThread& spu, spu_opcode_t op);
	void SHL(SPUThread& spu, spu_opcode_t op);
	void ROTH(SPUThread& spu, spu_opcode_t op);
	void ROTHM(SPUThread& spu, spu_opcode_t op);
	void ROTMAH(SPUThread& spu, spu_opcode_t op);
	void SHLH(SPUThread& spu, spu_opcode_t op);
	void ROTI(SPUThread& spu, spu_opcode_t op);
	void ROTMI(SPUThread& spu, spu_opcode_t op);
	void ROTMAI(SPUThread& spu, spu_opcode_t op);
	void SHLI(SPUThread& spu, spu_opcode_t op);
	void ROTHI(SPUThread& spu, spu_opcode_t op);
	void ROTHMI(SPUThread& spu, spu_opcode_t op);
	void ROTMAHI(SPUThread& spu, spu_opcode_t op);
	void SHLHI(SPUThread& spu, spu_opcode_t op);
	void A(SPUThread& spu, spu_opcode_t op);
	void AND(SPUThread& spu, spu_opcode_t op);
	void CG(SPUThread& spu, spu_opcode_t op);
	void AH(SPUThread& spu, spu_opcode_t op);
	void NAND(SPUThread& spu, spu_opcode_t op);
	void AVGB(SPUThread& spu, spu_opcode_t op);
	void MTSPR(SPUThread& spu, spu_opcode_t op);
	void WRCH(SPUThread& spu, spu_opcode_t op);
	void BIZ(SPUThread& spu, spu_opcode_t op);
	void BINZ(SPUThread& spu, spu_opcode_t op);
	void BIHZ(SPUThread& spu, spu_opcode_t op);
	void BIHNZ(SPUThread& spu, spu_opcode_t op);
	void STOPD(SPUThread& spu, spu_opcode_t op);
	void STQX(SPUThread& spu, spu_opcode_t op);
	void BI(SPUThread& spu, spu_opcode_t op);
	void BISL(SPUThread& spu, spu_opcode_t op);
	void IRET(SPUThread& spu, spu_opcode_t op);
	void BISLED(SPUThread& spu, spu_opcode_t op);
	void HBR(SPUThread& spu, spu_opcode_t op);
	void GB(SPUThread& spu, spu_opcode_t op);
	void GBH(SPUThread& spu, spu_opcode_t op);
	void GBB(SPUThread& spu, spu_opcode_t op);
	void FSM(SPUThread& spu, spu_opcode_t op);
	void FSMH(SPUThread& spu, spu_opcode_t op);
	void FSMB(SPUThread& spu, spu_opcode_t op);
	void LQX(SPUThread& spu, spu_opcode_t op);
	void ROTQBYBI(SPUThread& spu, spu_opcode_t op);
	void ROTQMBYBI(SPUThread& spu, spu_opcode_t op);
	void SHLQBYBI(SPUThread& spu, spu_opcode_t op);
	void CBX(SPUThread& spu, spu_opcode_t op);
	void CHX(SPUThread& spu, spu_opcode_t op);
	void CWX(SPUThread& spu, spu_opcode_t op);
	void CDX(SPUThread& spu, spu_opcode_t op);
	void ROTQBI(SPUThread& spu, spu_opcode_t op);
	void ROTQMBI(SPUThread& spu, spu_opcode_t op);
	void SHLQBI(SPUThread& spu, spu_opcode_t op);
	void ROTQBY(SPUThread& spu, spu_opcode_t op);
	void ROTQMBY(SPUThread& spu, spu_opcode_t op);
	void SHLQBY(SPUThread& spu, spu_opcode_t op);
	void ORX(SPUThread& spu, spu_opcode_t op);
	void CBD(SPUThread& spu, spu_opcode_t op);
	void CHD(SPUThread& spu, spu_opcode_t op);
	void CWD(SPUThread& spu, spu_opcode_t op);
	void CDD(SPUThread& spu, spu_opcode_t op);
	void ROTQBII(SPUThread& spu, spu_opcode_t op);
	void ROTQMBII(SPUThread& spu, spu_opcode_t op);
	void SHLQBII(SPUThread& spu, spu_opcode_t op);
	void ROTQBYI(SPUThread& spu, spu_opcode_t op);
	void ROTQMBYI(SPUThread& spu, spu_opcode_t op);
	void SHLQBYI(SPUThread& spu, spu_opcode_t op);
	void NOP(SPUThread& spu, spu_opcode_t op);
	void CGT(SPUThread& spu, spu_opcode_t op);
	void XOR(SPUThread& spu, spu_opcode_t op);
	void CGTH(SPUThread& spu, spu_opcode_t op);
	void EQV(SPUThread& spu, spu_opcode_t op);
	void CGTB(SPUThread& spu, spu_opcode_t op);
	void SUMB(SPUThread& spu, spu_opcode_t op);
	void HGT(SPUThread& spu, spu_opcode_t op);
	void CLZ(SPUThread& spu, spu_opcode_t op);
	void XSWD(SPUThread& spu, spu_opcode_t op);
	void XSHW(SPUThread& spu, spu_opcode_t op);
	void CNTB(SPUThread& spu, spu_opcode_t op);
	void XSBH(SPUThread& spu, spu_opcode_t op);
	void CLGT(SPUThread& spu, spu_opcode_t op);
	void ANDC(SPUThread& spu, spu_opcode_t op);
	void CLGTH(SPUThread& spu, spu_opcode_t op);
	void ORC(SPUThread& spu, spu_opcode_t op);
	void CLGTB(SPUThread& spu, spu_opcode_t op);
	void HLGT(SPUThread& spu, spu_opcode_t op);
	void CEQ(SPUThread& spu, spu_opcode_t op);
	void MPYHHU(SPUThread& spu, spu_opcode_t op);
	void ADDX(SPUThread& spu, spu_opcode_t op);
	void SFX(SPUThread& spu, spu_opcode_t op);
	void CGX(SPUThread& spu, spu_opcode_t op);
	void BGX(SPUThread& spu, spu_opcode_t op);
	void MPYHHA(SPUThread& spu, spu_opcode_t op);
	void MPYHHAU(SPUThread& spu, spu_opcode_t op);
	void MPY(SPUThread& spu, spu_opcode_t op);
	void MPYH(SPUThread& spu, spu_opcode_t op);
	void MPYHH(SPUThread& spu, spu_opcode_t op);
	void MPYS(SPUThread& spu, spu_opcode_t op);
	void CEQH(SPUThread& spu, spu_opcode_t op);
	void MPYU(SPUThread& spu, spu_opcode_t op);
	void CEQB(SPUThread& spu, spu_opcode_t op);
	void HEQ(SPUThread& spu, spu_opcode_t op);
	void BRZ(SPUThread& spu, spu_opcode_t op);
	void STQA(SPUThread& spu, spu_opcode_t op);
	void BRNZ(SPUThread& spu, spu_opcode_t op);
	void BRHZ(SPUThread& spu, spu_opcode_t op);
	void BRHNZ(SPUThread& spu, spu_opcode_t op);
	void STQR(SPUThread& spu, spu_opcode_t op);
	void BRA(SPUThread& spu, spu_opcode_t op);
	void LQA(SPUThread& spu, spu_opcode_t op);
	void BRASL(SPUThread& spu, spu_opcode_t op);
	void BR(SPUThread& spu, spu_opcode_t op);
	void FSMBI(SPUThread& spu, spu_opcode_t op);
	void BRSL(SPUThread& spu, spu_opcode_t op);
	void LQR(SPUThread& spu, spu_opcode_t op);
	void IL(SPUThread& spu, spu_opcode_t op);
	void ILHU(SPUThread& spu, spu_opcode_t op);
	void ILH(SPUThread& spu, spu_opcode_t op);
	void IOHL(SPUThread& spu, spu_opcode_t op);
	void ORI(SPUThread& spu, spu_opcode_t op);
	void ORHI(SPUThread& spu, spu_opcode_t op);
	void ORBI(SPUThread& spu, spu_opcode_t op);
	void SFI(SPUThread& spu, spu_opcode_t op);
	void SFHI(SPUThread& spu, spu_opcode_t op);
	void ANDI(SPUThread& spu, spu_opcode_t op);
	void ANDHI(SPUThread& spu, spu_opcode_t op);
	void ANDBI(SPUThread& spu, spu_opcode_t op);
	void AI(SPUThread& spu, spu_opcode_t op);
	void AHI(SPUThread& spu, spu_opcode_t op);
	void STQD(SPUThread& spu, spu_opcode_t op);
	void LQD(SPUThread& spu, spu_opcode_t op);
	void XORI(SPUThread& spu, spu_opcode_t op);
	void XORHI(SPUThread& spu, spu_opcode_t op);
	void XORBI(SPUThread& spu, spu_opcode_t op);
	void CGTI(SPUThread& spu, spu_opcode_t op);
	void CGTHI(SPUThread& spu, spu_opcode_t op);
	void CGTBI(SPUThread& spu, spu_opcode_t op);
	void HGTI(SPUThread& spu, spu_opcode_t op);
	void CLGTI(SPUThread& spu, spu_opcode_t op);
	void CLGTHI(SPUThread& spu, spu_opcode_t op);
	void CLGTBI(SPUThread& spu, spu_opcode_t op);
	void HLGTI(SPUThread& spu, spu_opcode_t op);
	void MPYI(SPUThread& spu, spu_opcode_t op);
	void MPYUI(SPUThread& spu, spu_opcode_t op);
	void CEQI(SPUThread& spu, spu_opcode_t op);
	void CEQHI(SPUThread& spu, spu_opcode_t op);
	void CEQBI(SPUThread& spu, spu_opcode_t op);
	void HEQI(SPUThread& spu, spu_opcode_t op);
	void HBRA(SPUThread& spu, spu_opcode_t op);
	void HBRR(SPUThread& spu, spu_opcode_t op);
	void ILA(SPUThread& spu, spu_opcode_t op);
	void SELB(SPUThread& spu, spu_opcode_t op);
	void SHUFB(SPUThread& spu, spu_opcode_t op);
	void MPYA(SPUThread& spu, spu_opcode_t op);
	void DFCGT(SPUThread& spu, spu_opcode_t op);
	void DFCMGT(SPUThread& spu, spu_opcode_t op);
	void DFTSV(SPUThread& spu, spu_opcode_t op);
	void DFCEQ(SPUThread& spu, spu_opcode_t op);
	void DFCMEQ(SPUThread& spu, spu_opcode_t op);

	namespace fast
	{
		void FREST(SPUThread& spu, spu_opcode_t op);
		void FRSQEST(SPUThread& spu, spu_opcode_t op);
		void FCGT(SPUThread& spu, spu_opcode_t op);
		void FA(SPUThread& spu, spu_opcode_t op);
		void FS(SPUThread& spu, spu_opcode_t op);
		void FM(SPUThread& spu, spu_opcode_t op);
		void FCMGT(SPUThread& spu, spu_opcode_t op);
		void DFA(SPUThread& spu, spu_opcode_t op);
		void DFS(SPUThread& spu, spu_opcode_t op);
		void DFM(SPUThread& spu, spu_opcode_t op);
		void DFMA(SPUThread& spu, spu_opcode_t op);
		void DFMS(SPUThread& spu, spu_opcode_t op);
		void DFNMS(SPUThread& spu, spu_opcode_t op);
		void DFNMA(SPUThread& spu, spu_opcode_t op);
		void FSCRRD(SPUThread& spu, spu_opcode_t op);
		void FESD(SPUThread& spu, spu_opcode_t op);
		void FRDS(SPUThread& spu, spu_opcode_t op);
		void FSCRWR(SPUThread& spu, spu_opcode_t op);
		void FCEQ(SPUThread& spu, spu_opcode_t op);
		void FCMEQ(SPUThread& spu, spu_opcode_t op);
		void FI(SPUThread& spu, spu_opcode_t op);
		void CFLTS(SPUThread& spu, spu_opcode_t op);
		void CFLTU(SPUThread& spu, spu_opcode_t op);
		void CSFLT(SPUThread& spu, spu_opcode_t op);
		void CUFLT(SPUThread& spu, spu_opcode_t op);
		void FNMS(SPUThread& spu, spu_opcode_t op);
		void FMA(SPUThread& spu, spu_opcode_t op);
		void FMS(SPUThread& spu, spu_opcode_t op);
	}

	namespace precise
	{
		void FREST(SPUThread& spu, spu_opcode_t op);
		void FRSQEST(SPUThread& spu, spu_opcode_t op);
		void FCGT(SPUThread& spu, spu_opcode_t op);
		void FA(SPUThread& spu, spu_opcode_t op);
		void FS(SPUThread& spu, spu_opcode_t op);
		void FM(SPUThread& spu, spu_opcode_t op);
		void FCMGT(SPUThread& spu, spu_opcode_t op);
		void DFA(SPUThread& spu, spu_opcode_t op);
		void DFS(SPUThread& spu, spu_opcode_t op);
		void DFM(SPUThread& spu, spu_opcode_t op);
		void DFMA(SPUThread& spu, spu_opcode_t op);
		void DFMS(SPUThread& spu, spu_opcode_t op);
		void DFNMS(SPUThread& spu, spu_opcode_t op);
		void DFNMA(SPUThread& spu, spu_opcode_t op);
		void FSCRRD(SPUThread& spu, spu_opcode_t op);
		void FESD(SPUThread& spu, spu_opcode_t op);
		void FRDS(SPUThread& spu, spu_opcode_t op);
		void FSCRWR(SPUThread& spu, spu_opcode_t op);
		void FCEQ(SPUThread& spu, spu_opcode_t op);
		void FCMEQ(SPUThread& spu, spu_opcode_t op);
		void FI(SPUThread& spu, spu_opcode_t op);
		void CFLTS(SPUThread& spu, spu_opcode_t op);
		void CFLTU(SPUThread& spu, spu_opcode_t op);
		void CSFLT(SPUThread& spu, spu_opcode_t op);
		void CUFLT(SPUThread& spu, spu_opcode_t op);
		void FNMS(SPUThread& spu, spu_opcode_t op);
		void FMA(SPUThread& spu, spu_opcode_t op);
		void FMS(SPUThread& spu, spu_opcode_t op);
	}
}
