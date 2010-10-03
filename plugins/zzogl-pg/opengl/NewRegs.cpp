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

#include "GS.h"
#include "Mem.h"
#include "NewRegs.h"
#include "PS2Etypes.h"

#include "zerogs.h"
#include "targets.h"

#ifdef USE_OLD_REGS
#include "Regs.h"
#else

const u32 g_primmult[8] = { 1, 2, 2, 3, 3, 3, 2, 0xff };
const u32 g_primsub[8] = { 1, 2, 1, 3, 1, 1, 2, 0 };

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

GIFRegHandler g_GIFPackedRegHandlers[16];
GIFRegHandler g_GIFRegHandlers[256];
GIFRegHandler g_GIFTempRegHandlers[16] = {0};

// values for keeping track of changes
u32 s_uTex1Data[2][2] = {{0, }};
u32 s_uClampData[2] = {0, };

//u32 results[65535] = {0, };

// return true if triangle SHOULD be painted.
// My brain hurts. --arcum42

// return true if triangle SHOULD be painted.
inline bool NoHighlights(int i)
{
//	This is hack-code, I still in search of correct reason, why some triangles should not be drawn.

	int dummy = 0;
	
	u32 resultA = prim->iip + (2 * (prim->tme)) + (4 * (prim->fge)) + (8 * (prim->abe)) + (16 * (prim->aa1)) + (32 * (prim->fst)) + (64 * (prim->ctxt)) + (128 * (prim->fix));
	
	const pixTest curtest = ZeroGS::vb[i].test;
	
	u32 result = curtest.ate + ((curtest.atst) << 1) +((curtest.afail) << 4) + ((curtest.date) << 6) + ((curtest.datm) << 7) + ((curtest.zte) << 8) + ((curtest.ztst)<< 9);
	
	if ((resultA == 0x310a) && (result == 0x0)) return false; // Radiata Stories
	
	//Old code
	return (!(conf.settings().xenosaga_spec) || !ZeroGS::vb[i].zbuf.zmsk || prim->iip) ;
}

void __gifCall GIFPackedRegHandlerNull(const u32* data)
{
	FUNCLOG
	ZZLog::Debug_Log("Unexpected packed reg handler %8.8lx_%8.8lx %x.", data[0], data[1], data[2]);
}

// All these just call their non-packed equivalent.
void __gifCall GIFPackedRegHandlerPRIM(const u32* data) { GIFRegHandlerPRIM(data); }

template <u32 i>
void __gifCall GIFPackedRegHandlerTEX0(const u32* data) 
{ 
	GIFRegHandlerTEX0<i>(data); 
}

template <u32 i>
void __gifCall GIFPackedRegHandlerCLAMP(const u32* data) 
{ 
	GIFRegHandlerCLAMP<i>(data); 
}

void __gifCall GIFPackedRegHandlerTEX0_1(const u32* data) { GIFRegHandlerTEX0<0>(data); }
void __gifCall GIFPackedRegHandlerTEX0_2(const u32* data) { GIFRegHandlerTEX0<1>(data); }
void __gifCall GIFPackedRegHandlerCLAMP_1(const u32* data) { GIFRegHandlerCLAMP<0>(data); }
void __gifCall GIFPackedRegHandlerCLAMP_2(const u32* data) { GIFRegHandlerCLAMP<1>(data); }
void __gifCall GIFPackedRegHandlerXYZF3(const u32* data) { GIFRegHandlerXYZF3(data); }
void __gifCall GIFPackedRegHandlerXYZ3(const u32* data) { GIFRegHandlerXYZ3(data); }

void __gifCall GIFPackedRegHandlerRGBA(const u32* data)
{
	FUNCLOG
	
	GIFPackedRGBA* r = (GIFPackedRGBA*)(data);
	gs.rgba = (r->R | (r->G <<  8) | (r->B << 16) | (r->A << 24));
	gs.vertexregs.rgba = gs.rgba;
	gs.vertexregs.q = gs.q;
	
	ZZLog::Greg_Log("Packed RGBA: 0x%x", gs.rgba);
}

void __gifCall GIFPackedRegHandlerSTQ(const u32* data)
{
	FUNCLOG
	GIFPackedSTQ* r = (GIFPackedSTQ*)(data);
	gs.vertexregs.s = r->S;
	gs.vertexregs.t = r->T;
	gs.q = r->Q;
	ZZLog::Greg_Log("Packed STQ: 0x%x, 0x%x, %f", r->S, r->T, r->Q);
}

void __gifCall GIFPackedRegHandlerUV(const u32* data)
{
	FUNCLOG
	GIFPackedUV* r = (GIFPackedUV*)(data);
	gs.vertexregs.u = r->U;
	gs.vertexregs.v = r->V;
	ZZLog::Greg_Log("Packed UV: 0x%x, 0x%x", r->U, r->V);
}

void __gifCall KickVertex(bool adc)
{
	FUNCLOG
	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		if (!adc && NoHighlights(prim->ctxt)) (*ZeroGS::drawfn[prim->prim])();
		
		gs.primC -= g_primsub[prim->prim];

		if (adc && prim->prim == 5)
		{
			/* tri fans need special processing */
			if (gs.nTriFanVert == gs.primIndex)
				gs.primIndex = gs.primNext();
		}
	}
}

void __gifCall GIFPackedRegHandlerXYZF2(const u32* data)
{
	FUNCLOG
	GIFPackedXYZF2* r = (GIFPackedXYZF2*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

	// Fix Vertexes up later.
	KickVertex(!!(r->ADC));
	ZZLog::Greg_Log("Packed XYZF2: 0x%x, 0x%x, 0x%x, %f", r->X, r->Y, r->Z, r->F);
}

void __gifCall GIFPackedRegHandlerXYZ2(const u32* data)
{
	FUNCLOG
	GIFPackedXYZ2* r = (GIFPackedXYZ2*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

	// Fix Vertexes up later.
	KickVertex(!!(r->ADC));
	ZZLog::Greg_Log("Packed XYZ2: 0x%x, 0x%x, 0x%x", r->X, r->Y, r->Z);
}

void __gifCall GIFPackedRegHandlerFOG(const u32* data)
{
	FUNCLOG
	GIFPackedFOG* r = (GIFPackedFOG*)(data);
	gs.vertexregs.f = r->F;
	ZZLog::Greg_Log("Packed FOG: 0x%x", r->F);
}

void __gifCall GIFPackedRegHandlerA_D(const u32* data)
{
	FUNCLOG
	GIFPackedA_D* r = (GIFPackedA_D*)(data);
	
	g_GIFRegHandlers[r->ADDR](data);
	ZZLog::Greg_Log("Packed A_D: 0x%x", r->ADDR);
}

void __gifCall GIFPackedRegHandlerNOP(const u32* data)
{
	FUNCLOG
}

void __gifCall GIFRegHandlerNull(const u32* data)
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

void __gifCall GIFRegHandlerRGBAQ(const u32* data)
{
	FUNCLOG
	GIFRegRGBAQ* r = (GIFRegRGBAQ*)(data);
	gs.rgba = (r->R | (r->G <<  8) | (r->B << 16) | (r->A << 24));
	gs.vertexregs.rgba = gs.rgba;
	gs.vertexregs.q = r->Q;
	ZZLog::Greg_Log("RGBAQ: 0x%x, 0x%x, 0x%x, %f", r->R, r->G, r->B, r->A, r->Q);
}

void __gifCall GIFRegHandlerST(const u32* data)
{
	FUNCLOG
	GIFRegST* r = (GIFRegST*)(data);
	gs.vertexregs.s = r->S;
	gs.vertexregs.t = r->T;
	ZZLog::Greg_Log("ST: 0x%x, 0x%x", r->S, r->T);
}

void __gifCall GIFRegHandlerUV(const u32* data)
{
	// Baroque breaks if u&v are 16 bits instead of 14.
	FUNCLOG
	GIFRegUV* r = (GIFRegUV*)(data);
	gs.vertexregs.u = r->U;
	gs.vertexregs.v = r->V;
	ZZLog::Greg_Log("UV: 0x%x, 0x%x", r->U, r->V);
}

void __gifCall GIFRegHandlerXYZF2(const u32* data)
{
	FUNCLOG
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

	KickVertex(false);
	ZZLog::Greg_Log("XYZF2: 0x%x, 0x%x, 0x%x, %f", r->X, r->Y, r->Z, r->F);
}

void __gifCall GIFRegHandlerXYZ2(const u32* data)
{
	FUNCLOG
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

	KickVertex(false);
	ZZLog::Greg_Log("XYZF2: 0x%x, 0x%x, 0x%x", r->X, r->Y, r->Z);
}

template <u32 i>
void __gifCall GIFRegHandlerTEX0(const u32* data)
{
	// Used on Mana Khemias opening dialog.
	FUNCLOG
	
	GIFRegTEX0* r = (GIFRegTEX0*)(data);
	u32 psm = ZZOglGet_psm_TexBitsFix(data[0]);
	
	ZZLog::Greg_Log("TEX0_%d: 0x%x", i, data);

	// Worry about this later.
	if (!NoHighlights(i)) return;

	if (m_Blocks[psm].bpp == 0)
	{
		// kh and others
		return;
	}
	
	// Order is important.
	ZeroGS::vb[i].uNextTex0Data[0] = r->ai32[0];
	ZeroGS::vb[i].uNextTex0Data[1] = r->ai32[1];
	ZeroGS::vb[i].bNeedTexCheck = 1;
	
	// don't update unless necessary
	if (PSMT_ISCLUT(psm))
	{
		if (ZeroGS::CheckChangeInClut(data[1], psm))
		{
			// loading clut, so flush whole texture
			ZeroGS::vb[i].FlushTexData();
		}
		else if (r->CSA != (ZeroGS::vb[i].uCurTex0.CSA))
		{
			// check if csa is the same!! (ffx bisaid island, grass)
			ZeroGS::Flush(i); // flush any previous entries
		}
	}
}

template <u32 i>
void __gifCall GIFRegHandlerCLAMP(const u32* data)
{
	FUNCLOG
	clampInfo& clamp = ZeroGS::vb[i].clamp;
	GIFRegCLAMP* r = (GIFRegCLAMP*)(data);

	// Worry about this later.
	if (!NoHighlights(i)) return;

	if ((s_uClampData[i] != data[0]) || (((clamp.minv >> 8) | (clamp.maxv << 2)) != (data[1]&0x0fff)))
	{
		ZeroGS::Flush(i);

		ZeroGS::vb[i].bTexConstsSync = false;
	}
	
	s_uClampData[i] = data[0];

	clamp.wms  = r->WMS;
	clamp.wmt  = r->WMT;
	clamp.minu = r->MINU;
	clamp.maxu = r->MAXU;
	clamp.minv = r->MINV;
	clamp.maxv = r->MAXV;
	ZZLog::Greg_Log("CLAMP_%d: 0x%x", i, data);
}

void __gifCall GIFRegHandlerFOG(const u32* data)
{
	FUNCLOG
	GIFRegFOG* r = (GIFRegFOG*)(data);
	gs.vertexregs.f = r->F;
	ZZLog::Greg_Log("FOG: 0x%x", r->F);
}

void __gifCall GIFRegHandlerXYZF3(const u32* data)
{
	FUNCLOG
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

	KickVertex(true);
	ZZLog::Greg_Log("XYZF3: 0x%x, 0x%x, 0x%x, %f", r->X, r->Y, r->Z, r->F);
}

void __gifCall GIFRegHandlerXYZ3(const u32* data)
{
	FUNCLOG
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

	KickVertex(true);
	ZZLog::Greg_Log("XYZ3: 0x%x, 0x%x, 0x%x", r->X, r->Y, r->Z);
}

void __gifCall GIFRegHandlerNOP(const u32* data)
{
	FUNCLOG
}

template <u32 i>
void __fastcall GIFRegHandlerTEX1(const u32* data)
{
	FUNCLOG
	GIFRegTEX1* r = (GIFRegTEX1*)(data);
	tex1Info& tex1 = ZeroGS::vb[i].tex1;

	// Worry about this later.
	if (!NoHighlights(i)) return;

	if (conf.bilinear == 1 && (tex1.mmag != r->MMAG || tex1.mmin != r->MMIN))
	{
		ZeroGS::Flush(i);
		ZeroGS::vb[i].bVarsTexSync = false;
	}

	tex1.lcm  = r->LCM;

	tex1.mxl  = r->MXL;
	tex1.mmag = r->MMAG;
	tex1.mmin = r->MMIN;
	tex1.mtba = r->MTBA;
	tex1.l	= r->L;
	tex1.k	= r->K;
	ZZLog::Greg_Log("TEX1_%d: 0x%x", i, data);
}

template <u32 i>
void __gifCall GIFRegHandlerTEX2(const u32* data)
{
	FUNCLOG
	tex0Info& tex0 = ZeroGS::vb[i].tex0;

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
	ZZLog::Greg_Log("TEX2_%d: 0x%x", i, data);
}

template <u32 i>
void __gifCall GIFRegHandlerXYOFFSET(const u32* data)
{
	FUNCLOG
	// Affects that Mana Khemia opening dialog (when i == 0).
	GIFRegXYOFFSET* r = (GIFRegXYOFFSET*)(data);
	ZeroGS::vb[i].offset.x = r->OFX;
	ZeroGS::vb[i].offset.y = r->OFY;
	ZZLog::Greg_Log("XYOFFSET_%d: 0x%x, 0x%x", i, r->OFX, r->OFY);
}

// Fill out the vertex queue(prim) and the attributes.
void __gifCall GIFRegHandlerPRIM(const u32 *data)
{
	FUNCLOG
	GIFRegPRIM* r = (GIFRegPRIM*)(data);

	//if (data[0] & ~0x3ff)
	//{
		//ZZLog::Warn_Log("Warning: unknown bits in prim %8.8lx_%8.8lx", data[1], data[0]);
	//}

	// Come back to this one...
	gs.nTriFanVert = gs.primIndex;

	gs.primC = 0;
	prim->prim = r->PRIM;
	gs._prim[0].prim = r->PRIM;
	gs._prim[1].prim = r->PRIM;
	gs._prim[1]._val = (data[0] >> 3) & 0xff; // Setting the next 8 flags after prim at once.

	ZeroGS::Prim();
	ZZLog::Greg_Log("PRIM");
}

// Fill out an alternate set of attributes.
void __gifCall GIFRegHandlerPRMODE(const u32* data)
{
	FUNCLOG
	//GIFRegPRMODE* r = (GIFRegPRMODE*)(data);
	// Re-examine all code dealing with PRIMs in a bit.
	gs._prim[0]._val = (data[0] >> 3) & 0xff;

	if (gs.prac == 0) ZeroGS::Prim();
	ZZLog::Greg_Log("PRMODE");
}

// Switch between the primary set of attributes (gs._prim[1]) and the secondary (gs._prim[0]).
void __gifCall GIFRegHandlerPRMODECONT(const u32* data)
{
	FUNCLOG
	// Turns all the text into colored blocks on the initial Mana Khemia dialog if not run.
	GIFRegPRMODECONT* r = (GIFRegPRMODECONT*)(data);
	gs.prac = r->AC;
	prim = &gs._prim[gs.prac];

	ZeroGS::Prim();
	ZZLog::Greg_Log("PRMODECONT");
}

void __gifCall GIFRegHandlerTEXCLUT(const u32* data)
{
	FUNCLOG
	// Affects background coloration of initial Mana Khemia dialog.
	GIFRegTEXCLUT* r = (GIFRegTEXCLUT*)(data);

	ZeroGS::vb[0].FlushTexData();
	ZeroGS::vb[1].FlushTexData();
	
	// Fixme.
	gs.clut.cbw = r->CBW << 6;
	gs.clut.cou = r->COU << 4;
	gs.clut.cov = r->COV;
	ZZLog::Greg_Log("TEXCLUT: CBW:0x%x, COU:0x%x, COV:0x%x",r->CBW, r->COU, r->COV);
}

void __gifCall GIFRegHandlerSCANMSK(const u32* data)
{
	FUNCLOG
	GIFRegSCANMSK* r = (GIFRegSCANMSK*)(data);
	
	if(r->MSK != gs.smask)
	{
		ZeroGS::FlushBoth();
//		ZeroGS::ResolveC(&ZeroGS::vb[0]);
//		ZeroGS::ResolveZ(&ZeroGS::vb[0]);
	}
	
	gs.smask = r->MSK;
	ZZLog::Greg_Log("SCANMSK: 0x%x",r->MSK);
}

template <u32 i>
void __gifCall GIFRegHandlerMIPTBP1(const u32* data)
{
	FUNCLOG
	GIFRegMIPTBP1* r = (GIFRegMIPTBP1*)(data);
	/*if(PRIM->CTXT == i && r != miptbp0)
	{
		Flush();
	}*/
	
	miptbpInfo& miptbp0 = ZeroGS::vb[i].miptbp0;
	miptbp0.tbp[0] = r->TBP1;
	miptbp0.tbw[0] = r->TBW1;
	miptbp0.tbp[1] = r->TBP2;
	miptbp0.tbw[1] = r->TBW2;
	miptbp0.tbp[2] = r->TBP3;
	miptbp0.tbw[2] = r->TBW3;
	ZZLog::Greg_Log("MIPTBP1_%d: TBP/TBW: (0x%x, 0x%x), (0x%x, 0x%x), (0x%x, 0x%x)", i, r->TBP1, r->TBW1, r->TBP2, r->TBW2, r->TBP3, r->TBW3);
}

template <u32 i>
void __gifCall GIFRegHandlerMIPTBP2(const u32* data)
{
	FUNCLOG
	GIFRegMIPTBP2* r = (GIFRegMIPTBP2*)(data);
	// Yep.
	
	miptbpInfo& miptbp1 = ZeroGS::vb[i].miptbp1;
	miptbp1.tbp[0] = r->TBP4;
	miptbp1.tbw[0] = r->TBW4;
	miptbp1.tbp[1] = r->TBP5;
	miptbp1.tbw[1] = r->TBW5;
	miptbp1.tbp[2] = r->TBP6;
	miptbp1.tbw[2] = r->TBW6;
	ZZLog::Greg_Log("MIPTBP2_%d: TBP/TBW: (0x%x, 0x%x), (0x%x, 0x%x), (0x%x, 0x%x)", i, r->TBP4, r->TBW4, r->TBP5, r->TBW5, r->TBP6, r->TBW6);
}

void __gifCall GIFRegHandlerTEXA(const u32* data)
{
	FUNCLOG
	// Background of initial Mana Khemia dialog.
	GIFRegTEXA* r = (GIFRegTEXA*)(data);
	
	if ((r->AEM != gs.texa.aem) || (r->TA0 != gs.texa.ta[0]) || (r->TA1 != gs.texa.ta[1]))
	{
		ZeroGS::FlushBoth();

		ZeroGS::vb[0].bTexConstsSync = false;
		ZeroGS::vb[1].bTexConstsSync = false;
	}
		
	gs.texa.aem = r->AEM;
	gs.texa.ta[0] = r->TA0;
	gs.texa.ta[1] = r->TA1;
	gs.texa.fta[0] = r->TA0 / 255.0f;
	gs.texa.fta[1] = r->TA1 / 255.0f;
	ZZLog::Greg_Log("TEXA: AEM:0x%x, TA0:0x%x, TA1:0x%x", r->AEM, r->TA0, r->TA1);
}

void __gifCall GIFRegHandlerFOGCOL(const u32* data)
{
	FUNCLOG
	GIFRegFOGCOL* r = (GIFRegFOGCOL*)(data);
	
	if (gs.fogcol != r->ai32[0])
	{
		ZeroGS::FlushBoth();
	}
	
	ZeroGS::SetFogColor(r);
	gs.fogcol = r->ai32[0];
	ZZLog::Greg_Log("FOGCOL: 0x%x", r->ai32[0]);
}

void __gifCall GIFRegHandlerTEXFLUSH(const u32* data)
{
	FUNCLOG
	// GSdx doesn't even do anything here.
	ZeroGS::SetTexFlush();
	ZZLog::Greg_Log("TEXFLUSH");
}

template <u32 i>
void __gifCall GIFRegHandlerSCISSOR(const u32* data)
{
	FUNCLOG
	GIFRegSCISSOR* r = (GIFRegSCISSOR*)(data);
	Rect2& scissor = ZeroGS::vb[i].scissor;

	Rect2 newscissor;

	// << 3?
	newscissor.x0 = r->SCAX0 << 3;
	newscissor.x1 = r->SCAX1 << 3;
	newscissor.y0 = r->SCAY0 << 3;
	newscissor.y1 = r->SCAY1 << 3;

	if (newscissor.x1 != scissor.x1 || newscissor.y1 != scissor.y1 ||
			newscissor.x0 != scissor.x0 || newscissor.y0 != scissor.y0)
	{
		ZeroGS::Flush(i);
		
		// flush everything
		ZeroGS::vb[i].bNeedFrameCheck = 1;
	}
	
	scissor = newscissor;
	
	//Hmm...
	/*
	if(PRIM->CTXT == i && r->SCISSOR != m_env.CTXT[i].SCISSOR)
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = (GSVector4i)r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();*/
	ZZLog::Greg_Log("SCISSOR%d", i);
}

template <u32 i>
void __gifCall GIFRegHandlerALPHA(const u32* data)
{
	FUNCLOG
	// Mana Khemia Opening Dialog (when i = 0).
	GIFRegALPHA* r = (GIFRegALPHA*)(data);
	alphaInfo newalpha;
	
	newalpha.a = r->A;
	newalpha.b = r->B;
	newalpha.c = r->C;
	newalpha.d = r->D;
	newalpha.fix = r->FIX;
	
	if (newalpha.a == 3) newalpha.a = 0;
	if (newalpha.b == 3) newalpha.b = 0;
	if (newalpha.c == 3) newalpha.c = 0;
	if (newalpha.d == 3) newalpha.d = 0;
	
	if ((newalpha.abcd != ZeroGS::vb[i].alpha.abcd) || (newalpha.fix != ZeroGS::vb[i].alpha.fix))
	{
		ZeroGS::Flush(i);
	}
	
	ZeroGS::vb[i].alpha = newalpha;
	ZZLog::Greg_Log("ALPHA%d: A:0x%x B:0x%x C:0x%x D:0x%x FIX:0x%x ", i, r->A, r->B, r->C, r->D, r->FIX);
}

void __gifCall GIFRegHandlerDIMX(const u32* data)
{
	FUNCLOG
	
	GIFRegDIMX* r = (GIFRegDIMX*)(data);
	// Not even handled? Fixme.
	bool update = false;

	if (r->i64 != gs.dimx.i64)
	{
		ZeroGS::FlushBoth();

		update = true;
	}

	gs.dimx.i64 = r->i64;

	if (update)
	{
		//gs.UpdateDIMX();
	}
	ZZLog::Greg_Log("DIMX");
}

void __gifCall GIFRegHandlerDTHE(const u32* data)
{
	FUNCLOG
	GIFRegDTHE* r = (GIFRegDTHE*)(data);
	
	if (r->DTHE != gs.dthe)
	{
		ZeroGS::FlushBoth();
	}
	
	gs.dthe = r->DTHE;
	ZZLog::Greg_Log("DTHE: 0x%x ", r->DTHE);
}

void __gifCall GIFRegHandlerCOLCLAMP(const u32* data)
{
	FUNCLOG
	GIFRegCOLCLAMP* r = (GIFRegCOLCLAMP*)(data);
	
	if (r->CLAMP != gs.colclamp)
	{
		ZeroGS::FlushBoth();
	}
	
	gs.colclamp = r->CLAMP;
	ZZLog::Greg_Log("COLCLAMP: 0x%x ", r->CLAMP);
}

template <u32 i>
void __gifCall GIFRegHandlerTEST(const u32* data)
{
	FUNCLOG
	pixTest* test = &ZeroGS::vb[i].test;
	GIFRegTEST* r = (GIFRegTEST*)(data);
	
	if (test->_val != r->ai32[0])
	{
		ZeroGS::Flush(i);
	}

	test->_val = r->ai32[0];
	ZZLog::Greg_Log("TEST%d", i);
}

void __gifCall GIFRegHandlerPABE(const u32* data)
{
	FUNCLOG
	GIFRegPABE* r = (GIFRegPABE*)(data);
	
	if (gs.pabe != r->PABE)
	{
		ZeroGS::FlushBoth();
//		ZeroGS::SetAlphaChanged(0, GPUREG_PABE);
//		ZeroGS::SetAlphaChanged(1, GPUREG_PABE);
	}

	gs.pabe = r->PABE;
	ZZLog::Greg_Log("PABE: 0x%x ", r->PABE);
}

template <u32 i>
void __gifCall GIFRegHandlerFBA(const u32* data)
{
	FUNCLOG
	GIFRegFBA* r = (GIFRegFBA*)(data);
	
	if (r->FBA != ZeroGS::vb[i].fba.fba)
	{
		ZeroGS::FlushBoth();
	}
	
	ZeroGS::vb[i].fba.fba = r->FBA;
	ZZLog::Greg_Log("FBA%d: 0x%x ", i, r->FBA);
}

template<u32 i>
void __gifCall GIFRegHandlerFRAME(const u32* data)
{
	FUNCLOG
	// Affects opening dialogs, movie, and menu on Mana Khemia.
	
	GIFRegFRAME* r = (GIFRegFRAME*)(data);
	frameInfo& gsfb = ZeroGS::vb[i].gsfb;
	
	int fbw = r->FBW * 64;
	int fbp = r->FBP * 32;
	int fbh = 0;
	
	if (gs.dthe != 0)
	{
		// Dither here.
		//ZZLog::Error_Log("frameWrite: Dither!");
	}
	
	if ((gsfb.fbp == fbp) &&
			(gsfb.fbw == fbw) &&
			(gsfb.psm == r->PSM) &&
			(gsfb.fbm == ZZOglGet_fbm_FrameBitsFix(data[0], data[1])))
	{
		return;
	}

	ZeroGS::FlushBoth();
	if (r->FBW > 0) fbh = ZZOgl_fbh_Calc(r->FBP, r->FBW, r->PSM);

	gsfb.fbp = fbp;
	gsfb.fbw = fbw;
	gsfb.psm = r->PSM;
	gsfb.fbh = fbh;
	gsfb.fbm = ZZOglGet_fbm_FrameBitsFix(data[0], data[1]);
	

	ZeroGS::vb[i].bNeedFrameCheck = 1;
	ZZLog::Greg_Log("FRAME_%d", i);
}

template <u32 i>
void __gifCall GIFRegHandlerZBUF(const u32* data)
{
	FUNCLOG
	// I'll wait a bit on this one.
	GIFRegZBUF* r = (GIFRegZBUF*)(data);
	ZZLog::Greg_Log("ZBUF_1");
	
	zbufInfo& zbuf = ZeroGS::vb[i].zbuf;
	int psm = (0x30 | r->PSM);
	int zbp = r->ZBP * 32;

	if (zbuf.zbp == zbp &&
			zbuf.psm == psm &&
			zbuf.zmsk == r->ZMSK)
	{
		return;
	}

	// error detection
	if (m_Blocks[psm].bpp == 0) return;

	ZeroGS::FlushBoth();

	zbuf.zbp = zbp;
	zbuf.psm = psm;
	zbuf.zmsk = r->ZMSK;

	ZeroGS::vb[i].zprimmask = 0xffffffff;

	if (zbuf.psm > 0x31) ZeroGS::vb[i].zprimmask = 0xffff;

	ZeroGS::vb[i].bNeedZCheck = 1;
}

void __gifCall GIFRegHandlerBITBLTBUF(const u32* data)
{
	FUNCLOG
	// Required for *all* graphics. (Checked on Mana Khemia)
	
	GIFRegBITBLTBUF* r = (GIFRegBITBLTBUF*)(data);
	// Wonder why the shift?
	gs.srcbufnew.bp  = r->SBP;   // * 64;
	gs.srcbufnew.bw  = r->SBW << 6;
	gs.srcbufnew.psm = r->SPSM;
	gs.dstbufnew.bp  = r->DBP;   // * 64;
	gs.dstbufnew.bw  = r->DBW << 6;
	gs.dstbufnew.psm = r->DPSM;

	if (gs.dstbufnew.bw == 0) gs.dstbufnew.bw = 64;
	// GSdx does this:
	
	/*if((gs.srcbufnew.bw & 1) && (gs.srcbufnew.psm == PSM_PSMT8 || gs.srcbufnew.psm == PSM_PSMT4))
	{
		gs.srcbufnew.bw &= ~1;
	}

	if((gs.dstbufnew.bw & 1) && (gs.dstbufnew.psm == PSM_PSMT8 || gs.dstbufnew.psm == PSM_PSMT4))
	{
		gs.dstbufnew.bw &= ~1; // namcoXcapcom: 5, 11, refered to as 4, 10 in TEX0.TBW later
	}*/
	ZZLog::Greg_Log("BITBLTBUF");
}

void __gifCall GIFRegHandlerTRXPOS(const u32* data)
{
	// Affects Mana Khemia opening background.
	FUNCLOG
	GIFRegTRXPOS* r = (GIFRegTRXPOS*)(data);
	
	gs.trxposnew.sx  = r->SSAX;
	gs.trxposnew.sy  = r->SSAY;
	gs.trxposnew.dx  = r->DSAX;
	gs.trxposnew.dy  = r->DSAY;
	gs.trxposnew.dirx = r->DIRX;
	gs.trxposnew.diry = r->DIRY;
	ZZLog::Greg_Log("TRXPOS: SSA:(0x%x/0x%x) DSA:(0x%x/0x%x) DIR:(0x%x/0x%x)", r->SSAX, r->SSAY, r->DSAX, r->DSAY, r->DIRX, r->DIRY);
}

void __gifCall GIFRegHandlerTRXREG(const u32* data)
{
	FUNCLOG
	GIFRegTRXREG* r = (GIFRegTRXREG*)(data);
	gs.imageWtemp = r->RRW;
	gs.imageHtemp = r->RRH;
	ZZLog::Greg_Log("TRXREG: RRW: 0x%x, RRH: 0x%x", r->RRW, r->RRH);
}

void __gifCall GIFRegHandlerTRXDIR(const u32* data)
{
	FUNCLOG
	GIFRegTRXDIR* r = (GIFRegTRXDIR*)(data);
	// Oh dear...
	
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
	
	gs.imageTransfer = r->XDIR;
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
	ZZLog::Greg_Log("TRXDIR");
}

void __gifCall GIFRegHandlerHWREG(const u32* data)
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
	ZZLog::Greg_Log("HWREG");
}

extern int g_GSMultiThreaded;

void __gifCall GIFRegHandlerSIGNAL(const u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		SIGLBLID->SIGID = (SIGLBLID->SIGID & ~data[1]) | (data[0] & data[1]);

		if (gs.CSRw & 0x1)
		{
			CSR->SIGNAL = 1;
		}

		if (!IMR->SIGMSK && GSirq) GSirq();
	}
}

void __gifCall GIFRegHandlerFINISH(const u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		if (gs.CSRw & 0x2) CSR->FINISH = 1;

		if (!IMR->FINISHMSK && GSirq) GSirq();
	}
}

void __gifCall GIFRegHandlerLABEL(const u32* data)
{
	FUNCLOG

	if (!g_GSMultiThreaded)
	{
		SIGLBLID->LBLID = (SIGLBLID->LBLID & ~data[1]) | (data[0] & data[1]);
	}
}


void SetMultithreaded()
{
	// Some older versions of PCSX2 didn't properly set the irq callback to NULL
	// in multithreaded mode (possibly because ZeroGS itself would assert in such
	// cases), and didn't bind them to a dummy callback either.  PCSX2 handles all
	// IRQs internally when multithreaded anyway -- so let's ignore them here:

	if (g_GSMultiThreaded)
	{
		g_GIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GIFRegHandlerNull;
		g_GIFRegHandlers[GIF_A_D_REG_FINISH] = &GIFRegHandlerNull;
		g_GIFRegHandlers[GIF_A_D_REG_LABEL] = &GIFRegHandlerNull;
	}
	else
	{
		g_GIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GIFRegHandlerSIGNAL;
		g_GIFRegHandlers[GIF_A_D_REG_FINISH] = &GIFRegHandlerFINISH;
		g_GIFRegHandlers[GIF_A_D_REG_LABEL] = &GIFRegHandlerLABEL;
	}
}

