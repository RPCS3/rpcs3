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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Misc.h"
#include "iR5900.h"
#include "Vif.h"
#include "VU.h"
#include "ix86/ix86.h"
#include "R3000A.h"

u16 g_x86AllocCounter = 0;
u16 g_xmmAllocCounter = 0;

EEINST* g_pCurInstInfo = NULL;

u32 g_cpuRegHasLive1 = 0, g_cpuPrevRegHasLive1 = 0; // set if upper 32 bits are live
u32 g_cpuRegHasSignExt = 0, g_cpuPrevRegHasSignExt = 0; // set if upper 32 bits are the sign extension of the lower integer

// used to make sure regs don't get changed while in recompiler
// use FreezeMMXRegs, FreezeXMMRegs
u32 g_recWriteback = 0;

#ifdef _DEBUG
char g_globalXMMLocked = 0;
#endif

_xmmregs xmmregs[XMMREGS], s_saveXMMregs[XMMREGS];

// X86 caching
_x86regs x86regs[X86REGS], s_saveX86regs[X86REGS];

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
	memzero_obj( xmmregs );
	g_xmmAllocCounter = 0;
	s_xmmchecknext = 0;
}

__forceinline void* _XMMGetAddr(int type, int reg, VURegs *VU)
{
	switch (type) {
		case XMMTYPE_VFREG:
			return (void*)VU_VFx_ADDR(reg);
			
		case XMMTYPE_ACC:
			return (void*)VU_ACCx_ADDR;
		
		case XMMTYPE_GPRREG:
			if( reg < 32 )
				assert( !(g_cpuHasConstReg & (1<<reg)) || (g_cpuFlushedConstReg & (1<<reg)) );
			return &cpuRegs.GPR.r[reg].UL[0];
			
		case XMMTYPE_FPREG:
			return &fpuRegs.fpr[reg];
		
		case XMMTYPE_FPACC:
			return &fpuRegs.ACC.f;
		
		default: 
			assert(0);
	}
	
	return NULL;
}

int  _getFreeXMMreg()
{
	int i, tempi;
	u32 bestcount = 0x10000;

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
	Console::Error("*PCSX2*: XMM Reg Allocation Error in _getFreeXMMreg()!");

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
		if ((xmmregs[i].inuse == 0)  || (xmmregs[i].type != XMMTYPE_VFREG) || 
		     (xmmregs[i].reg != vfreg) || (xmmregs[i].VU != XMM_CONV_VU(VU))) 
			continue;
		
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

	if (xmmreg == -1) 
		xmmreg = _getFreeXMMreg();
	else 
		_freeXMMreg(xmmreg);

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
		if (xmmregs[i].inuse && (xmmregs[i].type == (type&0xff)) && (xmmregs[i].reg == reg)) {

			if ( !(xmmregs[i].mode & MODE_READ) ) {
				if (mode & MODE_READ) {
					SSEX_MOVDQA_M128_to_XMM(i, (uptr)_XMMGetAddr(xmmregs[i].type, xmmregs[i].reg, xmmregs[i].VU ? &VU1 : &VU0));
				}
				else if (mode & MODE_READHALF) {
					if( g_xmmtypes[i] == XMMT_INT )
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

	if (xmmreg == -1) 
		xmmreg = _getFreeXMMreg();
	else 
		_freeXMMreg(xmmreg);

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_ACC;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].VU = XMM_CONV_VU(VU);
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;
	xmmregs[xmmreg].reg = 0;

	if (mode & MODE_READ) 
	{
		if( readfromreg >= 0 ) 
			SSE_MOVAPS_XMM_to_XMM(xmmreg, readfromreg);
		else 
			SSE_MOVAPS_M128_to_XMM(xmmreg, VU_ACCx_ADDR);
	}

	return xmmreg;
}

int _allocFPtoXMMreg(int xmmreg, int fpreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPREG) continue;
		if (xmmregs[i].reg != fpreg) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode & MODE_READ)) {
			SSE_MOVSS_M32_to_XMM(i, (uptr)&fpuRegs.fpr[fpreg].f);
			xmmregs[i].mode |= MODE_READ;
		}

		g_xmmtypes[i] = XMMT_FPS;
		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) xmmreg = _getFreeXMMreg();

	g_xmmtypes[xmmreg] = XMMT_FPS;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_FPREG;
	xmmregs[xmmreg].reg = fpreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;

	if (mode & MODE_READ) 
		SSE_MOVSS_M32_to_XMM(xmmreg, (uptr)&fpuRegs.fpr[fpreg].f);

	return xmmreg;
}

