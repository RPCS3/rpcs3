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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <unistd.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_fsaudio.h"

#include <fusionsound/fusionsound_version.h>

//#define SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC "libfusionsound.so"

#ifdef SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC
#include "SDL_name.h"
#include "SDL_loadso.h"
#else
#define SDL_NAME(X)	X
#endif

#if (FUSIONSOUND_MAJOR_VERSION == 1) && (FUSIONSOUND_MINOR_VERSION < 1)
typedef DFBResult DirectResult;
#endif

/* The tag name used by fusionsoundc audio */
#define SDL_FS_DRIVER_NAME         "fusionsound"
/* Buffers to use - more than 2 gives a lot of latency */
#define FUSION_BUFFERS				(2)

#ifdef SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC

static const char *fs_library = SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC;
static void *fs_handle = NULL;

static DirectResult (*SDL_NAME(FusionSoundInit)) (int *argc, char *(*argv[]));
static DirectResult (*SDL_NAME(FusionSoundCreate)) (IFusionSound **
                                                   ret_interface);

#define SDL_FS_SYM(x) { #x, (void **) (char *) &SDL_NAME(x) }
static struct
{
    const char *name;
    void **func;
} fs_functions[] = {
/* *INDENT-OFF* */
    SDL_FS_SYM(FusionSoundInit),
    SDL_FS_SYM(FusionSoundCreate),
/* *INDENT-ON* */
};

#undef SDL_FS_SYM

static void
UnloadFusionSoundLibrary()
{
    if (fs_handle != NULL) {
        SDL_UnloadObject(fs_handle);
        fs_handle = NULL;
    }
}

static int
LoadFusionSoundLibrary(void)
{
    int i, retval = -1;

    if (fs_handle == NULL) {
        fs_handle = SDL_LoadObject(fs_library);
        if (fs_handle != NULL) {
            retval = 0;
            for (i = 0; i < SDL_arraysize(fs_functions); ++i) {
                *fs_functions[i].func =
                    SDL_LoadFunction(fs_handle, fs_functions[i].name);
                if (!*fs_functions[i].func) {
                    retval = -1;
                    UnloadFusionSoundLibrary();
                    break;
                }
            }
        }
    }

    return retval;
}

#else

static void
UnloadFusionSoundLibrary()
{
    return;
}

static int
LoadFusionSoundLibrary(void)
{
    return 0;
}

#endif /* SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC */

/* This function waits until it is possible to write a full sound buffer */
static void
SDL_FS_WaitDevice(_THIS)
{
    this->hidden->stream->Wait(this->hidden->stream,
                               this->hidden->mixsamples);
}

static void
SDL_FS_PlayDevice(_THIS)
{
    DirectResult ret;

    ret = this->hidden->stream->Write(this->hidden->stream,
                                      this->hidden->mixbuf,
                                      this->hidden->mixsamples);
    /* If we couldn't write, assume fatal error for now */
    if (ret) {
        this->enabled = 0;
    }
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Wrote %d bytes of audio data\n", this->hidden->mixlen);
#endif
}

static void
SDL_FS_WaitDone(_THIS)
{
    this->hidden->stream->Wait(this->hidden->stream,
                               this->hidden->mixsamples * FUSION_BUFFERS);
}


static Uint8 *
SDL_FS_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}


static void
SDL_FS_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->stream) {
            this->hidden->stream->Release(this->hidden->stream);
            this->hidden->stream = NULL;
        }
        if (this->hidden->fs) {
            this->hidden->fs->Release(this->hidden->fs);
            this->hidden->fs = NULL;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


static int
SDL_FS_OpenDevice(_THIS, const char *devname, int iscapture)
{
    int bytes;
    SDL_AudioFormat test_format = 0, format = 0;
    FSSampleFormat fs_format;
    FSStreamDescription desc;
    DirectResult ret;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Try for a closest match on audio format */
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         !format && test_format;) {
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
        switch (test_format) {
        case AUDIO_U8:
            fs_format = FSSF_U8;
            bytes = 1;
            format = 1;
            break;
        case AUDIO_S16SYS:
            fs_format = FSSF_S16;
            bytes = 2;
            format = 1;
            break;
        case AUDIO_S32SYS:
            fs_format = FSSF_S32;
            bytes = 4;
            format = 1;
            break;
        case AUDIO_F32SYS:
            fs_format = FSSF_FLOAT;
            bytes = 4;
            format = 1;
            break;
        default:
            format = 0;
            break;
        }
        if (!format) {
            test_format = SDL_NextAudioFormat();
        }
    }

    if (format == 0) {
        SDL_FS_CloseDevice(this);
        SDL_SetError("Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    /* Retrieve the main sound interface. */
    ret = SDL_NAME(FusionSoundCreate) (&this->hidden->fs);
    if (ret) {
        SDL_FS_CloseDevice(this);
        SDL_SetError("Unable to initialize FusionSound: %d", ret);
        return 0;
    }

    this->hidden->mixsamples = this->spec.size / bytes / this->spec.channels;

    /* Fill stream description. */
    desc.flags = FSSDF_SAMPLERATE | FSSDF_BUFFERSIZE |
        FSSDF_CHANNELS | FSSDF_SAMPLEFORMAT | FSSDF_PREBUFFER;
    desc.samplerate = this->spec.freq;
    desc.buffersize = this->spec.size * FUSION_BUFFERS;
    desc.channels = this->spec.channels;
    desc.prebuffer = 10;
    desc.sampleformat = fs_format;

    ret =
        this->hidden->fs->CreateStream(this->hidden->fs, &desc,
                                       &this->hidden->stream);
    if (ret) {
        SDL_FS_CloseDevice(this);
        SDL_SetError("Unable to create FusionSoundStream: %d", ret);
        return 0;
    }

    /* See what we got */
    desc.flags = FSSDF_SAMPLERATE | FSSDF_BUFFERSIZE |
        FSSDF_CHANNELS | FSSDF_SAMPLEFORMAT;
    ret = this->hidden->stream->GetDescription(this->hidden->stream, &desc);

    this->spec.freq = desc.samplerate;
    this->spec.size =
        desc.buffersize / FUSION_BUFFERS * bytes * desc.channels;
    this->spec.channels = desc.channels;

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        SDL_FS_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    /* We're ready to rock and roll. :-) */
    return 1;
}


static void
SDL_FS_Deinitialize(void)
{
    UnloadFusionSoundLibrary();
}


static int
SDL_FS_Init(SDL_AudioDriverImpl * impl)
{
    if (LoadFusionSoundLibrary() < 0) {
        return 0;
    } else {
        DirectResult ret;

        ret = SDL_NAME(FusionSoundInit) (NULL, NULL);
        if (ret) {
            UnloadFusionSoundLibrary();
            SDL_SetError
                ("FusionSound: SDL_FS_init failed (FusionSoundInit: %d)",
                 ret);
            return 0;
        }
    }

    /* Set the function pointers */
    impl->OpenDevice = SDL_FS_OpenDevice;
    impl->PlayDevice = SDL_FS_PlayDevice;
    impl->WaitDevice = SDL_FS_WaitDevice;
    impl->GetDeviceBuf = SDL_FS_GetDeviceBuf;
    impl->CloseDevice = SDL_FS_CloseDevice;
    impl->WaitDone = SDL_FS_WaitDone;
    impl->Deinitialize = SDL_FS_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;   /* this audio target is available. */
}


AudioBootStrap FUSIONSOUND_bootstrap = {
    SDL_FS_DRIVER_NAME, "FusionSound", SDL_FS_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
