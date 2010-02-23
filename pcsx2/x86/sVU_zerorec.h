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

// Super VU recompiler - author: zerofrog(@gmail.com)

#pragma once

#include "sVU_Micro.h"

//Using assembly code from an external file.
#ifdef __LINUX__
extern "C" {
#endif
extern void SuperVUExecuteProgram(u32 startpc, int vuindex);
extern void SuperVUEndProgram();
extern void svudispfntemp();
#ifdef __LINUX__
}
#endif

extern void SuperVUDestroy(int vuindex);
extern void SuperVUReset(int vuindex);

// read = 0, will write to reg
// read = 1, will read from reg
// read = 2, addr of previously written reg (used for status and clip flags)
extern u32 SuperVUGetVIAddr(int reg, int read);

// if p == 0, flush q else flush p; if wait is != 0, waits for p/q
extern void SuperVUFlush(int p, int wait);


class recSuperVU0 : public BaseVUmicroCPU 
{
public:
	recSuperVU0();

	const char* GetShortName() const	{ return "sVU0"; }
	wxString GetLongName() const		{ return L"SuperVU0 Recompiler"; }

	void Allocate();
	void Shutdown() throw();
	void Reset();
	void ExecuteBlock(u32 cycles);
	void Clear(u32 Addr, u32 Size);
};

class recSuperVU1 : public BaseVUmicroCPU 
{
public:
	recSuperVU1();

	const char* GetShortName() const	{ return "sVU1"; }
	wxString GetLongName() const		{ return L"SuperVU1 Recompiler"; }

	void Allocate();
	void Shutdown() throw();
	void Reset();
	void ExecuteBlock(u32 cycles);
	void Clear(u32 Addr, u32 Size);
};
