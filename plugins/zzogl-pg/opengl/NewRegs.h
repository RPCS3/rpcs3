/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef NEWREGS_H_INCLUDED
#define NEWREGS_H_INCLUDED

#ifdef USE_OLD_REGS
#include "Regs.h"
#else
enum GIF_REG
{
	GIF_REG_PRIM	= 0x00,
	GIF_REG_RGBA	= 0x01,
	GIF_REG_STQ		= 0x02,
	GIF_REG_UV		= 0x03,
	GIF_REG_XYZF2	= 0x04,
	GIF_REG_XYZ2	= 0x05,
	GIF_REG_TEX0_1	= 0x06,
	GIF_REG_TEX0_2	= 0x07,
	GIF_REG_CLAMP_1	= 0x08,
	GIF_REG_CLAMP_2	= 0x09,
	GIF_REG_FOG		= 0x0a,
	GIF_REG_XYZF3	= 0x0c,
	GIF_REG_XYZ3	= 0x0d,
	GIF_REG_A_D		= 0x0e,
	GIF_REG_NOP		= 0x0f,
};

enum GIF_A_D_REG
{
	GIF_A_D_REG_PRIM		= 0x00,
	GIF_A_D_REG_RGBAQ		= 0x01,
	GIF_A_D_REG_ST			= 0x02,
	GIF_A_D_REG_UV			= 0x03,
	GIF_A_D_REG_XYZF2		= 0x04,
	GIF_A_D_REG_XYZ2		= 0x05,
	GIF_A_D_REG_TEX0_1		= 0x06,
	GIF_A_D_REG_TEX0_2		= 0x07,
	GIF_A_D_REG_CLAMP_1		= 0x08,
	GIF_A_D_REG_CLAMP_2		= 0x09,
	GIF_A_D_REG_FOG			= 0x0a,
	GIF_A_D_REG_XYZF3		= 0x0c,
	GIF_A_D_REG_XYZ3		= 0x0d,
	GIF_A_D_REG_NOP			= 0x0f,
	GIF_A_D_REG_TEX1_1		= 0x14,
	GIF_A_D_REG_TEX1_2		= 0x15,
	GIF_A_D_REG_TEX2_1		= 0x16,
	GIF_A_D_REG_TEX2_2		= 0x17,
	GIF_A_D_REG_XYOFFSET_1	= 0x18,
	GIF_A_D_REG_XYOFFSET_2	= 0x19,
	GIF_A_D_REG_PRMODECONT	= 0x1a,
	GIF_A_D_REG_PRMODE		= 0x1b,
	GIF_A_D_REG_TEXCLUT		= 0x1c,
	GIF_A_D_REG_SCANMSK		= 0x22,
	GIF_A_D_REG_MIPTBP1_1	= 0x34,
	GIF_A_D_REG_MIPTBP1_2	= 0x35,
	GIF_A_D_REG_MIPTBP2_1	= 0x36,
	GIF_A_D_REG_MIPTBP2_2	= 0x37,
	GIF_A_D_REG_TEXA		= 0x3b,
	GIF_A_D_REG_FOGCOL		= 0x3d,
	GIF_A_D_REG_TEXFLUSH	= 0x3f,
	GIF_A_D_REG_SCISSOR_1	= 0x40,
	GIF_A_D_REG_SCISSOR_2	= 0x41,
	GIF_A_D_REG_ALPHA_1		= 0x42,
	GIF_A_D_REG_ALPHA_2		= 0x43,
	GIF_A_D_REG_DIMX		= 0x44,
	GIF_A_D_REG_DTHE		= 0x45,
	GIF_A_D_REG_COLCLAMP	= 0x46,
	GIF_A_D_REG_TEST_1		= 0x47,
	GIF_A_D_REG_TEST_2		= 0x48,
	GIF_A_D_REG_PABE		= 0x49,
	GIF_A_D_REG_FBA_1		= 0x4a,
	GIF_A_D_REG_FBA_2		= 0x4b,
	GIF_A_D_REG_FRAME_1		= 0x4c,
	GIF_A_D_REG_FRAME_2		= 0x4d,
	GIF_A_D_REG_ZBUF_1		= 0x4e,
	GIF_A_D_REG_ZBUF_2		= 0x4f,
	GIF_A_D_REG_BITBLTBUF	= 0x50,
	GIF_A_D_REG_TRXPOS		= 0x51,
	GIF_A_D_REG_TRXREG		= 0x52,
	GIF_A_D_REG_TRXDIR		= 0x53,
	GIF_A_D_REG_HWREG		= 0x54,
	GIF_A_D_REG_SIGNAL		= 0x60,
	GIF_A_D_REG_FINISH		= 0x61,
	GIF_A_D_REG_LABEL		= 0x62,
};
// In case we want to change to/from __fastcall for GIF register handlers:
#define __gifCall __fastcall

