/*
 * Internal interfaces for PortAudio Apple AUHAL implementation
 *
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Written by Bjorn Roche of XO Audio LLC, from PA skeleton code.
 * Portions copied from code by Dominic Mazzoni (who wrote a HAL implementation)
 *
 * Dominic's code was based on code by Phil Burk, Darren Gibbs,
 * Gord Peters, Stephane Letz, and Greg Pfiel.
 *
 * The following people also deserve acknowledgements:
 *
 * Olivier Tristan for feedback and testing
 * Glenn Zelniker and Z-Systems engineering for sponsoring the Blocking I/O
 * interface.
 * 
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/**
 @file pa_mac_core
 @ingroup hostapi_src
 @author Bjorn Roche
 @brief AUHAL implementation of PortAudio
*/

#ifndef PA_MAC_CORE_INTERNAL_H__
#define PA_MAC_CORE_INTERNAL_H__

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#include "portaudio.h"
#include "pa_util.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_allocation.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_ringbuffer.h"

#include "pa_mac_core_blocking.h"

/* function prototypes */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

PaError PaMacCore_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define RING_BUFFER_ADVANCE_DENOMINATOR (4)

PaError ReadStream( PaStream* stream, void *buffer, unsigned long frames );
PaError WriteStream( PaStream* stream, const void *buffer, unsigned long frames );
signed long GetStreamReadAvailable( PaStream* stream );
signed long GetStreamWriteAvailable( PaStream* stream );
/* PaMacAUHAL - host api datastructure specific to this implementation */
typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;

    /* implementation specific data goes here */
    long devCount;
    AudioDeviceID *devIds; /*array of all audio devices*/
    AudioDeviceID defaultIn;
    AudioDeviceID defaultOut;
}
PaMacAUHAL;

typedef struct PaMacCoreDeviceProperties
{
    /* Values in Frames from property queries. */
    UInt32 safetyOffset;
    UInt32 bufferFrameSize;
    // UInt32 streamLatency; // Seems to be the same as deviceLatency!?
    UInt32 deviceLatency;
    /* Current device sample rate. May change! 
       These are initialized to the nominal device sample rate, 
       and updated with the actual sample rate, when/where available. 
       Note that these are the *device* sample rates, prior to any required 
       SR conversion. */
    Float64 sampleRate;
    Float64 samplePeriod; // reciprocal
}
PaMacCoreDeviceProperties;

/* stream data structure specifically for this implementation */
typedef struct PaMacCoreStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    /* implementation specific data goes here */
    bool bufferProcessorIsInitialized;
    AudioUnit inputUnit;
    AudioUnit outputUnit;
    AudioDeviceID inputDevice;
    AudioDeviceID outputDevice;
    size_t userInChan;
    size_t userOutChan;
    size_t inputFramesPerBuffer;
    size_t outputFramesPerBuffer;
    PaMacBlio blio;
    /* We use this ring buffer when input and out devs are different. */
    PaUtilRingBuffer inputRingBuffer;
    /* We may need to do SR conversion on input. */
    AudioConverterRef inputSRConverter;
    /* We need to preallocate an inputBuffer for reading data. */
    AudioBufferList inputAudioBufferList;
    AudioTimeStamp startTime;
    /* FIXME: instead of volatile, these should be properly memory barriered */
    volatile uint32_t xrunFlags; /*PaStreamCallbackFlags*/
    volatile enum {
       STOPPED          = 0, /* playback is completely stopped,
                                and the user has called StopStream(). */
       CALLBACK_STOPPED = 1, /* callback has requested stop,
                                but user has not yet called StopStream(). */
       STOPPING         = 2, /* The stream is in the process of closing
                                because the user has called StopStream.
                                This state is just used internally;
                                externally it is indistinguishable from
                                ACTIVE.*/
       ACTIVE           = 3  /* The stream is active and running. */
    } state;
    double sampleRate;
    PaMacCoreDeviceProperties  inputProperties;
    PaMacCoreDeviceProperties  outputProperties;
    
	/* data updated by main thread and notifications, protected by timingInformationMutex */
	int timingInformationMutexIsInitialized;
	pthread_mutex_t timingInformationMutex;

    /* These are written by the PA thread or from CoreAudio callbacks. Protected by the mutex. */
    Float64 timestampOffsetCombined;
    Float64 timestampOffsetInputDevice;
    Float64 timestampOffsetOutputDevice;
	
	/* Offsets in seconds to be applied to Apple timestamps to convert them to PA timestamps.
     * While the io proc is active, the following values are only accessed and manipulated by the ioproc */
    Float64 timestampOffsetCombined_ioProcCopy;
    Float64 timestampOffsetInputDevice_ioProcCopy;
    Float64 timestampOffsetOutputDevice_ioProcCopy;
    
}
PaMacCoreStream;

#endif /* PA_MAC_CORE_INTERNAL_H__ */
