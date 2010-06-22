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

#include <cmath>

#include "VUops.h"
#include "GS.h"

//Lower/Upper instructions can use that..
#define _Ft_ ((VU->code >> 16) & 0x1F)  // The rt part of the instruction register
#define _Fs_ ((VU->code >> 11) & 0x1F)  // The rd part of the instruction register
#define _Fd_ ((VU->code >>  6) & 0x1F)  // The sa part of the instruction register
#define _It_ (_Ft_ & 0xF)
#define _Is_ (_Fs_ & 0xF)
#define _Id_ (_Fd_ & 0xF)

#define _X ((VU->code>>24) & 0x1)
#define _Y ((VU->code>>23) & 0x1)
#define _Z ((VU->code>>22) & 0x1)
#define _W ((VU->code>>21) & 0x1)

#define _XYZW ((VU->code>>21) & 0xF)

#define _Fsf_ ((VU->code >> 21) & 0x03)
#define _Ftf_ ((VU->code >> 23) & 0x03)

#define _Imm11_		(s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff)
#define _UImm11_	(s32)(VU->code & 0x7ff)


static __aligned16 VECTOR RDzero;

static __releaseinline void __fastcall _vuFMACflush(VURegs * VU) {
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;

		if ((VU->cycle - VU->fmac[i].sCycle) >= VU->fmac[i].Cycle) {
			VUM_LOG("flushing FMAC pipe[%d] (macflag=%x)", i, VU->fmac[i].macflag);

			VU->fmac[i].enable = 0;
			VU->VI[REG_MAC_FLAG].UL = VU->fmac[i].macflag;
			VU->VI[REG_STATUS_FLAG].UL = VU->fmac[i].statusflag;
			VU->VI[REG_CLIP_FLAG].UL = VU->fmac[i].clipflag;
		}
	}
}

static __releaseinline void __fastcall _vuFDIVflush(VURegs * VU) {
	if (VU->fdiv.enable == 0) return;

	if ((VU->cycle - VU->fdiv.sCycle) >= VU->fdiv.Cycle) {
		VUM_LOG("flushing FDIV pipe");

		VU->fdiv.enable = 0;
		VU->VI[REG_Q].UL = VU->fdiv.reg.UL;
		VU->VI[REG_STATUS_FLAG].UL = VU->fdiv.statusflag;
	}
}

static __releaseinline void __fastcall _vuEFUflush(VURegs * VU) {
	if (VU->efu.enable == 0) return;

	if ((VU->cycle - VU->efu.sCycle) >= VU->efu.Cycle) {
//		VUM_LOG("flushing EFU pipe");

		VU->efu.enable = 0;
		VU->VI[REG_P].UL = VU->efu.reg.UL;
	}
}

// called at end of program
void _vuFlushAll(VURegs* VU)
{
	int nRepeat = 1, i;

	do {
		nRepeat = 0;

		for (i=0; i<8; i++) {
			if (VU->fmac[i].enable == 0) continue;

			nRepeat = 1;

			if ((VU->cycle - VU->fmac[i].sCycle) >= VU->fmac[i].Cycle) {
				VUM_LOG("flushing FMAC pipe[%d] (macflag=%x)", i, VU->fmac[i].macflag);

				VU->fmac[i].enable = 0;
				VU->VI[REG_MAC_FLAG].UL = VU->fmac[i].macflag;
				VU->VI[REG_STATUS_FLAG].UL = VU->fmac[i].statusflag;
				VU->VI[REG_CLIP_FLAG].UL = VU->fmac[i].clipflag;
			}
		}

		if (VU->fdiv.enable ) {

			nRepeat = 1;

			if ((VU->cycle - VU->fdiv.sCycle) >= VU->fdiv.Cycle) {
				VUM_LOG("flushing FDIV pipe");

				nRepeat = 1;
				VU->fdiv.enable = 0;
				VU->VI[REG_Q].UL = VU->fdiv.reg.UL;
				VU->VI[REG_STATUS_FLAG].UL = VU->fdiv.statusflag;
			}
		}

		if (VU->efu.enable) {

			nRepeat = 1;

			if ((VU->cycle - VU->efu.sCycle) >= VU->efu.Cycle) {
	//			VUM_LOG("flushing EFU pipe");

				nRepeat = 1;
				VU->efu.enable = 0;
				VU->VI[REG_P].UL = VU->efu.reg.UL;
			}
		}

		VU->cycle++;

	} while(nRepeat);
}

__forceinline void _vuTestPipes(VURegs * VU) {
	_vuFMACflush(VU);
	_vuFDIVflush(VU);
	_vuEFUflush(VU);
}

static void __fastcall _vuFMACTestStall(VURegs * VU, int reg, int xyzw) {
	int cycle;
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;
		if (VU->fmac[i].reg == reg &&
			VU->fmac[i].xyzw & xyzw) break;
	}

	if (i == 8) return;

	cycle = VU->fmac[i].Cycle - (VU->cycle - VU->fmac[i].sCycle) + 1; // add 1 delay! (fixes segaclassics bad geom)
	VU->fmac[i].enable = 0;
	VU->VI[REG_MAC_FLAG].UL = VU->fmac[i].macflag;
	VU->VI[REG_STATUS_FLAG].UL = VU->fmac[i].statusflag;
	VU->VI[REG_CLIP_FLAG].UL = VU->fmac[i].clipflag;
	VUM_LOG("FMAC[%d] stall %d", i, cycle);

	VU->cycle+= cycle;
	_vuTestPipes(VU);
}

static __releaseinline void __fastcall _vuFMACAdd(VURegs * VU, int reg, int xyzw) {
	int i;

	/* find a free fmac pipe */
	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 1) continue;
		break;
	}
	//if (i==8) Console.Error("*PCSX2*: error , out of fmacs %d", VU->cycle);


	VUM_LOG("adding FMAC pipe[%d]; xyzw=%x", i, xyzw);

	VU->fmac[i].enable = 1;
	VU->fmac[i].sCycle = VU->cycle;
	VU->fmac[i].Cycle = 3;
	VU->fmac[i].reg = reg;
	VU->fmac[i].xyzw = xyzw;
	VU->fmac[i].macflag = VU->macflag;
	VU->fmac[i].statusflag = VU->statusflag;
	VU->fmac[i].clipflag = VU->clipflag;
}

static __releaseinline void __fastcall _vuFDIVAdd(VURegs * VU, int cycles) {
	VUM_LOG("adding FDIV pipe");

	VU->fdiv.enable = 1;
	VU->fdiv.sCycle = VU->cycle;
	VU->fdiv.Cycle  = cycles;
	VU->fdiv.reg.F  = VU->q.F;
	VU->fdiv.statusflag = VU->statusflag;
}

static __releaseinline void __fastcall _vuEFUAdd(VURegs * VU, int cycles) {
//	VUM_LOG("adding EFU pipe\n");

	VU->efu.enable = 1;
	VU->efu.sCycle = VU->cycle;
	VU->efu.Cycle  = cycles;
	VU->efu.reg.F  = VU->p.F;
}

static __releaseinline void __fastcall _vuFlushFDIV(VURegs * VU) {
	int cycle;

	if (VU->fdiv.enable == 0) return;

	cycle = VU->fdiv.Cycle - (VU->cycle - VU->fdiv.sCycle);
	VUM_LOG("waiting FDIV pipe %d", cycle);

	VU->fdiv.enable = 0;
	VU->cycle+= cycle;
	VU->VI[REG_Q].UL = VU->fdiv.reg.UL;
	VU->VI[REG_STATUS_FLAG].UL = VU->fdiv.statusflag;
}

static __releaseinline void __fastcall _vuFlushEFU(VURegs * VU) {
	int cycle;

	if (VU->efu.enable == 0) return;

	cycle = VU->efu.Cycle - (VU->cycle - VU->efu.sCycle);
//	VUM_LOG("waiting EFU pipe %d", cycle);

	VU->efu.enable = 0;
	VU->cycle+= cycle;
	VU->VI[REG_P].UL = VU->efu.reg.UL;
}

__forceinline void _vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VFread0) {
		_vuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw);
	}
	if (VUregsn->VFread1) {
		_vuFMACTestStall(VU, VUregsn->VFread1, VUregsn->VFr1xyzw);
	}
}

__forceinline void _vuAddFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VFwrite) {
		_vuFMACAdd(VU, VUregsn->VFwrite, VUregsn->VFwxyzw);
	} else
	if (VUregsn->VIwrite & (1 << REG_CLIP_FLAG)) {
		_vuFMACAdd(VU, -REG_CLIP_FLAG, 0);
	} else {
		_vuFMACAdd(VU, 0, 0);
	}
}

__forceinline void _vuTestFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	_vuFlushFDIV(VU);
}

__forceinline void _vuAddFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VIwrite & (1 << REG_Q)) {
		_vuFDIVAdd(VU, VUregsn->cycles);
	}
}


__forceinline void _vuTestEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	_vuFlushEFU(VU);
}

__forceinline void _vuAddEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VIwrite & (1 << REG_P)) {
		_vuEFUAdd(VU, VUregsn->cycles);
	}
}

__forceinline void _vuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _vuTestFMACStalls(VU, VUregsn); break;
	}
}

__forceinline void _vuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _vuTestFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _vuTestFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _vuTestEFUStalls(VU, VUregsn); break;
	}
}

__forceinline void _vuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _vuAddFMACStalls(VU, VUregsn); break;
	}
}

__forceinline void _vuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _vuAddFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _vuAddFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _vuAddEFUStalls(VU, VUregsn); break;
	}
}


/******************************/
/*   VU Upper instructions    */
/******************************/
#ifndef INT_VUDOUBLEHACK
static float __fastcall vuDouble(u32 f)
{
	switch(f & 0x7f800000)
	{
		case 0x0:
			f &= 0x80000000;
			return *(float*)&f;
			break;
		case 0x7f800000:
		{
			u32 d = (f & 0x80000000)|0x7f7fffff;
			return *(float*)&d;
			break;
		}
	}
	return *(float*)&f;
}
#else
static __forceinline float vuDouble(u32 f)
{
	return *(float*)&f;
}
#endif

