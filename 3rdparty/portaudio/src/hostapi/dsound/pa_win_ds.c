/*
 * $Id: pa_win_ds.c 1606 2011-02-17 15:56:04Z rob_bielik $
 * Portable Audio I/O Library DirectSound implementation
 *
 * Authors: Phil Burk, Robert Marsanyi & Ross Bencina
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2007 Ross Bencina, Phil Burk, Robert Marsanyi
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

/** @file
 @ingroup hostapi_src
*/

#include <assert.h>
#include <stdio.h>
#include <string.h> /* strlen() */

#include <initguid.h> /* make sure ds guids get defined */
#include <windows.h>
#include <objbase.h>

/*
  Use the earliest version of DX required, no need to polute the namespace
*/
#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
#define DIRECTSOUND_VERSION 0x0800
#else
#define DIRECTSOUND_VERSION 0x0300
#endif
#include <dsound.h>
#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
#include <dsconf.h>
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_debugprint.h"

#include "pa_win_ds.h"
#include "pa_win_ds_dynlink.h"
#include "pa_win_waveformat.h"
#include "pa_win_wdmks_utils.h"

#ifndef PA_USE_WMME
#error "Portaudio internal error: PA_USE_WMME=0/1 not defined. pa_hostapi.h should ensure that it is."
#endif

#if (defined(WIN32) && (defined(_MSC_VER) && (_MSC_VER >= 1200))) /* MSC version 6 and above */
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "winmm.lib" )
#endif

/*
 provided in newer platform sdks and x64
 */
#ifndef DWORD_PTR
    #if defined(_WIN64)
        #define DWORD_PTR unsigned __int64
    #else
        #define DWORD_PTR unsigned long
    #endif
#endif

#define PRINT(x) PA_DEBUG(x);
#define ERR_RPT(x) PRINT(x)
#define DBUG(x)   PRINT(x)
#define DBUGX(x)  PRINT(x)

#define PA_USE_HIGH_LATENCY   (0)
#if PA_USE_HIGH_LATENCY
#define PA_WIN_9X_LATENCY     (500)
#define PA_WIN_NT_LATENCY     (600)
#else
#define PA_WIN_9X_LATENCY     (140)
#define PA_WIN_NT_LATENCY     (280)
#endif

#define PA_WIN_WDM_LATENCY       (120)

#define SECONDS_PER_MSEC      (0.001)
#define MSEC_PER_SECOND       (1000)

/* prototypes for functions declared in this file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

PaError PaWinDs_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );

#ifdef __cplusplus
}
#endif /* __cplusplus */

static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
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


/* FIXME: should convert hr to a string */
#define PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr ) \
    PaUtil_SetLastHostErrorInfo( paDirectSound, hr, "DirectSound error" )

/************************************************* DX Prototypes **********/
static BOOL CALLBACK CollectGUIDsProc(LPGUID lpGUID,
                                     LPCTSTR lpszDesc,
                                     LPCTSTR lpszDrvName,
                                     LPVOID lpContext );

/************************************************************************************/
/********************** Structures **************************************************/
/************************************************************************************/
/* PaWinDsHostApiRepresentation - host api datastructure specific to this implementation */

typedef struct PaWinDsDeviceInfo
{
    PaDeviceInfo        inheritedDeviceInfo;
    GUID                guid;
    GUID                *lpGUID;
    double              sampleRates[3];
    char deviceInputChannelCountIsKnown; /**<< if the system returns 0xFFFF then we don't really know the number of supported channels (1=>known, 0=>unknown)*/
    char deviceOutputChannelCountIsKnown; /**<< if the system returns 0xFFFF then we don't really know the number of supported channels (1=>known, 0=>unknown)*/
} PaWinDsDeviceInfo;

typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface    callbackStreamInterface;
    PaUtilStreamInterface    blockingStreamInterface;

    PaUtilAllocationGroup   *allocations;

    /* implementation specific data goes here */

    char                    comWasInitialized;

} PaWinDsHostApiRepresentation;


/* PaWinDsStream - a stream data structure specifically for this implementation */

typedef struct PaWinDsStream
{
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

/* DirectSound specific data. */
#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
    LPDIRECTSOUNDFULLDUPLEX8 pDirectSoundFullDuplex8;
#endif

/* Output */
    LPDIRECTSOUND        pDirectSound;
    LPDIRECTSOUNDBUFFER  pDirectSoundPrimaryBuffer;
    LPDIRECTSOUNDBUFFER  pDirectSoundOutputBuffer;
    DWORD                outputBufferWriteOffsetBytes;     /* last write position */
    INT                  outputBufferSizeBytes;
    INT                  bytesPerOutputFrame;
    /* Try to detect play buffer underflows. */
    LARGE_INTEGER        perfCounterTicksPerBuffer; /* counter ticks it should take to play a full buffer */
    LARGE_INTEGER        previousPlayTime;
    DWORD                previousPlayCursor;
    UINT                 outputUnderflowCount;
    BOOL                 outputIsRunning;
    INT                  finalZeroBytesWritten; /* used to determine when we've flushed the whole buffer */

/* Input */
    LPDIRECTSOUNDCAPTURE pDirectSoundCapture;
    LPDIRECTSOUNDCAPTUREBUFFER   pDirectSoundInputBuffer;
    INT                  bytesPerInputFrame;
    UINT                 readOffset;      /* last read position */
    UINT                 inputSize;

    
    MMRESULT         timerID;
    int              framesPerDSBuffer;
    double           framesWritten;
    double           secondsPerHostByte; /* Used to optimize latency calculation for outTime */

    PaStreamCallbackFlags callbackFlags;

    PaStreamFlags    streamFlags;
    int              callbackResult;
    HANDLE           processingCompleted;
    
/* FIXME - move all below to PaUtilStreamRepresentation */
    volatile int     isStarted;
    volatile int     isActive;
    volatile int     stopProcessing; /* stop thread once existing buffers have been returned */
    volatile int     abortProcessing; /* stop thread immediately */
} PaWinDsStream;


/************************************************************************************
** Duplicate the input string using the allocations allocator.
** A NULL string is converted to a zero length string.
** If memory cannot be allocated, NULL is returned.
**/
static char *DuplicateDeviceNameString( PaUtilAllocationGroup *allocations, const char* src )
{
    char *result = 0;
    
    if( src != NULL )
    {
        size_t len = strlen(src);
        result = (char*)PaUtil_GroupAllocateMemory( allocations, (long)(len + 1) );
        if( result )
            memcpy( (void *) result, src, len+1 );
    }
    else
    {
        result = (char*)PaUtil_GroupAllocateMemory( allocations, 1 );
        if( result )
            result[0] = '\0';
    }

    return result;
}

/************************************************************************************
** DSDeviceNameAndGUID, DSDeviceNameAndGUIDVector used for collecting preliminary
** information during device enumeration.
*/
typedef struct DSDeviceNameAndGUID{
    char *name; // allocated from parent's allocations, never deleted by this structure
    GUID guid;
    LPGUID lpGUID;
    void *pnpInterface;  // wchar_t* interface path, allocated using the DS host api's allocation group
} DSDeviceNameAndGUID;

typedef struct DSDeviceNameAndGUIDVector{
    PaUtilAllocationGroup *allocations;
    PaError enumerationError;

    int count;
    int free;
    DSDeviceNameAndGUID *items; // Allocated using LocalAlloc()
} DSDeviceNameAndGUIDVector;

typedef struct DSDeviceNamesAndGUIDs{
    PaWinDsHostApiRepresentation *winDsHostApi;
    DSDeviceNameAndGUIDVector inputNamesAndGUIDs;
    DSDeviceNameAndGUIDVector outputNamesAndGUIDs;
} DSDeviceNamesAndGUIDs;

static PaError InitializeDSDeviceNameAndGUIDVector(
        DSDeviceNameAndGUIDVector *guidVector, PaUtilAllocationGroup *allocations )
{
    PaError result = paNoError;

    guidVector->allocations = allocations;
    guidVector->enumerationError = paNoError;

    guidVector->count = 0;
    guidVector->free = 8;
    guidVector->items = (DSDeviceNameAndGUID*)LocalAlloc( LMEM_FIXED, sizeof(DSDeviceNameAndGUID) * guidVector->free );
    if( guidVector->items == NULL )
        result = paInsufficientMemory;
    
    return result;
}

static PaError ExpandDSDeviceNameAndGUIDVector( DSDeviceNameAndGUIDVector *guidVector )
{
    PaError result = paNoError;
    DSDeviceNameAndGUID *newItems;
    int i;
    
    /* double size of vector */
    int size = guidVector->count + guidVector->free;
    guidVector->free += size;

    newItems = (DSDeviceNameAndGUID*)LocalAlloc( LMEM_FIXED, sizeof(DSDeviceNameAndGUID) * size * 2 );
    if( newItems == NULL )
    {
        result = paInsufficientMemory;
    }
    else
    {
        for( i=0; i < guidVector->count; ++i )
        {
            newItems[i].name = guidVector->items[i].name;
            if( guidVector->items[i].lpGUID == NULL )
            {
                newItems[i].lpGUID = NULL;
            }
            else
            {
                newItems[i].lpGUID = &newItems[i].guid;
                memcpy( &newItems[i].guid, guidVector->items[i].lpGUID, sizeof(GUID) );;
            }
            newItems[i].pnpInterface = guidVector->items[i].pnpInterface;
        }

        LocalFree( guidVector->items );
        guidVector->items = newItems;
    }                                

    return result;
}

/*
    it's safe to call DSDeviceNameAndGUIDVector multiple times
*/
static PaError TerminateDSDeviceNameAndGUIDVector( DSDeviceNameAndGUIDVector *guidVector )
{
    PaError result = paNoError;

    if( guidVector->items != NULL )
    {
        if( LocalFree( guidVector->items ) != NULL )
            result = paInsufficientMemory;              /** @todo this isn't the correct error to return from a deallocation failure */

        guidVector->items = NULL;
    }

    return result;
}

/************************************************************************************
** Collect preliminary device information during DirectSound enumeration 
*/
static BOOL CALLBACK CollectGUIDsProc(LPGUID lpGUID,
                                     LPCTSTR lpszDesc,
                                     LPCTSTR lpszDrvName,
                                     LPVOID lpContext )
{
    DSDeviceNameAndGUIDVector *namesAndGUIDs = (DSDeviceNameAndGUIDVector*)lpContext;
    PaError error;

    (void) lpszDrvName; /* unused variable */

    if( namesAndGUIDs->free == 0 )
    {
        error = ExpandDSDeviceNameAndGUIDVector( namesAndGUIDs );
        if( error != paNoError )
        {
            namesAndGUIDs->enumerationError = error;
            return FALSE;
        }
    }
    
    /* Set GUID pointer, copy GUID to storage in DSDeviceNameAndGUIDVector. */
    if( lpGUID == NULL )
    {
        namesAndGUIDs->items[namesAndGUIDs->count].lpGUID = NULL;
    }
    else
    {
        namesAndGUIDs->items[namesAndGUIDs->count].lpGUID =
                &namesAndGUIDs->items[namesAndGUIDs->count].guid;
      
        memcpy( &namesAndGUIDs->items[namesAndGUIDs->count].guid, lpGUID, sizeof(GUID) );
    }

    namesAndGUIDs->items[namesAndGUIDs->count].name =
            DuplicateDeviceNameString( namesAndGUIDs->allocations, lpszDesc );
    if( namesAndGUIDs->items[namesAndGUIDs->count].name == NULL )
    {
        namesAndGUIDs->enumerationError = paInsufficientMemory;
        return FALSE;
    }

    namesAndGUIDs->items[namesAndGUIDs->count].pnpInterface = 0;

    ++namesAndGUIDs->count;
    --namesAndGUIDs->free;
    
    return TRUE;
}

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO

static void *DuplicateWCharString( PaUtilAllocationGroup *allocations, wchar_t *source )
{
    size_t len;
    wchar_t *result;

    len = wcslen( source );
    result = (wchar_t*)PaUtil_GroupAllocateMemory( allocations, (long) ((len+1) * sizeof(wchar_t)) );
    wcscpy( result, source );
    return result;
}

