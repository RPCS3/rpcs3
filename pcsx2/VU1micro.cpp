/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


// This module contains code shared by both the dynarec and interpreter versions
// of the VU0 micro.

#include "PrecompiledHeader.h"
#include "Common.h"

#include <cmath>

#include "VUmicro.h"

VURegs* g_pVU1;

#ifdef PCSX2_DEBUG
u32 vudump = 0;
#endif

#define VF_VAL(x) ((x==0x80000000)?0:(x))

// This is called by the COP2 as per the CTC instruction
void vu1ResetRegs()
{
	VU0.VI[REG_VPU_STAT].UL &= ~0xff00; // stop vu1
	VU0.VI[REG_FBRST].UL &= ~0xff00; // stop vu1
	vif1Regs->stat.VEW = false;
}

static int count;

void __fastcall vu1ExecMicro(u32 addr)
{
	while(VU0.VI[REG_VPU_STAT].UL & 0x100) {
		VUM_LOG("vu1ExecMicro > Stalling until current microprogram finishes");
		CpuVU1->ExecuteBlock(vu1RunCycles);
	}

	VUM_LOG("vu1ExecMicro %x", addr);
	VUM_LOG("vu1ExecMicro %x (count=%d)", addr, count++);

	VU0.VI[REG_VPU_STAT].UL &= ~0xFF00;
	VU0.VI[REG_VPU_STAT].UL |=  0x0100;

	vif1Regs->stat.VEW = true;
	if ((s32)addr != -1) VU1.VI[REG_TPC].UL = addr;
	_vuExecMicroDebug(VU1);

	CpuVU1->ExecuteBlock(vu1RunCycles);
}

_vuRegsTables(VU1, VU1regs);

void VU1unknown() {
	//assert(0);
	CPU_LOG("Unknown VU micromode opcode called"); 
}  
 
void VU1regsunknown(_VURegsNum *VUregsn) {
	//assert(0);
	CPU_LOG("Unknown VU micromode opcode called"); 
}  
 

 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 
 