void _vuABS(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X){ VU->VF[_Ft_].f.x = fabs(vuDouble(VU->VF[_Fs_].i.x)); }
	if (_Y){ VU->VF[_Ft_].f.y = fabs(vuDouble(VU->VF[_Fs_].i.y)); }
	if (_Z){ VU->VF[_Ft_].f.z = fabs(vuDouble(VU->VF[_Fs_].i.z)); }
	if (_W){ VU->VF[_Ft_].f.w = fabs(vuDouble(VU->VF[_Fs_].i.w)); }
}/*Reworked from define to function. asadr*/


void _vuADD(VURegs * VU) {
	VECTOR * dst;
	if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/


void _vuADDi(VURegs * VU) {
	VECTOR * dst;
	if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VI[REG_I].UL));} else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VI[REG_I].UL));} else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + vuDouble(VU->VI[REG_I].UL));} else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + vuDouble(VU->VI[REG_I].UL));} else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDq(VURegs * VU) {
	VECTOR * dst;
	if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VI[REG_Q].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VI[REG_Q].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + vuDouble(VU->VI[REG_Q].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + vuDouble(VU->VI[REG_Q].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/


void _vuADDx(VURegs * VU) {
	float ftx;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftx=vuDouble(VU->VF[_Ft_].i.x);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + ftx); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + ftx); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + ftx); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + ftx); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDy(VURegs * VU) {
	float fty;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	fty=vuDouble(VU->VF[_Ft_].i.y);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + fty);} else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + fty);} else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + fty);} else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + fty);} else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDz(VURegs * VU) {
	float ftz;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftz=vuDouble(VU->VF[_Ft_].i.z);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + ftz); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + ftz); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + ftz); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + ftz); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDw(VURegs * VU) {
	float ftw;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftw=vuDouble(VU->VF[_Ft_].i.w);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + ftw); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + ftw); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + ftw); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + ftw); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDA(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAi(VURegs * VU) {
	float ti = vuDouble(VU->VI[REG_I].UL);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + ti); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + ti); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + ti); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + ti); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAq(VURegs * VU) {
	float tf = vuDouble(VU->VI[REG_Q].UL);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + tf); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + tf); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + tf); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + tf); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAx(VURegs * VU) {
	float tx = vuDouble(VU->VF[_Ft_].i.x);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + tx); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + tx); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + tx); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + tx); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAy(VURegs * VU) {
	float ty = vuDouble(VU->VF[_Ft_].i.y);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + ty); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + ty); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + ty); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + ty); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAz(VURegs * VU) {
	float tz = vuDouble(VU->VF[_Ft_].i.z);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + tz); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + tz); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + tz); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + tz); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/

void _vuADDAw(VURegs * VU) {
	float tw = vuDouble(VU->VF[_Ft_].i.w);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) + tw); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) + tw); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) + tw); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) + tw); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/*Reworked from define to function. asadr*/


void _vuSUB(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VF[_Ft_].i.x));  } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VF[_Ft_].i.y));  } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VF[_Ft_].i.z));  } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VF[_Ft_].i.w));  } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBi(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBq(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBx(VURegs * VU) {
	float ftx;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftx=vuDouble(VU->VF[_Ft_].i.x);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - ftx); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - ftx); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - ftx); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - ftx); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBy(VURegs * VU) {
	float fty;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	fty=vuDouble(VU->VF[_Ft_].i.y);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - fty); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - fty); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - fty); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - fty); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBz(VURegs * VU) {
	float ftz;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftz=vuDouble(VU->VF[_Ft_].i.z);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - ftz); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - ftz); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - ftz); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - ftz); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBw(VURegs * VU) {
	float ftw;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

    ftw=vuDouble(VU->VF[_Ft_].i.w);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - ftw); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - ftw); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - ftw); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - ftw); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow


void _vuSUBA(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAi(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VI[REG_I].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAq(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - vuDouble(VU->VI[REG_Q].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAx(VURegs * VU) {
	float tx = vuDouble(VU->VF[_Ft_].i.x);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - tx); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - tx); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - tx); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - tx); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAy(VURegs * VU) {
	float ty = vuDouble(VU->VF[_Ft_].i.y);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - ty); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - ty); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - ty); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - ty); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAz(VURegs * VU) {
	float tz = vuDouble(VU->VF[_Ft_].i.z);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - tz); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - tz); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - tz); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - tz); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuSUBAw(VURegs * VU) {
	float tw = vuDouble(VU->VF[_Ft_].i.w);

	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) - tw); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) - tw); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) - tw); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) - tw); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}//updated 10/05/03 shadow

void _vuMUL(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

/* No need to presave I reg in ti. asadr */
void _vuMULi(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULq(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULx(VURegs * VU) {
	float ftx;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

 	ftx=vuDouble(VU->VF[_Ft_].i.x);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * ftx); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * ftx); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * ftx); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * ftx); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */


void _vuMULy(VURegs * VU) {
	float fty;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

 	fty=vuDouble(VU->VF[_Ft_].i.y);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * fty); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * fty); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * fty); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * fty); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULz(VURegs * VU) {
	float ftz;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

 	ftz=vuDouble(VU->VF[_Ft_].i.z);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * ftz); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * ftz); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * ftz); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * ftz); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULw(VURegs * VU) {
	float ftw;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftw=vuDouble(VU->VF[_Ft_].i.w);
	if (_X){ dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * ftw); } else VU_MACx_CLEAR(VU);
	if (_Y){ dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * ftw); } else VU_MACy_CLEAR(VU);
	if (_Z){ dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * ftw); } else VU_MACz_CLEAR(VU);
	if (_W){ dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * ftw); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */


void _vuMULA(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

/* No need to presave I reg in ti. asadr */
void _vuMULAi(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_I].UL)); } else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

/* No need to presave Q reg in ti. asadr */
void _vuMULAq(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_Q].UL)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

/* No need to presave X reg in ti. asadr */
void _vuMULAx(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.x)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULAy(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.y)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULAz(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.z)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMULAw(VURegs * VU) {
	if (_X){ VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACx_CLEAR(VU);
	if (_Y){ VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACy_CLEAR(VU);
	if (_Z){ VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACz_CLEAR(VU);
	if (_W){ VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w)); } else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 8/05/03 shadow */

void _vuMADD(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */


void _vuMADDi(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_I].UL))); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_I].UL))); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_I].UL))); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_I].UL))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

/* No need to presave . asadr */
void _vuMADDq(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

void _vuMADDx(VURegs * VU) {
	float ftx;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftx=vuDouble(VU->VF[_Ft_].i.x);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * ftx)); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * ftx)); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * ftx)); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * ftx)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

void _vuMADDy(VURegs * VU) {
	float fty;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	fty=vuDouble(VU->VF[_Ft_].i.y);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * fty)); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * fty)); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * fty)); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * fty)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

void _vuMADDz(VURegs * VU) {
	float ftz;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftz=vuDouble(VU->VF[_Ft_].i.z);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * ftz)); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * ftz)); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * ftz)); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * ftz)); else VU_MACw_CLEAR(VU);
	VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

void _vuMADDw(VURegs * VU) {
	float ftw;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftw=vuDouble(VU->VF[_Ft_].i.w);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * ftw)); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * ftw)); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * ftw)); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * ftw)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 10/05/03 shadow */

void _vuMADDA(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + (vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + (vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + (vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + (vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  10/05/03 shadow*/

void _vuMADDAi(VURegs * VU) {
	float ti = vuDouble(VU->VI[REG_I].UL);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * ti)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * ti)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * ti)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * ti)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  10/05/03 shadow*/

void _vuMADDAq(VURegs * VU) {
	float tq = vuDouble(VU->VI[REG_Q].UL);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * tq)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * tq)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * tq)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * tq)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last update  10/05/03 shadow*/

void _vuMADDAx(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last update 11/05/03 shadow*/

void _vuMADDAy(VURegs * VU) {
	if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last update  11/05/03 shadow*/

void _vuMADDAz(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last update  11/05/03 shadow*/

void _vuMADDAw(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) + ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) + ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) + ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) + ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last update  11/05/03 shadow*/

void _vuMSUB(VURegs * VU) {
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */

void _vuMSUBi(VURegs * VU) {
	float ti = vuDouble(VU->VI[REG_I].UL);
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * ti  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * ti  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * ti  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * ti  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */

void _vuMSUBq(VURegs * VU) {
	float tq = vuDouble(VU->VI[REG_Q].UL);
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x)  - ( vuDouble(VU->VF[_Fs_].i.x) * tq  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y)  - ( vuDouble(VU->VF[_Fs_].i.y) * tq  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z)  - ( vuDouble(VU->VF[_Fs_].i.z) * tq  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w)  - ( vuDouble(VU->VF[_Fs_].i.w) * tq  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */


void _vuMSUBx(VURegs * VU) {
	float ftx;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftx=vuDouble(VU->VF[_Ft_].i.x);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x)  - ( vuDouble(VU->VF[_Fs_].i.x) * ftx  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y)  - ( vuDouble(VU->VF[_Fs_].i.y) * ftx  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z)  - ( vuDouble(VU->VF[_Fs_].i.z) * ftx  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w)  - ( vuDouble(VU->VF[_Fs_].i.w) * ftx  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */


void _vuMSUBy(VURegs * VU) {
	float fty;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	fty=vuDouble(VU->VF[_Ft_].i.y);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x)  - ( vuDouble(VU->VF[_Fs_].i.x) * fty  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y)  - ( vuDouble(VU->VF[_Fs_].i.y) * fty  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z)  - ( vuDouble(VU->VF[_Fs_].i.z) * fty  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w)  - ( vuDouble(VU->VF[_Fs_].i.w) * fty  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */


void _vuMSUBz(VURegs * VU) {
	float ftz;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftz=vuDouble(VU->VF[_Ft_].i.z);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x)  - ( vuDouble(VU->VF[_Fs_].i.x) * ftz  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y)  - ( vuDouble(VU->VF[_Fs_].i.y) * ftz  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z)  - ( vuDouble(VU->VF[_Fs_].i.z) * ftz  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w)  - ( vuDouble(VU->VF[_Fs_].i.w) * ftz  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */

void _vuMSUBw(VURegs * VU) {
	float ftw;
	VECTOR * dst;
    if (_Fd_ == 0) dst = &RDzero;
	else dst = &VU->VF[_Fd_];

	ftw=vuDouble(VU->VF[_Ft_].i.w);
    if (_X) dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x)  - ( vuDouble(VU->VF[_Fs_].i.x) * ftw  ) ); else VU_MACx_CLEAR(VU);
    if (_Y) dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y)  - ( vuDouble(VU->VF[_Fs_].i.y) * ftw  ) ); else VU_MACy_CLEAR(VU);
    if (_Z) dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z)  - ( vuDouble(VU->VF[_Fs_].i.z) * ftw  ) ); else VU_MACz_CLEAR(VU);
    if (_W) dst->i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w)  - ( vuDouble(VU->VF[_Fs_].i.w) * ftw  ) ); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/* last update 11/05/03 shadow */


