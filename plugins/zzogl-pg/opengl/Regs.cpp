/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerorog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "Regs.h"
#include "PS2Etypes.h"

#include "zerogs.h"
#include "targets.h"

const u32 g_primmult[8] = { 1, 2, 2, 3, 3, 3, 2, 0xff };
const u32 g_primsub[8] = { 1, 2, 1, 3, 1, 1, 2, 0 };

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

GIFRegHandler g_GIFPackedRegHandlers[16] =
{
	GIFRegHandlerPRIM,		  GIFPackedRegHandlerRGBA,	GIFPackedRegHandlerSTQ, GIFPackedRegHandlerUV,
	GIFPackedRegHandlerXYZF2,   GIFPackedRegHandlerXYZ2,	GIFRegHandlerTEX0_1,	GIFRegHandlerTEX0_2,
	GIFRegHandlerCLAMP_1,	   GIFRegHandlerCLAMP_2,	   GIFPackedRegHandlerFOG, GIFPackedRegHandlerNull,
	GIFRegHandlerXYZF3,		 GIFRegHandlerXYZ3,		  GIFPackedRegHandlerA_D, GIFPackedRegHandlerNOP
};

GIFRegHandler g_GIFRegHandlers[] =
{
	GIFRegHandlerPRIM,	  GIFRegHandlerRGBAQ,	 GIFRegHandlerST,		GIFRegHandlerUV,
	GIFRegHandlerXYZF2,	 GIFRegHandlerXYZ2,	  GIFRegHandlerTEX0_1,	GIFRegHandlerTEX0_2,
	GIFRegHandlerCLAMP_1,   GIFRegHandlerCLAMP_2,   GIFRegHandlerFOG,	   GIFRegHandlerNull,
	GIFRegHandlerXYZF3,	 GIFRegHandlerXYZ3,	  GIFRegHandlerNOP,	   GIFRegHandlerNOP,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerTEX1_1,	GIFRegHandlerTEX1_2,	GIFRegHandlerTEX2_1,	GIFRegHandlerTEX2_2,
	GIFRegHandlerXYOFFSET_1, GIFRegHandlerXYOFFSET_2, GIFRegHandlerPRMODECONT, GIFRegHandlerPRMODE,
	GIFRegHandlerTEXCLUT,   GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerSCANMSK,   GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerMIPTBP1_1, GIFRegHandlerMIPTBP1_2, GIFRegHandlerMIPTBP2_1, GIFRegHandlerMIPTBP2_2,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerTEXA,
	GIFRegHandlerNull,	  GIFRegHandlerFOGCOL,	GIFRegHandlerNull,	  GIFRegHandlerTEXFLUSH,
	GIFRegHandlerSCISSOR_1, GIFRegHandlerSCISSOR_2, GIFRegHandlerALPHA_1,   GIFRegHandlerALPHA_2,
	GIFRegHandlerDIMX,	  GIFRegHandlerDTHE,	  GIFRegHandlerCOLCLAMP,  GIFRegHandlerTEST_1,
	GIFRegHandlerTEST_2,	GIFRegHandlerPABE,	  GIFRegHandlerFBA_1,	 GIFRegHandlerFBA_2,
	GIFRegHandlerFRAME_1,   GIFRegHandlerFRAME_2,   GIFRegHandlerZBUF_1,	GIFRegHandlerZBUF_2,
	GIFRegHandlerBITBLTBUF, GIFRegHandlerTRXPOS,	GIFRegHandlerTRXREG,	GIFRegHandlerTRXDIR,
	GIFRegHandlerHWREG,	 GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,	  GIFRegHandlerNull,
	GIFRegHandlerSIGNAL,	GIFRegHandlerFINISH,	GIFRegHandlerLABEL,	 GIFRegHandlerNull
};

C_ASSERT(sizeof(g_GIFRegHandlers) / sizeof(g_GIFRegHandlers[0]) == 100);

// values for keeping track of changes
u32 s_uTex1Data[2][2] = {{0, }};
u32 s_uClampData[2] = {0, };

u32 results[65535] = {0, };

