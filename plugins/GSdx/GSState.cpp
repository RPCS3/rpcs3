/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSState.h"

//#define DISABLE_BITMASKING
//#define DISABLE_COLCLAMP
//#define DISABLE_DATE

GSState::GSState()
	: m_version(6)
	, m_mt(false)
	, m_irq(NULL)
	, m_path3hack(0)
	, m_regs(NULL)
	, m_q(1.0f)
	, m_vprim(1)
	, m_crc(0)
	, m_options(0)
	, m_frameskip(0)
{
	m_sssize = 0;

	m_sssize += sizeof(m_version);
	m_sssize += sizeof(m_env.PRIM);
	m_sssize += sizeof(m_env.PRMODE);
	m_sssize += sizeof(m_env.PRMODECONT);
	m_sssize += sizeof(m_env.TEXCLUT);
	m_sssize += sizeof(m_env.SCANMSK);
	m_sssize += sizeof(m_env.TEXA);
	m_sssize += sizeof(m_env.FOGCOL);
	m_sssize += sizeof(m_env.DIMX);
	m_sssize += sizeof(m_env.DTHE);
	m_sssize += sizeof(m_env.COLCLAMP);
	m_sssize += sizeof(m_env.PABE);
	m_sssize += sizeof(m_env.BITBLTBUF);
	m_sssize += sizeof(m_env.TRXDIR);
	m_sssize += sizeof(m_env.TRXPOS);
	m_sssize += sizeof(m_env.TRXREG);
	m_sssize += sizeof(m_env.TRXREG); // obsolete

	for(int i = 0; i < 2; i++)
	{
		m_sssize += sizeof(m_env.CTXT[i].XYOFFSET);
		m_sssize += sizeof(m_env.CTXT[i].TEX0);
		m_sssize += sizeof(m_env.CTXT[i].TEX1);
		m_sssize += sizeof(m_env.CTXT[i].TEX2);
		m_sssize += sizeof(m_env.CTXT[i].CLAMP);
		m_sssize += sizeof(m_env.CTXT[i].MIPTBP1);
		m_sssize += sizeof(m_env.CTXT[i].MIPTBP2);
		m_sssize += sizeof(m_env.CTXT[i].SCISSOR);
		m_sssize += sizeof(m_env.CTXT[i].ALPHA);
		m_sssize += sizeof(m_env.CTXT[i].TEST);
		m_sssize += sizeof(m_env.CTXT[i].FBA);
		m_sssize += sizeof(m_env.CTXT[i].FRAME);
		m_sssize += sizeof(m_env.CTXT[i].ZBUF);
	}

	m_sssize += sizeof(m_v.RGBAQ);
	m_sssize += sizeof(m_v.ST);
	m_sssize += sizeof(m_v.UV);
	m_sssize += sizeof(m_v.XYZ);
	m_sssize += sizeof(m_v.FOG);

	m_sssize += sizeof(m_tr.x);
	m_sssize += sizeof(m_tr.y);
	m_sssize += m_mem.m_vmsize;
	m_sssize += (sizeof(m_path[0].tag) + sizeof(m_path[0].reg)) * countof(m_path);
	m_sssize += sizeof(m_q);

	PRIM = &m_env.PRIM;
//	CSR->rREV = 0x20;
	m_env.PRMODECONT.AC = 1;

	Reset();

	ResetHandlers();
}

GSState::~GSState()
{
}

void GSState::SetRegsMem(uint8* basemem)
{
	ASSERT(basemem);

	m_regs = (GSPrivRegSet*)basemem;
}

void GSState::SetIrqCallback(void (*irq)())
{
	m_irq = irq;
}

void GSState::SetMultithreaded(bool mt)
{
	// Some older versions of PCSX2 didn't properly set the irq callback to NULL
	// in multithreaded mode (possibly because ZeroGS itself would assert in such
	// cases), and didn't bind them to a dummy callback either.  PCSX2 handles all
	// IRQs internally when multithreaded anyway -- so let's ignore them here:

	m_mt = mt;

	if(mt)
	{
		m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerNull;
		m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerNull;
		m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerNull;
	}
	else
	{
		m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerSIGNAL;
		m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerFINISH;
		m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerLABEL;
	}
}

void GSState::SetFrameSkip(int skip)
{
	if(m_frameskip == skip) return;

	m_frameskip = skip;

	if(skip)
	{
		m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerNOP;

		m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerNOP;
	}
	else
	{
		m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
		m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<0>;
		m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<1>;
		m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerXYZF3;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerXYZ3;

		m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
		m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
		m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
		m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
		m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
		m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
	}
}

void GSState::Reset()
{
	memset(&m_path[0], 0, sizeof(m_path[0]) * countof(m_path));
	memset(&m_v, 0, sizeof(m_v));

//	PRIM = &m_env.PRIM;
//	m_env.PRMODECONT.AC = 1;

	m_env.Reset();

	m_context = &m_env.CTXT[0];
}

void GSState::ResetHandlers()
{
	for(size_t i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
	{
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;
	}

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
	m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
	m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerXYZF3;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerXYZ3;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	for(size_t i = 0; i < countof(m_fpGIFRegHandlers); i++)
	{
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;
	}

	m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
	m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
	m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
	m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOG] = &GSState::GIFRegHandlerFOG;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
	m_fpGIFRegHandlers[GIF_A_D_REG_NOP] = &GSState::GIFRegHandlerNOP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GSState::GIFRegHandlerTEX1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GSState::GIFRegHandlerTEX1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GSState::GIFRegHandlerTEX2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GSState::GIFRegHandlerTEX2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GSState::GIFRegHandlerXYOFFSET<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GSState::GIFRegHandlerXYOFFSET<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GSState::GIFRegHandlerTEXCLUT;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GSState::GIFRegHandlerSCANMSK;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GSState::GIFRegHandlerMIPTBP1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GSState::GIFRegHandlerMIPTBP1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GSState::GIFRegHandlerMIPTBP2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GSState::GIFRegHandlerMIPTBP2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXA] = &GSState::GIFRegHandlerTEXA;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GSState::GIFRegHandlerFOGCOL;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GSState::GIFRegHandlerTEXFLUSH;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GSState::GIFRegHandlerSCISSOR<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GSState::GIFRegHandlerSCISSOR<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GSState::GIFRegHandlerALPHA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GSState::GIFRegHandlerALPHA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_DIMX] = &GSState::GIFRegHandlerDIMX;
	m_fpGIFRegHandlers[GIF_A_D_REG_DTHE] = &GSState::GIFRegHandlerDTHE;
	m_fpGIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GSState::GIFRegHandlerCOLCLAMP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_1] = &GSState::GIFRegHandlerTEST<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_2] = &GSState::GIFRegHandlerTEST<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PABE] = &GSState::GIFRegHandlerPABE;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_1] = &GSState::GIFRegHandlerFBA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_2] = &GSState::GIFRegHandlerFBA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GSState::GIFRegHandlerFRAME<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GSState::GIFRegHandlerFRAME<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GSState::GIFRegHandlerZBUF<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GSState::GIFRegHandlerZBUF<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GSState::GIFRegHandlerBITBLTBUF;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GSState::GIFRegHandlerTRXPOS;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXREG] = &GSState::GIFRegHandlerTRXREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GSState::GIFRegHandlerTRXDIR;
	m_fpGIFRegHandlers[GIF_A_D_REG_HWREG] = &GSState::GIFRegHandlerHWREG;

	SetMultithreaded(m_mt);
}

GSVector4i GSState::GetDisplayRect(int i)
{
	if(i < 0) i = IsEnabled(1) ? 1 : 0;

	GSVector4i r;

	r.left = m_regs->DISP[i].DISPLAY.DX / (m_regs->DISP[i].DISPLAY.MAGH + 1);
	r.top = m_regs->DISP[i].DISPLAY.DY / (m_regs->DISP[i].DISPLAY.MAGV + 1);
	r.right = r.left + (m_regs->DISP[i].DISPLAY.DW + 1) / (m_regs->DISP[i].DISPLAY.MAGH + 1);
	r.bottom = r.top + (m_regs->DISP[i].DISPLAY.DH + 1) / (m_regs->DISP[i].DISPLAY.MAGV + 1);

	return r;
}

