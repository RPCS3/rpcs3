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
#include "GSdx.h"

//#define Offset_ST  // Fixes Persona3 mini map alignment which is off even in software rendering
//#define Offset_UV  // Fixes / breaks various titles

GSState::GSState()
	: m_version(6)
	, m_mt(false)
	, m_irq(NULL)
	, m_path3hack(0)
	, m_regs(NULL)
	, m_crc(0)
	, m_options(0)
	, m_frameskip(0)
	, m_vt(this)
	, m_q(1.0f)
{
	m_nativeres = !!theApp.GetConfig("nativeres", 0);

	memset(&m_v, 0, sizeof(m_v));
	memset(&m_vertex, 0, sizeof(m_vertex));
	memset(&m_index, 0, sizeof(m_index));

	m_v.RGBAQ.Q = 1.0f;

	GrowVertexBuffer();

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
	m_sssize += sizeof(m_v.FOG);
	m_sssize += sizeof(m_v.XYZ);
	m_sssize += sizeof(GIFReg); // obsolete

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

	s_n = 0;
	s_dump = !!theApp.GetConfig("dump", 0);
	s_save = !!theApp.GetConfig("save", 0);
	s_savez = !!theApp.GetConfig("savez", 0);
	s_saven = theApp.GetConfig("saven", 0);
}

GSState::~GSState()
{
	if(m_vertex.buff) _aligned_free(m_vertex.buff);
	if(m_index.buff) _aligned_free(m_index.buff);
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
		m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerNOP;

		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerNOP;
		m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerNOP;

		m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZF2] = &GSState::GIFPackedRegHandlerNOP;
		m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZ2] = &GSState::GIFPackedRegHandlerNOP;
	}
	else
	{
		UpdateVertexKick();
	}
}

void GSState::Reset()
{
	printf("GS reset\n");

	// FIXME: memset(m_mem.m_vm8, 0, m_mem.m_vmsize); // bios logo not shown cut in half after reset, missing graphics in GoW after first FMV
	memset(&m_path[0], 0, sizeof(m_path[0]) * countof(m_path));
	memset(&m_v, 0, sizeof(m_v));

//	PRIM = &m_env.PRIM;
//	m_env.PRMODECONT.AC = 1;

	m_env.Reset();

	m_context = &m_env.CTXT[0];

	m_vertex.head = 0;
	m_vertex.tail = 0;
	m_vertex.next = 0;
	m_index.tail = 0;
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
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = (GIFPackedRegHandler)(GIFRegHandler)&GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	#define SetHandlerXYZ(P) \
		m_fpGIFPackedRegHandlerXYZ[P][0] = &GSState::GIFPackedRegHandlerXYZF2<P, 0>; \
		m_fpGIFPackedRegHandlerXYZ[P][1] = &GSState::GIFPackedRegHandlerXYZF2<P, 1>; \
		m_fpGIFPackedRegHandlerXYZ[P][2] = &GSState::GIFPackedRegHandlerXYZ2<P, 0>; \
		m_fpGIFPackedRegHandlerXYZ[P][3] = &GSState::GIFPackedRegHandlerXYZ2<P, 1>; \
		m_fpGIFRegHandlerXYZ[P][0] = &GSState::GIFRegHandlerXYZF2<P, 0>; \
		m_fpGIFRegHandlerXYZ[P][1] = &GSState::GIFRegHandlerXYZF2<P, 1>; \
		m_fpGIFRegHandlerXYZ[P][2] = &GSState::GIFRegHandlerXYZ2<P, 0>; \
		m_fpGIFRegHandlerXYZ[P][3] = &GSState::GIFRegHandlerXYZ2<P, 1>; \
		m_fpGIFPackedRegHandlerSTQRGBAXYZF2[P] = &GSState::GIFPackedRegHandlerSTQRGBAXYZF2<P>; \
		m_fpGIFPackedRegHandlerSTQRGBAXYZ2[P] = &GSState::GIFPackedRegHandlerSTQRGBAXYZ2<P>; \

	SetHandlerXYZ(GS_POINTLIST);
	SetHandlerXYZ(GS_LINELIST);
	SetHandlerXYZ(GS_LINESTRIP);
	SetHandlerXYZ(GS_TRIANGLELIST);
	SetHandlerXYZ(GS_TRIANGLESTRIP);
	SetHandlerXYZ(GS_TRIANGLEFAN);
	SetHandlerXYZ(GS_SPRITE);
	SetHandlerXYZ(GS_INVALID);

	for(size_t i = 0; i < countof(m_fpGIFRegHandlers); i++)
	{
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;
	}

	m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
	m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
	m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
	m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOG] = &GSState::GIFRegHandlerFOG;
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

// There's a problem when games expand/shrink and relocate the visible area since GSdx doesn't support
// moving the output area. (Disgaea 2 intro FMV when upscaling is used, also those games hackfixed below.)
GSVector4i GSState::GetFrameRect(int i)
{
	if(i < 0) i = IsEnabled(1) ? 1 : 0;

	GSVector4i r = GetDisplayRect(i);

	int w = r.width();
	int h = r.height();

	//Fixme: These games have an extra black bar at bottom of screen
	if((m_game.title == CRC::DevilMayCry3 || m_game.title == CRC::SkyGunner) && (m_game.region == CRC::US || m_game.region == CRC::JP))
	{
		h = 448; //
	}
	
	if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD && h > 1) h >>= 1;

	//Breaks Disgaea2 FMV borders
	r.left = m_regs->DISP[i].DISPFB.DBX;
	r.top = m_regs->DISP[i].DISPFB.DBY;
	r.right = r.left + w;
	r.bottom = r.top + h;
	
	/*static GSVector4i old_r = (GSVector4i) 0;
	if ((old_r.left != r.left) || (old_r.right != r.right) || (old_r.top != r.top) || (old_r.right != r.right)){
		printf("w %d  h %d  left %d  top %d  right %d  bottom %d\n",w,h,r.left,r.top,r.right,r.bottom);
	}
	old_r = r;*/

	return r;
}

