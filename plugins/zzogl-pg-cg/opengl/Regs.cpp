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
#include "Regs.h"
#include "PS2Etypes.h"

#include "targets.h"
#include "ZZoglVB.h"
#include "ZZoglDrawing.h"

#ifdef USE_OLD_REGS

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

// Note that not all the registers are currently handled, even if they write values out.
// For reference, I'm starting a list of unhandled flags here. I'm sure I missed some,
// so feel free to add to this, or remove ones that are handled that I missed.
// Cases where these values are set would be useful, too.
//
// In GIFRegHandlerFOG, I don't see gs.vertexregs.f being used anywhere afterwards.
// GIFRegHandlerTEX1 doesn't look like anything other then mmag and mmin are handled.
//     This includes:
//          lcm  - the lod (level of detail) calculation method. If 0, it's (log2(1/|Q|)<<L)+K), whereas if it is one, it's just K.
//          mxl  - This is what MIPMAP level we use. The default is 0, and any other level uses miptbp0 & 1 to get the texture width. 
//          mtba - this is the base address specification for MIPMAP level 1+.
//          l    - Yeah, this is for the LOD calculation.
//          k    - This too.
//     This largely sums up as that we don't support MIPMAP level 1+ (much like GSdx), and LOD.
//
// In GIFRegHandlerSCANMSK, it doesn't look like gs.smask is used, though it may have been in the old resolve code.
//     Lets see: 00 is normal drawing, 01 is reserved, 10 prohibits drawing to even y coords, and 11 prohibits drawing to odd y coords.
//
// In GIFRegHandlerMIPTBP1 & 2, both miptbp0 & miptbp1 look unused, which isn't suprising, given mxl not being checked.
//
// GIFRegHandlerDIMX doesn't even have any code in it!
//     This is supposed to read in the matrix for dithering.
//
// In GIFRegHandlerDTHE, nothing is done with gs.dthe.
//     This goes right with the last one, because dthe is set to 1 when dithering with the dthe matrix.

// In GIFRegHandlerCOLCLAMP, gs.colclamp is not used.
//     This is color clamping on the RGB value. If it's 0, it is set to mask, the lower 8 bits are enabled, and it wraps around. At 1, it is clamped from 0-255.

//#define SPAM_UNUSED_REGISTERS
#ifdef SPAM_UNUSED_REGISTERS
#define REG_LOG ZZLog::Error_Log
#else
#define REG_LOG 0 && 
#endif

void __gifCall GIFPackedRegHandlerNull(const u32* data)
{
	FUNCLOG
	ZZLog::Debug_Log("Unexpected packed reg handler %8.8lx_%8.8lx %x.", data[0], data[1], data[2]);
}

// All these just call their non-packed equivalent.
void __gifCall GIFPackedRegHandlerPRIM(const u32* data) { GIFRegHandlerPRIM(data); }

template <u32 ctxt>
void __gifCall GIFPackedRegHandlerTEX0(const u32* data) { GIFRegHandlerTEX0<ctxt>(data); }

template <u32 ctxt>
void __gifCall GIFPackedRegHandlerCLAMP(const u32* data) { GIFRegHandlerCLAMP<ctxt>(data); }

void __gifCall GIFPackedRegHandlerXYZF3(const u32* data) { GIFRegHandlerXYZF3(data); }
void __gifCall GIFPackedRegHandlerXYZ3(const u32* data) { GIFRegHandlerXYZ3(data); }

void __gifCall GIFPackedRegHandlerRGBA(const u32* data)
{
	FUNCLOG
	GIFPackedRGBA* r = (GIFPackedRGBA*)(data);
	gs.rgba = (r->R | (r->G <<  8) | (r->B << 16) | (r->A << 24));
	gs.vertexregs.rgba = gs.rgba;
	gs.vertexregs.q = gs.q;
}

void __gifCall GIFPackedRegHandlerSTQ(const u32* data)
{
	FUNCLOG
//	GIFPackedSTQ* r = (GIFPackedSTQ*)(data);
//	gs.vertexregs.s = r->S;
//	gs.vertexregs.t = r->T;
//	gs.q = r->Q;
	// Despite this code generating a warning, it's correct. float -> float reduction. S and Y are missed mantissas.
	*(u32*)&gs.vertexregs.s = data[0] & 0xffffff00;
	*(u32*)&gs.vertexregs.t = data[1] & 0xffffff00;
	*(u32*)&gs.q = data[2];
}

void __gifCall GIFPackedRegHandlerUV(const u32* data)
{
	FUNCLOG
	GIFPackedUV* r = (GIFPackedUV*)(data);
	
	gs.vertexregs.u = r->U;
	gs.vertexregs.v = r->V;
}

void __gifCall GIFPackedRegHandlerXYZF2(const u32* data)
{
	FUNCLOG
	GIFPackedXYZF2* r = (GIFPackedXYZF2*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

	ZZKick->KickVertex(!!(r->ADC));
}

void __gifCall GIFPackedRegHandlerXYZ2(const u32* data)
{
	FUNCLOG
	GIFPackedXYZ2* r = (GIFPackedXYZ2*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

	ZZKick->KickVertex(!!(r->ADC));
}

void __gifCall GIFPackedRegHandlerFOG(const u32* data)
{
	FUNCLOG
	GIFPackedFOG* r = (GIFPackedFOG*)(data);
	gs.vertexregs.f = r->F;
//	gs.vertexregs.f = (data[3] & 0xff0) >> 4;
	if (gs.vertexregs.f != 0) REG_LOG("GIFPackedRegHandlerFOG == %d", gs.vertexregs.f);
}

void __gifCall GIFPackedRegHandlerA_D(const u32* data)
{
	FUNCLOG

	if ((data[2] & 0xff) < 100)
		g_GIFRegHandlers[data[2] & 0xff](data);
	else
		GIFRegHandlerNull(data);
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

void __gifCall GIFRegHandlerPRIM(const u32 *data)
{
	FUNCLOG

	//if (data[0] & ~0x3ff)
	//{
		//ZZLog::Warn_Log("Warning: unknown bits in prim %8.8lx_%8.8lx", data[1], data[0]);
	//}


	gs.primC = 0;
    u16 prim_type = (data[0]) & 0x7;
	prim->prim = prim_type;
	gs._prim[0].prim = prim_type;
	gs._prim[1].prim = prim_type;
	gs._prim[1]._val = (data[0] >> 3) & 0xff;

    gs.new_tri_fan = !(prim_type ^ PRIM_TRIANGLE_FAN);
    ZZKick->DirtyValidPrevPrim();

	Prim();
}

void __gifCall GIFRegHandlerRGBAQ(const u32* data)
{
	FUNCLOG
	gs.rgba = data[0];
	gs.vertexregs.rgba = data[0];
	*(u32*)&gs.vertexregs.q = data[1];
}

void __gifCall GIFRegHandlerST(const u32* data)
{
	FUNCLOG
	*(u32*)&gs.vertexregs.s = data[0] & 0xffffff00;
	*(u32*)&gs.vertexregs.t = data[1] & 0xffffff00;
	//*(u32*)&gs.q = data[2];
}

void __gifCall GIFRegHandlerUV(const u32* data)
{
	// Baroque breaks if u&v are 16 bits instead of 14.
	FUNCLOG
//	gs.vertexregs.u = (data[0]) & 0x3fff;
//	gs.vertexregs.v = (data[0] >> 16) & 0x3fff;
	GIFRegUV* r = (GIFRegUV*)(data);
	gs.vertexregs.u = r->U;
	gs.vertexregs.v = r->V;
}

void __gifCall GIFRegHandlerXYZF2(const u32* data)
{
	FUNCLOG
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

    ZZKick->KickVertex(false);
}

void __gifCall GIFRegHandlerXYZ2(const u32* data)
{
	FUNCLOG
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

    ZZKick->KickVertex(false);
}

template <u32 ctxt>
void __gifCall GIFRegHandlerTEX0(const u32* data)
{
	FUNCLOG
	
	if (!NoHighlights(ctxt)) return;
	
	u32 psm = ZZOglGet_psm_TexBitsFix(data[0]);

	if (m_Blocks[psm].bpp == 0)
	{
		// kh and others
		return;
	}

	vb[ctxt].uNextTex0Data[0] = data[0];
	vb[ctxt].uNextTex0Data[1] = data[1];
	vb[ctxt].bNeedTexCheck = 1;

	// don't update unless necessary

	if (PSMT_ISCLUT(psm))
	{
		if (CheckChangeInClut(data[1], psm))
		{
			// loading clut, so flush whole texture
			vb[ctxt].FlushTexData();
		}

		// check if csa is the same!! (ffx bisaid island, grass)
		else if ((data[1] & CPSM_CSA_BITMASK) != (vb[ctxt].uCurTex0Data[1] & CPSM_CSA_BITMASK))
		{
			Flush(ctxt); // flush any previous entries
		}
	}
}

template <u32 ctxt>
void __gifCall GIFRegHandlerCLAMP(const u32* data)
{
	FUNCLOG
	
	if (!NoHighlights(ctxt)) return;
	
	clampInfo& clamp = vb[ctxt].clamp;
	GIFRegCLAMP* r = (GIFRegCLAMP*)(data);

	if ((s_uClampData[ctxt] != data[0]) || (((clamp.minv >> 8) | (clamp.maxv << 2)) != (data[1]&0x0fff)))
	{
		Flush(ctxt);
		s_uClampData[ctxt] = data[0];

		clamp.wms  = r->WMS;
		clamp.wmt  = r->WMT;
		clamp.minu = r->MINU;
		clamp.maxu = r->MAXU;
		clamp.minv = r->MINV;
		clamp.maxv = r->MAXV;

		vb[ctxt].bTexConstsSync = false;
	}
}

void __gifCall GIFRegHandlerFOG(const u32* data)
{
	FUNCLOG
	//gs.gsvertex[gs.primIndex].f = (data[1] >> 24);	// shift to upper bits
	GIFRegFOG* r = (GIFRegFOG*)(data);
	gs.vertexregs.f = r->F;
	if (gs.vertexregs.f != 0) REG_LOG("GIFPackedRegHandlerFOG == %d", gs.vertexregs.f);
	
}

void __gifCall GIFRegHandlerXYZF3(const u32* data)
{
	FUNCLOG
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.add_vertex(r->X, r->Y,r->Z, r->F);

    ZZKick->KickVertex(true);
}

void __gifCall GIFRegHandlerXYZ3(const u32* data)
{
	FUNCLOG
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.add_vertex(r->X, r->Y,r->Z);

    ZZKick->KickVertex(true);
}

void __gifCall GIFRegHandlerNOP(const u32* data)
{
	FUNCLOG
}

template <u32 ctxt>
void __gifCall GIFRegHandlerTEX1(const u32* data)
{
	FUNCLOG
	
	if (!NoHighlights(ctxt)) return;
	
	GIFRegTEX1* r = (GIFRegTEX1*)(data);
	tex1Info& tex1 = vb[ctxt].tex1;

	if (conf.bilinear == 1 && (tex1.mmag != ((data[0] >>  5) & 0x1) || tex1.mmin != ((data[0] >>  6) & 0x7)))
	{
		Flush(ctxt);
		vb[ctxt].bVarsTexSync = false;
	}

	tex1.lcm  = r->LCM;

	tex1.mxl  = r->MXL;
	tex1.mmag = r->MMAG;
	tex1.mmin = r->MMIN;
	tex1.mtba = r->MTBA;
	tex1.l	= r->L;
	tex1.k	= r->K;
	
#ifdef SPAM_UNUSED_REGISTERS
	REG_LOG("Lcm = %d, l = %d, k = %d", tex1.lcm, tex1.l, tex1.k);
	if (tex1.mxl != 0) REG_LOG("MIPMAP level set to %d, which is unsupported.");
#endif
}

template <u32 ctxt>
void __gifCall GIFRegHandlerTEX2(const u32* data)
{
	FUNCLOG
	
	tex0Info& tex0 = vb[ctxt].tex0;

	vb[ctxt].FlushTexData();

	u32 psm = ZZOglGet_psm_TexBitsFix(data[0]);

	u32* s_uTex0Data = vb[ctxt].uCurTex0Data;

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
				texClutWrite(ctxt);
				// invalidate to make sure target didn't change!
				vb[ctxt].bVarsTexSync = false;
			}

			return;
		}
	}

	Flush(ctxt);

	vb[ctxt].bVarsTexSync = false;
	vb[ctxt].bTexConstsSync = false;

	s_uTex0Data[0] = (s_uTex0Data[0] & ~0x03f00000) | (psm << 20);
	s_uTex0Data[1] = (s_uTex0Data[1] & 0x1f) | (data[1] & ~0x1f);

	tex0.psm = ZZOglGet_psm_TexBitsFix(data[0]);

	if (PSMT_ISCLUT(tex0.psm)) CluttingForFlushedTex(&tex0, data[1], ctxt);
}

template <u32 ctxt>
void __gifCall GIFRegHandlerXYOFFSET(const u32* data)
{
	FUNCLOG
	GIFRegXYOFFSET* r = (GIFRegXYOFFSET*)(data);
	vb[ctxt].offset.x = r->OFX;
	vb[ctxt].offset.y = r->OFY;

//  if( !conf.interlace ) {
//	  vb[1].offset.x &= ~15;
//	  vb[1].offset.y &= ~15;
//  }
}

void __gifCall GIFRegHandlerPRMODECONT(const u32* data)
{
	FUNCLOG
	gs.prac = data[0] & 0x1;
	prim = &gs._prim[gs.prac];

	Prim();
}

void __gifCall GIFRegHandlerPRMODE(const u32* data)
{
	FUNCLOG
	gs._prim[0]._val = (data[0] >> 3) & 0xff;

	if (gs.prac == 0) Prim();
}

void __gifCall GIFRegHandlerTEXCLUT(const u32* data)
{
	FUNCLOG
	// Affects background coloration of initial Mana Khemia dialog.
	GIFRegTEXCLUT* r = (GIFRegTEXCLUT*)(data);

	vb[0].FlushTexData();
	vb[1].FlushTexData();

	gs.clut.cbw = r->CBW << 6;
	gs.clut.cou = r->COU << 4;
	gs.clut.cov = r->COV;
}

void __gifCall GIFRegHandlerSCANMSK(const u32* data)
{
	FUNCLOG
	GIFRegSCANMSK* r = (GIFRegSCANMSK*)(data);
//  FlushBoth();
//  ResolveC(&vb[0]);
//  ResolveZ(&vb[0]);

	gs.smask = r->MSK;
	REG_LOG("Scanmsk == %d", gs.smask);
}

template <u32 ctxt>
void __gifCall GIFRegHandlerMIPTBP1(const u32* data)
{
	FUNCLOG
	GIFRegMIPTBP1* r = (GIFRegMIPTBP1*)(data);
	
	miptbpInfo& miptbp0 = vb[ctxt].miptbp0;
	miptbp0.tbp[0] = r->TBP1;
	miptbp0.tbw[0] = r->TBW1;
	miptbp0.tbp[1] = r->TBP2;
	miptbp0.tbw[1] = r->TBW2;
	miptbp0.tbp[2] = r->TBP3;
	miptbp0.tbw[2] = r->TBW3;
	
#ifdef SPAM_UNUSED_REGISTERS
	if ((miptbp0.tbp[0] != 0) || (miptbp0.tbp[1] != 0) || (miptbp0.tbp[2] != 0))
	{
		REG_LOG("MIPTBP1: 0:%d(%d) 1:%d(%d) 2:%d(%d).", \
						miptbp0.tbp[0], miptbp0.tbw[0], miptbp0.tbp[1], miptbp0.tbw[1], miptbp0.tbp[2], miptbp0.tbw[2]);
	}
#endif
}

template <u32 ctxt>
void __gifCall GIFRegHandlerMIPTBP2(const u32* data)
{
	FUNCLOG
	GIFRegMIPTBP2* r = (GIFRegMIPTBP2*)(data);
	
	miptbpInfo& miptbp1 = vb[ctxt].miptbp1;
	miptbp1.tbp[0] = r->TBP4;
	miptbp1.tbw[0] = r->TBW4;
	miptbp1.tbp[1] = r->TBP5;
	miptbp1.tbw[1] = r->TBW5;
	miptbp1.tbp[2] = r->TBP6;
	miptbp1.tbw[2] = r->TBW6;
	
#ifdef SPAM_UNUSED_REGISTERS
	if ((miptbp1.tbp[0] != 0) || (miptbp1.tbp[1] != 0) || (miptbp1.tbp[2] != 0))
	{
		REG_LOG("MIPTBP2: 0:%d(%d) 1:%d(%d) 2:%d(%d).", \
					miptbp1.tbp[0], miptbp1.tbw[0], miptbp1.tbp[1], miptbp1.tbw[1], miptbp1.tbp[2], miptbp1.tbw[2]);
	}
#endif
}

void __gifCall GIFRegHandlerTEXA(const u32* data)
{
	FUNCLOG
	texaInfo newinfo;
	newinfo.aem = (data[0] >> 15) & 0x1;
	newinfo.ta[0] = data[0] & 0xff;
	newinfo.ta[1] = data[1] & 0xff;

	if (*(u32*)&newinfo != *(u32*)&gs.texa)
	{
		FlushBoth();
		
		*(u32*)&gs.texa = *(u32*) & newinfo;
		
		gs.texa.fta[0] = newinfo.ta[0] / 255.0f;
		gs.texa.fta[1] = newinfo.ta[1] / 255.0f;

		vb[0].bTexConstsSync = false;
		vb[1].bTexConstsSync = false;
	}
}

void __gifCall GIFRegHandlerFOGCOL(const u32* data)
{
	FUNCLOG
	GIFRegFOGCOL* r = (GIFRegFOGCOL*)(data);
	SetFogColor(r);
	gs.fogcol = r->ai32[0];
}

void __gifCall GIFRegHandlerTEXFLUSH(const u32* data)
{
	FUNCLOG
	SetTexFlush();
}

template <u32 ctxt>
void __gifCall GIFRegHandlerSCISSOR(const u32* data)
{
	FUNCLOG
	GIFRegSCISSOR* r = (GIFRegSCISSOR*)(data);
	
	Rect2& scissor = vb[ctxt].scissor;
	Rect2 newscissor;

	newscissor.x0 = r->SCAX0 << 3;
	newscissor.x1 = r->SCAX1 << 3;
	newscissor.y0 = r->SCAY0 << 3;
	newscissor.y1 = r->SCAY1 << 3;

	if (newscissor.x1 != scissor.x1 || newscissor.y1 != scissor.y1 ||
			newscissor.x0 != scissor.x0 || newscissor.y0 != scissor.y0)
	{
		Flush(ctxt);
		scissor = newscissor;

		// flush everything
		vb[ctxt].bNeedFrameCheck = 1;
	}
}

template <u32 ctxt>
void __gifCall GIFRegHandlerALPHA(const u32* data)
{
	FUNCLOG
	alphaInfo newalpha;
	newalpha.abcd = *(u8*)data;
	newalpha.fix = *(u8*)(data + 1);

	if (*(u16*)&newalpha != *(u16*)&vb[ctxt].alpha)
	{
		Flush(ctxt);

		if (newalpha.a == 3) newalpha.a = 0;
		if (newalpha.b == 3) newalpha.b = 0;
		if (newalpha.c == 3) newalpha.c = 0;
		if (newalpha.d == 3) newalpha.d = 0;

		*(u16*)&vb[ctxt].alpha = *(u16*) & newalpha;
	}
}

void __gifCall GIFRegHandlerDIMX(const u32* data)
{
	FUNCLOG
	GIFRegDIMX* r = (GIFRegDIMX*)(data);
	
	gs.dimx.i64 = r->i64;
}

void __gifCall GIFRegHandlerDTHE(const u32* data)
{
	FUNCLOG
	GIFRegDTHE* r = (GIFRegDTHE*)(data);
	
	gs.dthe = r->DTHE;
	if (gs.dthe != 0) REG_LOG("Dithering set. (but not implemented.)");
}

void __gifCall GIFRegHandlerCOLCLAMP(const u32* data)
{
	FUNCLOG
	GIFRegCOLCLAMP* r = (GIFRegCOLCLAMP*)(data);
	
	gs.colclamp = r->CLAMP;
	
	if (gs.colclamp == 0) 
		REG_LOG("COLCLAMP == MASK");
	else
		REG_LOG("COLCLAMP == CLAMP");
}

template <u32 ctxt>
void __gifCall GIFRegHandlerTEST(const u32* data)
{
	FUNCLOG
	
	pixTest* test = &vb[ctxt].test;

	if ((*(u32*)test & 0x0007ffff) == (data[0] & 0x0007ffff)) return;

	Flush(ctxt);

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

void __gifCall GIFRegHandlerPABE(const u32* data)
{
	FUNCLOG
	GIFRegPABE* r = (GIFRegPABE*)(data);
	//SetAlphaChanged(0, GPUREG_PABE);
	//SetAlphaChanged(1, GPUREG_PABE);
	FlushBoth();

	gs.pabe = r->PABE;
}

template <u32 ctxt>
void __gifCall GIFRegHandlerFBA(const u32* data)
{
	FUNCLOG
	GIFRegFBA* r = (GIFRegFBA*)(data);
	
	FlushBoth();
	
	vb[ctxt].fba.fba = r->FBA;
}

template <u32 ctxt>
void __gifCall GIFRegHandlerFRAME(const u32* data)
{
	FUNCLOG
	
	frameInfo& gsfb = vb[ctxt].gsfb;

	if ((gsfb.fbp == ZZOglGet_fbp_FrameBitsMult(data[0])) &&
			(gsfb.fbw == ZZOglGet_fbw_FrameBitsMult(data[0])) &&
			(gsfb.psm == ZZOglGet_psm_FrameBits(data[0])) &&
			(gsfb.fbm == ZZOglGet_fbm_FrameBits(data[0])))
	{
		return;
	}

	FlushBoth();

	gsfb.fbp = ZZOglGet_fbp_FrameBitsMult(data[0]);
	gsfb.fbw = ZZOglGet_fbw_FrameBitsMult(data[0]);
	gsfb.psm = ZZOglGet_psm_FrameBits(data[0]);
	gsfb.fbm = ZZOglGet_fbm_FrameBitsFix(data[0], data[1]);
	gsfb.fbh = ZZOglGet_fbh_FrameBitsCalc(data[0]);
//	gsfb.fbhCalc = gsfb.fbh;

	vb[ctxt].bNeedFrameCheck = 1;
}

template <u32 ctxt>
void __gifCall GIFRegHandlerZBUF(const u32* data)
{
	FUNCLOG
	zbufInfo& zbuf = vb[ctxt].zbuf;

	int psm = (0x30 | ((data[0] >> 24) & 0xf));

	if (zbuf.zbp == (data[0] & 0x1ff) * 32 &&
			zbuf.psm == psm &&
			zbuf.zmsk == (data[1] & 0x1))
	{
		return;
	}

	// error detection
	if (m_Blocks[psm].bpp == 0) return;

	FlushBoth();

	zbuf.zbp = (data[0] & 0x1ff) * 32;
	zbuf.psm = 0x30 | ((data[0] >> 24) & 0xf);
	zbuf.zmsk = data[1] & 0x1;

	vb[ctxt].bNeedZCheck = 1;
	vb[ctxt].zprimmask = 0xffffffff;

	if (zbuf.psm > 0x31) vb[ctxt].zprimmask = 0xffff;
}

void __gifCall GIFRegHandlerBITBLTBUF(const u32* data)
{
	FUNCLOG
	GIFRegBITBLTBUF* r = (GIFRegBITBLTBUF*)(data);
	
	gs.srcbufnew.bp  = r->SBP;
	gs.srcbufnew.bw  = r->SBW << 6;
	gs.srcbufnew.psm = r->SPSM;
	gs.dstbufnew.bp  = r->DBP;
	gs.dstbufnew.bw  = r->DBW << 6;
	gs.dstbufnew.psm = r->DPSM;

	if (gs.dstbufnew.bw == 0) gs.dstbufnew.bw = 64;
}

void __gifCall GIFRegHandlerTRXPOS(const u32* data)
{
	FUNCLOG
	GIFRegTRXPOS* r = (GIFRegTRXPOS*)(data);
	
	gs.trxposnew.sx  = r->SSAX;
	gs.trxposnew.sy  = r->SSAY;
	gs.trxposnew.dx  = r->DSAX;
	gs.trxposnew.dy  = r->DSAY;
	gs.trxposnew.dirx = r->DIRX;
	gs.trxposnew.diry = r->DIRY;
}

void __gifCall GIFRegHandlerTRXREG(const u32* data)
{
	FUNCLOG
	GIFRegTRXREG* r = (GIFRegTRXREG*)(data);
	gs.imageWtemp = r->RRW;
	gs.imageHtemp = r->RRH;
}

void __gifCall GIFRegHandlerTRXDIR(const u32* data)
{
	FUNCLOG
	// terminate any previous transfers

	switch (gs.imageTransfer)
	{

		case 0: // host->loc
			TerminateHostLocal();
			break;

		case 1: // loc->host
			TerminateLocalHost();
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
				InitTransferHostLocal();
				break;

			case 1: // loc->host
				InitTransferLocalHost();
				break;

			case 2:
				TransferLocalLocal();
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

void __gifCall GIFRegHandlerHWREG(const u32* data)
{
	FUNCLOG

	if (gs.imageTransfer == 0)
	{
		TransferHostLocal(data, 2);
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

void __gifCall GIFRegHandlerSIGNAL(const u32* data)
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

void __gifCall GIFRegHandlerFINISH(const u32* data)
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
