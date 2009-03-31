/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "GS.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCOP0.h"
#include "VUmicro.h"
#include "VUflags.h"
#include "iVUmicro.h"
#include "iVUops.h"
#include "iVUzerorec.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif
//------------------------------------------------------------------

// fixme - VUmicro should really use its own static vars for pc and branch.
// Sharing with the EE's copies of pc and branch is not cool! (air)

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ (( VU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ (( VU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ (( VU->code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X (( VU->code>>24) & 0x1)
#define _Y (( VU->code>>23) & 0x1)
#define _Z (( VU->code>>22) & 0x1)
#define _W (( VU->code>>21) & 0x1)

#define _XYZW_SS (_X+_Y+_Z+_W==1)

#define _Fsf_ (( VU->code >> 21) & 0x03)
#define _Ftf_ (( VU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff)
#define _UImm11_	(s32)(VU->code & 0x7ff)

#define VU_VFx_ADDR(x)  (uptr)&VU->VF[x].UL[0]
#define VU_VFy_ADDR(x)  (uptr)&VU->VF[x].UL[1]
#define VU_VFz_ADDR(x)  (uptr)&VU->VF[x].UL[2]
#define VU_VFw_ADDR(x)  (uptr)&VU->VF[x].UL[3]

#define VU_REGR_ADDR    (uptr)&VU->VI[REG_R]
#define VU_REGQ_ADDR    (uptr)&VU->VI[REG_Q]
#define VU_REGMAC_ADDR  (uptr)&VU->VI[REG_MAC_FLAG]

#define VU_VI_ADDR(x, read) GetVIAddr(VU, x, read, info)

#define VU_ACCx_ADDR    (uptr)&VU->ACC.UL[0]
#define VU_ACCy_ADDR    (uptr)&VU->ACC.UL[1]
#define VU_ACCz_ADDR    (uptr)&VU->ACC.UL[2]
#define VU_ACCw_ADDR    (uptr)&VU->ACC.UL[3]

#define _X_Y_Z_W  ((( VU->code >> 21 ) & 0xF ) )
//------------------------------------------------------------------


//------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------
int vucycle;

PCSX2_ALIGNED16(const float s_fones[8])		= {1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
PCSX2_ALIGNED16(const u32 s_mask[4])		= {0x007fffff, 0x007fffff, 0x007fffff, 0x007fffff};
PCSX2_ALIGNED16(const u32 s_expmask[4])		= {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};
PCSX2_ALIGNED16(const u32 g_minvals[4])		= {0xff7fffff, 0xff7fffff, 0xff7fffff, 0xff7fffff};
PCSX2_ALIGNED16(const u32 g_maxvals[4])		= {0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff};
PCSX2_ALIGNED16(const u32 const_clip[8])	= {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff,
											   0x80000000, 0x80000000, 0x80000000, 0x80000000};
PCSX2_ALIGNED(64, const u32 g_ones[4])		= {0x00000001, 0x00000001, 0x00000001, 0x00000001};
PCSX2_ALIGNED16(const u32 g_minvals_XYZW[16][4]) =
{
   { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }, //0000
   { 0xffffffff, 0xffffffff, 0xffffffff, 0xff7fffff }, //0001
   { 0xffffffff, 0xffffffff, 0xff7fffff, 0xffffffff }, //0010
   { 0xffffffff, 0xffffffff, 0xff7fffff, 0xff7fffff }, //0011
   { 0xffffffff, 0xff7fffff, 0xffffffff, 0xffffffff }, //0100
   { 0xffffffff, 0xff7fffff, 0xffffffff, 0xff7fffff }, //0101
   { 0xffffffff, 0xff7fffff, 0xff7fffff, 0xffffffff }, //0110
   { 0xffffffff, 0xff7fffff, 0xff7fffff, 0xff7fffff }, //0111
   { 0xff7fffff, 0xffffffff, 0xffffffff, 0xffffffff }, //1000
   { 0xff7fffff, 0xffffffff, 0xffffffff, 0xff7fffff }, //1001
   { 0xff7fffff, 0xffffffff, 0xff7fffff, 0xffffffff }, //1010
   { 0xff7fffff, 0xffffffff, 0xff7fffff, 0xff7fffff }, //1011
   { 0xff7fffff, 0xff7fffff, 0xffffffff, 0xffffffff }, //1100
   { 0xff7fffff, 0xff7fffff, 0xffffffff, 0xff7fffff }, //1101
   { 0xff7fffff, 0xff7fffff, 0xff7fffff, 0xffffffff }, //1110
   { 0xff7fffff, 0xff7fffff, 0xff7fffff, 0xff7fffff }, //1111
};
PCSX2_ALIGNED16(const u32 g_maxvals_XYZW[16][4])=
{
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }, //0000
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7f7fffff }, //0001
   { 0x7fffffff, 0x7fffffff, 0x7f7fffff, 0x7fffffff }, //0010
   { 0x7fffffff, 0x7fffffff, 0x7f7fffff, 0x7f7fffff }, //0011
   { 0x7fffffff, 0x7f7fffff, 0x7fffffff, 0x7fffffff }, //0100
   { 0x7fffffff, 0x7f7fffff, 0x7fffffff, 0x7f7fffff }, //0101
   { 0x7fffffff, 0x7f7fffff, 0x7f7fffff, 0x7fffffff }, //0110
   { 0x7fffffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff }, //0111
   { 0x7f7fffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }, //1000
   { 0x7f7fffff, 0x7fffffff, 0x7fffffff, 0x7f7fffff }, //1001
   { 0x7f7fffff, 0x7fffffff, 0x7f7fffff, 0x7fffffff }, //1010
   { 0x7f7fffff, 0x7fffffff, 0x7f7fffff, 0x7f7fffff }, //1011
   { 0x7f7fffff, 0x7f7fffff, 0x7fffffff, 0x7fffffff }, //1100
   { 0x7f7fffff, 0x7f7fffff, 0x7fffffff, 0x7f7fffff }, //1101
   { 0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7fffffff }, //1110
   { 0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff }, //1111
};
//------------------------------------------------------------------

//------------------------------------------------------------------
// VU Pipeline/Test Stalls/Analyzing Functions
//------------------------------------------------------------------
void _recvuFMACflush(VURegs * VU, bool intermediate) {
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;

		if( intermediate ) {
			if ((vucycle - VU->fmac[i].sCycle) > VU->fmac[i].Cycle) {
//				VUM_LOG("flushing FMAC pipe[%d]", i);
				VU->fmac[i].enable = 0;
			}
		}
		else {
			if ((vucycle - VU->fmac[i].sCycle) >= VU->fmac[i].Cycle) {
//				VUM_LOG("flushing FMAC pipe[%d]", i);
				VU->fmac[i].enable = 0;
			}
		}
	}
}

void _recvuFDIVflush(VURegs * VU, bool intermediate) {
	if (VU->fdiv.enable == 0) return;

	if( intermediate ) {
		if ((vucycle - VU->fdiv.sCycle) > VU->fdiv.Cycle) {
//			Console::WriteLn("flushing FDIV pipe");
			VU->fdiv.enable = 0;
		}
	}
	else {
		if ((vucycle - VU->fdiv.sCycle) >= VU->fdiv.Cycle) {
//			Console::WriteLn("flushing FDIV pipe");
			VU->fdiv.enable = 0;
		}
	}
}

void _recvuEFUflush(VURegs * VU, bool intermediate) {
	if (VU->efu.enable == 0) return;

	if( intermediate ) {
		if ((vucycle - VU->efu.sCycle) > VU->efu.Cycle) {
//			Console::WriteLn("flushing FDIV pipe");
			VU->efu.enable = 0;
		}
	}
	else {
		if ((vucycle - VU->efu.sCycle) >= VU->efu.Cycle) {
//			Console::WriteLn("flushing FDIV pipe");
			VU->efu.enable = 0;
		}
	}
}

void _recvuIALUflush(VURegs * VU, bool intermediate) {
	int i;

	for (i=0; i<8; i++) {
		if (VU->ialu[i].enable == 0) continue;

		if( intermediate ) {
			if ((vucycle - VU->ialu[i].sCycle) > VU->ialu[i].Cycle) {
//				VUM_LOG("flushing IALU pipe[%d]", i);
				VU->ialu[i].enable = 0;
			}
		}
		else {
			if ((vucycle - VU->ialu[i].sCycle) >= VU->ialu[i].Cycle) {
//				VUM_LOG("flushing IALU pipe[%d]", i);
				VU->ialu[i].enable = 0;
			}
		}
	}
}

void _recvuTestPipes(VURegs * VU, bool intermediate) { // intermediate = true if called by upper FMAC stall detection
	_recvuFMACflush(VU, intermediate);
	_recvuFDIVflush(VU, intermediate);
	_recvuEFUflush(VU, intermediate);
	_recvuIALUflush(VU, intermediate);
}

void _recvuFMACTestStall(VURegs * VU, int reg, int xyzw) {
	int cycle;
	int i;
	u32 mask = 0;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;
		if (VU->fmac[i].reg == reg && (VU->fmac[i].xyzw & xyzw)) break;
	}

	if (i == 8) return;

	// do a perchannel delay
	// old code
//	cycle = VU->fmac[i].Cycle - (vucycle - VU->fmac[i].sCycle);

	// new code
	mask = 4; // w
//	if( VU->fmac[i].xyzw & 1 ) mask = 4; // w
//	else if( VU->fmac[i].xyzw & 2 ) mask = 3; // z
//	else if( VU->fmac[i].xyzw & 4 ) mask = 2; // y
//	else {
//		assert(VU->fmac[i].xyzw & 8 );
//		mask = 1; // x
//	}

//	mask = 0;
//	if( VU->fmac[i].xyzw & 1 ) mask++; // w
//	else if( VU->fmac[i].xyzw & 2 ) mask++; // z
//	else if( VU->fmac[i].xyzw & 4 ) mask++; // y
//	else if( VU->fmac[i].xyzw & 8 ) mask++; // x

	assert( (int)VU->fmac[i].sCycle < (int)vucycle );
	cycle = 0;
	if( vucycle - VU->fmac[i].sCycle < mask )
		cycle = mask - (vucycle - VU->fmac[i].sCycle);

	VU->fmac[i].enable = 0;
	vucycle+= cycle;
	_recvuTestPipes(VU, true); // for lower instructions
}

void _recvuIALUTestStall(VURegs * VU, int reg) {
	int cycle;
	int i;
	u32 latency;

	for (i=0; i<8; i++) {
		if (VU->ialu[i].enable == 0) continue;
		if (VU->ialu[i].reg == reg) break;
	}

	if (i == 8) return;

	latency = VU->ialu[i].Cycle + 1;
	cycle = 0;
	if( vucycle - VU->ialu[i].sCycle < latency )
		cycle = latency - (vucycle - VU->ialu[i].sCycle);

	VU->ialu[i].enable = 0;
	vucycle+= cycle;
}

void _recvuFMACAdd(VURegs * VU, int reg, int xyzw) {
	int i;

	/* find a free fmac pipe */
	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 1) continue;
		break;
	}

	if (i==8) Console::Error("*PCSX2*: error , out of fmacs");
//	VUM_LOG("adding FMAC pipe[%d]; reg %d", i, reg);
	
	VU->fmac[i].enable = 1;
	VU->fmac[i].sCycle = vucycle;
	VU->fmac[i].Cycle = 3;
	VU->fmac[i].xyzw = xyzw; 
	VU->fmac[i].reg = reg; 
}

void _recvuFDIVAdd(VURegs * VU, int cycles) {
//	Console::WriteLn("adding FDIV pipe");
	VU->fdiv.enable = 1;
	VU->fdiv.sCycle = vucycle;
	VU->fdiv.Cycle  = cycles;
}

void _recvuEFUAdd(VURegs * VU, int cycles) {
//	Console::WriteLn("adding EFU pipe");
	VU->efu.enable = 1;
	VU->efu.sCycle = vucycle;
	VU->efu.Cycle  = cycles;
}

void _recvuIALUAdd(VURegs * VU, int reg, int cycles) {
	int i;

	/* find a free ialu pipe */
	for (i=0; i<8; i++) {
		if (VU->ialu[i].enable == 1) continue;
		break;
	}

	if (i==8) Console::Error("*PCSX2*: error , out of ialus");
	
	VU->ialu[i].enable = 1;
	VU->ialu[i].sCycle = vucycle;
	VU->ialu[i].Cycle = cycles;
	VU->ialu[i].reg = reg;
}

void _recvuTestIALUStalls(VURegs * VU, _VURegsNum *VUregsn) {

	int VIread0 = 0, VIread1 = 0; // max 2 integer registers are read simulataneously
	int i;

	for(i=0;i<16;i++) { // find used integer(vi00-vi15) registers
		if( (VUregsn->VIread >> i) & 1 ) {
			if( VIread0 ) VIread1 = i;
			else VIread0 = i;
		}
	}

	if( VIread0 ) _recvuIALUTestStall(VU, VIread0);
	if( VIread1 ) _recvuIALUTestStall(VU, VIread1);
}

void _recvuAddIALUStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VIwrite && VUregsn->cycles) {
		int VIWrite0 = 0;
		int i;

		for(i=0;i<16;i++) { // find used(vi00-vi15) registers
			if( (VUregsn->VIwrite >> i) & 1 ) {
				VIWrite0 = i;
			}
		}
		if( VIWrite0 ) _recvuIALUAdd(VU, VIWrite0, VUregsn->cycles);
	}
}

