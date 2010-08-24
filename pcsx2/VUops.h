/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "VU.h"
#include "VUflags.h"

#define float_to_int4(x)	(s32)((float)x * (1.0f / 0.0625f))
#define float_to_int12(x)	(s32)((float)x * (1.0f / 0.000244140625f))
#define float_to_int15(x)	(s32)((float)x * (1.0f / 0.000030517578125))

#define int4_to_float(x)	(float)((float)x * 0.0625f)
#define int12_to_float(x)	(float)((float)x * 0.000244140625f)
#define int15_to_float(x)	(float)((float)x * 0.000030517578125)

#define	MAC_Reset( VU ) VU->VI[REG_MAC_FLAG].UL = VU->VI[REG_MAC_FLAG].UL & (~0xFFFF)

#define __vuRegsCall __fastcall
typedef void __vuRegsCall FnType_VuRegsN(_VURegsNum *VUregsn);
typedef FnType_VuRegsN* Fnptr_VuRegsN;

extern __aligned16 const Fnptr_Void VU0_LOWER_OPCODE[128];
extern __aligned16 const Fnptr_Void VU0_UPPER_OPCODE[64];
extern __aligned16 const Fnptr_VuRegsN VU0regs_LOWER_OPCODE[128];
extern __aligned16 const Fnptr_VuRegsN VU0regs_UPPER_OPCODE[64];

extern __aligned16 const Fnptr_Void VU1_LOWER_OPCODE[128];
extern __aligned16 const Fnptr_Void VU1_UPPER_OPCODE[64];
extern __aligned16 const Fnptr_VuRegsN VU1regs_LOWER_OPCODE[128];
extern __aligned16 const Fnptr_VuRegsN VU1regs_UPPER_OPCODE[64];

extern void _vuTestPipes(VURegs * VU);
extern void _vuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
extern void _vuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn);
extern void _vuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn);
extern void _vuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn);

/******************************/
/*   VU Upper instructions    */
/******************************/

