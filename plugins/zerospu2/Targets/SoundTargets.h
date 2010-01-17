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
extern s32 MaxBuffer;

// Target List
#ifdef __LINUX__
#define ZEROSPU2_ALSA // Comment if Alsa isn't on the system.
#define ZEROSPU2_OSS // Comment if OSS isn't on the system.
#else
#define ZEROSPU2_DS
#endif
//#define ZEROSPU2_PORTAUDIO

#ifdef ZEROSPU2_OSS
#include "OSS.h"

extern void InitOSS();
#endif

#ifdef ZEROSPU2_ALSA
#include "Alsa.h"

extern void InitAlsa();
#endif

#ifdef ZEROSPU2_DS
#include "dsound51.h"

extern void InitDSound();
#endif

#ifdef ZEROSPU2_PORTAUDIO
#include "PortAudio.h"

extern void InitPortAudio();
#endif

extern int SetupSound();
extern void RemoveSound();
extern int SoundGetBytesBuffered();
extern void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // SOUNDTARGETS_H_INCLUDED
