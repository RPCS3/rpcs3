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
 
 // PortAudio support implemented by Zedr0n.
#ifndef PORTAUDIO_H_INCLUDED
#define PORTAUDIO_H_INCLUDED

#include "portaudio.h"
#include "SoundTargets.h"

extern int paSetupSound();
extern void paRemoveSound();
extern int paSoundGetBytesBuffered();
extern void paSoundFeedVoiceData(unsigned char* pSound,long lBytes);

// Pull in from Alsa.cpp
static SoundCallbacks PACmds =
{
	(intFunction)paSetupSound,
	(voidFunction)paRemoveSound,
	(intFunction)paSoundGetBytesBuffered,
	(soundFeedFunction)paSoundFeedVoiceData
};

#endif // PORTAUDIO_H_INCLUDED
