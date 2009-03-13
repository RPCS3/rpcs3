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

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////
// Startup Parameters.

enum StartupMode
{
	BootMode_Bios,
	BootMode_Quick,
	BootMode_Elf
};

class StartupParams
{
public:
	// Name of the CDVD or ELF image to load.
	// if NULL, the CDVD configured settings are used.
	const char* ImageName;

	bool NoGui;
	bool Enabled;
	StartupMode BootMode;

	// Plugin overrides
	const char* gsdll, *cdvddll, *spudll;
	const char* pad1dll, *pad2dll, *dev9dll;

	StartupParams() { memzero_obj(*this); }
};

extern StartupParams g_Startup;

//////////////////////////////////////////////////////////////////////////////////////////
// Core Gui APIs  (shared by all platforms)
//
// Most of these are implemented in SystemGui.cpp

extern void States_Load( const string& file );
extern void States_Save( const string& file );
extern void States_Load( int num );
extern void States_Save( int num );
extern bool States_isSlotUsed(int num);

extern bool g_EmulationInProgress;	// Set TRUE if a game is actively running (set to false on reset)

//////////////////////////////////////////////////////////////////////////////////////////
// External Gui APIs (platform specific)
//
// The following section contains API declarations for GUI callback functions that the
// Pcsx2 core expects and needs to be implemented by Pcsx2 platform dependent code
// (Win32/Linux).  If anything in this header comes up as a missing external during link,
// it means that the necessary platform dependent files are not being compiled, or the
// platform code is incomplete.
//
// These APIs have been namespaced to help simplify and organize the process of implementing
// them and resolving linker errors.
//

// Namespace housing gui-level implementations relating to events and signals such
// as keyboard events, menu/status updates, and cpu execution invocation.
namespace HostGui
{
	// Signal for informing the GUI that the saveslot status has been altered.
	// The guis hould re-enumerate the slot information displayed in the menu, or wherever.
	extern void ResetMenuSlots();
	
	// Signals to the GUI that execution of the emulator should begin.  This can be either
	// a blocking or non-blocking (threaded) action.
	extern void BeginExecution();
	
	// Signals the gui with a keystroke.  Handle or discard or dispatch, or enjoy its
	// pleasant flavor.
	extern void __fastcall KeyEvent( keyEvent* ev );

	// For issuing notices to both the status bar and the console at the same time.
	// Single-line text only please!  Multi-line msgs should be directed to the
	// console directly, thanks.
	extern void Notice( const std::string& text );

	// sets the contents of the pcsx2 window status bar.
	extern void SetStatusMsg( const std::string& text );
};