#pragma once

#define OP_REG u32
#define OP_sIMM s32
#define OP_uIMM u32
#define START_OPCODES_GROUP(x) /*x*/
#define ADD_OPCODE(name, regs) virtual void(##name##)##regs##=0
#define ADD_NULL_OPCODE(name) virtual void(##name##)()=0
#define END_OPCODES_GROUP(x) /*x*/

namespace SPU_opcodes
{
	enum SPU_0_10_Opcodes
	{
		STOP = 0x0,
		LNOP = 0x1,
		SYNC = 0x2,
		DSYNC = 0x3,
		MFSPR = 0xc,
		RDCH = 0xd,
		RCHCNT = 0xf,
		SF = 0x40,
		OR = 0x41,
		BG = 0x42,
		SFH = 0x48,
		NOR = 0x49,
		ABSDB = 0x53,
		ROT = 0x58,
		ROTM = 0x59,
		ROTMA = 0x5a,
		SHL = 0x5b,
		ROTH = 0x5c,
		ROTHM = 0x5d,
		ROTMAH = 0x5e,
		SHLH = 0x5f,
		ROTI = 0x78,
		ROTMI = 0x79,
		ROTMAI = 0x7a,
		SHLI = 0x7b,
		ROTHI = 0x7c,
		ROTHMI = 0x7d,
		ROTMAHI = 0x7e,
		SHLHI = 0x7f,
		A = 0xc0,
		AND = 0xc1,
		CG = 0xc2,
		AH = 0xc8,
		NAND = 0xc9,
		AVGB = 0xd3,
		MTSPR = 0x10c,
		WRCH = 0x10d,
		BIZ = 0x128,
		BINZ = 0x129,
		BIHZ = 0x12a,
		BIHNZ = 0x12b,
		STOPD = 0x140,
		STQX = 0x144,
		BI = 0x1a8,
		BISL = 0x1a9,
		IRET = 0x1aa,
		BISLED = 0x1ab,
		HBR = 0x1ac,
		GB = 0x1b0,
		GBH = 0x1b1,
		GBB = 0x1b2,
		FSM = 0x1b4,
		FSMH = 0x1b5,
		FSMB = 0x1b6,
		FREST = 0x1b8,
		FRSQEST = 0x1b9,
		LQX = 0x1c4,
		ROTQBYBI = 0x1cc,
		ROTQMBYBI = 0x1cd,
		SHLQBYBI = 0x1cf,
		CBX = 0x1d4,
		CHX = 0x1d5,
		CWX = 0x1d6,
		CDX = 0x1d7,
		ROTQBI = 0x1d8,
		ROTQMBI = 0x1d9,
		SHLQBI = 0x1db,
		ROTQBY = 0x1dc,
		ROTQMBY = 0x1dd,
		SHLQBY = 0x1df,
		ORX = 0x1f0,
		CBD = 0x1f4,
		CHD = 0x1f5,
		CWD = 0x1f6,
		CDD = 0x1f7,
		ROTQBII = 0x1f8,
		ROTQMBII = 0x1f9,
		SHLQBII = 0x1fb,
		ROTQBYI = 0x1fc,
		ROTQMBYI = 0x1fd,
		SHLQBYI = 0x1ff,
		NOP = 0x201,
		CGT = 0x240,
		XOR = 0x241,
		CGTH = 0x248,
		EQV = 0x249,
		CGTB = 0x250,
		SUMB = 0x253,
		HGT = 0x258,
		CLZ = 0x2a5,
		XSWD = 0x2a6,
		XSHW = 0x2ae,
		CNTB = 0x2b4,
		XSBH = 0x2b6,
		CLGT = 0x2c0,
		ANDC = 0x2c1,
		FCGT = 0x2c2,
		DFCGT = 0x2c3,
		FA = 0x2c4,
		FS = 0x2c5,
		FM = 0x2c6,
		CLGTH = 0x2c8,
		ORC = 0x2c9,
		FCMGT = 0x2ca,
		DFCMGT = 0x2cb,
		DFA = 0x2cc,
		DFS = 0x2cd,
		DFM = 0x2ce,
		CLGTB = 0x2d0,
		HLGT = 0x2d8,
		DFMA = 0x35c,
		DFMS = 0x35d,
		DFNMS = 0x35e,
		DFNMA = 0x35f,
		CEQ = 0x3c0,
		MPYHHU = 0x3ce,
		ADDX = 0x340,
		SFX = 0x341,
		CGX = 0x342,
		BGX = 0x343,
		MPYHHA = 0x346,
		MPYHHAU = 0x34e,
		FSCRRD = 0x398,
		FESD = 0x3b8,
		FRDS = 0x3b9,
		FSCRWR = 0x3ba,
		DFTSV = 0x3bf,
		FCEQ = 0x3c2,
		DFCEQ = 0x3c3,
		MPY = 0x3c4,
		MPYH = 0x3c5,
		MPYHH = 0x3c6,
		MPYS = 0x3c7,
		CEQH = 0x3c8,
		FCMEQ = 0x3ca,
		DFCMEQ = 0x3cb,
		MPYU = 0x3cc,
		CEQB = 0x3d0,
		FI = 0x3d4,
		HEQ = 0x3d8,
	};

