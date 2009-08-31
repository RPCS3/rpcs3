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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

struct KeyModifiers
{
	bool control;
	bool alt;
	bool shift;
	bool capslock;
};
extern struct KeyModifiers keymodifiers;

// Per ChickenLiver, this is being used to pass the GS plugins window handle to the Pad plugins.
// So a rename to pDisplay is in the works, but it will not, in fact, be removed.
extern uptr pDsp;	//Used in  GS, MTGS, Plugins, Misc

extern int GetPS2ElfName( wxString& dest ); // Used in Misc, System, Linux, CDVD

// Not sure what header these should go in. Probably not this one.
void SetCPUState(u32 sseMXCSR, u32 sseVUMXCSR);
extern u32 g_sseVUMXCSR, g_sseMXCSR;

void SaveGSState(const wxString& file);
void LoadGSState(const wxString& file);