void _vuMSUBA(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.x))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.y))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.z))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VF[_Ft_].i.w))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAi(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_I].UL))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_I].UL))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_I].UL))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_I].UL))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAq(VURegs * VU) {
    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * vuDouble(VU->VI[REG_Q].UL))); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAx(VURegs * VU) {
	float tx = vuDouble(VU->VF[_Ft_].i.x);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * tx)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * tx)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * tx)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * tx)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAy(VURegs * VU) {
	float ty = vuDouble(VU->VF[_Ft_].i.y);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * ty)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * ty)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * ty)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * ty)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAz(VURegs * VU) {
	float tz = vuDouble(VU->VF[_Ft_].i.z);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * tz)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * tz)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * tz)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * tz)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

void _vuMSUBAw(VURegs * VU) {
	float tw = vuDouble(VU->VF[_Ft_].i.w);

    if (_X) VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - ( vuDouble(VU->VF[_Fs_].i.x) * tw)); else VU_MACx_CLEAR(VU);
    if (_Y) VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - ( vuDouble(VU->VF[_Fs_].i.y) * tw)); else VU_MACy_CLEAR(VU);
    if (_Z) VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - ( vuDouble(VU->VF[_Fs_].i.z) * tw)); else VU_MACz_CLEAR(VU);
    if (_W) VU->ACC.i.w = VU_MACw_UPDATE(VU, vuDouble(VU->ACC.i.w) - ( vuDouble(VU->VF[_Fs_].i.w) * tw)); else VU_MACw_CLEAR(VU);
    VU_STAT_UPDATE(VU);
}/*last updated  11/05/03 shadow*/

u32 _MAX(u32 a, u32 b) {
	if (a & 0x80000000) { // -a
		if (b & 0x80000000) { // -b
			return (a & 0x7fffffff) > (b & 0x7fffffff) ? b : a;
		} else { // +b
			return b;
		}
	} else { // +a
		if (b & 0x80000000) { // -b
			return a;
		} else { // +b
			return (a & 0x7fffffff) > (b & 0x7fffffff) ? a : b;
		}
	}

	return 0;
}

void _vuMAX(VURegs * VU) {
	if (_Fd_ == 0) return;

	/* ft is bc */
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, (s32)VU->VF[_Ft_].i.x);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, (s32)VU->VF[_Ft_].i.y);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, (s32)VU->VF[_Ft_].i.z);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, (s32)VU->VF[_Ft_].i.w);
}//checked 13/05/03 shadow

void _vuMAXi(VURegs * VU) {
	if (_Fd_ == 0) return;

	/* ft is bc */
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, VU->VI[REG_I].UL);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, VU->VI[REG_I].UL);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, VU->VI[REG_I].UL);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, VU->VI[REG_I].UL);
}//checked 13/05/03 shadow

void _vuMAXx(VURegs * VU) {
	s32 ftx;
	if (_Fd_ == 0) return;

	ftx=(s32)VU->VF[_Ft_].i.x;
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, ftx);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, ftx);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, ftx);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, ftx);
}
//checked 13/05/03 shadow

void _vuMAXy(VURegs * VU) {
	s32 fty;
	if (_Fd_ == 0) return;

	fty=(s32)VU->VF[_Ft_].i.y;
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, fty);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, fty);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, fty);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, fty);
}//checked 13/05/03 shadow

void _vuMAXz(VURegs * VU) {
	s32 ftz;
	if (_Fd_ == 0) return;

	ftz=(s32)VU->VF[_Ft_].i.z;
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, ftz);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, ftz);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, ftz);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, ftz);
}

void _vuMAXw(VURegs * VU) {
	s32 ftw;
	if (_Fd_ == 0) return;

	ftw=(s32)VU->VF[_Ft_].i.w;
	if (_X) VU->VF[_Fd_].i.x = _MAX(VU->VF[_Fs_].i.x, ftw);
	if (_Y) VU->VF[_Fd_].i.y = _MAX(VU->VF[_Fs_].i.y, ftw);
	if (_Z) VU->VF[_Fd_].i.z = _MAX(VU->VF[_Fs_].i.z, ftw);
	if (_W) VU->VF[_Fd_].i.w = _MAX(VU->VF[_Fs_].i.w, ftw);
}

u32 _MINI(u32 a, u32 b) {
	if (a & 0x80000000) { // -a
		if (b & 0x80000000) { // -b
			return (a & 0x7fffffff) < (b & 0x7fffffff) ? b : a;
		} else { // +b
			return a;
		}
	} else { // +a
		if (b & 0x80000000) { // -b
			return b;
		} else { // +b
			return (a & 0x7fffffff) < (b & 0x7fffffff) ? a : b;
		}
	}

	return 0;
}

void _vuMINI(VURegs * VU) {
	if (_Fd_ == 0) return;

	/* ft is bc */
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, (s32)VU->VF[_Ft_].i.x);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, (s32)VU->VF[_Ft_].i.y);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, (s32)VU->VF[_Ft_].i.z);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, (s32)VU->VF[_Ft_].i.w);
}//checked 13/05/03 shadow

void _vuMINIi(VURegs * VU) {
	if (_Fd_ == 0) return;

	/* ft is bc */
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, VU->VI[REG_I].UL);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, VU->VI[REG_I].UL);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, VU->VI[REG_I].UL);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, VU->VI[REG_I].UL);
}//checked 13/05/03 shadow

void _vuMINIx(VURegs * VU) {
	s32 ftx;
	if (_Fd_ == 0) return;

	ftx=(s32)VU->VF[_Ft_].i.x;
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, ftx);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, ftx);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, ftx);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, ftx);
}
//checked 13/05/03 shadow

void _vuMINIy(VURegs * VU) {
	s32 fty;
	if (_Fd_ == 0) return;

	fty=(s32)VU->VF[_Ft_].i.y;
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, fty);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, fty);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, fty);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, fty);
}//checked 13/05/03 shadow

void _vuMINIz(VURegs * VU) {
	s32 ftz;
	if (_Fd_ == 0) return;

	ftz=(s32)VU->VF[_Ft_].i.z;
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, ftz);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, ftz);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, ftz);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, ftz);
}

void _vuMINIw(VURegs * VU) {
	s32 ftw;
	if (_Fd_ == 0) return;

	ftw=(s32)VU->VF[_Ft_].i.w;
	if (_X) VU->VF[_Fd_].i.x = _MINI(VU->VF[_Fs_].i.x, ftw);
	if (_Y) VU->VF[_Fd_].i.y = _MINI(VU->VF[_Fs_].i.y, ftw);
	if (_Z) VU->VF[_Fd_].i.z = _MINI(VU->VF[_Fs_].i.z, ftw);
	if (_W) VU->VF[_Fd_].i.w = _MINI(VU->VF[_Fs_].i.w, ftw);
}

void _vuOPMULA(VURegs * VU) {
	VU->ACC.i.x = VU_MACx_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Ft_].i.z));
	VU->ACC.i.y = VU_MACy_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Ft_].i.x));
	VU->ACC.i.z = VU_MACz_UPDATE(VU, vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Ft_].i.y));
	VU_STAT_UPDATE(VU);
}/*last updated  8/05/03 shadow*/

void _vuOPMSUB(VURegs * VU) {
	VECTOR * dst;
	float ftx, fty, ftz;
	float fsx, fsy, fsz;
	if (_Fd_ == 0)
		dst = &RDzero;
	else
		dst = &VU->VF[_Fd_];

	ftx = vuDouble(VU->VF[_Ft_].i.x); fty = vuDouble(VU->VF[_Ft_].i.y); ftz = vuDouble(VU->VF[_Ft_].i.z);
	fsx = vuDouble(VU->VF[_Fs_].i.x); fsy = vuDouble(VU->VF[_Fs_].i.y); fsz = vuDouble(VU->VF[_Fs_].i.z);
	dst->i.x = VU_MACx_UPDATE(VU, vuDouble(VU->ACC.i.x) - fsy * ftz);
	dst->i.y = VU_MACy_UPDATE(VU, vuDouble(VU->ACC.i.y) - fsz * ftx);
	dst->i.z = VU_MACz_UPDATE(VU, vuDouble(VU->ACC.i.z) - fsx * fty);
	VU_STAT_UPDATE(VU);
}/*last updated  8/05/03 shadow*/

void _vuNOP(VURegs * VU) {
}

void _vuFTOI0(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].SL[0] = (s32)vuDouble(VU->VF[_Fs_].i.x);
	if (_Y) VU->VF[_Ft_].SL[1] = (s32)vuDouble(VU->VF[_Fs_].i.y);
	if (_Z) VU->VF[_Ft_].SL[2] = (s32)vuDouble(VU->VF[_Fs_].i.z);
	if (_W) VU->VF[_Ft_].SL[3] = (s32)vuDouble(VU->VF[_Fs_].i.w);
}

void _vuFTOI4(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].SL[0] = float_to_int4(vuDouble(VU->VF[_Fs_].i.x));
	if (_Y) VU->VF[_Ft_].SL[1] = float_to_int4(vuDouble(VU->VF[_Fs_].i.y));
	if (_Z) VU->VF[_Ft_].SL[2] = float_to_int4(vuDouble(VU->VF[_Fs_].i.z));
	if (_W) VU->VF[_Ft_].SL[3] = float_to_int4(vuDouble(VU->VF[_Fs_].i.w));
}

