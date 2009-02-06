/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef __VU1OPS_H__
#define __VU1OPS_H__

#include "VU.h"

extern __forceinline u32 VU_MAC_UPDATE( int shift, VURegs * VU, float f);
extern __forceinline u32  VU_MACx_UPDATE(VURegs * VU, float x);
extern __forceinline u32  VU_MACy_UPDATE(VURegs * VU, float y);
extern __forceinline u32  VU_MACz_UPDATE(VURegs * VU, float z);
extern __forceinline u32  VU_MACw_UPDATE(VURegs * VU, float w);
extern __forceinline void VU_MACx_CLEAR(VURegs * VU);
extern __forceinline void VU_MACy_CLEAR(VURegs * VU);
extern __forceinline void VU_MACz_CLEAR(VURegs * VU);
extern __forceinline void VU_MACw_CLEAR(VURegs * VU);

#define float_to_int4(x)	(s32)((float)x * (1.0f / 0.0625f))
#define float_to_int12(x)	(s32)((float)x * (1.0f / 0.000244140625f))
#define float_to_int15(x)	(s32)((float)x * (1.0f / 0.000030517578125))

#define int4_to_float(x)	(float)((float)x * 0.0625f)
#define int12_to_float(x)	(float)((float)x * 0.000244140625f)
#define int15_to_float(x)	(float)((float)x * 0.000030517578125)

#define	MAC_Reset( VU ) VU->VI[REG_MAC_FLAG].UL = VU->VI[REG_MAC_FLAG].UL & (~0xFFFF)

void _vuSetCycleFlags(VURegs * VU);
void _vuFlushFDIV(VURegs * VU);
void _vuFlushEFU(VURegs * VU);
void _vuTestPipes(VURegs * VU);
void _vuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _vuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn);
void _vuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
void _vuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn);

/******************************/
/*   VU Upper instructions    */
/******************************/

void _vuABS(VURegs * VU);
void _vuADD(VURegs * VU);
void _vuADDi(VURegs * VU);
void _vuADDq(VURegs * VU);
void _vuADDx(VURegs * VU);
void _vuADDy(VURegs * VU);
void _vuADDz(VURegs * VU);
void _vuADDw(VURegs * VU);
void _vuADDA(VURegs * VU);
void _vuADDAi(VURegs * VU);
void _vuADDAq(VURegs * VU);
void _vuADDAx(VURegs * VU);
void _vuADDAy(VURegs * VU);
void _vuADDAz(VURegs * VU);
void _vuADDAw(VURegs * VU);
void _vuSUB(VURegs * VU);
void _vuSUBi(VURegs * VU);
void _vuSUBq(VURegs * VU);
void _vuSUBx(VURegs * VU);
void _vuSUBy(VURegs * VU);
void _vuSUBz(VURegs * VU);
void _vuSUBw(VURegs * VU);
void _vuSUBA(VURegs * VU);
void _vuSUBAi(VURegs * VU);
void _vuSUBAq(VURegs * VU);
void _vuSUBAx(VURegs * VU);
void _vuSUBAy(VURegs * VU);
void _vuSUBAz(VURegs * VU);
void _vuSUBAw(VURegs * VU);
void _vuMUL(VURegs * VU);
void _vuMULi(VURegs * VU);
void _vuMULq(VURegs * VU);
void _vuMULx(VURegs * VU);
void _vuMULy(VURegs * VU);
void _vuMULz(VURegs * VU);
void _vuMULw(VURegs * VU);
void _vuMULA(VURegs * VU);
void _vuMULAi(VURegs * VU);
void _vuMULAq(VURegs * VU);
void _vuMULAx(VURegs * VU);
void _vuMULAy(VURegs * VU);
void _vuMULAz(VURegs * VU);
void _vuMULAw(VURegs * VU);
void _vuMADD(VURegs * VU) ;
void _vuMADDi(VURegs * VU);
void _vuMADDq(VURegs * VU);
void _vuMADDx(VURegs * VU);
void _vuMADDy(VURegs * VU);
void _vuMADDz(VURegs * VU);
void _vuMADDw(VURegs * VU);
void _vuMADDA(VURegs * VU);
void _vuMADDAi(VURegs * VU);
void _vuMADDAq(VURegs * VU);
void _vuMADDAx(VURegs * VU);
void _vuMADDAy(VURegs * VU);
void _vuMADDAz(VURegs * VU);
void _vuMADDAw(VURegs * VU);
void _vuMSUB(VURegs * VU);
void _vuMSUBi(VURegs * VU);
void _vuMSUBq(VURegs * VU);
void _vuMSUBx(VURegs * VU);
void _vuMSUBy(VURegs * VU);
void _vuMSUBz(VURegs * VU) ;
void _vuMSUBw(VURegs * VU) ;
void _vuMSUBA(VURegs * VU);
void _vuMSUBAi(VURegs * VU);
void _vuMSUBAq(VURegs * VU);
void _vuMSUBAx(VURegs * VU);
void _vuMSUBAy(VURegs * VU);
void _vuMSUBAz(VURegs * VU);
void _vuMSUBAw(VURegs * VU);
void _vuMAX(VURegs * VU);
void _vuMAXi(VURegs * VU);
void _vuMAXx(VURegs * VU);
void _vuMAXy(VURegs * VU);
void _vuMAXz(VURegs * VU);
void _vuMAXw(VURegs * VU);
void _vuMINI(VURegs * VU);
void _vuMINIi(VURegs * VU);
void _vuMINIx(VURegs * VU);
void _vuMINIy(VURegs * VU);
void _vuMINIz(VURegs * VU);
void _vuMINIw(VURegs * VU);
void _vuOPMULA(VURegs * VU);
void _vuOPMSUB(VURegs * VU);
void _vuNOP(VURegs * VU);
void _vuFTOI0(VURegs * VU);
void _vuFTOI4(VURegs * VU);
void _vuFTOI12(VURegs * VU);
void _vuFTOI15(VURegs * VU);
void _vuITOF0(VURegs * VU) ;
void _vuITOF4(VURegs * VU) ;
void _vuITOF12(VURegs * VU);
void _vuITOF15(VURegs * VU);
void _vuCLIP(VURegs * VU); 
/******************************/ 
/*   VU Lower instructions    */ 
/******************************/ 
void _vuDIV(VURegs * VU);
void _vuSQRT(VURegs * VU);
void _vuRSQRT(VURegs * VU);
void _vuIADDI(VURegs * VU);
void _vuIADDIU(VURegs * VU);
void _vuIADD(VURegs * VU);
void _vuIAND(VURegs * VU);
void _vuIOR(VURegs * VU);
void _vuISUB(VURegs * VU);
void _vuISUBIU(VURegs * VU);
void _vuMOVE(VURegs * VU);
void _vuMFIR(VURegs * VU);
void _vuMTIR(VURegs * VU);
void _vuMR32(VURegs * VU);
void _vuLQ(VURegs * VU) ;
void _vuLQD(VURegs * VU);
void _vuLQI(VURegs * VU);
void _vuSQ(VURegs * VU);
void _vuSQD(VURegs * VU);
void _vuSQI(VURegs * VU);
void _vuILW(VURegs * VU);
void _vuISW(VURegs * VU);
void _vuILWR(VURegs * VU);
void _vuISWR(VURegs * VU);
void _vuLOI(VURegs * VU);
void _vuRINIT(VURegs * VU);
void _vuRGET(VURegs * VU);
void _vuRNEXT(VURegs * VU);
void _vuRXOR(VURegs * VU);
void _vuWAITQ(VURegs * VU);
void _vuFSAND(VURegs * VU);
void _vuFSEQ(VURegs * VU);
void _vuFSOR(VURegs * VU);
void _vuFSSET(VURegs * VU);
void _vuFMAND(VURegs * VU);
void _vuFMEQ(VURegs * VU);
void _vuFMOR(VURegs * VU);
void _vuFCAND(VURegs * VU);
void _vuFCEQ(VURegs * VU);
void _vuFCOR(VURegs * VU);
void _vuFCSET(VURegs * VU);
void _vuFCGET(VURegs * VU);
void _vuIBEQ(VURegs * VU);
void _vuIBGEZ(VURegs * VU);
void _vuIBGTZ(VURegs * VU);
void _vuIBLEZ(VURegs * VU);
void _vuIBLTZ(VURegs * VU);
void _vuIBNE(VURegs * VU);
void _vuB(VURegs * VU);
void _vuBAL(VURegs * VU);
void _vuJR(VURegs * VU);
void _vuJALR(VURegs * VU);
void _vuMFP(VURegs * VU);
void _vuWAITP(VURegs * VU);
void _vuESADD(VURegs * VU);
void _vuERSADD(VURegs * VU);
void _vuELENG(VURegs * VU);
void _vuERLENG(VURegs * VU);
void _vuEATANxy(VURegs * VU);
void _vuEATANxz(VURegs * VU);
void _vuESUM(VURegs * VU);
void _vuERCPR(VURegs * VU);
void _vuESQRT(VURegs * VU);
void _vuERSQRT(VURegs * VU);
void _vuESIN(VURegs * VU);
void _vuEATAN(VURegs * VU);
void _vuEEXP(VURegs * VU);
void _vuXITOP(VURegs * VU);
void _vuXGKICK(VURegs * VU);
void _vuXTOP(VURegs * VU);

/******************************/
/*   VU Upper instructions    */
/******************************/

void _vuRegsABS(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsADDAw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUB(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSUBAw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMUL(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMULAw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMADDAw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUB(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAq(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMSUBAw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAX(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAXi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAXx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAXy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAXz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMAXw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINI(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINIi(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINIx(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINIy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINIz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMINIw(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsOPMULA(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsOPMSUB(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsNOP(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFTOI0(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFTOI4(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFTOI12(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFTOI15(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsITOF0(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsITOF4(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsITOF12(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsITOF15(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsCLIP(VURegs * VU, _VURegsNum *VUregsn); 
/******************************/ 
/*   VU Lower instructions    */ 
/******************************/ 
void _vuRegsDIV(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSQRT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsRSQRT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIADDI(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIADDIU(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIADD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIAND(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIOR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsISUB(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsISUBIU(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMOVE(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMFIR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMTIR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMR32(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsLQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsLQD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsLQI(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSQD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsSQI(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsILW(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsISW(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsILWR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsISWR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsLOI(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsRINIT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsRGET(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsRNEXT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsRXOR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsWAITQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFSAND(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFSEQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFSOR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFSSET(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFMAND(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFMEQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFMOR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFCAND(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFCEQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFCOR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFCSET(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsFCGET(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBEQ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBGEZ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBGTZ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBLEZ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBLTZ(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsIBNE(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsB(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsBAL(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsJR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsJALR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsMFP(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsWAITP(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsESADD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsERSADD(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsELENG(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsERLENG(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsEATANxy(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsEATANxz(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsESUM(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsERCPR(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsESQRT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsERSQRT(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsESIN(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsEATAN(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsEEXP(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsXITOP(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsXGKICK(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsXTOP(VURegs * VU, _VURegsNum *VUregsn);

#endif
