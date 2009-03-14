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


#ifndef __PADAPI_H__
#define __PADAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

/* PAD plugin API  */
/* So obsolete that everyone uses it. */

// Basic functions.
EXPORT_C_(s32)  CALLBACK PADinitEx(char *configpath);
EXPORT_C_(s32)  CALLBACK PADopen(void *pDsp);
EXPORT_C_(void) CALLBACK PADclose();
EXPORT_C_(void) CALLBACK PADshutdown();
// PADkeyEvent is called every vsync (return NULL if no event)
EXPORT_C_(keyEvent*) CALLBACK PADkeyEvent();
EXPORT_C_(u8)   CALLBACK PADstartPoll(u8 pad);
EXPORT_C_(u8)   CALLBACK PADpoll(u8 value);
// returns: 1 if supported pad1
//			2 if supported pad2
//			3 if both are supported
EXPORT_C_(u8)  CALLBACK PADquery();

// call to give a hint to the PAD plugin to query for the keyboard state. A
// good plugin will query the OS for keyboard state ONLY in this function.
// This function is necessary when multithreading because otherwise
// the PAD plugin can get into deadlocks with the thread that really owns
// the window (and input). Note that PADupdate can be called from a different
// thread than the other functions, so mutex or other multithreading primitives
// have to be added to maintain data integrity.
EXPORT_C_(void) CALLBACK PADupdate(u8 pad);

// Extended functions
EXPORT_C_(s32)  CALLBACK PADfreeze(int mode, freezeData *data);
EXPORT_C_(void) CALLBACK PADgsDriverInfo(GSdriverInfo *info);
EXPORT_C_(void) CALLBACK PADconfigure();
EXPORT_C_(void) CALLBACK PADabout();
EXPORT_C_(s32)  CALLBACK PADtest();

#endif // __PADAPI_H__