#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "SPUThread.h"
#include "SPUInstrTable.h"
#include "SPUInterpreter.h"
#include "SPUInterpreter2.h"

void spu_interpreter::DEFAULT(SPUThread& CPU, spu_opcode_t op)
{
	SPUInterpreter inter(CPU); (*SPU_instr::rrr_list)(&inter, op.opcode);
}


void spu_interpreter::STOP(SPUThread& CPU, spu_opcode_t op)
{
	CPU.stop_and_signal(op.opcode & 0x3fff);
}

void spu_interpreter::LNOP(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::SYNC(SPUThread& CPU, spu_opcode_t op)
{
	_mm_mfence();
}

void spu_interpreter::DSYNC(SPUThread& CPU, spu_opcode_t op)
{
	_mm_mfence();
}

void spu_interpreter::MFSPR(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].clear();
}

void spu_interpreter::RDCH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(CPU.get_ch_value(op.ra));
}

void spu_interpreter::RCHCNT(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(CPU.get_ch_count(op.ra));
}

void spu_interpreter::SF(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::OR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BG(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::NOR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ABSDB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTHM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTMAH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTMI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTMAI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTHMI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTMAHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::A(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AND(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CG(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::NAND(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AVGB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MTSPR(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::WRCH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.set_ch_value(op.ra, CPU.GPR[op.rt]._u32[3]);
}

void spu_interpreter::BIZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BINZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BIHZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BIHNZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STOPD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BISL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::IRET(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BISLED(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HBR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::GB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::GBH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::GBB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSMH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSMB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FREST(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FRSQEST(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBYBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBYBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBYBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CBX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CHX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CWX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CDX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBY(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBY(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBY(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CBD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CHD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CWD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CDD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBYI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBYI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBYI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::NOP(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XOR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::EQV(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SUMB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSWD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSHW(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CNTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSBH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDC(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORC(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCMGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCMGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HLGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFNMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFNMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ADDX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BGX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHAU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSCRRD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FESD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FRDS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSCRWR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFTSV(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPY(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCMEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCMEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::CFLTS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CFLTU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CSFLT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CUFLT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::BRZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRNZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRHZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRHNZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRASL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSMBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRSL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::IL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILHU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::IOHL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::ORI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HLGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYUI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HEQI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::HBRA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HBRR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::SELB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHUFB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FNMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::UNK(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}