void VU1MI_ABS()  { _vuABS(&VU1); } 
void VU1MI_ADD()  { _vuADD(&VU1); } 
void VU1MI_ADDi() { _vuADDi(&VU1); }
void VU1MI_ADDq() { _vuADDq(&VU1); }
void VU1MI_ADDx() { _vuADDx(&VU1); }
void VU1MI_ADDy() { _vuADDy(&VU1); }
void VU1MI_ADDz() { _vuADDz(&VU1); }
void VU1MI_ADDw() { _vuADDw(&VU1); } 
void VU1MI_ADDA() { _vuADDA(&VU1); } 
void VU1MI_ADDAi() { _vuADDAi(&VU1); } 
void VU1MI_ADDAq() { _vuADDAq(&VU1); } 
void VU1MI_ADDAx() { _vuADDAx(&VU1); } 
void VU1MI_ADDAy() { _vuADDAy(&VU1); } 
void VU1MI_ADDAz() { _vuADDAz(&VU1); } 
void VU1MI_ADDAw() { _vuADDAw(&VU1); } 
void VU1MI_SUB()  { _vuSUB(&VU1); } 
void VU1MI_SUBi() { _vuSUBi(&VU1); } 
void VU1MI_SUBq() { _vuSUBq(&VU1); } 
void VU1MI_SUBx() { _vuSUBx(&VU1); } 
void VU1MI_SUBy() { _vuSUBy(&VU1); } 
void VU1MI_SUBz() { _vuSUBz(&VU1); } 
void VU1MI_SUBw() { _vuSUBw(&VU1); } 
void VU1MI_SUBA()  { _vuSUBA(&VU1); } 
void VU1MI_SUBAi() { _vuSUBAi(&VU1); } 
void VU1MI_SUBAq() { _vuSUBAq(&VU1); } 
void VU1MI_SUBAx() { _vuSUBAx(&VU1); } 
void VU1MI_SUBAy() { _vuSUBAy(&VU1); } 
void VU1MI_SUBAz() { _vuSUBAz(&VU1); } 
void VU1MI_SUBAw() { _vuSUBAw(&VU1); } 
void VU1MI_MUL()  { _vuMUL(&VU1); } 
void VU1MI_MULi() { _vuMULi(&VU1); } 
void VU1MI_MULq() { _vuMULq(&VU1); } 
void VU1MI_MULx() { _vuMULx(&VU1); } 
void VU1MI_MULy() { _vuMULy(&VU1); } 
void VU1MI_MULz() { _vuMULz(&VU1); } 
void VU1MI_MULw() { _vuMULw(&VU1); } 
void VU1MI_MULA()  { _vuMULA(&VU1); } 
void VU1MI_MULAi() { _vuMULAi(&VU1); } 
void VU1MI_MULAq() { _vuMULAq(&VU1); } 
void VU1MI_MULAx() { _vuMULAx(&VU1); } 
void VU1MI_MULAy() { _vuMULAy(&VU1); } 
void VU1MI_MULAz() { _vuMULAz(&VU1); } 
void VU1MI_MULAw() { _vuMULAw(&VU1); } 
void VU1MI_MADD()  { _vuMADD(&VU1); } 
void VU1MI_MADDi() { _vuMADDi(&VU1); } 
void VU1MI_MADDq() { _vuMADDq(&VU1); } 
void VU1MI_MADDx() { _vuMADDx(&VU1); } 
void VU1MI_MADDy() { _vuMADDy(&VU1); } 
void VU1MI_MADDz() { _vuMADDz(&VU1); } 
void VU1MI_MADDw() { _vuMADDw(&VU1); } 
void VU1MI_MADDA()  { _vuMADDA(&VU1); } 
void VU1MI_MADDAi() { _vuMADDAi(&VU1); } 
void VU1MI_MADDAq() { _vuMADDAq(&VU1); } 
void VU1MI_MADDAx() { _vuMADDAx(&VU1); } 
void VU1MI_MADDAy() { _vuMADDAy(&VU1); } 
void VU1MI_MADDAz() { _vuMADDAz(&VU1); } 
void VU1MI_MADDAw() { _vuMADDAw(&VU1); } 
void VU1MI_MSUB()  { _vuMSUB(&VU1); } 
void VU1MI_MSUBi() { _vuMSUBi(&VU1); } 
void VU1MI_MSUBq() { _vuMSUBq(&VU1); } 
void VU1MI_MSUBx() { _vuMSUBx(&VU1); } 
void VU1MI_MSUBy() { _vuMSUBy(&VU1); } 
void VU1MI_MSUBz() { _vuMSUBz(&VU1); } 
void VU1MI_MSUBw() { _vuMSUBw(&VU1); } 
void VU1MI_MSUBA()  { _vuMSUBA(&VU1); } 
void VU1MI_MSUBAi() { _vuMSUBAi(&VU1); } 
void VU1MI_MSUBAq() { _vuMSUBAq(&VU1); } 
void VU1MI_MSUBAx() { _vuMSUBAx(&VU1); } 
void VU1MI_MSUBAy() { _vuMSUBAy(&VU1); } 
void VU1MI_MSUBAz() { _vuMSUBAz(&VU1); } 
void VU1MI_MSUBAw() { _vuMSUBAw(&VU1); } 
void VU1MI_MAX()  { _vuMAX(&VU1); } 
void VU1MI_MAXi() { _vuMAXi(&VU1); } 
void VU1MI_MAXx() { _vuMAXx(&VU1); } 
void VU1MI_MAXy() { _vuMAXy(&VU1); } 
void VU1MI_MAXz() { _vuMAXz(&VU1); } 
void VU1MI_MAXw() { _vuMAXw(&VU1); } 
void VU1MI_MINI()  { _vuMINI(&VU1); } 
void VU1MI_MINIi() { _vuMINIi(&VU1); } 
void VU1MI_MINIx() { _vuMINIx(&VU1); } 
void VU1MI_MINIy() { _vuMINIy(&VU1); } 
void VU1MI_MINIz() { _vuMINIz(&VU1); } 
void VU1MI_MINIw() { _vuMINIw(&VU1); } 
void VU1MI_OPMULA() { _vuOPMULA(&VU1); } 
void VU1MI_OPMSUB() { _vuOPMSUB(&VU1); } 
void VU1MI_NOP() { _vuNOP(&VU1); } 
void VU1MI_FTOI0()  { _vuFTOI0(&VU1); } 
void VU1MI_FTOI4()  { _vuFTOI4(&VU1); } 
void VU1MI_FTOI12() { _vuFTOI12(&VU1); } 
void VU1MI_FTOI15() { _vuFTOI15(&VU1); } 
void VU1MI_ITOF0()  { _vuITOF0(&VU1); } 
void VU1MI_ITOF4()  { _vuITOF4(&VU1); } 
void VU1MI_ITOF12() { _vuITOF12(&VU1); } 
void VU1MI_ITOF15() { _vuITOF15(&VU1); } 
void VU1MI_CLIP() { _vuCLIP(&VU1); } 
 
/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 
 
