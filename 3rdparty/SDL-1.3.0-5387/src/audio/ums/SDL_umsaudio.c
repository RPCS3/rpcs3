/* I'm gambling no one uses this audio backend...we'll see who emails.  :)  */
#error this code has not been updated for SDL 1.3.
#error if no one emails icculus at icculus.org and tells him that this
#error  code is needed, this audio backend will eventually be removed from SDL.

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

    Carsten Griwodz
    griff@kom.tu-darmstadt.de

    based on linux/SDL_dspaudio.c by Sam Lantinga
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_umsaudio.h"

/* The tag name used by UMS audio */
#define UMS_DRIVER_NAME         "ums"

#define DEBUG_AUDIO 1

/* Audio driver functions */
static int UMS_OpenAudio(_THIS, SDL_AudioSpec * spec);
static void UMS_PlayAudio(_THIS);
static Uint8 *UMS_GetAudioBuf(_THIS);
static void UMS_CloseAudio(_THIS);

static UMSAudioDevice_ReturnCode UADOpen(_THIS, string device, string mode,
                                         long flags);
static UMSAudioDevice_ReturnCode UADClose(_THIS);
static UMSAudioDevice_ReturnCode UADGetBitsPerSample(_THIS, long *bits);
static UMSAudioDevice_ReturnCode UADSetBitsPerSample(_THIS, long bits);
static UMSAudioDevice_ReturnCode UADSetSampleRate(_THIS, long rate,
                                                  long *set_rate);
static UMSAudioDevice_ReturnCode UADSetByteOrder(_THIS, string byte_order);
static UMSAudioDevice_ReturnCode UADSetAudioFormatType(_THIS, string fmt);
static UMSAudioDevice_ReturnCode UADSetNumberFormat(_THIS, string fmt);
static UMSAudioDevice_ReturnCode UADInitialize(_THIS);
static UMSAudioDevice_ReturnCode UADStart(_THIS);
static UMSAudioDevice_ReturnCode UADStop(_THIS);
static UMSAudioDevice_ReturnCode UADSetTimeFormat(_THIS,
                                                  UMSAudioTypes_TimeFormat
                                                  fmt);
static UMSAudioDevice_ReturnCode UADWriteBuffSize(_THIS, long *buff_size);
static UMSAudioDevice_ReturnCode UADWriteBuffRemain(_THIS, long *buff_size);
static UMSAudioDevice_ReturnCode UADWriteBuffUsed(_THIS, long *buff_size);
static UMSAudioDevice_ReturnCode UADSetDMABufferSize(_THIS, long bytes,
                                                     long *bytes_ret);
static UMSAudioDevice_ReturnCode UADSetVolume(_THIS, long volume);
static UMSAudioDevice_ReturnCode UADSetBalance(_THIS, long balance);
static UMSAudioDevice_ReturnCode UADSetChannels(_THIS, long channels);
static UMSAudioDevice_ReturnCode UADPlayRemainingData(_THIS, boolean block);
static UMSAudioDevice_ReturnCode UADEnableOutput(_THIS, string output,
                                                 long *left_gain,
                                                 long *right_gain);
static UMSAudioDevice_ReturnCode UADWrite(_THIS, UMSAudioTypes_Buffer * buff,
                                          long samples,
                                          long *samples_written);

/* Audio driver bootstrap functions */
static int
Audio_Available(void)
{
    return 1;
}

static void
Audio_DeleteDevice(_THIS)
{
    if (this->hidden->playbuf._buffer)
        SDL_free(this->hidden->playbuf._buffer);
    if (this->hidden->fillbuf._buffer)
        SDL_free(this->hidden->fillbuf._buffer);
    _somFree(this->hidden->umsdev);
    SDL_free(this->hidden);
    SDL_free(this);
}