void _vuFTOI12(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].SL[0] = float_to_int12(vuDouble(VU->VF[_Fs_].i.x));
	if (_Y) VU->VF[_Ft_].SL[1] = float_to_int12(vuDouble(VU->VF[_Fs_].i.y));
	if (_Z) VU->VF[_Ft_].SL[2] = float_to_int12(vuDouble(VU->VF[_Fs_].i.z));
	if (_W) VU->VF[_Ft_].SL[3] = float_to_int12(vuDouble(VU->VF[_Fs_].i.w));
}

void _vuFTOI15(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].SL[0] = float_to_int15(vuDouble(VU->VF[_Fs_].i.x));
	if (_Y) VU->VF[_Ft_].SL[1] = float_to_int15(vuDouble(VU->VF[_Fs_].i.y));
	if (_Z) VU->VF[_Ft_].SL[2] = float_to_int15(vuDouble(VU->VF[_Fs_].i.z));
	if (_W) VU->VF[_Ft_].SL[3] = float_to_int15(vuDouble(VU->VF[_Fs_].i.w));
}

void _vuITOF0(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].f.x = (float)VU->VF[_Fs_].SL[0];
	if (_Y) VU->VF[_Ft_].f.y = (float)VU->VF[_Fs_].SL[1];
	if (_Z) VU->VF[_Ft_].f.z = (float)VU->VF[_Fs_].SL[2];
	if (_W) VU->VF[_Ft_].f.w = (float)VU->VF[_Fs_].SL[3];
}

void _vuITOF4(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].f.x = int4_to_float(VU->VF[_Fs_].SL[0]);
	if (_Y) VU->VF[_Ft_].f.y = int4_to_float(VU->VF[_Fs_].SL[1]);
	if (_Z) VU->VF[_Ft_].f.z = int4_to_float(VU->VF[_Fs_].SL[2]);
	if (_W) VU->VF[_Ft_].f.w = int4_to_float(VU->VF[_Fs_].SL[3]);
}

void _vuITOF12(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].f.x = int12_to_float(VU->VF[_Fs_].SL[0]);
	if (_Y) VU->VF[_Ft_].f.y = int12_to_float(VU->VF[_Fs_].SL[1]);
	if (_Z) VU->VF[_Ft_].f.z = int12_to_float(VU->VF[_Fs_].SL[2]);
	if (_W) VU->VF[_Ft_].f.w = int12_to_float(VU->VF[_Fs_].SL[3]);
}

void _vuITOF15(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].f.x = int15_to_float(VU->VF[_Fs_].SL[0]);
	if (_Y) VU->VF[_Ft_].f.y = int15_to_float(VU->VF[_Fs_].SL[1]);
	if (_Z) VU->VF[_Ft_].f.z = int15_to_float(VU->VF[_Fs_].SL[2]);
	if (_W) VU->VF[_Ft_].f.w = int15_to_float(VU->VF[_Fs_].SL[3]);
}

/* Different type of clipping by presaving w. asadr */
void _vuCLIP(VURegs * VU) {
	float value = fabs(vuDouble(VU->VF[_Ft_].i.w));

	VU->clipflag <<= 6;
	if ( vuDouble(VU->VF[_Fs_].i.x) > +value ) VU->clipflag|= 0x01;
	if ( vuDouble(VU->VF[_Fs_].i.x) < -value ) VU->clipflag|= 0x02;
	if ( vuDouble(VU->VF[_Fs_].i.y) > +value ) VU->clipflag|= 0x04;
	if ( vuDouble(VU->VF[_Fs_].i.y) < -value ) VU->clipflag|= 0x08;
	if ( vuDouble(VU->VF[_Fs_].i.z) > +value ) VU->clipflag|= 0x10;
	if ( vuDouble(VU->VF[_Fs_].i.z) < -value ) VU->clipflag|= 0x20;
	VU->clipflag = VU->clipflag & 0xFFFFFF;
	VU->VI[REG_CLIP_FLAG].UL = VU->clipflag;


}/*last update 16/07/05 refraction - Needs checking */


/******************************/
/*   VU Lower instructions    */
/******************************/

void _vuDIV(VURegs * VU) {
	float ft = vuDouble(VU->VF[_Ft_].UL[_Ftf_]);
	float fs = vuDouble(VU->VF[_Fs_].UL[_Fsf_]);

	VU->statusflag = (VU->statusflag&0xfcf)|((VU->statusflag&0x30)<<6);

	if (ft == 0.0) {
		if (fs == 0.0) {
			VU->statusflag |= 0x10;
		} else {
			VU->statusflag |= 0x20;
		}
		if ((VU->VF[_Ft_].UL[_Ftf_] & 0x80000000) ^
			(VU->VF[_Fs_].UL[_Fsf_] & 0x80000000)) {
			VU->q.UL = 0xFF7FFFFF;
		} else {
			VU->q.UL = 0x7F7FFFFF;
		}
	} else {
		VU->q.F = fs / ft;
		VU->q.F = vuDouble(VU->q.UL);
	}
} //last update 15/01/06 zerofrog

void _vuSQRT(VURegs * VU) {
	float ft = vuDouble(VU->VF[_Ft_].UL[_Ftf_]);

	VU->statusflag = (VU->statusflag&0xfcf)|((VU->statusflag&0x30)<<6);

	if (ft < 0.0 ) VU->statusflag |= 0x10;
	VU->q.F = sqrt(fabs(ft));
	VU->q.F = vuDouble(VU->q.UL);
} //last update 15/01/06 zerofrog

/* Eminent Bug - Dvisior == 0 Check Missing ( D Flag Not Set ) */
/* REFIXED....ASADR; rerefixed....zerofrog */
void _vuRSQRT(VURegs * VU) {
	float ft = vuDouble(VU->VF[_Ft_].UL[_Ftf_]);
	float fs = vuDouble(VU->VF[_Fs_].UL[_Fsf_]);
	float temp;

	VU->statusflag = (VU->statusflag&0xfcf)|((VU->statusflag&0x30)<<6);

	if ( ft == 0.0 ) {
		VU->statusflag |= 0x20;

		if( fs != 0 ) {
			if ((VU->VF[_Ft_].UL[_Ftf_] & 0x80000000) ^
				(VU->VF[_Fs_].UL[_Fsf_] & 0x80000000)) {
				VU->q.UL = 0xFF7FFFFF;
			} else {
				VU->q.UL = 0x7F7FFFFF;
			}
		}
		else {
			if ((VU->VF[_Ft_].UL[_Ftf_] & 0x80000000) ^
				(VU->VF[_Fs_].UL[_Fsf_] & 0x80000000)) {
				VU->q.UL = 0x80000000;
			} else {
				VU->q.UL = 0;
			}

			VU->statusflag |= 0x10;
		}

	} else {
		if (ft < 0.0) {
			VU->statusflag |= 0x10;
		}

		temp = sqrt(fabs(ft));
		VU->q.F = fs / temp;
		VU->q.F = vuDouble(VU->q.UL);
	}
} //last update 15/01/06 zerofrog

void _vuIADDI(VURegs * VU) {
	s16 imm = ((VU->code >> 6) & 0x1f);
	imm = ((imm & 0x10 ? 0xfff0 : 0) | (imm & 0xf));
	if(_It_ == 0) return;
	VU->VI[_It_].SS[0] = VU->VI[_Is_].SS[0] + imm;
}//last checked 17/05/03 shadow NOTE: not quite sure about that

void _vuIADDIU(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].SS[0] = VU->VI[_Is_].SS[0] + (((VU->code >> 10) & 0x7800) | (VU->code & 0x7ff));
}//last checked 17/05/03 shadow

void _vuIADD(VURegs * VU) {
	if(_Id_ == 0) return;
	VU->VI[_Id_].SS[0] = VU->VI[_Is_].SS[0] + VU->VI[_It_].SS[0];
}//last checked 17/05/03 shadow

void _vuIAND(VURegs * VU) {
	if(_Id_ == 0) return;
	VU->VI[_Id_].US[0] = VU->VI[_Is_].US[0] & VU->VI[_It_].US[0];
}//last checked 17/05/03 shadow

void _vuIOR(VURegs * VU) {
	if(_Id_ == 0) return;
	VU->VI[_Id_].US[0] = VU->VI[_Is_].US[0] | VU->VI[_It_].US[0];
}

void _vuISUB(VURegs * VU) {
	if(_Id_ == 0) return;
	VU->VI[_Id_].SS[0] = VU->VI[_Is_].SS[0] - VU->VI[_It_].SS[0];
}

void _vuISUBIU(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].SS[0] = VU->VI[_Is_].SS[0] - (((VU->code >> 10) & 0x7800) | (VU->code & 0x7ff));
}

void _vuMOVE(VURegs * VU) {
	if(_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].UL[0] = VU->VF[_Fs_].UL[0];
	if (_Y) VU->VF[_Ft_].UL[1] = VU->VF[_Fs_].UL[1];
	if (_Z) VU->VF[_Ft_].UL[2] = VU->VF[_Fs_].UL[2];
	if (_W) VU->VF[_Ft_].UL[3] = VU->VF[_Fs_].UL[3];
}//last checked 17/05/03 shadow

void _vuMFIR(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].SL[0] = (s32)VU->VI[_Is_].SS[0];
	if (_Y) VU->VF[_Ft_].SL[1] = (s32)VU->VI[_Is_].SS[0];
	if (_Z) VU->VF[_Ft_].SL[2] = (s32)VU->VI[_Is_].SS[0];
	if (_W) VU->VF[_Ft_].SL[3] = (s32)VU->VI[_Is_].SS[0];
}

// Big bug!!! mov from fs to ft not ft to fs. asadr
void _vuMTIR(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] =  *(u16*)&VU->VF[_Fs_].F[_Fsf_];
}

void _vuMR32(VURegs * VU) {
	u32 tx;
	if (_Ft_ == 0) return;

	tx = VU->VF[_Fs_].i.x;
	if (_X) VU->VF[_Ft_].i.x = VU->VF[_Fs_].i.y;
	if (_Y) VU->VF[_Ft_].i.y = VU->VF[_Fs_].i.z;
	if (_Z) VU->VF[_Ft_].i.z = VU->VF[_Fs_].i.w;
	if (_W) VU->VF[_Ft_].i.w = tx;
}//last updated 23/10/03 linuzappz