GSVector4i GSState::GetFrameRect(int i)
{
	if(i < 0) i = IsEnabled(1) ? 1 : 0;

	GSVector4i r = GetDisplayRect(i);

	int w = r.width();
	int h = r.height();

	if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD && h > 1) h >>= 1;

	//Breaks Disgaea2 FMV borders
	r.left = m_regs->DISP[i].DISPFB.DBX;
	r.top = m_regs->DISP[i].DISPFB.DBY;
	r.right = r.left + w;
	r.bottom = r.top + h;
	//printf("%d %d %d %d %d %d\n",w,h,r.left,r.top,r.right,r.bottom);
	return r;
}

GSVector2i GSState::GetDeviceSize(int i)
{
	// TODO: other params of SMODE1 should affect the true device display size

	// TODO2: pal games at 60Hz

	if(i < 0) i = IsEnabled(1) ? 1 : 0;

	GSVector4i r = GetDisplayRect(i);

	int w = r.width();
	int h = r.height();

	/*if(h == 2 * 416 || h == 2 * 448 || h == 2 * 512)
	{
		h /= 2;
	}
	else
	{
		h = (m_regs->SMODE1.CMOD & 1) ? 512 : 448;
	}*/

	//Fixme : Just slightly better than the hack above
	if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD && h > 1){
		if (!IsEnabled(0) || !IsEnabled(1)){h >>= 1;}
	}

	return GSVector2i(w, h);

}

bool GSState::IsEnabled(int i)
{
	ASSERT(i >= 0 && i < 2);

	if(i == 0 && m_regs->PMODE.EN1)
	{
		return m_regs->DISP[0].DISPLAY.DW || m_regs->DISP[0].DISPLAY.DH;
	}
	else if(i == 1 && m_regs->PMODE.EN2)
	{
		return m_regs->DISP[1].DISPLAY.DW || m_regs->DISP[1].DISPLAY.DH;
	}

	return false;
}

int GSState::GetFPS()
{
	return ((m_regs->SMODE1.CMOD & 1) ? 50 : 60) >> (1 - m_regs->SMODE2.INT);
}

// GIFPackedRegHandler*

__forceinline void GSState::GIFPackedRegHandlerNull(const GIFPackedReg* r)
{
	// ASSERT(0);
}

__forceinline void GSState::GIFPackedRegHandlerRGBA(const GIFPackedReg* r)
{
	#if _M_SSE >= 0x301

	GSVector4i mask = GSVector4i::load(0x0c080400);
	GSVector4i v = GSVector4i::load<false>(r).shuffle8(mask);

	m_v.RGBAQ.u32[0] = (uint32)GSVector4i::store(v);

	#elif _M_SSE >= 0x200

	GSVector4i v = GSVector4i::load<false>(r) & GSVector4i::x000000ff();

	m_v.RGBAQ.u32[0] = v.rgba32();

	#else

	m_v.RGBAQ.R = r->RGBA.R;
	m_v.RGBAQ.G = r->RGBA.G;
	m_v.RGBAQ.B = r->RGBA.B;
	m_v.RGBAQ.A = r->RGBA.A;

	#endif

	m_v.RGBAQ.Q = m_q;
}

__forceinline void GSState::GIFPackedRegHandlerSTQ(const GIFPackedReg* r)
{
	#if defined(_M_AMD64)

	m_v.ST.u64 = r->u64[0];

	#elif _M_SSE >= 0x200

	GSVector4i v = GSVector4i::loadl(r);
	GSVector4i::storel(&m_v.ST.u64, v);

	#else

	m_v.ST.S = r->STQ.S;
	m_v.ST.T = r->STQ.T;

	#endif

	m_q = r->STQ.Q;
}

__forceinline void GSState::GIFPackedRegHandlerUV(const GIFPackedReg* r)
{
	#if _M_SSE >= 0x200

	GSVector4i v = GSVector4i::loadl(r) & GSVector4i::x00003fff();
	m_v.UV.u32[0] = (uint32)GSVector4i::store(v.ps32(v));

	#else

	m_v.UV.U = r->UV.U;
	m_v.UV.V = r->UV.V;

	#endif
}

__forceinline void GSState::GIFPackedRegHandlerXYZF2(const GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZF2.X;
	m_v.XYZ.Y = r->XYZF2.Y;
	m_v.XYZ.Z = r->XYZF2.Z;
	m_v.FOG.F = r->XYZF2.F;

	VertexKick(r->XYZF2.ADC);
}

__forceinline void GSState::GIFPackedRegHandlerXYZ2(const GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZ2.X;
	m_v.XYZ.Y = r->XYZ2.Y;
	m_v.XYZ.Z = r->XYZ2.Z;

	VertexKick(r->XYZ2.ADC);
}

__forceinline void GSState::GIFPackedRegHandlerFOG(const GIFPackedReg* r)
{
	m_v.FOG.F = r->FOG.F;
}

__forceinline void GSState::GIFPackedRegHandlerA_D(const GIFPackedReg* r)
{
	(this->*m_fpGIFRegHandlers[r->A_D.ADDR])(&r->r);
}

__forceinline void GSState::GIFPackedRegHandlerNOP(const GIFPackedReg* r)
{
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(const GIFReg* r)
{
	// ASSERT(0);
}

__forceinline void GSState::ApplyPRIM(const GIFRegPRIM& prim)
{
	// ASSERT(r->PRIM.PRIM < 7);

	if(GSUtil::GetPrimClass(m_env.PRIM.PRIM) == GSUtil::GetPrimClass(prim.PRIM))
	{
		if((m_env.PRIM.u32[0] ^ prim.u32[0]) & 0x7f8) // all fields except PRIM
		{
			Flush();
		}
	}
	else
	{
		Flush();
	}

	m_env.PRIM = (GSVector4i)prim;
	m_env.PRMODE._PRIM = prim.PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();

	ResetPrim();
}

void GSState::GIFRegHandlerPRIM(const GIFReg* r)
{
	ALIGN_STACK(32);

	ApplyPRIM(r->PRIM);
}

__forceinline void GSState::GIFRegHandlerRGBAQ(const GIFReg* r)
{
	m_v.RGBAQ = (GSVector4i)r->RGBAQ;
}

__forceinline void GSState::GIFRegHandlerST(const GIFReg* r)
{
	m_v.ST = (GSVector4i)r->ST;
}

__forceinline void GSState::GIFRegHandlerUV(const GIFReg* r)
{
	m_v.UV.u32[0] = r->UV.u32[0] & 0x3fff3fff;
}

void GSState::GIFRegHandlerXYZF2(const GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.u32[0] = r->XYZF.u32[0];
	m_v.XYZ.u32[1] = r->XYZF.u32[1] & 0x00ffffff;
	m_v.FOG.u32[1] = r->XYZF.u32[1] & 0xff000000;

	VertexKick(false);
}

void GSState::GIFRegHandlerXYZ2(const GIFReg* r)
{
	m_v.XYZ = (GSVector4i)r->XYZ;

	VertexKick(false);
}

void GSState::ApplyTEX0(int i, GIFRegTEX0& TEX0)
{
	// even if TEX0 did not change, a new palette may have been uploaded and will overwrite the currently queued for drawing

	bool wt = m_mem.m_clut.WriteTest(TEX0, m_env.TEXCLUT);

	if(wt || PRIM->CTXT == i && TEX0 != m_env.CTXT[i].TEX0)
	{
		Flush();
	}

	TEX0.CPSM &= 0xa; // 1010b

	if((TEX0.TBW & 1) && (TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT4))
	{
		TEX0.TBW &= ~1; // GS User 2.6
	}

	if((TEX0.u32[0] ^ m_env.CTXT[i].TEX0.u32[0]) & 0x3ffffff) // TBP0 TBW PSM
	{
		m_env.CTXT[i].offset.tex = m_mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);
	}

	m_env.CTXT[i].TEX0 = (GSVector4i)TEX0;

	if(wt)
	{
		m_mem.m_clut.Write(m_env.CTXT[i].TEX0, m_env.TEXCLUT);
	}
}

