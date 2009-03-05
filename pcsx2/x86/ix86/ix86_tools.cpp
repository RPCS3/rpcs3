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
#include "ix86/ix86.h"

// used to make sure regs don't get changed while in recompiler
// use FreezeMMXRegs, FreezeXMMRegs

u8 g_globalMMXSaved = 0;
u8 g_globalXMMSaved = 0;

PCSX2_ALIGNED16( static u64 g_globalMMXData[8] );
PCSX2_ALIGNED16( static u64 g_globalXMMData[2*XMMREGS] );


/////////////////////////////////////////////////////////////////////
// SetCPUState -- for assugnment of SSE roundmodes and clampmodes.

u32 g_sseMXCSR = DEFAULT_sseMXCSR; 
u32 g_sseVUMXCSR = DEFAULT_sseVUMXCSR;

void SetCPUState(u32 sseMXCSR, u32 sseVUMXCSR)
{
	//Msgbox::Alert("SetCPUState: Config.sseMXCSR = %x; Config.sseVUMXCSR = %x \n", Config.sseMXCSR, Config.sseVUMXCSR);
	// SSE STATE //
	// WARNING: do not touch unless you know what you are doing

	sseMXCSR &= 0xffff; // clear the upper 16 bits since they shouldn't be set
	sseVUMXCSR &= 0xffff;

	if( !cpucaps.hasStreamingSIMD2Extensions )
	{
		// SSE1 cpus do not support Denormals Are Zero flag (throws an exception
		// if we don't mask them off)

		sseMXCSR &= ~0x0040;
		sseVUMXCSR &= ~0x0040;
	}

	g_sseMXCSR = sseMXCSR;
	g_sseVUMXCSR = sseVUMXCSR;

#ifdef _MSC_VER
	__asm ldmxcsr g_sseMXCSR; // set the new sse control
#else
    __asm__("ldmxcsr %0" : : "m"(g_sseMXCSR) );
#endif
	//g_sseVUMXCSR = g_sseMXCSR|0x6000;
}

/////////////////////////////////////////////////////////////////////
//
#ifndef __INTEL_COMPILER
extern "C"
{
#endif
__forceinline void FreezeMMXRegs_(int save)
{
	assert( g_EEFreezeRegs );

	if( save ) {
		g_globalMMXSaved++;
		if( g_globalMMXSaved>1 )
		{
			//SysPrintf("MMX Already Saved!\n");
			return;
		}

#ifdef _MSC_VER
		__asm {
			movntq mmword ptr [g_globalMMXData + 0], mm0
			movntq mmword ptr [g_globalMMXData + 8], mm1
			movntq mmword ptr [g_globalMMXData + 16], mm2
			movntq mmword ptr [g_globalMMXData + 24], mm3
			movntq mmword ptr [g_globalMMXData + 32], mm4
			movntq mmword ptr [g_globalMMXData + 40], mm5
			movntq mmword ptr [g_globalMMXData + 48], mm6
			movntq mmword ptr [g_globalMMXData + 56], mm7
			emms
		}
#else
        __asm__(".intel_syntax noprefix\n"
                "movq [%0+0x00], mm0\n"
                "movq [%0+0x08], mm1\n"
                "movq [%0+0x10], mm2\n"
                "movq [%0+0x18], mm3\n"
                "movq [%0+0x20], mm4\n"
                "movq [%0+0x28], mm5\n"
                "movq [%0+0x30], mm6\n"
                "movq [%0+0x38], mm7\n"
                "emms\n"
                ".att_syntax\n" : : "r"(g_globalMMXData) );
#endif

	}
	else {
		if( g_globalMMXSaved==0 )
		{
			//SysPrintf("MMX Not Saved!\n");
			return;
		}
		g_globalMMXSaved--;

		if( g_globalMMXSaved > 0 ) return;

#ifdef _MSC_VER
		__asm {
			movq mm0, mmword ptr [g_globalMMXData + 0]
			movq mm1, mmword ptr [g_globalMMXData + 8]
			movq mm2, mmword ptr [g_globalMMXData + 16]
			movq mm3, mmword ptr [g_globalMMXData + 24]
			movq mm4, mmword ptr [g_globalMMXData + 32]
			movq mm5, mmword ptr [g_globalMMXData + 40]
			movq mm6, mmword ptr [g_globalMMXData + 48]
			movq mm7, mmword ptr [g_globalMMXData + 56]
			emms
		}
#else
        __asm__(".intel_syntax noprefix\n"
                "movq mm0, [%0+0x00]\n"
                "movq mm1, [%0+0x08]\n"
                "movq mm2, [%0+0x10]\n"
                "movq mm3, [%0+0x18]\n"
                "movq mm4, [%0+0x20]\n"
                "movq mm5, [%0+0x28]\n"
                "movq mm6, [%0+0x30]\n"
                "movq mm7, [%0+0x38]\n"
                "emms\n"
                ".att_syntax\n" : : "r"(g_globalMMXData) );
#endif
	}
}

//////////////////////////////////////////////////////////////////////

__forceinline void FreezeXMMRegs_(int save)
{
	//SysPrintf("FreezeXMMRegs_(%d); [%d]\n", save, g_globalXMMSaved);
	assert( g_EEFreezeRegs );

	if( save ) {
		g_globalXMMSaved++;
		if( g_globalXMMSaved > 1 ){
			//SysPrintf("XMM Already saved\n");
			return;
		}


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
        __asm__(".intel_syntax noprefix\n"
                "movaps [%0+0x00], xmm0\n"
                "movaps [%0+0x10], xmm1\n"
                "movaps [%0+0x20], xmm2\n"
                "movaps [%0+0x30], xmm3\n"
                "movaps [%0+0x40], xmm4\n"
                "movaps [%0+0x50], xmm5\n"
                "movaps [%0+0x60], xmm6\n"
                "movaps [%0+0x70], xmm7\n"
                ".att_syntax\n" : : "r"(g_globalXMMData) );

#endif // _MSC_VER
	}
	else {
		if( g_globalXMMSaved==0 )
		{
			//SysPrintf("XMM Regs not saved!\n");
			return;
		}

        // TODO: really need to backup all regs?
		g_globalXMMSaved--;
		if( g_globalXMMSaved > 0 ) return;

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
        __asm__(".intel_syntax noprefix\n"
                "movaps xmm0, [%0+0x00]\n"
                "movaps xmm1, [%0+0x10]\n"
                "movaps xmm2, [%0+0x20]\n"
                "movaps xmm3, [%0+0x30]\n"
                "movaps xmm4, [%0+0x40]\n"
                "movaps xmm5, [%0+0x50]\n"
                "movaps xmm6, [%0+0x60]\n"
                "movaps xmm7, [%0+0x70]\n"
                ".att_syntax\n" : : "r"(g_globalXMMData) );

#endif // _MSC_VER
	}
}
#ifndef __INTEL_COMPILER
}
#endif