	enum SPU_0_9_Opcodes
	{
		CFLTS = 0x1d8,
		CFLTU = 0x1d9,
		CSFLT = 0x1da,
		CUFLT = 0x1db,
	};

	enum SPU_0_8_Opcodes
	{
		BRZ = 0x40,
		STQA = 0x41,
		BRNZ = 0x42,
		BRHZ = 0x44,
		BRHNZ = 0x46,
		STQR = 0x47,
		BRA = 0x60,
		LQA = 0x61,
		BRASL = 0x62,
		BR = 0x64,
		FSMBI = 0x65,
		BRSL = 0x66,
		LQR = 0x67,
		IL = 0x81,
		ILHU = 0x82,
		ILH = 0x83,
		IOHL = 0xc1,
	};

	enum SPU_0_7_Opcodes
	{
		ORI = 0x4,
		ORHI = 0x5,
		ORBI = 0x6,
		SFI = 0xc,
		SFHI = 0xd,
		ANDI = 0x14,
		ANDHI = 0x15,
		ANDBI = 0x16,
		AI = 0x1c,
		AHI = 0x1d,
		STQD = 0x24,
		LQD = 0x34,
		XORI = 0x44,
		XORHI = 0x45,
		XORBI = 0x46,
		CGTI = 0x4c,
		CGTHI = 0x4d,
		CGTBI = 0x4e,
		HGTI = 0x4f,
		CLGTI = 0x5c,
		CLGTHI = 0x5d,
		CLGTBI = 0x5e,
		HLGTI = 0x5f,
		MPYI = 0x74,
		MPYUI = 0x75,
		CEQI = 0x7c,
		CEQHI = 0x7d,
		CEQBI = 0x7e,
		HEQI = 0x7f,
	};

	enum SPU_0_6_Opcodes
	{
		HBRA = 0x8,
		HBRR = 0x9,
		ILA = 0x21,
	};

	enum SPU_0_3_Opcodes
	{
		SELB = 0x8,
		SHUFB = 0xb,
		MPYA = 0xc,
		FNMS = 0xd,
		FMA = 0xe,
		FMS = 0xf,
	};
};

class SPU_Opcodes
{
public:
	static u32 branchTarget(const u64 pc, const s32 imm)
	{
		return (pc + ((imm << 2) & ~0x3)) & 0x3fff0;
    }
	
	virtual void Exit()=0;