GSVector2i GSState::GetDeviceSize(int i)
{
	// TODO: return (m_regs->SMODE1.CMOD & 1) ? GSVector2i(640, 576) : GSVector2i(640, 480);

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
	if(m_regs->SMODE2.INT && m_regs->SMODE2.FFMD && h > 1)
	{
		if (IsEnabled(0) || IsEnabled(1))
		{
			h >>= 1;
		}
	}

	//Fixme: These games elude the code above, worked with the old hack
	else if(m_game.title == CRC::SilentHill2 || m_game.title == CRC::SilentHill3)
	{
		h /= 2; 
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

float GSState::GetFPS()
{
	float base_rate = ((m_regs->SMODE1.CMOD & 1) ? 25 : (30/1.001f));
	return base_rate * (m_regs->SMODE2.INT ? 2 : 1);
}

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(const GIFPackedReg* RESTRICT r)
{
	// ASSERT(0);
}

void GSState::GIFPackedRegHandlerRGBA(const GIFPackedReg* RESTRICT r)
{
	#if _M_SSE >= 0x301

	GSVector4i mask = GSVector4i::load(0x0c080400);
	GSVector4i v = GSVector4i::load<false>(r).shuffle8(mask);

	m_v.RGBAQ.u32[0] = (uint32)GSVector4i::store(v);

	#else

	GSVector4i v = GSVector4i::load<false>(r) & GSVector4i::x000000ff();

	m_v.RGBAQ.u32[0] = v.rgba32();

	#endif

	m_v.RGBAQ.Q = m_q;
}

void GSState::GIFPackedRegHandlerSTQ(const GIFPackedReg* RESTRICT r)
{
	#if defined(_M_AMD64)

	m_v.ST.u64 = r->u64[0];

	#else

	GSVector4i v = GSVector4i::loadl(r);
	GSVector4i::storel(&m_v.ST.u64, v);

	#endif

	m_q = r->STQ.Q;
	
#ifdef Offset_ST
	GIFRegTEX0 TEX0 = m_context->TEX0;
	m_v.ST.S -= 0.02f * m_q / (1 << TEX0.TW);
	m_v.ST.T -= 0.02f * m_q / (1 << TEX0.TH);
#endif
}

void GSState::GIFPackedRegHandlerUV(const GIFPackedReg* RESTRICT r)
{
	GSVector4i v = GSVector4i::loadl(r) & GSVector4i::x00003fff();

	m_v.UV = (uint32)GSVector4i::store(v.ps32(v));

#ifdef Offset_UV
	m_v.UV.U = min((uint16)m_v.UV.U, (uint16)(m_v.UV.U - 4U));
	m_v.UV.V = min((uint16)m_v.UV.V, (uint16)(m_v.UV.V - 4U));
#endif
}

template<uint32 prim, uint32 adc>
void GSState::GIFPackedRegHandlerXYZF2(const GIFPackedReg* RESTRICT r)
{
	/*
	m_v.XYZ.X = r->XYZF2.X;
	m_v.XYZ.Y = r->XYZF2.Y;
	m_v.XYZ.Z = r->XYZF2.Z;
	m_v.FOG = r->XYZF2.F;
	*/
	GSVector4i xy = GSVector4i::loadl(&r->u64[0]);
	GSVector4i zf = GSVector4i::loadl(&r->u64[1]);
	xy = xy.upl16(xy.srl<4>()).upl32(GSVector4i::loadl(&m_v.UV));
	zf = zf.srl32(4) & GSVector4i::x00ffffff().upl32(GSVector4i::x000000ff());

	m_v.m[1] = xy.upl32(zf);

	VertexKick<prim>(adc ? 1 : r->XYZF2.Skip());
}

template<uint32 prim, uint32 adc>
void GSState::GIFPackedRegHandlerXYZ2(const GIFPackedReg* RESTRICT r)
{
/*
	m_v.XYZ.X = r->XYZ2.X;
	m_v.XYZ.Y = r->XYZ2.Y;
	m_v.XYZ.Z = r->XYZ2.Z;
*/
	GSVector4i xy = GSVector4i::loadl(&r->u64[0]);
	GSVector4i z = GSVector4i::loadl(&r->u64[1]);
	GSVector4i xyz = xy.upl16(xy.srl<4>()).upl32(z);

	m_v.m[1] = xyz.upl64(GSVector4i::loadl(&m_v.UV));

	VertexKick<prim>(adc ? 1 : r->XYZ2.Skip());
}

void GSState::GIFPackedRegHandlerFOG(const GIFPackedReg* RESTRICT r)
{
	m_v.FOG = r->FOG.F;
}

void GSState::GIFPackedRegHandlerA_D(const GIFPackedReg* RESTRICT r)
{
	(this->*m_fpGIFRegHandlers[r->A_D.ADDR])(&r->r);
}

void GSState::GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r)
{
}

template<uint32 prim>
void GSState::GIFPackedRegHandlerSTQRGBAXYZF2(const GIFPackedReg* RESTRICT r, uint32 size)
{
	ASSERT(size > 0 && size % 3 == 0);

	const GIFPackedReg* RESTRICT r_end = r + size;

	while(r < r_end)
	{
		GSVector4i st = GSVector4i::loadl(&r[0].u64[0]);
		GSVector4i q = GSVector4i::loadl(&r[0].u64[1]);
		GSVector4i rgba = (GSVector4i::load<false>(&r[1]) & GSVector4i::x000000ff()).ps32().pu16();

		m_v.m[0] = st.upl64(rgba.upl32(q)); // TODO: only store the last one

		GSVector4i xy = GSVector4i::loadl(&r[2].u64[0]);
		GSVector4i zf = GSVector4i::loadl(&r[2].u64[1]);
		xy = xy.upl16(xy.srl<4>()).upl32(GSVector4i::loadl(&m_v.UV));
		zf = zf.srl32(4) & GSVector4i::x00ffffff().upl32(GSVector4i::x000000ff());

		m_v.m[1] = xy.upl32(zf); // TODO: only store the last one

		VertexKick<prim>(r[2].XYZF2.Skip());

		r += 3;
	}

	m_q = r[-3].STQ.Q; // remember the last one, STQ outputs this to the temp Q each time
}

template<uint32 prim>
void GSState::GIFPackedRegHandlerSTQRGBAXYZ2(const GIFPackedReg* RESTRICT r, uint32 size)
{
	ASSERT(size > 0 && size % 3 == 0);

	const GIFPackedReg* RESTRICT r_end = r + size;

	while(r < r_end)
	{
		GSVector4i st = GSVector4i::loadl(&r[0].u64[0]);
		GSVector4i q = GSVector4i::loadl(&r[0].u64[1]);
		GSVector4i rgba = (GSVector4i::load<false>(&r[1]) & GSVector4i::x000000ff()).ps32().pu16();

		m_v.m[0] = st.upl64(rgba.upl32(q)); // TODO: only store the last one

		GSVector4i xy = GSVector4i::loadl(&r[2].u64[0]);
		GSVector4i z = GSVector4i::loadl(&r[2].u64[1]);
		GSVector4i xyz = xy.upl16(xy.srl<4>()).upl32(z);

		m_v.m[1] = xyz.upl64(GSVector4i::loadl(&m_v.UV)); // TODO: only store the last one

		VertexKick<prim>(r[2].XYZ2.Skip());

		r += 3;
	}

	m_q = r[-3].STQ.Q; // remember the last one, STQ outputs this to the temp Q each time
}

void GSState::GIFPackedRegHandlerNOP(const GIFPackedReg* RESTRICT r, uint32 size)
{
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(const GIFReg* RESTRICT r)
{
	// ASSERT(0);
}

__forceinline void GSState::ApplyPRIM(uint32 prim)
{
	// ASSERT(r->PRIM.PRIM < 7);

	if(GSUtil::GetPrimClass(m_env.PRIM.PRIM) == GSUtil::GetPrimClass(prim & 7)) // NOTE: assume strips/fans are converted to lists
	{
		if((m_env.PRIM.u32[0] ^ prim) & 0x7f8) // all fields except PRIM
		{
			Flush();
		}
	}
	else
	{
		Flush();
	}

	m_env.PRIM.u32[0] = prim;
	m_env.PRMODE._PRIM = prim;

	UpdateContext();

	UpdateVertexKick();

	ASSERT(m_index.tail == 0 || m_index.buff[m_index.tail - 1] + 1 == m_vertex.next);

	if(m_index.tail == 0)
	{
		m_vertex.next = 0;
	}

	m_vertex.head = m_vertex.tail = m_vertex.next; // remove unused vertices from the end of the vertex buffer
}

void GSState::GIFRegHandlerPRIM(const GIFReg* RESTRICT r)
{
	ALIGN_STACK(32);

	ApplyPRIM(r->PRIM.u32[0]);
}

void GSState::GIFRegHandlerRGBAQ(const GIFReg* RESTRICT r)
{
	m_v.RGBAQ = (GSVector4i)r->RGBAQ;
}

void GSState::GIFRegHandlerST(const GIFReg* RESTRICT r)
{
	m_v.ST = (GSVector4i)r->ST;

#ifdef Offset_ST
	GIFRegTEX0 TEX0 = m_context->TEX0;
	m_v.ST.S -= 0.02f * m_q / (1 << TEX0.TW);
	m_v.ST.T -= 0.02f * m_q / (1 << TEX0.TH);
#endif
}

void GSState::GIFRegHandlerUV(const GIFReg* RESTRICT r)
{
	m_v.UV = r->UV.u32[0] & 0x3fff3fff;

#ifdef Offset_UV
	m_v.UV.U = min((uint16)m_v.UV.U, (uint16)(m_v.UV.U - 4U));
	m_v.UV.V = min((uint16)m_v.UV.V, (uint16)(m_v.UV.V - 4U));
#endif
}

template<uint32 prim, uint32 adc>
void GSState::GIFRegHandlerXYZF2(const GIFReg* RESTRICT r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	
/*
	m_v.XYZ.u32[0] = r->XYZF.u32[0];
	m_v.XYZ.u32[1] = r->XYZF.u32[1] & 0x00ffffff;
	m_v.FOG = r->XYZF.u32[1] >> 24;
*/

	GSVector4i xyzf = GSVector4i::loadl(&r->XYZF);
	GSVector4i xyz = xyzf & (GSVector4i::xffffffff().upl32(GSVector4i::x00ffffff()));
	GSVector4i uvf = GSVector4i::loadl(&m_v.UV).upl32(xyzf.srl32(24).srl<4>());
	
	m_v.m[1] = xyz.upl64(uvf);

	VertexKick<prim>(adc);
}

template<uint32 prim, uint32 adc>
void GSState::GIFRegHandlerXYZ2(const GIFReg* RESTRICT r)
{
	// m_v.XYZ = (GSVector4i)r->XYZ;

	m_v.m[1] = GSVector4i::load(&r->XYZ, &m_v.UV);

	VertexKick<prim>(adc);
}

template<int i> void GSState::ApplyTEX0(GIFRegTEX0& TEX0)
{
	// even if TEX0 did not change, a new palette may have been uploaded and will overwrite the currently queued for drawing

	bool wt = m_mem.m_clut.WriteTest(TEX0, m_env.TEXCLUT);

	// clut loading already covered with WriteTest, for drawing only have to check CPSM and CSA (MGS3 intro skybox would be drawn piece by piece without this)

	uint64 mask = 0x1f78001c3fffffffull; // TBP0 TBW PSM TW TCC TFX CPSM CSA

	if(wt || PRIM->CTXT == i && ((TEX0.u64 ^ m_env.CTXT[i].TEX0.u64) & mask))
	{
		Flush();
	}

	TEX0.CPSM &= 0xa; // 1010b

	if((TEX0.u32[0] ^ m_env.CTXT[i].TEX0.u32[0]) & 0x3ffffff) // TBP0 TBW PSM
	{
		m_env.CTXT[i].offset.tex = m_mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);
	}

	m_env.CTXT[i].TEX0 = (GSVector4i)TEX0;

	if(wt)
	{
		GIFRegBITBLTBUF BITBLTBUF;
		GSVector4i r;

		if(TEX0.CSM == 0)
		{
			BITBLTBUF.SBP = TEX0.CBP;
			BITBLTBUF.SBW = 1;
			BITBLTBUF.SPSM = TEX0.CSM;

			r.left = 0;
			r.top = 0;
			r.right = GSLocalMemory::m_psm[TEX0.CPSM].bs.x;
			r.bottom = GSLocalMemory::m_psm[TEX0.CPSM].bs.y;

			int blocks = 4;

			if(GSLocalMemory::m_psm[TEX0.CPSM].bpp == 16)
			{
				blocks >>= 1;
			}

			if(GSLocalMemory::m_psm[TEX0.PSM].bpp == 4)
			{
				blocks >>= 1;
			}
		
			for(int i = 0; i < blocks; i++, BITBLTBUF.SBP++)
			{
				InvalidateLocalMem(BITBLTBUF, r, true);
			}
		}
		else
		{
			BITBLTBUF.SBP = TEX0.CBP;
			BITBLTBUF.SBW = m_env.TEXCLUT.CBW;
			BITBLTBUF.SPSM = TEX0.CSM;

			r.left = m_env.TEXCLUT.COU;
			r.top = m_env.TEXCLUT.COV;
			r.right = r.left + GSLocalMemory::m_psm[TEX0.CPSM].pal;
			r.bottom = r.top + 1;
		
			InvalidateLocalMem(BITBLTBUF, r, true);
		}

		m_mem.m_clut.Write(m_env.CTXT[i].TEX0, m_env.TEXCLUT);
	}
}

template<int i> void GSState::GIFRegHandlerTEX0(const GIFReg* RESTRICT r)
{
	GIFRegTEX0 TEX0 = r->TEX0;

	// Tokyo Xtreme Racer Drift 2, TW/TH == 0, PRIM->FST == 1
	// Just setting the max texture size to make the texture cache allocate some surface. 
	// The vertex trace will narrow the updated area down to the minimum, upper-left 8x8 
	// for a single letter, but it may address the whole thing if it wants to.

	if(TEX0.TW > 10 || TEX0.TW == 0) TEX0.TW = 10;
	if(TEX0.TH > 10 || TEX0.TH == 0) TEX0.TH = 10;

	if((TEX0.TBW & 1) && (TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT4))
	{
		ASSERT(TEX0.TBW == 1); // TODO

		TEX0.TBW &= ~1; // GS User 2.6
	}

	ApplyTEX0<i>(TEX0);

	if(m_env.CTXT[i].TEX1.MTBA)
	{
		// NOTE 1: TEX1.MXL must not be automatically set to 3 here.
		// NOTE 2: Mipmap levels are tightly packed, if (tbw << 6) > (1 << tw) then the left-over space to the right is used. (common for PSM_PSMT4)
		// NOTE 3: Non-rectangular textures are treated as rectangular when calculating the occupied space (height is extended, not sure about width)

		uint32 bp = TEX0.TBP0;
		uint32 bw = TEX0.TBW;
		uint32 w = 1u << TEX0.TW;
		uint32 h = 1u << TEX0.TH;
		uint32 bpp = GSLocalMemory::m_psm[TEX0.PSM].bpp;

		if(h < w) h = w;

		bp += ((w * h * bpp >> 3) + 255) >> 8;
		bw = std::max<uint32>(bw >> 1, 1);
		w = std::max<uint32>(w >> 1, 1);
		h = std::max<uint32>(h >> 1, 1);

		m_env.CTXT[i].MIPTBP1.TBP1 = bp;
		m_env.CTXT[i].MIPTBP1.TBW1 = bw;

		bp += ((w * h * bpp >> 3) + 255) >> 8;
		bw = std::max<uint32>(bw >> 1, 1);
		w = std::max<uint32>(w >> 1, 1);
		h = std::max<uint32>(h >> 1, 1);

		m_env.CTXT[i].MIPTBP1.TBP2 = bp;
		m_env.CTXT[i].MIPTBP1.TBW2 = bw;

		bp += ((w * h * bpp >> 3) + 255) >> 8;
		bw = std::max<uint32>(bw >> 1, 1);
		w = std::max<uint32>(w >> 1, 1);
		h = std::max<uint32>(h >> 1, 1);

		m_env.CTXT[i].MIPTBP1.TBP3 = bp;
		m_env.CTXT[i].MIPTBP1.TBW3 = bw;

		// printf("MTBA\n");
	}
}

template<int i> void GSState::GIFRegHandlerCLAMP(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->CLAMP != m_env.CTXT[i].CLAMP)
	{
		Flush();
	}

	m_env.CTXT[i].CLAMP = (GSVector4i)r->CLAMP;
}

