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

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define PLUGINtypedefs // for GSgifTransfer1

#include "Common.h"
#include "GS.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"
#include "VUmicro.h"
#include "VUflags.h"
#include "iVUmicro.h"
#include "iVU0micro.h"
#include "iVU1micro.h"
#include "iVUops.h"
#include "iVUzerorec.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

extern _GSgifTransfer1    GSgifTransfer1;

int g_VuNanHandling = 0; // for now enable all the time

int vucycle;
int vucycleold;
_vuopinfo *cinfo = NULL;

//Lower/Upper instructions can use that..
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

PCSX2_ALIGNED16(float recMult_float_to_int4[4]) = { 16.0, 16.0, 16.0, 16.0 };
PCSX2_ALIGNED16(float recMult_float_to_int12[4]) = { 4096.0, 4096.0, 4096.0, 4096.0 };
PCSX2_ALIGNED16(float recMult_float_to_int15[4]) = { 32768.0, 32768.0, 32768.0, 32768.0 };

PCSX2_ALIGNED16(float recMult_int_to_float4[4]) = { 0.0625f, 0.0625f, 0.0625f, 0.0625f };
PCSX2_ALIGNED16(float recMult_int_to_float12[4]) = { 0.000244140625, 0.000244140625, 0.000244140625, 0.000244140625 };
PCSX2_ALIGNED16(float recMult_int_to_float15[4]) = { 0.000030517578125, 0.000030517578125, 0.000030517578125, 0.000030517578125 };
static s32 bpc;
_VURegsNum* g_VUregs = NULL;
u8 g_MACFlagTransform[256] = {0}; // used to flip xyzw bits

static int SSEmovMask[ 16 ][ 4 ] =
{
{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
{ 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF },
{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 },
{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 },
{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 },
{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000 },
{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF }
};

#define VU_SWAPSRC 0xf090 // don't touch

#define _vuIsRegSwappedWithTemp() (VU_SWAPSRC & (1<<_X_Y_Z_W))

// use for allocating vi regs
#define ALLOCTEMPX86(mode) _allocX86reg(-1, X86TYPE_TEMP, 0, ((info&PROCESS_VU_SUPER)?0:MODE_NOFRAME)|mode)
#define ALLOCVI(vi, mode) _allocX86reg(-1, X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), vi, ((info&PROCESS_VU_SUPER)?0:MODE_NOFRAME)|mode)
#define ADD_VI_NEEDED(vi) _addNeededX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), vi);

// 1 - src, 0 - dest				   wzyx
void VU_MERGE0(int dest, int src) { // 0000
}
void VU_MERGE1(int dest, int src) { // 1000
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc4);
}
void VU_MERGE2(int dest, int src) { // 0100
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x64);
}
void VU_MERGE3(int dest, int src) { // 1100
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE4(int dest, int src) { // 0010s
	SSE_MOVSS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xe4);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE5(int dest, int src) { // 1010
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xd8);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd8);
}
void VU_MERGE6(int dest, int src) { // 0110
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x9c);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x78);
}
void VU_MERGE7(int dest, int src) { // 1110s
	SSE_MOVSS_XMM_to_XMM(src, dest);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE8(int dest, int src) { // 0001
	SSE_MOVSS_XMM_to_XMM(dest, src);
}
void VU_MERGE9(int dest, int src) { // 1001
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc9);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd2);
}
void VU_MERGE10(int dest, int src) { // 0101
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x8d);
	SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x72);
}
void VU_MERGE11(int dest, int src) { // 1101
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE12(int dest, int src) { // 0011s
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xe4);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE13(int dest, int src) { // 1011s
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0x64);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
}
void VU_MERGE14(int dest, int src) { // 0111s
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xc4);
	SSE_MOVAPS_XMM_to_XMM(dest, src);
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
/*
#define VU_MERGE_REGS(dest, src) { \
	if( dest != src ) s_VuMerge[_X_Y_Z_W](dest, src); \
} \

#define VU_MERGE_REGS_CUSTOM(dest, src, xyzw) { \
	if( dest != src ) s_VuMerge[xyzw](dest, src); \
} \
*/
void VU_MERGE_REGS_CUSTOM(int dest, int src, int xyzw)
{
	xyzw &= 0xf;

	if(dest != src && xyzw != 0)
	{
		if(cpucaps.hasStreamingSIMD4Extensions)
		{
			xyzw = ((xyzw & 1) << 3) | ((xyzw & 2) << 1) | ((xyzw & 4) >> 1) | ((xyzw & 8) >> 3); 

			SSE4_BLENDPS_XMM_to_XMM(dest, src, xyzw);
		}
		else
		{
			s_VuMerge[xyzw](dest, src); 
		}
	}
}

#define VU_MERGE_REGS(dest, src) { \
	VU_MERGE_REGS_CUSTOM(dest, src, _X_Y_Z_W); \
} \

void _unpackVF_xyzw(int dstreg, int srcreg, int xyzw)
{
	// don't use pshufd
	if( dstreg == srcreg || !cpucaps.hasStreamingSIMD3Extensions) {
		if( dstreg != srcreg ) SSE_MOVAPS_XMM_to_XMM(dstreg, srcreg);
		switch (xyzw) {
			case 0: SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0x00); break;
			case 1: SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0x55); break;
			case 2: SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0xaa); break;
			case 3: SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0xff); break;
		}
	}
	else {
/*
		switch (xyzw) {
			case 0:
				SSE3_MOVSLDUP_XMM_to_XMM(dstreg, srcreg);
				SSE_MOVLHPS_XMM_to_XMM(dstreg, dstreg);
				break;
			case 1:
				SSE3_MOVSHDUP_XMM_to_XMM(dstreg, srcreg);
				SSE_MOVLHPS_XMM_to_XMM(dstreg, dstreg);
				break;
			case 2:
				SSE3_MOVSLDUP_XMM_to_XMM(dstreg, srcreg);
				SSE_MOVHLPS_XMM_to_XMM(dstreg, dstreg);
				break;
			case 3:
				SSE3_MOVSHDUP_XMM_to_XMM(dstreg, srcreg);
				SSE_MOVHLPS_XMM_to_XMM(dstreg, dstreg);
				break;
		}
*/
		switch (xyzw) {
			case 0:
				SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x00);
				break;
			case 1:
				SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x55);
				break;
			case 2:
				SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xaa);
				break;
			case 3:
				SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xff);
				break;
		}
	}
}

void _unpackVFSS_xyzw(int dstreg, int srcreg, int xyzw)
{
	if( cpucaps.hasStreamingSIMD4Extensions ) {
		switch (xyzw) {
			case 0: SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(0, 0, 0)); break;
			case 1: SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(1, 0, 0)); break;
			case 2: SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(2, 0, 0)); break;
			case 3: SSE4_INSERTPS_XMM_to_XMM(dstreg, srcreg, _MM_MK_INSERTPS_NDX(3, 0, 0)); break;
		}
	}
	else {
		switch (xyzw) {
			case 0:
				if( dstreg != srcreg ) SSE_MOVAPS_XMM_to_XMM(dstreg, srcreg);
				break;
			case 1:
				if( cpucaps.hasStreamingSIMD3Extensions ) SSE3_MOVSHDUP_XMM_to_XMM(dstreg, srcreg);
				else {
					if( dstreg != srcreg ) SSE_MOVAPS_XMM_to_XMM(dstreg, srcreg);
					SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0x55);
				}
				break;
			case 2:
				SSE_MOVHLPS_XMM_to_XMM(dstreg, srcreg);
				break;
			case 3:
				if( cpucaps.hasStreamingSIMD3Extensions && dstreg != srcreg ) {
					SSE3_MOVSHDUP_XMM_to_XMM(dstreg, srcreg);
					SSE_MOVHLPS_XMM_to_XMM(dstreg, dstreg);
				}
				else {
					if( dstreg != srcreg ) SSE_MOVAPS_XMM_to_XMM(dstreg, srcreg);
					SSE_SHUFPS_XMM_to_XMM(dstreg, dstreg, 0xff);
				}
				break;
		}
	}
}

void _vuFlipRegSS(VURegs * VU, int reg)
{
	assert( _XYZW_SS );
	if( _Y ) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xe1);
	else if( _Z ) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xc6);
	else if( _W ) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x27);
}

void _vuMoveSS(VURegs * VU, int dstreg, int srcreg)
{
	assert( _XYZW_SS );
	if( _Y ) _unpackVFSS_xyzw(dstreg, srcreg, 1);
	else if( _Z ) _unpackVFSS_xyzw(dstreg, srcreg, 2);
	else if( _W ) _unpackVFSS_xyzw(dstreg, srcreg, 3);
	else _unpackVFSS_xyzw(dstreg, srcreg, 0);
}

void _recvuFMACflush(VURegs * VU) {
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;

		if ((vucycle - VU->fmac[i].sCycle) >= VU->fmac[i].Cycle) {
#ifdef VUM_LOG
//			if (Log) { VUM_LOG("flushing FMAC pipe[%d]\n", i); }
#endif
			VU->fmac[i].enable = 0;
		}
	}
}

void _recvuFDIVflush(VURegs * VU) {
	if (VU->fdiv.enable == 0) return;

	if ((vucycle - VU->fdiv.sCycle) >= VU->fdiv.Cycle) {
//		SysPrintf("flushing FDIV pipe\n");
		VU->fdiv.enable = 0;
	}
}

void _recvuEFUflush(VURegs * VU) {
	if (VU->efu.enable == 0) return;

	if ((vucycle - VU->efu.sCycle) >= VU->efu.Cycle) {
//		SysPrintf("flushing FDIV pipe\n");
		VU->efu.enable = 0;
	}
}

void _recvuTestPipes(VURegs * VU) {
	_recvuFMACflush(VU);
	_recvuFDIVflush(VU);
	_recvuEFUflush(VU);
}

void _recvuFMACTestStall(VURegs * VU, int reg, int xyzw) {
	int cycle;
	int i;
	u32 mask = 0;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;
		if (VU->fmac[i].reg == reg &&
			(VU->fmac[i].xyzw & xyzw)) break;
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
	_recvuTestPipes(VU);
}

void _recvuFMACAdd(VURegs * VU, int reg, int xyzw) {
	int i;

	/* find a free fmac pipe */
	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 1) continue;
		break;
	}
	if (i==8) {
		SysPrintf("*PCSX2*: error , out of fmacs\n");
	}

#ifdef VUM_LOG
//	if (Log) { VUM_LOG("adding FMAC pipe[%d]; reg %d\n", i, reg); }
#endif
	VU->fmac[i].enable = 1;
	VU->fmac[i].sCycle = vucycle;
	VU->fmac[i].Cycle = 3;
	VU->fmac[i].xyzw = xyzw; 
	VU->fmac[i].reg = reg; 
}

void _recvuFDIVAdd(VURegs * VU, int cycles) {
//	SysPrintf("adding FDIV pipe\n");
	VU->fdiv.enable = 1;
	VU->fdiv.sCycle = vucycle;
	VU->fdiv.Cycle  = cycles;
}

void _recvuEFUAdd(VURegs * VU, int cycles) {
//	SysPrintf("adding EFU pipe\n");
	VU->efu.enable = 1;
	VU->efu.sCycle = vucycle;
	VU->efu.Cycle  = cycles;
}

void _recvuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {

	if( VUregsn->VFread0 && (VUregsn->VFread0 == VUregsn->VFread1) ) {
		_recvuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw|VUregsn->VFr1xyzw);
	}
	else {
		if (VUregsn->VFread0) {
			_recvuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw);
		}
		if (VUregsn->VFread1) {
			_recvuFMACTestStall(VU, VUregsn->VFread1, VUregsn->VFr1xyzw);
		}
	}
}

void _recvuAddFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VFwrite) {
		_recvuFMACAdd(VU, VUregsn->VFwrite, VUregsn->VFwxyzw);
	} else
	if (VUregsn->VIwrite & (1 << REG_CLIP_FLAG)) {
//		SysPrintf("REG_CLIP_FLAG pipe\n");
		_recvuFMACAdd(VU, -REG_CLIP_FLAG, 0);
	} else {
		_recvuFMACAdd(VU, 0, 0);
	}
}

void _recvuFlushFDIV(VURegs * VU) {
	int cycle;

	if (VU->fdiv.enable == 0) return;

	cycle = VU->fdiv.Cycle - (vucycle - VU->fdiv.sCycle);
//	SysPrintf("waiting FDIV pipe %d\n", cycle);
	VU->fdiv.enable = 0;
	vucycle+= cycle;
}

void _recvuFlushEFU(VURegs * VU) {
	int cycle;

	if (VU->efu.enable == 0) return;

	cycle = VU->efu.Cycle - (vucycle - VU->efu.sCycle);
//	SysPrintf("waiting FDIV pipe %d\n", cycle);
	VU->efu.enable = 0;
	vucycle+= cycle;
}

void _recvuTestFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	_recvuFlushFDIV(VU);
}

void _recvuTestEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
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
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn); break;
	}
}

void _recvuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _recvuTestFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuTestEFUStalls(VU, VUregsn); break;
	}
}

void _recvuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn); break;
	}
}

void _recvuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _recvuAddFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuAddEFUStalls(VU, VUregsn); break;
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
	if (VU == &VU1) {
		VU1regs_UPPER_OPCODE[VU->code & 0x3f](uregs);
	} else {
		VU0regs_UPPER_OPCODE[VU->code & 0x3f](uregs);
	}

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

	if (uregs->VIread & (1 << REG_Q)) {
		info->q |= 2;
	}

	if (uregs->VIread & (1 << REG_P)) {
		assert( VU == &VU1 );
		info->p |= 2;
	}

	// check upper flags
	if (ptr[1] & 0x80000000) { // I flag
		info->cycle = vucycle;
		memset(lregs, 0, sizeof(lregs));
	} else {

		VU->code = ptr[0]; 
		if (VU == &VU1) {
			VU1regs_LOWER_OPCODE[VU->code >> 25](lregs);
		} else {
			VU0regs_LOWER_OPCODE[VU->code >> 25](lregs);
		}

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
		if (lregs->VIread & (1 << REG_MAC_FLAG)) info->macflag|= VUOP_READ;

		if (lregs->VIwrite & (1 << REG_STATUS_FLAG)) info->statusflag|= VUOP_WRITE;
		if (lregs->VIwrite & (1 << REG_MAC_FLAG)) info->macflag|= VUOP_WRITE;

		if (lregs->VIread & (1 << REG_Q)) {
			info->q |= 2;
		}

		if (lregs->VIread & (1 << REG_P)) {
			assert( VU == &VU1 );
			info->p |= 2;
		}

		_recvuAddLowerStalls(VU, lregs);
	}
	_recvuAddUpperStalls(VU, uregs);

	_recvuTestPipes(VU);

	vucycle++;
}

