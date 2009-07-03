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
// Windows Redtape!  No windows.h should be included without it!
//
// This header's purpose is to include windows.h with the correct OS version info, and
// to undefine some of windows.h's more evil macros (min/max).  It also does a _WIN32
// check, so that we don't have to do it explicitly in every instance where it might
// be needed from non-Win32-specific files

#define NOMINMAX		// Disables other libs inclusion of their own min/max macros (we use std instead)

#ifdef _WIN32

// Force availability of to WinNT APIs (change to 0x600 to enable XP-specific APIs)
#ifndef WINVER 
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>

// disable Windows versions of min/max -- we'll use the typesafe STL versions instead.
#undef min
#undef max

#endif