void GSState::GIFRegHandlerFOG(const GIFReg* RESTRICT r)
{
	m_v.FOG = r->FOG.F;
}

void GSState::GIFRegHandlerNOP(const GIFReg* RESTRICT r)
{
}

template<int i> void GSState::GIFRegHandlerTEX1(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->TEX1 != m_env.CTXT[i].TEX1)
	{
		Flush();
	}

	m_env.CTXT[i].TEX1 = (GSVector4i)r->TEX1;
}

template<int i> void GSState::GIFRegHandlerTEX2(const GIFReg* RESTRICT r)
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

	ApplyTEX0<i>(TEX0);
}

template<int i> void GSState::GIFRegHandlerXYOFFSET(const GIFReg* RESTRICT r)
{
	GSVector4i o = (GSVector4i)r->XYOFFSET & GSVector4i::x0000ffff();

	if(!o.eq(m_env.CTXT[i].XYOFFSET))
	{
		Flush();
	}

	m_env.CTXT[i].XYOFFSET = o;

	m_env.CTXT[i].UpdateScissor();

	UpdateScissor();
}

void GSState::GIFRegHandlerPRMODECONT(const GIFReg* RESTRICT r)
{
	if(r->PRMODECONT != m_env.PRMODECONT)
	{
		Flush();
	}

	m_env.PRMODECONT.AC = r->PRMODECONT.AC;

	PRIM = m_env.PRMODECONT.AC ? &m_env.PRIM : (GIFRegPRIM*)&m_env.PRMODE;

	// if(PRIM->PRIM == 7) printf("Invalid PRMODECONT/PRIM\n");

	UpdateContext();

	UpdateVertexKick();
}

void GSState::GIFRegHandlerPRMODE(const GIFReg* RESTRICT r)
{
	if(!m_env.PRMODECONT.AC)
	{
		Flush();
	}

	uint32 _PRIM = m_env.PRMODE._PRIM;
	m_env.PRMODE = (GSVector4i)r->PRMODE;
	m_env.PRMODE._PRIM = _PRIM;

	UpdateContext();

	UpdateVertexKick();
}

void GSState::GIFRegHandlerTEXCLUT(const GIFReg* RESTRICT r)
{
	if(r->TEXCLUT != m_env.TEXCLUT)
	{
		Flush();
	}

	m_env.TEXCLUT = (GSVector4i)r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(const GIFReg* RESTRICT r)
{
	if(r->SCANMSK != m_env.SCANMSK)
	{
		Flush();
	}

	m_env.SCANMSK = (GSVector4i)r->SCANMSK;
}

template<int i> void GSState::GIFRegHandlerMIPTBP1(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->MIPTBP1 != m_env.CTXT[i].MIPTBP1)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP1 = (GSVector4i)r->MIPTBP1;
}

template<int i> void GSState::GIFRegHandlerMIPTBP2(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->MIPTBP2 != m_env.CTXT[i].MIPTBP2)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP2 = (GSVector4i)r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(const GIFReg* RESTRICT r)
{
	if(r->TEXA != m_env.TEXA)
	{
		Flush();
	}

	m_env.TEXA = (GSVector4i)r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(const GIFReg* RESTRICT r)
{
	if(r->FOGCOL != m_env.FOGCOL)
	{
		Flush();
	}

	m_env.FOGCOL = (GSVector4i)r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(const GIFReg* RESTRICT r)
{
	// TRACE(_T("TEXFLUSH\n"));
}

template<int i> void GSState::GIFRegHandlerSCISSOR(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->SCISSOR != m_env.CTXT[i].SCISSOR)
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = (GSVector4i)r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();

	UpdateScissor();
}

template<int i> void GSState::GIFRegHandlerALPHA(const GIFReg* RESTRICT r)
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

void GSState::GIFRegHandlerDIMX(const GIFReg* RESTRICT r)
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

void GSState::GIFRegHandlerDTHE(const GIFReg* RESTRICT r)
{
	if(r->DTHE != m_env.DTHE)
	{
		Flush();
	}

	m_env.DTHE = (GSVector4i)r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(const GIFReg* RESTRICT r)
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

template<int i> void GSState::GIFRegHandlerTEST(const GIFReg* RESTRICT r)
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

void GSState::GIFRegHandlerPABE(const GIFReg* RESTRICT r)
{
	if(r->PABE != m_env.PABE)
	{
		Flush();
	}

	m_env.PABE = (GSVector4i)r->PABE;
}

template<int i> void GSState::GIFRegHandlerFBA(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->FBA != m_env.CTXT[i].FBA)
	{
		Flush();
	}

	m_env.CTXT[i].FBA = (GSVector4i)r->FBA;
}

template<int i> void GSState::GIFRegHandlerFRAME(const GIFReg* RESTRICT r)
{
	if(PRIM->CTXT == i && r->FRAME != m_env.CTXT[i].FRAME)
	{
		Flush();
	}

	if((m_env.CTXT[i].FRAME.u32[0] ^ r->FRAME.u32[0]) & 0x3f3f01ff) // FBP FBW PSM
	{
		m_env.CTXT[i].offset.fb = m_mem.GetOffset(r->FRAME.Block(), r->FRAME.FBW, r->FRAME.PSM);
		m_env.CTXT[i].offset.zb = m_mem.GetOffset(m_env.CTXT[i].ZBUF.Block(), r->FRAME.FBW, m_env.CTXT[i].ZBUF.PSM);
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset(r->FRAME, m_env.CTXT[i].ZBUF);
		m_env.CTXT[i].offset.fzb4 = m_mem.GetPixelOffset4(r->FRAME, m_env.CTXT[i].ZBUF);
	}
	
	m_env.CTXT[i].FRAME = (GSVector4i)r->FRAME;
#ifdef DISABLE_BITMASKING
	m_env.CTXT[i].FRAME.FBMSK = GSVector4i::store(GSVector4i::load((int)m_env.CTXT[i].FRAME.FBMSK).eq8(GSVector4i::xffffffff()));
#endif
}

template<int i> void GSState::GIFRegHandlerZBUF(const GIFReg* RESTRICT r)
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
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset(m_env.CTXT[i].FRAME, ZBUF);
		m_env.CTXT[i].offset.fzb4 = m_mem.GetPixelOffset4(m_env.CTXT[i].FRAME, ZBUF);
	}

	m_env.CTXT[i].ZBUF = (GSVector4i)ZBUF;
}

void GSState::GIFRegHandlerBITBLTBUF(const GIFReg* RESTRICT r)
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

void GSState::GIFRegHandlerTRXPOS(const GIFReg* RESTRICT r)
{
	if(r->TRXPOS != m_env.TRXPOS)
	{
		FlushWrite();
	}

	m_env.TRXPOS = (GSVector4i)r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(const GIFReg* RESTRICT r)
{
	if(r->TRXREG != m_env.TRXREG)
	{
		FlushWrite();
	}

	m_env.TRXREG = (GSVector4i)r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(const GIFReg* RESTRICT r)
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
	default:
		__assume(0);
	}
}

void GSState::GIFRegHandlerHWREG(const GIFReg* RESTRICT r)
{
	ASSERT(m_env.TRXDIR.XDIR == 0); // host => local

	Write((uint8*)r, 8); // haunting ground
}