// return true if triangle SHOULD be painted.
inline bool NoHighlights(int i)
{
//	This is hack-code, I still in search of correct reason, why some triangles should not be drawn.

	// I'd have thought we could just test prim->_val and ZeroGS::vb[i].zbuf.psm directly...
	int resultA = prim->iip + ((prim->tme) << 1) + ((prim->fge) << 2) + ((prim->abe) << 3) + ((prim->aa1) << 4) + ((prim->fst) << 5) + ((prim->ctxt) << 6) + ((prim->fix) << 7) +
				  ((ZeroGS::vb[i].zbuf.psm) << 8);
//	if ( results[resultA] == 0 ) {
//		results[resultA] = 1;
//		ZZLog::Error_Log("%x = %d %d %d %d %d %d %d %d psm: %x", resultA, prim->iip, (prim->tme), (prim->fge), (prim->abe) , (prim->aa1) ,(prim->fst), (prim->ctxt), (prim->fix), ZeroGS::vb[i].zbuf.psm) ;
//	}
//	if (resultA == 0xb && ZeroGS::vb[i].zbuf.zmsk ) return false; //ATF

	const pixTest curtest = ZeroGS::vb[i].test;
	// Again, couldn't we just test curtest._val?
	int result = curtest.ate + ((curtest.atst) << 1) + ((curtest.afail) << 4) + ((curtest.date) << 6) + ((curtest.datm) << 7) + ((curtest.zte) << 8) + ((curtest.ztst) << 9);
//	if (resultA == 0xb)
//		if ( results[result] == 0) {
//			results[result] = 1;
//			ZZLog::Error_Log("0x%x = %d %d %d %d %d %d %d %d ", result, curtest.ate, curtest.atst, curtest.aref, curtest.afail, curtest.date, curtest.datm, curtest.zte, curtest.ztst);
//		}
//	0, -50b, -500, !-300, -30a, -50a, -5cb, +100 (zte==1), -50d
//	if (result == 0x50b && ZeroGS::vb[i].zbuf.zmsk ) return false; //ATF

	// if psm is 16S or 24, tme, abe, & fst are true, the rest are false, result is 0x302 or 0x700, and there is a mask.

	if ((resultA == 0x3a2a || resultA == 0x312a) && (result == 0x302 || result == 0x700) && (ZeroGS::vb[i].zbuf.zmsk)) return false; // Silent Hill:SM and Front Mission 5, result != 0x300

	// if psm is 24, abe is true, tme doesn't matter, the rest are false, result is 0x54c or 0x50c and there is a mask.
	if (((resultA == 0x3100) || (resultA == 0x3108)) && ((result == 0x54c) || (result == 0x50c)) && (ZeroGS::vb[i].zbuf.zmsk)) return false; // Okage

	// if psm is 24, abe & tme are true, the rest are false, and no result.
	if ((resultA == 0x310a) && (result == 0x0)) return false; // Radiata Stories

	// if psm is 16S, tme, abe, fst, and ctxt are true, the rest are false, result is 0x330 or 0x500, and there is a mask.
	if (resultA == 0x3a6a && (result == 0x300 || result == 0x500) && ZeroGS::vb[i].zbuf.zmsk) return false; // Okami, result != 0x30d

//	if ((resultA == 0x300b) && (result == 0x300) && ZeroGS::vb[i].zbuf.zmsk) return false; // ATF, but no Melty Blood

//	Old code
	return (!(g_GameSettings&GAME_XENOSPECHACK) || !ZeroGS::vb[i].zbuf.zmsk || prim->iip) ;
}

void __fastcall GIFPackedRegHandlerNull(u32* data)
{
	FUNCLOG
	ZZLog::Debug_Log("Unexpected packed reg handler %8.8lx_%8.8lx %x.", data[0], data[1], data[2]);
}

void __fastcall GIFPackedRegHandlerRGBA(u32* data)
{
	FUNCLOG
	gs.rgba = (data[0] & 0xff) |
			  ((data[1] & 0xff) <<  8) |
			  ((data[2] & 0xff) << 16) |
			  ((data[3] & 0xff) << 24);
	gs.vertexregs.rgba = gs.rgba;
	gs.vertexregs.q = gs.q;
}

void __fastcall GIFPackedRegHandlerSTQ(u32* data)
{
	FUNCLOG
	*(u32*)&gs.vertexregs.s = data[0] & 0xffffff00;
	*(u32*)&gs.vertexregs.t = data[1] & 0xffffff00;
	*(u32*)&gs.q = data[2];
}

void __fastcall GIFPackedRegHandlerUV(u32* data)
{
	FUNCLOG
	gs.vertexregs.u = data[0] & 0x3fff;
	gs.vertexregs.v = data[1] & 0x3fff;
}

void __forceinline KICK_VERTEX2()
{
	FUNCLOG

	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		if (NoHighlights(prim->ctxt)) (*ZeroGS::drawfn[prim->prim])();

		gs.primC -= g_primsub[prim->prim];
	}
}

void __forceinline KICK_VERTEX3()
{
	FUNCLOG

	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		gs.primC -= g_primsub[prim->prim];

		if (prim->prim == 5)
		{
			/* tri fans need special processing */
			if (gs.nTriFanVert == gs.primIndex)
				gs.primIndex = (gs.primIndex + 1) % ArraySize(gs.gsvertex);
		}
	}
}

void __fastcall GIFPackedRegHandlerXYZF2(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0] >> 0) & 0xffff;
	gs.vertexregs.y = (data[1] >> 0) & 0xffff;
	gs.vertexregs.z = (data[2] >> 4) & 0xffffff;
	gs.vertexregs.f = (data[3] >> 4) & 0xff;
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ArraySize(gs.gsvertex);

	if (data[3] & 0x8000)
	{
		KICK_VERTEX3();
	}
	else
	{
		KICK_VERTEX2();
	}
}

void __fastcall GIFPackedRegHandlerXYZ2(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0] >> 0) & 0xffff;
	gs.vertexregs.y = (data[1] >> 0) & 0xffff;
	gs.vertexregs.z = data[2];
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ArraySize(gs.gsvertex);

	if (data[3] & 0x8000)
	{
		KICK_VERTEX3();
	}
	else
	{
		KICK_VERTEX2();
	}
}

void __fastcall GIFPackedRegHandlerFOG(u32* data)
{
	FUNCLOG
	gs.vertexregs.f = (data[3] & 0xff0) >> 4;
}

void __fastcall GIFPackedRegHandlerA_D(u32* data)
{
	FUNCLOG

	if ((data[2] & 0xff) < 100)
		g_GIFRegHandlers[data[2] & 0xff](data);
	else
		GIFRegHandlerNull(data);
}

void __fastcall GIFPackedRegHandlerNOP(u32* data)
{
	FUNCLOG
}

extern int g_PrevBitwiseTexX, g_PrevBitwiseTexY;

void tex0Write(int i, u32 *data)
{
	FUNCLOG
	u32 psm = ZZOglGet_psm_TexBitsFix(data[0]);

	if (m_Blocks[psm].bpp == 0)
	{
		// kh and others
		return;
	}

	ZeroGS::vb[i].uNextTex0Data[0] = data[0];

	ZeroGS::vb[i].uNextTex0Data[1] = data[1];
	ZeroGS::vb[i].bNeedTexCheck = 1;

	// don't update unless necessary

	if (PSMT_ISCLUT(psm))
	{
		if (ZeroGS::CheckChangeInClut(data[1], psm))
		{
			// loading clut, so flush whole texture
			ZeroGS::vb[i].FlushTexData();
		}

		// check if csa is the same!! (ffx bisaid island, grass)
		else if ((data[1] & 0x1f780000) != (ZeroGS::vb[i].uCurTex0Data[1] & 0x1f780000))
		{
			ZeroGS::Flush(i); // flush any previous entries
		}
	}
}

void tex2Write(int i, u32 *data)
{
	FUNCLOG
	tex0Info& tex0 = ZeroGS::vb[i].tex0;

	if (ZeroGS::vb[i].bNeedTexCheck)
		ZeroGS::vb[i].FlushTexData();

	u32 psm = ZZOglGet_psm_TexBitsFix(data[0]);

	u32* s_uTex0Data = ZeroGS::vb[i].uCurTex0Data;

	// don't update unless necessary
//	if( ZZOglGet_psm_TexBitsFix(*s_uTex0Data) == ZZOglGet_psm_TexBitsFix(data[0]) ) { // psm is the same
	if (ZZOglAllExceptClutIsSame(s_uTex0Data, data))
	{
		if (!PSMT_ISCLUT(psm)) return;

		// have to write the CLUT again if changed
		if (ZZOglClutMinusCLDunchanged(s_uTex0Data, data))
		{
			tex0.cld = ZZOglGet_cld_TexBits(data[1]);

			if (tex0.cld != 0)
			{
				ZeroGS::texClutWrite(i);
				// invalidate to make sure target didn't change!
				ZeroGS::vb[i].bVarsTexSync = false;
			}

			return;
		}
	}

	ZeroGS::Flush(i);

	ZeroGS::vb[i].bVarsTexSync = false;
	ZeroGS::vb[i].bTexConstsSync = false;

	s_uTex0Data[0] = (s_uTex0Data[0] & ~0x03f00000) | (psm << 20);
	s_uTex0Data[1] = (s_uTex0Data[1] & 0x1f) | (data[1] & ~0x1f);

	tex0.psm = ZZOglGet_psm_TexBitsFix(data[0]);

	if (PSMT_ISCLUT(tex0.psm)) ZeroGS::CluttingForFlushedTex(&tex0, data[1], i);
}

__forceinline void frameWrite(int i, u32 *data)
{
	FUNCLOG
	frameInfo& gsfb = ZeroGS::vb[i].gsfb;

	if ((gsfb.fbp == ZZOglGet_fbp_FrameBitsMult(data[0])) &&
			(gsfb.fbw == ZZOglGet_fbw_FrameBitsMult(data[0])) &&
			(gsfb.psm == ZZOglGet_psm_FrameBits(data[0])) &&
			(gsfb.fbm == ZZOglGet_fbm_FrameBits(data[0])))
	{
		return;
	}

	ZeroGS::Flush(0);
	ZeroGS::Flush(1);

	gsfb.fbp = ZZOglGet_fbp_FrameBitsMult(data[0]);
	gsfb.fbw = ZZOglGet_fbw_FrameBitsMult(data[0]);
	gsfb.psm = ZZOglGet_psm_FrameBits(data[0]);
	gsfb.fbm = ZZOglGet_fbm_FrameBitsFix(data[0], data[1]);
	gsfb.fbh = ZZOglGet_fbh_FrameBitsCalc(data[0]);
//	gsfb.fbhCalc = gsfb.fbh;

	ZeroGS::vb[i].bNeedFrameCheck = 1;
}

__forceinline void testWrite(int i, u32 *data)
{
	FUNCLOG
	pixTest* test = &ZeroGS::vb[i].test;

	if ((*(u32*)test & 0x0007ffff) == (data[0] & 0x0007ffff)) return;

	ZeroGS::Flush(i);

	*(u32*)test = data[0];

//  test.ate   = (data[0]	  ) & 0x1;
//  test.atst  = (data[0] >>  1) & 0x7;
//  test.aref  = (data[0] >>  4) & 0xff;
//  test.afail = (data[0] >> 12) & 0x3;
//  test.date  = (data[0] >> 14) & 0x1;
//  test.datm  = (data[0] >> 15) & 0x1;
//  test.zte   = (data[0] >> 16) & 0x1;
//  test.ztst  = (data[0] >> 17) & 0x3;
}

__forceinline void clampWrite(int i, u32 *data)
{
	FUNCLOG
	clampInfo& clamp = ZeroGS::vb[i].clamp;

	if ((s_uClampData[i] != data[0]) || (((clamp.minv >> 8) | (clamp.maxv << 2)) != (data[1]&0x0fff)))
	{

		ZeroGS::Flush(i);
		s_uClampData[i] = data[0];

		clamp.wms  = (data[0]) & 0x3;
		clamp.wmt  = (data[0] >>  2) & 0x3;
		clamp.minu = (data[0] >>  4) & 0x3ff;
		clamp.maxu = (data[0] >> 14) & 0x3ff;
		clamp.minv = ((data[0] >> 24) & 0xff) | ((data[1] & 0x3) << 8);
		clamp.maxv = (data[1] >> 2) & 0x3ff;

		ZeroGS::vb[i].bTexConstsSync = false;
	}
}

void __fastcall GIFRegHandlerNull(u32* data)
{
	FUNCLOG
#ifdef _DEBUG

	if ((((uptr)&data[2])&0xffff) == 0) return;

	// 0x7f happens on a lot of games
	if (data[2] != 0x7f && (data[0] || data[1]))
	{
		ZZLog::Debug_Log("Unexpected reg handler %x %x %x.", data[0], data[1], data[2]);
	}

#endif
}

void __fastcall GIFRegHandlerPRIM(u32 *data)
{
	FUNCLOG

	if (data[0] & ~0x3ff)
	{
		//ZZLog::Warn_Log("Warning: unknown bits in prim %8.8lx_%8.8lx", data[1], data[0]);
	}

	gs.nTriFanVert = gs.primIndex;

	gs.primC = 0;
	prim->prim = (data[0]) & 0x7;
	gs._prim[0].prim = (data[0]) & 0x7;
	gs._prim[1].prim = (data[0]) & 0x7;
	gs._prim[1]._val = (data[0] >> 3) & 0xff;

	ZeroGS::Prim();
}

void __fastcall GIFRegHandlerRGBAQ(u32* data)
{
	FUNCLOG
	gs.rgba = data[0];
	gs.vertexregs.rgba = data[0];
	*(u32*)&gs.vertexregs.q = data[1];
}

void __fastcall GIFRegHandlerST(u32* data)
{
	FUNCLOG
	*(u32*)&gs.vertexregs.s = data[0] & 0xffffff00;
	*(u32*)&gs.vertexregs.t = data[1] & 0xffffff00;
	//*(u32*)&gs.q = data[2];
}

void __fastcall GIFRegHandlerUV(u32* data)
{
	FUNCLOG
	gs.vertexregs.u = (data[0]) & 0x3fff;
	gs.vertexregs.v = (data[0] >> 16) & 0x3fff;
}

void __fastcall GIFRegHandlerXYZF2(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0]) & 0xffff;
	gs.vertexregs.y = (data[0] >> (16)) & 0xffff;
	gs.vertexregs.z = data[1] & 0xffffff;
	gs.vertexregs.f = data[1] >> 24;
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ARRAY_SIZE(gs.gsvertex);

	KICK_VERTEX2();
}

void __fastcall GIFRegHandlerXYZ2(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0]) & 0xffff;
	gs.vertexregs.y = (data[0] >> (16)) & 0xffff;
	gs.vertexregs.z = data[1];
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ARRAY_SIZE(gs.gsvertex);

	KICK_VERTEX2();
}

void __fastcall GIFRegHandlerTEX0_1(u32* data)
{
	FUNCLOG

	if (!NoHighlights(0)) return;

	tex0Write(0, data);
}

void __fastcall GIFRegHandlerTEX0_2(u32* data)
{
	FUNCLOG

	if (!NoHighlights(1)) return;

	tex0Write(1, data);
}

void __fastcall GIFRegHandlerCLAMP_1(u32* data)
{
	FUNCLOG

	if (!NoHighlights(0)) return;

	clampWrite(0, data);
}

void __fastcall GIFRegHandlerCLAMP_2(u32* data)
{
	FUNCLOG

	if (!NoHighlights(1)) return;

	clampWrite(1, data);
}

void __fastcall GIFRegHandlerFOG(u32* data)
{
	FUNCLOG
	//gs.gsvertex[gs.primIndex].f = (data[1] >> 24);	// shift to upper bits
	gs.vertexregs.f = data[1] >> 24;
}

