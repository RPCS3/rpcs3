/*  ZeroGS
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

typedef void (__fastcall *GIFRegHandler)(u32* data);

void __fastcall	GIFPackedRegHandlerNull(u32* data);
void __fastcall GIFPackedRegHandlerRGBA(u32* data);
void __fastcall GIFPackedRegHandlerSTQ(u32* data);
void __fastcall GIFPackedRegHandlerUV(u32* data);
void __fastcall GIFPackedRegHandlerXYZF2(u32* data);
void __fastcall GIFPackedRegHandlerXYZ2(u32* data);
void __fastcall GIFPackedRegHandlerFOG(u32* data);
void __fastcall GIFPackedRegHandlerA_D(u32* data);
void __fastcall GIFPackedRegHandlerNOP(u32* data);

void __fastcall GIFRegHandlerNull(u32* data);
void __fastcall GIFRegHandlerPRIM(u32* data);
void __fastcall GIFRegHandlerRGBAQ(u32* data);
void __fastcall GIFRegHandlerST(u32* data);
void __fastcall GIFRegHandlerUV(u32* data);
void __fastcall GIFRegHandlerXYZF2(u32* data);
void __fastcall GIFRegHandlerXYZ2(u32* data);
void __fastcall GIFRegHandlerTEX0_1(u32* data);
void __fastcall GIFRegHandlerTEX0_2(u32* data);
void __fastcall GIFRegHandlerCLAMP_1(u32* data);
void __fastcall GIFRegHandlerCLAMP_2(u32* data);
void __fastcall GIFRegHandlerFOG(u32* data);
void __fastcall GIFRegHandlerXYZF3(u32* data);
void __fastcall GIFRegHandlerXYZ3(u32* data);
void __fastcall GIFRegHandlerNOP(u32* data);
void __fastcall GIFRegHandlerTEX1_1(u32* data);
void __fastcall GIFRegHandlerTEX1_2(u32* data);
void __fastcall GIFRegHandlerTEX2_1(u32* data);
void __fastcall GIFRegHandlerTEX2_2(u32* data);
void __fastcall GIFRegHandlerXYOFFSET_1(u32* data);
void __fastcall GIFRegHandlerXYOFFSET_2(u32* data);
void __fastcall GIFRegHandlerPRMODECONT(u32* data);
void __fastcall GIFRegHandlerPRMODE(u32* data);
void __fastcall GIFRegHandlerTEXCLUT(u32* data);
void __fastcall GIFRegHandlerSCANMSK(u32* data);
void __fastcall GIFRegHandlerMIPTBP1_1(u32* data);
void __fastcall GIFRegHandlerMIPTBP1_2(u32* data);
void __fastcall GIFRegHandlerMIPTBP2_1(u32* data);
void __fastcall GIFRegHandlerMIPTBP2_2(u32* data);
void __fastcall GIFRegHandlerTEXA(u32* data);
void __fastcall GIFRegHandlerFOGCOL(u32* data);
void __fastcall GIFRegHandlerTEXFLUSH(u32* data);
void __fastcall GIFRegHandlerSCISSOR_1(u32* data);
void __fastcall GIFRegHandlerSCISSOR_2(u32* data);
void __fastcall GIFRegHandlerALPHA_1(u32* data);
void __fastcall GIFRegHandlerALPHA_2(u32* data);
void __fastcall GIFRegHandlerDIMX(u32* data);
void __fastcall GIFRegHandlerDTHE(u32* data);
void __fastcall GIFRegHandlerCOLCLAMP(u32* data);
void __fastcall GIFRegHandlerTEST_1(u32* data);
void __fastcall GIFRegHandlerTEST_2(u32* data);
void __fastcall GIFRegHandlerPABE(u32* data);
void __fastcall GIFRegHandlerFBA_1(u32* data);
void __fastcall GIFRegHandlerFBA_2(u32* data);
void __fastcall GIFRegHandlerFRAME_1(u32* data);
void __fastcall GIFRegHandlerFRAME_2(u32* data);
void __fastcall GIFRegHandlerZBUF_1(u32* data);
void __fastcall GIFRegHandlerZBUF_2(u32* data);
void __fastcall GIFRegHandlerBITBLTBUF(u32* data);
void __fastcall GIFRegHandlerTRXPOS(u32* data);
void __fastcall GIFRegHandlerTRXREG(u32* data);
void __fastcall GIFRegHandlerTRXDIR(u32* data);
void __fastcall GIFRegHandlerHWREG(u32* data);
void __fastcall GIFRegHandlerSIGNAL(u32* data);
void __fastcall GIFRegHandlerFINISH(u32* data);
void __fastcall GIFRegHandlerLABEL(u32* data);

// special buffers that delay executing some insturctions
//#include <vector>
//
//class ExecuteBuffer
//{
//public:
//	virtual void Execute()=0;
//};
//
//class ExecuteBufferXeno : public ExecuteBuffer
//{
//public:
//	ExecuteBufferXeno();
//	virtual void Execute();
//
//	void SetTex0(u32* data);
//	void SetTex1(u32* data);
//	void SetClamp(u32* data);
//	void SetTri();
//
//	u32 clampdata[2];
//	u32 tex0data[2];
//	u32 tex1data[2];
//	primInfo curprim;
//	
//	std::vector<Vertex> vertices;
//	bool bCanExecute;
//};
//
//extern ExecuteBufferXeno g_ebXeno;

#endif
