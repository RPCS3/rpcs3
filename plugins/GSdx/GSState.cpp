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

GSState::GSState(BYTE* base, bool mt, void (*irq)(), int nloophack)
	: m_mt(mt)
	, m_irq(irq)
	, m_nloophack_org(nloophack)
	, m_nloophack(nloophack == 1)
	, m_crc(0)
	, m_options(0)
	, m_path3hack(0)
	, m_q(1.0f)
	, m_vprim(1)
	, m_version(5)
	, m_frameskip(0)
	, m_vkf(NULL)
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
	m_sssize += sizeof(m_env.TRXREG2);
	
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

	m_sssize += sizeof(m_x);
	m_sssize += sizeof(m_y);
	m_sssize += m_mem.m_vmsize;
	m_sssize += (sizeof(m_path[0].tag) + sizeof(m_path[0].nreg)) * 3;
	m_sssize += sizeof(m_q);

	ASSERT(base);

	PMODE = (GSRegPMODE*)(base + GS_PMODE);
	SMODE1 = (GSRegSMODE1*)(base + GS_SMODE1);
	SMODE2 = (GSRegSMODE2*)(base + GS_SMODE2);
	// SRFSH = (GSRegPMODE*)(base + GS_SRFSH);
	// SYNCH1 = (GSRegPMODE*)(base + GS_SYNCH1);
	// SYNCH2 = (GSRegPMODE*)(base + GS_SYNCH2);
	// SYNCV = (GSRegPMODE*)(base + GS_SYNCV);
	DISPFB[0] = (GSRegDISPFB*)(base + GS_DISPFB1);
	DISPFB[1] = (GSRegDISPFB*)(base + GS_DISPFB2);
	DISPLAY[0] = (GSRegDISPLAY*)(base + GS_DISPLAY1);
	DISPLAY[1] = (GSRegDISPLAY*)(base + GS_DISPLAY2);
	EXTBUF = (GSRegEXTBUF*)(base + GS_EXTBUF);
	EXTDATA = (GSRegEXTDATA*)(base + GS_EXTDATA);
	EXTWRITE = (GSRegEXTWRITE*)(base + GS_EXTWRITE);
	BGCOLOR = (GSRegBGCOLOR*)(base + GS_BGCOLOR);
	CSR = (GSRegCSR*)(base + GS_CSR);
	IMR = (GSRegIMR*)(base + GS_IMR);
	BUSDIR = (GSRegBUSDIR*)(base + GS_BUSDIR);
	SIGLBLID = (GSRegSIGLBLID*)(base + GS_SIGLBLID);

	memset(PMODE, 0, 0x1100);

	PRIM = &m_env.PRIM;
//	CSR->rREV = 0x20;
	m_env.PRMODECONT.AC = 1;

	m_x = m_y = 0;
	m_bytes = 0;
	m_maxbytes = 1024 * 1024 * 4;
	m_buff = (BYTE*)_aligned_malloc(m_maxbytes, 16);

	Reset();

	ResetHandlers();
}

GSState::~GSState()
{
	_aligned_free(m_buff);
}

void GSState::Reset()
{
	memset(&m_path[0], 0, sizeof(m_path[0]) * 3);
	memset(&m_v, 0, sizeof(m_v));

//	PRIM = &m_env.PRIM;
//	m_env.PRMODECONT.AC = 1;

	m_env.Reset();

	m_context = &m_env.CTXT[0];

	InvalidateTextureCache();
}

void GSState::ResetHandlers()
{
	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
	{
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;
	}

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
	m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
	m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = &GSState::GIFPackedRegHandlerTEX0<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = &GSState::GIFPackedRegHandlerTEX0<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerCLAMP<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerCLAMP<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerXYZF3;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerXYZ3;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	for(int i = 0; i < countof(m_fpGIFRegHandlers); i++)
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
	m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerSIGNAL;
	m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerFINISH;
	m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerLABEL;
}

CPoint GSState::GetDisplayPos(int i)
{
	ASSERT(i >= 0 && i < 2);

	CPoint p;

	p.x = DISPLAY[i]->DX / (DISPLAY[i]->MAGH + 1);
	p.y = DISPLAY[i]->DY / (DISPLAY[i]->MAGV + 1);

	return p;
}

CSize GSState::GetDisplaySize(int i)
{
	ASSERT(i >= 0 && i < 2);

	CSize s;

	s.cx = (DISPLAY[i]->DW + 1) / (DISPLAY[i]->MAGH + 1);
	s.cy = (DISPLAY[i]->DH + 1) / (DISPLAY[i]->MAGV + 1);

	return s;
}

CRect GSState::GetDisplayRect(int i)
{
	return CRect(GetDisplayPos(i), GetDisplaySize(i));
}

CSize GSState::GetDisplayPos()
{
	return GetDisplayPos(IsEnabled(1) ? 1 : 0);
}	

CSize GSState::GetDisplaySize()
{
	return GetDisplaySize(IsEnabled(1) ? 1 : 0);
}	

CRect GSState::GetDisplayRect()
{
	return GetDisplayRect(IsEnabled(1) ? 1 : 0);
}

CPoint GSState::GetFramePos(int i)
{
	ASSERT(i >= 0 && i < 2);

	return CPoint(DISPFB[i]->DBX, DISPFB[i]->DBY);
}

CSize GSState::GetFrameSize(int i)
{
	CSize s = GetDisplaySize(i);

	if(SMODE2->INT && SMODE2->FFMD && s.cy > 1) s.cy >>= 1;

	return s;
}

CRect GSState::GetFrameRect(int i)
{
	return CRect(GetFramePos(i), GetFrameSize(i));
}

CSize GSState::GetFramePos()
{
	return GetFramePos(IsEnabled(1) ? 1 : 0);
}	

CSize GSState::GetFrameSize()
{
	return GetFrameSize(IsEnabled(1) ? 1 : 0);
}	

CRect GSState::GetFrameRect()
{
	return GetFrameRect(IsEnabled(1) ? 1 : 0);
}

CSize GSState::GetDeviceSize(int i)
{
	// TODO: other params of SMODE1 should affect the true device display size

	// TODO2: pal games at 60Hz

	CSize s = GetDisplaySize(i);

	if(s.cy == 2*416 || s.cy == 2*448 || s.cy == 2*512)
	{
		s.cy /= 2;
	}
	else
	{
		s.cy = (SMODE1->CMOD & 1) ? 512 : 448;
	}

	return s;

}

CSize GSState::GetDeviceSize()
{
	return GetDeviceSize(IsEnabled(1) ? 1 : 0);
}

bool GSState::IsEnabled(int i)
{
	ASSERT(i >= 0 && i < 2);

	if(i == 0 && PMODE->EN1) 
	{
		return DISPLAY[0]->DW || DISPLAY[0]->DH;
	}
	else if(i == 1 && PMODE->EN2) 
	{
		return DISPLAY[1]->DW || DISPLAY[1]->DH;
	}

	return false;
}

int GSState::GetFPS()
{
	return ((SMODE1->CMOD & 1) ? 50 : 60) / (SMODE2->INT ? 1 : 2);
}

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(GIFPackedReg* r)
{
	// ASSERT(0);
}

void GSState::GIFPackedRegHandlerPRIM(GIFPackedReg* r)
{
	// ASSERT(r->r.PRIM.PRIM < 7);

	GIFRegHandlerPRIM(&r->r);
}

void GSState::GIFPackedRegHandlerRGBA(GIFPackedReg* r)
{
	#if _M_SSE >= 0x301

	GSVector4i mask = GSVector4i::load(0x0c080400);
	GSVector4i v = GSVector4i::load<false>(r).shuffle8(mask);
	m_v.RGBAQ.ai32[0] = (UINT32)GSVector4i::store(v);

	#elif _M_SSE >= 0x200

	GSVector4i v = GSVector4i::load<false>(r) & GSVector4i::x000000ff();
	m_v.RGBAQ.ai32[0] = v.rgba32();

	#else

	m_v.RGBAQ.R = r->RGBA.R;
	m_v.RGBAQ.G = r->RGBA.G;
	m_v.RGBAQ.B = r->RGBA.B;
	m_v.RGBAQ.A = r->RGBA.A;

	#endif

	m_v.RGBAQ.Q = m_q;
}

void GSState::GIFPackedRegHandlerSTQ(GIFPackedReg* r)
{
	#if defined(_M_AMD64)

	m_v.ST.i64 = r->ai64[0];

	#elif _M_SSE >= 0x200

	GSVector4i v = GSVector4i::loadl(r);
	GSVector4i::storel(&m_v.ST.i64, v);

	#else

	m_v.ST.S = r->STQ.S;
	m_v.ST.T = r->STQ.T;

	#endif

	m_q = r->STQ.Q;
}

void GSState::GIFPackedRegHandlerUV(GIFPackedReg* r)
{
	#if _M_SSE >= 0x200

	GSVector4i v = GSVector4i::loadl(r) & GSVector4i::x00003fff();
	m_v.UV.ai32[0] = (UINT32)GSVector4i::store(v.ps32(v));

	#else

	m_v.UV.U = r->UV.U;
	m_v.UV.V = r->UV.V;

	#endif
}

void GSState::GIFPackedRegHandlerXYZF2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZF2.X;
	m_v.XYZ.Y = r->XYZF2.Y;
	m_v.XYZ.Z = r->XYZF2.Z;
	m_v.FOG.F = r->XYZF2.F;

	VertexKick(r->XYZF2.ADC);
}

