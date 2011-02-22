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

/*
 * Driver for native OpenBSD/NetBSD audio(4).
 * vedge@vedge.com.ar.
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/audioio.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_bsdaudio.h"

/* The tag name used by NetBSD/OpenBSD audio */
#ifdef __NetBSD__
#define BSD_AUDIO_DRIVER_NAME         "netbsd"
#define BSD_AUDIO_DRIVER_DESC         "Native NetBSD audio"
#else
#define BSD_AUDIO_DRIVER_NAME         "openbsd"
#define BSD_AUDIO_DRIVER_DESC         "Native OpenBSD audio"
#endif

/* Open the audio device for playback, and don't block if busy */
/* #define USE_BLOCKING_WRITES */

/* Use timer for synchronization */
/* #define USE_TIMER_SYNC */

/* #define DEBUG_AUDIO */
/* #define DEBUG_AUDIO_STREAM */

#ifdef USE_BLOCKING_WRITES
#define OPEN_FLAGS_OUTPUT O_WRONLY
#define OPEN_FLAGS_INPUT O_RDONLY
#else
#define OPEN_FLAGS_OUTPUT (O_WRONLY|O_NONBLOCK)
#define OPEN_FLAGS_INPUT (O_RDONLY|O_NONBLOCK)
#endif

/* !!! FIXME: so much cut and paste with dsp/dma drivers... */
static char **outputDevices = NULL;
static int outputDeviceCount = 0;
static char **inputDevices = NULL;
static int inputDeviceCount = 0;

static inline void
free_device_list(char ***devs, int *count)
{
    SDL_FreeUnixAudioDevices(devs, count);
}

static inline void
build_device_list(int iscapture, char ***devs, int *count)
{
    const int flags = ((iscapture) ? OPEN_FLAGS_INPUT : OPEN_FLAGS_OUTPUT);
    free_device_list(devs, count);
    SDL_EnumUnixAudioDevices(flags, 0, NULL, devs, count);
}

static inline void
build_device_lists(void)
{
    build_device_list(0, &outputDevices, &outputDeviceCount);
    build_device_list(1, &inputDevices, &inputDeviceCount);
}


static inline void
free_device_lists(void)
{
    free_device_list(&outputDevices, &outputDeviceCount);
    free_device_list(&inputDevices, &inputDeviceCount);
}


static void
BSDAUDIO_Deinitialize(void)
{
    free_device_lists();
}


static int
BSDAUDIO_DetectDevices(int iscapture)
{
    if (iscapture) {
        build_device_list(1, &inputDevices, &inputDeviceCount);
        return inputDeviceCount;
    } else {
        build_device_list(0, &outputDevices, &outputDeviceCount);
        return outputDeviceCount;
    }

    return 0;                   /* shouldn't ever hit this. */
}

static const char *
BSDAUDIO_GetDeviceName(int index, int iscapture)
{
    if ((iscapture) && (index < inputDeviceCount)) {
        return inputDevices[index];
    } else if ((!iscapture) && (index < outputDeviceCount)) {
        return outputDevices[index];
    }

    SDL_SetError("No such device");
    return NULL;
}


