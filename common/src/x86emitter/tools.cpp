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
#include "internal.h"
#include "tools.h"

// To make sure regs don't get changed while in the recompiler,
// use Freeze/Thaw in MMXRegisters, XMMRegisters, & Registers.

__aligned16 u64 g_globalMMXData[8];
__aligned16 u64 g_globalXMMData[2*iREGCNT_XMM];

/////////////////////////////////////////////////////////////////////
// MMX Register Freezing
//

namespace MMXRegisters
{
    u8 g_globalMMXSaved = 0;
    
    __forceinline u8 Depth()
    {
        return g_globalMMXSaved;
    }
    
    __forceinline bool Saved()
    {
        return ( Depth() > 0);
    }
    
    __forceinline bool SavedRepeatedly()
    {
        return ( Depth() > 1);
    }
    
    __forceinline void Freeze()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Notice("FreezeMMXRegs_(%d); [%d]\n", save, g_globalMMXSaved);
            g_globalMMXSaved++;
		
        if (SavedRepeatedly())
        {
            //DevCon.Notice("MMX Already Saved!\n");
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
        __asm__ volatile(
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
            ".att_syntax\n" : : [g_globalMMXData]"r"(g_globalMMXData) : "memory"
        );
#endif
    }
    
    __forceinline void Thaw()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Notice("FreezeMMXRegs_(%d); [%d]\n", save, g_globalMMXSaved);
            
        if (!Saved())
        {
            //DevCon.Notice("MMX Not Saved!\n");
            return;
        }
        g_globalMMXSaved--;

        if (Saved()) return;
		
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
        __asm__ volatile(
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
            ".att_syntax\n" : :  [g_globalMMXData]"r"(g_globalMMXData) : "memory"
        );
#endif
    }  
}

//////////////////////////////////////////////////////////////////////
// XMM Register Freezing
//

namespace XMMRegisters
{  
    u8 g_globalXMMSaved = 0;

    __forceinline u8 Depth()
    {
        return g_globalXMMSaved;
    }
    
    __forceinline bool Saved()
    {
        return ( Depth() > 0);
    }
    
    __forceinline bool SavedRepeatedly()
    {
        return ( Depth() > 1);
    }
    
    __forceinline void Freeze()
    {
        if (!g_EEFreezeRegs) return;
            
        //DevCon.Notice("FreezeXMMRegs_(%d); [%d]\n", save, g_globalXMMSaved);
            
        g_globalXMMSaved++;
        
        if (SavedRepeatedly())
        {
            //DevCon.Notice("XMM Already saved\n");
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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movaps [%[g_globalXMMData]+0x00], xmm0\n"
            "movaps [%[g_globalXMMData]+0x10], xmm1\n"
            "movaps [%[g_globalXMMData]+0x20], xmm2\n"
            "movaps [%[g_globalXMMData]+0x30], xmm3\n"
            "movaps [%[g_globalXMMData]+0x40], xmm4\n"
            "movaps [%[g_globalXMMData]+0x50], xmm5\n"
            "movaps [%[g_globalXMMData]+0x60], xmm6\n"
            "movaps [%[g_globalXMMData]+0x70], xmm7\n"
            ".att_syntax\n" : : [g_globalXMMData]"r"(g_globalXMMData) : "memory"
        );
#endif // _MSC_VER
    }  
        
    __forceinline void Thaw()
    {
        if (!g_EEFreezeRegs) return;

        //DevCon.Notice("FreezeXMMRegs_(%d); [%d]\n", save, g_globalXMMSaved);
            
       if (!Saved())
        {
            //DevCon.Notice("XMM Regs not saved!\n");
            return;
        }

        // TODO: really need to backup all regs?
        g_globalXMMSaved--;
        if (Saved()) return;

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
        __asm__ volatile(
            ".intel_syntax noprefix\n"
            "movaps xmm0, [%[g_globalXMMData]+0x00]\n"
            "movaps xmm1, [%[g_globalXMMData]+0x10]\n"
            "movaps xmm2, [%[g_globalXMMData]+0x20]\n"
            "movaps xmm3, [%[g_globalXMMData]+0x30]\n"
            "movaps xmm4, [%[g_globalXMMData]+0x40]\n"
            "movaps xmm5, [%[g_globalXMMData]+0x50]\n"
            "movaps xmm6, [%[g_globalXMMData]+0x60]\n"
            "movaps xmm7, [%[g_globalXMMData]+0x70]\n"
            ".att_syntax\n" : : [g_globalXMMData]"r"(g_globalXMMData) : "memory"
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
    
    __forceinline bool SavedRepeatedly()
    {
        return (XMMRegisters::SavedRepeatedly() || MMXRegisters::SavedRepeatedly());
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