void _recvuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn, bool upper) {

	if( VUregsn->VFread0 && (VUregsn->VFread0 == VUregsn->VFread1) ) {
		_recvuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw|VUregsn->VFr1xyzw);
	}
	else {
		if (VUregsn->VFread0) _recvuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw);
		if (VUregsn->VFread1) _recvuFMACTestStall(VU, VUregsn->VFread1, VUregsn->VFr1xyzw);
	}

	if( !upper && VUregsn->VIread ) _recvuTestIALUStalls(VU, VUregsn); // for lower instructions which read integer reg
}

void _recvuAddFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {

	if (VUregsn->VFwrite) _recvuFMACAdd(VU, VUregsn->VFwrite, VUregsn->VFwxyzw);
	else if (VUregsn->VIwrite & (1 << REG_CLIP_FLAG)) _recvuFMACAdd(VU, -REG_CLIP_FLAG, 0); // REG_CLIP_FLAG pipe
	else _recvuFMACAdd(VU, 0, 0); // cause no data dependency with fp registers
}

void _recvuFlushFDIV(VURegs * VU) {
	int cycle;

	if (VU->fdiv.enable == 0) return;

	cycle = VU->fdiv.Cycle - (vucycle - VU->fdiv.sCycle);
//	Console::WriteLn("waiting FDIV pipe %d", params cycle);
	VU->fdiv.enable = 0;
	vucycle+= cycle;
}

