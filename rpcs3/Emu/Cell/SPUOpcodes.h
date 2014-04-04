#pragma once

namespace SPU_opcodes
{
	enum SPU_0_10_Opcodes
	{
		STOP      = 0x0,
		LNOP      = 0x1,
		SYNC      = 0x2,
		DSYNC     = 0x3,
		MFSPR     = 0xc,
		RDCH      = 0xd,
		RCHCNT    = 0xf,
		SF        = 0x40,
		OR        = 0x41,
		BG        = 0x42,
		SFH       = 0x48,
		NOR       = 0x49,
		ABSDB     = 0x53,
		ROT       = 0x58,
		ROTM      = 0x59,
		ROTMA     = 0x5a,
		SHL       = 0x5b,
		ROTH      = 0x5c,
		ROTHM     = 0x5d,
		ROTMAH    = 0x5e,
		SHLH      = 0x5f,
		ROTI      = 0x78,
		ROTMI     = 0x79,
		ROTMAI    = 0x7a,
		SHLI      = 0x7b,
		ROTHI     = 0x7c,
		ROTHMI    = 0x7d,
		ROTMAHI   = 0x7e,
		SHLHI     = 0x7f,
		A         = 0xc0,
		AND       = 0xc1,
		CG        = 0xc2,
		AH        = 0xc8,
		NAND      = 0xc9,
		AVGB      = 0xd3,
		MTSPR     = 0x10c,
		WRCH      = 0x10d,
		BIZ       = 0x128,
		BINZ      = 0x129,
		BIHZ      = 0x12a,
		BIHNZ     = 0x12b,
		STOPD     = 0x140,
		STQX      = 0x144,
		BI        = 0x1a8,
		BISL      = 0x1a9,
		IRET      = 0x1aa,
		BISLED    = 0x1ab,
		HBR       = 0x1ac,
		GB        = 0x1b0,
		GBH       = 0x1b1,
		GBB       = 0x1b2,
		FSM       = 0x1b4,
		FSMH      = 0x1b5,
		FSMB      = 0x1b6,
		FREST     = 0x1b8,
		FRSQEST   = 0x1b9,
		LQX       = 0x1c4,
		ROTQBYBI  = 0x1cc,
		ROTQMBYBI = 0x1cd,
		SHLQBYBI  = 0x1cf,
		CBX       = 0x1d4,
		CHX       = 0x1d5,
		CWX       = 0x1d6,
		CDX       = 0x1d7,
		ROTQBI    = 0x1d8,
		ROTQMBI   = 0x1d9,
		SHLQBI    = 0x1db,
		ROTQBY    = 0x1dc,
		ROTQMBY   = 0x1dd,
		SHLQBY    = 0x1df,
		ORX       = 0x1f0,
		CBD       = 0x1f4,
		CHD       = 0x1f5,
		CWD       = 0x1f6,
		CDD       = 0x1f7,
		ROTQBII   = 0x1f8,
		ROTQMBII  = 0x1f9,
		SHLQBII   = 0x1fb,
		ROTQBYI   = 0x1fc,
		ROTQMBYI  = 0x1fd,
		SHLQBYI   = 0x1ff,
		NOP       = 0x201,
		CGT       = 0x240,
		XOR       = 0x241,
		CGTH      = 0x248,
		EQV       = 0x249,
		CGTB      = 0x250,
		SUMB      = 0x253,
		HGT       = 0x258,
		CLZ       = 0x2a5,
		XSWD      = 0x2a6,
		XSHW      = 0x2ae,
		CNTB      = 0x2b4,
		XSBH      = 0x2b6,
		CLGT      = 0x2c0,
		ANDC      = 0x2c1,
		FCGT      = 0x2c2,
		DFCGT     = 0x2c3,
		FA        = 0x2c4,
		FS        = 0x2c5,
		FM        = 0x2c6,
		CLGTH     = 0x2c8,
		ORC       = 0x2c9,
		FCMGT     = 0x2ca,
		DFCMGT    = 0x2cb,
		DFA       = 0x2cc,
		DFS       = 0x2cd,
		DFM       = 0x2ce,
		CLGTB     = 0x2d0,
		HLGT      = 0x2d8,
		DFMA      = 0x35c,
		DFMS      = 0x35d,
		DFNMS     = 0x35e,
		DFNMA     = 0x35f,
		CEQ       = 0x3c0,
		MPYHHU    = 0x3ce,
		ADDX      = 0x340,
		SFX       = 0x341,
		CGX       = 0x342,
		BGX       = 0x343,
		MPYHHA    = 0x346,
		MPYHHAU   = 0x34e,
		FSCRRD    = 0x398,
		FESD      = 0x3b8,
		FRDS      = 0x3b9,
		FSCRWR    = 0x3ba,
		DFTSV     = 0x3bf,
		FCEQ      = 0x3c2,
		DFCEQ     = 0x3c3,
		MPY       = 0x3c4,
		MPYH      = 0x3c5,
		MPYHH     = 0x3c6,
		MPYS      = 0x3c7,
		CEQH      = 0x3c8,
		FCMEQ     = 0x3ca,
		DFCMEQ    = 0x3cb,
		MPYU      = 0x3cc,
		CEQB      = 0x3d0,
		FI        = 0x3d4,
		HEQ       = 0x3d8,
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
		BRZ   = 0x40,
		STQA  = 0x41,
		BRNZ  = 0x42,
		BRHZ  = 0x44,
		BRHNZ = 0x46,
		STQR  = 0x47,
		BRA   = 0x60,
		LQA   = 0x61,
		BRASL = 0x62,
		BR    = 0x64,
		FSMBI = 0x65,
		BRSL  = 0x66,
		LQR   = 0x67,
		IL    = 0x81,
		ILHU  = 0x82,
		ILH   = 0x83,
		IOHL  = 0xc1,
	};

