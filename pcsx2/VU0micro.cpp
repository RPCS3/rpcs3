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

using namespace R5900;

#define VF_VAL(x) ((x==0x80000000)?0:(x))

// This is called by the COP2 as per the CTC instruction
void vu0ResetRegs()
{
	VU0.VI[REG_VPU_STAT].UL &= ~0xff; // stop vu0
	VU0.VI[REG_FBRST].UL &= ~0xff; // stop vu0
	vif0Regs->stat.VEW = false;
}

void VU0MI_XGKICK() {
}

void VU0MI_XTOP() {
}

void __fastcall vu0ExecMicro(u32 addr) {
	VUM_LOG("vu0ExecMicro %x", addr);
	
	if(VU0.VI[REG_VPU_STAT].UL & 0x1) {
		DevCon.Warning("vu0ExecMicro > Stalling for previous microprogram to finish");
		vu0Finish();
	}

	VU0.VI[REG_VPU_STAT].UL &= ~0xFF;
	VU0.VI[REG_VPU_STAT].UL |=  0x01;

	if ((s32)addr != -1) VU0.VI[REG_TPC].UL = addr;
	_vuExecMicroDebug(VU0);
	CpuVU0->ExecuteBlock();

	// If the VU0 program didn't finish then we'll want to finish it up
	// pretty soon.  This fixes vmhacks in some games (Naruto Ultimate Ninja 2)
	if(VU0.VI[REG_VPU_STAT].UL & 0x1)
		cpuSetNextBranchDelta( 192 );		// fixme : ideally this should be higher, like 512 or so.
}

void VU0unknown() {
	pxFailDev("Unknown VU micromode opcode called");
	CPU_LOG("Unknown VU micromode opcode called"); 
}  

void VU0regsunknown(_VURegsNum *VUregsn) {
	pxFailDev("Unknown VU micromode opcode called");
	CPU_LOG("Unknown VU micromode opcode called"); 
}  
 
_vuRegsTables(VU0, VU0regs);
 
 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 
 
