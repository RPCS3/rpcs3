#pragma once

#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Cell/PPCDisAsm.h"

static const char* spu_reg_name[128] =
{
	"$LR", "$SP", "$2", "$3", "$4", "$5", "$6", "$7",
	"$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15",
	"$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
	"$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31",
	"$32", "$33", "$34", "$35", "$36", "$37", "$38", "$39",
	"$40", "$41", "$42", "$43", "$44", "$45", "$46", "$47",
	"$48", "$49", "$50", "$51", "$52", "$53", "$54", "$55",
	"$56", "$57", "$58", "$59", "$60", "$61", "$62", "$63",
	"$64", "$65", "$66", "$67", "$68", "$69", "$70", "$71",
	"$72", "$73", "$74", "$75", "$76", "$77", "$78", "$79",
	"$80", "$81", "$82", "$83", "$84", "$85", "$86", "$87",
	"$88", "$89", "$90", "$91", "$92", "$93", "$94", "$95",
	"$96", "$97", "$98", "$99", "$100", "$101", "$102", "$103",
	"$104", "$105", "$106", "$107", "$108", "$109", "$110", "$111",
	"$112", "$113", "$114", "$115", "$116", "$117", "$118", "$119",
	"$120", "$121", "$122", "$123", "$124", "$125", "$126", "$127",
};

static const char* spu_spreg_name[128] =
{
	"$sp0", "$sp1", "$sp2", "$sp3", "$sp4", "$sp5", "$sp6", "$sp7",
	"$sp8", "$sp9", "$sp10", "$sp11", "$sp12", "$sp13", "$sp14", "$sp15",
	"$sp16", "$sp17", "$sp18", "$sp19", "$sp20", "$sp21", "$sp22", "$sp23",
	"$sp24", "$sp25", "$sp26", "$sp27", "$sp28", "$sp29", "$sp30", "$sp31",
	"$sp32", "$sp33", "$sp34", "$sp35", "$sp36", "$sp37", "$sp38", "$sp39",
	"$sp40", "$sp41", "$sp42", "$sp43", "$sp44", "$sp45", "$sp46", "$sp47",
	"$sp48", "$sp49", "$sp50", "$sp51", "$sp52", "$sp53", "$sp54", "$sp55",
	"$sp56", "$sp57", "$sp58", "$sp59", "$sp60", "$sp61", "$sp62", "$sp63",
	"$sp64", "$sp65", "$sp66", "$sp67", "$sp68", "$sp69", "$sp70", "$sp71",
	"$sp72", "$sp73", "$sp74", "$sp75", "$sp76", "$sp77", "$sp78", "$sp79",
	"$sp80", "$sp81", "$sp82", "$sp83", "$sp84", "$sp85", "$sp86", "$sp87",
	"$sp88", "$sp89", "$sp90", "$sp91", "$sp92", "$sp93", "$sp94", "$sp95",
	"$sp96", "$sp97", "$sp98", "$sp99", "$sp100", "$sp101", "$sp102", "$sp103",
	"$sp104", "$sp105", "$sp106", "$sp107", "$sp108", "$sp109", "$sp110", "$sp111",
	"$sp112", "$sp113", "$sp114", "$sp115", "$sp116", "$sp117", "$sp118", "$sp119",
	"$sp120", "$sp121", "$sp122", "$sp123", "$sp124", "$sp125", "$sp126", "$sp127",
};

