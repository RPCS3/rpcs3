#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

class SPU_DisAsm 
	: public SPU_Opcodes
	, public PPC_DisAsm
{
public:
	PPCThread& CPU;

	SPU_DisAsm()
		: PPC_DisAsm(*(PPCThread*)NULL, DumpMode)
		, CPU(*(PPCThread*)NULL)
	{
	}

	SPU_DisAsm(PPCThread& cpu, DisAsmModes mode = NormalMode)
		: PPC_DisAsm(cpu, mode)
		, CPU(cpu)
	{
	}

	~SPU_DisAsm()
	{
	}

private:
	void Exit()
	{
		if(m_mode == NormalMode && !disasm_frame->exit)
		{
			disasm_frame->Close();
		}

		this->~SPU_DisAsm();
	}

	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return branchTarget(m_mode == NormalMode ? CPU.PC : dump_pc, imm);
	}

private:
	//0 - 10
	void STOP(u32 code)
	{
		Write(wxString::Format("stop 0x%x", code));
	}
	void LNOP()
	{
		Write("lnop");
	}
	void SYNC(u32 Cbit)
	{
		Write(wxString::Format("sync %d", Cbit));
	}
	void DSYNC()
	{
		Write("dsync");
	}
	void MFSPR(u32 rt, u32 sa)
	{
		Write(wxString::Format("mfspr %s,%s", spu_reg_name[rt], spu_reg_name[sa]));   // Are SPR mapped on the GPR or are there 128 additional registers ?
	}
	void RDCH(u32 rt, u32 ra)
	{
		Write(wxString::Format("rdch %s,%s", spu_reg_name[rt], spu_ch_name[ra]));
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		Write(wxString::Format("rchcnt %s,%s", spu_reg_name[rt], spu_ch_name[ra]));
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("sf %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void OR(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("or %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void BG(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("bg %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SFH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("sfh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void NOR(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("nor %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ABSDB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("absdb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rot %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTM(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTMA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SHL(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("shl %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("roth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTHM(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rothm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTMAH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotmah %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SHLH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("shlh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("roti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTMI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotmi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotmai %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("shli %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rothi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rothmi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotmahi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("shlhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void A(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("a %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void AND(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("and %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CG(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cg %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void AH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("ah %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void NAND(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("nand %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void AVGB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("avgb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MTSPR(u32 rt, u32 sa)
	{
		Write(wxString::Format("mtspr %s,%s", spu_reg_name[rt], spu_reg_name[sa]));
	}
	void WRCH(u32 ra, u32 rt)
	{
		Write(wxString::Format("wrch %s,%s", spu_ch_name[ra], spu_reg_name[rt]));
	}
	void BIZ(u32 rt, u32 ra)
	{
		Write(wxString::Format("biz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void BINZ(u32 rt, u32 ra)
	{
		Write(wxString::Format("binz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void BIHZ(u32 rt, u32 ra)
	{
		Write(wxString::Format("bihz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void BIHNZ(u32 rt, u32 ra)
	{
		Write(wxString::Format("bihnz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		Write(wxString::Format("bihnz %s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("stqx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void BI(u32 ra)
	{
		Write(wxString::Format("bi %s", spu_reg_name[ra]));
	}
	void BISL(u32 rt, u32 ra)
	{
		Write(wxString::Format("bisl %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void IRET(u32 ra)
	{
		Write(wxString::Format("iret %s", spu_reg_name[ra]));
	}
	void BISLED(u32 rt, u32 ra)
	{
		Write(wxString::Format("bisled %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void HBR(u32 p, u32 ro, u32 ra)
	{
		Write(wxString::Format("hbr 0x%x,%s", DisAsmBranchTarget(ro), spu_reg_name[ra]));
	}
	void GB(u32 rt, u32 ra)
	{
		Write(wxString::Format("gb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void GBH(u32 rt, u32 ra)
	{
		Write(wxString::Format("gbh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void GBB(u32 rt, u32 ra)
	{
		Write(wxString::Format("gbb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FSM(u32 rt, u32 ra)
	{
		Write(wxString::Format("fsm %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FSMH(u32 rt, u32 ra)
	{
		Write(wxString::Format("fsmh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FSMB(u32 rt, u32 ra)
	{
		Write(wxString::Format("fsmb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FREST(u32 rt, u32 ra)
	{
		Write(wxString::Format("frest %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		Write(wxString::Format("frsqest %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("lqx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqmbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("shlqbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cbx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("chx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cwx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cdx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqmbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("shlqbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("rotqmby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("shlqby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ORX(u32 rt, u32 ra)
	{
		Write(wxString::Format("orx %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void CBD(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("cbd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("chd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("cwd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("cdd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotqbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotqmbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("shlqbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("rotqmbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("shlqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void NOP(u32 rt)
	{
		Write(wxString::Format("nop %s", spu_reg_name[rt]));
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("xor %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cgth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("eqv %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cgtb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("sumb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void HGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("hgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CLZ(u32 rt, u32 ra)
	{
		Write(wxString::Format("clz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void XSWD(u32 rt, u32 ra)
	{
		Write(wxString::Format("xswd %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void XSHW(u32 rt, u32 ra)
	{
		Write(wxString::Format("xshw %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void CNTB(u32 rt, u32 ra)
	{
		Write(wxString::Format("cntb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void XSBH(u32 rt, u32 ra)
	{
		Write(wxString::Format("xsbh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void CLGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("clgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ANDC(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("andc %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FCGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fcgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfcgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fa %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fs %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CLGTH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("clgth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ORC(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("orc %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FCMGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fcmgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfcmgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfa %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfs %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("clgtb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("hlgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFMA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFMS(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfms %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFNMS(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfnms %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFNMA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfnma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("ceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyhhu %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void ADDX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("addx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void SFX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("sfx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CGX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("cgx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void BGX(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("bgx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYHHA(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyhha %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyhhau %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FSCRRD(u32 rt)
	{
		Write(wxString::Format("fscrrd %s", spu_reg_name[rt]));
	}
	void FESD(u32 rt, u32 ra)
	{
		Write(wxString::Format("fesd %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FRDS(u32 rt, u32 ra)
	{
		Write(wxString::Format("frds %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		Write(wxString::Format("fscrwr %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		Write(wxString::Format("dftsv %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpy %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyhh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpys %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("ceqh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fcmeq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("dfcmeq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("mpyu %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("ceqb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void FI(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("fi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		Write(wxString::Format("heq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		Write(wxString::Format("cflts %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		Write(wxString::Format("cfltu %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		Write(wxString::Format("csflt %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		Write(wxString::Format("cuflt %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		Write(wxString::Format("brz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void STQA(u32 rt, s32 i16)
	{
		Write(wxString::Format("stqa %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BRNZ(u32 rt, s32 i16)
	{
		Write(wxString::Format("brnz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BRHZ(u32 rt, s32 i16)
	{
		Write(wxString::Format("brhz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		Write(wxString::Format("brhnz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void STQR(u32 rt, s32 i16)
	{
		Write(wxString::Format("stqr %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BRA(s32 i16)
	{
		Write(wxString::Format("bra 0x%x", DisAsmBranchTarget(i16)));
	}
	void LQA(u32 rt, s32 i16)
	{
		Write(wxString::Format("lqa %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BRASL(u32 rt, s32 i16)
	{
		Write(wxString::Format("brasl %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void BR(s32 i16)
	{
		Write(wxString::Format("br 0x%x", DisAsmBranchTarget(i16)));
	}
	void FSMBI(u32 rt, s32 i16)
	{
		Write(wxString::Format("fsmbi %s,%d", spu_reg_name[rt], i16));
	}
	void BRSL(u32 rt, s32 i16)
	{
		Write(wxString::Format("brsl %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void LQR(u32 rt, s32 i16)
	{
		Write(wxString::Format("lqr %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	void IL(u32 rt, s32 i16)
	{
		Write(wxString::Format("il %s,%d", spu_reg_name[rt], i16));
	}
	void ILHU(u32 rt, s32 i16)
	{
		Write(wxString::Format("ilhu %s,%d", spu_reg_name[rt], i16));
	}
	void ILH(u32 rt, s32 i16)
	{
		Write(wxString::Format("ilh %s,%d", spu_reg_name[rt], i16));
	}
	void IOHL(u32 rt, s32 i16)
	{
		Write(wxString::Format("iolh %s,%d", spu_reg_name[rt], i16));
	}
	

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ori %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("orhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("orbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void SFI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("sfi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void SFHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("sfhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("andi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("andhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("andbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ai %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ahi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void STQD(u32 rt, s32 i10, u32 ra)
	{
		Write(wxString::Format("stqd %s,%d(%s)", spu_reg_name[rt], i10, spu_reg_name[ra]));
	}
	void LQD(u32 rt, s32 i10, u32 ra)
	{
		Write(wxString::Format("lqd %s,%d(%s)", spu_reg_name[rt], i10, spu_reg_name[ra]));
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("xori %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("xorhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("xorbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("cgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("cgthi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("cgtbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("hgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("clgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("clgthi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("clgtbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("hlgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("mpyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("mpyui %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ceqi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ceqhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("ceqbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		Write(wxString::Format("heqi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}

	//0 - 6
	void HBRA(s32 ro, s32 i16)
	{
		Write(wxString::Format("hbra 0x%x,0x%x", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16)));
	}
	void HBRR(s32 ro, s32 i16)
	{
		Write(wxString::Format("hbrr 0x%x,0x%x", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16)));
	}
	void ILA(u32 rt, s32 i18)
	{
		Write(wxString::Format("ila %s,%d", spu_reg_name[rt], i18));
	}

	//0 - 3
	void SELB(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("selb %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	void SHUFB(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("shufb %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	void MPYA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("mpya %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	void FNMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("fnms %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	void FMA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("fma %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	void FMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		Write(wxString::Format("fms %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP