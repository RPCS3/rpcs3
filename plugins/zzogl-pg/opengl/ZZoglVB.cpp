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

// Zerogs:VB implementation.
// VB stands for Visual Buffer, as I think

//------------------- Includes
#include "zerogs.h"
#include "targets.h"
#include "GS.h"
#include "Mem.h"

// ----------------- Defines
#define MINMAX_SHIFT	3

//------------------ Constants

// ----------------- Global Variables
int maxmin = 608;
// ----------------- Code

// Constructor. Set width and height to 1
VB::VB()
{
	memset(this, 0, sizeof(VB));
	tex0.tw = 1;
	tex0.th = 1;
}

// Destructor
VB::~VB()
{
	Destroy();
}

void VB::Destroy()
{
	_aligned_free(pBufferData);
	pBufferData = NULL;
	nNumVertices = 0;

	prndr = NULL;
	pdepth = NULL;
}

int ConstraintReason;

// Return number of 64-pixels block, that guaranted could be hold in memory
// from gsfb.fbp and tbp (textrure pase), zbuf.zbp (Z-buffer), frame.fbp
// (previous frame).
inline int VB::FindMinimalMemoryConstrain(int tbp, int maxpos)
{
	int MinConstraint = maxpos;

	// make sure texture is far away from tbp
	{
		int Constraint = tbp - gsfb.fbp;

		if ((0 < Constraint) && (Constraint < MinConstraint))
		{
			MinConstraint = Constraint;
			ConstraintReason = 1;
		}
	}

	// offroad uses 0x80 fbp which messes up targets
	// special case when double buffering (hamsterball)
	// Suikoden 3 require e00 have this issue too. P3 - 0x1000.

	if (prndr != NULL)
	{
		int Constraint = frame.fbp - gsfb.fbp;

		if ((0x0 < Constraint) && (Constraint < MinConstraint))
		{
			MinConstraint = Constraint;
			ConstraintReason = 2;
		}
	}

	// old caching method
	// zmsk necessary for KH movie
	if (!zbuf.zmsk)
	{
		int Constraint = zbuf.zbp - gsfb.fbp;

		if ((0 < Constraint) && (Constraint < MinConstraint))
		{
			MinConstraint = Constraint;
			ConstraintReason = 3;
		}
	}

	// In 16Bit mode in one Word frame stored 2 pixels
	if (PSMT_ISHALF(gsfb.psm)) MinConstraint *= 2;

	return MinConstraint ;
}

// Return number of 64 pizel words that could be placed in Z-Buffer
// If no Z-buffer present return old constraint
inline int VB::FindZbufferMemoryConstrain(int tbp, int maxpos)
{
	int MinConstraint = maxpos;

	// Check tbp / zbuffer constraint
	if (!zbuf.zmsk)
	{
		int Constraint = (tbp - zbuf.zbp) * (PSMT_ISHALF(zbuf.psm) ? 2 : 1);

		if ((0 < Constraint) && (Constraint < MinConstraint))
		{
			MinConstraint = Constraint;
			ConstraintReason = 4;
		}
	}

	return MinConstraint;
}

// Return heights limiter from scissor...
inline int GetScissorY(int y)
{
	int fbh = (y >> MINMAX_SHIFT) + 1;

	if (fbh > 2 && (fbh & 1)) fbh -= 1;

	return fbh;
}

//There is several reasons to limit a height of frame: maximum buffer size, calculated size
//from fbw and fbh and scissoring.
inline int VB::FindMinimalHeightConstrain(int maxpos)
{
	int MinConstraint = maxpos;

	if (maxmin < MinConstraint)
	{
		MinConstraint = maxmin;
		ConstraintReason = 5;
	}

	if (gsfb.fbh < MinConstraint)
	{
		MinConstraint = gsfb.fbh;
		ConstraintReason = 6;
	}

	int ScissorConstraint = GetScissorY(scissor.y1) ;

	if (ScissorConstraint < MinConstraint)
	{
		MinConstraint = ScissorConstraint;
		ConstraintReason = 7;
	}

	return MinConstraint;
}

// 32 bit frames have additional constraints to frame
// maxpos was maximum length of frame at normal constraints
inline void VB::CheckFrame32bitRes(int maxpos)
{
	int fbh = frame.fbh;

	if (frame.fbh >= 512)
	{
		// neopets hack
		maxmin = min(maxmin, frame.fbh);
		frame.fbh = maxmin;
		ConstraintReason = 8;
	}

	// ffxii hack to stop resolving
	if (frame.fbp >= 0x3000 && fbh >= 0x1a0)
	{
		int endfbp = frame.fbp + frame.fbw * fbh / (PSMT_ISHALF(gsfb.psm) ? 128 : 64);

		// see if there is a previous render target in the way, reduce

		for (CRenderTargetMngr::MAPTARGETS::iterator itnew = s_RTs.mapTargets.begin(); itnew != s_RTs.mapTargets.end(); ++itnew)
		{
			if (itnew->second->fbp > frame.fbp && endfbp > itnew->second->fbp)
			{
				endfbp = itnew->second->fbp;
			}
		}

		frame.fbh = (endfbp - frame.fbp) * (PSMT_ISHALF(gsfb.psm) ? 128 :  64) / frame.fbw;

		if (frame.fbh < fbh) ConstraintReason = 9;
	}

}

// This is the main code for frame resizing.
// It checks for several reasons to resize and resizes if it needs to.
// 4Mb memory in 64 bit (4 bytes) words.
// |------------------------|---------------------|----------|----------|---------------------|
// 0                     gsfb.fbp               zbuff.zpb   tbp    frame.fbp              2^20/64
inline int VB::CheckFrameAddConstraints(int tbp)
{
	if (gsfb.fbw <= 0)
	{
		ERROR_LOG_SPAM("render target null, no constraints. Ignoring\n");
		return -1;
	}

	// Memory region after fbp
	int maxmemorypos = 0x4000 - gsfb.fbp;

	ConstraintReason = 0;

	maxmemorypos = FindMinimalMemoryConstrain(tbp, maxmemorypos);
	maxmemorypos = FindZbufferMemoryConstrain(tbp, maxmemorypos);

	int maxpos = 64 * maxmemorypos ;

	maxpos /= gsfb.fbw;

	//? atelier iris crashes without it
	if (maxpos > 256) maxpos &= ~0x1f;

#ifdef DEVBUILD
	int noscissorpos = maxpos;
	int ConstrainR1 = ConstraintReason;
#endif

	maxpos = FindMinimalHeightConstrain(maxpos);

	frame = gsfb;
	frame.fbh = maxpos;

	if (!PSMT_ISHALF(frame.psm) || !(conf.settings().full_16_bit_res)) CheckFrame32bitRes(maxpos);

#ifdef DEVBUILD
	if (frame.fbh == 0xe2)
		ZZLog::Debug_Log("Const: %x %x %d| %x %d %x %x", frame.fbh, frame.fbw, ConstraintReason, noscissorpos, ConstrainR1, tbp, frame.fbp);
#endif

// 	Fixme: Reserved psm for framebuffers
//	gsfb.psm &= 0xf; // shadow tower

	return 0;
}

// Check if after resizing new depth target is needed to be used.
// it returns 2 if a new depth target is used. 
inline int VB::CheckFrameResolveDepth(int tbp)
{
	int result = 0;
	CDepthTarget* pprevdepth = pdepth;
	pdepth = NULL;

	// just z changed
	frameInfo f = CreateFrame(zbuf.zbp, prndr->fbw, prndr->fbh, zbuf.psm, (zbuf.psm == 0x31) ? 0xff000000 : 0);

	CDepthTarget* pnewdepth = (CDepthTarget*)s_DepthRTs.GetTarg(f, CRenderTargetMngr::TO_DepthBuffer | CRenderTargetMngr::TO_StrictHeight |
							  (zbuf.zmsk ? CRenderTargetMngr::TO_Virtual : 0), get_maxheight(zbuf.zbp, gsfb.fbw, 0));

	assert(pnewdepth != NULL && prndr != NULL);
	if (pnewdepth->fbh != prndr->fbh) ZZLog::Debug_Log("pnewdepth->fbh(0x%x) != prndr->fbh(0x%x)", pnewdepth->fbh, prndr->fbh);
	//assert(pnewdepth->fbh == prndr->fbh);

	if ((pprevdepth != pnewdepth) || (pprevdepth != NULL && (pprevdepth->status & CRenderTarget::TS_NeedUpdate)))
		result = 2;

	pdepth = pnewdepth;

	return result;
}