void ResetRegs()
{
	for (int i = 0; i < 16; i++)
	{
		g_GIFPackedRegHandlers[i] = &GIFPackedRegHandlerNull;
	}

	g_GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerPRIM;
	g_GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerRGBA;
	g_GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerSTQ;
	g_GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerUV;
	g_GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerXYZF2;
	g_GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerXYZ2;
	g_GIFPackedRegHandlers[GIF_REG_TEX0_1] = &GIFPackedRegHandlerTEX0<0>;
	g_GIFPackedRegHandlers[GIF_REG_TEX0_2] = &GIFPackedRegHandlerTEX0<1>;
	g_GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerCLAMP<0>;
	g_GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerCLAMP<1>;
	g_GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerFOG;
	g_GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerXYZF3;
	g_GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerXYZ3;
	g_GIFPackedRegHandlers[GIF_REG_A_D] = &GIFPackedRegHandlerA_D;
	g_GIFPackedRegHandlers[GIF_REG_NOP] = &GIFPackedRegHandlerNOP;
	
	for (int i = 0; i < 256; i++)
	{
		g_GIFRegHandlers[i] = &GIFPackedRegHandlerNull;
	}
	
	g_GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerPRIM;
	g_GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerRGBAQ;
	g_GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerST;
	g_GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerUV;
	g_GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerXYZF2;
	g_GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerXYZ2;
	g_GIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GIFRegHandlerTEX0<0>;
	g_GIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GIFRegHandlerTEX0<1>;
	g_GIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GIFRegHandlerCLAMP<0>;
	g_GIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GIFRegHandlerCLAMP<1>;
	g_GIFRegHandlers[GIF_A_D_REG_FOG] = &GIFRegHandlerFOG;
	g_GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerXYZF3;
	g_GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerXYZ3;
	g_GIFRegHandlers[GIF_A_D_REG_NOP] = &GIFRegHandlerNOP;
	g_GIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GIFRegHandlerTEX1<0>;
	g_GIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GIFRegHandlerTEX1<1>;
	g_GIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GIFRegHandlerTEX2<0>;
	g_GIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GIFRegHandlerTEX2<1>;
	g_GIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GIFRegHandlerXYOFFSET<0>;
	g_GIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GIFRegHandlerXYOFFSET<1>;
	g_GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerPRMODECONT;
	g_GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerPRMODE;
	g_GIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GIFRegHandlerTEXCLUT;
	g_GIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GIFRegHandlerSCANMSK;
	g_GIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GIFRegHandlerMIPTBP1<0>;
	g_GIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GIFRegHandlerMIPTBP1<1>;
	g_GIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GIFRegHandlerMIPTBP2<0>;
	g_GIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GIFRegHandlerMIPTBP2<1>;
	g_GIFRegHandlers[GIF_A_D_REG_TEXA] = &GIFRegHandlerTEXA;
	g_GIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GIFRegHandlerFOGCOL;
	g_GIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GIFRegHandlerTEXFLUSH;
	g_GIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GIFRegHandlerSCISSOR<0>;
	g_GIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GIFRegHandlerSCISSOR<1>;
	g_GIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GIFRegHandlerALPHA<0>;
	g_GIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GIFRegHandlerALPHA<1>;
	g_GIFRegHandlers[GIF_A_D_REG_DIMX] = &GIFRegHandlerDIMX;
	g_GIFRegHandlers[GIF_A_D_REG_DTHE] = &GIFRegHandlerDTHE;
	g_GIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GIFRegHandlerCOLCLAMP;
	g_GIFRegHandlers[GIF_A_D_REG_TEST_1] = &GIFRegHandlerTEST<0>;
	g_GIFRegHandlers[GIF_A_D_REG_TEST_2] = &GIFRegHandlerTEST<1>;
	g_GIFRegHandlers[GIF_A_D_REG_PABE] = &GIFRegHandlerPABE;
	g_GIFRegHandlers[GIF_A_D_REG_FBA_1] = &GIFRegHandlerFBA<0>;
	g_GIFRegHandlers[GIF_A_D_REG_FBA_2] = &GIFRegHandlerFBA<1>;
	g_GIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GIFRegHandlerFRAME<0>;
	g_GIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GIFRegHandlerFRAME<1>;
	g_GIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GIFRegHandlerZBUF<0>;
	g_GIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GIFRegHandlerZBUF<1>;
	g_GIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GIFRegHandlerBITBLTBUF;
	g_GIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GIFRegHandlerTRXPOS;
	g_GIFRegHandlers[GIF_A_D_REG_TRXREG] = &GIFRegHandlerTRXREG;
	g_GIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GIFRegHandlerTRXDIR;
	g_GIFRegHandlers[GIF_A_D_REG_HWREG] = &GIFRegHandlerHWREG;
	SetMultithreaded();
}

void WriteTempRegs()
{
	memcpy(g_GIFTempRegHandlers, g_GIFPackedRegHandlers, sizeof(g_GIFTempRegHandlers));
}

void SetFrameSkip(bool skip)
{
	if (skip)
	{
		g_GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerNOP;

		g_GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerNOP;
	}
	else
	{
		g_GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerPRIM;
		g_GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerRGBA;
		g_GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerSTQ;
		g_GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerUV;
		g_GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerXYZF2;
		g_GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerXYZ2;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerCLAMP<0>;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerCLAMP<1>;
		g_GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerFOG;
		g_GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerXYZF3;
		g_GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerXYZ3;

		g_GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerPRIM;
		g_GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerRGBAQ;
		g_GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerST;
		g_GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerUV;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerXYZF2;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerXYZ2;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerXYZF3;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerXYZ3;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerPRMODECONT;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerPRMODE;
	}
}
#endif