void GSState::GIFRegHandlerSIGNAL(const GIFReg* RESTRICT r)
{
	m_regs->SIGLBLID.SIGID = (m_regs->SIGLBLID.SIGID & ~r->SIGNAL.IDMSK) | (r->SIGNAL.ID & r->SIGNAL.IDMSK);

	if(m_regs->CSR.wSIGNAL) m_regs->CSR.rSIGNAL = 1;
	if(!m_regs->IMR.SIGMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerFINISH(const GIFReg* RESTRICT r)
{
	if(m_regs->CSR.wFINISH) m_regs->CSR.rFINISH = 1;
	if(!m_regs->IMR.FINISHMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerLABEL(const GIFReg* RESTRICT r)
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

	GSVector4i r;

	r.left = m_env.TRXPOS.DSAX;
	r.top = m_env.TRXPOS.DSAY;
	r.right = r.left + m_env.TRXREG.RRW;
	r.bottom = r.top + m_env.TRXREG.RRH;

	InvalidateVideoMem(m_env.BITBLTBUF, r);
	
	//int y = m_tr.y;

	GSLocalMemory::writeImage wi = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wi;

	(m_mem.*wi)(m_tr.x, m_tr.y, &m_tr.buff[m_tr.start], len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

	m_tr.start += len;

	m_perfmon.Put(GSPerfMon::Swizzle, len);

	/*
	GSVector4i r;

	r.left = m_env.TRXPOS.DSAX;
	r.top = y;
	r.right = r.left + m_env.TRXREG.RRW;
	r.bottom = std::min<int>(r.top + m_env.TRXREG.RRH, m_tr.x == r.left ? m_tr.y : m_tr.y + 1);

	InvalidateVideoMem(m_env.BITBLTBUF, r);
	*/
/*
	static int n = 0;
	string s;
	s = format("c:\\temp1\\[%04d]_%05x_%d_%d_%d_%d_%d_%d.bmp",
		n++, (int)m_env.BITBLTBUF.DBP, (int)m_env.BITBLTBUF.DBW, (int)m_env.BITBLTBUF.DPSM,
		r.left, r.top, r.right, r.bottom);
	m_mem.SaveBMP(s, m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM, r.right, r.bottom);
*/
}

void GSState::FlushPrim()
{
	if(m_index.tail > 0)
	{
		GSVertex buff[2];

		size_t head = m_vertex.head;
		size_t tail = m_vertex.tail;
		size_t next = m_vertex.next;
		size_t unused = 0;

		if(tail > head)
		{
			switch(PRIM->PRIM)
			{
			case GS_POINTLIST:
				ASSERT(0);
				break;
			case GS_LINELIST:
			case GS_LINESTRIP:
			case GS_SPRITE:
			case GS_TRIANGLELIST:
			case GS_TRIANGLESTRIP:
				unused = tail - head;
				memcpy(buff, &m_vertex.buff[head], sizeof(GSVertex) * unused);
				break;
			case GS_TRIANGLEFAN:
				buff[0] = m_vertex.buff[head]; unused = 1;
				if(tail - 1 > head) {buff[1] = m_vertex.buff[tail - 1]; unused = 2;}
				break;
			case GS_INVALID:
				break;
			default:
				__assume(0);
			}
				
			ASSERT(unused < GSUtil::GetVertexCount(PRIM->PRIM));
		}

		if(GSLocalMemory::m_psm[m_context->FRAME.PSM].fmt < 3 && GSLocalMemory::m_psm[m_context->ZBUF.PSM].fmt < 3)
		{
			// FIXME: berserk fpsm = 27 (8H)

			m_vt.Update(m_vertex.buff, m_index.buff, m_index.tail, GSUtil::GetPrimClass(PRIM->PRIM));

			Draw();

			m_perfmon.Put(GSPerfMon::Draw, 1);
			m_perfmon.Put(GSPerfMon::Prim, m_index.tail / GSUtil::GetVertexCount(PRIM->PRIM));
		}

		m_index.tail = 0;

		m_vertex.head = 0;

		if(unused > 0)
		{
			memcpy(m_vertex.buff, buff, sizeof(GSVertex) * unused);

			m_vertex.tail = unused;
			m_vertex.next = next > head ? next - head : 0;
		}
		else
		{
			m_vertex.tail = 0;
			m_vertex.next = 0;
		}
	}
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

		GSVector4i r;

		r.left = m_env.TRXPOS.DSAX;
		r.top = m_env.TRXPOS.DSAY;
		r.right = r.left + m_env.TRXREG.RRW;
		r.bottom = r.top + m_env.TRXREG.RRH;

		InvalidateVideoMem(m_env.BITBLTBUF, r);

		(m_mem.*psm.wi)(m_tr.x, m_tr.y, mem, m_tr.total, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

		m_tr.start = m_tr.end = m_tr.total;

		m_perfmon.Put(GSPerfMon::Swizzle, len);

		/*
		static int n = 0;
		string s;
		s = format("c:\\temp1\\[%04d]_%05x_%d_%d_%d_%d_%d_%d.bmp",
			n++, (int)m_env.BITBLTBUF.DBP, (int)m_env.BITBLTBUF.DBW, (int)m_env.BITBLTBUF.DPSM,
			r.left, r.top, r.right, r.bottom);
		m_mem.SaveBMP(s, m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW, m_env.BITBLTBUF.DPSM, r.right, r.bottom);
		*/
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

	m_q = 1.0f;
}

void GSState::ReadFIFO(uint8* mem, int size)
{
	GSPerfMonAutoTimer pmat(&m_perfmon);

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

static hash_map<uint64, uint64> s_tags;

template<int index> void GSState::Transfer(const uint8* mem, uint32 size)
{
	GSPerfMonAutoTimer pmat(&m_perfmon);

	const uint8* start = mem;

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		if(path.nloop == 0)
		{
			path.SetTag(mem);

			if(0)
			{
				GIFTag* t = (GIFTag*)mem;
				uint64 hash;
				if(t->NREG < 8) hash = t->u32[2] & ((1 << t->NREG * 4) - 1);
				else if(t->NREG < 16) {hash = t->u32[2]; ((uint32*)&hash)[1] = t->u32[3] & ((1 << (t->NREG - 8) * 4) - 1);}
				else hash = t->u64[1];
				s_tags[hash] += path.nloop * path.nreg;
			}

			mem += sizeof(GIFTag);
			size--;

			if(path.nloop > 0) // eeuser 7.2.2. GIFtag: "... when NLOOP is 0, the GIF does not output anything, and values other than the EOP field are disregarded."
			{
				m_q = 1.0f;

				// ASSERT(!(path.tag.PRE && path.tag.FLG == GIF_FLG_REGLIST)); // kingdom hearts

				if(path.tag.PRE && path.tag.FLG == GIF_FLG_PACKED)
				{
					ApplyPRIM(path.tag.PRIM);
				}
			}
		}
		else
		{
			uint32 total;

			switch(path.tag.FLG)
			{
			case GIF_FLG_PACKED:

				// get to the start of the loop

				if(path.reg != 0)
				{
					do
					{
						(this->*m_fpGIFPackedRegHandlers[path.GetReg()])((GIFPackedReg*)mem);

						mem += sizeof(GIFPackedReg);
						size--;
					}
					while(path.StepReg() && size > 0 && path.reg != 0);
				}

				// all data available? usually is

				total = path.nloop * path.nreg;

				if(size >= total)
				{
					size -= total;

					switch(path.type)
					{
					case GIFPath::TYPE_UNKNOWN:

						{
							uint32 reg = 0;

							do
							{
								(this->*m_fpGIFPackedRegHandlers[path.GetReg(reg++)])((GIFPackedReg*)mem);

								mem += sizeof(GIFPackedReg);

								reg = reg & ((int)(reg - path.nreg) >> 31); // resets reg back to 0 when it becomes equal to path.nreg
							}
							while(--total > 0);
						}

						break;

					case GIFPath::TYPE_ADONLY: // very common

						do
						{
							(this->*m_fpGIFRegHandlers[((GIFPackedReg*)mem)->A_D.ADDR])(&((GIFPackedReg*)mem)->r);

							mem += sizeof(GIFPackedReg);
						}
						while(--total > 0);

						break;
					
					case GIFPath::TYPE_STQRGBAXYZF2: // majority of the vertices are formatted like this

						(this->*m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZF2])((GIFPackedReg*)mem, total);

						mem += total * sizeof(GIFPackedReg);

						break;

					case GIFPath::TYPE_STQRGBAXYZ2:

						(this->*m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZ2])((GIFPackedReg*)mem, total);

						mem += total * sizeof(GIFPackedReg);

						break;

					default:

						__assume(0);
					}

					path.nloop = 0;
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

				// TODO: do it similar to packed operation

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
	WriteState(data, &m_v.FOG);
	WriteState(data, &m_v.XYZ);
	data += sizeof(GIFReg); // obsolite
	WriteState(data, &m_tr.x);
	WriteState(data, &m_tr.y);
	WriteState(data, m_mem.m_vm8, m_mem.m_vmsize);

	for(size_t i = 0; i < countof(m_path); i++)
	{
		m_path[i].tag.NREG = m_path[i].nreg;
		m_path[i].tag.NLOOP = m_path[i].nloop;
		m_path[i].tag.REGS = 0;

		for(size_t j = 0; j < countof(m_path[i].regs.u8); j++)
		{
			m_path[i].tag.u32[2 + (j >> 3)] |= m_path[i].regs.u8[j] << ((j & 7) << 2);
		}

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
	ReadState(&m_v.FOG, data);
	ReadState(&m_v.XYZ, data);
	data += sizeof(GIFReg); // obsolite
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

	UpdateContext();

	UpdateVertexKick();

	m_env.UpdateDIMX();

	for(size_t i = 0; i < 2; i++)
	{
		m_env.CTXT[i].UpdateScissor();

		m_env.CTXT[i].offset.fb = m_mem.GetOffset(m_env.CTXT[i].FRAME.Block(), m_env.CTXT[i].FRAME.FBW, m_env.CTXT[i].FRAME.PSM);
		m_env.CTXT[i].offset.zb = m_mem.GetOffset(m_env.CTXT[i].ZBUF.Block(), m_env.CTXT[i].FRAME.FBW, m_env.CTXT[i].ZBUF.PSM);
		m_env.CTXT[i].offset.tex = m_mem.GetOffset(m_env.CTXT[i].TEX0.TBP0, m_env.CTXT[i].TEX0.TBW, m_env.CTXT[i].TEX0.PSM);
		m_env.CTXT[i].offset.fzb = m_mem.GetPixelOffset(m_env.CTXT[i].FRAME, m_env.CTXT[i].ZBUF);
		m_env.CTXT[i].offset.fzb4 = m_mem.GetPixelOffset4(m_env.CTXT[i].FRAME, m_env.CTXT[i].ZBUF);
	}

	UpdateScissor();

m_perfmon.SetFrame(5000);

	return 0;
}

void GSState::SetGameCRC(uint32 crc, int options)
{
	m_crc = crc;
	m_options = options;
	m_game = CRC::Lookup(crc);
}

//

void GSState::UpdateContext()
{
	m_context = &m_env.CTXT[PRIM->CTXT];

	UpdateScissor();
}

void GSState::UpdateScissor()
{
	m_scissor = m_context->scissor.ofex;
	m_ofxy = m_context->scissor.ofxy;
}

void GSState::UpdateVertexKick() 
{
	if(m_frameskip) return;

	uint32 prim = PRIM->PRIM;

	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = m_fpGIFPackedRegHandlerXYZ[prim][0];
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = m_fpGIFPackedRegHandlerXYZ[prim][1];
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = m_fpGIFPackedRegHandlerXYZ[prim][2];
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = m_fpGIFPackedRegHandlerXYZ[prim][3];

	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = m_fpGIFRegHandlerXYZ[prim][0];
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = m_fpGIFRegHandlerXYZ[prim][1];
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = m_fpGIFRegHandlerXYZ[prim][2];
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = m_fpGIFRegHandlerXYZ[prim][3];

	m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZF2] = m_fpGIFPackedRegHandlerSTQRGBAXYZF2[prim];
	m_fpGIFPackedRegHandlersC[GIF_REG_STQRGBAXYZ2] = m_fpGIFPackedRegHandlerSTQRGBAXYZ2[prim];
}

void GSState::GrowVertexBuffer()
{
	int maxcount = std::max<int>(m_vertex.maxcount * 3 / 2, 10000);

	GSVertex* vertex = (GSVertex*)_aligned_malloc(sizeof(GSVertex) * maxcount, 16);
	uint32* index = (uint32*)_aligned_malloc(sizeof(uint32) * maxcount * 3, 16); // worst case is slightly less than vertex number * 3

	if(m_vertex.buff != NULL)
	{
		memcpy(vertex, m_vertex.buff, sizeof(GSVertex) * m_vertex.tail);

		_aligned_free(m_vertex.buff);
	}

	if(m_index.buff != NULL)
	{
		memcpy(index, m_index.buff, sizeof(uint32) * m_index.tail);
		
		_aligned_free(m_index.buff);
	}

	m_vertex.buff = vertex;
	m_vertex.maxcount = maxcount - 3; // -3 to have some space at the end of the buffer before DrawingKick can grow it
	m_index.buff = index;
}

template<uint32 prim> 
__forceinline void GSState::VertexKick(uint32 skip)
{
	ASSERT(m_vertex.tail < m_vertex.maxcount);

	size_t head = m_vertex.head;
	size_t tail = m_vertex.tail;
	size_t next = m_vertex.next;
	size_t xy_tail = m_vertex.xy_tail;

	// callers should write XYZUVF to m_v.m[1] in one piece to have this load store-forwarded, either by the cpu or the compiler when this function is inlined

	GSVector4i v0(m_v.m[0]);
	GSVector4i v1(m_v.m[1]); 

	GSVector4i* RESTRICT tailptr = (GSVector4i*)&m_vertex.buff[tail];

	tailptr[0] = v0;
	tailptr[1] = v1;

	m_vertex.xy[xy_tail & 3] = GSVector4(v1.upl32(v1.sub16(GSVector4i::load(m_ofxy)).sra16(4)).upl16()); // zw not sign extended, only useful for eq tests

	m_vertex.tail = ++tail;
	m_vertex.xy_tail = ++xy_tail;

	size_t n = 0;

	switch(prim)
	{
	case GS_POINTLIST: n = 1; break;
	case GS_LINELIST: n = 2; break;
	case GS_LINESTRIP: n = 2; break;
	case GS_TRIANGLELIST: n = 3; break;
	case GS_TRIANGLESTRIP: n = 3; break;
	case GS_TRIANGLEFAN: n = 3; break;
	case GS_SPRITE: n = 2; break;
	case GS_INVALID: n = 1; break;
	}

	size_t m = tail - head;

	if(m < n)
	{
		return;
	}

	if(skip == 0 && (prim != GS_TRIANGLEFAN || m <= 4)) // m_vertex.xy only knows about the last 4 vertices, head could be far behind for fan
	{
		GSVector4 v0, v1, v2, v3;

		v0 = m_vertex.xy[(xy_tail + 1) & 3]; // T-3
		v1 = m_vertex.xy[(xy_tail + 2) & 3]; // T-2
		v2 = m_vertex.xy[(xy_tail + 3) & 3]; // T-1
		v3 = m_vertex.xy[(xy_tail - m) & 3]; // H

		GSVector4 pmin, pmax, cross;

		switch(prim)
		{
		case GS_POINTLIST:
			pmin = v2;
			pmax = v2;
			break;
		case GS_LINELIST:
		case GS_LINESTRIP:
		case GS_SPRITE:
			pmin = v2.min(v1);
			pmax = v2.max(v1);
			break;
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
			pmin = v2.min(v1.min(v0));
			pmax = v2.max(v1.max(v0));
			break;
		case GS_TRIANGLEFAN:
			pmin = v2.min(v1.min(v3));
			pmax = v2.max(v1.max(v3));
			break;
		}

		GSVector4 test = pmax < m_scissor | pmin > m_scissor.zwxy(); 
		
		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
		case GS_TRIANGLEFAN:
		case GS_SPRITE:
			test |= m_nativeres ? (pmin == pmax).zwzw() : pmin == pmax;
			break;
		}

		switch(prim)
		{
		case GS_TRIANGLELIST:
		case GS_TRIANGLESTRIP:
			cross = (v2 - v1) * (v2 - v0).yxwz();
			test |= cross == cross.yxwz();
			break;
		case GS_TRIANGLEFAN:
			cross = (v2 - v1) * (v2 - v3).yxwz();
			test |= cross == cross.yxwz();
			break;
		}
		
		skip |= test.mask() & 3;
	}

	if(skip != 0)
	{
		switch(prim)
		{
		case GS_POINTLIST:
		case GS_LINELIST:
		case GS_TRIANGLELIST:
		case GS_SPRITE:
		case GS_INVALID: 
			m_vertex.tail = head; // no need to check or grow the buffer length
			break;
		case GS_LINESTRIP:
		case GS_TRIANGLESTRIP:
			m_vertex.head = head + 1;
			// fall through
		case GS_TRIANGLEFAN:
			if(tail >= m_vertex.maxcount) GrowVertexBuffer(); // in case too many vertices were skipped
			break;
		default: 
			__assume(0);
		}

		return;
	}

	if(tail >= m_vertex.maxcount) GrowVertexBuffer();

	uint32* RESTRICT buff = &m_index.buff[m_index.tail];

	switch(prim)
	{
	case GS_POINTLIST:
		buff[0] = head + 0;
		m_vertex.head = head + 1;
		m_vertex.next = head + 1;
		m_index.tail += 1;
		break;
	case GS_LINELIST:
		buff[0] = head + 0;
		buff[1] = head + 1;
		m_vertex.head = head + 2;
		m_vertex.next = head + 2;
		m_index.tail += 2;
		break;
	case GS_LINESTRIP:
		if(next < head) 
		{
			m_vertex.buff[next + 0] = m_vertex.buff[head + 0];
			m_vertex.buff[next + 1] = m_vertex.buff[head + 1];
			head = next; 
			m_vertex.tail = next + 2;
		}
		buff[0] = head + 0;
		buff[1] = head + 1;
		m_vertex.head = head + 1;
		m_vertex.next = head + 2;
		m_index.tail += 2;
		break;
	case GS_TRIANGLELIST:
		buff[0] = head + 0;
		buff[1] = head + 1;
		buff[2] = head + 2;
		m_vertex.head = head + 3;
		m_vertex.next = head + 3;
		m_index.tail += 3;
		break;
	case GS_TRIANGLESTRIP:
		if(next < head) 
		{
			m_vertex.buff[next + 0] = m_vertex.buff[head + 0];
			m_vertex.buff[next + 1] = m_vertex.buff[head + 1];
			m_vertex.buff[next + 2] = m_vertex.buff[head + 2];
			head = next; 
			m_vertex.tail = next + 3;
		}
		buff[0] = head + 0;
		buff[1] = head + 1;
		buff[2] = head + 2;
		m_vertex.head = head + 1;
		m_vertex.next = head + 3;
		m_index.tail += 3;
		break;
	case GS_TRIANGLEFAN:
		// TODO: remove gaps, next == head && head < tail - 3 || next > head && next < tail - 2 (very rare)
		buff[0] = head + 0;
		buff[1] = tail - 2;
		buff[2] = tail - 1;
		m_vertex.next = tail;
		m_index.tail += 3;
		break;
	case GS_SPRITE:	
		buff[0] = head + 0;
		buff[1] = head + 1;
		m_vertex.head = head + 2;
		m_vertex.next = head + 2;
		m_index.tail += 2;
		break;
	case GS_INVALID:
		m_vertex.tail = head;
		break;
	default:
		__assume(0);
	}
}

void GSState::GetTextureMinMax(GSVector4i& r, const GIFRegTEX0& TEX0, const GIFRegCLAMP& CLAMP, bool linear)
{
	int tw = TEX0.TW;
	int th = TEX0.TH;

	int w = 1 << tw;
	int h = 1 << th;

	GSVector4i tr(0, 0, w, h);

	int wms = CLAMP.WMS;
	int wmt = CLAMP.WMT;

	int minu = (int)CLAMP.MINU;
	int minv = (int)CLAMP.MINV;
	int maxu = (int)CLAMP.MAXU;
	int maxv = (int)CLAMP.MAXV;

	GSVector4i vr = tr;

	switch(wms)
	{
	case CLAMP_REPEAT:
		break;
	case CLAMP_CLAMP:
		break;
	case CLAMP_REGION_CLAMP:
		if(vr.x < minu) vr.x = minu;
		if(vr.z > maxu + 1) vr.z = maxu + 1;
		break;
	case CLAMP_REGION_REPEAT:
		vr.x = maxu;
		vr.z = vr.x + (minu + 1);
		break;
	default:
		__assume(0);
	}

	switch(wmt)
	{
	case CLAMP_REPEAT:
		break;
	case CLAMP_CLAMP:
		break;
	case CLAMP_REGION_CLAMP:
		if(vr.y < minv) vr.y = minv;
		if(vr.w > maxv + 1) vr.w = maxv + 1;
		break;
	case CLAMP_REGION_REPEAT:
		vr.y = maxv;
		vr.w = vr.y + (minv + 1);
		break;
	default:
		__assume(0);
	}

	if(wms + wmt < 6)
	{
		GSVector4 st = m_vt.m_min.t.xyxy(m_vt.m_max.t);

		if(linear)
		{
			st += GSVector4(-0x8000, 0x8000).xxyy();
		}

		GSVector4i uv = GSVector4i(st).sra32(16);

		GSVector4i u, v;

		int mask = 0;

		if(wms == CLAMP_REPEAT || wmt == CLAMP_REPEAT)
		{
			u = uv & GSVector4i::xffffffff().srl32(32 - tw);
			v = uv & GSVector4i::xffffffff().srl32(32 - th);

			GSVector4i uu = uv.sra32(tw);
			GSVector4i vv = uv.sra32(th);

			mask = (uu.upl32(vv) == uu.uph32(vv)).mask();
		}

		uv = uv.rintersect(tr);

		switch(wms)
		{
		case CLAMP_REPEAT:
			if(mask & 0x000f) {if(vr.x < u.x) vr.x = u.x; if(vr.z > u.z + 1) vr.z = u.z + 1;}
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			if(vr.x < uv.x) vr.x = uv.x;
			if(vr.z > uv.z + 1) vr.z = uv.z + 1;
			break;
		case CLAMP_REGION_REPEAT:
			break;
		default:
			__assume(0);
		}

		switch(wmt)
		{
		case CLAMP_REPEAT:
			if(mask & 0xf000) {if(vr.y < v.y) vr.y = v.y; if(vr.w > v.w + 1) vr.w = v.w + 1;}
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			if(vr.y < uv.y) vr.y = uv.y;
			if(vr.w > uv.w + 1) vr.w = uv.w + 1;
			break;
		case CLAMP_REGION_REPEAT:
			break;
		default:
			__assume(0);
		}
	}

	vr = vr.rintersect(tr);

	if(vr.rempty())
	{
		// NOTE: this can happen when texcoords are all outside the texture or clamping area is zero, but we can't 
		// let the texture cache update nothing, the sampler will still need a single texel from the border somewhere
		// examples: 
		// - ICO opening menu (texture looks like the miniature silhouette of everything except the sky)
		// - THPS (no visible problems)
		// - NFSMW (strange rectangles on screen, might be unrelated)

		vr = (vr + GSVector4i(-1, +1).xxyy()).rintersect(tr);
	}

	r = vr;
}

void GSState::GetAlphaMinMax()
{
	if(m_vt.m_alpha.valid)
	{
		return;
	}

	const GSDrawingEnvironment& env = m_env;
	const GSDrawingContext* context = m_context;

	GSVector4i a = m_vt.m_min.c.uph32(m_vt.m_max.c).zzww();

	if(PRIM->TME && context->TEX0.TCC)
	{
		switch(GSLocalMemory::m_psm[context->TEX0.PSM].fmt)
		{
		case 0:
			a.y = 0;
			a.w = 0xff;
			break;
		case 1:
			a.y = env.TEXA.AEM ? 0 : env.TEXA.TA0;
			a.w = env.TEXA.TA0;
			break;
		case 2:
			a.y = env.TEXA.AEM ? 0 : min(env.TEXA.TA0, env.TEXA.TA1);
			a.w = max(env.TEXA.TA0, env.TEXA.TA1);
			break;
		case 3:
			m_mem.m_clut.GetAlphaMinMax32(a.y, a.w);
			break;
		default:
			__assume(0);
		}

		switch(context->TEX0.TFX)
		{
		case TFX_MODULATE:
			a.x = (a.x * a.y) >> 7;
			a.z = (a.z * a.w) >> 7;
			if(a.x > 0xff) a.x = 0xff;
			if(a.z > 0xff) a.z = 0xff;
			break;
		case TFX_DECAL:
			a.x = a.y;
			a.z = a.w;
			break;
		case TFX_HIGHLIGHT:
			a.x = a.x + a.y;
			a.z = a.z + a.w;
			if(a.x > 0xff) a.x = 0xff;
			if(a.z > 0xff) a.z = 0xff;
			break;
		case TFX_HIGHLIGHT2:
			a.x = a.y;
			a.z = a.w;
			break;
		default:
			__assume(0);
		}
	}

	m_vt.m_alpha.min = a.x;
	m_vt.m_alpha.max = a.z;
	m_vt.m_alpha.valid = true;
}

bool GSState::TryAlphaTest(uint32& fm, uint32& zm)
{
	const GSDrawingContext* context = m_context;

	bool pass = true;

	if(context->TEST.ATST == ATST_NEVER)
	{
		pass = false;
	}
	else if(context->TEST.ATST != ATST_ALWAYS)
	{
		GetAlphaMinMax();

		int amin = m_vt.m_alpha.min;
		int amax = m_vt.m_alpha.max;

		int aref = context->TEST.AREF;

		switch(context->TEST.ATST)
		{
		case ATST_NEVER:
			pass = false;
			break;
		case ATST_ALWAYS:
			pass = true;
			break;
		case ATST_LESS:
			if(amax < aref) pass = true;
			else if(amin >= aref) pass = false;
			else return false;
			break;
		case ATST_LEQUAL:
			if(amax <= aref) pass = true;
			else if(amin > aref) pass = false;
			else return false;
			break;
		case ATST_EQUAL:
			if(amin == aref && amax == aref) pass = true;
			else if(amin > aref || amax < aref) pass = false;
			else return false;
			break;
		case ATST_GEQUAL:
			if(amin >= aref) pass = true;
			else if(amax < aref) pass = false;
			else return false;
			break;
		case ATST_GREATER:
			if(amin > aref) pass = true;
			else if(amax <= aref) pass = false;
			else return false;
			break;
		case ATST_NOTEQUAL:
			if(amin == aref && amax == aref) pass = false;
			else if(amin > aref || amax < aref) pass = true;
			else return false;
			break;
		default:
			__assume(0);
		}
	}

	if(!pass)
	{
		switch(context->TEST.AFAIL)
		{
		case AFAIL_KEEP: fm = zm = 0xffffffff; break;
		case AFAIL_FB_ONLY: zm = 0xffffffff; break;
		case AFAIL_ZB_ONLY: fm = 0xffffffff; break;
		case AFAIL_RGB_ONLY: fm |= 0xff000000; zm = 0xffffffff; break;
		default: __assume(0);
		}
	}

	return true;
}

bool GSState::IsOpaque()
{
	if(PRIM->AA1)
	{
		return false;
	}

	if(!PRIM->ABE)
	{
		return true;
	}

	const GSDrawingContext* context = m_context;

	int amin = 0, amax = 0xff;

	if(context->ALPHA.A != context->ALPHA.B)
	{
		if(context->ALPHA.C == 0)
		{
			GetAlphaMinMax();

			amin = m_vt.m_alpha.min;
			amax = m_vt.m_alpha.max;
		}
		else if(context->ALPHA.C == 1)
		{
			if(context->FRAME.PSM == PSM_PSMCT24 || context->FRAME.PSM == PSM_PSMZ24)
			{
				amin = amax = 0x80;
			}
		}
		else if(context->ALPHA.C == 2)
		{
			amin = amax = context->ALPHA.FIX;
		}
	}

	return context->ALPHA.IsOpaque(amin, amax);
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
CRC::Region g_crc_region = CRC::NoRegion;

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
		else if(!fi.TME && fi.FBP == fi.TBP0 && fi.TBP0 == 0x2000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT24)
		{
			if(g_crc_region == CRC::US || g_crc_region == CRC::JP || g_crc_region == CRC::KO)
			{
				skip = 119;	//ntsc
			}
			else
			{
				skip = 136;	//pal
			}
		}
	}

	return true;
}

bool GSC_DBZBT2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && /*fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 &&*/ (fi.TBP0 == 0x01c00 || fi.TBP0 == 0x02000) && fi.TPSM == PSM_PSMZ16)
		{
			skip = 26; //27
		}
		else if(!fi.TME && (fi.FBP == 0x02a00 || fi.FBP == 0x03000) && fi.FPSM == PSM_PSMCT16)
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
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMT8H)
		{
			//not needed anymore?
			//skip = 24; // blur
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			if(fi.FBMSK == 0x00000)
			{
				skip = 28; // outline
			}
			if(fi.FBMSK == 0x00FFFFFF)
			{
				skip = 1;
			}
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 5;
		}
		else if(fi.TME && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03f00 && fi.TPSM == PSM_PSMCT32)
		{
			if (fi.FBP == 0x03400)
			{
				skip = 1;	//PAL
			}
			if(fi.FBP == 0x02e00)
			{
				skip = 3;	//NTSC
			}
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
    if(skip == 0)
    {
            if(fi.TME /*&& fi.FBP == 0x03d80*/ && fi.FPSM == 0 && fi.TBP0 == 0x03fc0 && fi.TPSM == 1)
            {
                    skip = 48;	//removes sky bloom
            }
            /*
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
            }*/
    }


     


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
		if(fi.TME && fi.FBP >= 0x02f00 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180 /*|| fi.TBP0 == 0x01a40*/) && fi.TPSM == PSM_PSMT8) //TBP0 0x1a40 progressive
		{
			skip = 770;	//ntsc, progressive 1540
		}
		if(g_crc_region == CRC::EU && fi.TME && fi.FBP >= 0x03400 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400 ) && fi.TPSM == PSM_PSMT8)
		{
			skip = 880;	//pal
		}
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01400) && fi.FPSM == PSM_PSMCT24 && fi.TBP0 >= 0x03420 && fi.TPSM == PSM_PSMT8)
		{
			// TODO: removes gfx from where it is not supposed to (garage)
			// skip = 58;
		}
	}

	return true;
}