// Analyze an op - first pass
void _vurecAnalyzeOp(VURegs *VU, _vuopinfo *info) {
	_VURegsNum lregs;
	_VURegsNum uregs;
	int *ptr; 

//	SysPrintf("_vurecAnalyzeOp %x; %p\n", pc, info);
	ptr = (int*)&VU->Micro[pc]; 
	pc += 8; 

/*	SysPrintf("_vurecAnalyzeOp Upper: %s\n", disVU1MicroUF( ptr[1], pc ) );
	if ((ptr[1] & 0x80000000) == 0) {
		SysPrintf("_vurecAnalyzeOp Lower: %s\n", disVU1MicroLF( ptr[0], pc ) );
	}*/
	if (ptr[1] & 0x40000000) {
		branch |= 8; 
	} 
 
	VU->code = ptr[1];
	if (VU == &VU1) {
		VU1regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	} else {
		VU0regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	}

	_recvuTestUpperStalls(VU, &uregs);
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
					info->statusflag|= VUOP_WRITE;
					info->macflag|= VUOP_WRITE;
					break;
			}
			break;
		case 0x3d:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7:
					break;
				default:
					info->statusflag|= VUOP_WRITE;
					info->macflag|= VUOP_WRITE;
					break;
			}
			break;
		case 0x3e:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag|= VUOP_WRITE;
					info->macflag|= VUOP_WRITE;
					break;
			}
			break;
		case 0x3f:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7: case 0xb:
					break;
				default:
					info->statusflag|= VUOP_WRITE;
					info->macflag|= VUOP_WRITE;
					break;
			}
			break;

		default:
			info->statusflag|= VUOP_WRITE;
			info->macflag|= VUOP_WRITE;
			break;
	}

	if (uregs.VIwrite & (1 << REG_CLIP_FLAG)) {
		info->clipflag |= VUOP_WRITE;
	}

	if (uregs.VIread & (1 << REG_Q)) {
		info->q |= VUOP_READ;
	}

	if (uregs.VIread & (1 << REG_P)) {
		assert( VU == &VU1 );
		info->p |= VUOP_READ;
	}

	/* check upper flags */ 
	if (ptr[1] & 0x80000000) { /* I flag */ 
		info->cycle = vucycle;

	} else {

		VU->code = ptr[0]; 
		if (VU == &VU1) {
			VU1regs_LOWER_OPCODE[VU->code >> 25](&lregs);
		} else {
			VU0regs_LOWER_OPCODE[VU->code >> 25](&lregs);
		}

		_recvuTestLowerStalls(VU, &lregs);
		info->cycle = vucycle;

		if (lregs.pipe == VUPIPE_BRANCH) {
			branch |= 1;
		}

		if (lregs.VIwrite & (1 << REG_Q)) {
//			SysPrintf("write to Q\n");
			info->q |= VUOP_WRITE;
			info->cycles = lregs.cycles;
		}
		else if (lregs.pipe == VUPIPE_FDIV) {
			info->q |= 8|1;
		}

		if (lregs.VIwrite & (1 << REG_P)) {
//			SysPrintf("write to P\n");
			info->p |= VUOP_WRITE;
			info->cycles = lregs.cycles;
		}
		else if (lregs.pipe == VUPIPE_EFU) {
			assert( VU == &VU1 );
			info->p |= 8|1;
		}

		if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
			info->clipflag|= VUOP_READ;
		}

		if (lregs.VIread & (1 << REG_STATUS_FLAG)) {
			info->statusflag|= VUOP_READ;
		}

		if (lregs.VIread & (1 << REG_MAC_FLAG)) {
			info->macflag|= VUOP_READ;
		}

		_recvuAddLowerStalls(VU, &lregs);
	}
	_recvuAddUpperStalls(VU, &uregs);

	_recvuTestPipes(VU);

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

	if( cinfo->statusflag & 1 ) info |= PROCESS_VU_UPDATEFLAGS;
	if( cinfo->macflag & 1) info |= PROCESS_VU_UPDATEFLAGS;
	
	if( regs->pipe == 0xff ) info |= PROCESS_VU_COP2;

	return info;
}

// returns the correct VI addr
u32 GetVIAddr(VURegs * VU, int reg, int read, int info)
{
	if( info & PROCESS_VU_SUPER ) return SuperVUGetVIAddr(reg, read);
	if( info & PROCESS_VU_COP2 ) return (uptr)&VU->VI[reg].UL;

	if( read != 1 ) {
		if( reg == REG_MAC_FLAG ) return (uptr)&VU->macflag;
		if( reg == REG_CLIP_FLAG ) return (uptr)&VU->clipflag;
		if( reg == REG_STATUS_FLAG ) return (uptr)&VU->statusflag;
		if( reg == REG_Q ) return (uptr)&VU->q;
		if( reg == REG_P ) return (uptr)&VU->p;
	}

	return (uptr)&VU->VI[reg].UL;
}

// gets a temp reg that is not EEREC_TEMP
int _vuGetTempXMMreg(int info)
{
	int t1reg = -1;

	if( _hasFreeXMMreg() ) {
		t1reg = _allocTempXMMreg(XMMT_FPS, -1);
		/*
		if( t1reg == EEREC_TEMP && _hasFreeXMMreg() ) {
			int t = _allocTempXMMreg(XMMT_FPS, -1);
			_freeXMMreg(t1reg);
			t1reg = t;
			_freeXMMreg(t1reg);
		}
		else {
			_freeXMMreg(t1reg);
			t1reg = -1;
		}
		*/
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

PCSX2_ALIGNED16(u32 g_minvals[4]) = {0xff7fffff, 0xff7fffff, 0xff7fffff, 0xff7fffff};
PCSX2_ALIGNED16(u32 g_maxvals[4]) = {0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff};

static PCSX2_ALIGNED16(int const_clip[]) = {
	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff,
	0x80000000, 0x80000000, 0x80000000, 0x80000000 };

static PCSX2_ALIGNED16(u32 s_FloatMinMax[]) = {
	0x007fffff, 0x007fffff, 0x007fffff, 0x007fffff,
	0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff,
	0,			0,			0,			0 };

static PCSX2_ALIGNED16(float s_fones[]) = { 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
static PCSX2_ALIGNED16(u32 s_mask[]) = {0x7fffff, 0x7fffff, 0x7fffff, 0x7fffff };
static PCSX2_ALIGNED16(u32 s_expmask[]) = {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};

static PCSX2_ALIGNED16(u32 s_overflowmask[]) = {0xf0000000, 0xf0000000, 0xf0000000, 0xf0000000};

void SetVUNanMode(int mode)
{
    g_VuNanHandling = mode;
    if( mode )
        SysPrintf("enabling vunan mode");
}

void CheckForOverflowSS_(int fdreg, int t0reg)
{
	assert( t0reg != fdreg );
    SSE_XORPS_XMM_to_XMM(t0reg, t0reg);
    SSE_CMPORDSS_XMM_to_XMM(t0reg, fdreg);
    if( g_VuNanHandling )
        SSE_ORPS_M128_to_XMM(t0reg, (uptr)s_overflowmask);
    SSE_ANDPS_XMM_to_XMM(fdreg, t0reg);

//	SSE_MOVSS_M32_to_XMM(t0reg, (u32)s_expmask);
//	SSE_ANDPS_XMM_to_XMM(t0reg, fdreg);
//	SSE_CMPNESS_M32_to_XMM(t0reg, (u32)s_expmask);
//	SSE_ANDPS_XMM_to_XMM(fdreg, t0reg);
}

	
	
void CheckForOverflow_(int fdreg, int t0reg, int keepxyzw)
{
//	SSE_MAXPS_M128_to_XMM(fdreg, (u32)g_minvals);
//	SSE_MINPS_M128_to_XMM(fdreg, (u32)g_maxvals);

    SSE_XORPS_XMM_to_XMM(t0reg, t0reg);
    SSE_CMPORDPS_XMM_to_XMM(t0reg, fdreg);

    /*if( g_VuNanHandling )
        SSE_ORPS_M128_to_XMM(t0reg, (uptr)s_overflowmask);*/

    // for partial masks, sometimes regs can be integers
    if( keepxyzw != 15 )
	    SSE_ORPS_M128_to_XMM(t0reg, (uptr)&SSEmovMask[15-keepxyzw][0]);
    SSE_ANDPS_XMM_to_XMM(fdreg, t0reg);

//	SSE_MOVAPS_M128_to_XMM(t0reg, (u32)s_expmask);
//	SSE_ANDPS_XMM_to_XMM(t0reg, fdreg);
//	SSE_CMPNEPS_M128_to_XMM(t0reg, (u32)s_expmask);
//	//SSE_ORPS_M128_to_XMM(t0reg, (u32)g_minvals);
//	SSE_ANDPS_XMM_to_XMM(fdreg, t0reg);
}

void CheckForOverflow(VURegs *VU, int info, int regd)
{
	if( CHECK_FORCEABS && EEREC_TEMP != regd) {
		// changing the order produces different results (tektag)
		CheckForOverflow_(regd, EEREC_TEMP, _X_Y_Z_W);
	}
}

// if unordered replaces with 0x7f7fffff
void ClampUnordered(int regd, int t0reg, int dosign)
{
	SSE_XORPS_XMM_to_XMM(t0reg, t0reg);
	SSE_CMPORDPS_XMM_to_XMM(t0reg, regd);
	SSE_ORPS_M128_to_XMM(t0reg, (uptr)&const_clip[4]);
	SSE_ANDPS_XMM_to_XMM(regd, t0reg);
	SSE_ANDNPS_M128_to_XMM(t0reg, (uptr)g_maxvals);
	SSE_ORPS_XMM_to_XMM(regd, t0reg);
}

//__declspec(naked) void temp()
//{
//    __asm ret
//}

// VU Flags
// NOTE: flags don't compute under/over flows since it is highly unlikely
// that games used them. Including them will lower performance.
void recUpdateFlags(VURegs * VU, int reg, int info)
{
	u32 flagmask;
	u8* pjmp;
	u32 macaddr, stataddr, prevstataddr;
	int x86macflag, x86newflag, x86oldflag;
	const static u8 macarr[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
	if( !(info & PROCESS_VU_UPDATEFLAGS) )
		return;

	flagmask = macarr[_X_Y_Z_W];
	macaddr = VU_VI_ADDR(REG_MAC_FLAG, 0);
	stataddr = VU_VI_ADDR(REG_STATUS_FLAG, 0);
	prevstataddr = VU_VI_ADDR(REG_STATUS_FLAG, 2);

	if( stataddr == 0 ) {
		stataddr = prevstataddr;
	}
	//assert( stataddr != 0);
	

	// 20 insts
	x86newflag = ALLOCTEMPX86(MODE_8BITREG);
	x86macflag = ALLOCTEMPX86(0);
	x86oldflag = ALLOCTEMPX86(0);
	
	// can do with 8 bits since only computing zero/sign flags
	if( EEREC_TEMP != reg ) {
		SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);  //Clear EEREC_TEMP
		SSE_CMPEQPS_XMM_to_XMM(EEREC_TEMP, reg);  // set all F's if each vector is zero
 
		MOV32MtoR(x86oldflag, prevstataddr);        // load the previous status in to x86oldflag

		SSE_MOVMSKPS_XMM_to_R32(x86newflag, EEREC_TEMP); // move the sign bits of the previous calculation (is reg vec zero) in to x86newflag

		XOR32RtoR(EAX, EAX);  //Clear EAX

		//if( !(g_VUGameFixes&VUFIX_SIGNEDZERO) ) {
			SSE_ANDNPS_XMM_to_XMM(EEREC_TEMP, reg); // necessary!  //EEREC_TEMP = !EEREC_TEMP & reg, 
													//  so if the result was zero before, EEREC_TEMP will now be blank.
		//}

		AND32ItoR(x86newflag, 0x0f&flagmask);  //Grab "Is zero" bits from the first calculation
		pjmp = JZ8(0); //Skip if none are
		OR32ItoR(EAX, 1); // Set if they are
		x86SetJ8(pjmp);

		/*if( !(g_VUGameFixes&VUFIX_SIGNEDZERO) )*/ SSE_MOVMSKPS_XMM_to_R32(x86macflag, EEREC_TEMP); // Grab sign bits from before, remember if "reg"
																								//Was zero, so will the sign bits
		//else SSE_MOVMSKPS_XMM_to_R32(x86macflag, reg); // unless we are using the signed zero fix, in which case, we keep it either way ;)

		
		AND32ItoR(x86macflag, 0x0f&flagmask); // Seperate the vectors we are using
		pjmp = JZ8(0);
		OR32ItoR(EAX, 2); // Set the "Signed" flag if it is signed
		x86SetJ8(pjmp);
		SHL32ItoR(x86newflag, 4); // Shift the zero flags left 4
		OR32RtoR(x86macflag, x86newflag);
	}
	else {
		SSE_MOVMSKPS_XMM_to_R32(x86macflag, EEREC_TEMP); // mask is < 0 (including 80000000) Get sign bits of all 4 vectors 
												  // put results in lower 4 bits of x86macflag

		MOV32MtoR(x86oldflag, prevstataddr); //move current (previous) status register to x86oldflag
		XOR32RtoR(EAX, EAX); //Clear EAX for our new flag

		SSE_CMPEQPS_M128_to_XMM(EEREC_TEMP, (uptr)&s_FloatMinMax[8]); //if the result zero? 
																	  //set to all F's (true) or All 0's on each vector (depending on result)
		
		SSE_MOVMSKPS_XMM_to_R32(x86newflag, EEREC_TEMP); // put the sign bit results from the previous calculation in x86newflag
														 // so x86newflag == 0xf if EEREC_TEMP is zero and == 0x0 if it is all a value.

		//if( !(g_VUGameFixes&VUFIX_SIGNEDZERO) ) {
			NOT32R(x86newflag); //flip all bits from previous calculation, so now if the result was zero, the result here is 0's
			AND32RtoR(x86macflag, x86newflag); //check non-zero macs against signs of initial register values
											   // so if the result was zero, regardless of if its signed or not, it wont set the signed flags
		//}

		AND32ItoR(x86macflag, 0xf&flagmask); //seperate out the flags we are actually using?
		pjmp = JZ8(0); //if none are the flags are set to 1 (aka the result is non-zero & positive, or they were zero) dont set the "signed" flag
		OR32ItoR(EAX, 2); //else we are signed
		x86SetJ8(pjmp);

		//if( !(g_VUGameFixes&VUFIX_SIGNEDZERO) ) { //Flip the bits back again so we have our "its zero" values
			NOT32R(x86newflag); //flip!
		//}

		AND32ItoR(x86newflag, 0xf&flagmask); //mask out the vectors we didnt use
		pjmp = JZ8(0);  //If none were zero skip
		OR32ItoR(EAX, 1); //We had a zero, so set el status flag with "zero":p
		x86SetJ8(pjmp);

		SHL32ItoR(x86newflag, 4);    //Move our zero flags left 4
		OR32RtoR(x86macflag, x86newflag); //then stick our signed flags intront of it
	}

	// x86macflag - new untransformed mac flag, EAX - new status bits, x86oldflag - old status flag
	// x86macflag = zero_wzyx | sign_wzyx
    MOV8RmtoROffset(x86newflag, x86macflag, (u32)g_MACFlagTransform); // transform
	//MOV32RtoR(x86macflag, x86newflag );
	//MOV32RtoR(x86macflag, x86oldflag);
	//SHL32ItoR(x86macflag, 6);
    //OR32RtoR(x86oldflag, x86macflag);
    
    if( macaddr != 0 ) {

        // has to write full 32 bits!
        MOV8RtoM(macaddr, x86newflag);

        // vampire night breaks with (g_VUGameFixes&VUFIX_EXTRAFLAGS), crazi taxi needs it
       /* if( (g_VUGameFixes&VUFIX_EXTRAFLAGS) && flagmask != 0xf ) {
            MOV8MtoR(x86newflag, VU_VI_ADDR(REG_MAC_FLAG, 2)); // get previous written
            AND8ItoR(x86newflag, ~g_MACFlagTransform[(flagmask|(flagmask<<4))]);
            OR8RtoM(macaddr, x86newflag);
        }	 */   
    }

	//AND32ItoR(x86oldflag, 0x0c0);
	SHR32ItoR(x86oldflag, 6);
	OR32RtoR(x86oldflag, EAX);
	SHL32ItoR(x86oldflag, 6);
	OR32RtoR(x86oldflag, EAX);
    //SHL32ItoR(EAX,6);
    //OR32RtoR(x86oldflag, EAX);
	MOV32RtoM(stataddr, x86oldflag);

	_freeX86reg(x86macflag);
	_freeX86reg(x86newflag);
	_freeX86reg(x86oldflag);
}

/******************************/
/*   VU Upper instructions    */
/******************************/

static PCSX2_ALIGNED16(int const_abs_table[16][4]) = 
{
   { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
   { 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff },
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0xffffffff },
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0x7fffffff },
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0xffffffff },
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0x7fffffff },
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0xffffffff },
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff },
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff },
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0x7fffffff },
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff },
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0x7fffffff },
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0xffffffff },
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0x7fffffff },
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0xffffffff },
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff },
};