template<int i> void GSState::GIFRegHandlerTEX0(const GIFReg* r)
{
	GIFRegTEX0 TEX0 = r->TEX0;

	if(TEX0.TW > 10) TEX0.TW = 10;
	if(TEX0.TH > 10) TEX0.TH = 10;

	ApplyTEX0(i, TEX0);

	if(m_env.CTXT[i].TEX1.MTBA)
	{
		uint32 bpp = GSLocalMemory::m_psm[TEX0.PSM].bpp;

		uint32 tbp = TEX0.TBP0;
		uint32 tbw = TEX0.TBW;
		uint32 th = TEX0.TH;

		if(th >= 3)
		{
			tbp += (((tbw << 6) * (1 << th) * bpp >> 3) + 255) >> 8;
			tbw = std::max<uint32>(tbw >> 1, 1);
			th--;

			m_env.CTXT[i].MIPTBP1.TBP1 = tbp;
			m_env.CTXT[i].MIPTBP1.TBW1 = tbw;

			tbp += (((tbw << 6) * (1 << th) * bpp >> 3) + 255) >> 8;
			tbw = std::max<uint32>(tbw >> 1, 1);
			th--;

			m_env.CTXT[i].MIPTBP1.TBP2 = tbp;
			m_env.CTXT[i].MIPTBP1.TBW2 = tbw;

			tbp += (((tbw << 6) * (1 << th) * bpp >> 3) + 255) >> 8;
			tbw = std::max<uint32>(tbw >> 1, 1);
			th--;

			m_env.CTXT[i].MIPTBP1.TBP3 = tbp;
			m_env.CTXT[i].MIPTBP1.TBW3 = tbw;

			// NOTE: TEX1.MXL must not be automatically set to 3 here
		}
		else
		{
			ASSERT(0);
		}

		// printf("MTBA\n");
	}
}

template<int i> void GSState::GIFRegHandlerCLAMP(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->CLAMP != m_env.CTXT[i].CLAMP)
	{
		Flush();
	}

	m_env.CTXT[i].CLAMP = (GSVector4i)r->CLAMP;
}

void GSState::GIFRegHandlerFOG(const GIFReg* r)
{
	m_v.FOG = (GSVector4i)r->FOG;
}

void GSState::GIFRegHandlerXYZF3(const GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.u32[0] = r->XYZF.u32[0];
	m_v.XYZ.u32[1] = r->XYZF.u32[1] & 0x00ffffff;
	m_v.FOG.u32[1] = r->XYZF.u32[1] & 0xff000000;

	VertexKick(true);
}

void GSState::GIFRegHandlerXYZ3(const GIFReg* r)
{
	m_v.XYZ = (GSVector4i)r->XYZ;

	VertexKick(true);
}

void GSState::GIFRegHandlerNOP(const GIFReg* r)
{
}

template<int i> void GSState::GIFRegHandlerTEX1(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->TEX1 != m_env.CTXT[i].TEX1)
	{
		Flush();
	}

	m_env.CTXT[i].TEX1 = (GSVector4i)r->TEX1;
}

template<int i> void GSState::GIFRegHandlerTEX2(const GIFReg* r)
{
	// m_env.CTXT[i].TEX2 = r->TEX2; // not used

	// TEX2 is a masked write to TEX0, for performing CLUT swaps (palette swaps).
	// It only applies the following fields:
	//    CLD, CSA, CSM, CPSM, CBP, PSM.
	// It ignores these fields (uses existing values in the context):
	//    TFX, TCC, TH, TW, TBW, and TBP0

	uint64 mask = 0xFFFFFFE003F00000ull; // TEX2 bits
	GIFRegTEX0 TEX0;
	TEX0.u64 = (m_env.CTXT[i].TEX0.u64 & ~mask) | (r->u64 & mask);

	ApplyTEX0(i, TEX0);
}

template<int i> void GSState::GIFRegHandlerXYOFFSET(const GIFReg* r)
{
	GSVector4i o = (GSVector4i)r->XYOFFSET & GSVector4i::x0000ffff();

	if(!o.eq(m_env.CTXT[i].XYOFFSET))
	{
		Flush();
	}

	m_env.CTXT[i].XYOFFSET = o;

	m_env.CTXT[i].UpdateScissor();
}

void GSState::GIFRegHandlerPRMODECONT(const GIFReg* r)
{
	if(r->PRMODECONT != m_env.PRMODECONT)
	{
		Flush();
	}

	m_env.PRMODECONT.AC = r->PRMODECONT.AC;

	PRIM = m_env.PRMODECONT.AC ? &m_env.PRIM : (GIFRegPRIM*)&m_env.PRMODE;

	// if(PRIM->PRIM == 7) printf("Invalid PRMODECONT/PRIM\n");

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();
}

void GSState::GIFRegHandlerPRMODE(const GIFReg* r)
{
	if(!m_env.PRMODECONT.AC)
	{
		Flush();
	}

	uint32 _PRIM = m_env.PRMODE._PRIM;
	m_env.PRMODE = (GSVector4i)r->PRMODE;
	m_env.PRMODE._PRIM = _PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();
}

void GSState::GIFRegHandlerTEXCLUT(const GIFReg* r)
{
	if(r->TEXCLUT != m_env.TEXCLUT)
	{
		Flush();
	}

	m_env.TEXCLUT = (GSVector4i)r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(const GIFReg* r)
{
	if(r->SCANMSK != m_env.SCANMSK)
	{
		Flush();
	}

	m_env.SCANMSK = (GSVector4i)r->SCANMSK;
}

template<int i> void GSState::GIFRegHandlerMIPTBP1(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->MIPTBP1 != m_env.CTXT[i].MIPTBP1)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP1 = (GSVector4i)r->MIPTBP1;
}

template<int i> void GSState::GIFRegHandlerMIPTBP2(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->MIPTBP2 != m_env.CTXT[i].MIPTBP2)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP2 = (GSVector4i)r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(const GIFReg* r)
{
	if(r->TEXA != m_env.TEXA)
	{
		Flush();
	}

	m_env.TEXA = (GSVector4i)r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(const GIFReg* r)
{
	if(r->FOGCOL != m_env.FOGCOL)
	{
		Flush();
	}

	m_env.FOGCOL = (GSVector4i)r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(const GIFReg* r)
{
	// TRACE(_T("TEXFLUSH\n"));
}

template<int i> void GSState::GIFRegHandlerSCISSOR(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->SCISSOR != m_env.CTXT[i].SCISSOR)
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = (GSVector4i)r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();
}

template<int i> void GSState::GIFRegHandlerALPHA(const GIFReg* r)
{
	ASSERT(r->ALPHA.A != 3);
	ASSERT(r->ALPHA.B != 3);
	ASSERT(r->ALPHA.C != 3);
	ASSERT(r->ALPHA.D != 3);

	if(PRIM->CTXT == i && r->ALPHA != m_env.CTXT[i].ALPHA)
	{
		Flush();
	}

	m_env.CTXT[i].ALPHA = (GSVector4i)r->ALPHA;

	// A/B/C/D == 3? => 2

	m_env.CTXT[i].ALPHA.u32[0] = ((~m_env.CTXT[i].ALPHA.u32[0] >> 1) | 0xAA) & m_env.CTXT[i].ALPHA.u32[0];
}

void GSState::GIFRegHandlerDIMX(const GIFReg* r)
{
	bool update = false;

	if(r->DIMX != m_env.DIMX)
	{
		Flush();

		update = true;
	}

	m_env.DIMX = (GSVector4i)r->DIMX;

	if(update)
	{
		m_env.UpdateDIMX();
	}
}

void GSState::GIFRegHandlerDTHE(const GIFReg* r)
{
	if(r->DTHE != m_env.DTHE)
	{
		Flush();
	}

	m_env.DTHE = (GSVector4i)r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(const GIFReg* r)
{
	if(r->COLCLAMP != m_env.COLCLAMP)
	{
		Flush();
	}

	m_env.COLCLAMP = (GSVector4i)r->COLCLAMP;
#ifdef DISABLE_COLCLAMP
	m_env.COLCLAMP.CLAMP = 1;
#endif
}

template<int i> void GSState::GIFRegHandlerTEST(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->TEST != m_env.CTXT[i].TEST)
	{
		Flush();
	}

	m_env.CTXT[i].TEST = (GSVector4i)r->TEST;
#ifdef DISABLE_DATE
	m_env.CTXT[i].TEST.DATE = 0;
#endif
}

void GSState::GIFRegHandlerPABE(const GIFReg* r)
{
	if(r->PABE != m_env.PABE)
	{
		Flush();
	}

	m_env.PABE = (GSVector4i)r->PABE;
}

template<int i> void GSState::GIFRegHandlerFBA(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->FBA != m_env.CTXT[i].FBA)
	{
		Flush();
	}

	m_env.CTXT[i].FBA = (GSVector4i)r->FBA;
}

