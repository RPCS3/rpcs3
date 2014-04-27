/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#ifdef __LINUX__

extern "C"
{
	// aVUzerorec.S
	void* SuperVUGetProgram(u32 startpc, int vuindex);
	void SuperVUCleanupProgram(u32 startpc, int vuindex);
}
#endif

#endif