void recVUMI_ABS(VURegs *VU, int info)
{
	if ( _Ft_ == 0 ) return;

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_abs_table[ _X_Y_Z_W ][ 0 ] );

		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	} else {
		if( EEREC_T != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
		SSE_ANDPS_M128_to_XMM(EEREC_T, (uptr)&const_abs_table[ _X_Y_Z_W ][ 0 ] );	
	}
}

PCSX2_ALIGNED16(float s_two[4]) = {0,0,0,2};

void recVUMI_ADD(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( _Fs_ == 0 && _Ft_ == 0 ) {
		if( _X_Y_Z_W != 0xf ) {
			SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (u32)s_two);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			SSE_MOVAPS_M128_to_XMM(EEREC_D, (u32)s_two);
		}
	}
	else {
		if( _X_Y_Z_W == 8 ) {
			if (EEREC_D == EEREC_S) SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if (EEREC_D == EEREC_S) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}

//	if( _Fd_ == 0 && (_Fs_ == 0 || _Ft_ == 0) )
//		info |= PROCESS_VU_UPDATEFLAGS;

	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_ADD_iq(VURegs *VU, uptr addr, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( _XYZW_SS ) {
		if( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_ADDSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);

			// have to flip over EEREC_D if computing flags!
			//if( (info & PROCESS_VU_UPDATEFLAGS) )
				_vuFlipRegSS(VU, EEREC_D);
		}
		else if( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_ADDSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if( _X ) {
				if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_M32_to_XMM(EEREC_D, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_D == EEREC_S || EEREC_D == EEREC_TEMP) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		} else {
			if( EEREC_D == EEREC_TEMP ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else if( EEREC_D == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_D, addr); 
				SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}

	recUpdateFlags(VU, EEREC_D, info);

	if( addr == VU_REGQ_ADDR ) CheckForOverflow(VU, info, EEREC_D);
}

void recVUMI_ADD_xyzw(VURegs *VU, int xyzw, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( _Ft_ == 0 && xyzw < 3 ) {
		// just move
		if( _X_Y_Z_W != 0xf ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if( EEREC_D != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
	else if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP) ) {
		if( xyzw == 0 ) {
			if( EEREC_D == EEREC_T ) {
				SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
	}
	else if( _Fs_ == 0 && !_W ) {
		// just move
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

//        SSE_MOVAPS_XMM_to_M128((u32)s_tempmem, EEREC_TEMP);
//        SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip);
//        SSE_CMPNLTPS_M128_to_XMM(EEREC_TEMP, (u32)s_FloatMinMax);
//        SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)s_tempmem);

		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_D == EEREC_S || EEREC_D == EEREC_TEMP)
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
		
		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		} else {
			if( EEREC_D == EEREC_TEMP ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else if( EEREC_D == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			else {
				_unpackVF_xyzw(EEREC_D, EEREC_T, xyzw);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
	
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_ADDi(VURegs *VU, int info) { recVUMI_ADD_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_ADDq(VURegs *VU, int info) { recVUMI_ADD_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_ADDx(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 0, info); }
void recVUMI_ADDy(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 1, info); }
void recVUMI_ADDz(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 2, info); }
void recVUMI_ADDw(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 3, info); }

void recVUMI_ADDA(VURegs *VU, int info)
{
	if( _X_Y_Z_W == 8 ) {
		if (EEREC_ACC == EEREC_S) SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if (EEREC_ACC == EEREC_T) SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
	}
	else {
		if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if( EEREC_ACC == EEREC_T ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDA_iq(VURegs *VU, int addr, int info)
{
	if( _XYZW_SS ) {
		assert( EEREC_ACC != EEREC_TEMP );
		if( EEREC_ACC == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_ACC);
			SSE_ADDSS_M32_to_XMM(EEREC_ACC, addr);
			_vuFlipRegSS(VU, EEREC_ACC);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_ADDSS_M32_to_XMM(EEREC_ACC, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_ACC == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
		}
		else {
			if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_ACC, addr); 
				SSE_SHUFPS_XMM_to_XMM(EEREC_ACC, EEREC_ACC, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			}
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDA_xyzw(VURegs *VU, int xyzw, int info)
{
	if( _X_Y_Z_W == 8 ) {
		if( xyzw == 0 ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			if( _Fs_ == 0 ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_ACC == EEREC_S )
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);

			VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
		} else {
			if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			else {
				_unpackVF_xyzw(EEREC_ACC, EEREC_T, xyzw);
				SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			}
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDAi(VURegs *VU, int info) { recVUMI_ADDA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_ADDAq(VURegs *VU, int info) { recVUMI_ADDA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_ADDAx(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 0, info); }
void recVUMI_ADDAy(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 1, info); }
void recVUMI_ADDAz(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 2, info); }
void recVUMI_ADDAw(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 3, info); }

void recVUMI_SUB(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( EEREC_S == EEREC_T ) {
		if (_X_Y_Z_W != 0xf) SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&SSEmovMask[15-_X_Y_Z_W][0]);
		else SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (EEREC_D == EEREC_S) SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
		else if (EEREC_D == EEREC_T) {
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if (_X_Y_Z_W != 0xf) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( _Ft_ > 0 || _W ) SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if (EEREC_D == EEREC_S) SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}

	recUpdateFlags(VU, EEREC_D, info);
	// neopets works better with this?
	//CheckForOverflow(info, EEREC_D);
}

void recVUMI_SUB_iq(VURegs *VU, int addr, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( _XYZW_SS ) {
		if( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);

			// have to flip over EEREC_D if computing flags!
			//if( (info & PROCESS_VU_UPDATEFLAGS) )
				_vuFlipRegSS(VU, EEREC_D);
		}
		else if( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if( _X ) {
				if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_D);
				SSE_SUBSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_D);
			}
		}
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_D, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_D == EEREC_TEMP ) {
				SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				if (EEREC_D != EEREC_S) SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
	}

	recUpdateFlags(VU, EEREC_D, info);

	if( addr == VU_REGQ_ADDR ) CheckForOverflow(VU, info, EEREC_D);
}

static PCSX2_ALIGNED16(s_unaryminus[4]) = {0x80000000, 0, 0, 0};

void recVUMI_SUB_xyzw(VURegs *VU, int xyzw, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	if( _X_Y_Z_W == 8 ) {
        if( EEREC_D == EEREC_TEMP ) {
			
            switch (xyzw) {
		        case 0:
			        if( EEREC_TEMP != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			        break;
		        case 1:
			        if( cpucaps.hasStreamingSIMD3Extensions ) SSE3_MOVSLDUP_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			        else {
				        if( EEREC_TEMP != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				        SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
			        }
			        break;
		        case 2:
			        SSE_MOVLHPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			        break;
		        case 3:
			        if( cpucaps.hasStreamingSIMD3Extensions && EEREC_TEMP != EEREC_S ) {
				        SSE3_MOVSLDUP_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				        SSE_MOVLHPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
			        }
			        else {
				        if( EEREC_TEMP != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				        SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
			        }
			        break;
	        }

			SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_T);

			// have to flip over EEREC_D if computing flags!
            //if( (info & PROCESS_VU_UPDATEFLAGS) ) {
                if( xyzw == 1 ) SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0xe1); // y
	            else if( xyzw == 2 ) SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0xc6); // z
	            else if( xyzw == 3 ) SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x27); // w
            //}
		}
		else {	
		    if( xyzw == 0 ) {
			    if( EEREC_D == EEREC_T ) {
				    if( _Fs_ > 0 ) SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_S);
				    SSE_XORPS_M128_to_XMM(EEREC_D, (u32)s_unaryminus);
			    }
			    else {
				    if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				    SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
			    }
		    }
		    else {
			    _unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			    SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			    SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		    }
        }
	}
//	else if( _XYZW_SS && xyzw == 0 ) {
//		if( EEREC_D == EEREC_S ) {
//			if( EEREC_D == EEREC_T ) {
//				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
//				_vuFlipRegSS(VU, EEREC_D);
//				SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
//				_vuFlipRegSS(VU, EEREC_D);
//			}
//			else {
//				_vuFlipRegSS(VU, EEREC_D);
//				SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
//				_vuFlipRegSS(VU, EEREC_D);
//			}
//		}
//		else if( EEREC_D == EEREC_T ) {
//			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Y?1:(_Z?2:3));
//			SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
//			_vuFlipRegSS(VU, EEREC_D);
//			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
//			_vuFlipRegSS(VU, EEREC_D);
//		}
//		else {
//			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Y?1:(_Z?2:3));
//			_vuFlipRegSS(VU, EEREC_D);
//			SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
//			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
//			_vuFlipRegSS(VU, EEREC_D);
//		}
//	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_D, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_D == EEREC_TEMP ) {
				SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				if( EEREC_D != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
	}

	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_SUBi(VURegs *VU, int info) { recVUMI_SUB_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_SUBq(VURegs *VU, int info) { recVUMI_SUB_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_SUBx(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 0, info); }
void recVUMI_SUBy(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 1, info); }
void recVUMI_SUBz(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 2, info); }
void recVUMI_SUBw(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 3, info); }

void recVUMI_SUBA(VURegs *VU, int info)
{
	if( EEREC_S == EEREC_T ) {
		if (_X_Y_Z_W != 0xf) SSE_ANDPS_M128_to_XMM(EEREC_ACC, (uptr)&SSEmovMask[15-_X_Y_Z_W][0]);
		else SSE_XORPS_XMM_to_XMM(EEREC_ACC, EEREC_ACC);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (EEREC_ACC == EEREC_S) SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if (EEREC_ACC == EEREC_T) {
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
	}
	else {
		if( EEREC_ACC == EEREC_S ) SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if( EEREC_ACC == EEREC_T ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBA_iq(VURegs *VU, int addr, int info)
{
	if( _XYZW_SS ) {
		if( EEREC_ACC == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_ACC);
			SSE_SUBSS_M32_to_XMM(EEREC_ACC, addr);
			_vuFlipRegSS(VU, EEREC_ACC);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_SUBSS_M32_to_XMM(EEREC_ACC, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_ACC);
				SSE_SUBSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_ACC);
			}
		}
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_ACC, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_ACC != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBA_xyzw(VURegs *VU, int xyzw, int info)
{
	if( _X_Y_Z_W == 8 ) {
		if( xyzw == 0 ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_ACC, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_ACC != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBAi(VURegs *VU, int info) { recVUMI_SUBA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_SUBAq(VURegs *VU, int info) { recVUMI_SUBA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_SUBAx(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 0, info); }
void recVUMI_SUBAy(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 1, info); }
void recVUMI_SUBAz(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 2, info); }
void recVUMI_SUBAw(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 3, info); }

void recVUMI_MUL_toD(VURegs *VU, int regd, int info)
{
	if (_X_Y_Z_W == 1 && (_Ft_ == 0 || _Fs_==0) ) { // W
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, _Ft_ ? EEREC_T : EEREC_S);
		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else if( _Fd_ == _Fs_ && _Fs_ == _Ft_ && _XYZW_SS ) {
		_vuFlipRegSS(VU, EEREC_D);
		SSE_MULSS_XMM_to_XMM(EEREC_D, EEREC_D);
		_vuFlipRegSS(VU, EEREC_D);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (regd == EEREC_S) SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
		else if (regd == EEREC_T) SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else {
		if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
		else if (regd == EEREC_T) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
		else {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
		}
	}
}

void recVUMI_MUL_iq_toD(VURegs *VU, int addr, int regd, int info)
{
	if( _XYZW_SS ) {
		if( regd == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_M32_to_XMM(regd, addr);
			_vuFlipRegSS(VU, EEREC_S);
		}
		else if( regd == EEREC_S ) {
			_vuFlipRegSS(VU, regd);
			SSE_MULSS_M32_to_XMM(regd, addr);
			_vuFlipRegSS(VU, regd);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				SSE_MULSS_M32_to_XMM(regd, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_TEMP || regd == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_TEMP ) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			else if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(regd, addr); 
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x00);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			}		
		}
	}
}

void recVUMI_MUL_xyzw_toD(VURegs *VU, int xyzw, int regd, int info)
{
	if( _Ft_ == 0 ) {
		if( xyzw < 3 ) {
			if (_X_Y_Z_W != 0xf) {	
				SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else {
				SSE_XORPS_XMM_to_XMM(regd, regd);
			}
		}
		else {
			assert(xyzw==3);
			if (_X_Y_Z_W != 0xf) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else if( regd != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
		}
	}
	else if( _X_Y_Z_W == 8 ) {
		if( regd == EEREC_TEMP ) {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
		}
		else {
			if( xyzw == 0 ) {
				if( regd == EEREC_T ) {
					SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
				}
				else {
					SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
					SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
				}
			}
			else {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				SSE_MULSS_XMM_to_XMM(regd, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_TEMP || regd == EEREC_S )
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {	
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_TEMP ) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			else if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			else {
				_unpackVF_xyzw(regd, EEREC_T, xyzw);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			}
		}
	}
}

void recVUMI_MUL(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MUL_iq(VURegs *VU, int addr, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_iq_toD(VU, addr, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
	// spacefisherman needs overflow checking on MULi.z
	if( addr == VU_REGQ_ADDR || _Z )
		CheckForOverflow(VU, info, EEREC_D);
}

void recVUMI_MUL_xyzw(VURegs *VU, int xyzw, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_xyzw_toD(VU, xyzw, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MULi(VURegs *VU, int info) { recVUMI_MUL_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MULq(VURegs *VU, int info) { recVUMI_MUL_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MULx(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 0, info); }
void recVUMI_MULy(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 1, info); }
void recVUMI_MULz(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 2, info); }
void recVUMI_MULw(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 3, info); }

void recVUMI_MULA( VURegs *VU, int info )
{
	recVUMI_MUL_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULA_iq(VURegs *VU, int addr, int info)
{	
	recVUMI_MUL_iq_toD(VU, addr, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULA_xyzw(VURegs *VU, int xyzw, int info)
{
	recVUMI_MUL_xyzw_toD(VU, xyzw, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULAi(VURegs *VU, int info) { recVUMI_MULA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MULAq(VURegs *VU, int info) { recVUMI_MULA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MULAx(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 0, info); }
void recVUMI_MULAy(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 1, info); }
void recVUMI_MULAz(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 2, info); }
void recVUMI_MULAw(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 3, info); }

void recVUMI_MADD_toD(VURegs *VU, int regd, int info)
{
	if( _X_Y_Z_W == 8 ) {
		if( regd == EEREC_ACC ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if (regd == EEREC_T) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if (regd == EEREC_S) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else {
		if( regd == EEREC_ACC ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if (regd == EEREC_T) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if (regd == EEREC_S) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
}

void recVUMI_MADD_iq_toD(VURegs *VU, int addr, int regd, int info)
{
	if( _X_Y_Z_W == 8 ) {
		if( regd == EEREC_ACC ) {
			if( _Fs_ == 0 ) {
				// add addr to w
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
				SSE_ADDSS_M32_to_XMM(regd, addr);
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
			}
			else {
				assert( EEREC_TEMP < XMMREGS );
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
			}
		}
		else if( regd == EEREC_S ) {
			SSE_MULSS_M32_to_XMM(regd, addr);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_M32_to_XMM(regd, addr);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
	else {
		if( _Fs_ == 0 ) {
			// add addr to w
			if( _W ) {
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
				SSE_ADDSS_M32_to_XMM(regd, addr);
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x27);
			}

			return;
		}

		if( _X_Y_Z_W != 0xf || regd == EEREC_ACC || regd == EEREC_TEMP || regd == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_ACC ) {
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
			}
			else if( regd == EEREC_S ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else if( regd == EEREC_TEMP ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else {
				SSE_MOVSS_M32_to_XMM(regd, addr);
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x00);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
		}
	}
}

void recVUMI_MADD_xyzw_toD(VURegs *VU, int xyzw, int regd, int info)
{
	if( _Ft_ == 0 ) {

		if( xyzw == 3 ) {
			// just add
			if( _X_Y_Z_W == 8 ) {
				if( regd == EEREC_S ) SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
				else {
					if( regd != EEREC_ACC ) SSE_MOVSS_XMM_to_XMM(regd, EEREC_ACC);
					SSE_ADDSS_XMM_to_XMM(regd, EEREC_S);
				}
			}
			else {
				if( _X_Y_Z_W != 0xf ) {
					SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
					SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

					VU_MERGE_REGS(regd, EEREC_TEMP);
				}
				else {
					if( regd == EEREC_S ) SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
					else {
						if( regd != EEREC_ACC ) SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
						SSE_ADDPS_XMM_to_XMM(regd, EEREC_S);
					}
				}
			}
		}
		else {
			// just move acc to regd
			if( _X_Y_Z_W != 0xf ) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else {
				if( regd != EEREC_ACC ) SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
			}
		}

		return;
	}

	if( _X_Y_Z_W == 8 ) {
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if( regd == EEREC_ACC ) {
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if( regd == EEREC_S ) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_TEMP);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_ACC || regd == EEREC_TEMP || regd == EEREC_S )
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_ACC ) {
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
			}
			else if( regd == EEREC_S ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else if( regd == EEREC_TEMP ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else {
				_unpackVF_xyzw(regd, EEREC_T, xyzw);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
		}
	}
}

void recVUMI_MADD(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MADD_iq(VURegs *VU, int addr, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_iq_toD(VU, addr, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);

	if( addr == VU_REGQ_ADDR ) CheckForOverflow(VU, info, EEREC_D);
}

void recVUMI_MADD_xyzw(VURegs *VU, int xyzw, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_xyzw_toD(VU, xyzw, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);

	// super bust-a-move arrows
	CheckForOverflow(VU, info, EEREC_D);
}

void recVUMI_MADDi(VURegs *VU, int info) { recVUMI_MADD_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MADDq(VURegs *VU, int info) { recVUMI_MADD_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MADDx(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 0, info); }
void recVUMI_MADDy(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 1, info); }
void recVUMI_MADDz(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 2, info); }
void recVUMI_MADDw(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 3, info); }

void recVUMI_MADDA( VURegs *VU, int info )
{
	recVUMI_MADD_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAi( VURegs *VU , int info)
{
	recVUMI_MADD_iq_toD( VU, VU_VI_ADDR(REG_I, 1), EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAq( VURegs *VU , int info)
{
	recVUMI_MADD_iq_toD( VU, VU_REGQ_ADDR, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAx( VURegs *VU , int info)
{
	recVUMI_MADD_xyzw_toD(VU, 0, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAy( VURegs *VU , int info)
{
	recVUMI_MADD_xyzw_toD(VU, 1, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAz( VURegs *VU , int info)
{
	recVUMI_MADD_xyzw_toD(VU, 2, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAw( VURegs *VU , int info)
{
	recVUMI_MADD_xyzw_toD(VU, 3, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUB_toD(VURegs *VU, int regd, int info)
{
	if (_X_Y_Z_W != 0xf) {
		int t1reg = _vuGetTempXMMreg(info);

		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		if( t1reg >= 0 ) {
			SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_ACC);
			SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

			VU_MERGE_REGS(regd, t1reg);
			_freeXMMreg(t1reg);
		}
		else {
			SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
	}
	else {
		if( regd == EEREC_S ) {
			assert( regd != EEREC_ACC );
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_T ) {
			assert( regd != EEREC_ACC );
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( regd != EEREC_ACC ) SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);	
		}
	}
}

void recVUMI_MSUB_temp_toD(VURegs *VU, int regd, int info)
{
	if (_X_Y_Z_W != 0xf) {
		int t1reg = _vuGetTempXMMreg(info);

		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);

		if( t1reg >= 0 ) {
			SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_ACC);
			SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

			if( regd != EEREC_TEMP ) {
				VU_MERGE_REGS(regd, t1reg);
			}
			else
				SSE_MOVAPS_XMM_to_XMM(regd, t1reg);

			_freeXMMreg(t1reg);
		}
		else {
			SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
	}
	else {
		if( regd == EEREC_ACC ) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);	
		}
		else if( regd == EEREC_S ) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else {
			if( regd != EEREC_ACC ) SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);	
		}
	}
}

void recVUMI_MSUB_iq_toD(VURegs *VU, int regd, int addr, int info)
{
	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
	SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
	recVUMI_MSUB_temp_toD(VU, regd, info);
}

void recVUMI_MSUB_xyzw_toD(VURegs *VU, int regd, int xyzw, int info)
{
	_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
	recVUMI_MSUB_temp_toD(VU, regd, info);
}

void recVUMI_MSUB(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUB_iq(VURegs *VU, int addr, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_iq_toD(VU, EEREC_D, addr, info);
	recUpdateFlags(VU, EEREC_D, info);

	if( addr == VU_REGQ_ADDR ) CheckForOverflow(VU, info, EEREC_D);
}

void recVUMI_MSUBi(VURegs *VU, int info) { recVUMI_MSUB_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MSUBq(VURegs *VU, int info) { recVUMI_MSUB_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MSUBx(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 0, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBy(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 1, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBz(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 2, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBw(VURegs *VU, int info)
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 3, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBA( VURegs *VU, int info )
{
	recVUMI_MSUB_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAi( VURegs *VU, int info )
{
	recVUMI_MSUB_iq_toD( VU, EEREC_ACC, VU_VI_ADDR(REG_I, 1), info );
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAq( VURegs *VU, int info )
{
	recVUMI_MSUB_iq_toD( VU, EEREC_ACC, VU_REGQ_ADDR, info );
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAx( VURegs *VU, int info )
{
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 0, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAy( VURegs *VU, int info )
{
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 1, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAz( VURegs *VU, int info )
{
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 2, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAw( VURegs *VU, int info )
{
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 3, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MAX(VURegs *VU, int info)
{	
	if ( _Fd_ == 0 ) return;

	if( _X_Y_Z_W == 8 ) {
		if (EEREC_D == EEREC_S) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
		else if (EEREC_D == EEREC_T) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if( EEREC_D == EEREC_S ) SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_T);
		else if( EEREC_D == EEREC_T ) SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
}

void recVUMI_MAX_iq(VURegs *VU, int addr, int info)
{	
	if ( _Fd_ == 0 ) return;

	if( _XYZW_SS ) {
		if( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);

			// have to flip over EEREC_D if computing flags!
			//if( (info & PROCESS_VU_UPDATEFLAGS) )
				_vuFlipRegSS(VU, EEREC_D);
		}
		else if( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_D);
				SSE_MAXSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_D);
			}
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if(EEREC_D == EEREC_S) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
			SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_M32_to_XMM(EEREC_D, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
			SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
}

void recVUMI_MAX_xyzw(VURegs *VU, int xyzw, int info)
{	
	if ( _Fd_ == 0 ) return;

	if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP)) {
		if( _Fs_ == 0 && _Ft_ == 0 ) {
			if( xyzw < 3 ) {
				SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (u32)s_fones);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
		else {
			if( xyzw == 0 ) {
				if( EEREC_D == EEREC_S ) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
				else if( EEREC_D == EEREC_T ) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_S);
				else {
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
				}
			}
			else {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		if( _Fs_ == 0 && _Ft_ == 0 ) {
			if( xyzw < 3 ) {
				if( _X_Y_Z_W & 1 ) {
					// w included, so insert the whole reg
					SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[0]);
				}
				else {
					// w not included, can zero out
					SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				}
			}
			else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (u32)s_fones);
		}
		else {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		}
		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if( _Fs_ == 0 && _Ft_ == 0 ) {
			if( xyzw < 3 ) SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
			else SSE_MOVAPS_M128_to_XMM(EEREC_D, (u32)s_fones);
		}
		else {
			if (EEREC_D == EEREC_S) {
				_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				_unpackVF_xyzw(EEREC_D, EEREC_T, xyzw);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
}

void recVUMI_MAXi(VURegs *VU, int info) { recVUMI_MAX_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MAXx(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 0, info); }
void recVUMI_MAXy(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 1, info); }
void recVUMI_MAXz(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 2, info); }
void recVUMI_MAXw(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 3, info); }

void recVUMI_MINI(VURegs *VU, int info)
{
	if ( _Fd_ == 0 ) return;

	if( _X_Y_Z_W == 8 ) {
		if (EEREC_D == EEREC_S) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
		else if (EEREC_D == EEREC_T) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_S);
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if( EEREC_D == EEREC_S ) {
			// need for GT4 vu0rec
			ClampUnordered(EEREC_T, EEREC_TEMP, 0);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
		else if( EEREC_D == EEREC_T ) {
			// need for GT4 vu0rec
			ClampUnordered(EEREC_S, EEREC_TEMP, 0);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
}

void recVUMI_MINI_iq(VURegs *VU, int addr, int info)
{
	if ( _Fd_ == 0 ) return;

	if( _XYZW_SS ) {
		if( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MINSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);

			// have to flip over EEREC_D if computing flags!
			//if( (info & PROCESS_VU_UPDATEFLAGS) )
				_vuFlipRegSS(VU, EEREC_D);
		}
		else if( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_MINSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINSS_M32_to_XMM(EEREC_D, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_D);
				SSE_MINSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_D);
			}
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); 
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if(EEREC_D == EEREC_S) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_M32_to_XMM(EEREC_D, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
}

void recVUMI_MINI_xyzw(VURegs *VU, int xyzw, int info)
{
	if ( _Fd_ == 0 ) return;

	if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP)) {
		if( xyzw == 0 ) {
			if( EEREC_D == EEREC_S ) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( EEREC_D == EEREC_T ) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			if( EEREC_D != EEREC_S ) SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
		SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if (EEREC_D == EEREC_S) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
		else {
			_unpackVF_xyzw(EEREC_D, EEREC_T, xyzw);
			SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
}

void recVUMI_MINIi(VURegs *VU, int info) { recVUMI_MINI_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MINIx(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 0, info); }
void recVUMI_MINIy(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 1, info); }
void recVUMI_MINIz(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 2, info); }
void recVUMI_MINIw(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 3, info); }

void recVUMI_OPMULA( VURegs *VU, int info )
{
	SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
	SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xD2);			// EEREC_T = WYXZ
	SSE_SHUFPS_XMM_to_XMM( EEREC_TEMP, EEREC_TEMP, 0xC9 );	// EEREC_TEMP = WXZY
	SSE_MULPS_XMM_to_XMM( EEREC_TEMP, EEREC_T );

	VU_MERGE_REGS_CUSTOM(EEREC_ACC, EEREC_TEMP, 14);

	// revert EEREC_T
	if( EEREC_T != EEREC_ACC )
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xC9);

	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_OPMSUB( VURegs *VU, int info )
{
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);

	SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
	SSE_SHUFPS_XMM_to_XMM( EEREC_T, EEREC_T, 0xD2 );           // EEREC_T = WYXZ
	SSE_SHUFPS_XMM_to_XMM( EEREC_TEMP, EEREC_TEMP, 0xC9 );     // EEREC_TEMP = WXZY
	SSE_MULPS_XMM_to_XMM( EEREC_TEMP, EEREC_T);

	// negate and add
	SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
	SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
	VU_MERGE_REGS_CUSTOM(EEREC_D, EEREC_TEMP, 14);
	
	// revert EEREC_T
	if( EEREC_T != EEREC_D )
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xC9);

	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_NOP( VURegs *VU, int info ) 
{
}

void recVUMI_FTOI0(VURegs *VU, int info)
{	
	if ( _Ft_ == 0 ) return; 

	if (_X_Y_Z_W != 0xf) {
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_S);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_S);
	}	
}

void recVUMI_FTOIX(VURegs *VU, int addr, int info)
{
	if ( _Ft_ == 0 ) return; 

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_M128_to_XMM(EEREC_TEMP, addr);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if (EEREC_T != EEREC_S) SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
		SSE_MULPS_M128_to_XMM(EEREC_T, addr);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_T);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_T);
	}
}

void recVUMI_FTOI4( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int4[0], info); }
void recVUMI_FTOI12( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int12[0], info); }
void recVUMI_FTOI15( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int15[0], info); }

void recVUMI_ITOF0( VURegs *VU, int info )
{
	if ( _Ft_ == 0 ) return;

	if (_X_Y_Z_W != 0xf) {
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		else {
			_deleteVFtoXMMreg(_Fs_, VU==&VU1, 1);
			SSE2EMU_CVTDQ2PS_M128_to_XMM(EEREC_TEMP, VU_VFx_ADDR( _Fs_ ));
		}
		
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		xmmregs[EEREC_T].mode |= MODE_WRITE;
	}
	else {
		if( cpucaps.hasStreamingSIMD2Extensions ) SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_T, EEREC_S);
		else {
			_deleteVFtoXMMreg(_Fs_, VU==&VU1, 1);
			SSE2EMU_CVTDQ2PS_M128_to_XMM(EEREC_T, VU_VFx_ADDR( _Fs_ ));
			xmmregs[EEREC_T].mode |= MODE_WRITE;
		}
	}
}

void recVUMI_ITOFX(VURegs *VU, int addr, int info)
{
	if ( _Ft_ == 0 ) return; 

	if (_X_Y_Z_W != 0xf) {
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		else {
			_deleteVFtoXMMreg(_Fs_, VU==&VU1, 1);
			SSE2EMU_CVTDQ2PS_M128_to_XMM(EEREC_TEMP, VU_VFx_ADDR( _Fs_ ));
		}

		SSE_MULPS_M128_to_XMM(EEREC_TEMP, addr);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		xmmregs[EEREC_T].mode |= MODE_WRITE;
	} else {
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_T, EEREC_S);
		else {
			_deleteVFtoXMMreg(_Fs_, VU==&VU1, 1);
			SSE2EMU_CVTDQ2PS_M128_to_XMM(EEREC_T, VU_VFx_ADDR( _Fs_ ));
			xmmregs[EEREC_T].mode |= MODE_WRITE;
		}

		SSE_MULPS_M128_to_XMM(EEREC_T, addr);
	}
}

void recVUMI_ITOF4( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float4[0], info); }
void recVUMI_ITOF12( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float12[0], info); }
void recVUMI_ITOF15( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float15[0], info); }

void recVUMI_CLIP(VURegs *VU, int info)
{
	int t1reg = EEREC_D;
	int t2reg = EEREC_ACC;
	int x86temp0, x86temp1;

	u32 clipaddr = VU_VI_ADDR(REG_CLIP_FLAG, 0);
	u32 prevclipaddr = VU_VI_ADDR(REG_CLIP_FLAG, 2);

	if( clipaddr == 0 ) {
		// battle star has a clip right before fcset
		SysPrintf("skipping vu clip\n");
		return;
	}
	assert( clipaddr != 0 );
	assert( t1reg != t2reg && t1reg != EEREC_TEMP && t2reg != EEREC_TEMP );

	x86temp1 = ALLOCTEMPX86(MODE_8BITREG);
	x86temp0 = ALLOCTEMPX86(0);

	if( _Ft_ == 0 ) {
		// all 1s
		SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)&s_fones[0]);
		SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)&s_fones[4]);

		MOV32MtoR(EAX, prevclipaddr);
	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, 3);
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (int)const_clip);

		MOV32MtoR(EAX, prevclipaddr);
	
		SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_TEMP);
		SSE_ORPS_M128_to_XMM(t1reg, (uptr)&const_clip[4]);
	}

	SSE_CMPNLEPS_XMM_to_XMM(t1reg, EEREC_S);  //-w, -z, -y, -x
	SSE_CMPLTPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); //+w, +z, +y, +x

	SHL32ItoR(EAX, 6);

	SSE_MOVAPS_XMM_to_XMM(t2reg, EEREC_TEMP); //t2 = +w, +z, +y, +x
	SSE_UNPCKLPS_XMM_to_XMM(EEREC_TEMP, t1reg); //EEREC_TEMP = -y,+y,-x,+x
	SSE_UNPCKHPS_XMM_to_XMM(t2reg, t1reg); //t2reg = -w,+w,-z,+z
	SSE_MOVMSKPS_XMM_to_R32(x86temp0, EEREC_TEMP); // -y,+y,-x,+x
	SSE_MOVMSKPS_XMM_to_R32(x86temp1, t2reg); // -w,+w,-z,+z

	AND32ItoR(EAX, 0xffffff);

	AND8ItoR(x86temp1, 0x3);
	SHL32ItoR(x86temp1, 4);
	OR32RtoR(EAX, x86temp0);
	OR32RtoR(EAX, x86temp1);

	MOV32RtoM(clipaddr, EAX);
	/*if( !(info&(PROCESS_VU_SUPER|PROCESS_VU_COP2)) )*/ MOV32RtoM((uptr)&VU->VI[REG_CLIP_FLAG], EAX);

	_freeXMMreg(t1reg);
	_freeXMMreg(t2reg);

	_freeX86reg(x86temp0);
	_freeX86reg(x86temp1);
}

/******************************/
/*   VU Lower instructions    */
/******************************/

void recVUMI_DIV(VURegs *VU, int info)
{
	int t1reg;

	if( _Fs_ == 0 ) {

		if( _Ft_ == 0 ) {
			if( _Fsf_ < 3 ) { // 0/ft
				if( _Ftf_ < 3 ) { // 0/0
					//SysPrintf("DIV 0/0\n");
					OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag (only when 0/0)
					MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff);				
				} else { // 0/1
					//SysPrintf("DIV 0/1\n");
					OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
					MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff);
				}
			}else if( _Ftf_ < 3 ) { // 1/0
				//SysPrintf("DIV 1/0\n");
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff);
			} else { // 1/1
				//SysPrintf("DIV 1/1\n");
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x3f800000);
			}
			return;
		}

		if( _Fsf_ == 3 ) { // = 1
			// don't use RCPSS (very bad precision)
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);

			if( _Ftf_ != 0 || (xmmregs[EEREC_T].mode & MODE_WRITE) )
			{
				if( _Ftf_ )
				{
					t1reg = _vuGetTempXMMreg(info);

					if( t1reg >= 0 )
					{
						_unpackVFSS_xyzw(t1reg, EEREC_T, _Ftf_);

						SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, t1reg);

						_freeXMMreg(t1reg);
					}
					else
					{
						SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, (0xe4e4>>(2*_Ftf_))&0xff);
						SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
						SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, (0xe4e4>>(8-2*_Ftf_))&0xff); // revert
					}
				}
				else
				{
					SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
				}
			}
			else {
				SSE_DIVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Ft_].UL[_Ftf_]);
			}
		}
		else { // = 0 So result is -MAX/+MAX
			//SysPrintf("FS = 0, FT != 0\n");
			OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag (only when 0/0)
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_ORPS_M128_to_XMM(EEREC_TEMP, (u32)&g_maxvals);
			SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
			//MOV32ItoR(VU_VI_ADDR(REG_Q, 0), 0);
			return;
		}
	}
	else {
		if( _Ft_ == 0 ) {
			if( _Ftf_ < 3 ) { 
				//SysPrintf("FS != 0, FT == 0\n");
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff);
				
			} else {
				//SysPrintf("FS != 0, FT == 1\n");
				if( _Fsf_ == 0 ) SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				else _unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
				SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
			}
				
			return;
		}

		if( _Fsf_ == 0 ) SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		else _unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		if( _Ftf_ )
		{
			t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 )
			{
				_unpackVFSS_xyzw(t1reg, EEREC_T, _Ftf_);

				SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, t1reg);

				_freeXMMreg(t1reg);
			}
			else
			{
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, (0xe4e4>>(2*_Ftf_))&0xff);
				SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, (0xe4e4>>(8-2*_Ftf_))&0xff); // revert
			}
		}
		else
		{
			SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		}
	}

	//if( !CHECK_FORCEABS ) {
		SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
		SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);
	//}

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
}

void recVUMI_SQRT( VURegs *VU, int info )
{
	int vftemp = ALLOCTEMPX86(MODE_8BITREG);
	u8* pjmp;

	AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFDF); //Divide flag cleared regardless of result
	
	if( _Ftf_ ) {
		if( xmmregs[EEREC_T].mode & MODE_WRITE ) {
			//SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip);
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			_unpackVF_xyzw(EEREC_TEMP, EEREC_TEMP, _Ftf_);
		}
		else {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Ft_].UL[_Ftf_]);
			//SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip);
			}
	}
	else {
		//SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (u32)const_clip);
		SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
	}
	/* Check for negative divide */
	XOR32RtoR(vftemp, vftemp);
	SSE_MOVMSKPS_XMM_to_R32(vftemp, EEREC_TEMP);
	AND32ItoR(vftemp, 1);  //Check sign
	pjmp = JZ8(0); //Skip if none are
	//SysPrintf("Invalid SQRT\n");
	OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag - Negative number sqrt
	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip); //So we do a cardinal sqrt
	x86SetJ8(pjmp);

	//SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip);

	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

	SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);
	SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
	_freeX86reg(vftemp);
	
}

