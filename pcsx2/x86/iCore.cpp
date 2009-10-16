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

#include "System.h"
#include "iR5900.h"
#include "Vif.h"
#include "VU.h"
#include "R3000A.h"

__threadlocal u8  *j8Ptr[32];
__threadlocal u32 *j32Ptr[32];

u16 g_x86AllocCounter = 0;
u16 g_xmmAllocCounter = 0;

EEINST* g_pCurInstInfo = NULL;

u32 g_cpuRegHasLive1 = 0, g_cpuPrevRegHasLive1 = 0; // set if upper 32 bits are live
u32 g_cpuRegHasSignExt = 0, g_cpuPrevRegHasSignExt = 0; // set if upper 32 bits are the sign extension of the lower integer

// used to make sure regs don't get changed while in recompiler
// use FreezeMMXRegs, FreezeXMMRegs
u32 g_recWriteback = 0;

_xmmregs xmmregs[iREGCNT_XMM], s_saveXMMregs[iREGCNT_XMM];

// X86 caching
_x86regs x86regs[iREGCNT_GPR], s_saveX86regs[iREGCNT_GPR];

// XMM Caching
#define VU_VFx_ADDR(x)  (uptr)&VU->VF[x].UL[0]
#define VU_ACCx_ADDR    (uptr)&VU->ACC.UL[0]

static int s_xmmchecknext = 0;

void _initXMMregs() {
	memzero( xmmregs );
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
		
		jNO_DEFAULT
	}
	
	return NULL;
}

int  _getFreeXMMreg()
{
	int i, tempi;
	u32 bestcount = 0x10000;

	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[(i+s_xmmchecknext)%iREGCNT_XMM].inuse == 0) {
			int ret = (s_xmmchecknext+i)%iREGCNT_XMM;
			s_xmmchecknext = (s_xmmchecknext+i+1)%iREGCNT_XMM;
			return ret;
		}
	}

	// check for dead regs
	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			if (!(EEINST_ISLIVEXMM(xmmregs[i].reg))) {
				_freeXMMreg(i);
				return i;
			}
		}
	}

	// check for future xmm usage
	for (i=0; i<iREGCNT_XMM; i++) {
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
	for (i=0; i<iREGCNT_XMM; i++) {
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
	Console.Error("*PCSX2*: XMM Reg Allocation Error in _getFreeXMMreg()!");

	return -1;
}

int _allocTempXMMreg(XMMSSEType type, int xmmreg) {
	if (xmmreg == -1) 
		xmmreg = _getFreeXMMreg();
	else 
		_freeXMMreg(xmmreg);

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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) 
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_ACC) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _addNeededFPtoXMMreg(int fpreg) {
	int i;

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPACC) continue;

		xmmregs[i].counter = g_xmmAllocCounter++; // update counter
		xmmregs[i].needed = 1;
		break;
	}
}

void _clearNeededXMMregs() {
	int i;

	for (i=0; i<iREGCNT_XMM; i++) {

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
	
	for (i=0; i<iREGCNT_XMM; i++) 
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
								
								for (t0reg = 0; t0reg < iREGCNT_XMM; ++t0reg) 
								{
									if (!xmmregs[t0reg].inuse ) 
										break;
								}

								if (t0reg < iREGCNT_XMM ) 
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
	
	for (i=0; i<iREGCNT_XMM; i++) {
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
								for(t0reg = 0; t0reg < iREGCNT_XMM; ++t0reg ) {
									if( !xmmregs[t0reg].inuse ) break;
								}

								if( t0reg < iREGCNT_XMM ) {
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
	for (i=0; i<iREGCNT_XMM; i++) {

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
	for (i=0; i<iREGCNT_XMM; i++) {
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
	assert( xmmreg < iREGCNT_XMM );

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
					for(t0reg = 0; t0reg < iREGCNT_XMM; ++t0reg ) {
						if( !xmmregs[t0reg].inuse ) break;
					}

					if( t0reg < iREGCNT_XMM ) 
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
					
					for(t0reg = 0; t0reg < iREGCNT_XMM; ++t0reg ) {
						if( !xmmregs[t0reg].inuse ) break;
					}

					if( t0reg < iREGCNT_XMM ) 
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
	for (i=0; i<iREGCNT_XMM; i++) {
		if( xmmregs[i].inuse && (xmmregs[i].mode&MODE_WRITE) ) ++num;
	}

	return num;
}

u8 _hasFreeXMMreg()
{
	int i;

	for (i=0; i<iREGCNT_XMM; i++) {
		if (!xmmregs[i].inuse) return 1;
	}

	// check for dead regs
	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			if( !EEINST_ISLIVEXMM(xmmregs[i].reg) ) {
				return 1;
			}
		}
	}

	// check for dead regs
	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].inuse) continue;
		break;
	}

	if( i == iREGCNT_XMM ) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
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

	for (i=0; i<iREGCNT_XMM; i++) {
		if (xmmregs[i].inuse == 0) continue;

		assert( xmmregs[i].type != XMMTYPE_TEMP );
		//assert( xmmregs[i].mode & (MODE_READ|MODE_WRITE) );

		_freeXMMreg(i);
	}
}

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
	memzero( *pinst );
	memset8<EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2>( pinst->regs );
	memset8<EEINST_LIVE0>( pinst->fpuregs );
}

// returns nonzero value if reg has been written between [startpc, endpc-4]
u32 _recIsRegWritten(EEINST* pinst, int size, u8 xmmtype, u8 reg)
{
	u32  i, inst = 1;
	
	while(size-- > 0) {
		for(i = 0; i < ArraySize(pinst->writeType); ++i) {
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
		for(i = 0; i < ArraySize(pinst->writeType); ++i) {
			if( pinst->writeType[i] == xmmtype && pinst->writeReg[i] == reg )
				return inst;
		}
		for(i = 0; i < ArraySize(pinst->readType); ++i) {
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
		for(i = 0; i < ArraySize(pinst.writeType); ++i) {
			if( pinst.writeType[i] == XMMTYPE_TEMP ) {
				pinst.writeType[i] = type;
				pinst.writeReg[i] = reg;
				return;
			}
		}
		assert(0);
	}
	else {
		for(i = 0; i < ArraySize(pinst.readType); ++i) {
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
