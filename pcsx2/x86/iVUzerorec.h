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

// Super VU recompiler - author: zerofrog(@gmail.com)

#ifndef VU1_SUPER_RECOMPILER
#define VU1_SUPER_RECOMPILER

#ifdef __cplusplus
extern "C" {
#endif

void SuperVUInit(int vuindex); // if vuindex is -1, inits the global VU resources
void SuperVUDestroy(int vuindex); // if vuindex is -1, destroys everything
void SuperVUReset(int vuindex); // if vuindex is -1, resets everything

void SuperVUExecuteProgram(u32 startpc, int vuindex);
void SuperVUClear(u32 startpc, u32 size, int vuindex);

u64 SuperVUGetRecTimes(int clear);

// read = 0, will write to reg
// read = 1, will read from reg
// read = 2, addr of previously written reg (used for status and clip flags)
u32 SuperVUGetVIAddr(int reg, int read);

// if p == 0, flush q else flush p; if wait is != 0, waits for p/q
void SuperVUFlush(int p, int wait);

#ifdef __cplusplus
}
#endif

#endif
