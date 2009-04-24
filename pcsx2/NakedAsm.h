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
 
 //  Externs for various routines that are defined in assembly files on Linux.
#ifndef NAKED_ASM_H
#define NAKED_ASM_H

#include "coroutine.h"

// Common to Windows and Linux
extern "C" 
{
// acoroutine.S
void so_call(coroutine_t coro);
void so_resume(void);
void so_exit(void);

// I can't find where the Linux recRecompile is defined.  Is it used anymore?
// If so, namespacing might break it. :/  (air)
void recRecompile( u32 startpc );

// aR3000A.S
void iopRecRecompile(u32 startpc);
}

// Linux specific
#ifdef __LINUX__

PCSX2_ALIGNED16( u8 _xmm_backup[16*2] );
PCSX2_ALIGNED16( u8 _mmx_backup[8*4] );

extern "C" 
{
// aVUzerorec.S
void* SuperVUGetProgram(u32 startpc, int vuindex);
void SuperVUCleanupProgram(u32 startpc, int vuindex);
void svudispfn();
	
// aR3000A.S
void iopJITCompile();
void iopJITCompileInBlock();
void iopDispatcherReg();
	
// aR5900-32.S
void JITCompile();
void JITCompileInBlock();
void DispatcherReg();

}
#endif
#endif