bool GSC_GT3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x02de0 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.TPSM == PSM_PSMT8)
		{
			skip = 770;
		}
	}

	return true;
}

bool GSC_GTConcept(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP >= 0x03420 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400) && fi.TPSM == PSM_PSMT8)
		{
			skip = 880;
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
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x008c0 || fi.TBP0 == 0x00a00) && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.FPSM == fi.TPSM)
		{
			return false; // allowed
		}

		if(fi.TME && (fi.FBP == 0x01e40 || fi.FBP == 0x02200)  && fi.FPSM == PSM_PSMZ24 && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x01400) && fi.TPSM == PSM_PSMZ24)
		{
			skip = 42;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03c00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
		else if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00))
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
		if(fi.TME && (fi.FBP == 0x02d60 || fi.FBP == 0x02d80 || fi.FBP == 0x02ea0 || fi.FBP == 0x03620) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 95;
		}
		else if(fi.TME && (fi.FBP == 0x02bc0 || fi.FBP == 0x02be0 || fi.FBP == 0x02d00) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
		else if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 5;	//city at sunset's... sun...
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
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1000;
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
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16)
		{
			skip = 3;
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
			if( fi.FBP == 0x00100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x00100 && fi.TPSM == PSM_PSMCT16 // ntsc
				|| fi.FBP == 0x02100 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x02100 && fi.TPSM == PSM_PSMCT16) // pal
			{
				skip = 1000; // shadows
			}
			if((fi.FBP == 0x00100 || fi.FBP == 0x02100) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 & 0x03000) == 0x03000
				&& (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4)
				&& ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
			{
					skip = 1; // wall of fog
			}
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00100 || fi.FBP == 0x02100) && fi.FPSM == PSM_PSMCT16)
		{
			skip = 3;
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
		if(fi.TME && fi.FPSM == PSM_PSMCT16S && fi.TBP0 == 0x00000 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000; // shadow
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMCT16S)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_SimpsonsGame(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03000 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 2;
		}
	}
	
	return true;
}