void VU1MI_DIV() { _vuDIV(&VU1); } 
void VU1MI_SQRT() { _vuSQRT(&VU1); } 
void VU1MI_RSQRT() { _vuRSQRT(&VU1); } 
void VU1MI_IADD() { _vuIADD(&VU1); } 
void VU1MI_IADDI() { _vuIADDI(&VU1); } 
void VU1MI_IADDIU() { _vuIADDIU(&VU1); } 
void VU1MI_IAND() { _vuIAND(&VU1); } 
void VU1MI_IOR() { _vuIOR(&VU1); } 
void VU1MI_ISUB() { _vuISUB(&VU1); } 
void VU1MI_ISUBIU() { _vuISUBIU(&VU1); } 
void VU1MI_MOVE() { _vuMOVE(&VU1); } 
void VU1MI_MFIR() { _vuMFIR(&VU1); } 
void VU1MI_MTIR() { _vuMTIR(&VU1); } 
void VU1MI_MR32() { _vuMR32(&VU1); } 
void VU1MI_LQ() { _vuLQ(&VU1); } 
void VU1MI_LQD() { _vuLQD(&VU1); } 
void VU1MI_LQI() { _vuLQI(&VU1); } 
void VU1MI_SQ() { _vuSQ(&VU1); } 
void VU1MI_SQD() { _vuSQD(&VU1); } 
void VU1MI_SQI() { _vuSQI(&VU1); } 
void VU1MI_ILW() { _vuILW(&VU1); } 
void VU1MI_ISW() { _vuISW(&VU1); } 
void VU1MI_ILWR() { _vuILWR(&VU1); } 
void VU1MI_ISWR() { _vuISWR(&VU1); } 
void VU1MI_RINIT() { _vuRINIT(&VU1); } 
void VU1MI_RGET()  { _vuRGET(&VU1); } 
void VU1MI_RNEXT() { _vuRNEXT(&VU1); } 
void VU1MI_RXOR()  { _vuRXOR(&VU1); } 
void VU1MI_WAITQ() { _vuWAITQ(&VU1); } 
void VU1MI_FSAND() { _vuFSAND(&VU1); } 
void VU1MI_FSEQ()  { _vuFSEQ(&VU1); } 
void VU1MI_FSOR()  { _vuFSOR(&VU1); } 
void VU1MI_FSSET() { _vuFSSET(&VU1); } 
void VU1MI_FMAND() { _vuFMAND(&VU1); } 
void VU1MI_FMEQ()  { _vuFMEQ(&VU1); } 
void VU1MI_FMOR()  { _vuFMOR(&VU1); } 
void VU1MI_FCAND() { _vuFCAND(&VU1); } 
void VU1MI_FCEQ()  { _vuFCEQ(&VU1); } 
void VU1MI_FCOR()  { _vuFCOR(&VU1); } 
void VU1MI_FCSET() { _vuFCSET(&VU1); } 
void VU1MI_FCGET() { _vuFCGET(&VU1); } 
void VU1MI_IBEQ() { _vuIBEQ(&VU1); } 
void VU1MI_IBGEZ() { _vuIBGEZ(&VU1); } 
void VU1MI_IBGTZ() { _vuIBGTZ(&VU1); } 
void VU1MI_IBLTZ() { _vuIBLTZ(&VU1); } 
void VU1MI_IBLEZ() { _vuIBLEZ(&VU1); } 
void VU1MI_IBNE() { _vuIBNE(&VU1); } 
void VU1MI_B()   { _vuB(&VU1); } 
void VU1MI_BAL() { _vuBAL(&VU1); } 
void VU1MI_JR()   { _vuJR(&VU1); } 
void VU1MI_JALR() { _vuJALR(&VU1); } 
void VU1MI_MFP() { _vuMFP(&VU1); } 
void VU1MI_WAITP() { _vuWAITP(&VU1); } 
void VU1MI_ESADD()   { _vuESADD(&VU1); } 
void VU1MI_ERSADD()  { _vuERSADD(&VU1); } 
void VU1MI_ELENG()   { _vuELENG(&VU1); } 
void VU1MI_ERLENG()  { _vuERLENG(&VU1); } 
void VU1MI_EATANxy() { _vuEATANxy(&VU1); } 
void VU1MI_EATANxz() { _vuEATANxz(&VU1); } 
void VU1MI_ESUM()    { _vuESUM(&VU1); } 
void VU1MI_ERCPR()   { _vuERCPR(&VU1); } 
void VU1MI_ESQRT()   { _vuESQRT(&VU1); } 
void VU1MI_ERSQRT()  { _vuERSQRT(&VU1); } 
void VU1MI_ESIN()    { _vuESIN(&VU1); } 
void VU1MI_EATAN()   { _vuEATAN(&VU1); } 
void VU1MI_EEXP()    { _vuEEXP(&VU1); } 
void VU1MI_XITOP()   { _vuXITOP(&VU1); }
void VU1MI_XGKICK()  { _vuXGKICK(&VU1); }
void VU1MI_XTOP()    { _vuXTOP(&VU1); }


 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 
 