static SDL_AudioDevice *
Audio_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;

    /*
     * Allocate and initialize management storage and private management
     * storage for this SDL-using library.
     */
    this = (SDL_AudioDevice *) SDL_malloc(sizeof(SDL_AudioDevice));
    if (this) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)
            SDL_malloc((sizeof *this->hidden));
    }
    if ((this == NULL) || (this->hidden == NULL)) {
        SDL_OutOfMemory();
        if (this) {
            SDL_free(this);
        }
        return (0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Creating UMS Audio device\n");
#endif

    /*
     * Calls for UMS env initialization and audio object construction.
     */
    this->hidden->ev = somGetGlobalEnvironment();
    this->hidden->umsdev = UMSAudioDeviceNew();

    /*
     * Set the function pointers.
     */
    this->OpenAudio = UMS_OpenAudio;
    this->WaitAudio = NULL;     /* we do blocking output */
    this->PlayAudio = UMS_PlayAudio;
    this->GetAudioBuf = UMS_GetAudioBuf;
    this->CloseAudio = UMS_CloseAudio;
    this->free = Audio_DeleteDevice;

#ifdef DEBUG_AUDIO
    fprintf(stderr, "done\n");
#endif
    return this;
}

AudioBootStrap UMS_bootstrap = {
    UMS_DRIVER_NAME, "AIX UMS audio",
    Audio_Available, Audio_CreateDevice, 0
};

static Uint8 *
UMS_GetAudioBuf(_THIS)
{
#ifdef DEBUG_AUDIO
    fprintf(stderr, "enter UMS_GetAudioBuf\n");
#endif
    return this->hidden->fillbuf._buffer;
/*
    long                      bufSize;
    UMSAudioDevice_ReturnCode rc;

    rc = UADSetTimeFormat(this, UMSAudioTypes_Bytes );
    rc = UADWriteBuffSize(this,  bufSize );
*/
}

static void
UMS_CloseAudio(_THIS)
{
    UMSAudioDevice_ReturnCode rc;

#ifdef DEBUG_AUDIO
    fprintf(stderr, "enter UMS_CloseAudio\n");
#endif
    rc = UADPlayRemainingData(this, TRUE);
    rc = UADStop(this);
    rc = UADClose(this);
}

static void
UMS_PlayAudio(_THIS)
{
    UMSAudioDevice_ReturnCode rc;
    long samplesToWrite;
    long samplesWritten;
    UMSAudioTypes_Buffer swpbuf;

#ifdef DEBUG_AUDIO
    fprintf(stderr, "enter UMS_PlayAudio\n");
#endif
    samplesToWrite =
        this->hidden->playbuf._length / this->hidden->bytesPerSample;
    do {
        rc = UADWrite(this, &this->hidden->playbuf,
                      samplesToWrite, &samplesWritten);
        samplesToWrite -= samplesWritten;

        /* rc values: UMSAudioDevice_Success
         *            UMSAudioDevice_Failure
         *            UMSAudioDevice_Preempted
         *            UMSAudioDevice_Interrupted
         *            UMSAudioDevice_DeviceError
         */
        if (rc == UMSAudioDevice_DeviceError) {
#ifdef DEBUG_AUDIO
            fprintf(stderr, "Returning from PlayAudio with devices error\n");
#endif
            return;
        }
    } while (samplesToWrite > 0);

    SDL_LockAudio();
    SDL_memcpy(&swpbuf, &this->hidden->playbuf, sizeof(UMSAudioTypes_Buffer));
    SDL_memcpy(&this->hidden->playbuf, &this->hidden->fillbuf,
               sizeof(UMSAudioTypes_Buffer));
    SDL_memcpy(&this->hidden->fillbuf, &swpbuf, sizeof(UMSAudioTypes_Buffer));
    SDL_UnlockAudio();

#ifdef DEBUG_AUDIO
    fprintf(stderr, "Wrote audio data and swapped buffer\n");
#endif
}

#if 0
//      /* Set the DSP frequency */
//      value = spec->freq;
//      if ( ioctl(this->hidden->audio_fd, SOUND_PCM_WRITE_RATE, &value) < 0 ) {
//              SDL_SetError("Couldn't set audio frequency");
//              return(-1);
//      }
//      spec->freq = value;
#endif

static int
UMS_OpenAudio(_THIS, SDL_AudioSpec * spec)
{
    char *audiodev = "/dev/paud0";
    long lgain;
    long rgain;
    long outRate;
    long outBufSize;
    long bitsPerSample;
    long samplesPerSec;
    long success;
    SDL_AudioFormat test_format;
    int frag_spec;
    UMSAudioDevice_ReturnCode rc;

#ifdef DEBUG_AUDIO
    fprintf(stderr, "enter UMS_OpenAudio\n");
#endif
    rc = UADOpen(this, audiodev, "PLAY", UMSAudioDevice_BlockingIO);
    if (rc != UMSAudioDevice_Success) {
        SDL_SetError("Couldn't open %s: %s", audiodev, strerror(errno));
        return -1;
    }

    rc = UADSetAudioFormatType(this, "PCM");

    success = 0;
    test_format = SDL_FirstAudioFormat(spec->format);
    do {
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
        switch (test_format) {
        case AUDIO_U8:
/* from the mac code: better ? */
/* sample_bits = spec->size / spec->samples / spec->channels * 8; */
            success = 1;
            bitsPerSample = 8;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "MSB");  /* irrelevant */
            rc = UADSetNumberFormat(this, "UNSIGNED");
            break;
        case AUDIO_S8:
            success = 1;
            bitsPerSample = 8;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "MSB");  /* irrelevant */
            rc = UADSetNumberFormat(this, "SIGNED");
            break;
        case AUDIO_S16LSB:
            success = 1;
            bitsPerSample = 16;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "LSB");
            rc = UADSetNumberFormat(this, "SIGNED");
            break;
        case AUDIO_S16MSB:
            success = 1;
            bitsPerSample = 16;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "MSB");
            rc = UADSetNumberFormat(this, "SIGNED");
            break;
        case AUDIO_U16LSB:
            success = 1;
            bitsPerSample = 16;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "LSB");
            rc = UADSetNumberFormat(this, "UNSIGNED");
            break;
        case AUDIO_U16MSB:
            success = 1;
            bitsPerSample = 16;
            rc = UADSetSampleRate(this, spec->freq << 16, &outRate);
            rc = UADSetByteOrder(this, "MSB");
            rc = UADSetNumberFormat(this, "UNSIGNED");
            break;
        default:
            break;
        }
        if (!success) {
            test_format = SDL_NextAudioFormat();
        }
    } while (!success && test_format);

    if (success == 0) {
        SDL_SetError("Couldn't find any hardware audio formats");
        return -1;
    }

    spec->format = test_format;

    for (frag_spec = 0; (0x01 << frag_spec) < spec->size; ++frag_spec);
    if ((0x01 << frag_spec) != spec->size) {
        SDL_SetError("Fragment size must be a power of two");
        return -1;
    }
    if (frag_spec > 2048)
        frag_spec = 2048;

    this->hidden->bytesPerSample = (bitsPerSample / 8) * spec->channels;
    samplesPerSec = this->hidden->bytesPerSample * outRate;

    this->hidden->playbuf._length = 0;
    this->hidden->playbuf._maximum = spec->size;
    this->hidden->playbuf._buffer = (unsigned char *) SDL_malloc(spec->size);
    this->hidden->fillbuf._length = 0;
    this->hidden->fillbuf._maximum = spec->size;
    this->hidden->fillbuf._buffer = (unsigned char *) SDL_malloc(spec->size);

    rc = UADSetBitsPerSample(this, bitsPerSample);
    rc = UADSetDMABufferSize(this, frag_spec, &outBufSize);
    rc = UADSetChannels(this, spec->channels);  /* functions reduces to mono or stereo */

    lgain = 100;                /*maximum left input gain */
    rgain = 100;                /*maimum right input gain */
    rc = UADEnableOutput(this, "LINE_OUT", &lgain, &rgain);
    rc = UADInitialize(this);
    rc = UADStart(this);
    rc = UADSetVolume(this, 100);
    rc = UADSetBalance(this, 0);

    /* We're ready to rock and roll. :-) */
    return 0;
}