	//0 - 10
	ADD_OPCODE(STOP,(OP_uIMM code));
	ADD_OPCODE(LNOP,());
	ADD_OPCODE(SYNC, (OP_uIMM Cbit));
	ADD_OPCODE(DSYNC, ());
	ADD_OPCODE(MFSPR,(OP_REG rt, OP_REG sa));
	ADD_OPCODE(RDCH,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(RCHCNT,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(SF,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(OR,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(BG,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SFH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(NOR,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ABSDB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTM,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTMA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHL,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTHM,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTMAH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHLH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTMI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTMAI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SHLI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTHI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTHMI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTMAHI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SHLHI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(A,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(AND,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CG,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(AH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(NAND,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(AVGB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MTSPR,(OP_REG rt, OP_REG sa));
	ADD_OPCODE(WRCH,(OP_REG ra, OP_REG rt));
	ADD_OPCODE(BIZ,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(BINZ,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(BIHZ,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(BIHNZ,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(STOPD,(OP_REG rc, OP_REG ra, OP_REG rb));
	ADD_OPCODE(STQX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(BI,(OP_REG ra));
	ADD_OPCODE(BISL,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(IRET,(OP_REG ra));
	ADD_OPCODE(BISLED,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(HBR,(OP_REG p, OP_REG ro, OP_REG ra));
	ADD_OPCODE(GB,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(GBH,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(GBB,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FSM,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FSMH,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FSMB,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FREST,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FRSQEST,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(LQX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQBYBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQMBYBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHLQBYBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CBX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CHX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CWX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CDX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQMBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHLQBI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQBY,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ROTQMBY,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SHLQBY,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ORX,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(CBD,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(CHD,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(CWD,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(CDD,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTQBII,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTQMBII,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SHLQBII,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTQBYI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(ROTQMBYI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(SHLQBYI,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(NOP,(OP_REG rt));
	ADD_OPCODE(CGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(XOR,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CGTH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(EQV,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CGTB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SUMB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(HGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CLZ,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(XSWD,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(XSHW,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(CNTB,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(XSBH,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(CLGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ANDC,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FCGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFCGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FS,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FM,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CLGTH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ORC,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FCMGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFCMGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFS,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFM,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CLGTB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(HLGT,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFMA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFMS,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFNMS,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFNMA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CEQ,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYHHU,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(ADDX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(SFX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CGX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(BGX,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYHHA,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYHHAU,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FSCRRD,(OP_REG rt));
	ADD_OPCODE(FESD,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FRDS,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(FSCRWR,(OP_REG rt, OP_REG ra));
	ADD_OPCODE(DFTSV,(OP_REG rt, OP_REG ra, OP_sIMM i7));
	ADD_OPCODE(FCEQ,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFCEQ,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPY,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYHH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYS,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CEQH,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FCMEQ,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(DFCMEQ,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(MPYU,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(CEQB,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(FI,(OP_REG rt, OP_REG ra, OP_REG rb));
	ADD_OPCODE(HEQ,(OP_REG rt, OP_REG ra, OP_REG rb));

	//0 - 9
	ADD_OPCODE(CFLTS,(OP_REG rt, OP_REG ra, OP_sIMM i8));
	ADD_OPCODE(CFLTU,(OP_REG rt, OP_REG ra, OP_sIMM i8));
	ADD_OPCODE(CSFLT,(OP_REG rt, OP_REG ra, OP_sIMM i8));
	ADD_OPCODE(CUFLT,(OP_REG rt, OP_REG ra, OP_sIMM i8));

	//0 - 8
	ADD_OPCODE(BRZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(STQA,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRNZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRHZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRHNZ,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(STQR,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRA,(OP_sIMM i16));
	ADD_OPCODE(LQA,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRASL,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BR,(OP_sIMM i16));
	ADD_OPCODE(FSMBI,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(BRSL,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(LQR,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(IL,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(ILHU,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(ILH,(OP_REG rt, OP_sIMM i16));
	ADD_OPCODE(IOHL,(OP_REG rt, OP_sIMM i16));

	//0 - 7
	ADD_OPCODE(ORI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(ORHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(ORBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(SFI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(SFHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(ANDI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(ANDHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(ANDBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(AI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(AHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(STQD,(OP_REG rt, OP_sIMM i10, OP_REG ra));
	ADD_OPCODE(LQD,(OP_REG rt, OP_sIMM i10, OP_REG ra));
	ADD_OPCODE(XORI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(XORHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(XORBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CGTI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CGTHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CGTBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(HGTI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CLGTI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CLGTHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CLGTBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(HLGTI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(MPYI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(MPYUI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CEQI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CEQHI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(CEQBI,(OP_REG rt, OP_REG ra, OP_sIMM i10));
	ADD_OPCODE(HEQI,(OP_REG rt, OP_REG ra, OP_sIMM i10));

	//0 - 6
	ADD_OPCODE(HBRA,(OP_sIMM ro, OP_sIMM i16));
	ADD_OPCODE(HBRR,(OP_sIMM ro, OP_sIMM i16));
	ADD_OPCODE(ILA,(OP_REG rt, OP_sIMM i18));

	//0 - 3
	ADD_OPCODE(SELB,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(SHUFB,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(MPYA,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(FNMS,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(FMA,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));
	ADD_OPCODE(FMS,(OP_REG rc, OP_REG ra, OP_REG rb, OP_REG rt));

	ADD_OPCODE(UNK,(const s32 code, const s32 opcode, const s32 gcode));
};

#undef START_OPCODES_GROUP
#undef ADD_OPCODE
#undef ADD_NULL_OPCODE
#undef END_OPCODES_GROUP