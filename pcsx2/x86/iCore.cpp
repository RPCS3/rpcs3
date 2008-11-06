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
#include <string.h>
#include <assert.h>
#include <malloc.h>

extern "C" {

#include "PS2Etypes.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include "System.h"
#include "R5900.h"
#include "Vif.h"
#include "VU.h"
#include "ix86/ix86.h"
#include "iCore.h"
#include "R3000A.h"

u16 g_x86AllocCounter = 0;
u16 g_xmmAllocCounter = 0;

EEINST* g_pCurInstInfo = NULL;

u32 g_cpuRegHasLive1 = 0, g_cpuPrevRegHasLive1 = 0; // set if upper 32 bits are live
u32 g_cpuRegHasSignExt = 0, g_cpuPrevRegHasSignExt = 0; // set if upper 32 bits are the sign extension of the lower integer

// used to make sure regs don't get changed while in recompiler
// use FreezeMMXRegs, FreezeXMMRegs
u8 g_globalXMMSaved = 0;
u32 g_recWriteback = 0;

#ifdef _DEBUG
char g_globalXMMLocked = 0;
#endif

_xmmregs xmmregs[XMMREGS], s_saveXMMregs[XMMREGS];
PCSX2_ALIGNED16(u64 g_globalXMMData[2*XMMREGS];)

// X86 caching
_x86regs x86regs[X86REGS], s_saveX86regs[X86REGS];

} // end extern "C"

#include <vector>
using namespace std;

//void _eeSetLoadStoreReg(int gprreg, u32 offset, int x86reg)
//{
//	int regs[2] = {ESI, EDI};
//
//	int i = _checkX86reg(X86TYPE_MEMOFFSET, gprreg, MODE_WRITE);
//	if( i < 0 ) {
//		for(i = 0; i < 2; ++i) {
//			if( !x86regs[regs[i]].inuse ) break;
//		}
//
//		assert( i < 2 );
//		i = regs[i];
//	}
//
//	if( i != x86reg ) MOV32RtoR(x86reg, i);
//	x86regs[i].extra = offset;
//}

//int _eeGeLoadStoreReg(int gprreg, int* poffset)
//{
//	int i = _checkX86reg(X86TYPE_MEMOFFSET, gprreg, MODE_READ);
//	if( i >= 0 ) return -1;
//
//	if( poffset ) *poffset = x86regs[i].extra;
//	return i;
//}

// XMM Caching
#define VU_VFx_ADDR(x)  (uptr)&VU->VF[x].UL[0]
#define VU_ACCx_ADDR    (uptr)&VU->ACC.UL[0]

static int s_xmmchecknext = 0;

void _initXMMregs() {
	memset(xmmregs, 0, sizeof(xmmregs));
	g_xmmAllocCounter = 0;
	s_xmmchecknext = 0;
}

__forceinline void* _XMMGetAddr(int type, int reg, VURegs *VU)
{
	if (type == XMMTYPE_VFREG ) return (void*)VU_VFx_ADDR(reg);
	else if (type == XMMTYPE_ACC ) return (void*)VU_ACCx_ADDR;
	else if (type == XMMTYPE_GPRREG) {
		if( reg < 32 )
			assert( !(g_cpuHasConstReg & (1<<reg)) || (g_cpuFlushedConstReg & (1<<reg)) );
		return &cpuRegs.GPR.r[reg].UL[0];
	}
	else if (type == XMMTYPE_FPREG ) return &fpuRegs.fpr[reg];
	else if (type == XMMTYPE_FPACC ) return &fpuRegs.ACC.f;

	assert(0);
	return NULL;
}

int  _getFreeXMMreg()
{
	int i, tempi;
	u32 bestcount = 0x10000;
	if( !cpucaps.hasStreamingSIMDExtensions ) return -1;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[(i+s_xmmchecknext)%XMMREGS].inuse == 0) {
			int ret = (s_xmmchecknext+i)%XMMREGS;
			s_xmmchecknext = (s_xmmchecknext+i+1)%XMMREGS;
			return ret;
		}
	}

	// check for dead regs
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			if( !(g_pCurInstInfo->regs[xmmregs[i].reg] & (EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)) ) {
				_freeXMMreg(i);
				return i;
			}
		}
	}

	// check for future xmm usage
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			if( !(g_pCurInstInfo->regs[xmmregs[i].reg] & EEINST_XMM) ) {
				_freeXMMreg(i);
				return i;
			}
		}
	}

	tempi = -1;
	bestcount = 0xffff;
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type != XMMTYPE_TEMP) {

			if( xmmregs[i].counter < bestcount ) {
				tempi = i;
				bestcount = xmmregs[i].counter;
			}
			continue;
		}

		_freeXMMreg(i);
		return i;
	}

	if( tempi != -1 ) {
		_freeXMMreg(tempi);
		return tempi;
	}
	SysPrintf("*PCSX2*: VUrec ERROR\n");

	return -1;
}

int _allocTempXMMreg(XMMSSEType type, int xmmreg) {
	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}
	else {
		_freeXMMreg(xmmreg);
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_TEMP;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;
	g_xmmtypes[xmmreg] = type;

	return xmmreg;
}

int _allocVFtoXMMreg(VURegs *VU, int xmmreg, int vfreg, int mode) {
	int i;
	int readfromreg = -1;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0 || xmmregs[i].type != XMMTYPE_VFREG || xmmregs[i].reg != vfreg || xmmregs[i].VU != XMM_CONV_VU(VU) ) continue;

		if( xmmreg >= 0 ) {
			// requested specific reg, so return that instead
			if( i != xmmreg ) {
				if( xmmregs[i].mode & MODE_READ ) readfromreg = i;
				//if( xmmregs[i].mode & MODE_WRITE ) mode |= MODE_WRITE;
				mode |= xmmregs[i].mode&MODE_WRITE;
				xmmregs[i].inuse = 0;
				break;
			}
		}

		xmmregs[i].needed = 1;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ) ) {
			SSE_MOVAPS_M128_to_XMM(i, VU_VFx_ADDR(vfreg));
			xmmregs[i].mode |= MODE_READ;
		}

		g_xmmtypes[i] = XMMT_FPS;
		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}
	else {
		_freeXMMreg(xmmreg);
	}

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_VFREG;
	xmmregs[xmmreg].reg = vfreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].VU = XMM_CONV_VU(VU);
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;
	if (mode & MODE_READ) {
		if( readfromreg >= 0 ) SSE_MOVAPS_XMM_to_XMM(xmmreg, readfromreg);
		else SSE_MOVAPS_M128_to_XMM(xmmreg, VU_VFx_ADDR(xmmregs[xmmreg].reg));
	}

	return xmmreg;
}

int _checkXMMreg(int type, int reg, int mode)
{
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse && xmmregs[i].type == (type&0xff) && xmmregs[i].reg == reg ) {

			if( !(xmmregs[i].mode & MODE_READ) && (mode&(MODE_READ|MODE_READHALF)) ) {
				if(mode&MODE_READ)
					SSEX_MOVDQA_M128_to_XMM(i, (uptr)_XMMGetAddr(xmmregs[i].type, xmmregs[i].reg, xmmregs[i].VU ? &VU1 : &VU0));
				else {
					if( cpucaps.hasStreamingSIMD2Extensions && g_xmmtypes[i]==XMMT_INT )
						SSE2_MOVQ_M64_to_XMM(i, (uptr)_XMMGetAddr(xmmregs[i].type, xmmregs[i].reg, xmmregs[i].VU ? &VU1 : &VU0));
					else
						SSE_MOVLPS_M64_to_XMM(i, (uptr)_XMMGetAddr(xmmregs[i].type, xmmregs[i].reg, xmmregs[i].VU ? &VU1 : &VU0));
				}
			}

			xmmregs[i].mode |= mode&~MODE_READHALF;
			xmmregs[i].counter = g_xmmAllocCounter++; // update counter
			xmmregs[i].needed = 1;
			return i;
		}
	}

	return -1;
}

int _allocACCtoXMMreg(VURegs *VU, int xmmreg, int mode) {
	int i;
	int readfromreg = -1;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_ACC) continue;
		if (xmmregs[i].VU != XMM_CONV_VU(VU) ) continue;

		if( xmmreg >= 0 ) {
			// requested specific reg, so return that instead
			if( i != xmmreg ) {
				if( xmmregs[i].mode & MODE_READ ) readfromreg = i;
				//if( xmmregs[i].mode & MODE_WRITE ) mode |= MODE_WRITE;
				mode |= xmmregs[i].mode&MODE_WRITE;
				xmmregs[i].inuse = 0;
				break;
			}
		}

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVAPS_M128_to_XMM(i, VU_ACCx_ADDR);
			xmmregs[i].mode |= MODE_READ;
		}

		g_xmmtypes[i] = XMMT_FPS;
		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}
	else {
		_freeXMMreg(xmmreg);
	}

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_ACC;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].VU = XMM_CONV_VU(VU);
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;
	xmmregs[xmmreg].reg = 0;

	if (mode & MODE_READ) {
		if( readfromreg >= 0 ) SSE_MOVAPS_XMM_to_XMM(xmmreg, readfromreg);
		else SSE_MOVAPS_M128_to_XMM(xmmreg, VU_ACCx_ADDR);
	}

	return xmmreg;
}

int _allocFPtoXMMreg(int xmmreg, int fpreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPREG) continue;
		if (xmmregs[i].reg != fpreg) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVSS_M32_to_XMM(i, (uptr)&fpuRegs.fpr[fpreg].f);
			xmmregs[i].mode |= MODE_READ;
		}

		g_xmmtypes[i] = XMMT_FPS;
		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_FPREG;
	xmmregs[xmmreg].reg = fpreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;

	if (mode & MODE_READ) {
		SSE_MOVSS_M32_to_XMM(xmmreg, (uptr)&fpuRegs.fpr[fpreg].f);
	}

	return xmmreg;
}

int _allocGPRtoXMMreg(int xmmreg, int gprreg, int mode)
{
	int i;
	if( !cpucaps.hasStreamingSIMDExtensions ) return -1;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_GPRREG) continue;
		if (xmmregs[i].reg != gprreg) continue;