template<int i> void GSState::GIFRegHandlerFRAME(const GIFReg* r)
{
	if(PRIM->CTXT == i && r->FRAME != m_env.CTXT[i].FRAME)
	{
		Flush();
	}

	if((m_env.CTXT[i].FRAME.u32[0] ^ r->FRAME.u32[0]) & 0x3f3f01ff) // FBP FBW PSM
	{
		m_env.CTXT[i].offset.fb = m_mem.GetOffset(r->FRAME.Block(), r->FRAME.FBW, r->FRAME.PSM);
		m_env.CTXT[i].offset.zb = m_mem.GetOffset(m_env.CTXT[i].ZBUF.Block(), r->FRAME.FBW, m_env.CTXT[i].ZBUF.PSM);
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset4(r->FRAME, m_env.CTXT[i].ZBUF);
	}

	m_env.CTXT[i].FRAME = (GSVector4i)r->FRAME;
#ifdef DISABLE_BITMASKING
	m_env.CTXT[i].FRAME.FBMSK = GSVector4i::store(GSVector4i::load((int)m_env.CTXT[i].FRAME.FBMSK).eq8(GSVector4i::xffffffff()));
#endif
}

template<int i> void GSState::GIFRegHandlerZBUF(const GIFReg* r)
{
	GIFRegZBUF ZBUF = r->ZBUF;

	if(ZBUF.u32[0] == 0)
	{
		// during startup all regs are cleared to 0 (by the bios or something), so we mask z until this register becomes valid

		ZBUF.ZMSK = 1;
	}

	ZBUF.PSM |= 0x30;

	if(ZBUF.PSM != PSM_PSMZ32
	&& ZBUF.PSM != PSM_PSMZ24
	&& ZBUF.PSM != PSM_PSMZ16
	&& ZBUF.PSM != PSM_PSMZ16S)
	{
		ZBUF.PSM = PSM_PSMZ32;
	}

	if(PRIM->CTXT == i && ZBUF != m_env.CTXT[i].ZBUF)
	{
		Flush();
	}

	if((m_env.CTXT[i].ZBUF.u32[0] ^ ZBUF.u32[0]) & 0x3f0001ff) // ZBP PSM
	{
		m_env.CTXT[i].offset.zb = m_mem.GetOffset(ZBUF.Block(), m_env.CTXT[i].FRAME.FBW, ZBUF.PSM);
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset4(m_env.CTXT[i].FRAME, ZBUF);
	}

	m_env.CTXT[i].ZBUF = (GSVector4i)ZBUF;
}

void GSState::GIFRegHandlerBITBLTBUF(const GIFReg* r)
{
	if(r->BITBLTBUF != m_env.BITBLTBUF)
	{
		FlushWrite();
	}

	m_env.BITBLTBUF = (GSVector4i)r->BITBLTBUF;

	if((m_env.BITBLTBUF.SBW & 1) && (m_env.BITBLTBUF.SPSM == PSM_PSMT8 || m_env.BITBLTBUF.SPSM == PSM_PSMT4))
	{
		m_env.BITBLTBUF.SBW &= ~1;
	}

	if((m_env.BITBLTBUF.DBW & 1) && (m_env.BITBLTBUF.DPSM == PSM_PSMT8 || m_env.BITBLTBUF.DPSM == PSM_PSMT4))
	{
		m_env.BITBLTBUF.DBW &= ~1; // namcoXcapcom: 5, 11, refered to as 4, 10 in TEX0.TBW later
	}
}