typedef void __gifCall FnType_GIFRegHandler(const u32* data);
typedef FnType_GIFRegHandler* GIFRegHandler;

extern FnType_GIFRegHandler GIFPackedRegHandlerNull;
extern FnType_GIFRegHandler GIFPackedRegHandlerRGBA;
extern FnType_GIFRegHandler GIFPackedRegHandlerSTQ;
extern FnType_GIFRegHandler GIFPackedRegHandlerUV;
extern FnType_GIFRegHandler GIFPackedRegHandlerXYZF2;
extern FnType_GIFRegHandler GIFPackedRegHandlerXYZ2;
extern FnType_GIFRegHandler GIFPackedRegHandlerFOG;
extern FnType_GIFRegHandler GIFPackedRegHandlerA_D;
extern FnType_GIFRegHandler GIFPackedRegHandlerNOP;

// These are unimplemented, and fall back on the non-packed versions.
extern FnType_GIFRegHandler GIFPackedRegHandlerPRIM;

template<u32 i>
extern FnType_GIFRegHandler GIFPackedRegHandlerTEX0;

template<u32 i>
extern FnType_GIFRegHandler GIFPackedRegHandlerCLAMP;

extern FnType_GIFRegHandler GIFPackedRegHandlerXYZF3;
extern FnType_GIFRegHandler GIFPackedRegHandlerXYZ3;

extern FnType_GIFRegHandler GIFRegHandlerNull;
extern FnType_GIFRegHandler GIFRegHandlerPRIM;
extern FnType_GIFRegHandler GIFRegHandlerRGBAQ;
extern FnType_GIFRegHandler GIFRegHandlerST;
extern FnType_GIFRegHandler GIFRegHandlerUV;
extern FnType_GIFRegHandler GIFRegHandlerXYZF2;
extern FnType_GIFRegHandler GIFRegHandlerXYZ2;

template<u32 i>
extern FnType_GIFRegHandler GIFRegHandlerTEX0;

template<u32 i>
extern FnType_GIFRegHandler GIFRegHandlerCLAMP;

extern FnType_GIFRegHandler GIFRegHandlerFOG;
extern FnType_GIFRegHandler GIFRegHandlerXYZF3;
extern FnType_GIFRegHandler GIFRegHandlerXYZ3;
extern FnType_GIFRegHandler GIFRegHandlerNOP;

template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerTEX1;

template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerTEX2;

template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerXYOFFSET;

extern FnType_GIFRegHandler GIFRegHandlerPRMODECONT;
extern FnType_GIFRegHandler GIFRegHandlerPRMODE;
extern FnType_GIFRegHandler GIFRegHandlerTEXCLUT;
extern FnType_GIFRegHandler GIFRegHandlerSCANMSK;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerMIPTBP1;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerMIPTBP2;
extern FnType_GIFRegHandler GIFRegHandlerTEXA;
extern FnType_GIFRegHandler GIFRegHandlerFOGCOL;
extern FnType_GIFRegHandler GIFRegHandlerTEXFLUSH;

template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerSCISSOR;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerALPHA;

