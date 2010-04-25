/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DSOUND51_H_INCLUDED
#define DSOUND51_H_INCLUDED

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "SoundTargets.h"
#include "../zerospu2.h"

/////////////////////////////////////
// use DirectSound for the sound
#include <dsound.h>

extern int DSSetupSound();
extern void DSRemoveSound();
extern int DSSoundGetBytesBuffered();
extern void DSSoundFeedVoiceData(unsigned char* pSound,long lBytes);

static SoundCallbacks DSCmds =
{
	(intFunction)DSSetupSound,
	(voidFunction)DSRemoveSound,
	(intFunction)DSSoundGetBytesBuffered,
	(soundFeedFunction)DSSoundFeedVoiceData
};

#endif
