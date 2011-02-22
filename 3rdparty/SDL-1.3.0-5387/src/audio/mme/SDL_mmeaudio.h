/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#ifndef _SDL_mmeaudio_h
#define _SDL_mmeaudio_h

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this
#define NUM_BUFFERS 2

struct SharedMem
{
    HWAVEOUT sound;
    WAVEHDR wHdr[NUM_BUFFERS];
    PCMWAVEFORMAT wFmt;
};

struct SDL_PrivateAudioData
{
    Uint8 *mixbuf;              /* The raw allocated mixing buffer */
    struct SharedMem *shm;
    int next_buffer;
};

#endif /* _SDL_mmeaudio_h */

/* vi: set ts=4 sw=4 expandtab: */