	enum SPU_0_7_Opcodes
	{
		ORI    = 0x4,
		ORHI   = 0x5,
		ORBI   = 0x6,
		SFI    = 0xc,
		SFHI   = 0xd,
		ANDI   = 0x14,
		ANDHI  = 0x15,
		ANDBI  = 0x16,
		AI     = 0x1c,
		AHI    = 0x1d,
		STQD   = 0x24,
		LQD    = 0x34,
		XORI   = 0x44,
		XORHI  = 0x45,
		XORBI  = 0x46,
		CGTI   = 0x4c,
		CGTHI  = 0x4d,
		CGTBI  = 0x4e,
		HGTI   = 0x4f,
		CLGTI  = 0x5c,
		CLGTHI = 0x5d,
		CLGTBI = 0x5e,
		HLGTI  = 0x5f,
		MPYI   = 0x74,
		MPYUI  = 0x75,
		CEQI   = 0x7c,
		CEQHI  = 0x7d,
		CEQBI  = 0x7e,
		HEQI   = 0x7f,
	};

	enum SPU_0_6_Opcodes
	{
		HBRA = 0x8,
		HBRR = 0x9,
		ILA  = 0x21,
	};

	enum SPU_0_3_Opcodes
	{
		SELB  = 0x8,
		SHUFB = 0xb,
		MPYA  = 0xc,
		FNMS  = 0xd,
		FMA   = 0xe,
		FMS   = 0xf,
	};
};

class SPUOpcodes
{
public:
	static u32 branchTarget(const u64 pc, const s32 imm)
	{
		return (pc + (imm << 2)) & 0x3fffc;
	}