void recVUMI_RSQRT(VURegs *VU, int info)
{
	int vftemp = ALLOCTEMPX86(MODE_8BITREG);
	u8* njmp;
	
	if( _Ftf_ ) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		_unpackVF_xyzw(EEREC_TEMP, EEREC_TEMP, _Ftf_);
	}
	else {
		SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
	}
	/* Check for negative divide */
	XOR32RtoR(vftemp, vftemp);
	SSE_MOVMSKPS_XMM_to_R32(vftemp, EEREC_TEMP);
	AND32ItoR(vftemp, 1);  //Check sign
	njmp = JZ8(0); //Skip if none are
	//SysPrintf("Invalid RSQRT\n");
	OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag - Negative number sqrt
	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip); //So we do a cardinal sqrt
	x86SetJ8(njmp);

	//SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)const_clip);

	if( _Fs_ == 0 ) {
		if( _Fsf_ == 3 ) {
			if(_Ft_ != 0 ||_Ftf_ == 3 )
			{
				//SysPrintf("_Fs_ = 0.3 _Ft_ != 0 || _Ft_ = 0.3 \n");
				SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);    //Dont use RSQRT, terrible accuracy
				SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
				//SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				_unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
				SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_Q, 0));

				
			} 
			else 
			{
				//SysPrintf("FS0.3 / 0!\n");
				SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff);
				_freeX86reg(vftemp);
				return;
			}
		}else {
			if(_Ft_ != 0 || _Ftf_ == 3) {
				//SysPrintf("FS = 0 FT != 0\n");
				//SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0x7f7fffff); // +MAX, no negative in here :p
				_freeX86reg(vftemp);
				return;
			} 
			else 
			{
				//SysPrintf("FS = 0 FT = 0!\n");
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag (only when 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0);
				_freeX86reg(vftemp);
				return;
			}
		}
	}
	else {
		int t1reg;
		if( _Ft_ == 0 ) {
			if( _Ftf_ < 3 ) {
				//SysPrintf("FS != 0 FT = 0!\n");
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); //Invalid Flag (only when 0/0)
				SSE_MOVMSKPS_XMM_to_R32(vftemp, EEREC_S);
				SHL32ItoR(vftemp, 31);  //Check sign				
				OR32ItoR(vftemp, 0x7f7fffff);
				MOV32RtoM(VU_VI_ADDR(REG_Q, 0), vftemp);
				_freeX86reg(vftemp);
				return;
			}else { 
				//SysPrintf("FS != 0 FT = 1!\n");
				OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); //Zero divide (only when not 0/0)
				MOV32ItoM(VU_VI_ADDR(REG_Q, 0), 0xff7fffff);
				_freeX86reg(vftemp);
				return;
			} 
			
		} 
		//SysPrintf("Normal RSQRT\n");
 
		SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
 
		t1reg = _vuGetTempXMMreg(info);
 
		if( t1reg >= 0 )
		{
			_unpackVFSS_xyzw(t1reg, EEREC_S, _Fsf_);
			SSE_DIVSS_XMM_to_XMM(t1reg, EEREC_TEMP);
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, t1reg);
			_freeXMMreg(t1reg);
		}
		else
		{
			SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
			SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_Q, 0));
		}
	}

	//if( !CHECK_FORCEABS ) {
		SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);
		SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
	//}

	
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
	_freeX86reg(vftemp);
}