#ifndef __x86_64__
		assert( _checkMMXreg(MMX_GPR|gprreg, mode) == -1 );
#endif
		g_xmmtypes[i] = XMMT_INT;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			if( gprreg == 0 ) {
				SSEX_PXOR_XMM_to_XMM(i, i);
			}
			else {
				//assert( !(g_cpuHasConstReg & (1<<gprreg)) || (g_cpuFlushedConstReg & (1<<gprreg)) );
				_flushConstReg(gprreg);
				SSEX_MOVDQA_M128_to_XMM(i, (uptr)&cpuRegs.GPR.r[gprreg].UL[0]);
			}
			xmmregs[i].mode |= MODE_READ;
		}

		if( (mode & MODE_WRITE) && gprreg < 32 ) {
			g_cpuHasConstReg &= ~(1<<gprreg);
			//assert( !(g_cpuHasConstReg & (1<<gprreg)) );
		}

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	// currently only gpr regs are const
	if( (mode & MODE_WRITE) && gprreg < 32 ) {
		//assert( !(g_cpuHasConstReg & (1<<gprreg)) );
		g_cpuHasConstReg &= ~(1<<gprreg);
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	g_xmmtypes[xmmreg] = XMMT_INT;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_GPRREG;
	xmmregs[xmmreg].reg = gprreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;

	if (mode & MODE_READ) {
		if( gprreg == 0 ) {
			SSEX_PXOR_XMM_to_XMM(xmmreg, xmmreg);
		}
		else {
			// DOX86
			int mmxreg;
			if( (mode&MODE_READ) ) _flushConstReg(gprreg);

#ifndef __x86_64__
			if( (mmxreg = _checkMMXreg(MMX_GPR+gprreg, 0)) >= 0 ) {
				// transfer
				if (cpucaps.hasStreamingSIMD2Extensions ) {

					SetMMXstate();
					SSE2_MOVQ2DQ_MM_to_XMM(xmmreg, mmxreg);
					SSE2_PUNPCKLQDQ_XMM_to_XMM(xmmreg, xmmreg);
					SSE2_PUNPCKHQDQ_M128_to_XMM(xmmreg, (u32)&cpuRegs.GPR.r[gprreg].UL[0]);

					if( mmxregs[mmxreg].mode & MODE_WRITE ) {

						// instead of setting to write, just flush to mem
						if( !(mode & MODE_WRITE) ) {
							SetMMXstate();
							MOVQRtoM((u32)&cpuRegs.GPR.r[gprreg].UL[0], mmxreg);
						}
						//xmmregs[xmmreg].mode |= MODE_WRITE;
					}
					
					// don't flush
					mmxregs[mmxreg].inuse = 0;
				}
				else {
					_freeMMXreg(mmxreg);
					SSEX_MOVDQA_M128_to_XMM(xmmreg, (u32)&cpuRegs.GPR.r[gprreg].UL[0]);
				}
			}
#else
            if( (mmxreg = _checkX86reg(X86TYPE_GPR, gprreg, 0)) >= 0 ) {
                SSE2_MOVQ_R_to_XMM(xmmreg, mmxreg);
                SSE_MOVHPS_M64_to_XMM(xmmreg, (uptr)&cpuRegs.GPR.r[gprreg].UL[0]);

                // read only, instead of setting to write, just flush to mem
                if( !(mode&MODE_WRITE) && (x86regs[mmxreg].mode & MODE_WRITE) ) {
                    MOV64RtoM((uptr)&cpuRegs.GPR.r[gprreg].UL[0], mmxreg);
                }

                x86regs[mmxreg].inuse = 0;
            }
#endif
			else
            {
				SSEX_MOVDQA_M128_to_XMM(xmmreg, (uptr)&cpuRegs.GPR.r[gprreg].UL[0]);
			}
		}
	}
	else {
#ifndef __x86_64__
		_deleteMMXreg(MMX_GPR+gprreg, 0);
#else
        _deleteX86reg(X86TYPE_GPR, gprreg, 0);
#endif
	}

	return xmmreg;
}

int _allocFPACCtoXMMreg(int xmmreg, int mode)
{
	int i;
	if( !cpucaps.hasStreamingSIMDExtensions ) return -1;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPACC) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVSS_M32_to_XMM(i, (uptr)&fpuRegs.ACC.f);
			xmmregs[i].mode |= MODE_READ;
		}

		g_xmmtypes[i] = XMMT_FPS;
		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_FPACC;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].reg = 0;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;

	if (mode & MODE_READ) {
		SSE_MOVSS_M32_to_XMM(xmmreg, (uptr)&fpuRegs.ACC.f);
	}

	return xmmreg;
}

void _addNeededVFtoXMMreg(int vfreg) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_VFREG) continue;
		if (xmmregs[i].reg != vfreg) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
	}
}

void _addNeededGPRtoXMMreg(int gprreg)
{
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_GPRREG) continue;
		if (xmmregs[i].reg != gprreg) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _addNeededACCtoXMMreg() {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_ACC) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _addNeededFPtoXMMreg(int fpreg) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPREG) continue;
		if (xmmregs[i].reg != fpreg) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _addNeededFPACCtoXMMreg() {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPACC) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _clearNeededXMMregs() {
	int i;

	for (i=0; i<XMMREGS; i++) {

		if( xmmregs[i].needed ) {

			// setup read to any just written regs
			if( xmmregs[i].inuse && (xmmregs[i].mode&MODE_WRITE) )
				xmmregs[i].mode |= MODE_READ;
			xmmregs[i].needed = 0;
		}

		if( xmmregs[i].inuse ) {
			assert( xmmregs[i].type != XMMTYPE_TEMP );
		}
	}
}

void _deleteVFtoXMMreg(int reg, int vu, int flush)
{
	int i;
	VURegs *VU = vu ? &VU1 : &VU0;
	
	for (i=0; i<XMMREGS; i++) {

		if (xmmregs[i].inuse && xmmregs[i].type == XMMTYPE_VFREG && xmmregs[i].reg == reg && xmmregs[i].VU == vu) {

			switch(flush) {
				case 0:
					_freeXMMreg(i);
					break;
				case 1:
				case 2:
					if( xmmregs[i].mode & MODE_WRITE ) {
						assert( reg != 0 );

						if( xmmregs[i].mode & MODE_VUXYZ ) {

							if( xmmregs[i].mode & MODE_VUZ ) {
								// xyz, don't destroy w
								int t0reg;
								for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
									if( !xmmregs[t0reg].inuse ) break;
								}

								if( t0reg < XMMREGS ) {
									SSE_MOVHLPS_XMM_to_XMM(t0reg, i);
									SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
									SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[i].reg)+8, t0reg);
								}
								else {
									// no free reg
									SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
									SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[i].reg)+8, i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
								}
							}
							else {
								// xy
								SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
							}
						}
						else SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(xmmregs[i].reg), i);
						
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}

					if( flush == 2 )
						xmmregs[i].inuse = 0;
					break;
			}
				
			return;
		}
	}
}

void _deleteACCtoXMMreg(int vu, int flush)
{
	int i;
	VURegs *VU = vu ? &VU1 : &VU0;
	
	for (i=0; i<XMMREGS; i++) {
	
		if (xmmregs[i].inuse && xmmregs[i].type == XMMTYPE_ACC && xmmregs[i].VU == vu) {

			switch(flush) {
				case 0:
					_freeXMMreg(i);
					break;
				case 1:
				case 2:
					if( xmmregs[i].mode & MODE_WRITE ) {

						if( xmmregs[i].mode & MODE_VUXYZ ) {

							if( xmmregs[i].mode & MODE_VUZ ) {
								// xyz, don't destroy w
								int t0reg;
								for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
									if( !xmmregs[t0reg].inuse ) break;
								}

								if( t0reg < XMMREGS ) {
									SSE_MOVHLPS_XMM_to_XMM(t0reg, i);
									SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, i);
									SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, t0reg);
								}
								else {
									// no free reg
									SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
									//SSE_MOVHLPS_XMM_to_XMM(i, i);
									SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
								}
							}
							else {
								// xy
								SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, i);
							}
						}
						else SSE_MOVAPS_XMM_to_M128(VU_ACCx_ADDR, i);
						
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}

					if( flush == 2 )
						xmmregs[i].inuse = 0;
					break;
			}
				
			return;
		}
	}
}

// when flush is 1 or 2, only commits the reg to mem (still leave its xmm entry)
void _deleteGPRtoXMMreg(int reg, int flush)
{
	int i;
	for (i=0; i<XMMREGS; i++) {

		if (xmmregs[i].inuse && xmmregs[i].type == XMMTYPE_GPRREG && xmmregs[i].reg == reg ) {

			switch(flush) {
				case 0:
					_freeXMMreg(i);
					break;
				case 1:
				case 2:
					if( xmmregs[i].mode & MODE_WRITE ) {
						assert( reg != 0 );

						//assert( g_xmmtypes[i] == XMMT_INT );
						SSEX_MOVDQA_XMM_to_M128((uptr)&cpuRegs.GPR.r[reg].UL[0], i);
						
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}

					if( flush == 2 )
						xmmregs[i].inuse = 0;
					break;
			}
				
			return;
		}
	}
}

void _deleteFPtoXMMreg(int reg, int flush)
{
	int i;
	for (i=0; i<XMMREGS; i++) {

		if (xmmregs[i].inuse && xmmregs[i].type == XMMTYPE_FPREG && xmmregs[i].reg == reg ) {

			switch(flush) {
				case 0:
					_freeXMMreg(i);
					break;
				case 1:
				case 2:
					if( flush == 1 && (xmmregs[i].mode & MODE_WRITE) ) {
						SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.fpr[reg].UL, i);
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}

					if( flush == 2 )
						xmmregs[i].inuse = 0;
					break;
			}
				
			return;
		}
	}
}

void _freeXMMreg(int xmmreg) {
	VURegs *VU = xmmregs[xmmreg].VU ? &VU1 : &VU0;
	assert( xmmreg < XMMREGS );

	if (!xmmregs[xmmreg].inuse) return;
	
	if (xmmregs[xmmreg].type == XMMTYPE_VFREG && (xmmregs[xmmreg].mode & MODE_WRITE) ) {
		if( xmmregs[xmmreg].mode & MODE_VUXYZ ) {

			if( xmmregs[xmmreg].mode & MODE_VUZ ) {
				// don't destroy w
				int t0reg;
				for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
					if( !xmmregs[t0reg].inuse ) break;
				}

				if( t0reg < XMMREGS ) {
					SSE_MOVHLPS_XMM_to_XMM(t0reg, xmmreg);
					SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
					SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[xmmreg].reg)+8, t0reg);
				}
				else {
					// no free reg
					SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
					SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
					//SSE_MOVHLPS_XMM_to_XMM(xmmreg, xmmreg);
					SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[xmmreg].reg)+8, xmmreg);
					SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
				}
			}
			else {
				SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
			}
		}
		else SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
	}
	else if (xmmregs[xmmreg].type == XMMTYPE_ACC && (xmmregs[xmmreg].mode & MODE_WRITE) ) {
		if( xmmregs[xmmreg].mode & MODE_VUXYZ ) {

			if( xmmregs[xmmreg].mode & MODE_VUZ ) {
				// don't destroy w
				int t0reg;
				for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
					if( !xmmregs[t0reg].inuse ) break;
				}

				if( t0reg < XMMREGS ) {
					SSE_MOVHLPS_XMM_to_XMM(t0reg, xmmreg);
					SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
					SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, t0reg);
				}
				else {
					// no free reg
					SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
					SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
					//SSE_MOVHLPS_XMM_to_XMM(xmmreg, xmmreg);
					SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, xmmreg);
					SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
				}
			}
			else {
				SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
			}
		}
		else SSE_MOVAPS_XMM_to_M128(VU_ACCx_ADDR, xmmreg);
	}
	else if (xmmregs[xmmreg].type == XMMTYPE_GPRREG && (xmmregs[xmmreg].mode & MODE_WRITE) ) {
		assert( xmmregs[xmmreg].reg != 0 );
		//assert( g_xmmtypes[xmmreg] == XMMT_INT );
		SSEX_MOVDQA_XMM_to_M128((uptr)&cpuRegs.GPR.r[xmmregs[xmmreg].reg].UL[0], xmmreg);
	}
	else if (xmmregs[xmmreg].type == XMMTYPE_FPREG && (xmmregs[xmmreg].mode & MODE_WRITE)) {
		SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.fpr[xmmregs[xmmreg].reg], xmmreg);
	}
	else if (xmmregs[xmmreg].type == XMMTYPE_FPACC && (xmmregs[xmmreg].mode & MODE_WRITE)) {
		SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.ACC.f, xmmreg);
	}

	xmmregs[xmmreg].mode &= ~(MODE_WRITE|MODE_VUXYZ);
	xmmregs[xmmreg].inuse = 0;
}

int _getNumXMMwrite()
{
	int num = 0, i;
	for (i=0; i<XMMREGS; i++) {
		if( xmmregs[i].inuse && (xmmregs[i].mode&MODE_WRITE) ) ++num;
	}

	return num;
}

u8 _hasFreeXMMreg()
{
	int i;

	if( !cpucaps.hasStreamingSIMDExtensions ) return 0;

	for (i=0; i<XMMREGS; i++) {
		if (!xmmregs[i].inuse) return 1;
	}

	// check for dead regs
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			if( !EEINST_ISLIVEXMM(xmmregs[i].reg) ) {
				return 1;
			}
		}
	}

	// check for dead regs
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG  ) {
			if( !(g_pCurInstInfo->regs[xmmregs[i].reg]&EEINST_USED) ) {
				return 1;
			}
		}
	}
	return 0;
}

void _moveXMMreg(int xmmreg)
{
	int i;
	if( !xmmregs[xmmreg].inuse ) return;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse) continue;
		break;
	}

	if( i == XMMREGS ) {
		_freeXMMreg(xmmreg);
		return;
	}

	// move
	xmmregs[i] = xmmregs[xmmreg];
	xmmregs[xmmreg].inuse = 0;
	SSEX_MOVDQA_XMM_to_XMM(i, xmmreg);
}

void _flushXMMregs()
{
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;

		assert( xmmregs[i].type != XMMTYPE_TEMP );
		assert( xmmregs[i].mode & (MODE_READ|MODE_WRITE) );

		_freeXMMreg(i);
		xmmregs[i].inuse = 1;
		xmmregs[i].mode &= ~MODE_WRITE;
		xmmregs[i].mode |= MODE_READ;
	}
}

void _freeXMMregs()
{
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;

		assert( xmmregs[i].type != XMMTYPE_TEMP );
		//assert( xmmregs[i].mode & (MODE_READ|MODE_WRITE) );

		_freeXMMreg(i);
	}
}

#if !defined(_MSC_VER) || !defined(__x86_64__)

