#pragma once

#include "PPCDisAsm.h"
#include "SPUOpcodes.h"
#include "util/v128.hpp"

enum spu_stop_syscall : u32;

static constexpr const char* spu_reg_name[128] =
{
	"lr", "sp", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
	"r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
	"r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
	"r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
	"r56", "r57", "r58", "r59", "r60", "r61", "r62", "r63",
	"r64", "r65", "r66", "r67", "r68", "r69", "r70", "r71",
	"r72", "r73", "r74", "r75", "r76", "r77", "r78", "r79",
	"r80", "r81", "r82", "r83", "r84", "r85", "r86", "r87",
	"r88", "r89", "r90", "r91", "r92", "r93", "r94", "r95",
	"r96", "r97", "r98", "r99", "r100", "r101", "r102", "r103",
	"r104", "r105", "r106", "r107", "r108", "r109", "r110", "r111",
	"r112", "r113", "r114", "r115", "r116", "r117", "r118", "r119",
	"r120", "r121", "r122", "r123", "r124", "r125", "r126", "r127",
};

static constexpr const char* spu_spreg_name[128] =
{
	"spr0", "spr1", "spr2", "spr3", "spr4", "spr5", "spr6", "spr7",
	"spr8", "spr9", "spr10", "spr11", "spr12", "spr13", "spr14", "spr15",
	"spr16", "spr17", "spr18", "spr19", "spr20", "spr21", "spr22", "spr23",
	"spr24", "spr25", "spr26", "spr27", "spr28", "spr29", "spr30", "spr31",
	"spr32", "spr33", "spr34", "spr35", "spr36", "spr37", "spr38", "spr39",
	"spr40", "spr41", "spr42", "spr43", "spr44", "spr45", "spr46", "spr47",
	"spr48", "spr49", "spr50", "spr51", "spr52", "spr53", "spr54", "spr55",
	"spr56", "spr57", "spr58", "spr59", "spr60", "spr61", "spr62", "spr63",
	"spr64", "spr65", "spr66", "spr67", "spr68", "spr69", "spr70", "spr71",
	"spr72", "spr73", "spr74", "spr75", "spr76", "spr77", "spr78", "spr79",
	"spr80", "spr81", "spr82", "spr83", "spr84", "spr85", "spr86", "spr87",
	"spr88", "spr89", "spr90", "spr91", "spr92", "spr93", "spr94", "spr95",
	"spr96", "spr97", "spr98", "spr99", "spr100", "spr101", "spr102", "spr103",
	"spr104", "spr105", "spr106", "spr107", "spr108", "spr109", "spr110", "spr111",
	"spr112", "spr113", "spr114", "spr115", "spr116", "spr117", "spr118", "spr119",
	"spr120", "spr121", "spr122", "spr123", "spr124", "spr125", "spr126", "spr127",
};

static constexpr const char* spu_ch_name[128] =
{
	"SPU_RdEventStat", "SPU_WrEventMask", "SPU_WrEventAck", "SPU_RdSigNotify1",
	"SPU_RdSigNotify2", "ch5", "ch6", "SPU_WrDec", "SPU_RdDec",
	"MFC_WrMSSyncReq", "ch10", "SPU_RdEventMask", "MFC_RdTagMask", "SPU_RdMachStat",
	"SPU_WrSRR0", "SPU_RdSRR0", "MFC_LSA", "MFC_EAH", "MFC_EAL", "MFC_Size",
	"MFC_TagID", "MFC_Cmd", "MFC_WrTagMask", "MFC_WrTagUpdate", "MFC_RdTagStat",
	"MFC_RdListStallStat", "MFC_WrListStallAck", "MFC_RdAtomicStat",
	"SPU_WrOutMbox", "SPU_RdInMbox", "SPU_WrOutIntrMbox", "ch31", "ch32",
	"ch33", "ch34", "ch35", "ch36", "ch37", "ch38", "ch39", "ch40",
	"ch41", "ch42", "ch43", "ch44", "ch45", "ch46", "ch47", "ch48",
	"ch49", "ch50", "ch51", "ch52", "ch53", "ch54", "ch55", "ch56",
	"ch57", "ch58", "ch59", "ch60", "ch61", "ch62", "ch63", "ch64",
	"ch65", "ch66", "ch67", "ch68", "SPU_Set_Bkmk_Tag", "SPU_PM_Start_Ev", "SPU_PM_Stop_Ev", "ch72",
	"ch73", "ch74", "ch75", "ch76", "ch77", "ch78", "ch79", "ch80",
	"ch81", "ch82", "ch83", "ch84", "ch85", "ch86", "ch87", "ch88",
	"ch89", "ch90", "ch91", "ch92", "ch93", "ch94", "ch95", "ch96",
	"ch97", "ch98", "ch99", "ch100", "ch101", "ch102", "ch103", "ch104",
	"ch105", "ch106", "ch107", "ch108", "ch109", "ch110", "ch111", "ch112",
	"ch113", "ch114", "ch115", "ch116", "ch117", "ch118", "ch119", "ch120",
	"ch121", "ch122", "ch123", "ch124", "ch125", "ch126", "ch127",
};