extern FnType_GIFRegHandler GIFRegHandlerDIMX;
extern FnType_GIFRegHandler GIFRegHandlerDTHE;
extern FnType_GIFRegHandler GIFRegHandlerCOLCLAMP;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerTEST;
extern FnType_GIFRegHandler GIFRegHandlerPABE;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerFBA;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerFRAME;
template <u32 i>
extern FnType_GIFRegHandler GIFRegHandlerZBUF;
extern FnType_GIFRegHandler GIFRegHandlerBITBLTBUF;
extern FnType_GIFRegHandler GIFRegHandlerTRXPOS;
extern FnType_GIFRegHandler GIFRegHandlerTRXREG;
extern FnType_GIFRegHandler GIFRegHandlerTRXDIR;
extern FnType_GIFRegHandler GIFRegHandlerHWREG;
extern FnType_GIFRegHandler GIFRegHandlerSIGNAL;
extern FnType_GIFRegHandler GIFRegHandlerFINISH;
extern FnType_GIFRegHandler GIFRegHandlerLABEL;

// GifReg & GifPackedReg structs from GSdx, slightly modified

enum GS_ATST
{
	ATST_NEVER		= 0,
	ATST_ALWAYS		= 1,
	ATST_LESS		= 2,
	ATST_LEQUAL		= 3,
	ATST_EQUAL		= 4,
	ATST_GEQUAL		= 5,
	ATST_GREATER	= 6,
	ATST_NOTEQUAL	= 7,
};

enum GS_AFAIL
{
	AFAIL_KEEP		= 0,
	AFAIL_FB_ONLY	= 1,
	AFAIL_ZB_ONLY	= 2,
	AFAIL_RGB_ONLY	= 3,
};

// GIFReg

REG64_(GIFReg, ALPHA)
	u32 A:2;
	u32 B:2;
	u32 C:2;
	u32 D:2;
	u32 _PAD1:24;
	u32 FIX:8;
	u32 _PAD2:24;
REG_END2
	// opaque => output will be Cs/As
	__forceinline bool IsOpaque() const {return (A == B || C == 2 && FIX == 0) && D == 0 || (A == 0 && B == D && C == 2 && FIX == 0x80);}
	__forceinline bool IsOpaque(int amin, int amax) const {return (A == B || amax == 0) && D == 0 || A == 0 && B == D && amin == 0x80 && amax == 0x80;}
REG_END2

REG64_(GIFReg, BITBLTBUF)
	u32 SBP:14;
	u32 _PAD1:2;
	u32 SBW:6;
	u32 _PAD2:2;
	u32 SPSM:6;
	u32 _PAD3:2;
	u32 DBP:14;
	u32 _PAD4:2;
	u32 DBW:6;
	u32 _PAD5:2;
	u32 DPSM:6;
	u32 _PAD6:2;
REG_END

REG64_(GIFReg, CLAMP)
union
{
	struct
	{
		u32 WMS:2;
		u32 WMT:2;
		u32 MINU:10;
		u32 MAXU:10;
		u32 _PAD1:8;
		u32 _PAD2:2;
		u32 MAXV:10;
		u32 _PAD3:20;
	};

	struct
	{
		u64 _PAD4:24;
		u64 MINV:10;
		u64 _PAD5:30;
	};
};
REG_END

REG64_(GIFReg, COLCLAMP)
	u32 CLAMP:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, DIMX)
	s32 DM00:3;
	s32 _PAD00:1;
	s32 DM01:3;
	s32 _PAD01:1;
	s32 DM02:3;
	s32 _PAD02:1;
	s32 DM03:3;
	s32 _PAD03:1;
	s32 DM10:3;
	s32 _PAD10:1;
	s32 DM11:3;
	s32 _PAD11:1;
	s32 DM12:3;
	s32 _PAD12:1;
	s32 DM13:3;
	s32 _PAD13:1;
	s32 DM20:3;
	s32 _PAD20:1;
	s32 DM21:3;
	s32 _PAD21:1;
	s32 DM22:3;
	s32 _PAD22:1;
	s32 DM23:3;
	s32 _PAD23:1;
	s32 DM30:3;
	s32 _PAD30:1;
	s32 DM31:3;
	s32 _PAD31:1;
	s32 DM32:3;
	s32 _PAD32:1;
	s32 DM33:3;
	s32 _PAD33:1;
REG_END

REG64_(GIFReg, DTHE)
	u32 DTHE:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, FBA)
	u32 FBA:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, FINISH)
	u32 _PAD1:32;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, FOG)
	u32 _PAD1:32;
	u32 _PAD2:24;
	u32 F:8;
REG_END

REG64_(GIFReg, FOGCOL)
	u32 FCR:8;
	u32 FCG:8;
	u32 FCB:8;
	u32 _PAD1:8;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, FRAME)
	u32 FBP:9;
	u32 _PAD1:7;
	u32 FBW:6;
	u32 _PAD2:2;
	u32 PSM:6;
	u32 _PAD3:2;
	u32 FBMSK:32;
REG_END2
	u32 Block() const {return FBP << 5;}
REG_END2

REG64_(GIFReg, HWREG)
	u32 DATA_LOWER:32;
	u32 DATA_UPPER:32;
REG_END

REG64_(GIFReg, LABEL)
	u32 ID:32;
	u32 IDMSK:32;
REG_END

REG64_(GIFReg, MIPTBP1)
	u64 TBP1:14;
	u64 TBW1:6;
	u64 TBP2:14;
	u64 TBW2:6;
	u64 TBP3:14;
	u64 TBW3:6;
	u64 _PAD:4;
REG_END

REG64_(GIFReg, MIPTBP2)
	u64 TBP4:14;
	u64 TBW4:6;
	u64 TBP5:14;
	u64 TBW5:6;
	u64 TBP6:14;
	u64 TBW6:6;
	u64 _PAD:4;
REG_END

REG64_(GIFReg, NOP)
	u32 _PAD1:32;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, PABE)
	u32 PABE:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, PRIM)
	u32 PRIM:3;
	u32 IIP:1;
	u32 TME:1;
	u32 FGE:1;
	u32 ABE:1;
	u32 AA1:1;
	u32 FST:1;
	u32 CTXT:1;
	u32 FIX:1;
	u32 _PAD1:21;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, PRMODE)
	u32 _PRIM:3;
	u32 IIP:1;
	u32 TME:1;
	u32 FGE:1;
	u32 ABE:1;
	u32 AA1:1;
	u32 FST:1;
	u32 CTXT:1;
	u32 FIX:1;
	u32 _PAD2:21;
	u32 _PAD3:32;
REG_END

REG64_(GIFReg, PRMODECONT)
	u32 AC:1;
	u32 _PAD1:31;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, RGBAQ)
	u32 R:8;
	u32 G:8;
	u32 B:8;
	u32 A:8;
	float Q;
REG_END

REG64_(GIFReg, SCANMSK)
	u32 MSK:2;
	u32 _PAD1:30;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, SCISSOR)
	u32 SCAX0:11;
	u32 _PAD1:5;
	u32 SCAX1:11;
	u32 _PAD2:5;
	u32 SCAY0:11;
	u32 _PAD3:5;
	u32 SCAY1:11;
	u32 _PAD4:5;
REG_END

REG64_(GIFReg, SIGNAL)
	u32 ID:32;
	u32 IDMSK:32;
REG_END

REG64_(GIFReg, ST)
	float S;
	float T;
REG_END

REG64_(GIFReg, TEST)
	u32 ATE:1;
	u32 ATST:3;
	u32 AREF:8;
	u32 AFAIL:2;
	u32 DATE:1;
	u32 DATM:1;
	u32 ZTE:1;
	u32 ZTST:2;
	u32 _PAD1:13;
	u32 _PAD2:32;
REG_END2
	__forceinline bool DoFirstPass() {return !ATE || ATST != ATST_NEVER;} // not all pixels fail automatically
	__forceinline bool DoSecondPass() {return ATE && ATST != ATST_ALWAYS && AFAIL != AFAIL_KEEP;} // pixels may fail, write fb/z
	__forceinline bool NoSecondPass() {return ATE && ATST != ATST_ALWAYS && AFAIL == AFAIL_KEEP;} // pixels may fail, no output
REG_END2

REG64_(GIFReg, TEX0)
union
{
	struct
	{
		u32 TBP0:14;
		u32 TBW:6;
		u32 PSM:6;
		u32 TW:4;
		u32 _PAD1:2;
		u32 _PAD2:2;
		u32 TCC:1;
		u32 TFX:2;
		u32 CBP:14;
		u32 CPSM:4;
		u32 CSM:1;
		u32 CSA:5;
		u32 CLD:3;
	};

