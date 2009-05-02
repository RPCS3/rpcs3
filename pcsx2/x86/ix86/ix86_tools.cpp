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

#include "System.h"
#include "ix86.h"

// used to make sure regs don't get changed while in recompiler
// use FreezeMMXRegs, FreezeXMMRegs

u8 g_globalMMXSaved = 0;
u8 g_globalXMMSaved = 0;

PCSX2_ALIGNED16( static u64 g_globalMMXData[8] );
PCSX2_ALIGNED16( static u64 g_globalXMMData[2*iREGCNT_XMM] );

namespace x86Emitter
{
	void xStoreReg( const xRegisterSSE& src )
	{
		xMOVDQA( &g_globalXMMData[src.Id*2], src );
	}

	void xRestoreReg( const xRegisterSSE& dest )
	{
		xMOVDQA( dest, &g_globalXMMData[dest.Id*2] );
	}
}


/////////////////////////////////////////////////////////////////////
// SetCPUState -- for assignment of SSE roundmodes and clampmodes.

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
    __asm__("ldmxcsr %[g_sseMXCSR]" : : [g_sseMXCSR]"m"(g_sseMXCSR) );
#endif
	//g_sseVUMXCSR = g_sseMXCSR|0x6000;
}

/////////////////////////////////////////////////////////////////////
// MMX Register Freezing
#ifndef __INTEL_COMPILER
extern "C"
{
#endif

__forceinline void FreezeRegs(int save)
{
	FreezeXMMRegs(save);
	FreezeMMXRegs(save);
}

__forceinline void FreezeMMXRegs_(int save)
{
	//DevCon::Notice("FreezeMMXRegs_(%d); [%d]\n", save, g_globalMMXSaved);
	assert( g_EEFreezeRegs );

	if( save ) {
		g_globalMMXSaved++;
		if( g_globalMMXSaved>1 )
		{
			//DevCon::Notice("MMX Already Saved!\n");
			return;
		}

#ifdef _MSC_VER
		__asm {
			mov ecx, offset g_globalMMXData
			movntq mmword ptr [ecx+0], mm0
			movntq mmword ptr [ecx+8], mm1
			movntq mmword ptr [ecx+16], mm2
			movntq mmword ptr [ecx+24], mm3
			movntq mmword ptr [ecx+32], mm4
			movntq mmword ptr [ecx+40], mm5
			movntq mmword ptr [ecx+48], mm6
			movntq mmword ptr [ecx+56], mm7
			emms
		}
#else
        __asm__(
			".intel_syntax noprefix\n"
			"movq [%[g_globalMMXData]+0x00], mm0\n"
			"movq [%[g_globalMMXData]+0x08], mm1\n"
			"movq [%[g_globalMMXData]+0x10], mm2\n"
			"movq [%[g_globalMMXData]+0x18], mm3\n"
			"movq [%[g_globalMMXData]+0x20], mm4\n"
			"movq [%[g_globalMMXData]+0x28], mm5\n"
			"movq [%[g_globalMMXData]+0x30], mm6\n"
			"movq [%[g_globalMMXData]+0x38], mm7\n"
			"emms\n"
			".att_syntax\n" : : [g_globalMMXData]"r"(g_globalMMXData)
		);
#endif

	}
	else {
		if( g_globalMMXSaved==0 )
		{
			//DevCon::Notice("MMX Not Saved!\n");
			return;
		}
		g_globalMMXSaved--;

		if( g_globalMMXSaved > 0 ) return;

#ifdef _MSC_VER
		__asm {
			mov ecx, offset g_globalMMXData
			movq mm0, mmword ptr [ecx+0]
			movq mm1, mmword ptr [ecx+8]
			movq mm2, mmword ptr [ecx+16]
			movq mm3, mmword ptr [ecx+24]
			movq mm4, mmword ptr [ecx+32]
			movq mm5, mmword ptr [ecx+40]
			movq mm6, mmword ptr [ecx+48]
			movq mm7, mmword ptr [ecx+56]
			emms
		}
#else
        __asm__(
			".intel_syntax noprefix\n"
			"movq mm0, [%[g_globalMMXData]+0x00]\n"
			"movq mm1, [%[g_globalMMXData]+0x08]\n"
			"movq mm2, [%[g_globalMMXData]+0x10]\n"
			"movq mm3, [%[g_globalMMXData]+0x18]\n"
			"movq mm4, [%[g_globalMMXData]+0x20]\n"
			"movq mm5, [%[g_globalMMXData]+0x28]\n"
			"movq mm6, [%[g_globalMMXData]+0x30]\n"
			"movq mm7, [%[g_globalMMXData]+0x38]\n"
			"emms\n"
			".att_syntax\n" : :  [g_globalMMXData]"r"(g_globalMMXData)
		);
#endif
	}
}

//////////////////////////////////////////////////////////////////////
// XMM Register Freezing
__forceinline void FreezeXMMRegs_(int save)
{
	//DevCon::Notice("FreezeXMMRegs_(%d); [%d]\n", save, g_globalXMMSaved);
	assert( g_EEFreezeRegs );

	if( save )
	{
		g_globalXMMSaved++;
		if( g_globalXMMSaved > 1 ){
			//DevCon::Notice("XMM Already saved\n");
			return;
		}


#ifdef _MSC_VER
        __asm {
			mov ecx, offset g_globalXMMData
			movaps xmmword ptr [ecx+0x00], xmm0
			movaps xmmword ptr [ecx+0x10], xmm1
			movaps xmmword ptr [ecx+0x20], xmm2
			movaps xmmword ptr [ecx+0x30], xmm3
			movaps xmmword ptr [ecx+0x40], xmm4
			movaps xmmword ptr [ecx+0x50], xmm5
			movaps xmmword ptr [ecx+0x60], xmm6
			movaps xmmword ptr [ecx+0x70], xmm7
        }

#else
        __asm__(
			".intel_syntax noprefix\n"
			"movaps [%[g_globalXMMData]+0x00], xmm0\n"
			"movaps [%[g_globalXMMData]+0x10], xmm1\n"
			"movaps [%[g_globalXMMData]+0x20], xmm2\n"
			"movaps [%[g_globalXMMData]+0x30], xmm3\n"
			"movaps [%[g_globalXMMData]+0x40], xmm4\n"
			"movaps [%[g_globalXMMData]+0x50], xmm5\n"
			"movaps [%[g_globalXMMData]+0x60], xmm6\n"
			"movaps [%[g_globalXMMData]+0x70], xmm7\n"
			".att_syntax\n" : :  [g_globalXMMData]"r"(g_globalXMMData)
		);

#endif // _MSC_VER
	}
	else
	{
		if( g_globalXMMSaved==0 )
		{
			//DevCon::Notice("XMM Regs not saved!\n");
			return;
		}

        // TODO: really need to backup all regs?
		g_globalXMMSaved--;
		if( g_globalXMMSaved > 0 ) return;

#ifdef _MSC_VER
        __asm
        {
			mov ecx, offset g_globalXMMData
			movaps xmm0, xmmword ptr [ecx+0x00]
			movaps xmm1, xmmword ptr [ecx+0x10]
			movaps xmm2, xmmword ptr [ecx+0x20]
			movaps xmm3, xmmword ptr [ecx+0x30]
			movaps xmm4, xmmword ptr [ecx+0x40]
			movaps xmm5, xmmword ptr [ecx+0x50]
			movaps xmm6, xmmword ptr [ecx+0x60]
			movaps xmm7, xmmword ptr [ecx+0x70]
        }

#else
        __asm__(
			".intel_syntax noprefix\n"
			"movaps xmm0, [%[g_globalXMMData]+0x00]\n"
			"movaps xmm1, [%[g_globalXMMData]+0x10]\n"
			"movaps xmm2, [%[g_globalXMMData]+0x20]\n"
			"movaps xmm3, [%[g_globalXMMData]+0x30]\n"
			"movaps xmm4, [%[g_globalXMMData]+0x40]\n"
			"movaps xmm5, [%[g_globalXMMData]+0x50]\n"
			"movaps xmm6, [%[g_globalXMMData]+0x60]\n"
			"movaps xmm7, [%[g_globalXMMData]+0x70]\n"
			".att_syntax\n" : : [g_globalXMMData]"r"(g_globalXMMData)
		);

#endif // _MSC_VER
	}
}
#ifndef __INTEL_COMPILER
}
#endif