void VU1regsMI_ABS(_VURegsNum *VUregsn)  { _vuRegsABS(&VU1, VUregsn); } 
void VU1regsMI_ADD(_VURegsNum *VUregsn)  { _vuRegsADD(&VU1, VUregsn); } 
void VU1regsMI_ADDi(_VURegsNum *VUregsn) { _vuRegsADDi(&VU1, VUregsn); }
void VU1regsMI_ADDq(_VURegsNum *VUregsn) { _vuRegsADDq(&VU1, VUregsn); }
void VU1regsMI_ADDx(_VURegsNum *VUregsn) { _vuRegsADDx(&VU1, VUregsn); }
void VU1regsMI_ADDy(_VURegsNum *VUregsn) { _vuRegsADDy(&VU1, VUregsn); }
void VU1regsMI_ADDz(_VURegsNum *VUregsn) { _vuRegsADDz(&VU1, VUregsn); }
void VU1regsMI_ADDw(_VURegsNum *VUregsn) { _vuRegsADDw(&VU1, VUregsn); } 
void VU1regsMI_ADDA(_VURegsNum *VUregsn) { _vuRegsADDA(&VU1, VUregsn); } 
void VU1regsMI_ADDAi(_VURegsNum *VUregsn) { _vuRegsADDAi(&VU1, VUregsn); } 
void VU1regsMI_ADDAq(_VURegsNum *VUregsn) { _vuRegsADDAq(&VU1, VUregsn); } 
void VU1regsMI_ADDAx(_VURegsNum *VUregsn) { _vuRegsADDAx(&VU1, VUregsn); } 
void VU1regsMI_ADDAy(_VURegsNum *VUregsn) { _vuRegsADDAy(&VU1, VUregsn); } 
void VU1regsMI_ADDAz(_VURegsNum *VUregsn) { _vuRegsADDAz(&VU1, VUregsn); } 
void VU1regsMI_ADDAw(_VURegsNum *VUregsn) { _vuRegsADDAw(&VU1, VUregsn); } 
void VU1regsMI_SUB(_VURegsNum *VUregsn)  { _vuRegsSUB(&VU1, VUregsn); } 
void VU1regsMI_SUBi(_VURegsNum *VUregsn) { _vuRegsSUBi(&VU1, VUregsn); } 
void VU1regsMI_SUBq(_VURegsNum *VUregsn) { _vuRegsSUBq(&VU1, VUregsn); } 
void VU1regsMI_SUBx(_VURegsNum *VUregsn) { _vuRegsSUBx(&VU1, VUregsn); } 
void VU1regsMI_SUBy(_VURegsNum *VUregsn) { _vuRegsSUBy(&VU1, VUregsn); } 
void VU1regsMI_SUBz(_VURegsNum *VUregsn) { _vuRegsSUBz(&VU1, VUregsn); } 
void VU1regsMI_SUBw(_VURegsNum *VUregsn) { _vuRegsSUBw(&VU1, VUregsn); } 
void VU1regsMI_SUBA(_VURegsNum *VUregsn)  { _vuRegsSUBA(&VU1, VUregsn); } 
void VU1regsMI_SUBAi(_VURegsNum *VUregsn) { _vuRegsSUBAi(&VU1, VUregsn); } 
void VU1regsMI_SUBAq(_VURegsNum *VUregsn) { _vuRegsSUBAq(&VU1, VUregsn); } 
void VU1regsMI_SUBAx(_VURegsNum *VUregsn) { _vuRegsSUBAx(&VU1, VUregsn); } 
void VU1regsMI_SUBAy(_VURegsNum *VUregsn) { _vuRegsSUBAy(&VU1, VUregsn); } 
void VU1regsMI_SUBAz(_VURegsNum *VUregsn) { _vuRegsSUBAz(&VU1, VUregsn); } 
void VU1regsMI_SUBAw(_VURegsNum *VUregsn) { _vuRegsSUBAw(&VU1, VUregsn); } 
void VU1regsMI_MUL(_VURegsNum *VUregsn)  { _vuRegsMUL(&VU1, VUregsn); } 
void VU1regsMI_MULi(_VURegsNum *VUregsn) { _vuRegsMULi(&VU1, VUregsn); } 
void VU1regsMI_MULq(_VURegsNum *VUregsn) { _vuRegsMULq(&VU1, VUregsn); } 
void VU1regsMI_MULx(_VURegsNum *VUregsn) { _vuRegsMULx(&VU1, VUregsn); } 
void VU1regsMI_MULy(_VURegsNum *VUregsn) { _vuRegsMULy(&VU1, VUregsn); } 
void VU1regsMI_MULz(_VURegsNum *VUregsn) { _vuRegsMULz(&VU1, VUregsn); } 
void VU1regsMI_MULw(_VURegsNum *VUregsn) { _vuRegsMULw(&VU1, VUregsn); } 
void VU1regsMI_MULA(_VURegsNum *VUregsn)  { _vuRegsMULA(&VU1, VUregsn); } 
void VU1regsMI_MULAi(_VURegsNum *VUregsn) { _vuRegsMULAi(&VU1, VUregsn); } 
void VU1regsMI_MULAq(_VURegsNum *VUregsn) { _vuRegsMULAq(&VU1, VUregsn); } 
void VU1regsMI_MULAx(_VURegsNum *VUregsn) { _vuRegsMULAx(&VU1, VUregsn); } 
void VU1regsMI_MULAy(_VURegsNum *VUregsn) { _vuRegsMULAy(&VU1, VUregsn); } 
void VU1regsMI_MULAz(_VURegsNum *VUregsn) { _vuRegsMULAz(&VU1, VUregsn); } 
void VU1regsMI_MULAw(_VURegsNum *VUregsn) { _vuRegsMULAw(&VU1, VUregsn); } 
void VU1regsMI_MADD(_VURegsNum *VUregsn)  { _vuRegsMADD(&VU1, VUregsn); } 
void VU1regsMI_MADDi(_VURegsNum *VUregsn) { _vuRegsMADDi(&VU1, VUregsn); } 
void VU1regsMI_MADDq(_VURegsNum *VUregsn) { _vuRegsMADDq(&VU1, VUregsn); } 
void VU1regsMI_MADDx(_VURegsNum *VUregsn) { _vuRegsMADDx(&VU1, VUregsn); } 
void VU1regsMI_MADDy(_VURegsNum *VUregsn) { _vuRegsMADDy(&VU1, VUregsn); } 
void VU1regsMI_MADDz(_VURegsNum *VUregsn) { _vuRegsMADDz(&VU1, VUregsn); } 
void VU1regsMI_MADDw(_VURegsNum *VUregsn) { _vuRegsMADDw(&VU1, VUregsn); } 
void VU1regsMI_MADDA(_VURegsNum *VUregsn)  { _vuRegsMADDA(&VU1, VUregsn); } 
void VU1regsMI_MADDAi(_VURegsNum *VUregsn) { _vuRegsMADDAi(&VU1, VUregsn); } 
void VU1regsMI_MADDAq(_VURegsNum *VUregsn) { _vuRegsMADDAq(&VU1, VUregsn); } 
void VU1regsMI_MADDAx(_VURegsNum *VUregsn) { _vuRegsMADDAx(&VU1, VUregsn); } 
void VU1regsMI_MADDAy(_VURegsNum *VUregsn) { _vuRegsMADDAy(&VU1, VUregsn); } 
void VU1regsMI_MADDAz(_VURegsNum *VUregsn) { _vuRegsMADDAz(&VU1, VUregsn); } 
void VU1regsMI_MADDAw(_VURegsNum *VUregsn) { _vuRegsMADDAw(&VU1, VUregsn); } 
void VU1regsMI_MSUB(_VURegsNum *VUregsn)  { _vuRegsMSUB(&VU1, VUregsn); } 
void VU1regsMI_MSUBi(_VURegsNum *VUregsn) { _vuRegsMSUBi(&VU1, VUregsn); } 
void VU1regsMI_MSUBq(_VURegsNum *VUregsn) { _vuRegsMSUBq(&VU1, VUregsn); } 
void VU1regsMI_MSUBx(_VURegsNum *VUregsn) { _vuRegsMSUBx(&VU1, VUregsn); } 
void VU1regsMI_MSUBy(_VURegsNum *VUregsn) { _vuRegsMSUBy(&VU1, VUregsn); } 
void VU1regsMI_MSUBz(_VURegsNum *VUregsn) { _vuRegsMSUBz(&VU1, VUregsn); } 
void VU1regsMI_MSUBw(_VURegsNum *VUregsn) { _vuRegsMSUBw(&VU1, VUregsn); } 
void VU1regsMI_MSUBA(_VURegsNum *VUregsn)  { _vuRegsMSUBA(&VU1, VUregsn); } 
void VU1regsMI_MSUBAi(_VURegsNum *VUregsn) { _vuRegsMSUBAi(&VU1, VUregsn); } 
void VU1regsMI_MSUBAq(_VURegsNum *VUregsn) { _vuRegsMSUBAq(&VU1, VUregsn); } 
void VU1regsMI_MSUBAx(_VURegsNum *VUregsn) { _vuRegsMSUBAx(&VU1, VUregsn); } 
void VU1regsMI_MSUBAy(_VURegsNum *VUregsn) { _vuRegsMSUBAy(&VU1, VUregsn); } 
void VU1regsMI_MSUBAz(_VURegsNum *VUregsn) { _vuRegsMSUBAz(&VU1, VUregsn); } 
void VU1regsMI_MSUBAw(_VURegsNum *VUregsn) { _vuRegsMSUBAw(&VU1, VUregsn); } 
void VU1regsMI_MAX(_VURegsNum *VUregsn)  { _vuRegsMAX(&VU1, VUregsn); } 
void VU1regsMI_MAXi(_VURegsNum *VUregsn) { _vuRegsMAXi(&VU1, VUregsn); } 
void VU1regsMI_MAXx(_VURegsNum *VUregsn) { _vuRegsMAXx(&VU1, VUregsn); } 
void VU1regsMI_MAXy(_VURegsNum *VUregsn) { _vuRegsMAXy(&VU1, VUregsn); } 
void VU1regsMI_MAXz(_VURegsNum *VUregsn) { _vuRegsMAXz(&VU1, VUregsn); } 
void VU1regsMI_MAXw(_VURegsNum *VUregsn) { _vuRegsMAXw(&VU1, VUregsn); } 
void VU1regsMI_MINI(_VURegsNum *VUregsn)  { _vuRegsMINI(&VU1, VUregsn); } 
void VU1regsMI_MINIi(_VURegsNum *VUregsn) { _vuRegsMINIi(&VU1, VUregsn); } 
void VU1regsMI_MINIx(_VURegsNum *VUregsn) { _vuRegsMINIx(&VU1, VUregsn); } 
void VU1regsMI_MINIy(_VURegsNum *VUregsn) { _vuRegsMINIy(&VU1, VUregsn); } 
void VU1regsMI_MINIz(_VURegsNum *VUregsn) { _vuRegsMINIz(&VU1, VUregsn); } 
void VU1regsMI_MINIw(_VURegsNum *VUregsn) { _vuRegsMINIw(&VU1, VUregsn); } 
void VU1regsMI_OPMULA(_VURegsNum *VUregsn) { _vuRegsOPMULA(&VU1, VUregsn); } 
void VU1regsMI_OPMSUB(_VURegsNum *VUregsn) { _vuRegsOPMSUB(&VU1, VUregsn); } 
void VU1regsMI_NOP(_VURegsNum *VUregsn) { _vuRegsNOP(&VU1, VUregsn); } 
void VU1regsMI_FTOI0(_VURegsNum *VUregsn)  { _vuRegsFTOI0(&VU1, VUregsn); } 
void VU1regsMI_FTOI4(_VURegsNum *VUregsn)  { _vuRegsFTOI4(&VU1, VUregsn); } 
void VU1regsMI_FTOI12(_VURegsNum *VUregsn) { _vuRegsFTOI12(&VU1, VUregsn); } 
void VU1regsMI_FTOI15(_VURegsNum *VUregsn) { _vuRegsFTOI15(&VU1, VUregsn); } 
void VU1regsMI_ITOF0(_VURegsNum *VUregsn)  { _vuRegsITOF0(&VU1, VUregsn); } 
void VU1regsMI_ITOF4(_VURegsNum *VUregsn)  { _vuRegsITOF4(&VU1, VUregsn); } 
void VU1regsMI_ITOF12(_VURegsNum *VUregsn) { _vuRegsITOF12(&VU1, VUregsn); } 
void VU1regsMI_ITOF15(_VURegsNum *VUregsn) { _vuRegsITOF15(&VU1, VUregsn); } 
void VU1regsMI_CLIP(_VURegsNum *VUregsn) { _vuRegsCLIP(&VU1, VUregsn); } 
 