bool GSC_Genji(const GSFrameInfo& fi, int& skip)
{
	if( !skip && fi.TME && (fi.FBP == 0x700 || fi.FBP == 0x0) && fi.TBP0 == 0x1500 && fi.TPSM )
		skip=1;

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
		/*if(fi.TME && (fi.FBP == 0x018c0 || fi.FBP == 0x02180) && fi.FPSM == fi.TPSM && fi.TBP0 >= 0x03200 && fi.TPSM == PSM_PSMCT32)	//NTSC only, !(fi.TBP0 == 0x03580 || fi.TBP0 == 0x03960)
		{
			skip = 1;	//red garbage in lost forest, removes other effects...
		}
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1;	//garbage in cutscenes, doesn't remove completely, better use "Alpha Hack"
		}*/
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

bool GSC_SuikodenTactics(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && fi.TPSM == PSM_PSMT8H && fi.FPSM == 0 && fi.FBMSK == 0x0FF000000 && fi.TBP0 == 0 && GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM))
		{
			skip = 4;
		}
	}

	return true;
}

bool GSC_Tenchu(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.TPSM == PSM_PSMZ16 && fi.FPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3; 
		}
	}
	
	return true;
}

bool GSC_Sly3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00700 || fi.FBP == 0x00a80 || fi.FBP == 0x00e00) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00700 || fi.TBP0 == 0x00a80 || fi.TBP0 == 0x00e00) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;
		}
	}

	return true;
}