extern void _vuABS(VURegs * VU);
extern void _vuADD(VURegs * VU);
extern void _vuADDi(VURegs * VU);
extern void _vuADDq(VURegs * VU);
extern void _vuADDx(VURegs * VU);
extern void _vuADDy(VURegs * VU);
extern void _vuADDz(VURegs * VU);
extern void _vuADDw(VURegs * VU);
extern void _vuADDA(VURegs * VU);
extern void _vuADDAi(VURegs * VU);
extern void _vuADDAq(VURegs * VU);
extern void _vuADDAx(VURegs * VU);
extern void _vuADDAy(VURegs * VU);
extern void _vuADDAz(VURegs * VU);
extern void _vuADDAw(VURegs * VU);
extern void _vuSUB(VURegs * VU);
extern void _vuSUBi(VURegs * VU);
extern void _vuSUBq(VURegs * VU);
extern void _vuSUBx(VURegs * VU);
extern void _vuSUBy(VURegs * VU);
extern void _vuSUBz(VURegs * VU);
extern void _vuSUBw(VURegs * VU);
extern void _vuSUBA(VURegs * VU);
extern void _vuSUBAi(VURegs * VU);
extern void _vuSUBAq(VURegs * VU);
extern void _vuSUBAx(VURegs * VU);
extern void _vuSUBAy(VURegs * VU);
extern void _vuSUBAz(VURegs * VU);
extern void _vuSUBAw(VURegs * VU);
extern void _vuMUL(VURegs * VU);
extern void _vuMULi(VURegs * VU);
extern void _vuMULq(VURegs * VU);
extern void _vuMULx(VURegs * VU);
extern void _vuMULy(VURegs * VU);
extern void _vuMULz(VURegs * VU);
extern void _vuMULw(VURegs * VU);
extern void _vuMULA(VURegs * VU);
extern void _vuMULAi(VURegs * VU);
extern void _vuMULAq(VURegs * VU);
extern void _vuMULAx(VURegs * VU);
extern void _vuMULAy(VURegs * VU);
extern void _vuMULAz(VURegs * VU);
extern void _vuMULAw(VURegs * VU);
extern void _vuMADD(VURegs * VU) ;
extern void _vuMADDi(VURegs * VU);
extern void _vuMADDq(VURegs * VU);
extern void _vuMADDx(VURegs * VU);
extern void _vuMADDy(VURegs * VU);
extern void _vuMADDz(VURegs * VU);
extern void _vuMADDw(VURegs * VU);
extern void _vuMADDA(VURegs * VU);
extern void _vuMADDAi(VURegs * VU);
extern void _vuMADDAq(VURegs * VU);
extern void _vuMADDAx(VURegs * VU);
extern void _vuMADDAy(VURegs * VU);
extern void _vuMADDAz(VURegs * VU);
extern void _vuMADDAw(VURegs * VU);
extern void _vuMSUB(VURegs * VU);
extern void _vuMSUBi(VURegs * VU);
extern void _vuMSUBq(VURegs * VU);
extern void _vuMSUBx(VURegs * VU);
extern void _vuMSUBy(VURegs * VU);
extern void _vuMSUBz(VURegs * VU) ;
extern void _vuMSUBw(VURegs * VU) ;
extern void _vuMSUBA(VURegs * VU);
extern void _vuMSUBAi(VURegs * VU);
extern void _vuMSUBAq(VURegs * VU);
extern void _vuMSUBAx(VURegs * VU);
extern void _vuMSUBAy(VURegs * VU);
extern void _vuMSUBAz(VURegs * VU);
extern void _vuMSUBAw(VURegs * VU);
extern void _vuMAX(VURegs * VU);
extern void _vuMAXi(VURegs * VU);
extern void _vuMAXx(VURegs * VU);
extern void _vuMAXy(VURegs * VU);
extern void _vuMAXz(VURegs * VU);
extern void _vuMAXw(VURegs * VU);
extern void _vuMINI(VURegs * VU);
extern void _vuMINIi(VURegs * VU);
extern void _vuMINIx(VURegs * VU);
extern void _vuMINIy(VURegs * VU);
extern void _vuMINIz(VURegs * VU);
extern void _vuMINIw(VURegs * VU);
extern void _vuOPMULA(VURegs * VU);
extern void _vuOPMSUB(VURegs * VU);
extern void _vuNOP(VURegs * VU);
extern void _vuFTOI0(VURegs * VU);
extern void _vuFTOI4(VURegs * VU);
extern void _vuFTOI12(VURegs * VU);
extern void _vuFTOI15(VURegs * VU);
extern void _vuITOF0(VURegs * VU) ;
extern void _vuITOF4(VURegs * VU) ;
extern void _vuITOF12(VURegs * VU);
extern void _vuITOF15(VURegs * VU);
extern void _vuCLIP(VURegs * VU);
/******************************/
/*   VU Lower instructions    */
/******************************/
extern void _vuDIV(VURegs * VU);
extern void _vuSQRT(VURegs * VU);
extern void _vuRSQRT(VURegs * VU);
extern void _vuIADDI(VURegs * VU);
extern void _vuIADDIU(VURegs * VU);
extern void _vuIADD(VURegs * VU);
extern void _vuIAND(VURegs * VU);
extern void _vuIOR(VURegs * VU);
extern void _vuISUB(VURegs * VU);
extern void _vuISUBIU(VURegs * VU);
extern void _vuMOVE(VURegs * VU);
extern void _vuMFIR(VURegs * VU);
extern void _vuMTIR(VURegs * VU);
extern void _vuMR32(VURegs * VU);
extern void _vuLQ(VURegs * VU) ;
extern void _vuLQD(VURegs * VU);
extern void _vuLQI(VURegs * VU);
extern void _vuSQ(VURegs * VU);
extern void _vuSQD(VURegs * VU);
extern void _vuSQI(VURegs * VU);
extern void _vuILW(VURegs * VU);
extern void _vuISW(VURegs * VU);
extern void _vuILWR(VURegs * VU);
extern void _vuISWR(VURegs * VU);
extern void _vuLOI(VURegs * VU);
extern void _vuRINIT(VURegs * VU);
extern void _vuRGET(VURegs * VU);
extern void _vuRNEXT(VURegs * VU);
extern void _vuRXOR(VURegs * VU);
extern void _vuWAITQ(VURegs * VU);
extern void _vuFSAND(VURegs * VU);
extern void _vuFSEQ(VURegs * VU);
extern void _vuFSOR(VURegs * VU);
extern void _vuFSSET(VURegs * VU);
extern void _vuFMAND(VURegs * VU);
extern void _vuFMEQ(VURegs * VU);
extern void _vuFMOR(VURegs * VU);
extern void _vuFCAND(VURegs * VU);
extern void _vuFCEQ(VURegs * VU);
extern void _vuFCOR(VURegs * VU);
extern void _vuFCSET(VURegs * VU);
extern void _vuFCGET(VURegs * VU);
extern void _vuIBEQ(VURegs * VU);
extern void _vuIBGEZ(VURegs * VU);
extern void _vuIBGTZ(VURegs * VU);
extern void _vuIBLEZ(VURegs * VU);
extern void _vuIBLTZ(VURegs * VU);
extern void _vuIBNE(VURegs * VU);
extern void _vuB(VURegs * VU);
extern void _vuBAL(VURegs * VU);
extern void _vuJR(VURegs * VU);
extern void _vuJALR(VURegs * VU);
extern void _vuMFP(VURegs * VU);
extern void _vuWAITP(VURegs * VU);
extern void _vuESADD(VURegs * VU);
extern void _vuERSADD(VURegs * VU);
extern void _vuELENG(VURegs * VU);
extern void _vuERLENG(VURegs * VU);
extern void _vuEATANxy(VURegs * VU);
extern void _vuEATANxz(VURegs * VU);
extern void _vuESUM(VURegs * VU);
extern void _vuERCPR(VURegs * VU);
extern void _vuESQRT(VURegs * VU);
extern void _vuERSQRT(VURegs * VU);
extern void _vuESIN(VURegs * VU);
extern void _vuEATAN(VURegs * VU);
extern void _vuEEXP(VURegs * VU);
extern void _vuXITOP(VURegs * VU);
extern void _vuXGKICK(VURegs * VU);
extern void _vuXTOP(VURegs * VU);