void _recvuFlushEFU(VURegs * VU) {
	int cycle;

	if (VU->efu.enable == 0) return;

	cycle = VU->efu.Cycle - (vucycle - VU->efu.sCycle);
//	Console::WriteLn("waiting FDIV pipe %d", params cycle);
	VU->efu.enable = 0;
	vucycle+= cycle;
}

void _recvuTestFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
	_recvuTestFMACStalls(VU,VUregsn, false);
	_recvuFlushFDIV(VU);
}

void _recvuTestEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
	_recvuTestFMACStalls(VU,VUregsn, false);
	_recvuFlushEFU(VU);
}

void _recvuAddFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	if (VUregsn->VIwrite & (1 << REG_Q)) {
		_recvuFDIVAdd(VU, VUregsn->cycles);
	}
}

void _recvuAddEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	if (VUregsn->VIwrite & (1 << REG_P)) {
		_recvuEFUAdd(VU, VUregsn->cycles);
	}
}

void _recvuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn, true); break;
	}
}

void _recvuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn, false); break;
		case VUPIPE_FDIV: _recvuTestFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuTestEFUStalls(VU, VUregsn);	 break;
		case VUPIPE_IALU: _recvuTestIALUStalls(VU, VUregsn); break;
		case VUPIPE_BRANCH: _recvuTestIALUStalls(VU, VUregsn); break;
	}
}

void _recvuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn);	break;
	}
}

void _recvuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _recvuAddFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuAddEFUStalls(VU, VUregsn);	break;
		case VUPIPE_IALU: _recvuAddIALUStalls(VU, VUregsn);	break; // note: only ILW and ILWR cause stall in IALU pipe
	}
}

void SuperVUAnalyzeOp(VURegs *VU, _vuopinfo *info, _VURegsNum* pCodeRegs)
{
	_VURegsNum* lregs;
	_VURegsNum* uregs;
	int *ptr; 

	lregs = pCodeRegs;
	uregs = pCodeRegs+1;

	ptr = (int*)&VU->Micro[pc]; 
	pc += 8; 

	if (ptr[1] & 0x40000000) { // EOP
		branch |= 8; 
	} 
 
	VU->code = ptr[1];
	if (VU == &VU1) VU1regs_UPPER_OPCODE[VU->code & 0x3f](uregs);
	else VU0regs_UPPER_OPCODE[VU->code & 0x3f](uregs);

	_recvuTestUpperStalls(VU, uregs);
	switch(VU->code & 0x3f) {
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x1d: case 0x1f:
		case 0x2b: case 0x2f:
			break;

		case 0x3c:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3d:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3e:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3f:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7: case 0xb:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;

		default:
			info->statusflag = 4;
			info->macflag = 4;
			break;
	}

	if (uregs->VIread & (1 << REG_Q)) { info->q |= 2; }
	if (uregs->VIread & (1 << REG_P)) { info->p |= 2; assert( VU == &VU1 ); }

	// check upper flags
	if (ptr[1] & 0x80000000) { // I flag
		info->cycle = vucycle;
		memzero_obj(*lregs);
	} 
	else {

		VU->code = ptr[0]; 
		if (VU == &VU1) VU1regs_LOWER_OPCODE[VU->code >> 25](lregs);
		else VU0regs_LOWER_OPCODE[VU->code >> 25](lregs);

		_recvuTestLowerStalls(VU, lregs);
		info->cycle = vucycle;

		if (lregs->pipe == VUPIPE_BRANCH) {
			branch |= 1;
		}

		if (lregs->VIwrite & (1 << REG_Q)) {
			info->q |= 4;
			info->cycles = lregs->cycles;
			info->pqinst = (VU->code&2)>>1; // rsqrt is 2
		}
		else if (lregs->pipe == VUPIPE_FDIV) {
			info->q |= 8|1;
			info->pqinst = 0;
		}

		if (lregs->VIwrite & (1 << REG_P)) {
			assert( VU == &VU1 );
			info->p |= 4;
			info->cycles = lregs->cycles;

			switch( VU->code & 0xff ) {
				case 0xfd: info->pqinst = 0; break; //eatan
				case 0x7c: info->pqinst = 0; break; //eatanxy
				case 0x7d: info->pqinst = 0; break; //eatanzy
				case 0xfe: info->pqinst = 1; break; //eexp
				case 0xfc: info->pqinst = 2; break; //esin
				case 0x3f: info->pqinst = 3; break; //erleng
				case 0x3e: info->pqinst = 4; break; //eleng
				case 0x3d: info->pqinst = 4; break; //ersadd
				case 0xbd: info->pqinst = 4; break; //ersqrt
				case 0xbe: info->pqinst = 5; break; //ercpr
				case 0xbc: info->pqinst = 5; break; //esqrt
				case 0x7e: info->pqinst = 5; break; //esum
				case 0x3c: info->pqinst = 6; break; //esadd
				default: assert(0);
			}
		}
		else if (lregs->pipe == VUPIPE_EFU) {
			info->p |= 8|1;
		}

		if (lregs->VIread & (1 << REG_STATUS_FLAG)) info->statusflag|= VUOP_READ;
		if (lregs->VIread & (1 << REG_MAC_FLAG))	info->macflag|= VUOP_READ;

		if (lregs->VIwrite & (1 << REG_STATUS_FLAG)) info->statusflag|= VUOP_WRITE;
		if (lregs->VIwrite & (1 << REG_MAC_FLAG))	 info->macflag|= VUOP_WRITE;

		if (lregs->VIread & (1 << REG_Q)) { info->q |= 2; }
		if (lregs->VIread & (1 << REG_P)) { info->p |= 2; assert( VU == &VU1 ); }

		_recvuAddLowerStalls(VU, lregs);
	}

	_recvuAddUpperStalls(VU, uregs);
	_recvuTestPipes(VU, false);

	vucycle++;
}

