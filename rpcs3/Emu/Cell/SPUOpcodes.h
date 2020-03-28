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

inline u32 spu_branch_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fffc;
}

inline u32 spu_ls_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fff0;
}

static u32 spu_decode(u32 inst)
{
	return inst >> 21;
}

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

		constexpr instruction_info(u32 m, u32 v, T p)
			: magn(m)
			, value(v)
			, pointer(p)
		{
		}

		constexpr instruction_info(u32 m, u32 v, const T* p)
			: magn(m)
			, value(v)
			, pointer(*p)
		{
		}
	};

public:
	constexpr spu_decoder()
	{
		const std::initializer_list<instruction_info> instructions
		{
			{ 0, 0x0, &D::STOP },
			{ 0, 0x1, &D::LNOP },
			{ 0, 0x2, &D::SYNC },
			{ 0, 0x3, &D::DSYNC },
			{ 0, 0xc, &D::MFSPR },
			{ 0, 0xd, &D::RDCH },
			{ 0, 0xf, &D::RCHCNT },
			{ 0, 0x40, &D::SF },
			{ 0, 0x41, &D::OR },
			{ 0, 0x42, &D::BG },
			{ 0, 0x48, &D::SFH },
			{ 0, 0x49, &D::NOR },
			{ 0, 0x53, &D::ABSDB },
			{ 0, 0x58, &D::ROT },
			{ 0, 0x59, &D::ROTM },
			{ 0, 0x5a, &D::ROTMA },
			{ 0, 0x5b, &D::SHL },
			{ 0, 0x5c, &D::ROTH },
			{ 0, 0x5d, &D::ROTHM },
			{ 0, 0x5e, &D::ROTMAH },
			{ 0, 0x5f, &D::SHLH },
			{ 0, 0x78, &D::ROTI },
			{ 0, 0x79, &D::ROTMI },
			{ 0, 0x7a, &D::ROTMAI },
			{ 0, 0x7b, &D::SHLI },
			{ 0, 0x7c, &D::ROTHI },
			{ 0, 0x7d, &D::ROTHMI },
			{ 0, 0x7e, &D::ROTMAHI },
			{ 0, 0x7f, &D::SHLHI },
			{ 0, 0xc0, &D::A },
			{ 0, 0xc1, &D::AND },
			{ 0, 0xc2, &D::CG },
			{ 0, 0xc8, &D::AH },
			{ 0, 0xc9, &D::NAND },
			{ 0, 0xd3, &D::AVGB },
			{ 0, 0x10c, &D::MTSPR },
			{ 0, 0x10d, &D::WRCH },
			{ 0, 0x128, &D::BIZ },
			{ 0, 0x129, &D::BINZ },
			{ 0, 0x12a, &D::BIHZ },
			{ 0, 0x12b, &D::BIHNZ },
			{ 0, 0x140, &D::STOPD },
			{ 0, 0x144, &D::STQX },
			{ 0, 0x1a8, &D::BI },
			{ 0, 0x1a9, &D::BISL },
			{ 0, 0x1aa, &D::IRET },
			{ 0, 0x1ab, &D::BISLED },
			{ 0, 0x1ac, &D::HBR },
			{ 0, 0x1b0, &D::GB },
			{ 0, 0x1b1, &D::GBH },
			{ 0, 0x1b2, &D::GBB },
			{ 0, 0x1b4, &D::FSM },
			{ 0, 0x1b5, &D::FSMH },
			{ 0, 0x1b6, &D::FSMB },
			{ 0, 0x1b8, &D::FREST },
			{ 0, 0x1b9, &D::FRSQEST },
			{ 0, 0x1c4, &D::LQX },
			{ 0, 0x1cc, &D::ROTQBYBI },
			{ 0, 0x1cd, &D::ROTQMBYBI },
			{ 0, 0x1cf, &D::SHLQBYBI },
			{ 0, 0x1d4, &D::CBX },
			{ 0, 0x1d5, &D::CHX },
			{ 0, 0x1d6, &D::CWX },
			{ 0, 0x1d7, &D::CDX },
			{ 0, 0x1d8, &D::ROTQBI },
			{ 0, 0x1d9, &D::ROTQMBI },
			{ 0, 0x1db, &D::SHLQBI },
			{ 0, 0x1dc, &D::ROTQBY },
			{ 0, 0x1dd, &D::ROTQMBY },
			{ 0, 0x1df, &D::SHLQBY },
			{ 0, 0x1f0, &D::ORX },
			{ 0, 0x1f4, &D::CBD },
			{ 0, 0x1f5, &D::CHD },
			{ 0, 0x1f6, &D::CWD },
			{ 0, 0x1f7, &D::CDD },
			{ 0, 0x1f8, &D::ROTQBII },
			{ 0, 0x1f9, &D::ROTQMBII },
			{ 0, 0x1fb, &D::SHLQBII },
			{ 0, 0x1fc, &D::ROTQBYI },
			{ 0, 0x1fd, &D::ROTQMBYI },
			{ 0, 0x1ff, &D::SHLQBYI },
			{ 0, 0x201, &D::NOP },
			{ 0, 0x240, &D::CGT },
			{ 0, 0x241, &D::XOR },
			{ 0, 0x248, &D::CGTH },
			{ 0, 0x249, &D::EQV },
			{ 0, 0x250, &D::CGTB },
			{ 0, 0x253, &D::SUMB },
			{ 0, 0x258, &D::HGT },
			{ 0, 0x2a5, &D::CLZ },
			{ 0, 0x2a6, &D::XSWD },
			{ 0, 0x2ae, &D::XSHW },
			{ 0, 0x2b4, &D::CNTB },
			{ 0, 0x2b6, &D::XSBH },
			{ 0, 0x2c0, &D::CLGT },
			{ 0, 0x2c1, &D::ANDC },
			{ 0, 0x2c2, &D::FCGT },
			{ 0, 0x2c3, &D::DFCGT },
			{ 0, 0x2c4, &D::FA },
			{ 0, 0x2c5, &D::FS },
			{ 0, 0x2c6, &D::FM },
			{ 0, 0x2c8, &D::CLGTH },
			{ 0, 0x2c9, &D::ORC },
			{ 0, 0x2ca, &D::FCMGT },
			{ 0, 0x2cb, &D::DFCMGT },
			{ 0, 0x2cc, &D::DFA },
			{ 0, 0x2cd, &D::DFS },
			{ 0, 0x2ce, &D::DFM },
			{ 0, 0x2d0, &D::CLGTB },
			{ 0, 0x2d8, &D::HLGT },
			{ 0, 0x35c, &D::DFMA },
			{ 0, 0x35d, &D::DFMS },
			{ 0, 0x35e, &D::DFNMS },
			{ 0, 0x35f, &D::DFNMA },
			{ 0, 0x3c0, &D::CEQ },
			{ 0, 0x3ce, &D::MPYHHU },
			{ 0, 0x340, &D::ADDX },
			{ 0, 0x341, &D::SFX },
			{ 0, 0x342, &D::CGX },
			{ 0, 0x343, &D::BGX },
			{ 0, 0x346, &D::MPYHHA },
			{ 0, 0x34e, &D::MPYHHAU },
			{ 0, 0x398, &D::FSCRRD },
			{ 0, 0x3b8, &D::FESD },
			{ 0, 0x3b9, &D::FRDS },
			{ 0, 0x3ba, &D::FSCRWR },
			{ 0, 0x3bf, &D::DFTSV },
			{ 0, 0x3c2, &D::FCEQ },
			{ 0, 0x3c3, &D::DFCEQ },
			{ 0, 0x3c4, &D::MPY },
			{ 0, 0x3c5, &D::MPYH },
			{ 0, 0x3c6, &D::MPYHH },
			{ 0, 0x3c7, &D::MPYS },
			{ 0, 0x3c8, &D::CEQH },
			{ 0, 0x3ca, &D::FCMEQ },
			{ 0, 0x3cb, &D::DFCMEQ },
			{ 0, 0x3cc, &D::MPYU },
			{ 0, 0x3d0, &D::CEQB },
			{ 0, 0x3d4, &D::FI },
			{ 0, 0x3d8, &D::HEQ },
			{ 1, 0x1d8, &D::CFLTS },
			{ 1, 0x1d9, &D::CFLTU },
			{ 1, 0x1da, &D::CSFLT },
			{ 1, 0x1db, &D::CUFLT },
			{ 2, 0x40, &D::BRZ },
			{ 2, 0x41, &D::STQA },
			{ 2, 0x42, &D::BRNZ },
			{ 2, 0x44, &D::BRHZ },
			{ 2, 0x46, &D::BRHNZ },
			{ 2, 0x47, &D::STQR },
			{ 2, 0x60, &D::BRA },
			{ 2, 0x61, &D::LQA },
			{ 2, 0x62, &D::BRASL },
			{ 2, 0x64, &D::BR },
			{ 2, 0x65, &D::FSMBI },
			{ 2, 0x66, &D::BRSL },
			{ 2, 0x67, &D::LQR },
			{ 2, 0x81, &D::IL },
			{ 2, 0x82, &D::ILHU },
			{ 2, 0x83, &D::ILH },
			{ 2, 0xc1, &D::IOHL },
			{ 3, 0x4, &D::ORI },
			{ 3, 0x5, &D::ORHI },
			{ 3, 0x6, &D::ORBI },
			{ 3, 0xc, &D::SFI },
			{ 3, 0xd, &D::SFHI },
			{ 3, 0x14, &D::ANDI },
			{ 3, 0x15, &D::ANDHI },
			{ 3, 0x16, &D::ANDBI },
			{ 3, 0x1c, &D::AI },
			{ 3, 0x1d, &D::AHI },
			{ 3, 0x24, &D::STQD },
			{ 3, 0x34, &D::LQD },
			{ 3, 0x44, &D::XORI },
			{ 3, 0x45, &D::XORHI },
			{ 3, 0x46, &D::XORBI },
			{ 3, 0x4c, &D::CGTI },
			{ 3, 0x4d, &D::CGTHI },
			{ 3, 0x4e, &D::CGTBI },
			{ 3, 0x4f, &D::HGTI },
			{ 3, 0x5c, &D::CLGTI },
			{ 3, 0x5d, &D::CLGTHI },
			{ 3, 0x5e, &D::CLGTBI },
			{ 3, 0x5f, &D::HLGTI },
			{ 3, 0x74, &D::MPYI },
			{ 3, 0x75, &D::MPYUI },
			{ 3, 0x7c, &D::CEQI },
			{ 3, 0x7d, &D::CEQHI },
			{ 3, 0x7e, &D::CEQBI },
			{ 3, 0x7f, &D::HEQI },
			{ 4, 0x8, &D::HBRA },
			{ 4, 0x9, &D::HBRR },
			{ 4, 0x21, &D::ILA },
			{ 7, 0x8, &D::SELB },
			{ 7, 0xb, &D::SHUFB },
			{ 7, 0xc, &D::MPYA },
			{ 7, 0xd, &D::FNMS },
			{ 7, 0xe, &D::FMA },
			{ 7, 0xf, &D::FMS },
		};

		for (auto& x : m_table)
		{
			x = &D::UNK;
		}

		for (auto& entry : instructions)
		{
			for (u32 i = 0; i < 1u << entry.magn; i++)
			{
				m_table[entry.value << entry.magn | i] = entry.pointer;
			}
		}
	}

	const std::array<T, 2048>& get_table() const
	{
		return m_table;
	}

	T decode(u32 inst) const
	{
		return m_table[spu_decode(inst)];
	}
};