// --------------------------------------------------------------------------------------
//  Load / Store Instructions (VU Interpreter)
// --------------------------------------------------------------------------------------

__forceinline u32* GET_VU_MEM(VURegs* VU, u32 addr)		// non-static, also used by sVU for now.
{
	if( VU == g_pVU1 ) return (u32*)(VU1.Mem+(addr&0x3fff));
	if( addr >= 0x4000 ) return (u32*)(VU0.Mem+(addr&0x43f0)); // get VF and VI regs (they're mapped to 0x4xx0 in VU0 mem!)
	return (u32*)(VU0.Mem+(addr&0x0fff)); // for addr 0x0000 to 0x4000 just wrap around
}

void _vuLQ(VURegs * VU) {
	s16 imm;
	u16 addr;
	u32 *ptr;

	if (_Ft_ == 0) return;

	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff);
	addr = ((imm + VU->VI[_Is_].SS[0]) * 16)& (VU == &VU1 ? 0x3fff : 0xfff);

 	ptr = (u32*)GET_VU_MEM(VU, addr);
	if (_X) VU->VF[_Ft_].UL[0] = ptr[0];
	if (_Y) VU->VF[_Ft_].UL[1] = ptr[1];
	if (_Z) VU->VF[_Ft_].UL[2] = ptr[2];
	if (_W) VU->VF[_Ft_].UL[3] = ptr[3];
}

void _vuLQD( VURegs * VU ) {
	u32 addr;
	u32 *ptr;

	if (_Is_ != 0) VU->VI[_Is_].US[0]--;
	if (_Ft_ == 0) return;

	addr = (VU->VI[_Is_].US[0] * 16) & (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u32*)GET_VU_MEM(VU, addr);
	if (_X) VU->VF[_Ft_].UL[0] = ptr[0];
	if (_Y) VU->VF[_Ft_].UL[1] = ptr[1];
	if (_Z) VU->VF[_Ft_].UL[2] = ptr[2];
	if (_W) VU->VF[_Ft_].UL[3] = ptr[3];
}

void _vuLQI(VURegs * VU) {
	if (_Ft_) {
		u32 addr;
		u32 *ptr;

		addr = (VU->VI[_Is_].US[0] * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
		ptr = (u32*)GET_VU_MEM(VU, addr);
		if (_X) VU->VF[_Ft_].UL[0] = ptr[0];
		if (_Y) VU->VF[_Ft_].UL[1] = ptr[1];
		if (_Z) VU->VF[_Ft_].UL[2] = ptr[2];
		if (_W) VU->VF[_Ft_].UL[3] = ptr[3];
	}
	if (_Fs_ != 0) VU->VI[_Is_].US[0]++;
}

/* addr is now signed. Asadr */
void _vuSQ(VURegs * VU) {
	s16 imm;
	u16 addr;
	u32 *ptr;

	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff);
	addr = ((imm + VU->VI[_It_].SS[0]) * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u32*)GET_VU_MEM(VU, addr);
	if (_X) ptr[0] = VU->VF[_Fs_].UL[0];
	if (_Y) ptr[1] = VU->VF[_Fs_].UL[1];
	if (_Z) ptr[2] = VU->VF[_Fs_].UL[2];
	if (_W) ptr[3] = VU->VF[_Fs_].UL[3];
}

void _vuSQD(VURegs * VU) {
	u32 addr;
	u32 *ptr;

	if(_Ft_ != 0) VU->VI[_It_].US[0]--;
	addr = (VU->VI[_It_].US[0] * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u32*)GET_VU_MEM(VU, addr);
	if (_X) ptr[0] = VU->VF[_Fs_].UL[0];
	if (_Y) ptr[1] = VU->VF[_Fs_].UL[1];
	if (_Z) ptr[2] = VU->VF[_Fs_].UL[2];
	if (_W) ptr[3] = VU->VF[_Fs_].UL[3];
}

void _vuSQI(VURegs * VU) {
	u32 addr;
	u32 *ptr;

	addr = (VU->VI[_It_].US[0] * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u32*)GET_VU_MEM(VU, addr);
	if (_X) ptr[0] = VU->VF[_Fs_].UL[0];
	if (_Y) ptr[1] = VU->VF[_Fs_].UL[1];
	if (_Z) ptr[2] = VU->VF[_Fs_].UL[2];
	if (_W) ptr[3] = VU->VF[_Fs_].UL[3];
	if(_Ft_ != 0) VU->VI[_It_].US[0]++;
}

/* addr now signed. asadr */
void _vuILW(VURegs * VU) {
	s16 imm;
	u16 addr;
	u16 *ptr;
	if (_It_ == 0) return;

 	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff);
	addr = ((imm + VU->VI[_Is_].SS[0]) * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u16*)GET_VU_MEM(VU, addr);
	if (_X) VU->VI[_It_].US[0] = ptr[0];
	if (_Y) VU->VI[_It_].US[0] = ptr[2];
	if (_Z) VU->VI[_It_].US[0] = ptr[4];
	if (_W) VU->VI[_It_].US[0] = ptr[6];
}

void _vuISW(VURegs * VU) {
	s16 imm;
	u16 addr;
	u16 *ptr;

 	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff);
	addr = ((imm + VU->VI[_Is_].SS[0]) * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u16*)GET_VU_MEM(VU, addr);
	if (_X) { ptr[0] = VU->VI[_It_].US[0]; ptr[1] = 0; }
	if (_Y) { ptr[2] = VU->VI[_It_].US[0]; ptr[3] = 0; }
	if (_Z) { ptr[4] = VU->VI[_It_].US[0]; ptr[5] = 0; }
	if (_W) { ptr[6] = VU->VI[_It_].US[0]; ptr[7] = 0; }
}

void _vuILWR(VURegs * VU) {
	u32 addr;
	u16 *ptr;
	if (_It_ == 0) return;

	addr = (VU->VI[_Is_].US[0] * 16)& (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u16*)GET_VU_MEM(VU, addr);
	if (_X) VU->VI[_It_].US[0] = ptr[0];
	if (_Y) VU->VI[_It_].US[0] = ptr[2];
	if (_Z) VU->VI[_It_].US[0] = ptr[4];
	if (_W) VU->VI[_It_].US[0] = ptr[6];
}

void _vuISWR(VURegs * VU) {
	u32 addr;
	u16 *ptr;

	addr = (VU->VI[_Is_].US[0] * 16) & (VU == &VU1 ? 0x3fff : 0xfff);
	ptr = (u16*)GET_VU_MEM(VU, addr);
	if (_X) { ptr[0] = VU->VI[_It_].US[0]; ptr[1] = 0; }
	if (_Y) { ptr[2] = VU->VI[_It_].US[0]; ptr[3] = 0; }
	if (_Z) { ptr[4] = VU->VI[_It_].US[0]; ptr[5] = 0; }
	if (_W) { ptr[6] = VU->VI[_It_].US[0]; ptr[7] = 0; }
}

/* code contributed by _Riff_

The following code implements a Galois form M-series LFSR that can be configured to have a width from 0 to 32.
A Galois field can be represented as G(X) = g_m * X^m + g_(m-1) * X^(m-1) + ... + g_1 * X^1 + g0.
A Galois form M-Series LFSR represents a Galois field where g0 = g_m = 1 and the generated set contains 2^M - 1 values.
In modulo-2 arithmetic, addition is replaced by XOR and multiplication is replaced by AND.
The code is written in such a way that the polynomial lsb (g0) should be set to 0 and g_m is not represented.
As an example for setting the polynomial variable correctly, the 23-bit M-series generating polynomial X^23+X^14
  would be specified as (1 << 14).
*/

//The two-tap 23 stage M-series polynomials are x23+x18 and x23+x14 ((1 << 18) and (1 << 14), respectively).
//The reverse sequences can be generated by x23+x(23-18) and x23+x(23-14) ((1 << 9) and (1 << 5), respectively)
u32 poly = 1 << 5;

void SetPoly(u32 newPoly) {
	poly = poly & ~1;
}

void AdvanceLFSR(VURegs * VU) {
	// code from www.project-fao.org (which is no longer there)
	int x = (VU->VI[REG_R].UL >> 4) & 1;
	int y = (VU->VI[REG_R].UL >> 22) & 1;
	VU->VI[REG_R].UL <<= 1;
	VU->VI[REG_R].UL ^= x ^ y;
	VU->VI[REG_R].UL = (VU->VI[REG_R].UL&0x7fffff)|0x3f800000;
}

void _vuRINIT(VURegs * VU) {
	VU->VI[REG_R].UL = 0x3F800000 | (VU->VF[_Fs_].UL[_Fsf_] & 0x007FFFFF);
}

void _vuRGET(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].UL[0] = VU->VI[REG_R].UL;
	if (_Y) VU->VF[_Ft_].UL[1] = VU->VI[REG_R].UL;
	if (_Z) VU->VF[_Ft_].UL[2] = VU->VI[REG_R].UL;
	if (_W) VU->VF[_Ft_].UL[3] = VU->VI[REG_R].UL;
}

void _vuRNEXT(VURegs * VU) {
	if (_Ft_ == 0) return;
	AdvanceLFSR(VU);
	if (_X) VU->VF[_Ft_].UL[0] = VU->VI[REG_R].UL;
	if (_Y) VU->VF[_Ft_].UL[1] = VU->VI[REG_R].UL;
	if (_Z) VU->VF[_Ft_].UL[2] = VU->VI[REG_R].UL;
	if (_W) VU->VF[_Ft_].UL[3] = VU->VI[REG_R].UL;
}

void _vuRXOR(VURegs * VU) {
	VU->VI[REG_R].UL = 0x3F800000 | ((VU->VI[REG_R].UL ^ VU->VF[_Fs_].UL[_Fsf_]) & 0x007FFFFF);
}

void _vuWAITQ(VURegs * VU) {
}

void _vuFSAND(VURegs * VU) {
	u16 imm;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = (VU->VI[REG_STATUS_FLAG].US[0] & 0xFFF) & imm;
}