void GSState::GIFPackedRegHandlerXYZ2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZ2.X;
	m_v.XYZ.Y = r->XYZ2.Y;
	m_v.XYZ.Z = r->XYZ2.Z;

	VertexKick(r->XYZ2.ADC);
}

template<int i> void GSState::GIFPackedRegHandlerTEX0(GIFPackedReg* r)
{
	GIFRegHandlerTEX0<i>((GIFReg*)&r->ai64[0]);
}

template<int i> void GSState::GIFPackedRegHandlerCLAMP(GIFPackedReg* r)
{
	GIFRegHandlerCLAMP<i>((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerFOG(GIFPackedReg* r)
{
	m_v.FOG.F = r->FOG.F;
}

void GSState::GIFPackedRegHandlerXYZF3(GIFPackedReg* r)
{
	GIFRegHandlerXYZF3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerXYZ3(GIFPackedReg* r)
{
	GIFRegHandlerXYZ3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerA_D(GIFPackedReg* r)
{
	(this->*m_fpGIFRegHandlers[(BYTE)r->A_D.ADDR])(&r->r);
}

void GSState::GIFPackedRegHandlerA_D(GIFPackedReg* r, int size)
{
	for(int i = 0; i < size; i++)
	{
		(this->*m_fpGIFRegHandlers[(BYTE)r[i].A_D.ADDR])(&r[i].r);
	}
}

void GSState::GIFPackedRegHandlerNOP(GIFPackedReg* r)
{
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(GIFReg* r)
{
	// ASSERT(0);
}

void GSState::GIFRegHandlerPRIM(GIFReg* r)
{
	// ASSERT(r->PRIM.PRIM < 7);

	if(GSUtil::GetPrimClass(m_env.PRIM.PRIM) == GSUtil::GetPrimClass(r->PRIM.PRIM))
	{
		if(((m_env.PRIM.i64 ^ r->PRIM.i64) & ~7) != 0)
		{
			Flush();
		}
	}
	else
	{
		Flush();
	}

	m_env.PRIM = (GSVector4i)r->PRIM;
	m_env.PRMODE._PRIM = r->PRIM.PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();

	ResetPrim();
}

void GSState::GIFRegHandlerRGBAQ(GIFReg* r)
{
	m_v.RGBAQ = (GSVector4i)r->RGBAQ;
}

void GSState::GIFRegHandlerST(GIFReg* r)
{
	m_v.ST = (GSVector4i)r->ST;
}

void GSState::GIFRegHandlerUV(GIFReg* r)
{
	m_v.UV.ai32[0] = r->UV.ai32[0] & 0x3fff3fff;
}

void GSState::GIFRegHandlerXYZF2(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(false);
}

void GSState::GIFRegHandlerXYZ2(GIFReg* r)
{
	m_v.XYZ = (GSVector4i)r->XYZ;

	VertexKick(false);
}

template<int i> void GSState::GIFRegHandlerTEX0(GIFReg* r)
{
	// even if TEX0 did not change, a new palette may have been uploaded and will overwrite the currently queued for drawing

	bool wt = m_mem.m_clut.WriteTest(r->TEX0, m_env.TEXCLUT);

	if(wt || PRIM->CTXT == i && !(m_env.CTXT[i].TEX0 == (GSVector4i)r->TEX0).alltrue())
	{
		Flush(); 
	}

	m_env.CTXT[i].TEX0 = (GSVector4i)r->TEX0;

	if(m_env.CTXT[i].TEX0.TW > 10) m_env.CTXT[i].TEX0.TW = 10;
	if(m_env.CTXT[i].TEX0.TH > 10) m_env.CTXT[i].TEX0.TH = 10;

	m_env.CTXT[i].TEX0.CPSM &= 0xa; // 1010b

	if((m_env.CTXT[i].TEX0.TBW & 1) && (m_env.CTXT[i].TEX0.PSM == PSM_PSMT8 || m_env.CTXT[i].TEX0.PSM == PSM_PSMT4))
	{
		m_env.CTXT[i].TEX0.TBW &= ~1; // GS User 2.6
	}

	if(wt)
	{
		m_mem.m_clut.Write(m_env.CTXT[i].TEX0, m_env.TEXCLUT);
	}
}

template<int i> void GSState::GIFRegHandlerCLAMP(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].CLAMP == (GSVector4i)r->CLAMP).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].CLAMP = (GSVector4i)r->CLAMP;
}

void GSState::GIFRegHandlerFOG(GIFReg* r)
{
	m_v.FOG = (GSVector4i)r->FOG;
}

void GSState::GIFRegHandlerXYZF3(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(true);
}

void GSState::GIFRegHandlerXYZ3(GIFReg* r)
{
	m_v.XYZ = (GSVector4i)r->XYZ;

	VertexKick(true);
}

void GSState::GIFRegHandlerNOP(GIFReg* r)
{
}

template<int i> void GSState::GIFRegHandlerTEX1(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].TEX1 == (GSVector4i)r->TEX1).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].TEX1 = (GSVector4i)r->TEX1;
}

