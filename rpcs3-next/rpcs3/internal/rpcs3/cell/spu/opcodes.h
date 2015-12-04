#pragma once

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

#define DEFINE_SPU_OPCODES(ns) { \
	{ 10, 0x0, ns STOP }, \
	{ 10, 0x1, ns LNOP }, \
	{ 10, 0x2, ns SYNC }, \
	{ 10, 0x3, ns DSYNC }, \
	{ 10, 0xc, ns MFSPR }, \
	{ 10, 0xd, ns RDCH }, \
	{ 10, 0xf, ns RCHCNT }, \
	{ 10, 0x40, ns SF }, \
	{ 10, 0x41, ns OR }, \
	{ 10, 0x42, ns BG }, \
	{ 10, 0x48, ns SFH }, \
	{ 10, 0x49, ns NOR }, \
	{ 10, 0x53, ns ABSDB }, \
	{ 10, 0x58, ns ROT }, \
	{ 10, 0x59, ns ROTM }, \
	{ 10, 0x5a, ns ROTMA }, \
	{ 10, 0x5b, ns SHL }, \
	{ 10, 0x5c, ns ROTH }, \
	{ 10, 0x5d, ns ROTHM }, \
	{ 10, 0x5e, ns ROTMAH }, \
	{ 10, 0x5f, ns SHLH }, \
	{ 10, 0x78, ns ROTI }, \
	{ 10, 0x79, ns ROTMI }, \
	{ 10, 0x7a, ns ROTMAI }, \
	{ 10, 0x7b, ns SHLI }, \
	{ 10, 0x7c, ns ROTHI }, \
	{ 10, 0x7d, ns ROTHMI }, \
	{ 10, 0x7e, ns ROTMAHI }, \
	{ 10, 0x7f, ns SHLHI }, \
	{ 10, 0xc0, ns A }, \
	{ 10, 0xc1, ns AND }, \
	{ 10, 0xc2, ns CG }, \
	{ 10, 0xc8, ns AH }, \
	{ 10, 0xc9, ns NAND }, \
	{ 10, 0xd3, ns AVGB }, \
	{ 10, 0x10c, ns MTSPR }, \
	{ 10, 0x10d, ns WRCH }, \
	{ 10, 0x128, ns BIZ }, \
	{ 10, 0x129, ns BINZ }, \
	{ 10, 0x12a, ns BIHZ }, \
	{ 10, 0x12b, ns BIHNZ }, \
	{ 10, 0x140, ns STOPD }, \
	{ 10, 0x144, ns STQX }, \
	{ 10, 0x1a8, ns BI }, \
	{ 10, 0x1a9, ns BISL }, \
	{ 10, 0x1aa, ns IRET }, \
	{ 10, 0x1ab, ns BISLED }, \
	{ 10, 0x1ac, ns HBR }, \
	{ 10, 0x1b0, ns GB }, \
	{ 10, 0x1b1, ns GBH }, \
	{ 10, 0x1b2, ns GBB }, \
	{ 10, 0x1b4, ns FSM }, \
	{ 10, 0x1b5, ns FSMH }, \
	{ 10, 0x1b6, ns FSMB }, \
	{ 10, 0x1b8, ns FREST }, \
	{ 10, 0x1b9, ns FRSQEST }, \
	{ 10, 0x1c4, ns LQX }, \
	{ 10, 0x1cc, ns ROTQBYBI }, \
	{ 10, 0x1cd, ns ROTQMBYBI }, \
	{ 10, 0x1cf, ns SHLQBYBI }, \
	{ 10, 0x1d4, ns CBX }, \
	{ 10, 0x1d5, ns CHX }, \
	{ 10, 0x1d6, ns CWX }, \
	{ 10, 0x1d7, ns CDX }, \
	{ 10, 0x1d8, ns ROTQBI }, \
	{ 10, 0x1d9, ns ROTQMBI }, \
	{ 10, 0x1db, ns SHLQBI }, \
	{ 10, 0x1dc, ns ROTQBY }, \
	{ 10, 0x1dd, ns ROTQMBY }, \
	{ 10, 0x1df, ns SHLQBY }, \
	{ 10, 0x1f0, ns ORX }, \
	{ 10, 0x1f4, ns CBD }, \
	{ 10, 0x1f5, ns CHD }, \
	{ 10, 0x1f6, ns CWD }, \
	{ 10, 0x1f7, ns CDD }, \
	{ 10, 0x1f8, ns ROTQBII }, \
	{ 10, 0x1f9, ns ROTQMBII }, \
	{ 10, 0x1fb, ns SHLQBII }, \
	{ 10, 0x1fc, ns ROTQBYI }, \
	{ 10, 0x1fd, ns ROTQMBYI }, \
	{ 10, 0x1ff, ns SHLQBYI }, \
	{ 10, 0x201, ns NOP }, \
	{ 10, 0x240, ns CGT }, \
	{ 10, 0x241, ns XOR }, \
	{ 10, 0x248, ns CGTH }, \
	{ 10, 0x249, ns EQV }, \
	{ 10, 0x250, ns CGTB }, \
	{ 10, 0x253, ns SUMB }, \
	{ 10, 0x258, ns HGT }, \
	{ 10, 0x2a5, ns CLZ }, \
	{ 10, 0x2a6, ns XSWD }, \
	{ 10, 0x2ae, ns XSHW }, \
	{ 10, 0x2b4, ns CNTB }, \
	{ 10, 0x2b6, ns XSBH }, \
	{ 10, 0x2c0, ns CLGT }, \
	{ 10, 0x2c1, ns ANDC }, \
	{ 10, 0x2c2, ns FCGT }, \
	{ 10, 0x2c3, ns DFCGT }, \
	{ 10, 0x2c4, ns FA }, \
	{ 10, 0x2c5, ns FS }, \
	{ 10, 0x2c6, ns FM }, \
	{ 10, 0x2c8, ns CLGTH }, \
	{ 10, 0x2c9, ns ORC }, \
	{ 10, 0x2ca, ns FCMGT }, \
	{ 10, 0x2cb, ns DFCMGT }, \
	{ 10, 0x2cc, ns DFA }, \
	{ 10, 0x2cd, ns DFS }, \
	{ 10, 0x2ce, ns DFM }, \
	{ 10, 0x2d0, ns CLGTB }, \
	{ 10, 0x2d8, ns HLGT }, \
	{ 10, 0x35c, ns DFMA }, \
	{ 10, 0x35d, ns DFMS }, \
	{ 10, 0x35e, ns DFNMS }, \
	{ 10, 0x35f, ns DFNMA }, \
	{ 10, 0x3c0, ns CEQ }, \
	{ 10, 0x3ce, ns MPYHHU }, \
	{ 10, 0x340, ns ADDX }, \
	{ 10, 0x341, ns SFX }, \
	{ 10, 0x342, ns CGX }, \
	{ 10, 0x343, ns BGX }, \
	{ 10, 0x346, ns MPYHHA }, \
	{ 10, 0x34e, ns MPYHHAU }, \
	{ 10, 0x398, ns FSCRRD }, \
	{ 10, 0x3b8, ns FESD }, \
	{ 10, 0x3b9, ns FRDS }, \
	{ 10, 0x3ba, ns FSCRWR }, \
	{ 10, 0x3bf, ns DFTSV }, \
	{ 10, 0x3c2, ns FCEQ }, \
	{ 10, 0x3c3, ns DFCEQ }, \
	{ 10, 0x3c4, ns MPY }, \
	{ 10, 0x3c5, ns MPYH }, \
	{ 10, 0x3c6, ns MPYHH }, \
	{ 10, 0x3c7, ns MPYS }, \
	{ 10, 0x3c8, ns CEQH }, \
	{ 10, 0x3ca, ns FCMEQ }, \
	{ 10, 0x3cb, ns DFCMEQ }, \
	{ 10, 0x3cc, ns MPYU }, \
	{ 10, 0x3d0, ns CEQB }, \
	{ 10, 0x3d4, ns FI }, \
	{ 10, 0x3d8, ns HEQ }, \
	{ 9, 0x1d8, ns CFLTS }, \
	{ 9, 0x1d9, ns CFLTU }, \
	{ 9, 0x1da, ns CSFLT }, \
	{ 9, 0x1db, ns CUFLT }, \
	{ 8, 0x40, ns BRZ }, \
	{ 8, 0x41, ns STQA }, \
	{ 8, 0x42, ns BRNZ }, \
	{ 8, 0x44, ns BRHZ }, \
	{ 8, 0x46, ns BRHNZ }, \
	{ 8, 0x47, ns STQR }, \
	{ 8, 0x60, ns BRA }, \
	{ 8, 0x61, ns LQA }, \
	{ 8, 0x62, ns BRASL }, \
	{ 8, 0x64, ns BR }, \
	{ 8, 0x65, ns FSMBI }, \
	{ 8, 0x66, ns BRSL }, \
	{ 8, 0x67, ns LQR }, \
	{ 8, 0x81, ns IL }, \
	{ 8, 0x82, ns ILHU }, \
	{ 8, 0x83, ns ILH }, \
	{ 8, 0xc1, ns IOHL }, \
	{ 7, 0x4, ns ORI }, \
	{ 7, 0x5, ns ORHI }, \
	{ 7, 0x6, ns ORBI }, \
	{ 7, 0xc, ns SFI }, \
	{ 7, 0xd, ns SFHI }, \
	{ 7, 0x14, ns ANDI }, \
	{ 7, 0x15, ns ANDHI }, \
	{ 7, 0x16, ns ANDBI }, \
	{ 7, 0x1c, ns AI }, \
	{ 7, 0x1d, ns AHI }, \
	{ 7, 0x24, ns STQD }, \
	{ 7, 0x34, ns LQD }, \
	{ 7, 0x44, ns XORI }, \
	{ 7, 0x45, ns XORHI }, \
	{ 7, 0x46, ns XORBI }, \
	{ 7, 0x4c, ns CGTI }, \
	{ 7, 0x4d, ns CGTHI }, \
	{ 7, 0x4e, ns CGTBI }, \
	{ 7, 0x4f, ns HGTI }, \
	{ 7, 0x5c, ns CLGTI }, \
	{ 7, 0x5d, ns CLGTHI }, \
	{ 7, 0x5e, ns CLGTBI }, \
	{ 7, 0x5f, ns HLGTI }, \
	{ 7, 0x74, ns MPYI }, \
	{ 7, 0x75, ns MPYUI }, \
	{ 7, 0x7c, ns CEQI }, \
	{ 7, 0x7d, ns CEQHI }, \
	{ 7, 0x7e, ns CEQBI }, \
	{ 7, 0x7f, ns HEQI }, \
	{ 6, 0x8, ns HBRA }, \
	{ 6, 0x9, ns HBRR }, \
	{ 6, 0x21, ns ILA }, \
	{ 3, 0x8, ns SELB }, \
	{ 3, 0xb, ns SHUFB }, \
	{ 3, 0xc, ns MPYA }, \
	{ 3, 0xd, ns FNMS }, \
	{ 3, 0xe, ns FMA }, \
	{ 3, 0xf, ns FMS }, \
}

template<typename T> class spu_opcode_table_t
{
	std::array<T, 2048> m_data;

	struct opcode_entry_t
	{
		u32 group;
		u32 value;
		T pointer;
	};

public:
	// opcode table initialization (TODO: optimize it a bit)
	spu_opcode_table_t(std::initializer_list<opcode_entry_t> opcodes, T default_value = {})
	{
		for (u32 i = 0; i < 2048; i++)
		{
			m_data[i] = default_value;

			for (auto& op : opcodes)
			{
				if (((i << 21) & (INT_MIN >> op.group)) == (op.value << (31 - op.group)))
				{
					m_data[i] = op.pointer;
					break;
				}
			}
		}
	}

	// access opcode table
	T operator [](u32 opcode_data) const
	{
		// the whole decoding process is shifting opcode data
		return m_data[opcode_data >> 21];
	}
};

inline u32 spu_branch_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fffc;
}

inline u32 spu_ls_target(u32 pc, u32 imm = 0)
{
	return (pc + (imm << 2)) & 0x3fff0;
}