int _allocGPRtoXMMreg(int xmmreg, int gprreg, int mode)
{
	int i;

	for (i=0; i<XMMREGS; i++) 
	{
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_GPRREG) continue;
		if (xmmregs[i].reg != gprreg) continue;

		assert( _checkMMXreg(MMX_GPR|gprreg, mode) == -1 );

		g_xmmtypes[i] = XMMT_INT;

		if (!(xmmregs[i].mode & MODE_READ) && (mode & MODE_READ)) 
		{
			if (gprreg == 0 ) 
			{
				SSEX_PXOR_XMM_to_XMM(i, i);
			}
			else 
			{
				//assert( !(g_cpuHasConstReg & (1<<gprreg)) || (g_cpuFlushedConstReg & (1<<gprreg)) );
				_flushConstReg(gprreg);
				SSEX_MOVDQA_M128_to_XMM(i, (uptr)&cpuRegs.GPR.r[gprreg].UL[0]);
			}
			xmmregs[i].mode |= MODE_READ;
		}

		if  ((mode & MODE_WRITE) && (gprreg < 32)) 
		{
			g_cpuHasConstReg &= ~(1<<gprreg);
			//assert( !(g_cpuHasConstReg & (1<<gprreg)) );
		}

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	// currently only gpr regs are const
	// fixme - do we really need to execute this both here and in the loop?
	if ((mode & MODE_WRITE) && gprreg < 32) 
	{
		//assert( !(g_cpuHasConstReg & (1<<gprreg)) );
		g_cpuHasConstReg &= ~(1<<gprreg);
	}

	if (xmmreg == -1) xmmreg = _getFreeXMMreg();

	g_xmmtypes[xmmreg] = XMMT_INT;
	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_GPRREG;
	xmmregs[xmmreg].reg = gprreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].counter = g_xmmAllocCounter++;

	if (mode & MODE_READ) 
	{
		if (gprreg == 0 ) 
		{
			SSEX_PXOR_XMM_to_XMM(xmmreg, xmmreg);
		}
		else 
		{
			// DOX86
			int mmxreg;
			
			if (mode & MODE_READ) _flushConstReg(gprreg);

			mmxreg = _checkMMXreg(MMX_GPR+gprreg, 0);
			
			if (mmxreg >= 0 ) 
			{
				// transfer
				SetMMXstate();
				SSE2_MOVQ2DQ_MM_to_XMM(xmmreg, mmxreg);
				SSE2_PUNPCKLQDQ_XMM_to_XMM(xmmreg, xmmreg);
				SSE2_PUNPCKHQDQ_M128_to_XMM(xmmreg, (u32)&cpuRegs.GPR.r[gprreg].UL[0]);

				if (mmxregs[mmxreg].mode & MODE_WRITE ) 
				{
					// instead of setting to write, just flush to mem
					if  (!(mode & MODE_WRITE)) 
					{
						SetMMXstate();
						MOVQRtoM((u32)&cpuRegs.GPR.r[gprreg].UL[0], mmxreg);
					}
					//xmmregs[xmmreg].mode |= MODE_WRITE;
				}
					
				// don't flush
				mmxregs[mmxreg].inuse = 0;
			}
			else
				SSEX_MOVDQA_M128_to_XMM(xmmreg, (uptr)&cpuRegs.GPR.r[gprreg].UL[0]);
		}
	}
	else 
	_deleteMMXreg(MMX_GPR+gprreg, 0);

	return xmmreg;
}