void __fastcall GIFRegHandlerXYZF3(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0]) & 0xffff;
	gs.vertexregs.y = (data[0] >> (16)) & 0xffff;
	gs.vertexregs.z = data[1] & 0xffffff;
	gs.vertexregs.f = data[1] >> 24;
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ARRAY_SIZE(gs.gsvertex);

	KICK_VERTEX3();
}

void __fastcall GIFRegHandlerXYZ3(u32* data)
{
	FUNCLOG
	gs.vertexregs.x = (data[0]) & 0xffff;
	gs.vertexregs.y = (data[0] >> (16)) & 0xffff;
	gs.vertexregs.z = data[1];
	gs.gsvertex[gs.primIndex] = gs.vertexregs;
	gs.primIndex = (gs.primIndex + 1) % ARRAY_SIZE(gs.gsvertex);

	KICK_VERTEX3();
}

void __fastcall GIFRegHandlerNOP(u32* data)
{
	FUNCLOG
}

void tex1Write(int i, u32* data)
{
	FUNCLOG
	tex1Info& tex1 = ZeroGS::vb[i].tex1;

	if (conf.bilinear == 1 && (tex1.mmag != ((data[0] >>  5) & 0x1) || tex1.mmin != ((data[0] >>  6) & 0x7)))
	{
		ZeroGS::Flush(i);
		ZeroGS::vb[i].bVarsTexSync = false;
	}

	tex1.lcm  = (data[0]) & 0x1;

	tex1.mxl  = (data[0] >>  2) & 0x7;
	tex1.mmag = (data[0] >>  5) & 0x1;
	tex1.mmin = (data[0] >>  6) & 0x7;
	tex1.mtba = (data[0] >>  9) & 0x1;
	tex1.l	= (data[0] >> 19) & 0x3;
	tex1.k	= (data[1] >> 4) & 0xff;
}

void __fastcall GIFRegHandlerTEX1_1(u32* data)
{
	FUNCLOG

	if (!NoHighlights(0)) return;

	tex1Write(0, data);
}

void __fastcall GIFRegHandlerTEX1_2(u32* data)
{
	FUNCLOG

	if (!NoHighlights(1)) return;

	tex1Write(1, data);
}

void __fastcall GIFRegHandlerTEX2_1(u32* data)
{
	FUNCLOG
	tex2Write(0, data);
}

void __fastcall GIFRegHandlerTEX2_2(u32* data)
{
	FUNCLOG
	tex2Write(1, data);
}

void __fastcall GIFRegHandlerXYOFFSET_1(u32* data)
{
	FUNCLOG
	// eliminator low 4 bits for now
	ZeroGS::vb[0].offset.x = (data[0]) & 0xffff;
	ZeroGS::vb[0].offset.y = (data[1]) & 0xffff;

//  if( !conf.interlace ) {
//	  ZeroGS::vb[0].offset.x &= ~15;
//	  ZeroGS::vb[0].offset.y &= ~15;
//  }
}

void __fastcall GIFRegHandlerXYOFFSET_2(u32* data)
{
	FUNCLOG
	ZeroGS::vb[1].offset.x = (data[0]) & 0xffff;
	ZeroGS::vb[1].offset.y = (data[1]) & 0xffff;

//  if( !conf.interlace ) {
//	  ZeroGS::vb[1].offset.x &= ~15;
//	  ZeroGS::vb[1].offset.y &= ~15;
//  }
}

void __fastcall GIFRegHandlerPRMODECONT(u32* data)
{
	FUNCLOG
	gs.prac = data[0] & 0x1;
	prim = &gs._prim[gs.prac];

	ZeroGS::Prim();
}

void __fastcall GIFRegHandlerPRMODE(u32* data)
{
	FUNCLOG
	gs._prim[0]._val = (data[0] >> 3) & 0xff;

	if (gs.prac == 0)
		ZeroGS::Prim();
}

void __fastcall GIFRegHandlerTEXCLUT(u32* data)
{
	FUNCLOG

	if (ZeroGS::vb[0].bNeedTexCheck) ZeroGS::vb[0].FlushTexData();
	if (ZeroGS::vb[1].bNeedTexCheck) ZeroGS::vb[1].FlushTexData();

	gs.clut.cbw = ((data[0]) & 0x3f) * 64;
	gs.clut.cou = ((data[0] >>  6) & 0x3f) * 16;
	gs.clut.cov = (data[0] >> 12) & 0x3ff;
}

void __fastcall GIFRegHandlerSCANMSK(u32* data)
{
	FUNCLOG
//  ZeroGS::Flush(0);
//  ZeroGS::Flush(1);
//  ZeroGS::ResolveC(&ZeroGS::vb[0]);
//  ZeroGS::ResolveZ(&ZeroGS::vb[0]);

	gs.smask = data[0] & 0x3;
}