static BOOL CALLBACK KsPropertySetEnumerateCallback( PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data, LPVOID context )
{
    int i;
    DSDeviceNamesAndGUIDs *deviceNamesAndGUIDs = (DSDeviceNamesAndGUIDs*)context;

    if( data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER )
    {
        for( i=0; i < deviceNamesAndGUIDs->outputNamesAndGUIDs.count; ++i )
        {
            if( deviceNamesAndGUIDs->outputNamesAndGUIDs.items[i].lpGUID
                && memcmp( &data->DeviceId, deviceNamesAndGUIDs->outputNamesAndGUIDs.items[i].lpGUID, sizeof(GUID) ) == 0 )
            {
                deviceNamesAndGUIDs->outputNamesAndGUIDs.items[i].pnpInterface = 
                        (char*)DuplicateWCharString( deviceNamesAndGUIDs->winDsHostApi->allocations, data->Interface );
                break;
            }
        }
    }
    else if( data->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE )
    {
        for( i=0; i < deviceNamesAndGUIDs->inputNamesAndGUIDs.count; ++i )
        {
            if( deviceNamesAndGUIDs->inputNamesAndGUIDs.items[i].lpGUID
                && memcmp( &data->DeviceId, deviceNamesAndGUIDs->inputNamesAndGUIDs.items[i].lpGUID, sizeof(GUID) ) == 0 )
            {
                deviceNamesAndGUIDs->inputNamesAndGUIDs.items[i].pnpInterface = 
                        (char*)DuplicateWCharString( deviceNamesAndGUIDs->winDsHostApi->allocations, data->Interface );
                break;
            }
        }
    }

    return TRUE;
}


static GUID pawin_CLSID_DirectSoundPrivate = 
{ 0x11ab3ec0, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x00, 0xc0, 0x4f, 0xc2, 0x8a, 0xca };

static GUID pawin_DSPROPSETID_DirectSoundDevice = 
{ 0x84624f82, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x00, 0xc0, 0x4f, 0xc2, 0x8a, 0xca };

static GUID pawin_IID_IKsPropertySet = 
{ 0x31efac30, 0x515c, 0x11d0, 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93 };


/*
    FindDevicePnpInterfaces fills in the pnpInterface fields in deviceNamesAndGUIDs
    with UNICODE file paths to the devices. The DS documentation mentions
    at least two techniques by which these Interface paths can be found using IKsPropertySet on
    the DirectSound class object. One is using the DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION 
    property, and the other is using DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE.
    I tried both methods and only the second worked. I found two postings on the
    net from people who had the same problem with the first method, so I think the method used here is 
    more common/likely to work. The probem is that IKsPropertySet_Get returns S_OK
    but the fields of the device description are not filled in.

    The mechanism we use works by registering an enumeration callback which is called for 
    every DSound device. Our callback searches for a device in our deviceNamesAndGUIDs list
    with the matching GUID and copies the pointer to the Interface path.
    Note that we could have used this enumeration callback to perform the original 
    device enumeration, however we choose not to so we can disable this step easily.

    Apparently the IKsPropertySet mechanism was added in DirectSound 9c 2004
    http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.mmedia/2004-12/0099.html

        -- rossb
*/
static void FindDevicePnpInterfaces( DSDeviceNamesAndGUIDs *deviceNamesAndGUIDs )
{
    IClassFactory *pClassFactory;
   
    if( paWinDsDSoundEntryPoints.DllGetClassObject(&pawin_CLSID_DirectSoundPrivate, &IID_IClassFactory, (PVOID *) &pClassFactory) == S_OK ){
        IKsPropertySet *pPropertySet;
        if( pClassFactory->lpVtbl->CreateInstance( pClassFactory, NULL, &pawin_IID_IKsPropertySet, (PVOID *) &pPropertySet) == S_OK ){
            
            DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA data;
            ULONG bytesReturned;

            data.Callback = KsPropertySetEnumerateCallback;
            data.Context = deviceNamesAndGUIDs;

            IKsPropertySet_Get( pPropertySet,
                &pawin_DSPROPSETID_DirectSoundDevice,
                DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W,
                NULL,
                0,
                &data,
                sizeof(data),
                &bytesReturned
            );
            
            IKsPropertySet_Release( pPropertySet );
        }
        pClassFactory->lpVtbl->Release( pClassFactory );
    }

    /*
        The following code fragment, which I chose not to use, queries for the 
        device interface for a device with a specific GUID:

        ULONG BytesReturned;
        DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA Property;

        memset (&Property, 0, sizeof(Property));
        Property.DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
        Property.DeviceId = *lpGUID;  

        hr = IKsPropertySet_Get( pPropertySet,
            &pawin_DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W,
            NULL,
            0,
            &Property,
            sizeof(Property),
            &BytesReturned
        );

        if( hr == S_OK )
        {
            //pnpInterface = Property.Interface;
        }
    */
}
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */


/* 
    GUIDs for emulated devices which we blacklist below.
    are there more than two of them??
*/

GUID IID_IRolandVSCEmulated1 = {0xc2ad1800, 0xb243, 0x11ce, 0xa8, 0xa4, 0x00, 0xaa, 0x00, 0x6c, 0x45, 0x01};
GUID IID_IRolandVSCEmulated2 = {0xc2ad1800, 0xb243, 0x11ce, 0xa8, 0xa4, 0x00, 0xaa, 0x00, 0x6c, 0x45, 0x02};


#define PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_  (13) /* must match array length below */
static double defaultSampleRateSearchOrder_[] =
    { 44100.0, 48000.0, 32000.0, 24000.0, 22050.0, 88200.0, 96000.0, 192000.0,
        16000.0, 12000.0, 11025.0, 9600.0, 8000.0 };

/************************************************************************************
** Extract capabilities from an output device, and add it to the device info list
** if successful. This function assumes that there is enough room in the
** device info list to accomodate all entries.
**
** The device will not be added to the device list if any errors are encountered.
*/
static PaError AddOutputDeviceInfoFromDirectSound(
        PaWinDsHostApiRepresentation *winDsHostApi, char *name, LPGUID lpGUID, char *pnpInterface )
{
    PaUtilHostApiRepresentation  *hostApi = &winDsHostApi->inheritedHostApiRep;
    PaWinDsDeviceInfo            *winDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[hostApi->info.deviceCount];
    PaDeviceInfo                 *deviceInfo = &winDsDeviceInfo->inheritedDeviceInfo;
    HRESULT                       hr;
    LPDIRECTSOUND                 lpDirectSound;
    DSCAPS                        caps;
    int                           deviceOK = TRUE;
    PaError                       result = paNoError;
    int                           i;

    /* Copy GUID to the device info structure. Set pointer. */
    if( lpGUID == NULL )
    {
        winDsDeviceInfo->lpGUID = NULL;
    }
    else
    {
        memcpy( &winDsDeviceInfo->guid, lpGUID, sizeof(GUID) );
        winDsDeviceInfo->lpGUID = &winDsDeviceInfo->guid;
    }
    
    if( lpGUID )
    {
        if (IsEqualGUID (&IID_IRolandVSCEmulated1,lpGUID) ||
            IsEqualGUID (&IID_IRolandVSCEmulated2,lpGUID) )
        {
            PA_DEBUG(("BLACKLISTED: %s \n",name));
            return paNoError;
        }
    }

    /* Create a DirectSound object for the specified GUID
        Note that using CoCreateInstance doesn't work on windows CE.
    */
    hr = paWinDsDSoundEntryPoints.DirectSoundCreate( lpGUID, &lpDirectSound, NULL );

    /** try using CoCreateInstance because DirectSoundCreate was hanging under
        some circumstances - note this was probably related to the
        #define BOOL short bug which has now been fixed
        @todo delete this comment and the following code once we've ensured
        there is no bug.
    */
    /*
    hr = CoCreateInstance( &CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSound, (void**)&lpDirectSound );

    if( hr == S_OK )
    {
        hr = IDirectSound_Initialize( lpDirectSound, lpGUID );
    }
    */
    
    if( hr != DS_OK )
    {
        if (hr == DSERR_ALLOCATED)
            PA_DEBUG(("AddOutputDeviceInfoFromDirectSound %s DSERR_ALLOCATED\n",name));
        DBUG(("Cannot create DirectSound for %s. Result = 0x%x\n", name, hr ));
        if (lpGUID)
            DBUG(("%s's GUID: {0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x, 0x%x} \n",
                 name,
                 lpGUID->Data1,
                 lpGUID->Data2,
                 lpGUID->Data3,
                 lpGUID->Data4[0],
                 lpGUID->Data4[1],
                 lpGUID->Data4[2],
                 lpGUID->Data4[3],
                 lpGUID->Data4[4],
                 lpGUID->Data4[5],
                 lpGUID->Data4[6],
                 lpGUID->Data4[7]));

        deviceOK = FALSE;
    }
    else
    {
        /* Query device characteristics. */
        memset( &caps, 0, sizeof(caps) ); 
        caps.dwSize = sizeof(caps);
        hr = IDirectSound_GetCaps( lpDirectSound, &caps );
        if( hr != DS_OK )
        {
            DBUG(("Cannot GetCaps() for DirectSound device %s. Result = 0x%x\n", name, hr ));
            deviceOK = FALSE;
        }
        else
        {

#if PA_USE_WMME
            if( caps.dwFlags & DSCAPS_EMULDRIVER )
            {
                /* If WMME supported, then reject Emulated drivers because they are lousy. */
                deviceOK = FALSE;
            }
#endif

            if( deviceOK )
            {
                deviceInfo->maxInputChannels = 0;
                winDsDeviceInfo->deviceInputChannelCountIsKnown = 1;

                /* DS output capabilities only indicate supported number of channels
                   using two flags which indicate mono and/or stereo.
                   We assume that stereo devices may support more than 2 channels
                   (as is the case with 5.1 devices for example) and so
                   set deviceOutputChannelCountIsKnown to 0 (unknown).
                   In this case OpenStream will try to open the device
                   when the user requests more than 2 channels, rather than
                   returning an error. 
                */
                if( caps.dwFlags & DSCAPS_PRIMARYSTEREO )
                {
                    deviceInfo->maxOutputChannels = 2;
                    winDsDeviceInfo->deviceOutputChannelCountIsKnown = 0;
                }
                else
                {
                    deviceInfo->maxOutputChannels = 1;
                    winDsDeviceInfo->deviceOutputChannelCountIsKnown = 1;
                }

                /* Guess channels count from speaker configuration. We do it only when 
                   pnpInterface is NULL or when PAWIN_USE_WDMKS_DEVICE_INFO is undefined.
                */
#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
                if( !pnpInterface )
#endif
                {
                    DWORD spkrcfg;
                    if( SUCCEEDED(IDirectSound_GetSpeakerConfig( lpDirectSound, &spkrcfg )) )
                    {
                        int count = 0;
                        switch (DSSPEAKER_CONFIG(spkrcfg))
                        {
                            case DSSPEAKER_HEADPHONE:        count = 2; break;
                            case DSSPEAKER_MONO:             count = 1; break;
                            case DSSPEAKER_QUAD:             count = 4; break;
                            case DSSPEAKER_STEREO:           count = 2; break;
                            case DSSPEAKER_SURROUND:         count = 4; break;
                            case DSSPEAKER_5POINT1:          count = 6; break;
                            case DSSPEAKER_7POINT1:          count = 8; break;
#ifndef DSSPEAKER_7POINT1_SURROUND
#define DSSPEAKER_7POINT1_SURROUND 0x00000008
#endif                            
                            case DSSPEAKER_7POINT1_SURROUND: count = 8; break;
#ifndef DSSPEAKER_5POINT1_SURROUND
#define DSSPEAKER_5POINT1_SURROUND 0x00000009
#endif
                            case DSSPEAKER_5POINT1_SURROUND: count = 6; break;
                        }
                        if( count )
                        {
                            deviceInfo->maxOutputChannels = count;
                            winDsDeviceInfo->deviceOutputChannelCountIsKnown = 1;
                        }
                    }
                }

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
                if( pnpInterface )
                {
                    int count = PaWin_WDMKS_QueryFilterMaximumChannelCount( pnpInterface, /* isInput= */ 0  );
                    if( count > 0 )
                    {
                        deviceInfo->maxOutputChannels = count;
                        winDsDeviceInfo->deviceOutputChannelCountIsKnown = 1;
                    }
                }
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

                deviceInfo->defaultLowInputLatency = 0.;    /** @todo IMPLEMENT ME */
                deviceInfo->defaultLowOutputLatency = 0.;   /** @todo IMPLEMENT ME */
                deviceInfo->defaultHighInputLatency = 0.;   /** @todo IMPLEMENT ME */
                deviceInfo->defaultHighOutputLatency = 0.;  /** @todo IMPLEMENT ME */
                
                /* initialize defaultSampleRate */
                
                if( caps.dwFlags & DSCAPS_CONTINUOUSRATE )
                {
                    /* initialize to caps.dwMaxSecondarySampleRate incase none of the standard rates match */
                    deviceInfo->defaultSampleRate = caps.dwMaxSecondarySampleRate;

                    for( i = 0; i < PA_DEFAULTSAMPLERATESEARCHORDER_COUNT_; ++i )
                    {
                        if( defaultSampleRateSearchOrder_[i] >= caps.dwMinSecondarySampleRate
                                && defaultSampleRateSearchOrder_[i] <= caps.dwMaxSecondarySampleRate )
                        {
                            deviceInfo->defaultSampleRate = defaultSampleRateSearchOrder_[i];
                            break;
                        }
                    }
                }
                else if( caps.dwMinSecondarySampleRate == caps.dwMaxSecondarySampleRate )
                {
                    if( caps.dwMinSecondarySampleRate == 0 )
                    {
                        /*
                        ** On my Thinkpad 380Z, DirectSoundV6 returns min-max=0 !!
                        ** But it supports continuous sampling.
                        ** So fake range of rates, and hope it really supports it.
                        */
                        deviceInfo->defaultSampleRate = 44100.0f;

                        DBUG(("PA - Reported rates both zero. Setting to fake values for device #%s\n", name ));
                    }
                    else
                    {
	                    deviceInfo->defaultSampleRate = caps.dwMaxSecondarySampleRate;
                    }
                }
                else if( (caps.dwMinSecondarySampleRate < 1000.0) && (caps.dwMaxSecondarySampleRate > 50000.0) )
                {
                    /* The EWS88MT drivers lie, lie, lie. The say they only support two rates, 100 & 100000.
                    ** But we know that they really support a range of rates!
                    ** So when we see a ridiculous set of rates, assume it is a range.
                    */
                  deviceInfo->defaultSampleRate = 44100.0f;
                  DBUG(("PA - Sample rate range used instead of two odd values for device #%s\n", name ));
                }
                else deviceInfo->defaultSampleRate = caps.dwMaxSecondarySampleRate;


                //printf( "min %d max %d\n", caps.dwMinSecondarySampleRate, caps.dwMaxSecondarySampleRate );
                // dwFlags | DSCAPS_CONTINUOUSRATE 
            }
        }

        IDirectSound_Release( lpDirectSound );
    }

    if( deviceOK )
    {
        deviceInfo->name = name;

        if( lpGUID == NULL )
            hostApi->info.defaultOutputDevice = hostApi->info.deviceCount;
            
        hostApi->info.deviceCount++;
    }

    return result;
}