void GSState::GIFRegHandlerTRXPOS(const GIFReg* r)
{
	if(r->TRXPOS != m_env.TRXPOS)
	{
		FlushWrite();
	}

	m_env.TRXPOS = (GSVector4i)r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(const GIFReg* r)
{
	if(r->TRXREG != m_env.TRXREG)
	{
		FlushWrite();
	}

	m_env.TRXREG = (GSVector4i)r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(const GIFReg* r)
{
	Flush();

	m_env.TRXDIR = (GSVector4i)r->TRXDIR;

	switch(m_env.TRXDIR.XDIR)
	{
	case 0: // host -> local
		m_tr.Init(m_env.TRXPOS.DSAX, m_env.TRXPOS.DSAY);
		break;
	case 1: // local -> host
		m_tr.Init(m_env.TRXPOS.SSAX, m_env.TRXPOS.SSAY);
		break;
	case 2: // local -> local
		Move();
		break;
	case 3:
		ASSERT(0);
		break;
	}
}

void GSState::GIFRegHandlerHWREG(const GIFReg* r)
{
	ASSERT(m_env.TRXDIR.XDIR == 0); // host => local

	Write((uint8*)r, 8); // haunting ground
}

void GSState::GIFRegHandlerSIGNAL(const GIFReg* r)
{
	m_regs->SIGLBLID.SIGID = (m_regs->SIGLBLID.SIGID & ~r->SIGNAL.IDMSK) | (r->SIGNAL.ID & r->SIGNAL.IDMSK);

	if(m_regs->CSR.wSIGNAL) m_regs->CSR.rSIGNAL = 1;
	if(!m_regs->IMR.SIGMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerFINISH(const GIFReg* r)
{
	if(m_regs->CSR.wFINISH) m_regs->CSR.rFINISH = 1;
	if(!m_regs->IMR.FINISHMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerLABEL(const GIFReg* r)
{
	m_regs->SIGLBLID.LBLID = (m_regs->SIGLBLID.LBLID & ~r->LABEL.IDMSK) | (r->LABEL.ID & r->LABEL.IDMSK);
}

//

void GSState::Flush()
{
	FlushWrite();

	FlushPrim();
}

void GSState::FlushWrite()
{
	int len = m_tr.end - m_tr.start;

	if(len <= 0) return;

	int y = m_tr.y;

	GSLocalMemory::writeImage wi = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wi;

	(m_mem.*wi)(m_tr.x, m_tr.y, &m_tr.buff[m_tr.start], len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

	m_tr.start += len;

	m_perfmon.Put(GSPerfMon::Swizzle, len);

	GSVector4i r;

	r.left = m_env.TRXPOS.DSAX;
	r.top = y;
	r.right = r.left + m_env.TRXREG.RRW;
	r.bottom = std::min<int>(r.top + m_env.TRXREG.RRH, m_tr.x == r.left ? m_tr.y : m_tr.y + 1);

	InvalidateVideoMem(m_env.BITBLTBUF, r);
/*
	static int n = 0;
	string s;
	s = format("c:\\temp1\\[%04d]_%05x_%d_%d_%d_%d_%d_%d.bmp",
		n++, (int)m_env.BITBLTBUF.DBP, (int)m_env.BITBLTBUF.DBW, (int)m_env.BITBLTBUF.DPSM,
		r.left, r.top, r.right, r.bottom);
	m_mem.SaveBMP(s, m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM, r.right, r.bottom);
*/
}

//

void GSState::Write(const uint8* mem, int len)
{
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM];

	// printf("Write len=%d DBP=%05x DBW=%d DPSM=%d DSAX=%d DSAY=%d RRW=%d RRH=%d\n", len, m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM, m_env.TRXPOS.DSAX, m_env.TRXPOS.DSAY, m_env.TRXREG.RRW, m_env.TRXREG.RRH);

	if(!m_tr.Update(w, h, psm.trbpp, len))
	{
		return;
	}

	if(PRIM->TME && (m_env.BITBLTBUF.DBP == m_context->TEX0.TBP0 || m_env.BITBLTBUF.DBP == m_context->TEX0.CBP)) // TODO: hmmmm
	{
		FlushPrim();
	}

	if(m_tr.end == 0 && len >= m_tr.total)
	{
		// received all data in one piece, no need to buffer it

		// printf("%d >= %d\n", len, m_tr.total);

		(m_mem.*psm.wi)(m_tr.x, m_tr.y, mem, m_tr.total, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

		m_tr.start = m_tr.end = m_tr.total;

		m_perfmon.Put(GSPerfMon::Swizzle, len);

		GSVector4i r;

		r.left = m_env.TRXPOS.DSAX;
		r.top = m_env.TRXPOS.DSAY;
		r.right = r.left + m_env.TRXREG.RRW;
		r.bottom = r.top + m_env.TRXREG.RRH;

		InvalidateVideoMem(m_env.BITBLTBUF, r);
	}
	else
	{
		// printf("%d += %d (%d)\n", m_tr.end, len, m_tr.total);

		memcpy(&m_tr.buff[m_tr.end], mem, len);

		m_tr.end += len;

		if(m_tr.end >= m_tr.total)
		{
			FlushWrite();
		}
	}

	m_mem.m_clut.Invalidate();
}

void GSState::Read(uint8* mem, int len)
{
	if(len <= 0) return;

	int sx = m_env.TRXPOS.SSAX;
	int sy = m_env.TRXPOS.SSAY;
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;

	// printf("Read len=%d SBP=%05x SBW=%d SPSM=%d SSAX=%d SSAY=%d RRW=%d RRH=%d\n", len, (int)m_env.BITBLTBUF.SBP, (int)m_env.BITBLTBUF.SBW, (int)m_env.BITBLTBUF.SPSM, sx, sy, w, h);

	if(!m_tr.Update(w, h, GSLocalMemory::m_psm[m_env.BITBLTBUF.SPSM].trbpp, len))
	{
		return;
	}

	if(m_tr.x == sx && m_tr.y == sy)
	{
		InvalidateLocalMem(m_env.BITBLTBUF, GSVector4i(sx, sy, sx + w, sy + h));
	}

	m_mem.ReadImageX(m_tr.x, m_tr.y, mem, len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);
}

void GSState::Move()
{
	// ffxii uses this to move the top/bottom of the scrolling menus offscreen and then blends them back over the text to create a shading effect
	// guitar hero copies the far end of the board to do a similar blend too

	int sx = m_env.TRXPOS.SSAX;
	int sy = m_env.TRXPOS.SSAY;
	int dx = m_env.TRXPOS.DSAX;
	int dy = m_env.TRXPOS.DSAY;
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;

	InvalidateLocalMem(m_env.BITBLTBUF, GSVector4i(sx, sy, sx + w, sy + h));
	InvalidateVideoMem(m_env.BITBLTBUF, GSVector4i(dx, dy, dx + w, dy + h));

	int xinc = 1;
	int yinc = 1;

	if(m_env.TRXPOS.DIRX) {sx += w - 1; dx += w - 1; xinc = -1;}
	if(m_env.TRXPOS.DIRY) {sy += h - 1; dy += h - 1; yinc = -1;}
/*
	printf("%05x %d %d => %05x %d %d (%d%d), %d %d %d %d %d %d\n",
		m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW, m_env.BITBLTBUF.SPSM,
		m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM,
		m_env.TRXPOS.DIRX, m_env.TRXPOS.DIRY,
		sx, sy, dx, dy, w, h);
*/
/*
	GSLocalMemory::readPixel rp = GSLocalMemory::m_psm[m_env.BITBLTBUF.SPSM].rp;
	GSLocalMemory::writePixel wp = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wp;

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_mem.*wp)(dx, dy, (m_mem.*rp)(sx, sy, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW), m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW);
*/

	const GSLocalMemory::psm_t& spsm = GSLocalMemory::m_psm[m_env.BITBLTBUF.SPSM];
	const GSLocalMemory::psm_t& dpsm = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM];

	// TODO: unroll inner loops (width has special size requirement, must be multiples of 1 << n, depending on the format)

	GSOffset* RESTRICT spo = m_mem.GetOffset(m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW, m_env.BITBLTBUF.SPSM);
	GSOffset* RESTRICT dpo = m_mem.GetOffset(m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM);

	if(spsm.trbpp == dpsm.trbpp && spsm.trbpp >= 16)
	{
		int* RESTRICT scol = &spo->pixel.col[0][sx];
		int* RESTRICT dcol = &dpo->pixel.col[0][dx];

		if(spsm.trbpp == 32)
		{
			if(xinc > 0)
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint32* RESTRICT s = &m_mem.m_vm32[spo->pixel.row[sy]];
					uint32* RESTRICT d = &m_mem.m_vm32[dpo->pixel.row[dy]];

					for(int x = 0; x < w; x++) d[dcol[x]] = s[scol[x]];
				}
			}
			else
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint32* RESTRICT s = &m_mem.m_vm32[spo->pixel.row[sy]];
					uint32* RESTRICT d = &m_mem.m_vm32[dpo->pixel.row[dy]];

					for(int x = 0; x > -w; x--) d[dcol[x]] = s[scol[x]];
				}
			}
		}
		else if(spsm.trbpp == 24)
		{
			if(xinc > 0)
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint32* RESTRICT s = &m_mem.m_vm32[spo->pixel.row[sy]];
					uint32* RESTRICT d = &m_mem.m_vm32[dpo->pixel.row[dy]];

					for(int x = 0; x < w; x++) d[dcol[x]] = (d[dcol[x]] & 0xff000000) | (s[scol[x]] & 0x00ffffff);
				}
			}
			else
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint32* RESTRICT s = &m_mem.m_vm32[spo->pixel.row[sy]];
					uint32* RESTRICT d = &m_mem.m_vm32[dpo->pixel.row[dy]];

					for(int x = 0; x > -w; x--) d[dcol[x]] = (d[dcol[x]] & 0xff000000) | (s[scol[x]] & 0x00ffffff);
				}
			}
		}
		else // if(spsm.trbpp == 16)
		{
			if(xinc > 0)
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint16* RESTRICT s = &m_mem.m_vm16[spo->pixel.row[sy]];
					uint16* RESTRICT d = &m_mem.m_vm16[dpo->pixel.row[dy]];

					for(int x = 0; x < w; x++) d[dcol[x]] = s[scol[x]];
				}
			}
			else
			{
				for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
				{
					uint16* RESTRICT s = &m_mem.m_vm16[spo->pixel.row[sy]];
					uint16* RESTRICT d = &m_mem.m_vm16[dpo->pixel.row[dy]];

					for(int x = 0; x > -w; x--) d[dcol[x]] = s[scol[x]];
				}
			}
		}
	}
	else if(m_env.BITBLTBUF.SPSM == PSM_PSMT8 && m_env.BITBLTBUF.DPSM == PSM_PSMT8)
	{
		if(xinc > 0)
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint8* RESTRICT s = &m_mem.m_vm8[spo->pixel.row[sy]];
				uint8* RESTRICT d = &m_mem.m_vm8[dpo->pixel.row[dy]];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x < w; x++) d[dcol[x]] = s[scol[x]];
			}
		}
		else
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint8* RESTRICT s = &m_mem.m_vm8[spo->pixel.row[sy]];
				uint8* RESTRICT d = &m_mem.m_vm8[dpo->pixel.row[dy]];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x > -w; x--) d[dcol[x]] = s[scol[x]];
			}
		}
	}
	else if(m_env.BITBLTBUF.SPSM == PSM_PSMT4 && m_env.BITBLTBUF.DPSM == PSM_PSMT4)
	{
		if(xinc > 0)
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint32 sbase = spo->pixel.row[sy];
				uint32 dbase = dpo->pixel.row[dy];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x < w; x++) m_mem.WritePixel4(dbase + dcol[x], m_mem.ReadPixel4(sbase + scol[x]));
			}
		}
		else
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint32 sbase = spo->pixel.row[sy];
				uint32 dbase = dpo->pixel.row[dy];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x > -w; x--) m_mem.WritePixel4(dbase + dcol[x], m_mem.ReadPixel4(sbase + scol[x]));
			}
		}
	}
	else
	{
		if(xinc > 0)
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint32 sbase = spo->pixel.row[sy];
				uint32 dbase = dpo->pixel.row[dy];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x < w; x++) (m_mem.*dpsm.wpa)(dbase + dcol[x], (m_mem.*spsm.rpa)(sbase + scol[x]));
			}
		}
		else
		{
			for(int y = 0; y < h; y++, sy += yinc, dy += yinc)
			{
				uint32 sbase = spo->pixel.row[sy];
				uint32 dbase = dpo->pixel.row[dy];

				int* RESTRICT scol = &spo->pixel.col[sy & 7][sx];
				int* RESTRICT dcol = &dpo->pixel.col[dy & 7][dx];

				for(int x = 0; x > -w; x--) (m_mem.*dpsm.wpa)(dbase + dcol[x], (m_mem.*spsm.rpa)(sbase + scol[x]));
			}
		}
	}
}

