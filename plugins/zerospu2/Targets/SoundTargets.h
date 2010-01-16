/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog; modified by arcum42
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
 
#ifndef SOUNDTARGETS_H_INCLUDED
#define SOUNDTARGETS_H_INCLUDED

#include "Pcsx2Defs.h"

typedef void (*voidFunction)();
typedef int (*intFunction)();
typedef bool (*boolFunction)();
typedef void (*soundFeedFunction)(unsigned char *, long);

struct SoundCallbacks
{
	intFunction SetupSound;
	voidFunction RemoveSound;
	intFunction SoundGetBytesBuffered;
	soundFeedFunction SoundFeedVoiceData;
};

extern SoundCallbacks *SoundCmds;
extern u32 SOUNDSIZE;
extern u32 MaxBuffer;

// Target List
#define ZEROSPU2_ALSA // Comment if Alsa isn't on the system.
#define ZEROSPU2_OSS // Comment if OSS isn't on the system.

#ifdef ZEROSPU2_OSS
#include "Targets/OSS.h"

extern void InitOSS();
#endif

#ifdef ZEROSPU2_ALSA
#include "Targets/Alsa.h"

extern void InitAlsa();
#endif

extern int SetupSound();
extern void RemoveSound();
extern int SoundGetBytesBuffered();
extern void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // SOUNDTARGETS_H_INCLUDED
