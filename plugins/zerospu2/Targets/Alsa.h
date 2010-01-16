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
 
#ifndef ALSA_H_INCLUDED
#define ALSA_H_INCLUDED

#include "SoundTargets.h"

#include <alsa/asoundlib.h>
#define ALSA_MEM_DEF

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#ifdef ALSA_MEM_DEF
#define ALSA_MEM_EXTERN
#else
#define ALSA_MEM_EXTERN extern
#endif

extern int AlsaSetupSound();
extern void AlsaRemoveSound();
extern int AlsaSoundGetBytesBuffered();
extern void AlsaSoundFeedVoiceData(unsigned char* pSound,long lBytes);

// Pull in from Alsa.cpp
static SoundCallbacks AlsaCmds =
{
	(intFunction)AlsaSetupSound,
	(voidFunction)AlsaRemoveSound,
	(intFunction)AlsaSoundGetBytesBuffered,
	(soundFeedFunction)AlsaSoundFeedVoiceData
};

#endif // ALSA_H_INCLUDED