static void
BSDAUDIO_Status(_THIS)
{
#ifdef DEBUG_AUDIO
    /* *INDENT-OFF* */
    audio_info_t info;

    if (ioctl(this->hidden->audio_fd, AUDIO_GETINFO, &info) < 0) {
        fprintf(stderr, "AUDIO_GETINFO failed.\n");
        return;
    }
    fprintf(stderr, "\n"
            "[play/record info]\n"
            "buffer size	:   %d bytes\n"
            "sample rate	:   %i Hz\n"
            "channels	:   %i\n"
            "precision	:   %i-bit\n"
            "encoding	:   0x%x\n"
            "seek		:   %i\n"
            "sample count	:   %i\n"
            "EOF count	:   %i\n"
            "paused		:   %s\n"
            "error occured	:   %s\n"
            "waiting		:   %s\n"
            "active		:   %s\n"
            "",
            info.play.buffer_size,
            info.play.sample_rate,
            info.play.channels,
            info.play.precision,
            info.play.encoding,
            info.play.seek,
            info.play.samples,
            info.play.eof,
            info.play.pause ? "yes" : "no",
            info.play.error ? "yes" : "no",
            info.play.waiting ? "yes" : "no",
            info.play.active ? "yes" : "no");

    fprintf(stderr, "\n"
            "[audio info]\n"
            "monitor_gain	:   %i\n"
            "hw block size	:   %d bytes\n"
            "hi watermark	:   %i\n"
            "lo watermark	:   %i\n"
            "audio mode	:   %s\n"
            "",
            info.monitor_gain,
            info.blocksize,
            info.hiwat, info.lowat,
            (info.mode == AUMODE_PLAY) ? "PLAY"
            : (info.mode = AUMODE_RECORD) ? "RECORD"
            : (info.mode == AUMODE_PLAY_ALL ? "PLAY_ALL" : "?"));
    /* *INDENT-ON* */
#endif /* DEBUG_AUDIO */
}


/* This function waits until it is possible to write a full sound buffer */
static void
BSDAUDIO_WaitDevice(_THIS)
{
#ifndef USE_BLOCKING_WRITES     /* Not necessary when using blocking writes */
    /* See if we need to use timed audio synchronization */
    if (this->hidden->frame_ticks) {
        /* Use timer for general audio synchronization */
        Sint32 ticks;

        ticks =
            ((Sint32) (this->hidden->next_frame - SDL_GetTicks())) -
            FUDGE_TICKS;
        if (ticks > 0) {
            SDL_Delay(ticks);
        }
    } else {
        /* Use select() for audio synchronization */
        fd_set fdset;
        struct timeval timeout;

        FD_ZERO(&fdset);
        FD_SET(this->hidden->audio_fd, &fdset);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Waiting for audio to get ready\n");
#endif
        if (select(this->hidden->audio_fd + 1, NULL, &fdset, NULL, &timeout)
            <= 0) {
            const char *message =
                "Audio timeout - buggy audio driver? (disabled)";
            /* In general we should never print to the screen,
               but in this case we have no other way of letting
               the user know what happened.
             */
            fprintf(stderr, "SDL: %s\n", message);
            this->enabled = 0;
            /* Don't try to close - may hang */
            this->hidden->audio_fd = -1;
#ifdef DEBUG_AUDIO
            fprintf(stderr, "Done disabling audio\n");
#endif
        }
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Ready!\n");
#endif
    }
#endif /* !USE_BLOCKING_WRITES */
}

static void
BSDAUDIO_PlayDevice(_THIS)
{
    int written, p = 0;

    /* Write the audio data, checking for EAGAIN on broken audio drivers */
    do {
        written = write(this->hidden->audio_fd,
                        &this->hidden->mixbuf[p], this->hidden->mixlen - p);

        if (written > 0)
            p += written;
        if (written == -1 && errno != 0 && errno != EAGAIN && errno != EINTR) {
            /* Non recoverable error has occurred. It should be reported!!! */
            perror("audio");
            break;
        }

        if (p < written
            || ((written < 0) && ((errno == 0) || (errno == EAGAIN)))) {
            SDL_Delay(1);       /* Let a little CPU time go by */
        }
    } while (p < written);

    /* If timer synchronization is enabled, set the next write frame */
    if (this->hidden->frame_ticks) {
        this->hidden->next_frame += this->hidden->frame_ticks;
    }

    /* If we couldn't write, assume fatal error for now */
    if (written < 0) {
        this->enabled = 0;
    }
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Wrote %d bytes of audio data\n", written);
#endif
}

