/*  GSnull
 *  Copyright (C) 2004-2009 PCSX2 Team
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
#include "Registers.h"

GIFRegHandler GIFPackedRegHandlers[16];
GIFRegHandler GIFRegHandlers[256];

// For now, I'm just rigging this up to store all the register information coming in, without doing 
// any of the normal processing.

void __gifCall GIFPackedRegHandlerNull(const u32* data)
{
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
	GIFPackedRGBA* r = (GIFPackedRGBA*)(data);
	gs.regs.RGBAQ.R = r->R;
	gs.regs.RGBAQ.G = r->G;
	gs.regs.RGBAQ.B = r->B;
	gs.regs.RGBAQ.A = r->A;
	gs.regs.RGBAQ.Q = gs.q;
}

void __gifCall GIFPackedRegHandlerSTQ(const u32* data)
{
	GIFPackedSTQ* r = (GIFPackedSTQ*)(data);
	gs.regs.ST.S = r->S;
	gs.regs.ST.T = r->T;
	gs.q = r->Q;
}

void __gifCall GIFPackedRegHandlerUV(const u32* data)
{
	GIFPackedUV* r = (GIFPackedUV*)(data);
	gs.regs.UV.U = r->U;
	gs.regs.UV.V = r->V;
}

void __gifCall KickVertex(bool adc)
{
}

void __gifCall GIFPackedRegHandlerXYZF2(const u32* data)
{
	GIFPackedXYZF2* r = (GIFPackedXYZF2*)(data);
	gs.regs.XYZ.X = r->X;
	gs.regs.XYZ.Y = r->Y;
	gs.regs.XYZ.Z = r->Z;
	gs.regs.FOG.F = r->F;
}

void __gifCall GIFPackedRegHandlerXYZ2(const u32* data)
{
	GIFPackedXYZ2* r = (GIFPackedXYZ2*)(data);
	gs.regs.XYZ.X = r->X;
	gs.regs.XYZ.Y = r->Y;
	gs.regs.XYZ.Z = r->Z;
}

void __gifCall GIFPackedRegHandlerFOG(const u32* data)
{
	GIFPackedFOG* r = (GIFPackedFOG*)(data);
	gs.regs.FOG.F = r->F;
}

void __gifCall GIFPackedRegHandlerA_D(const u32* data)
{
	GIFPackedA_D* r = (GIFPackedA_D*)(data);
	
	GIFRegHandlers[r->ADDR](data);
}

void __gifCall GIFPackedRegHandlerNOP(const u32* data)
{
}

void __gifCall GIFRegHandlerNull(const u32* data)
{
}

void __gifCall GIFRegHandlerRGBAQ(const u32* data)
{
	GIFRegRGBAQ* r = (GIFRegRGBAQ*)(data);
	gs.regs.RGBAQ._u64 = r->_u64;
}

void __gifCall GIFRegHandlerST(const u32* data)
{
	GIFRegST* r = (GIFRegST*)(data);
	gs.regs.ST._u64 = r->_u64;
}

void __gifCall GIFRegHandlerUV(const u32* data)
{
	GIFRegUV* r = (GIFRegUV*)(data);
	gs.regs.UV._u64 = r->_u64;
}

void __gifCall GIFRegHandlerXYZF2(const u32* data)
{
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.regs.XYZF._u64 = r->_u64;
}

void __gifCall GIFRegHandlerXYZ2(const u32* data)
{
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.regs.XYZ._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerTEX0(const u32* data)
{
	GIFRegTEX0* r = (GIFRegTEX0*)(data);
	gs.ctxt_regs[i].TEX0._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerCLAMP(const u32* data)
{
	GIFRegCLAMP* r = (GIFRegCLAMP*)(data);
	gs.ctxt_regs[i].CLAMP._u64 = r->_u64;
}

void __gifCall GIFRegHandlerFOG(const u32* data)
{
	GIFRegFOG* r = (GIFRegFOG*)(data);
	gs.regs.FOG.F = r->F;
}

void __gifCall GIFRegHandlerXYZF3(const u32* data)
{
	GIFRegXYZF* r = (GIFRegXYZF*)(data);
	gs.regs.XYZF._u64 = r->_u64;
}

void __gifCall GIFRegHandlerXYZ3(const u32* data)
{
	GIFRegXYZ* r = (GIFRegXYZ*)(data);
	gs.regs.XYZ._u64 = r->_u64;
}

void __gifCall GIFRegHandlerNOP(const u32* data)
{
}

template <u32 i>
void __fastcall GIFRegHandlerTEX1(const u32* data)
{
	GIFRegTEX1* r = (GIFRegTEX1*)(data);
	gs.ctxt_regs[i].TEX1._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerTEX2(const u32* data)
{
	GIFRegTEX2* r = (GIFRegTEX2*)(data);
	gs.ctxt_regs[i].TEX2._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerXYOFFSET(const u32* data)
{
	GIFRegXYOFFSET* r = (GIFRegXYOFFSET*)(data);
	gs.ctxt_regs[i].XYOFFSET.OFX = r->OFX;
	gs.ctxt_regs[i].XYOFFSET.OFY = r->OFY;
}

// Fill out the vertex queue(prim) and the attributes.
void __gifCall GIFRegHandlerPRIM(const u32 *data)
{
	GIFRegPRIM* r = (GIFRegPRIM*)(data);
	gs.regs.PRIM._u64 = r->_u64;
}

// Fill out an alternate set of attributes.
void __gifCall GIFRegHandlerPRMODE(const u32* data)
{
	GIFRegPRMODE* r = (GIFRegPRMODE*)(data);
	gs.regs.PRMODE._u64 = r->_u64;
}

// Switch between the primary set of attributes and the secondary.
void __gifCall GIFRegHandlerPRMODECONT(const u32* data)
{
	GIFRegPRMODECONT* r = (GIFRegPRMODECONT*)(data);
	gs.regs.PRMODECONT._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTEXCLUT(const u32* data)
{
	GIFRegTEXCLUT* r = (GIFRegTEXCLUT*)(data);
	gs.regs.TEXCLUT._u64 = r->_u64;
}

void __gifCall GIFRegHandlerSCANMSK(const u32* data)
{
	GIFRegSCANMSK* r = (GIFRegSCANMSK*)(data);
	gs.regs.SCANMSK._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerMIPTBP1(const u32* data)
{
	GIFRegMIPTBP1* r = (GIFRegMIPTBP1*)(data);
	gs.ctxt_regs[i].MIPTBP1._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerMIPTBP2(const u32* data)
{
	GIFRegMIPTBP2* r = (GIFRegMIPTBP2*)(data);
	gs.ctxt_regs[i].MIPTBP2._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTEXA(const u32* data)
{
	GIFRegTEXA* r = (GIFRegTEXA*)(data);
	gs.regs.TEXA._u64 = r->_u64;
}

void __gifCall GIFRegHandlerFOGCOL(const u32* data)
{
	GIFRegFOGCOL* r = (GIFRegFOGCOL*)(data);
	gs.regs.FOGCOL._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTEXFLUSH(const u32* data)
{
	GIFRegTEXFLUSH* r = (GIFRegTEXFLUSH*)(data);
	gs.regs.TEXFLUSH._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerSCISSOR(const u32* data)
{
	GIFRegSCISSOR* r = (GIFRegSCISSOR*)(data);
	gs.ctxt_regs[i].SCISSOR._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerALPHA(const u32* data)
{
	GIFRegALPHA* r = (GIFRegALPHA*)(data);
	gs.ctxt_regs[i].ALPHA._u64 = r->_u64;
}

void __gifCall GIFRegHandlerDIMX(const u32* data)
{
	GIFRegDIMX* r = (GIFRegDIMX*)(data);
	gs.regs.DIMX._u64 = r->_u64;
}

void __gifCall GIFRegHandlerDTHE(const u32* data)
{
	GIFRegDTHE* r = (GIFRegDTHE*)(data);
	gs.regs.DTHE._u64 = r->_u64;
}

void __gifCall GIFRegHandlerCOLCLAMP(const u32* data)
{
	GIFRegCOLCLAMP* r = (GIFRegCOLCLAMP*)(data);
	gs.regs.COLCLAMP._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerTEST(const u32* data)
{
	GIFRegTEST* r = (GIFRegTEST*)(data);
	gs.ctxt_regs[i].TEST._u64 = r->_u64;
}

void __gifCall GIFRegHandlerPABE(const u32* data)
{
	GIFRegPABE* r = (GIFRegPABE*)(data);
	gs.regs.PABE._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerFBA(const u32* data)
{
	GIFRegFBA* r = (GIFRegFBA*)(data);
	gs.ctxt_regs[i].FBA._u64 = r->_u64;
}

template<u32 i>
void __gifCall GIFRegHandlerFRAME(const u32* data)
{
	GIFRegFRAME* r = (GIFRegFRAME*)(data);
	gs.ctxt_regs[i].FRAME._u64 = r->_u64;
}

template <u32 i>
void __gifCall GIFRegHandlerZBUF(const u32* data)
{
	GIFRegZBUF* r = (GIFRegZBUF*)(data);
	gs.ctxt_regs[i].ZBUF._u64 = r->_u64;
}

void __gifCall GIFRegHandlerBITBLTBUF(const u32* data)
{
	GIFRegBITBLTBUF* r = (GIFRegBITBLTBUF*)(data);
	gs.regs.BITBLTBUF._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTRXPOS(const u32* data)
{
	GIFRegTRXPOS* r = (GIFRegTRXPOS*)(data);
	gs.regs.TRXPOS._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTRXREG(const u32* data)
{
	GIFRegTRXREG* r = (GIFRegTRXREG*)(data);
	gs.regs.TRXREG._u64 = r->_u64;
}

void __gifCall GIFRegHandlerTRXDIR(const u32* data)
{
	GIFRegTRXDIR* r = (GIFRegTRXDIR*)(data);
	gs.regs.TRXDIR._u64 = r->_u64;
}

void __gifCall GIFRegHandlerHWREG(const u32* data)
{
	GIFRegHWREG* r = (GIFRegHWREG*)(data);
	gs.regs.HWREG._u64 = r->_u64;
}


void __gifCall GIFRegHandlerSIGNAL(const u32* data)
{
	GIFRegSIGNAL* r = (GIFRegSIGNAL*)(data);
	gs.regs.SIGNAL._u64 = r->_u64;
}

void __gifCall GIFRegHandlerFINISH(const u32* data)
{
	GIFRegFINISH* r = (GIFRegFINISH*)(data);
	gs.regs.FINISH._u64 = r->_u64;
}

void __gifCall GIFRegHandlerLABEL(const u32* data)
{
	GIFRegLABEL* r = (GIFRegLABEL*)(data);
	gs.regs.LABEL._u64 = r->_u64;
}


void SetMultithreaded()
{
	// Some older versions of PCSX2 didn't properly set the irq callback to NULL
	// in multithreaded mode (possibly because ZeroGS itself would assert in such
	// cases), and didn't bind them to a dummy callback either.  PCSX2 handles all
	// IRQs internally when multithreaded anyway -- so let's ignore them here:

	if (gs.MultiThreaded)
	{
		GIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GIFRegHandlerNull;
		GIFRegHandlers[GIF_A_D_REG_FINISH] = &GIFRegHandlerNull;
		GIFRegHandlers[GIF_A_D_REG_LABEL] = &GIFRegHandlerNull;
	}
	else
	{
		GIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GIFRegHandlerSIGNAL;
		GIFRegHandlers[GIF_A_D_REG_FINISH] = &GIFRegHandlerFINISH;
		GIFRegHandlers[GIF_A_D_REG_LABEL] = &GIFRegHandlerLABEL;
	}
}

void ResetRegs()
{
	for (int i = 0; i < 16; i++)
	{
		GIFPackedRegHandlers[i] = &GIFPackedRegHandlerNull;
	}

	GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerPRIM;
	GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerRGBA;
	GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerSTQ;
	GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerUV;
	GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerXYZF2;
	GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerXYZ2;
	GIFPackedRegHandlers[GIF_REG_TEX0_1] = &GIFPackedRegHandlerTEX0<0>;
	GIFPackedRegHandlers[GIF_REG_TEX0_2] = &GIFPackedRegHandlerTEX0<1>;
	GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerCLAMP<0>;
	GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerCLAMP<1>;
	GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerFOG;
	GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerXYZF3;
	GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerXYZ3;
	GIFPackedRegHandlers[GIF_REG_A_D] = &GIFPackedRegHandlerA_D;
	GIFPackedRegHandlers[GIF_REG_NOP] = &GIFPackedRegHandlerNOP;
	
	for (int i = 0; i < 256; i++)
	{
		GIFRegHandlers[i] = &GIFRegHandlerNull;
	}
	
	GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerPRIM;
	GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerRGBAQ;
	GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerST;
	GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerUV;
	GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerXYZF2;
	GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerXYZ2;
	GIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GIFRegHandlerTEX0<0>;
	GIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GIFRegHandlerTEX0<1>;
	GIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GIFRegHandlerCLAMP<0>;
	GIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GIFRegHandlerCLAMP<1>;
	GIFRegHandlers[GIF_A_D_REG_FOG] = &GIFRegHandlerFOG;
	GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerXYZF3;
	GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerXYZ3;
	GIFRegHandlers[GIF_A_D_REG_NOP] = &GIFRegHandlerNOP;
	GIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GIFRegHandlerTEX1<0>;
	GIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GIFRegHandlerTEX1<1>;
	GIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GIFRegHandlerTEX2<0>;
	GIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GIFRegHandlerTEX2<1>;
	GIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GIFRegHandlerXYOFFSET<0>;
	GIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GIFRegHandlerXYOFFSET<1>;
	GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerPRMODECONT;
	GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerPRMODE;
	GIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GIFRegHandlerTEXCLUT;
	GIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GIFRegHandlerSCANMSK;
	GIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GIFRegHandlerMIPTBP1<0>;
	GIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GIFRegHandlerMIPTBP1<1>;
	GIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GIFRegHandlerMIPTBP2<0>;
	GIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GIFRegHandlerMIPTBP2<1>;
	GIFRegHandlers[GIF_A_D_REG_TEXA] = &GIFRegHandlerTEXA;
	GIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GIFRegHandlerFOGCOL;
	GIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GIFRegHandlerTEXFLUSH;
	GIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GIFRegHandlerSCISSOR<0>;
	GIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GIFRegHandlerSCISSOR<1>;
	GIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GIFRegHandlerALPHA<0>;
	GIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GIFRegHandlerALPHA<1>;
	GIFRegHandlers[GIF_A_D_REG_DIMX] = &GIFRegHandlerDIMX;
	GIFRegHandlers[GIF_A_D_REG_DTHE] = &GIFRegHandlerDTHE;
	GIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GIFRegHandlerCOLCLAMP;
	GIFRegHandlers[GIF_A_D_REG_TEST_1] = &GIFRegHandlerTEST<0>;
	GIFRegHandlers[GIF_A_D_REG_TEST_2] = &GIFRegHandlerTEST<1>;
	GIFRegHandlers[GIF_A_D_REG_PABE] = &GIFRegHandlerPABE;
	GIFRegHandlers[GIF_A_D_REG_FBA_1] = &GIFRegHandlerFBA<0>;
	GIFRegHandlers[GIF_A_D_REG_FBA_2] = &GIFRegHandlerFBA<1>;
	GIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GIFRegHandlerFRAME<0>;
	GIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GIFRegHandlerFRAME<1>;
	GIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GIFRegHandlerZBUF<0>;
	GIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GIFRegHandlerZBUF<1>;
	GIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GIFRegHandlerBITBLTBUF;
	GIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GIFRegHandlerTRXPOS;
	GIFRegHandlers[GIF_A_D_REG_TRXREG] = &GIFRegHandlerTRXREG;
	GIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GIFRegHandlerTRXDIR;
	GIFRegHandlers[GIF_A_D_REG_HWREG] = &GIFRegHandlerHWREG;
	SetMultithreaded();
}

void SetFrameSkip(bool skip)
{
	if (skip)
	{
		GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerNOP;
		GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerNOP;

		GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerNOP;
		GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerNOP;
	}
	else
	{
		GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerPRIM;
		GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerRGBA;
		GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerSTQ;
		GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerUV;
		GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerXYZF2;
		GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerXYZ2;
		GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerCLAMP<0>;
		GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerCLAMP<1>;
		GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerFOG;
		GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerXYZF3;
		GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerXYZ3;

		GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerPRIM;
		GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerRGBAQ;
		GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerST;
		GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerUV;
		GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerXYZF2;
		GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerXYZ2;
		GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerXYZF3;
		GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerXYZ3;
		GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerPRMODECONT;
		GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerPRMODE;
	}
}
