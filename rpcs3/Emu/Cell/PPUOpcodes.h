#pragma once

#define OP_REG u32
#define OP_sIMM s32
#define OP_uIMM u32
#define START_OPCODES_GROUP(x) /*x*/
#define ADD_OPCODE(name, regs) virtual void(##name##)##regs##=0
#define ADD_NULL_OPCODE(name) virtual void(##name##)()=0
#define END_OPCODES_GROUP(x) /*x*/

namespace PPU_opcodes
{
	enum PPU_MainOpcodes
	{
		TDI 	= 0x02, //Trap Doubleword Immediate 
		TWI 	= 0x03, //Trap Word Immediate
		G_04 	= 0x04,
		MULLI 	= 0x07, //Multiply Low Immediate
		SUBFIC 	= 0x08, //Subtract from Immediate Carrying
		//DOZI	= 0x09,
		CMPLI 	= 0x0a, //Compare Logical Immediate
		CMPI 	= 0x0b, //Compare Immediate
		ADDIC 	= 0x0c, //Add Immediate Carrying
		ADDIC_ 	= 0x0d, //Add Immediate Carrying and Record
		ADDI 	= 0x0e, //Add Immediate
		ADDIS 	= 0x0f, //Add Immediate Shifted
		BC 		= 0x10, //Branch Conditional
		SC 		= 0x11, //System Call
		B 		= 0x12, //Branch
		G_13 	= 0x13,
		RLWIMI 	= 0x14, //Rotate Left Word Immediate then Mask Insert
		RLWINM 	= 0x15, //Rotate Left Word Immediate then AND with Mask
		RLWNM 	= 0x17, //Rotate Left Word then AND with Mask
		ORI 	= 0x18, //OR Immediate
		ORIS 	= 0x19, //OR Immediate Shifted
		XORI 	= 0x1a, //XOR Immediate
		XORIS 	= 0x1b, //XOR Immediate Shifted
		ANDI_ 	= 0x1c, //AND Immediate
		ANDIS_ 	= 0x1d, //AND Immediate Shifted
		G_1e 	= 0x1e,
		G_1f 	= 0x1f,
		LWZ 	= 0x20, //Load Word and Zero Indexed
		LWZU 	= 0x21, //Load Word and Zero with Update Indexed
		LBZ 	= 0x22, //Load Byte and Zero
		LBZU 	= 0x23, //Load Byte and Zero with Update
		STW 	= 0x24, //Store Word
		STWU 	= 0x25, //Store Word with Update
		STB 	= 0x26, //Store Byte
		STBU 	= 0x27, //Store Byte with Update
		LHZ 	= 0x28, //Load Halfword and Zero
		LHZU 	= 0x29, //Load Halfword and Zero with Update
		LHA		= 0x2a, //Load Halfword Algebraic with Update
		LHAU 	= 0x2b, //Load Halfword Algebraic
		STH 	= 0x2c, //Store Halfword
		STHU 	= 0x2d, //Store Halfword with Update
		LMW 	= 0x2e, //Load Multiple Word
		STMW 	= 0x2f, //Store Multiple Word
		LFS 	= 0x30, //Load Floating-Point Single
		LFSU 	= 0x31, //Load Floating-Point Single with Update
		LFD 	= 0x32, //Load Floating-Point Double
		LFDU 	= 0x33, //Load Floating-Point Double with Update
		STFS 	= 0x34, //Store Floating-Point Single
		STFSU 	= 0x35, //Store Floating-Point Single with Update
		STFD 	= 0x36, //Store Floating-Point Double
		STFDU 	= 0x37, //Store Floating-Point Double with Update
		LFQ 	= 0x38, //
		LFQU 	= 0x39, //
		G_3a 	= 0x3a,
		G_3b 	= 0x3b,
		G_3e 	= 0x3e,
		G_3f 	= 0x3f,
	};

