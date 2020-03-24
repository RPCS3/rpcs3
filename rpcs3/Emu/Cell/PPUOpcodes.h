#pragma once

#include "Utilities/BitField.h"
#include "Utilities/asm.h"

template<typename T, u32 I, u32 N> using ppu_bf_t = bf_t<T, sizeof(T) * 8 - N - I, N>;

union ppu_opcode_t
{
	u32 opcode;

	ppu_bf_t<u32, 0, 6> main; // 0..5
	cf_t<ppu_bf_t<u32, 30, 1>, ppu_bf_t<u32, 16, 5>> sh64; // 30 + 16..20
	cf_t<ppu_bf_t<u32, 26, 1>, ppu_bf_t<u32, 21, 5>> mbe64; // 26 + 21..25
	ppu_bf_t<u32, 11, 5> vuimm; // 11..15
	ppu_bf_t<u32, 6, 5> vs; // 6..10
	ppu_bf_t<u32, 22, 4> vsh; // 22..25
	ppu_bf_t<u32, 21, 1> oe; // 21
	ppu_bf_t<u32, 11, 10> spr; // 11..20
	ppu_bf_t<u32, 21, 5> vc; // 21..25
	ppu_bf_t<u32, 16, 5> vb; // 16..20
	ppu_bf_t<u32, 11, 5> va; // 11..15
	ppu_bf_t<u32, 6, 5> vd; // 6..10
	ppu_bf_t<u32, 31, 1> lk; // 31
	ppu_bf_t<u32, 30, 1> aa; // 30
	ppu_bf_t<u32, 16, 5> rb; // 16..20
	ppu_bf_t<u32, 11, 5> ra; // 11..15
	ppu_bf_t<u32, 6, 5> rd; // 6..10
	ppu_bf_t<u32, 16, 16> uimm16; // 16..31
	ppu_bf_t<u32, 11, 1> l11; // 11
	ppu_bf_t<u32, 6, 5> rs; // 6..10
	ppu_bf_t<s32, 16, 16> simm16; // 16..31, signed
	ppu_bf_t<s32, 16, 14> ds; // 16..29, signed
	ppu_bf_t<s32, 11, 5> vsimm; // 11..15, signed
	ppu_bf_t<s32, 6, 26> ll; // 6..31, signed
	ppu_bf_t<s32, 6, 24> li; // 6..29, signed
	ppu_bf_t<u32, 20, 7> lev; // 20..26
	ppu_bf_t<u32, 16, 4> i; // 16..19
	ppu_bf_t<u32, 11, 3> crfs; // 11..13
	ppu_bf_t<u32, 10, 1> l10; // 10
	ppu_bf_t<u32, 6, 3> crfd; // 6..8
	ppu_bf_t<u32, 16, 5> crbb; // 16..20
	ppu_bf_t<u32, 11, 5> crba; // 11..15
	ppu_bf_t<u32, 6, 5> crbd; // 6..10
	ppu_bf_t<u32, 31, 1> rc; // 31
	ppu_bf_t<u32, 26, 5> me32; // 26..30
	ppu_bf_t<u32, 21, 5> mb32; // 21..25
	ppu_bf_t<u32, 16, 5> sh32; // 16..20
	ppu_bf_t<u32, 11, 5> bi; // 11..15
	ppu_bf_t<u32, 6, 5> bo; // 6..10
	ppu_bf_t<u32, 19, 2> bh; // 19..20
	ppu_bf_t<u32, 21, 5> frc; // 21..25
	ppu_bf_t<u32, 16, 5> frb; // 16..20
	ppu_bf_t<u32, 11, 5> fra; // 11..15
	ppu_bf_t<u32, 6, 5> frd; // 6..10
	ppu_bf_t<u32, 12, 8> crm; // 12..19
	ppu_bf_t<u32, 6, 5> frs; // 6..10
	ppu_bf_t<u32, 7, 8> flm; // 7..14
	ppu_bf_t<u32, 6, 1> l6; // 6
	ppu_bf_t<u32, 15, 1> l15; // 15
	cf_t<ppu_bf_t<s32, 16, 14>, ff_t<u32, 0, 2>> bt14;
	cf_t<ppu_bf_t<s32, 6, 24>, ff_t<u32, 0, 2>> bt24;
};

inline u64 ppu_rotate_mask(u32 mb, u32 me)
{
	return utils::ror64(~0ull << (~(me - mb) & 63), mb);
}

inline u32 ppu_decode(u32 inst)
{
	return (inst >> 26 | inst << (32 - 26)) & 0x1ffff; // Rotate + mask
}

// PPU decoder object. D provides functions. T is function pointer type returned.
template <typename D, typename T = decltype(&D::UNK)>
class ppu_decoder
{
	// Fast lookup table
	std::array<T, 0x20000> m_table{};

	struct instruction_info
	{
		u32 value;
		T pointer;
		u32 magn; // Non-zero for "columns" (effectively, number of most significant bits "eaten")

		constexpr instruction_info(u32 v, T p, u32 m = 0)
			: value(v)
			, pointer(p)
			, magn(m)
		{
		}

		constexpr instruction_info(u32 v, const T* p, u32 m = 0)
			: value(v)
			, pointer(*p)
			, magn(m)
		{
		}
	};

	// Fill lookup table
	constexpr void fill_table(u32 main_op, u32 count, u32 sh, std::initializer_list<instruction_info> entries)
	{
		if (sh < 11)
		{
			for (const auto& v : entries)
			{
				for (u32 i = 0; i < 1u << (v.magn + (11 - sh - count)); i++)
				{
					for (u32 j = 0; j < 1u << sh; j++)
					{
						m_table.at((((((i << (count - v.magn)) | v.value) << sh) | j) << 6) | main_op) = v.pointer;
					}
				}
			}
		}
		else
		{
			// Main table (special case)
			for (const auto& v : entries)
			{
				for (u32 i = 0; i < 1u << 11; i++)
				{
					m_table.at(i << 6 | v.value) = v.pointer;
				}
			}
		}
	}

public:
	constexpr ppu_decoder()
	{
		for (auto& x : m_table)
		{
			x = &D::UNK;
		}

		// Main opcodes (field 0..5)
		fill_table(0x00, 6, -1,
		{
			{ 0x02, &D::TDI },
			{ 0x03, &D::TWI },
			{ 0x07, &D::MULLI },
			{ 0x08, &D::SUBFIC },
			{ 0x0a, &D::CMPLI },
			{ 0x0b, &D::CMPI },
			{ 0x0c, &D::ADDIC },
			{ 0x0d, &D::ADDIC },
			{ 0x0e, &D::ADDI },
			{ 0x0f, &D::ADDIS },
			{ 0x10, &D::BC },
			{ 0x11, &D::SC },
			{ 0x12, &D::B },
			{ 0x14, &D::RLWIMI },
			{ 0x15, &D::RLWINM },
			{ 0x17, &D::RLWNM },
			{ 0x18, &D::ORI },
			{ 0x19, &D::ORIS },
			{ 0x1a, &D::XORI },
			{ 0x1b, &D::XORIS },
			{ 0x1c, &D::ANDI },
			{ 0x1d, &D::ANDIS },
			{ 0x20, &D::LWZ },
			{ 0x21, &D::LWZU },
			{ 0x22, &D::LBZ },
			{ 0x23, &D::LBZU },
			{ 0x24, &D::STW },
			{ 0x25, &D::STWU },
			{ 0x26, &D::STB },
			{ 0x27, &D::STBU },
			{ 0x28, &D::LHZ },
			{ 0x29, &D::LHZU },
			{ 0x2a, &D::LHA },
			{ 0x2b, &D::LHAU },
			{ 0x2c, &D::STH },
			{ 0x2d, &D::STHU },
			{ 0x2e, &D::LMW },
			{ 0x2f, &D::STMW },
			{ 0x30, &D::LFS },
			{ 0x31, &D::LFSU },
			{ 0x32, &D::LFD },
			{ 0x33, &D::LFDU },
			{ 0x34, &D::STFS },
			{ 0x35, &D::STFSU },
			{ 0x36, &D::STFD },
			{ 0x37, &D::STFDU },
		});

		// Group 0x04 opcodes (field 21..31)
		fill_table(0x04, 11, 0,
		{
			{ 0x0, &D::VADDUBM },
			{ 0x2, &D::VMAXUB },
			{ 0x4, &D::VRLB },
			{ 0x6, &D::VCMPEQUB, 1 },
			{ 0x8, &D::VMULOUB },
			{ 0xa, &D::VADDFP },
			{ 0xc, &D::VMRGHB },
			{ 0xe, &D::VPKUHUM },

			{ 0x20, &D::VMHADDSHS, 5 },
			{ 0x21, &D::VMHRADDSHS, 5 },
			{ 0x22, &D::VMLADDUHM, 5 },
			{ 0x24, &D::VMSUMUBM, 5 },
			{ 0x25, &D::VMSUMMBM, 5 },
			{ 0x26, &D::VMSUMUHM, 5 },
			{ 0x27, &D::VMSUMUHS, 5 },
			{ 0x28, &D::VMSUMSHM, 5 },
			{ 0x29, &D::VMSUMSHS, 5 },
			{ 0x2a, &D::VSEL, 5 },
			{ 0x2b, &D::VPERM, 5 },
			{ 0x2c, &D::VSLDOI, 5 },
			{ 0x2e, &D::VMADDFP, 5 },
			{ 0x2f, &D::VNMSUBFP, 5 },

			{ 0x40, &D::VADDUHM },
			{ 0x42, &D::VMAXUH },
			{ 0x44, &D::VRLH },
			{ 0x46, &D::VCMPEQUH, 1 },
			{ 0x48, &D::VMULOUH },
			{ 0x4a, &D::VSUBFP },
			{ 0x4c, &D::VMRGHH },
			{ 0x4e, &D::VPKUWUM },
			{ 0x80, &D::VADDUWM },
			{ 0x82, &D::VMAXUW },
			{ 0x84, &D::VRLW },
			{ 0x86, &D::VCMPEQUW, 1 },
			{ 0x8c, &D::VMRGHW },
			{ 0x8e, &D::VPKUHUS },
			{ 0xc6, &D::VCMPEQFP, 1 },
			{ 0xce, &D::VPKUWUS },

			{ 0x102, &D::VMAXSB },
			{ 0x104, &D::VSLB },
			{ 0x108, &D::VMULOSB },
			{ 0x10a, &D::VREFP },
			{ 0x10c, &D::VMRGLB },
			{ 0x10e, &D::VPKSHUS },
			{ 0x142, &D::VMAXSH },
			{ 0x144, &D::VSLH },
			{ 0x148, &D::VMULOSH },
			{ 0x14a, &D::VRSQRTEFP },
			{ 0x14c, &D::VMRGLH },
			{ 0x14e, &D::VPKSWUS },
			{ 0x180, &D::VADDCUW },
			{ 0x182, &D::VMAXSW },
			{ 0x184, &D::VSLW },
			{ 0x18a, &D::VEXPTEFP },
			{ 0x18c, &D::VMRGLW },
			{ 0x18e, &D::VPKSHSS },
			{ 0x1c4, &D::VSL },
			{ 0x1c6, &D::VCMPGEFP, 1 },
			{ 0x1ca, &D::VLOGEFP },
			{ 0x1ce, &D::VPKSWSS },
			{ 0x200, &D::VADDUBS },
			{ 0x202, &D::VMINUB },
			{ 0x204, &D::VSRB },
			{ 0x206, &D::VCMPGTUB, 1 },
			{ 0x208, &D::VMULEUB },
			{ 0x20a, &D::VRFIN },
			{ 0x20c, &D::VSPLTB },
			{ 0x20e, &D::VUPKHSB },
			{ 0x240, &D::VADDUHS },
			{ 0x242, &D::VMINUH },
			{ 0x244, &D::VSRH },
			{ 0x246, &D::VCMPGTUH, 1 },
			{ 0x248, &D::VMULEUH },
			{ 0x24a, &D::VRFIZ },
			{ 0x24c, &D::VSPLTH },
			{ 0x24e, &D::VUPKHSH },
			{ 0x280, &D::VADDUWS },
			{ 0x282, &D::VMINUW },
			{ 0x284, &D::VSRW },
			{ 0x286, &D::VCMPGTUW, 1 },
			{ 0x28a, &D::VRFIP },
			{ 0x28c, &D::VSPLTW },
			{ 0x28e, &D::VUPKLSB },
			{ 0x2c4, &D::VSR },
			{ 0x2c6, &D::VCMPGTFP, 1 },
			{ 0x2ca, &D::VRFIM },
			{ 0x2ce, &D::VUPKLSH },
			{ 0x300, &D::VADDSBS },
			{ 0x302, &D::VMINSB },
			{ 0x304, &D::VSRAB },
			{ 0x306, &D::VCMPGTSB, 1 },
			{ 0x308, &D::VMULESB },
			{ 0x30a, &D::VCFUX },
			{ 0x30c, &D::VSPLTISB },
			{ 0x30e, &D::VPKPX },
			{ 0x340, &D::VADDSHS },
			{ 0x342, &D::VMINSH },
			{ 0x344, &D::VSRAH },
			{ 0x346, &D::VCMPGTSH, 1 },
			{ 0x348, &D::VMULESH },
			{ 0x34a, &D::VCFSX },
			{ 0x34c, &D::VSPLTISH },
			{ 0x34e, &D::VUPKHPX },
			{ 0x380, &D::VADDSWS },
			{ 0x382, &D::VMINSW },
			{ 0x384, &D::VSRAW },
			{ 0x386, &D::VCMPGTSW, 1 },
			{ 0x38a, &D::VCTUXS },
			{ 0x38c, &D::VSPLTISW },
			{ 0x3c6, &D::VCMPBFP, 1 },
			{ 0x3ca, &D::VCTSXS },
			{ 0x3ce, &D::VUPKLPX },
			{ 0x400, &D::VSUBUBM },
			{ 0x402, &D::VAVGUB },
			{ 0x404, &D::VAND },
			{ 0x40a, &D::VMAXFP },
			{ 0x40c, &D::VSLO },
			{ 0x440, &D::VSUBUHM },
			{ 0x442, &D::VAVGUH },
			{ 0x444, &D::VANDC },
			{ 0x44a, &D::VMINFP },
			{ 0x44c, &D::VSRO },
			{ 0x480, &D::VSUBUWM },
			{ 0x482, &D::VAVGUW },
			{ 0x484, &D::VOR },
			{ 0x4c4, &D::VXOR },
			{ 0x502, &D::VAVGSB },
			{ 0x504, &D::VNOR },
			{ 0x542, &D::VAVGSH },
			{ 0x580, &D::VSUBCUW },
			{ 0x582, &D::VAVGSW },
			{ 0x600, &D::VSUBUBS },
			{ 0x604, &D::MFVSCR },
			{ 0x608, &D::VSUM4UBS },
			{ 0x640, &D::VSUBUHS },
			{ 0x644, &D::MTVSCR },
			{ 0x648, &D::VSUM4SHS },
			{ 0x680, &D::VSUBUWS },
			{ 0x688, &D::VSUM2SWS },
			{ 0x700, &D::VSUBSBS },
			{ 0x708, &D::VSUM4SBS },
			{ 0x740, &D::VSUBSHS },
			{ 0x780, &D::VSUBSWS },
			{ 0x788, &D::VSUMSWS },
		});

		// Group 0x13 opcodes (field 21..30)
		fill_table(0x13, 10, 1,
		{
			{ 0x000, &D::MCRF },
			{ 0x010, &D::BCLR },
			{ 0x021, &D::CRNOR },
			{ 0x081, &D::CRANDC },
			{ 0x096, &D::ISYNC },
			{ 0x0c1, &D::CRXOR },
			{ 0x0e1, &D::CRNAND },
			{ 0x101, &D::CRAND },
			{ 0x121, &D::CREQV },
			{ 0x1a1, &D::CRORC },
			{ 0x1c1, &D::CROR },
			{ 0x210, &D::BCCTR },
		});

		// Group 0x1e opcodes (field 27..30)
		fill_table(0x1e, 4, 1,
		{
			{ 0x0, &D::RLDICL },
			{ 0x1, &D::RLDICL },
			{ 0x2, &D::RLDICR },
			{ 0x3, &D::RLDICR },
			{ 0x4, &D::RLDIC },
			{ 0x5, &D::RLDIC },
			{ 0x6, &D::RLDIMI },
			{ 0x7, &D::RLDIMI },
			{ 0x8, &D::RLDCL },
			{ 0x9, &D::RLDCR },
		});

		// Group 0x1f opcodes (field 21..30)
		fill_table(0x1f, 10, 1,
		{
			{ 0x000, &D::CMP },
			{ 0x004, &D::TW },
			{ 0x006, &D::LVSL },
			{ 0x007, &D::LVEBX },
			{ 0x008, &D::SUBFC, 1 },
			{ 0x009, &D::MULHDU },
			{ 0x00a, &D::ADDC, 1 },
			{ 0x00b, &D::MULHWU },
			{ 0x013, &D::MFOCRF },
			{ 0x014, &D::LWARX },
			{ 0x015, &D::LDX },
			{ 0x017, &D::LWZX },
			{ 0x018, &D::SLW },
			{ 0x01a, &D::CNTLZW },
			{ 0x01b, &D::SLD },
			{ 0x01c, &D::AND },
			{ 0x020, &D::CMPL },
			{ 0x026, &D::LVSR },
			{ 0x027, &D::LVEHX },
			{ 0x028, &D::SUBF, 1 },
			{ 0x035, &D::LDUX },
			{ 0x036, &D::DCBST },
			{ 0x037, &D::LWZUX },
			{ 0x03a, &D::CNTLZD },
			{ 0x03c, &D::ANDC },
			{ 0x044, &D::TD },
			{ 0x047, &D::LVEWX },
			{ 0x049, &D::MULHD },
			{ 0x04b, &D::MULHW },
			{ 0x054, &D::LDARX },
			{ 0x056, &D::DCBF },
			{ 0x057, &D::LBZX },
			{ 0x067, &D::LVX },
			{ 0x068, &D::NEG, 1 },
			{ 0x077, &D::LBZUX },
			{ 0x07c, &D::NOR },
			{ 0x087, &D::STVEBX },
			{ 0x088, &D::SUBFE, 1 },
			{ 0x08a, &D::ADDE, 1 },
			{ 0x090, &D::MTOCRF },
			{ 0x095, &D::STDX },
			{ 0x096, &D::STWCX },
			{ 0x097, &D::STWX },
			{ 0x0a7, &D::STVEHX },
			{ 0x0b5, &D::STDUX },
			{ 0x0b7, &D::STWUX },
			{ 0x0c7, &D::STVEWX },
			{ 0x0c8, &D::SUBFZE, 1 },
			{ 0x0ca, &D::ADDZE, 1 },
			{ 0x0d6, &D::STDCX },
			{ 0x0d7, &D::STBX },
			{ 0x0e7, &D::STVX },
			{ 0x0e8, &D::SUBFME, 1 },
			{ 0x0e9, &D::MULLD, 1 },
			{ 0x0ea, &D::ADDME, 1 },
			{ 0x0eb, &D::MULLW, 1 },
			{ 0x0f6, &D::DCBTST },
			{ 0x0f7, &D::STBUX },
			{ 0x10a, &D::ADD, 1 },
			{ 0x116, &D::DCBT },
			{ 0x117, &D::LHZX },
			{ 0x11c, &D::EQV },
			{ 0x136, &D::ECIWX },
			{ 0x137, &D::LHZUX },
			{ 0x13c, &D::XOR },
			{ 0x153, &D::MFSPR },
			{ 0x155, &D::LWAX },
			{ 0x156, &D::DST },
			{ 0x157, &D::LHAX },
			{ 0x167, &D::LVXL },
			{ 0x173, &D::MFTB },
			{ 0x175, &D::LWAUX },
			{ 0x176, &D::DSTST },
			{ 0x177, &D::LHAUX },
			{ 0x197, &D::STHX },
			{ 0x19c, &D::ORC },
			{ 0x1b6, &D::ECOWX },
			{ 0x1b7, &D::STHUX },
			{ 0x1bc, &D::OR },
			{ 0x1c9, &D::DIVDU, 1 },
			{ 0x1cb, &D::DIVWU, 1 },
			{ 0x1d3, &D::MTSPR },
			{ 0x1d6, &D::DCBI },
			{ 0x1dc, &D::NAND },
			{ 0x1e7, &D::STVXL },
			{ 0x1e9, &D::DIVD, 1 },
			{ 0x1eb, &D::DIVW, 1 },
			{ 0x207, &D::LVLX },
			{ 0x214, &D::LDBRX },
			{ 0x215, &D::LSWX },
			{ 0x216, &D::LWBRX },
			{ 0x217, &D::LFSX },
			{ 0x218, &D::SRW },
			{ 0x21b, &D::SRD },
			{ 0x227, &D::LVRX },
			{ 0x237, &D::LFSUX },
			{ 0x255, &D::LSWI },
			{ 0x256, &D::SYNC },
			{ 0x257, &D::LFDX },
			{ 0x277, &D::LFDUX },
			{ 0x287, &D::STVLX },
			{ 0x294, &D::STDBRX },
			{ 0x295, &D::STSWX },
			{ 0x296, &D::STWBRX },
			{ 0x297, &D::STFSX },
			{ 0x2a7, &D::STVRX },
			{ 0x2b7, &D::STFSUX },
			{ 0x2d5, &D::STSWI },
			{ 0x2d7, &D::STFDX },
			{ 0x2f7, &D::STFDUX },
			{ 0x307, &D::LVLXL },
			{ 0x316, &D::LHBRX },
			{ 0x318, &D::SRAW },
			{ 0x31a, &D::SRAD },
			{ 0x327, &D::LVRXL },
			{ 0x336, &D::DSS },
			{ 0x338, &D::SRAWI },
			{ 0x33a, &D::SRADI },
			{ 0x33b, &D::SRADI },
			{ 0x356, &D::EIEIO },
			{ 0x387, &D::STVLXL },
			{ 0x396, &D::STHBRX },
			{ 0x39a, &D::EXTSH },
			{ 0x3a7, &D::STVRXL },
			{ 0x3ba, &D::EXTSB },
			{ 0x3d7, &D::STFIWX },
			{ 0x3da, &D::EXTSW },
			{ 0x3d6, &D::ICBI },
			{ 0x3f6, &D::DCBZ },
		});

		// Group 0x3a opcodes (field 30..31)
		fill_table(0x3a, 2, 0,
		{
			{ 0x0, &D::LD },
			{ 0x1, &D::LDU },
			{ 0x2, &D::LWA },
		});

		// Group 0x3b opcodes (field 21..30)
		fill_table(0x3b, 10, 1,
		{
			{ 0x12, &D::FDIVS, 5 },
			{ 0x14, &D::FSUBS, 5 },
			{ 0x15, &D::FADDS, 5 },
			{ 0x16, &D::FSQRTS, 5 },
			{ 0x18, &D::FRES, 5 },
			{ 0x19, &D::FMULS, 5 },
			{ 0x1c, &D::FMSUBS, 5 },
			{ 0x1d, &D::FMADDS, 5 },
			{ 0x1e, &D::FNMSUBS, 5 },
			{ 0x1f, &D::FNMADDS, 5 },
		});

		// Group 0x3e opcodes (field 30..31)
		fill_table(0x3e, 2, 0,
		{
			{ 0x0, &D::STD },
			{ 0x1, &D::STDU },
		});

		// Group 0x3f opcodes (field 21..30)
		fill_table(0x3f, 10, 1,
		{
			{ 0x026, &D::MTFSB1 },
			{ 0x040, &D::MCRFS },
			{ 0x046, &D::MTFSB0 },
			{ 0x086, &D::MTFSFI },
			{ 0x247, &D::MFFS },
			{ 0x2c7, &D::MTFSF },

			{ 0x000, &D::FCMPU },
			{ 0x00c, &D::FRSP },
			{ 0x00e, &D::FCTIW },
			{ 0x00f, &D::FCTIWZ },

			{ 0x012, &D::FDIV, 5 },
			{ 0x014, &D::FSUB, 5 },
			{ 0x015, &D::FADD, 5 },
			{ 0x016, &D::FSQRT, 5 },
			{ 0x017, &D::FSEL, 5 },
			{ 0x019, &D::FMUL, 5 },
			{ 0x01a, &D::FRSQRTE, 5 },
			{ 0x01c, &D::FMSUB, 5 },
			{ 0x01d, &D::FMADD, 5 },
			{ 0x01e, &D::FNMSUB, 5 },
			{ 0x01f, &D::FNMADD, 5 },

			{ 0x020, &D::FCMPO },
			{ 0x028, &D::FNEG },
			{ 0x048, &D::FMR },
			{ 0x088, &D::FNABS },
			{ 0x108, &D::FABS },
			{ 0x32e, &D::FCTID },
			{ 0x32f, &D::FCTIDZ },
			{ 0x34e, &D::FCFID },
		});
	}

	const std::array<T, 0x20000>& get_table() const
	{
		return m_table;
	}

	T decode(u32 inst) const
	{
		return m_table[ppu_decode(inst)];
	}
};

namespace ppu_instructions
{
	namespace fields
	{
		enum
		{
			r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11,
			r12, r13, r14, r15, r16, r17, r18, r19, r20, r21,
			r22, r23, r24, r25, r26, r27, r28, r29, r30, r31,
		};

		enum
		{
			f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11,
			f12, f13, f14, f15, F16, f17, f18, f19, f20, f21,
			f22, f23, f24, f25, f26, f27, f28, f29, f30, f31,
		};

		enum
		{
			v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,
			v12, v13, v14, v15, v16, v17, v18, v19, v20, v21,
			v22, v23, v24, v25, v26, v27, v28, v29, v30, v31,
		};

		enum
		{
			cr0, cr1, cr2, cr3, cr4, cr5, cr6, cr7,
		};
	}

	using namespace fields;

	inline u32 ADDI(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x0eu << 26 }; op.rd = rt; op.ra = ra; op.simm16 = si; return op.opcode; }
	inline u32 ADDIS(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x0fu << 26 }; op.rd = rt; op.ra = ra; op.simm16 = si; return op.opcode; }
	inline u32 ORI(u32 rt, u32 ra, u32 ui) { ppu_opcode_t op{ 0x18u << 26 }; op.rd = rt; op.ra = ra; op.uimm16 = ui; return op.opcode; }
	inline u32 ORIS(u32 rt, u32 ra, u32 ui) { ppu_opcode_t op{ 0x19u << 26 }; op.rd = rt; op.ra = ra; op.uimm16 = ui; return op.opcode; }
	inline u32 OR(u32 ra, u32 rs, u32 rb, bool rc = false) { ppu_opcode_t op{ 0x1fu << 26 | 0x1bcu << 1 }; op.rs = rs; op.ra = ra; op.rb = rb; op.rc = rc; return op.opcode; }
	inline u32 SC(u32 lev) { ppu_opcode_t op{ 0x11u << 26 | 1 << 1 }; op.lev = lev; return op.opcode; }
	inline u32 B(s32 li, bool aa = false, bool lk = false) { ppu_opcode_t op{0x12u << 26}; op.ll = li; op.aa = aa; op.lk = lk; return op.opcode; }
	inline u32 BC(u32 bo, u32 bi, s32 bd, bool aa = false, bool lk = false) { ppu_opcode_t op{ 0x10u << 26 }; op.bo = bo; op.bi = bi; op.ds = bd / 4; op.aa = aa; op.lk = lk; return op.opcode; }
	inline u32 BCLR(u32 bo, u32 bi, u32 bh, bool lk = false) { ppu_opcode_t op{ 0x13u << 26 | 0x10u << 1 }; op.bo = bo; op.bi = bi; op.bh = bh; op.lk = lk; return op.opcode; }
	inline u32 BCCTR(u32 bo, u32 bi, u32 bh, bool lk = false) { ppu_opcode_t op{ 0x13u << 26 | 0x210u << 1 }; op.bo = bo; op.bi = bi; op.bh = bh; op.lk = lk; return op.opcode; }
	inline u32 MFSPR(u32 rt, u32 spr) { ppu_opcode_t op{ 0x1fu << 26 | 0x153u << 1 }; op.rd = rt; op.spr = spr; return op.opcode; }
	inline u32 MTSPR(u32 spr, u32 rs) { ppu_opcode_t op{ 0x1fu << 26 | 0x1d3u << 1 }; op.rs = rs; op.spr = spr; return op.opcode; }
	inline u32 LWZ(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x20u << 26 }; op.rd = rt; op.ra = ra; op.simm16 = si; return op.opcode; }
	inline u32 STD(u32 rs, u32 ra, s32 si) { ppu_opcode_t op{ 0x3eu << 26 }; op.rs = rs; op.ra = ra; op.ds = si / 4; return op.opcode; }
	inline u32 STDU(u32 rs, u32 ra, s32 si) { ppu_opcode_t op{ 0x3eu << 26 | 1 }; op.rs = rs; op.ra = ra; op.ds = si / 4; return op.opcode; }
	inline u32 LD(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x3au << 26 }; op.rd = rt; op.ra = ra; op.ds = si / 4; return op.opcode; }
	inline u32 LDU(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x3au << 26 | 1 }; op.rd = rt; op.ra = ra; op.ds = si / 4; return op.opcode; }
	inline u32 CMPI(u32 bf, u32 l, u32 ra, u32 ui) { ppu_opcode_t op{ 0xbu << 26 }; op.crfd = bf; op.l10 = l; op.ra = ra; op.uimm16 = ui; return op.opcode; }
	inline u32 CMPLI(u32 bf, u32 l, u32 ra, u32 ui) { ppu_opcode_t op{ 0xau << 26 }; op.crfd = bf; op.l10 = l; op.ra = ra; op.uimm16 = ui; return op.opcode; }
	inline u32 RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc = false) { ppu_opcode_t op{ 30 << 26 }; op.ra = ra; op.rs = rs; op.sh64 = sh; op.mbe64 = mb; op.rc = rc; return op.opcode; }
	inline u32 RLDICR(u32 ra, u32 rs, u32 sh, u32 mb, bool rc = false) { return RLDICL(ra, rs, sh, mb, rc) | 1 << 2; }
	inline u32 STFD(u32 frs, u32 ra, s32 si) { ppu_opcode_t op{ 54u << 26 }; op.frs = frs; op.ra = ra; op.simm16 = si; return op.opcode; }
	inline u32 STVX(u32 vs, u32 ra, u32 rb) { ppu_opcode_t op{ 31 << 26 | 231 << 1 }; op.vs = vs; op.ra = ra; op.rb = rb; return op.opcode; }
	inline u32 LFD(u32 frd, u32 ra, s32 si) { ppu_opcode_t op{ 50u << 26 }; op.frd = frd; op.ra = ra; op.simm16 = si; return op.opcode; }
	inline u32 LVX(u32 vd, u32 ra, u32 rb) { ppu_opcode_t op{ 31 << 26 | 103 << 1 }; op.vd = vd; op.ra = ra; op.rb = rb; return op.opcode; }

	namespace implicts
	{
		inline u32 NOP() { return ORI(r0, r0, 0); }
		inline u32 MR(u32 rt, u32 ra) { return OR(rt, ra, ra, false); }
		inline u32 LI(u32 rt, u32 imm) { return ADDI(rt, r0, imm); }
		inline u32 LIS(u32 rt, u32 imm) { return ADDIS(rt, r0, imm); }

		inline u32 BLR() { return BCLR(0x10 | 0x04, 0, 0); }
		inline u32 BCTR() { return BCCTR(0x10 | 0x04, 0, 0); }
		inline u32 BCTRL() { return BCCTR(0x10 | 0x04, 0, 0, true); }
		inline u32 MFCTR(u32 reg) { return MFSPR(reg, 9 << 5); }
		inline u32 MTCTR(u32 reg) { return MTSPR(9 << 5, reg); }
		inline u32 MFLR(u32 reg) { return MFSPR(reg, 8 << 5); }
		inline u32 MTLR(u32 reg) { return MTSPR(8 << 5, reg); }

		inline u32 BNE(u32 cr, s32 imm) { return BC(4, 2 | cr << 2, imm); }
		inline u32 BEQ(u32 cr, s32 imm) { return BC(12, 2 | cr << 2, imm); }
		inline u32 BGT(u32 cr, s32 imm) { return BC(12, 1 | cr << 2, imm); }
		inline u32 BNE(s32 imm) { return BNE(cr0, imm); }
		inline u32 BEQ(s32 imm) { return BEQ(cr0, imm); }
		inline u32 BGT(s32 imm) { return BGT(cr0, imm); }

		inline u32 CMPDI(u32 cr, u32 reg, u32 imm) { return CMPI(cr, 1, reg, imm); }
		inline u32 CMPDI(u32 reg, u32 imm) { return CMPDI(cr0, reg, imm); }
		inline u32 CMPWI(u32 cr, u32 reg, u32 imm) { return CMPI(cr, 0, reg, imm); }
		inline u32 CMPWI(u32 reg, u32 imm) { return CMPWI(cr0, reg, imm); }
		inline u32 CMPLDI(u32 cr, u32 reg, u32 imm) { return CMPLI(cr, 1, reg, imm); }
		inline u32 CMPLDI(u32 reg, u32 imm) { return CMPLDI(cr0, reg, imm); }
		inline u32 CMPLWI(u32 cr, u32 reg, u32 imm) { return CMPLI(cr, 0, reg, imm); }
		inline u32 CMPLWI(u32 reg, u32 imm) { return CMPLWI(cr0, reg, imm); }

		inline u32 EXTRDI(u32 x, u32 y, u32 n, u32 b) { return RLDICL(x, y, b + n, 64 - b, false); }
		inline u32 SRDI(u32 x, u32 y, u32 n) { return RLDICL(x, y, 64 - n, n, false); }
		inline u32 CLRLDI(u32 x, u32 y, u32 n) { return RLDICL(x, y, 0, n, false); }
		inline u32 CLRRDI(u32 x, u32 y, u32 n) { return RLDICR(x, y, 0, 63 - n, false); }

		inline u32 TRAP() { return 0x7FE00008; } // tw 31,r0,r0
	}

	using namespace implicts;
}