void GSState::SoftReset(uint32 mask)
{
	if(mask & 1)
	{
		memset(&m_path[0], 0, sizeof(GIFPath));
		memset(&m_path[3], 0, sizeof(GIFPath));
	}

	if(mask & 2) memset(&m_path[1], 0, sizeof(GIFPath));
	if(mask & 4) memset(&m_path[2], 0, sizeof(GIFPath));

	m_env.TRXDIR.XDIR = 3; //-1 ; set it to invalid value

	m_q = 1;
}

void GSState::ReadFIFO(uint8* mem, int size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	Flush();

	size *= 16;

	Read(mem, size);

	if(m_dump)
	{
		m_dump.ReadFIFO(size);
	}
}

template void GSState::Transfer<0>(const uint8* mem, uint32 size);
template void GSState::Transfer<1>(const uint8* mem, uint32 size);
template void GSState::Transfer<2>(const uint8* mem, uint32 size);
template void GSState::Transfer<3>(const uint8* mem, uint32 size);

template<int index> void GSState::Transfer(const uint8* mem, uint32 size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	const uint8* start = mem;

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		if(path.nloop == 0)
		{
			path.SetTag(mem);

			mem += sizeof(GIFTag);
			size--;

			if(path.nloop > 0) // eeuser 7.2.2. GIFtag: "... when NLOOP is 0, the GIF does not output anything, and values other than the EOP field are disregarded."
			{
				m_q = 1.0f;

				// ASSERT(!(path.tag.PRE && path.tag.FLG == GIF_FLG_REGLIST)); // kingdom hearts

				if(path.tag.PRE && path.tag.FLG == GIF_FLG_PACKED)
				{
					GIFRegPRIM r;
					r.u64 = path.tag.PRIM;
					ApplyPRIM(r);
				}
			}
		}
		else
		{
			switch(path.tag.FLG)
			{
			case GIF_FLG_PACKED:

				// first try a shortcut for a very common case

				if(path.adonly && size >= path.nloop)
				{
					size -= path.nloop;

					do
					{
						(this->*m_fpGIFRegHandlers[((GIFPackedReg*)mem)->A_D.ADDR])(&((GIFPackedReg*)mem)->r);

						mem += sizeof(GIFPackedReg);
					}
					while(--path.nloop > 0);
				}
				else
				{
					do
					{
						(this->*m_fpGIFPackedRegHandlers[path.GetReg()])((GIFPackedReg*)mem);

						mem += sizeof(GIFPackedReg);
						size--;
					}
					while(path.StepReg() && size > 0);
				}

				break;

			case GIF_FLG_REGLIST:

				size *= 2;

				do
				{
					(this->*m_fpGIFRegHandlers[path.GetReg()])((GIFReg*)mem);

					mem += sizeof(GIFReg);
					size--;
				}
				while(path.StepReg() && size > 0);

				if(size & 1) mem += sizeof(GIFReg);

				size /= 2;

				break;

			case GIF_FLG_IMAGE2: // hmmm

				ASSERT(0);

				path.nloop = 0;

				break;

			case GIF_FLG_IMAGE:

				{
					int len = (int)min(size, path.nloop);

					//ASSERT(!(len&3));

					switch(m_env.TRXDIR.XDIR)
					{
					case 0:
						Write(mem, len * 16);
						break;
					case 1:
						// This can't happen; downloads can not be started or performed as part of
						// a GIFtag operation.  They're an entirely separate process that can only be
						// done through the ReverseFIFO transfer (aka ReadFIFO). --air
						ASSERT(0);
						//Read(mem, len * 16);
						break;
					case 2:
						Move();
						break;
					case 3:
						ASSERT(0);
						break;
					default:
						__assume(0);
					}

					mem += len * 16;
					path.nloop -= len;
					size -= len;
				}

				break;

			default:
				__assume(0);
			}
		}

		if(index == 0)
		{
			if(path.tag.EOP && path.nloop == 0)
			{
				break;
			}
		}
	}

	if(m_dump && mem > start)
	{
		m_dump.Transfer(index, start, mem - start);
	}

	if(index == 0)
	{
		if(size == 0 && path.nloop > 0)
		{
			if(m_mt)
			{
				// Hackfix for BIOS, which sends an incomplete packet when it does an XGKICK without
				// having an EOP specified anywhere in VU1 memory.  Needed until PCSX2 is fixed to
				// handle it more properly (ie, without looping infinitely).

				path.nloop = 0;
			}
			else
			{
				// Unused in 0.9.7 and above, but might as well keep this for now; allows GSdx
				// to work with legacy editions of PCSX2.

				Transfer<0>(mem - 0x4000, 0x4000 / 16);
			}
		}
	}
}

template<class T> static void WriteState(uint8*& dst, T* src, size_t len = sizeof(T))
{
	memcpy(dst, src, len);
	dst += len;
}

template<class T> static void ReadState(T* dst, uint8*& src, size_t len = sizeof(T))
{
	memcpy(dst, src, len);
	src += len;
}

int GSState::Freeze(GSFreezeData* fd, bool sizeonly)
{
	if(sizeonly)
	{
		fd->size = m_sssize;
		return 0;
	}

	if(!fd->data || fd->size < m_sssize)
	{
		return -1;
	}

	Flush();

	uint8* data = fd->data;

	WriteState(data, &m_version);
	WriteState(data, &m_env.PRIM);
	WriteState(data, &m_env.PRMODE);
	WriteState(data, &m_env.PRMODECONT);
	WriteState(data, &m_env.TEXCLUT);
	WriteState(data, &m_env.SCANMSK);
	WriteState(data, &m_env.TEXA);
	WriteState(data, &m_env.FOGCOL);
	WriteState(data, &m_env.DIMX);
	WriteState(data, &m_env.DTHE);
	WriteState(data, &m_env.COLCLAMP);
	WriteState(data, &m_env.PABE);
	WriteState(data, &m_env.BITBLTBUF);
	WriteState(data, &m_env.TRXDIR);
	WriteState(data, &m_env.TRXPOS);
	WriteState(data, &m_env.TRXREG);
	WriteState(data, &m_env.TRXREG); // obsolete

	for(int i = 0; i < 2; i++)
	{
		WriteState(data, &m_env.CTXT[i].XYOFFSET);
		WriteState(data, &m_env.CTXT[i].TEX0);
		WriteState(data, &m_env.CTXT[i].TEX1);
		WriteState(data, &m_env.CTXT[i].TEX2);
		WriteState(data, &m_env.CTXT[i].CLAMP);
		WriteState(data, &m_env.CTXT[i].MIPTBP1);
		WriteState(data, &m_env.CTXT[i].MIPTBP2);
		WriteState(data, &m_env.CTXT[i].SCISSOR);
		WriteState(data, &m_env.CTXT[i].ALPHA);
		WriteState(data, &m_env.CTXT[i].TEST);
		WriteState(data, &m_env.CTXT[i].FBA);
		WriteState(data, &m_env.CTXT[i].FRAME);
		WriteState(data, &m_env.CTXT[i].ZBUF);
	}

	WriteState(data, &m_v.RGBAQ);
	WriteState(data, &m_v.ST);
	WriteState(data, &m_v.UV);
	WriteState(data, &m_v.XYZ);
	WriteState(data, &m_v.FOG);
	WriteState(data, &m_tr.x);
	WriteState(data, &m_tr.y);
	WriteState(data, m_mem.m_vm8, m_mem.m_vmsize);

	for(size_t i = 0; i < countof(m_path); i++)
	{
		m_path[i].tag.NREG = m_path[i].nreg;
		m_path[i].tag.NLOOP = m_path[i].nloop;

		WriteState(data, &m_path[i].tag);
		WriteState(data, &m_path[i].reg);
	}

	WriteState(data, &m_q);

	return 0;
}

