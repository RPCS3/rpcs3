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


/* TODO
 -Fix the flags Proper as they aren't handle now..
 -Add BC Table opcodes
 -Add Interlock in QMFC2,QMTC2,CFC2,CTC2
 -Finish instruction set
 -Bug Fixes!!!
*/

#include "PrecompiledHeader.h"
#include "Common.h"

#include <cmath>

#include "R5900OpcodeTables.h"
#include "VUmicro.h"

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define _X (cpuRegs.code>>24) & 0x1
#define _Y (cpuRegs.code>>23) & 0x1
#define _Z (cpuRegs.code>>22) & 0x1
#define _W (cpuRegs.code>>21) & 0x1

#define _Fsf_ ((cpuRegs.code >> 21) & 0x03)
#define _Ftf_ ((cpuRegs.code >> 23) & 0x03)

using namespace R5900;

__aligned16 VURegs VU0;

void COP2_BC2() { Int_COP2BC2PrintTable[_Rt_]();}
void COP2_SPECIAL() { Int_COP2SPECIAL1PrintTable[_Funct_]();}

void COP2_SPECIAL2() {
	Int_COP2SPECIAL2PrintTable[(cpuRegs.code & 0x3) | ((cpuRegs.code >> 4) & 0x7c)]();
}

void COP2_Unknown()
{
	CPU_LOG("Unknown COP2 opcode called");
}

//****************************************************************************

__forceinline void _vu0run(bool breakOnMbit) {
	
	if (!(VU0.VI[REG_VPU_STAT].UL & 1)) return;

	int startcycle = VU0.cycle;
	VU0.flags &= ~VUFLAG_MFLAGSET;

	do {
		// knockout kings 2002 loops here with sVU
		if (breakOnMbit && (VU0.cycle-startcycle > 0x1000)) {
			Console.Warning("VU0 perma-stall, breaking execution...");
			break; // mVU will never get here (it handles mBit internally)
		}
		CpuVU0.ExecuteBlock();
	} while ((VU0.VI[REG_VPU_STAT].UL & 1)						// E-bit Termination
	  &&	(!breakOnMbit || !(VU0.flags & VUFLAG_MFLAGSET)));	// M-bit Break

	//NEW
	cpuRegs.cycle += (VU0.cycle-startcycle)*2;
}

void _vu0WaitMicro()   { _vu0run(1); } // Runs VU0 Micro Until E-bit or M-Bit End
void _vu0FinishMicro() { _vu0run(0); } // Runs VU0 Micro Until E-Bit End

namespace R5900 {
namespace Interpreter{
namespace OpcodeImpl
{
	void LQC2() {
		u32 addr = cpuRegs.GPR.r[_Rs_].UL[0] + (s16)cpuRegs.code;
		if (_Ft_) {
			memRead128(addr,   &VU0.VF[_Ft_].UD[0]);
		} else {
			u64 val[2];
 			memRead128(addr,   val);
		}
	}

	// Asadr.Changed
	//TODO: check this
	// HUH why ? doesn't make any sense ...
	void SQC2() {
		u32 addr = _Imm_ + cpuRegs.GPR.r[_Rs_].UL[0];
		//memWrite64(addr,  VU0.VF[_Ft_].UD[0]); 
		//memWrite64(addr+8,VU0.VF[_Ft_].UD[1]); 
		memWrite128(addr,  &VU0.VF[_Ft_].UD[0]); 
	}
}}}


void QMFC2() {
	if (cpuRegs.code & 1) {
		_vu0WaitMicro();
	}
	if (_Rt_ == 0) return;
	cpuRegs.GPR.r[_Rt_].UD[0] = VU0.VF[_Fs_].UD[0];
	cpuRegs.GPR.r[_Rt_].UD[1] = VU0.VF[_Fs_].UD[1];
} 

void QMTC2() {
	if (cpuRegs.code & 1) {
		_vu0WaitMicro();
	}
	if (_Fs_ == 0) return;
	VU0.VF[_Fs_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0];
	VU0.VF[_Fs_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1];
}

void CFC2() { 
	if (cpuRegs.code & 1) {
		_vu0WaitMicro();
	}
	if (_Rt_ == 0) return;
	cpuRegs.GPR.r[_Rt_].UL[0] = VU0.VI[_Fs_].UL;
	if(VU0.VI[_Fs_].UL & 0x80000000)
		cpuRegs.GPR.r[_Rt_].UL[1] = 0xffffffff;
	else
		cpuRegs.GPR.r[_Rt_].UL[1] = 0;
}

void CTC2() {
	if (cpuRegs.code & 1) {
		_vu0WaitMicro();
	}
	if (_Fs_ == 0) return;

	switch(_Fs_) {
		case REG_MAC_FLAG: // read-only
		case REG_TPC:      // read-only
		case REG_VPU_STAT: // read-only
			break;
		case REG_FBRST:
			VU0.VI[REG_FBRST].UL = cpuRegs.GPR.r[_Rt_].UL[0] & 0x0C0C;
			if (cpuRegs.GPR.r[_Rt_].UL[0] & 0x1) { // VU0 Force Break
				Console.Error("fixme: VU0 Force Break");
			}
			if (cpuRegs.GPR.r[_Rt_].UL[0] & 0x2) { // VU0 Reset
				//Console.WriteLn("fixme: VU0 Reset");
				vu0ResetRegs();
			}
			if (cpuRegs.GPR.r[_Rt_].UL[0] & 0x100) { // VU1 Force Break
				Console.Error("fixme: VU1 Force Break");
			}
			if (cpuRegs.GPR.r[_Rt_].UL[0] & 0x200) { // VU1 Reset
//				Console.WriteLn("fixme: VU1 Reset");
				vu1ResetRegs();
			}
			break;
		case REG_CMSAR1: // REG_CMSAR1
			if (!(VU0.VI[REG_VPU_STAT].UL & 0x100) ) {
				vu1ExecMicro(cpuRegs.GPR.r[_Rt_].US[0]);	// Execute VU1 Micro SubRoutine
			}
			break;
		default:
			VU0.VI[_Fs_].UL = cpuRegs.GPR.r[_Rt_].UL[0];
			break;
	}
}

//---------------------------------------------------------------------------------------


__forceinline void SYNCMSFLAGS() 
{
	VU0.VI[REG_STATUS_FLAG].UL = VU0.statusflag; 
	VU0.VI[REG_MAC_FLAG].UL = VU0.macflag;
}

__forceinline void SYNCFDIV() 
{
	VU0.VI[REG_Q].UL = VU0.q.UL; 
	VU0.VI[REG_STATUS_FLAG].UL = VU0.statusflag;
}

void VABS()  { VU0.code = cpuRegs.code; _vuABS(&VU0); }
void VADD()  { VU0.code = cpuRegs.code; _vuADD(&VU0); SYNCMSFLAGS(); }
void VADDi() { VU0.code = cpuRegs.code; _vuADDi(&VU0); SYNCMSFLAGS(); }
void VADDq() { VU0.code = cpuRegs.code; _vuADDq(&VU0); SYNCMSFLAGS(); }
void VADDx() { VU0.code = cpuRegs.code; _vuADDx(&VU0); SYNCMSFLAGS(); }
void VADDy() { VU0.code = cpuRegs.code; _vuADDy(&VU0); SYNCMSFLAGS(); }
void VADDz() { VU0.code = cpuRegs.code; _vuADDz(&VU0); SYNCMSFLAGS(); }
void VADDw() { VU0.code = cpuRegs.code; _vuADDw(&VU0); SYNCMSFLAGS(); }
void VADDA() { VU0.code = cpuRegs.code; _vuADDA(&VU0); SYNCMSFLAGS(); }
void VADDAi() { VU0.code = cpuRegs.code; _vuADDAi(&VU0); SYNCMSFLAGS(); }
void VADDAq() { VU0.code = cpuRegs.code; _vuADDAq(&VU0); SYNCMSFLAGS(); }
void VADDAx() { VU0.code = cpuRegs.code; _vuADDAx(&VU0); SYNCMSFLAGS(); }
void VADDAy() { VU0.code = cpuRegs.code; _vuADDAy(&VU0); SYNCMSFLAGS(); }
void VADDAz() { VU0.code = cpuRegs.code; _vuADDAz(&VU0); SYNCMSFLAGS(); }
void VADDAw() { VU0.code = cpuRegs.code; _vuADDAw(&VU0); SYNCMSFLAGS(); }
void VSUB()  { VU0.code = cpuRegs.code; _vuSUB(&VU0); SYNCMSFLAGS(); }
void VSUBi() { VU0.code = cpuRegs.code; _vuSUBi(&VU0); SYNCMSFLAGS(); }
void VSUBq() { VU0.code = cpuRegs.code; _vuSUBq(&VU0); SYNCMSFLAGS(); }
void VSUBx() { VU0.code = cpuRegs.code; _vuSUBx(&VU0); SYNCMSFLAGS(); }
void VSUBy() { VU0.code = cpuRegs.code; _vuSUBy(&VU0); SYNCMSFLAGS(); }
void VSUBz() { VU0.code = cpuRegs.code; _vuSUBz(&VU0); SYNCMSFLAGS(); }
void VSUBw() { VU0.code = cpuRegs.code; _vuSUBw(&VU0); SYNCMSFLAGS(); }
void VSUBA()  { VU0.code = cpuRegs.code; _vuSUBA(&VU0); SYNCMSFLAGS(); }
void VSUBAi() { VU0.code = cpuRegs.code; _vuSUBAi(&VU0); SYNCMSFLAGS(); }
void VSUBAq() { VU0.code = cpuRegs.code; _vuSUBAq(&VU0); SYNCMSFLAGS(); }
void VSUBAx() { VU0.code = cpuRegs.code; _vuSUBAx(&VU0); SYNCMSFLAGS(); }
void VSUBAy() { VU0.code = cpuRegs.code; _vuSUBAy(&VU0); SYNCMSFLAGS(); }
void VSUBAz() { VU0.code = cpuRegs.code; _vuSUBAz(&VU0); SYNCMSFLAGS(); }
void VSUBAw() { VU0.code = cpuRegs.code; _vuSUBAw(&VU0); SYNCMSFLAGS(); }
void VMUL()  { VU0.code = cpuRegs.code; _vuMUL(&VU0); SYNCMSFLAGS(); }
void VMULi() { VU0.code = cpuRegs.code; _vuMULi(&VU0); SYNCMSFLAGS(); }
void VMULq() { VU0.code = cpuRegs.code; _vuMULq(&VU0); SYNCMSFLAGS(); }
void VMULx() { VU0.code = cpuRegs.code; _vuMULx(&VU0); SYNCMSFLAGS(); }
void VMULy() { VU0.code = cpuRegs.code; _vuMULy(&VU0); SYNCMSFLAGS(); }
void VMULz() { VU0.code = cpuRegs.code; _vuMULz(&VU0); SYNCMSFLAGS(); }
void VMULw() { VU0.code = cpuRegs.code; _vuMULw(&VU0); SYNCMSFLAGS(); }
void VMULA()  { VU0.code = cpuRegs.code; _vuMULA(&VU0); SYNCMSFLAGS(); }
void VMULAi() { VU0.code = cpuRegs.code; _vuMULAi(&VU0); SYNCMSFLAGS(); }
void VMULAq() { VU0.code = cpuRegs.code; _vuMULAq(&VU0); SYNCMSFLAGS(); }
void VMULAx() { VU0.code = cpuRegs.code; _vuMULAx(&VU0); SYNCMSFLAGS(); }
void VMULAy() { VU0.code = cpuRegs.code; _vuMULAy(&VU0); SYNCMSFLAGS(); }
void VMULAz() { VU0.code = cpuRegs.code; _vuMULAz(&VU0); SYNCMSFLAGS(); }
void VMULAw() { VU0.code = cpuRegs.code; _vuMULAw(&VU0); SYNCMSFLAGS(); }
void VMADD()  { VU0.code = cpuRegs.code; _vuMADD(&VU0); SYNCMSFLAGS(); }
void VMADDi() { VU0.code = cpuRegs.code; _vuMADDi(&VU0); SYNCMSFLAGS(); }
void VMADDq() { VU0.code = cpuRegs.code; _vuMADDq(&VU0); SYNCMSFLAGS(); }
void VMADDx() { VU0.code = cpuRegs.code; _vuMADDx(&VU0); SYNCMSFLAGS(); }
void VMADDy() { VU0.code = cpuRegs.code; _vuMADDy(&VU0); SYNCMSFLAGS(); }
void VMADDz() { VU0.code = cpuRegs.code; _vuMADDz(&VU0); SYNCMSFLAGS(); }
void VMADDw() { VU0.code = cpuRegs.code; _vuMADDw(&VU0); SYNCMSFLAGS(); }
void VMADDA()  { VU0.code = cpuRegs.code; _vuMADDA(&VU0); SYNCMSFLAGS(); }
void VMADDAi() { VU0.code = cpuRegs.code; _vuMADDAi(&VU0); SYNCMSFLAGS(); }
void VMADDAq() { VU0.code = cpuRegs.code; _vuMADDAq(&VU0); SYNCMSFLAGS(); }
void VMADDAx() { VU0.code = cpuRegs.code; _vuMADDAx(&VU0); SYNCMSFLAGS(); }
void VMADDAy() { VU0.code = cpuRegs.code; _vuMADDAy(&VU0); SYNCMSFLAGS(); }
void VMADDAz() { VU0.code = cpuRegs.code; _vuMADDAz(&VU0); SYNCMSFLAGS(); }
void VMADDAw() { VU0.code = cpuRegs.code; _vuMADDAw(&VU0); SYNCMSFLAGS(); }
void VMSUB()  { VU0.code = cpuRegs.code; _vuMSUB(&VU0); SYNCMSFLAGS(); }
void VMSUBi() { VU0.code = cpuRegs.code; _vuMSUBi(&VU0); SYNCMSFLAGS(); }
void VMSUBq() { VU0.code = cpuRegs.code; _vuMSUBq(&VU0); SYNCMSFLAGS(); }
void VMSUBx() { VU0.code = cpuRegs.code; _vuMSUBx(&VU0); SYNCMSFLAGS(); }
void VMSUBy() { VU0.code = cpuRegs.code; _vuMSUBy(&VU0); SYNCMSFLAGS(); }
void VMSUBz() { VU0.code = cpuRegs.code; _vuMSUBz(&VU0); SYNCMSFLAGS(); }
void VMSUBw() { VU0.code = cpuRegs.code; _vuMSUBw(&VU0); SYNCMSFLAGS(); }
void VMSUBA()  { VU0.code = cpuRegs.code; _vuMSUBA(&VU0); SYNCMSFLAGS(); }
void VMSUBAi() { VU0.code = cpuRegs.code; _vuMSUBAi(&VU0); SYNCMSFLAGS(); }
void VMSUBAq() { VU0.code = cpuRegs.code; _vuMSUBAq(&VU0); SYNCMSFLAGS(); }
void VMSUBAx() { VU0.code = cpuRegs.code; _vuMSUBAx(&VU0); SYNCMSFLAGS(); }
void VMSUBAy() { VU0.code = cpuRegs.code; _vuMSUBAy(&VU0); SYNCMSFLAGS(); }
void VMSUBAz() { VU0.code = cpuRegs.code; _vuMSUBAz(&VU0); SYNCMSFLAGS(); }
void VMSUBAw() { VU0.code = cpuRegs.code; _vuMSUBAw(&VU0); SYNCMSFLAGS(); }
void VMAX()  { VU0.code = cpuRegs.code; _vuMAX(&VU0); }
void VMAXi() { VU0.code = cpuRegs.code; _vuMAXi(&VU0); }
void VMAXx() { VU0.code = cpuRegs.code; _vuMAXx(&VU0); }
void VMAXy() { VU0.code = cpuRegs.code; _vuMAXy(&VU0); }
void VMAXz() { VU0.code = cpuRegs.code; _vuMAXz(&VU0); }
void VMAXw() { VU0.code = cpuRegs.code; _vuMAXw(&VU0); }
void VMINI()  { VU0.code = cpuRegs.code; _vuMINI(&VU0); }
void VMINIi() { VU0.code = cpuRegs.code; _vuMINIi(&VU0); }
void VMINIx() { VU0.code = cpuRegs.code; _vuMINIx(&VU0); }
void VMINIy() { VU0.code = cpuRegs.code; _vuMINIy(&VU0); }
void VMINIz() { VU0.code = cpuRegs.code; _vuMINIz(&VU0); }
void VMINIw() { VU0.code = cpuRegs.code; _vuMINIw(&VU0); }
void VOPMULA() { VU0.code = cpuRegs.code; _vuOPMULA(&VU0); SYNCMSFLAGS(); }
void VOPMSUB() { VU0.code = cpuRegs.code; _vuOPMSUB(&VU0); SYNCMSFLAGS(); }
void VNOP()    { VU0.code = cpuRegs.code; _vuNOP(&VU0); }
void VFTOI0()  { VU0.code = cpuRegs.code; _vuFTOI0(&VU0); }
void VFTOI4()  { VU0.code = cpuRegs.code; _vuFTOI4(&VU0); }
void VFTOI12() { VU0.code = cpuRegs.code; _vuFTOI12(&VU0); }
void VFTOI15() { VU0.code = cpuRegs.code; _vuFTOI15(&VU0); }
void VITOF0()  { VU0.code = cpuRegs.code; _vuITOF0(&VU0); }
void VITOF4()  { VU0.code = cpuRegs.code; _vuITOF4(&VU0); }
void VITOF12() { VU0.code = cpuRegs.code; _vuITOF12(&VU0); }
void VITOF15() { VU0.code = cpuRegs.code; _vuITOF15(&VU0); }
void VCLIPw()  { VU0.code = cpuRegs.code; _vuCLIP(&VU0); VU0.VI[REG_CLIP_FLAG].UL = VU0.clipflag; }

void VDIV()    { VU0.code = cpuRegs.code; _vuDIV(&VU0); SYNCFDIV(); }
void VSQRT()   { VU0.code = cpuRegs.code; _vuSQRT(&VU0); SYNCFDIV(); }
void VRSQRT()  { VU0.code = cpuRegs.code; _vuRSQRT(&VU0); SYNCFDIV(); }
void VIADD()   { VU0.code = cpuRegs.code; _vuIADD(&VU0); }
void VIADDI()  { VU0.code = cpuRegs.code; _vuIADDI(&VU0); }
void VIADDIU() { VU0.code = cpuRegs.code; _vuIADDIU(&VU0); }
void VIAND()   { VU0.code = cpuRegs.code; _vuIAND(&VU0); }
void VIOR()    { VU0.code = cpuRegs.code; _vuIOR(&VU0); }
void VISUB()   { VU0.code = cpuRegs.code; _vuISUB(&VU0); }
void VISUBIU() { VU0.code = cpuRegs.code; _vuISUBIU(&VU0); }
void VMOVE()   { VU0.code = cpuRegs.code; _vuMOVE(&VU0); }
void VMFIR()   { VU0.code = cpuRegs.code; _vuMFIR(&VU0); }
void VMTIR()   { VU0.code = cpuRegs.code; _vuMTIR(&VU0); }
void VMR32()   { VU0.code = cpuRegs.code; _vuMR32(&VU0); }
void VLQ()     { VU0.code = cpuRegs.code; _vuLQ(&VU0); }
void VLQD()    { VU0.code = cpuRegs.code; _vuLQD(&VU0); }
void VLQI()    { VU0.code = cpuRegs.code; _vuLQI(&VU0); }
void VSQ()     { VU0.code = cpuRegs.code; _vuSQ(&VU0); }
void VSQD()    { VU0.code = cpuRegs.code; _vuSQD(&VU0); }
void VSQI()    { VU0.code = cpuRegs.code; _vuSQI(&VU0); }
void VILW()    { VU0.code = cpuRegs.code; _vuILW(&VU0); }
void VISW()    { VU0.code = cpuRegs.code; _vuISW(&VU0); }
void VILWR()   { VU0.code = cpuRegs.code; _vuILWR(&VU0); }
void VISWR()   { VU0.code = cpuRegs.code; _vuISWR(&VU0); }
void VRINIT()  { VU0.code = cpuRegs.code; _vuRINIT(&VU0); }
void VRGET()   { VU0.code = cpuRegs.code; _vuRGET(&VU0); }
void VRNEXT()  { VU0.code = cpuRegs.code; _vuRNEXT(&VU0); }
void VRXOR()   { VU0.code = cpuRegs.code; _vuRXOR(&VU0); }
void VWAITQ()  { VU0.code = cpuRegs.code; _vuWAITQ(&VU0); }
void VFSAND()  { VU0.code = cpuRegs.code; _vuFSAND(&VU0); }
void VFSEQ()   { VU0.code = cpuRegs.code; _vuFSEQ(&VU0); }
void VFSOR()   { VU0.code = cpuRegs.code; _vuFSOR(&VU0); }
void VFSSET()  { VU0.code = cpuRegs.code; _vuFSSET(&VU0); }
void VFMAND()  { VU0.code = cpuRegs.code; _vuFMAND(&VU0); }
void VFMEQ()   { VU0.code = cpuRegs.code; _vuFMEQ(&VU0); }
void VFMOR()   { VU0.code = cpuRegs.code; _vuFMOR(&VU0); }
void VFCAND()  { VU0.code = cpuRegs.code; _vuFCAND(&VU0); }
void VFCEQ()   { VU0.code = cpuRegs.code; _vuFCEQ(&VU0); }
void VFCOR()   { VU0.code = cpuRegs.code; _vuFCOR(&VU0); }
void VFCSET()  { VU0.code = cpuRegs.code; _vuFCSET(&VU0); }
void VFCGET()  { VU0.code = cpuRegs.code; _vuFCGET(&VU0); }
void VXITOP()  { VU0.code = cpuRegs.code; _vuXITOP(&VU0); }

// fixme: Shouldn't anything calling this function be calling vu0WaitMicro instead?
// Meaning that this function stalls, but doesn't increment the cpuRegs.cycle like
// you would think it should.
void vu0Finish()
{
	if( (VU0.VI[REG_VPU_STAT].UL & 0x1) ) {
		int i = 0;

		while(i++ < 32) {
			CpuVU0.ExecuteBlock();
			if(!(VU0.VI[REG_VPU_STAT].UL & 0x1))
				break;
		}
		if(VU0.VI[REG_VPU_STAT].UL & 0x1) {
			VU0.VI[REG_VPU_STAT].UL &= ~1;
			// this log tends to spam a lot (MGS3)
			//Console.Warning("vu0Finish > stall aborted by force.");
		}
	}
}