void _vuFSEQ(VURegs * VU) {
	u16 imm;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_It_ == 0) return;
	if((VU->VI[REG_STATUS_FLAG].US[0] & 0xFFF) == imm) VU->VI[_It_].US[0] = 1;
	else VU->VI[_It_].US[0] = 0;
}

void _vuFSOR(VURegs * VU) {
	u16 imm;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = (VU->VI[REG_STATUS_FLAG].US[0] & 0xFFF) | imm;
}

void _vuFSSET(VURegs * VU) {
	u16 imm = 0;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7FF);
	VU->statusflag = (imm & 0xFC0) | (VU->VI[REG_STATUS_FLAG].US[0] & 0x3F);
}

void _vuFMAND(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = VU->VI[_Is_].US[0] & (VU->VI[REG_MAC_FLAG].UL & 0xFFFF);
}

void _vuFMEQ(VURegs * VU) {
	if(_It_ == 0) return;
	if((VU->VI[REG_MAC_FLAG].UL & 0xFFFF) == VU->VI[_Is_].US[0]){
	VU->VI[_It_].US[0] =1;} else { VU->VI[_It_].US[0] =0; }
}

void _vuFMOR(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = (VU->VI[REG_MAC_FLAG].UL & 0xFFFF) | VU->VI[_Is_].US[0];
}

void _vuFCAND(VURegs * VU) {
	if((VU->VI[REG_CLIP_FLAG].UL & 0xFFFFFF) & (VU->code & 0xFFFFFF)) VU->VI[1].US[0] = 1;
	else VU->VI[1].US[0] = 0;
}

void _vuFCEQ(VURegs * VU) {
	if((VU->VI[REG_CLIP_FLAG].UL & 0xFFFFFF) == (VU->code & 0xFFFFFF)) VU->VI[1].US[0] = 1;
	else VU->VI[1].US[0] = 0;
}

void _vuFCOR(VURegs * VU) {
	u32 hold = (VU->VI[REG_CLIP_FLAG].UL & 0xFFFFFF) | ( VU->code & 0xFFFFFF);
	if(hold == 0xFFFFFF) VU->VI[1].US[0] = 1;
	else VU->VI[1].US[0] = 0;
}

void _vuFCSET(VURegs * VU) {
	VU->clipflag = (u32) (VU->code & 0xFFFFFF);
	VU->VI[REG_CLIP_FLAG].UL = (u32) (VU->code & 0xFFFFFF);
}

void _vuFCGET(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = VU->VI[REG_CLIP_FLAG].UL & 0x0FFF;
}

s32 _branchAddr(VURegs * VU) {
	s32 bpc = VU->VI[REG_TPC].SL + ( _Imm11_ * 8 );
	bpc&= (VU == &VU1) ? 0x3fff : 0x0fff;
	return bpc;
}

void _setBranch(VURegs * VU, u32 bpc) {
	VU->branch = 2;
	VU->branchpc = bpc;
}

void _vuIBEQ(VURegs * VU) {
	if (VU->VI[_It_].US[0] == VU->VI[_Is_].US[0]) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuIBGEZ(VURegs * VU) {
	if (VU->VI[_Is_].SS[0] >= 0) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuIBGTZ(VURegs * VU) {
	if (VU->VI[_Is_].SS[0] > 0) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuIBLEZ(VURegs * VU) {
	if (VU->VI[_Is_].SS[0] <= 0) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuIBLTZ(VURegs * VU) {
	if (VU->VI[_Is_].SS[0] < 0) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuIBNE(VURegs * VU) {
	if (VU->VI[_It_].US[0] != VU->VI[_Is_].US[0]) {
		s32 bpc = _branchAddr(VU);
		_setBranch(VU, bpc);
	}
}

void _vuB(VURegs * VU) {
	s32 bpc = _branchAddr(VU);
	_setBranch(VU, bpc);
}

void _vuBAL(VURegs * VU) {
	s32 bpc = _branchAddr(VU);

	if (_It_) VU->VI[_It_].US[0] = (VU->VI[REG_TPC].UL + 8)/8;

	_setBranch(VU, bpc);
}

void _vuJR(VURegs * VU) {
    u32 bpc = VU->VI[_Is_].US[0] * 8;
	_setBranch(VU, bpc);
}

void _vuJALR(VURegs * VU) {
    u32 bpc = VU->VI[_Is_].US[0] * 8;
	if (_It_) VU->VI[_It_].US[0] = (VU->VI[REG_TPC].UL + 8)/8;

	_setBranch(VU, bpc);
}

void _vuMFP(VURegs * VU) {
	if (_Ft_ == 0) return;

	if (_X) VU->VF[_Ft_].i.x = VU->VI[REG_P].UL;
	if (_Y) VU->VF[_Ft_].i.y = VU->VI[REG_P].UL;
	if (_Z) VU->VF[_Ft_].i.z = VU->VI[REG_P].UL;
	if (_W) VU->VF[_Ft_].i.w = VU->VI[REG_P].UL;
}

void _vuWAITP(VURegs * VU) {
}

void _vuESADD(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Fs_].i.z);
	VU->p.F = p;
}

void _vuERSADD(VURegs * VU) {
	float p = (vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Fs_].i.x)) + (vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Fs_].i.y)) + (vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Fs_].i.z));
	if (p != 0.0)
		p = 1.0f / p;
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to value being -ve for sqrt *asadr */
void _vuELENG(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Fs_].i.z);
	if(p >= 0){
		p = sqrt(p);
	}
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to divisor being = 0 *asadr */
void _vuERLENG(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].i.x) * vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Fs_].i.y) * vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Fs_].i.z) * vuDouble(VU->VF[_Fs_].i.z);
	if (p >= 0) {
		p = sqrt(p);
		if (p != 0) {
			p = 1.0f / p;
		}
	}
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to divisor being = 0 *asadr */
void _vuEATANxy(VURegs * VU) {
	float p = 0;
	if(vuDouble(VU->VF[_Fs_].i.x) != 0) {
		p = atan2(vuDouble(VU->VF[_Fs_].i.y), vuDouble(VU->VF[_Fs_].i.x));
	}
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to divisor being = 0 *asadr */
void _vuEATANxz(VURegs * VU) {
	float p = 0;
	if(vuDouble(VU->VF[_Fs_].i.x) != 0) {
		p = atan2(vuDouble(VU->VF[_Fs_].i.z), vuDouble(VU->VF[_Fs_].i.x));
	}
	VU->p.F = p;
}

void _vuESUM(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].i.x) + vuDouble(VU->VF[_Fs_].i.y) + vuDouble(VU->VF[_Fs_].i.z) + vuDouble(VU->VF[_Fs_].i.w);
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to divisor being = 0 *asadr */
void _vuERCPR(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].UL[_Fsf_]);
	if (p != 0){
		p = 1.0 / p;
	}
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to Value being -ve for sqrt *asadr */
void _vuESQRT(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].UL[_Fsf_]);
	if (p >= 0){
		p = sqrt(p);
	}
	VU->p.F = p;
}

/* Fixed. Could have caused crash due to divisor being = 0 *asadr */
void _vuERSQRT(VURegs * VU) {
	float p = vuDouble(VU->VF[_Fs_].UL[_Fsf_]);
	if (p >= 0) {
		p = sqrt(p);
		if (p) {
			p = 1.0f / p;
		}
	}
	VU->p.F = p;
}

void _vuESIN(VURegs * VU) {
	float p = sin(vuDouble(VU->VF[_Fs_].UL[_Fsf_]));
	VU->p.F = p;
}

void _vuEATAN(VURegs * VU) {
	float p = atan(vuDouble(VU->VF[_Fs_].UL[_Fsf_]));
	VU->p.F = p;
}

void _vuEEXP(VURegs * VU) {
	float p = exp(-(vuDouble(VU->VF[_Fs_].UL[_Fsf_])));
	VU->p.F = p;
}

void _vuXITOP(VURegs * VU) {
	if (_It_ == 0) return;
	VU->VI[_It_].US[0] = VU->vifRegs->itop;
}

void _vuXGKICK(VURegs * VU)
{
	// flush all pipelines first (in the right order)
	_vuFlushAll(VU);

	u8* data = ((u8*)VU->Mem + ((VU->VI[_Is_].US[0]*16) & 0x3fff));
	u32 size;
	size = GetMTGS().PrepDataPacket( GIF_PATH_1, data, (0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff)) >> 4, false);
	u8* pmem = GetMTGS().GetDataPacketPtr();

	if((size << 4) > (u32)(0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff)))
	{
		//DevCon.Warning("addr + Size = 0x%x, transferring %x then doing %x", ((VU->VI[_Is_].US[0]*16) & 0x3fff) + (size << 4), (0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff)) >> 4, size - (0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff) >> 4));
		memcpy_aligned(pmem, (u8*)VU->Mem+((VU->VI[_Is_].US[0]*16) & 0x3fff), 0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff));
		size -= (0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff)) >> 4;
		//DevCon.Warning("Size left %x", size);
		pmem += 0x4000-((VU->VI[_Is_].US[0]*16) & 0x3fff);
		memcpy_aligned(pmem, (u8*)VU->Mem, size<<4);
	}
	else {
		memcpy_aligned(pmem, (u8*)VU->Mem+((VU->VI[_Is_].US[0]*16) & 0x3fff), size<<4);
	}
	GetMTGS().SendDataPacket();
}

void _vuXTOP(VURegs * VU) {
	if(_It_ == 0) return;
	VU->VI[_It_].US[0] = (u16)VU->vifRegs->top;
}

#define GET_VF0_FLAG(reg) (((reg)==0)?(1<<REG_VF0_FLAG):0)

#define VUREGS_FDFSI(OP, ACC) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Fd_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (1 << REG_I)|(ACC?(1<<REG_ACC_FLAG):0)|GET_VF0_FLAG(_Fs_); \
}

#define VUREGS_FDFSQ(OP, ACC) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Fd_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (1 << REG_Q)|(ACC?(1<<REG_ACC_FLAG):0)|GET_VF0_FLAG(_Fs_); \
}

#define VUREGS_FDFSFT(OP, ACC) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Fd_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = _Ft_; \
	VUregsn->VFr1xyzw= _XYZW; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (ACC?(1<<REG_ACC_FLAG):0)|GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_); \
}