void _addISIMMtoIT(VURegs *VU, s16 imm, int info)
{
	int fsreg = -1, ftreg;
	if (_Ft_ == 0) return;

	if( _Fs_ == 0 ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);
		MOV32ItoR(ftreg, imm&0xffff);
		return;
	}

	ADD_VI_NEEDED(_Ft_);
	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if (ftreg == fsreg) {
		if (imm != 0 ) {
			ADD16ItoR(ftreg, imm);
		}
	} else {
		if( imm ) {
			LEA32RtoR(ftreg, fsreg, imm);
			MOVZX32R16toR(ftreg, ftreg);
		}
		else MOV32RtoR(ftreg, fsreg);
	}
}

void recVUMI_IADDI(VURegs *VU, int info)
{
	s16 imm;

	if ( _Ft_ == 0 ) return;

	imm = ( VU->code >> 6 ) & 0x1f;
	imm = ( imm & 0x10 ? 0xfff0 : 0) | ( imm & 0xf );
	_addISIMMtoIT(VU, imm, info);
}

void recVUMI_IADDIU(VURegs *VU, int info)
{
	int imm;

	if ( _Ft_ == 0 ) return;

	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	_addISIMMtoIT(VU, imm, info);
}

void recVUMI_IADD( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;

	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	}

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
		}
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
		}
	}
	else {
		ADD_VI_NEEDED(_Ft_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) ADD32RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) ADD32RtoR(fdreg, fsreg);
		else LEA16RRtoR(fdreg, fsreg, ftreg);
		MOVZX32R16toR(fdreg, fdreg); // neeed since don't know if fdreg's upper bits are 0
	}
}