void VU0MI_ABS()  { _vuABS(&VU0); } 
void VU0MI_ADD()  { _vuADD(&VU0); } 
void VU0MI_ADDi() { _vuADDi(&VU0); }
void VU0MI_ADDq() { _vuADDq(&VU0); }
void VU0MI_ADDx() { _vuADDx(&VU0); }
void VU0MI_ADDy() { _vuADDy(&VU0); }
void VU0MI_ADDz() { _vuADDz(&VU0); }
void VU0MI_ADDw() { _vuADDw(&VU0); } 
void VU0MI_ADDA() { _vuADDA(&VU0); } 
void VU0MI_ADDAi() { _vuADDAi(&VU0); } 
void VU0MI_ADDAq() { _vuADDAq(&VU0); } 
void VU0MI_ADDAx() { _vuADDAx(&VU0); } 
void VU0MI_ADDAy() { _vuADDAy(&VU0); } 
void VU0MI_ADDAz() { _vuADDAz(&VU0); } 
void VU0MI_ADDAw() { _vuADDAw(&VU0); } 
void VU0MI_SUB()  { _vuSUB(&VU0); } 
void VU0MI_SUBi() { _vuSUBi(&VU0); } 
void VU0MI_SUBq() { _vuSUBq(&VU0); } 
void VU0MI_SUBx() { _vuSUBx(&VU0); } 
void VU0MI_SUBy() { _vuSUBy(&VU0); } 
void VU0MI_SUBz() { _vuSUBz(&VU0); } 
void VU0MI_SUBw() { _vuSUBw(&VU0); } 
void VU0MI_SUBA()  { _vuSUBA(&VU0); } 
void VU0MI_SUBAi() { _vuSUBAi(&VU0); } 
void VU0MI_SUBAq() { _vuSUBAq(&VU0); } 
void VU0MI_SUBAx() { _vuSUBAx(&VU0); } 
void VU0MI_SUBAy() { _vuSUBAy(&VU0); } 
void VU0MI_SUBAz() { _vuSUBAz(&VU0); } 
void VU0MI_SUBAw() { _vuSUBAw(&VU0); } 
void VU0MI_MUL()  { _vuMUL(&VU0); } 
void VU0MI_MULi() { _vuMULi(&VU0); } 
void VU0MI_MULq() { _vuMULq(&VU0); } 
void VU0MI_MULx() { _vuMULx(&VU0); } 
void VU0MI_MULy() { _vuMULy(&VU0); } 
void VU0MI_MULz() { _vuMULz(&VU0); } 
void VU0MI_MULw() { _vuMULw(&VU0); } 
void VU0MI_MULA()  { _vuMULA(&VU0); } 
void VU0MI_MULAi() { _vuMULAi(&VU0); } 
void VU0MI_MULAq() { _vuMULAq(&VU0); } 
void VU0MI_MULAx() { _vuMULAx(&VU0); } 
void VU0MI_MULAy() { _vuMULAy(&VU0); } 
void VU0MI_MULAz() { _vuMULAz(&VU0); } 
void VU0MI_MULAw() { _vuMULAw(&VU0); } 
void VU0MI_MADD()  { _vuMADD(&VU0); } 
void VU0MI_MADDi() { _vuMADDi(&VU0); } 
void VU0MI_MADDq() { _vuMADDq(&VU0); } 
void VU0MI_MADDx() { _vuMADDx(&VU0); } 
void VU0MI_MADDy() { _vuMADDy(&VU0); } 
void VU0MI_MADDz() { _vuMADDz(&VU0); } 
void VU0MI_MADDw() { _vuMADDw(&VU0); } 
void VU0MI_MADDA()  { _vuMADDA(&VU0); } 
void VU0MI_MADDAi() { _vuMADDAi(&VU0); } 
void VU0MI_MADDAq() { _vuMADDAq(&VU0); } 
void VU0MI_MADDAx() { _vuMADDAx(&VU0); } 
void VU0MI_MADDAy() { _vuMADDAy(&VU0); } 
void VU0MI_MADDAz() { _vuMADDAz(&VU0); } 
void VU0MI_MADDAw() { _vuMADDAw(&VU0); } 
void VU0MI_MSUB()  { _vuMSUB(&VU0); } 
void VU0MI_MSUBi() { _vuMSUBi(&VU0); } 
void VU0MI_MSUBq() { _vuMSUBq(&VU0); } 
void VU0MI_MSUBx() { _vuMSUBx(&VU0); } 
void VU0MI_MSUBy() { _vuMSUBy(&VU0); } 
void VU0MI_MSUBz() { _vuMSUBz(&VU0); } 
void VU0MI_MSUBw() { _vuMSUBw(&VU0); } 
void VU0MI_MSUBA()  { _vuMSUBA(&VU0); } 
void VU0MI_MSUBAi() { _vuMSUBAi(&VU0); } 
void VU0MI_MSUBAq() { _vuMSUBAq(&VU0); } 
void VU0MI_MSUBAx() { _vuMSUBAx(&VU0); } 
void VU0MI_MSUBAy() { _vuMSUBAy(&VU0); } 
void VU0MI_MSUBAz() { _vuMSUBAz(&VU0); } 
void VU0MI_MSUBAw() { _vuMSUBAw(&VU0); } 
void VU0MI_MAX()  { _vuMAX(&VU0); } 
void VU0MI_MAXi() { _vuMAXi(&VU0); } 
void VU0MI_MAXx() { _vuMAXx(&VU0); } 
void VU0MI_MAXy() { _vuMAXy(&VU0); } 
void VU0MI_MAXz() { _vuMAXz(&VU0); } 
void VU0MI_MAXw() { _vuMAXw(&VU0); } 
void VU0MI_MINI()  { _vuMINI(&VU0); } 
void VU0MI_MINIi() { _vuMINIi(&VU0); } 
void VU0MI_MINIx() { _vuMINIx(&VU0); } 
void VU0MI_MINIy() { _vuMINIy(&VU0); } 
void VU0MI_MINIz() { _vuMINIz(&VU0); } 
void VU0MI_MINIw() { _vuMINIw(&VU0); } 
void VU0MI_OPMULA() { _vuOPMULA(&VU0); } 
void VU0MI_OPMSUB() { _vuOPMSUB(&VU0); } 
void VU0MI_NOP() { _vuNOP(&VU0); } 
void VU0MI_FTOI0()  { _vuFTOI0(&VU0); } 
void VU0MI_FTOI4()  { _vuFTOI4(&VU0); } 
void VU0MI_FTOI12() { _vuFTOI12(&VU0); } 
void VU0MI_FTOI15() { _vuFTOI15(&VU0); } 
void VU0MI_ITOF0()  { _vuITOF0(&VU0); } 
void VU0MI_ITOF4()  { _vuITOF4(&VU0); } 
void VU0MI_ITOF12() { _vuITOF12(&VU0); } 
void VU0MI_ITOF15() { _vuITOF15(&VU0); } 
void VU0MI_CLIP() { _vuCLIP(&VU0); } 
 
/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 
 