	//0 - 10
	virtual void STOP(u32 code) = 0;
	virtual void LNOP() = 0;
	virtual void SYNC(u32 Cbit) = 0;
	virtual void DSYNC() = 0;
	virtual void MFSPR(u32  rt, u32  sa) = 0;
	virtual void RDCH(u32  rt, u32  ra) = 0;
	virtual void RCHCNT(u32  rt, u32  ra) = 0;
	virtual void SF(u32  rt, u32  ra, u32  rb) = 0;
	virtual void OR(u32  rt, u32  ra, u32  rb) = 0;
	virtual void BG(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SFH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void NOR(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ABSDB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTM(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTMA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SHL(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTHM(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTMAH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SHLH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTMI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTMAI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void SHLI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTHI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTHMI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTMAHI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void SHLHI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void A(u32  rt, u32  ra, u32  rb) = 0;
	virtual void AND(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CG(u32  rt, u32  ra, u32  rb) = 0;
	virtual void AH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void NAND(u32  rt, u32  ra, u32  rb) = 0;
	virtual void AVGB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MTSPR(u32  rt, u32  sa) = 0;
	virtual void WRCH(u32  ra, u32  rt) = 0;
	virtual void BIZ(u32  rt, u32  ra) = 0;
	virtual void BINZ(u32  rt, u32  ra) = 0;
	virtual void BIHZ(u32  rt, u32  ra) = 0;
	virtual void BIHNZ(u32  rt, u32  ra) = 0;
	virtual void STOPD(u32  rc, u32  ra, u32  rb) = 0;
	virtual void STQX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void BI(u32  ra) = 0;
	virtual void BISL(u32  rt, u32  ra) = 0;
	virtual void IRET(u32  ra) = 0;
	virtual void BISLED(u32  rt, u32  ra) = 0;
	virtual void HBR(u32  p, u32  ro, u32  ra) = 0;
	virtual void GB(u32  rt, u32  ra) = 0;
	virtual void GBH(u32  rt, u32  ra) = 0;
	virtual void GBB(u32  rt, u32  ra) = 0;
	virtual void FSM(u32  rt, u32  ra) = 0;
	virtual void FSMH(u32  rt, u32  ra) = 0;
	virtual void FSMB(u32  rt, u32  ra) = 0;
	virtual void FREST(u32  rt, u32  ra) = 0;
	virtual void FRSQEST(u32  rt, u32  ra) = 0;
	virtual void LQX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQBYBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQMBYBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SHLQBYBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CBX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CHX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CWX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CDX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQMBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SHLQBI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQBY(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ROTQMBY(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SHLQBY(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ORX(u32  rt, u32  ra) = 0;
	virtual void CBD(u32  rt, u32  ra, s32 i7) = 0;
	virtual void CHD(u32  rt, u32  ra, s32 i7) = 0;
	virtual void CWD(u32  rt, u32  ra, s32 i7) = 0;
	virtual void CDD(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTQBII(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTQMBII(u32  rt, u32  ra, s32 i7) = 0;
	virtual void SHLQBII(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTQBYI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void ROTQMBYI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void SHLQBYI(u32  rt, u32  ra, s32 i7) = 0;
	virtual void NOP(u32  rt) = 0;
	virtual void CGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void XOR(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CGTH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void EQV(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CGTB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SUMB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void HGT(u32  rt, s32  ra, s32  rb) = 0;
	virtual void CLZ(u32  rt, u32  ra) = 0;
	virtual void XSWD(u32  rt, u32  ra) = 0;
	virtual void XSHW(u32  rt, u32  ra) = 0;
	virtual void CNTB(u32  rt, u32  ra) = 0;
	virtual void XSBH(u32  rt, u32  ra) = 0;
	virtual void CLGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ANDC(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FCGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFCGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FS(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FM(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CLGTH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ORC(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FCMGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFCMGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFS(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFM(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CLGTB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void HLGT(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFMA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFMS(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFNMS(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFNMA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CEQ(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYHHU(u32  rt, u32  ra, u32  rb) = 0;
	virtual void ADDX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void SFX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CGX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void BGX(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYHHA(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYHHAU(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FSCRRD(u32  rt) = 0;
	virtual void FESD(u32  rt, u32  ra) = 0;
	virtual void FRDS(u32  rt, u32  ra) = 0;
	virtual void FSCRWR(u32  rt, u32  ra) = 0;
	virtual void DFTSV(u32  rt, u32  ra, s32 i7) = 0;
	virtual void FCEQ(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFCEQ(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPY(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYHH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYS(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CEQH(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FCMEQ(u32  rt, u32  ra, u32  rb) = 0;
	virtual void DFCMEQ(u32  rt, u32  ra, u32  rb) = 0;
	virtual void MPYU(u32  rt, u32  ra, u32  rb) = 0;
	virtual void CEQB(u32  rt, u32  ra, u32  rb) = 0;
	virtual void FI(u32  rt, u32  ra, u32  rb) = 0;
	virtual void HEQ(u32  rt, u32  ra, u32  rb) = 0;

	//0 - 9
	virtual void CFLTS(u32  rt, u32  ra, s32 i8) = 0;
	virtual void CFLTU(u32  rt, u32  ra, s32 i8) = 0;
	virtual void CSFLT(u32  rt, u32  ra, s32 i8) = 0;
	virtual void CUFLT(u32  rt, u32  ra, s32 i8) = 0;

	//0 - 8
	virtual void BRZ(u32  rt, s32 i16) = 0;
	virtual void STQA(u32  rt, s32 i16) = 0;
	virtual void BRNZ(u32  rt, s32 i16) = 0;
	virtual void BRHZ(u32  rt, s32 i16) = 0;
	virtual void BRHNZ(u32  rt, s32 i16) = 0;
	virtual void STQR(u32  rt, s32 i16) = 0;
	virtual void BRA(s32 i16) = 0;
	virtual void LQA(u32  rt, s32 i16) = 0;
	virtual void BRASL(u32  rt, s32 i16) = 0;
	virtual void BR(s32 i16) = 0;
	virtual void FSMBI(u32  rt, s32 i16) = 0;
	virtual void BRSL(u32  rt, s32 i16) = 0;
	virtual void LQR(u32  rt, s32 i16) = 0;
	virtual void IL(u32  rt, s32 i16) = 0;
	virtual void ILHU(u32  rt, s32 i16) = 0;
	virtual void ILH(u32  rt, s32 i16) = 0;
	virtual void IOHL(u32  rt, s32 i16) = 0;

	//0 - 7
	virtual void ORI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void ORHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void ORBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void SFI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void SFHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void ANDI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void ANDHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void ANDBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void AI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void AHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void STQD(u32  rt, s32 i10, u32  ra) = 0;
	virtual void LQD(u32  rt, s32 i10, u32  ra) = 0;
	virtual void XORI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void XORHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void XORBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CGTI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CGTHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CGTBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void HGTI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CLGTI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CLGTHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CLGTBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void HLGTI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void MPYI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void MPYUI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CEQI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CEQHI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void CEQBI(u32  rt, u32  ra, s32 i10) = 0;
	virtual void HEQI(u32  rt, u32  ra, s32 i10) = 0;

	//0 - 6
	virtual void HBRA(s32 ro, s32 i16) = 0;
	virtual void HBRR(s32 ro, s32 i16) = 0;
	virtual void ILA(u32  rt, u32 i18) = 0;

	//0 - 3
	virtual void SELB(u32  rc, u32  ra, u32  rb, u32  rt) = 0;
	virtual void SHUFB(u32  rc, u32  ra, u32  rb, u32  rt) = 0;
	virtual void MPYA(u32  rc, u32  ra, u32  rb, u32  rt) = 0;
	virtual void FNMS(u32  rc, u32  ra, u32  rb, u32  rt) = 0;
	virtual void FMA(u32  rc, u32  ra, u32  rb, u32  rt) = 0;
	virtual void FMS(u32  rc, u32  ra, u32  rb, u32  rt) = 0;

	virtual void UNK(u32 code, u32 opcode, u32 gcode) = 0;
};
