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

#include "Utilities/RedtapeWindows.h"		// our "safe" include of windows (sets version and undefs uselessness)

#include "System.h"
#include "resource.h"

#define COMPILEDATE         __DATE__


extern void StreamException_ThrowLastError( const wxString& streamname, HANDLE result=INVALID_HANDLE_VALUE );
extern void StreamException_ThrowFromErrno( const wxString& streamname, errno_t errcode );
extern bool StreamException_LogFromErrno( const wxString& streamname, const wxChar* action, errno_t result );
extern bool StreamException_LogLastError( const wxString& streamname, const wxChar* action, HANDLE result=INVALID_HANDLE_VALUE );