bool GSC_Sly2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME &&  (fi.FBP == 0x00000 || fi.FBP == 0x00700 || fi.FBP == 0x00800) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;
		}
	}
	
	return true;
}

bool GSC_DemonStone(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01400 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
	}
	
	return true;
}

bool GSC_BigMuthaTruckers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT16)
		{
			skip = 3;
		}
	}
	
	return true;
}

bool GSC_TimeSplitters2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0FF000000)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_ReZ(const GSFrameInfo& fi, int& skip)
{
	//not needed anymore
	/*if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x00a00) && fi.FPSM == fi.TPSM && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1; 
		}
	}*/

	return true;
}

bool GSC_LordOfTheRingsTwoTowers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x01180 || fi.FBP == 0x01400) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 1000;//shadows
		}
		else if(fi.TME && fi.TPSM == PSM_PSMZ16 && fi.TBP0 == 0x01400 && fi.FPSM == PSM_PSMCT16 && fi.FBMSK == 0x03FFF)
		{
			skip = 3;	//wall of fog
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x01400) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 2;
		}
	}
	
	return true;
}

bool GSC_LordOfTheRingsThirdAge(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4 && fi.FBMSK == 0xFF000000)
		{
			skip = 1000;	//shadows
		}
	}
	else
	{
		if (fi.TME && (fi.FBP == 0x0 || fi.FBP == 0x00e00 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x03000 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_RedDeadRevolver(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0x02420 || fi.FBP == 0x025e0) && fi.FPSM == PSM_PSMCT24)
		{
			skip = 1200;
		}
		else if(fi.TME && (fi.FBP == 0x00800 || fi.FBP == 0x009c0) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x01600 || fi.TBP0 == 0x017c0) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 2;	//filter
		}
		else if(fi.FBP == 0x03700 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 2;	//blur
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00800 || fi.FBP == 0x009c0) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_HeavyMetalThunder(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 100;
		}
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x02a00 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}

	return true;
}

bool GSC_BleachBladeBattlers(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01180 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x03fc0 && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_Castlevania(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT16S && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 2;
		}
	}

	return true;
}

bool GSC_Black(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME /*&& (fi.FBP == 0x00000 || fi.FBP == 0x008c0)*/ && fi.FPSM == PSM_PSMCT16 && (fi.TBP0 == 0x01a40 || fi.TBP0 == 0x01b80 || fi.TBP0 == 0x030c0) && fi.TPSM == PSM_PSMZ16 || (GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)))
		{
			skip = 5;
		}
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x008c0 || fi.FBP == 0x0a00 ) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT4)
		{
			skip = 0;
		}
		else if(!fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 0;
		}
	}
	
	return true;
}

bool GSC_FFVIIDoC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x02c00 && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1;
		}
		if(!fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSM_PSMCT24)
		{
			//skip = 1;
		}
	}
	
	return true;
}

bool GSC_StarWarsForceUnleashed(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x038a0 || fi.FBP == 0x03ae0) && fi.FPSM == fi.TPSM && fi.TBP0 == 0x02300 && fi.TPSM == PSM_PSMZ24)
		{
			skip = 1000;	//9, shadows
		}
	}
	else
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x034a0 || fi.TBP0 == 0x36e0) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;	
		}

	}
	
	return true;
}

bool GSC_StarWarsBattlefront(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x0 && fi.FBP < 0x01000) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x02000 && fi.TBP0 < 0x03000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_StarWarsBattlefront2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x01000 && fi.FBP < 0x02000) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x0 && fi.TBP0 < 0x01000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
		if(fi.TME && (fi.FBP > 0x01000 && fi.FBP < 0x02000) && fi.FPSM == PSM_PSMZ32 && (fi.TBP0 > 0x0 && fi.TBP0 < 0x01000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_BlackHawkDown(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x01800 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 2;	//wall of fog
		}
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8)
		{
			skip = 5;	//night filter
		}
	}
	
	return true;
}

bool GSC_DevilMayCry3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMCT16 && fi.TBP0 == 0x01000 && fi.TPSM == PSM_PSMZ16)
		{
			skip = 32;
		}
		if(fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMZ32 && fi.TBP0 == 0x0800 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 16;
		}
		if(fi.TME && fi.FBP == 0x01800 && fi.FPSM == PSM_PSMCT32 && fi.TBP0 == 0x0 && fi.TPSM == PSM_PSMT8H)
		{
			skip = 24;
		}
	}
	
	return true;
}

bool GSC_Burnout(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x01dc0 || fi.FBP == 0x02200) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x01dc0 || fi.TBP0 == 0x02200) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 4;
		}
		else if(fi.TME && fi.FPSM == PSM_PSMCT16 && fi.TPSM == PSM_PSMZ16)	//fog
		{
			if(fi.FBP == 0x00a00 && fi.TBP0 == 0x01e00)	
			{
				skip = 4; //pal
			}
			if(fi.FBP == 0x008c0 && fi.TBP0 == 0x01a40)
			{
				skip = 3; //ntsc
			}
		}
		else if (fi.TME && (fi.FBP == 0x02d60 || fi.FBP == 0x033a0) && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x02d60 || fi.TBP0 == 0x033a0) && fi.TPSM == PSM_PSMCT32 && fi.FBMSK == 0x0)
		{
			skip = 2; //impact screen
		}
	}
	
	return true;
}

bool GSC_MidnightClub3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP > 0x01d00 && fi.FBP <= 0x02a00) && fi.FPSM == PSM_PSMCT32 && (fi.FBP >= 0x01600 && fi.FBP < 0x03260) && fi.TPSM == PSM_PSMT8H)
		{
			skip = 1;
		}
	}
	
	return true;
}

bool GSC_SpyroNewBeginning(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && fi.TBP0 == 0x034a0 && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;
		}
	}
	
	return true;
}

bool GSC_SpyroEternalNight(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == fi.TPSM && (fi.TBP0 == 0x034a0 ||fi.TBP0 == 0x035a0 || fi.TBP0 == 0x036e0) && fi.TPSM == PSM_PSMCT16)
		{
			skip = 2;
		}
	}
	
	return true;
}

bool GSC_TalesOfLegendia(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x3f80 || fi.FBP == 0x03fa0) && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMT8)
		{
			skip = 3; //3, 9
		}
		if(fi.TME && fi.FBP == 0x3800 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMZ32)
		{
			skip = 2;
		}
	}
		
	return true;
}

bool GSC_NanoBreaker(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x0 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x03800 || fi.TBP0 == 0x03900) && fi.TPSM == PSM_PSMCT16S)
		{
			skip = 2;
		}
	}
		
	return true;
}

bool GSC_Kunoichi(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(!fi.TME && (fi.FBP == 0x0 || fi.FBP == 0x00700 || fi.FBP == 0x00800) && fi.FPSM == PSM_PSMCT32 && (fi.TPSM == PSM_PSMT8 || fi.TPSM == PSM_PSMT4) && fi.FBMSK == 0x00FFFFFF)
		{
			skip = 3;
		}
	}
		
	return true;
}

bool GSC_Yakuza(const GSFrameInfo& fi, int& skip)
{
	if(1
		&& !skip
		&& !fi.TME
		&& (0
			|| fi.FBP == 0x1c20 && fi.TBP0 == 0xe00		//ntsc (EU and US DVDs)
			|| fi.FBP == 0x1e20 && fi.TBP0 == 0x1000	//pal1
			|| fi.FBP == 0x1620 && fi.TBP0 == 0x800		//pal2
		)
		&& fi.TPSM == PSM_PSMZ24
		&& fi.FPSM == PSM_PSMCT32
		/*
		&& fi.FBMSK	==0xffffff
		&& fi.TZTST
		&& !GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)
		*/
	)
	{
		skip=3;
	}
	return true;
}