int eeVURecompileCode(VURegs *VU, _VURegsNum* regs)
{
	int info = 0;
	int vfread0=-1, vfread1 = -1, vfwrite = -1, vfacc = -1, vftemp=-1;

	assert( regs != NULL );

	if( regs->VFread0 ) _addNeededVFtoXMMreg(regs->VFread0);
	if( regs->VFread1 ) _addNeededVFtoXMMreg(regs->VFread1);
	if( regs->VFwrite ) _addNeededVFtoXMMreg(regs->VFwrite);
	if( regs->VIread & (1<<REG_ACC_FLAG) ) _addNeededACCtoXMMreg();
	if( regs->VIread & (1<<REG_VF0_FLAG) ) _addNeededVFtoXMMreg(0);

	// alloc
	if( regs->VFread0 ) vfread0 = _allocVFtoXMMreg(VU, -1, regs->VFread0, MODE_READ);
	else if( regs->VIread & (1<<REG_VF0_FLAG) ) vfread0 = _allocVFtoXMMreg(VU, -1, 0, MODE_READ);
	if( regs->VFread1 ) vfread1 = _allocVFtoXMMreg(VU, -1, regs->VFread1, MODE_READ);
	else if( (regs->VIread & (1<<REG_VF0_FLAG)) && regs->VFr1xyzw != 0xff) vfread1 = _allocVFtoXMMreg(VU, -1, 0, MODE_READ);

	if( regs->VIread & (1<<REG_ACC_FLAG )) {
		vfacc = _allocACCtoXMMreg(VU, -1, ((regs->VIwrite&(1<<REG_ACC_FLAG))?MODE_WRITE:0)|MODE_READ);
	}
	else if( regs->VIwrite & (1<<REG_ACC_FLAG) ) {
		vfacc = _allocACCtoXMMreg(VU, -1, MODE_WRITE|(regs->VFwxyzw != 0xf?MODE_READ:0));
	}

	if( regs->VFwrite ) {
		assert( !(regs->VIwrite&(1<<REG_ACC_FLAG)) );
		vfwrite = _allocVFtoXMMreg(VU, -1, regs->VFwrite, MODE_WRITE|(regs->VFwxyzw != 0xf?MODE_READ:0));
	}

	if( vfacc>= 0 ) info |= PROCESS_EE_SET_ACC(vfacc);
	if( vfwrite >= 0 ) {
		if( regs->VFwrite == _Ft_ && vfread1 < 0 ) {
			info |= PROCESS_EE_SET_T(vfwrite);
		}
		else {
			assert( regs->VFwrite == _Fd_ );
			info |= PROCESS_EE_SET_D(vfwrite);
		}
	}

	if( vfread0 >= 0 ) info |= PROCESS_EE_SET_S(vfread0);
	if( vfread1 >= 0 ) info |= PROCESS_EE_SET_T(vfread1);

	vftemp = _allocTempXMMreg(XMMT_FPS, -1);
	info |= PROCESS_VU_SET_TEMP(vftemp);

	if( regs->VIwrite & (1 << REG_CLIP_FLAG) ) {
		// CLIP inst, need two extra temp registers, put it EEREC_D and EEREC_ACC
		int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
		int t2reg = _allocTempXMMreg(XMMT_FPS, -1);

		info |= PROCESS_EE_SET_D(t1reg);
		info |= PROCESS_EE_SET_ACC(t2reg);

		_freeXMMreg(t1reg); // don't need
		_freeXMMreg(t2reg); // don't need
	}
	else if( regs->VIwrite & (1<<REG_P) ) {
		int t1reg = _allocTempXMMreg(XMMT_FPS, -1);
		info |= PROCESS_EE_SET_D(t1reg);
		_freeXMMreg(t1reg); // don't need
	}

	_freeXMMreg(vftemp); // don't need it

	return info;
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// Misc VU/VI Allocation Functions
//------------------------------------------------------------------
// returns the correct VI addr
u32 GetVIAddr(VURegs * VU, int reg, int read, int info) 
{
	if( info & PROCESS_VU_SUPER )	 return SuperVUGetVIAddr(reg, read);
	if( info & PROCESS_VU_COP2 )	 return (uptr)&VU->VI[reg].UL;

	if( read != 1 ) {
		if( reg == REG_MAC_FLAG )	 return (uptr)&VU->macflag;
		if( reg == REG_CLIP_FLAG )	 return (uptr)&VU->clipflag;
		if( reg == REG_STATUS_FLAG ) return (uptr)&VU->statusflag;
		if( reg == REG_Q )			 return (uptr)&VU->q;
		if( reg == REG_P )			 return (uptr)&VU->p;
	}

	return (uptr)&VU->VI[reg].UL;
}

// gets a temp reg that is not EEREC_TEMP
int _vuGetTempXMMreg(int info) 
{
	int t1reg = -1;

	if( _hasFreeXMMreg() ) {
		t1reg = _allocTempXMMreg(XMMT_FPS, -1);

		if( t1reg == EEREC_TEMP ) {
			if( _hasFreeXMMreg() ) {
				int t = _allocTempXMMreg(XMMT_FPS, -1);
				_freeXMMreg(t1reg);
				t1reg = t;
			}
			else {
				_freeXMMreg(t1reg);
				t1reg = -1;
			}
		}
	}
	
	return t1reg;
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// Misc VU Reg Flipping/Merging Functions
//------------------------------------------------------------------
void _unpackVF_xyzw(int dstreg, int srcreg, int xyzw)
{
	switch (xyzw) {
		case 0: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x00); break;
		case 1: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x55); break;
		case 2: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xaa); break;
		case 3: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xff); break;
	}
}