template<int i> void GSState::GIFRegHandlerTEX2(GIFReg* r)
{
	// m_env.CTXT[i].TEX2 = r->TEX2; // not used

	UINT64 mask = 0xFFFFFFE003F00000ui64; // TEX2 bits

	r->i64 = (r->i64 & mask) | (m_env.CTXT[i].TEX0.i64 & ~mask);

	GIFRegHandlerTEX0<i>(r);
}

template<int i> void GSState::GIFRegHandlerXYOFFSET(GIFReg* r)
{
	GSVector4i o = (GSVector4i)r->XYOFFSET & GSVector4i::x0000ffff();

	if(!(m_env.CTXT[i].XYOFFSET == o).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].XYOFFSET = o;

	m_env.CTXT[i].UpdateScissor();
}

void GSState::GIFRegHandlerPRMODECONT(GIFReg* r)
{
	if(!(m_env.PRMODECONT == (GSVector4i)r->PRMODECONT).alltrue())
	{
		Flush();
	}

	m_env.PRMODECONT.AC = r->PRMODECONT.AC;

	PRIM = m_env.PRMODECONT.AC ? &m_env.PRIM : (GIFRegPRIM*)&m_env.PRMODE;

	if(PRIM->PRIM == 7) TRACE(_T("Invalid PRMODECONT/PRIM\n"));

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();
}

void GSState::GIFRegHandlerPRMODE(GIFReg* r)
{
	if(!m_env.PRMODECONT.AC)
	{
		Flush();
	}

	UINT32 _PRIM = m_env.PRMODE._PRIM;
	m_env.PRMODE = (GSVector4i)r->PRMODE;
	m_env.PRMODE._PRIM = _PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();
}

void GSState::GIFRegHandlerTEXCLUT(GIFReg* r)
{
	if(!(m_env.TEXCLUT == (GSVector4i)r->TEXCLUT).alltrue())
	{
		Flush();
	}

	m_env.TEXCLUT = (GSVector4i)r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(GIFReg* r)
{
	if(!(m_env.SCANMSK == (GSVector4i)r->SCANMSK).alltrue())
	{
		Flush();
	}

	m_env.SCANMSK = (GSVector4i)r->SCANMSK;
}

template<int i> void GSState::GIFRegHandlerMIPTBP1(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].MIPTBP1 == (GSVector4i)r->MIPTBP1).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP1 = (GSVector4i)r->MIPTBP1;
}

template<int i> void GSState::GIFRegHandlerMIPTBP2(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].MIPTBP2 == (GSVector4i)r->MIPTBP2).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP2 = (GSVector4i)r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(GIFReg* r)
{
	if(!(m_env.TEXA == (GSVector4i)r->TEXA).alltrue())
	{
		Flush();
	}

	m_env.TEXA = (GSVector4i)r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(GIFReg* r)
{
	if(!(m_env.FOGCOL == (GSVector4i)r->FOGCOL).alltrue())
	{
		Flush();
	}

	m_env.FOGCOL = (GSVector4i)r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(GIFReg* r)
{
	// TRACE(_T("TEXFLUSH\n"));

	// InvalidateTextureCache();
}

template<int i> void GSState::GIFRegHandlerSCISSOR(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].SCISSOR == (GSVector4i)r->SCISSOR).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = (GSVector4i)r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();
}

template<int i> void GSState::GIFRegHandlerALPHA(GIFReg* r)
{
	ASSERT(r->ALPHA.A != 3);
	ASSERT(r->ALPHA.B != 3);
	ASSERT(r->ALPHA.C != 3);
	ASSERT(r->ALPHA.D != 3);

	if(PRIM->CTXT == i && !(m_env.CTXT[i].ALPHA == (GSVector4i)r->ALPHA).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].ALPHA = (GSVector4i)r->ALPHA;

	// A/B/C/D == 3? => 2

	m_env.CTXT[i].ALPHA.ai32[0] = ((~m_env.CTXT[i].ALPHA.ai32[0] >> 1) | 0xAA) & m_env.CTXT[i].ALPHA.ai32[0];
}

void GSState::GIFRegHandlerDIMX(GIFReg* r)
{
	bool update = false;

	if(!(m_env.DIMX == (GSVector4i)r->DIMX).alltrue())
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

void GSState::GIFRegHandlerDTHE(GIFReg* r)
{
	if(!(m_env.DTHE == (GSVector4i)r->DTHE).alltrue())
	{
		Flush();
	}

	m_env.DTHE = (GSVector4i)r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(GIFReg* r)
{
	if(!(m_env.COLCLAMP == (GSVector4i)r->COLCLAMP).alltrue())
	{
		Flush();
	}

	m_env.COLCLAMP = (GSVector4i)r->COLCLAMP;
}

template<int i> void GSState::GIFRegHandlerTEST(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].TEST == (GSVector4i)r->TEST).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].TEST = (GSVector4i)r->TEST;
}