/******************************/
/*   VU Upper instructions    */
/******************************/

extern void _vuRegsABS(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsADDAw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUB(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSUBAw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMUL(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMULAw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMADDAw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUB(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAq(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMSUBAw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAX(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAXi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAXx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAXy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAXz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMAXw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINI(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINIi(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINIx(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINIy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINIz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMINIw(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsOPMULA(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsOPMSUB(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsNOP(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFTOI0(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFTOI4(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFTOI12(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFTOI15(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsITOF0(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsITOF4(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsITOF12(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsITOF15(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsCLIP(const VURegs* VU, _VURegsNum *VUregsn);
/******************************/
/*   VU Lower instructions    */
/******************************/
extern void _vuRegsDIV(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSQRT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsRSQRT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIADDI(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIADDIU(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIADD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIAND(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIOR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsISUB(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsISUBIU(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMOVE(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMFIR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMTIR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMR32(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsLQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsLQD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsLQI(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSQD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsSQI(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsILW(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsISW(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsILWR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsISWR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsLOI(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsRINIT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsRGET(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsRNEXT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsRXOR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsWAITQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFSAND(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFSEQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFSOR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFSSET(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFMAND(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFMEQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFMOR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFCAND(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFCEQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFCOR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFCSET(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsFCGET(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBEQ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBGEZ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBGTZ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBLEZ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBLTZ(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsIBNE(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsB(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsBAL(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsJR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsJALR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsMFP(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsWAITP(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsESADD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsERSADD(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsELENG(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsERLENG(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsEATANxy(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsEATANxz(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsESUM(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsERCPR(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsESQRT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsERSQRT(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsESIN(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsEATAN(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsEEXP(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsXITOP(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsXGKICK(const VURegs* VU, _VURegsNum *VUregsn);
extern void _vuRegsXTOP(const VURegs* VU, _VURegsNum *VUregsn);