#define VUREGS_FDFSFTxyzw(OP, xyzw, ACC) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Fd_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = _Ft_; \
	VUregsn->VFr1xyzw= xyzw; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (ACC?(1<<REG_ACC_FLAG):0)|GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_); \
}

#define VUREGS_FDFSFTx(OP, ACC) VUREGS_FDFSFTxyzw(OP, 8, ACC)
#define VUREGS_FDFSFTy(OP, ACC) VUREGS_FDFSFTxyzw(OP, 4, ACC)
#define VUREGS_FDFSFTz(OP, ACC) VUREGS_FDFSFTxyzw(OP, 2, ACC)
#define VUREGS_FDFSFTw(OP, ACC) VUREGS_FDFSFTxyzw(OP, 1, ACC)


#define VUREGS_ACCFSI(OP, readacc) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFwxyzw= _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = (1<<REG_ACC_FLAG); \
	VUregsn->VIread  = (1 << REG_I)|GET_VF0_FLAG(_Fs_)|((readacc||_XYZW!=15)?(1<<REG_ACC_FLAG):0); \
}

#define VUREGS_ACCFSQ(OP, readacc) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFwxyzw= _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = (1<<REG_ACC_FLAG); \
	VUregsn->VIread  = (1 << REG_Q)|GET_VF0_FLAG(_Fs_)|((readacc||_XYZW!=15)?(1<<REG_ACC_FLAG):0); \
}

#define VUREGS_ACCFSFT(OP, readacc) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFwxyzw= _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = _Ft_; \
	VUregsn->VFr1xyzw= _XYZW; \
	VUregsn->VIwrite = (1<<REG_ACC_FLAG); \
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_)|((readacc||_XYZW!=15)?(1<<REG_ACC_FLAG):0); \
}

#define VUREGS_ACCFSFTxyzw(OP, xyzw, readacc) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFwxyzw= _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = _Ft_; \
	VUregsn->VFr1xyzw= xyzw; \
	VUregsn->VIwrite = (1<<REG_ACC_FLAG); \
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_)|((readacc||_XYZW!=15)?(1<<REG_ACC_FLAG):0); \
}

#define VUREGS_ACCFSFTx(OP, readacc) VUREGS_ACCFSFTxyzw(OP, 8, readacc)
#define VUREGS_ACCFSFTy(OP, readacc) VUREGS_ACCFSFTxyzw(OP, 4, readacc)
#define VUREGS_ACCFSFTz(OP, readacc) VUREGS_ACCFSFTxyzw(OP, 2, readacc)
#define VUREGS_ACCFSFTw(OP, readacc) VUREGS_ACCFSFTxyzw(OP, 1, readacc)

#define VUREGS_FTFS(OP) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Ft_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
	VUregsn->VFr1xyzw = 0xff; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (_Ft_ ? GET_VF0_FLAG(_Fs_) : 0); \
}

#define VUREGS_IDISIT(OP) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_IALU; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFread0 = 0; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = 1 << _Id_; \
	VUregsn->VIread  = (1 << _Is_) | (1 << _It_); \
	VUregsn->cycles  = 0; \
}

#define VUREGS_ITIS(OP) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_IALU; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFread0 = 0; \
	VUregsn->VFread1 = 0; \
	VUregsn->VIwrite = 1 << _It_; \
	VUregsn->VIread  = 1 << _Is_; \
	VUregsn->cycles  = 0; \
}

#define VUREGS_PFS_xyzw(OP, _cycles) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_EFU; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = 0; \
    VUregsn->VIwrite = 1 << REG_P; \
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_); \
	VUregsn->cycles  = _cycles; \
}

#define VUREGS_PFS_fsf(OP, _cycles) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_EFU; \
	VUregsn->VFwrite = 0; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= 1 << (3-_Fsf_); \
	VUregsn->VFread1 = 0; \
    VUregsn->VIwrite = 1 << REG_P; \
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_); \
	VUregsn->cycles  = _cycles; \
}

VUREGS_FTFS(ABS);

VUREGS_FDFSFT(ADD, 0);
VUREGS_FDFSI(ADDi, 0);
VUREGS_FDFSQ(ADDq, 0);
VUREGS_FDFSFTx(ADDx, 0);
VUREGS_FDFSFTy(ADDy, 0);
VUREGS_FDFSFTz(ADDz, 0);
VUREGS_FDFSFTw(ADDw, 0);

VUREGS_ACCFSFT(ADDA, 0);
VUREGS_ACCFSI(ADDAi, 0);
VUREGS_ACCFSQ(ADDAq, 0);
VUREGS_ACCFSFTx(ADDAx, 0);
VUREGS_ACCFSFTy(ADDAy, 0);
VUREGS_ACCFSFTz(ADDAz, 0);
VUREGS_ACCFSFTw(ADDAw, 0);

VUREGS_FDFSFT(SUB, 0);
VUREGS_FDFSI(SUBi, 0);
VUREGS_FDFSQ(SUBq, 0);
VUREGS_FDFSFTx(SUBx, 0);
VUREGS_FDFSFTy(SUBy, 0);
VUREGS_FDFSFTz(SUBz, 0);
VUREGS_FDFSFTw(SUBw, 0);

VUREGS_ACCFSFT(SUBA, 0);
VUREGS_ACCFSI(SUBAi, 0);
VUREGS_ACCFSQ(SUBAq, 0);
VUREGS_ACCFSFTx(SUBAx, 0);
VUREGS_ACCFSFTy(SUBAy, 0);
VUREGS_ACCFSFTz(SUBAz, 0);
VUREGS_ACCFSFTw(SUBAw, 0);

#define VUREGS_FDFSFTxyzw_MUL(OP, ACC, xyzw) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	if( _Ft_ == 0 && xyzw > 1 && _XYZW == 0xf ) { /* resetting to 0 */ \
		VUregsn->pipe = VUPIPE_FMAC; \
		VUregsn->VFwrite = ACC?0:_Fd_; \
		VUregsn->VFwxyzw = _XYZW; \
		VUregsn->VFread0 = 0; \
		VUregsn->VFr0xyzw= _XYZW; \
		VUregsn->VFread1 = 0; \
		VUregsn->VFr1xyzw= xyzw; \
		VUregsn->VIwrite = (ACC?(1<<REG_ACC_FLAG):0); \
		VUregsn->VIread  = (ACC&&(_XYZW!=15))?(1<<REG_ACC_FLAG):0; \
	} \
	else { \
		VUregsn->pipe = VUPIPE_FMAC; \
		VUregsn->VFwrite = ACC?0:_Fd_; \
		VUregsn->VFwxyzw = _XYZW; \
		VUregsn->VFread0 = _Fs_; \
		VUregsn->VFr0xyzw= _XYZW; \
		VUregsn->VFread1 = _Ft_; \
		VUregsn->VFr1xyzw= xyzw; \
		VUregsn->VIwrite = (ACC?(1<<REG_ACC_FLAG):0); \
		VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|((ACC&&(_XYZW!=15))?(1<<REG_ACC_FLAG):0); \
	} \
}

VUREGS_FDFSFT(MUL, 0);
VUREGS_FDFSI(MULi, 0);
VUREGS_FDFSQ(MULq, 0);
VUREGS_FDFSFTxyzw_MUL(MULx, 0, 8);
VUREGS_FDFSFTxyzw_MUL(MULy, 0, 4);
VUREGS_FDFSFTxyzw_MUL(MULz, 0, 2);
VUREGS_FDFSFTxyzw_MUL(MULw, 0, 1);

VUREGS_ACCFSFT(MULA, 0);
VUREGS_ACCFSI(MULAi, 0);
VUREGS_ACCFSQ(MULAq, 0);
VUREGS_FDFSFTxyzw_MUL(MULAx, 1, 8);
VUREGS_FDFSFTxyzw_MUL(MULAy, 1, 4);
VUREGS_FDFSFTxyzw_MUL(MULAz, 1, 2);
VUREGS_FDFSFTxyzw_MUL(MULAw, 1, 1);

VUREGS_FDFSFT(MADD, 1);
VUREGS_FDFSI(MADDi, 1);
VUREGS_FDFSQ(MADDq, 1);

#define VUREGS_FDFSFT_0_xyzw(OP, xyzw) \
void _vuRegs##OP(VURegs * VU, _VURegsNum *VUregsn) { \
	VUregsn->pipe = VUPIPE_FMAC; \
	VUregsn->VFwrite = _Fd_; \
	VUregsn->VFwxyzw = _XYZW; \
	VUregsn->VFread0 = _Fs_; \
	VUregsn->VFr0xyzw= _XYZW; \
	VUregsn->VFread1 = _Ft_; \
	VUregsn->VFr1xyzw= xyzw; \
	VUregsn->VIwrite = 0; \
	VUregsn->VIread  = (1<<REG_ACC_FLAG)|(_Ft_ ? GET_VF0_FLAG(_Fs_) : 0); \
}

VUREGS_FDFSFT_0_xyzw(MADDx, 8);
VUREGS_FDFSFT_0_xyzw(MADDy, 4);
VUREGS_FDFSFT_0_xyzw(MADDz, 2);

void _vuRegsMADDw(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
	VUregsn->VFwrite = _Fd_;
	VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= _XYZW;
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 1;
	VUregsn->VIwrite = 0;
	VUregsn->VIread  = (1<<REG_ACC_FLAG)|GET_VF0_FLAG(_Fs_);
}

VUREGS_ACCFSFT(MADDA, 1);
VUREGS_ACCFSI(MADDAi, 1);
VUREGS_ACCFSQ(MADDAq, 1);
VUREGS_ACCFSFTx(MADDAx, 1);
VUREGS_ACCFSFTy(MADDAy, 1);
VUREGS_ACCFSFTz(MADDAz, 1);
VUREGS_ACCFSFTw(MADDAw, 1);

VUREGS_FDFSFT(MSUB, 1);
VUREGS_FDFSI(MSUBi, 1);
VUREGS_FDFSQ(MSUBq, 1);
VUREGS_FDFSFTx(MSUBx, 1);
VUREGS_FDFSFTy(MSUBy, 1);
VUREGS_FDFSFTz(MSUBz, 1);
VUREGS_FDFSFTw(MSUBw, 1);

