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

#pragma once
#include "CDVD/CDVD.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Startup Parameters.

enum StartupModeType
{
	Startup_FromCDVD   = 0,
	Startup_FromELF    = 1, // not compatible with bios flag, probably
};

enum CDVD_SourceType;

class StartupParams
{
public:
	// Name of the CDVD image to load.
	// if NULL, the CDVD plugin configured settings are used.
	const char* ImageName;
	
	// Name of the ELF file to load.  If null, the CDVD is booted instead.
	const char* ElfFile;

	bool	NoGui;
	bool	Enabled;
	StartupModeType StartupMode;
	CDVD_SourceType CdvdSource;

	// Ignored when booting ELFs.
	bool SkipBios;

	// Plugin overrides
	const char* gsdll, *cdvddll, *spudll;
	const char* pad1dll, *pad2dll, *dev9dll;

	StartupParams() { memzero(*this); }
};

extern StartupParams g_Startup;

extern void States_Save( const wxString& file );
extern bool States_isSlotUsed(int num);

extern void States_FreezeCurrentSlot();
extern void States_DefrostCurrentSlot();
extern void States_FreezeCurrentSlot();
extern void States_CycleSlotForward();
extern void States_CycleSlotBackward();

extern void States_SetCurrentSlot( int slot );
extern int  States_GetCurrentSlot( int slot );
