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

#include "../../core/windows/SDL_windows.h"
#include <mmsystem.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_dibaudio.h"
#if defined(_WIN32_WCE) && (_WIN32_WCE < 300)
#include "win_ce_semaphore.h"
#endif

#if defined(_WIN32_WCE)
#define WINDOWS_OS_NAME "Windows CE/PocketPC"
#elif defined(WIN64)
#define WINDOWS_OS_NAME "Win64"
#else
#define WINDOWS_OS_NAME "Win32"
#endif

/* The Win32 callback for filling the WAVE device */
static void CALLBACK
FillSound(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
          DWORD dwParam1, DWORD dwParam2)
{
    SDL_AudioDevice *this = (SDL_AudioDevice *) dwInstance;

    /* Only service "buffer done playing" messages */
    if (uMsg != WOM_DONE)
        return;

    /* Signal that we are done playing a buffer */
#if defined(_WIN32_WCE) && (_WIN32_WCE < 300)
    ReleaseSemaphoreCE(this->hidden->audio_sem, 1, NULL);
#else
    ReleaseSemaphore(this->hidden->audio_sem, 1, NULL);
#endif
}

static void
SetMMerror(char *function, MMRESULT code)
{
    size_t len;
    char errbuf[MAXERRORLENGTH];
    wchar_t werrbuf[MAXERRORLENGTH];

    SDL_snprintf(errbuf, SDL_arraysize(errbuf), "%s: ", function);
    len = SDL_strlen(errbuf);

    waveOutGetErrorText(code, werrbuf, MAXERRORLENGTH - len);
    WideCharToMultiByte(CP_ACP, 0, werrbuf, -1, errbuf + len,
                        MAXERRORLENGTH - len, NULL, NULL);

    SDL_SetError("%s", errbuf);
}

/* Set high priority for the audio thread */
static void
WINWAVEOUT_ThreadInit(_THIS)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

void
WINWAVEOUT_WaitDevice(_THIS)
{
    /* Wait for an audio chunk to finish */
#if defined(_WIN32_WCE) && (_WIN32_WCE < 300)
    WaitForSemaphoreCE(this->hidden->audio_sem, INFINITE);
#else
    WaitForSingleObject(this->hidden->audio_sem, INFINITE);
#endif
}

Uint8 *
WINWAVEOUT_GetDeviceBuf(_THIS)
{
    return (Uint8 *) (this->hidden->
                      wavebuf[this->hidden->next_buffer].lpData);
}

void
WINWAVEOUT_PlayDevice(_THIS)
{
    /* Queue it up */
    waveOutWrite(this->hidden->sound,
                 &this->hidden->wavebuf[this->hidden->next_buffer],
                 sizeof(this->hidden->wavebuf[0]));
    this->hidden->next_buffer = (this->hidden->next_buffer + 1) % NUM_BUFFERS;
}

void
WINWAVEOUT_WaitDone(_THIS)
{
    int i, left;

    do {
        left = NUM_BUFFERS;
        for (i = 0; i < NUM_BUFFERS; ++i) {
            if (this->hidden->wavebuf[i].dwFlags & WHDR_DONE) {
                --left;
            }
        }
        if (left > 0) {
            SDL_Delay(100);
        }
    } while (left > 0);
}

