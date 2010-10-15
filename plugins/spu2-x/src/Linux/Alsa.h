/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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

 // Modified by arcum42@gmail.com

 #ifndef __LINUX_H__
 #define __LINUX_H__

#include <assert.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#define SOUNDSIZE 500000
#define SAMPLE_RATE 48000L

// Pull in from Alsa.cpp
extern int AlsaSetupSound();
extern void AlsaRemoveSound();
extern int AlsaSoundGetBytesBuffered();
extern void AlsaSoundFeedVoiceData(unsigned char* pSound,long lBytes);

extern int SetupSound();
extern void RemoveSound();
extern int SoundGetBytesBuffered();
extern void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // __LINUX_H__