void FreezeXMMRegs_(int save)
{
	assert( g_EEFreezeRegs );

	if( save ) {
		if( g_globalXMMSaved ){
			//SysPrintf("XMM Already saved\n");
			return;
			}
		// only necessary for nonsse CPUs (very rare)
		if( !cpucaps.hasStreamingSIMDExtensions )
			return;
		g_globalXMMSaved = 1;

#ifdef _MSC_VER
        __asm {
			movaps xmmword ptr [g_globalXMMData + 0x00], xmm0
			movaps xmmword ptr [g_globalXMMData + 0x10], xmm1
			movaps xmmword ptr [g_globalXMMData + 0x20], xmm2
			movaps xmmword ptr [g_globalXMMData + 0x30], xmm3
			movaps xmmword ptr [g_globalXMMData + 0x40], xmm4
			movaps xmmword ptr [g_globalXMMData + 0x50], xmm5
			movaps xmmword ptr [g_globalXMMData + 0x60], xmm6
			movaps xmmword ptr [g_globalXMMData + 0x70], xmm7
        }

#else
        __asm__(".intel_syntax\n"
                "movaps [%0+0x00], %%xmm0\n"
                "movaps [%0+0x10], %%xmm1\n"
                "movaps [%0+0x20], %%xmm2\n"
                "movaps [%0+0x30], %%xmm3\n"
                "movaps [%0+0x40], %%xmm4\n"
                "movaps [%0+0x50], %%xmm5\n"
                "movaps [%0+0x60], %%xmm6\n"
                "movaps [%0+0x70], %%xmm7\n"
#ifdef __x86_64__
                "movaps [%0+0x80], %%xmm0\n"
                "movaps [%0+0x90], %%xmm1\n"
                "movaps [%0+0xa0], %%xmm2\n"
                "movaps [%0+0xb0], %%xmm3\n"
                "movaps [%0+0xc0], %%xmm4\n"
                "movaps [%0+0xd0], %%xmm5\n"
                "movaps [%0+0xe0], %%xmm6\n"
                "movaps [%0+0xf0], %%xmm7\n"
#endif
                ".att_syntax\n" : : "r"(g_globalXMMData) );

#endif // _MSC_VER
	}
	else {
		if( !g_globalXMMSaved ){
			//SysPrintf("XMM Regs not saved!\n");
			return;
			}

        // TODO: really need to backup all regs?
		g_globalXMMSaved = 0;

#ifdef _MSC_VER
        __asm {
			movaps xmm0, xmmword ptr [g_globalXMMData + 0x00]
			movaps xmm1, xmmword ptr [g_globalXMMData + 0x10]
			movaps xmm2, xmmword ptr [g_globalXMMData + 0x20]
			movaps xmm3, xmmword ptr [g_globalXMMData + 0x30]
			movaps xmm4, xmmword ptr [g_globalXMMData + 0x40]
			movaps xmm5, xmmword ptr [g_globalXMMData + 0x50]
			movaps xmm6, xmmword ptr [g_globalXMMData + 0x60]
			movaps xmm7, xmmword ptr [g_globalXMMData + 0x70]
        }

#else
        __asm__(".intel_syntax\n"
                "movaps %%xmm0, [%0+0x00]\n"
                "movaps %%xmm1, [%0+0x10]\n"
                "movaps %%xmm2, [%0+0x20]\n"
                "movaps %%xmm3, [%0+0x30]\n"
                "movaps %%xmm4, [%0+0x40]\n"
                "movaps %%xmm5, [%0+0x50]\n"
                "movaps %%xmm6, [%0+0x60]\n"
                "movaps %%xmm7, [%0+0x70]\n"
#ifdef __x86_64__
                "movaps %%xmm8, [%0+0x80]\n"
                "movaps %%xmm9, [%0+0x90]\n"
                "movaps %%xmm10, [%0+0xa0]\n"
                "movaps %%xmm11, [%0+0xb0]\n"
                "movaps %%xmm12, [%0+0xc0]\n"
                "movaps %%xmm13, [%0+0xd0]\n"
                "movaps %%xmm14, [%0+0xe0]\n"
                "movaps %%xmm15, [%0+0xf0]\n"
#endif
                ".att_syntax\n" : : "r"(g_globalXMMData) );

#endif // _MSC_VER
	}
}

#endif

// PSX 
void _psxMoveGPRtoR(x86IntRegType to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoR( to, g_psxConstRegs[fromgpr] );
	else {
		// check x86
		MOV32MtoR(to, (uptr)&psxRegs.GPR.r[ fromgpr ] );
	}
}

void _psxMoveGPRtoM(u32 to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoM( to, g_psxConstRegs[fromgpr] );
	else {
		// check x86
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[ fromgpr ] );
		MOV32RtoM(to, EAX );
	}
}

void _psxMoveGPRtoRm(x86IntRegType to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoRmOffset( to, g_psxConstRegs[fromgpr], 0 );
	else {
		// check x86
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[ fromgpr ] );
		MOV32RtoRm(to, EAX );
	}
}

