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

/* Allow access to an ESD network stream mixing buffer */

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <esd.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_esdaudio.h"

#ifdef SDL_AUDIO_DRIVER_ESD_DYNAMIC
#include "SDL_name.h"
#include "SDL_loadso.h"
#else
#define SDL_NAME(X)	X
#endif

/* The tag name used by ESD audio */
#define ESD_DRIVER_NAME		"esd"

#ifdef SDL_AUDIO_DRIVER_ESD_DYNAMIC

static const char *esd_library = SDL_AUDIO_DRIVER_ESD_DYNAMIC;
static void *esd_handle = NULL;

static int (*SDL_NAME(esd_open_sound)) (const char *host);
static int (*SDL_NAME(esd_close)) (int esd);
static int (*SDL_NAME(esd_play_stream)) (esd_format_t format, int rate,
                                         const char *host, const char *name);

#define SDL_ESD_SYM(x) { #x, (void **) (char *) &SDL_NAME(x) }
static struct
{
    const char *name;
    void **func;
} const esd_functions[] = {
    SDL_ESD_SYM(esd_open_sound),
    SDL_ESD_SYM(esd_close), SDL_ESD_SYM(esd_play_stream),
};

#undef SDL_ESD_SYM

static void
UnloadESDLibrary()
{
    if (esd_handle != NULL) {
        SDL_UnloadObject(esd_handle);
        esd_handle = NULL;
    }
}

static int
LoadESDLibrary(void)
{
    int i, retval = -1;

    if (esd_handle == NULL) {
        esd_handle = SDL_LoadObject(esd_library);
        if (esd_handle) {
            retval = 0;
            for (i = 0; i < SDL_arraysize(esd_functions); ++i) {
                *esd_functions[i].func =
                    SDL_LoadFunction(esd_handle, esd_functions[i].name);
                if (!*esd_functions[i].func) {
                    retval = -1;
                    UnloadESDLibrary();
                    break;
                }
            }
        }
    }
    return retval;
}

#else

static void
UnloadESDLibrary()
{
    return;
}

static int
LoadESDLibrary(void)
{
    return 0;
}

#endif /* SDL_AUDIO_DRIVER_ESD_DYNAMIC */


/* This function waits until it is possible to write a full sound buffer */
static void
ESD_WaitDevice(_THIS)
{
    Sint32 ticks;

    /* Check to see if the thread-parent process is still alive */
    {
        static int cnt = 0;
        /* Note that this only works with thread implementations 
           that use a different process id for each thread.
         */
        /* Check every 10 loops */
        if (this->hidden->parent && (((++cnt) % 10) == 0)) {
            if (kill(this->hidden->parent, 0) < 0 && errno == ESRCH) {
                this->enabled = 0;
            }
        }
    }

    /* Use timer for general audio synchronization */
    ticks =
        ((Sint32) (this->hidden->next_frame - SDL_GetTicks())) - FUDGE_TICKS;
    if (ticks > 0) {
        SDL_Delay(ticks);
    }
}

static void
ESD_PlayDevice(_THIS)
{
    int written = 0;

    /* Write the audio data, checking for EAGAIN on broken audio drivers */
    do {
        written = write(this->hidden->audio_fd,
                        this->hidden->mixbuf, this->hidden->mixlen);
        if ((written < 0) && ((errno == 0) || (errno == EAGAIN))) {
            SDL_Delay(1);       /* Let a little CPU time go by */
        }
    } while ((written < 0) &&
             ((errno == 0) || (errno == EAGAIN) || (errno == EINTR)));

    /* Set the next write frame */
    this->hidden->next_frame += this->hidden->frame_ticks;

    /* If we couldn't write, assume fatal error for now */
    if (written < 0) {
        this->enabled = 0;
    }
}

static Uint8 *
ESD_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static void
ESD_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->audio_fd >= 0) {
            SDL_NAME(esd_close) (this->hidden->audio_fd);
            this->hidden->audio_fd = -1;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

/* Try to get the name of the program */
static char *
get_progname(void)
{
    char *progname = NULL;
#ifdef __LINUX__
    FILE *fp;
    static char temp[BUFSIZ];

    SDL_snprintf(temp, SDL_arraysize(temp), "/proc/%d/cmdline", getpid());
    fp = fopen(temp, "r");
    if (fp != NULL) {
        if (fgets(temp, sizeof(temp) - 1, fp)) {
            progname = SDL_strrchr(temp, '/');
            if (progname == NULL) {
                progname = temp;
            } else {
                progname = progname + 1;
            }
        }
        fclose(fp);
    }
#endif
    return (progname);
}


static int
ESD_OpenDevice(_THIS, const char *devname, int iscapture)
{
    esd_format_t format = (ESD_STREAM | ESD_PLAY);
    SDL_AudioFormat test_format = 0;
    int found = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));
    this->hidden->audio_fd = -1;

    /* Convert audio spec to the ESD audio format */
    /* Try for a closest match on audio format */
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         !found && test_format; test_format = SDL_NextAudioFormat()) {
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
        found = 1;
        switch (test_format) {
        case AUDIO_U8:
            format |= ESD_BITS8;
            break;
        case AUDIO_S16SYS:
            format |= ESD_BITS16;
            break;
        default:
            found = 0;
            break;
        }
    }

    if (!found) {
        ESD_CloseDevice(this);
        SDL_SetError("Couldn't find any hardware audio formats");
        return 0;
    }

    if (this->spec.channels == 1) {
        format |= ESD_MONO;
    } else {
        format |= ESD_STEREO;
    }
#if 0
    this->spec.samples = ESD_BUF_SIZE;  /* Darn, no way to change this yet */
#endif

    /* Open a connection to the ESD audio server */
    this->hidden->audio_fd =
        SDL_NAME(esd_play_stream) (format, this->spec.freq, NULL,
                                   get_progname());

    if (this->hidden->audio_fd < 0) {
        ESD_CloseDevice(this);
        SDL_SetError("Couldn't open ESD connection");
        return 0;
    }

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);
    this->hidden->frame_ticks =
        (float) (this->spec.samples * 1000) / this->spec.freq;
    this->hidden->next_frame = SDL_GetTicks() + this->hidden->frame_ticks;

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        ESD_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    /* Get the parent process id (we're the parent of the audio thread) */
    this->hidden->parent = getpid();

    /* We're ready to rock and roll. :-) */
    return 1;
}

static void
ESD_Deinitialize(void)
{
    UnloadESDLibrary();
}

static int
ESD_Init(SDL_AudioDriverImpl * impl)
{
    if (LoadESDLibrary() < 0) {
        return 0;
    } else {
        int connection = 0;

        /* Don't start ESD if it's not running */
        SDL_setenv("ESD_NO_SPAWN", "1", 0);

        connection = SDL_NAME(esd_open_sound) (NULL);
        if (connection < 0) {
            UnloadESDLibrary();
            SDL_SetError("ESD: esd_open_sound failed (no audio server?)");
            return 0;
        }
        SDL_NAME(esd_close) (connection);
    }

    /* Set the function pointers */
    impl->OpenDevice = ESD_OpenDevice;
    impl->PlayDevice = ESD_PlayDevice;
    impl->WaitDevice = ESD_WaitDevice;
    impl->GetDeviceBuf = ESD_GetDeviceBuf;
    impl->CloseDevice = ESD_CloseDevice;
    impl->Deinitialize = ESD_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;   /* this audio target is available. */
}


AudioBootStrap ESD_bootstrap = {
    ESD_DRIVER_NAME, "Enlightened Sound Daemon", ESD_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