void __fastcall GIFRegHandlerMIPTBP1_1(u32* data)
{
	FUNCLOG
	miptbpInfo& miptbp0 = ZeroGS::vb[0].miptbp0;
	miptbp0.tbp[0] = (data[0]) & 0x3fff;
	miptbp0.tbw[0] = (data[0] >> 14) & 0x3f;
	miptbp0.tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
	miptbp0.tbw[1] = (data[1] >>  2) & 0x3f;
	miptbp0.tbp[2] = (data[1] >>  8) & 0x3fff;
	miptbp0.tbw[2] = (data[1] >> 22) & 0x3f;
}

void __fastcall GIFRegHandlerMIPTBP1_2(u32* data)
{
	FUNCLOG
	miptbpInfo& miptbp0 = ZeroGS::vb[1].miptbp0;
	miptbp0.tbp[0] = (data[0]) & 0x3fff;
	miptbp0.tbw[0] = (data[0] >> 14) & 0x3f;
	miptbp0.tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
	miptbp0.tbw[1] = (data[1] >>  2) & 0x3f;
	miptbp0.tbp[2] = (data[1] >>  8) & 0x3fff;
	miptbp0.tbw[2] = (data[1] >> 22) & 0x3f;
}

void __fastcall GIFRegHandlerMIPTBP2_1(u32* data)
{
	FUNCLOG
	miptbpInfo& miptbp1 = ZeroGS::vb[0].miptbp1;
	miptbp1.tbp[0] = (data[0]) & 0x3fff;
	miptbp1.tbw[0] = (data[0] >> 14) & 0x3f;
	miptbp1.tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
	miptbp1.tbw[1] = (data[1] >>  2) & 0x3f;
	miptbp1.tbp[2] = (data[1] >>  8) & 0x3fff;
	miptbp1.tbw[2] = (data[1] >> 22) & 0x3f;
}

void __fastcall GIFRegHandlerMIPTBP2_2(u32* data)
{
	FUNCLOG
	miptbpInfo& miptbp1 = ZeroGS::vb[1].miptbp1;
	miptbp1.tbp[0] = (data[0]) & 0x3fff;
	miptbp1.tbw[0] = (data[0] >> 14) & 0x3f;
	miptbp1.tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
	miptbp1.tbw[1] = (data[1] >>  2) & 0x3f;
	miptbp1.tbp[2] = (data[1] >>  8) & 0x3fff;
	miptbp1.tbw[2] = (data[1] >> 22) & 0x3f;
}

void __fastcall GIFRegHandlerTEXA(u32* data)
{
	FUNCLOG
	texaInfo newinfo;
	newinfo.aem = (data[0] >> 15) & 0x1;
	newinfo.ta[0] = data[0] & 0xff;
	newinfo.ta[1] = data[1] & 0xff;

	if (*(u32*)&newinfo != *(u32*)&gs.texa)
	{
		ZeroGS::Flush(0);
		ZeroGS::Flush(1);
		*(u32*)&gs.texa = *(u32*) & newinfo;
		gs.texa.fta[0] = newinfo.ta[0] / 255.0f;
		gs.texa.fta[1] = newinfo.ta[1] / 255.0f;

		ZeroGS::vb[0].bTexConstsSync = false;
		ZeroGS::vb[1].bTexConstsSync = false;
	}
}

void __fastcall GIFRegHandlerFOGCOL(u32* data)
{
	FUNCLOG
	ZeroGS::SetFogColor(data[0]&0xffffff);
}

void __fastcall GIFRegHandlerTEXFLUSH(u32* data)
{
	FUNCLOG
	ZeroGS::SetTexFlush();
}

void __fastcall GIFRegHandlerSCISSOR_1(u32* data)
{
	FUNCLOG
	Rect2& scissor = ZeroGS::vb[0].scissor;

	Rect2 newscissor;

	newscissor.x0 = ((data[0]) & 0x7ff) << 3;
	newscissor.x1 = ((data[0] >> 16) & 0x7ff) << 3;
	newscissor.y0 = ((data[1]) & 0x7ff) << 3;
	newscissor.y1 = ((data[1] >> 16) & 0x7ff) << 3;

	if (newscissor.x1 != scissor.x1 || newscissor.y1 != scissor.y1 ||
			newscissor.x0 != scissor.x0 || newscissor.y0 != scissor.y0)
	{

		ZeroGS::Flush(0);
		scissor = newscissor;

		ZeroGS::vb[0].bNeedFrameCheck = 1;
	}
}

void __fastcall GIFRegHandlerSCISSOR_2(u32* data)
{
	FUNCLOG
	Rect2& scissor = ZeroGS::vb[1].scissor;

	Rect2 newscissor;

	newscissor.x0 = ((data[0]) & 0x7ff) << 3;
	newscissor.x1 = ((data[0] >> 16) & 0x7ff) << 3;
	newscissor.y0 = ((data[1]) & 0x7ff) << 3;
	newscissor.y1 = ((data[1] >> 16) & 0x7ff) << 3;

	if (newscissor.x1 != scissor.x1 || newscissor.y1 != scissor.y1 ||
			newscissor.x0 != scissor.x0 || newscissor.y0 != scissor.y0)
	{

		ZeroGS::Flush(1);
		scissor = newscissor;

		// flush everything
		ZeroGS::vb[1].bNeedFrameCheck = 1;
	}
}