PCSX2_ALIGNED16(u32 s_zeros[4]) = {0};
int _signExtendXMMtoM(u32 to, x86SSERegType from, int candestroy)
{
	int t0reg;
	g_xmmtypes[from] = XMMT_INT;
	if( candestroy ) {
		if( g_xmmtypes[from] == XMMT_FPS || !cpucaps.hasStreamingSIMD2Extensions ) SSE_MOVSS_XMM_to_M32(to, from);
		else SSE2_MOVD_XMM_to_M32(to, from);

		if( cpucaps.hasStreamingSIMD2Extensions ) {
			SSE2_PSRAD_I8_to_XMM(from, 31);
			SSE2_MOVD_XMM_to_M32(to+4, from);
			return 1;
		}
		else {
			SSE_MOVSS_XMM_to_M32(to+4, from);
			SAR32ItoM(to+4, 31);
			return 0;
		}
	}
	else {
		// can't destroy and type is int
		assert( g_xmmtypes[from] == XMMT_INT );

		if( cpucaps.hasStreamingSIMD2Extensions ) {
			if( _hasFreeXMMreg() ) {
				xmmregs[from].needed = 1;
				t0reg = _allocTempXMMreg(XMMT_INT, -1);
				SSEX_MOVDQA_XMM_to_XMM(t0reg, from);
				SSE2_PSRAD_I8_to_XMM(from, 31);
				SSE2_MOVD_XMM_to_M32(to, t0reg);
				SSE2_MOVD_XMM_to_M32(to+4, from);

				// swap xmm regs.. don't ask
				xmmregs[t0reg] = xmmregs[from];
				xmmregs[from].inuse = 0;
			}
			else {
				SSE2_MOVD_XMM_to_M32(to+4, from);
				SSE2_MOVD_XMM_to_M32(to, from);
				SAR32ItoM(to+4, 31);
			}
		}
		else {
			SSE_MOVSS_XMM_to_M32(to+4, from);
			SSE_MOVSS_XMM_to_M32(to, from);
			SAR32ItoM(to+4, 31);
		}

		return 0;
	}

	assert(0);
}

int _allocCheckGPRtoXMM(EEINST* pinst, int gprreg, int mode)
{
	if( pinst->regs[gprreg] & EEINST_XMM ) return _allocGPRtoXMMreg(-1, gprreg, mode);
	
	return _checkXMMreg(XMMTYPE_GPRREG, gprreg, mode);
}

int _allocCheckFPUtoXMM(EEINST* pinst, int fpureg, int mode)
{
	if( pinst->fpuregs[fpureg] & EEINST_XMM ) return _allocFPtoXMMreg(-1, fpureg, mode);
	
	return _checkXMMreg(XMMTYPE_FPREG, fpureg, mode);
}

int _allocCheckGPRtoX86(EEINST* pinst, int gprreg, int mode)
{
	if( pinst->regs[gprreg] & EEINST_USED )
        return _allocX86reg(-1, X86TYPE_GPR, gprreg, mode);
	
	return _checkX86reg(X86TYPE_GPR, gprreg, mode);
}

void _recClearInst(EEINST* pinst)
{
	memset(&pinst->regs[0], EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2, sizeof(pinst->regs));
	memset(&pinst->fpuregs[0], EEINST_LIVE0, sizeof(pinst->fpuregs));
	memset(&pinst->info, 0, sizeof(EEINST)-sizeof(pinst->regs)-sizeof(pinst->fpuregs));
}

// returns nonzero value if reg has been written between [startpc, endpc-4]
int _recIsRegWritten(EEINST* pinst, int size, u8 xmmtype, u8 reg)
{
	int i, inst = 1;
	while(size-- > 0) {
		for(i = 0; i < ARRAYSIZE(pinst->writeType); ++i) {
			if( pinst->writeType[i] == xmmtype && pinst->writeReg[i] == reg )
				return inst;
		}
		++inst;
		pinst++;
	}

	return 0;
}

int _recIsRegUsed(EEINST* pinst, int size, u8 xmmtype, u8 reg)
{
	int i, inst = 1;
	while(size-- > 0) {
		for(i = 0; i < ARRAYSIZE(pinst->writeType); ++i) {
			if( pinst->writeType[i] == xmmtype && pinst->writeReg[i] == reg )
				return inst;
		}
		for(i = 0; i < ARRAYSIZE(pinst->readType); ++i) {
			if( pinst->readType[i] == xmmtype && pinst->readReg[i] == reg )
				return inst;
		}
		++inst;
		pinst++;
	}

	return 0;
}

void _recFillRegister(EEINST* pinst, int type, int reg, int write)
{
	int i = 0;
	if( write ) {
		for(i = 0; i < ARRAYSIZE(pinst->writeType); ++i) {
			if( pinst->writeType[i] == XMMTYPE_TEMP ) {
				pinst->writeType[i] = type;
				pinst->writeReg[i] = reg;
				return;
			}
		}
		assert(0);
	}
	else {
		for(i = 0; i < ARRAYSIZE(pinst->readType); ++i) {
			if( pinst->readType[i] == XMMTYPE_TEMP ) {
				pinst->readType[i] = type;
				pinst->readReg[i] = reg;
				return;
			}
		}
		assert(0);
	}
}

void SetMMXstate() {
#ifdef __x86_64__
    assert(0);
#else
	x86FpuState = MMX_STATE;
#endif
}

// Writebacks //
void _recClearWritebacks()
{
}

void _recAddWriteBack(int cycle, u32 viwrite, EEINST* parent)
{
}

EEINSTWRITEBACK* _recCheckWriteBack(int cycle)
{
	return NULL;
}