/************************************************************************************
** Extract capabilities from an input device, and add it to the device info list
** if successful. This function assumes that there is enough room in the
** device info list to accomodate all entries.
**
** The device will not be added to the device list if any errors are encountered.
*/
static PaError AddInputDeviceInfoFromDirectSoundCapture(
        PaWinDsHostApiRepresentation *winDsHostApi, char *name, LPGUID lpGUID, char *pnpInterface )
{
    PaUtilHostApiRepresentation  *hostApi = &winDsHostApi->inheritedHostApiRep;
    PaWinDsDeviceInfo            *winDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[hostApi->info.deviceCount];
    PaDeviceInfo                 *deviceInfo = &winDsDeviceInfo->inheritedDeviceInfo;
    HRESULT                       hr;
    LPDIRECTSOUNDCAPTURE          lpDirectSoundCapture;
    DSCCAPS                       caps;
    int                           deviceOK = TRUE;
    PaError                       result = paNoError;
    
    /* Copy GUID to the device info structure. Set pointer. */
    if( lpGUID == NULL )
    {
        winDsDeviceInfo->lpGUID = NULL;
    }
    else
    {
        winDsDeviceInfo->lpGUID = &winDsDeviceInfo->guid;
        memcpy( &winDsDeviceInfo->guid, lpGUID, sizeof(GUID) );
    }

    hr = paWinDsDSoundEntryPoints.DirectSoundCaptureCreate( lpGUID, &lpDirectSoundCapture, NULL );

    /** try using CoCreateInstance because DirectSoundCreate was hanging under
        some circumstances - note this was probably related to the
        #define BOOL short bug which has now been fixed
        @todo delete this comment and the following code once we've ensured
        there is no bug.
    */
    /*
    hr = CoCreateInstance( &CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectSoundCapture, (void**)&lpDirectSoundCapture );
    */
    if( hr != DS_OK )
    {
        DBUG(("Cannot create Capture for %s. Result = 0x%x\n", name, hr ));
        deviceOK = FALSE;
    }
    else
    {
        /* Query device characteristics. */
        memset( &caps, 0, sizeof(caps) );
        caps.dwSize = sizeof(caps);
        hr = IDirectSoundCapture_GetCaps( lpDirectSoundCapture, &caps );
        if( hr != DS_OK )
        {
            DBUG(("Cannot GetCaps() for Capture device %s. Result = 0x%x\n", name, hr ));
            deviceOK = FALSE;
        }
        else
        {
#if PA_USE_WMME
            if( caps.dwFlags & DSCAPS_EMULDRIVER )
            {
                /* If WMME supported, then reject Emulated drivers because they are lousy. */
                deviceOK = FALSE;
            }
#endif

            if( deviceOK )
            {
                deviceInfo->maxInputChannels = caps.dwChannels;
                winDsDeviceInfo->deviceInputChannelCountIsKnown = 1;

                deviceInfo->maxOutputChannels = 0;
                winDsDeviceInfo->deviceOutputChannelCountIsKnown = 1;

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
                if( pnpInterface )
                {
                    int count = PaWin_WDMKS_QueryFilterMaximumChannelCount( pnpInterface, /* isInput= */ 1  );
                    if( count > 0 )
                    {
                        deviceInfo->maxInputChannels = count;
                        winDsDeviceInfo->deviceInputChannelCountIsKnown = 1;
                    }
                }
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

                deviceInfo->defaultLowInputLatency = 0.;    /** @todo IMPLEMENT ME */
                deviceInfo->defaultLowOutputLatency = 0.;   /** @todo IMPLEMENT ME */
                deviceInfo->defaultHighInputLatency = 0.;   /** @todo IMPLEMENT ME */
                deviceInfo->defaultHighOutputLatency = 0.;  /** @todo IMPLEMENT ME */

/*  constants from a WINE patch by Francois Gouget, see:
    http://www.winehq.com/hypermail/wine-patches/2003/01/0290.html

    ---
    Date: Fri, 14 May 2004 10:38:12 +0200 (CEST)
    From: Francois Gouget <fgouget@ ... .fr>
    To: Ross Bencina <rbencina@ ... .au>
    Subject: Re: Permission to use wine 48/96 wave patch in BSD licensed library

    [snip]

    I give you permission to use the patch below under the BSD license.
    http://www.winehq.com/hypermail/wine-patches/2003/01/0290.html

    [snip]
*/
#ifndef WAVE_FORMAT_48M08
#define WAVE_FORMAT_48M08      0x00001000    /* 48     kHz, Mono,   8-bit  */
#define WAVE_FORMAT_48S08      0x00002000    /* 48     kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_48M16      0x00004000    /* 48     kHz, Mono,   16-bit */
#define WAVE_FORMAT_48S16      0x00008000    /* 48     kHz, Stereo, 16-bit */
#define WAVE_FORMAT_96M08      0x00010000    /* 96     kHz, Mono,   8-bit  */
#define WAVE_FORMAT_96S08      0x00020000    /* 96     kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_96M16      0x00040000    /* 96     kHz, Mono,   16-bit */
#define WAVE_FORMAT_96S16      0x00080000    /* 96     kHz, Stereo, 16-bit */
#endif

                /* defaultSampleRate */
                if( caps.dwChannels == 2 )
                {
                    if( caps.dwFormats & WAVE_FORMAT_4S16 )
                        deviceInfo->defaultSampleRate = 44100.0;
                    else if( caps.dwFormats & WAVE_FORMAT_48S16 )
                        deviceInfo->defaultSampleRate = 48000.0;
                    else if( caps.dwFormats & WAVE_FORMAT_2S16 )
                        deviceInfo->defaultSampleRate = 22050.0;
                    else if( caps.dwFormats & WAVE_FORMAT_1S16 )
                        deviceInfo->defaultSampleRate = 11025.0;
                    else if( caps.dwFormats & WAVE_FORMAT_96S16 )
                        deviceInfo->defaultSampleRate = 96000.0;
                    else
                        deviceInfo->defaultSampleRate = 0.;
                }
                else if( caps.dwChannels == 1 )
                {
                    if( caps.dwFormats & WAVE_FORMAT_4M16 )
                        deviceInfo->defaultSampleRate = 44100.0;
                    else if( caps.dwFormats & WAVE_FORMAT_48M16 )
                        deviceInfo->defaultSampleRate = 48000.0;
                    else if( caps.dwFormats & WAVE_FORMAT_2M16 )
                        deviceInfo->defaultSampleRate = 22050.0;
                    else if( caps.dwFormats & WAVE_FORMAT_1M16 )
                        deviceInfo->defaultSampleRate = 11025.0;
                    else if( caps.dwFormats & WAVE_FORMAT_96M16 )
                        deviceInfo->defaultSampleRate = 96000.0;
                    else
                        deviceInfo->defaultSampleRate = 0.;
                }
                else deviceInfo->defaultSampleRate = 0.;
            }
        }
        
        IDirectSoundCapture_Release( lpDirectSoundCapture );
    }

    if( deviceOK )
    {
        deviceInfo->name = name;

        if( lpGUID == NULL )
            hostApi->info.defaultInputDevice = hostApi->info.deviceCount;

        hostApi->info.deviceCount++;
    }

    return result;
}


/***********************************************************************************/
PaError PaWinDs_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    int i, deviceCount;
    PaWinDsHostApiRepresentation *winDsHostApi;
    DSDeviceNamesAndGUIDs deviceNamesAndGUIDs;

    PaWinDsDeviceInfo *deviceInfoArray;
    char comWasInitialized = 0;

    /*
        If COM is already initialized CoInitialize will either return
        FALSE, or RPC_E_CHANGED_MODE if it was initialised in a different
        threading mode. In either case we shouldn't consider it an error
        but we need to be careful to not call CoUninitialize() if 
        RPC_E_CHANGED_MODE was returned.
    */

    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) && hr != RPC_E_CHANGED_MODE )
        return paUnanticipatedHostError;

    if( hr != RPC_E_CHANGED_MODE )
        comWasInitialized = 1;

    /* initialise guid vectors so they can be safely deleted on error */
    deviceNamesAndGUIDs.winDsHostApi = NULL;
    deviceNamesAndGUIDs.inputNamesAndGUIDs.items = NULL;
    deviceNamesAndGUIDs.outputNamesAndGUIDs.items = NULL;

    PaWinDs_InitializeDSoundEntryPoints();

    winDsHostApi = (PaWinDsHostApiRepresentation*)PaUtil_AllocateMemory( sizeof(PaWinDsHostApiRepresentation) );
    if( !winDsHostApi )
    {
        result = paInsufficientMemory;
        goto error;
    }

    winDsHostApi->comWasInitialized = comWasInitialized;

    winDsHostApi->allocations = PaUtil_CreateAllocationGroup();
    if( !winDsHostApi->allocations )
    {
        result = paInsufficientMemory;
        goto error;
    }

    *hostApi = &winDsHostApi->inheritedHostApiRep;
    (*hostApi)->info.structVersion = 1;
    (*hostApi)->info.type = paDirectSound;
    (*hostApi)->info.name = "Windows DirectSound";
    
    (*hostApi)->info.deviceCount = 0;
    (*hostApi)->info.defaultInputDevice = paNoDevice;
    (*hostApi)->info.defaultOutputDevice = paNoDevice;

    
/* DSound - enumerate devices to count them and to gather their GUIDs */

    result = InitializeDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.inputNamesAndGUIDs, winDsHostApi->allocations );
    if( result != paNoError )
        goto error;

    result = InitializeDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.outputNamesAndGUIDs, winDsHostApi->allocations );
    if( result != paNoError )
        goto error;

    paWinDsDSoundEntryPoints.DirectSoundCaptureEnumerateA( (LPDSENUMCALLBACK)CollectGUIDsProc, (void *)&deviceNamesAndGUIDs.inputNamesAndGUIDs );

    paWinDsDSoundEntryPoints.DirectSoundEnumerateA( (LPDSENUMCALLBACK)CollectGUIDsProc, (void *)&deviceNamesAndGUIDs.outputNamesAndGUIDs );

    if( deviceNamesAndGUIDs.inputNamesAndGUIDs.enumerationError != paNoError )
    {
        result = deviceNamesAndGUIDs.inputNamesAndGUIDs.enumerationError;
        goto error;
    }

    if( deviceNamesAndGUIDs.outputNamesAndGUIDs.enumerationError != paNoError )
    {
        result = deviceNamesAndGUIDs.outputNamesAndGUIDs.enumerationError;
        goto error;
    }

    deviceCount = deviceNamesAndGUIDs.inputNamesAndGUIDs.count + deviceNamesAndGUIDs.outputNamesAndGUIDs.count;

#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
    if( deviceCount > 0 )
    {
        deviceNamesAndGUIDs.winDsHostApi = winDsHostApi;
        FindDevicePnpInterfaces( &deviceNamesAndGUIDs );
    }
#endif /* PAWIN_USE_WDMKS_DEVICE_INFO */

    if( deviceCount > 0 )
    {
        /* allocate array for pointers to PaDeviceInfo structs */
        (*hostApi)->deviceInfos = (PaDeviceInfo**)PaUtil_GroupAllocateMemory(
                winDsHostApi->allocations, sizeof(PaDeviceInfo*) * deviceCount );
        if( !(*hostApi)->deviceInfos )
        {
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all PaDeviceInfo structs in a contiguous block */
        deviceInfoArray = (PaWinDsDeviceInfo*)PaUtil_GroupAllocateMemory(
                winDsHostApi->allocations, sizeof(PaWinDsDeviceInfo) * deviceCount );
        if( !deviceInfoArray )
        {
            result = paInsufficientMemory;
            goto error;
        }

        for( i=0; i < deviceCount; ++i )
        {
            PaDeviceInfo *deviceInfo = &deviceInfoArray[i].inheritedDeviceInfo;
            deviceInfo->structVersion = 2;
            deviceInfo->hostApi = hostApiIndex;
            deviceInfo->name = 0;
            (*hostApi)->deviceInfos[i] = deviceInfo;
        }

        for( i=0; i < deviceNamesAndGUIDs.inputNamesAndGUIDs.count; ++i )
        {
            result = AddInputDeviceInfoFromDirectSoundCapture( winDsHostApi,
                    deviceNamesAndGUIDs.inputNamesAndGUIDs.items[i].name,
                    deviceNamesAndGUIDs.inputNamesAndGUIDs.items[i].lpGUID,
                    deviceNamesAndGUIDs.inputNamesAndGUIDs.items[i].pnpInterface );
            if( result != paNoError )
                goto error;
        }

        for( i=0; i < deviceNamesAndGUIDs.outputNamesAndGUIDs.count; ++i )
        {
            result = AddOutputDeviceInfoFromDirectSound( winDsHostApi,
                    deviceNamesAndGUIDs.outputNamesAndGUIDs.items[i].name,
                    deviceNamesAndGUIDs.outputNamesAndGUIDs.items[i].lpGUID,
                    deviceNamesAndGUIDs.outputNamesAndGUIDs.items[i].pnpInterface );
            if( result != paNoError )
                goto error;
        }
    }    

    result = TerminateDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.inputNamesAndGUIDs );
    if( result != paNoError )
        goto error;

    result = TerminateDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.outputNamesAndGUIDs );
    if( result != paNoError )
        goto error;

    
    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &winDsHostApi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &winDsHostApi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );

    return result;

error:
    if( winDsHostApi )
    {
        if( winDsHostApi->allocations )
        {
            PaUtil_FreeAllAllocations( winDsHostApi->allocations );
            PaUtil_DestroyAllocationGroup( winDsHostApi->allocations );
        }
                
        PaUtil_FreeMemory( winDsHostApi );
    }

    TerminateDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.inputNamesAndGUIDs );
    TerminateDSDeviceNameAndGUIDVector( &deviceNamesAndGUIDs.outputNamesAndGUIDs );

    if( comWasInitialized )
        CoUninitialize();

    return result;
}


/***********************************************************************************/
static void Terminate( struct PaUtilHostApiRepresentation *hostApi )
{
    PaWinDsHostApiRepresentation *winDsHostApi = (PaWinDsHostApiRepresentation*)hostApi;
    char comWasInitialized = winDsHostApi->comWasInitialized;

    /*
        IMPLEMENT ME:
            - clean up any resources not handled by the allocation group
    */

    if( winDsHostApi->allocations )
    {
        PaUtil_FreeAllAllocations( winDsHostApi->allocations );
        PaUtil_DestroyAllocationGroup( winDsHostApi->allocations );
    }

    PaUtil_FreeMemory( winDsHostApi );

    PaWinDs_TerminateDSoundEntryPoints();

    if( comWasInitialized )
        CoUninitialize();
}


/* Set minimal latency based on whether NT or Win95.
 * NT has higher latency.
 */
static int PaWinDS_GetMinSystemLatency( void )
{
    int minLatencyMsec;
    /* Set minimal latency based on whether NT or other OS.
     * NT has higher latency.
     */
    OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	GetVersionEx( &osvi );
    DBUG(("PA - PlatformId = 0x%x\n", osvi.dwPlatformId ));
    DBUG(("PA - MajorVersion = 0x%x\n", osvi.dwMajorVersion ));
    DBUG(("PA - MinorVersion = 0x%x\n", osvi.dwMinorVersion ));
    /* Check for NT */
	if( (osvi.dwMajorVersion == 4) && (osvi.dwPlatformId == 2) )
	{
		minLatencyMsec = PA_WIN_NT_LATENCY;
	}
	else if(osvi.dwMajorVersion >= 5)
	{
		minLatencyMsec = PA_WIN_WDM_LATENCY;
	}
	else
	{
		minLatencyMsec = PA_WIN_9X_LATENCY;
	}
    return minLatencyMsec;
}

static PaError ValidateWinDirectSoundSpecificStreamInfo(
        const PaStreamParameters *streamParameters,
        const PaWinDirectSoundStreamInfo *streamInfo )
{
	if( streamInfo )
	{
	    if( streamInfo->size != sizeof( PaWinDirectSoundStreamInfo )
	            || streamInfo->version != 1 )
	    {
	        return paIncompatibleHostApiSpecificStreamInfo;
	    }
	}

	return paNoError;
}

/***********************************************************************************/
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate )
{
    PaError result;
    PaWinDsDeviceInfo *inputWinDsDeviceInfo, *outputWinDsDeviceInfo;
    PaDeviceInfo *inputDeviceInfo, *outputDeviceInfo;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaWinDirectSoundStreamInfo *inputStreamInfo, *outputStreamInfo;

    if( inputParameters )
    {
        inputWinDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[ inputParameters->device ];
        inputDeviceInfo = &inputWinDsDeviceInfo->inheritedDeviceInfo;

        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if( inputWinDsDeviceInfo->deviceInputChannelCountIsKnown
                && inputChannelCount > inputDeviceInfo->maxInputChannels )
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        inputStreamInfo = (PaWinDirectSoundStreamInfo*)inputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinDirectSoundSpecificStreamInfo( inputParameters, inputStreamInfo );
		if( result != paNoError ) return result;
    }
    else
    {
        inputChannelCount = 0;
    }

    if( outputParameters )
    {
        outputWinDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[ outputParameters->device ];
        outputDeviceInfo = &outputWinDsDeviceInfo->inheritedDeviceInfo;

        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        
        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */

        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that output device can support inputChannelCount */
        if( outputWinDsDeviceInfo->deviceOutputChannelCountIsKnown
                && outputChannelCount > outputDeviceInfo->maxOutputChannels )
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        outputStreamInfo = (PaWinDirectSoundStreamInfo*)outputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinDirectSoundSpecificStreamInfo( outputParameters, outputStreamInfo );
		if( result != paNoError ) return result;
    }
    else
    {
        outputChannelCount = 0;
    }
    
    /*
        IMPLEMENT ME:

            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported if necessary

            - check that the device supports sampleRate

        Because the buffer adapter handles conversion between all standard
        sample formats, the following checks are only required if paCustomFormat
        is implemented, or under some other unusual conditions.

            - check that input device can support inputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - check that output device can support outputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format
    */

    return paFormatIsSupported;
}


/*************************************************************************
** Determine minimum number of buffers required for this host based
** on minimum latency. Latency can be optionally set by user by setting
** an environment variable. For example, to set latency to 200 msec, put:
**
**    set PA_MIN_LATENCY_MSEC=200
**
** in the AUTOEXEC.BAT file and reboot.
** If the environment variable is not set, then the latency will be determined
** based on the OS. Windows NT has higher latency than Win95.
*/
#define PA_LATENCY_ENV_NAME  ("PA_MIN_LATENCY_MSEC")
#define PA_ENV_BUF_SIZE  (32)

static int PaWinDs_GetMinLatencyFrames( double sampleRate )
{
    char      envbuf[PA_ENV_BUF_SIZE];
    DWORD     hresult;
    int       minLatencyMsec = 0;

    /* Let user determine minimal latency by setting environment variable. */
    hresult = GetEnvironmentVariable( PA_LATENCY_ENV_NAME, envbuf, PA_ENV_BUF_SIZE );
    if( (hresult > 0) && (hresult < PA_ENV_BUF_SIZE) )
    {
        minLatencyMsec = atoi( envbuf );
    }
    else
    {
        minLatencyMsec = PaWinDS_GetMinSystemLatency();
#if PA_USE_HIGH_LATENCY
        PRINT(("PA - Minimum Latency set to %d msec!\n", minLatencyMsec ));
#endif

    }

    return (int) (minLatencyMsec * sampleRate * SECONDS_PER_MSEC);
}


#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
static HRESULT InitFullDuplexInputOutputBuffers( PaWinDsStream *stream,
                                       PaWinDsDeviceInfo *inputDevice,
                                       PaSampleFormat hostInputSampleFormat,
                                       WORD inputChannelCount, 
                                       int bytesPerInputBuffer,
                                       PaWinWaveFormatChannelMask inputChannelMask,
                                       PaWinDsDeviceInfo *outputDevice,
                                       PaSampleFormat hostOutputSampleFormat,
                                       WORD outputChannelCount, 
                                       int bytesPerOutputBuffer,
                                       PaWinWaveFormatChannelMask outputChannelMask,
                                       unsigned long nFrameRate
                                        )
{
    HRESULT hr;
    DSCBUFFERDESC  captureDesc;
    PaWinWaveFormat captureWaveFormat;
    DSBUFFERDESC   secondaryRenderDesc;
    PaWinWaveFormat renderWaveFormat;
    LPDIRECTSOUNDBUFFER8 pRenderBuffer8;
    LPDIRECTSOUNDCAPTUREBUFFER8 pCaptureBuffer8;

    // capture buffer description

    // only try wave format extensible. assume it's available on all ds 8 systems
    PaWin_InitializeWaveFormatExtensible( &captureWaveFormat, inputChannelCount, 
                hostInputSampleFormat, PaWin_SampleFormatToLinearWaveFormatTag( hostInputSampleFormat ),
                nFrameRate, inputChannelMask );

    ZeroMemory(&captureDesc, sizeof(DSCBUFFERDESC));
    captureDesc.dwSize = sizeof(DSCBUFFERDESC);
    captureDesc.dwFlags = 0;
    captureDesc.dwBufferBytes = bytesPerInputBuffer;
    captureDesc.lpwfxFormat = (WAVEFORMATEX*)&captureWaveFormat;

    // render buffer description

    PaWin_InitializeWaveFormatExtensible( &renderWaveFormat, outputChannelCount, 
                hostOutputSampleFormat, PaWin_SampleFormatToLinearWaveFormatTag( hostOutputSampleFormat ),
                nFrameRate, outputChannelMask );

    ZeroMemory(&secondaryRenderDesc, sizeof(DSBUFFERDESC));
    secondaryRenderDesc.dwSize = sizeof(DSBUFFERDESC);
    secondaryRenderDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    secondaryRenderDesc.dwBufferBytes = bytesPerOutputBuffer;
    secondaryRenderDesc.lpwfxFormat = (WAVEFORMATEX*)&renderWaveFormat;

    /* note that we don't create a primary buffer here at all */

    hr = paWinDsDSoundEntryPoints.DirectSoundFullDuplexCreate8( 
            inputDevice->lpGUID, outputDevice->lpGUID,
            &captureDesc, &secondaryRenderDesc,
            GetDesktopWindow(), /* see InitOutputBuffer() for a discussion of whether this is a good idea */
            DSSCL_EXCLUSIVE,
            &stream->pDirectSoundFullDuplex8,
            &pCaptureBuffer8,
            &pRenderBuffer8,
            NULL /* pUnkOuter must be NULL */ 
        );

    if( hr == DS_OK )
    {
        PA_DEBUG(("DirectSoundFullDuplexCreate succeeded!\n"));

        /* retrieve the pre ds 8 buffer interfaces which are used by the rest of the code */

        hr = IUnknown_QueryInterface( pCaptureBuffer8, &IID_IDirectSoundCaptureBuffer, (LPVOID *)&stream->pDirectSoundInputBuffer );
        
        if( hr == DS_OK )
            hr = IUnknown_QueryInterface( pRenderBuffer8, &IID_IDirectSoundBuffer, (LPVOID *)&stream->pDirectSoundOutputBuffer );

        /* release the ds 8 interfaces, we don't need them */
        IUnknown_Release( pCaptureBuffer8 );
        IUnknown_Release( pRenderBuffer8 );

        if( !stream->pDirectSoundInputBuffer || !stream->pDirectSoundOutputBuffer ){
            /* couldn't get pre ds 8 interfaces for some reason. clean up. */
            if( stream->pDirectSoundInputBuffer )
            {
                IUnknown_Release( stream->pDirectSoundInputBuffer );
                stream->pDirectSoundInputBuffer = NULL;
            }

            if( stream->pDirectSoundOutputBuffer )
            {
                IUnknown_Release( stream->pDirectSoundOutputBuffer );
                stream->pDirectSoundOutputBuffer = NULL;
            }
            
            IUnknown_Release( stream->pDirectSoundFullDuplex8 );
            stream->pDirectSoundFullDuplex8 = NULL;
        }
    }
    else
    {
        PA_DEBUG(("DirectSoundFullDuplexCreate failed. hr=%d\n", hr));
    }

    return hr;
}
#endif /* PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE */


static HRESULT InitInputBuffer( PaWinDsStream *stream, PaWinDsDeviceInfo *device, PaSampleFormat sampleFormat, unsigned long nFrameRate, WORD nChannels, int bytesPerBuffer, PaWinWaveFormatChannelMask channelMask )
{
    DSCBUFFERDESC  captureDesc;
    PaWinWaveFormat waveFormat;
    HRESULT        result;
    
    if( (result = paWinDsDSoundEntryPoints.DirectSoundCaptureCreate( 
            device->lpGUID, &stream->pDirectSoundCapture, NULL) ) != DS_OK ){
         ERR_RPT(("PortAudio: DirectSoundCaptureCreate() failed!\n"));
         return result;
    }

    // Setup the secondary buffer description
    ZeroMemory(&captureDesc, sizeof(DSCBUFFERDESC));
    captureDesc.dwSize = sizeof(DSCBUFFERDESC);
    captureDesc.dwFlags = 0;
    captureDesc.dwBufferBytes = bytesPerBuffer;
    captureDesc.lpwfxFormat = (WAVEFORMATEX*)&waveFormat;
    
    // Create the capture buffer

    // first try WAVEFORMATEXTENSIBLE. if this fails, fall back to WAVEFORMATEX
    PaWin_InitializeWaveFormatExtensible( &waveFormat, nChannels, 
                sampleFormat, PaWin_SampleFormatToLinearWaveFormatTag( sampleFormat ),
                nFrameRate, channelMask );

    if( IDirectSoundCapture_CreateCaptureBuffer( stream->pDirectSoundCapture,
                  &captureDesc, &stream->pDirectSoundInputBuffer, NULL) != DS_OK )
    {
        PaWin_InitializeWaveFormatEx( &waveFormat, nChannels, sampleFormat, 
                PaWin_SampleFormatToLinearWaveFormatTag( sampleFormat ), nFrameRate );

        if ((result = IDirectSoundCapture_CreateCaptureBuffer( stream->pDirectSoundCapture,
                    &captureDesc, &stream->pDirectSoundInputBuffer, NULL)) != DS_OK) return result;
    }

    stream->readOffset = 0;  // reset last read position to start of buffer
    return DS_OK;
}


static HRESULT InitOutputBuffer( PaWinDsStream *stream, PaWinDsDeviceInfo *device, PaSampleFormat sampleFormat, unsigned long nFrameRate, WORD nChannels, int bytesPerBuffer, PaWinWaveFormatChannelMask channelMask )
{
    HRESULT        result;
    HWND           hWnd;
    HRESULT        hr;
    PaWinWaveFormat waveFormat;
    DSBUFFERDESC   primaryDesc;
    DSBUFFERDESC   secondaryDesc;
    
    if( (hr = paWinDsDSoundEntryPoints.DirectSoundCreate( 
                device->lpGUID, &stream->pDirectSound, NULL )) != DS_OK ){
        ERR_RPT(("PortAudio: DirectSoundCreate() failed!\n"));
        return hr;
    }

    // We were using getForegroundWindow() but sometimes the ForegroundWindow may not be the
    // applications's window. Also if that window is closed before the Buffer is closed
    // then DirectSound can crash. (Thanks for Scott Patterson for reporting this.)
    // So we will use GetDesktopWindow() which was suggested by Miller Puckette.
    // hWnd = GetForegroundWindow();
    //
    //  FIXME: The example code I have on the net creates a hidden window that
    //      is managed by our code - I think we should do that - one hidden
    //      window for the whole of Pa_DS
    //
    hWnd = GetDesktopWindow();

    // Set cooperative level to DSSCL_EXCLUSIVE so that we can get 16 bit output, 44.1 KHz.
    // exclusive also prevents unexpected sounds from other apps during a performance.
    if ((hr = IDirectSound_SetCooperativeLevel( stream->pDirectSound,
              hWnd, DSSCL_EXCLUSIVE)) != DS_OK)
    {
        return hr;
    }

    // -----------------------------------------------------------------------
    // Create primary buffer and set format just so we can specify our custom format.
    // Otherwise we would be stuck with the default which might be 8 bit or 22050 Hz.
    // Setup the primary buffer description
    ZeroMemory(&primaryDesc, sizeof(DSBUFFERDESC));
    primaryDesc.dwSize        = sizeof(DSBUFFERDESC);
    primaryDesc.dwFlags       = DSBCAPS_PRIMARYBUFFER; // all panning, mixing, etc done by synth
    primaryDesc.dwBufferBytes = 0;
    primaryDesc.lpwfxFormat   = NULL;
    // Create the buffer
    if ((result = IDirectSound_CreateSoundBuffer( stream->pDirectSound,
                  &primaryDesc, &stream->pDirectSoundPrimaryBuffer, NULL)) != DS_OK)
        goto error;

    // Set the primary buffer's format

    // first try WAVEFORMATEXTENSIBLE. if this fails, fall back to WAVEFORMATEX
    PaWin_InitializeWaveFormatExtensible( &waveFormat, nChannels, 
                sampleFormat, PaWin_SampleFormatToLinearWaveFormatTag( sampleFormat ),
                nFrameRate, channelMask );

    if( IDirectSoundBuffer_SetFormat( stream->pDirectSoundPrimaryBuffer, (WAVEFORMATEX*)&waveFormat) != DS_OK )
    {
        PaWin_InitializeWaveFormatEx( &waveFormat, nChannels, sampleFormat, 
                PaWin_SampleFormatToLinearWaveFormatTag( sampleFormat ), nFrameRate );

        if((result = IDirectSoundBuffer_SetFormat( stream->pDirectSoundPrimaryBuffer, (WAVEFORMATEX*)&waveFormat)) != DS_OK)
            goto error;
    }

    // ----------------------------------------------------------------------
    // Setup the secondary buffer description
    ZeroMemory(&secondaryDesc, sizeof(DSBUFFERDESC));
    secondaryDesc.dwSize = sizeof(DSBUFFERDESC);
    secondaryDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
    secondaryDesc.dwBufferBytes = bytesPerBuffer;
    secondaryDesc.lpwfxFormat = (WAVEFORMATEX*)&waveFormat; /* waveFormat contains whatever format was negotiated for the primary buffer above */
    // Create the secondary buffer
    if ((result = IDirectSound_CreateSoundBuffer( stream->pDirectSound,
                  &secondaryDesc, &stream->pDirectSoundOutputBuffer, NULL)) != DS_OK)
      goto error;
    
    return DS_OK;

error:

    if( stream->pDirectSoundPrimaryBuffer )
    {
        IDirectSoundBuffer_Release( stream->pDirectSoundPrimaryBuffer );
        stream->pDirectSoundPrimaryBuffer = NULL;
    }

    return result;
}

/***********************************************************************************/
/* see pa_hostapi.h for a list of validity guarantees made about OpenStream parameters */

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
    PaError result = paNoError;
    PaWinDsHostApiRepresentation *winDsHostApi = (PaWinDsHostApiRepresentation*)hostApi;
    PaWinDsStream *stream = 0;
    int bufferProcessorIsInitialized = 0;
    int streamRepresentationIsInitialized = 0;
    PaWinDsDeviceInfo *inputWinDsDeviceInfo, *outputWinDsDeviceInfo;
    PaDeviceInfo *inputDeviceInfo, *outputDeviceInfo;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;
    unsigned long suggestedInputLatencyFrames, suggestedOutputLatencyFrames;
    PaWinDirectSoundStreamInfo *inputStreamInfo, *outputStreamInfo;
    PaWinWaveFormatChannelMask inputChannelMask, outputChannelMask;

    if( inputParameters )
    {
        inputWinDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[ inputParameters->device ];
        inputDeviceInfo = &inputWinDsDeviceInfo->inheritedDeviceInfo;

        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
        suggestedInputLatencyFrames = (unsigned long)(inputParameters->suggestedLatency * sampleRate);

        /* IDEA: the following 3 checks could be performed by default by pa_front
            unless some flag indicated otherwise */
            
        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if( inputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if( inputWinDsDeviceInfo->deviceInputChannelCountIsKnown
                && inputChannelCount > inputDeviceInfo->maxInputChannels )
            return paInvalidChannelCount;
            
        /* validate hostApiSpecificStreamInfo */
        inputStreamInfo = (PaWinDirectSoundStreamInfo*)inputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinDirectSoundSpecificStreamInfo( inputParameters, inputStreamInfo );
		if( result != paNoError ) return result;

        if( inputStreamInfo && inputStreamInfo->flags & paWinDirectSoundUseChannelMask )
            inputChannelMask = inputStreamInfo->channelMask;
        else
            inputChannelMask = PaWin_DefaultChannelMask( inputChannelCount );
    }
    else
    {
        inputChannelCount = 0;
		inputSampleFormat = 0;
        suggestedInputLatencyFrames = 0;
    }


    if( outputParameters )
    {
        outputWinDsDeviceInfo = (PaWinDsDeviceInfo*) hostApi->deviceInfos[ outputParameters->device ];
        outputDeviceInfo = &outputWinDsDeviceInfo->inheritedDeviceInfo;

        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
        suggestedOutputLatencyFrames = (unsigned long)(outputParameters->suggestedLatency * sampleRate);

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if( outputParameters->device == paUseHostApiSpecificDeviceSpecification )
            return paInvalidDevice;

        /* check that output device can support outputChannelCount */
        if( outputWinDsDeviceInfo->deviceOutputChannelCountIsKnown
                && outputChannelCount > outputDeviceInfo->maxOutputChannels )
            return paInvalidChannelCount;

        /* validate hostApiSpecificStreamInfo */
        outputStreamInfo = (PaWinDirectSoundStreamInfo*)outputParameters->hostApiSpecificStreamInfo;
		result = ValidateWinDirectSoundSpecificStreamInfo( outputParameters, outputStreamInfo );
		if( result != paNoError ) return result;   

        if( outputStreamInfo && outputStreamInfo->flags & paWinDirectSoundUseChannelMask )
            outputChannelMask = outputStreamInfo->channelMask;
        else
            outputChannelMask = PaWin_DefaultChannelMask( outputChannelCount );
    }
    else
    {
        outputChannelCount = 0;
		outputSampleFormat = 0;
        suggestedOutputLatencyFrames = 0;
    }


    /*
        IMPLEMENT ME:

        ( the following two checks are taken care of by PaUtil_InitializeBufferProcessor() )

            - check that input device can support inputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - check that output device can support outputSampleFormat, or that
                we have the capability to convert from outputSampleFormat to
                a native format

            - if a full duplex stream is requested, check that the combination
                of input and output parameters is supported

            - check that the device supports sampleRate

            - alter sampleRate to a close allowable rate if possible / necessary

            - validate suggestedInputLatency and suggestedOutputLatency parameters,
                use default values where necessary
    */


    /* validate platform specific flags */
    if( (streamFlags & paPlatformSpecificFlags) != 0 )
        return paInvalidFlag; /* unexpected platform specific flag */


    stream = (PaWinDsStream*)PaUtil_AllocateMemory( sizeof(PaWinDsStream) );
    if( !stream )
    {
        result = paInsufficientMemory;
        goto error;
    }

    memset( stream, 0, sizeof(PaWinDsStream) ); /* initialize all stream variables to 0 */

    if( streamCallback )
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &winDsHostApi->callbackStreamInterface, streamCallback, userData );
    }
    else
    {
        PaUtil_InitializeStreamRepresentation( &stream->streamRepresentation,
                                               &winDsHostApi->blockingStreamInterface, streamCallback, userData );
    }
    
    streamRepresentationIsInitialized = 1;

    stream->streamFlags = streamFlags;

    PaUtil_InitializeCpuLoadMeasurer( &stream->cpuLoadMeasurer, sampleRate );


    if( inputParameters )
    {
        /* IMPLEMENT ME - establish which  host formats are available */
        PaSampleFormat nativeInputFormats = paInt16;
        //PaSampleFormat nativeFormats = paUInt8 | paInt16 | paInt24 | paInt32 | paFloat32;

        hostInputSampleFormat =
            PaUtil_SelectClosestAvailableFormat( nativeInputFormats, inputParameters->sampleFormat );
    }
	else
	{
		hostInputSampleFormat = 0;
	}

    if( outputParameters )
    {
        /* IMPLEMENT ME - establish which  host formats are available */
        PaSampleFormat nativeOutputFormats = paInt16;
        //PaSampleFormat nativeOutputFormats = paUInt8 | paInt16 | paInt24 | paInt32 | paFloat32;

        hostOutputSampleFormat =
            PaUtil_SelectClosestAvailableFormat( nativeOutputFormats, outputParameters->sampleFormat );
    }
    else
	{
		hostOutputSampleFormat = 0;
	}

    result =  PaUtil_InitializeBufferProcessor( &stream->bufferProcessor,
                    inputChannelCount, inputSampleFormat, hostInputSampleFormat,
                    outputChannelCount, outputSampleFormat, hostOutputSampleFormat,
                    sampleRate, streamFlags, framesPerBuffer,
                    framesPerBuffer, /* ignored in paUtilVariableHostBufferSizePartialUsageAllowed mode. */
                /* This next mode is required because DS can split the host buffer when it wraps around. */
                    paUtilVariableHostBufferSizePartialUsageAllowed,
                    streamCallback, userData );
    if( result != paNoError )
        goto error;

    bufferProcessorIsInitialized = 1;

    stream->streamRepresentation.streamInfo.inputLatency =
            (PaTime)PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor) / sampleRate;   /* FIXME: only includes buffer processor latency */
    stream->streamRepresentation.streamInfo.outputLatency =
            (PaTime)PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor) / sampleRate;    /* FIXME: only includes buffer processor latency */
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

    
/* DirectSound specific initialization */ 
    {
        HRESULT          hr;
        int              bytesPerDirectSoundInputBuffer, bytesPerDirectSoundOutputBuffer;
        int              userLatencyFrames;
        int              minLatencyFrames;
        unsigned long    integerSampleRate = (unsigned long) (sampleRate + 0.5);
    
        stream->timerID = 0;

        stream->processingCompleted = CreateEvent( NULL, /* bManualReset = */ TRUE, /* bInitialState = */ FALSE, NULL );
        if( stream->processingCompleted == NULL )
        {
            result = paInsufficientMemory;
            goto error;
        }

    /* Get system minimum latency. */
        minLatencyFrames = PaWinDs_GetMinLatencyFrames( sampleRate );

    /* Let user override latency by passing latency parameter. */
        userLatencyFrames = (suggestedInputLatencyFrames > suggestedOutputLatencyFrames)
                    ? suggestedInputLatencyFrames
                    : suggestedOutputLatencyFrames;
        if( userLatencyFrames > 0 ) minLatencyFrames = userLatencyFrames;

    /* Calculate stream->framesPerDSBuffer depending on framesPerBuffer */
        if( framesPerBuffer == paFramesPerBufferUnspecified )
        {
        /* App support variable framesPerBuffer */
            stream->framesPerDSBuffer = minLatencyFrames;

            stream->streamRepresentation.streamInfo.outputLatency = (double)(minLatencyFrames - 1) / sampleRate;
        }
        else
        {
        /* Round up to number of buffers needed to guarantee that latency. */
            int numUserBuffers = (minLatencyFrames + framesPerBuffer - 1) / framesPerBuffer;
            if( numUserBuffers < 1 ) numUserBuffers = 1;
            numUserBuffers += 1; /* So we have latency worth of buffers ahead of current buffer. */
            stream->framesPerDSBuffer = framesPerBuffer * numUserBuffers;

            stream->streamRepresentation.streamInfo.outputLatency = (double)(framesPerBuffer * (numUserBuffers-1)) / sampleRate;
        }

        {
            /** @todo REVIEW: this calculation seems incorrect to me - rossb. */
            int msecLatency = (int) ((stream->framesPerDSBuffer * MSEC_PER_SECOND) / sampleRate);
            PRINT(("PortAudio on DirectSound - Latency = %d frames, %d msec\n", stream->framesPerDSBuffer, msecLatency ));
        }

        /* set up i/o parameters */

        /* ------------------ OUTPUT */
        if( outputParameters )
        {
            LARGE_INTEGER  counterFrequency;

            /*
            PaDeviceInfo *deviceInfo = hostApi->deviceInfos[ outputParameters->device ];
            DBUG(("PaHost_OpenStream: deviceID = 0x%x\n", outputParameters->device));
            */
            
            int bytesPerSample = Pa_GetSampleSize(hostOutputSampleFormat);
            bytesPerDirectSoundOutputBuffer = stream->framesPerDSBuffer * outputParameters->channelCount * bytesPerSample;
            if( bytesPerDirectSoundOutputBuffer < DSBSIZE_MIN )
            {
                result = paBufferTooSmall;
                goto error;
            }
            else if( bytesPerDirectSoundOutputBuffer > DSBSIZE_MAX )
            {
                result = paBufferTooBig;
                goto error;
            }

            /* Calculate value used in latency calculation to avoid real-time divides. */
            stream->secondsPerHostByte = 1.0 /
                (stream->bufferProcessor.bytesPerHostOutputSample *
                outputChannelCount * sampleRate);

            stream->outputBufferSizeBytes = bytesPerDirectSoundOutputBuffer;
            stream->outputIsRunning = FALSE;
            stream->outputUnderflowCount = 0;
            stream->bytesPerOutputFrame = outputParameters->channelCount * bytesPerSample;

            /* perfCounterTicksPerBuffer is used by QueryOutputSpace for overflow detection */
            if( QueryPerformanceFrequency( &counterFrequency ) )
            {
                stream->perfCounterTicksPerBuffer.QuadPart = (counterFrequency.QuadPart * stream->framesPerDSBuffer) / integerSampleRate;
            }
            else
            {
                stream->perfCounterTicksPerBuffer.QuadPart = 0;
            }
        }

        /* ------------------ INPUT */
        if( inputParameters )
        {
            /*
            PaDeviceInfo *deviceInfo = hostApi->deviceInfos[ inputParameters->device ];
            DBUG(("PaHost_OpenStream: deviceID = 0x%x\n", inputParameters->device));
            */
            
            int bytesPerSample = Pa_GetSampleSize(hostInputSampleFormat);
            bytesPerDirectSoundInputBuffer = stream->framesPerDSBuffer * inputParameters->channelCount * bytesPerSample;
            if( bytesPerDirectSoundInputBuffer < DSBSIZE_MIN )
            {
                result = paBufferTooSmall;
                goto error;
            }
            else if( bytesPerDirectSoundInputBuffer > DSBSIZE_MAX )
            {
                result = paBufferTooBig;
                goto error;
            }

            stream->bytesPerInputFrame = inputParameters->channelCount * bytesPerSample;

            stream->inputSize = bytesPerDirectSoundInputBuffer;
        }

        /* open/create the DirectSound buffers */

        /* interface ptrs should be zeroed when stream is zeroed. */
        assert( stream->pDirectSoundCapture == NULL );
        assert( stream->pDirectSoundInputBuffer == NULL );
        assert( stream->pDirectSound == NULL );
        assert( stream->pDirectSoundPrimaryBuffer == NULL );
        assert( stream->pDirectSoundOutputBuffer == NULL );
        

        if( inputParameters && outputParameters )
        {
#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
            /* try to use the full-duplex DX8 API to create the buffers.
                if that fails we fall back to the half-duplex API below */

            hr = InitFullDuplexInputOutputBuffers( stream,
                                       (PaWinDsDeviceInfo*)hostApi->deviceInfos[inputParameters->device],
                                       hostInputSampleFormat,
                                       (WORD)inputParameters->channelCount, bytesPerDirectSoundInputBuffer,
                                       inputChannelMask,
                                       (PaWinDsDeviceInfo*)hostApi->deviceInfos[outputParameters->device],
                                       hostOutputSampleFormat,
                                       (WORD)outputParameters->channelCount, bytesPerDirectSoundOutputBuffer,
                                       outputChannelMask,
                                       integerSampleRate
                                        );
            DBUG(("InitFullDuplexInputOutputBuffers() returns %x\n", hr));
            /* ignore any error returned by InitFullDuplexInputOutputBuffers. 
                we retry opening the buffers below */
#endif /* PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE */
        }

        /*  create half duplex buffers. also used for full-duplex streams which didn't 
            succeed when using the full duplex API. that could happen because
            DX8 or greater isnt installed, the i/o devices aren't the same 
            physical device. etc.
        */

        if( outputParameters && !stream->pDirectSoundOutputBuffer )
        {
            hr = InitOutputBuffer( stream,
                                       (PaWinDsDeviceInfo*)hostApi->deviceInfos[outputParameters->device],
                                       hostOutputSampleFormat,
                                       integerSampleRate,
                                       (WORD)outputParameters->channelCount, bytesPerDirectSoundOutputBuffer,
                                       outputChannelMask );
            DBUG(("InitOutputBuffer() returns %x\n", hr));
            if( hr != DS_OK )
            {
                result = paUnanticipatedHostError;
                PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
                goto error;
            }
        }

        if( inputParameters && !stream->pDirectSoundInputBuffer )
        {
            hr = InitInputBuffer( stream,
                                      (PaWinDsDeviceInfo*)hostApi->deviceInfos[inputParameters->device],
                                      hostInputSampleFormat,
                                      integerSampleRate,
                                      (WORD)inputParameters->channelCount, bytesPerDirectSoundInputBuffer,
                                      inputChannelMask );
            DBUG(("InitInputBuffer() returns %x\n", hr));
            if( hr != DS_OK )
            {
                ERR_RPT(("PortAudio: DSW_InitInputBuffer() returns %x\n", hr));
                result = paUnanticipatedHostError;
                PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
                goto error;
            }
        }
    }

    *s = (PaStream*)stream;

    return result;

error:
    if( stream )
    {
        if( stream->processingCompleted != NULL )
            CloseHandle( stream->processingCompleted );

        if( stream->pDirectSoundOutputBuffer )
        {
            IDirectSoundBuffer_Stop( stream->pDirectSoundOutputBuffer );
            IDirectSoundBuffer_Release( stream->pDirectSoundOutputBuffer );
            stream->pDirectSoundOutputBuffer = NULL;
        }

        if( stream->pDirectSoundPrimaryBuffer )
        {
            IDirectSoundBuffer_Release( stream->pDirectSoundPrimaryBuffer );
            stream->pDirectSoundPrimaryBuffer = NULL;
        }

        if( stream->pDirectSoundInputBuffer )
        {
            IDirectSoundCaptureBuffer_Stop( stream->pDirectSoundInputBuffer );
            IDirectSoundCaptureBuffer_Release( stream->pDirectSoundInputBuffer );
            stream->pDirectSoundInputBuffer = NULL;
        }

        if( stream->pDirectSoundCapture )
        {
            IDirectSoundCapture_Release( stream->pDirectSoundCapture );
            stream->pDirectSoundCapture = NULL;
        }

        if( stream->pDirectSound )
        {
            IDirectSound_Release( stream->pDirectSound );
            stream->pDirectSound = NULL;
        }

#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
        if( stream->pDirectSoundFullDuplex8 )
        {
            IDirectSoundFullDuplex_Release( stream->pDirectSoundFullDuplex8 );
            stream->pDirectSoundFullDuplex8 = NULL;
        }
#endif
        if( bufferProcessorIsInitialized )
            PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );

        if( streamRepresentationIsInitialized )
            PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );

        PaUtil_FreeMemory( stream );
    }

    return result;
}


/************************************************************************************
 * Determine how much space can be safely written to in DS buffer.
 * Detect underflows and overflows.
 * Does not allow writing into safety gap maintained by DirectSound.
 */
static HRESULT QueryOutputSpace( PaWinDsStream *stream, long *bytesEmpty )
{
    HRESULT hr;
    DWORD   playCursor;
    DWORD   writeCursor;
    long    numBytesEmpty;
    long    playWriteGap;
    // Query to see how much room is in buffer.
    hr = IDirectSoundBuffer_GetCurrentPosition( stream->pDirectSoundOutputBuffer,
            &playCursor, &writeCursor );
    if( hr != DS_OK )
    {
        return hr;
    }

    // Determine size of gap between playIndex and WriteIndex that we cannot write into.
    playWriteGap = writeCursor - playCursor;
    if( playWriteGap < 0 ) playWriteGap += stream->outputBufferSizeBytes; // unwrap

    /* DirectSound doesn't have a large enough playCursor so we cannot detect wrap-around. */
    /* Attempt to detect playCursor wrap-around and correct it. */
    if( stream->outputIsRunning && (stream->perfCounterTicksPerBuffer.QuadPart != 0) )
    {
        /* How much time has elapsed since last check. */
        LARGE_INTEGER   currentTime;
        LARGE_INTEGER   elapsedTime;
        long            bytesPlayed;
        long            bytesExpected;
        long            buffersWrapped;

        QueryPerformanceCounter( &currentTime );
        elapsedTime.QuadPart = currentTime.QuadPart - stream->previousPlayTime.QuadPart;
        stream->previousPlayTime = currentTime;

        /* How many bytes does DirectSound say have been played. */
        bytesPlayed = playCursor - stream->previousPlayCursor;
        if( bytesPlayed < 0 ) bytesPlayed += stream->outputBufferSizeBytes; // unwrap
        stream->previousPlayCursor = playCursor;

        /* Calculate how many bytes we would have expected to been played by now. */
        bytesExpected = (long) ((elapsedTime.QuadPart * stream->outputBufferSizeBytes) / stream->perfCounterTicksPerBuffer.QuadPart);
        buffersWrapped = (bytesExpected - bytesPlayed) / stream->outputBufferSizeBytes;
        if( buffersWrapped > 0 )
        {
            playCursor += (buffersWrapped * stream->outputBufferSizeBytes);
            bytesPlayed += (buffersWrapped * stream->outputBufferSizeBytes);
        }
    }
    numBytesEmpty = playCursor - stream->outputBufferWriteOffsetBytes;
    if( numBytesEmpty < 0 ) numBytesEmpty += stream->outputBufferSizeBytes; // unwrap offset

    /* Have we underflowed? */
    if( numBytesEmpty > (stream->outputBufferSizeBytes - playWriteGap) )
    {
        if( stream->outputIsRunning )
        {
            stream->outputUnderflowCount += 1;
        }

        /*
            From MSDN:
                The write cursor indicates the position at which it is safe  
            to write new data to the buffer. The write cursor always leads the
            play cursor, typically by about 15 milliseconds' worth of audio
            data.
                It is always safe to change data that is behind the position 
            indicated by the lpdwCurrentPlayCursor parameter.
        */

        stream->outputBufferWriteOffsetBytes = writeCursor;
        numBytesEmpty = stream->outputBufferSizeBytes - playWriteGap;
    }
    *bytesEmpty = numBytesEmpty;
    return hr;
}

/***********************************************************************************/
static int TimeSlice( PaWinDsStream *stream )
{
    long              numFrames = 0;
    long              bytesEmpty = 0;
    long              bytesFilled = 0;
    long              bytesToXfer = 0;
    long              framesToXfer = 0;
    long              numInFramesReady = 0;
    long              numOutFramesReady = 0;
    long              bytesProcessed;
    HRESULT           hresult;
    double            outputLatency = 0;
    PaStreamCallbackTimeInfo timeInfo = {0,0,0}; /** @todo implement inputBufferAdcTime */
    
/* Input */
    LPBYTE            lpInBuf1 = NULL;
    LPBYTE            lpInBuf2 = NULL;
    DWORD             dwInSize1 = 0;
    DWORD             dwInSize2 = 0;
/* Output */
    LPBYTE            lpOutBuf1 = NULL;
    LPBYTE            lpOutBuf2 = NULL;
    DWORD             dwOutSize1 = 0;
    DWORD             dwOutSize2 = 0;

    /* How much input data is available? */
    if( stream->bufferProcessor.inputChannelCount > 0 )
    {
        HRESULT hr;
        DWORD capturePos;
        DWORD readPos;
        long  filled = 0;
        // Query to see how much data is in buffer.
        // We don't need the capture position but sometimes DirectSound doesn't handle NULLS correctly
        // so let's pass a pointer just to be safe.
        hr = IDirectSoundCaptureBuffer_GetCurrentPosition( stream->pDirectSoundInputBuffer, &capturePos, &readPos );
        if( hr == DS_OK )
        {
            filled = readPos - stream->readOffset;
            if( filled < 0 ) filled += stream->inputSize; // unwrap offset
            bytesFilled = filled;
        }
            // FIXME: what happens if IDirectSoundCaptureBuffer_GetCurrentPosition fails?

        framesToXfer = numInFramesReady = bytesFilled / stream->bytesPerInputFrame; 
        outputLatency = ((double)bytesFilled) * stream->secondsPerHostByte;  // FIXME: this doesn't look right. we're calculating output latency in input branch. also secondsPerHostByte is only initialized for the output stream

        /** @todo Check for overflow */
    }

    /* How much output room is available? */
    if( stream->bufferProcessor.outputChannelCount > 0 )
    {
        UINT previousUnderflowCount = stream->outputUnderflowCount;
        QueryOutputSpace( stream, &bytesEmpty );
        framesToXfer = numOutFramesReady = bytesEmpty / stream->bytesPerOutputFrame;

        /* Check for underflow */
        if( stream->outputUnderflowCount != previousUnderflowCount )
            stream->callbackFlags |= paOutputUnderflow;
    }

    if( (numInFramesReady > 0) && (numOutFramesReady > 0) )
    {
        framesToXfer = (numOutFramesReady < numInFramesReady) ? numOutFramesReady : numInFramesReady;
    }

    if( framesToXfer > 0 )
    {

        PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

    /* The outputBufferDacTime parameter should indicates the time at which
        the first sample of the output buffer is heard at the DACs. */
        timeInfo.currentTime = PaUtil_GetTime();
        timeInfo.outputBufferDacTime = timeInfo.currentTime + outputLatency; // FIXME: QueryOutputSpace gets the playback position, we could use that (?)


        PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo, stream->callbackFlags );
        stream->callbackFlags = 0;
        
    /* Input */
        if( stream->bufferProcessor.inputChannelCount > 0 )
        {
            bytesToXfer = framesToXfer * stream->bytesPerInputFrame;
            hresult = IDirectSoundCaptureBuffer_Lock ( stream->pDirectSoundInputBuffer,
                stream->readOffset, bytesToXfer,
                (void **) &lpInBuf1, &dwInSize1,
                (void **) &lpInBuf2, &dwInSize2, 0);
            if (hresult != DS_OK)
            {
                ERR_RPT(("DirectSound IDirectSoundCaptureBuffer_Lock failed, hresult = 0x%x\n",hresult));
                /* PA_DS_SET_LAST_DIRECTSOUND_ERROR( hresult ); */
                PaUtil_ResetBufferProcessor( &stream->bufferProcessor ); /* flush the buffer processor */
                stream->callbackResult = paComplete;
                goto error2;
            }

            numFrames = dwInSize1 / stream->bytesPerInputFrame;
            PaUtil_SetInputFrameCount( &stream->bufferProcessor, numFrames );
            PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor, 0, lpInBuf1, 0 );
        /* Is input split into two regions. */
            if( dwInSize2 > 0 )
            {
                numFrames = dwInSize2 / stream->bytesPerInputFrame;
                PaUtil_Set2ndInputFrameCount( &stream->bufferProcessor, numFrames );
                PaUtil_Set2ndInterleavedInputChannels( &stream->bufferProcessor, 0, lpInBuf2, 0 );
            }
        }

    /* Output */
        if( stream->bufferProcessor.outputChannelCount > 0 )
        {
            bytesToXfer = framesToXfer * stream->bytesPerOutputFrame;
            hresult = IDirectSoundBuffer_Lock ( stream->pDirectSoundOutputBuffer,
                stream->outputBufferWriteOffsetBytes, bytesToXfer,
                (void **) &lpOutBuf1, &dwOutSize1,
                (void **) &lpOutBuf2, &dwOutSize2, 0);
            if (hresult != DS_OK)
            {
                ERR_RPT(("DirectSound IDirectSoundBuffer_Lock failed, hresult = 0x%x\n",hresult));
                /* PA_DS_SET_LAST_DIRECTSOUND_ERROR( hresult ); */
                PaUtil_ResetBufferProcessor( &stream->bufferProcessor ); /* flush the buffer processor */
                stream->callbackResult = paComplete;
                goto error1;
            }

            numFrames = dwOutSize1 / stream->bytesPerOutputFrame;
            PaUtil_SetOutputFrameCount( &stream->bufferProcessor, numFrames );
            PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor, 0, lpOutBuf1, 0 );

        /* Is output split into two regions. */
            if( dwOutSize2 > 0 )
            {
                numFrames = dwOutSize2 / stream->bytesPerOutputFrame;
                PaUtil_Set2ndOutputFrameCount( &stream->bufferProcessor, numFrames );
                PaUtil_Set2ndInterleavedOutputChannels( &stream->bufferProcessor, 0, lpOutBuf2, 0 );
            }
        }

        numFrames = PaUtil_EndBufferProcessing( &stream->bufferProcessor, &stream->callbackResult );
        stream->framesWritten += numFrames;
        
        if( stream->bufferProcessor.outputChannelCount > 0 )
        {
        /* FIXME: an underflow could happen here */

        /* Update our buffer offset and unlock sound buffer */
            bytesProcessed = numFrames * stream->bytesPerOutputFrame;
            stream->outputBufferWriteOffsetBytes = (stream->outputBufferWriteOffsetBytes + bytesProcessed) % stream->outputBufferSizeBytes;
            IDirectSoundBuffer_Unlock( stream->pDirectSoundOutputBuffer, lpOutBuf1, dwOutSize1, lpOutBuf2, dwOutSize2);
        }

error1:
        if( stream->bufferProcessor.inputChannelCount > 0 )
        {
        /* FIXME: an overflow could happen here */

        /* Update our buffer offset and unlock sound buffer */
            bytesProcessed = numFrames * stream->bytesPerInputFrame;
            stream->readOffset = (stream->readOffset + bytesProcessed) % stream->inputSize;
            IDirectSoundCaptureBuffer_Unlock( stream->pDirectSoundInputBuffer, lpInBuf1, dwInSize1, lpInBuf2, dwInSize2);
        }
error2:

        PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, numFrames );        
    }

    if( stream->callbackResult == paComplete && !PaUtil_IsBufferProcessorOutputEmpty( &stream->bufferProcessor ) )
    {
        /* don't return completed until the buffer processor has been drained */
        return paContinue;
    }
    else
    {
        return stream->callbackResult;
    }
}
/*******************************************************************/

static HRESULT ZeroAvailableOutputSpace( PaWinDsStream *stream )
{
    HRESULT hr;
    LPBYTE lpbuf1 = NULL;
    LPBYTE lpbuf2 = NULL;
    DWORD dwsize1 = 0;
    DWORD dwsize2 = 0;
    long  bytesEmpty;
    hr = QueryOutputSpace( stream, &bytesEmpty );
    if (hr != DS_OK) return hr;
    if( bytesEmpty == 0 ) return DS_OK;
    // Lock free space in the DS
    hr = IDirectSoundBuffer_Lock( stream->pDirectSoundOutputBuffer, stream->outputBufferWriteOffsetBytes,
                                    bytesEmpty, (void **) &lpbuf1, &dwsize1,
                                    (void **) &lpbuf2, &dwsize2, 0);
    if (hr == DS_OK)
    {
        // Copy the buffer into the DS
        ZeroMemory(lpbuf1, dwsize1);
        if(lpbuf2 != NULL)
        {
            ZeroMemory(lpbuf2, dwsize2);
        }
        // Update our buffer offset and unlock sound buffer
        stream->outputBufferWriteOffsetBytes = (stream->outputBufferWriteOffsetBytes + dwsize1 + dwsize2) % stream->outputBufferSizeBytes;
        IDirectSoundBuffer_Unlock( stream->pDirectSoundOutputBuffer, lpbuf1, dwsize1, lpbuf2, dwsize2);

        stream->finalZeroBytesWritten += dwsize1 + dwsize2;
    }
    return hr;
}