	enum G_04Opcodes
	{
		VADDUBM = 0x0,
		VMAXUB = 0x2,
		VRLB = 0x4,
		VCMPEQUB = 0x6,
		VMULOUB = 0x8,
		VADDFP = 0xa,
		VMRGHB = 0xc,
		VPKUHUM = 0xe,
		VADDUHM = 0x40,
		VMAXUH = 0x42,
		VRLH = 0x44,
		VCMPEQUH = 0x46,
		VMULOUH = 0x48,
		VSUBFP = 0x4a,
		VMRGHH = 0x4c,
		VPKUWUM = 0x4e,
		VADDUWM = 0x80,
		VMAXUW = 0x82,
		VRLW = 0x84,
		VCMPEQUW = 0x86,
		VMRGHW = 0x8c,
		VPKUHUS = 0x8e,
		VCMPEQFP = 0xc6,
		VPKUWUS = 0xce,
		VMAXSB = 0x102,
		VSLB = 0x104,
		VMULOSB = 0x108,
		VREFP = 0x10a,
		VMRGLB = 0x10c,
		VPKSHUS = 0x10e,
		VMAXSH = 0x142,
		VSLH = 0x144,
		VMULOSH = 0x148,
		VRSQRTEFP = 0x14a,
		VMRGLH = 0x14c,
		VPKSWUS = 0x14e,
		VADDCUW = 0x180,
		VMAXSW = 0x182,
		VSLW = 0x184,
		VEXPTEFP = 0x18a,
		VMRGLW = 0x18c,
		VPKSHSS = 0x18e,
		VSL = 0x1c4,
		VCMPGEFP = 0x1c6,
		VLOGEFP = 0x1ca,
		VPKSWSS = 0x1ce,
		VADDUBS = 0x200,
		VMINUB = 0x202,
		VSRB = 0x204,
		VCMPGTUB = 0x206,
		VMULEUB = 0x208,
		VRFIN = 0x20a,
		VSPLTB = 0x20c,
		VUPKHSB = 0x20e,
		VADDUHS = 0x240,
		VMINUH = 0x242,
		VSRH = 0x244,
		VCMPGTUH = 0x246,
		VMULEUH = 0x248,
		VRFIZ = 0x24a,
		VSPLTH = 0x24c,
		VUPKHSH = 0x24e,
		VADDUWS = 0x280,
		VMINUW = 0x282,
		VSRW = 0x284,
		VCMPGTUW = 0x286,
		VRFIP = 0x28a,
		VSPLTW = 0x28c,
		VUPKLSB = 0x28e,
		VSR = 0x2c4,
		VCMPGTFP = 0x2c6,
		VRFIM = 0x2ca,
		VUPKLSH = 0x2ce,
		VADDSBS = 0x300,
		VMINSB = 0x302,
		VSRAB = 0x304,
		VCMPGTSB = 0x306,
		VMULESB = 0x308,
		VCFUX = 0x30a,
		VSPLTISB = 0x30c,
		VPKPX = 0x30e,
		VADDSHS = 0x340,
		VMINSH = 0x342,
		VSRAH = 0x344,
		VCMPGTSH = 0x346,
		VMULESH = 0x348,
		VCFSX = 0x34a,
		VSPLTISH = 0x34c,
		VUPKHPX = 0x34e,
		VADDSWS = 0x380,
		VMINSW = 0x382,
		VSRAW = 0x384,
		VCMPGTSW = 0x386,
		VCTUXS = 0x38a,
		VSPLTISW = 0x38c,
		VCMPBFP = 0x3c6,
		VCTSXS = 0x3ca,
		VUPKLPX = 0x3ce,
		VSUBUBM = 0x400,
		VAVGUB = 0x402,
		VAND = 0x404,
		VCMPEQUB_ = 0x406,
		VMAXFP = 0x40a,
		VSLO = 0x40c,
		VSUBUHM = 0x440,
		VAVGUH = 0x442,
		VANDC = 0x444,
		VCMPEQUH_ = 0x446,
		VMINFP = 0x44a,
		VSRO = 0x44c,
		VSUBUWM = 0x480,
		VAVGUW = 0x482,
		VOR = 0x484,
		VCMPEQUW_ = 0x486,
		VXOR = 0x4c4,
		VCMPEQFP_ = 0x4c6,
		VAVGSB = 0x502,
		VNOR = 0x504,
		VAVGSH = 0x542,
		VSUBCUW = 0x580,
		VAVGSW = 0x582,
		VCMPGEFP_ = 0x5c6,
		VSUBUBS = 0x600,
		MFVSCR = 0x604,
		VCMPGTUB_ = 0x606,
		VSUM4UBS = 0x608,
		VSUBUHS = 0x640,
		MTVSCR = 0x644,
		VCMPGTUH_ = 0x646,
		VSUM4SHS = 0x648,
		VSUBUWS = 0x680,
		VCMPGTUW_ = 0x686,
		VSUM2SWS = 0x688,
		VCMPGTFP_ = 0x6c6,
		VSUBSBS = 0x700,
		VCMPGTSB_ = 0x706,
		VSUM4SBS = 0x708,
		VSUBSHS = 0x740,
		VCMPGTSH_ = 0x746,
		VSUBSWS = 0x780,
		VCMPGTSW_ = 0x786,
		VSUMSWS = 0x788,
		VCMPBFP_ = 0x7c6,
	};

