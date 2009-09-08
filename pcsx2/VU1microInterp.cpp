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


#include "PrecompiledHeader.h"

#include "Common.h"
#include "DebugTools/Debug.h"
#include "VUmicro.h"

extern void _vuFlushAll(VURegs* VU);

_vuTables(VU1, VU1);

void _vu1ExecUpper(VURegs* VU, u32 *ptr) {
	VU->code = ptr[1]; 
	IdebugUPPER(VU1);
	VU1_UPPER_OPCODE[VU->code & 0x3f](); 
}

void _vu1ExecLower(VURegs* VU, u32 *ptr) {
	VU->code = ptr[0]; 
	IdebugLOWER(VU1);
	VU1_LOWER_OPCODE[VU->code >> 25](); 
}

int vu1branch = 0;

static void _vu1Exec(VURegs* VU)
{
	_VURegsNum lregs;
	_VURegsNum uregs;
	VECTOR _VF;
	VECTOR _VFc;
	REG_VI _VI;
	REG_VI _VIc;
	u32 *ptr;
	int vfreg;
	int vireg;
	int discard=0;

	if(VU->VI[REG_TPC].UL >= VU->maxmicro){
		CPU_LOG("VU1 memory overflow!!: %x", VU->VI[REG_TPC].UL);
		VU->VI[REG_TPC].UL &= 0x3FFF;
		/*VU0.VI[REG_VPU_STAT].UL&= ~0x100;
		VU->cycle++;
		return;*/
	}
	ptr = (u32*)&VU->Micro[VU->VI[REG_TPC].UL]; 
	VU->VI[REG_TPC].UL+=8; 		

	if (ptr[1] & 0x40000000) { /* E flag */ 
		VU->ebit = 2;
	}
	if (ptr[1] & 0x10000000) { /* D flag */
		if (VU0.VI[REG_FBRST].UL & 0x400) {
			VU0.VI[REG_VPU_STAT].UL|= 0x200;
			hwIntcIrq(INTC_VU1);
		}
	}
	if (ptr[1] & 0x08000000) { /* T flag */
		if (VU0.VI[REG_FBRST].UL & 0x800) {
			VU0.VI[REG_VPU_STAT].UL|= 0x400;
			hwIntcIrq(INTC_VU1);
		}
	}

	VUM_LOG("VU->cycle = %d (flags st=%x;mac=%x;clip=%x,q=%f)", VU->cycle, VU->statusflag, VU->macflag, VU->clipflag, VU->q.F);

	VU->code = ptr[1]; 
	VU1regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
#ifndef INT_VUSTALLHACK
	_vuTestUpperStalls(VU, &uregs);
#endif

	/* check upper flags */ 
	if (ptr[1] & 0x80000000) { /* I flag */ 
		_vu1ExecUpper(VU, ptr);

		VU->VI[REG_I].UL = ptr[0];
	} else {
		VU->code = ptr[0];
		VU1regs_LOWER_OPCODE[VU->code >> 25](&lregs);
#ifndef INT_VUSTALLHACK
		_vuTestLowerStalls(VU, &lregs);
#endif

		vu1branch = lregs.pipe == VUPIPE_BRANCH;

		vfreg = 0; vireg = 0;
		if (uregs.VFwrite) {
			if (lregs.VFwrite == uregs.VFwrite) {
//				Console::Notice("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle");
				discard = 1;
			}
			if (lregs.VFread0 == uregs.VFwrite ||
				lregs.VFread1 == uregs.VFwrite) {
//				Console::WriteLn("saving reg %d at pc=%x", i, VU->VI[REG_TPC].UL);
				_VF = VU->VF[uregs.VFwrite];
				vfreg = uregs.VFwrite;
			}
		}
		if (uregs.VIread & (1 << REG_CLIP_FLAG)) {
			if (lregs.VIwrite & (1 << REG_CLIP_FLAG)) {
				Console::Notice("*PCSX2*: Warning, VI write to the same reg in both lower/upper cycle");
				discard = 1;
			}
			if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
				_VI = VU->VI[REG_CLIP_FLAG];
				vireg = REG_CLIP_FLAG;
			}
		}

		_vu1ExecUpper(VU, ptr);

		if (discard == 0) {
			if (vfreg) {
				_VFc = VU->VF[vfreg];
				VU->VF[vfreg] = _VF;
			}
			if (vireg) {
				_VIc = VU->VI[vireg];
				VU->VI[vireg] = _VI;
			}

			_vu1ExecLower(VU, ptr);

			if (vfreg) {
				VU->VF[vfreg] = _VFc;
			}
			if (vireg) {
				VU->VI[vireg] = _VIc;
			}
		}
	}
	_vuAddUpperStalls(VU, &uregs);
	_vuAddLowerStalls(VU, &lregs);

	_vuTestPipes(VU);

	if (VU->branch > 0) {
		if (VU->branch-- == 1) {
			VU->VI[REG_TPC].UL = VU->branchpc;
		}
	}

	if( VU->ebit > 0 ) {
		if( VU->ebit-- == 1 ) {
			_vuFlushAll(VU);
			VU0.VI[REG_VPU_STAT].UL&= ~0x100;
			vif1Regs->stat&= ~VIF1_STAT_VEW;
		}
	}
}

void vu1Exec(VURegs* VU)
{
	_vu1Exec(VU);
	VU->cycle++;

	if (VU->VI[0].UL != 0) DbgCon::Error("VI[0] != 0!!!!\n");
	if (VU->VF[0].f.x != 0.0f) DbgCon::Error("VF[0].x != 0.0!!!!\n");
	if (VU->VF[0].f.y != 0.0f) DbgCon::Error("VF[0].y != 0.0!!!!\n");
	if (VU->VF[0].f.z != 0.0f) DbgCon::Error("VF[0].z != 0.0!!!!\n");
	if (VU->VF[0].f.w != 1.0f) DbgCon::Error("VF[0].w != 1.0!!!!\n");
}

namespace VU1micro
{
	void intAlloc()
	{
	}

	void __fastcall intClear(u32 Addr, u32 Size)
	{
	}

	void intShutdown()
	{
	}

	static void intReset()
	{
	}

	static void intStep()
	{
		vu1Exec( &VU1 );
	}

	static void intExecuteBlock()
	{
		int i;

		for (i = 128; i--;) {
			if ((VU0.VI[REG_VPU_STAT].UL & 0x100) == 0)
				break;

			vu1Exec(&VU1);
		}

		if( i < 0 && (VU1.branch || VU1.ebit) ) {
			// execute one more
			vu1Exec(&VU1);
		}
	}
}

using namespace VU1micro;

const VUmicroCpu intVU1 = 
{
	intReset
,	intStep
,	intExecuteBlock
,	intClear
};