bool GSC_Yakuza2(const GSFrameInfo& fi, int& skip)
{
	if(1
		&& !skip
		&& !fi.TME
		&& (0
			|| fi.FBP == 0x1c20 && fi.TBP0 == 0xe00		//ntsc (EU DVD)
			|| fi.FBP == 0x1e20 && fi.TBP0 == 0x1000	//pal1
			|| fi.FBP == 0x1620 && fi.TBP0 == 0x800		//pal2
		)
		&& fi.TPSM == PSM_PSMZ24
		&& fi.FPSM == PSM_PSMCT32
		/*
		&& fi.FBMSK	==0xffffff
		&& fi.TZTST
		&& !GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)
		*/
	)
	{
		skip=17;
	}
	return true;
}

bool GSC_SkyGunner(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(!fi.TME && !(fi.FBP == 0x0 || fi.FBP == 0x00800 || fi.FBP == 0x008c0 || fi.FBP == 0x03e00) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x0 || fi.TBP0 == 0x01800) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1; //Huge Vram usage
		}
	}
	
	return true;
}

bool GSC_JamesBondEverythingOrNothing(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{

		if(fi.TME && (fi.FBP < 0x02000 && !(fi.FBP == 0x0 || fi.FBP == 0x00e00)) && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 > 0x01c00 && fi.TBP0 < 0x03000) && fi.TPSM == PSM_PSMT8)
		{
			skip = 1; //Huge Vram usage
		}
	}
	
	return true;
}

#ifdef ENABLE_DYNAMIC_CRC_HACK

#define DYNA_DLL_PATH "c:/dev/pcsx2/trunk/tools/dynacrchack/DynaCrcHack.dll"

#include <sys/stat.h>
/***************************************************************************
	AutoReloadLibrary : Automatically reloads a dll if the file was modified.
		Uses a temporary copy of the watched dll such that the original
		can be modified while the copy is loaded and used.

	NOTE: The API is not platform specific, but current implementation is Win32.
***************************************************************************/
class AutoReloadLibrary
{
private:
	string	m_dllPath, m_loadedDllPath;
	DWORD	m_minMsBetweenProbes;
	time_t	m_lastFileModification;
	DWORD	m_lastProbe;
	HMODULE	m_library;

	string	GetTempName()
	{
		string result = m_loadedDllPath + ".tmp"; //default name
		TCHAR tmpPath[MAX_PATH], tmpName[MAX_PATH];
		DWORD ret = GetTempPath(MAX_PATH, tmpPath);
		if(ret && ret <= MAX_PATH && GetTempFileName(tmpPath, TEXT("GSdx"), 0, tmpName))
			result = tmpName;

		return result;
	};

	void	UnloadLib()
	{
		if( !m_library )
			return;

		FreeLibrary( m_library );
		m_library = NULL;

		// If can't delete (might happen when GSdx closes), schedule delete on reboot
		if(!DeleteFile( m_loadedDllPath.c_str() ) )
			MoveFileEx( m_loadedDllPath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
	}

public:
	AutoReloadLibrary( const string dllPath, const int minMsBetweenProbes=100 )
		: m_minMsBetweenProbes( minMsBetweenProbes )
		, m_dllPath( dllPath )
		, m_lastFileModification( 0 )
		, m_lastProbe( 0 )
		, m_library( 0 )
	{};

	~AutoReloadLibrary(){ UnloadLib();	};

	// If timeout has ellapsed, probe the dll for change, and reload if it was changed.
	// If it returns true, then the dll was freed/reloaded, and any symbol addresse previously obtained is now invalid and needs to be re-obtained.
	// Overhead is very low when when probe timeout has not ellapsed, and especially if current timestamp is supplied as argument.
	// Note: there's no relation between the file modification date and currentMs value, so it need'nt neccessarily be an actual timestamp.
	// Note: isChanged is guarenteed to return true at least once
	//       (even if the file doesn't exist, at which case the following GetSymbolAddress will return NULL)
	bool isChanged( const DWORD currentMs=0 )
	{
		DWORD current = currentMs? currentMs : GetTickCount();
		if( current >= m_lastProbe && ( current - m_lastProbe ) < m_minMsBetweenProbes )
			return false;

		bool firstTime = !m_lastProbe;
		m_lastProbe = current;

		struct stat s;
		if( stat( m_dllPath.c_str(), &s ) )
		{	
			// File doesn't exist or other error, unload dll
			bool wasLoaded = m_library?true:false;
			UnloadLib();	
			return firstTime || wasLoaded;	// Changed if previously loaded or the first time accessing this method (and file doesn't exist)
		}

		if( m_lastFileModification == s.st_mtime )
			return false;
		m_lastFileModification = s.st_mtime;

		// File modified, reload
		UnloadLib();

		if( !CopyFile( m_dllPath.c_str(), ( m_loadedDllPath = GetTempName() ).c_str(), false ) )
			return true;

		m_library = LoadLibrary( m_loadedDllPath.c_str() );
		return true;
	};

	// Return value is NULL if the dll isn't loaded (failure or doesn't exist) or if the symbol isn't found.
	void* GetSymbolAddress( const char* name ){ return m_library? GetProcAddress( m_library, name ) : NULL; };
};


// Use DynamicCrcHack function from a dll which can be modified while GSdx/PCSX2 is running.
// return value is true if the call succeeded or false otherwise (If the hack could not be invoked: no dll/function/etc).
// result contains the result of the hack call.

typedef uint32 (__cdecl* DynaHackType)(uint32, uint32, uint32, uint32, uint32, uint32, uint32, int32*, uint32, int32);

bool IsInvokedDynamicCrcHack( GSFrameInfo &fi, int& skip, int region, bool &result )
{
	static AutoReloadLibrary dll( DYNA_DLL_PATH );
	static DynaHackType dllFunc = NULL;

	if( dll.isChanged() )
	{
		dllFunc = (DynaHackType)dll.GetSymbolAddress( "DynamicCrcHack" );
		printf( "GSdx: Dynamic CRC-hacks: %s\n", dllFunc?
			"Loaded OK    (-> overriding internal hacks)" : "Not available    (-> using internal hacks)");
	}
	
	if( !dllFunc )
		return false;
	
	int32	skip32 = skip;
	bool	hasSharedBits = GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM);
	result	= dllFunc( fi.FBP, fi.FPSM, fi.FBMSK, fi.TBP0, fi.TPSM, fi.TZTST, (uint32)fi.TME, &skip32, (uint32)region, (uint32)(hasSharedBits?1:0) )?true:false;
	skip	= skip32;

	return true;
}

#endif

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
		map[CRC::GT3] = GSC_GT3;
		map[CRC::GTConcept] = GSC_GTConcept;
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
		map[CRC::SimpsonsGame] = GSC_SimpsonsGame;
		map[CRC::Genji] = GSC_Genji;
		map[CRC::StarOcean3] = GSC_StarOcean3;
		map[CRC::ValkyrieProfile2] = GSC_ValkyrieProfile2;
		map[CRC::RadiataStories] = GSC_RadiataStories;
		map[CRC::HauntingGround] = GSC_HauntingGround;
		map[CRC::SuikodenTactics] = GSC_SuikodenTactics;
		map[CRC::TenchuWoH] = GSC_Tenchu;
		map[CRC::TenchuFS] = GSC_Tenchu;
		map[CRC::Sly3] = GSC_Sly3;
		map[CRC::Sly2] = GSC_Sly2;
		map[CRC::DemonStone] = GSC_DemonStone;
		map[CRC::BigMuthaTruckers] = GSC_BigMuthaTruckers;
		map[CRC::TimeSplitters2] = GSC_TimeSplitters2;
		map[CRC::ReZ] = GSC_ReZ;
		map[CRC::LordOfTheRingsTwoTowers] = GSC_LordOfTheRingsTwoTowers;
		map[CRC::LordOfTheRingsThirdAge] = GSC_LordOfTheRingsThirdAge;
		map[CRC::RedDeadRevolver] = GSC_RedDeadRevolver;
		map[CRC::HeavyMetalThunder] = GSC_HeavyMetalThunder;
		map[CRC::BleachBladeBattlers] = GSC_BleachBladeBattlers;
		map[CRC::CastlevaniaCoD] = GSC_Castlevania;
		map[CRC::CastlevaniaLoI] = GSC_Castlevania;
		map[CRC::Black] = GSC_Black;
		map[CRC::FFVIIDoC] = GSC_FFVIIDoC;
		map[CRC::StarWarsForceUnleashed] = GSC_StarWarsForceUnleashed;
		map[CRC::StarWarsBattlefront] = GSC_StarWarsBattlefront;
		map[CRC::StarWarsBattlefront2] = GSC_StarWarsBattlefront2;
		map[CRC::BlackHawkDown] = GSC_BlackHawkDown;
		map[CRC::DevilMayCry3] = GSC_DevilMayCry3;
		map[CRC::BurnoutTakedown] = GSC_Burnout;
		map[CRC::BurnoutRevenge] = GSC_Burnout;
		map[CRC::BurnoutDominator] = GSC_Burnout;
		map[CRC::MidnightClub3] = GSC_MidnightClub3;
		map[CRC::SpyroNewBeginning] = GSC_SpyroNewBeginning;
		map[CRC::SpyroEternalNight] = GSC_SpyroEternalNight;
		map[CRC::TalesOfLegendia] = GSC_TalesOfLegendia;
		map[CRC::NanoBreaker] = GSC_NanoBreaker;
		map[CRC::Kunoichi] = GSC_Kunoichi;
		map[CRC::Yakuza] = GSC_Yakuza;
		map[CRC::Yakuza2] = GSC_Yakuza2;
		map[CRC::SkyGunner] = GSC_SkyGunner;
		map[CRC::JamesBondEverythingOrNothing] = GSC_JamesBondEverythingOrNothing;
	}

	// TODO: just set gsc in SetGameCRC once

	GetSkipCount gsc = map[m_game.title];
	g_crc_region = m_game.region;

#ifdef ENABLE_DYNAMIC_CRC_HACK
	bool res=false; if(IsInvokedDynamicCrcHack(fi, skip, g_crc_region, res)){ if( !res ) return false;	} else
#endif
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
