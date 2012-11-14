#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/DisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

#define START_OPCODES_GROUP(x) /*x*/
#define END_OPCODES_GROUP(x) /*x*/

class SPU_DisAsm 
	: public SPU_Opcodes
	, public DisAsm
{
public:
	PPCThread& CPU;

	SPU_DisAsm()
		: DisAsm(*(PPCThread*)NULL, DumpMode)
		, CPU(*(PPCThread*)NULL)
	{
	}

	SPU_DisAsm(PPCThread& cpu, DisAsmModes mode = NormalMode)
		: DisAsm(cpu, mode)
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
	virtual void SHLI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shli %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void A(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("a %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void SPU_AND(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("and %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void LQX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("lqx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void WRCH(OP_REG ra, OP_REG rt)
	{
		Write(wxString::Format("wrch %s,%s", spu_ch_name[ra], spu_reg_name[rt]));
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
	virtual void HBR(OP_REG p, OP_REG ro, OP_REG ra)
	{
		Write(wxString::Format("hbr 0x%x,%s", DisAsmBranchTarget(ro), spu_reg_name[ra]));
	}
	virtual void CWX(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("cwx %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}
	virtual void ROTQBY(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("rotqby %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], rb));
	}
	virtual void ROTQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("rotqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SHLQBYI(OP_REG rt, OP_REG ra, OP_sIMM i7)
	{
		Write(wxString::Format("shlqbyi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i7));
	}
	virtual void SPU_NOP(OP_REG rt)
	{
		Write(wxString::Format("nop %s", spu_reg_name[rt]));
	}
	virtual void CLGT(OP_REG rt, OP_REG ra, OP_REG rb)
	{
		Write(wxString::Format("clgt %s,%s,%s", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]));
	}

	//0 - 8
	virtual void BRZ(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("brz %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
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
	virtual void IL(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("il %s,%d", spu_reg_name[rt], i16));
	}
	virtual void LQR(OP_REG rt, OP_sIMM i16)
	{
		Write(wxString::Format("lqr %s,0x%x", spu_reg_name[rt], DisAsmBranchTarget(i16)));
	}

	//0 - 7
	virtual void SPU_ORI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ori %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
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
	virtual void CLGTI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("clgti %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CLGTHI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("clgthi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}
	virtual void CEQI(OP_REG rt, OP_REG ra, OP_sIMM i10)
	{
		Write(wxString::Format("ceqi %s,%s,%d", spu_reg_name[rt], spu_reg_name[ra], i10));
	}

	//0 - 6
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

	virtual void UNK(const s32 code, const s32 opcode, const s32 gcode)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%08x)", code));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP