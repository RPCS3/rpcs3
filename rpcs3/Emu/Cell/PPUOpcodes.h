#pragma once

#include "Utilities/BitField.h"

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

constexpr u64 ppu_rotate_mask(u32 mb, u32 me)
{
	const u64 mask = ~0ull << (~(me - mb) & 63);
	return (mask >> (mb & 63)) | (mask << ((64 - mb) & 63));
}

constexpr u32 ppu_decode(u32 inst)
{
	return ((inst >> 26) | (inst << 6)) & 0x1ffff; // Rotate + mask
}

std::array<u32, 2> op_branch_targets(u32 pc, ppu_opcode_t op);

// PPU decoder object. D provides functions. T is function pointer type returned.
template <typename D, typename T = decltype(&D::UNK)>
class ppu_decoder
{
	// Fast lookup table
	std::array<T, 0x20000> m_table{};

	struct instruction_info
	{
		u32 value;
		T ptr0;
		T ptr_rc;
		u32 magn; // Non-zero for "columns" (effectively, number of most significant bits "eaten")

		constexpr instruction_info(u32 v, T p, T p_rc, u32 m = 0)
			: value(v)
			, ptr0(p)
			, ptr_rc(p_rc)
			, magn(m)
		{
		}

		constexpr instruction_info(u32 v, const T* p, const T* p_rc, u32 m = 0)
			: value(v)
			, ptr0(*p)
			, ptr_rc(*p_rc)
			, magn(m)
		{
		}
	};

	// Fill lookup table
	void fill_table(u32 main_op, u32 count, u32 sh, std::initializer_list<instruction_info> entries) noexcept
	{
		if (sh < 11)
		{
			for (const auto& v : entries)
			{
				for (u32 i = 0; i < 1u << (v.magn + (11 - sh - count)); i++)
				{
					for (u32 j = 0; j < 1u << sh; j++)
					{
						const u32 k = (((i << (count - v.magn)) | v.value) << sh) | j;
						::at32(m_table, (k << 6) | main_op) = k & 1 ? v.ptr_rc : v.ptr0;
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
					::at32(m_table, i << 6 | v.value) = i & 1 ? v.ptr_rc : v.ptr0;
				}
			}
		}
	}

	// Helper
	static const D& _first(const D& arg)
	{
		return arg;
	}

public:
	template <typename... Args>
	ppu_decoder(const Args&... args) noexcept
	{
		// If an object is passed to the constructor, assign values from that object
#define GET_(name) [&]{ if constexpr (sizeof...(Args) > 0) return _first(args...).name; else return &D::name; }()
#define GET(name) GET_(name), GET_(name)
#define GETRC(name) GET_(name), GET_(name##_)

		static_assert(sizeof...(Args) <= 1);

		for (auto& x : m_table)
		{
			x = GET(UNK);
		}

		// Main opcodes (field 0..5)
		fill_table(0x00, 6, -1,
		{
			{ 0x02, GET(TDI) },
			{ 0x03, GET(TWI) },
			{ 0x07, GET(MULLI) },
			{ 0x08, GET(SUBFIC) },
			{ 0x0a, GET(CMPLI) },
			{ 0x0b, GET(CMPI) },
			{ 0x0c, GET(ADDIC) },
			{ 0x0d, GET(ADDIC) },
			{ 0x0e, GET(ADDI) },
			{ 0x0f, GET(ADDIS) },
			{ 0x10, GET(BC) },
			{ 0x11, GET(SC) },
			{ 0x12, GET(B) },
			{ 0x14, GETRC(RLWIMI) },
			{ 0x15, GETRC(RLWINM) },
			{ 0x17, GETRC(RLWNM) },
			{ 0x18, GET(ORI) },
			{ 0x19, GET(ORIS) },
			{ 0x1a, GET(XORI) },
			{ 0x1b, GET(XORIS) },
			{ 0x1c, GET(ANDI) },
			{ 0x1d, GET(ANDIS) },
			{ 0x20, GET(LWZ) },
			{ 0x21, GET(LWZU) },
			{ 0x22, GET(LBZ) },
			{ 0x23, GET(LBZU) },
			{ 0x24, GET(STW) },
			{ 0x25, GET(STWU) },
			{ 0x26, GET(STB) },
			{ 0x27, GET(STBU) },
			{ 0x28, GET(LHZ) },
			{ 0x29, GET(LHZU) },
			{ 0x2a, GET(LHA) },
			{ 0x2b, GET(LHAU) },
			{ 0x2c, GET(STH) },
			{ 0x2d, GET(STHU) },
			{ 0x2e, GET(LMW) },
			{ 0x2f, GET(STMW) },
			{ 0x30, GET(LFS) },
			{ 0x31, GET(LFSU) },
			{ 0x32, GET(LFD) },
			{ 0x33, GET(LFDU) },
			{ 0x34, GET(STFS) },
			{ 0x35, GET(STFSU) },
			{ 0x36, GET(STFD) },
			{ 0x37, GET(STFDU) },
		});

		// Group 0x04 opcodes (field 21..31)
		fill_table(0x04, 11, 0,
		{
			{ 0x0, GET(VADDUBM) },
			{ 0x2, GET(VMAXUB) },
			{ 0x4, GET(VRLB) },
			{ 0x006, GET(VCMPEQUB) },
			{ 0x406, GET(VCMPEQUB_) },
			{ 0x8, GET(VMULOUB) },
			{ 0xa, GET(VADDFP) },
			{ 0xc, GET(VMRGHB) },
			{ 0xe, GET(VPKUHUM) },

			{ 0x20, GET(VMHADDSHS), 5 },
			{ 0x21, GET(VMHRADDSHS), 5 },
			{ 0x22, GET(VMLADDUHM), 5 },
			{ 0x24, GET(VMSUMUBM), 5 },
			{ 0x25, GET(VMSUMMBM), 5 },
			{ 0x26, GET(VMSUMUHM), 5 },
			{ 0x27, GET(VMSUMUHS), 5 },
			{ 0x28, GET(VMSUMSHM), 5 },
			{ 0x29, GET(VMSUMSHS), 5 },
			{ 0x2a, GET(VSEL), 5 },
			{ 0x2b, GET(VPERM), 5 },
			{ 0x2c, GET(VSLDOI), 5 },
			{ 0x2e, GET(VMADDFP), 5 },
			{ 0x2f, GET(VNMSUBFP), 5 },

			{ 0x40, GET(VADDUHM) },
			{ 0x42, GET(VMAXUH) },
			{ 0x44, GET(VRLH) },
			{ 0x046, GET(VCMPEQUH) },
			{ 0x446, GET(VCMPEQUH_) },
			{ 0x48, GET(VMULOUH) },
			{ 0x4a, GET(VSUBFP) },
			{ 0x4c, GET(VMRGHH) },
			{ 0x4e, GET(VPKUWUM) },
			{ 0x80, GET(VADDUWM) },
			{ 0x82, GET(VMAXUW) },
			{ 0x84, GET(VRLW) },
			{ 0x086, GET(VCMPEQUW) },
			{ 0x486, GET(VCMPEQUW_) },
			{ 0x8c, GET(VMRGHW) },
			{ 0x8e, GET(VPKUHUS) },
			{ 0x0c6, GET(VCMPEQFP) },
			{ 0x4c6, GET(VCMPEQFP_) },
			{ 0xce, GET(VPKUWUS) },

			{ 0x102, GET(VMAXSB) },
			{ 0x104, GET(VSLB) },
			{ 0x108, GET(VMULOSB) },
			{ 0x10a, GET(VREFP) },
			{ 0x10c, GET(VMRGLB) },
			{ 0x10e, GET(VPKSHUS) },
			{ 0x142, GET(VMAXSH) },
			{ 0x144, GET(VSLH) },
			{ 0x148, GET(VMULOSH) },
			{ 0x14a, GET(VRSQRTEFP) },
			{ 0x14c, GET(VMRGLH) },
			{ 0x14e, GET(VPKSWUS) },
			{ 0x180, GET(VADDCUW) },
			{ 0x182, GET(VMAXSW) },
			{ 0x184, GET(VSLW) },
			{ 0x18a, GET(VEXPTEFP) },
			{ 0x18c, GET(VMRGLW) },
			{ 0x18e, GET(VPKSHSS) },
			{ 0x1c4, GET(VSL) },
			{ 0x1c6, GET(VCMPGEFP) },
			{ 0x5c6, GET(VCMPGEFP_) },
			{ 0x1ca, GET(VLOGEFP) },
			{ 0x1ce, GET(VPKSWSS) },
			{ 0x200, GET(VADDUBS) },
			{ 0x202, GET(VMINUB) },
			{ 0x204, GET(VSRB) },
			{ 0x206, GET(VCMPGTUB) },
			{ 0x606, GET(VCMPGTUB_) },
			{ 0x208, GET(VMULEUB) },
			{ 0x20a, GET(VRFIN) },
			{ 0x20c, GET(VSPLTB) },
			{ 0x20e, GET(VUPKHSB) },
			{ 0x240, GET(VADDUHS) },
			{ 0x242, GET(VMINUH) },
			{ 0x244, GET(VSRH) },
			{ 0x246, GET(VCMPGTUH) },
			{ 0x646, GET(VCMPGTUH_) },
			{ 0x248, GET(VMULEUH) },
			{ 0x24a, GET(VRFIZ) },
			{ 0x24c, GET(VSPLTH) },
			{ 0x24e, GET(VUPKHSH) },
			{ 0x280, GET(VADDUWS) },
			{ 0x282, GET(VMINUW) },
			{ 0x284, GET(VSRW) },
			{ 0x286, GET(VCMPGTUW) },
			{ 0x686, GET(VCMPGTUW_) },
			{ 0x28a, GET(VRFIP) },
			{ 0x28c, GET(VSPLTW) },
			{ 0x28e, GET(VUPKLSB) },
			{ 0x2c4, GET(VSR) },
			{ 0x2c6, GET(VCMPGTFP) },
			{ 0x6c6, GET(VCMPGTFP_) },
			{ 0x2ca, GET(VRFIM) },
			{ 0x2ce, GET(VUPKLSH) },
			{ 0x300, GET(VADDSBS) },
			{ 0x302, GET(VMINSB) },
			{ 0x304, GET(VSRAB) },
			{ 0x306, GET(VCMPGTSB) },
			{ 0x706, GET(VCMPGTSB_) },
			{ 0x308, GET(VMULESB) },
			{ 0x30a, GET(VCFUX) },
			{ 0x30c, GET(VSPLTISB) },
			{ 0x30e, GET(VPKPX) },
			{ 0x340, GET(VADDSHS) },
			{ 0x342, GET(VMINSH) },
			{ 0x344, GET(VSRAH) },
			{ 0x346, GET(VCMPGTSH) },
			{ 0x746, GET(VCMPGTSH_) },
			{ 0x348, GET(VMULESH) },
			{ 0x34a, GET(VCFSX) },
			{ 0x34c, GET(VSPLTISH) },
			{ 0x34e, GET(VUPKHPX) },
			{ 0x380, GET(VADDSWS) },
			{ 0x382, GET(VMINSW) },
			{ 0x384, GET(VSRAW) },
			{ 0x386, GET(VCMPGTSW) },
			{ 0x786, GET(VCMPGTSW_) },
			{ 0x38a, GET(VCTUXS) },
			{ 0x38c, GET(VSPLTISW) },
			{ 0x3c6, GET(VCMPBFP) },
			{ 0x7c6, GET(VCMPBFP_) },
			{ 0x3ca, GET(VCTSXS) },
			{ 0x3ce, GET(VUPKLPX) },
			{ 0x400, GET(VSUBUBM) },
			{ 0x402, GET(VAVGUB) },
			{ 0x404, GET(VAND) },
			{ 0x40a, GET(VMAXFP) },
			{ 0x40c, GET(VSLO) },
			{ 0x440, GET(VSUBUHM) },
			{ 0x442, GET(VAVGUH) },
			{ 0x444, GET(VANDC) },
			{ 0x44a, GET(VMINFP) },
			{ 0x44c, GET(VSRO) },
			{ 0x480, GET(VSUBUWM) },
			{ 0x482, GET(VAVGUW) },
			{ 0x484, GET(VOR) },
			{ 0x4c4, GET(VXOR) },
			{ 0x502, GET(VAVGSB) },
			{ 0x504, GET(VNOR) },
			{ 0x542, GET(VAVGSH) },
			{ 0x580, GET(VSUBCUW) },
			{ 0x582, GET(VAVGSW) },
			{ 0x600, GET(VSUBUBS) },
			{ 0x604, GET(MFVSCR) },
			{ 0x608, GET(VSUM4UBS) },
			{ 0x640, GET(VSUBUHS) },
			{ 0x644, GET(MTVSCR) },
			{ 0x648, GET(VSUM4SHS) },
			{ 0x680, GET(VSUBUWS) },
			{ 0x688, GET(VSUM2SWS) },
			{ 0x700, GET(VSUBSBS) },
			{ 0x708, GET(VSUM4SBS) },
			{ 0x740, GET(VSUBSHS) },
			{ 0x780, GET(VSUBSWS) },
			{ 0x788, GET(VSUMSWS) },
		});

		// Group 0x13 opcodes (field 21..30)
		fill_table(0x13, 10, 1,
		{
			{ 0x000, GET(MCRF) },
			{ 0x010, GET(BCLR) },
			{ 0x021, GET(CRNOR) },
			{ 0x081, GET(CRANDC) },
			{ 0x096, GET(ISYNC) },
			{ 0x0c1, GET(CRXOR) },
			{ 0x0e1, GET(CRNAND) },
			{ 0x101, GET(CRAND) },
			{ 0x121, GET(CREQV) },
			{ 0x1a1, GET(CRORC) },
			{ 0x1c1, GET(CROR) },
			{ 0x210, GET(BCCTR) },
		});

		// Group 0x1e opcodes (field 27..30)
		fill_table(0x1e, 4, 1,
		{
			{ 0x0, GETRC(RLDICL) },
			{ 0x1, GETRC(RLDICL) },
			{ 0x2, GETRC(RLDICR) },
			{ 0x3, GETRC(RLDICR) },
			{ 0x4, GETRC(RLDIC) },
			{ 0x5, GETRC(RLDIC) },
			{ 0x6, GETRC(RLDIMI) },
			{ 0x7, GETRC(RLDIMI) },
			{ 0x8, GETRC(RLDCL) },
			{ 0x9, GETRC(RLDCR) },
		});

		// Group 0x1f opcodes (field 21..30)
		fill_table(0x1f, 10, 1,
		{
			{ 0x000, GET(CMP) },
			{ 0x004, GET(TW) },
			{ 0x006, GET(LVSL) },
			{ 0x007, GET(LVEBX) },
			{ 0x008, GETRC(SUBFC) },
			{ 0x208, GETRC(SUBFCO) },
			{ 0x009, GETRC(MULHDU) },
			{ 0x00a, GETRC(ADDC) },
			{ 0x20a, GETRC(ADDCO) },
			{ 0x00b, GETRC(MULHWU) },
			{ 0x013, GET(MFOCRF) },
			{ 0x014, GET(LWARX) },
			{ 0x015, GET(LDX) },
			{ 0x017, GET(LWZX) },
			{ 0x018, GETRC(SLW) },
			{ 0x01a, GETRC(CNTLZW) },
			{ 0x01b, GETRC(SLD) },
			{ 0x01c, GETRC(AND) },
			{ 0x020, GET(CMPL) },
			{ 0x026, GET(LVSR) },
			{ 0x027, GET(LVEHX) },
			{ 0x028, GETRC(SUBF) },
			{ 0x228, GETRC(SUBFO) },
			{ 0x035, GET(LDUX) },
			{ 0x036, GET(DCBST) },
			{ 0x037, GET(LWZUX) },
			{ 0x03a, GETRC(CNTLZD) },
			{ 0x03c, GETRC(ANDC) },
			{ 0x044, GET(TD) },
			{ 0x047, GET(LVEWX) },
			{ 0x049, GETRC(MULHD) },
			{ 0x04b, GETRC(MULHW) },
			{ 0x054, GET(LDARX) },
			{ 0x056, GET(DCBF) },
			{ 0x057, GET(LBZX) },
			{ 0x067, GET(LVX) },
			{ 0x068, GETRC(NEG) },
			{ 0x268, GETRC(NEGO) },
			{ 0x077, GET(LBZUX) },
			{ 0x07c, GETRC(NOR) },
			{ 0x087, GET(STVEBX) },
			{ 0x088, GETRC(SUBFE) },
			{ 0x288, GETRC(SUBFEO) },
			{ 0x08a, GETRC(ADDE) },
			{ 0x28a, GETRC(ADDEO) },
			{ 0x090, GET(MTOCRF) },
			{ 0x095, GET(STDX) },
			{ 0x096, GET(STWCX) },
			{ 0x097, GET(STWX) },
			{ 0x0a7, GET(STVEHX) },
			{ 0x0b5, GET(STDUX) },
			{ 0x0b7, GET(STWUX) },
			{ 0x0c7, GET(STVEWX) },
			{ 0x0c8, GETRC(SUBFZE) },
			{ 0x2c8, GETRC(SUBFZEO) },
			{ 0x0ca, GETRC(ADDZE) },
			{ 0x2ca, GETRC(ADDZEO) },
			{ 0x0d6, GET(STDCX) },
			{ 0x0d7, GET(STBX) },
			{ 0x0e7, GET(STVX) },
			{ 0x0e8, GETRC(SUBFME) },
			{ 0x2e8, GETRC(SUBFMEO) },
			{ 0x0e9, GETRC(MULLD) },
			{ 0x2e9, GETRC(MULLDO) },
			{ 0x0ea, GETRC(ADDME) },
			{ 0x2ea, GETRC(ADDMEO) },
			{ 0x0eb, GETRC(MULLW) },
			{ 0x2eb, GETRC(MULLWO) },
			{ 0x0f6, GET(DCBTST) },
			{ 0x0f7, GET(STBUX) },
			{ 0x10a, GETRC(ADD) },
			{ 0x30a, GETRC(ADDO) },
			{ 0x116, GET(DCBT) },
			{ 0x117, GET(LHZX) },
			{ 0x11c, GETRC(EQV) },
			{ 0x136, GET(ECIWX) },
			{ 0x137, GET(LHZUX) },
			{ 0x13c, GETRC(XOR) },
			{ 0x153, GET(MFSPR) },
			{ 0x155, GET(LWAX) },
			{ 0x156, GET(DST) },
			{ 0x157, GET(LHAX) },
			{ 0x167, GET(LVXL) },
			{ 0x173, GET(MFTB) },
			{ 0x175, GET(LWAUX) },
			{ 0x176, GET(DSTST) },
			{ 0x177, GET(LHAUX) },
			{ 0x197, GET(STHX) },
			{ 0x19c, GETRC(ORC) },
			{ 0x1b6, GET(ECOWX) },
			{ 0x1b7, GET(STHUX) },
			{ 0x1bc, GETRC(OR) },
			{ 0x1c9, GETRC(DIVDU) },
			{ 0x3c9, GETRC(DIVDUO) },
			{ 0x1cb, GETRC(DIVWU) },
			{ 0x3cb, GETRC(DIVWUO) },
			{ 0x1d3, GET(MTSPR) },
			{ 0x1d6, GET(DCBI) },
			{ 0x1dc, GETRC(NAND) },
			{ 0x1e7, GET(STVXL) },
			{ 0x1e9, GETRC(DIVD) },
			{ 0x3e9, GETRC(DIVDO) },
			{ 0x1eb, GETRC(DIVW) },
			{ 0x3eb, GETRC(DIVWO) },
			{ 0x207, GET(LVLX) },
			{ 0x214, GET(LDBRX) },
			{ 0x215, GET(LSWX) },
			{ 0x216, GET(LWBRX) },
			{ 0x217, GET(LFSX) },
			{ 0x218, GETRC(SRW) },
			{ 0x21b, GETRC(SRD) },
			{ 0x227, GET(LVRX) },
			{ 0x237, GET(LFSUX) },
			{ 0x255, GET(LSWI) },
			{ 0x256, GET(SYNC) },
			{ 0x257, GET(LFDX) },
			{ 0x277, GET(LFDUX) },
			{ 0x287, GET(STVLX) },
			{ 0x294, GET(STDBRX) },
			{ 0x295, GET(STSWX) },
			{ 0x296, GET(STWBRX) },
			{ 0x297, GET(STFSX) },
			{ 0x2a7, GET(STVRX) },
			{ 0x2b7, GET(STFSUX) },
			{ 0x2d5, GET(STSWI) },
			{ 0x2d7, GET(STFDX) },
			{ 0x2f7, GET(STFDUX) },
			{ 0x307, GET(LVLXL) },
			{ 0x316, GET(LHBRX) },
			{ 0x318, GETRC(SRAW) },
			{ 0x31a, GETRC(SRAD) },
			{ 0x327, GET(LVRXL) },
			{ 0x336, GET(DSS) },
			{ 0x338, GETRC(SRAWI) },
			{ 0x33a, GETRC(SRADI) },
			{ 0x33b, GETRC(SRADI) },
			{ 0x356, GET(EIEIO) },
			{ 0x387, GET(STVLXL) },
			{ 0x396, GET(STHBRX) },
			{ 0x39a, GETRC(EXTSH) },
			{ 0x3a7, GET(STVRXL) },
			{ 0x3ba, GETRC(EXTSB) },
			{ 0x3d7, GET(STFIWX) },
			{ 0x3da, GETRC(EXTSW) },
			{ 0x3d6, GET(ICBI) },
			{ 0x3f6, GET(DCBZ) },
		});

		// Group 0x3a opcodes (field 30..31)
		fill_table(0x3a, 2, 0,
		{
			{ 0x0, GET(LD) },
			{ 0x1, GET(LDU) },
			{ 0x2, GET(LWA) },
		});

		// Group 0x3b opcodes (field 21..30)
		fill_table(0x3b, 10, 1,
		{
			{ 0x12, GETRC(FDIVS), 5 },
			{ 0x14, GETRC(FSUBS), 5 },
			{ 0x15, GETRC(FADDS), 5 },
			{ 0x16, GETRC(FSQRTS), 5 },
			{ 0x18, GETRC(FRES), 5 },
			{ 0x19, GETRC(FMULS), 5 },
			{ 0x1c, GETRC(FMSUBS), 5 },
			{ 0x1d, GETRC(FMADDS), 5 },
			{ 0x1e, GETRC(FNMSUBS), 5 },
			{ 0x1f, GETRC(FNMADDS), 5 },
		});

		// Group 0x3e opcodes (field 30..31)
		fill_table(0x3e, 2, 0,
		{
			{ 0x0, GET(STD) },
			{ 0x1, GET(STDU) },
		});

		// Group 0x3f opcodes (field 21..30)
		fill_table(0x3f, 10, 1,
		{
			{ 0x026, GETRC(MTFSB1) },
			{ 0x040, GET(MCRFS) },
			{ 0x046, GETRC(MTFSB0) },
			{ 0x086, GETRC(MTFSFI) },
			{ 0x247, GETRC(MFFS) },
			{ 0x2c7, GETRC(MTFSF) },

			{ 0x000, GET(FCMPU) },
			{ 0x00c, GETRC(FRSP) },
			{ 0x00e, GETRC(FCTIW) },
			{ 0x00f, GETRC(FCTIWZ) },

			{ 0x012, GETRC(FDIV), 5 },
			{ 0x014, GETRC(FSUB), 5 },
			{ 0x015, GETRC(FADD), 5 },
			{ 0x016, GETRC(FSQRT), 5 },
			{ 0x017, GETRC(FSEL), 5 },
			{ 0x019, GETRC(FMUL), 5 },
			{ 0x01a, GETRC(FRSQRTE), 5 },
			{ 0x01c, GETRC(FMSUB), 5 },
			{ 0x01d, GETRC(FMADD), 5 },
			{ 0x01e, GETRC(FNMSUB), 5 },
			{ 0x01f, GETRC(FNMADD), 5 },

			{ 0x020, GET(FCMPO) },
			{ 0x028, GETRC(FNEG) },
			{ 0x048, GETRC(FMR) },
			{ 0x088, GETRC(FNABS) },
			{ 0x108, GETRC(FABS) },
			{ 0x32e, GETRC(FCTID) },
			{ 0x32f, GETRC(FCTIDZ) },
			{ 0x34e, GETRC(FCFID) },
		});
	}

	const std::array<T, 0x20000>& get_table() const noexcept
	{
		return m_table;
	}

	T decode(u32 inst) const noexcept
	{
		return m_table[ppu_decode(inst)];
	}
};

#undef GET_
#undef GET
#undef GETRC

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
	inline u32 XORIS(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x1bu << 26 }; op.rd = rt; op.ra = ra; op.simm16 = si; return op.opcode; }
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
	inline u32 STW(u32 rt, u32 ra, s32 si) { ppu_opcode_t op{ 0x24u << 26 }; op.rd = rt; op.ra = ra; op.simm16 = si; return op.opcode; }
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
	inline constexpr u32 EIEIO() { return 0x7c0006ac; }

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

		inline constexpr u32 TRAP() { return 0x7FE00008; } // tw 31,r0,r0
	}

	using namespace implicts;
}