static const char* spu_ch_name[128] =
{
	"$SPU_RdEventStat", "$SPU_WrEventMask", "$SPU_WrEventAck", "$SPU_RdSigNotify1",
	"$SPU_RdSigNotify2", "$ch5", "$ch6", "$SPU_WrDec", "$SPU_RdDec",
	"$MFC_WrMSSyncReq", "$ch10", "$SPU_RdEventMask", "$MFC_RdTagMask", "$SPU_RdMachStat",
	"$SPU_WrSRR0", "$SPU_RdSRR0", "$MFC_LSA", "$MFC_EAH", "$MFC_EAL", "$MFC_Size",
	"$MFC_TagID", "$MFC_Cmd", "$MFC_WrTagMask", "$MFC_WrTagUpdate", "$MFC_RdTagStat",
	"$MFC_RdListStallStat", "$MFC_WrListStallAck", "$MFC_RdAtomicStat",
	"$SPU_WrOutMbox", "$SPU_RdInMbox", "$SPU_WrOutIntrMbox", "$ch31", "$ch32",
	"$ch33", "$ch34", "$ch35", "$ch36", "$ch37", "$ch38", "$ch39", "$ch40",
	"$ch41", "$ch42", "$ch43", "$ch44", "$ch45", "$ch46", "$ch47", "$ch48",
	"$ch49", "$ch50", "$ch51", "$ch52", "$ch53", "$ch54", "$ch55", "$ch56",
	"$ch57", "$ch58", "$ch59", "$ch60", "$ch61", "$ch62", "$ch63", "$ch64",
	"$ch65", "$ch66", "$ch67", "$ch68", "$ch69", "$ch70", "$ch71", "$ch72",
	"$ch73", "$ch74", "$ch75", "$ch76", "$ch77", "$ch78", "$ch79", "$ch80",
	"$ch81", "$ch82", "$ch83", "$ch84", "$ch85", "$ch86", "$ch87", "$ch88",
	"$ch89", "$ch90", "$ch91", "$ch92", "$ch93", "$ch94", "$ch95", "$ch96",
	"$ch97", "$ch98", "$ch99", "$ch100", "$ch101", "$ch102", "$ch103", "$ch104",
	"$ch105", "$ch106", "$ch107", "$ch108", "$ch109", "$ch110", "$ch111", "$ch112",
	"$ch113", "$ch114", "$ch115", "$ch116", "$ch117", "$ch118", "$ch119", "$ch120",
	"$ch121", "$ch122", "$ch123", "$ch124", "$ch125", "$ch126", "$ch127",
};

class SPUDisAsm 
	: public SPUOpcodes
	, public PPCDisAsm
{
public:
	SPUDisAsm(CPUDisAsmMode mode) : PPCDisAsm(mode)
	{
	}

	~SPUDisAsm()
	{
	}

private:
	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return branchTarget(dump_pc, imm);
	}

