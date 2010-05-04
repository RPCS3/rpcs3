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

#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__

typedef int (*irxHLE)(); // return 1 if handled, otherwise 0
typedef void (*irxDEBUG)();

namespace R3000A
{
	const char* irxImportLibname(u32 entrypc);
	const char* irxImportFuncname(const char libname[8], u16 index);
	irxHLE irxImportHLE(const char libname[8], u16 index);
	irxDEBUG irxImportDebug(const char libname[8], u16 index);
	void __fastcall irxImportLog(const char libname[8], u16 index, const char *funcname);
	int __fastcall irxImportExec(const char libname[8], u16 index);
}

#endif /* __PSXBIOS_H__ */