// Check if after resizing, a new render target is needed to be used. Also perform deptarget check.
// Returns 1 if only 1 render target is changed and 3 -- if both.
inline int VB::CheckFrameResolveRender(int tbp)
{
	int result = 0;

	CRenderTarget* pprevrndr = prndr;
	prndr = NULL;
	CDepthTarget* pprevdepth = pdepth;
	pdepth = NULL;
	// Set renderes to NULL to prevent Flushing.

	CRenderTarget* pnewtarg = s_RTs.GetTarg(frame, 0, maxmin);
	assert(pnewtarg != NULL);

	// pnewtarg->fbh >= 0x1c0 needed for ffx

	if ((pnewtarg->fbh >= 0x1c0) && pnewtarg->fbh > frame.fbh && zbuf.zbp < tbp && !zbuf.zmsk)
	{
		// check if zbuf is in the way of the texture (suikoden5)
		int maxallowedfbh = (tbp - zbuf.zbp) * (PSMT_ISHALF(zbuf.psm) ? 128 : 64) / gsfb.fbw;

		if (PSMT_ISHALF(gsfb.psm)) maxallowedfbh *= 2;

		if (pnewtarg->fbh > maxallowedfbh + 32)   // +32 needed for ffx2
		{
			// destroy and recreate
			s_RTs.DestroyAllTargs(0, 0x100, pnewtarg->fbw);
			pnewtarg = s_RTs.GetTarg(frame, 0, maxmin);
			assert(pnewtarg != NULL);
		}
	}

	ZZLog::Prim_Log("frame_%d: fbp=0x%x fbw=%d fbh=%d(%d) psm=0x%x fbm=0x%x\n", ictx, gsfb.fbp, gsfb.fbw, gsfb.fbh, pnewtarg->fbh, gsfb.psm, gsfb.fbm);

	if ((pprevrndr != pnewtarg) || (pprevrndr != NULL && (pprevrndr->status & CRenderTarget::TS_NeedUpdate)))
		result = 1;

	prndr = pnewtarg;

	pdepth = pprevdepth;

	result |= CheckFrameResolveDepth(tbp);

	return result;
}

// After frame resetting, it is possible that 16 to 32 or 32 to 16 (color bits) conversion should be made.
inline void VB::CheckFrame16vs32Conversion()
{
	if (prndr->status & CRenderTarget::TS_NeedConvert32)
	{
		if (pdepth->pdepth != 0) pdepth->SetDepthStencilSurface();

		prndr->fbh *= 2;
		prndr->ConvertTo32();
		prndr->status &= ~CRenderTarget::TS_NeedConvert32;
	}
	else if (prndr->status & CRenderTarget::TS_NeedConvert16)
	{
		if (pdepth->pdepth != 0) pdepth->SetDepthStencilSurface();

		prndr->fbh /= 2;
		prndr->ConvertTo16();
		prndr->status &= ~CRenderTarget::TS_NeedConvert16;
	}
}

// A lot of times, the target is too big and overwrites the texture.
// If tbp != 0, use it to bound.
void VB::CheckFrame(int tbp)
{
	GL_REPORT_ERRORD();
	
	static int bChanged;

	if (bNeedZCheck)
	{
		ZZLog::Prim_Log("zbuf_%d: zbp=0x%x psm=0x%x, zmsk=%d\n", ictx, zbuf.zbp, zbuf.psm, zbuf.zmsk);
		//zbuf = *zb;
	}

	if (m_Blocks[gsfb.psm].bpp == 0)
	{
		ZZLog::Error_Log("CheckFrame invalid bpp %d.", gsfb.psm);
		return;
	}

	bChanged = 0;

	if (bNeedFrameCheck)
	{
		// important to set before calling GetTarg
		bNeedFrameCheck = 0;
		bNeedZCheck = 0;

		if (CheckFrameAddConstraints(tbp) == -1) return;

		if ((prndr != NULL) && (prndr->psm != gsfb.psm))
		{
			// behavior for dest alpha varies
			ResetAlphaVariables();
		}

		bChanged = CheckFrameResolveRender(tbp);

		CheckFrame16vs32Conversion();
	}
	else if (bNeedZCheck)
	{
		bNeedZCheck = 0;

		if (prndr != NULL && gsfb.fbw > 0) CheckFrameResolveDepth(tbp);
	}

	if (prndr != NULL) SetContextTarget(ictx);
	GL_REPORT_ERRORD();
}

