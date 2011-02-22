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

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5
#include <AudioUnit/AUNTComponent.h>
#endif

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_coreaudio.h"

#define DEBUG_COREAUDIO 0

typedef struct COREAUDIO_DeviceList
{
    AudioDeviceID id;
    const char *name;
} COREAUDIO_DeviceList;

static COREAUDIO_DeviceList *inputDevices = NULL;
static int inputDeviceCount = 0;
static COREAUDIO_DeviceList *outputDevices = NULL;
static int outputDeviceCount = 0;

static void
free_device_list(COREAUDIO_DeviceList ** devices, int *devCount)
{
    if (*devices) {
        int i = *devCount;
        while (i--)
            SDL_free((void *) (*devices)[i].name);
        SDL_free(*devices);
        *devices = NULL;
    }
    *devCount = 0;
}


static void
build_device_list(int iscapture, COREAUDIO_DeviceList ** devices,
                  int *devCount)
{
    Boolean outWritable = 0;
    OSStatus result = noErr;
    UInt32 size = 0;
    AudioDeviceID *devs = NULL;
    UInt32 i = 0;
    UInt32 max = 0;

    free_device_list(devices, devCount);

    result = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
                                          &size, &outWritable);

    if (result != kAudioHardwareNoError)
        return;

    devs = (AudioDeviceID *) alloca(size);
    if (devs == NULL)
        return;

    max = size / sizeof(AudioDeviceID);
    *devices = (COREAUDIO_DeviceList *) SDL_malloc(max * sizeof(**devices));
    if (*devices == NULL)
        return;

    result = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
                                      &size, devs);
    if (result != kAudioHardwareNoError)
        return;

    for (i = 0; i < max; i++) {
        CFStringRef cfstr = NULL;
        char *ptr = NULL;
        AudioDeviceID dev = devs[i];
        AudioBufferList *buflist = NULL;
        int usable = 0;
        CFIndex len = 0;

        result = AudioDeviceGetPropertyInfo(dev, 0, iscapture,
                                            kAudioDevicePropertyStreamConfiguration,
                                            &size, &outWritable);
        if (result != noErr)
            continue;

        buflist = (AudioBufferList *) SDL_malloc(size);
        if (buflist == NULL)
            continue;

        result = AudioDeviceGetProperty(dev, 0, iscapture,
                                        kAudioDevicePropertyStreamConfiguration,
                                        &size, buflist);

        if (result == noErr) {
            UInt32 j;
            for (j = 0; j < buflist->mNumberBuffers; j++) {
                if (buflist->mBuffers[j].mNumberChannels > 0) {
                    usable = 1;
                    break;
                }
            }
        }

        SDL_free(buflist);

        if (!usable)
            continue;

        size = sizeof(CFStringRef);
        result = AudioDeviceGetProperty(dev, 0, iscapture,
                                        kAudioDevicePropertyDeviceNameCFString,
                                        &size, &cfstr);

        if (result != kAudioHardwareNoError)
            continue;

        len = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr),
                                                kCFStringEncodingUTF8);

        ptr = (char *) SDL_malloc(len + 1);
        usable = ((ptr != NULL) &&
                  (CFStringGetCString
                   (cfstr, ptr, len + 1, kCFStringEncodingUTF8)));

        CFRelease(cfstr);

        if (usable) {
            len = strlen(ptr);
            /* Some devices have whitespace at the end...trim it. */
            while ((len > 0) && (ptr[len - 1] == ' ')) {
                len--;
            }
            usable = (len > 0);
        }

        if (!usable) {
            SDL_free(ptr);
        } else {
            ptr[len] = '\0';

#if DEBUG_COREAUDIO
            printf("COREAUDIO: Found %s device #%d: '%s' (devid %d)\n",
                   ((iscapture) ? "capture" : "output"),
                   (int) *devCount, ptr, (int) dev);
#endif

            (*devices)[*devCount].id = dev;
            (*devices)[*devCount].name = ptr;
            (*devCount)++;
        }
    }
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