	enum G_04_VA_Opcodes
	{
		VMHADDSHS = 0x20,
		VMHRADDSHS = 0x21,
		VMLADDUHM = 0x22,
		VMSUMUBM = 0x24,
		VMSUMMBM = 0x25,
		VMSUMUHM = 0x26,
		VMSUMUHS = 0x27,
		VMSUMSHM = 0x28,
		VMSUMSHS = 0x29,
		VSEL = 0x2a,
		VPERM = 0x2b,
		VSLDOI = 0x2c,
		VMADDFP = 0x2e,
		VNMSUBFP = 0x2f,
	};

	enum G_13Opcodes //Field 21 - 30
	{
		MCRF 	= 0x000,
		BCLR 	= 0x010,
		CRNOR 	= 0x021,
		CRANDC 	= 0x081,
		ISYNC 	= 0x096,
		CRXOR 	= 0x0c1,
		CRNAND 	= 0x0e1,
		CRAND 	= 0x101,
		CREQV 	= 0x121,
		CRORC 	= 0x1a1,
		CROR 	= 0x1c1,
		BCCTR 	= 0x210,
	};

	enum G_1eOpcodes //Field 27 - 29
	{
		RLDICL 	= 0x0,
		RLDICR 	= 0x1,
		RLDIC 	= 0x2,
		RLDIMI 	= 0x3,
	};

	enum G_1fOpcodes //Field 21 - 30
	{
		CMP 	= 0x000,
		TW 		= 0x004,
		LVSL		= 0x006, //Load Vector for Shift Left
		LVEBX 	= 0x007, //Load Vector Element Byte Indexed
		SUBFC 	= 0x008, //Subtract from Carrying
		MULHDU 	= 0x009,
		ADDC 	= 0x00a,
		MULHWU 	= 0x00b,
		MFOCRF 	= 0x013,
		LWARX 	= 0x014,
		LDX 	= 0x015,
		LWZX 	= 0x017,
		SLW 	= 0x018,
		CNTLZW 	= 0x01a,
		SLD 	= 0x01b,
		AND 	= 0x01c,
		CMPL 	= 0x020,
		LVSR	= 0x026, //Load Vector for Shift Right
		LVEHX 	= 0x027, //Load Vector Element Halfword Indexed
		SUBF 	= 0x028,
		LDUX 	= 0x035, //Load Doubleword with Update Indexed
		DCBST 	= 0x036,
		CNTLZD 	= 0x03a,
		ANDC 	= 0x03c,
		LVEWX 	= 0x047, //Load Vector Element Word Indexed
		MULHD 	= 0x049,
		MULHW 	= 0x04b,
		LDARX 	= 0x054,
		DCBF 	= 0x056,
		LBZX 	= 0x057,
		LVX 	= 0x067, //Load Vector Indexed
		NEG 	= 0x068,
		LBZUX 	= 0x077,
		NOR 	= 0x07c,
		STVEBX	= 0x087, //Store Vector Element Byte Indexed
		SUBFE 	= 0x088, //Subtract from Extended
		ADDE 	= 0x08a,
		MTOCRF 	= 0x090,
		STDX 	= 0x095,
		STWCX_ 	= 0x096,
		STWX 	= 0x097,
		STVEHX	= 0x0a7, //Store Vector Element Halfword Indexed
		STDUX 	= 0x0b5,
		STVEWX	= 0x0c7, //Store Vector Element Word Indexed
		ADDZE 	= 0x0ca,
		STDCX_ 	= 0x0d6,
		STBX 	= 0x0d7,
		STVX 	= 0x0e7,
		MULLD 	= 0x0e9,
		ADDME 	= 0x0ea,
		MULLW 	= 0x0eb,
		DCBTST 	= 0x0f6,
		DOZ 	= 0x108,
		ADD 	= 0x10a,
		DCBT 	= 0x116,
		LHZX 	= 0x117,
		EQV 	= 0x11c,
		ECIWX 	= 0x136,
		LHZUX 	= 0x137,
		XOR 	= 0x13c,
		MFSPR 	= 0x153,
		DST		= 0x156, //Data Stream Touch
		LHAX 	= 0x157,
		LVXL 	= 0x167, //Load Vector Indexed Last
		ABS 	= 0x168,
		MFTB 	= 0x173,
		DSTST	= 0x176, //Data Stream Touch for Store
		LHAUX 	= 0x177,
		STHX	= 0x197, //Store Halfword Indexed
		ORC 	= 0x19c, //OR with Complement
		ECOWX 	= 0x1b6,
		OR 		= 0x1bc,
		DIVDU 	= 0x1c9,
		DIVWU 	= 0x1cb,
		MTSPR 	= 0x1d3,
		DCBI 	= 0x1d6,
		NAND	= 0x1dc,
		STVXL	= 0x1e7, //Store Vector Indexed Last
		DIVD 	= 0x1e9,
		DIVW 	= 0x1eb,
		LVLX	= 0x207, //Load Vector Left Indexed
		LWBRX 	= 0x216,
		LFSX 	= 0x217,
		SRW 	= 0x218,
		SRD 	= 0x21b,
		LVRX	= 0x227, //Load Vector Right Indexed
		LFSUX 	= 0x237,
		SYNC 	= 0x256,
		LFDX 	= 0x257,
		LFDUX	= 0x277,
		STVLX	= 0x287, //Store Vector Left Indexed
		STFSX 	= 0x297,
		STVRX	= 0x2a7, //Store Vector Right Indexed
		STFDX	= 0x2d7, //Store Floating-Point Double Indexed
		LVLXL	= 0x307, //Load Vector Left Indexed Last
		LHBRX 	= 0x316,
		SRAW 	= 0x318,
		SRAD 	= 0x31a,
		LVRXL	= 0x327, //Load Vector Right Indexed Last
		DSS		= 0x336, //Data Stream Stop
		SRAWI 	= 0x338,
		SRADI1 	= 0x33a, //sh_5 == 0
		SRADI2 	= 0x33b, //sh_5 != 0
		EIEIO	= 0x356,
		STVLXL	= 0x387, //Store Vector Left Indexed Last
		EXTSH 	= 0x39a,
		STVRXL	= 0x3a7, //Store Vector Right Indexed Last
		EXTSB 	= 0x3ba,
		STFIWX 	= 0x3d7,
		EXTSW 	= 0x3da,
		ICBI 	= 0x3d6,
		DCBZ 	= 0x3f6,
	};

