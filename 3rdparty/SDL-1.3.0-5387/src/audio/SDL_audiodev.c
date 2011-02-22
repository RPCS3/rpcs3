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

/* Get the name of the audio device we use for output */

#if SDL_AUDIO_DRIVER_BSD || SDL_AUDIO_DRIVER_OSS || SDL_AUDIO_DRIVER_SUNAUDIO

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> /* For close() */

#include "SDL_stdinc.h"
#include "SDL_audiodev_c.h"

#ifndef _PATH_DEV_DSP
#if defined(__NETBSD__) || defined(__OPENBSD__)
#define _PATH_DEV_DSP  "/dev/audio"
#else
#define _PATH_DEV_DSP  "/dev/dsp"
#endif
#endif
#ifndef _PATH_DEV_DSP24
#define _PATH_DEV_DSP24	"/dev/sound/dsp"
#endif
#ifndef _PATH_DEV_AUDIO
#define _PATH_DEV_AUDIO	"/dev/audio"
#endif

static inline void
test_device(const char *fname, int flags, int (*test) (int fd),
            char ***devices, int *devCount)
{
    struct stat sb;
    if ((stat(fname, &sb) == 0) && (S_ISCHR(sb.st_mode))) {
        int audio_fd = open(fname, flags, 0);
        if ((audio_fd >= 0) && (test(audio_fd))) {
            void *p =
                SDL_realloc(*devices, ((*devCount) + 1) * sizeof(char *));
            if (p != NULL) {
                size_t len = strlen(fname) + 1;
                char *str = (char *) SDL_malloc(len);
                *devices = (char **) p;
                if (str != NULL) {
                    SDL_strlcpy(str, fname, len);
                    (*devices)[(*devCount)++] = str;
                }
            }
            close(audio_fd);
        }
    }
}

void
SDL_FreeUnixAudioDevices(char ***devices, int *devCount)
{
    int i = *devCount;
    if ((i > 0) && (*devices != NULL)) {
        while (i--) {
            SDL_free((*devices)[i]);
        }
    }

    if (*devices != NULL) {
        SDL_free(*devices);
    }

    *devices = NULL;
    *devCount = 0;
}

static int
test_stub(int fd)
{
    return 1;
}

void
SDL_EnumUnixAudioDevices(int flags, int classic, int (*test) (int fd),
                         char ***devices, int *devCount)
{
    const char *audiodev;
    char audiopath[1024];

    if (test == NULL)
        test = test_stub;

    /* Figure out what our audio device is */
    if (((audiodev = SDL_getenv("SDL_PATH_DSP")) == NULL) &&
        ((audiodev = SDL_getenv("AUDIODEV")) == NULL)) {
        if (classic) {
            audiodev = _PATH_DEV_AUDIO;
        } else {
            struct stat sb;

            /* Added support for /dev/sound/\* in Linux 2.4 */
            if (((stat("/dev/sound", &sb) == 0) && S_ISDIR(sb.st_mode))
                && ((stat(_PATH_DEV_DSP24, &sb) == 0)
                    && S_ISCHR(sb.st_mode))) {
                audiodev = _PATH_DEV_DSP24;
            } else {
                audiodev = _PATH_DEV_DSP;
            }
        }
    }
    test_device(audiodev, flags, test, devices, devCount);

    if (SDL_strlen(audiodev) < (sizeof(audiopath) - 3)) {
        int instance = 0;
        while (instance++ <= 64) {
            SDL_snprintf(audiopath, SDL_arraysize(audiopath),
                         "%s%d", audiodev, instance);
            test_device(audiopath, flags, test, devices, devCount);
        }
    }
}

#endif /* Audio driver selection */
/* vi: set ts=4 sw=4 expandtab: */