void VU0MI_DIV() { _vuDIV(&VU0); } 
void VU0MI_SQRT() { _vuSQRT(&VU0); } 
void VU0MI_RSQRT() { _vuRSQRT(&VU0); } 
void VU0MI_IADD() { _vuIADD(&VU0); } 
void VU0MI_IADDI() { _vuIADDI(&VU0); } 
void VU0MI_IADDIU() { _vuIADDIU(&VU0); } 
void VU0MI_IAND() { _vuIAND(&VU0); } 
void VU0MI_IOR() { _vuIOR(&VU0); } 
void VU0MI_ISUB() { _vuISUB(&VU0); } 
void VU0MI_ISUBIU() { _vuISUBIU(&VU0); } 
void VU0MI_MOVE() { _vuMOVE(&VU0); } 
void VU0MI_MFIR() { _vuMFIR(&VU0); } 
void VU0MI_MTIR() { _vuMTIR(&VU0); } 
void VU0MI_MR32() { _vuMR32(&VU0); } 
void VU0MI_LQ() { _vuLQ(&VU0); } 
void VU0MI_LQD() { _vuLQD(&VU0); } 
void VU0MI_LQI() { _vuLQI(&VU0); } 
void VU0MI_SQ() { _vuSQ(&VU0); } 
void VU0MI_SQD() { _vuSQD(&VU0); } 
void VU0MI_SQI() { _vuSQI(&VU0); } 
void VU0MI_ILW() { _vuILW(&VU0); } 
void VU0MI_ISW() { _vuISW(&VU0); } 
void VU0MI_ILWR() { _vuILWR(&VU0); } 
void VU0MI_ISWR() { _vuISWR(&VU0); } 
void VU0MI_RINIT() { _vuRINIT(&VU0); } 
void VU0MI_RGET()  { _vuRGET(&VU0); } 
void VU0MI_RNEXT() { _vuRNEXT(&VU0); } 
void VU0MI_RXOR()  { _vuRXOR(&VU0); } 
void VU0MI_WAITQ() { _vuWAITQ(&VU0); } 
void VU0MI_FSAND() { _vuFSAND(&VU0); } 
void VU0MI_FSEQ()  { _vuFSEQ(&VU0); } 
void VU0MI_FSOR()  { _vuFSOR(&VU0); } 
void VU0MI_FSSET() { _vuFSSET(&VU0); } 
void VU0MI_FMAND() { _vuFMAND(&VU0); } 
void VU0MI_FMEQ()  { _vuFMEQ(&VU0); } 
void VU0MI_FMOR()  { _vuFMOR(&VU0); } 
void VU0MI_FCAND() { _vuFCAND(&VU0); } 
void VU0MI_FCEQ()  { _vuFCEQ(&VU0); } 
void VU0MI_FCOR()  { _vuFCOR(&VU0); } 
void VU0MI_FCSET() { _vuFCSET(&VU0); } 
void VU0MI_FCGET() { _vuFCGET(&VU0); } 
void VU0MI_IBEQ() { _vuIBEQ(&VU0); } 
void VU0MI_IBGEZ() { _vuIBGEZ(&VU0); } 
void VU0MI_IBGTZ() { _vuIBGTZ(&VU0); } 
void VU0MI_IBLTZ() { _vuIBLTZ(&VU0); } 
void VU0MI_IBLEZ() { _vuIBLEZ(&VU0); } 
void VU0MI_IBNE() { _vuIBNE(&VU0); } 
void VU0MI_B()   { _vuB(&VU0); } 
void VU0MI_BAL() { _vuBAL(&VU0); } 
void VU0MI_JR()   { _vuJR(&VU0); } 
void VU0MI_JALR() { _vuJALR(&VU0); } 
void VU0MI_MFP() { _vuMFP(&VU0); } 
void VU0MI_WAITP() { _vuWAITP(&VU0); } 
void VU0MI_ESADD()   { _vuESADD(&VU0); } 
void VU0MI_ERSADD()  { _vuERSADD(&VU0); } 
void VU0MI_ELENG()   { _vuELENG(&VU0); } 
void VU0MI_ERLENG()  { _vuERLENG(&VU0); } 
void VU0MI_EATANxy() { _vuEATANxy(&VU0); } 
void VU0MI_EATANxz() { _vuEATANxz(&VU0); } 
void VU0MI_ESUM()    { _vuESUM(&VU0); } 
void VU0MI_ERCPR()   { _vuERCPR(&VU0); } 
void VU0MI_ESQRT()   { _vuESQRT(&VU0); } 
void VU0MI_ERSQRT()  { _vuERSQRT(&VU0); } 
void VU0MI_ESIN()    { _vuESIN(&VU0); } 
void VU0MI_EATAN()   { _vuEATAN(&VU0); } 
void VU0MI_EEXP()    { _vuEEXP(&VU0); } 
void VU0MI_XITOP() { _vuXITOP(&VU0); }