void recVUMI_IAND( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;
	
	if ( ( _Fs_ == 0 ) || ( _Ft_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	}

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	if( fdreg == fsreg ) AND16RtoR(fdreg, ftreg);
	else if( fdreg == ftreg ) AND16RtoR(fdreg, fsreg);
	else {
		MOV32RtoR(fdreg, ftreg);
		AND32RtoR(fdreg, fsreg);
	}
}

void recVUMI_IOR( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;

	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	} 

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
		}
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
		}
	}
	else
	{
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) OR16RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) OR16RtoR(fdreg, fsreg);
		else {
			MOV32RtoR(fdreg, fsreg);
			OR32RtoR(fdreg, ftreg);
		}
	}
}

void recVUMI_ISUB( VURegs *VU, int info )
{
   int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;

	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	} 
	
	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
		}
		NEG16R(fdreg);
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else {
			MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
		}
	}
	else
	{
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) SUB16RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) {
			SUB16RtoR(fdreg, fsreg);
			NEG16R(fdreg);
		}
		else {
			MOV32RtoR(fdreg, fsreg);
			SUB16RtoR(fdreg, ftreg);
		}
	}
}

void recVUMI_ISUBIU( VURegs *VU, int info )
{
	s16 imm;

	if ( _Ft_ == 0 ) return;

	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	imm = -imm;
	_addISIMMtoIT(VU, (u32)imm & 0xffff, info);
}

void recVUMI_MOVE( VURegs *VU, int info )
{	
	if (_Ft_ == 0) return;

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if( EEREC_T != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
	}
}

void recVUMI_MFIR( VURegs *VU, int info )
{
	static u32 s_temp;
	if ( _Ft_ == 0 ) return;

	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, 1);

	if( cpucaps.hasStreamingSIMD2Extensions ) {
		if( _XYZW_SS ) {
			SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Fs_, 1)-2);
			_vuFlipRegSS(VU, EEREC_T);
			SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
			SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
			_vuFlipRegSS(VU, EEREC_T);
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Fs_, 1)-2);
			SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
			VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		} else {
			SSE2_MOVD_M32_to_XMM(EEREC_T, VU_VI_ADDR(_Fs_, 1)-2);
			SSE2_PSRAD_I8_to_XMM(EEREC_T, 16);
			SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
		}
	}
	else {
		MOVSX32M16toR(EAX, VU_VI_ADDR(_Fs_, 1));
		MOV32RtoM((uptr)&s_temp, EAX);

		if( _X_Y_Z_W != 0xf ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&s_temp);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
			VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_M32_to_XMM(EEREC_T, (uptr)&s_temp);
			SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
		}
	}
}

void recVUMI_MTIR( VURegs *VU, int info )
{
	if ( _Ft_ == 0 ) return;

	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, 2);

	if( _Fsf_ == 0 ) {
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_Ft_, 0), EEREC_S);
	}
	else {
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_Ft_, 0), EEREC_TEMP);
	}

	AND32ItoM(VU_VI_ADDR(_Ft_, 0), 0xffff);
} 

void recVUMI_MR32( VURegs *VU, int info )
{	
	if (_Ft_ == 0) return;

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x39);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if( EEREC_T != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x39);
	}
}

// if x86reg < 0, reads directly from offset
void _loadEAX(VURegs *VU, int x86reg, uptr offset, int info)
{
    assert( offset < 0x80000000 );

	if( x86reg >= 0 ) {
		switch(_X_Y_Z_W) {
			case 3: // ZW
				SSE_MOVHPS_RmOffset_to_XMM(EEREC_T, x86reg, offset+8);
				break;
			case 6: // YZ
				SSE_SHUFPS_RmOffset_to_XMM(EEREC_T, x86reg, offset, 0x9c);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x78);
				break;

			case 8: // X
				SSE_MOVSS_RmOffset_to_XMM(EEREC_TEMP, x86reg, offset);
				SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
				break;
			case 9: // XW
				SSE_SHUFPS_RmOffset_to_XMM(EEREC_T, x86reg, offset, 0xc9);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xd2);
				break;
			case 12: // XY
				SSE_MOVLPS_RmOffset_to_XMM(EEREC_T, x86reg, offset);
				break;
			case 15:
				if( VU == &VU1 ) SSE_MOVAPSRmtoROffset(EEREC_T, x86reg, offset);
				else SSE_MOVUPSRmtoROffset(EEREC_T, x86reg, offset);
				break;
			default:
				if( VU == &VU1 ) SSE_MOVAPSRmtoROffset(EEREC_TEMP, x86reg, offset);
				else SSE_MOVUPSRmtoROffset(EEREC_TEMP, x86reg, offset);

				VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
				break;
		}
	}
	else {
		switch(_X_Y_Z_W) {
			case 3: // ZW
				SSE_MOVHPS_M64_to_XMM(EEREC_T, offset+8);
				break;
			case 6: // YZ
				SSE_SHUFPS_M128_to_XMM(EEREC_T, offset, 0x9c);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x78);
				break;
			case 8: // X
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, offset);
				SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
				break;
			case 9: // XW
				SSE_SHUFPS_M128_to_XMM(EEREC_T, offset, 0xc9);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xd2);
				break;
			case 12: // XY
				SSE_MOVLPS_M64_to_XMM(EEREC_T, offset);
				break;
			case 15:
				if( VU == &VU1 ) SSE_MOVAPS_M128_to_XMM(EEREC_T, offset);
				else SSE_MOVUPS_M128_to_XMM(EEREC_T, offset);
				break;
			default:
				if( VU == &VU1 ) SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
				else SSE_MOVUPS_M128_to_XMM(EEREC_TEMP, offset);
				VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
				break;
		}
	}
}

int recVUTransformAddr(int x86reg, VURegs* VU, int vireg, int imm)
{
	u8* pjmp[2];
	if( x86reg == EAX ) {
		if (imm) ADD32ItoR(x86reg, imm);
	}
	else {
		if( imm ) LEA32RtoR(EAX, x86reg, imm);
		else MOV32RtoR(EAX, x86reg);
	}
	
	if( VU == &VU1 ) {
		SHL32ItoR(EAX, 4);
		AND32ItoR(EAX, 0x3fff);
	}
	else {
		// if addr >= 4200, reads integers
		CMP32ItoR(EAX, 0x420);
		pjmp[0] = JL8(0);
		AND32ItoR(EAX, 0x1f);
		SHL32ItoR(EAX, 2);
		OR32ItoR(EAX, 0x4200);
		
		pjmp[1] = JMP8(0);
		x86SetJ8(pjmp[0]);
		SHL32ItoR(EAX, 4);
		AND32ItoR(EAX, 0xfff); // can be removed

		x86SetJ8(pjmp[1]);
	}

	return EAX;
}

void recVUMI_LQ(VURegs *VU, int info)
{
	s16 imm;

	if ( _Ft_ == 0 ) return;

	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff); 
	if (_Fs_ == 0) {
		_loadEAX(VU, -1, (uptr)GET_VU_MEM(VU, (u32)imm*16), info);
	} else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		_loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, imm), (uptr)VU->Mem, info);
	}
}

void recVUMI_LQD( VURegs *VU, int info )
{
	int fsreg;

	if ( _Fs_ != 0 ) {
		fsreg = ALLOCVI(_Fs_, MODE_READ|MODE_WRITE);
		SUB16ItoR( fsreg, 1 );
	}

	if ( _Ft_ == 0 ) return;

	if ( _Fs_ == 0 ) {
		_loadEAX(VU, -1, (uptr)VU->Mem, info);
	} else {
		_loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem, info);
	}
}

void recVUMI_LQI(VURegs *VU, int info)
{
	int fsreg;

	if ( _Ft_ == 0 ) {
		if( _Fs_ != 0 ) {
			if( (fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_WRITE|MODE_READ)) >= 0 ) {
				ADD16ItoR(fsreg, 1);
			}
			else {
				ADD16ItoM( VU_VI_ADDR( _Fs_, 0 ), 1 );
			}
		}
		return;
	}

    if (_Fs_ == 0) {
		_loadEAX(VU, -1, (uptr)VU->Mem, info);
    } else {
		fsreg = ALLOCVI(_Fs_, MODE_READ|MODE_WRITE);
		_loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem, info);
		ADD16ItoR( fsreg, 1 );
    }
}

void _saveEAX(VURegs *VU, int x86reg, uptr offset, int info)
{
	int t1reg;
	assert( offset < 0x80000000 );

	if( _Fs_ == 0 ) {
		if( _XYZW_SS ) {
			u32 c = _W ? 0x3f800000 : 0;
			if( x86reg >= 0 ) MOV32ItoRmOffset(x86reg, c, offset+(_W?12:(_Z?8:(_Y?4:0))));
			else MOV32ItoM(offset+(_W?12:(_Z?8:(_Y?4:0))), c);
		}
		else {
			int zeroreg = (x86reg == EAX) ? ALLOCTEMPX86(0) : EAX;

			XOR32RtoR(zeroreg, zeroreg);
			if( x86reg >= 0 ) {
				if( _X ) MOV32RtoRmOffset(x86reg, zeroreg, offset);
				if( _Y ) MOV32RtoRmOffset(x86reg, zeroreg, offset+4);
				if( _Z ) MOV32RtoRmOffset(x86reg, zeroreg, offset+8);
				if( _W ) MOV32ItoRmOffset(x86reg, 0x3f800000, offset+12);
			}
			else {
				if( _X ) MOV32RtoM(offset, zeroreg);
				if( _Y ) MOV32RtoM(offset+4, zeroreg);
				if( _Z ) MOV32RtoM(offset+8, zeroreg);
				if( _W ) MOV32ItoM(offset+12, 0x3f800000);
			}

			if( zeroreg != EAX ) _freeX86reg(zeroreg);
		}
		return;
	}

	switch(_X_Y_Z_W) {
		case 1: // W
			//SysPrintf("SAVE EAX W\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x27);
			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_S);
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x27);
			break;
		case 2: // Z
			//SysPrintf("SAVE EAX Z\n");
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+8);
			else SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			break;
		case 3: // ZW
			//SysPrintf("SAVE EAX ZW\n");
			if( x86reg >= 0 ) SSE_MOVHPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+8);
			else SSE_MOVHPS_XMM_to_M64(offset+8, EEREC_S);
			break;
		case 5: // YW
			//SysPrintf("SAVE EAX YW\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+4);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset+4, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			break;
		case 4: // Y
			//SysPrintf("SAVE EAX Y\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xe1);
			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset+4);
			else SSE_MOVSS_XMM_to_M32(offset+4, EEREC_S);
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xe1);
			break;
		case 6: // YZ
			//SysPrintf("SAVE EAX YZ\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xc9);
			if( x86reg >= 0 ) SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+4);
			else SSE_MOVLPS_XMM_to_M64(offset+4, EEREC_S);
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xd2);
			break;
		case 7: // YZW
			//SysPrintf("SAVE EAX YZW\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x39);
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+4);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset+4, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x93);
			break;
		case 8: // X
			//SysPrintf("SAVE EAX X\n");
			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			break;
		case 9: // XW
			//SysPrintf("SAVE EAX XW\n");
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			
			if( cpucaps.hasStreamingSIMD3Extensions ) SSE3_MOVSLDUP_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
			else SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x55);

			if( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);

			break;
		case 12: // XY
			//SysPrintf("SAVE EAX XY\n");
			if( x86reg >= 0 ) SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+0);
			else SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
			break;

		case 13: // XYW
			//SysPrintf("SAVE EAX XYW\n");
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB4); //ZWYX
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+0);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
				
			}
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB4);
			break;
		case 14: // XYZ
			//SysPrintf("SAVE EAX XYZ\n");
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+0);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+8);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			}
			break;
		case 15: // XYZW
			//SysPrintf("SAVE EAX XYZW\n");
			if( VU == &VU1 ) {
				if( x86reg >= 0 ) SSE_MOVAPSRtoRmOffset(x86reg, EEREC_S, offset+0);
				else SSE_MOVAPS_XMM_to_M128(offset, EEREC_S);
			}
			else {
				if( x86reg >= 0 ) SSE_MOVUPSRtoRmOffset(x86reg, EEREC_S, offset+0);
				else {
					if( offset & 15 ) SSE_MOVUPS_XMM_to_M128(offset, EEREC_S);
					else SSE_MOVAPS_XMM_to_M128(offset, EEREC_S);
				}
			}
			break;
		default:
			//SysPrintf("SAVEEAX Default %d\n", _X_Y_Z_W);
			// EEREC_D is a temp reg
			// find the first nonwrite reg
			t1reg = _vuGetTempXMMreg(info);

			if( t1reg < 0 ) {
				for(t1reg = 0; t1reg < XMMREGS; ++t1reg) {
					if( xmmregs[t1reg].inuse && !(xmmregs[t1reg].mode&MODE_WRITE) ) break;
				}

				if( t1reg == XMMREGS ) t1reg = -1;
				else {
					if( t1reg != EEREC_S ) _allocTempXMMreg(XMMT_FPS, t1reg);
				}
			}

			if( t1reg >= 0 ) {
				// found a temp reg
				if( VU == &VU1 ) {
					if( x86reg >= 0 ) SSE_MOVAPSRmtoROffset(EEREC_TEMP, x86reg, offset);
					else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
				}
				else {
					if( x86reg >= 0 ) SSE_MOVUPSRmtoROffset(EEREC_TEMP, x86reg, offset);
					else {
						if( offset & 15 ) SSE_MOVUPS_M128_to_XMM(EEREC_TEMP, offset);
						else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
					}
				}
				
				if( t1reg != EEREC_S ) SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);

				VU_MERGE_REGS(EEREC_TEMP, t1reg);

				if( VU == &VU1 ) {
					if( x86reg >= 0 ) SSE_MOVAPSRtoRmOffset(x86reg, EEREC_TEMP, offset);
					else SSE_MOVAPS_XMM_to_M128(offset, EEREC_TEMP);
				}
				else {
					if( x86reg >= 0 ) SSE_MOVUPSRtoRmOffset(x86reg, EEREC_TEMP, offset);
					else SSE_MOVUPS_XMM_to_M128(offset, EEREC_TEMP);
				}

				if( t1reg != EEREC_S ) _freeXMMreg(t1reg);
				else {
					// read back the data
					SSE_MOVAPS_M128_to_XMM(EEREC_S, (uptr)&VU->VF[_Fs_]);
				}
			}
			else {
				// do it with one reg
				SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);

				if( VU == &VU1 ) {
					if( x86reg >= 0 ) SSE_MOVAPSRmtoROffset(EEREC_TEMP, x86reg, offset);
					else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
				}
				else {
					if( x86reg >= 0 ) SSE_MOVUPSRmtoROffset(EEREC_TEMP, x86reg, offset);
					else {
						if( offset & 15 ) SSE_MOVUPS_M128_to_XMM(EEREC_TEMP, offset);
						else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
					}
				}

				VU_MERGE_REGS(EEREC_TEMP, EEREC_S);

				if( VU == &VU1 ) {
					if( x86reg >= 0 ) SSE_MOVAPSRtoRmOffset(x86reg, EEREC_TEMP, offset);
					else SSE_MOVAPS_XMM_to_M128(offset, EEREC_TEMP);
				}
				else {
					if( x86reg >= 0 ) SSE_MOVUPSRtoRmOffset(x86reg, EEREC_TEMP, offset);
					else {
						if( offset & 15 ) SSE_MOVUPS_XMM_to_M128(offset, EEREC_TEMP);
						else SSE_MOVAPS_XMM_to_M128(offset, EEREC_TEMP);
					}
				}

				// read back the data
				SSE_MOVAPS_M128_to_XMM(EEREC_S, (uptr)&VU->VF[_Fs_]);
			}

			break;
	}
}