static UMSAudioDevice_ReturnCode
UADGetBitsPerSample(_THIS, long *bits)
{
    return UMSAudioDevice_get_bits_per_sample(this->hidden->umsdev,
                                              this->hidden->ev, bits);
}

static UMSAudioDevice_ReturnCode
UADSetBitsPerSample(_THIS, long bits)
{
    return UMSAudioDevice_set_bits_per_sample(this->hidden->umsdev,
                                              this->hidden->ev, bits);
}

static UMSAudioDevice_ReturnCode
UADSetSampleRate(_THIS, long rate, long *set_rate)
{
    /* from the mac code: sample rate = spec->freq << 16; */
    return UMSAudioDevice_set_sample_rate(this->hidden->umsdev,
                                          this->hidden->ev, rate, set_rate);
}

static UMSAudioDevice_ReturnCode
UADSetByteOrder(_THIS, string byte_order)
{
    return UMSAudioDevice_set_byte_order(this->hidden->umsdev,
                                         this->hidden->ev, byte_order);
}

static UMSAudioDevice_ReturnCode
UADSetAudioFormatType(_THIS, string fmt)
{
    /* possible PCM, A_LAW or MU_LAW */
    return UMSAudioDevice_set_audio_format_type(this->hidden->umsdev,
                                                this->hidden->ev, fmt);
}

static UMSAudioDevice_ReturnCode
UADSetNumberFormat(_THIS, string fmt)
{
    /* possible SIGNED, UNSIGNED, or TWOS_COMPLEMENT */
    return UMSAudioDevice_set_number_format(this->hidden->umsdev,
                                            this->hidden->ev, fmt);
}