	enum G_3aOpcodes //Field 30 - 31
	{
		LD 	= 0x0,
		LDU	= 0x1,
	};

	enum G_3bOpcodes //Field 26 - 30
	{
		FDIVS 	= 0x12,
		FSUBS 	= 0x14,
		FADDS 	= 0x15,
		FSQRTS 	= 0x16,
		FRES 	= 0x18,
		FMULS 	= 0x19,
		FMSUBS 	= 0x1c,
		FMADDS 	= 0x1d,
		FNMSUBS	= 0x1e,
		FNMADDS	= 0x1f,
	};

	enum G_3eOpcodes //Field 30 - 31
	{
		STD		= 0x0,
		STDU	= 0x1,
	};

	enum G_3fOpcodes //Field 21 - 30
	{	
		MTFSB1	= 0x026,
		MCRFS	= 0x040,
		MTFSB0	= 0x046,
		MTFSFI 	= 0x086,
		MFFS 	= 0x247,
		MTFSF	= 0x2c7,

		FCMPU 	= 0x000,
		FRSP 	= 0x00c,
		FCTIW 	= 0x00e,
		FCTIWZ 	= 0x00f,
		FDIV 	= 0x012,
		FSUB 	= 0x014,
		FADD 	= 0x015,
		FSQRT 	= 0x016,
		FSEL 	= 0x017,
		FMUL 	= 0x019,
		FRSQRTE	= 0x01a,
		FMSUB	= 0x01c,
		FMADD 	= 0x01d,
		FNMSUB 	= 0x01e,
		FNMADD 	= 0x01f,
		FCMPO 	= 0x020,
		FNEG 	= 0x028,
		FMR 	= 0x048,
		FNABS 	= 0x088,
		FABS 	= 0x108,
		FCTID 	= 0x32e,
		FCTIDZ 	= 0x32f,
		FCFID 	= 0x34e,
	};
}

class PPU_Opcodes
{
public:
	virtual void Exit()=0;

	static u64 branchTarget(const u64 pc, const u64 imm)
	{
		return pc + (imm & ~0x3);
    }

	ADD_NULL_OPCODE(NULL_OP);
	ADD_NULL_OPCODE(NOP);

