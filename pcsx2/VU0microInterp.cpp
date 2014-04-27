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


#include "PrecompiledHeader.h"
#include "Common.h"

#include "VUmicro.h"

extern void _vuFlushAll(VURegs* VU);

static void _vu0ExecUpper(VURegs* VU, u32 *ptr) {
	VU->code = ptr[1];
	IdebugUPPER(VU0);
	VU0_UPPER_OPCODE[VU->code & 0x3f]();
}

static void _vu0ExecLower(VURegs* VU, u32 *ptr) {
	VU->code = ptr[0];
	IdebugLOWER(VU0);
	VU0_LOWER_OPCODE[VU->code >> 25]();
}

int vu0branch = 0;
static void _vu0Exec(VURegs* VU)
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

	ptr = (u32*)&VU->Micro[VU->VI[REG_TPC].UL];
	VU->VI[REG_TPC].UL+=8;

	if (ptr[1] & 0x40000000) {
		VU->ebit = 2;
	}
	if (ptr[1] & 0x20000000) { /* M flag */
		VU->flags|= VUFLAG_MFLAGSET;
//		Console.WriteLn("fixme: M flag set");
	}
	if (ptr[1] & 0x10000000) { /* D flag */
		if (VU0.VI[REG_FBRST].UL & 0x4) {
			VU0.VI[REG_VPU_STAT].UL|= 0x2;
			hwIntcIrq(INTC_VU0);
			VU->ebit = 1;
		}
		
	}
	if (ptr[1] & 0x08000000) { /* T flag */
		if (VU0.VI[REG_FBRST].UL & 0x8) {
			VU0.VI[REG_VPU_STAT].UL|= 0x4;
			hwIntcIrq(INTC_VU0);
			VU->ebit = 1;
		}
		
	}

	VU->code = ptr[1];
	VU0regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
#ifndef INT_VUSTALLHACK
	_vuTestUpperStalls(VU, &uregs);
#endif

	/* check upper flags */
	if (ptr[1] & 0x80000000) { /* I flag */
		_vu0ExecUpper(VU, ptr);

		VU->VI[REG_I].UL = ptr[0];
		memset(&lregs, 0, sizeof(lregs));
	} else {
		VU->code = ptr[0];
		VU0regs_LOWER_OPCODE[VU->code >> 25](&lregs);
#ifndef INT_VUSTALLHACK
		_vuTestLowerStalls(VU, &lregs);
#endif

		vu0branch = lregs.pipe == VUPIPE_BRANCH;

		vfreg = 0; vireg = 0;
		if (uregs.VFwrite) {
			if (lregs.VFwrite == uregs.VFwrite) {
//				Console.Warning("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle");
				discard = 1;
			}
			if (lregs.VFread0 == uregs.VFwrite ||
				lregs.VFread1 == uregs.VFwrite) {
//				Console.WriteLn("saving reg %d at pc=%x", i, VU->VI[REG_TPC].UL);
				_VF = VU->VF[uregs.VFwrite];
				vfreg = uregs.VFwrite;
			}
		}
		if (uregs.VIread & (1 << REG_CLIP_FLAG)) {
			if (lregs.VIwrite & (1 << REG_CLIP_FLAG)) {
				Console.Warning("*PCSX2*: Warning, VI write to the same reg in both lower/upper cycle");
				discard = 1;
			}
			if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
				_VI = VU0.VI[REG_CLIP_FLAG];
				vireg = REG_CLIP_FLAG;
			}
		}

		_vu0ExecUpper(VU, ptr);

		if (discard == 0) {
			if (vfreg) {
				_VFc = VU->VF[vfreg];
				VU->VF[vfreg] = _VF;
			}
			if (vireg) {
				_VIc = VU->VI[vireg];
				VU->VI[vireg] = _VI;
			}

			_vu0ExecLower(VU, ptr);

			if (vfreg) {
				VU->VF[vfreg] = _VFc;
			}
			if (vireg) {
				VU->VI[vireg] = _VIc;
			}
		}
	}
	_vuAddUpperStalls(VU, &uregs);

	if (!(ptr[1] & 0x80000000))
		_vuAddLowerStalls(VU, &lregs);

	_vuTestPipes(VU);

	if(VU->VIBackupCycles > 0) 
		VU->VIBackupCycles--;

	if (VU->branch > 0) {
		if (VU->branch-- == 1) {
			VU->VI[REG_TPC].UL = VU->branchpc;

			if(VU->takedelaybranch == true)
			{				
				VU->branch = 2;
				DevCon.Warning("VU0 - Branch/Jump in Delay Slot");			
				VU->branchpc = VU->delaybranchpc;
				VU->delaybranchpc = 0;
				VU->takedelaybranch = false;
			}			
		}
	}

	if( VU->ebit > 0 ) {
		if( VU->ebit-- == 1 ) {
			VU->VIBackupCycles = 0;
			_vuFlushAll(VU);
			VU0.VI[REG_VPU_STAT].UL&= ~0x1; /* E flag */
			vif0Regs.stat.VEW = false;
		}
	}
}

void vu0Exec(VURegs* VU)
{
	VU0.VI[REG_TPC].UL &= VU0_PROGMASK;
	_vu0Exec(VU);
	VU->cycle++;

	if (VU->VI[0].UL != 0) DbgCon.Error("VI[0] != 0!!!!\n");
	if (VU->VF[0].f.x != 0.0f) DbgCon.Error("VF[0].x != 0.0!!!!\n");
	if (VU->VF[0].f.y != 0.0f) DbgCon.Error("VF[0].y != 0.0!!!!\n");
	if (VU->VF[0].f.z != 0.0f) DbgCon.Error("VF[0].z != 0.0!!!!\n");
	if (VU->VF[0].f.w != 1.0f) DbgCon.Error("VF[0].w != 1.0!!!!\n");
}

// --------------------------------------------------------------------------------------
//  VU0microInterpreter
// --------------------------------------------------------------------------------------
InterpVU0::InterpVU0()
{
	m_Idx = 0;
	IsInterpreter = true;
}

void InterpVU0::Step()
{
	vu0Exec( &VU0 );
}

void InterpVU0::Execute(u32 cycles)
{
	for (int i = (int)cycles; i > 0 ; i--) {
		if (!(VU0.VI[REG_VPU_STAT].UL & 0x1)) {
			if (VU0.branch || VU0.ebit) {
				vu0Exec(&VU0); // run branch delay slot?
			}
			break;
		}
		vu0Exec(&VU0);
	}
}