void _unpackVFSS_xyzw(int dstreg, int srcreg, int xyzw)
{
	switch (xyzw) {
		case 0: SSE_MOVSS_XMM_to_XMM(dstreg, srcreg); break;
		case 1: if ( cpucaps.hasStreamingSIMD4Extensions ) SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(1, 0, 0));	
				else SSE2_PSHUFLW_XMM_to_XMM(dstreg, srcreg, 0xee);
				break;
		case 2: SSE_MOVHLPS_XMM_to_XMM(dstreg, srcreg); break;
		case 3: if ( cpucaps.hasStreamingSIMD4Extensions ) SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(3, 0, 0));
				else { SSE_MOVHLPS_XMM_to_XMM(dstreg, srcreg); SSE2_PSHUFLW_XMM_to_XMM(dstreg, dstreg, 0xee); }
				break;
	}
}

void _vuFlipRegSS(VURegs * VU, int reg)
{
	assert( _XYZW_SS );
	if( _Y ) SSE2_PSHUFLW_XMM_to_XMM(reg, reg, 0x4e);
	else if( _Z ) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xc6);
	else if( _W ) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x27);
}

void _vuFlipRegSS_xyzw(int reg, int xyzw)
{
	switch ( xyzw ) {
		case 1: SSE2_PSHUFLW_XMM_to_XMM(reg, reg, 0x4e); break;
		case 2: SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xc6); break;
		case 3: SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x27); break;
	}
}

void _vuMoveSS(VURegs * VU, int dstreg, int srcreg)
{
	assert( _XYZW_SS );
	if( _Y ) _unpackVFSS_xyzw(dstreg, srcreg, 1);
	else if( _Z ) _unpackVFSS_xyzw(dstreg, srcreg, 2);
	else if( _W ) _unpackVFSS_xyzw(dstreg, srcreg, 3);
	else _unpackVFSS_xyzw(dstreg, srcreg, 0);
}

// 1 - src, 0 - dest				   wzyx
void VU_MERGE0(int dest, int src) { // 0000s
}
void VU_MERGE1(int dest, int src) { // 1000
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc4);
}
void VU_MERGE1b(int dest, int src) { // 1000s
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
}
void VU_MERGE2(int dest, int src) { // 0100
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x64);
}
void VU_MERGE2b(int dest, int src) { // 0100s
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
}
void VU_MERGE3(int dest, int src) { // 1100s
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE4(int dest, int src) { // 0010
	SSE_MOVSS_XMM_to_XMM(src, dest);
	SSE2_MOVSD_XMM_to_XMM(dest, src);
}
void VU_MERGE4b(int dest, int src) { // 0010s
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
}
void VU_MERGE5(int dest, int src) { // 1010
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xd8);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd8);
}
void VU_MERGE5b(int dest, int src) { // 1010s
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
}
void VU_MERGE6(int dest, int src) { // 0110
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x9c);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x78);
}
void VU_MERGE6b(int dest, int src) { // 0110s
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
}
void VU_MERGE7(int dest, int src) { // 1110
	SSE_MOVSS_XMM_to_XMM(src, dest);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE7b(int dest, int src) { // 1110s
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
}
void VU_MERGE8(int dest, int src) { // 0001s
	SSE_MOVSS_XMM_to_XMM(dest, src);
}
void VU_MERGE9(int dest, int src) { // 1001
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc9);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd2);
}
void VU_MERGE9b(int dest, int src) { // 1001s
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
}
void VU_MERGE10(int dest, int src) { // 0101
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x8d);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x72);
}
void VU_MERGE10b(int dest, int src) { // 0101s
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
}
void VU_MERGE11(int dest, int src) { // 1101s
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE12(int dest, int src) { // 0011
	SSE2_MOVSD_XMM_to_XMM(dest, src);
}
void VU_MERGE13(int dest, int src) { // 1011
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0x64);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE13b(int dest, int src) { // 1011s
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0x27);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x27);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
}
void VU_MERGE14(int dest, int src) { // 0111
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xc4);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE14b(int dest, int src) { // 0111s
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xE1);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, src, 0xC6);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xC6);
}
void VU_MERGE15(int dest, int src) { // 1111s
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}

typedef void (*VUMERGEFN)(int dest, int src);

static VUMERGEFN s_VuMerge[16] = {
	VU_MERGE0, VU_MERGE1, VU_MERGE2, VU_MERGE3,
	VU_MERGE4, VU_MERGE5, VU_MERGE6, VU_MERGE7,
	VU_MERGE8, VU_MERGE9, VU_MERGE10, VU_MERGE11,
	VU_MERGE12, VU_MERGE13, VU_MERGE14, VU_MERGE15 };

static VUMERGEFN s_VuMerge2[16] = {
	VU_MERGE0, VU_MERGE1b, VU_MERGE2b, VU_MERGE3,
	VU_MERGE4b, VU_MERGE5b, VU_MERGE6b, VU_MERGE7b,
	VU_MERGE8, VU_MERGE9b, VU_MERGE10b, VU_MERGE11,
	VU_MERGE12, VU_MERGE13b, VU_MERGE14b, VU_MERGE15 };

