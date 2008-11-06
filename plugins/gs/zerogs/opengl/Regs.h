/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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

#ifndef __GSREGS_H__
#define __GSREGS_H__

#ifdef _MSC_VER
typedef void (__fastcall *GIFRegHandler)(u32* data);
#else

#ifdef __x86_64__
typedef void (*GIFRegHandler)(u32* data);
#else
typedef void (__attribute__((fastcall)) *GIFRegHandler)(u32* data);
#endif

#endif

void FASTCALL(GIFPackedRegHandlerNull(u32* data));
void FASTCALL(GIFPackedRegHandlerRGBA(u32* data));
void FASTCALL(GIFPackedRegHandlerSTQ(u32* data));
void FASTCALL(GIFPackedRegHandlerUV(u32* data));
void FASTCALL(GIFPackedRegHandlerXYZF2(u32* data));
void FASTCALL(GIFPackedRegHandlerXYZ2(u32* data));
void FASTCALL(GIFPackedRegHandlerFOG(u32* data));
void FASTCALL(GIFPackedRegHandlerA_D(u32* data));
void FASTCALL(GIFPackedRegHandlerNOP(u32* data));

void FASTCALL(GIFRegHandlerNull(u32* data));
void FASTCALL(GIFRegHandlerPRIM(u32* data));
void FASTCALL(GIFRegHandlerRGBAQ(u32* data));
void FASTCALL(GIFRegHandlerST(u32* data));
void FASTCALL(GIFRegHandlerUV(u32* data));
void FASTCALL(GIFRegHandlerXYZF2(u32* data));
void FASTCALL(GIFRegHandlerXYZ2(u32* data));
void FASTCALL(GIFRegHandlerTEX0_1(u32* data));
void FASTCALL(GIFRegHandlerTEX0_2(u32* data));
void FASTCALL(GIFRegHandlerCLAMP_1(u32* data));
void FASTCALL(GIFRegHandlerCLAMP_2(u32* data));
void FASTCALL(GIFRegHandlerFOG(u32* data));
void FASTCALL(GIFRegHandlerXYZF3(u32* data));
void FASTCALL(GIFRegHandlerXYZ3(u32* data));
void FASTCALL(GIFRegHandlerNOP(u32* data));
void FASTCALL(GIFRegHandlerTEX1_1(u32* data));
void FASTCALL(GIFRegHandlerTEX1_2(u32* data));
void FASTCALL(GIFRegHandlerTEX2_1(u32* data));
void FASTCALL(GIFRegHandlerTEX2_2(u32* data));
void FASTCALL(GIFRegHandlerXYOFFSET_1(u32* data));
void FASTCALL(GIFRegHandlerXYOFFSET_2(u32* data));
void FASTCALL(GIFRegHandlerPRMODECONT(u32* data));
void FASTCALL(GIFRegHandlerPRMODE(u32* data));
void FASTCALL(GIFRegHandlerTEXCLUT(u32* data));
void FASTCALL(GIFRegHandlerSCANMSK(u32* data));
void FASTCALL(GIFRegHandlerMIPTBP1_1(u32* data));
void FASTCALL(GIFRegHandlerMIPTBP1_2(u32* data));
void FASTCALL(GIFRegHandlerMIPTBP2_1(u32* data));
void FASTCALL(GIFRegHandlerMIPTBP2_2(u32* data));
void FASTCALL(GIFRegHandlerTEXA(u32* data));
void FASTCALL(GIFRegHandlerFOGCOL(u32* data));
void FASTCALL(GIFRegHandlerTEXFLUSH(u32* data));
void FASTCALL(GIFRegHandlerSCISSOR_1(u32* data));
void FASTCALL(GIFRegHandlerSCISSOR_2(u32* data));
void FASTCALL(GIFRegHandlerALPHA_1(u32* data));
void FASTCALL(GIFRegHandlerALPHA_2(u32* data));
void FASTCALL(GIFRegHandlerDIMX(u32* data));
void FASTCALL(GIFRegHandlerDTHE(u32* data));
void FASTCALL(GIFRegHandlerCOLCLAMP(u32* data));
void FASTCALL(GIFRegHandlerTEST_1(u32* data));
void FASTCALL(GIFRegHandlerTEST_2(u32* data));
void FASTCALL(GIFRegHandlerPABE(u32* data));
void FASTCALL(GIFRegHandlerFBA_1(u32* data));
void FASTCALL(GIFRegHandlerFBA_2(u32* data));
void FASTCALL(GIFRegHandlerFRAME_1(u32* data));
void FASTCALL(GIFRegHandlerFRAME_2(u32* data));
void FASTCALL(GIFRegHandlerZBUF_1(u32* data));
void FASTCALL(GIFRegHandlerZBUF_2(u32* data));
void FASTCALL(GIFRegHandlerBITBLTBUF(u32* data));
void FASTCALL(GIFRegHandlerTRXPOS(u32* data));
void FASTCALL(GIFRegHandlerTRXREG(u32* data));
void FASTCALL(GIFRegHandlerTRXDIR(u32* data));
void FASTCALL(GIFRegHandlerHWREG(u32* data));
void FASTCALL(GIFRegHandlerSIGNAL(u32* data));
void FASTCALL(GIFRegHandlerFINISH(u32* data));
void FASTCALL(GIFRegHandlerLABEL(u32* data));

#endif
