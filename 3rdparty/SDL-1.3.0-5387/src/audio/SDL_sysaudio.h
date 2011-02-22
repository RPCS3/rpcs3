/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is SDL_free software; you can redistribute it and/or
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

#ifndef _SDL_sysaudio_h
#define _SDL_sysaudio_h

#include "SDL_mutex.h"
#include "SDL_thread.h"

/* The SDL audio driver */
typedef struct SDL_AudioDevice SDL_AudioDevice;
#define _THIS	SDL_AudioDevice *_this

typedef struct SDL_AudioDriverImpl
{
    int (*DetectDevices) (int iscapture);
    const char *(*GetDeviceName) (int index, int iscapture);
    int (*OpenDevice) (_THIS, const char *devname, int iscapture);
    void (*ThreadInit) (_THIS); /* Called by audio thread at start */
    void (*WaitDevice) (_THIS);
    void (*PlayDevice) (_THIS);
    Uint8 *(*GetDeviceBuf) (_THIS);
    void (*WaitDone) (_THIS);
    void (*CloseDevice) (_THIS);
    void (*LockDevice) (_THIS);
    void (*UnlockDevice) (_THIS);
    void (*Deinitialize) (void);

    /* Some flags to push duplicate code into the core and reduce #ifdefs. */
    int ProvidesOwnCallbackThread:1;
    int SkipMixerLock:1;
    int HasCaptureSupport:1;
    int OnlyHasDefaultOutputDevice:1;
    int OnlyHasDefaultInputDevice:1;
} SDL_AudioDriverImpl;


typedef struct SDL_AudioDriver
{
    /* * * */
    /* The name of this audio driver */
    const char *name;

    /* * * */
    /* The description of this audio driver */
    const char *desc;

    SDL_AudioDriverImpl impl;
} SDL_AudioDriver;


/* Streamer */
typedef struct
{
    Uint8 *buffer;
    int max_len;                /* the maximum length in bytes */
    int read_pos, write_pos;    /* the position of the write and read heads in bytes */
} SDL_AudioStreamer;


/* Define the SDL audio driver structure */
struct SDL_AudioDevice
{
    /* * * */
    /* Data common to all devices */

    /* The current audio specification (shared with audio thread) */
    SDL_AudioSpec spec;

    /* An audio conversion block for audio format emulation */
    SDL_AudioCVT convert;

    /* The streamer, if sample rate conversion necessitates it */
    int use_streamer;
    SDL_AudioStreamer streamer;

    /* Current state flags */
    int iscapture;
    int enabled;
    int paused;
    int opened;

    /* Fake audio buffer for when the audio hardware is busy */
    Uint8 *fake_stream;

    /* A semaphore for locking the mixing buffers */
    SDL_mutex *mixer_lock;

    /* A thread to feed the audio device */
    SDL_Thread *thread;
    SDL_threadID threadid;

    /* * * */
    /* Data private to this driver */
    struct SDL_PrivateAudioData *hidden;
};
#undef _THIS

typedef struct AudioBootStrap
{
    const char *name;
    const char *desc;
    int (*init) (SDL_AudioDriverImpl * impl);
    int demand_only:1;          /* 1==request explicitly, or it won't be available. */
} AudioBootStrap;

#endif /* _SDL_sysaudio_h */

/* vi: set ts=4 sw=4 expandtab: */