namespace utils
{
	class shm;
}

void comment_constant(std::string& last_opocde, u64 value, bool print_float = true);

class SPUDisAsm final : public PPCDisAsm
{
	std::shared_ptr<utils::shm> m_shm;
public:
	SPUDisAsm(cpu_disasm_mode mode, const u8* offset, u32 start_pc = 0) : PPCDisAsm(mode, offset, start_pc)
	{
	}

	~SPUDisAsm() = default;

	void set_shm(std::shared_ptr<utils::shm> shm)
	{
		m_shm = std::move(shm);
	}

private:
	virtual u32 DisAsmBranchTarget(const s32 imm) override
	{
		return spu_branch_target(dump_pc, imm);
	}

	static char BrIndirectSuffix(u32 de)
	{
		switch (de)
		{
		case 0b01: return 'e';
		case 0b10: return 'd';
		case 0b11: return '!';
		default: return '\0';
		}
	}

	void DisAsm(const char* op)
	{
		last_opcode += op;
	}
	void DisAsm(std::string_view op, u32 a1)
	{
		fmt::append(last_opcode, "%-*s 0x%x", PadOp(), op, a1);
	}
	void DisAsm(std::string_view op, const char* a1)
	{
		fmt::append(last_opcode, "%-*s %s", PadOp(), op, a1);
	}
	void DisAsm(std::string_view op, const char* a1, const char* a2)
	{
		fmt::append(last_opcode, "%-*s %s,%s", PadOp(), op, a1, a2);
	}
	void DisAsm(std::string_view op, int a1, const char* a2)
	{
		fmt::append(last_opcode, "%-*s 0x%x,%s", PadOp(), op, a1, a2);
	}
	void DisAsm(std::string_view op, const char* a1, int a2)
	{
		fmt::append(last_opcode, "%-*s %s,%s", PadOp(), op, a1, SignedHex(a2));
	}
	void DisAsm(std::string_view op, int a1, int a2)
	{
		fmt::append(last_opcode, "%-*s 0x%x,0x%x", PadOp(), op, a1, a2);
	}
	void DisAsm(std::string_view op, const char* a1, const char* a2, const char* a3)
	{
		fmt::append(last_opcode, "%-*s %s,%s,%s", PadOp(), op, a1, a2, a3);
	}
	void DisAsm(std::string_view op, const char* a1, int a2, const char* a3)
	{
		fmt::append(last_opcode, "%-*s %s,%s(%s)", PadOp(), op, a1, SignedHex(a2), a3);
	}
	void DisAsm(std::string_view op, const char* a1, const char* a2, int a3)
	{
		fmt::append(last_opcode, "%-*s %s,%s,%s", PadOp(), op, a1, a2, SignedHex(a3));
	}
	void DisAsm(std::string_view op, const char* a1, const char* a2, const char* a3, const char* a4)
	{
		fmt::append(last_opcode, "%-*s %s,%s,%s,%s", PadOp(), op, a1, a2, a3, a4);
	}

	using field_de_t = decltype(spu_opcode_t::de);
	void DisAsm(std::string_view op, field_de_t de, const char* a1)
	{
		const char c = BrIndirectSuffix(de);

		if (c == '!')
		{
			// Invalid
			fmt::append(last_opcode, "?? ?? (%s)", op);
			return;
		}

		fmt::append(last_opcode, "%-*s %s", PadOp(op, c ? 1 : 0), op, a1);
		insert_char_if(op, !!c, c);
	}
	void DisAsm(std::string_view op, field_de_t de, const char* a1, const char* a2)
	{
		const char c = BrIndirectSuffix(de);

		if (c == '!')
		{
			fmt::append(last_opcode, "?? ?? (%s)", op);
			return;
		}

		fmt::append(last_opcode, "%-*s %s,%s", PadOp(op, c ? 1 : 0), op, a1, a2);
		insert_char_if(op, !!c, c);
	}

public:
	u32 disasm(u32 pc) override;
	std::pair<const void*, usz> get_memory_span() const override;
	std::unique_ptr<CPUDisAsm> copy_type_erased() const override;
	std::pair<bool, v128> try_get_const_value(u32 reg, u32 pc = -1, u32 TTL = 10) const;

