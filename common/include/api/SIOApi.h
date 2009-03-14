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


#ifndef __SIOAPI_H__
#define __SIOAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

/* SIO plugin API */

// basic funcs

// Single plugin can only be PAD, MTAP, RM, or MC.  Only load one plugin of each type,
// but not both a PAD and MTAP.  Simplifies plugin selection and interface, as well
// as API.
s32  CALLBACK SIOinit(char *configpath);
s32  CALLBACK SIOopen(void *pDisplay);
void CALLBACK SIOclose();
void CALLBACK SIOshutdown();
// Returns 0 if device doesn't exist.  Simplifies things.  Also means you don't
// have to distinguish between MTAP and PAD SIO plugins.
s32   CALLBACK SIOstartPoll(u8 deviceType, u32 port, u32 slot, u8 *returnValue);
// Returns 0 on the last output byte.
s32   CALLBACK SIOpoll(u8 value, u8 *returnValue);

EXPORT_C_(keyEvent*) CALLBACK SIOkeyEvent();

// returns: SIO_TYPE_{PAD,MTAP,RM,MC}
u32  CALLBACK SIOquery();

// extended funcs

void CALLBACK SIOconfigure();
void CALLBACK SIOabout();
s32  CALLBACK SIOtest();

#endif

#endif // __SIOAPI_H__