int GSState::Defrost(const GSFreezeData* fd)
{
	if(!fd || !fd->data || fd->size == 0)
	{
		return -1;
	}

	if(fd->size < m_sssize)
	{
		return -1;
	}

	uint8* data = fd->data;

	int version;

	ReadState(&version, data);

	if(version > m_version)
	{
		printf("GSdx: Savestate version is incompatible.  Load aborted.\n" );

		return -1;
	}

	Flush();

	Reset();

	ReadState(&m_env.PRIM, data);
	ReadState(&m_env.PRMODE, data);
	ReadState(&m_env.PRMODECONT, data);
	ReadState(&m_env.TEXCLUT, data);
	ReadState(&m_env.SCANMSK, data);
	ReadState(&m_env.TEXA, data);
	ReadState(&m_env.FOGCOL, data);
	ReadState(&m_env.DIMX, data);
	ReadState(&m_env.DTHE, data);
	ReadState(&m_env.COLCLAMP, data);
	ReadState(&m_env.PABE, data);
	ReadState(&m_env.BITBLTBUF, data);
	ReadState(&m_env.TRXDIR, data);
	ReadState(&m_env.TRXPOS, data);
	ReadState(&m_env.TRXREG, data);
	ReadState(&m_env.TRXREG, data); // obsolete

	for(int i = 0; i < 2; i++)
	{
		ReadState(&m_env.CTXT[i].XYOFFSET, data);
		ReadState(&m_env.CTXT[i].TEX0, data);
		ReadState(&m_env.CTXT[i].TEX1, data);
		ReadState(&m_env.CTXT[i].TEX2, data);
		ReadState(&m_env.CTXT[i].CLAMP, data);
		ReadState(&m_env.CTXT[i].MIPTBP1, data);
		ReadState(&m_env.CTXT[i].MIPTBP2, data);
		ReadState(&m_env.CTXT[i].SCISSOR, data);
		ReadState(&m_env.CTXT[i].ALPHA, data);
		ReadState(&m_env.CTXT[i].TEST, data);
		ReadState(&m_env.CTXT[i].FBA, data);
		ReadState(&m_env.CTXT[i].FRAME, data);
		ReadState(&m_env.CTXT[i].ZBUF, data);

		m_env.CTXT[i].XYOFFSET.OFX &= 0xffff;
		m_env.CTXT[i].XYOFFSET.OFY &= 0xffff;

		if(version <= 4)
		{
			data += sizeof(uint32) * 7; // skip
		}
	}

	ReadState(&m_v.RGBAQ, data);
	ReadState(&m_v.ST, data);
	ReadState(&m_v.UV, data);
	ReadState(&m_v.XYZ, data);
	ReadState(&m_v.FOG, data);
	ReadState(&m_tr.x, data);
	ReadState(&m_tr.y, data);
	ReadState(m_mem.m_vm8, data, m_mem.m_vmsize);

	m_tr.total = 0; // TODO: restore transfer state

	for(size_t i = 0; i < countof(m_path); i++)
	{
		ReadState(&m_path[i].tag, data);
		ReadState(&m_path[i].reg, data);

		m_path[i].SetTag(&m_path[i].tag); // expand regs
	}

	ReadState(&m_q, data);

	PRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();

	m_env.UpdateDIMX();

	for(size_t i = 0; i < 2; i++)
	{
		m_env.CTXT[i].UpdateScissor();

		m_env.CTXT[i].offset.fb = m_mem.GetOffset(m_env.CTXT[i].FRAME.Block(), m_env.CTXT[i].FRAME.FBW, m_env.CTXT[i].FRAME.PSM);
		m_env.CTXT[i].offset.zb = m_mem.GetOffset(m_env.CTXT[i].ZBUF.Block(), m_env.CTXT[i].FRAME.FBW, m_env.CTXT[i].ZBUF.PSM);
		m_env.CTXT[i].offset.tex = m_mem.GetOffset(m_env.CTXT[i].TEX0.TBP0, m_env.CTXT[i].TEX0.TBW, m_env.CTXT[i].TEX0.PSM);
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset4(m_env.CTXT[i].FRAME, m_env.CTXT[i].ZBUF);
	}

m_perfmon.SetFrame(5000);

	return 0;
}

void GSState::SetGameCRC(uint32 crc, int options)
{
	m_crc = crc;
	m_options = options;
	m_game = CRC::Lookup(crc);
}

// GSTransferBuffer

GSState::GSTransferBuffer::GSTransferBuffer()
{
	x = y = 0;
	start = end = total = 0;
	buff = (uint8*)_aligned_malloc(1024 * 1024 * 4, 32);
}

GSState::GSTransferBuffer::~GSTransferBuffer()
{
	_aligned_free(buff);
}

void GSState::GSTransferBuffer::Init(int tx, int ty)
{
	x = tx;
	y = ty;
	total = 0;
}

bool GSState::GSTransferBuffer::Update(int tw, int th, int bpp, int& len)
{
	if(total == 0)
	{
		start = end = 0;
		total = std::min<int>((tw * bpp >> 3) * th, 1024 * 1024 * 4);
		overflow = false;
	}

	int remaining = total - end;

	if(len > remaining)
	{
		if(!overflow)
		{
			overflow = true;

			// printf("GS transfer overflow\n");
		}

		len = remaining;
	}

	return len > 0;
}

// hacks

struct GSFrameInfo
{
	uint32 FBP;
	uint32 FPSM;
	uint32 FBMSK;
	uint32 TBP0;
	uint32 TPSM;
	uint32 TZTST;
	bool TME;
};

typedef bool (*GetSkipCount)(const GSFrameInfo& fi, int& skip);

bool GSC_Okami(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSM_PSMT4)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_MetalGearSolid3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1000; // 76, 79
		}
		else if(fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSM_PSMCT24 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1000; // 69
		}
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_DBZBT2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && /*fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 &&*/ fi.TBP0 == 0x02000 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 27;
		}
		else if(!fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSM_PSMCT16)
		{
			skip = 10;
		}
	}

	return true;
}

bool GSC_DBZBT3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00) && fi.TPSM == PSM_PSMT8H)
		{
			skip = 24; // blur
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 28; // outline
		}
	}

	return true;
}

bool GSC_SFEX3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00500 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00f00 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2; // blur
		}
	}

	return true;
}

bool GSC_Bully(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSM_PSMCT16S && fi.TBP0 == 0x02300 && fi.TPSM == PSM_PSMZ16S)
		{
			skip = 6;
		}
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_BullyCC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(!fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSM_PSMCT24)
		{
			skip = 9;
		}
	}

	return true;
}
bool GSC_SoTC(const GSFrameInfo& fi, int& skip)
{
	// Not needed anymore? What did it fix anyway? (rama)
	/*if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02b80 && fi.FPSM == PSM_PSMCT24 && fi.TBP0 == 0x01e80 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 9;
		}
		else if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 8;
		}
		else if(fi.TME && fi.FBP == 0x01e80 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03880 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 8;
		}
	}*/

	return true;
}

bool GSC_OnePieceGrandAdventure(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x00f00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_OnePieceGrandBattle(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00f00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_ICO(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03d00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 3;
		}
		else if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02800 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 1;
		}
	}
	else
	{
		if(fi.TME && fi.TBP0 == 0x00800 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_GT4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x03440 || fi.FBP >= 0x03e00) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400) && fi.TPSM == PSM_PSMT8)
		{
			skip = 880;
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01400) && fi.FPSM == PSM_PSMCT24 && fi.TBP0 >= 0x03420 && fi.TPSM == PSM_PSMT8)
		{
			// TODO: removes gfx from where it is not supposed to (garage)
			// skip = 58;
		}
	}

	return true;
}