void
WINWAVEOUT_CloseDevice(_THIS)
{
    /* Close up audio */
    if (this->hidden != NULL) {
        int i;

        if (this->hidden->audio_sem) {
#if defined(_WIN32_WCE) && (_WIN32_WCE < 300)
            CloseSynchHandle(this->hidden->audio_sem);
#else
            CloseHandle(this->hidden->audio_sem);
#endif
            this->hidden->audio_sem = 0;
        }

        if (this->hidden->sound) {
            waveOutClose(this->hidden->sound);
            this->hidden->sound = 0;
        }

        /* Clean up mixing buffers */
        for (i = 0; i < NUM_BUFFERS; ++i) {
            if (this->hidden->wavebuf[i].dwUser != 0xFFFF) {
                waveOutUnprepareHeader(this->hidden->sound,
                                       &this->hidden->wavebuf[i],
                                       sizeof(this->hidden->wavebuf[i]));
                this->hidden->wavebuf[i].dwUser = 0xFFFF;
            }
        }

        if (this->hidden->mixbuf != NULL) {
            /* Free raw mixing buffer */
            SDL_free(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

int
WINWAVEOUT_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;
    MMRESULT result;
    WAVEFORMATEX waveformat;
    int i;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Initialize the wavebuf structures for closing */
    for (i = 0; i < NUM_BUFFERS; ++i)
        this->hidden->wavebuf[i].dwUser = 0xFFFF;

    while ((!valid_datatype) && (test_format)) {
        valid_datatype = 1;
        this->spec.format = test_format;
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S16:
        case AUDIO_S32:
            break;              /* valid. */

        default:
            valid_datatype = 0;
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

    if (!valid_datatype) {
        WINWAVEOUT_CloseDevice(this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    /* Set basic WAVE format parameters */
    SDL_memset(&waveformat, '\0', sizeof(waveformat));
    waveformat.wFormatTag = WAVE_FORMAT_PCM;
    waveformat.wBitsPerSample = SDL_AUDIO_BITSIZE(this->spec.format);

    if (this->spec.channels > 2)
        this->spec.channels = 2;        /* !!! FIXME: is this right? */

    waveformat.nChannels = this->spec.channels;
    waveformat.nSamplesPerSec = this->spec.freq;
    waveformat.nBlockAlign =
        waveformat.nChannels * (waveformat.wBitsPerSample / 8);
    waveformat.nAvgBytesPerSec =
        waveformat.nSamplesPerSec * waveformat.nBlockAlign;

    /* Check the buffer size -- minimum of 1/4 second (word aligned) */
    if (this->spec.samples < (this->spec.freq / 4))
        this->spec.samples = ((this->spec.freq / 4) + 3) & ~3;

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&this->spec);

    /* Open the audio device */
    result = waveOutOpen(&this->hidden->sound, WAVE_MAPPER, &waveformat,
                         (DWORD_PTR) FillSound, (DWORD_PTR) this,
                         CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        WINWAVEOUT_CloseDevice(this);
        SetMMerror("waveOutOpen()", result);
        return 0;
    }
#ifdef SOUND_DEBUG
    /* Check the sound device we retrieved */
    {
        WAVEOUTCAPS caps;

        result = waveOutGetDevCaps((UINT) this->hidden->sound,
                                   &caps, sizeof(caps));
        if (result != MMSYSERR_NOERROR) {
            WINWAVEOUT_CloseDevice(this);
            SetMMerror("waveOutGetDevCaps()", result);
            return 0;
        }
        printf("Audio device: %s\n", caps.szPname);
    }
#endif

    /* Create the audio buffer semaphore */
    this->hidden->audio_sem =
#if defined(_WIN32_WCE) && (_WIN32_WCE < 300)
        CreateSemaphoreCE(NULL, NUM_BUFFERS - 1, NUM_BUFFERS, NULL);
#else
        CreateSemaphore(NULL, NUM_BUFFERS - 1, NUM_BUFFERS, NULL);
#endif
    if (this->hidden->audio_sem == NULL) {
        WINWAVEOUT_CloseDevice(this);
        SDL_SetError("Couldn't create semaphore");
        return 0;
    }

    /* Create the sound buffers */
    this->hidden->mixbuf =
        (Uint8 *) SDL_malloc(NUM_BUFFERS * this->spec.size);
    if (this->hidden->mixbuf == NULL) {
        WINWAVEOUT_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    for (i = 0; i < NUM_BUFFERS; ++i) {
        SDL_memset(&this->hidden->wavebuf[i], '\0',
                   sizeof(this->hidden->wavebuf[i]));
        this->hidden->wavebuf[i].dwBufferLength = this->spec.size;
        this->hidden->wavebuf[i].dwFlags = WHDR_DONE;
        this->hidden->wavebuf[i].lpData =
            (LPSTR) & this->hidden->mixbuf[i * this->spec.size];
        result = waveOutPrepareHeader(this->hidden->sound,
                                      &this->hidden->wavebuf[i],
                                      sizeof(this->hidden->wavebuf[i]));
        if (result != MMSYSERR_NOERROR) {
            WINWAVEOUT_CloseDevice(this);
            SetMMerror("waveOutPrepareHeader()", result);
            return 0;
        }
    }

    return 1;                   /* Ready to go! */
}


static int
WINWAVEOUT_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = WINWAVEOUT_OpenDevice;
    impl->ThreadInit = WINWAVEOUT_ThreadInit;
    impl->PlayDevice = WINWAVEOUT_PlayDevice;
    impl->WaitDevice = WINWAVEOUT_WaitDevice;
    impl->WaitDone = WINWAVEOUT_WaitDone;
    impl->GetDeviceBuf = WINWAVEOUT_GetDeviceBuf;
    impl->CloseDevice = WINWAVEOUT_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME: Is this true? */

    return 1;   /* this audio target is available. */
}

AudioBootStrap WINWAVEOUT_bootstrap = {
    "waveout", WINDOWS_OS_NAME " WaveOut", WINWAVEOUT_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
