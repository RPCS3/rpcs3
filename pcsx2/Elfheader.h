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

#ifndef __ELF_H__
#define __ELF_H__

//2002-09-20 (Florin)
extern char args[256];		//to be filled by GUI
extern unsigned int args_ptr;

//-------------------
extern void loadElfFile(const wxString& filename);
extern u32  loadElfCRC(const char *filename);
extern void ElfApplyPatches();

extern u32 ElfCRC;

#endif
