#pragma once

#include "PPCDisAsm.h"
#include "SPUOpcodes.h"

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

class SPUDisAsm final : public PPCDisAsm
{
public:
	SPUDisAsm(CPUDisAsmMode mode) : PPCDisAsm(mode)
	{
	}

	~SPUDisAsm()
	{
	}

private:
	virtual u32 DisAsmBranchTarget(const s32 imm) override
	{
		return spu_branch_target(dump_pc, imm);
	}

	static const char* BrIndirectSuffix(u32 de)
	{
		switch (de)
		{
		case 0b01: return "e";
		case 0b10: return "d";
		//case 0b11: return "(undef)";
		default: return "";
		}
	}

private:
	std::string& FixOp(std::string& op)
	{
		op.append(std::max<int>(10 - ::narrow<int>(op.size()), 0),' ');
		return op;
	}
	void DisAsm(const char* op)
	{
		Write(op);
	}
	void DisAsm(std::string op, u32 a1)
	{
		Write(fmt::format("%s 0x%x", FixOp(op), a1));
	}
	void DisAsm(std::string op, const char* a1)
	{
		Write(fmt::format("%s %s", FixOp(op), a1));
	}
	void DisAsm(std::string op, const char* a1, const char* a2)
	{
		Write(fmt::format("%s %s,%s", FixOp(op), a1, a2));
	}
	void DisAsm(std::string op, int a1, const char* a2)
	{
		Write(fmt::format("%s 0x%x,%s", FixOp(op), a1, a2));
	}
	void DisAsm(std::string op, const char* a1, int a2)
	{
		Write(fmt::format("%s %s,0x%x", FixOp(op), a1, a2));
	}
	void DisAsm(std::string op, int a1, int a2)
	{
		Write(fmt::format("%s 0x%x,0x%x", FixOp(op), a1, a2));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, const char* a3)
	{
		Write(fmt::format("%s %s,%s,%s", FixOp(op), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, int a2, const char* a3)
	{
		Write(fmt::format("%s %s,0x%x(%s)", FixOp(op), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, int a3)
	{
		Write(fmt::format("%s %s,%s,0x%x", FixOp(op), a1, a2, a3));
	}
	void DisAsm(std::string op, const char* a1, const char* a2, const char* a3, const char* a4)
	{
		Write(fmt::format("%s %s,%s,%s,%s", FixOp(op), a1, a2, a3, a4));
	}

	using field_de_t = decltype(spu_opcode_t::de);
	void DisAsm(std::string op, field_de_t de, const char* a1)
	{
		Write(fmt::format("%s %s", FixOp(op.append(BrIndirectSuffix(de))), a1));
	}
	void DisAsm(std::string op, field_de_t de, const char* a1, const char* a2)
	{
		Write(fmt::format("%s %s", FixOp(op.append(BrIndirectSuffix(de))), a1, a2));
	}

public:
	u32 disasm(u32 pc) override;

	//0 - 10
	void STOP(spu_opcode_t op)
	{
		op.rb ? UNK(op) : DisAsm("stop", op.opcode & 0x3fff);
	}
	void LNOP(spu_opcode_t /*op*/)
	{
		DisAsm("lnop");
	}
	void SYNC(spu_opcode_t op)
	{
		DisAsm(op.c ? "syncc" : "sync");
	}
	void DSYNC(spu_opcode_t /*op*/)
	{
		DisAsm("dsync");
	}
	void MFSPR(spu_opcode_t op)
	{
		DisAsm("mfspr", spu_reg_name[op.rt], spu_spreg_name[op.ra]);
	}
	void RDCH(spu_opcode_t op)
	{
		DisAsm("rdch", spu_reg_name[op.rt], spu_ch_name[op.ra]);
	}
	void RCHCNT(spu_opcode_t op)
	{
		DisAsm("rchcnt", spu_reg_name[op.rt], spu_ch_name[op.ra]);
	}
	void SF(spu_opcode_t op)
	{
		DisAsm("sf", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void OR(spu_opcode_t op)
	{
		DisAsm("or", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void BG(spu_opcode_t op)
	{
		DisAsm("bg", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SFH(spu_opcode_t op)
	{
		DisAsm("sfh", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void NOR(spu_opcode_t op)
	{
		DisAsm("nor", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ABSDB(spu_opcode_t op)
	{
		DisAsm("absdb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROT(spu_opcode_t op)
	{
		DisAsm("rot", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTM(spu_opcode_t op)
	{
		DisAsm("rotm", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTMA(spu_opcode_t op)
	{
		DisAsm("rotma", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SHL(spu_opcode_t op)
	{
		DisAsm("shl", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTH(spu_opcode_t op)
	{
		DisAsm("roth", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTHM(spu_opcode_t op)
	{
		DisAsm("rothm", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTMAH(spu_opcode_t op)
	{
		DisAsm("rotmah", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SHLH(spu_opcode_t op)
	{
		DisAsm("shlh", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTI(spu_opcode_t op)
	{
		DisAsm("roti", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTMI(spu_opcode_t op)
	{
		DisAsm("rotmi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTMAI(spu_opcode_t op)
	{
		DisAsm("rotmai", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void SHLI(spu_opcode_t op)
	{
		DisAsm("shli", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTHI(spu_opcode_t op)
	{
		DisAsm("rothi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTHMI(spu_opcode_t op)
	{
		DisAsm("rothmi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTMAHI(spu_opcode_t op)
	{
		DisAsm("rotmahi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void SHLHI(spu_opcode_t op)
	{
		DisAsm("shlhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void A(spu_opcode_t op)
	{
		DisAsm("a", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void AND(spu_opcode_t op)
	{
		DisAsm("and", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CG(spu_opcode_t op)
	{
		DisAsm("cg", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void AH(spu_opcode_t op)
	{
		DisAsm("ah", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void NAND(spu_opcode_t op)
	{
		DisAsm("nand", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void AVGB(spu_opcode_t op)
	{
		DisAsm("avgb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MTSPR(spu_opcode_t op)
	{
		DisAsm("mtspr", spu_spreg_name[op.ra], spu_reg_name[op.rt]);
	}
	void WRCH(spu_opcode_t op)
	{
		DisAsm("wrch", spu_ch_name[op.ra], spu_reg_name[op.rt]);
	}
	void BIZ(spu_opcode_t op)
	{
		DisAsm("biz", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void BINZ(spu_opcode_t op)
	{
		DisAsm("binz", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void BIHZ(spu_opcode_t op)
	{
		DisAsm("bihz", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void BIHNZ(spu_opcode_t op)
	{
		DisAsm("bihnz", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void STOPD(spu_opcode_t op)
	{
		DisAsm("stopd", spu_reg_name[op.rc], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void STQX(spu_opcode_t op)
	{
		DisAsm("stqx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void BI(spu_opcode_t op)
	{
		DisAsm("bi", op.de, spu_reg_name[op.ra]);
	}
	void BISL(spu_opcode_t op)
	{
		DisAsm("bisl", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void IRET(spu_opcode_t op)
	{
		DisAsm("iret", op.de, spu_reg_name[op.ra]);
	}
	void BISLED(spu_opcode_t op)
	{
		DisAsm("bisled", op.de, spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void HBR(spu_opcode_t op)
	{
		DisAsm(op.c ? "hbrp" : "hbr", DisAsmBranchTarget((op.roh << 7) | op.rt), spu_reg_name[op.ra]);
	}
	void GB(spu_opcode_t op)
	{
		DisAsm("gb", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void GBH(spu_opcode_t op)
	{
		DisAsm("gbh", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void GBB(spu_opcode_t op)
	{
		DisAsm("gbb", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FSM(spu_opcode_t op)
	{
		DisAsm("fsm", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FSMH(spu_opcode_t op)
	{
		DisAsm("fsmh", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FSMB(spu_opcode_t op)
	{
		DisAsm("fsmb", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FREST(spu_opcode_t op)
	{
		DisAsm("frest", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FRSQEST(spu_opcode_t op)
	{
		DisAsm("frsqest", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void LQX(spu_opcode_t op)
	{
		DisAsm("lqx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQBYBI(spu_opcode_t op)
	{
		DisAsm("rotqbybi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQMBYBI(spu_opcode_t op)
	{
		DisAsm("rotqmbybi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SHLQBYBI(spu_opcode_t op)
	{
		DisAsm("shlqbybi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CBX(spu_opcode_t op)
	{
		DisAsm("cbx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CHX(spu_opcode_t op)
	{
		DisAsm("chx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CWX(spu_opcode_t op)
	{
		DisAsm("cwx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CDX(spu_opcode_t op)
	{
		DisAsm("cdx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQBI(spu_opcode_t op)
	{
		DisAsm("rotqbi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQMBI(spu_opcode_t op)
	{
		DisAsm("rotqmbi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SHLQBI(spu_opcode_t op)
	{
		DisAsm("shlqbi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQBY(spu_opcode_t op)
	{
		DisAsm("rotqby", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ROTQMBY(spu_opcode_t op)
	{
		DisAsm("rotqmby", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SHLQBY(spu_opcode_t op)
	{
		DisAsm("shlqby", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ORX(spu_opcode_t op)
	{
		DisAsm("orx", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void CBD(spu_opcode_t op)
	{
		DisAsm("cbd", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void CHD(spu_opcode_t op)
	{
		DisAsm("chd", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void CWD(spu_opcode_t op)
	{
		DisAsm("cwd", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void CDD(spu_opcode_t op)
	{
		DisAsm("cdd", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTQBII(spu_opcode_t op)
	{
		DisAsm("rotqbii", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTQMBII(spu_opcode_t op)
	{
		DisAsm("rotqmbii", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void SHLQBII(spu_opcode_t op)
	{
		DisAsm("shlqbii", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTQBYI(spu_opcode_t op)
	{
		DisAsm("rotqbyi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void ROTQMBYI(spu_opcode_t op)
	{
		DisAsm("rotqmbyi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void SHLQBYI(spu_opcode_t op)
	{
		DisAsm("shlqbyi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void NOP(spu_opcode_t op)
	{
		DisAsm("nop", spu_reg_name[op.rt]);
	}
	void CGT(spu_opcode_t op)
	{
		DisAsm("cgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void XOR(spu_opcode_t op)
	{
		DisAsm("xor", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CGTH(spu_opcode_t op)
	{
		DisAsm("cgth", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void EQV(spu_opcode_t op)
	{
		DisAsm("eqv", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CGTB(spu_opcode_t op)
	{
		DisAsm("cgtb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SUMB(spu_opcode_t op)
	{
		DisAsm("sumb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void HGT(spu_opcode_t op)
	{
		DisAsm("hgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CLZ(spu_opcode_t op)
	{
		DisAsm("clz", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void XSWD(spu_opcode_t op)
	{
		DisAsm("xswd", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void XSHW(spu_opcode_t op)
	{
		DisAsm("xshw", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void CNTB(spu_opcode_t op)
	{
		DisAsm("cntb", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void XSBH(spu_opcode_t op)
	{
		DisAsm("xsbh", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void CLGT(spu_opcode_t op)
	{
		DisAsm("clgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ANDC(spu_opcode_t op)
	{
		DisAsm("andc", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FCGT(spu_opcode_t op)
	{
		DisAsm("fcgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFCGT(spu_opcode_t op)
	{
		DisAsm("dfcgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FA(spu_opcode_t op)
	{
		DisAsm("fa", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FS(spu_opcode_t op)
	{
		DisAsm("fs", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FM(spu_opcode_t op)
	{
		DisAsm("fm", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CLGTH(spu_opcode_t op)
	{
		DisAsm("clgth", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ORC(spu_opcode_t op)
	{
		DisAsm("orc", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FCMGT(spu_opcode_t op)
	{
		DisAsm("fcmgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFCMGT(spu_opcode_t op)
	{
		DisAsm("dfcmgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFA(spu_opcode_t op)
	{
		DisAsm("dfa", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFS(spu_opcode_t op)
	{
		DisAsm("dfs", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFM(spu_opcode_t op)
	{
		DisAsm("dfm", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CLGTB(spu_opcode_t op)
	{
		DisAsm("clgtb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void HLGT(spu_opcode_t op)
	{
		DisAsm("hlgt", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFMA(spu_opcode_t op)
	{
		DisAsm("dfma", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFMS(spu_opcode_t op)
	{
		DisAsm("dfms", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFNMS(spu_opcode_t op)
	{
		DisAsm("dfnms", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFNMA(spu_opcode_t op)
	{
		DisAsm("dfnma", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CEQ(spu_opcode_t op)
	{
		DisAsm("ceq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYHHU(spu_opcode_t op)
	{
		DisAsm("mpyhhu", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void ADDX(spu_opcode_t op)
	{
		DisAsm("addx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void SFX(spu_opcode_t op)
	{
		DisAsm("sfx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CGX(spu_opcode_t op)
	{
		DisAsm("cgx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void BGX(spu_opcode_t op)
	{
		DisAsm("bgx", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYHHA(spu_opcode_t op)
	{
		DisAsm("mpyhha", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYHHAU(spu_opcode_t op)
	{
		DisAsm("mpyhhau", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FSCRRD(spu_opcode_t op)
	{
		DisAsm("fscrrd", spu_reg_name[op.rt]);
	}
	void FESD(spu_opcode_t op)
	{
		DisAsm("fesd", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FRDS(spu_opcode_t op)
	{
		DisAsm("frds", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void FSCRWR(spu_opcode_t op)
	{
		DisAsm("fscrwr", spu_reg_name[op.rt], spu_reg_name[op.ra]);
	}
	void DFTSV(spu_opcode_t op)
	{
		DisAsm("dftsv", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si7);
	}
	void FCEQ(spu_opcode_t op)
	{
		DisAsm("fceq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFCEQ(spu_opcode_t op)
	{
		DisAsm("dfceq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPY(spu_opcode_t op)
	{
		DisAsm("mpy", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYH(spu_opcode_t op)
	{
		DisAsm("mpyh", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYHH(spu_opcode_t op)
	{
		DisAsm("mpyhh", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYS(spu_opcode_t op)
	{
		DisAsm("mpys", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CEQH(spu_opcode_t op)
	{
		DisAsm("ceqh", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FCMEQ(spu_opcode_t op)
	{
		DisAsm("fcmeq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void DFCMEQ(spu_opcode_t op)
	{
		DisAsm("dfcmeq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void MPYU(spu_opcode_t op)
	{
		DisAsm("mpyu", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void CEQB(spu_opcode_t op)
	{
		DisAsm("ceqb", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void FI(spu_opcode_t op)
	{
		DisAsm("fi", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}
	void HEQ(spu_opcode_t op)
	{
		DisAsm("heq", spu_reg_name[op.rt], spu_reg_name[op.ra], spu_reg_name[op.rb]);
	}

	//0 - 9
	void CFLTS(spu_opcode_t op)
	{
		DisAsm("cflts", spu_reg_name[op.rt], spu_reg_name[op.ra], op.i8);
	}
	void CFLTU(spu_opcode_t op)
	{
		DisAsm("cfltu", spu_reg_name[op.rt], spu_reg_name[op.ra], op.i8);
	}
	void CSFLT(spu_opcode_t op)
	{
		DisAsm("csflt", spu_reg_name[op.rt], spu_reg_name[op.ra], op.i8);
	}
	void CUFLT(spu_opcode_t op)
	{
		DisAsm("cuflt", spu_reg_name[op.rt], spu_reg_name[op.ra], op.i8);
	}

	//0 - 8
	void BRZ(spu_opcode_t op)
	{
		DisAsm("brz", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void STQA(spu_opcode_t op)
	{
		DisAsm("stqa", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16 - dump_pc / 4));
	}
	void BRNZ(spu_opcode_t op)
	{
		DisAsm("brnz", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void BRHZ(spu_opcode_t op)
	{
		DisAsm("brhz", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void BRHNZ(spu_opcode_t op)
	{
		DisAsm("brhnz", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void STQR(spu_opcode_t op)
	{
		DisAsm("stqr", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void BRA(spu_opcode_t op)
	{
		DisAsm("bra", DisAsmBranchTarget(op.i16 - dump_pc / 4));
	}
	void LQA(spu_opcode_t op)
	{
		DisAsm("lqa", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16 - dump_pc / 4));
	}
	void BRASL(spu_opcode_t op)
	{
		DisAsm("brasl", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16 - dump_pc / 4));
	}
	void BR(spu_opcode_t op)
	{
		DisAsm("br", DisAsmBranchTarget(op.i16));
	}
	void FSMBI(spu_opcode_t op)
	{
		DisAsm("fsmbi", spu_reg_name[op.rt], op.i16);
	}
	void BRSL(spu_opcode_t op)
	{
		DisAsm("brsl", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void LQR(spu_opcode_t op)
	{
		DisAsm("lqr", spu_reg_name[op.rt], DisAsmBranchTarget(op.i16));
	}
	void IL(spu_opcode_t op)
	{
		DisAsm("il", spu_reg_name[op.rt], op.si16);
	}
	void ILHU(spu_opcode_t op)
	{
		DisAsm("ilhu", spu_reg_name[op.rt], op.i16);
	}
	void ILH(spu_opcode_t op)
	{
		DisAsm("ilh", spu_reg_name[op.rt], op.i16);
	}
	void IOHL(spu_opcode_t op)
	{
		DisAsm("iohl", spu_reg_name[op.rt], op.i16);
	}

	//0 - 7
	void ORI(spu_opcode_t op)
	{
		DisAsm("ori", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ORHI(spu_opcode_t op)
	{
		DisAsm("orhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ORBI(spu_opcode_t op)
	{
		DisAsm("orbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void SFI(spu_opcode_t op)
	{
		DisAsm("sfi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void SFHI(spu_opcode_t op)
	{
		DisAsm("sfhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ANDI(spu_opcode_t op)
	{
		DisAsm("andi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ANDHI(spu_opcode_t op)
	{
		DisAsm("andhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ANDBI(spu_opcode_t op)
	{
		DisAsm("andbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void AI(spu_opcode_t op)
	{
		DisAsm("ai", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void AHI(spu_opcode_t op)
	{
		DisAsm("ahi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void STQD(spu_opcode_t op)
	{
		DisAsm("stqd", spu_reg_name[op.rt], op.si10 << 4, spu_reg_name[op.ra]);
	}
	void LQD(spu_opcode_t op)
	{
		DisAsm("lqd", spu_reg_name[op.rt], op.si10 << 4, spu_reg_name[op.ra]);
	}
	void XORI(spu_opcode_t op)
	{
		DisAsm("xori", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void XORHI(spu_opcode_t op)
	{
		DisAsm("xorhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void XORBI(spu_opcode_t op)
	{
		DisAsm("xorbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CGTI(spu_opcode_t op)
	{
		DisAsm("cgti", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CGTHI(spu_opcode_t op)
	{
		DisAsm("cgthi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CGTBI(spu_opcode_t op)
	{
		DisAsm("cgtbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void HGTI(spu_opcode_t op)
	{
		DisAsm("hgti", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CLGTI(spu_opcode_t op)
	{
		DisAsm("clgti", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CLGTHI(spu_opcode_t op)
	{
		DisAsm("clgthi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CLGTBI(spu_opcode_t op)
	{
		DisAsm("clgtbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void HLGTI(spu_opcode_t op)
	{
		DisAsm("hlgti", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void MPYI(spu_opcode_t op)
	{
		DisAsm("mpyi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void MPYUI(spu_opcode_t op)
	{
		DisAsm("mpyui", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CEQI(spu_opcode_t op)
	{
		DisAsm("ceqi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CEQHI(spu_opcode_t op)
	{
		DisAsm("ceqhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void CEQBI(spu_opcode_t op)
	{
		DisAsm("ceqbi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void HEQI(spu_opcode_t op)
	{
		DisAsm("heqi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}

	//0 - 6
	void HBRA(spu_opcode_t op)
	{
		DisAsm("hbra", DisAsmBranchTarget((op.r0h << 7) | op.rt), DisAsmBranchTarget(op.i16 - dump_pc / 4));
	}
	void HBRR(spu_opcode_t op)
	{
		DisAsm("hbrr", DisAsmBranchTarget((op.r0h << 7) | op.rt), DisAsmBranchTarget(op.i16));
	}
	void ILA(spu_opcode_t op)
	{
		DisAsm("ila", spu_reg_name[op.rt], op.i18);
	}

	//0 - 3
	void SELB(spu_opcode_t op)
	{
		DisAsm("selb", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}
	void SHUFB(spu_opcode_t op)
	{
		DisAsm("shufb", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}
	void MPYA(spu_opcode_t op)
	{
		DisAsm("mpya", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}
	void FNMS(spu_opcode_t op)
	{
		DisAsm("fnms", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}
	void FMA(spu_opcode_t op)
	{
		DisAsm("fma", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}
	void FMS(spu_opcode_t op)
	{
		DisAsm("fms", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);
	}

	void UNK(spu_opcode_t op)
	{
		Write(fmt::format("Unknown/Illegal opcode! (0x%08x)", op.opcode));
	}
};