	ADD_OPCODE(TDI,(OP_uIMM to, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(TWI,(OP_uIMM to, OP_REG ra, OP_sIMM simm16));

	START_OPCODES_GROUP(G_04)
		ADD_OPCODE(MFVSCR,(OP_REG vd));
		ADD_OPCODE(MTVSCR,(OP_REG vb));
		ADD_OPCODE(VADDCUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDSBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDSHS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDSWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUBM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUHM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUHS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUWM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VADDUWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAND,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VANDC,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGSB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGSH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGSW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VAVGUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCFSX,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VCFUX,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VCMPBFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPBFP_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQFP_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUB_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUH_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPEQUW_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGEFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGEFP_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTFP_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSB_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSH_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTSW_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUB_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUH_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCMPGTUW_,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VCTSXS,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VCTUXS,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VEXPTEFP,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VLOGEFP,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VMADDFP,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMAXFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXSB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXSH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXSW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMAXUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMHADDSHS,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMHRADDSHS,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMINFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINSB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINSH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINSW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMINUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMLADDUHM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMRGHB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMRGHH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMRGHW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMRGLB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMRGLH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMRGLW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMSUMMBM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMSUMSHM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMSUMSHS,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMSUMUBM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMSUMUHM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMSUMUHS,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VMULESB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULESH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULEUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULEUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULOSB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULOSH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULOUB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VMULOUH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VNMSUBFP,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VNOR,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VOR,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPERM,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VPKPX,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKSHSS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKSHUS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKSWSS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKSWUS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKUHUM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKUHUS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKUWUM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VPKUWUS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VREFP,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VRFIM,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VRFIN,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VRFIP,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VRFIZ,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VRLB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VRLH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VRLW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VRSQRTEFP,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VSEL,(OP_REG vd, OP_REG va, OP_REG vb, OP_REG vc));
		ADD_OPCODE(VSL,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSLB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSLDOI,(OP_REG vd, OP_REG va, OP_REG vb, OP_uIMM sh));
		ADD_OPCODE(VSLH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSLO,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSLW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSPLTB,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VSPLTH,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VSPLTISB,(OP_REG vd, OP_sIMM simm5));
		ADD_OPCODE(VSPLTISH,(OP_REG vd, OP_sIMM simm5));
		ADD_OPCODE(VSPLTISW,(OP_REG vd, OP_sIMM simm5));
		ADD_OPCODE(VSPLTW,(OP_REG vd, OP_uIMM uimm5, OP_REG vb));
		ADD_OPCODE(VSR,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRAB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRAH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRAW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRB,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRH,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRO,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSRW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBCUW,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBFP,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBSBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBSHS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBSWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUBM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUHM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUHS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUWM,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUBUWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUMSWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUM2SWS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUM4SBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUM4SHS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VSUM4UBS,(OP_REG vd, OP_REG va, OP_REG vb));
		ADD_OPCODE(VUPKHPX,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VUPKHSB,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VUPKHSH,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VUPKLPX,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VUPKLSB,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VUPKLSH,(OP_REG vd, OP_REG vb));
		ADD_OPCODE(VXOR,(OP_REG vd, OP_REG va, OP_REG vb));
	END_OPCODES_GROUP(G_04);

	ADD_OPCODE(MULLI,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(SUBFIC,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(CMPLI,(OP_REG bf, OP_REG l, OP_REG ra, OP_uIMM uimm16));
	ADD_OPCODE(CMPI,(OP_REG bf, OP_REG l, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(ADDIC,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(ADDIC_,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(ADDI,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(ADDIS,(OP_REG rd, OP_REG ra, OP_sIMM simm16));
	ADD_OPCODE(BC,(OP_REG bo, OP_REG bi, OP_sIMM bd, OP_REG aa, OP_REG lk));
	ADD_OPCODE(SC,(OP_sIMM sc_code));
	ADD_OPCODE(B,(OP_sIMM ll, OP_REG aa, OP_REG lk));
	
	START_OPCODES_GROUP(G_13)
		ADD_OPCODE(MCRF,(OP_REG crfd, OP_REG crfs));
		ADD_OPCODE(BCLR,(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk));
		ADD_OPCODE(CRNOR,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CRANDC,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(ISYNC,());
		ADD_OPCODE(CRXOR,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CRNAND,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CRAND,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CREQV,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CRORC,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(CROR,(OP_REG bt, OP_REG ba, OP_REG bb));
		ADD_OPCODE(BCCTR,(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk));
	END_OPCODES_GROUP(G_13);
	
	ADD_OPCODE(RLWIMI,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc));
	ADD_OPCODE(RLWINM,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc));
	ADD_OPCODE(RLWNM,(OP_REG ra, OP_REG rs, OP_REG rb, OP_REG MB, OP_REG ME, bool rc));
	ADD_OPCODE(ORI,(OP_REG rs, OP_REG ra, OP_uIMM uimm16));
	ADD_OPCODE(ORIS,(OP_REG rs, OP_REG ra, OP_uIMM uimm16));
	ADD_OPCODE(XORI,(OP_REG ra, OP_REG rs, OP_uIMM uimm16));
	ADD_OPCODE(XORIS,(OP_REG ra, OP_REG rs, OP_uIMM uimm16));
	ADD_OPCODE(ANDI_,(OP_REG ra, OP_REG rs, OP_uIMM uimm16));
	ADD_OPCODE(ANDIS_,(OP_REG ra, OP_REG rs, OP_uIMM uimm16));

	START_OPCODES_GROUP(G_1e)
		ADD_OPCODE(RLDICL,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc));
		ADD_OPCODE(RLDICR,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG me, bool rc));
		ADD_OPCODE(RLDIC,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc));
		ADD_OPCODE(RLDIMI,(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc));
	END_OPCODES_GROUP(G_1e);

	START_OPCODES_GROUP(G_1f)
		/*0x000*/ADD_OPCODE(CMP,(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb));
		/*0x004*/ADD_OPCODE(TW,(OP_uIMM to, OP_REG ra, OP_REG rb));
		/*0x006*/ADD_OPCODE(LVSL,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x007*/ADD_OPCODE(LVEBX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x008*/ADD_OPCODE(SUBFC,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x009*/ADD_OPCODE(MULHDU,(OP_REG rd, OP_REG ra, OP_REG rb, bool rc));
		/*0x00a*/ADD_OPCODE(ADDC,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x00b*/ADD_OPCODE(MULHWU,(OP_REG rd, OP_REG ra, OP_REG rb, bool rc));
		/*0x013*/ADD_OPCODE(MFOCRF,(OP_uIMM a, OP_REG rd, OP_uIMM crm));
		/*0x014*/ADD_OPCODE(LWARX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x015*/ADD_OPCODE(LDX,(OP_REG ra, OP_REG rs, OP_REG rb));
		/*0x017*/ADD_OPCODE(LWZX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x018*/ADD_OPCODE(SLW,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x01a*/ADD_OPCODE(CNTLZW,(OP_REG ra, OP_REG rs, bool rc));
		/*0x01b*/ADD_OPCODE(SLD,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x01c*/ADD_OPCODE(AND,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x020*/ADD_OPCODE(CMPL,(OP_REG bf, OP_REG l, OP_REG ra, OP_REG rb));
		/*0x026*/ADD_OPCODE(LVSR,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x027*/ADD_OPCODE(LVEHX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x028*/ADD_OPCODE(SUBF,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x035*/ADD_OPCODE(LDUX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x036*/ADD_OPCODE(DCBST,(OP_REG ra, OP_REG rb));
		/*0x03a*/ADD_OPCODE(CNTLZD,(OP_REG ra, OP_REG rs, bool rc));
		/*0x03c*/ADD_OPCODE(ANDC,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x047*/ADD_OPCODE(LVEWX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x049*/ADD_OPCODE(MULHD,(OP_REG rd, OP_REG ra, OP_REG rb, bool rc));
		/*0x04b*/ADD_OPCODE(MULHW,(OP_REG rd, OP_REG ra, OP_REG rb, bool rc));
		/*0x054*/ADD_OPCODE(LDARX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x056*/ADD_OPCODE(DCBF,(OP_REG ra, OP_REG rb));
		/*0x057*/ADD_OPCODE(LBZX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x067*/ADD_OPCODE(LVX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x068*/ADD_OPCODE(NEG,(OP_REG rd, OP_REG ra, OP_REG oe, bool rc));
		/*0x077*/ADD_OPCODE(LBZUX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x07c*/ADD_OPCODE(NOR,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x087*/ADD_OPCODE(STVEBX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x088*/ADD_OPCODE(SUBFE,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x08a*/ADD_OPCODE(ADDE,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x090*/ADD_OPCODE(MTOCRF,(OP_REG crm, OP_REG rs));
		/*0x095*/ADD_OPCODE(STDX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x096*/ADD_OPCODE(STWCX_,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x097*/ADD_OPCODE(STWX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x0a7*/ADD_OPCODE(STVEHX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x0b5*/ADD_OPCODE(STDUX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x0c7*/ADD_OPCODE(STVEWX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x0ca*/ADD_OPCODE(ADDZE,(OP_REG rd, OP_REG ra, OP_REG oe, bool rc));
		/*0x0d6*/ADD_OPCODE(STDCX_,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x0d7*/ADD_OPCODE(STBX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x0e7*/ADD_OPCODE(STVX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x0e9*/ADD_OPCODE(MULLD,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x0ea*/ADD_OPCODE(ADDME,(OP_REG rd, OP_REG ra, OP_REG oe, bool rc));
		/*0x0eb*/ADD_OPCODE(MULLW,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x0f6*/ADD_OPCODE(DCBTST,(OP_REG th, OP_REG ra, OP_REG rb));
		/*0x10a*/ADD_OPCODE(ADD,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x116*/ADD_OPCODE(DCBT,(OP_REG ra, OP_REG rb, OP_REG th));
		/*0x117*/ADD_OPCODE(LHZX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x11c*/ADD_OPCODE(EQV,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x136*/ADD_OPCODE(ECIWX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x137*/ADD_OPCODE(LHZUX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x13c*/ADD_OPCODE(XOR,(OP_REG rs, OP_REG ra, OP_REG rb, bool rc));
		/*0x153*/ADD_OPCODE(MFSPR,(OP_REG rd, OP_REG spr));
		/*0x156*/ADD_OPCODE(DST,(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t));
		/*0x157*/ADD_OPCODE(LHAX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x167*/ADD_OPCODE(LVXL,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x168*/ADD_OPCODE(ABS,(OP_REG rd, OP_REG ra, OP_REG oe, bool rc));
		/*0x173*/ADD_OPCODE(MFTB,(OP_REG rd, OP_REG spr));
		/*0x176*/ADD_OPCODE(DSTST,(OP_REG ra, OP_REG rb, OP_uIMM strm, OP_uIMM t));
		/*0x177*/ADD_OPCODE(LHAUX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x197*/ADD_OPCODE(STHX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x19c*/ADD_OPCODE(ORC,(OP_REG rs, OP_REG ra, OP_REG rb, bool rc));
		/*0x1b6*/ADD_OPCODE(ECOWX,(OP_REG rs, OP_REG ra, OP_REG rb));
		/*0x1bc*/ADD_OPCODE(OR,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x1c9*/ADD_OPCODE(DIVDU,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x1cb*/ADD_OPCODE(DIVWU,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x1d3*/ADD_OPCODE(MTSPR,(OP_REG spr, OP_REG rs));
		/*0x1d6*///DCBI
		/*0x1dc*/ADD_OPCODE(NAND,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x1e7*/ADD_OPCODE(STVXL,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x1e9*/ADD_OPCODE(DIVD,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x1eb*/ADD_OPCODE(DIVW,(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc));
		/*0x207*/ADD_OPCODE(LVLX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x216*/ADD_OPCODE(LWBRX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x217*/ADD_OPCODE(LFSX,(OP_REG frd, OP_REG ra, OP_REG rb));
		/*0x218*/ADD_OPCODE(SRW,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x21b*/ADD_OPCODE(SRD,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x227*/ADD_OPCODE(LVRX,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x237*/ADD_OPCODE(LFSUX,(OP_REG frd, OP_REG ra, OP_REG rb));
		/*0x256*/ADD_OPCODE(SYNC,(OP_uIMM l));
		/*0x257*/ADD_OPCODE(LFDX,(OP_REG frd, OP_REG ra, OP_REG rb));
		/*0x277*/ADD_OPCODE(LFDUX,(OP_REG frd, OP_REG ra, OP_REG rb));
		/*0x287*/ADD_OPCODE(STVLX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x297*/ADD_OPCODE(STFSX,(OP_REG frs, OP_REG ra, OP_REG rb));
		/*0x2a7*/ADD_OPCODE(STVRX,(OP_REG vs, OP_REG ra, OP_REG rb));
		/*0x2d7*/ADD_OPCODE(STFDX,(OP_REG frs, OP_REG ra, OP_REG rb));
		/*0x307*/ADD_OPCODE(LVLXL,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x316*/ADD_OPCODE(LHBRX,(OP_REG rd, OP_REG ra, OP_REG rb));
		/*0x318*/ADD_OPCODE(SRAW,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x31a*/ADD_OPCODE(SRAD,(OP_REG ra, OP_REG rs, OP_REG rb, bool rc));
		/*0x327*/ADD_OPCODE(LVRXL,(OP_REG vd, OP_REG ra, OP_REG rb));
		/*0x336*/ADD_OPCODE(DSS,(OP_uIMM strm, OP_uIMM a));
		/*0x338*/ADD_OPCODE(SRAWI,(OP_REG ra, OP_REG rs, OP_REG sh, bool rc));
		/*0x33a*/ADD_OPCODE(SRADI1,(OP_REG ra, OP_REG rs, OP_REG sh, bool rc));
		/*0x33b*/ADD_OPCODE(SRADI2,(OP_REG ra, OP_REG rs, OP_REG sh, bool rc));
		/*0x356*/ADD_OPCODE(EIEIO,());
		/*0x387*/ADD_OPCODE(STVLXL,(OP_REG sd, OP_REG ra, OP_REG rb));
		/*0x39a*/ADD_OPCODE(EXTSH,(OP_REG ra, OP_REG rs, bool rc));
		/*0x3a7*/ADD_OPCODE(STVRXL,(OP_REG sd, OP_REG ra, OP_REG rb));
		/*0x3ba*/ADD_OPCODE(EXTSB,(OP_REG ra, OP_REG rs, bool rc));
		/*0x3d7*/ADD_OPCODE(STFIWX,(OP_REG frs, OP_REG ra, OP_REG rb));
		/*0x3da*/ADD_OPCODE(EXTSW,(OP_REG ra, OP_REG rs, bool rc));
		/*0x3d6*///ICBI
		/*0x3f6*/ADD_OPCODE(DCBZ,(OP_REG ra, OP_REG rb));
	END_OPCODES_GROUP(G_1f);
	
	ADD_OPCODE(LWZ,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LWZU,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LBZ,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LBZU,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STW,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STWU,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STB,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STBU,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LHZ,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LHZU,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STH,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STHU,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LMW,(OP_REG rd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STMW,(OP_REG rs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LFS,(OP_REG frd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LFSU,(OP_REG frd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LFD,(OP_REG frd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(LFDU,(OP_REG frd, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STFS,(OP_REG frs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STFSU,(OP_REG frs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STFD,(OP_REG frs, OP_REG ra, OP_sIMM d));
	ADD_OPCODE(STFDU,(OP_REG frs, OP_REG ra, OP_sIMM d));

	START_OPCODES_GROUP(G_3a)
		ADD_OPCODE(LD,(OP_REG rd, OP_REG ra, OP_sIMM ds));
		ADD_OPCODE(LDU,(OP_REG rd, OP_REG ra, OP_sIMM ds));
	END_OPCODES_GROUP(G_3a);

	START_OPCODES_GROUP(G_3b)
		ADD_OPCODE(FDIVS,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FSUBS,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FADDS,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FSQRTS,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FRES,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FMULS,(OP_REG frd, OP_REG fra, OP_REG frc, bool rc));
		ADD_OPCODE(FMADDS,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FMSUBS,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FNMSUBS,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FNMADDS,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
	END_OPCODES_GROUP(G_3b);
	
	START_OPCODES_GROUP(G_3e)
		ADD_OPCODE(STD,(OP_REG rs, OP_REG ra, OP_sIMM ds));
		ADD_OPCODE(STDU,(OP_REG rs, OP_REG ra, OP_sIMM ds));
	END_OPCODES_GROUP(G_3e);

	START_OPCODES_GROUP(G_3f)
		ADD_OPCODE(MTFSB1,(OP_REG bt, bool rc));
		ADD_OPCODE(MCRFS,(OP_REG bf, OP_REG bfa));
		ADD_OPCODE(MTFSB0,(OP_REG bt, bool rc));
		ADD_OPCODE(MTFSFI,(OP_REG crfd, OP_REG i, bool rc));
		ADD_OPCODE(MFFS,(OP_REG frd, bool rc));
		ADD_OPCODE(MTFSF,(OP_REG flm, OP_REG frb, bool rc));

		ADD_OPCODE(FCMPU,(OP_REG bf, OP_REG fra, OP_REG frb));
		ADD_OPCODE(FRSP,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FCTIW,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FCTIWZ,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FDIV,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FSUB,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FADD,(OP_REG frd, OP_REG fra, OP_REG frb, bool rc));
		ADD_OPCODE(FSQRT,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FSEL,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FMUL,(OP_REG frd, OP_REG fra, OP_REG frc, bool rc));
		ADD_OPCODE(FRSQRTE,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FMSUB,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FMADD,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FNMSUB,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FNMADD,(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc));
		ADD_OPCODE(FCMPO,(OP_REG crfd, OP_REG fra, OP_REG frb));
		ADD_OPCODE(FNEG,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FMR,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FNABS,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FABS,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FCTID,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FCTIDZ,(OP_REG frd, OP_REG frb, bool rc));
		ADD_OPCODE(FCFID,(OP_REG frd, OP_REG frb, bool rc));
	END_OPCODES_GROUP(G_3f);

	ADD_OPCODE(UNK,(const u32 code, const u32 opcode, const u32 gcode));
};

//instr_caller* g_instrs[0x40];

#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP