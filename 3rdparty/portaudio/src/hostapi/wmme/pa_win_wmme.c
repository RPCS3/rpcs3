/*
 * $Id: pa_win_wmme.c 1874 2012-10-31 06:20:59Z rbencina $
 * pa_win_wmme.c
 * Implementation of PortAudio for Windows MultiMedia Extensions (WMME)       
 *                                                                                         
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Authors: Ross Bencina and Phil Burk
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
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

/* Modification History:
 PLB = Phil Burk
 JM = Julien Maillard
 RDB = Ross Bencina
 PLB20010402 - sDevicePtrs now allocates based on sizeof(pointer)
 PLB20010413 - check for excessive numbers of channels
 PLB20010422 - apply Mike Berry's changes for CodeWarrior on PC
               including conditional inclusion of memory.h,
               and explicit typecasting on memory allocation
 PLB20010802 - use GlobalAlloc for sDevicesPtr instead of PaHost_AllocFastMemory
 PLB20010816 - pass process instead of thread to SetPriorityClass()
 PLB20010927 - use number of frames instead of real-time for CPULoad calculation.
 JM20020118 - prevent hung thread when buffers underflow.
 PLB20020321 - detect Win XP versus NT, 9x; fix DBUG typo; removed init of CurrentCount
 RDB20020411 - various renaming cleanups, factored streamData alloc and cpu usage init
 RDB20020417 - stopped counting WAVE_MAPPER when there were no real devices
               refactoring, renaming and fixed a few edge case bugs
 RDB20020531 - converted to V19 framework
 ** NOTE  maintanance history is now stored in CVS **
*/

/** @file
	@ingroup hostapi_src

    @brief Win32 host API implementation for the Windows MultiMedia Extensions (WMME) audio API.
*/

/*
    How it works:

    For both callback and blocking read/write streams we open the MME devices
    in CALLBACK_EVENT mode. In this mode, MME signals an Event object whenever
    it has finished with a buffer (either filled it for input, or played it
    for output). Where necessary, we block waiting for Event objects using
    WaitMultipleObjects().

    When implementing a PA callback stream, we set up a high priority thread
    which waits on the MME buffer Events and drains/fills the buffers when
    they are ready.

    When implementing a PA blocking read/write stream, we simply wait on these
    Events (when necessary) inside the ReadStream() and WriteStream() functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <mmsystem.h>
#ifndef UNDER_CE
#include <process.h>
#endif
#include <assert.h>
/* PLB20010422 - "memory.h" doesn't work on CodeWarrior for PC. Thanks Mike Berry for the mod. */
#ifndef __MWERKS__
#include <malloc.h>
#include <memory.h>
#endif /* __MWERKS__ */

#include "portaudio.h"
#include "pa_trace.h"
#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_debugprint.h"

#include "pa_win_wmme.h"
#include "pa_win_waveformat.h"

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
#include "pa_win_wdmks_utils.h"
#ifndef DRV_QUERYDEVICEINTERFACE
#define DRV_QUERYDEVICEINTERFACE     (DRV_RESERVED + 12)
#endif
#ifndef DRV_QUERYDEVICEINTERFACESIZE
#define DRV_QUERYDEVICEINTERFACESIZE (DRV_RESERVED + 13)
#endif
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

/* use CreateThread for CYGWIN, _beginthreadex for all others */
#if !defined(__CYGWIN__) && !defined(_WIN32_WCE)
#define CREATE_THREAD (HANDLE)_beginthreadex( 0, 0, ProcessingThreadProc, stream, 0, &stream->processingThreadId )
#define PA_THREAD_FUNC static unsigned WINAPI
#define PA_THREAD_ID unsigned
#else
#define CREATE_THREAD CreateThread( 0, 0, ProcessingThreadProc, stream, 0, &stream->processingThreadId )
#define PA_THREAD_FUNC static DWORD WINAPI
#define PA_THREAD_ID DWORD
#endif
#if (defined(_WIN32_WCE))
#pragma comment(lib, "Coredll.lib")
#elif (defined(WIN32) && (defined(_MSC_VER) && (_MSC_VER >= 1200))) /* MSC version 6 and above */
#pragma comment(lib, "winmm.lib")
#endif

/*
 provided in newer platform sdks
 */
#ifndef DWORD_PTR
    #if defined(_WIN64)
        #define DWORD_PTR unsigned __int64
    #else
        #define DWORD_PTR unsigned long
    #endif
#endif

/************************************************* Constants ********/

#define PA_MME_USE_HIGH_DEFAULT_LATENCY_    (0)  /* For debugging glitches. */

#if PA_MME_USE_HIGH_DEFAULT_LATENCY_
 #define PA_MME_WIN_9X_DEFAULT_LATENCY_                             (0.4)
 #define PA_MME_MIN_HOST_OUTPUT_BUFFER_COUNT_                       (4)
 #define PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_FULL_DUPLEX_	        (4)
 #define PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_HALF_DUPLEX_	        (4)
 #define PA_MME_HOST_BUFFER_GRANULARITY_FRAMES_WHEN_UNSPECIFIED_	(16)
 #define PA_MME_MAX_HOST_BUFFER_SECS_				                (0.3)       /* Do not exceed unless user buffer exceeds */
 #define PA_MME_MAX_HOST_BUFFER_BYTES_				                (32 * 1024) /* Has precedence over PA_MME_MAX_HOST_BUFFER_SECS_, some drivers are known to crash with buffer sizes > 32k */
#else
 #define PA_MME_WIN_9X_DEFAULT_LATENCY_                             (0.2)
 #define PA_MME_MIN_HOST_OUTPUT_BUFFER_COUNT_                       (2)
 #define PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_FULL_DUPLEX_	        (3)         /* always use at least 3 input buffers for full duplex */
 #define PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_HALF_DUPLEX_	        (2)
 #define PA_MME_HOST_BUFFER_GRANULARITY_FRAMES_WHEN_UNSPECIFIED_	(16)
 #define PA_MME_MAX_HOST_BUFFER_SECS_				                (0.1)       /* Do not exceed unless user buffer exceeds */
 #define PA_MME_MAX_HOST_BUFFER_BYTES_				                (32 * 1024) /* Has precedence over PA_MME_MAX_HOST_BUFFER_SECS_, some drivers are known to crash with buffer sizes > 32k */
#endif

/* Use higher latency for NT because it is even worse at real-time
   operation than Win9x.
*/
#define PA_MME_WIN_NT_DEFAULT_LATENCY_                              (0.4)

/* Default low latency for WDM based systems. This is based on a rough
   survey of workable latency settings using patest_wmme_find_best_latency_params.c.
   See pdf attached to ticket 185 for a graph of the survey results:
   http://www.portaudio.com/trac/ticket/185
   
   Workable latencies varied between 40ms and ~80ms on different systems (different
   combinations of hardware, 32 and 64 bit, WinXP, Vista and Win7. We didn't
   get enough Vista results to know if Vista has systemically worse latency.
   For now we choose a safe value across all Windows versions here.
*/
#define PA_MME_WIN_WDM_DEFAULT_LATENCY_                             (0.090)


/* When client suggestedLatency could result in many host buffers, we aim to have around 8, 
   based off Windows documentation that suggests that the kmixer uses 8 buffers. This choice
   is somewhat arbitrary here, since we havn't observed significant stability degredation 
   with using either more, or less buffers.     
*/
#define PA_MME_TARGET_HOST_BUFFER_COUNT_    8

#define PA_MME_MIN_TIMEOUT_MSEC_        (1000)

static const char constInputMapperSuffix_[] = " - Input";
static const char constOutputMapperSuffix_[] = " - Output";

/*
copies TCHAR string to explicit char string
*/
char *StrTCpyToC(char *to, const TCHAR *from)
{
#if !defined(_UNICODE) && !defined(UNICODE)
	return strcpy(to, from);
#else
	int count = wcslen(from);
	if (count != 0)
		if (WideCharToMultiByte(CP_ACP, 0, from, count, to, count, NULL, NULL) == 0)
			return NULL;
	return to;
#endif
}

/*
returns length of TCHAR string
*/
size_t StrTLen(const TCHAR *str)
{
#if !defined(_UNICODE) && !defined(UNICODE)
	return strlen(str);
#else
	return wcslen(str);	
#endif
}

/********************************************************************/

typedef struct PaWinMmeStream PaWinMmeStream;     /* forward declaration */

/* prototypes for functions declared in this file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

PaError PaWinMme_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */

static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** stream,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );
static PaError CloseStream( PaStream* stream );
static PaError StartStream( PaStream *stream );
static PaError StopStream( PaStream *stream );
static PaError AbortStream( PaStream *stream );
static PaError IsStreamStopped( PaStream *s );
static PaError IsStreamActive( PaStream *stream );
static PaTime GetStreamTime( PaStream *stream );
static double GetStreamCpuLoad( PaStream* stream );
static PaError ReadStream( PaStream* stream, void *buffer, unsigned long frames );
static PaError WriteStream( PaStream* stream, const void *buffer, unsigned long frames );
static signed long GetStreamReadAvailable( PaStream* stream );
static signed long GetStreamWriteAvailable( PaStream* stream );


/* macros for setting last host error information */

#ifdef UNICODE

#define PA_MME_SET_LAST_WAVEIN_ERROR( mmresult ) \
    {                                                                   \
        wchar_t mmeErrorTextWide[ MAXERRORLENGTH ];                     \
        char mmeErrorText[ MAXERRORLENGTH ];                            \
        waveInGetErrorText( mmresult, mmeErrorTextWide, MAXERRORLENGTH );   \
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,\
            mmeErrorTextWide, -1, mmeErrorText, MAXERRORLENGTH, NULL, NULL );  \
        PaUtil_SetLastHostErrorInfo( paMME, mmresult, mmeErrorText );   \
    }

#define PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult ) \
    {                                                                   \
        wchar_t mmeErrorTextWide[ MAXERRORLENGTH ];                     \
        char mmeErrorText[ MAXERRORLENGTH ];                            \
        waveOutGetErrorText( mmresult, mmeErrorTextWide, MAXERRORLENGTH );  \
        WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,\
            mmeErrorTextWide, -1, mmeErrorText, MAXERRORLENGTH, NULL, NULL );  \
        PaUtil_SetLastHostErrorInfo( paMME, mmresult, mmeErrorText );   \
    }
    
#else /* !UNICODE */

#define PA_MME_SET_LAST_WAVEIN_ERROR( mmresult ) \
    {                                                                   \
        char mmeErrorText[ MAXERRORLENGTH ];                            \
        waveInGetErrorText( mmresult, mmeErrorText, MAXERRORLENGTH );   \
        PaUtil_SetLastHostErrorInfo( paMME, mmresult, mmeErrorText );   \
    }

#define PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult ) \
    {                                                                   \
        char mmeErrorText[ MAXERRORLENGTH ];                            \
        waveOutGetErrorText( mmresult, mmeErrorText, MAXERRORLENGTH );  \
        PaUtil_SetLastHostErrorInfo( paMME, mmresult, mmeErrorText );   \
    }

#endif /* UNICODE */


static void PaMme_SetLastSystemError( DWORD errorCode )
{
    char *lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
    );
    PaUtil_SetLastHostErrorInfo( paMME, errorCode, lpMsgBuf );
    LocalFree( lpMsgBuf );
}

#define PA_MME_SET_LAST_SYSTEM_ERROR( errorCode ) \
    PaMme_SetLastSystemError( errorCode )


/* PaError returning wrappers for some commonly used win32 functions
    note that we allow passing a null ptr to have no effect.
*/

static PaError CreateEventWithPaError( HANDLE *handle,
        LPSECURITY_ATTRIBUTES lpEventAttributes,
        BOOL bManualReset,
        BOOL bInitialState,
        LPCTSTR lpName )
{
    PaError result = paNoError;

    *handle = NULL;
    
    *handle = CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName );
    if( *handle == NULL )
    {
        result = paUnanticipatedHostError;
        PA_MME_SET_LAST_SYSTEM_ERROR( GetLastError() );
    }

    return result;
}


static PaError ResetEventWithPaError( HANDLE handle )
{
    PaError result = paNoError;

    if( handle )
    {
        if( ResetEvent( handle ) == 0 )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_SYSTEM_ERROR( GetLastError() );
        }
    }

    return result;
}


static PaError CloseHandleWithPaError( HANDLE handle )
{
    PaError result = paNoError;
    
    if( handle )
    {
        if( CloseHandle( handle ) == 0 )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_SYSTEM_ERROR( GetLastError() );
        }
    }
    
    return result;
}


/* PaWinMmeHostApiRepresentation - host api datastructure specific to this implementation */

typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;
    
    int inputDeviceCount, outputDeviceCount;

    /** winMmeDeviceIds is an array of WinMme device ids.
        fields in the range [0, inputDeviceCount) are input device ids,
        and [inputDeviceCount, inputDeviceCount + outputDeviceCount) are output
        device ids.
     */ 
    UINT *winMmeDeviceIds;
}
PaWinMmeHostApiRepresentation;


typedef struct
{
    PaDeviceInfo inheritedDeviceInfo;
    DWORD dwFormats; /**<< standard formats bitmask from the WAVEINCAPS and WAVEOUTCAPS structures */
    char deviceInputChannelCountIsKnown; /**<< if the system returns 0xFFFF then we don't really know the number of supported channels (1=>known, 0=>unknown)*/
    char deviceOutputChannelCountIsKnown; /**<< if the system returns 0xFFFF then we don't really know the number of supported channels (1=>known, 0=>unknown)*/
}
PaWinMmeDeviceInfo;


/*************************************************************************
 * Returns recommended device ID.
 * On the PC, the recommended device can be specified by the user by
 * setting an environment variable. For example, to use device #1.
 *
 *    set PA_RECOMMENDED_OUTPUT_DEVICE=1
 *
 * The user should first determine the available device ID by using
 * the supplied application "pa_devs".
 */
#define PA_ENV_BUF_SIZE_  (32)
#define PA_REC_IN_DEV_ENV_NAME_  ("PA_RECOMMENDED_INPUT_DEVICE")
#define PA_REC_OUT_DEV_ENV_NAME_  ("PA_RECOMMENDED_OUTPUT_DEVICE")
static PaDeviceIndex GetEnvDefaultDeviceID( char *envName )
{
    PaDeviceIndex recommendedIndex = paNoDevice;
    DWORD   hresult;
    char    envbuf[PA_ENV_BUF_SIZE_];

#ifndef WIN32_PLATFORM_PSPC /* no GetEnvironmentVariable on PocketPC */

    /* Let user determine default device by setting environment variable. */
    hresult = GetEnvironmentVariableA( envName, envbuf, PA_ENV_BUF_SIZE_ );
    if( (hresult > 0) && (hresult < PA_ENV_BUF_SIZE_) )
    {
        recommendedIndex = atoi( envbuf );
    }
#endif

    return recommendedIndex;
}


static void InitializeDefaultDeviceIdsFromEnv( PaWinMmeHostApiRepresentation *hostApi )
{
    PaDeviceIndex device;

    /* input */
    device = GetEnvDefaultDeviceID( PA_REC_IN_DEV_ENV_NAME_ );
    if( device != paNoDevice &&
            ( device >= 0 && device < hostApi->inheritedHostApiRep.info.deviceCount ) &&
            hostApi->inheritedHostApiRep.deviceInfos[ device ]->maxInputChannels > 0 )
    {
        hostApi->inheritedHostApiRep.info.defaultInputDevice = device;
    }

    /* output */
    device = GetEnvDefaultDeviceID( PA_REC_OUT_DEV_ENV_NAME_ );
    if( device != paNoDevice &&
            ( device >= 0 && device < hostApi->inheritedHostApiRep.info.deviceCount ) &&
            hostApi->inheritedHostApiRep.deviceInfos[ device ]->maxOutputChannels > 0 )
    {
        hostApi->inheritedHostApiRep.info.defaultOutputDevice = device;
    }
}


/** Convert external PA ID to a windows multimedia device ID
*/
static UINT LocalDeviceIndexToWinMmeDeviceId( PaWinMmeHostApiRepresentation *hostApi, PaDeviceIndex device )
{
    assert( device >= 0 && device < hostApi->inputDeviceCount + hostApi->outputDeviceCount );

	return hostApi->winMmeDeviceIds[ device ];
}


static int SampleFormatAndWinWmmeSpecificFlagsToLinearWaveFormatTag( PaSampleFormat sampleFormat, unsigned long winMmeSpecificFlags )
{
    int waveFormatTag = 0;

    if( winMmeSpecificFlags & paWinMmeWaveFormatDolbyAc3Spdif )
        waveFormatTag = PAWIN_WAVE_FORMAT_DOLBY_AC3_SPDIF;
    else if( winMmeSpecificFlags & paWinMmeWaveFormatWmaSpdif )
        waveFormatTag = PAWIN_WAVE_FORMAT_WMA_SPDIF;
    else
        waveFormatTag = PaWin_SampleFormatToLinearWaveFormatTag( sampleFormat );

    return waveFormatTag;
}


static PaError QueryInputWaveFormatEx( int deviceId, WAVEFORMATEX *waveFormatEx )
{
    MMRESULT mmresult;
    
    switch( mmresult = waveInOpen( NULL, deviceId, waveFormatEx, 0, 0, WAVE_FORMAT_QUERY ) )
    {
        case MMSYSERR_NOERROR:
            return paNoError;
        case MMSYSERR_ALLOCATED:    /* Specified resource is already allocated. */
            return paDeviceUnavailable;
        case MMSYSERR_NODRIVER:	    /* No device driver is present. */
            return paDeviceUnavailable;
        case MMSYSERR_NOMEM:	    /* Unable to allocate or lock memory. */
            return paInsufficientMemory;
        case WAVERR_BADFORMAT:      /* Attempted to open with an unsupported waveform-audio format. */
            return paSampleFormatNotSupported;
                    
        case MMSYSERR_BADDEVICEID:	/* Specified device identifier is out of range. */
            /* falls through */
        default:
            PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
            return paUnanticipatedHostError;
    }
}


static PaError QueryOutputWaveFormatEx( int deviceId, WAVEFORMATEX *waveFormatEx )
{
    MMRESULT mmresult;
    
    switch( mmresult = waveOutOpen( NULL, deviceId, waveFormatEx, 0, 0, WAVE_FORMAT_QUERY ) )
    {
        case MMSYSERR_NOERROR:
            return paNoError;
        case MMSYSERR_ALLOCATED:    /* Specified resource is already allocated. */
            return paDeviceUnavailable;
        case MMSYSERR_NODRIVER:	    /* No device driver is present. */
            return paDeviceUnavailable;
        case MMSYSERR_NOMEM:	    /* Unable to allocate or lock memory. */
            return paInsufficientMemory;
        case WAVERR_BADFORMAT:      /* Attempted to open with an unsupported waveform-audio format. */
            return paSampleFormatNotSupported;
                    
        case MMSYSERR_BADDEVICEID:	/* Specified device identifier is out of range. */
            /* falls through */
        default:
            PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
            return paUnanticipatedHostError;
    }
}


static PaError QueryFormatSupported( PaDeviceInfo *deviceInfo,
        PaError (*waveFormatExQueryFunction)(int, WAVEFORMATEX*),
        int winMmeDeviceId, int channels, double sampleRate, unsigned long winMmeSpecificFlags )
{
    PaWinMmeDeviceInfo *winMmeDeviceInfo = (PaWinMmeDeviceInfo*)deviceInfo;
    PaWinWaveFormat waveFormat;
    PaSampleFormat sampleFormat;
    int waveFormatTag;
    
    /* @todo at the moment we only query with 16 bit sample format and directout speaker config*/

    sampleFormat = paInt16;
    waveFormatTag = SampleFormatAndWinWmmeSpecificFlagsToLinearWaveFormatTag( sampleFormat, winMmeSpecificFlags );

    if( waveFormatTag == PaWin_SampleFormatToLinearWaveFormatTag( paInt16 ) ){
    
        /* attempt bypass querying the device for linear formats */

        if( sampleRate == 11025.0
            && ( (channels == 1 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_1M16))
                || (channels == 2 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_1S16)) ) ){

            return paNoError;
        }

        if( sampleRate == 22050.0
            && ( (channels == 1 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_2M16))
                || (channels == 2 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_2S16)) ) ){

            return paNoError;
        }

        if( sampleRate == 44100.0
            && ( (channels == 1 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_4M16))
                || (channels == 2 && (winMmeDeviceInfo->dwFormats & WAVE_FORMAT_4S16)) ) ){

            return paNoError;
        }
    }


    /* first, attempt to query the device using WAVEFORMATEXTENSIBLE, 
       if this fails we fall back to WAVEFORMATEX */

    PaWin_InitializeWaveFormatExtensible( &waveFormat, channels, sampleFormat, waveFormatTag,
            sampleRate, PAWIN_SPEAKER_DIRECTOUT );

    if( waveFormatExQueryFunction( winMmeDeviceId, (WAVEFORMATEX*)&waveFormat ) == paNoError )
        return paNoError;

    PaWin_InitializeWaveFormatEx( &waveFormat, channels, sampleFormat, waveFormatTag, sampleRate );

    return waveFormatExQueryFunction( winMmeDeviceId, (WAVEFORMATEX*)&waveFormat );
}


#define PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_  (13) /* must match array length below */
static double defaultSampleRateSearchOrder_[] =
    { 44100.0, 48000.0, 32000.0, 24000.0, 22050.0, 88200.0, 96000.0, 192000.0,
        16000.0, 12000.0, 11025.0, 9600.0, 8000.0 };

static void DetectDefaultSampleRate( PaWinMmeDeviceInfo *winMmeDeviceInfo, int winMmeDeviceId,
        PaError (*waveFormatExQueryFunction)(int, WAVEFORMATEX*), int maxChannels )
{
    PaDeviceInfo *deviceInfo = &winMmeDeviceInfo->inheritedDeviceInfo;
    int i;
    
    deviceInfo->defaultSampleRate = 0.;

    for( i=0; i < PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_; ++i )
    {
        double sampleRate = defaultSampleRateSearchOrder_[ i ]; 
        PaError paerror = QueryFormatSupported( deviceInfo, waveFormatExQueryFunction, winMmeDeviceId, maxChannels, sampleRate, 0 );
        if( paerror == paNoError )
        {
            deviceInfo->defaultSampleRate = sampleRate;
            break;
        }
    }
}


#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
static int QueryWaveInKSFilterMaxChannels( int waveInDeviceId, int *maxChannels )
{
    void *devicePath;
    DWORD devicePathSize;
    int result = 0;

    if( waveInMessage((HWAVEIN)waveInDeviceId, DRV_QUERYDEVICEINTERFACESIZE,
            (DWORD_PTR)&devicePathSize, 0 ) != MMSYSERR_NOERROR )
        return 0;

    devicePath = PaUtil_AllocateMemory( devicePathSize );
    if( !devicePath )
        return 0;

    /* apparently DRV_QUERYDEVICEINTERFACE returns a unicode interface path, although this is undocumented */
    if( waveInMessage((HWAVEIN)waveInDeviceId, DRV_QUERYDEVICEINTERFACE,
            (DWORD_PTR)devicePath, devicePathSize ) == MMSYSERR_NOERROR )
    {
        int count = PaWin_WDMKS_QueryFilterMaximumChannelCount( devicePath, /* isInput= */ 1  );
        if( count > 0 )
        {
            *maxChannels = count;
            result = 1;
        }
    }

    PaUtil_FreeMemory( devicePath );

    return result;
}
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */


static PaError InitializeInputDeviceInfo( PaWinMmeHostApiRepresentation *winMmeHostApi,
        PaWinMmeDeviceInfo *winMmeDeviceInfo, UINT winMmeInputDeviceId, int *success )
{
    PaError result = paNoError;
    char *deviceName; /* non-const ptr */
    MMRESULT mmresult;
    WAVEINCAPS wic;
    PaDeviceInfo *deviceInfo = &winMmeDeviceInfo->inheritedDeviceInfo;
    
    *success = 0;

    mmresult = waveInGetDevCaps( winMmeInputDeviceId, &wic, sizeof( WAVEINCAPS ) );
    if( mmresult == MMSYSERR_NOMEM )
    {
        result = paInsufficientMemory;
        goto error;
    }
    else if( mmresult != MMSYSERR_NOERROR )
    {
        /* instead of returning paUnanticipatedHostError we return
            paNoError, but leave success set as 0. This allows
            Pa_Initialize to just ignore this device, without failing
            the entire initialisation process.
        */
        return paNoError;
    }           

    if( winMmeInputDeviceId == WAVE_MAPPER )
    {
        /* Append I/O suffix to WAVE_MAPPER device. */
        deviceName = (char *)PaUtil_GroupAllocateMemory(
                    winMmeHostApi->allocations,
					(long) (StrTLen( wic.szPname ) + 1 + sizeof(constInputMapperSuffix_)) );
        if( !deviceName )
        {
            result = paInsufficientMemory;
            goto error;
        }
        StrTCpyToC( deviceName, wic.szPname );
        strcat( deviceName, constInputMapperSuffix_ );
    }
    else
    {
        deviceName = (char*)PaUtil_GroupAllocateMemory(
                    winMmeHostApi->allocations, 
					(long) (StrTLen( wic.szPname ) + 1) );
        if( !deviceName )
        {
            result = paInsufficientMemory;
            goto error;
        }
        StrTCpyToC( deviceName, wic.szPname  );
    }
    deviceInfo->name = deviceName;

    if( wic.wChannels == 0xFFFF || wic.wChannels < 1 || wic.wChannels > 255 ){
        /* For Windows versions using WDM (possibly Windows 98 ME and later)
         * the kernel mixer sits between the application and the driver. As a result,
         * wave*GetDevCaps often kernel mixer channel counts, which are unlimited.
         * When this happens we assume the device is stereo and set a flag
         * so that other channel counts can be tried with OpenStream -- i.e. when
         * device*ChannelCountIsKnown is false, OpenStream will try whatever
         * channel count you supply.
         * see also InitializeOutputDeviceInfo() below.
     */

        PA_DEBUG(("Pa_GetDeviceInfo: Num input channels reported as %d! Changed to 2.\n", wic.wChannels ));
        deviceInfo->maxInputChannels = 2;
        winMmeDeviceInfo->deviceInputChannelCountIsKnown = 0;
    }else{
        deviceInfo->maxInputChannels = wic.wChannels;
        winMmeDeviceInfo->deviceInputChannelCountIsKnown = 1;
    }

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
    winMmeDeviceInfo->deviceInputChannelCountIsKnown = 
            QueryWaveInKSFilterMaxChannels( winMmeInputDeviceId, &deviceInfo->maxInputChannels );
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

    winMmeDeviceInfo->dwFormats = wic.dwFormats;

    DetectDefaultSampleRate( winMmeDeviceInfo, winMmeInputDeviceId,
            QueryInputWaveFormatEx, deviceInfo->maxInputChannels );

    *success = 1;
    
error:
    return result;
}


#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
static int QueryWaveOutKSFilterMaxChannels( int waveOutDeviceId, int *maxChannels )
{
    void *devicePath;
    DWORD devicePathSize;
    int result = 0;

    if( waveOutMessage((HWAVEOUT)waveOutDeviceId, DRV_QUERYDEVICEINTERFACESIZE,
            (DWORD_PTR)&devicePathSize, 0 ) != MMSYSERR_NOERROR )
        return 0;

    devicePath = PaUtil_AllocateMemory( devicePathSize );
    if( !devicePath )
        return 0;

    /* apparently DRV_QUERYDEVICEINTERFACE returns a unicode interface path, although this is undocumented */
    if( waveOutMessage((HWAVEOUT)waveOutDeviceId, DRV_QUERYDEVICEINTERFACE,
            (DWORD_PTR)devicePath, devicePathSize ) == MMSYSERR_NOERROR )
    {
        int count = PaWin_WDMKS_QueryFilterMaximumChannelCount( devicePath, /* isInput= */ 0  );
        if( count > 0 )
        {
            *maxChannels = count;
            result = 1;
        }
    }

    PaUtil_FreeMemory( devicePath );

    return result;
}
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */


static PaError InitializeOutputDeviceInfo( PaWinMmeHostApiRepresentation *winMmeHostApi,
        PaWinMmeDeviceInfo *winMmeDeviceInfo, UINT winMmeOutputDeviceId, int *success )
{
    PaError result = paNoError;
    char *deviceName; /* non-const ptr */
    MMRESULT mmresult;
    WAVEOUTCAPS woc;
    PaDeviceInfo *deviceInfo = &winMmeDeviceInfo->inheritedDeviceInfo;
#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
    int wdmksDeviceOutputChannelCountIsKnown;
#endif

    *success = 0;

    mmresult = waveOutGetDevCaps( winMmeOutputDeviceId, &woc, sizeof( WAVEOUTCAPS ) );
    if( mmresult == MMSYSERR_NOMEM )
    {
        result = paInsufficientMemory;
        goto error;
    }
    else if( mmresult != MMSYSERR_NOERROR )
    {
        /* instead of returning paUnanticipatedHostError we return
            paNoError, but leave success set as 0. This allows
            Pa_Initialize to just ignore this device, without failing
            the entire initialisation process.
        */
        return paNoError;
    }

    if( winMmeOutputDeviceId == WAVE_MAPPER )
    {
        /* Append I/O suffix to WAVE_MAPPER device. */
        deviceName = (char *)PaUtil_GroupAllocateMemory(
                    winMmeHostApi->allocations, 
					(long) (StrTLen( woc.szPname ) + 1 + sizeof(constOutputMapperSuffix_)) );
        if( !deviceName )
        {
            result = paInsufficientMemory;
            goto error;
        }
        StrTCpyToC( deviceName, woc.szPname );
        strcat( deviceName, constOutputMapperSuffix_ );
    }
    else
    {
        deviceName = (char*)PaUtil_GroupAllocateMemory(
                    winMmeHostApi->allocations, 
					(long) (StrTLen( woc.szPname ) + 1) );
        if( !deviceName )
        {
            result = paInsufficientMemory;
            goto error;
        }
        StrTCpyToC( deviceName, woc.szPname  );
    }
    deviceInfo->name = deviceName;

    if( woc.wChannels == 0xFFFF || woc.wChannels < 1 || woc.wChannels > 255 ){
        /* For Windows versions using WDM (possibly Windows 98 ME and later)
         * the kernel mixer sits between the application and the driver. As a result,
         * wave*GetDevCaps often kernel mixer channel counts, which are unlimited.
         * When this happens we assume the device is stereo and set a flag
         * so that other channel counts can be tried with OpenStream -- i.e. when
         * device*ChannelCountIsKnown is false, OpenStream will try whatever
         * channel count you supply.
         * see also InitializeInputDeviceInfo() above.
     */

        PA_DEBUG(("Pa_GetDeviceInfo: Num output channels reported as %d! Changed to 2.\n", woc.wChannels ));
        deviceInfo->maxOutputChannels = 2;
        winMmeDeviceInfo->deviceOutputChannelCountIsKnown = 0;
    }else{
        deviceInfo->maxOutputChannels = woc.wChannels;
        winMmeDeviceInfo->deviceOutputChannelCountIsKnown = 1;
    }

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
    wdmksDeviceOutputChannelCountIsKnown = QueryWaveOutKSFilterMaxChannels( 
			winMmeOutputDeviceId, &deviceInfo->maxOutputChannels );
    if( wdmksDeviceOutputChannelCountIsKnown && !winMmeDeviceInfo->deviceOutputChannelCountIsKnown )
        winMmeDeviceInfo->deviceOutputChannelCountIsKnown = 1;
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

    winMmeDeviceInfo->dwFormats = woc.dwFormats;

    DetectDefaultSampleRate( winMmeDeviceInfo, winMmeOutputDeviceId,
            QueryOutputWaveFormatEx, deviceInfo->maxOutputChannels );

    *success = 1;
    
error:
    return result;
}


static void GetDefaultLatencies( PaTime *defaultLowLatency, PaTime *defaultHighLatency )
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof( osvi );
	GetVersionEx( &osvi );

    /* Check for NT */
    if( (osvi.dwMajorVersion == 4) && (osvi.dwPlatformId == 2) )
    {
        *defaultLowLatency = PA_MME_WIN_NT_DEFAULT_LATENCY_;
    }
    else if(osvi.dwMajorVersion >= 5)
    {
        *defaultLowLatency  = PA_MME_WIN_WDM_DEFAULT_LATENCY_;
    }
    else
    {
        *defaultLowLatency  = PA_MME_WIN_9X_DEFAULT_LATENCY_;
    }     

    *defaultHighLatency = *defaultLowLatency * 2;
}


PaError PaWinMme_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    int i;
    PaWinMmeHostApiRepresentation *winMmeHostApi;
    int inputDeviceCount, outputDeviceCount, maximumPossibleDeviceCount;
    PaWinMmeDeviceInfo *deviceInfoArray;
    int deviceInfoInitializationSucceeded;
    PaTime defaultLowLatency, defaultHighLatency;
    DWORD waveInPreferredDevice, waveOutPreferredDevice;
    DWORD preferredDeviceStatusFlags;

    winMmeHostApi = (PaWinMmeHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaWinMmeHostApiRepresentation) );
    if( !winMmeHostApi )
    {
        result = paInsufficientMemory;
        goto error;
    }

    winMmeHostApi->allocations = PaUtil_CreateAllocationGroup();
    if( !winMmeHostApi->allocations )
    {
        result = paInsufficientMemory;
        goto error;
    }

    *hostApi = &winMmeHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paMME;
    (*hostApi)->info.name = "MME";

    
    /* initialise device counts and default devices under the assumption that
        there are no devices. These values are incremented below if and when
        devices are successfully initialized.
    */
    (*hostApi)->info.deviceCount = 0;
    (*hostApi)->info.defaultInputDevice = paNoDevice;
    (*hostApi)->info.defaultOutputDevice = paNoDevice;
    winMmeHostApi->inputDeviceCount = 0;
    winMmeHostApi->outputDeviceCount = 0;

#if !defined(DRVM_MAPPER_PREFERRED_GET)
/* DRVM_MAPPER_PREFERRED_GET is defined in mmddk.h but we avoid a dependency on the DDK by defining it here */
#define DRVM_MAPPER_PREFERRED_GET    (0x2000+21)
#endif

    /* the following calls assume that if wave*Message fails the preferred device parameter won't be modified */
    preferredDeviceStatusFlags = 0;
    waveInPreferredDevice = -1;
    waveInMessage( (HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&waveInPreferredDevice, (DWORD_PTR)&preferredDeviceStatusFlags );

    preferredDeviceStatusFlags = 0;
    waveOutPreferredDevice = -1;
    waveOutMessage( (HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&waveOutPreferredDevice, (DWORD_PTR)&preferredDeviceStatusFlags );

    maximumPossibleDeviceCount = 0;

    inputDeviceCount = waveInGetNumDevs();
    if( inputDeviceCount > 0 )
    	maximumPossibleDeviceCount += inputDeviceCount + 1;	/* assume there is a WAVE_MAPPER */

    outputDeviceCount = waveOutGetNumDevs();
    if( outputDeviceCount > 0 )
	    maximumPossibleDeviceCount += outputDeviceCount + 1;	/* assume there is a WAVE_MAPPER */


    if( maximumPossibleDeviceCount > 0 ){

        (*hostApi)->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
                winMmeHostApi->allocations, sizeof(PaDeviceInfo*) * maximumPossibleDeviceCount );
        if( !(*hostApi)->deviceInfos )
        {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaWinMmeDeviceInfo*)PaUtil_GroupAllocateMemory(
                winMmeHostApi->allocations, sizeof(PaWinMmeDeviceInfo) * maximumPossibleDeviceCount );
        if( !deviceInfoArray )
        {
            result = paInsufficientMemory;
            goto error;
        }

        winMmeHostApi->winMmeDeviceIds = (UINT*)PaUtil_GroupAllocateMemory(
                winMmeHostApi->allocations, sizeof(int) * maximumPossibleDeviceCount );
        if( !winMmeHostApi->winMmeDeviceIds )
        {
            result = paInsufficientMemory;
            goto error;
        }

        GetDefaultLatencies( &defaultLowLatency, &defaultHighLatency );

        if( inputDeviceCount > 0 ){
            /* -1 is the WAVE_MAPPER */
            for( i = -1; i < inputDeviceCount; ++i ){
                UINT winMmeDeviceId = (UINT)((i==-1) ? WAVE_MAPPER : i);
                PaWinMmeDeviceInfo *wmmeDeviceInfo = &deviceInfoArray[ (*hostApi)->info.deviceCount ];
                PaDeviceInfo *deviceInfo = &wmmeDeviceInfo->inheritedDeviceInfo;
                deviceInfo->structVersion = 2;
                deviceInfo->hostApi = hostApiIndex;

                deviceInfo->maxInputChannels = 0;
                wmmeDeviceInfo->deviceInputChannelCountIsKnown = 1;
                deviceInfo->maxOutputChannels = 0;
                wmmeDeviceInfo->deviceOutputChannelCountIsKnown = 1;

                deviceInfo->defaultLowInputLatency = defaultLowLatency;
                deviceInfo->defaultLowOutputLatency = defaultLowLatency;
                deviceInfo->defaultHighInputLatency = defaultHighLatency;
                deviceInfo->defaultHighOutputLatency = defaultHighLatency;

                result = InitializeInputDeviceInfo( winMmeHostApi, wmmeDeviceInfo,
                        winMmeDeviceId, &deviceInfoInitializationSucceeded );
                if( result != paNoError )
                    goto error;

                if( deviceInfoInitializationSucceeded ){
                    if( (*hostApi)->info.defaultInputDevice == paNoDevice ){
                        /* if there is currently no default device, use the first one available */
                        (*hostApi)->info.defaultInputDevice = (*hostApi)->info.deviceCount;
                    
                    }else if( winMmeDeviceId == waveInPreferredDevice ){
                        /* set the default device to the system preferred device */
                        (*hostApi)->info.defaultInputDevice = (*hostApi)->info.deviceCount;
                    }

                    winMmeHostApi->winMmeDeviceIds[ (*hostApi)->info.deviceCount ] = winMmeDeviceId;
                    (*hostApi)->deviceInfos[ (*hostApi)->info.deviceCount ] = deviceInfo;

                    winMmeHostApi->inputDeviceCount++;
                    (*hostApi)->info.deviceCount++;
                }
            }
        }

        if( outputDeviceCount > 0 ){
            /* -1 is the WAVE_MAPPER */
            for( i = -1; i < outputDeviceCount; ++i ){
                UINT winMmeDeviceId = (UINT)((i==-1) ? WAVE_MAPPER : i);
                PaWinMmeDeviceInfo *wmmeDeviceInfo = &deviceInfoArray[ (*hostApi)->info.deviceCount ];
                PaDeviceInfo *deviceInfo = &wmmeDeviceInfo->inheritedDeviceInfo;
                deviceInfo->structVersion = 2;
                deviceInfo->hostApi = hostApiIndex;

                deviceInfo->maxInputChannels = 0;
                wmmeDeviceInfo->deviceInputChannelCountIsKnown = 1;
                deviceInfo->maxOutputChannels = 0;
                wmmeDeviceInfo->deviceOutputChannelCountIsKnown = 1;

                deviceInfo->defaultLowInputLatency = defaultLowLatency;
                deviceInfo->defaultLowOutputLatency = defaultLowLatency;
                deviceInfo->defaultHighInputLatency = defaultHighLatency;
                deviceInfo->defaultHighOutputLatency = defaultHighLatency; 

                result = InitializeOutputDeviceInfo( winMmeHostApi, wmmeDeviceInfo,
                        winMmeDeviceId, &deviceInfoInitializationSucceeded );
                if( result != paNoError )
                    goto error;

                if( deviceInfoInitializationSucceeded ){
                    if( (*hostApi)->info.defaultOutputDevice == paNoDevice ){
                        /* if there is currently no default device, use the first one available */
                        (*hostApi)->info.defaultOutputDevice = (*hostApi)->info.deviceCount;

                    }else if( winMmeDeviceId == waveOutPreferredDevice ){
                        /* set the default device to the system preferred device */
                        (*hostApi)->info.defaultOutputDevice = (*hostApi)->info.deviceCount;
                    }

                    winMmeHostApi->winMmeDeviceIds[ (*hostApi)->info.deviceCount ] = winMmeDeviceId;
                    (*hostApi)->deviceInfos[ (*hostApi)->info.deviceCount ] = deviceInfo;

                    winMmeHostApi->outputDeviceCount++;
                    (*hostApi)->info.deviceCount++;
                }
            }
        }
    }
    
    InitializeDefaultDeviceIdsFromEnv( winMmeHostApi );

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &winMmeHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &winMmeHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    return result;

error:
    if( winMmeHostApi )
    {
        if( winMmeHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( winMmeHostApi->allocations );
            PaUtil_DestroyAllocationGroup( winMmeHostApi->allocations );
        }
        
        PaUtil_FreeMemory( winMmeHostApi );
    }

    return result;
}


static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaWinMmeHostApiRepresentation *winMmeHostApi = (PaWinMmeHostApiRepresentation*)hostApi;

    if( winMmeHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( winMmeHostApi->allocations );
        PaUtil_DestroyAllocationGroup( winMmeHostApi->allocations );
    }

    PaUtil_FreeMemory( winMmeHostApi );
}


static PaError IsInputChannelCountSupported( PaWinMmeDeviceInfo* deviceInfo, int channelCount )
{
    PaError result = paNoError;

    if( channelCount > 0
            && deviceInfo->deviceInputChannelCountIsKnown
            && channelCount > deviceInfo->inheritedDeviceInfo.maxInputChannels ){

        result = paInvalidChannelCount; 
    }

    return result;
}

static PaError IsOutputChannelCountSupported( PaWinMmeDeviceInfo* deviceInfo, int channelCount )
{
    PaError result = paNoError;

    if( channelCount > 0
            && deviceInfo->deviceOutputChannelCountIsKnown
            && channelCount > deviceInfo->inheritedDeviceInfo.maxOutputChannels ){

        result = paInvalidChannelCount; 
    }

    return result;
}

static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaWinMmeHostApiRepresentation *winMmeHostApi = (PaWinMmeHostApiRepresentation*)hostApi;
    PaDeviceInfo *inputDeviceInfo, *outputDeviceInfo;
    int inputChannelCount, outputChannelCount;
    int inputMultipleDeviceChannelCount, outputMultipleDeviceChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaWinMmeStreamInfo *inputStreamInfo, *outputStreamInfo;
    UINT winMmeInputDeviceId, winMmeOutputDeviceId;
    unsigned int i;
    PaError paerror;

    /* The calls to QueryFormatSupported below are intended to detect invalid
        sample rates. If we assume that the channel count and format are OK,
        then the only thing that could fail is the sample rate. This isn't
        strictly true, but I can't think of a better way to test that the
        sample rate is valid.
    */  
    
    if( inputParameters )
    {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
        inputStreamInfo = inputParameters->hostApiSpecificStreamInfo;
        
        /* all standard sample formats are supported by the buffer adapter,
             this implementation doesn't support any custom sample formats */
        if( inputSampleFormat & paCustomFormat )
            return paSampleFormatNotSupported;

        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification
                && inputStreamInfo && (inputStreamInfo->flags & paWinMmeUseMultipleDevices) )
        {
            inputMultipleDeviceChannelCount = 0;
            for( i=0; i< inputStreamInfo->deviceCount; ++i )
            {
                inputMultipleDeviceChannelCount += inputStreamInfo->devices[i].channelCount;
                    
                inputDeviceInfo = hostApi->deviceInfos[ inputStreamInfo->devices[i].device ];

                /* check that input device can support inputChannelCount */
                if( inputStreamInfo->devices[i].channelCount < 1 )
                    return paInvalidChannelCount;

                paerror = IsInputChannelCountSupported( (PaWinMmeDeviceInfo*)inputDeviceInfo, 
                        inputStreamInfo->devices[i].channelCount );
                if( paerror != paNoError )
                    return paerror;

                /* test for valid sample rate, see comment above */
                winMmeInputDeviceId = LocalDeviceIndexToWinMmeDeviceId( winMmeHostApi, inputStreamInfo->devices[i].device );
                paerror = QueryFormatSupported( inputDeviceInfo, QueryInputWaveFormatEx, 
                        winMmeInputDeviceId, inputStreamInfo->devices[i].channelCount, sampleRate, 
                        ((inputStreamInfo) ? inputStreamInfo->flags : 0) );
                if( paerror != paNoError )
                    return paInvalidSampleRate;
            }
                
            if( inputMultipleDeviceChannelCount != inputChannelCount )
                return paIncompatibleHostApiSpecificStreamInfo;                  
        }
        else
        {
            if( inputStreamInfo && (inputStreamInfo->flags & paWinMmeUseMultipleDevices) )
                return paIncompatibleHostApiSpecificStreamInfo; /* paUseHostApiSpecificDeviceSpecification was not supplied as the input device */

            inputDeviceInfo = hostApi->deviceInfos[ inputParameters->device ];

            /* check that input device can support inputChannelCount */
            paerror = IsInputChannelCountSupported( (PaWinMmeDeviceInfo*)inputDeviceInfo, inputChannelCount );
            if( paerror != paNoError )
                return paerror;

            /* test for valid sample rate, see comment above */
            winMmeInputDeviceId = LocalDeviceIndexToWinMmeDeviceId( winMmeHostApi, inputParameters->device );
            paerror = QueryFormatSupported( inputDeviceInfo, QueryInputWaveFormatEx, 
                    winMmeInputDeviceId, inputChannelCount, sampleRate,
                    ((inputStreamInfo) ? inputStreamInfo->flags : 0) );
            if( paerror != paNoError )
                return paInvalidSampleRate;
        }
    }

    if( outputParameters )
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        outputStreamInfo = outputParameters->hostApiSpecificStreamInfo;

        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if( outputSampleFormat & paCustomFormat )
            return paSampleFormatNotSupported;

        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification
                && outputStreamInfo && (outputStreamInfo->flags & paWinMmeUseMultipleDevices) )
        {
            outputMultipleDeviceChannelCount = 0;
            for( i=0; i< outputStreamInfo->deviceCount; ++i )
            {
                outputMultipleDeviceChannelCount += outputStreamInfo->devices[i].channelCount;
                    
                outputDeviceInfo = hostApi->deviceInfos[ outputStreamInfo->devices[i].device ];

                /* check that output device can support outputChannelCount */
                if( outputStreamInfo->devices[i].channelCount < 1 )
                    return paInvalidChannelCount;

                paerror = IsOutputChannelCountSupported( (PaWinMmeDeviceInfo*)outputDeviceInfo, 
                        outputStreamInfo->devices[i].channelCount );
                if( paerror != paNoError )
                    return paerror;

                /* test for valid sample rate, see comment above */
                winMmeOutputDeviceId = LocalDeviceIndexToWinMmeDeviceId( winMmeHostApi, outputStreamInfo->devices[i].device );
                paerror = QueryFormatSupported( outputDeviceInfo, QueryOutputWaveFormatEx, 
                        winMmeOutputDeviceId, outputStreamInfo->devices[i].channelCount, sampleRate,
                        ((outputStreamInfo) ? outputStreamInfo->flags : 0) );
                if( paerror != paNoError )
                    return paInvalidSampleRate;
            }
                
            if( outputMultipleDeviceChannelCount != outputChannelCount )
                return paIncompatibleHostApiSpecificStreamInfo;            
        }
        else
        {
            if( outputStreamInfo && (outputStreamInfo->flags & paWinMmeUseMultipleDevices) )
                return paIncompatibleHostApiSpecificStreamInfo; /* paUseHostApiSpecificDeviceSpecification was not supplied as the output device */

            outputDeviceInfo = hostApi->deviceInfos[ outputParameters->device ];

            /* check that output device can support outputChannelCount */
            paerror = IsOutputChannelCountSupported( (PaWinMmeDeviceInfo*)outputDeviceInfo, outputChannelCount );
            if( paerror != paNoError )
                return paerror;

            /* test for valid sample rate, see comment above */
            winMmeOutputDeviceId = LocalDeviceIndexToWinMmeDeviceId( winMmeHostApi, outputParameters->device );
            paerror = QueryFormatSupported( outputDeviceInfo, QueryOutputWaveFormatEx, 
                    winMmeOutputDeviceId, outputChannelCount, sampleRate,
                    ((outputStreamInfo) ? outputStreamInfo->flags : 0) );
            if( paerror != paNoError )
                return paInvalidSampleRate;
        }
    }
    
    /*
            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported

            - check that the device supports sampleRate

            for mme all we can do is test that the input and output devices
            support the requested sample rate and number of channels. we
            cannot test for full duplex compatibility.
    */                                             

    return paFormatIsSupported;
}


static unsigned long ComputeHostBufferCountForFixedBufferSizeFrames(
        unsigned long suggestedLatencyFrames,
        unsigned long hostBufferSizeFrames,
        unsigned long minimumBufferCount )
{
    /* Calculate the number of buffers of length hostFramesPerBuffer 
       that fit in suggestedLatencyFrames, rounding up to the next integer.

       The value (hostBufferSizeFrames - 1) below is to ensure the buffer count is rounded up.
    */
    unsigned long resultBufferCount = ((suggestedLatencyFrames + (hostBufferSizeFrames - 1)) / hostBufferSizeFrames);

    /* We always need one extra buffer for processing while the rest are queued/playing.
       i.e. latency is framesPerBuffer * (bufferCount - 1)
    */
    resultBufferCount += 1;

    if( resultBufferCount < minimumBufferCount ) /* clamp to minimum buffer count */
        resultBufferCount = minimumBufferCount;

    return resultBufferCount;
}


static unsigned long ComputeHostBufferSizeGivenHardUpperLimit( 
        unsigned long userFramesPerBuffer,
        unsigned long absoluteMaximumBufferSizeFrames )
{
    static unsigned long primes_[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 
            29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 0 }; /* zero terminated */

    unsigned long result = userFramesPerBuffer;
    int i;

    assert( absoluteMaximumBufferSizeFrames > 67 ); /* assume maximum is large and we're only factoring by small primes */

    /* search for the largest integer factor of userFramesPerBuffer less 
       than or equal to absoluteMaximumBufferSizeFrames */

    /* repeatedly divide by smallest prime factors until a buffer size 
       smaller than absoluteMaximumBufferSizeFrames is found */
    while( result > absoluteMaximumBufferSizeFrames ){

        /* search for the smallest prime factor of result */
        for( i=0; primes_[i] != 0; ++i ) 
        {
            unsigned long p = primes_[i];
            unsigned long divided = result / p;
            if( divided*p == result )
            {
                result = divided;
                break; /* continue with outer while loop */
            }
        }
        if( primes_[i] == 0 )
        { /* loop failed to find a prime factor, return an approximate result */
            unsigned long d = (userFramesPerBuffer + (absoluteMaximumBufferSizeFrames-1))
                    / absoluteMaximumBufferSizeFrames;
            return userFramesPerBuffer / d;
        }
    }

    return result;
}


static PaError SelectHostBufferSizeFramesAndHostBufferCount(
        unsigned long suggestedLatencyFrames,
        unsigned long userFramesPerBuffer,
        unsigned long minimumBufferCount,
        unsigned long preferredMaximumBufferSizeFrames, /* try not to exceed this. for example, don't exceed when coalescing buffers */
        unsigned long absoluteMaximumBufferSizeFrames,  /* never exceed this, a hard limit */
        unsigned long *hostBufferSizeFrames,
        unsigned long *hostBufferCount )
{
    unsigned long effectiveUserFramesPerBuffer;
    unsigned long numberOfUserBuffersPerHostBuffer;


    if( userFramesPerBuffer == paFramesPerBufferUnspecified ){

        effectiveUserFramesPerBuffer = PA_MME_HOST_BUFFER_GRANULARITY_FRAMES_WHEN_UNSPECIFIED_;

    }else{

        if( userFramesPerBuffer > absoluteMaximumBufferSizeFrames ){

            /* user has requested a user buffer that's larger than absoluteMaximumBufferSizeFrames.
               try to choose a buffer size that is equal or smaller than absoluteMaximumBufferSizeFrames
               but is also an integer factor of userFramesPerBuffer, so as to distribute computation evenly.
               the buffer processor will handle the block adaption between host and user buffer sizes.
               see http://www.portaudio.com/trac/ticket/189 for discussion.
            */

            effectiveUserFramesPerBuffer = ComputeHostBufferSizeGivenHardUpperLimit( userFramesPerBuffer, absoluteMaximumBufferSizeFrames );
            assert( effectiveUserFramesPerBuffer <= absoluteMaximumBufferSizeFrames );

            /* try to ensure that duration of host buffering is at least as 
                large as duration of user buffer. */
            if( suggestedLatencyFrames < userFramesPerBuffer )
                suggestedLatencyFrames = userFramesPerBuffer; 

        }else{

            effectiveUserFramesPerBuffer = userFramesPerBuffer;
        }
    }
                        
    /* compute a host buffer count based on suggestedLatencyFrames and our granularity */

    *hostBufferSizeFrames = effectiveUserFramesPerBuffer;

    *hostBufferCount = ComputeHostBufferCountForFixedBufferSizeFrames(
            suggestedLatencyFrames, *hostBufferSizeFrames, minimumBufferCount );

    if( *hostBufferSizeFrames >= userFramesPerBuffer )
    {
        /*
            If there are too many host buffers we would like to coalesce 
            them by packing an integer number of user buffers into each host buffer.
            We try to coalesce such that hostBufferCount will lie between 
            PA_MME_TARGET_HOST_BUFFER_COUNT_ and (PA_MME_TARGET_HOST_BUFFER_COUNT_*2)-1.
            We limit coalescing to avoid exceeding either absoluteMaximumBufferSizeFrames and
            preferredMaximumBufferSizeFrames. 

            First, compute a coalescing factor: the number of user buffers per host buffer.
            The goal is to achieve PA_MME_TARGET_HOST_BUFFER_COUNT_ total buffer count.
            Since our latency is computed based on (*hostBufferCount - 1) we compute a
            coalescing factor based on (*hostBufferCount - 1) and (PA_MME_TARGET_HOST_BUFFER_COUNT_-1).

            The + (PA_MME_TARGET_HOST_BUFFER_COUNT_-2) term below is intended to round up.
        */
        numberOfUserBuffersPerHostBuffer = ((*hostBufferCount - 1) + (PA_MME_TARGET_HOST_BUFFER_COUNT_-2)) / (PA_MME_TARGET_HOST_BUFFER_COUNT_ - 1);
        
        if( numberOfUserBuffersPerHostBuffer > 1 )
        {
            unsigned long maxCoalescedBufferSizeFrames = (absoluteMaximumBufferSizeFrames < preferredMaximumBufferSizeFrames) /* minimum of our limits */
                            ? absoluteMaximumBufferSizeFrames
                            : preferredMaximumBufferSizeFrames;

            unsigned long maxUserBuffersPerHostBuffer = maxCoalescedBufferSizeFrames / effectiveUserFramesPerBuffer; /* don't coalesce more than this */

            if( numberOfUserBuffersPerHostBuffer > maxUserBuffersPerHostBuffer )
                numberOfUserBuffersPerHostBuffer = maxUserBuffersPerHostBuffer;

            *hostBufferSizeFrames = effectiveUserFramesPerBuffer * numberOfUserBuffersPerHostBuffer;

            /* recompute hostBufferCount to approximate suggestedLatencyFrames now that hostBufferSizeFrames is larger */
            *hostBufferCount = ComputeHostBufferCountForFixedBufferSizeFrames(
                    suggestedLatencyFrames, *hostBufferSizeFrames, minimumBufferCount );
        }
    }

    return paNoError;
}


static PaError CalculateMaxHostSampleFrameSizeBytes(
        int channelCount,
        PaSampleFormat hostSampleFormat,
        const PaWinMmeStreamInfo *streamInfo,
        int *hostSampleFrameSizeBytes )
{
    unsigned int i;
    /* PA WMME streams may aggregate multiple WMME devices. When the stream addresses 
       more than one device in a single direction, maxDeviceChannelCount is the maximum 
       number of channels used by a single device.
    */
    int maxDeviceChannelCount = channelCount;
    int hostSampleSizeBytes = Pa_GetSampleSize( hostSampleFormat );
    if( hostSampleSizeBytes < 0 )
    {
        return hostSampleSizeBytes; /* the value of hostSampleSize here is an error code, not a sample size */
    }

    if( streamInfo && ( streamInfo->flags & paWinMmeUseMultipleDevices ) )
    {
        maxDeviceChannelCount = streamInfo->devices[0].channelCount;
        for( i=1; i< streamInfo->deviceCount; ++i )
        {
            if( streamInfo->devices[i].channelCount > maxDeviceChannelCount )
                maxDeviceChannelCount = streamInfo->devices[i].channelCount;
        }
    }

    *hostSampleFrameSizeBytes = hostSampleSizeBytes * maxDeviceChannelCount;

    return paNoError;
}


/* CalculateBufferSettings() fills the framesPerHostInputBuffer, hostInputBufferCount,
   framesPerHostOutputBuffer and hostOutputBufferCount parameters based on the values
   of the other parameters.
*/

static PaError CalculateBufferSettings(
        unsigned long *hostFramesPerInputBuffer, unsigned long *hostInputBufferCount,
        unsigned long *hostFramesPerOutputBuffer, unsigned long *hostOutputBufferCount,
        int inputChannelCount, PaSampleFormat hostInputSampleFormat,
        PaTime suggestedInputLatency, const PaWinMmeStreamInfo *inputStreamInfo,
        int outputChannelCount, PaSampleFormat hostOutputSampleFormat,
        PaTime suggestedOutputLatency, const PaWinMmeStreamInfo *outputStreamInfo,
        double sampleRate, unsigned long userFramesPerBuffer )
{
    PaError result = paNoError;
    
    if( inputChannelCount > 0 ) /* stream has input */
    {
        int hostInputFrameSizeBytes;
        result = CalculateMaxHostSampleFrameSizeBytes( 
                inputChannelCount, hostInputSampleFormat, inputStreamInfo, &hostInputFrameSizeBytes );
        if( result != paNoError )
            goto error;

        if( inputStreamInfo
                && ( inputStreamInfo->flags & paWinMmeUseLowLevelLatencyParameters ) )
        {
            /* input - using low level latency parameters if provided */

            if( inputStreamInfo->bufferCount <= 0
                    || inputStreamInfo->framesPerBuffer <= 0 )
            {
                result = paIncompatibleHostApiSpecificStreamInfo;
                goto error;
            }

            *hostFramesPerInputBuffer = inputStreamInfo->framesPerBuffer;
            *hostInputBufferCount = inputStreamInfo->bufferCount;
        }
        else
        {
            /* input - not using low level latency parameters, so compute 
               hostFramesPerInputBuffer and hostInputBufferCount
               based on userFramesPerBuffer and suggestedInputLatency. */

            unsigned long minimumBufferCount = (outputChannelCount > 0)
                    ? PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_FULL_DUPLEX_
                    : PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_HALF_DUPLEX_;

            result = SelectHostBufferSizeFramesAndHostBufferCount(
                    (unsigned long)(suggestedInputLatency * sampleRate), /* (truncate) */
                    userFramesPerBuffer,
                    minimumBufferCount,
                    (unsigned long)(PA_MME_MAX_HOST_BUFFER_SECS_ * sampleRate), /* in frames. preferred maximum */
                    (PA_MME_MAX_HOST_BUFFER_BYTES_ / hostInputFrameSizeBytes),  /* in frames. a hard limit. note truncation due to 
                                                                                division is intentional here to limit max bytes */
                    hostFramesPerInputBuffer,
                    hostInputBufferCount );
            if( result != paNoError )
                goto error;
        }
    }
    else
    {
        *hostFramesPerInputBuffer = 0;
        *hostInputBufferCount = 0;
    }

    if( outputChannelCount > 0 ) /* stream has output */
    {
        if( outputStreamInfo
                && ( outputStreamInfo->flags & paWinMmeUseLowLevelLatencyParameters ) )
        {
            /* output - using low level latency parameters */

            if( outputStreamInfo->bufferCount <= 0
                    || outputStreamInfo->framesPerBuffer <= 0 )
            {
                result = paIncompatibleHostApiSpecificStreamInfo;
                goto error;
            }

            *hostFramesPerOutputBuffer = outputStreamInfo->framesPerBuffer;
            *hostOutputBufferCount = outputStreamInfo->bufferCount;

            if( inputChannelCount > 0 ) /* full duplex */
            {
                /* harmonize hostFramesPerInputBuffer and hostFramesPerOutputBuffer */

                if( *hostFramesPerInputBuffer != *hostFramesPerOutputBuffer )
                {
                    if( inputStreamInfo
                            && ( inputStreamInfo->flags & paWinMmeUseLowLevelLatencyParameters ) )
                    { 
                        /* a custom StreamInfo was used for specifying both input
                            and output buffer sizes. We require that the larger buffer size
                            must be a multiple of the smaller buffer size */

                        if( *hostFramesPerInputBuffer < *hostFramesPerOutputBuffer )
                        {
                            if( *hostFramesPerOutputBuffer % *hostFramesPerInputBuffer != 0 )
                            {
                                result = paIncompatibleHostApiSpecificStreamInfo;
                                goto error;
                            }
                        }
                        else
                        {
                            assert( *hostFramesPerInputBuffer > *hostFramesPerOutputBuffer );
                            if( *hostFramesPerInputBuffer % *hostFramesPerOutputBuffer != 0 )
                            {
                                result = paIncompatibleHostApiSpecificStreamInfo;
                                goto error;
                            }
                        }                        
                    }
                    else
                    {
                        /* a custom StreamInfo was not used for specifying the input buffer size,
                            so use the output buffer size, and approximately the suggested input latency. */

                        *hostFramesPerInputBuffer = *hostFramesPerOutputBuffer;

                        *hostInputBufferCount = ComputeHostBufferCountForFixedBufferSizeFrames(
                                (unsigned long)(suggestedInputLatency * sampleRate), 
                                *hostFramesPerInputBuffer, 
                                PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_FULL_DUPLEX_ );
                    }
                }
            }
        }
        else
        {
            /* output - no low level latency parameters, so compute hostFramesPerOutputBuffer and hostOutputBufferCount
                based on userFramesPerBuffer and suggestedOutputLatency. */

            int hostOutputFrameSizeBytes;
            result = CalculateMaxHostSampleFrameSizeBytes( 
                    outputChannelCount, hostOutputSampleFormat, outputStreamInfo, &hostOutputFrameSizeBytes );
            if( result != paNoError )
                goto error;

            /* compute the output buffer size and count */

            result = SelectHostBufferSizeFramesAndHostBufferCount(
                    (unsigned long)(suggestedOutputLatency * sampleRate), /* (truncate) */
                    userFramesPerBuffer,
                    PA_MME_MIN_HOST_OUTPUT_BUFFER_COUNT_,
                    (unsigned long)(PA_MME_MAX_HOST_BUFFER_SECS_ * sampleRate), /* in frames. preferred maximum */
                    (PA_MME_MAX_HOST_BUFFER_BYTES_ / hostOutputFrameSizeBytes),  /* in frames. a hard limit. note truncation due to 
                                                                                 division is intentional here to limit max bytes */
                    hostFramesPerOutputBuffer,
                    hostOutputBufferCount );
            if( result != paNoError )
                goto error;

            if( inputChannelCount > 0 ) /* full duplex */
            {
                /* harmonize hostFramesPerInputBuffer and hostFramesPerOutputBuffer */

                /* ensure that both input and output buffer sizes are the same.
                    if they don't match at this stage, choose the smallest one
                    and use that for input and output and recompute the corresponding
                    buffer count accordingly.
                */

                if( *hostFramesPerOutputBuffer != *hostFramesPerInputBuffer )
                {
                    if( hostFramesPerInputBuffer < hostFramesPerOutputBuffer )
                    {
                        *hostFramesPerOutputBuffer = *hostFramesPerInputBuffer;

                        *hostOutputBufferCount = ComputeHostBufferCountForFixedBufferSizeFrames(
                                (unsigned long)(suggestedOutputLatency * sampleRate), 
                                *hostOutputBufferCount, 
                                PA_MME_MIN_HOST_OUTPUT_BUFFER_COUNT_ );
                    }
                    else
                    {
                        *hostFramesPerInputBuffer = *hostFramesPerOutputBuffer;

                        *hostInputBufferCount = ComputeHostBufferCountForFixedBufferSizeFrames(
                                (unsigned long)(suggestedInputLatency * sampleRate), 
                                *hostFramesPerInputBuffer, 
                                PA_MME_MIN_HOST_INPUT_BUFFER_COUNT_FULL_DUPLEX_ );
                    }
                }   
            }
        }
    }
    else
    {
        *hostFramesPerOutputBuffer = 0;
        *hostOutputBufferCount = 0;
    }

error:
    return result;
}


typedef struct
{
    HANDLE bufferEvent;
    void *waveHandles;
    unsigned int deviceCount;
    /* unsigned int channelCount; */
    WAVEHDR **waveHeaders;                  /* waveHeaders[device][buffer] */
    unsigned int bufferCount;
    unsigned int currentBufferIndex;
    unsigned int framesPerBuffer;
    unsigned int framesUsedInCurrentBuffer;
}PaWinMmeSingleDirectionHandlesAndBuffers;

/* prototypes for functions operating on PaWinMmeSingleDirectionHandlesAndBuffers */

static void InitializeSingleDirectionHandlesAndBuffers( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers );
static PaError InitializeWaveHandles( PaWinMmeHostApiRepresentation *winMmeHostApi,
        PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers,
        unsigned long winMmeSpecificFlags,
        unsigned long bytesPerHostSample,
        double sampleRate, PaWinMmeDeviceAndChannelCount *devices,
        unsigned int deviceCount, PaWinWaveFormatChannelMask channelMask, int isInput );
static PaError TerminateWaveHandles( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers, int isInput, int currentlyProcessingAnError );
static PaError InitializeWaveHeaders( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers,
        unsigned long hostBufferCount,
        PaSampleFormat hostSampleFormat,
        unsigned long framesPerHostBuffer,
        PaWinMmeDeviceAndChannelCount *devices,
        int isInput );
static void TerminateWaveHeaders( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers, int isInput );


static void InitializeSingleDirectionHandlesAndBuffers( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers )
{
    handlesAndBuffers->bufferEvent = 0;
    handlesAndBuffers->waveHandles = 0;
    handlesAndBuffers->deviceCount = 0;
    handlesAndBuffers->waveHeaders = 0;
    handlesAndBuffers->bufferCount = 0;
}    

static PaError InitializeWaveHandles( PaWinMmeHostApiRepresentation *winMmeHostApi,
        PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers,
        unsigned long winMmeSpecificFlags,
        unsigned long bytesPerHostSample,
        double sampleRate, PaWinMmeDeviceAndChannelCount *devices,
        unsigned int deviceCount, PaWinWaveFormatChannelMask channelMask, int isInput )
{
    PaError result;
    MMRESULT mmresult;
    signed int i, j;
    PaSampleFormat sampleFormat;
    int waveFormatTag;

    /* for error cleanup we expect that InitializeSingleDirectionHandlesAndBuffers()
        has already been called to zero some fields */       

    result = CreateEventWithPaError( &handlesAndBuffers->bufferEvent, NULL, FALSE, FALSE, NULL );
    if( result != paNoError ) goto error;

    if( isInput )
        handlesAndBuffers->waveHandles = (void*)PaUtil_AllocateMemory( sizeof(HWAVEIN) * deviceCount );
    else
        handlesAndBuffers->waveHandles = (void*)PaUtil_AllocateMemory( sizeof(HWAVEOUT) * deviceCount );
    if( !handlesAndBuffers->waveHandles )
    {
        result = paInsufficientMemory;
        goto error;
    }

    handlesAndBuffers->deviceCount = deviceCount;

    for( i = 0; i < (signed int)deviceCount; ++i )
    {
        if( isInput )
            ((HWAVEIN*)handlesAndBuffers->waveHandles)[i] = 0;
        else
            ((HWAVEOUT*)handlesAndBuffers->waveHandles)[i] = 0;
    }

    /* @todo at the moment we only use 16 bit sample format */
    sampleFormat = paInt16;
    waveFormatTag = SampleFormatAndWinWmmeSpecificFlagsToLinearWaveFormatTag( sampleFormat, winMmeSpecificFlags );

    for( i = 0; i < (signed int)deviceCount; ++i )
    {
        PaWinWaveFormat waveFormat;
        UINT winMmeDeviceId = LocalDeviceIndexToWinMmeDeviceId( winMmeHostApi, devices[i].device );
    
        /* @todo: consider providing a flag or #define to not try waveformat extensible 
           this could just initialize j to 1 the first time round. */

        for( j = 0; j < 2; ++j )
        {
            switch(j){
                case 0:     
                    /* first, attempt to open the device using WAVEFORMATEXTENSIBLE, 
                        if this fails we fall back to WAVEFORMATEX */

                    PaWin_InitializeWaveFormatExtensible( &waveFormat, devices[i].channelCount, 
                            sampleFormat, waveFormatTag, sampleRate, channelMask );
                    break;
                
                case 1:
                    /* retry with WAVEFORMATEX */

                    PaWin_InitializeWaveFormatEx( &waveFormat, devices[i].channelCount, 
                            sampleFormat, waveFormatTag, sampleRate );
                    break;
            }

            /* REVIEW: consider not firing an event for input when a full duplex
                stream is being used. this would probably depend on the
                neverDropInput flag. */

            if( isInput )
            {
                mmresult = waveInOpen( &((HWAVEIN*)handlesAndBuffers->waveHandles)[i], winMmeDeviceId, 
                                    (WAVEFORMATEX*)&waveFormat,
                               (DWORD_PTR)handlesAndBuffers->bufferEvent, (DWORD_PTR)0, CALLBACK_EVENT );
            }
            else
            {
                mmresult = waveOutOpen( &((HWAVEOUT*)handlesAndBuffers->waveHandles)[i], winMmeDeviceId, 
                                    (WAVEFORMATEX*)&waveFormat,
                                (DWORD_PTR)handlesAndBuffers->bufferEvent, (DWORD_PTR)0, CALLBACK_EVENT );
            }

            if( mmresult == MMSYSERR_NOERROR )
            {
                break; /* success */
            }
            else if( j == 0 )
            {
                continue; /* try again with WAVEFORMATEX */
            }
            else
            {
                switch( mmresult )
                {
                    case MMSYSERR_ALLOCATED:    /* Specified resource is already allocated. */
                        result = paDeviceUnavailable;
                        break;
                    case MMSYSERR_NODRIVER:	    /* No device driver is present. */
                        result = paDeviceUnavailable;
                        break;
                    case MMSYSERR_NOMEM:	    /* Unable to allocate or lock memory. */
                        result = paInsufficientMemory;
                        break;

                    case MMSYSERR_BADDEVICEID:	/* Specified device identifier is out of range. */
                        /* falls through */

                    case WAVERR_BADFORMAT:      /* Attempted to open with an unsupported waveform-audio format. */
                                                    /* This can also occur if we try to open the device with an unsupported
                                                     * number of channels. This is attempted when device*ChannelCountIsKnown is
                                                     * set to 0. 
                                                     */
                        /* falls through */
                    default:
                        result = paUnanticipatedHostError;
                        if( isInput )
                        {
                            PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                        }
                        else
                        {
                            PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                        }
                }
                goto error;
            }
        }
    }

    return result;

error:
    TerminateWaveHandles( handlesAndBuffers, isInput, 1 /* currentlyProcessingAnError */ );

    return result;
}


static PaError TerminateWaveHandles( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers, int isInput, int currentlyProcessingAnError )
{
    PaError result = paNoError;
    MMRESULT mmresult;
    signed int i;
    
    if( handlesAndBuffers->waveHandles )
    {
        for( i = handlesAndBuffers->deviceCount-1; i >= 0; --i )
        {
            if( isInput )
            {
                if( ((HWAVEIN*)handlesAndBuffers->waveHandles)[i] )
                    mmresult = waveInClose( ((HWAVEIN*)handlesAndBuffers->waveHandles)[i] );
                else
                    mmresult = MMSYSERR_NOERROR;
            }
            else
            {
                if( ((HWAVEOUT*)handlesAndBuffers->waveHandles)[i] )
                    mmresult = waveOutClose( ((HWAVEOUT*)handlesAndBuffers->waveHandles)[i] );
                else
                    mmresult = MMSYSERR_NOERROR;
            }

            if( mmresult != MMSYSERR_NOERROR &&
                !currentlyProcessingAnError ) /* don't update the error state if we're already processing an error */
            {
                result = paUnanticipatedHostError;
                if( isInput )
                {
                    PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                }
                else
                {
                    PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                }
                /* note that we don't break here, we try to continue closing devices */
            }
        }

        PaUtil_FreeMemory( handlesAndBuffers->waveHandles );
        handlesAndBuffers->waveHandles = 0;
    }

    if( handlesAndBuffers->bufferEvent )
    {
        result = CloseHandleWithPaError( handlesAndBuffers->bufferEvent );
        handlesAndBuffers->bufferEvent = 0;
    }
    
    return result;
}


static PaError InitializeWaveHeaders( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers,
        unsigned long hostBufferCount,
        PaSampleFormat hostSampleFormat,
        unsigned long framesPerHostBuffer,
        PaWinMmeDeviceAndChannelCount *devices,
        int isInput )
{
    PaError result = paNoError;
    MMRESULT mmresult;
    WAVEHDR *deviceWaveHeaders;
    signed int i, j;

    /* for error cleanup we expect that InitializeSingleDirectionHandlesAndBuffers()
        has already been called to zero some fields */
        

    /* allocate an array of pointers to arrays of wave headers, one array of
        wave headers per device */
    handlesAndBuffers->waveHeaders = (WAVEHDR**)PaUtil_AllocateMemory( sizeof(WAVEHDR*) * handlesAndBuffers->deviceCount );
    if( !handlesAndBuffers->waveHeaders )
    {
        result = paInsufficientMemory;
        goto error;
    }
    
    for( i = 0; i < (signed int)handlesAndBuffers->deviceCount; ++i )
        handlesAndBuffers->waveHeaders[i] = 0;

    handlesAndBuffers->bufferCount = hostBufferCount;

    for( i = 0; i < (signed int)handlesAndBuffers->deviceCount; ++i )
    {
        int bufferBytes = Pa_GetSampleSize( hostSampleFormat ) *
                framesPerHostBuffer * devices[i].channelCount;
        if( bufferBytes < 0 )
        {
            result = paInternalError;
            goto error;
        }

        /* Allocate an array of wave headers for device i */
        deviceWaveHeaders = (WAVEHDR *) PaUtil_AllocateMemory( sizeof(WAVEHDR)*hostBufferCount );
        if( !deviceWaveHeaders )
        {
            result = paInsufficientMemory;
            goto error;
        }

        for( j=0; j < (signed int)hostBufferCount; ++j )
            deviceWaveHeaders[j].lpData = 0;

        handlesAndBuffers->waveHeaders[i] = deviceWaveHeaders;

        /* Allocate a buffer for each wave header */
        for( j=0; j < (signed int)hostBufferCount; ++j )
        {
            deviceWaveHeaders[j].lpData = (char *)PaUtil_AllocateMemory( bufferBytes );
            if( !deviceWaveHeaders[j].lpData )
            {
                result = paInsufficientMemory;
                goto error;
            }
            deviceWaveHeaders[j].dwBufferLength = bufferBytes;
            deviceWaveHeaders[j].dwUser = 0xFFFFFFFF; /* indicates that *PrepareHeader() has not yet been called, for error clean up code */

            if( isInput )
            {
                mmresult = waveInPrepareHeader( ((HWAVEIN*)handlesAndBuffers->waveHandles)[i], &deviceWaveHeaders[j], sizeof(WAVEHDR) );
                if( mmresult != MMSYSERR_NOERROR )
                {
                    result = paUnanticipatedHostError;
                    PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                    goto error;
                }
            }
            else /* output */
            {
                mmresult = waveOutPrepareHeader( ((HWAVEOUT*)handlesAndBuffers->waveHandles)[i], &deviceWaveHeaders[j], sizeof(WAVEHDR) );
                if( mmresult != MMSYSERR_NOERROR )
                {
                    result = paUnanticipatedHostError;
                    PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                    goto error;
                }
            }
            deviceWaveHeaders[j].dwUser = devices[i].channelCount;
        }
    }

    return result;

error:
    TerminateWaveHeaders( handlesAndBuffers, isInput );
    
    return result;
}


static void TerminateWaveHeaders( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers, int isInput )
{
    signed int i, j;
    WAVEHDR *deviceWaveHeaders;
    
    if( handlesAndBuffers->waveHeaders )
    {
        for( i = handlesAndBuffers->deviceCount-1; i >= 0 ; --i )
        {
            deviceWaveHeaders = handlesAndBuffers->waveHeaders[i];  /* wave headers for device i */
            if( deviceWaveHeaders )
            {
                for( j = handlesAndBuffers->bufferCount-1; j >= 0; --j )
                {
                    if( deviceWaveHeaders[j].lpData )
                    {
                        if( deviceWaveHeaders[j].dwUser != 0xFFFFFFFF )
                        {
                            if( isInput )
                                waveInUnprepareHeader( ((HWAVEIN*)handlesAndBuffers->waveHandles)[i], &deviceWaveHeaders[j], sizeof(WAVEHDR) );
                            else
                                waveOutUnprepareHeader( ((HWAVEOUT*)handlesAndBuffers->waveHandles)[i], &deviceWaveHeaders[j], sizeof(WAVEHDR) );
                        }

                        PaUtil_FreeMemory( deviceWaveHeaders[j].lpData );
                    }
                }

                PaUtil_FreeMemory( deviceWaveHeaders );
            }
        }

        PaUtil_FreeMemory( handlesAndBuffers->waveHeaders );
        handlesAndBuffers->waveHeaders = 0;
    }
}



/* PaWinMmeStream - a stream data structure specifically for this implementation */
/* note that struct PaWinMmeStream is typedeffed to PaWinMmeStream above. */
struct PaWinMmeStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    int primeStreamUsingCallback;

    PaWinMmeSingleDirectionHandlesAndBuffers input;
    PaWinMmeSingleDirectionHandlesAndBuffers output;

    /* Processing thread management -------------- */
    HANDLE abortEvent;
    HANDLE processingThread;
    PA_THREAD_ID processingThreadId;

    char throttleProcessingThreadOnOverload; /* 0 -> don't throtte, non-0 -> throttle */
    int processingThreadPriority;
    int highThreadPriority;
    int throttledThreadPriority;
    unsigned long throttledSleepMsecs;

    int isStopped;
    volatile int isActive;
    volatile int stopProcessing; /* stop thread once existing buffers have been returned */
    volatile int abortProcessing; /* stop thread immediately */

    DWORD allBuffersDurationMs; /* used to calculate timeouts */
};

/* updates deviceCount if PaWinMmeUseMultipleDevices is used */

static PaError ValidateWinMmeSpecificStreamInfo(
        const PaStreamParameters *streamParameters,
        const PaWinMmeStreamInfo *streamInfo,
        unsigned long *winMmeSpecificFlags,
        char *throttleProcessingThreadOnOverload,
        unsigned long *deviceCount )
{
	if( streamInfo )
	{
	    if( streamInfo->size != sizeof( PaWinMmeStreamInfo )
	            || streamInfo->version != 1 )
	    {
	        return paIncompatibleHostApiSpecificStreamInfo;
	    }

        *winMmeSpecificFlags = streamInfo->flags;

	    if( streamInfo->flags & paWinMmeDontThrottleOverloadedProcessingThread )
	        *throttleProcessingThreadOnOverload = 0;
            
	    if( streamInfo->flags & paWinMmeUseMultipleDevices )
	    {
	        if( streamParameters->device != paUseHostApiSpecificDeviceSpecification )
	            return paInvalidDevice;
	
			*deviceCount = streamInfo->deviceCount;
		}	
	}

	return paNoError;
}

static PaError RetrieveDevicesFromStreamParameters(
        struct PaUtilHostApiRepresentation *hostApi,
        const PaStreamParameters *streamParameters,
        const PaWinMmeStreamInfo *streamInfo,
        PaWinMmeDeviceAndChannelCount *devices,
        unsigned long deviceCount )
{
    PaError result = paNoError;
    unsigned int i;
    int totalChannelCount;
    PaDeviceIndex hostApiDevice;
    
	if( streamInfo && streamInfo->flags & paWinMmeUseMultipleDevices )
	{
		totalChannelCount = 0;
	    for( i=0; i < deviceCount; ++i )
	    {
	        /* validate that the device number is within range */
	        result = PaUtil_DeviceIndexToHostApiDeviceIndex( &hostApiDevice,
	                        streamInfo->devices[i].device, hostApi );
	        if( result != paNoError )
	            return result;
	        
	        devices[i].device = hostApiDevice;
	        devices[i].channelCount = streamInfo->devices[i].channelCount;
	
	        totalChannelCount += devices[i].channelCount;
	    }
	
	    if( totalChannelCount != streamParameters->channelCount )
	    {
	        /* channelCount must match total channels specified by multiple devices */
	        return paInvalidChannelCount; /* REVIEW use of this error code */
	    }
	}	
	else
	{		
	    devices[0].device = streamParameters->device;
	    devices[0].channelCount = streamParameters->channelCount;
	}

    return result;
}

static PaError ValidateInputChannelCounts(
        struct PaUtilHostApiRepresentation *hostApi,
        PaWinMmeDeviceAndChannelCount *devices,
        unsigned long deviceCount )
{
    unsigned int i;
    PaWinMmeDeviceInfo *inputDeviceInfo;
    PaError paerror;

	for( i=0; i < deviceCount; ++i )
	{
        if( devices[i].channelCount < 1 )
        	return paInvalidChannelCount;

        inputDeviceInfo = 
                (PaWinMmeDeviceInfo*)hostApi->deviceInfos[ devices[i].device ];

        paerror = IsInputChannelCountSupported( inputDeviceInfo, devices[i].channelCount );
        if( paerror != paNoError )
            return paerror;
	}

    return paNoError;
}

static PaError ValidateOutputChannelCounts(
        struct PaUtilHostApiRepresentation *hostApi,
        PaWinMmeDeviceAndChannelCount *devices,
        unsigned long deviceCount )
{
    unsigned int i;
    PaWinMmeDeviceInfo *outputDeviceInfo;
    PaError paerror;

	for( i=0; i < deviceCount; ++i )
	{
        if( devices[i].channelCount < 1 )
        	return paInvalidChannelCount;

        outputDeviceInfo = 
                (PaWinMmeDeviceInfo*)hostApi->deviceInfos[ devices[i].device ];

        paerror = IsOutputChannelCountSupported( outputDeviceInfo, devices[i].channelCount );
        if( paerror != paNoError )
            return paerror;
	}

    return paNoError;
}


/* the following macros are intended to improve the readability of the following code */
#define PA_IS_INPUT_STREAM_( stream ) ( stream ->input.waveHandles )
#define PA_IS_OUTPUT_STREAM_( stream ) ( stream ->output.waveHandles )
#define PA_IS_FULL_DUPLEX_STREAM_( stream ) ( stream ->input.waveHandles && stream ->output.waveHandles )
#define PA_IS_HALF_DUPLEX_STREAM_( stream ) ( !(stream ->input.waveHandles && stream ->output.waveHandles) )

static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData )
{
    PaError result;
    PaWinMmeHostApiRepresentation *winMmeHostApi = (PaWinMmeHostApiRepresentation*)hostApi;
    PaWinMmeStream *stream = 0;
    int bufferProcessorIsInitialized = 0;
    int streamRepresentationIsInitialized = 0;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    double suggestedInputLatency, suggestedOutputLatency;
    PaWinMmeStreamInfo *inputStreamInfo, *outputStreamInfo;
    PaWinWaveFormatChannelMask inputChannelMask, outputChannelMask;
    unsigned long framesPerHostInputBuffer;
    unsigned long hostInputBufferCount;
    unsigned long framesPerHostOutputBuffer;
    unsigned long hostOutputBufferCount;
    unsigned long framesPerBufferProcessorCall;
    PaWinMmeDeviceAndChannelCount *inputDevices = 0;  /* contains all devices and channel counts as local host api ids, even when PaWinMmeUseMultipleDevices is not used */
    unsigned long winMmeSpecificInputFlags = 0;
    unsigned long inputDeviceCount = 0;            
    PaWinMmeDeviceAndChannelCount *outputDevices = 0;
    unsigned long winMmeSpecificOutputFlags = 0;
    unsigned long outputDeviceCount = 0;                /* contains all devices and channel counts as local host api ids, even when PaWinMmeUseMultipleDevices is not used */
    char throttleProcessingThreadOnOverload = 1;

    
    if( inputParameters )
    {
		inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
        suggestedInputLatency = inputParameters->suggestedLatency;

      	inputDeviceCount = 1;

		/* validate input hostApiSpecificStreamInfo */
        inputStreamInfo = (PaWinMmeStreamInfo*)inputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinMmeSpecificStreamInfo( inputParameters, inputStreamInfo,
                &winMmeSpecificInputFlags,
				&throttleProcessingThreadOnOverload,
				&inputDeviceCount );
		if( result != paNoError ) return result;

		inputDevices = (PaWinMmeDeviceAndChannelCount*)alloca( sizeof(PaWinMmeDeviceAndChannelCount) * inputDeviceCount );
        if( !inputDevices ) return paInsufficientMemory;

		result = RetrieveDevicesFromStreamParameters( hostApi, inputParameters, inputStreamInfo, inputDevices, inputDeviceCount );
		if( result != paNoError ) return result;

		result = ValidateInputChannelCounts( hostApi, inputDevices, inputDeviceCount );
		if( result != paNoError ) return result;

        hostInputSampleFormat =
            PaUtil_SelectClosestAvailableFormat( paInt16 /* native formats */, inputSampleFormat );

        if( inputDeviceCount != 1 ){
            /* always use direct speakers when using multi-device multichannel mode */
            inputChannelMask = PAWIN_SPEAKER_DIRECTOUT;           
        }
        else
        {
            if( inputStreamInfo && inputStreamInfo->flags & paWinMmeUseChannelMask )
                inputChannelMask = inputStreamInfo->channelMask;
            else
                inputChannelMask = PaWin_DefaultChannelMask( inputDevices[0].channelCount );
        }
	}
    else
    {
        inputChannelCount = 0;
        inputSampleFormat = 0;
        suggestedInputLatency = 0.;
        inputStreamInfo = 0;
        hostInputSampleFormat = 0;
    }


    if( outputParameters )
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        suggestedOutputLatency = outputParameters->suggestedLatency;

        outputDeviceCount = 1;

		/* validate output hostApiSpecificStreamInfo */
        outputStreamInfo = (PaWinMmeStreamInfo*)outputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinMmeSpecificStreamInfo( outputParameters, outputStreamInfo,
                &winMmeSpecificOutputFlags,
				&throttleProcessingThreadOnOverload,
				&outputDeviceCount );
		if( result != paNoError ) return result;

		outputDevices = (PaWinMmeDeviceAndChannelCount*)alloca( sizeof(PaWinMmeDeviceAndChannelCount) * outputDeviceCount );
        if( !outputDevices ) return paInsufficientMemory;

		result = RetrieveDevicesFromStreamParameters( hostApi, outputParameters, outputStreamInfo, outputDevices, outputDeviceCount );
		if( result != paNoError ) return result;

		result = ValidateOutputChannelCounts( hostApi, outputDevices, outputDeviceCount );
		if( result != paNoError ) return result;

        hostOutputSampleFormat =
            PaUtil_SelectClosestAvailableFormat( paInt16 /* native formats */, outputSampleFormat );

        if( outputDeviceCount != 1 ){
            /* always use direct speakers when using multi-device multichannel mode */
            outputChannelMask = PAWIN_SPEAKER_DIRECTOUT;           
        }
        else
        {
            if( outputStreamInfo && outputStreamInfo->flags & paWinMmeUseChannelMask )
                outputChannelMask = outputStreamInfo->channelMask;
            else
                outputChannelMask = PaWin_DefaultChannelMask( outputDevices[0].channelCount );
        }
    }
    else
    {
        outputChannelCount = 0;
        outputSampleFormat = 0;
        outputStreamInfo = 0;
        hostOutputSampleFormat = 0;
        suggestedOutputLatency = 0.;
    }


    /*
        IMPLEMENT ME:
            - alter sampleRate to a close allowable rate if possible / necessary
    */


    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform specific flag */


    /* always disable clipping and dithering if we are outputting a raw spdif stream */
    if( (winMmeSpecificOutputFlags & paWinMmeWaveFormatDolbyAc3Spdif)
            || (winMmeSpecificOutputFlags & paWinMmeWaveFormatWmaSpdif) ){

        streamFlags = streamFlags | paClipOff | paDitherOff;
    }


    result = CalculateBufferSettings( &framesPerHostInputBuffer, &hostInputBufferCount,
                &framesPerHostOutputBuffer, &hostOutputBufferCount,
                inputChannelCount, hostInputSampleFormat, suggestedInputLatency, inputStreamInfo,
                outputChannelCount, hostOutputSampleFormat, suggestedOutputLatency, outputStreamInfo,
                sampleRate, framesPerBuffer );
    if( result != paNoError ) goto error;


    stream = (PaWinMmeStream*)PaUtil_AllocateMemory( sizeof(PaWinMmeStream) );
    if( !stream )
    {
        result = paInsufficientMemory;
        goto error;
    }

    InitializeSingleDirectionHandlesAndBuffers( &stream->input );
    InitializeSingleDirectionHandlesAndBuffers( &stream->output );

    stream->abortEvent = 0;
    stream->processingThread = 0;

    stream->throttleProcessingThreadOnOverload = throttleProcessingThreadOnOverload;

    PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                           ( (streamCallback)
                                            ? &winMmeHostApi->callbackStreamInterface
                                            : &winMmeHostApi->blockingStreamInterface ),
                                           streamCallback, userData );
    streamRepresentationIsInitialized = 1;

    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );


    if( inputParameters && outputParameters ) /* full duplex */
    {
        if( framesPerHostInputBuffer < framesPerHostOutputBuffer )
        {
            assert( (framesPerHostOutputBuffer % framesPerHostInputBuffer) == 0 ); /* CalculateBufferSettings() should guarantee this condition */

            framesPerBufferProcessorCall = framesPerHostInputBuffer;
        }
        else
        {
            assert( (framesPerHostInputBuffer % framesPerHostOutputBuffer) == 0 ); /* CalculateBufferSettings() should guarantee this condition */
            
            framesPerBufferProcessorCall = framesPerHostOutputBuffer;
        }
    }
    else if( inputParameters )
    {
        framesPerBufferProcessorCall = framesPerHostInputBuffer;
    }
    else if( outputParameters )
    {
        framesPerBufferProcessorCall = framesPerHostOutputBuffer;
    }

    stream->input.framesPerBuffer = framesPerHostInputBuffer;
    stream->output.framesPerBuffer = framesPerHostOutputBuffer;

    result =  PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
                    inputChannelCount, inputSampleFormat, hostInputSampleFormat,
                    outputChannelCount, outputSampleFormat, hostOutputSampleFormat,
                    sampleRate, streamFlags, framesPerBuffer,
                    framesPerBufferProcessorCall, paUtilFixedHostBufferSize,
                    streamCallback, userData );
    if( result != paNoError ) goto error;
    
    bufferProcessorIsInitialized = 1;

    /* stream info input latency is the minimum buffering latency (unlike suggested and default which are *maximums*) */
    stream->streamRepresentation.streamInfo.inputLatency =
            (double)(PaUtil_GetBufferProcessorInputLatencyFrames(&stream->bufferProcessor)
                + framesPerHostInputBuffer) / sampleRate;
    stream->streamRepresentation.streamInfo.outputLatency =
            (double)(PaUtil_GetBufferProcessorOutputLatencyFrames(&stream->bufferProcessor)
                + (framesPerHostOutputBuffer * (hostOutputBufferCount-1))) / sampleRate;
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

    stream->primeStreamUsingCallback = ( (streamFlags&paPrimeOutputBuffersUsingStreamCallback) && streamCallback ) ? 1 : 0;

    /* time to sleep when throttling due to >100% cpu usage.
        -a quater of a buffer's duration */
    stream->throttledSleepMsecs =
            (unsigned long)(stream->bufferProcessor.framesPerHostBuffer *
             stream->bufferProcessor.samplePeriod * .25 * 1000);

    stream->isStopped = 1;
    stream->isActive = 0;


    /* for maximum compatibility with multi-device multichannel drivers,
        we first open all devices, then we prepare all buffers, finally
        we start all devices ( in StartStream() ). teardown in reverse order.
    */

    if( inputParameters )
    {
        result = InitializeWaveHandles( winMmeHostApi, &stream->input,
                winMmeSpecificInputFlags,
                stream->bufferProcessor.bytesPerHostInputSample, sampleRate,
                inputDevices, inputDeviceCount, inputChannelMask, 1 /* isInput */ );
        if( result != paNoError ) goto error;
    }
    
    if( outputParameters )
    {
        result = InitializeWaveHandles( winMmeHostApi, &stream->output,
                winMmeSpecificOutputFlags,
                stream->bufferProcessor.bytesPerHostOutputSample, sampleRate,
                outputDevices, outputDeviceCount, outputChannelMask, 0 /* isInput */ );
        if( result != paNoError ) goto error;
    }

    if( inputParameters )
    {
        result = InitializeWaveHeaders( &stream->input, hostInputBufferCount,
                hostInputSampleFormat, framesPerHostInputBuffer, inputDevices, 1 /* isInput */ );
        if( result != paNoError ) goto error;
    }

    if( outputParameters )
    {
        result = InitializeWaveHeaders( &stream->output, hostOutputBufferCount,
                hostOutputSampleFormat, framesPerHostOutputBuffer, outputDevices, 0 /* not isInput */ );
        if( result != paNoError ) goto error;

        stream->allBuffersDurationMs = (DWORD) (1000.0 * (framesPerHostOutputBuffer * stream->output.bufferCount) / sampleRate);
    }
    else
    {
        stream->allBuffersDurationMs = (DWORD) (1000.0 * (framesPerHostInputBuffer * stream->input.bufferCount) / sampleRate);
    }

    
    if( streamCallback )
    {
        /* abort event is only needed for callback streams */
        result = CreateEventWithPaError( &stream->abortEvent, NULL, TRUE, FALSE, NULL );
        if( result != paNoError ) goto error;
    }

    *s = (PaStream*)stream;

    return result;

error:

    if( stream )
    {
        if( stream->abortEvent )
            CloseHandle( stream->abortEvent );
            
        TerminateWaveHeaders( &stream->output, 0 /* not isInput */ );
        TerminateWaveHeaders( &stream->input, 1 /* isInput */ );

        TerminateWaveHandles( &stream->output, 0 /* not isInput */, 1 /* currentlyProcessingAnError */ );
        TerminateWaveHandles( &stream->input, 1 /* isInput */, 1 /* currentlyProcessingAnError */ );

        if( bufferProcessorIsInitialized )
            PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );

        if( streamRepresentationIsInitialized )
            PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );

        PaUtil_FreeMemory( stream );
    }

    return result;
}


/* return non-zero if all current buffers are done */
static int BuffersAreDone( WAVEHDR **waveHeaders, unsigned int deviceCount, int bufferIndex )
{
    unsigned int i;
    
    for( i=0; i < deviceCount; ++i )
    {
        if( !(waveHeaders[i][ bufferIndex ].dwFlags & WHDR_DONE) )
        {
            return 0;
        }         
    }

    return 1;
}

static int CurrentInputBuffersAreDone( PaWinMmeStream *stream )
{
    return BuffersAreDone( stream->input.waveHeaders, stream->input.deviceCount, stream->input.currentBufferIndex );
}

static int CurrentOutputBuffersAreDone( PaWinMmeStream *stream )
{
    return BuffersAreDone( stream->output.waveHeaders, stream->output.deviceCount, stream->output.currentBufferIndex );
}


/* return non-zero if any buffers are queued */
static int NoBuffersAreQueued( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers )
{
    unsigned int i, j;

    if( handlesAndBuffers->waveHandles )
    {
        for( i=0; i < handlesAndBuffers->bufferCount; ++i )
        {
            for( j=0; j < handlesAndBuffers->deviceCount; ++j )
            {
                if( !( handlesAndBuffers->waveHeaders[ j ][ i ].dwFlags & WHDR_DONE) )
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}


#define PA_CIRCULAR_INCREMENT_( current, max )\
    ( (((current) + 1) >= (max)) ? (0) : (current+1) )

#define PA_CIRCULAR_DECREMENT_( current, max )\
    ( ((current) == 0) ? ((max)-1) : (current-1) )
    

static signed long GetAvailableFrames( PaWinMmeSingleDirectionHandlesAndBuffers *handlesAndBuffers )
{
    signed long result = 0;
    unsigned int i;
    
    if( BuffersAreDone( handlesAndBuffers->waveHeaders, handlesAndBuffers->deviceCount, handlesAndBuffers->currentBufferIndex ) )
    {
        /* we could calculate the following in O(1) if we kept track of the
            last done buffer */
        result = handlesAndBuffers->framesPerBuffer - handlesAndBuffers->framesUsedInCurrentBuffer;

        i = PA_CIRCULAR_INCREMENT_( handlesAndBuffers->currentBufferIndex, handlesAndBuffers->bufferCount );
        while( i != handlesAndBuffers->currentBufferIndex )
        {
            if( BuffersAreDone( handlesAndBuffers->waveHeaders, handlesAndBuffers->deviceCount, i ) )
            {
                result += handlesAndBuffers->framesPerBuffer;
                i = PA_CIRCULAR_INCREMENT_( i, handlesAndBuffers->bufferCount );
            }
            else
                break;
        }
    }

    return result;
}


static PaError AdvanceToNextInputBuffer( PaWinMmeStream *stream )
{
    PaError result = paNoError;
    MMRESULT mmresult;
    unsigned int i;

    for( i=0; i < stream->input.deviceCount; ++i )
    {
        stream->input.waveHeaders[i][ stream->input.currentBufferIndex ].dwFlags &= ~WHDR_DONE;
        mmresult = waveInAddBuffer( ((HWAVEIN*)stream->input.waveHandles)[i],
                                    &stream->input.waveHeaders[i][ stream->input.currentBufferIndex ],
                                    sizeof(WAVEHDR) );
        if( mmresult != MMSYSERR_NOERROR )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
        }
    }

    stream->input.currentBufferIndex =
            PA_CIRCULAR_INCREMENT_( stream->input.currentBufferIndex, stream->input.bufferCount );

    stream->input.framesUsedInCurrentBuffer = 0;

    return result;
}


static PaError AdvanceToNextOutputBuffer( PaWinMmeStream *stream )
{
    PaError result = paNoError;
    MMRESULT mmresult;
    unsigned int i;

    for( i=0; i < stream->output.deviceCount; ++i )
    {
        mmresult = waveOutWrite( ((HWAVEOUT*)stream->output.waveHandles)[i],
                                 &stream->output.waveHeaders[i][ stream->output.currentBufferIndex ],
                                 sizeof(WAVEHDR) );
        if( mmresult != MMSYSERR_NOERROR )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
        }
    }

    stream->output.currentBufferIndex =
            PA_CIRCULAR_INCREMENT_( stream->output.currentBufferIndex, stream->output.bufferCount );

    stream->output.framesUsedInCurrentBuffer = 0;
    
    return result;
}


/* requeue all but the most recent input with the driver. Used for catching
    up after a total input buffer underrun */
static PaError CatchUpInputBuffers( PaWinMmeStream *stream )
{
    PaError result = paNoError;
    unsigned int i;
    
    for( i=0; i < stream->input.bufferCount - 1; ++i )
    {
        result = AdvanceToNextInputBuffer( stream );
        if( result != paNoError )
            break;
    }

    return result;
}


/* take the most recent output and duplicate it to all other output buffers
    and requeue them. Used for catching up after a total output buffer underrun.
*/
static PaError CatchUpOutputBuffers( PaWinMmeStream *stream )
{
    PaError result = paNoError;
    unsigned int i, j;
    unsigned int previousBufferIndex =
            PA_CIRCULAR_DECREMENT_( stream->output.currentBufferIndex, stream->output.bufferCount );

    for( i=0; i < stream->output.bufferCount - 1; ++i )
    {
        for( j=0; j < stream->output.deviceCount; ++j )
        {
            if( stream->output.waveHeaders[j][ stream->output.currentBufferIndex ].lpData
                    != stream->output.waveHeaders[j][ previousBufferIndex ].lpData )
            {
                CopyMemory( stream->output.waveHeaders[j][ stream->output.currentBufferIndex ].lpData,
                            stream->output.waveHeaders[j][ previousBufferIndex ].lpData,
                            stream->output.waveHeaders[j][ stream->output.currentBufferIndex ].dwBufferLength );
            }
        }

        result = AdvanceToNextOutputBuffer( stream );
        if( result != paNoError )
            break;
    }

    return result;
}


PA_THREAD_FUNC ProcessingThreadProc( void *pArg )
{
    PaWinMmeStream *stream = (PaWinMmeStream *)pArg;
    HANDLE events[3];
    int eventCount = 0;
    DWORD result = paNoError;
    DWORD waitResult;
    DWORD timeout = (unsigned long)(stream->allBuffersDurationMs * 0.5);
    int hostBuffersAvailable;
    signed int hostInputBufferIndex, hostOutputBufferIndex;
    PaStreamCallbackFlags statusFlags;
    int callbackResult;
    int done = 0;
    unsigned int channel, i;
    unsigned long framesProcessed;
    
    /* prepare event array for call to WaitForMultipleObjects() */
    if( stream->input.bufferEvent )
        events[eventCount++] = stream->input.bufferEvent;
    if( stream->output.bufferEvent )
        events[eventCount++] = stream->output.bufferEvent;
    events[eventCount++] = stream->abortEvent;

    statusFlags = 0; /** @todo support paInputUnderflow, paOutputOverflow and paNeverDropInput */
    
    /* loop until something causes us to stop */
    do{
        /* wait for MME to signal that a buffer is available, or for
            the PA abort event to be signaled.

          When this indicates that one or more buffers are available
          NoBuffersAreQueued() and Current*BuffersAreDone are used below to
          poll for additional done buffers. NoBuffersAreQueued() will fail
          to identify an underrun/overflow if the driver doesn't mark all done
          buffers prior to signalling the event. Some drivers do this
          (eg RME Digi96, and others don't eg VIA PC 97 input). This isn't a
          huge problem, it just means that we won't always be able to detect
          underflow/overflow.
        */
        waitResult = WaitForMultipleObjects( eventCount, events, FALSE /* wait all = FALSE */, timeout );
        if( waitResult == WAIT_FAILED )
        {
            result = paUnanticipatedHostError;
            /** @todo FIXME/REVIEW: can't return host error info from an asyncronous thread. see http://www.portaudio.com/trac/ticket/143 */
            done = 1;
        }
        else if( waitResult == WAIT_TIMEOUT )
        {
            /* if a timeout is encountered, continue */
        }

        if( stream->abortProcessing )
        {
            /* Pa_AbortStream() has been called, stop processing immediately */
            done = 1;
        }
        else if( stream->stopProcessing )
        {
            /* Pa_StopStream() has been called or the user callback returned
                non-zero, processing will continue until all output buffers
                are marked as done. The stream will stop immediately if it
                is input-only.
            */

            if( PA_IS_OUTPUT_STREAM_(stream) )
            {
                if( NoBuffersAreQueued( &stream->output ) )
                    done = 1; /* Will cause thread to return. */
            }
            else
            {
                /* input only stream */
                done = 1; /* Will cause thread to return. */
            }
        }
        else
        {
            hostBuffersAvailable = 1;

            /* process all available host buffers */
            do
            {
                hostInputBufferIndex = -1;
                hostOutputBufferIndex = -1;
                
                if( PA_IS_INPUT_STREAM_(stream) )
                {
                    if( CurrentInputBuffersAreDone( stream ) )
                    {
                        if( NoBuffersAreQueued( &stream->input ) )
                        {
                            /** @todo
                               if all of the other buffers are also ready then
                               we discard all but the most recent. This is an
                               input buffer overflow. FIXME: these buffers should
                               be passed to the callback in a paNeverDropInput
                               stream. http://www.portaudio.com/trac/ticket/142

                               note that it is also possible for an input overflow
                               to happen while the callback is processing a buffer.
                               that is handled further down.
                            */
                            result = CatchUpInputBuffers( stream );
                            if( result != paNoError )
                                done = 1;

                            statusFlags |= paInputOverflow;
                        }

                        hostInputBufferIndex = stream->input.currentBufferIndex;
                    }
                }

                if( PA_IS_OUTPUT_STREAM_(stream) )
                {
                    if( CurrentOutputBuffersAreDone( stream ) )
                    {
                        /* ok, we have an output buffer */
                        
                        if( NoBuffersAreQueued( &stream->output ) )
                        {
                            /*
                            if all of the other buffers are also ready, catch up by copying
                            the most recently generated buffer into all but one of the output
                            buffers.

                            note that this catch up code only handles the case where all
                            buffers have been played out due to this thread not having
                            woken up at all. a more common case occurs when this thread
                            is woken up, processes one buffer, but takes too long, and as
                            a result all the other buffers have become un-queued. that
                            case is handled further down.
                            */

                            result = CatchUpOutputBuffers( stream );
                            if( result != paNoError )
                                done = 1;

                            statusFlags |= paOutputUnderflow;
                        }

                        hostOutputBufferIndex = stream->output.currentBufferIndex;
                    }
                }

               
                if( (PA_IS_FULL_DUPLEX_STREAM_(stream) && hostInputBufferIndex != -1 && hostOutputBufferIndex != -1) ||
                        (PA_IS_HALF_DUPLEX_STREAM_(stream) && ( hostInputBufferIndex != -1 || hostOutputBufferIndex != -1 ) ) )
                {
                    PaStreamCallbackTimeInfo timeInfo = {0,0,0}; /** @todo implement inputBufferAdcTime */


                    if( PA_IS_OUTPUT_STREAM_(stream) )
                    {
                        /* set timeInfo.currentTime and calculate timeInfo.outputBufferDacTime
                            from the current wave out position */
                        MMTIME mmtime;
                        double timeBeforeGetPosition, timeAfterGetPosition;
                        double time;
                        long framesInBufferRing; 		
                        long writePosition;
                        long playbackPosition;
                        HWAVEOUT firstWaveOutDevice = ((HWAVEOUT*)stream->output.waveHandles)[0];
                        
                        mmtime.wType = TIME_SAMPLES;
                        timeBeforeGetPosition = PaUtil_GetTime();
                        waveOutGetPosition( firstWaveOutDevice, &mmtime, sizeof(MMTIME) );
                        timeAfterGetPosition = PaUtil_GetTime();

                        timeInfo.currentTime = timeAfterGetPosition;

                        /* approximate time at which wave out position was measured
                            as half way between timeBeforeGetPosition and timeAfterGetPosition */
                        time = timeBeforeGetPosition + (timeAfterGetPosition - timeBeforeGetPosition) * .5;
                        
                        framesInBufferRing = stream->output.bufferCount * stream->bufferProcessor.framesPerHostBuffer;
                        playbackPosition = mmtime.u.sample % framesInBufferRing;

                        writePosition = stream->output.currentBufferIndex * stream->bufferProcessor.framesPerHostBuffer
                                + stream->output.framesUsedInCurrentBuffer;
                       
                        if( playbackPosition >= writePosition ){
                            timeInfo.outputBufferDacTime =
                                    time + ((double)( writePosition + (framesInBufferRing - playbackPosition) ) * stream->bufferProcessor.samplePeriod );
                        }else{
                            timeInfo.outputBufferDacTime =
                                    time + ((double)( writePosition - playbackPosition ) * stream->bufferProcessor.samplePeriod );
                        }
                    }


                    PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

                    PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo, statusFlags  );

                    /* reset status flags once they have been passed to the buffer processor */
                    statusFlags = 0;

                    if( PA_IS_INPUT_STREAM_(stream) )
                    {
                        PaUtil_SetInputFrameCount( &stream->bufferProcessor, 0 /* default to host buffer size */ );

                        channel = 0;
                        for( i=0; i<stream->input.deviceCount; ++i )
                        {
                             /* we have stored the number of channels in the buffer in dwUser */
                            int channelCount = (int)stream->input.waveHeaders[i][ hostInputBufferIndex ].dwUser;
                            
                            PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor, channel,
                                    stream->input.waveHeaders[i][ hostInputBufferIndex ].lpData +
                                        stream->input.framesUsedInCurrentBuffer * channelCount *
                                        stream->bufferProcessor.bytesPerHostInputSample,
                                    channelCount );
                                    

                            channel += channelCount;
                        }
                    }

                    if( PA_IS_OUTPUT_STREAM_(stream) )
                    {
                        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, 0 /* default to host buffer size */ );
                        
                        channel = 0;
                        for( i=0; i<stream->output.deviceCount; ++i )
                        {
                            /* we have stored the number of channels in the buffer in dwUser */
                            int channelCount = (int)stream->output.waveHeaders[i][ hostOutputBufferIndex ].dwUser;

                            PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, channel,
                                    stream->output.waveHeaders[i][ hostOutputBufferIndex ].lpData +
                                        stream->output.framesUsedInCurrentBuffer * channelCount *
                                        stream->bufferProcessor.bytesPerHostOutputSample,
                                    channelCount );

                            channel += channelCount;
                        }
                    }

                    callbackResult = paContinue;
                    framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor, &callbackResult );

                    stream->input.framesUsedInCurrentBuffer += framesProcessed;
                    stream->output.framesUsedInCurrentBuffer += framesProcessed;

                    PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );

                    if( callbackResult == paContinue )
                    {
                        /* nothing special to do */
                    }
                    else if( callbackResult == paAbort )
                    {
                        stream->abortProcessing = 1;
                        done = 1;
                        /** @todo FIXME: should probably reset the output device immediately once the callback returns paAbort 
                            see: http://www.portaudio.com/trac/ticket/141
                        */
                        result = paNoError;
                    }
                    else
                    {
                        /* User callback has asked us to stop with paComplete or other non-zero value */
                        stream->stopProcessing = 1; /* stop once currently queued audio has finished */
                        result = paNoError;
                    }


                    if( PA_IS_INPUT_STREAM_(stream)
                            && stream->stopProcessing == 0 && stream->abortProcessing == 0
                            && stream->input.framesUsedInCurrentBuffer == stream->input.framesPerBuffer )
                    {
                        if( NoBuffersAreQueued( &stream->input ) )
                        {
                            /** @todo need to handle PaNeverDropInput here where necessary */
                            result = CatchUpInputBuffers( stream );
                            if( result != paNoError )
                                done = 1;

                            statusFlags |= paInputOverflow;
                        }

                        result = AdvanceToNextInputBuffer( stream );
                        if( result != paNoError )
                            done = 1;
                    }

                    
                    if( PA_IS_OUTPUT_STREAM_(stream) && !stream->abortProcessing )
                    {
                        if( stream->stopProcessing &&
                                stream->output.framesUsedInCurrentBuffer < stream->output.framesPerBuffer )
                        {
                            /* zero remaining samples in output output buffer and flush */

                            stream->output.framesUsedInCurrentBuffer += PaUtil_ZeroOutput( &stream->bufferProcessor,
                                    stream->output.framesPerBuffer - stream->output.framesUsedInCurrentBuffer );

                            /* we send the entire buffer to the output devices, but we could
                                just send a partial buffer, rather than zeroing the unused
                                samples.
                            */
                        }

                        if( stream->output.framesUsedInCurrentBuffer == stream->output.framesPerBuffer )
                        {
                            /* check for underflow before enquing the just-generated buffer,
                                but recover from underflow after enquing it. This ensures
                                that the most recent audio segment is repeated */
                            int outputUnderflow = NoBuffersAreQueued( &stream->output );

                            result = AdvanceToNextOutputBuffer( stream );
                            if( result != paNoError )
                                done = 1;

                            if( outputUnderflow && !done && !stream->stopProcessing )
                            {
                                /* Recover from underflow in the case where the
                                    underflow occured while processing the buffer
                                    we just finished */

                                result = CatchUpOutputBuffers( stream );
                                if( result != paNoError )
                                    done = 1;

                                statusFlags |= paOutputUnderflow;
                            }
                        }
                    }
                    
                    if( stream->throttleProcessingThreadOnOverload != 0 )
                    {
                        if( stream->stopProcessing || stream->abortProcessing )
                        {
                            if( stream->processingThreadPriority != stream->highThreadPriority )
                            {
                                SetThreadPriority( stream->processingThread, stream->highThreadPriority );
                                stream->processingThreadPriority = stream->highThreadPriority;
                            }
                        }
                        else if( PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer ) > 1. )
                        {
                            if( stream->processingThreadPriority != stream->throttledThreadPriority )
                            {
                                SetThreadPriority( stream->processingThread, stream->throttledThreadPriority );
                                stream->processingThreadPriority = stream->throttledThreadPriority;
                            }

                            /* sleep to give other processes a go */
                            Sleep( stream->throttledSleepMsecs );
                        }
                        else
                        {
                            if( stream->processingThreadPriority != stream->highThreadPriority )
                            {
                                SetThreadPriority( stream->processingThread, stream->highThreadPriority );
                                stream->processingThreadPriority = stream->highThreadPriority;
                            }
                        }
                    }
                }
                else
                {
                    hostBuffersAvailable = 0;
                }
            }
            while( hostBuffersAvailable &&
                    stream->stopProcessing == 0 &&
                    stream->abortProcessing == 0 &&
                    !done );
        }
    }
    while( !done );

    stream->isActive = 0;

    if( stream->streamRepresentation.streamFinishedCallback != 0 )
        stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );

    PaUtil_ResetCpuLoadMeasurer( &stream->cpuLoadMeasurer );
    
    return result;
}


/*
    When CloseStream() is called, the multi-api layer ensures that
    the stream has already been stopped or aborted.
*/
static PaError CloseStream( PaStream* s )
{
    PaError result;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;

    result = CloseHandleWithPaError( stream->abortEvent );
    if( result != paNoError ) goto error;
    
    TerminateWaveHeaders( &stream->output, 0 /* not isInput */ );
    TerminateWaveHeaders( &stream->input, 1 /* isInput */ );

    TerminateWaveHandles( &stream->output, 0 /* not isInput */, 0 /* not currentlyProcessingAnError */ );
    TerminateWaveHandles( &stream->input, 1 /* isInput */, 0 /* not currentlyProcessingAnError */ );
    
    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );
    PaUtil_FreeMemory( stream );

error:
    /** @todo REVIEW: what is the best way to clean up a stream if an error is detected? */
    return result;
}


static PaError StartStream( PaStream *s )
{
    PaError result;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    MMRESULT mmresult;
    unsigned int i, j;
    int callbackResult;
	unsigned int channel;
 	unsigned long framesProcessed;
	PaStreamCallbackTimeInfo timeInfo = {0,0,0}; /** @todo implement this for stream priming */
    
    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );
    
    if( PA_IS_INPUT_STREAM_(stream) )
    {
        for( i=0; i<stream->input.bufferCount; ++i )
        {
            for( j=0; j<stream->input.deviceCount; ++j )
            {
                stream->input.waveHeaders[j][i].dwFlags &= ~WHDR_DONE;
                mmresult = waveInAddBuffer( ((HWAVEIN*)stream->input.waveHandles)[j], &stream->input.waveHeaders[j][i], sizeof(WAVEHDR) );
                if( mmresult != MMSYSERR_NOERROR )
                {
                    result = paUnanticipatedHostError;
                    PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                    goto error;
                }
            }
        }
        stream->input.currentBufferIndex = 0;
        stream->input.framesUsedInCurrentBuffer = 0;
    }

    if( PA_IS_OUTPUT_STREAM_(stream) )
    {
        for( i=0; i<stream->output.deviceCount; ++i )
        {
            if( (mmresult = waveOutPause( ((HWAVEOUT*)stream->output.waveHandles)[i] )) != MMSYSERR_NOERROR )
            {
                result = paUnanticipatedHostError;
                PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                goto error;
            }
        }

        for( i=0; i<stream->output.bufferCount; ++i )
        {
            if( stream->primeStreamUsingCallback )
            {

                stream->output.framesUsedInCurrentBuffer = 0;
                do{

                    PaUtil_BeginBufferProcessing( &stream->bufferProcessor,
                            &timeInfo,
                            paPrimingOutput | ((stream->input.bufferCount > 0 ) ? paInputUnderflow : 0));

                    if( stream->input.bufferCount > 0 )
                        PaUtil_SetNoInput( &stream->bufferProcessor );

                    PaUtil_SetOutputFrameCount( &stream->bufferProcessor, 0 /* default to host buffer size */ );

                    channel = 0;
                    for( j=0; j<stream->output.deviceCount; ++j )
                    {
                        /* we have stored the number of channels in the buffer in dwUser */
                        int channelCount = (int)stream->output.waveHeaders[j][i].dwUser;

                        PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, channel,
                                stream->output.waveHeaders[j][i].lpData +
                                stream->output.framesUsedInCurrentBuffer * channelCount *
                                stream->bufferProcessor.bytesPerHostOutputSample,
                                channelCount );

                        /* we have stored the number of channels in the buffer in dwUser */
                        channel += channelCount;
                    }

                    callbackResult = paContinue;
                    framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor, &callbackResult );
                    stream->output.framesUsedInCurrentBuffer += framesProcessed;

                    if( callbackResult != paContinue )
                    {
                        /** @todo fix this, what do we do if callback result is non-zero during stream
                            priming?

                            for complete: play out primed waveHeaders as usual
                            for abort: clean up immediately.
                       */
                    }

                }while( stream->output.framesUsedInCurrentBuffer != stream->output.framesPerBuffer );

            }
            else
            {
                for( j=0; j<stream->output.deviceCount; ++j )
                {
                    ZeroMemory( stream->output.waveHeaders[j][i].lpData, stream->output.waveHeaders[j][i].dwBufferLength );
                }
            }   

            /* we queue all channels of a single buffer frame (accross all
                devices, because some multidevice multichannel drivers work
                better this way */
            for( j=0; j<stream->output.deviceCount; ++j )
            {
                mmresult = waveOutWrite( ((HWAVEOUT*)stream->output.waveHandles)[j], &stream->output.waveHeaders[j][i], sizeof(WAVEHDR) );
                if( mmresult != MMSYSERR_NOERROR )
                {
                    result = paUnanticipatedHostError;
                    PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                    goto error;
                }
            }
        }
        stream->output.currentBufferIndex = 0;
        stream->output.framesUsedInCurrentBuffer = 0;
    }


    stream->isStopped = 0;
    stream->isActive = 1;
    stream->stopProcessing = 0;
    stream->abortProcessing = 0;

    result = ResetEventWithPaError( stream->input.bufferEvent );
    if( result != paNoError ) goto error;

    result = ResetEventWithPaError( stream->output.bufferEvent );
    if( result != paNoError ) goto error;
    
    
    if( stream->streamRepresentation.streamCallback )
    {
        /* callback stream */

        result = ResetEventWithPaError( stream->abortEvent );
        if( result != paNoError ) goto error;

        /* Create thread that waits for audio buffers to be ready for processing. */
        stream->processingThread = CREATE_THREAD;
        if( !stream->processingThread )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_SYSTEM_ERROR( GetLastError() );
            goto error;
        }

        /** @todo could have mme specific stream parameters to allow the user
            to set the callback thread priorities */
        stream->highThreadPriority = THREAD_PRIORITY_TIME_CRITICAL;
        stream->throttledThreadPriority = THREAD_PRIORITY_NORMAL;

        if( !SetThreadPriority( stream->processingThread, stream->highThreadPriority ) )
        {
            result = paUnanticipatedHostError;
            PA_MME_SET_LAST_SYSTEM_ERROR( GetLastError() );
            goto error;
        }
        stream->processingThreadPriority = stream->highThreadPriority;
    }
    else
    {
        /* blocking read/write stream */

    }

    if( PA_IS_INPUT_STREAM_(stream) )
    {
        for( i=0; i < stream->input.deviceCount; ++i )
        {
            mmresult = waveInStart( ((HWAVEIN*)stream->input.waveHandles)[i] );
            PA_DEBUG(("Pa_StartStream: waveInStart returned = 0x%X.\n", mmresult));
            if( mmresult != MMSYSERR_NOERROR )
            {
                result = paUnanticipatedHostError;
                PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                goto error;
            }
        }
    }

    if( PA_IS_OUTPUT_STREAM_(stream) )
    {
        for( i=0; i < stream->output.deviceCount; ++i )
        {
            if( (mmresult = waveOutRestart( ((HWAVEOUT*)stream->output.waveHandles)[i] )) != MMSYSERR_NOERROR )
            {
                result = paUnanticipatedHostError;
                PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                goto error;
            }
        }
    }

    return result;

error:
    /** @todo FIXME: implement recovery as best we can
    This should involve rolling back to a state as-if this function had never been called
    */
    return result;
}


static PaError StopStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    int timeout;
    DWORD waitResult;
    MMRESULT mmresult;
    signed int hostOutputBufferIndex;
    unsigned int channel, waitCount, i;                  
    
    /** @todo
        REVIEW: the error checking in this function needs review. the basic
        idea is to return from this function in a known state - for example
        there is no point avoiding calling waveInReset just because
        the thread times out.
    */

    if( stream->processingThread )
    {
        /* callback stream */

        /* Tell processing thread to stop generating more data and to let current data play out. */
        stream->stopProcessing = 1;

        /* Calculate timeOut longer than longest time it could take to return all buffers. */
        timeout = (int)(stream->allBuffersDurationMs * 1.5);
        if( timeout < PA_MME_MIN_TIMEOUT_MSEC_ )
            timeout = PA_MME_MIN_TIMEOUT_MSEC_;

        PA_DEBUG(("WinMME StopStream: waiting for background thread.\n"));

        waitResult = WaitForSingleObject( stream->processingThread, timeout );
        if( waitResult == WAIT_TIMEOUT )
        {
            /* try to abort */
            stream->abortProcessing = 1;
            SetEvent( stream->abortEvent );
            waitResult = WaitForSingleObject( stream->processingThread, timeout );
            if( waitResult == WAIT_TIMEOUT )
            {
                PA_DEBUG(("WinMME StopStream: timed out while waiting for background thread to finish.\n"));
                result = paTimedOut;
            }
        }

        CloseHandle( stream->processingThread );
        stream->processingThread = NULL;
    }
    else
    {
        /* blocking read / write stream */

        if( PA_IS_OUTPUT_STREAM_(stream) )
        {
            if( stream->output.framesUsedInCurrentBuffer > 0 )
            {
                /* there are still unqueued frames in the current buffer, so flush them */

                hostOutputBufferIndex = stream->output.currentBufferIndex;

                PaUtil_SetOutputFrameCount( &stream->bufferProcessor,
                        stream->output.framesPerBuffer - stream->output.framesUsedInCurrentBuffer );
                
                channel = 0;
                for( i=0; i<stream->output.deviceCount; ++i )
                {
                    /* we have stored the number of channels in the buffer in dwUser */
                    int channelCount = (int)stream->output.waveHeaders[i][ hostOutputBufferIndex ].dwUser;

                    PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, channel,
                            stream->output.waveHeaders[i][ hostOutputBufferIndex ].lpData +
                                stream->output.framesUsedInCurrentBuffer * channelCount *
                                stream->bufferProcessor.bytesPerHostOutputSample,
                            channelCount );

                    channel += channelCount;
                }

                PaUtil_ZeroOutput( &stream->bufferProcessor,
                        stream->output.framesPerBuffer - stream->output.framesUsedInCurrentBuffer );

                /* we send the entire buffer to the output devices, but we could
                    just send a partial buffer, rather than zeroing the unused
                    samples.
                */
                AdvanceToNextOutputBuffer( stream );
            }
            

            timeout = (stream->allBuffersDurationMs / stream->output.bufferCount) + 1;
            if( timeout < PA_MME_MIN_TIMEOUT_MSEC_ )
                timeout = PA_MME_MIN_TIMEOUT_MSEC_;

            waitCount = 0;
            while( !NoBuffersAreQueued( &stream->output ) && waitCount <= stream->output.bufferCount )
            {
                /* wait for MME to signal that a buffer is available */
                waitResult = WaitForSingleObject( stream->output.bufferEvent, timeout );
                if( waitResult == WAIT_FAILED )
                {
                    break;
                }
                else if( waitResult == WAIT_TIMEOUT )
                {
                    /* keep waiting */
                }

                ++waitCount;
            }
        }
    }

    if( PA_IS_OUTPUT_STREAM_(stream) )
    {
        for( i =0; i < stream->output.deviceCount; ++i )
        {
            mmresult = waveOutReset( ((HWAVEOUT*)stream->output.waveHandles)[i] );
            if( mmresult != MMSYSERR_NOERROR )
            {
                result = paUnanticipatedHostError;
                PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
            }
        }
    }

    if( PA_IS_INPUT_STREAM_(stream) )
    {
        for( i=0; i < stream->input.deviceCount; ++i )
        {
            mmresult = waveInReset( ((HWAVEIN*)stream->input.waveHandles)[i] );
            if( mmresult != MMSYSERR_NOERROR )
            {
                result = paUnanticipatedHostError;
                PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
            }
        }
    }

    stream->isStopped = 1;
    stream->isActive = 0;

    return result;
}


static PaError AbortStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    int timeout;
    DWORD waitResult;
    MMRESULT mmresult;
    unsigned int i;
    
    /** @todo
        REVIEW: the error checking in this function needs review. the basic
        idea is to return from this function in a known state - for example
        there is no point avoiding calling waveInReset just because
        the thread times out.
    */

    if( stream->processingThread )
    {
        /* callback stream */
        
        /* Tell processing thread to abort immediately */
        stream->abortProcessing = 1;
        SetEvent( stream->abortEvent );
    }


    if( PA_IS_OUTPUT_STREAM_(stream) )
    {
        for( i =0; i < stream->output.deviceCount; ++i )
        {
            mmresult = waveOutReset( ((HWAVEOUT*)stream->output.waveHandles)[i] );
            if( mmresult != MMSYSERR_NOERROR )
            {
                PA_MME_SET_LAST_WAVEOUT_ERROR( mmresult );
                return paUnanticipatedHostError;
            }
        }
    }

    if( PA_IS_INPUT_STREAM_(stream) )
    {
        for( i=0; i < stream->input.deviceCount; ++i )
        {
            mmresult = waveInReset( ((HWAVEIN*)stream->input.waveHandles)[i] );
            if( mmresult != MMSYSERR_NOERROR )
            {
                PA_MME_SET_LAST_WAVEIN_ERROR( mmresult );
                return paUnanticipatedHostError;
            }
        }
    }


    if( stream->processingThread )
    {
        /* callback stream */
        
        PA_DEBUG(("WinMME AbortStream: waiting for background thread.\n"));

        /* Calculate timeOut longer than longest time it could take to return all buffers. */
        timeout = (int)(stream->allBuffersDurationMs * 1.5);
        if( timeout < PA_MME_MIN_TIMEOUT_MSEC_ )
            timeout = PA_MME_MIN_TIMEOUT_MSEC_;
            
        waitResult = WaitForSingleObject( stream->processingThread, timeout );
        if( waitResult == WAIT_TIMEOUT )
        {
            PA_DEBUG(("WinMME AbortStream: timed out while waiting for background thread to finish.\n"));
            return paTimedOut;
        }

        CloseHandle( stream->processingThread );
        stream->processingThread = NULL;
    }

    stream->isStopped = 1;
    stream->isActive = 0;

    return result;
}


static PaError IsStreamStopped( PaStream *s )
{
    PaWinMmeStream *stream = (PaWinMmeStream*)s;

    return stream->isStopped;
}


static PaError IsStreamActive( PaStream *s )
{
    PaWinMmeStream *stream = (PaWinMmeStream*)s;

    return stream->isActive;
}


static PaTime GetStreamTime( PaStream *s )
{
    (void) s; /* unused parameter */
    
    return PaUtil_GetTime();
}


static double GetStreamCpuLoad( PaStream* s )
{
    PaWinMmeStream *stream = (PaWinMmeStream*)s;

    return PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
}


/*
    As separate stream interfaces are used for blocking and callback
    streams, the following functions can be guaranteed to only be called
    for blocking streams.
*/

static PaError ReadStream( PaStream* s,
                           void *buffer,
                           unsigned long frames )
{
    PaError result = paNoError;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    void *userBuffer;
    unsigned long framesRead = 0;
    unsigned long framesProcessed;
    signed int hostInputBufferIndex;
    DWORD waitResult;
    DWORD timeout = (unsigned long)(stream->allBuffersDurationMs * 0.5);
    unsigned int channel, i;
    
    if( PA_IS_INPUT_STREAM_(stream) )
    {
        /* make a local copy of the user buffer pointer(s). this is necessary
            because PaUtil_CopyInput() advances these pointers every time
            it is called.
        */
        if( stream->bufferProcessor.userInputIsInterleaved )
        {
            userBuffer = buffer;
        }
        else
        {
            userBuffer = (void*)alloca( sizeof(void*) * stream->bufferProcessor.inputChannelCount );
            if( !userBuffer )
                return paInsufficientMemory;
            for( i = 0; i<stream->bufferProcessor.inputChannelCount; ++i )
                ((void**)userBuffer)[i] = ((void**)buffer)[i];
        }
        
        do{
            if( CurrentInputBuffersAreDone( stream ) )
            {
                if( NoBuffersAreQueued( &stream->input ) )
                {
                    /** @todo REVIEW: consider what to do if the input overflows.
                        do we requeue all of the buffers? should we be running
                        a thread to make sure they are always queued? 
                        see: http://www.portaudio.com/trac/ticket/117
                        */

                    result = paInputOverflowed;
                }

                hostInputBufferIndex = stream->input.currentBufferIndex;

                PaUtil_SetInputFrameCount( &stream->bufferProcessor,
                        stream->input.framesPerBuffer - stream->input.framesUsedInCurrentBuffer );
                
                channel = 0;
                for( i=0; i<stream->input.deviceCount; ++i )
                {
                    /* we have stored the number of channels in the buffer in dwUser */
                    int channelCount = (int)stream->input.waveHeaders[i][ hostInputBufferIndex ].dwUser;

                    PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor, channel,
                            stream->input.waveHeaders[i][ hostInputBufferIndex ].lpData +
                                stream->input.framesUsedInCurrentBuffer * channelCount *
                                stream->bufferProcessor.bytesPerHostInputSample,
                            channelCount );

                    channel += channelCount;
                }
                
                framesProcessed = PaUtil_CopyInput( &stream->bufferProcessor, &userBuffer, frames - framesRead );

                stream->input.framesUsedInCurrentBuffer += framesProcessed;
                if( stream->input.framesUsedInCurrentBuffer == stream->input.framesPerBuffer )
                {
                    result = AdvanceToNextInputBuffer( stream );
                    if( result != paNoError )
                        break;
                }

                framesRead += framesProcessed;      

            }else{
                /* wait for MME to signal that a buffer is available */
                waitResult = WaitForSingleObject( stream->input.bufferEvent, timeout );
                if( waitResult == WAIT_FAILED )
                {
                    result = paUnanticipatedHostError;
                    break;
                }
                else if( waitResult == WAIT_TIMEOUT )
                {
                    /* if a timeout is encountered, continue,
                        perhaps we should give up eventually
                    */
                }         
            }
        }while( framesRead < frames );
    }
    else
    {
        result = paCanNotReadFromAnOutputOnlyStream;
    }

    return result;
}


static PaError WriteStream( PaStream* s,
                            const void *buffer,
                            unsigned long frames )
{
    PaError result = paNoError;
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    const void *userBuffer;
    unsigned long framesWritten = 0;
    unsigned long framesProcessed;
    signed int hostOutputBufferIndex;
    DWORD waitResult;
    DWORD timeout = (unsigned long)(stream->allBuffersDurationMs * 0.5);
    unsigned int channel, i;

        
    if( PA_IS_OUTPUT_STREAM_(stream) )
    {
        /* make a local copy of the user buffer pointer(s). this is necessary
            because PaUtil_CopyOutput() advances these pointers every time
            it is called.
        */
        if( stream->bufferProcessor.userOutputIsInterleaved )
        {
            userBuffer = buffer;
        }
        else
        {
            userBuffer = (const void*)alloca( sizeof(void*) * stream->bufferProcessor.outputChannelCount );
            if( !userBuffer )
                return paInsufficientMemory;
            for( i = 0; i<stream->bufferProcessor.outputChannelCount; ++i )
                ((const void**)userBuffer)[i] = ((const void**)buffer)[i];
        }

        do{
            if( CurrentOutputBuffersAreDone( stream ) )
            {
                if( NoBuffersAreQueued( &stream->output ) )
                {
                    /** @todo REVIEW: consider what to do if the output
                    underflows. do we requeue all the existing buffers with
                    zeros? should we run a separate thread to keep the buffers
                    enqueued at all times? 
                    see: http://www.portaudio.com/trac/ticket/117
                    */

                    result = paOutputUnderflowed;
                }

                hostOutputBufferIndex = stream->output.currentBufferIndex;

                PaUtil_SetOutputFrameCount( &stream->bufferProcessor,
                        stream->output.framesPerBuffer - stream->output.framesUsedInCurrentBuffer );
                
                channel = 0;
                for( i=0; i<stream->output.deviceCount; ++i )
                {
                    /* we have stored the number of channels in the buffer in dwUser */
                    int channelCount = (int)stream->output.waveHeaders[i][ hostOutputBufferIndex ].dwUser;

                    PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, channel,
                            stream->output.waveHeaders[i][ hostOutputBufferIndex ].lpData +
                                stream->output.framesUsedInCurrentBuffer * channelCount *
                                stream->bufferProcessor.bytesPerHostOutputSample,
                            channelCount );

                    channel += channelCount;
                }
                
                framesProcessed = PaUtil_CopyOutput( &stream->bufferProcessor, &userBuffer, frames - framesWritten );

                stream->output.framesUsedInCurrentBuffer += framesProcessed;
                if( stream->output.framesUsedInCurrentBuffer == stream->output.framesPerBuffer )
                {
                    result = AdvanceToNextOutputBuffer( stream );
                    if( result != paNoError )
                        break;
                }

                framesWritten += framesProcessed;
            }
            else
            {
                /* wait for MME to signal that a buffer is available */
                waitResult = WaitForSingleObject( stream->output.bufferEvent, timeout );
                if( waitResult == WAIT_FAILED )
                {
                    result = paUnanticipatedHostError;
                    break;
                }
                else if( waitResult == WAIT_TIMEOUT )
                {
                    /* if a timeout is encountered, continue,
                        perhaps we should give up eventually
                    */
                }             
            }        
        }while( framesWritten < frames );
    }
    else
    {
        result = paCanNotWriteToAnInputOnlyStream;
    }
    
    return result;
}


static signed long GetStreamReadAvailable( PaStream* s )
{
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    
    if( PA_IS_INPUT_STREAM_(stream) )
        return GetAvailableFrames( &stream->input );
    else
        return paCanNotReadFromAnOutputOnlyStream;
}


static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaWinMmeStream *stream = (PaWinMmeStream*)s;
    
    if( PA_IS_OUTPUT_STREAM_(stream) )
        return GetAvailableFrames( &stream->output );
    else
        return paCanNotWriteToAnInputOnlyStream;
}


/* NOTE: the following functions are MME-stream specific, and are called directly
    by client code. We need to check for many more error conditions here because
    we don't have the benefit of pa_front.c's parameter checking.
*/

static PaError GetWinMMEStreamPointer( PaWinMmeStream **stream, PaStream *s )
{
    PaError result;
    PaUtilHostApiRepresentation *hostApi;
    PaWinMmeHostApiRepresentation *winMmeHostApi;
    
    result = PaUtil_ValidateStreamPointer( s );
    if( result != paNoError )
        return result;

    result = PaUtil_GetHostApiRepresentation( &hostApi, paMME );
    if( result != paNoError )
        return result;

    winMmeHostApi = (PaWinMmeHostApiRepresentation*)hostApi;
    
    /* note, the following would be easier if there was a generic way of testing
        that a stream belongs to a specific host API */
    
    if( PA_STREAM_REP( s )->streamInterface == &winMmeHostApi->callbackStreamInterface
            || PA_STREAM_REP( s )->streamInterface == &winMmeHostApi->blockingStreamInterface )
    {
        /* s is a WinMME stream */
        *stream = (PaWinMmeStream *)s;
        return paNoError;
    }
    else
    {
        return paIncompatibleStreamHostApi;
    }
}


int PaWinMME_GetStreamInputHandleCount( PaStream* s )
{
    PaWinMmeStream *stream;
    PaError result = GetWinMMEStreamPointer( &stream, s );

    if( result == paNoError )
        return (PA_IS_INPUT_STREAM_(stream)) ? stream->input.deviceCount : 0;
    else
        return result;
}


HWAVEIN PaWinMME_GetStreamInputHandle( PaStream* s, int handleIndex )
{
    PaWinMmeStream *stream;
    PaError result = GetWinMMEStreamPointer( &stream, s );

    if( result == paNoError
            && PA_IS_INPUT_STREAM_(stream)
            && handleIndex >= 0
            && (unsigned int)handleIndex < stream->input.deviceCount )
        return ((HWAVEIN*)stream->input.waveHandles)[handleIndex];
    else
        return 0;
}


int PaWinMME_GetStreamOutputHandleCount( PaStream* s)
{
    PaWinMmeStream *stream;
    PaError result = GetWinMMEStreamPointer( &stream, s );

    if( result == paNoError )
        return (PA_IS_OUTPUT_STREAM_(stream)) ? stream->output.deviceCount : 0;
    else
        return result;
}


HWAVEOUT PaWinMME_GetStreamOutputHandle( PaStream* s, int handleIndex )
{
    PaWinMmeStream *stream;
    PaError result = GetWinMMEStreamPointer( &stream, s );

    if( result == paNoError
            && PA_IS_OUTPUT_STREAM_(stream)
            && handleIndex >= 0
            && (unsigned int)handleIndex < stream->output.deviceCount )
        return ((HWAVEOUT*)stream->output.waveHandles)[handleIndex];
    else
        return 0;
}
