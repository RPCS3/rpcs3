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
#include "tools.h"

// To make sure regs don't get changed while in the recompiler,
// use Freeze/Thaw in MMXRegisters, XMMRegisters, & Registers.


// used to disable register freezing during cpuBranchTests (registers
// are safe then since they've been completely flushed)
bool g_EEFreezeRegs = false;

/////////////////////////////////////////////////////////////////////
// MMX Register Freezing
//

namespace MMXRegisters
{
    u8 stack_depth = 0;
    __aligned16 u64 data[8];

    __forceinline bool Saved()
    {
        return ( stack_depth > 0);
    }

    __forceinline void Freeze()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Warning("MMXRegisters::Freeze: depth[%d]\n", stack_depth);
        stack_depth++;

        if (stack_depth > 1)
        {
            //DevCon.Warning("MMX Already Saved!\n");
            return;
        }

#ifdef _MSC_VER
        __asm {
            mov ecx, offset data
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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movq [%[data]+0x00], mm0\n"
            "movq [%[data]+0x08], mm1\n"
            "movq [%[data]+0x10], mm2\n"
            "movq [%[data]+0x18], mm3\n"
            "movq [%[data]+0x20], mm4\n"
            "movq [%[data]+0x28], mm5\n"
            "movq [%[data]+0x30], mm6\n"
            "movq [%[data]+0x38], mm7\n"
            "emms\n"
            ".att_syntax\n" : : [data]"r"(data) : "memory"
        );
#endif
    }

    __forceinline void Thaw()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Warning("MMXRegisters::Thaw: depth[%d]\n", stack_depth);

        if (!Saved())
        {
            //DevCon.Warning("MMX Not Saved!\n");
            return;
        }
        stack_depth--;

        if (Saved()) return;

#ifdef _MSC_VER
        __asm {
            mov ecx, offset data
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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movq mm0, [%[data]+0x00]\n"
            "movq mm1, [%[data]+0x08]\n"
            "movq mm2, [%[data]+0x10]\n"
            "movq mm3, [%[data]+0x18]\n"
            "movq mm4, [%[data]+0x20]\n"
            "movq mm5, [%[data]+0x28]\n"
            "movq mm6, [%[data]+0x30]\n"
            "movq mm7, [%[data]+0x38]\n"
            "emms\n"
            ".att_syntax\n" : :  [data]"r"(data) : "memory"
        );
#endif
    }
}

//////////////////////////////////////////////////////////////////////
// XMM Register Freezing
//

namespace XMMRegisters
{
    u8 stack_depth = 0;
    __aligned16 u64 data[2*iREGCNT_XMM];

    __forceinline bool Saved()
    {
        return ( stack_depth > 0);
    }

    __forceinline void Freeze()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Warning("XMMRegisters::Freeze: depth[%d]\n", Depth());

        stack_depth++;

        if (stack_depth > 1)
        {
            //DevCon.Warning("XMM Already saved\n");
            return;
        }

#ifdef _MSC_VER
        __asm {
            mov ecx, offset data
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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movaps [%[data]+0x00], xmm0\n"
            "movaps [%[data]+0x10], xmm1\n"
            "movaps [%[data]+0x20], xmm2\n"
            "movaps [%[data]+0x30], xmm3\n"
            "movaps [%[data]+0x40], xmm4\n"
            "movaps [%[data]+0x50], xmm5\n"
            "movaps [%[data]+0x60], xmm6\n"
            "movaps [%[data]+0x70], xmm7\n"
            ".att_syntax\n" : : [data]"r"(data) : "memory"
        );
#endif // _MSC_VER
    }

    __forceinline void Thaw()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Warning("XMMRegisters::Thaw: depth[%d]\n", Depth());

        if (!Saved())
        {
            //DevCon.Warning("XMM Regs not saved!\n");
            return;
        }

        // TODO: really need to backup all regs?
        stack_depth--;
        if (Saved()) return;

#ifdef _MSC_VER
        __asm
        {
            mov ecx, offset data
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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movaps xmm0, [%[data]+0x00]\n"
            "movaps xmm1, [%[data]+0x10]\n"
            "movaps xmm2, [%[data]+0x20]\n"
            "movaps xmm3, [%[data]+0x30]\n"
            "movaps xmm4, [%[data]+0x40]\n"
            "movaps xmm5, [%[data]+0x50]\n"
            "movaps xmm6, [%[data]+0x60]\n"
            "movaps xmm7, [%[data]+0x70]\n"
            ".att_syntax\n" : : [data]"r"(data) : "memory"
        );
#endif // _MSC_VER
    }
};

//////////////////////////////////////////////////////////////////////
// Register Freezing
//

namespace Registers
{
    __forceinline bool Saved()
    {
        return (XMMRegisters::Saved() || MMXRegisters::Saved());
    }

    __forceinline void Freeze()
    {
        XMMRegisters::Freeze();
        MMXRegisters::Freeze();
    }

    __forceinline void Thaw()
    {
        XMMRegisters::Thaw();
        MMXRegisters::Thaw();
    }
}