	struct
	{
		u64 _PAD3:30;
		u64 TH:4;
		u64 _PAD4:30;
	};
};
REG_END2 
	__forceinline bool IsRepeating() {return (u32)((u32)1 << TW) > (u32)(TBW << (u32)6);}
REG_END2

REG64_(GIFReg, TEX1)
	u32 LCM:1;
	u32 _PAD1:1;
	u32 MXL:3;
	u32 MMAG:1;
	u32 MMIN:3;
	u32 MTBA:1;
	u32 _PAD2:9;
	u32 L:2;
	u32 _PAD3:11;
	s32  K:12; // 1:7:4
	u32 _PAD4:20;
REG_END2
	bool IsMinLinear() const {return (MMIN == 1) || (MMIN & 4);}
	bool IsMagLinear() const {return MMAG;}
REG_END2

REG64_(GIFReg, TEX2)
	u32 _PAD1:20;
	u32 PSM:6;
	u32 _PAD2:6;
	u32 _PAD3:5;
	u32 CBP:14;
	u32 CPSM:4;
	u32 CSM:1;
	u32 CSA:5;
	u32 CLD:3;
REG_END

REG64_(GIFReg, TEXA)
	u32 TA0:8;
	u32 _PAD1:7;
	u32 AEM:1;
	u32 _PAD2:16;
	u32 TA1:8;
	u32 _PAD3:24;
REG_END

REG64_(GIFReg, TEXCLUT)
	u32 CBW:6;
	u32 COU:6;
	u32 COV:10;
	u32 _PAD1:10;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, TEXFLUSH)
	u32 _PAD1:32;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, TRXDIR)
	u32 XDIR:2;
	u32 _PAD1:30;
	u32 _PAD2:32;
REG_END

REG64_(GIFReg, TRXPOS)
	u32 SSAX:11;
	u32 _PAD1:5;
	u32 SSAY:11;
	u32 _PAD2:5;
	u32 DSAX:11;
	u32 _PAD3:5;
	u32 DSAY:11;
	u32 DIRY:1;
	u32 DIRX:1;
	u32 _PAD4:3;
REG_END

REG64_(GIFReg, TRXREG)
	u32 RRW:12;
	u32 _PAD1:20;
	u32 RRH:12;
	u32 _PAD2:20;
REG_END

REG64_(GIFReg, UV)
	u32 U:14;
	u32 _PAD1:2;
	u32 V:14;
	u32 _PAD2:2;
	u32 _PAD3:32;
REG_END

REG64_(GIFReg, XYOFFSET)
	u32 OFX:16; 
	u32 _PAD1:16;
	u32 OFY:16; 
	u32 _PAD2:16;
REG_END

REG64_(GIFReg, XYZ)
	u32 X:16;
	u32 Y:16;
	u32 Z:32;
REG_END

REG64_(GIFReg, XYZF)
	u32 X:16;
	u32 Y:16;
	u32 Z:24;
	u32 F:8;
REG_END

REG64_(GIFReg, ZBUF)
	u32 ZBP:9;
	u32 _PAD1:15;
	// u32 PSM:4;
	// u32 _PAD2:4;
	u32 PSM:6;
	u32 _PAD2:2;
	u32 ZMSK:1;
	u32 _PAD3:31;
REG_END2
	u32 Block() const {return ZBP << 5;}
REG_END2

