////////////////////////////////////////////////////////////////////////////////
///
/// Generic version of the x86 CPU extension detection routine.
///
/// This file is for GNU & other non-Windows compilers, see 'cpu_detect_x86_win.cpp' 
/// for the Microsoft compiler version.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2009-02-25 19:13:51 +0200 (Wed, 25 Feb 2009) $
// File revision : $Revision: 4 $
//
// $Id: cpu_detect_x86_gcc.cpp 67 2009-02-25 17:13:51Z oparviai $
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

#include <stdexcept>
#include <string>
#include "cpu_detect.h"
#include "STTypes.h"

using namespace std;

#include <stdio.h>

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
#if (!(ALLOW_X86_OPTIMIZATIONS) || !(__GNUC__))

    return 0; // always disable extensions on non-x86 platforms.

#else
    uint res = 0;

    if (_dwDisabledISA == 0xffffffff) return 0;

    asm volatile(
        "\n\txor     %%esi, %%esi"       // clear %%esi = result register
        // check if 'cpuid' instructions is available by toggling eflags bit 21

        "\n\tpushf"                      // save eflags to stack
        "\n\tmovl    (%%esp), %%eax"     // load eax from stack (with eflags)
        "\n\tmovl    %%eax, %%ecx"       // save the original eflags values to ecx
        "\n\txor     $0x00200000, %%eax" // toggle bit 21
        "\n\tmovl    %%eax, (%%esp)"     // store toggled eflags to stack
        "\n\tpopf"                       // load eflags from stack
        "\n\tpushf"                      // save updated eflags to stack
        "\n\tmovl    (%%esp), %%eax"     // load eax from stack
        "\n\tpopf"                       // pop stack to restore esp
        "\n\txor     %%edx, %%edx"       // clear edx for defaulting no mmx
        "\n\tcmp     %%ecx, %%eax"       // compare to original eflags values
        "\n\tjz      end"                // jumps to 'end' if cpuid not present
        // cpuid instruction available, test for presence of mmx instructions

        "\n\tmovl    $1, %%eax"
        "\n\tcpuid"
        "\n\ttest    $0x00800000, %%edx"
        "\n\tjz      end"                // branch if MMX not available

        "\n\tor      $0x01, %%esi"       // otherwise add MMX support bit

        "\n\ttest    $0x02000000, %%edx"
        "\n\tjz      test3DNow"          // branch if SSE not available

        "\n\tor      $0x08, %%esi"       // otherwise add SSE support bit

    "\n\ttest3DNow:"
        // test for precense of AMD extensions
        "\n\tmov     $0x80000000, %%eax"
        "\n\tcpuid"
        "\n\tcmp     $0x80000000, %%eax"
        "\n\tjbe     end"                 // branch if no AMD extensions detected

        // test for precense of 3DNow! extension
        "\n\tmov     $0x80000001, %%eax"
        "\n\tcpuid"
        "\n\ttest    $0x80000000, %%edx"
        "\n\tjz      end"                  // branch if 3DNow! not detected

        "\n\tor      $0x02, %%esi"         // otherwise add 3DNow support bit

    "\n\tend:"

        "\n\tmov     %%esi, %0"

      : "=r" (res)
      : /* no inputs */
      : "%edx", "%eax", "%ecx", "%esi" );
      
    return res & ~_dwDisabledISA;
#endif
}