// Modifies the Source Reg!
void VU_MERGE_REGS_CUSTOM(int dest, int src, int xyzw) {
	xyzw &= 0xf;
	if ( (dest != src) && (xyzw != 0) ) {
		if ( cpucaps.hasStreamingSIMD4Extensions && (xyzw != 0x8) && (xyzw != 0xf) ) {
			xyzw = ((xyzw & 1) << 3) | ((xyzw & 2) << 1) | ((xyzw & 4) >> 1) | ((xyzw & 8) >> 3); 
			SSE4_BLENDPS_XMM_to_XMM(dest, src, xyzw);
		}
		else s_VuMerge[xyzw](dest, src); 
	}
}
// Doesn't Modify the Source Reg! (ToDo: s_VuMerge2() has room for optimization)
void VU_MERGE_REGS_SAFE(int dest, int src, int xyzw) {
	xyzw &= 0xf;
	if ( (dest != src) && (xyzw != 0) ) {
		if ( cpucaps.hasStreamingSIMD4Extensions && (xyzw != 0x8) && (xyzw != 0xf) ) {
			xyzw = ((xyzw & 1) << 3) | ((xyzw & 2) << 1) | ((xyzw & 4) >> 1) | ((xyzw & 8) >> 3); 
			SSE4_BLENDPS_XMM_to_XMM(dest, src, xyzw);
		}
		else s_VuMerge2[xyzw](dest, src); 
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// Misc VU Reg Clamping/Overflow Functions
//------------------------------------------------------------------
#define CLAMP_NORMAL_SSE4(n) \
	SSE_MOVAPS_XMM_to_XMM(regTemp, regd);\
	SSE4_PMINUD_M128_to_XMM(regd, (uptr)&g_minvals_XYZW[n][0]);\
	SSE2_PSUBD_XMM_to_XMM(regTemp, regd);\
	SSE2_PCMPGTD_M128_to_XMM(regTemp, (uptr)&g_ones[0]);\
	SSE4_PMINSD_M128_to_XMM(regd, (uptr)&g_maxvals_XYZW[n][0]);\
	SSE2_PSLLD_I8_to_XMM(regTemp, 31);\
	SSE_XORPS_XMM_to_XMM(regd, regTemp);

#define CLAMP_SIGN_SSE4(n) \
	SSE4_PMINSD_M128_to_XMM(regd, (uptr)&g_maxvals_XYZW[n][0]);\
	SSE4_PMINUD_M128_to_XMM(regd, (uptr)&g_minvals_XYZW[n][0]);

void vFloat0(int regd, int regTemp) { } //0000
void vFloat1(int regd, int regTemp) { //1000
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
}
void vFloat1c(int regd, int regTemp) { //1000
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(1);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat2(int regd, int regTemp) { //0100
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
}
void vFloat2c(int regd, int regTemp) { //0100
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(2);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat3(int regd, int regTemp) { //1100
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x36);
}
void vFloat3b(int regd, int regTemp) { //1100 //regTemp is Modified
	SSE2_MOVSD_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE2_MOVSD_XMM_to_XMM(regd, regTemp);
}
void vFloat3c(int regd, int regTemp) { //1100
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(3);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x36);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat4(int regd, int regTemp) { //0010
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
}
void vFloat4c(int regd, int regTemp) { //0010
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(4);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat5(int regd, int regTemp) { //1010
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x2d);
}
void vFloat5b(int regd, int regTemp) { //1010 //regTemp is Modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_NORMAL_SSE4(5);
	}
	else {
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x2d);
	}
}
void vFloat5c(int regd, int regTemp) { //1010
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(5);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x2d);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat6(int regd, int regTemp) { //0110
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc9);
}
void vFloat6b(int regd, int regTemp) { //0110 //regTemp is Modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_NORMAL_SSE4(6);
	}
	else {
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc9);
	}
}
void vFloat6c(int regd, int regTemp) { //0110
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(6);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc9);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat7(int regd, int regTemp) { //1110
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x39);
}
void vFloat7_useEAX(int regd, int regTemp) { //1110 //EAX is Modified
	SSE2_MOVD_XMM_to_R(EAX, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	if ( cpucaps.hasStreamingSIMD4Extensions )
		SSE4_PINSRD_R32_to_XMM(regd, EAX, 0x00);
	else {
		SSE_PINSRW_R32_to_XMM(regd, EAX, 0);
		SHR32ItoR(EAX, 16);
		SSE_PINSRW_R32_to_XMM(regd, EAX, 1);
	}
}
void vFloat7b(int regd, int regTemp) { //1110 //regTemp is Modified
	SSE_MOVSS_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE_MOVSS_XMM_to_XMM(regd, regTemp);
}
void vFloat7c(int regd, int regTemp) { //1110
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(7);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x39);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat7c_useEAX(int regd, int regTemp) { //1110 //EAX is Modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(7);
	}
	else {
		SSE2_MOVD_XMM_to_R(EAX, regd);
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
		SSE2_MOVD_R_to_XMM(regTemp, EAX);
		SSE_MOVSS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat8(int regd, int regTemp) { //0001
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
}
void vFloat8c(int regd, int regTemp) { //0001
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(8);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat9(int regd, int regTemp) { //1001
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
}
void vFloat9b(int regd, int regTemp) { //1001 //regTemp is Modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_NORMAL_SSE4(9);
	}
	else {
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	}
}
void vFloat9c(int regd, int regTemp) { //1001
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(9);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat10(int regd, int regTemp) { //0101
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
}
void vFloat10b(int regd, int regTemp) { //0101 //regTemp is Modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_NORMAL_SSE4(10);
	}
	else {
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	}
}
void vFloat10c(int regd, int regTemp) { //0101
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(10);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat11(int regd, int regTemp) { //1101
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x36);
}
void vFloat11_useEAX(int regd, int regTemp) { //1101 //EAX is Modified
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE2_MOVD_XMM_to_R(EAX, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	if ( cpucaps.hasStreamingSIMD4Extensions )
		SSE4_PINSRD_R32_to_XMM(regd, EAX, 0x00);
	else {
		SSE_PINSRW_R32_to_XMM(regd, EAX, 0);
		SHR32ItoR(EAX, 16);
		SSE_PINSRW_R32_to_XMM(regd, EAX, 1);
	}
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
}
void vFloat11b(int regd, int regTemp) { //1101 //regTemp is Modified
	SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE_MOVSS_XMM_to_XMM(regTemp, regd);
	SSE2_MOVSD_XMM_to_XMM(regd, regTemp);
}
void vFloat11c(int regd, int regTemp) { //1101
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(11);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x36);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat11c_useEAX(int regd, int regTemp) { //1101 // EAX is modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(11);
	}
	else {
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE2_MOVD_XMM_to_R(EAX, regd);
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
		SSE2_MOVD_R_to_XMM(regTemp, EAX);
		SSE_MOVSS_XMM_to_XMM(regd, regTemp);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	}
}
void vFloat12(int regd, int regTemp) { //0011
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
}
void vFloat12b(int regd, int regTemp) { //0011 //regTemp is Modified
	SSE_MOVHLPS_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE2_PUNPCKLQDQ_XMM_to_XMM(regd, regTemp);
}
void vFloat12c(int regd, int regTemp) { //0011
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(12);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat13(int regd, int regTemp) { //1011
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x2d);
}
void vFloat13_useEAX(int regd, int regTemp) { //1011 // EAX is modified
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE2_MOVD_XMM_to_R(EAX, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	if ( cpucaps.hasStreamingSIMD4Extensions )
		SSE4_PINSRD_R32_to_XMM(regd, EAX, 0x00);
	else {
		SSE_PINSRW_R32_to_XMM(regd, EAX, 0);
		SHR32ItoR(EAX, 16);
		SSE_PINSRW_R32_to_XMM(regd, EAX, 1);
	}
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
}
void vFloat13b(int regd, int regTemp) { //1011 //regTemp is Modified
	SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE_MOVHLPS_XMM_to_XMM(regTemp, regd);
	SSE_SHUFPS_XMM_to_XMM(regd, regTemp, 0x64);
}
void vFloat13c(int regd, int regTemp) { //1011
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(13);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x2d);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat13c_useEAX(int regd, int regTemp) { //1011 // EAX is modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(13);
	}
	else {
		SSE2_PSHUFD_XMM_to_XMM(regd, regd, 0xc6);
		SSE2_MOVD_XMM_to_R(EAX, regd);
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
		SSE2_MOVD_R_to_XMM(regTemp, EAX);
		SSE_MOVSS_XMM_to_XMM(regd, regTemp);
		SSE2_PSHUFD_XMM_to_XMM(regd, regd, 0xc6);
	}
}
void vFloat14(int regd, int regTemp) { //0111
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
	SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc9);
}
void vFloat14_useEAX(int regd, int regTemp) { //0111 // EAX is modified
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
	SSE2_MOVD_XMM_to_R(EAX, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	if ( cpucaps.hasStreamingSIMD4Extensions )
		SSE4_PINSRD_R32_to_XMM(regd, EAX, 0x00);
	else {
		SSE_PINSRW_R32_to_XMM(regd, EAX, 0);
		SHR32ItoR(EAX, 16);
		SSE_PINSRW_R32_to_XMM(regd, EAX, 1);
	}
	SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
}
void vFloat14b(int regd, int regTemp) { //0111 //regTemp is Modified
	SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
	SSE_MOVHLPS_XMM_to_XMM(regTemp, regd);
	SSE_SHUFPS_XMM_to_XMM(regd, regTemp, 0xc4);
}
void vFloat14c(int regd, int regTemp) { //0111
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(14);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE2_PSHUFLW_XMM_to_XMM(regd, regd, 0x4e);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc6);
		SSE_MINSS_M32_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXSS_M32_to_XMM(regd, (uptr)g_minvals);
		SSE_SHUFPS_XMM_to_XMM(regd, regd, 0xc9);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}
void vFloat14c_useEAX(int regd, int regTemp) { //0111 // EAX is modified
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(14);
	}
	else {
		SSE2_PSHUFD_XMM_to_XMM(regd, regd, 0x27);
		SSE2_MOVD_XMM_to_R(EAX, regd);
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
		SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
		SSE2_MOVD_R_to_XMM(regTemp, EAX);
		SSE_MOVSS_XMM_to_XMM(regd, regTemp);
		SSE2_PSHUFD_XMM_to_XMM(regd, regd, 0x27);
	}
}
void vFloat15(int regd, int regTemp) { //1111
	SSE_MINPS_M128_to_XMM(regd, (uptr)g_maxvals);
	SSE_MAXPS_M128_to_XMM(regd, (uptr)g_minvals);
}
void vFloat15c(int regd, int regTemp) { //1111
	if ( cpucaps.hasStreamingSIMD4Extensions ) {
		CLAMP_SIGN_SSE4(15);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(regTemp, regd);
		SSE_ANDPS_M128_to_XMM(regTemp, (uptr)&const_clip[4]);
		SSE_MINPS_M128_to_XMM(regd, (uptr)&g_maxvals[0]);
		SSE_MAXPS_M128_to_XMM(regd, (uptr)&g_minvals[0]);
		SSE_ORPS_XMM_to_XMM(regd, regTemp);
	}
}