static int
find_device_id(const char *devname, int iscapture, AudioDeviceID * id)
{
    int i = ((iscapture) ? inputDeviceCount : outputDeviceCount);
    COREAUDIO_DeviceList *devs = ((iscapture) ? inputDevices : outputDevices);
    while (i--) {
        if (SDL_strcmp(devname, devs->name) == 0) {
            *id = devs->id;
            return 1;
        }
        devs++;
    }

    return 0;
}


static int
COREAUDIO_DetectDevices(int iscapture)
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
COREAUDIO_GetDeviceName(int index, int iscapture)
{
    if ((iscapture) && (index < inputDeviceCount)) {
        return inputDevices[index].name;
    } else if ((!iscapture) && (index < outputDeviceCount)) {
        return outputDevices[index].name;
    }

    SDL_SetError("No such device");
    return NULL;
}


static void
COREAUDIO_Deinitialize(void)
{
    free_device_lists();
}


/* The CoreAudio callback */
static OSStatus
outputCallback(void *inRefCon,
               AudioUnitRenderActionFlags * ioActionFlags,
               const AudioTimeStamp * inTimeStamp,
               UInt32 inBusNumber, UInt32 inNumberFrames,
               AudioBufferList * ioData)
{
    SDL_AudioDevice *this = (SDL_AudioDevice *) inRefCon;
    AudioBuffer *abuf;
    UInt32 remaining, len;
    void *ptr;
    UInt32 i;

    /* Only do anything if audio is enabled and not paused */
    if (!this->enabled || this->paused) {
        for (i = 0; i < ioData->mNumberBuffers; i++) {
            abuf = &ioData->mBuffers[i];
            SDL_memset(abuf->mData, this->spec.silence, abuf->mDataByteSize);
        }
        return 0;
    }

    /* No SDL conversion should be needed here, ever, since we accept
       any input format in OpenAudio, and leave the conversion to CoreAudio.
     */
    /*
       assert(!this->convert.needed);
       assert(this->spec.channels == ioData->mNumberChannels);
     */

    for (i = 0; i < ioData->mNumberBuffers; i++) {
        abuf = &ioData->mBuffers[i];
        remaining = abuf->mDataByteSize;
        ptr = abuf->mData;
        while (remaining > 0) {
            if (this->hidden->bufferOffset >= this->hidden->bufferSize) {
                /* Generate the data */
                SDL_memset(this->hidden->buffer, this->spec.silence,
                           this->hidden->bufferSize);
                SDL_mutexP(this->mixer_lock);
                (*this->spec.callback)(this->spec.userdata,
                            this->hidden->buffer, this->hidden->bufferSize);
                SDL_mutexV(this->mixer_lock);
                this->hidden->bufferOffset = 0;
            }

            len = this->hidden->bufferSize - this->hidden->bufferOffset;
            if (len > remaining)
                len = remaining;
            SDL_memcpy(ptr, (char *)this->hidden->buffer +
                       this->hidden->bufferOffset, len);
            ptr = (char *)ptr + len;
            remaining -= len;
            this->hidden->bufferOffset += len;
        }
    }

    return 0;
}

static OSStatus
inputCallback(void *inRefCon,
              AudioUnitRenderActionFlags * ioActionFlags,
              const AudioTimeStamp * inTimeStamp,
              UInt32 inBusNumber, UInt32 inNumberFrames,
              AudioBufferList * ioData)
{
    //err = AudioUnitRender(afr->fAudioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, afr->fAudioBuffer);
    // !!! FIXME: write me!
    return noErr;
}