void recVUMI_SQ(VURegs *VU, int info)
{
	s16 imm;
 
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if ( _Ft_ == 0 ) {
		_saveEAX(VU, -1, (uptr)GET_VU_MEM(VU, (int)imm * 16), info);
	}
	else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ);
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, imm), (uptr)VU->Mem, info);
	}
}

void recVUMI_SQD(VURegs *VU, int info)
{
	if (_Ft_ == 0) {
		_saveEAX(VU, -1, (uptr)VU->Mem, info);
	} else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ|MODE_WRITE);
		SUB16ItoR( ftreg, 1 );
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, 0), (uptr)VU->Mem, info);
	}
}

void recVUMI_SQI(VURegs *VU, int info)
{
	if (_Ft_ == 0) {
		_saveEAX(VU, -1, (uptr)VU->Mem, info);
	} else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ|MODE_WRITE);
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, 0), (uptr)VU->Mem, info);

		ADD16ItoR( ftreg, 1 );
	}
}

void recVUMI_ILW(VURegs *VU, int info)
{
	int ftreg;
	s16 imm, off;
 
	if ( _Ft_ == 0 ) return;

	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff);
	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if ( _Fs_ == 0 ) {
		MOVZX32M16toR( ftreg, (uptr)GET_VU_MEM(VU, (int)imm * 16 + off) );
	}
	else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		MOV32RmtoROffset(ftreg, recVUTransformAddr(fsreg, VU, _Fs_, imm), (uptr)VU->Mem + off);
	}
}

void recVUMI_ISW( VURegs *VU, int info )
{
	s16 imm;
	
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 

	if (_Fs_ == 0) {
		uptr off = (uptr)GET_VU_MEM(VU, (int)imm * 16);
		int ftreg = ALLOCVI(_Ft_, MODE_READ);

		if (_X) MOV32RtoM(off, ftreg);
		if (_Y) MOV32RtoM(off+4, ftreg);
		if (_Z) MOV32RtoM(off+8, ftreg);
		if (_W) MOV32RtoM(off+12, ftreg);
	}
	else {
		int x86reg, fsreg, ftreg;

		ADD_VI_NEEDED(_Ft_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		x86reg = recVUTransformAddr(fsreg, VU, _Fs_, imm);

		if (_X) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+12);
	}
}

void recVUMI_ILWR( VURegs *VU, int info )
{
	int off, ftreg;

	if ( _Ft_ == 0 ) return;

	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if ( _Fs_ == 0 ) {
		MOVZX32M16toR( ftreg, (uptr)VU->Mem + off );
	}
	else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		MOVZX32Rm16toROffset(ftreg, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem + off);
	}
}

void recVUMI_ISWR( VURegs *VU, int info )
{
	int ftreg;
	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	if (_Fs_ == 0) {
		if (_X) MOV32RtoM((uptr)VU->Mem, ftreg);
		if (_Y) MOV32RtoM((uptr)VU->Mem+4, ftreg);
		if (_Z) MOV32RtoM((uptr)VU->Mem+8, ftreg);
		if (_W) MOV32RtoM((uptr)VU->Mem+12, ftreg);
	}
	else {
		int x86reg;
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		x86reg = recVUTransformAddr(fsreg, VU, _Fs_, 0);

		if (_X) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+12);
	}
}

void recVUMI_RINIT(VURegs *VU, int info)
{
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {

		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 2);

		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)s_mask);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)s_fones);
		SSE_MOVSS_XMM_to_M32(VU_REGR_ADDR, EEREC_TEMP);
	}
	else {
		int rreg = ALLOCVI(REG_R, MODE_WRITE);

		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		MOV32MtoR( rreg, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );

		AND32ItoR( rreg, 0x7fffff );
		OR32ItoR( rreg, 0x7f << 23 );
	}
}

void recVUMI_RGET(VURegs *VU, int info)
{
	if ( _Ft_ == 0 ) return;

	_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1);

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_REGR_ADDR);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_T, VU_REGR_ADDR);
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
	}
}

void recVUMI_RNEXT( VURegs *VU, int info )
{
	int rreg, x86temp0, x86temp1;
	if ( _Ft_ == 0) return;

	rreg = ALLOCVI(REG_R, MODE_WRITE|MODE_READ);

	x86temp0 = ALLOCTEMPX86(0);
	x86temp1 = ALLOCTEMPX86(0);

	// code from www.project-fao.org
	MOV32MtoR(rreg, VU_REGR_ADDR);
	MOV32RtoR(x86temp0, rreg);
	SHR32ItoR(x86temp0, 4);
	AND32ItoR(x86temp0, 1);

	MOV32RtoR(x86temp1, rreg);
	SHR32ItoR(x86temp1, 22);
	AND32ItoR(x86temp1, 1);

	SHL32ItoR(rreg, 1);
	XOR32RtoR(x86temp0, x86temp1);
	XOR32RtoR(rreg, x86temp0);
	AND32ItoR(rreg, 0x7fffff);
	OR32ItoR(rreg, 0x3f800000);

	_freeX86reg(x86temp0);
	_freeX86reg(x86temp1);

	recVUMI_RGET(VU, info);
}

void recVUMI_RXOR( VURegs *VU, int info )
{
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode & MODE_NOFLUSH) ) {
		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1);
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		SSE_XORPS_M128_to_XMM(EEREC_TEMP, VU_REGR_ADDR);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (u32)s_mask);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (u32)s_fones);
		SSE_MOVSS_XMM_to_M32(VU_REGR_ADDR, EEREC_TEMP);
	}
	else {
		int rreg = ALLOCVI(REG_R, MODE_WRITE|MODE_READ);

		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		XOR32MtoR( rreg, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );
		AND32ItoR( rreg, 0x7fffff );
		OR32ItoR ( rreg, 0x3f800000 );
	}
} 

void recVUMI_WAITQ( VURegs *VU, int info )
{
//	if( info & PROCESS_VU_SUPER ) {
//		//CALLFunc(waitqfn);
//		SuperVUFlush(0, 1);
//	}
}

void recVUMI_FSAND( VURegs *VU, int info )
{
	int ftreg;
	u16 imm;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_Ft_ == 0) return; 

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOV32MtoR(ftreg, VU_VI_ADDR(REG_STATUS_FLAG, 1));
	AND32ItoR( ftreg, 0xFF&imm ); // yes 0xff not 0xfff since only first 8 bits are valid!
}

void recVUMI_FSEQ( VURegs *VU, int info )
{
	int ftreg;
	u32 imm;
	if ( _Ft_ == 0 ) return;

	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOVZX32M8toR( EAX, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	XOR32RtoR(ftreg, ftreg);

	CMP16ItoR(EAX, imm);
	SETE8R(ftreg);
}

void recVUMI_FSOR( VURegs *VU, int info )
{
	int ftreg;
	u32 imm;
	if(_Ft_ == 0) return; 
	
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOVZX32M8toR( ftreg, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	OR32ItoR( ftreg, imm );
}

void recVUMI_FSSET(VURegs *VU, int info)
{
	u32 writeaddr = VU_VI_ADDR(REG_STATUS_FLAG, 0);
	u32 prevaddr = VU_VI_ADDR(REG_STATUS_FLAG, 2);

	u16 imm = 0;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7FF);

    // keep the low 6 bits ONLY if the upper instruction is an fmac instruction (otherwise rewrite) - metal gear solid 3
//    if( (info & PROCESS_VU_SUPER) && VUREC_FMAC ) {
//        MOV32MtoR(EAX, prevaddr);
//	    AND32ItoR(EAX, 0x3f);
//	    if ((imm&0xfc0) != 0) OR32ItoR(EAX, imm & 0xFC0);
//        MOV32RtoM(writeaddr ? writeaddr : prevaddr, EAX);
//    }
//    else {
        MOV32ItoM(writeaddr ? writeaddr : prevaddr, imm&0xfc0);
//    }
}

void recVUMI_FMAND( VURegs *VU, int info )
{
	int fsreg, ftreg;
	if ( _Ft_ == 0 ) return;

	fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_8BITREG);

	if( fsreg >= 0 ) {
		if( ftreg != fsreg ) MOV32RtoR(ftreg, fsreg);
	}
	else MOV8MtoR(ftreg, VU_VI_ADDR(_Fs_, 1));

	//AND16MtoR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
	AND8MtoR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
	MOVZX32R8toR(ftreg, ftreg);
}

void recVUMI_FMEQ( VURegs *VU, int info )
{
	int ftreg, fsreg;
	if ( _Ft_ == 0 ) return;

	if( _Ft_ == _Fs_ ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_READ|MODE_8BITREG);
		// really 8 since not doing under/over flows
		CMP8MtoR(ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(EAX);
		MOVZX32R8toR(ftreg, EAX);
	}
	else {
		ADD_VI_NEEDED(_Fs_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_8BITREG);

		XOR32RtoR(ftreg, ftreg);
		
		CMP8MtoR(fsreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(ftreg);
	}
}

void recVUMI_FMOR( VURegs *VU, int info )
{
	int fsreg, ftreg;
	if ( _Ft_ == 0 ) return;

	if( _Fs_ == 0 ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_8BITREG);
		MOVZX32M8toR(ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
	}
	if( _Ft_ == _Fs_ ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_READ|MODE_8BITREG);
		OR8MtoR(ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
	}
	else {
		fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);

		MOVZX32M8toR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));

		if( fsreg >= 0 ) {
			OR16RtoR( ftreg, fsreg);
		}
		else {
			OR16MtoR( ftreg, VU_VI_ADDR(_Fs_, 1));
		}
	}
}

void recVUMI_FCAND( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);

	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	XOR32RtoR(ftreg, ftreg);
	AND32ItoR( EAX, VU->code & 0xFFFFFF );
	SETNZ8R(ftreg);
}

void recVUMI_FCEQ( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);

	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	//AND32ItoR( EAX, 0xffffff);
	XOR32RtoR(ftreg, ftreg);
	CMP32ItoR( EAX, VU->code&0xffffff );
	SETE8R(ftreg);
}

void recVUMI_FCOR( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);

	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	//AND32ItoR( EAX, 0xffffff);
	XOR32RtoR(ftreg, ftreg);
	OR32ItoR( EAX, VU->code | 0xff000000 );
	ADD32ItoR(EAX, 1);

	// set to 1 if EAX is 0
	SETZ8R(ftreg);
}

void recVUMI_FCSET( VURegs *VU, int info )
{
	u32 addr = VU_VI_ADDR(REG_CLIP_FLAG, 0);

	MOV32ItoM(addr ? addr : VU_VI_ADDR(REG_CLIP_FLAG, 2), VU->code&0xffffff );

	if( !(info & (PROCESS_VU_SUPER|PROCESS_VU_COP2)) )
		MOV32ItoM( VU_VI_ADDR(REG_CLIP_FLAG, 1), VU->code&0xffffff );
}