static void CALLBACK TimerCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD dw1, DWORD dw2)
{
    PaWinDsStream *stream;
    int isFinished = 0;

    /* suppress unused variable warnings */
    (void) uID;
    (void) uMsg;
    (void) dw1;
    (void) dw2;
    
    stream = (PaWinDsStream *) dwUser;
    if( stream == NULL ) return;

    if( stream->isActive )
    {
        if( stream->abortProcessing )
        {
            isFinished = 1;
        }
        else if( stream->stopProcessing )
        {
            if( stream->bufferProcessor.outputChannelCount > 0 )
            {
                ZeroAvailableOutputSpace( stream );
                if( stream->finalZeroBytesWritten >= stream->outputBufferSizeBytes )
                {
                    /* once we've flushed the whole output buffer with zeros we know all data has been played */
                    isFinished = 1;
                }
            }
            else
            {
                isFinished = 1;
            }
        }
        else
        {
            int callbackResult = TimeSlice( stream );
            if( callbackResult != paContinue )
            {
                /* FIXME implement handling of paComplete and paAbort if possible 
                   At the moment this should behave as if paComplete was called and 
                   flush the buffer.
                */

                stream->stopProcessing = 1;
            }
        }

        if( isFinished )
        {
            if( stream->streamRepresentation.streamFinishedCallback != 0 )
                stream->streamRepresentation.streamFinishedCallback( stream->streamRepresentation.userData );

            stream->isActive = 0; /* don't set this until the stream really is inactive */
            SetEvent( stream->processingCompleted );
        }
    }
}

/***********************************************************************************
    When CloseStream() is called, the multi-api layer ensures that
    the stream has already been stopped or aborted.
*/
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaWinDsStream *stream = (PaWinDsStream*)s;

    CloseHandle( stream->processingCompleted );

    // Cleanup the sound buffers
    if( stream->pDirectSoundOutputBuffer )
    {
        IDirectSoundBuffer_Stop( stream->pDirectSoundOutputBuffer );
        IDirectSoundBuffer_Release( stream->pDirectSoundOutputBuffer );
        stream->pDirectSoundOutputBuffer = NULL;
    }

    if( stream->pDirectSoundPrimaryBuffer )
    {
        IDirectSoundBuffer_Release( stream->pDirectSoundPrimaryBuffer );
        stream->pDirectSoundPrimaryBuffer = NULL;
    }

    if( stream->pDirectSoundInputBuffer )
    {
        IDirectSoundCaptureBuffer_Stop( stream->pDirectSoundInputBuffer );
        IDirectSoundCaptureBuffer_Release( stream->pDirectSoundInputBuffer );
        stream->pDirectSoundInputBuffer = NULL;
    }

    if( stream->pDirectSoundCapture )
    {
        IDirectSoundCapture_Release( stream->pDirectSoundCapture );
        stream->pDirectSoundCapture = NULL;
    }

    if( stream->pDirectSound )
    {
        IDirectSound_Release( stream->pDirectSound );
        stream->pDirectSound = NULL;
    }

#ifdef PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE
    if( stream->pDirectSoundFullDuplex8 )
    {
        IDirectSoundFullDuplex_Release( stream->pDirectSoundFullDuplex8 );
        stream->pDirectSoundFullDuplex8 = NULL;
    }
#endif

    PaUtil_TerminateBufferProcessor( &stream->bufferProcessor );
    PaUtil_TerminateStreamRepresentation( &stream->streamRepresentation );
    PaUtil_FreeMemory( stream );

    return result;
}

/***********************************************************************************/
static HRESULT ClearOutputBuffer( PaWinDsStream *stream )
{
    PaError          result = paNoError;
    unsigned char*   pDSBuffData;
    DWORD            dwDataLen;
    HRESULT          hr;

    hr = IDirectSoundBuffer_SetCurrentPosition( stream->pDirectSoundOutputBuffer, 0 );
    DBUG(("PaHost_ClearOutputBuffer: IDirectSoundBuffer_SetCurrentPosition returned = 0x%X.\n", hr));
    if( hr != DS_OK )
        return hr;

    // Lock the DS buffer
    if ((hr = IDirectSoundBuffer_Lock( stream->pDirectSoundOutputBuffer, 0, stream->outputBufferSizeBytes, (LPVOID*)&pDSBuffData,
                                           &dwDataLen, NULL, 0, 0)) != DS_OK )
        return hr;

    // Zero the DS buffer
    ZeroMemory(pDSBuffData, dwDataLen);
    // Unlock the DS buffer
    if ((hr = IDirectSoundBuffer_Unlock( stream->pDirectSoundOutputBuffer, pDSBuffData, dwDataLen, NULL, 0)) != DS_OK)
        return hr;
    
    // Let DSound set the starting write position because if we set it to zero, it looks like the
    // buffer is full to begin with. This causes a long pause before sound starts when using large buffers.
    if ((hr = IDirectSoundBuffer_GetCurrentPosition( stream->pDirectSoundOutputBuffer,
            &stream->previousPlayCursor, &stream->outputBufferWriteOffsetBytes )) != DS_OK)
        return hr;

    /* printf("DSW_InitOutputBuffer: playCursor = %d, writeCursor = %d\n", playCursor, dsw->dsw_WriteOffset ); */

    return DS_OK;
}

static PaError StartStream( PaStream *s )
{
    PaError          result = paNoError;
    PaWinDsStream   *stream = (PaWinDsStream*)s;
    HRESULT          hr;
        
    stream->callbackResult = paContinue;
    PaUtil_ResetBufferProcessor( &stream->bufferProcessor );
    
    ResetEvent( stream->processingCompleted );

    if( stream->bufferProcessor.inputChannelCount > 0 )
    {
        // Start the buffer capture
        if( stream->pDirectSoundInputBuffer != NULL ) // FIXME: not sure this check is necessary
        {
            hr = IDirectSoundCaptureBuffer_Start( stream->pDirectSoundInputBuffer, DSCBSTART_LOOPING );
        }

        DBUG(("StartStream: DSW_StartInput returned = 0x%X.\n", hr));
        if( hr != DS_OK )
        {
            result = paUnanticipatedHostError;
            PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
            goto error;
        }
    }

    stream->framesWritten = 0;
    stream->callbackFlags = 0;

    stream->abortProcessing = 0;
    stream->stopProcessing = 0;
    stream->isActive = 1;

    if( stream->bufferProcessor.outputChannelCount > 0 )
    {
        QueryPerformanceCounter( &stream->previousPlayTime );
        stream->finalZeroBytesWritten = 0;

        hr = ClearOutputBuffer( stream );
        if( hr != DS_OK )
        {
            result = paUnanticipatedHostError;
            PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
            goto error;
        }

        if( stream->streamRepresentation.streamCallback && (stream->streamFlags & paPrimeOutputBuffersUsingStreamCallback) )
        {
            stream->callbackFlags = paPrimingOutput;

            TimeSlice( stream );
            /* we ignore the return value from TimeSlice here and start the stream as usual.
                The first timer callback will detect if the callback has completed. */

            stream->callbackFlags = 0;
        }

        // Start the buffer playback in a loop.
        if( stream->pDirectSoundOutputBuffer != NULL ) // FIXME: not sure this needs to be checked here
        {
            hr = IDirectSoundBuffer_Play( stream->pDirectSoundOutputBuffer, 0, 0, DSBPLAY_LOOPING );
            DBUG(("PaHost_StartOutput: IDirectSoundBuffer_Play returned = 0x%X.\n", hr));
            if( hr != DS_OK )
            {
                result = paUnanticipatedHostError;
                PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
                goto error;
            }
            stream->outputIsRunning = TRUE;
        }
    }

    if( stream->streamRepresentation.streamCallback )
    {
        /* Create timer that will wake us up so we can fill the DSound buffer. */
        int resolution;
        int framesPerWakeup = stream->framesPerDSBuffer / 4;
        int msecPerWakeup = MSEC_PER_SECOND * framesPerWakeup / (int) stream->streamRepresentation.streamInfo.sampleRate;
        if( msecPerWakeup < 10 ) msecPerWakeup = 10;
        else if( msecPerWakeup > 100 ) msecPerWakeup = 100;
        resolution = msecPerWakeup/4;
        stream->timerID = timeSetEvent( msecPerWakeup, resolution, (LPTIMECALLBACK) TimerCallback,
                                             (DWORD_PTR) stream, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS );
    
        if( stream->timerID == 0 )
        {
            stream->isActive = 0;
            result = paUnanticipatedHostError;
            PA_DS_SET_LAST_DIRECTSOUND_ERROR( hr );
            goto error;
        }
    }

    stream->isStarted = TRUE;

error:
    return result;
}


/***********************************************************************************/
static PaError StopStream( PaStream *s )
{
    PaError result = paNoError;
    PaWinDsStream *stream = (PaWinDsStream*)s;
    HRESULT          hr;
    int timeoutMsec;

    if( stream->streamRepresentation.streamCallback )
    {
        stream->stopProcessing = 1;

        /* Set timeout at 4 times maximum time we might wait. */
        timeoutMsec = (int) (4 * MSEC_PER_SECOND * (stream->framesPerDSBuffer / stream->streamRepresentation.streamInfo.sampleRate));

        WaitForSingleObject( stream->processingCompleted, timeoutMsec );
    }

    if( stream->timerID != 0 )
    {
        timeKillEvent(stream->timerID);  /* Stop callback timer. */
        stream->timerID = 0;
    }


    if( stream->bufferProcessor.outputChannelCount > 0 )
    {
        // Stop the buffer playback
        if( stream->pDirectSoundOutputBuffer != NULL )
        {
            stream->outputIsRunning = FALSE;
            // FIXME: what happens if IDirectSoundBuffer_Stop returns an error?
            hr = IDirectSoundBuffer_Stop( stream->pDirectSoundOutputBuffer );

            if( stream->pDirectSoundPrimaryBuffer )
                IDirectSoundBuffer_Stop( stream->pDirectSoundPrimaryBuffer ); /* FIXME we never started the primary buffer so I'm not sure we need to stop it */
        }
    }

    if( stream->bufferProcessor.inputChannelCount > 0 )
    {
        // Stop the buffer capture
        if( stream->pDirectSoundInputBuffer != NULL )
        {
            // FIXME: what happens if IDirectSoundCaptureBuffer_Stop returns an error?
            hr = IDirectSoundCaptureBuffer_Stop( stream->pDirectSoundInputBuffer );
        }
    }

    stream->isStarted = FALSE;

    return result;
}


/***********************************************************************************/
static PaError AbortStream( PaStream *s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    stream->abortProcessing = 1;
    return StopStream( s );
}


/***********************************************************************************/
static PaError IsStreamStopped( PaStream *s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    return !stream->isStarted;
}


/***********************************************************************************/
static PaError IsStreamActive( PaStream *s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    return stream->isActive;
}

/***********************************************************************************/
static PaTime GetStreamTime( PaStream *s )
{
    /* suppress unused variable warnings */
    (void) s;

    return PaUtil_GetTime();
}


/***********************************************************************************/
static double GetStreamCpuLoad( PaStream* s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    return PaUtil_GetCpuLoad( &stream->cpuLoadMeasurer );
}


/***********************************************************************************
    As separate stream interfaces are used for blocking and callback
    streams, the following functions can be guaranteed to only be called
    for blocking streams.
*/

static PaError ReadStream( PaStream* s,
                           void *buffer,
                           unsigned long frames )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    /* suppress unused variable warnings */
    (void) buffer;
    (void) frames;
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/

    return paNoError;
}


/***********************************************************************************/
static PaError WriteStream( PaStream* s,
                            const void *buffer,
                            unsigned long frames )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    /* suppress unused variable warnings */
    (void) buffer;
    (void) frames;
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/

    return paNoError;
}


/***********************************************************************************/
static signed long GetStreamReadAvailable( PaStream* s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    /* suppress unused variable warnings */
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/

    return 0;
}


/***********************************************************************************/
static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaWinDsStream *stream = (PaWinDsStream*)s;

    /* suppress unused variable warnings */
    (void) stream;
    
    /* IMPLEMENT ME, see portaudio.h for required behavior*/

    return 0;
}