vFloat vFloats1[16] = { //regTemp is not modified
	vFloat0, vFloat1, vFloat2, vFloat3,
	vFloat4, vFloat5, vFloat6, vFloat7,
	vFloat8, vFloat9, vFloat10, vFloat11,
	vFloat12, vFloat13, vFloat14, vFloat15 };

vFloat vFloats1_useEAX[16] = { //regTemp is not modified but EAX is used
	vFloat0, vFloat1, vFloat2, vFloat3,
	vFloat4, vFloat5, vFloat6, vFloat7_useEAX,
	vFloat8, vFloat9, vFloat10, vFloat11_useEAX,
	vFloat12, vFloat13_useEAX, vFloat14_useEAX, vFloat15 };

vFloat vFloats2[16] = { //regTemp is modified
	vFloat0, vFloat1, vFloat2, vFloat3b,
	vFloat4, vFloat5b, vFloat6b, vFloat7b,
	vFloat8, vFloat9b, vFloat10b, vFloat11b,
	vFloat12b, vFloat13b, vFloat14b, vFloat15 };

vFloat vFloats4[16] = { //regTemp is modified
	vFloat0, vFloat1c, vFloat2c, vFloat3c,
	vFloat4c, vFloat5c, vFloat6c, vFloat7c,
	vFloat8c, vFloat9c, vFloat10c, vFloat11c,
	vFloat12c, vFloat13c, vFloat14c, vFloat15c };

vFloat vFloats4_useEAX[16] = { //regTemp is modified and EAX is used
	vFloat0, vFloat1c, vFloat2c, vFloat3c,
	vFloat4c, vFloat5c, vFloat6c, vFloat7c_useEAX,
	vFloat8c, vFloat9c, vFloat10c, vFloat11c_useEAX,
	vFloat12c, vFloat13c_useEAX, vFloat14c_useEAX, vFloat15c };

//------------------------------------------------------------------
//	Clamping Functions (wrapper for vFloat* functions)
//		vuFloat			: "normal" clamping
//		vuFloat_useEAX	: "normal" clamping (faster but EAX is modified)
//		vuFloat2		: "normal" clamping (fastest but regTemp is modified)
//		vuFloat3		: "preserve sign" clamping for pointer
//		vuFloat4		: "preserve sign" clamping (regTemp is modified; *FASTEST* on SSE4 CPUs)
//		vuFloat4_useEAX	: "preserve sign" clamping (faster but regTemp and EAX are modified)
//		vuFloat5		: wrapper function for vuFloat2 and vuFloat4
//		vuFloat5_useEAX	: wrapper function for vuFloat2 and vuFloat4_useEAX
//		vuFloatExtra	: for debugging
//
//		Notice 1: vuFloat*_useEAX may be slower on AMD CPUs, which have independent execution pipeline for
//		          vector and scalar instructions (need checks)
//		Notice 2: recVUMI_MUL_xyzw_toD and recVUMI_MADD_xyzw_toD use vFloats directly!
//------------------------------------------------------------------