static void
COREAUDIO_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->audioUnitOpened) {
            OSStatus result = noErr;
            AURenderCallbackStruct callback;
            const AudioUnitElement output_bus = 0;
            const AudioUnitElement input_bus = 1;
            const int iscapture = this->iscapture;
            const AudioUnitElement bus =
                ((iscapture) ? input_bus : output_bus);
            const AudioUnitScope scope =
                ((iscapture) ? kAudioUnitScope_Output :
                 kAudioUnitScope_Input);

            /* stop processing the audio unit */
            result = AudioOutputUnitStop(this->hidden->audioUnit);

            /* Remove the input callback */
            SDL_memset(&callback, '\0', sizeof(AURenderCallbackStruct));
            result = AudioUnitSetProperty(this->hidden->audioUnit,
                                          kAudioUnitProperty_SetRenderCallback,
                                          scope, bus, &callback,
                                          sizeof(callback));

            CloseComponent(this->hidden->audioUnit);
            this->hidden->audioUnitOpened = 0;
        }
        SDL_free(this->hidden->buffer);
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


#define CHECK_RESULT(msg) \
    if (result != noErr) { \
        COREAUDIO_CloseDevice(this); \
        SDL_SetError("CoreAudio error (%s): %d", msg, (int) result); \
        return 0; \
    }

static int
find_device_by_name(_THIS, const char *devname, int iscapture)
{
    AudioDeviceID devid = 0;
    OSStatus result = noErr;
    UInt32 size = 0;
    UInt32 alive = 0;
    pid_t pid = 0;

    if (devname == NULL) {
        size = sizeof(AudioDeviceID);
        const AudioHardwarePropertyID propid =
            ((iscapture) ? kAudioHardwarePropertyDefaultInputDevice :
             kAudioHardwarePropertyDefaultOutputDevice);

        result = AudioHardwareGetProperty(propid, &size, &devid);
        CHECK_RESULT("AudioHardwareGetProperty (default device)");
    } else {
        if (!find_device_id(devname, iscapture, &devid)) {
            SDL_SetError("CoreAudio: No such audio device.");
            return 0;
        }
    }

    size = sizeof(alive);
    result = AudioDeviceGetProperty(devid, 0, iscapture,
                                    kAudioDevicePropertyDeviceIsAlive,
                                    &size, &alive);
    CHECK_RESULT
        ("AudioDeviceGetProperty (kAudioDevicePropertyDeviceIsAlive)");

    if (!alive) {
        SDL_SetError("CoreAudio: requested device exists, but isn't alive.");
        return 0;
    }

    size = sizeof(pid);
    result = AudioDeviceGetProperty(devid, 0, iscapture,
                                    kAudioDevicePropertyHogMode, &size, &pid);

    /* some devices don't support this property, so errors are fine here. */
    if ((result == noErr) && (pid != -1)) {
        SDL_SetError("CoreAudio: requested device is being hogged.");
        return 0;
    }

    this->hidden->deviceID = devid;
    return 1;
}


static int
prepare_audiounit(_THIS, const char *devname, int iscapture,
                  const AudioStreamBasicDescription * strdesc)
{
    OSStatus result = noErr;
    AURenderCallbackStruct callback;
    ComponentDescription desc;
    Component comp = NULL;
    const AudioUnitElement output_bus = 0;
    const AudioUnitElement input_bus = 1;
    const AudioUnitElement bus = ((iscapture) ? input_bus : output_bus);
    const AudioUnitScope scope = ((iscapture) ? kAudioUnitScope_Output :
                                  kAudioUnitScope_Input);

    if (!find_device_by_name(this, devname, iscapture)) {
        SDL_SetError("Couldn't find requested CoreAudio device");
        return 0;
    }

    SDL_memset(&desc, '\0', sizeof(ComponentDescription));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    comp = FindNextComponent(NULL, &desc);
    if (comp == NULL) {
        SDL_SetError("Couldn't find requested CoreAudio component");
        return 0;
    }

    /* Open & initialize the audio unit */
    result = OpenAComponent(comp, &this->hidden->audioUnit);
    CHECK_RESULT("OpenAComponent");

    this->hidden->audioUnitOpened = 1;

    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioOutputUnitProperty_CurrentDevice,
                                  kAudioUnitScope_Global, 0,
                                  &this->hidden->deviceID,
                                  sizeof(AudioDeviceID));
    CHECK_RESULT
        ("AudioUnitSetProperty (kAudioOutputUnitProperty_CurrentDevice)");

    /* Set the data format of the audio unit. */
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  scope, bus, strdesc, sizeof(*strdesc));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_StreamFormat)");

    /* Set the audio callback */
    SDL_memset(&callback, '\0', sizeof(AURenderCallbackStruct));
    callback.inputProc = ((iscapture) ? inputCallback : outputCallback);
    callback.inputProcRefCon = this;
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  scope, bus, &callback, sizeof(callback));
    CHECK_RESULT
        ("AudioUnitSetProperty (kAudioUnitProperty_SetRenderCallback)");

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate a sample buffer */
    this->hidden->bufferOffset = this->hidden->bufferSize = this->spec.size;
    this->hidden->buffer = SDL_malloc(this->hidden->bufferSize);

    result = AudioUnitInitialize(this->hidden->audioUnit);
    CHECK_RESULT("AudioUnitInitialize");

    /* Finally, start processing of the audio unit */
    result = AudioOutputUnitStart(this->hidden->audioUnit);
    CHECK_RESULT("AudioOutputUnitStart");

    /* We're running! */
    return 1;
}


