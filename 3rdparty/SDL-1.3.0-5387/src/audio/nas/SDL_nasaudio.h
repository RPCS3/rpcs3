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

    This driver was written by:
    Erik Inge Bolsø
    knan@mo.himolde.no
*/
#include "SDL_config.h"

#ifndef _SDL_nasaudio_h
#define _SDL_nasaudio_h

#ifdef __sgi
#include <nas/audiolib.h>
#else
#include <audio/audiolib.h>
#endif
#include <sys/time.h>

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    AuServer *aud;
    AuFlowID flow;
    AuDeviceID dev;

    /* Raw mixing buffer */
    Uint8 *mixbuf;
    int mixlen;

    int written;
    int really;
    int bps;
    struct timeval last_tv;
    int buf_free;
};
#endif /* _SDL_nasaudio_h */

/* vi: set ts=4 sw=4 expandtab: */