/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 
 
void VU0regsMI_ABS(_VURegsNum *VUregsn)  { _vuRegsABS(&VU0, VUregsn); } 
void VU0regsMI_ADD(_VURegsNum *VUregsn)  { _vuRegsADD(&VU0, VUregsn); } 
void VU0regsMI_ADDi(_VURegsNum *VUregsn) { _vuRegsADDi(&VU0, VUregsn); }
void VU0regsMI_ADDq(_VURegsNum *VUregsn) { _vuRegsADDq(&VU0, VUregsn); }
void VU0regsMI_ADDx(_VURegsNum *VUregsn) { _vuRegsADDx(&VU0, VUregsn); }
void VU0regsMI_ADDy(_VURegsNum *VUregsn) { _vuRegsADDy(&VU0, VUregsn); }
void VU0regsMI_ADDz(_VURegsNum *VUregsn) { _vuRegsADDz(&VU0, VUregsn); }
void VU0regsMI_ADDw(_VURegsNum *VUregsn) { _vuRegsADDw(&VU0, VUregsn); } 
void VU0regsMI_ADDA(_VURegsNum *VUregsn) { _vuRegsADDA(&VU0, VUregsn); } 
void VU0regsMI_ADDAi(_VURegsNum *VUregsn) { _vuRegsADDAi(&VU0, VUregsn); } 
void VU0regsMI_ADDAq(_VURegsNum *VUregsn) { _vuRegsADDAq(&VU0, VUregsn); } 
void VU0regsMI_ADDAx(_VURegsNum *VUregsn) { _vuRegsADDAx(&VU0, VUregsn); } 
void VU0regsMI_ADDAy(_VURegsNum *VUregsn) { _vuRegsADDAy(&VU0, VUregsn); } 
void VU0regsMI_ADDAz(_VURegsNum *VUregsn) { _vuRegsADDAz(&VU0, VUregsn); } 
void VU0regsMI_ADDAw(_VURegsNum *VUregsn) { _vuRegsADDAw(&VU0, VUregsn); } 
void VU0regsMI_SUB(_VURegsNum *VUregsn)  { _vuRegsSUB(&VU0, VUregsn); } 
void VU0regsMI_SUBi(_VURegsNum *VUregsn) { _vuRegsSUBi(&VU0, VUregsn); } 
void VU0regsMI_SUBq(_VURegsNum *VUregsn) { _vuRegsSUBq(&VU0, VUregsn); } 
void VU0regsMI_SUBx(_VURegsNum *VUregsn) { _vuRegsSUBx(&VU0, VUregsn); } 
void VU0regsMI_SUBy(_VURegsNum *VUregsn) { _vuRegsSUBy(&VU0, VUregsn); } 
void VU0regsMI_SUBz(_VURegsNum *VUregsn) { _vuRegsSUBz(&VU0, VUregsn); } 
void VU0regsMI_SUBw(_VURegsNum *VUregsn) { _vuRegsSUBw(&VU0, VUregsn); } 
void VU0regsMI_SUBA(_VURegsNum *VUregsn)  { _vuRegsSUBA(&VU0, VUregsn); } 
void VU0regsMI_SUBAi(_VURegsNum *VUregsn) { _vuRegsSUBAi(&VU0, VUregsn); } 
void VU0regsMI_SUBAq(_VURegsNum *VUregsn) { _vuRegsSUBAq(&VU0, VUregsn); } 
void VU0regsMI_SUBAx(_VURegsNum *VUregsn) { _vuRegsSUBAx(&VU0, VUregsn); } 
void VU0regsMI_SUBAy(_VURegsNum *VUregsn) { _vuRegsSUBAy(&VU0, VUregsn); } 
void VU0regsMI_SUBAz(_VURegsNum *VUregsn) { _vuRegsSUBAz(&VU0, VUregsn); } 
void VU0regsMI_SUBAw(_VURegsNum *VUregsn) { _vuRegsSUBAw(&VU0, VUregsn); } 
void VU0regsMI_MUL(_VURegsNum *VUregsn)  { _vuRegsMUL(&VU0, VUregsn); } 
void VU0regsMI_MULi(_VURegsNum *VUregsn) { _vuRegsMULi(&VU0, VUregsn); } 
void VU0regsMI_MULq(_VURegsNum *VUregsn) { _vuRegsMULq(&VU0, VUregsn); } 
void VU0regsMI_MULx(_VURegsNum *VUregsn) { _vuRegsMULx(&VU0, VUregsn); } 
void VU0regsMI_MULy(_VURegsNum *VUregsn) { _vuRegsMULy(&VU0, VUregsn); } 
void VU0regsMI_MULz(_VURegsNum *VUregsn) { _vuRegsMULz(&VU0, VUregsn); } 
void VU0regsMI_MULw(_VURegsNum *VUregsn) { _vuRegsMULw(&VU0, VUregsn); } 
void VU0regsMI_MULA(_VURegsNum *VUregsn)  { _vuRegsMULA(&VU0, VUregsn); } 
void VU0regsMI_MULAi(_VURegsNum *VUregsn) { _vuRegsMULAi(&VU0, VUregsn); } 
void VU0regsMI_MULAq(_VURegsNum *VUregsn) { _vuRegsMULAq(&VU0, VUregsn); } 
void VU0regsMI_MULAx(_VURegsNum *VUregsn) { _vuRegsMULAx(&VU0, VUregsn); } 
void VU0regsMI_MULAy(_VURegsNum *VUregsn) { _vuRegsMULAy(&VU0, VUregsn); } 
void VU0regsMI_MULAz(_VURegsNum *VUregsn) { _vuRegsMULAz(&VU0, VUregsn); } 
void VU0regsMI_MULAw(_VURegsNum *VUregsn) { _vuRegsMULAw(&VU0, VUregsn); } 
void VU0regsMI_MADD(_VURegsNum *VUregsn)  { _vuRegsMADD(&VU0, VUregsn); } 
void VU0regsMI_MADDi(_VURegsNum *VUregsn) { _vuRegsMADDi(&VU0, VUregsn); } 
void VU0regsMI_MADDq(_VURegsNum *VUregsn) { _vuRegsMADDq(&VU0, VUregsn); } 
void VU0regsMI_MADDx(_VURegsNum *VUregsn) { _vuRegsMADDx(&VU0, VUregsn); } 
void VU0regsMI_MADDy(_VURegsNum *VUregsn) { _vuRegsMADDy(&VU0, VUregsn); } 
void VU0regsMI_MADDz(_VURegsNum *VUregsn) { _vuRegsMADDz(&VU0, VUregsn); } 
void VU0regsMI_MADDw(_VURegsNum *VUregsn) { _vuRegsMADDw(&VU0, VUregsn); } 
void VU0regsMI_MADDA(_VURegsNum *VUregsn)  { _vuRegsMADDA(&VU0, VUregsn); } 
void VU0regsMI_MADDAi(_VURegsNum *VUregsn) { _vuRegsMADDAi(&VU0, VUregsn); } 
void VU0regsMI_MADDAq(_VURegsNum *VUregsn) { _vuRegsMADDAq(&VU0, VUregsn); } 
void VU0regsMI_MADDAx(_VURegsNum *VUregsn) { _vuRegsMADDAx(&VU0, VUregsn); } 
void VU0regsMI_MADDAy(_VURegsNum *VUregsn) { _vuRegsMADDAy(&VU0, VUregsn); } 
void VU0regsMI_MADDAz(_VURegsNum *VUregsn) { _vuRegsMADDAz(&VU0, VUregsn); } 
void VU0regsMI_MADDAw(_VURegsNum *VUregsn) { _vuRegsMADDAw(&VU0, VUregsn); } 
void VU0regsMI_MSUB(_VURegsNum *VUregsn)  { _vuRegsMSUB(&VU0, VUregsn); } 
void VU0regsMI_MSUBi(_VURegsNum *VUregsn) { _vuRegsMSUBi(&VU0, VUregsn); } 
void VU0regsMI_MSUBq(_VURegsNum *VUregsn) { _vuRegsMSUBq(&VU0, VUregsn); } 
void VU0regsMI_MSUBx(_VURegsNum *VUregsn) { _vuRegsMSUBx(&VU0, VUregsn); } 
void VU0regsMI_MSUBy(_VURegsNum *VUregsn) { _vuRegsMSUBy(&VU0, VUregsn); } 
void VU0regsMI_MSUBz(_VURegsNum *VUregsn) { _vuRegsMSUBz(&VU0, VUregsn); } 
void VU0regsMI_MSUBw(_VURegsNum *VUregsn) { _vuRegsMSUBw(&VU0, VUregsn); } 
void VU0regsMI_MSUBA(_VURegsNum *VUregsn)  { _vuRegsMSUBA(&VU0, VUregsn); } 
void VU0regsMI_MSUBAi(_VURegsNum *VUregsn) { _vuRegsMSUBAi(&VU0, VUregsn); } 
void VU0regsMI_MSUBAq(_VURegsNum *VUregsn) { _vuRegsMSUBAq(&VU0, VUregsn); } 
void VU0regsMI_MSUBAx(_VURegsNum *VUregsn) { _vuRegsMSUBAx(&VU0, VUregsn); } 
void VU0regsMI_MSUBAy(_VURegsNum *VUregsn) { _vuRegsMSUBAy(&VU0, VUregsn); } 
void VU0regsMI_MSUBAz(_VURegsNum *VUregsn) { _vuRegsMSUBAz(&VU0, VUregsn); } 
void VU0regsMI_MSUBAw(_VURegsNum *VUregsn) { _vuRegsMSUBAw(&VU0, VUregsn); } 
void VU0regsMI_MAX(_VURegsNum *VUregsn)  { _vuRegsMAX(&VU0, VUregsn); } 
void VU0regsMI_MAXi(_VURegsNum *VUregsn) { _vuRegsMAXi(&VU0, VUregsn); } 
void VU0regsMI_MAXx(_VURegsNum *VUregsn) { _vuRegsMAXx(&VU0, VUregsn); } 
void VU0regsMI_MAXy(_VURegsNum *VUregsn) { _vuRegsMAXy(&VU0, VUregsn); } 
void VU0regsMI_MAXz(_VURegsNum *VUregsn) { _vuRegsMAXz(&VU0, VUregsn); } 
void VU0regsMI_MAXw(_VURegsNum *VUregsn) { _vuRegsMAXw(&VU0, VUregsn); } 
void VU0regsMI_MINI(_VURegsNum *VUregsn)  { _vuRegsMINI(&VU0, VUregsn); } 
void VU0regsMI_MINIi(_VURegsNum *VUregsn) { _vuRegsMINIi(&VU0, VUregsn); } 
void VU0regsMI_MINIx(_VURegsNum *VUregsn) { _vuRegsMINIx(&VU0, VUregsn); } 
void VU0regsMI_MINIy(_VURegsNum *VUregsn) { _vuRegsMINIy(&VU0, VUregsn); } 
void VU0regsMI_MINIz(_VURegsNum *VUregsn) { _vuRegsMINIz(&VU0, VUregsn); } 
void VU0regsMI_MINIw(_VURegsNum *VUregsn) { _vuRegsMINIw(&VU0, VUregsn); } 
void VU0regsMI_OPMULA(_VURegsNum *VUregsn) { _vuRegsOPMULA(&VU0, VUregsn); } 
void VU0regsMI_OPMSUB(_VURegsNum *VUregsn) { _vuRegsOPMSUB(&VU0, VUregsn); } 
void VU0regsMI_NOP(_VURegsNum *VUregsn) { _vuRegsNOP(&VU0, VUregsn); } 
void VU0regsMI_FTOI0(_VURegsNum *VUregsn)  { _vuRegsFTOI0(&VU0, VUregsn); } 
void VU0regsMI_FTOI4(_VURegsNum *VUregsn)  { _vuRegsFTOI4(&VU0, VUregsn); } 
void VU0regsMI_FTOI12(_VURegsNum *VUregsn) { _vuRegsFTOI12(&VU0, VUregsn); } 
void VU0regsMI_FTOI15(_VURegsNum *VUregsn) { _vuRegsFTOI15(&VU0, VUregsn); } 
void VU0regsMI_ITOF0(_VURegsNum *VUregsn)  { _vuRegsITOF0(&VU0, VUregsn); } 
void VU0regsMI_ITOF4(_VURegsNum *VUregsn)  { _vuRegsITOF4(&VU0, VUregsn); } 
void VU0regsMI_ITOF12(_VURegsNum *VUregsn) { _vuRegsITOF12(&VU0, VUregsn); } 
void VU0regsMI_ITOF15(_VURegsNum *VUregsn) { _vuRegsITOF15(&VU0, VUregsn); } 
void VU0regsMI_CLIP(_VURegsNum *VUregsn) { _vuRegsCLIP(&VU0, VUregsn); } 
 
/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 
 
void VU0regsMI_DIV(_VURegsNum *VUregsn) { _vuRegsDIV(&VU0, VUregsn); } 
void VU0regsMI_SQRT(_VURegsNum *VUregsn) { _vuRegsSQRT(&VU0, VUregsn); } 
void VU0regsMI_RSQRT(_VURegsNum *VUregsn) { _vuRegsRSQRT(&VU0, VUregsn); } 
void VU0regsMI_IADD(_VURegsNum *VUregsn) { _vuRegsIADD(&VU0, VUregsn); } 
void VU0regsMI_IADDI(_VURegsNum *VUregsn) { _vuRegsIADDI(&VU0, VUregsn); } 
void VU0regsMI_IADDIU(_VURegsNum *VUregsn) { _vuRegsIADDIU(&VU0, VUregsn); } 
void VU0regsMI_IAND(_VURegsNum *VUregsn) { _vuRegsIAND(&VU0, VUregsn); } 
void VU0regsMI_IOR(_VURegsNum *VUregsn) { _vuRegsIOR(&VU0, VUregsn); } 
void VU0regsMI_ISUB(_VURegsNum *VUregsn) { _vuRegsISUB(&VU0, VUregsn); } 
void VU0regsMI_ISUBIU(_VURegsNum *VUregsn) { _vuRegsISUBIU(&VU0, VUregsn); } 
void VU0regsMI_MOVE(_VURegsNum *VUregsn) { _vuRegsMOVE(&VU0, VUregsn); } 
void VU0regsMI_MFIR(_VURegsNum *VUregsn) { _vuRegsMFIR(&VU0, VUregsn); } 
void VU0regsMI_MTIR(_VURegsNum *VUregsn) { _vuRegsMTIR(&VU0, VUregsn); } 
void VU0regsMI_MR32(_VURegsNum *VUregsn) { _vuRegsMR32(&VU0, VUregsn); } 
void VU0regsMI_LQ(_VURegsNum *VUregsn) { _vuRegsLQ(&VU0, VUregsn); } 
void VU0regsMI_LQD(_VURegsNum *VUregsn) { _vuRegsLQD(&VU0, VUregsn); } 
void VU0regsMI_LQI(_VURegsNum *VUregsn) { _vuRegsLQI(&VU0, VUregsn); } 
void VU0regsMI_SQ(_VURegsNum *VUregsn) { _vuRegsSQ(&VU0, VUregsn); } 
void VU0regsMI_SQD(_VURegsNum *VUregsn) { _vuRegsSQD(&VU0, VUregsn); } 
void VU0regsMI_SQI(_VURegsNum *VUregsn) { _vuRegsSQI(&VU0, VUregsn); } 
void VU0regsMI_ILW(_VURegsNum *VUregsn) { _vuRegsILW(&VU0, VUregsn); } 
void VU0regsMI_ISW(_VURegsNum *VUregsn) { _vuRegsISW(&VU0, VUregsn); } 
void VU0regsMI_ILWR(_VURegsNum *VUregsn) { _vuRegsILWR(&VU0, VUregsn); } 
void VU0regsMI_ISWR(_VURegsNum *VUregsn) { _vuRegsISWR(&VU0, VUregsn); } 
void VU0regsMI_RINIT(_VURegsNum *VUregsn) { _vuRegsRINIT(&VU0, VUregsn); } 
void VU0regsMI_RGET(_VURegsNum *VUregsn)  { _vuRegsRGET(&VU0, VUregsn); } 
void VU0regsMI_RNEXT(_VURegsNum *VUregsn) { _vuRegsRNEXT(&VU0, VUregsn); } 
void VU0regsMI_RXOR(_VURegsNum *VUregsn)  { _vuRegsRXOR(&VU0, VUregsn); } 
void VU0regsMI_WAITQ(_VURegsNum *VUregsn) { _vuRegsWAITQ(&VU0, VUregsn); } 
void VU0regsMI_FSAND(_VURegsNum *VUregsn) { _vuRegsFSAND(&VU0, VUregsn); } 
void VU0regsMI_FSEQ(_VURegsNum *VUregsn)  { _vuRegsFSEQ(&VU0, VUregsn); } 
void VU0regsMI_FSOR(_VURegsNum *VUregsn)  { _vuRegsFSOR(&VU0, VUregsn); } 
void VU0regsMI_FSSET(_VURegsNum *VUregsn) { _vuRegsFSSET(&VU0, VUregsn); } 
void VU0regsMI_FMAND(_VURegsNum *VUregsn) { _vuRegsFMAND(&VU0, VUregsn); } 
void VU0regsMI_FMEQ(_VURegsNum *VUregsn)  { _vuRegsFMEQ(&VU0, VUregsn); } 
void VU0regsMI_FMOR(_VURegsNum *VUregsn)  { _vuRegsFMOR(&VU0, VUregsn); } 
void VU0regsMI_FCAND(_VURegsNum *VUregsn) { _vuRegsFCAND(&VU0, VUregsn); } 
void VU0regsMI_FCEQ(_VURegsNum *VUregsn)  { _vuRegsFCEQ(&VU0, VUregsn); } 
void VU0regsMI_FCOR(_VURegsNum *VUregsn)  { _vuRegsFCOR(&VU0, VUregsn); } 
void VU0regsMI_FCSET(_VURegsNum *VUregsn) { _vuRegsFCSET(&VU0, VUregsn); } 
void VU0regsMI_FCGET(_VURegsNum *VUregsn) { _vuRegsFCGET(&VU0, VUregsn); } 
void VU0regsMI_IBEQ(_VURegsNum *VUregsn) { _vuRegsIBEQ(&VU0, VUregsn); } 
void VU0regsMI_IBGEZ(_VURegsNum *VUregsn) { _vuRegsIBGEZ(&VU0, VUregsn); } 
void VU0regsMI_IBGTZ(_VURegsNum *VUregsn) { _vuRegsIBGTZ(&VU0, VUregsn); } 
void VU0regsMI_IBLTZ(_VURegsNum *VUregsn) { _vuRegsIBLTZ(&VU0, VUregsn); } 
void VU0regsMI_IBLEZ(_VURegsNum *VUregsn) { _vuRegsIBLEZ(&VU0, VUregsn); } 
void VU0regsMI_IBNE(_VURegsNum *VUregsn) { _vuRegsIBNE(&VU0, VUregsn); } 
void VU0regsMI_B(_VURegsNum *VUregsn)   { _vuRegsB(&VU0, VUregsn); } 
void VU0regsMI_BAL(_VURegsNum *VUregsn) { _vuRegsBAL(&VU0, VUregsn); } 
void VU0regsMI_JR(_VURegsNum *VUregsn)   { _vuRegsJR(&VU0, VUregsn); } 
void VU0regsMI_JALR(_VURegsNum *VUregsn) { _vuRegsJALR(&VU0, VUregsn); } 
void VU0regsMI_MFP(_VURegsNum *VUregsn) { _vuRegsMFP(&VU0, VUregsn); } 
void VU0regsMI_WAITP(_VURegsNum *VUregsn) { _vuRegsWAITP(&VU0, VUregsn); } 
void VU0regsMI_ESADD(_VURegsNum *VUregsn)   { _vuRegsESADD(&VU0, VUregsn); } 
void VU0regsMI_ERSADD(_VURegsNum *VUregsn)  { _vuRegsERSADD(&VU0, VUregsn); } 
void VU0regsMI_ELENG(_VURegsNum *VUregsn)   { _vuRegsELENG(&VU0, VUregsn); } 
void VU0regsMI_ERLENG(_VURegsNum *VUregsn)  { _vuRegsERLENG(&VU0, VUregsn); } 
void VU0regsMI_EATANxy(_VURegsNum *VUregsn) { _vuRegsEATANxy(&VU0, VUregsn); } 
void VU0regsMI_EATANxz(_VURegsNum *VUregsn) { _vuRegsEATANxz(&VU0, VUregsn); } 
void VU0regsMI_ESUM(_VURegsNum *VUregsn)    { _vuRegsESUM(&VU0, VUregsn); } 
void VU0regsMI_ERCPR(_VURegsNum *VUregsn)   { _vuRegsERCPR(&VU0, VUregsn); } 
void VU0regsMI_ESQRT(_VURegsNum *VUregsn)   { _vuRegsESQRT(&VU0, VUregsn); } 
void VU0regsMI_ERSQRT(_VURegsNum *VUregsn)  { _vuRegsERSQRT(&VU0, VUregsn); } 
void VU0regsMI_ESIN(_VURegsNum *VUregsn)    { _vuRegsESIN(&VU0, VUregsn); } 
void VU0regsMI_EATAN(_VURegsNum *VUregsn)   { _vuRegsEATAN(&VU0, VUregsn); } 
void VU0regsMI_EEXP(_VURegsNum *VUregsn)    { _vuRegsEEXP(&VU0, VUregsn); } 
void VU0regsMI_XITOP(_VURegsNum *VUregsn)   { _vuRegsXITOP(&VU0, VUregsn); }
void VU0regsMI_XGKICK(_VURegsNum *VUregsn)  { _vuRegsXGKICK(&VU0, VUregsn); }
void VU0regsMI_XTOP(_VURegsNum *VUregsn)    { _vuRegsXTOP(&VU0, VUregsn); }