// Clamps +/-NaN to +fMax and +/-Inf to +/-fMax (doesn't use any temp regs)
void vuFloat( int info, int regd, int XYZW) {
	if( CHECK_VU_OVERFLOW ) {
		/*if ( (XYZW != 0) && (XYZW != 8) && (XYZW != 0xF) ) {
			int t1reg = _vuGetTempXMMreg(info);
			if (t1reg >= 0) {
				vuFloat2( regd, t1reg, XYZW );
				_freeXMMreg( t1reg );
				return;
			}
		}*/
		//vuFloatExtra(regd, XYZW);
		vFloats1[XYZW](regd, regd);
	}
}

// Clamps +/-NaN to +fMax and +/-Inf to +/-fMax (uses EAX as a temp register; faster but **destroys EAX**)
void vuFloat_useEAX( int info, int regd, int XYZW) {
	if( CHECK_VU_OVERFLOW ) {
		vFloats1_useEAX[XYZW](regd, regd);
	}
}

// Clamps +/-NaN to +fMax and +/-Inf to +/-fMax (uses a temp reg)
void vuFloat2(int regd, int regTemp, int XYZW) {
	if( CHECK_VU_OVERFLOW ) {
		//vuFloatExtra(regd, XYZW);
		vFloats2[XYZW](regd, regTemp);
	}
}

// Clamps +/-NaN and +/-Inf to +/-fMax (uses a temp reg)
void vuFloat4(int regd, int regTemp, int XYZW) {
	if( CHECK_VU_OVERFLOW ) {
		vFloats4[XYZW](regd, regTemp);
	}
}

// Clamps +/-NaN and +/-Inf to +/-fMax (uses a temp reg, and uses EAX as a temp register; faster but **destroys EAX**)
void vuFloat4_useEAX(int regd, int regTemp, int XYZW) {
	if( CHECK_VU_OVERFLOW ) {
		vFloats4_useEAX[XYZW](regd, regTemp);
	}
}

// Uses vuFloat4 or vuFloat2 depending on the CHECK_VU_SIGN_OVERFLOW setting
void vuFloat5(int regd, int regTemp, int XYZW) {
	if (CHECK_VU_SIGN_OVERFLOW) {
		vuFloat4(regd, regTemp, XYZW);
	}
	else vuFloat2(regd, regTemp, XYZW);
}

// Uses vuFloat4_useEAX or vuFloat2 depending on the CHECK_VU_SIGN_OVERFLOW setting (uses EAX as a temp register; faster but **destoroyes EAX**)
void vuFloat5_useEAX(int regd, int regTemp, int XYZW) {
	if (CHECK_VU_SIGN_OVERFLOW) {
		vuFloat4_useEAX(regd, regTemp, XYZW);
	}
	else vuFloat2(regd, regTemp, XYZW);
}

// Clamps +/-infs to +/-fMax, and +/-NaNs to +/-fMax
void vuFloat3(uptr x86ptr) {
	u8* pjmp;

	if( CHECK_VU_OVERFLOW ) {
		CMP32ItoM(x86ptr, 0x7f800000 );
		pjmp = JL8(0); // Signed Comparison
			MOV32ItoM(x86ptr, 0x7f7fffff );
		x86SetJ8(pjmp);

		CMP32ItoM(x86ptr, 0xff800000 );
		pjmp = JB8(0); // Unsigned Comparison
			MOV32ItoM(x86ptr, 0xff7fffff );
		x86SetJ8(pjmp);
	}
}

PCSX2_ALIGNED16(u64 vuFloatData[2]);
PCSX2_ALIGNED16(u64 vuFloatData2[2]);
// Makes NaN == 0, Infinities stay the same; Very Slow - Use only for debugging
void vuFloatExtra( int regd, int XYZW) {
	int t1reg = (regd == 0) ? (regd + 1) : (regd - 1);
	int t2reg = (regd <= 1) ? (regd + 2) : (regd - 2);
	SSE_MOVAPS_XMM_to_M128( (uptr)vuFloatData, t1reg );
	SSE_MOVAPS_XMM_to_M128( (uptr)vuFloatData2, t2reg );

	SSE_XORPS_XMM_to_XMM(t1reg, t1reg); 
	SSE_CMPORDPS_XMM_to_XMM(t1reg, regd); 
	SSE_MOVAPS_XMM_to_XMM(t2reg, regd);
	SSE_ANDPS_XMM_to_XMM(t2reg, t1reg);
	VU_MERGE_REGS_CUSTOM(regd, t2reg, XYZW);
	
	SSE_MOVAPS_M128_to_XMM( t1reg, (uptr)vuFloatData );
	SSE_MOVAPS_M128_to_XMM( t2reg, (uptr)vuFloatData2 );
}

static PCSX2_ALIGNED16(u32 tempRegX[]) = {0x00000000, 0x00000000, 0x00000000, 0x00000000};

// Called by testWhenOverflow() function
void testPrintOverflow() {
	tempRegX[0] &= 0xff800000;
	tempRegX[1] &= 0xff800000;
	tempRegX[2] &= 0xff800000;
	tempRegX[3] &= 0xff800000;
	if ( (tempRegX[0] == 0x7f800000) || (tempRegX[1] == 0x7f800000) || (tempRegX[2] == 0x7f800000) || (tempRegX[3] == 0x7f800000) )
		Console::Notice( "VU OVERFLOW!: Changing to +Fmax!!!!!!!!!!!!" );
	if ( (tempRegX[0] == 0xff800000) || (tempRegX[1] == 0xff800000) || (tempRegX[2] == 0xff800000) || (tempRegX[3] == 0xff800000) )
		Console::Notice( "VU OVERFLOW!: Changing to -Fmax!!!!!!!!!!!!" );
}

// Outputs to the console when overflow has occured.
void testWhenOverflow(int info, int regd, int t0reg) {
	SSE_MOVAPS_XMM_to_M128((uptr)tempRegX, regd);
	CALLFunc((uptr)testPrintOverflow);
}
