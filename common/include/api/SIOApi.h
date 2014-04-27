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

// Called by SIO_TYPE_MTAP plugins to change the slot.
// Slot is then passed to memcard and pad plugins on SIOstartPoll.
// MTAP SIO plugins should ignore slot values on startPoll, as should RMs (probably).

// Port isn't strictly necessary, but doesn't hurt.
typedef int (CALLBACK * SIOchangeSlotCB)(int port, int slot);

// Basic functions.

// Types is an or-ed combination of SioTypes to initialize.  It'd probably be simplest
// just to make each SIO plugin support only one type, for simplicity.
// SIOchangeSlotCB should *only* be called by MTAP plugins.
EXPORT_C_(s32) SIOinit(int types, SIOchangeSlotCB f);

// Single plugin can only be PAD, MTAP, RM, or MC.  Only load one plugin of each type,
// but not both a PAD and MTAP.  Simplifies plugin selection and interface, as well
// as API.
// pDisplay normally is passed a handle to the GS plugins window.
EXPORT_C_(s32) SIOopen(void *pDisplay);
EXPORT_C_(void) SIOclose();
EXPORT_C_(void) SIOshutdown();

// Returns 0 if device doesn't exist.  Means old pad plugins can just say nothing
// connected to other slots, and SIOpoll won't be called on those slots, ideally.
EXPORT_C_(s32) SIOstartPoll(u8 deviceType, u32 port, u32 slot, u8 *returnValue);

// Returns 0 on the last output byte.
EXPORT_C_(s32) SIOpoll(u8 value, u8 *returnValue);

// returns: SIO_TYPE_{PAD,MTAP,RM,MC}
EXPORT_C_(u32) SIOquery();

// extended funcs

EXPORT_C_(void) SIOconfigure();
EXPORT_C_(keyEvent*) CALLBACK SIOkeyEvent();

// Save one type at a time.  If a plugin supports all 4 types,
// should expect 4 calls.  Increases savestate compatibility.
EXPORT_C_(s32) SIOfreeze(u8 mode, freezeData *data, int type);
EXPORT_C_(void) SIOabout();
EXPORT_C_(s32) SIOtest();

enum {
SIO_TYPE_PAD = 0x00000001,
SIO_TYPE_MTAP = 0x00000004,
SIO_TYPE_RM = 0x00000040,
SIO_TYPE_MC = 0x00000100
} SioTypes;

#endif // __SIOAPI_H__