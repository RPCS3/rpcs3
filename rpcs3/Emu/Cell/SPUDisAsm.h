#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

#define START_OPCODES_GROUP(x) /*x*/
#define END_OPCODES_GROUP(x) /*x*/

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
	virtual void Exit()
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
	virtual void STOP(OP_uIMM code)
	{
		Write(wxString::Format("stop 0x%x", code));
	}
	virtual void LNOP()
	{
		Write("lnop");
	}
	virtual void SYNC(OP_uIMM Cbit)
	{
		Write(wxString::Format("sync %d", Cbit));
	}
	virtual void DSYNC()
	{
		Write("dsync");
	}
	virtual void MFSPR(OP_REG rt, OP_REG sa)
	{
		Write(wxString::Format("mfspr %s,%s", spu_reg_name[rt], spu_reg_name[sa]));   // Are SPR mapped on the GPR or are there 128 additional registers ?
	}
	virtual void RDCH(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("rdch %s,%s", spu_reg_name[rt], spu_ch_name[ra]));
	}
	virtual void RCHCNT(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("rchcnt %s,%s", spu_reg_name[rt], spu_ch_name[ra]));
	}
	virtual void SF(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("sf %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void OR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("or %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void BG(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("bg %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SFH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("sfh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void NOR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("nor %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ABSDB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("absdb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rot %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SHL(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("shl %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("roth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTHM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rothm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTMAH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotmah %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SHLH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("shlh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("roti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTMI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotmi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTMAI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotmai %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SHLI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shli %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rothi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTHMI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rothmi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTMAHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotmahi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SHLHI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shlhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void A(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("a %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void AND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("and %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CG(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cg %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void AH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("ah %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void NAND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("nand %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void AVGB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("avgb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MTSPR(OP_REG rt, OP_REG sa)
	{
		Write(wxString::Format("mtspr %s,%s", spu_reg_name[rt], spu_reg_name[sa]));
	}
	virtual void WRCH(OP_REG ra, OP_REG rt)
	{
		Write(wxString::Format("wrch %s,%s", spu_ch_name[ra], spu_reg_name[rt]));
	}
	virtual void BIZ(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("biz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void BINZ(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("binz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void BIHZ(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("bihz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void BIHNZ(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("bihnz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void STOPD(OP_REG rc, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("bihnz %s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void STQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("stqx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void BI(OP_REG ra)
	{
		Write(wxString::Format("bi %s", spu_reg_name[ra]));
	}
	virtual void BISL(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("bisl %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void IRET(OP_REG ra)
	{
		Write(wxString::Format("iret %s", spu_reg_name[ra]));
	}
	virtual void BISLED(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("bisled %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void HBR(OP_REG p, OP_REG ro, OP_REG ra)
	{
		Write(wxString::Format("hbr 0x%x,%s", DisAsmBranchTarget(ro), spu_reg_name[ra]));
	}
	virtual void GB(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("gb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void GBH(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("gbh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void GBB(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("gbb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FSM(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("fsm %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FSMH(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("fsmh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FSMB(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("fsmb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FREST(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("frest %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FRSQEST(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("frsqest %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void LQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("lqx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQMBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqmbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SHLQBYBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("shlqbybi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CBX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cbx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CHX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("chx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CWX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cwx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CDX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cdx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQMBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqmbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SHLQBI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("shlqbi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQMBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqmby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SHLQBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("shlqby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ORX(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("orx %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void CBD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("cbd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void CHD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("chd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void CWD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("cwd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void CDD(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("cdd %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTQBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotqbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTQMBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotqmbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SHLQBII(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shlqbii %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void ROTQMBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotqmbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SHLQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shlqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void NOP(OP_REG rt)
	{
		Write(wxString::Format("nop %s", spu_reg_name[rt]));
	}
	virtual void CGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void XOR(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("xor %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CGTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cgth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void EQV(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("eqv %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CGTB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cgtb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SUMB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("sumb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void HGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("hgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CLZ(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("clz %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void XSWD(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("xswd %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void XSHW(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("xshw %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void CNTB(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("cntb %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void XSBH(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("xsbh %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void CLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("clgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ANDC(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("andc %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FCGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fcgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFCGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfcgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fa %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fs %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CLGTH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("clgth %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ORC(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("orc %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FCMGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fcmgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFCMGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfcmgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfa %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfs %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFM(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfm %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CLGTB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("clgtb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void HLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("hlgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFMS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfms %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFNMS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfnms %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFNMA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfnma %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("ceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYHHU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyhhu %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ADDX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("addx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SFX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("sfx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CGX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cgx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void BGX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("bgx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYHHA(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyhha %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYHHAU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyhhau %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FSCRRD(OP_REG rt)
	{
		Write(wxString::Format("fscrrd %s", spu_reg_name[rt]));
	}
	virtual void FESD(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("fesd %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FRDS(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("frds %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void FSCRWR(OP_REG rt, OP_REG ra)
	{
		Write(wxString::Format("fscrwr %s,%s", spu_reg_name[rt], spu_reg_name[ra]));
	}
	virtual void DFTSV(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("dftsv %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void FCEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFCEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfceq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpy %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYHH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyhh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYS(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpys %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CEQH(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("ceqh %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FCMEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fcmeq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void DFCMEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("dfcmeq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void MPYU(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("mpyu %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void CEQB(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("ceqb %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void FI(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("fi %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void HEQ(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("heq %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}

   //0 - 9
	virtual void CFLTS(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		Write(wxString::Format("cflts %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	virtual void CFLTU(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		Write(wxString::Format("cfltu %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	virtual void CSFLT(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		Write(wxString::Format("csflt %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}
	virtual void CUFLT(OP_REG rt, OP_REG ra, OP_sIMM i8)
	{
		Write(wxString::Format("cuflt %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i8));
	}

	//0 - 8
	virtual void BRZ(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void STQA(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("stqa %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BRNZ(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brnz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BRHZ(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brhz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BRHNZ(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brhnz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void STQR(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("stqr %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BRA(OP_sIMM i16)
	{
		Write(wxString::Format("bra 0x%x", DisAsmBranchTarget(i16)));
	}
	virtual void LQA(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("lqa %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BRASL(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brasl %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void BR(OP_sIMM i16)
	{
		Write(wxString::Format("br 0x%x", DisAsmBranchTarget(i16)));
	}
	virtual void FSMBI(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("fsmbi %s,%d", spu_reg_name[rt], i16));
	}
	virtual void BRSL(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brsl %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void LQR(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("lqr %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}
	virtual void IL(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("il %s,%d", spu_reg_name[rt], i16));
	}
	virtual void ILHU(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("ilhu %s,%d", spu_reg_name[rt], i16));
	}
	virtual void ILH(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("ilh %s,%d", spu_reg_name[rt], i16));
	}
	virtual void IOHL(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("iolh %s,%d", spu_reg_name[rt], i16));
	}
	

	//0 - 7
	virtual void ORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ori %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void ORHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("orhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void ORBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("orbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void SFI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("sfi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void SFHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("sfhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void ANDI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("andi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void ANDHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("andhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void ANDBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("andbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void AI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ai %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void AHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ahi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void STQD(OP_REG rt, OP_sIMM i10, OP_REG ra)
	{
		Write(wxString::Format("stqd %s,%d(%s)", spu_reg_name[rt], i10, spu_reg_name[ra]));
	}
	virtual void LQD(OP_REG rt, OP_sIMM i10, OP_REG ra)
	{
		Write(wxString::Format("lqd %s,%d(%s)", spu_reg_name[rt], i10, spu_reg_name[ra]));
	}
	virtual void XORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("xori %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void XORHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("xorhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void XORBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("xorbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("cgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CGTHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("cgthi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CGTBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("cgtbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void HGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("hgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CLGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("clgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CLGTHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("clgthi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CLGTBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("clgtbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void HLGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("hlgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void MPYI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("mpyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void MPYUI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("mpyui %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ceqi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CEQHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ceqhi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CEQBI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ceqbi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void HEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("heqi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}

	//0 - 6
	virtual void HBRA(OP_sIMM ro, OP_sIMM i16)
	{
		Write(wxString::Format("hbra 0x%x,0x%x", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16)));
	}
	virtual void HBRR(OP_sIMM ro, OP_sIMM i16)
	{
		Write(wxString::Format("hbrr 0x%x,0x%x", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16)));
	}
	virtual void ILA(OP_REG rt, OP_sIMM i18)
	{
		Write(wxString::Format("ila %s,%d", spu_reg_name[rt], i18));
	}

	//0 - 3
	virtual void SELB(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("selb %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	virtual void SHUFB(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("shufb %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	virtual void MPYA(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("mpya %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	virtual void FNMS(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("fnms %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	virtual void FMA(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("fma %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}
	virtual void FMS(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt)
	{
		Write(wxString::Format("fms %s,%s,%s,%s", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]));
	}

	virtual void UNK(const s32 code, const s32 opcode, const s32 gcode)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%08x)", code));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP