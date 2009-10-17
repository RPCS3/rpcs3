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

 //  Externs for various routines that are defined in assembly files on Linux.
#ifndef NAKED_ASM_H
#define NAKED_ASM_H

#include "IPU/coroutine.h"

// Common to Windows and Linux
extern "C"
{
	// acoroutine.S
	void so_call(coroutine_t coro);
	void so_resume(void);
	void so_exit(void);

	void recRecompile( u32 startpc );

	// aR3000A.S
	void iopRecRecompile(u32 startpc);
}

#ifdef __LINUX__

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

}
#endif

#endif