	// Get constant value if the original array is made of only repetitions of the same value
	template <typename T> requires (sizeof(T) < sizeof(v128) && !(sizeof(v128) % sizeof(T)))
	std::pair<bool, T> try_get_const_equal_value_array(u32 reg, u32 pc = -1, u32 TTL = 10) const
	{
		auto [ok, res] = try_get_const_value(reg, pc, TTL);

		if (!ok)
		{
			return {};
		}

		v128 test_value{};

		T sample{};
		std::memcpy(&sample, res._bytes, sizeof(T));

		for (u32 i = 0; i < sizeof(v128); i += sizeof(T))
		{
			std::memcpy(test_value._bytes + i, &sample, sizeof(T));
		}

		if (test_value != res)
		{
			return {};
		}

		return {ok, sample};
	}

	struct insert_mask_info
	{
		u32 type_size;
		u32 dst_index;
		u32 src_index;
	};

	static insert_mask_info try_get_insert_mask_info(const v128& mask);

	//0 - 10
	void STOP(spu_opcode_t op)
	{
		op.rb ? UNK(op) : DisAsm("stop", fmt::format("0x%x #%s", op.opcode & 0x3fff, spu_stop_syscall{op.opcode & 0x3fff}).c_str());
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

	void WRCH(spu_opcode_t op);

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
		if (!op.si7)
		{
			// Made-up mnemonic: as MR on PPU
			DisAsm("mr", spu_reg_name[op.rt], spu_reg_name[op.ra]);
			return;
		}

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
		if (op.rt && op.rt != 127u)
		{
			// Valid but makes no sense
			DisAsm("br??", DisAsmBranchTarget(op.i16));
			return;
		}

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
		comment_constant(last_opcode, op.i16 << 16);
	}
	void ILH(spu_opcode_t op)
	{
		DisAsm("ilh", spu_reg_name[op.rt], op.i16);
		comment_constant(last_opcode, (op.i16 << 16) | op.i16);
	}

	void IOHL(spu_opcode_t op);

	//0 - 7
	void ORI(spu_opcode_t op)
	{
		if (!op.si10)
		{
			// Made-up mnemonic: as MR on PPU
			DisAsm("mr", spu_reg_name[op.rt], spu_reg_name[op.ra]);
			return;
		}

		DisAsm("ori", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);

		if (auto [is_const, value] = try_get_const_equal_value_array<u32>(+op.ra); is_const)
		{
			// Comment constant formation
			comment_constant(last_opcode, value | static_cast<u32>(op.si10));
		}
	}
	void ORHI(spu_opcode_t op)
	{
		DisAsm("orhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void ORBI(spu_opcode_t op)
	{
		DisAsm("orbi", spu_reg_name[op.rt], spu_reg_name[op.ra], static_cast<u8>(op.si10));
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
		DisAsm("andbi", spu_reg_name[op.rt], spu_reg_name[op.ra], static_cast<u8>(op.si10));
	}
	void AI(spu_opcode_t op)
	{
		DisAsm("ai", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);

		if (auto [is_const, value] = try_get_const_equal_value_array<u32>(op.ra); is_const)
		{
			// Comment constant formation
			comment_constant(last_opcode, value + static_cast<u32>(op.si10));
		}
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

		if (auto [is_const, value] = try_get_const_equal_value_array<u32>(op.ra); is_const)
		{
			// Comment constant formation
			comment_constant(last_opcode, value ^ static_cast<u32>(op.si10));
		}
	}
	void XORHI(spu_opcode_t op)
	{
		DisAsm("xorhi", spu_reg_name[op.rt], spu_reg_name[op.ra], op.si10);
	}
	void XORBI(spu_opcode_t op)
	{
		DisAsm("xorbi", spu_reg_name[op.rt], spu_reg_name[op.ra], static_cast<u8>(op.si10));
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
	void SHUFB(spu_opcode_t op);
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

	void UNK(spu_opcode_t /*op*/)
	{
		DisAsm("?? ??");
	}
};