// This is the case, most easy to perform, when nothing was changed
inline void VB::FlushTexUnchangedClutDontUpdate()
{
	if (ZZOglGet_cld_TexBits(uNextTex0Data[1]))
	{
		texClutWrite(ictx);
		// invalidate to make sure target didn't change!
		bVarsTexSync = false;
	}
}

// The second of easy branch. We does not change storage model, so we don't need to
// update anything except texture itself
inline void VB::FlushTexClutDontUpdate()
{
	if (!ZZOglClutStorageUnchanged(uCurTex0Data, uNextTex0Data)) Flush(ictx);

	// clut memory isn't going to be loaded so can ignore, but at least update CSA and CPSM!
	uCurTex0Data[1] = (uCurTex0Data[1] & CPSM_CSA_NOTMASK) | (uNextTex0Data[1] & CPSM_CSA_BITMASK);

	tex0.csa  = ZZOglGet_csa_TexBits(uNextTex0Data[1]);
	tex0.cpsm = ZZOglGet_cpsm_TexBits(uNextTex0Data[1]);

	texClutWrite(ictx);

	bVarsTexSync = false;
}


// Set texture variables after big change
inline void VB::FlushTexSetNewVars(u32 psm)
{
	tex0.tbp0 = ZZOglGet_tbp0_TexBits(uNextTex0Data[0]);
	tex0.tbw  = ZZOglGet_tbw_TexBitsMult(uNextTex0Data[0]);
	tex0.psm  = psm;
	tex0.tw   = ZZOglGet_tw_TexBitsExp(uNextTex0Data[0]);
	tex0.th   = ZZOglGet_th_TexBitsExp(uNextTex0Data[0], uNextTex0Data[1]);

	tex0.tcc  = ZZOglGet_tcc_TexBits(uNextTex0Data[1]);
	tex0.tfx  = ZZOglGet_tfx_TexBits(uNextTex0Data[1]);

	fiTexWidth[ictx] = (1 / 16.0f) / tex0.tw;
	fiTexHeight[ictx] = (1 / 16.0f) / tex0.th;
}

// Flush == draw on screen
// This function made VB state consistant before real Flush.
void VB::FlushTexData()
{
	GL_REPORT_ERRORD();
	
	//assert(bNeedTexCheck);
	if (bNeedTexCheck)
	{
		bNeedTexCheck = 0;

		u32 psm = ZZOglGet_psm_TexBitsFix(uNextTex0Data[0]);

		// don't update unless necessary

		if (ZZOglAllExceptClutIsSame(uCurTex0Data, uNextTex0Data))
		{
			// Don't need to do anything if there is no clutting and VB tex data was not changed
			if (!PSMT_ISCLUT(psm)) return;

			// have to write the CLUT again if only CLD was changed
			if (ZZOglClutMinusCLDunchanged(uCurTex0Data, uNextTex0Data))
			{
				FlushTexUnchangedClutDontUpdate();
				return;
			}

			// Cld bit is 0 means that clut buffer stay unchanged
			if (ZZOglGet_cld_TexBits(uNextTex0Data[1]) == 0)
			{
				FlushTexClutDontUpdate();
				return;
			}
		}

		// Made the full update
		Flush(ictx);

		bVarsTexSync = false;
		bTexConstsSync = false;

		uCurTex0Data[0] = uNextTex0Data[0];
		uCurTex0Data[1] = uNextTex0Data[1];

		FlushTexSetNewVars(psm);

		if (PSMT_ISCLUT(psm)) CluttingForFlushedTex(&tex0, uNextTex0Data[1], ictx) ;
		GL_REPORT_ERRORD();
	}
}
