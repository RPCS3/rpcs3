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

/* Tru64 UNIX MME support */
#include <mme_api.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_mmeaudio.h"

static BOOL inUse[NUM_BUFFERS];

static void
SetMMerror(char *function, MMRESULT code)
{
    int len;
    char errbuf[MAXERRORLENGTH];

    SDL_snprintf(errbuf, SDL_arraysize(errbuf), "%s: ", function);
    len = SDL_strlen(errbuf);
    waveOutGetErrorText(code, errbuf + len, MAXERRORLENGTH - len);
    SDL_SetError("%s", errbuf);
}

static void CALLBACK
MME_Callback(HWAVEOUT hwo,
             UINT uMsg, DWORD dwInstance, LPARAM dwParam1, LPARAM dwParam2)
{
    WAVEHDR *wp = (WAVEHDR *) dwParam1;

    if (uMsg == WOM_DONE)
        inUse[wp->dwUser] = FALSE;
}

static int
MME_OpenDevice(_THIS, const char *devname, int iscapture)
{
    int valid_format = 0;
    MMRESULT result;
    Uint8 *mixbuf = NULL;
    int i;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Set basic WAVE format parameters */
    this->hidden->shm = mmeAllocMem(sizeof(*this->hidden->shm));
    if (this->hidden->shm == NULL) {
        MME_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }

    SDL_memset(this->hidden->shm, '\0', sizeof(*this->hidden->shm));
    this->hidden->shm->sound = 0;
    this->hidden->shm->wFmt.wf.wFormatTag = WAVE_FORMAT_PCM;

    /* Determine the audio parameters from the AudioSpec */
    /* Try for a closest match on audio format */
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         !valid_format && test_format;) {
        valid_format = 1;
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S16:
        case AUDIO_S32:
            break;
        default:
            valid_format = 0;
            test_format = SDL_NextAudioFormat();
        }
    }

    if (!valid_format) {
        MME_CloseDevice(this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    this->spec.format = test_format;
    this->hidden->shm->wFmt.wBitsPerSample = SDL_AUDIO_BITSIZE(test_format);

    /* !!! FIXME: Can this handle more than stereo? */
    this->hidden->shm->wFmt.wf.nChannels = this->spec.channels;
    this->hidden->shm->wFmt.wf.nSamplesPerSec = this->spec.freq;
    this->hidden->shm->wFmt.wf.nBlockAlign =
        this->hidden->shm->wFmt.wf.nChannels *
        this->hidden->shm->wFmt.wBitsPerSample / 8;
    this->hidden->shm->wFmt.wf.nAvgBytesPerSec =
        this->hidden->shm->wFmt.wf.nSamplesPerSec *
        this->hidden->shm->wFmt.wf.nBlockAlign;

    /* Check the buffer size -- minimum of 1/4 second (word aligned) */
    if (this->spec.samples < (this->spec.freq / 4))
        this->spec.samples = ((this->spec.freq / 4) + 3) & ~3;

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&this->spec);

    /* Open the audio device */
    result = waveOutOpen(&(this->hidden->shm->sound),
                         WAVE_MAPPER,
                         &(this->hidden->shm->wFmt.wf),
                         MME_Callback,
                         NULL, (CALLBACK_FUNCTION | WAVE_OPEN_SHAREABLE));
    if (result != MMSYSERR_NOERROR) {
        MME_CloseDevice(this);
        SetMMerror("waveOutOpen()", result);
        return 0;
    }

    /* Create the sound buffers */
    mixbuf = (Uint8 *) mmeAllocBuffer(NUM_BUFFERS * (this->spec.size));
    if (mixbuf == NULL) {
        MME_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    this->hidden->mixbuf = mixbuf;

    for (i = 0; i < NUM_BUFFERS; i++) {
        this->hidden->shm->wHdr[i].lpData = &mixbuf[i * (this->spec.size)];
        this->hidden->shm->wHdr[i].dwBufferLength = this->spec.size;
        this->hidden->shm->wHdr[i].dwFlags = 0;
        this->hidden->shm->wHdr[i].dwUser = i;
        this->hidden->shm->wHdr[i].dwLoops = 0; /* loop control counter */
        this->hidden->shm->wHdr[i].lpNext = NULL;       /* reserved for driver */
        this->hidden->shm->wHdr[i].reserved = 0;
        inUse[i] = FALSE;
    }
    this->hidden->next_buffer = 0;

    return 1;
}

static void
MME_WaitDevice(_THIS)
{
    while (inUse[this->hidden->next_buffer]) {
        mmeWaitForCallbacks();
        mmeProcessCallbacks();
    }
}

static Uint8 *
MME_GetDeviceBuf(_THIS)
{
    void *retval = this->hidden->shm->wHdr[this->hidden->next_buffer].lpData;
    inUse[this->hidden->next_buffer] = TRUE;
    return (Uint8 *) retval;
}

static void
MME_PlayDevice(_THIS)
{
    /* Queue it up */
    waveOutWrite(this->hidden->shm->sound,
                 &(this->hidden->shm->wHdr[this->hidden->next_buffer]),
                 sizeof(WAVEHDR));
    this->hidden->next_buffer = (this->hidden->next_buffer + 1) % NUM_BUFFERS;
}

static void
MME_WaitDone(_THIS)
{
    MMRESULT result;
    int i;

    if (this->hidden->shm->sound) {
        for (i = 0; i < NUM_BUFFERS; i++)
            while (inUse[i]) {
                mmeWaitForCallbacks();
                mmeProcessCallbacks();
            }
        result = waveOutReset(this->hidden->shm->sound);
        if (result != MMSYSERR_NOERROR)
            SetMMerror("waveOutReset()", result);
        mmeProcessCallbacks();
    }
}

static void
MME_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        MMRESULT result;

        if (this->hidden->mixbuf) {
            result = mmeFreeBuffer(this->hidden->mixbuf);
            if (result != MMSYSERR_NOERROR)
                SetMMerror("mmeFreeBuffer", result);
            this->hidden->mixbuf = NULL;
        }

        if (this->hidden->shm) {
            if (this->hidden->shm->sound) {
                result = waveOutClose(this->hidden->shm->sound);
                if (result != MMSYSERR_NOERROR)
                    SetMMerror("waveOutClose()", result);
                mmeProcessCallbacks();
            }
            result = mmeFreeMem(this->hidden->shm);
            if (result != MMSYSERR_NOERROR)
                SetMMerror("mmeFreeMem()", result);
            this->hidden->shm = NULL;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
MME_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = MME_OpenDevice;
    impl->WaitDevice = MME_WaitDevice;
    impl->WaitDone = MME_WaitDone;
    impl->PlayDevice = MME_PlayDevice;
    impl->GetDeviceBuf = MME_GetDeviceBuf;
    impl->CloseDevice = MME_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;

    return 1;   /* this audio target is available. */
}

/* !!! FIXME: Windows "windib" driver is called waveout, too */
AudioBootStrap MMEAUDIO_bootstrap = {
    "waveout", "Tru64 MME WaveOut", MME_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