int _allocFPACCtoXMMreg(int xmmreg, int mode)
{
	int i;

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
	
	for (i=0; i<XMMREGS; i++) 
	{
		if (xmmregs[i].inuse && (xmmregs[i].type == XMMTYPE_VFREG) && 
		   (xmmregs[i].reg == reg) && (xmmregs[i].VU == vu))  
		{
			switch(flush) {
				case 0:
					_freeXMMreg(i);
					break;
				case 1:
				case 2:
					if( xmmregs[i].mode & MODE_WRITE ) 
					{
						assert( reg != 0 );

						if( xmmregs[i].mode & MODE_VUXYZ ) 
						{
							if( xmmregs[i].mode & MODE_VUZ ) 
							{
								// xyz, don't destroy w
								int t0reg;
								
								for (t0reg = 0; t0reg < XMMREGS; ++t0reg) 
								{
									if (!xmmregs[t0reg].inuse ) 
										break;
								}

								if (t0reg < XMMREGS ) 
								{
									SSE_MOVHLPS_XMM_to_XMM(t0reg, i);
									SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
									SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[i].reg)+8, t0reg);
								}
								else 
								{
									// no free reg
									SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
									SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[i].reg)+8, i);
									SSE_SHUFPS_XMM_to_XMM(i, i, 0xc6);
								}
							}
							else 
							{
								// xy
								SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[i].reg), i);
							}
						}
						else SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(xmmregs[i].reg), i);
						
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}

					if (flush == 2) xmmregs[i].inuse = 0;
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
		if (xmmregs[i].inuse && (xmmregs[i].type == XMMTYPE_ACC) && (xmmregs[i].VU == vu)) {

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
					return;

				case 1:
					if (xmmregs[i].mode & MODE_WRITE) {
						SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.fpr[reg].UL, i);
						// get rid of MODE_WRITE since don't want to flush again
						xmmregs[i].mode &= ~MODE_WRITE;
						xmmregs[i].mode |= MODE_READ;
					}
					return;

				case 2:
					xmmregs[i].inuse = 0;
					return;
			}
		}
	}
}

void _freeXMMreg(int xmmreg) 
{
	assert( xmmreg < XMMREGS );

	if (!xmmregs[xmmreg].inuse) return;
	
	if (xmmregs[xmmreg].mode & MODE_WRITE) {
	switch (xmmregs[xmmreg].type) {
		case XMMTYPE_VFREG:
		{
			const VURegs *VU = xmmregs[xmmreg].VU ? &VU1 : &VU0;
			if( xmmregs[xmmreg].mode & MODE_VUXYZ ) 
			{
				if( xmmregs[xmmreg].mode & MODE_VUZ ) 
				{
					// don't destroy w
					int t0reg;
					for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
						if( !xmmregs[t0reg].inuse ) break;
					}

					if( t0reg < XMMREGS ) 
					{
						SSE_MOVHLPS_XMM_to_XMM(t0reg, xmmreg);
						SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
						SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[xmmreg].reg)+8, t0reg);
					}
					else 
					{
						// no free reg
						SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
						SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
						//SSE_MOVHLPS_XMM_to_XMM(xmmreg, xmmreg);
						SSE_MOVSS_XMM_to_M32(VU_VFx_ADDR(xmmregs[xmmreg].reg)+8, xmmreg);
						SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
					}
				}
				else 
				{
					SSE_MOVLPS_XMM_to_M64(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
				}
			}
			else 
			{
				SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
			}
		}
		break;
		
		case XMMTYPE_ACC:
		{
			const VURegs *VU = xmmregs[xmmreg].VU ? &VU1 : &VU0;
			if( xmmregs[xmmreg].mode & MODE_VUXYZ ) 
			{
				if( xmmregs[xmmreg].mode & MODE_VUZ ) 
				{
					// don't destroy w
					int t0reg;
					
					for(t0reg = 0; t0reg < XMMREGS; ++t0reg ) {
						if( !xmmregs[t0reg].inuse ) break;
					}

					if( t0reg < XMMREGS ) 
					{
						SSE_MOVHLPS_XMM_to_XMM(t0reg, xmmreg);
						SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
						SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, t0reg);
					}
					else 
					{
						// no free reg
						SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
						SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
						//SSE_MOVHLPS_XMM_to_XMM(xmmreg, xmmreg);
						SSE_MOVSS_XMM_to_M32(VU_ACCx_ADDR+8, xmmreg);
						SSE_SHUFPS_XMM_to_XMM(xmmreg, xmmreg, 0xc6);
					}
				}
				else 
				{
					SSE_MOVLPS_XMM_to_M64(VU_ACCx_ADDR, xmmreg);
				}
			}
			else 
			{
				SSE_MOVAPS_XMM_to_M128(VU_ACCx_ADDR, xmmreg);
			}
		}
		break;
		
		case XMMTYPE_GPRREG:
			assert( xmmregs[xmmreg].reg != 0 );
			//assert( g_xmmtypes[xmmreg] == XMMT_INT );
			SSEX_MOVDQA_XMM_to_M128((uptr)&cpuRegs.GPR.r[xmmregs[xmmreg].reg].UL[0], xmmreg);
			break;
	
		case XMMTYPE_FPREG:
			SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.fpr[xmmregs[xmmreg].reg], xmmreg);
			break;
	
		case XMMTYPE_FPACC:
			SSE_MOVSS_XMM_to_M32((uptr)&fpuRegs.ACC.f, xmmreg);
			break;
	
		default:
			break;
	}
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

PCSX2_ALIGNED16(u32 s_zeros[4]) = {0};
int _signExtendXMMtoM(u32 to, x86SSERegType from, int candestroy)
{
	int t0reg;
	g_xmmtypes[from] = XMMT_INT;
	if( candestroy ) {
		if( g_xmmtypes[from] == XMMT_FPS ) SSE_MOVSS_XMM_to_M32(to, from);
		else SSE2_MOVD_XMM_to_M32(to, from);

		SSE2_PSRAD_I8_to_XMM(from, 31);
		SSE2_MOVD_XMM_to_M32(to+4, from);
		return 1;
	}
	else {
		// can't destroy and type is int
		assert( g_xmmtypes[from] == XMMT_INT );

		
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
	memzero_obj( *pinst );
	memset8_obj<EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2>( pinst->regs );
	memset8_obj<EEINST_LIVE0>( pinst->fpuregs );
}

// returns nonzero value if reg has been written between [startpc, endpc-4]
u32 _recIsRegWritten(EEINST* pinst, int size, u8 xmmtype, u8 reg)
{
	u32  i, inst = 1;
	
	while(size-- > 0) {
		for(i = 0; i < ARRAYSIZE(pinst->writeType); ++i) {
			if ((pinst->writeType[i] == xmmtype) && (pinst->writeReg[i] == reg))
				return inst;
		}
		++inst;
		pinst++;
	}

	return 0;
}

u32 _recIsRegUsed(EEINST* pinst, int size, u8 xmmtype, u8 reg)
{
	u32 i, inst = 1;
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

void _recFillRegister(EEINST& pinst, int type, int reg, int write)
{
	u32 i = 0;
	if (write ) {
		for(i = 0; i < ARRAYSIZE(pinst.writeType); ++i) {
			if( pinst.writeType[i] == XMMTYPE_TEMP ) {
				pinst.writeType[i] = type;
				pinst.writeReg[i] = reg;
				return;
			}
		}
		assert(0);
	}
	else {
		for(i = 0; i < ARRAYSIZE(pinst.readType); ++i) {
			if( pinst.readType[i] == XMMTYPE_TEMP ) {
				pinst.readType[i] = type;
				pinst.readReg[i] = reg;
				return;
			}
		}
		assert(0);
	}
}

void SetMMXstate() {
	x86FpuState = MMX_STATE;
}

////////////////////////////////////////////////////
//#include "R3000A.h"
//#include "PsxCounters.h"
//#include "PsxMem.h"
//extern tIPU_BP g_BP;

#if 0
extern u32 psxdump;
extern void iDumpPsxRegisters(u32 startpc, u32 temp); 
extern Counter counters[6];
extern int rdram_devices;	// put 8 for TOOL and 2 for PS2 and PSX
extern int rdram_sdevid;
#endif

void iDumpRegisters(u32 startpc, u32 temp)
{
// [TODO] fixme : this code is broken and has no labels.  Needs a rewrite to be useful.

#if 0

	int i;
	const char* pstr;// = temp ? "t" : "";
	const u32 dmacs[] = {0x8000, 0x9000, 0xa000, 0xb000, 0xb400, 0xc000, 0xc400, 0xc800, 0xd000, 0xd400 };
    const char* psymb;
	
	if (temp)
		pstr = "t";
	else
		pstr = "";
	
    psymb = disR5900GetSym(startpc);

    if( psymb != NULL )
        __Log("%sreg(%s): %x %x c:%x\n", pstr, psymb, startpc, cpuRegs.interrupt, cpuRegs.cycle);
    else
        __Log("%sreg: %x %x c:%x\n", pstr, startpc, cpuRegs.interrupt, cpuRegs.cycle);
	for(i = 1; i < 32; ++i) __Log("%s: %x_%x_%x_%x\n", disRNameGPR[i], cpuRegs.GPR.r[i].UL[3], cpuRegs.GPR.r[i].UL[2], cpuRegs.GPR.r[i].UL[1], cpuRegs.GPR.r[i].UL[0]);
    //for(i = 0; i < 32; i+=4) __Log("cp%d: %x_%x_%x_%x\n", i, cpuRegs.CP0.r[i], cpuRegs.CP0.r[i+1], cpuRegs.CP0.r[i+2], cpuRegs.CP0.r[i+3]);
	//for(i = 0; i < 32; ++i) __Log("%sf%d: %f %x\n", pstr, i, fpuRegs.fpr[i].f, fpuRegs.fprc[i]);
	//for(i = 1; i < 32; ++i) __Log("%svf%d: %f %f %f %f, vi: %x\n", pstr, i, VU0.VF[i].F[3], VU0.VF[i].F[2], VU0.VF[i].F[1], VU0.VF[i].F[0], VU0.VI[i].UL);
	for(i = 0; i < 32; ++i) __Log("%sf%d: %x %x\n", pstr, i, fpuRegs.fpr[i].UL, fpuRegs.fprc[i]);
	for(i = 1; i < 32; ++i) __Log("%svf%d: %x %x %x %x, vi: %x\n", pstr, i, VU0.VF[i].UL[3], VU0.VF[i].UL[2], VU0.VF[i].UL[1], VU0.VF[i].UL[0], VU0.VI[i].UL);
	__Log("%svfACC: %x %x %x %x\n", pstr, VU0.ACC.UL[3], VU0.ACC.UL[2], VU0.ACC.UL[1], VU0.ACC.UL[0]);
	__Log("%sLO: %x_%x_%x_%x, HI: %x_%x_%x_%x\n", pstr, cpuRegs.LO.UL[3], cpuRegs.LO.UL[2], cpuRegs.LO.UL[1], cpuRegs.LO.UL[0],
	cpuRegs.HI.UL[3], cpuRegs.HI.UL[2], cpuRegs.HI.UL[1], cpuRegs.HI.UL[0]);
	__Log("%sCycle: %x %x, Count: %x\n", pstr, cpuRegs.cycle, g_nextBranchCycle, cpuRegs.CP0.n.Count);
	iDumpPsxRegisters(psxRegs.pc, temp);

    __Log("f410,30,40: %x %x %x, %d %d\n", psHu32(0xf410), psHu32(0xf430), psHu32(0xf440), rdram_sdevid, rdram_devices);
	__Log("cyc11: %x %x; vu0: %x, vu1: %x\n", cpuRegs.sCycle[1], cpuRegs.eCycle[1], VU0.cycle, VU1.cycle);

	__Log("%scounters: %x %x; psx: %x %x\n", pstr, nextsCounter, nextCounter, psxNextsCounter, psxNextCounter);
	for(i = 0; i < 4; ++i) {
		__Log("eetimer%d: count: %x mode: %x target: %x %x; %x %x; %x %x %x %x\n", i,
			counters[i].count, counters[i].mode, counters[i].target, counters[i].hold, counters[i].rate,
			counters[i].interrupt, counters[i].Cycle, counters[i].sCycle, counters[i].CycleT, counters[i].sCycleT);
	}
	__Log("VIF0_STAT = %x, VIF1_STAT = %x\n", psHu32(0x3800), psHu32(0x3C00));
	__Log("ipu %x %x %x %x; bp: %x %x %x %x\n", psHu32(0x2000), psHu32(0x2010), psHu32(0x2020), psHu32(0x2030), g_BP.BP, g_BP.bufferhasnew, g_BP.FP, g_BP.IFC);
	__Log("gif: %x %x %x\n", psHu32(0x3000), psHu32(0x3010), psHu32(0x3020));
	for(i = 0; i < ARRAYSIZE(dmacs); ++i) {
		DMACh* p = (DMACh*)(PS2MEM_HW+dmacs[i]);
		__Log("dma%d c%x m%x q%x t%x s%x\n", i, p->chcr, p->madr, p->qwc, p->tadr, p->sadr);
	}
	__Log("dmac %x %x %x %x\n", psHu32(DMAC_CTRL), psHu32(DMAC_STAT), psHu32(DMAC_RBSR), psHu32(DMAC_RBOR));
	__Log("intc %x %x\n", psHu32(INTC_STAT), psHu32(INTC_MASK));
	__Log("sif: %x %x %x %x %x\n", psHu32(0xf200), psHu32(0xf220), psHu32(0xf230), psHu32(0xf240), psHu32(0xf260));
#endif
}