REG64_SET(GIFReg)
	GIFRegALPHA			ALPHA;
	GIFRegBITBLTBUF		BITBLTBUF;
	GIFRegCLAMP			CLAMP;
	GIFRegCOLCLAMP		COLCLAMP;
	GIFRegDIMX			DIMX;
	GIFRegDTHE			DTHE;
	GIFRegFBA			FBA;
	GIFRegFINISH		FINISH;
	GIFRegFOG			FOG;
	GIFRegFOGCOL		FOGCOL;
	GIFRegFRAME			FRAME;
	GIFRegHWREG			HWREG;
	GIFRegLABEL			LABEL;
	GIFRegMIPTBP1		MIPTBP1;
	GIFRegMIPTBP2		MIPTBP2;
	GIFRegNOP			NOP;
	GIFRegPABE			PABE;
	GIFRegPRIM			PRIM;
	GIFRegPRMODE		PRMODE;
	GIFRegPRMODECONT	PRMODECONT;
	GIFRegRGBAQ			RGBAQ;
	GIFRegSCANMSK		SCANMSK;
	GIFRegSCISSOR		SCISSOR;
	GIFRegSIGNAL		SIGNAL;
	GIFRegST			ST;
	GIFRegTEST			TEST;
	GIFRegTEX0			TEX0;
	GIFRegTEX1			TEX1;
	GIFRegTEX2			TEX2;
	GIFRegTEXA			TEXA;
	GIFRegTEXCLUT		TEXCLUT;
	GIFRegTEXFLUSH		TEXFLUSH;
	GIFRegTRXDIR		TRXDIR;
	GIFRegTRXPOS		TRXPOS;
	GIFRegTRXREG		TRXREG;
	GIFRegUV			UV;
	GIFRegXYOFFSET		XYOFFSET;
	GIFRegXYZ			XYZ;
	GIFRegXYZF			XYZF;
	GIFRegZBUF			ZBUF;
REG_SET_END

// GIFPacked

REG128_(GIFPacked, PRIM)
	u32 PRIM:11;
	u32 _PAD1:21;
	u32 _PAD2:32;
	u32 _PAD3:32;
	u32 _PAD4:32;
REG_END

REG128_(GIFPacked, RGBA)
	u32 R:8;
	u32 _PAD1:24;
	u32 G:8;
	u32 _PAD2:24;
	u32 B:8;
	u32 _PAD3:24;
	u32 A:8;
	u32 _PAD4:24;
REG_END

REG128_(GIFPacked, STQ)
	float S;
	float T;
	float Q;
	u32 _PAD1:32;
REG_END

REG128_(GIFPacked, UV)
	u32 U:14;
	u32 _PAD1:18;
	u32 V:14;
	u32 _PAD2:18;
	u32 _PAD3:32;
	u32 _PAD4:32;
REG_END

REG128_(GIFPacked, XYZF2)
	u32 X:16;
	u32 _PAD1:16;
	u32 Y:16;
	u32 _PAD2:16;
	u32 _PAD3:4;
	u32 Z:24;
	u32 _PAD4:4;
	u32 _PAD5:4;
	u32 F:8;
	u32 _PAD6:3;
	u32 ADC:1;
	u32 _PAD7:16;
REG_END

REG128_(GIFPacked, XYZ2)
	u32 X:16;
	u32 _PAD1:16;
	u32 Y:16;
	u32 _PAD2:16;
	u32 Z:32;
	u32 _PAD3:15;
	u32 ADC:1;
	u32 _PAD4:16;
REG_END

REG128_(GIFPacked, FOG)
	u32 _PAD1:32;
	u32 _PAD2:32;
	u32 _PAD3:32;
	u32 _PAD4:4;
	u32 F:8;
	u32 _PAD5:20;
REG_END

REG128_(GIFPacked, A_D)
	u64 DATA:64;
	u32 ADDR:8; // enum GIF_A_D_REG
	u32 _PAD1:24;
	u32 _PAD2:32;
REG_END

REG128_(GIFPacked, NOP)
	u32 _PAD1:32;
	u32 _PAD2:32;
	u32 _PAD3:32;
	u32 _PAD4:32;
REG_END

REG128_SET(GIFPackedReg)
	GIFReg			r;
	GIFPackedPRIM	PRIM;
	GIFPackedRGBA	RGBA;
	GIFPackedSTQ	STQ;
	GIFPackedUV		UV;
	GIFPackedXYZF2	XYZF2;
	GIFPackedXYZ2	XYZ2;
	GIFPackedFOG	FOG;
	GIFPackedA_D	A_D;
	GIFPackedNOP	NOP;
REG_SET_END

extern void WriteTempRegs();
extern void SetFrameSkip(bool skip);
extern void ResetRegs();
#endif

#endif // NEWREGS_H_INCLUDED