void __fastcall GIFRegHandlerALPHA_1(u32* data)
{
	FUNCLOG
	alphaInfo newalpha;
	newalpha.abcd = *(u8*)data;
	newalpha.fix = *(u8*)(data + 1);

	if (*(u16*)&newalpha != *(u16*)&ZeroGS::vb[0].alpha)
	{
		ZeroGS::Flush(0);

		if (newalpha.a == 3) newalpha.a = 0;
		if (newalpha.b == 3) newalpha.b = 0;
		if (newalpha.c == 3) newalpha.c = 0;
		if (newalpha.d == 3) newalpha.d = 0;

		*(u16*)&ZeroGS::vb[0].alpha = *(u16*) & newalpha;
	}
}

void __fastcall GIFRegHandlerALPHA_2(u32* data)
{
	FUNCLOG
	alphaInfo newalpha;
	newalpha.abcd = *(u8*)data;
	newalpha.fix = *(u8*)(data + 1);

	if (*(u16*)&newalpha != *(u16*)&ZeroGS::vb[1].alpha)
	{
		ZeroGS::Flush(1);

		if (newalpha.a == 3) newalpha.a = 0;
		if (newalpha.b == 3) newalpha.b = 0;
		if (newalpha.c == 3) newalpha.c = 0;
		if (newalpha.d == 3) newalpha.d = 0;

		*(u16*)&ZeroGS::vb[1].alpha = *(u16*) & newalpha;
	}
}

void __fastcall GIFRegHandlerDIMX(u32* data)
{
	FUNCLOG
}

void __fastcall GIFRegHandlerDTHE(u32* data)
{
	FUNCLOG
	gs.dthe = data[0] & 0x1;
}

void __fastcall GIFRegHandlerCOLCLAMP(u32* data)
{
	FUNCLOG
	gs.colclamp = data[0] & 0x1;
}

void __fastcall GIFRegHandlerTEST_1(u32* data)
{
	FUNCLOG
	testWrite(0, data);
}

void __fastcall GIFRegHandlerTEST_2(u32* data)
{
	FUNCLOG
	testWrite(1, data);
}

void __fastcall GIFRegHandlerPABE(u32* data)
{
	FUNCLOG
	//ZeroGS::SetAlphaChanged(0, GPUREG_PABE);
	//ZeroGS::SetAlphaChanged(1, GPUREG_PABE);
	ZeroGS::Flush(0);
	ZeroGS::Flush(1);

	gs.pabe = *data & 0x1;
}

void __fastcall GIFRegHandlerFBA_1(u32* data)
{
	FUNCLOG
	ZeroGS::Flush(0);
	ZeroGS::Flush(1);
	ZeroGS::vb[0].fba.fba = *data & 0x1;
}

void __fastcall GIFRegHandlerFBA_2(u32* data)
{
	FUNCLOG
	ZeroGS::Flush(0);
	ZeroGS::Flush(1);
	ZeroGS::vb[1].fba.fba = *data & 0x1;
}

void __fastcall GIFRegHandlerFRAME_1(u32* data)
{
	FUNCLOG
	frameWrite(0, data);
}

void __fastcall GIFRegHandlerFRAME_2(u32* data)
{
	FUNCLOG
	frameWrite(1, data);
}

void __fastcall GIFRegHandlerZBUF_1(u32* data)
{
	FUNCLOG
	zbufInfo& zbuf = ZeroGS::vb[0].zbuf;

	int psm = (0x30 | ((data[0] >> 24) & 0xf));

	if (zbuf.zbp == (data[0] & 0x1ff) * 32 &&
			zbuf.psm == psm &&
			zbuf.zmsk == (data[1] & 0x1))
	{
		return;
	}

	// error detection
	if (m_Blocks[psm].bpp == 0) return;

	ZeroGS::Flush(0);
	ZeroGS::Flush(1);

	zbuf.zbp = (data[0] & 0x1ff) * 32;
	zbuf.psm = 0x30 | ((data[0] >> 24) & 0xf);
	zbuf.zmsk = data[1] & 0x1;

	ZeroGS::vb[0].zprimmask = 0xffffffff;

	if (zbuf.psm > 0x31) ZeroGS::vb[0].zprimmask = 0xffff;

	ZeroGS::vb[0].bNeedZCheck = 1;
}