static Uint8 *
BSDAUDIO_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static void
BSDAUDIO_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->audio_fd >= 0) {
            close(this->hidden->audio_fd);
            this->hidden->audio_fd = -1;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
BSDAUDIO_OpenDevice(_THIS, const char *devname, int iscapture)
{
    const int flags = ((iscapture) ? OPEN_FLAGS_INPUT : OPEN_FLAGS_OUTPUT);
    SDL_AudioFormat format = 0;
    audio_info_t info;

    /* We don't care what the devname is...we'll try to open anything. */
    /*  ...but default to first name in the list... */
    if (devname == NULL) {
        if (((iscapture) && (inputDeviceCount == 0)) ||
            ((!iscapture) && (outputDeviceCount == 0))) {
            SDL_SetError("No such audio device");
            return 0;
        }
        devname = ((iscapture) ? inputDevices[0] : outputDevices[0]);
    }

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Open the audio device */
    this->hidden->audio_fd = open(devname, flags, 0);
    if (this->hidden->audio_fd < 0) {
        SDL_SetError("Couldn't open %s: %s", devname, strerror(errno));
        return 0;
    }

    AUDIO_INITINFO(&info);

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Set to play mode */
    info.mode = AUMODE_PLAY;
    if (ioctl(this->hidden->audio_fd, AUDIO_SETINFO, &info) < 0) {
        BSDAUDIO_CloseDevice(this);
        SDL_SetError("Couldn't put device into play mode");
        return 0;
    }

    AUDIO_INITINFO(&info);
    for (format = SDL_FirstAudioFormat(this->spec.format);
         format; format = SDL_NextAudioFormat()) {
        switch (format) {
        case AUDIO_U8:
            info.play.encoding = AUDIO_ENCODING_ULINEAR;
            info.play.precision = 8;
            break;
        case AUDIO_S8:
            info.play.encoding = AUDIO_ENCODING_SLINEAR;
            info.play.precision = 8;
            break;
        case AUDIO_S16LSB:
            info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
            info.play.precision = 16;
            break;
        case AUDIO_S16MSB:
            info.play.encoding = AUDIO_ENCODING_SLINEAR_BE;
            info.play.precision = 16;
            break;
        case AUDIO_U16LSB:
            info.play.encoding = AUDIO_ENCODING_ULINEAR_LE;
            info.play.precision = 16;
            break;
        case AUDIO_U16MSB:
            info.play.encoding = AUDIO_ENCODING_ULINEAR_BE;
            info.play.precision = 16;
            break;
        default:
            continue;
        }

        if (ioctl(this->hidden->audio_fd, AUDIO_SETINFO, &info) == 0) {
            break;
        }
    }

    if (!format) {
        BSDAUDIO_CloseDevice(this);
        SDL_SetError("No supported encoding for 0x%x", this->spec.format);
        return 0;
    }

    this->spec.format = format;

    AUDIO_INITINFO(&info);
    info.play.channels = this->spec.channels;
    if (ioctl(this->hidden->audio_fd, AUDIO_SETINFO, &info) == -1) {
        this->spec.channels = 1;
    }
    AUDIO_INITINFO(&info);
    info.play.sample_rate = this->spec.freq;
    info.blocksize = this->spec.size;
    info.hiwat = 5;
    info.lowat = 3;
    (void) ioctl(this->hidden->audio_fd, AUDIO_SETINFO, &info);
    (void) ioctl(this->hidden->audio_fd, AUDIO_GETINFO, &info);
    this->spec.freq = info.play.sample_rate;
    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        BSDAUDIO_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    BSDAUDIO_Status(this);

    /* We're ready to rock and roll. :-) */
    return (0);
}

static int
BSDAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->DetectDevices = BSDAUDIO_DetectDevices;
    impl->GetDeviceName = BSDAUDIO_GetDeviceName;
    impl->OpenDevice = BSDAUDIO_OpenDevice;
    impl->PlayDevice = BSDAUDIO_PlayDevice;
    impl->WaitDevice = BSDAUDIO_WaitDevice;
    impl->GetDeviceBuf = BSDAUDIO_GetDeviceBuf;
    impl->CloseDevice = BSDAUDIO_CloseDevice;
    impl->Deinitialize = BSDAUDIO_Deinitialize;

    build_device_lists();

    return 1;   /* this audio target is available. */
}


AudioBootStrap BSD_AUDIO_bootstrap = {
    BSD_AUDIO_DRIVER_NAME, BSD_AUDIO_DRIVER_DESC, BSDAUDIO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