private:
	std::string& FixOp(std::string& op)
	{
		op.append(std::max<int>(10 - (int)op.length(), 0),' ');
		return op;
	}
	void DisAsm(const char* op)
	{
		Write(op);
	}
	void DisAsm(std::string op, u32 a1)
	{
		Write(fmt::Format("%s 0x%x", FixOp(op).c_str(), a1));
	}
	void DisAsm(std::string op, const char* a1)
	{
		Write(fmt::Format("%s %s", FixOp(op).c_str(), a1));
	}
	void DisAsm(std::string op, const char* a1, const char* a2)
	{
		Write(fmt::Format("%s %s,%s", FixOp(op).c_str(), a1, a2));
	}
	void DisAsm(std::string op, int a1, const char* a2)
	{
		Write(fmt::Format("%s 0x%x,%s", FixOp(op).c_str(), a1, a2));
	}
	void DisAsm(std::string op, const char* a1, int a2)
	{
		Write(fmt::Format("%s %s,0x%x", FixOp(op).c_str(), a1, a2));
	}
	void DisAsm(std::string op, int a1, int a2)
	{
		Write(fmt::Format("%s 0x%x,0x%x", FixOp(op).c_str(), a1, a2));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, const char* a3)
	{
		Write(fmt::Format("%s %s,%s,%s", FixOp(op).c_str(), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, int a2, const char* a3)
	{
		Write(fmt::Format("%s %s,0x%x(%s)", FixOp(op).c_str(), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, int a3)
	{
		Write(fmt::Format("%s %s,%s,0x%x", FixOp(op).c_str(), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, const char* a3, const char* a4)
	{
		Write(fmt::Format("%s %s,%s,%s,%s", FixOp(op).c_str(), a1, a2, a3, a4));
	}
	//0 - 10
	void STOP(u32 code)
	{
		DisAsm("stop", code);
	}
	void LNOP()
	{
		DisAsm("lnop");
	}
	void SYNC(u32 Cbit)
	{
		DisAsm("sync", Cbit);
	}
	void DSYNC()
	{
		DisAsm("dsync");
	}
	void MFSPR(u32 rt, u32 sa)
	{
		DisAsm("mfspr", spu_reg_name[rt], spu_spreg_name[sa]);
	}
	void RDCH(u32 rt, u32 ra)
	{
		DisAsm("rdch", spu_reg_name[rt], spu_ch_name[ra]);
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		DisAsm("rchcnt", spu_reg_name[rt], spu_ch_name[ra]);
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("sf", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void OR(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("or", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void BG(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("bg", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SFH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("sfh", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void NOR(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("nor", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ABSDB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("absdb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rot", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTM(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotm", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTMA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotma", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SHL(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("shl", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("roth", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTHM(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rothm", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTMAH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotmah", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SHLH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("shlh", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("roti", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTMI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotmi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotmai", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("shli", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rothi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rothmi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotmahi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("shlhi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void A(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("a", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void AND(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("and", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CG(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cg", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void AH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("ah", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void NAND(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("nand", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void AVGB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("avgb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MTSPR(u32 rt, u32 sa)
	{
		DisAsm("mtspr", spu_spreg_name[sa], spu_reg_name[rt]);
	}
	void WRCH(u32 ra, u32 rt)
	{
		DisAsm("wrch", spu_ch_name[ra], spu_reg_name[rt]);
	}
	void BIZ(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("biz", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void BINZ(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("binz", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void BIHZ(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("bihz", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void BIHNZ(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("bihnz", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		DisAsm("bihnz", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("stqx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void BI(u32 intr, u32 ra)
	{
		DisAsm("bi", spu_reg_name[ra]);
	}
	void BISL(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("bisl", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void IRET(u32 ra)
	{
		DisAsm("iret", spu_reg_name[ra]);
	}
	void BISLED(u32 intr, u32 rt, u32 ra)
	{
		DisAsm("bisled", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void HBR(u32 p, u32 ro, u32 ra)
	{
		DisAsm("hbr", DisAsmBranchTarget(ro), spu_reg_name[ra]);
	}
	void GB(u32 rt, u32 ra)
	{
		DisAsm("gb", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void GBH(u32 rt, u32 ra)
	{
		DisAsm("gbh", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void GBB(u32 rt, u32 ra)
	{
		DisAsm("gbb", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FSM(u32 rt, u32 ra)
	{
		DisAsm("fsm", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FSMH(u32 rt, u32 ra)
	{
		DisAsm("fsmh", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FSMB(u32 rt, u32 ra)
	{
		DisAsm("fsmb", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FREST(u32 rt, u32 ra)
	{
		DisAsm("frest", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		DisAsm("frsqest", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("lqx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqbybi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqmbybi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("shlqbybi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cbx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("chx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cwx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cdx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqbi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqmbi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("shlqbi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqby", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("rotqmby", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("shlqby", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ORX(u32 rt, u32 ra)
	{
		DisAsm("orx", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void CBD(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("cbd", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("chd", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("cwd", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("cdd", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotqbii", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotqmbii", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("shlqbii", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotqbyi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("rotqmbyi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("shlqbyi", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void NOP(u32 rt)
	{
		DisAsm("nop", spu_reg_name[rt]);
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("xor", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cgth", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("eqv", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cgtb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("sumb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void HGT(u32 rt, s32 ra, s32 rb)
	{
		DisAsm("hgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CLZ(u32 rt, u32 ra)
	{
		DisAsm("clz", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void XSWD(u32 rt, u32 ra)
	{
		DisAsm("xswd", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void XSHW(u32 rt, u32 ra)
	{
		DisAsm("xshw", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void CNTB(u32 rt, u32 ra)
	{
		DisAsm("cntb", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void XSBH(u32 rt, u32 ra)
	{
		DisAsm("xsbh", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void CLGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("clgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ANDC(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("andc", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FCGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fcgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfcgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fa", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fs", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fm", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CLGTH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("clgth", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ORC(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("orc", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FCMGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fcmgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfcmgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfa", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfs", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfm", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("clgtb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("hlgt", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFMA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfma", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFMS(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfms", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFNMS(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfnms", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFNMA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfnma", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("ceq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyhhu", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void ADDX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("addx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void SFX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("sfx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CGX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("cgx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void BGX(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("bgx", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYHHA(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyhha", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyhhau", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FSCRRD(u32 rt)
	{
		DisAsm("fscrrd", spu_reg_name[rt]);
	}
	void FESD(u32 rt, u32 ra)
	{
		DisAsm("fesd", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FRDS(u32 rt, u32 ra)
	{
		DisAsm("frds", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		DisAsm("fscrwr", spu_reg_name[rt], spu_reg_name[ra]);
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		DisAsm("dftsv", spu_reg_name[rt], spu_reg_name[ra], i7);
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fceq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfceq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpy", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyh", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyhh", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpys", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("ceqh", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fcmeq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("dfcmeq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("mpyu", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("ceqb", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void FI(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("fi", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		DisAsm("heq", spu_reg_name[rt], spu_reg_name[ra], spu_reg_name[rb]);
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		DisAsm("cflts", spu_reg_name[rt], spu_reg_name[ra], i8);
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		DisAsm("cfltu", spu_reg_name[rt], spu_reg_name[ra], i8);
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		DisAsm("csflt", spu_reg_name[rt], spu_reg_name[ra], i8);
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		DisAsm("cuflt", spu_reg_name[rt], spu_reg_name[ra], i8);
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		DisAsm("brz", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void STQA(u32 rt, s32 i16)
	{
		DisAsm("stqa", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BRNZ(u32 rt, s32 i16)
	{
		DisAsm("brnz", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BRHZ(u32 rt, s32 i16)
	{
		DisAsm("brhz", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		DisAsm("brhnz", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void STQR(u32 rt, s32 i16)
	{
		DisAsm("stqr", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BRA(s32 i16)
	{
		DisAsm("bra", DisAsmBranchTarget(i16));
	}
	void LQA(u32 rt, s32 i16)
	{
		DisAsm("lqa", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BRASL(u32 rt, s32 i16)
	{
		DisAsm("brasl", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void BR(s32 i16)
	{
		DisAsm("br", DisAsmBranchTarget(i16));
	}
	void FSMBI(u32 rt, s32 i16)
	{
		DisAsm("fsmbi", spu_reg_name[rt], i16);
	}
	void BRSL(u32 rt, s32 i16)
	{
		DisAsm("brsl", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void LQR(u32 rt, s32 i16)
	{
		DisAsm("lqr", spu_reg_name[rt], DisAsmBranchTarget(i16));
	}
	void IL(u32 rt, s32 i16)
	{
		DisAsm("il", spu_reg_name[rt], i16);
	}
	void ILHU(u32 rt, s32 i16)
	{
		DisAsm("ilhu", spu_reg_name[rt], i16);
	}
	void ILH(u32 rt, s32 i16)
	{
		DisAsm("ilh", spu_reg_name[rt], i16);
	}
	void IOHL(u32 rt, s32 i16)
	{
		DisAsm("iolh", spu_reg_name[rt], i16);
	}

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ori", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("orhi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("orbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void SFI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("sfi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void SFHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("sfhi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("andi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("andhi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("andbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ai", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ahi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void STQD(u32 rt, s32 i10, u32 ra)
	{
		DisAsm("stqd", spu_reg_name[rt], i10, spu_reg_name[ra]);
	}
	void LQD(u32 rt, s32 i10, u32 ra)
	{
		DisAsm("lqd", spu_reg_name[rt], i10, spu_reg_name[ra]);
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("xori", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("xorhi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("xorbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("cgti", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("cgthi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("cgtbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("hgti", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("clgti", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("clgthi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("clgtbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("hlgti", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("mpyi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("mpyui", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ceqi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ceqhi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("ceqbi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		DisAsm("heqi", spu_reg_name[rt], spu_reg_name[ra], i10);
	}

	//0 - 6
	void HBRA(s32 ro, s32 i16)
	{
		DisAsm("hbra", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16));
	}
	void HBRR(s32 ro, s32 i16)
	{
		DisAsm("hbrr", DisAsmBranchTarget(ro), DisAsmBranchTarget(i16));
	}
	void ILA(u32 rt, u32 i18)
	{
		DisAsm("ila", spu_reg_name[rt], i18);
	}

	//0 - 3
	void SELB(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("selb", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}
	void SHUFB(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("shufb", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}
	void MPYA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("mpya", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}
	void FNMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("fnms", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}
	void FMA(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("fma", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}
	void FMS(u32 rc, u32 ra, u32 rb, u32 rt)
	{
		DisAsm("fms", spu_reg_name[rc], spu_reg_name[ra], spu_reg_name[rb], spu_reg_name[rt]);
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		Write(fmt::Format("Unknown/Illegal opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}
};