extern "C" void cpudetectSSE3(void* pfnCallSSE3)
{
	cpucaps.hasStreamingSIMD3Extensions = 1;

#ifdef _MSC_VER
	__try {
        ((void (*)())pfnCallSSE3)();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		cpucaps.hasStreamingSIMD3Extensions = 0;
	}
#else // linux

#ifdef PCSX2_FORCESSE3
    cpucaps.hasStreamingSIMD3Extensions = 1;
#else
    // exception handling doesn't work, so disable for x86 builds of linux
    cpucaps.hasStreamingSIMD3Extensions = 0;
#endif
//    try {
//        __asm__("call *%0" : : "m"(pfnCallSSE3) );
//	}
//    catch(...) {
//        SysPrintf("no SSE3 found\n");
//        cpucaps.hasStreamingSIMD3Extensions = 0;
//    }
#endif
}

extern "C" void cpudetectSSE4(void* pfnCallSSE4)
{
return;
	cpucaps.hasStreamingSIMD4Extensions = 1;

#ifdef _MSC_VER
	__try {
        ((void (*)())pfnCallSSE4)();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		cpucaps.hasStreamingSIMD4Extensions = 0;
	}
#else // linux

#ifdef PCSX2_FORCESSE4
    cpucaps.hasStreamingSIMD4Extensions = 1;
#else
    // exception handling doesn't work, so disable for x86 builds of linux
    cpucaps.hasStreamingSIMD4Extensions = 0;
#endif
//    try {
//        __asm__("call *%0" : : "m"(pfnCallSSE4) );
//	}
//    catch(...) {
//        SysPrintf("no SSE4.1 found\n");
//        cpucaps.hasStreamingSIMD4Extensions = 0;
//    }
#endif
}

struct BASEBLOCKS
{
	// 0 - ee, 1 - iop
	inline void Add(BASEBLOCKEX*);
	inline void Remove(BASEBLOCKEX*);
	inline int Get(u32 startpc);
	inline void Reset();

	inline BASEBLOCKEX** GetAll(int* pnum);

	vector<BASEBLOCKEX*> blocks;
};

void BASEBLOCKS::Add(BASEBLOCKEX* pex)
{
	assert( pex != NULL );

	switch(blocks.size()) {
		case 0:
			blocks.push_back(pex);
			return;
		case 1:
			assert( blocks.front()->startpc != pex->startpc );

			if( blocks.front()->startpc < pex->startpc ) {
				blocks.push_back(pex);
			}
			else blocks.insert(blocks.begin(), pex);

			return;

		default:
		{
			int imin = 0, imax = blocks.size(), imid;

			while(imin < imax) {
				imid = (imin+imax)>>1;

				if( blocks[imid]->startpc > pex->startpc ) imax = imid;
				else imin = imid+1;
			}

			assert( imin == blocks.size() || blocks[imin]->startpc > pex->startpc );
			if( imin > 0 ) assert( blocks[imin-1]->startpc < pex->startpc );
			blocks.insert(blocks.begin()+imin, pex);

			return;
		}
	}
}

int BASEBLOCKS::Get(u32 startpc)
{
	switch(blocks.size()) {
		case 1:
			return 0;
		case 2:
			return blocks.front()->startpc < startpc;

		default:
		{
			int imin = 0, imax = blocks.size()-1, imid;

			while(imin < imax) {
				imid = (imin+imax)>>1;

				if( blocks[imid]->startpc > startpc ) imax = imid;
				else if( blocks[imid]->startpc == startpc ) return imid;
				else imin = imid+1;
			}

			assert( blocks[imin]->startpc == startpc );
			return imin;
		}
	}
}

void BASEBLOCKS::Remove(BASEBLOCKEX* pex)
{
	assert( pex != NULL );
	int i = Get(pex->startpc);
	assert( blocks[i] == pex ); 
	blocks.erase(blocks.begin()+i);
}

void BASEBLOCKS::Reset()
{
	blocks.resize(0);
	blocks.reserve(512);
}

BASEBLOCKEX** BASEBLOCKS::GetAll(int* pnum)
{
	assert( pnum != NULL );
	*pnum = blocks.size();
	return &blocks[0];
}

static BASEBLOCKS s_vecBaseBlocksEx[2];

void AddBaseBlockEx(BASEBLOCKEX* pex, int cpu)
{
	s_vecBaseBlocksEx[cpu].Add(pex);
}

BASEBLOCKEX* GetBaseBlockEx(u32 startpc, int cpu)
{
	return s_vecBaseBlocksEx[cpu].blocks[s_vecBaseBlocksEx[cpu].Get(startpc)];
}

void RemoveBaseBlockEx(BASEBLOCKEX* pex, int cpu)
{
	s_vecBaseBlocksEx[cpu].Remove(pex);
}

void ResetBaseBlockEx(int cpu)
{
	s_vecBaseBlocksEx[cpu].Reset();
}

BASEBLOCKEX** GetAllBaseBlocks(int* pnum, int cpu)
{
	return s_vecBaseBlocksEx[cpu].GetAll(pnum);
}

#endif // PCSX2_NORECBUILD
