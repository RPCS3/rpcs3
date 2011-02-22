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

#ifndef _SDL_bsdaudio_h
#define _SDL_bsdaudio_h

#include "../SDL_sysaudio.h"

#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    /* The file descriptor for the audio device */
    int audio_fd;

    /* The parent process id, to detect when application quits */
    pid_t parent;

    /* Raw mixing buffer */
    Uint8 *mixbuf;
    int mixlen;

    /* Support for audio timing using a timer, in addition to select() */
    float frame_ticks;
    float next_frame;
};

#define FUDGE_TICKS	10      /* The scheduler overhead ticks per frame */

#endif /* _SDL_bsdaudio_h */

/* vi: set ts=4 sw=4 expandtab: */