void recVUMI_FCGET( VURegs *VU, int info )
{
	int ftreg;
	if(_Ft_ == 0) return;

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOV32MtoR(ftreg, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	AND32ItoR(ftreg, 0x0fff);
}

// SuperVU branch fns are in ivuzerorec.cpp
static s32 _recbranchAddr(VURegs * VU)
{
	bpc = pc + (_Imm11_ << 3); 
	if (bpc < 0) {
		bpc = pc + (_UImm11_ << 3); 
	}
	if (VU == &VU1) {
		bpc&= 0x3fff;
	} else {
		bpc&= 0x0fff;
	}

	return bpc;
}

void recVUMI_IBEQ(VURegs *VU, int info)
{
	int fsreg, ftreg;
	ADD_VI_NEEDED(_Ft_);
	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	bpc = _recbranchAddr(VU); 

	CMP16RtoR( fsreg, ftreg );
	j8Ptr[ 0 ] = JNE8( 0 );

	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBGEZ( VURegs *VU, int info )
{
	int fsreg = ALLOCVI(_Fs_, MODE_READ);
	
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( VU_VI_ADDR(REG_TPC, 0), (int)pc );
	CMP16ItoR( fsreg, 0x0 );
	j8Ptr[ 0 ] = JL8( 0 );

	// supervu will take care of the rest
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBGTZ( VURegs *VU, int info )
{
	int fsreg = ALLOCVI(_Fs_, MODE_READ);
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( VU_VI_ADDR(REG_TPC, 0), (int)pc );
	CMP16ItoR( fsreg, 0x0 );
	j8Ptr[ 0 ] = JLE8( 0 );

	// supervu will take care of the rest
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBLEZ( VURegs *VU, int info )
{
	int fsreg = ALLOCVI(_Fs_, MODE_READ);

	bpc = _recbranchAddr(VU); 

	MOV16ItoM( VU_VI_ADDR(REG_TPC, 0), (int)pc );
	CMP16ItoR( fsreg, 0x0 );
	j8Ptr[ 0 ] = JG8( 0 );

	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBLTZ( VURegs *VU, int info )
{
	int fsreg = ALLOCVI(_Fs_, MODE_READ);

	bpc = _recbranchAddr(VU); 

	MOV16ItoM( VU_VI_ADDR(REG_TPC, 0), (int)pc );
	CMP16ItoR( fsreg, 0x0 );
	j8Ptr[ 0 ] = JGE8( 0 );

	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );
    
	branch |= 1;
}

void recVUMI_IBNE( VURegs *VU, int info )
{
	int fsreg, ftreg;
	ADD_VI_NEEDED(_Ft_);
	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	bpc = _recbranchAddr(VU); 

	MOV16ItoM( VU_VI_ADDR(REG_TPC, 0), (int)pc );
	CMP16RtoR( fsreg, ftreg );
	j8Ptr[ 0 ] = JE8( 0 );

	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );
    
	branch |= 1;
}

void recVUMI_B(VURegs *VU, int info)
{
	// supervu will take care of the rest
	bpc = _recbranchAddr(VU); 

	MOV32ItoM(VU_VI_ADDR(REG_TPC, 0), bpc);

	branch |= 3;
}

void recVUMI_BAL( VURegs *VU, int info )
{
	bpc = _recbranchAddr(VU); 

	MOV32ItoM(VU_VI_ADDR(REG_TPC, 0), bpc);

	if ( _Ft_ ) {
		int ftreg = ALLOCVI(_Ft_, MODE_WRITE);
		MOV16ItoR( ftreg, (pc+8)>>3 );
	}

	branch |= 3;
}

void recVUMI_JR(VURegs *VU, int info)
{
	// fsreg cannot be ESP
	int fsreg = ALLOCVI(_Fs_, MODE_READ);
	LEA32RStoR(EAX, fsreg, 3);
	MOV32RtoM(VU_VI_ADDR(REG_TPC, 0), EAX);

	branch |= 3;
}

void recVUMI_JALR(VURegs *VU, int info)
{
	// fsreg cannot be ESP
	int fsreg = ALLOCVI(_Fs_, MODE_READ);
	LEA32RStoR(EAX, fsreg, 3);
	MOV32RtoM(VU_VI_ADDR(REG_TPC, 0), EAX);

	if ( _Ft_ ) {
		int ftreg = ALLOCVI(_Ft_, MODE_WRITE);
		MOV16ItoR( ftreg, (pc+8)>>3 );
	}

	branch |= 3;
}

void recVUMI_MFP(VURegs *VU, int info)
{
	if (_Ft_ == 0) return; 

	if( _XYZW_SS ) {
		_vuFlipRegSS(VU, EEREC_T);
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 1));
		SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		_vuFlipRegSS(VU, EEREC_T);
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 1));
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_T, VU_VI_ADDR(REG_P, 1));
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
	}
}

static PCSX2_ALIGNED16(float s_tempmem[4]);

void recVUMI_WAITP(VURegs *VU, int info)
{
//	if( info & PROCESS_VU_SUPER )
//		SuperVUFlush(1, 1);
}

// in all EFU insts, EEREC_D is a temp reg
void vuSqSumXYZ(int regd, int regs, int regtemp)
{
	if( cpucaps.hasStreamingSIMD4Extensions )
	{
		SSE_MOVAPS_XMM_to_XMM(regd, regs);
		SSE4_DPPS_XMM_to_XMM(regd, regd, 0x71);
	}
	else 
	{
		SSE_MOVAPS_XMM_to_XMM(regtemp, regs);
		SSE_MULPS_XMM_to_XMM(regtemp, regtemp);

		if( cpucaps.hasStreamingSIMD3Extensions ) {
			SSE3_HADDPS_XMM_to_XMM(regd, regtemp);
			SSE_ADDPS_XMM_to_XMM(regd, regtemp); // regd.z = x+y+z
			SSE_MOVHLPS_XMM_to_XMM(regd, regd); // move to x
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, regtemp);
			SSE_SHUFPS_XMM_to_XMM(regtemp, regtemp, 0xE1);
			SSE_ADDSS_XMM_to_XMM(regd, regtemp);
			SSE_SHUFPS_XMM_to_XMM(regtemp, regtemp, 0xD2);
			SSE_ADDSS_XMM_to_XMM(regd, regtemp);
			//SSE_SHUFPS_XMM_to_XMM(regtemp, regtemp, 0xC6);
		}
	}
	
	//SysPrintf("SUMXYZ\n");
}

void recVUMI_ESADD( VURegs *VU, int info)
{
	assert( VU == &VU1 );
	//SysPrintf("ESADD\n");
	if( cpucaps.hasStreamingSIMD4Extensions )
	{
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE4_DPPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x71);
	}
	else 
	{
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

		if( cpucaps.hasStreamingSIMD3Extensions ) {
			SSE3_HADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP); // EEREC_D.z = x+y+z
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_D); // move to x
		}
		else {
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x55);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
	}

	CheckForOverflowSS_(EEREC_TEMP, EEREC_D);
	//SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
	//SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_ERSADD( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	// almost same as vuSqSumXYZ

	if( cpucaps.hasStreamingSIMD4Extensions )
	{
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE4_DPPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x71);
	}
	else 
	{
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

		if( cpucaps.hasStreamingSIMD3Extensions ) {
			SSE3_HADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP); // EEREC_D.z = x+y+z
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_D); // move to x
		}
		else {
			SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x55);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
	}

	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);

	// don't use RCPSS (very bad precision)
	SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	CheckForOverflowSS_(EEREC_TEMP, EEREC_D);
	//SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
	//SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_ELENG( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP);
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_ERLENG( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP); //Dont want to use EEREC_D incase it overwrites something
	//SysPrintf("ERLENG\n");
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);
	SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 0));
	//SSE_RSQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	//CheckForOverflowSS_(EEREC_TEMP, EEREC_D);
	//SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
	//SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);
	
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_EATANxy( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		SSE_MOVLPS_XMM_to_M64((u32)s_tempmem, EEREC_S);
		FLD32((uptr)&s_tempmem[0]);
		FLD32((uptr)&s_tempmem[1]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[0]);
		FLD32((uptr)&VU->VF[_Fs_].UL[1]);
	}

	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}

void recVUMI_EATANxz( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		SSE_MOVLPS_XMM_to_M64((u32)s_tempmem, EEREC_S);
		FLD32((uptr)&s_tempmem[0]);
		FLD32((uptr)&s_tempmem[2]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[0]);
		FLD32((uptr)&VU->VF[_Fs_].UL[2]);
	}
	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}

void recVUMI_ESUM( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( cpucaps.hasStreamingSIMD3Extensions ) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE3_HADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
		SSE3_HADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
	}
	else {
		SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); // y+w, x+z
		SSE_UNPCKLPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // y+w, y+w, x+z, x+z
		SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		SSE_ADDSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	}

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_ERCPR( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);

	// don't use RCPSS (very bad precision)
	if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
		if( _Fsf_ ) SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, (0xe4e4>>(2*_Fsf_))&0xff);
		SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);

		// revert
		if( _Fsf_ ) SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, (0xe4e4>>(8-2*_Fsf_))&0xff);
	}
	else {
		SSE_DIVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}

	CheckForOverflowSS_(EEREC_TEMP, EEREC_D);
	//SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]);
	//SSE_MAXSS_M32_to_XMM(EEREC_TEMP, (uptr)&g_minvals[0]);
	
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

void recVUMI_ESQRT( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( _Fsf_ ) {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
			SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
		}
		else {
			SSE_SQRTSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Fs_].UL[_Fsf_]);
		}
	}
	else SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}

#if defined(_MSC_VER) && !defined(__x86_64__)

static u32 s_saveecx, s_saveedx, s_saveebx, s_saveesi, s_saveedi, s_saveebp;
float tempsqrt = 0;
extern float vuDouble(u32 f);
__declspec(naked) void tempERSQRT()
{
    __asm {
		mov s_saveecx, ecx
		mov s_saveedx, edx
		mov s_saveebx, ebx
		mov s_saveesi, esi
		mov s_saveedi, edi
		mov s_saveebp, ebp
	}

	if (tempsqrt >= 0) {
		tempsqrt = fpusqrtf(tempsqrt);
		if (tempsqrt) {
			tempsqrt = 1.0f / tempsqrt; 
		}
        tempsqrt = vuDouble(*(u32*)&tempsqrt);
	}

    __asm {
		mov ecx, s_saveecx
		mov edx, s_saveedx
		mov ebx, s_saveebx
		mov esi, s_saveesi
		mov edi, s_saveedi
		mov ebp, s_saveebp
		ret
	}
}
#endif

void recVUMI_ERSQRT( VURegs *VU, int info )
{
		int t1reg;
 
	assert( VU == &VU1 );
 
//    if( _Fsf_ ) {
//		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
//			_unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);        
//            SSE_MOVSS_XMM_to_M32((uptr)&tempsqrt, EEREC_TEMP);
//		}
//		else {
//            MOV32MtoR(EAX, (uptr)&VU->VF[_Fs_].UL[_Fsf_]);
//            MOV32RtoM((uptr)&tempsqrt, EAX);
//		}
//	}
//    else {
//        SSE_MOVSS_XMM_to_M32((uptr)&tempsqrt, EEREC_S);
//    }
//
//    
//    CALLFunc((uptr)tempERSQRT);
//    MOV32MtoR(EAX, (uptr)&tempsqrt);
//    MOV32RtoM(VU_VI_ADDR(REG_P, 0), EAX);
/*
    // need to explicitly check for 0 (naruto ultimate ninja)
	if( _Fsf_ ) {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			//int t0reg = _allocTempXMMreg(XMMT_FPS, -1);
			_unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
            //SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
            //SSE_CMPNESS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			SysPrintf("ERSQRT\n");
			SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
			SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);
			SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 0));
            //SSE_ANDPS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
		}
		else {
            //SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
            //CMP32ItoM((uptr)&VU->VF[_Fs_].UL[_Fsf_], 0);
            //j8Ptr[0] = JE8(0);
			SysPrintf("ERSQRT2\n");
			SSE_RSQRTSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Fs_].UL[_Fsf_]);
            //x86SetJ8(j8Ptr[0]);
		}
	}
    else {
		SysPrintf("ERSQRT3\n");
        SSE_RSQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
        //SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
        //SSE_CMPNESS_XMM_to_XMM(EEREC_D, EEREC_S);
        //SSE_ANDPS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
    }

*/
	//SysPrintf("ERSQRT\n");
	if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
		if( _Fsf_ ) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		}
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}
 
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
 
	t1reg = _vuGetTempXMMreg(info);
 
	if( t1reg >= 0 )
	{
		SSE_MOVSS_M32_to_XMM(t1reg, (uptr)&VU->VF[0].UL[3]);
		SSE_DIVSS_XMM_to_XMM(t1reg, EEREC_TEMP);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), t1reg);
 
		_freeXMMreg(t1reg);
	}
	else
	{
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[3]);
		SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 0));
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
	}
}

void recVUMI_ESIN( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((u32)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((u32)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FLD32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}

	FSIN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}

void recVUMI_EATAN( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((u32)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((u32)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FLD32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}
	}

	FLD1();
	FLD32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}

void recVUMI_EEXP( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	FLDL2E();

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((u32)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((u32)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FMUL32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FMUL32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}

	// basically do 2^(log_2(e) * val)
	FLD(0);
	FRNDINT();
	FXCH(1);
	FSUB32Rto0(1);
	F2XM1();
	FLD1();
	FADD320toR(1);
	FSCALE();
	FSTP(1);
	
	FSTP32(VU_VI_ADDR(REG_P, 0));
}

void recVUMI_XITOP( VURegs *VU, int info )
{
	int ftreg;
	if (_Ft_ == 0) return;
	
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOVZX32M16toR( ftreg, (uptr)&VU->vifRegs->itop );
}

void recVUMI_XTOP( VURegs *VU, int info )
{
	int ftreg;
	if ( _Ft_ == 0 ) return;

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOVZX32M16toR( ftreg, (uptr)&VU->vifRegs->top );
}

#if defined(_WIN32) && !defined(WIN32_PTHREADS)
extern HANDLE g_hGsEvent;
#else
extern pthread_cond_t g_condGsEvent;
#endif

void VU1XGKICK_MTGSTransfer(u32 *pMem, u32 addr)
{
	u32 size;
    u8* pmem;
	u32* data = (u32*)((u8*)pMem + (addr&0x3fff));

	static int scount = 0;
	++scount;

	size = GSgifTransferDummy(0, data, (0x4000-(addr&0x3fff))>>4);

	size = 0x4000-(size<<4)-(addr&0x3fff);
    assert( size >= 0 );

    // can't exceed 0x4000
//    left = addr+size-0x4000;
//    if( left > 0 ) size -= left;
    if( size > 0 ) {

	    pmem = GSRingBufCopy(NULL, size, GS_RINGTYPE_P1);
	    assert( pmem != NULL );
		FreezeMMXRegs(1);
	    memcpy_fast(pmem, (u8*)pMem+addr, size);
		FreezeMMXRegs(0);
    //    if( left > 0 ) {
    //        memcpy_fast(pmem+size-left, (u8*)pMem, left);
    //    }
	    GSRINGBUF_DONECOPY(pmem, size);

        if( !CHECK_DUALCORE ) {
#if defined(_WIN32) && !defined(WIN32_PTHREADS)
            SetEvent(g_hGsEvent);
#else
            pthread_cond_signal(&g_condGsEvent);
#endif
        }
	}
}

void recVUMI_XGKICK( VURegs *VU, int info )
{
	int fsreg = ALLOCVI(_Fs_, MODE_READ);
	_freeX86reg(fsreg);
	SHL32ItoR(fsreg, 4);
	AND32ItoR(fsreg, 0x3fff);

    iFlushCall(FLUSH_NOCONST);
        
	_callPushArg(MEM_X86TAG, fsreg, X86ARG2);
	_callPushArg(MEM_CONSTTAG, (uptr)VU->Mem, X86ARG1);

	if( CHECK_MULTIGS ) {
		CALLFunc((uptr)VU1XGKICK_MTGSTransfer);
#ifndef __x86_64__
		ADD32ItoR(ESP, 8);
#endif
	}
	else {
		FreezeMMXRegs(1);
		//FreezeXMMRegs(1);
		CALLFunc((uptr)GSgifTransfer1);
		FreezeMMXRegs(0);
		//FreezeXMMRegs(0);
	}
}

#endif // PCSX2_NORECBUILD