static int
COREAUDIO_OpenDevice(_THIS, const char *devname, int iscapture)
{
    AudioStreamBasicDescription strdesc;
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return (0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Setup a AudioStreamBasicDescription with the requested format */
    SDL_memset(&strdesc, '\0', sizeof(AudioStreamBasicDescription));
    strdesc.mFormatID = kAudioFormatLinearPCM;
    strdesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    strdesc.mChannelsPerFrame = this->spec.channels;
    strdesc.mSampleRate = this->spec.freq;
    strdesc.mFramesPerPacket = 1;

    while ((!valid_datatype) && (test_format)) {
        this->spec.format = test_format;
        /* Just a list of valid SDL formats, so people don't pass junk here. */
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S8:
        case AUDIO_U16LSB:
        case AUDIO_S16LSB:
        case AUDIO_U16MSB:
        case AUDIO_S16MSB:
        case AUDIO_S32LSB:
        case AUDIO_S32MSB:
        case AUDIO_F32LSB:
        case AUDIO_F32MSB:
            valid_datatype = 1;
            strdesc.mBitsPerChannel = SDL_AUDIO_BITSIZE(this->spec.format);
            if (SDL_AUDIO_ISBIGENDIAN(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;

            if (SDL_AUDIO_ISFLOAT(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
            else if (SDL_AUDIO_ISSIGNED(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            break;
        }
    }

    if (!valid_datatype) {      /* shouldn't happen, but just in case... */
        COREAUDIO_CloseDevice(this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    strdesc.mBytesPerFrame =
        strdesc.mBitsPerChannel * strdesc.mChannelsPerFrame / 8;
    strdesc.mBytesPerPacket =
        strdesc.mBytesPerFrame * strdesc.mFramesPerPacket;

    if (!prepare_audiounit(this, devname, iscapture, &strdesc)) {
        COREAUDIO_CloseDevice(this);
        return 0;               /* prepare_audiounit() will call SDL_SetError()... */
    }

    return 1;                   /* good to go. */
}

static int
COREAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->DetectDevices = COREAUDIO_DetectDevices;
    impl->GetDeviceName = COREAUDIO_GetDeviceName;
    impl->OpenDevice = COREAUDIO_OpenDevice;
    impl->CloseDevice = COREAUDIO_CloseDevice;
    impl->Deinitialize = COREAUDIO_Deinitialize;
    impl->ProvidesOwnCallbackThread = 1;

    build_device_lists();       /* do an initial check for devices... */

    return 1;   /* this audio target is available. */
}

AudioBootStrap COREAUDIO_bootstrap = {
    "coreaudio", "Mac OS X CoreAudio", COREAUDIO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
