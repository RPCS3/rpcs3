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

#pragma once

// NOTICE!  This file is intended as a temporary placebo only, until such time that the
// memorycard system is properly extracted into a plugin system (which would make it a
// separate project file).
//
// Please do not move contents from MemoryCardfile.cpp, such as class definitions, into 
// this file.  I'd prefer they stay in MemoryCardFile.cpp for now. --air

extern uint FileMcd_GetMtapPort(uint slot);
extern uint FileMcd_GetMtapSlot(uint slot);
extern bool FileMcd_IsMultitapSlot( uint slot );
extern wxFileName FileMcd_GetSimpleName(uint slot);
extern wxString FileMcd_GetDefaultName(uint slot);