void __fastcall GIFRegHandlerZBUF_2(u32* data)
{
	FUNCLOG
	zbufInfo& zbuf = ZeroGS::vb[1].zbuf;

	int psm = (0x30 | ((data[0] >> 24) & 0xf));

	if (zbuf.zbp == (data[0] & 0x1ff) * 32 &&
			zbuf.psm == psm &&
			zbuf.zmsk == (data[1] & 0x1))
	{
		return;
	}

	// error detection
	if (m_Blocks[psm].bpp == 0)
		return;

	ZeroGS::Flush(0);
	ZeroGS::Flush(1);

	zbuf.zbp = (data[0] & 0x1ff) * 32;

	zbuf.psm = 0x30 | ((data[0] >> 24) & 0xf);

	zbuf.zmsk = data[1] & 0x1;

	ZeroGS::vb[1].bNeedZCheck = 1;
	ZeroGS::vb[1].zprimmask = 0xffffffff;

	if (zbuf.psm > 0x31) ZeroGS::vb[1].zprimmask = 0xffff;
}

void __fastcall GIFRegHandlerBITBLTBUF(u32* data)
{
	FUNCLOG
	gs.srcbufnew.bp  = ((data[0]) & 0x3fff);   // * 64;
	gs.srcbufnew.bw  = ((data[0] >> 16) & 0x3f) * 64;
	gs.srcbufnew.psm = (data[0] >> 24) & 0x3f;
	gs.dstbufnew.bp  = ((data[1]) & 0x3fff);   // * 64;
	gs.dstbufnew.bw  = ((data[1] >> 16) & 0x3f) * 64;
	gs.dstbufnew.psm = (data[1] >> 24) & 0x3f;

	if (gs.dstbufnew.bw == 0) gs.dstbufnew.bw = 64;
}

void __fastcall GIFRegHandlerTRXPOS(u32* data)
{
	FUNCLOG
	gs.trxposnew.sx  = (data[0]) & 0x7ff;
	gs.trxposnew.sy  = (data[0] >> 16) & 0x7ff;
	gs.trxposnew.dx  = (data[1]) & 0x7ff;
	gs.trxposnew.dy  = (data[1] >> 16) & 0x7ff;
	gs.trxposnew.dir = (data[1] >> 27) & 0x3;
}

void __fastcall GIFRegHandlerTRXREG(u32* data)
{
	FUNCLOG
	gs.imageWtemp = data[0] & 0xfff;
	gs.imageHtemp = data[1] & 0xfff;
}

void __fastcall GIFRegHandlerTRXDIR(u32* data)
{
	FUNCLOG
	// terminate any previous transfers

	switch (gs.imageTransfer)
	{

		case 0: // host->loc
			gs.imageTransfer = -1;
			break;

		case 1: // loc->host
			ZeroGS::TerminateLocalHost();
			break;
	}

	gs.srcbuf = gs.srcbufnew;

	gs.dstbuf = gs.dstbufnew;
	gs.trxpos = gs.trxposnew;
	gs.imageTransfer = data[0] & 0x3;
	gs.imageWnew = gs.imageWtemp;
	gs.imageHnew = gs.imageHtemp;

	if (gs.imageWnew > 0 && gs.imageHnew > 0)
	{
		switch (gs.imageTransfer)
		{
			case 0: // host->loc
				ZeroGS::InitTransferHostLocal();
				break;

			case 1: // loc->host
				ZeroGS::InitTransferLocalHost();
				break;

			case 2:
				ZeroGS::TransferLocalLocal();
				break;

			case 3:
				gs.imageTransfer = -1;
				break;

			default:
				assert(0);
		}
	}
	else
	{
#if defined(ZEROGS_DEVBUILD)
		ZZLog::Warn_Log("Dummy transfer.");
#endif
		gs.imageTransfer = -1;
	}
}

void __fastcall GIFRegHandlerHWREG(u32* data)
{
	FUNCLOG

	if (gs.imageTransfer == 0)
	{
		ZeroGS::TransferHostLocal(data, 2);
	}
	else
	{
#if defined(ZEROGS_DEVBUILD)
		ZZLog::Error_Log("ZeroGS: HWREG!? %8.8x_%8.8x", data[0], data[1]);
		//assert(0);
#endif
	}
}

extern int g_GSMultiThreaded;

void __fastcall GIFRegHandlerSIGNAL(u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		SIGLBLID->SIGID = (SIGLBLID->SIGID & ~data[1]) | (data[0] & data[1]);

//	  if (gs.CSRw & 0x1) CSR->SIGNAL = 1;
//	  if (!IMR->SIGMSK && GSirq)
//		  GSirq();

		if (gs.CSRw & 0x1)
		{
			CSR->SIGNAL = 1;
			//gs.CSRw &= ~1;
		}

		if (!IMR->SIGMSK && GSirq) GSirq();
	}
}

void __fastcall GIFRegHandlerFINISH(u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		if (gs.CSRw & 0x2) CSR->FINISH = 1;

		if (!IMR->FINISHMSK && GSirq) GSirq();

//	  if( gs.CSRw & 2 ) {
//		  //gs.CSRw &= ~2;
//		  //CSR->FINISH = 0;
//
//
//	  }
//	  CSR->FINISH = 1;
//
//	  if( !IMR->FINISHMSK && GSirq )
//		  GSirq();
	}
}

void __fastcall GIFRegHandlerLABEL(u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		SIGLBLID->LBLID = (SIGLBLID->LBLID & ~data[1]) | (data[0] & data[1]);
	}
}
