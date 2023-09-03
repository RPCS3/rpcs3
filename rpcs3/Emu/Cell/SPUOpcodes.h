#pragma once

#include "Utilities/BitField.h"

union spu_opcode_t
{
	u32 opcode;

	bf_t<u32, 0, 7> rt; // 25..31, for 3-op instructions
	bf_t<u32, 0, 7> rc; // 25..31
	bf_t<u32, 7, 7> ra; // 18..24
	bf_t<u32, 14, 7> rb; // 11..17
	bf_t<u32, 21, 7> rt4; // 4..10, for 4-op instructions
	bf_t<u32, 18, 1> e; // 13, "enable interrupts" bit
	bf_t<u32, 19, 1> d; // 12, "disable interrupts" bit
	bf_t<u32, 18, 2> de; // 12..13 combined 'e' and 'd' bits
	bf_t<u32, 20, 1> c; // 11, "C" bit for SYNC instruction
	bf_t<s32, 23, 2> r0h; // 7..8, signed
	bf_t<s32, 14, 2> roh; // 16..17, signed
	bf_t<u32, 14, 7> i7; // 11..17
	bf_t<s32, 14, 7> si7; // 11..17, signed
	bf_t<u32, 14, 8> i8; // 10..17
	bf_t<s32, 14, 10> si10; // 8..17, signed
	bf_t<u32, 7, 16> i16; // 9..24
	bf_t<s32, 7, 16> si16; // 9..24, signed
	bf_t<u32, 7, 18> i18; // 7..24
};

constexpr u32 spu_branch_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fffc;
}

constexpr u32 spu_ls_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fff0;
}

constexpr u32 spu_decode(u32 inst)
{
	return inst >> 21;
}

std::array<u32, 2> op_branch_targets(u32 pc, spu_opcode_t op);

// SPU decoder object. D provides functions. T is function pointer type returned.
template <typename D, typename T = decltype(&D::UNK)>
class spu_decoder
{
	// Fast lookup table
	std::array<T, 2048> m_table{};

	struct instruction_info
	{
		u32 magn; // Count = 2 ^ magn
		u32 value;
		T pointer;

		instruction_info(u32 m, u32 v, T p) noexcept
			: magn(m)
			, value(v)
			, pointer(p)
		{
		}

		instruction_info(u32 m, u32 v, const T* p) noexcept
			: magn(m)
			, value(v)
			, pointer(*p)
		{
		}
	};