void GSState::GIFRegHandlerPABE(GIFReg* r)
{
	if(!(m_env.PABE == (GSVector4i)r->PABE).alltrue())
	{
		Flush();
	}

	m_env.PABE = (GSVector4i)r->PABE;
}

template<int i> void GSState::GIFRegHandlerFBA(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].FBA == (GSVector4i)r->FBA).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].FBA = (GSVector4i)r->FBA;
}

template<int i> void GSState::GIFRegHandlerFRAME(GIFReg* r)
{
	if(PRIM->CTXT == i && !(m_env.CTXT[i].FRAME == (GSVector4i)r->FRAME).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].FRAME = (GSVector4i)r->FRAME;
}

template<int i> void GSState::GIFRegHandlerZBUF(GIFReg* r)
{
	if(r->ZBUF.ai32[0] == 0)
	{
		// during startup all regs are cleared to 0 (by the bios or something), so we mask z until this register becomes valid

		r->ZBUF.ZMSK = 1; 
	}

	r->ZBUF.PSM |= 0x30;

	if(PRIM->CTXT == i && !(m_env.CTXT[i].ZBUF == (GSVector4i)r->ZBUF).alltrue())
	{
		Flush();
	}

	m_env.CTXT[i].ZBUF = (GSVector4i)r->ZBUF;

	if(m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ32
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ24
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16S)
	{
		m_env.CTXT[i].ZBUF.PSM = PSM_PSMZ32;
	}
}