static UMSAudioDevice_ReturnCode
UADInitialize(_THIS)
{
    return UMSAudioDevice_initialize(this->hidden->umsdev, this->hidden->ev);
}

static UMSAudioDevice_ReturnCode
UADStart(_THIS)
{
    return UMSAudioDevice_start(this->hidden->umsdev, this->hidden->ev);
}

static UMSAudioDevice_ReturnCode
UADSetTimeFormat(_THIS, UMSAudioTypes_TimeFormat fmt)
{
    /*
     * Switches the time format to the new format, immediately.
     * possible UMSAudioTypes_Msecs, UMSAudioTypes_Bytes or UMSAudioTypes_Samples
     */
    return UMSAudioDevice_set_time_format(this->hidden->umsdev,
                                          this->hidden->ev, fmt);
}

static UMSAudioDevice_ReturnCode
UADWriteBuffSize(_THIS, long *buff_size)
{
    /*
     * returns write buffer size in the current time format
     */
    return UMSAudioDevice_write_buff_size(this->hidden->umsdev,
                                          this->hidden->ev, buff_size);
}

static UMSAudioDevice_ReturnCode
UADWriteBuffRemain(_THIS, long *buff_size)
{
    /*
     * returns amount of available space in the write buffer
     * in the current time format
     */
    return UMSAudioDevice_write_buff_remain(this->hidden->umsdev,
                                            this->hidden->ev, buff_size);
}

static UMSAudioDevice_ReturnCode
UADWriteBuffUsed(_THIS, long *buff_size)
{
    /*
     * returns amount of filled space in the write buffer
     * in the current time format
     */
    return UMSAudioDevice_write_buff_used(this->hidden->umsdev,
                                          this->hidden->ev, buff_size);
}

static UMSAudioDevice_ReturnCode
UADSetDMABufferSize(_THIS, long bytes, long *bytes_ret)
{
    /*
     * Request a new DMA buffer size, maximum requested size 2048.
     * Takes effect with next initialize() call.
     * Devices may or may not support DMA.
     */
    return UMSAudioDevice_set_DMA_buffer_size(this->hidden->umsdev,
                                              this->hidden->ev,
                                              bytes, bytes_ret);
}

static UMSAudioDevice_ReturnCode
UADSetVolume(_THIS, long volume)
{
    /*
     * Set the volume.
     * Takes effect immediately.
     */
    return UMSAudioDevice_set_volume(this->hidden->umsdev,
                                     this->hidden->ev, volume);
}

static UMSAudioDevice_ReturnCode
UADSetBalance(_THIS, long balance)
{
    /*
     * Set the balance.
     * Takes effect immediately.
     */
    return UMSAudioDevice_set_balance(this->hidden->umsdev,
                                      this->hidden->ev, balance);
}

static UMSAudioDevice_ReturnCode
UADSetChannels(_THIS, long channels)
{
    /*
     * Set mono or stereo.
     * Takes effect with next initialize() call.
     */
    if (channels != 1)
        channels = 2;
    return UMSAudioDevice_set_number_of_channels(this->hidden->umsdev,
                                                 this->hidden->ev, channels);
}

static UMSAudioDevice_ReturnCode
UADOpen(_THIS, string device, string mode, long flags)
{
    return UMSAudioDevice_open(this->hidden->umsdev,
                               this->hidden->ev, device, mode, flags);
}

static UMSAudioDevice_ReturnCode
UADWrite(_THIS, UMSAudioTypes_Buffer * buff,
         long samples, long *samples_written)
{
    return UMSAudioDevice_write(this->hidden->umsdev,
                                this->hidden->ev,
                                buff, samples, samples_written);
}

static UMSAudioDevice_ReturnCode
UADPlayRemainingData(_THIS, boolean block)
{
    return UMSAudioDevice_play_remaining_data(this->hidden->umsdev,
                                              this->hidden->ev, block);
}

static UMSAudioDevice_ReturnCode
UADStop(_THIS)
{
    return UMSAudioDevice_stop(this->hidden->umsdev, this->hidden->ev);
}

static UMSAudioDevice_ReturnCode
UADClose(_THIS)
{
    return UMSAudioDevice_close(this->hidden->umsdev, this->hidden->ev);
}

static UMSAudioDevice_ReturnCode
UADEnableOutput(_THIS, string output, long *left_gain, long *right_gain)
{
    return UMSAudioDevice_enable_output(this->hidden->umsdev,
                                        this->hidden->ev,
                                        output, left_gain, right_gain);
}

/* vi: set ts=4 sw=4 expandtab: */
