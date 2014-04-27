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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "SoundTargets.h"

SoundCallbacks *SoundCmds;
u32 SOUNDSIZE;
s32 MaxBuffer;

#ifdef __LINUX__
#include "Linux.h"

#ifdef ZEROSPU2_ALSA
void InitAlsa()
{
	SoundCmds = &AlsaCmds;
	SOUNDSIZE = 500000;
	//MaxBuffer = 80000;
}
#endif

#ifdef ZEROSPU2_OSS

void InitOSS()
{
	SoundCmds = &OSSCmds;
	SOUNDSIZE = 76800;
	MaxBuffer = 80000;
}
#endif

#else
#include "dsound51.h"

void InitDSound()
{
	SoundCmds = &DSCmds;
	SOUNDSIZE = 76800;
	MaxBuffer = 80000;
}
#endif

#ifdef ZEROSPU2_PORTAUDIO
#include "PortAudio.h"

void InitPortAudio()
{
	SoundCmds = &PACmds;
	SOUNDSIZE = 4096;
}
#endif

int SetupSound()
{
	return SoundCmds->SetupSound();
}
void RemoveSound()
{
	SoundCmds->RemoveSound();
}
int SoundGetBytesBuffered()
{
	return SoundCmds->SoundGetBytesBuffered();
}
void SoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
	SoundCmds->SoundFeedVoiceData(pSound, lBytes);
}