void GSState::GIFRegHandlerBITBLTBUF(GIFReg* r)
{
	if(!(m_env.BITBLTBUF == (GSVector4i)r->BITBLTBUF).alltrue())
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

void GSState::GIFRegHandlerTRXPOS(GIFReg* r)
{
	if(!(m_env.TRXPOS == (GSVector4i)r->TRXPOS).alltrue())
	{
		FlushWrite();
	}

	m_env.TRXPOS = (GSVector4i)r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(GIFReg* r)
{
	if(!(m_env.TRXREG == (GSVector4i)r->TRXREG).alltrue() || !(m_env.TRXREG2 == (GSVector4i)r->TRXREG).alltrue())
	{
		FlushWrite();
	}

	m_env.TRXREG = (GSVector4i)r->TRXREG;
	m_env.TRXREG2 = (GSVector4i)r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(GIFReg* r)
{
	Flush();

	m_env.TRXDIR = (GSVector4i)r->TRXDIR;

	switch(m_env.TRXDIR.XDIR)
	{
	case 0: // host -> local
		m_x = m_env.TRXPOS.DSAX;
		m_y = m_env.TRXPOS.DSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 1: // local -> host
		m_x = m_env.TRXPOS.SSAX;
		m_y = m_env.TRXPOS.SSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 2: // local -> local
		Move();
		break;
	case 3: 
		ASSERT(0);
		break;
	}
}

void GSState::GIFRegHandlerHWREG(GIFReg* r)
{
	ASSERT(m_env.TRXDIR.XDIR == 0); // host => local

	Write((BYTE*)r, 8); // hunting ground
}

void GSState::GIFRegHandlerSIGNAL(GIFReg* r)
{
	if(m_mt) return;

	SIGLBLID->SIGID = (SIGLBLID->SIGID & ~r->SIGNAL.IDMSK) | (r->SIGNAL.ID & r->SIGNAL.IDMSK);

	if(CSR->wSIGNAL) CSR->rSIGNAL = 1;
	if(!IMR->SIGMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerFINISH(GIFReg* r)
{
	if(m_mt) return;

	if(CSR->wFINISH) CSR->rFINISH = 1;
	if(!IMR->FINISHMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerLABEL(GIFReg* r)
{
	if(m_mt) return;

	SIGLBLID->LBLID = (SIGLBLID->LBLID & ~r->LABEL.IDMSK) | (r->LABEL.ID & r->LABEL.IDMSK);
}

//

void GSState::Flush()
{
	FlushWrite();

	FlushPrim();
}

void GSState::FlushWrite()
{
	FlushWrite(m_buff, m_bytes);

	m_bytes = 0;
}

void GSState::FlushWrite(BYTE* mem, int len)
{
	if(len > 0)
	{
/*
CSize bs = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].bs;

if((m_x & (bs.cx - 1)) || (m_env.TRXREG.RRW & (bs.cx - 1))
|| (m_y & (bs.cy - 1)) || (m_env.TRXREG.RRH & (bs.cy - 1))
|| m_x != m_env.TRXPOS.DSAX)
{
	printf("*** [%d]: %d %d, %d %d %d %d\n", m_env.BITBLTBUF.DPSM, m_env.TRXPOS.DSAX, m_env.TRXPOS.DSAY, m_x, m_y, m_env.TRXREG.RRW, m_env.TRXREG.RRH);
}

if((len % ((m_env.TRXREG.RRW - m_x) * GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].trbpp / 8)) != 0)
{
	printf("*** [%d]: %d %d\n", m_env.BITBLTBUF.DPSM, len, ((m_env.TRXREG.RRW - m_x) * GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].trbpp / 8));
}
*/
		int y = m_y;

		GSLocalMemory::writeImage wi = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wi;

		(m_mem.*wi)(m_x, m_y, mem, len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

		m_perfmon.Put(GSPerfMon::Swizzle, len);

		//ASSERT(m_env.TRXREG.RRH >= m_y - y);

		CRect r;
		
		r.left = m_env.TRXPOS.DSAX;
		r.top = y;
		r.right = m_env.TRXREG.RRW;
		r.bottom = min(m_x == m_env.TRXPOS.DSAX ? m_y : m_y + 1, m_env.TRXREG.RRH);

		InvalidateVideoMem(m_env.BITBLTBUF, r);
/*
		static int n = 0;
		CString str;
		str.Format(_T("c:\\temp1\\[%04d]_%05x_%d_%d_%d_%d_%d_%d.bmp"), 
			n++, (int)m_env.BITBLTBUF.DBP, (int)m_env.BITBLTBUF.DBW, (int)m_env.BITBLTBUF.DPSM, 
			r.left, r.top, r.right, r.bottom);
		m_mem.SaveBMP(str, m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM, r.right, r.bottom);
*/
	}
}

//

void GSState::Write(BYTE* mem, int len)
{
/*
	TRACE(_T("Write len=%d DBP=%05x DBW=%d DPSM=%d DSAX=%d DSAY=%d RRW=%d RRH=%d\n"), 
		  len, (int)m_env.BITBLTBUF.DBP, (int)m_env.BITBLTBUF.DBW, (int)m_env.BITBLTBUF.DPSM, 
		  (int)m_env.TRXPOS.DSAX, (int)m_env.TRXPOS.DSAY,
		  (int)m_env.TRXREG.RRW, (int)m_env.TRXREG.RRH);
*/
	if(len == 0) return;

	if(m_y >= m_env.TRXREG.RRH) return; // TODO: handle overflow during writing data too (just chop len below somewhere)

	// TODO: hmmmm

	if(PRIM->TME && (m_env.BITBLTBUF.DBP == m_context->TEX0.TBP0 || m_env.BITBLTBUF.DBP == m_context->TEX0.CBP))
	{
		FlushPrim();
	}

	int bpp = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].trbpp;

	int pitch = (m_env.TRXREG.RRW - m_env.TRXPOS.DSAX) * bpp >> 3;

	if(pitch <= 0) {ASSERT(0); return;}

	int height = len / pitch;

	if(height > m_env.TRXREG.RRH - m_env.TRXPOS.DSAY)
	{
		height = m_env.TRXREG.RRH - m_env.TRXPOS.DSAY;

		len = height * pitch;
	}

	if(m_bytes > 0 || height < m_env.TRXREG.RRH - m_env.TRXPOS.DSAY)
	{
		ASSERT(len <= m_maxbytes); // more than 4mb into a 4mb local mem doesn't make sense

		len = min(m_maxbytes, len);

		if(m_bytes + len > m_maxbytes)
		{
			FlushWrite();
		}

		memcpy(&m_buff[m_bytes], mem, len);

		m_bytes += len;
	}
	else
	{
		FlushWrite(mem, len);
	}

	m_mem.m_clut.Invalidate();
}

void GSState::Read(BYTE* mem, int len)
{
	/*
	TRACE(_T("Read len=%d SBP=%05x SBW=%d SPSM=%d SSAX=%d SSAY=%d RRW=%d RRH=%d\n"), 
		  len, (int)m_env.BITBLTBUF.SBP, (int)m_env.BITBLTBUF.SBW, (int)m_env.BITBLTBUF.SPSM, 
		  (int)m_env.TRXPOS.SSAX, (int)m_env.TRXPOS.SSAY,
		  (int)m_env.TRXREG.RRW, (int)m_env.TRXREG.RRH);
	*/

	if(m_y >= (int)m_env.TRXREG.RRH) {ASSERT(0); return;}

	if(m_x == m_env.TRXPOS.SSAX && m_y == m_env.TRXPOS.SSAY)
	{
		CRect r(m_env.TRXPOS.SSAX, m_env.TRXPOS.SSAY, m_env.TRXREG.RRW, m_env.TRXREG.RRH);

		InvalidateLocalMem(m_env.BITBLTBUF, r);
	}

	// TODO

	m_mem.ReadImageX(m_x, m_y, mem, len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);
}

void GSState::Move()
{
	// ffxii uses this to move the top/bottom of the scrolling menus offscreen and then blends them back over the text to create a shading effect
	// guitar hero copies the far end of the board to do a similar blend too

	GSLocalMemory::readPixel rp = GSLocalMemory::m_psm[m_env.BITBLTBUF.SPSM].rp;
	GSLocalMemory::writePixel wp = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wp;

	int sx = m_env.TRXPOS.SSAX;
	int dx = m_env.TRXPOS.DSAX;
	int sy = m_env.TRXPOS.SSAY;
	int dy = m_env.TRXPOS.DSAY;
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;
	int xinc = 1;
	int yinc = 1;

	if(sx < dx) sx += w-1, dx += w-1, xinc = -1;
	if(sy < dy) sy += h-1, dy += h-1, yinc = -1;

	InvalidateLocalMem(m_env.BITBLTBUF, CRect(CPoint(sx, sy), CSize(w, h)));
	InvalidateVideoMem(m_env.BITBLTBUF, CRect(CPoint(dx, dy), CSize(w, h)));

	// TODO: use rowOffset

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_mem.*wp)(dx, dy, (m_mem.*rp)(sx, sy, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW), m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW);
}

void GSState::SoftReset(BYTE mask)
{
	if(mask & 1) memset(&m_path[0], 0, sizeof(GIFPath));
	if(mask & 2) memset(&m_path[1], 0, sizeof(GIFPath));
	if(mask & 4) memset(&m_path[2], 0, sizeof(GIFPath));

	m_env.TRXDIR.XDIR = 3; //-1 ; set it to invalid value

	m_q = 1;
}

void GSState::ReadFIFO(BYTE* mem, int size)
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

template void GSState::Transfer<0>(BYTE* mem, UINT32 size);
template void GSState::Transfer<1>(BYTE* mem, UINT32 size);
template void GSState::Transfer<2>(BYTE* mem, UINT32 size);

template<int index> void GSState::Transfer(BYTE* mem, UINT32 size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	BYTE* start = mem;

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		bool eop = false;

		if(path.tag.NLOOP == 0)
		{
			path.SetTag(mem);

			mem += sizeof(GIFTag);
			size--;

			m_q = 1.0f;

			if(index == 2 && path.tag.EOP)
			{
				m_path3hack = 1;
			}

			if(path.tag.PRE)
			{
				ASSERT(path.tag.FLG != GIF_FLG_IMAGE); // kingdom hearts, ffxii, tales of abyss, berserk

				if((path.tag.FLG & 2) == 0)
				{
					GIFReg r;
					r.i64 = path.tag.PRIM;
					(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
				}
			}

			if(path.tag.EOP)
			{
				eop = true;
			}
			else if(path.tag.NLOOP == 0)
			{
				if(index == 0 && m_nloophack)
				{
					continue;
				}

				eop = true;
			}
		}

		if(path.tag.NLOOP > 0)
		{
			switch(path.tag.FLG)
			{
			case GIF_FLG_PACKED:

				// first try a shortcut for a very common case

				if(path.nreg == 0 && path.tag.NREG == 1 && size >= path.tag.NLOOP && path.GetReg() == GIF_REG_A_D)
				{
					int n = path.tag.NLOOP;

					GIFPackedRegHandlerA_D((GIFPackedReg*)mem, n);

					mem += n * sizeof(GIFPackedReg);
					size -= n;

					path.tag.NLOOP = 0;
				}
				else
				{
					while(size > 0)
					{
						(this->*m_fpGIFPackedRegHandlers[path.GetReg()])((GIFPackedReg*)mem);

						size--;
						mem += sizeof(GIFPackedReg);

						if((++path.nreg & 0xf) == path.tag.NREG) 
						{
							path.nreg = 0; 
							path.tag.NLOOP--;

							if(path.tag.NLOOP == 0)
							{
								break;
							}
						}
					}
				}

				break;

			case GIF_FLG_REGLIST:

				size *= 2;

				while(size > 0)
				{
					(this->*m_fpGIFRegHandlers[path.GetReg()])((GIFReg*)mem);

					size--;
					mem += sizeof(GIFReg);

					if((++path.nreg & 0xf) == path.tag.NREG) 
					{
						path.nreg = 0; 
						path.tag.NLOOP--;

						if(path.tag.NLOOP == 0)
						{
							break;
						}
					}
				}
			
				if(size & 1) mem += sizeof(GIFReg);

				size /= 2;

				break;

			case GIF_FLG_IMAGE2: // hmmm

				ASSERT(0);

				path.tag.NLOOP = 0;

				break;

			case GIF_FLG_IMAGE:
				{
					int len = (int)min(size, path.tag.NLOOP);

					//ASSERT(!(len&3));

					switch(m_env.TRXDIR.XDIR)
					{
					case 0:
						Write(mem, len * 16);
						break;
					case 1: 
						Read(mem, len * 16);
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
					path.tag.NLOOP -= len;
					size -= len;
				}

				break;

			default: 
				__assume(0);
			}
		}

		if(eop && ((int)size <= 0 || index == 0))
		{
			break;
		}
	}

	// FIXME: dq8, pcsx2 error probably

	if(index == 0)
	{
		if(!path.tag.EOP && path.tag.NLOOP > 0)
		{
			path.tag.NLOOP = 0;

			TRACE(_T("path1 hack\n"));
		}
	}

	if(m_dump && mem > start)
	{
		m_dump.Transfer(index, start, mem - start);
	}
}

template<class T> static void WriteState(BYTE*& dst, T* src, size_t len = sizeof(T))
{
	memcpy(dst, src, len);
	dst += len;
}

template<class T> static void ReadState(T* dst, BYTE*& src, size_t len = sizeof(T))
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

	BYTE* data = fd->data;

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
	WriteState(data, &m_env.TRXREG2);

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
	WriteState(data, &m_x);
	WriteState(data, &m_y);
	WriteState(data, m_mem.m_vm8, m_mem.m_vmsize);

	for(int i = 0; i < 3; i++)
	{
		WriteState(data, &m_path[i].tag);
		WriteState(data, &m_path[i].nreg);
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

	BYTE* data = fd->data;

	int version;

	ReadState(&version, data);

	if(version > m_version)
	{
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
	ReadState(&m_env.TRXREG2, data);

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
			data += sizeof(DWORD) * 7; // skip 
		}
	}

	ReadState(&m_v.RGBAQ, data);
	ReadState(&m_v.ST, data);
	ReadState(&m_v.UV, data);
	ReadState(&m_v.XYZ, data);
	ReadState(&m_v.FOG, data);
	ReadState(&m_x, data);
	ReadState(&m_y, data);
	ReadState(m_mem.m_vm8, data, m_mem.m_vmsize);

	for(int i = 0; i < 3; i++)
	{
		ReadState(&m_path[i].tag, data);
		ReadState(&m_path[i].nreg, data);

		m_path[i].SetTag(&m_path[i].tag); // expand regs
	}

	ReadState(&m_q, data);

	PRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateVertexKick();

	m_env.UpdateDIMX();

	m_env.CTXT[0].UpdateScissor();
	m_env.CTXT[1].UpdateScissor();

m_perfmon.SetFrame(5000);

	return 0;
}

void GSState::SetGameCRC(DWORD crc, int options)
{
	m_crc = crc;
	m_options = options;
	m_game = CRC::Lookup(crc);

	if(m_nloophack_org == 2)
	{
		m_nloophack = m_game.nloophack;
	}
}

void GSState::SetFrameSkip(int frameskip)
{
	if(m_frameskip != frameskip)
	{
		m_frameskip = frameskip;

		if(frameskip)
		{
			m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerNOP;
			m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerNOP;
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
			m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
			m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
			m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
			m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
			m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerCLAMP<0>;
			m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerCLAMP<1>;
			m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerXYZF3;
			m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerXYZ3;

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
}

// hacks

struct GSFrameInfo
{
	DWORD FBP;
	DWORD FPSM;
	bool TME;
	DWORD TBP0;
	DWORD TPSM;
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
		if(fi.TME && fi.FBP == 0x00f00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00500 || fi.TBP0 == 0x00000) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 4;
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
	if(skip == 0)
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
	}

	return true;
}

bool GSC_OnePieceGrandAdventure(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 3;
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
	}
	else
	{
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

bool GSState::IsBadFrame(int& skip)
{
	GSFrameInfo fi;

	fi.FBP = m_context->FRAME.Block();
	fi.FPSM = m_context->FRAME.PSM;
	fi.TME = PRIM->TME;
	fi.TBP0 = m_context->TEX0.TBP0;
	fi.TPSM = m_context->TEX0.PSM;

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
		map[CRC::ICO] = GSC_ICO;
		map[CRC::GT4] = GSC_GT4;
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
		map[CRC::GodOfWar2] = GSC_GodOfWar;
		map[CRC::GiTS] = GSC_GiTS;
		map[CRC::Onimusha3] = GSC_Onimusha3;
		map[CRC::TalesOfAbyss] = GSC_TalesOfAbyss;
	}

	// TODO: just set gsc in SetGameCRC once

	GetSkipCount gsc = map[m_game.title];

	if(gsc && !gsc(fi, skip))
	{
		return false;
	}

	if(skip == 0)
	{
		if(fi.TME)
		{
			if(GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM))
			{
				// skip = 1;
			}

			// depth textures (bully, mgs3s1 intro)

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
