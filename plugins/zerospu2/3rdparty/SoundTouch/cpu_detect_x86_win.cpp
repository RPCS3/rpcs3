////////////////////////////////////////////////////////////////////////////////
///
/// Win32 version of the x86 CPU detect routine.
///
/// This file is to be compiled in Windows platform with Microsoft Visual C++ 
/// Compiler. Please see 'cpu_detect_x86_gcc.cpp' for the gcc compiler version 
/// for all GNU platforms.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.10 $
//
// $Id: cpu_detect_x86_win.cpp,v 1.10 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include "cpu_detect.h"

#ifndef _WIN32
#error wrong platform - this source code file is exclusively for Win32 platform
#endif

//////////////////////////////////////////////////////////////////////////////
//
// processor instructions extension detection routines
//
//////////////////////////////////////////////////////////////////////////////

// Flag variable indicating whick ISA extensions are disabled (for debugging)
static uint _dwDisabledISA = 0x00;      // 0xffffffff; //<- use this to disable all extensions


// Disables given set of instruction extensions. See SUPPORT_... defines.
void disableExtensions(uint dwDisableMask)
{
    _dwDisabledISA = dwDisableMask;
}



/// Checks which instruction set extensions are supported by the CPU.
uint detectCPUextensions(void)
{
    uint res = 0;

    if (_dwDisabledISA == 0xffffffff) return 0;

    _asm 
    {
        ; check if 'cpuid' instructions is available by toggling eflags bit 21
        ;
        xor     esi, esi            ; clear esi = result register

        pushfd                      ; save eflags to stack
        pop     eax                 ; load eax from stack (with eflags)
        mov     ecx, eax            ; save the original eflags values to ecx
        xor     eax, 0x00200000     ; toggle bit 21
        push    eax                 ; store toggled eflags to stack
        popfd                       ; load eflags from stack
        pushfd                      ; save updated eflags to stack
        pop     eax                 ; load from stack
        xor     edx, edx            ; clear edx for defaulting no mmx
        cmp     eax, ecx            ; compare to original eflags values
        jz      end                 ; jumps to 'end' if cpuid not present

        ; cpuid instruction available, test for presence of mmx instructions 
        mov     eax, 1
        cpuid
        test    edx, 0x00800000
        jz      end                 ; branch if MMX not available

        or      esi, SUPPORT_MMX    ; otherwise add MMX support bit

        test    edx, 0x02000000
        jz      test3DNow           ; branch if SSE not available

        or      esi, SUPPORT_SSE    ; otherwise add SSE support bit

    test3DNow:
        ; test for precense of AMD extensions
        mov     eax, 0x80000000
        cpuid
        cmp     eax, 0x80000000
        jbe     end                ; branch if no AMD extensions detected

        ; test for precense of 3DNow! extension
        mov     eax, 0x80000001
        cpuid
        test    edx, 0x80000000
        jz      end                 ; branch if 3DNow! not detected

        or      esi, SUPPORT_3DNOW  ; otherwise add 3DNow support bit

    end:

        mov     res, esi
    }

    return res & ~_dwDisabledISA;
}