	// Helper
	static const D& _first(const D& arg)
	{
		return arg;
	}

public:
	template <typename... Args>
	spu_decoder(const Args&... args) noexcept
	{
		// If an object is passed to the constructor, assign values from that object
#define GET(name) [&]{ if constexpr (sizeof...(Args) > 0) return _first(args...).name; else return &D::name; }()

		static_assert(sizeof...(Args) <= 1);

		const std::initializer_list<instruction_info> instructions
		{
			{ 0, 0x0, GET(STOP) },
			{ 0, 0x1, GET(LNOP) },
			{ 0, 0x2, GET(SYNC) },
			{ 0, 0x3, GET(DSYNC) },
			{ 0, 0xc, GET(MFSPR) },
			{ 0, 0xd, GET(RDCH) },
			{ 0, 0xf, GET(RCHCNT) },
			{ 0, 0x40, GET(SF) },
			{ 0, 0x41, GET(OR) },
			{ 0, 0x42, GET(BG) },
			{ 0, 0x48, GET(SFH) },
			{ 0, 0x49, GET(NOR) },
			{ 0, 0x53, GET(ABSDB) },
			{ 0, 0x58, GET(ROT) },
			{ 0, 0x59, GET(ROTM) },
			{ 0, 0x5a, GET(ROTMA) },
			{ 0, 0x5b, GET(SHL) },
			{ 0, 0x5c, GET(ROTH) },
			{ 0, 0x5d, GET(ROTHM) },
			{ 0, 0x5e, GET(ROTMAH) },
			{ 0, 0x5f, GET(SHLH) },
			{ 0, 0x78, GET(ROTI) },
			{ 0, 0x79, GET(ROTMI) },
			{ 0, 0x7a, GET(ROTMAI) },
			{ 0, 0x7b, GET(SHLI) },
			{ 0, 0x7c, GET(ROTHI) },
			{ 0, 0x7d, GET(ROTHMI) },
			{ 0, 0x7e, GET(ROTMAHI) },
			{ 0, 0x7f, GET(SHLHI) },
			{ 0, 0xc0, GET(A) },
			{ 0, 0xc1, GET(AND) },
			{ 0, 0xc2, GET(CG) },
			{ 0, 0xc8, GET(AH) },
			{ 0, 0xc9, GET(NAND) },
			{ 0, 0xd3, GET(AVGB) },
			{ 0, 0x10c, GET(MTSPR) },
			{ 0, 0x10d, GET(WRCH) },
			{ 0, 0x128, GET(BIZ) },
			{ 0, 0x129, GET(BINZ) },
			{ 0, 0x12a, GET(BIHZ) },
			{ 0, 0x12b, GET(BIHNZ) },
			{ 0, 0x140, GET(STOPD) },
			{ 0, 0x144, GET(STQX) },
			{ 0, 0x1a8, GET(BI) },
			{ 0, 0x1a9, GET(BISL) },
			{ 0, 0x1aa, GET(IRET) },
			{ 0, 0x1ab, GET(BISLED) },
			{ 0, 0x1ac, GET(HBR) },
			{ 0, 0x1b0, GET(GB) },
			{ 0, 0x1b1, GET(GBH) },
			{ 0, 0x1b2, GET(GBB) },
			{ 0, 0x1b4, GET(FSM) },
			{ 0, 0x1b5, GET(FSMH) },
			{ 0, 0x1b6, GET(FSMB) },
			{ 0, 0x1b8, GET(FREST) },
			{ 0, 0x1b9, GET(FRSQEST) },
			{ 0, 0x1c4, GET(LQX) },
			{ 0, 0x1cc, GET(ROTQBYBI) },
			{ 0, 0x1cd, GET(ROTQMBYBI) },
			{ 0, 0x1cf, GET(SHLQBYBI) },
			{ 0, 0x1d4, GET(CBX) },
			{ 0, 0x1d5, GET(CHX) },
			{ 0, 0x1d6, GET(CWX) },
			{ 0, 0x1d7, GET(CDX) },
			{ 0, 0x1d8, GET(ROTQBI) },
			{ 0, 0x1d9, GET(ROTQMBI) },
			{ 0, 0x1db, GET(SHLQBI) },
			{ 0, 0x1dc, GET(ROTQBY) },
			{ 0, 0x1dd, GET(ROTQMBY) },
			{ 0, 0x1df, GET(SHLQBY) },
			{ 0, 0x1f0, GET(ORX) },
			{ 0, 0x1f4, GET(CBD) },
			{ 0, 0x1f5, GET(CHD) },
			{ 0, 0x1f6, GET(CWD) },
			{ 0, 0x1f7, GET(CDD) },
			{ 0, 0x1f8, GET(ROTQBII) },
			{ 0, 0x1f9, GET(ROTQMBII) },
			{ 0, 0x1fb, GET(SHLQBII) },
			{ 0, 0x1fc, GET(ROTQBYI) },
			{ 0, 0x1fd, GET(ROTQMBYI) },
			{ 0, 0x1ff, GET(SHLQBYI) },
			{ 0, 0x201, GET(NOP) },
			{ 0, 0x240, GET(CGT) },
			{ 0, 0x241, GET(XOR) },
			{ 0, 0x248, GET(CGTH) },
			{ 0, 0x249, GET(EQV) },
			{ 0, 0x250, GET(CGTB) },
			{ 0, 0x253, GET(SUMB) },
			{ 0, 0x258, GET(HGT) },
			{ 0, 0x2a5, GET(CLZ) },
			{ 0, 0x2a6, GET(XSWD) },
			{ 0, 0x2ae, GET(XSHW) },
			{ 0, 0x2b4, GET(CNTB) },
			{ 0, 0x2b6, GET(XSBH) },
			{ 0, 0x2c0, GET(CLGT) },
			{ 0, 0x2c1, GET(ANDC) },
			{ 0, 0x2c2, GET(FCGT) },
			{ 0, 0x2c3, GET(DFCGT) },
			{ 0, 0x2c4, GET(FA) },
			{ 0, 0x2c5, GET(FS) },
			{ 0, 0x2c6, GET(FM) },
			{ 0, 0x2c8, GET(CLGTH) },
			{ 0, 0x2c9, GET(ORC) },
			{ 0, 0x2ca, GET(FCMGT) },
			{ 0, 0x2cb, GET(DFCMGT) },
			{ 0, 0x2cc, GET(DFA) },
			{ 0, 0x2cd, GET(DFS) },
			{ 0, 0x2ce, GET(DFM) },
			{ 0, 0x2d0, GET(CLGTB) },
			{ 0, 0x2d8, GET(HLGT) },
			{ 0, 0x35c, GET(DFMA) },
			{ 0, 0x35d, GET(DFMS) },
			{ 0, 0x35e, GET(DFNMS) },
			{ 0, 0x35f, GET(DFNMA) },
			{ 0, 0x3c0, GET(CEQ) },
			{ 0, 0x3ce, GET(MPYHHU) },
			{ 0, 0x340, GET(ADDX) },
			{ 0, 0x341, GET(SFX) },
			{ 0, 0x342, GET(CGX) },
			{ 0, 0x343, GET(BGX) },
			{ 0, 0x346, GET(MPYHHA) },
			{ 0, 0x34e, GET(MPYHHAU) },
			{ 0, 0x398, GET(FSCRRD) },
			{ 0, 0x3b8, GET(FESD) },
			{ 0, 0x3b9, GET(FRDS) },
			{ 0, 0x3ba, GET(FSCRWR) },
			{ 0, 0x3bf, GET(DFTSV) },
			{ 0, 0x3c2, GET(FCEQ) },
			{ 0, 0x3c3, GET(DFCEQ) },
			{ 0, 0x3c4, GET(MPY) },
			{ 0, 0x3c5, GET(MPYH) },
			{ 0, 0x3c6, GET(MPYHH) },
			{ 0, 0x3c7, GET(MPYS) },
			{ 0, 0x3c8, GET(CEQH) },
			{ 0, 0x3ca, GET(FCMEQ) },
			{ 0, 0x3cb, GET(DFCMEQ) },
			{ 0, 0x3cc, GET(MPYU) },
			{ 0, 0x3d0, GET(CEQB) },
			{ 0, 0x3d4, GET(FI) },
			{ 0, 0x3d8, GET(HEQ) },
			{ 1, 0x1d8, GET(CFLTS) },
			{ 1, 0x1d9, GET(CFLTU) },
			{ 1, 0x1da, GET(CSFLT) },
			{ 1, 0x1db, GET(CUFLT) },
			{ 2, 0x40, GET(BRZ) },
			{ 2, 0x41, GET(STQA) },
			{ 2, 0x42, GET(BRNZ) },
			{ 2, 0x44, GET(BRHZ) },
			{ 2, 0x46, GET(BRHNZ) },
			{ 2, 0x47, GET(STQR) },
			{ 2, 0x60, GET(BRA) },
			{ 2, 0x61, GET(LQA) },
			{ 2, 0x62, GET(BRASL) },
			{ 2, 0x64, GET(BR) },
			{ 2, 0x65, GET(FSMBI) },
			{ 2, 0x66, GET(BRSL) },
			{ 2, 0x67, GET(LQR) },
			{ 2, 0x81, GET(IL) },
			{ 2, 0x82, GET(ILHU) },
			{ 2, 0x83, GET(ILH) },
			{ 2, 0xc1, GET(IOHL) },
			{ 3, 0x4, GET(ORI) },
			{ 3, 0x5, GET(ORHI) },
			{ 3, 0x6, GET(ORBI) },
			{ 3, 0xc, GET(SFI) },
			{ 3, 0xd, GET(SFHI) },
			{ 3, 0x14, GET(ANDI) },
			{ 3, 0x15, GET(ANDHI) },
			{ 3, 0x16, GET(ANDBI) },
			{ 3, 0x1c, GET(AI) },
			{ 3, 0x1d, GET(AHI) },
			{ 3, 0x24, GET(STQD) },
			{ 3, 0x34, GET(LQD) },
			{ 3, 0x44, GET(XORI) },
			{ 3, 0x45, GET(XORHI) },
			{ 3, 0x46, GET(XORBI) },
			{ 3, 0x4c, GET(CGTI) },
			{ 3, 0x4d, GET(CGTHI) },
			{ 3, 0x4e, GET(CGTBI) },
			{ 3, 0x4f, GET(HGTI) },
			{ 3, 0x5c, GET(CLGTI) },
			{ 3, 0x5d, GET(CLGTHI) },
			{ 3, 0x5e, GET(CLGTBI) },
			{ 3, 0x5f, GET(HLGTI) },
			{ 3, 0x74, GET(MPYI) },
			{ 3, 0x75, GET(MPYUI) },
			{ 3, 0x7c, GET(CEQI) },
			{ 3, 0x7d, GET(CEQHI) },
			{ 3, 0x7e, GET(CEQBI) },
			{ 3, 0x7f, GET(HEQI) },
			{ 4, 0x8, GET(HBRA) },
			{ 4, 0x9, GET(HBRR) },
			{ 4, 0x21, GET(ILA) },
			{ 7, 0x8, GET(SELB) },
			{ 7, 0xb, GET(SHUFB) },
			{ 7, 0xc, GET(MPYA) },
			{ 7, 0xd, GET(FNMS) },
			{ 7, 0xe, GET(FMA) },
			{ 7, 0xf, GET(FMS) },
		};

		for (auto& x : m_table)
		{
			x = GET(UNK);
		}

		for (auto& entry : instructions)
		{
			for (u32 i = 0; i < 1u << entry.magn; i++)
			{
				m_table[entry.value << entry.magn | i] = entry.pointer;
			}
		}
	}

	const std::array<T, 2048>& get_table() const noexcept
	{
		return m_table;
	}

	T decode(u32 inst) const noexcept
	{
		return m_table[spu_decode(inst)];
	}
};

#undef GET