/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 
 
void VU1regsMI_DIV(_VURegsNum *VUregsn) { _vuRegsDIV(&VU1, VUregsn); } 
void VU1regsMI_SQRT(_VURegsNum *VUregsn) { _vuRegsSQRT(&VU1, VUregsn); } 
void VU1regsMI_RSQRT(_VURegsNum *VUregsn) { _vuRegsRSQRT(&VU1, VUregsn); } 
void VU1regsMI_IADD(_VURegsNum *VUregsn) { _vuRegsIADD(&VU1, VUregsn); } 
void VU1regsMI_IADDI(_VURegsNum *VUregsn) { _vuRegsIADDI(&VU1, VUregsn); } 
void VU1regsMI_IADDIU(_VURegsNum *VUregsn) { _vuRegsIADDIU(&VU1, VUregsn); } 
void VU1regsMI_IAND(_VURegsNum *VUregsn) { _vuRegsIAND(&VU1, VUregsn); } 
void VU1regsMI_IOR(_VURegsNum *VUregsn) { _vuRegsIOR(&VU1, VUregsn); } 
void VU1regsMI_ISUB(_VURegsNum *VUregsn) { _vuRegsISUB(&VU1, VUregsn); } 
void VU1regsMI_ISUBIU(_VURegsNum *VUregsn) { _vuRegsISUBIU(&VU1, VUregsn); } 
void VU1regsMI_MOVE(_VURegsNum *VUregsn) { _vuRegsMOVE(&VU1, VUregsn); } 
void VU1regsMI_MFIR(_VURegsNum *VUregsn) { _vuRegsMFIR(&VU1, VUregsn); } 
void VU1regsMI_MTIR(_VURegsNum *VUregsn) { _vuRegsMTIR(&VU1, VUregsn); } 
void VU1regsMI_MR32(_VURegsNum *VUregsn) { _vuRegsMR32(&VU1, VUregsn); } 
void VU1regsMI_LQ(_VURegsNum *VUregsn) { _vuRegsLQ(&VU1, VUregsn); } 
void VU1regsMI_LQD(_VURegsNum *VUregsn) { _vuRegsLQD(&VU1, VUregsn); } 
void VU1regsMI_LQI(_VURegsNum *VUregsn) { _vuRegsLQI(&VU1, VUregsn); } 
void VU1regsMI_SQ(_VURegsNum *VUregsn) { _vuRegsSQ(&VU1, VUregsn); } 
void VU1regsMI_SQD(_VURegsNum *VUregsn) { _vuRegsSQD(&VU1, VUregsn); } 
void VU1regsMI_SQI(_VURegsNum *VUregsn) { _vuRegsSQI(&VU1, VUregsn); } 
void VU1regsMI_ILW(_VURegsNum *VUregsn) { _vuRegsILW(&VU1, VUregsn); } 
void VU1regsMI_ISW(_VURegsNum *VUregsn) { _vuRegsISW(&VU1, VUregsn); } 
void VU1regsMI_ILWR(_VURegsNum *VUregsn) { _vuRegsILWR(&VU1, VUregsn); } 
void VU1regsMI_ISWR(_VURegsNum *VUregsn) { _vuRegsISWR(&VU1, VUregsn); } 
void VU1regsMI_RINIT(_VURegsNum *VUregsn) { _vuRegsRINIT(&VU1, VUregsn); } 
void VU1regsMI_RGET(_VURegsNum *VUregsn)  { _vuRegsRGET(&VU1, VUregsn); } 
void VU1regsMI_RNEXT(_VURegsNum *VUregsn) { _vuRegsRNEXT(&VU1, VUregsn); } 
void VU1regsMI_RXOR(_VURegsNum *VUregsn)  { _vuRegsRXOR(&VU1, VUregsn); } 
void VU1regsMI_WAITQ(_VURegsNum *VUregsn) { _vuRegsWAITQ(&VU1, VUregsn); } 
void VU1regsMI_FSAND(_VURegsNum *VUregsn) { _vuRegsFSAND(&VU1, VUregsn); } 
void VU1regsMI_FSEQ(_VURegsNum *VUregsn)  { _vuRegsFSEQ(&VU1, VUregsn); } 
void VU1regsMI_FSOR(_VURegsNum *VUregsn)  { _vuRegsFSOR(&VU1, VUregsn); } 
void VU1regsMI_FSSET(_VURegsNum *VUregsn) { _vuRegsFSSET(&VU1, VUregsn); } 
void VU1regsMI_FMAND(_VURegsNum *VUregsn) { _vuRegsFMAND(&VU1, VUregsn); } 
void VU1regsMI_FMEQ(_VURegsNum *VUregsn)  { _vuRegsFMEQ(&VU1, VUregsn); } 
void VU1regsMI_FMOR(_VURegsNum *VUregsn)  { _vuRegsFMOR(&VU1, VUregsn); } 
void VU1regsMI_FCAND(_VURegsNum *VUregsn) { _vuRegsFCAND(&VU1, VUregsn); } 
void VU1regsMI_FCEQ(_VURegsNum *VUregsn)  { _vuRegsFCEQ(&VU1, VUregsn); } 
void VU1regsMI_FCOR(_VURegsNum *VUregsn)  { _vuRegsFCOR(&VU1, VUregsn); } 
void VU1regsMI_FCSET(_VURegsNum *VUregsn) { _vuRegsFCSET(&VU1, VUregsn); } 
void VU1regsMI_FCGET(_VURegsNum *VUregsn) { _vuRegsFCGET(&VU1, VUregsn); } 
void VU1regsMI_IBEQ(_VURegsNum *VUregsn) { _vuRegsIBEQ(&VU1, VUregsn); } 
void VU1regsMI_IBGEZ(_VURegsNum *VUregsn) { _vuRegsIBGEZ(&VU1, VUregsn); } 
void VU1regsMI_IBGTZ(_VURegsNum *VUregsn) { _vuRegsIBGTZ(&VU1, VUregsn); } 
void VU1regsMI_IBLTZ(_VURegsNum *VUregsn) { _vuRegsIBLTZ(&VU1, VUregsn); } 
void VU1regsMI_IBLEZ(_VURegsNum *VUregsn) { _vuRegsIBLEZ(&VU1, VUregsn); } 
void VU1regsMI_IBNE(_VURegsNum *VUregsn) { _vuRegsIBNE(&VU1, VUregsn); } 
void VU1regsMI_B(_VURegsNum *VUregsn)   { _vuRegsB(&VU1, VUregsn); } 
void VU1regsMI_BAL(_VURegsNum *VUregsn) { _vuRegsBAL(&VU1, VUregsn); } 
void VU1regsMI_JR(_VURegsNum *VUregsn)   { _vuRegsJR(&VU1, VUregsn); } 
void VU1regsMI_JALR(_VURegsNum *VUregsn) { _vuRegsJALR(&VU1, VUregsn); } 
void VU1regsMI_MFP(_VURegsNum *VUregsn) { _vuRegsMFP(&VU1, VUregsn); } 
void VU1regsMI_WAITP(_VURegsNum *VUregsn) { _vuRegsWAITP(&VU1, VUregsn); } 
void VU1regsMI_ESADD(_VURegsNum *VUregsn)   { _vuRegsESADD(&VU1, VUregsn); } 
void VU1regsMI_ERSADD(_VURegsNum *VUregsn)  { _vuRegsERSADD(&VU1, VUregsn); } 
void VU1regsMI_ELENG(_VURegsNum *VUregsn)   { _vuRegsELENG(&VU1, VUregsn); } 
void VU1regsMI_ERLENG(_VURegsNum *VUregsn)  { _vuRegsERLENG(&VU1, VUregsn); } 
void VU1regsMI_EATANxy(_VURegsNum *VUregsn) { _vuRegsEATANxy(&VU1, VUregsn); } 
void VU1regsMI_EATANxz(_VURegsNum *VUregsn) { _vuRegsEATANxz(&VU1, VUregsn); } 
void VU1regsMI_ESUM(_VURegsNum *VUregsn)    { _vuRegsESUM(&VU1, VUregsn); } 
void VU1regsMI_ERCPR(_VURegsNum *VUregsn)   { _vuRegsERCPR(&VU1, VUregsn); } 
void VU1regsMI_ESQRT(_VURegsNum *VUregsn)   { _vuRegsESQRT(&VU1, VUregsn); } 
void VU1regsMI_ERSQRT(_VURegsNum *VUregsn)  { _vuRegsERSQRT(&VU1, VUregsn); } 
void VU1regsMI_ESIN(_VURegsNum *VUregsn)    { _vuRegsESIN(&VU1, VUregsn); } 
void VU1regsMI_EATAN(_VURegsNum *VUregsn)   { _vuRegsEATAN(&VU1, VUregsn); } 
void VU1regsMI_EEXP(_VURegsNum *VUregsn)    { _vuRegsEEXP(&VU1, VUregsn); } 
void VU1regsMI_XITOP(_VURegsNum *VUregsn)   { _vuRegsXITOP(&VU1, VUregsn); }
void VU1regsMI_XGKICK(_VURegsNum *VUregsn)  { _vuRegsXGKICK(&VU1, VUregsn); }
void VU1regsMI_XTOP(_VURegsNum *VUregsn)    { _vuRegsXTOP(&VU1, VUregsn); }
