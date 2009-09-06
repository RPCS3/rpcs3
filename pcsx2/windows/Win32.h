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

#include "Utilities/RedtapeWindows.h"		// our "safe" include of windows (sets version and undefs uselessness)

#include "System.h"
#include "HostGui.h"
#include "resource.h"

#define COMPILEDATE         __DATE__

//Exception handler for the VTLB-based recompilers.
int SysPageFaultExceptionFilter(EXCEPTION_POINTERS* eps);

#define PCSX2_MEM_PROTECT_BEGIN() __try {
#define PCSX2_MEM_PROTECT_END() } __except(SysPageFaultExceptionFilter(GetExceptionInformation())) {}

// --->>  Patch Browser Stuff (in the event we ever use it)

void ListPatches (HWND hW);
int ReadPatch (HWND hW, char fileName[1024]);
char * lTrim (char *s);
BOOL Save_Patch_Proc( char * filename );

// <<--- END Patch Browser

extern SafeArray<u8>* g_RecoveryState;
extern SafeArray<u8>* g_gsRecoveryState;
extern const char* g_pRunGSState;
extern int g_SaveGSStream;

extern void StreamException_ThrowLastError( const wxString& streamname, HANDLE result=INVALID_HANDLE_VALUE );
extern void StreamException_ThrowFromErrno( const wxString& streamname, errno_t errcode );
extern bool StreamException_LogFromErrno( const wxString& streamname, const wxChar* action, errno_t result );
extern bool StreamException_LogLastError( const wxString& streamname, const wxChar* action, HANDLE result=INVALID_HANDLE_VALUE );