bool GSC_WildArms4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_WildArms5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_Manhunt2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03c20 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01400 && fi.TPSM == PSM_PSMT8)
		{
			skip = 640;
		}
	}

	return true;
}

bool GSC_CrashBandicootWoC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00a00) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(fi.TME && fi.FBP == 0x02200 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01400 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 41;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03c00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
		else if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_ResidentEvil4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 176;
		}
	}

	return true;
}

bool GSC_Spartan(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 107;
		}
	}

	return true;
}

bool GSC_AceCombat4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02a00 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01600 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 71; // clouds (z, 16-bit)
		}
		else if(fi.TME && fi.FBP == 0x02900 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 28; // blur
		}
	}

	return true;
}

bool GSC_Drakengard2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x026c0 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 64;
		}
	}

	return true;
}

bool GSC_Tekken5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02ea0 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 95;
		}
	}

	return true;
}

bool GSC_IkkiTousen(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00a80 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01180 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 1000; // shadow (result is broken without depth copy, also includes 16 bit)
		}
		else if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSM_PSMZ24 && fi.TBP0 == 0x01180 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 11; // blur
		}
	}
	else if(skip > 7)
	{
		if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00700 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 7; // the last steps of shadow drawing
		}
	}

	return true;
}

bool GSC_GodOfWar(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 30;
		}
		else if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0xff000000)
		{
			skip = 1; // blur
		}
		else if(fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8 && ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
		{
			skip = 1; // wall of fog
		}
	}

	return true;
}

bool GSC_GodOfWar2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
			if(fi.FBP == 0x00100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00100 && fi.TPSM == PSM_PSMCT16 // ntsc
				|| fi.FBP == 0x02100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02100 && fi.TPSM == PSM_PSMCT16) // pal
			{
				skip = 29; // shadows
			}
			if(fi.FBP == 0x00100 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 & 0x03000) == 0x03000
				&& (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4)
				&& ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000))){
					skip = 1; // wall of fog
			}
		}
	}

	return true;
}

bool GSC_GiTS(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01400 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02e40 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1315;
		}
	}
	else
	{
	}

	return true;
}

bool GSC_Onimusha3(const GSFrameInfo& fi, int& skip)
{
	if(fi.TME /*&& (fi.FBP == 0x00000 || fi.FBP == 0x00700)*/ && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000 || fi.TBP0 == 0x01200) && (fi.TPSM == PSM_PSMCT32 || fi.TPSM == PSM_PSMCT24))
	{
		skip = 1;
	}

	return true;
}

bool GSC_TalesOfAbyss(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMT8) // copies the z buffer to the alpha channel of the fb
		{
			skip = 1000;
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && (fi.TBP0 == 0x03560 || fi.TBP0 == 0x038e0) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}
	else
	{
		if(fi.TME && fi.TPSM != PSM_PSMT8)
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_SonicUnleashed(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02200 && fi.FPSM == PSM_PSMCT16S && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000; // shadow
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02200 && fi.TPSM == PSM_PSMCT16S)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_Genji(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01500 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00e00 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 6; //
		}
	}
	else
	{
	}

	return true;
}

bool GSC_StarOcean3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			skip = 1000; //
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_ValkyrieProfile2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			skip = 1000; //
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_RadiataStories(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH)
		{
			skip = 1000; //
		}
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4HH))
		{
			skip = 0;
		}
	}

	return true;
}

bool GSC_HauntingGround(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16S && fi.FBMSK == 0x03FFF)
		{
			skip = 1;
		}
		else if(fi.TME && fi.FBP == 0x3000 && fi.TBP0 == 0x3380)
		{
			skip = 1; // bloom
		}
		else if(fi.TME && fi.FBP == fi.TBP0 && fi.TBP0 == 0x3000 && fi.FBMSK == 0xFFFFFF &&
			GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM))
		{
			skip = 1;
		}
	}

	return true;
}

bool GSState::IsBadFrame(int& skip, int UserHacks_SkipDraw)
{
	GSFrameInfo fi;

	fi.FBP = m_context->FRAME.Block();
	fi.FPSM = m_context->FRAME.PSM;
	fi.FBMSK = m_context->FRAME.FBMSK;
	fi.TME = PRIM->TME;
	fi.TBP0 = m_context->TEX0.TBP0;
	fi.TPSM = m_context->TEX0.PSM;
	fi.TZTST = m_context->TEST.ZTST;

	static GetSkipCount map[CRC::TitleCount];
	static bool inited = false;

	if(!inited)
	{
		inited = true;

		memset(map, 0, sizeof(map));

		map[CRC::Okami] = GSC_Okami;
		map[CRC::MetalGearSolid3] = GSC_MetalGearSolid3;
		map[CRC::DBZBT2] = GSC_DBZBT2;
		map[CRC::DBZBT3] = GSC_DBZBT3;
		map[CRC::SFEX3] = GSC_SFEX3;
		map[CRC::Bully] = GSC_Bully;
		map[CRC::BullyCC] = GSC_BullyCC;
		map[CRC::SoTC] = GSC_SoTC;
		map[CRC::OnePieceGrandAdventure] = GSC_OnePieceGrandAdventure;
		map[CRC::OnePieceGrandBattle] = GSC_OnePieceGrandBattle;
		map[CRC::ICO] = GSC_ICO;
		map[CRC::GT4] = GSC_GT4;
		map[CRC::WildArms4] = GSC_WildArms4;
		map[CRC::WildArms5] = GSC_WildArms5;
		map[CRC::Manhunt2] = GSC_Manhunt2;
		map[CRC::CrashBandicootWoC] = GSC_CrashBandicootWoC;
		map[CRC::ResidentEvil4] = GSC_ResidentEvil4;
		map[CRC::Spartan] = GSC_Spartan;
		map[CRC::AceCombat4] = GSC_AceCombat4;
		map[CRC::Drakengard2] = GSC_Drakengard2;
		map[CRC::Tekken5] = GSC_Tekken5;
		map[CRC::IkkiTousen] = GSC_IkkiTousen;
		map[CRC::GodOfWar] = GSC_GodOfWar;
		map[CRC::GodOfWar2] = GSC_GodOfWar2;
		map[CRC::GiTS] = GSC_GiTS;
		map[CRC::Onimusha3] = GSC_Onimusha3;
		map[CRC::TalesOfAbyss] = GSC_TalesOfAbyss;
		map[CRC::SonicUnleashed] = GSC_SonicUnleashed;
		map[CRC::Genji] = GSC_Genji;
		map[CRC::StarOcean3] = GSC_StarOcean3;
		map[CRC::ValkyrieProfile2] = GSC_ValkyrieProfile2;
		map[CRC::RadiataStories] = GSC_RadiataStories;
		map[CRC::HauntingGround] = GSC_HauntingGround;
	}

	// TODO: just set gsc in SetGameCRC once

	GetSkipCount gsc = map[m_game.title];

	if(gsc && !gsc(fi, skip))
	{
		return false;
	}

	if(skip == 0 && (UserHacks_SkipDraw > 0) )
	{
		if(fi.TME)
		{
			// depth textures (bully, mgs3s1 intro, Front Mission 5)
			if( (fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S) ||
				// General, often problematic post processing
				(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)) )
			{
				skip = UserHacks_SkipDraw;
			}
		}
	}
	// Mimic old GSdx behavior (skipping all depth textures with a skip value of 1), to avoid floods of bug reports
	// that games are broken, when the user hasn't found the skiphack yet. (God of War, sigh...)
	else if (skip == 0)
	{
		if(fi.TME)
		{
			if(fi.TPSM == PSM_PSMZ32 || fi.TPSM == PSM_PSMZ24 || fi.TPSM == PSM_PSMZ16 || fi.TPSM == PSM_PSMZ16S)
			{
				skip = 1;
			}
		}
	}

	if(skip > 0)
	{
		skip--;

		return true;
	}

	return false;
}