VUREGS_ACCFSFT(MSUBA, 1);
VUREGS_ACCFSI(MSUBAi, 1);
VUREGS_ACCFSQ(MSUBAq, 1);
VUREGS_ACCFSFTx(MSUBAx, 1);
VUREGS_ACCFSFTy(MSUBAy, 1);
VUREGS_ACCFSFTz(MSUBAz, 1);
VUREGS_ACCFSFTw(MSUBAw, 1);

VUREGS_FDFSFT(MAX, 0);
VUREGS_FDFSI(MAXi, 0);
VUREGS_FDFSFTx(MAXx_, 0);
VUREGS_FDFSFTy(MAXy_, 0);
VUREGS_FDFSFTz(MAXz_, 0);
VUREGS_FDFSFTw(MAXw_, 0);

void _vuRegsMAXx(VURegs * VU, _VURegsNum *VUregsn) {
	_vuRegsMAXx_(VU, VUregsn);
	if( _Fs_ == 0 && _Ft_ == 0 ) VUregsn->VIread &= ~(1<<REG_VF0_FLAG);
}
void _vuRegsMAXy(VURegs * VU, _VURegsNum *VUregsn) {
	_vuRegsMAXy_(VU, VUregsn);
	if( _Fs_ == 0 && _Ft_ == 0 ) VUregsn->VIread &= ~(1<<REG_VF0_FLAG);
}
void _vuRegsMAXz(VURegs * VU, _VURegsNum *VUregsn) {
	_vuRegsMAXz_(VU, VUregsn);
	if( _Fs_ == 0 && _Ft_ == 0 ) VUregsn->VIread &= ~(1<<REG_VF0_FLAG);
}
void _vuRegsMAXw(VURegs * VU, _VURegsNum *VUregsn) {
	_vuRegsMAXw_(VU, VUregsn);
	if( _Fs_ == 0 && _Ft_ == 0 ) VUregsn->VIread &= ~(1<<REG_VF0_FLAG);
}

VUREGS_FDFSFT(MINI, 0);
VUREGS_FDFSI(MINIi, 0);
VUREGS_FDFSFTx(MINIx, 0);
VUREGS_FDFSFTy(MINIy, 0);
VUREGS_FDFSFTz(MINIz, 0);
VUREGS_FDFSFTw(MINIw, 0);

void _vuRegsOPMULA(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
	VUregsn->VFwrite = 0;
	VUregsn->VFwxyzw= 0xE;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 0xE;
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 0xE;
	VUregsn->VIwrite = 1<<REG_ACC_FLAG;
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_)|(1<<REG_ACC_FLAG);
}

void _vuRegsOPMSUB(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
	VUregsn->VFwrite = _Fd_;
	VUregsn->VFwxyzw= 0xE;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 0xE;
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 0xE;
	VUregsn->VIwrite = 0;
	VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_)|(1<<REG_ACC_FLAG);
}

void _vuRegsNOP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_NONE;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 0;
}

VUREGS_FTFS(FTOI0);
VUREGS_FTFS(FTOI4);
VUREGS_FTFS(FTOI12);
VUREGS_FTFS(FTOI15);
VUREGS_FTFS(ITOF0);
VUREGS_FTFS(ITOF4);
VUREGS_FTFS(ITOF12);
VUREGS_FTFS(ITOF15);

void _vuRegsCLIP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 0xE;
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 0x1;
    VUregsn->VIwrite = 1 << REG_CLIP_FLAG;
    VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_)|(1 << REG_CLIP_FLAG);
}

/******************************/
/*   VU Lower instructions    */
/******************************/

void _vuRegsDIV(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FDIV;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 1 << (3-_Fsf_);
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 1 << (3-_Ftf_);
    VUregsn->VIwrite = 1 << REG_Q;
    VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_);
	VUregsn->cycles  = 6;
}

void _vuRegsSQRT(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FDIV;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = 0;
	VUregsn->VFr0xyzw = 0;
    VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw = 1 << (3-_Ftf_);
    VUregsn->VIwrite = 1 << REG_Q;
    VUregsn->VIread  = GET_VF0_FLAG(_Ft_);
	VUregsn->cycles  = 6;
}

void _vuRegsRSQRT(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FDIV;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 1 << (3-_Fsf_);
	VUregsn->VFread1 = _Ft_;
	VUregsn->VFr1xyzw= 1 << (3-_Ftf_);
    VUregsn->VIwrite = 1 << REG_Q;
    VUregsn->VIread  = GET_VF0_FLAG(_Fs_)|GET_VF0_FLAG(_Ft_);
	VUregsn->cycles  = 12;
}

VUREGS_ITIS(IADDI);
VUREGS_ITIS(IADDIU);
VUREGS_IDISIT(IADD);
VUREGS_IDISIT(IAND);
VUREGS_IDISIT(IOR);
VUREGS_IDISIT(ISUB);
VUREGS_ITIS(ISUBIU);

VUREGS_FTFS(MOVE);

void _vuRegsMFIR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsMTIR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= 1 << (3-_Fsf_);
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = GET_VF0_FLAG(_Fs_);
}

void _vuRegsMR32(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
	VUregsn->VFwrite = _Ft_;
	VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= (_XYZW >> 1) | ((_XYZW << 3) & 0xf);  //rotate
	VUregsn->VFread1 = 0;
	VUregsn->VFr1xyzw = 0xff;
	VUregsn->VIwrite = 0;
	VUregsn->VIread  = (_Ft_ ? GET_VF0_FLAG(_Fs_) : 0);
}

void _vuRegsLQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsLQD(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _Is_;
    VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsLQI(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _Is_;
    VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsSQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= _XYZW;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 1 << _It_;
}

void _vuRegsSQD(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= _XYZW;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << _It_;
}

void _vuRegsSQI(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= _XYZW;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << _It_;
}

void _vuRegsILW(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << _Is_;
	VUregsn->cycles  = 3;
}

void _vuRegsISW(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = (1 << _Is_) | (1 << _It_);
}

void _vuRegsILWR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = (1 << _It_);
    VUregsn->VIread  = (1 << _Is_);
	VUregsn->cycles  = 3;
}

void _vuRegsISWR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = (1 << _Is_) | (1 << _It_);
}

void _vuRegsRINIT(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= 1 << (3-_Fsf_);
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << REG_R;
    VUregsn->VIread  = GET_VF0_FLAG(_Fs_);
}

void _vuRegsRGET(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 1 << REG_R;
}

void _vuRegsRNEXT(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
	VUregsn->VFread0 = 0;
	VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << REG_R;
    VUregsn->VIread  = 1 << REG_R;
}

void _vuRegsRXOR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
	VUregsn->VFread0 = _Fs_;
    VUregsn->VFr0xyzw= 1 << (3-_Fsf_);
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << REG_R;
    VUregsn->VIread  = (1 << REG_R)|GET_VF0_FLAG(_Fs_);
}

void _vuRegsWAITQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FDIV;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 0;
}

void _vuRegsFSAND(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << REG_STATUS_FLAG;
}

void _vuRegsFSEQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << REG_STATUS_FLAG;
}

void _vuRegsFSOR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << REG_STATUS_FLAG;
}

void _vuRegsFSSET(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << REG_STATUS_FLAG;
	VUregsn->VIread  = 0;
}

void _vuRegsFMAND(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = (1 << REG_MAC_FLAG) | (1 << _Is_);
}

void _vuRegsFMEQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = (1 << REG_MAC_FLAG) | (1 << _Is_);
}

void _vuRegsFMOR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = (1 << REG_MAC_FLAG) | (1 << _Is_);
}

void _vuRegsFCAND(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << 1;
    VUregsn->VIread  = 1 << REG_CLIP_FLAG;
}

void _vuRegsFCEQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << 1;
    VUregsn->VIread  = 1 << REG_CLIP_FLAG;
}

void _vuRegsFCOR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << 1;
    VUregsn->VIread  = 1 << REG_CLIP_FLAG;
}

void _vuRegsFCSET(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << REG_CLIP_FLAG;
    VUregsn->VIread  = 0;
}

void _vuRegsFCGET(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 1 << REG_CLIP_FLAG;
}

void _vuRegsIBEQ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = (1 << _Is_) | (1 << _It_);
}

void _vuRegsIBGEZ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsIBGTZ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsIBLEZ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsIBLTZ(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsIBNE(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = (1 << _Is_) | (1 << _It_);
}

void _vuRegsB(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 0;
}

void _vuRegsBAL(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 0;
}

void _vuRegsJR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsJALR(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_BRANCH;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 1 << _It_;
	VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsMFP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_FMAC;
    VUregsn->VFwrite = _Ft_;
    VUregsn->VFwxyzw = _XYZW;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
	VUregsn->VIwrite = 0;
	VUregsn->VIread  = 1 << REG_P;
}

void _vuRegsWAITP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_EFU;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 0;
}

VUREGS_PFS_xyzw(ESADD, 10);
VUREGS_PFS_xyzw(ERSADD, 17);
VUREGS_PFS_xyzw(ELENG, 17);
VUREGS_PFS_xyzw(ERLENG, 23);
VUREGS_PFS_xyzw(EATANxy, 53);
VUREGS_PFS_xyzw(EATANxz, 53);
VUREGS_PFS_xyzw(ESUM, 11);
VUREGS_PFS_fsf(ERCPR, 11);
VUREGS_PFS_fsf(ESQRT, 11);
VUREGS_PFS_fsf(ERSQRT, 17);
VUREGS_PFS_fsf(ESIN, 28);
VUREGS_PFS_fsf(EATAN, 53);
VUREGS_PFS_fsf(EEXP, 43);

void _vuRegsXITOP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 0;
	VUregsn->cycles  = 0;
}

void _vuRegsXGKICK(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_XGKICK;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 0;
    VUregsn->VIread  = 1 << _Is_;
}

void _vuRegsXTOP(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->pipe = VUPIPE_IALU;
    VUregsn->VFwrite = 0;
    VUregsn->VFread0 = 0;
    VUregsn->VFread1 = 0;
    VUregsn->VIwrite = 1 << _It_;
    VUregsn->VIread  = 0;
	VUregsn->cycles  = 0;
}
