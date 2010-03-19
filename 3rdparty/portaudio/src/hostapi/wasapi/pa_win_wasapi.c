/*
 * Portable Audio I/O Library WASAPI implementation
 * Copyright (c) 2006-2010 David Viens, Dmitry Kostjuchenko
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

/** @file
 @ingroup hostapi_src
 @brief WASAPI implementation of support for a host API.
 @note pa_wasapi currently requires minimum VC 2005, and the latest Vista SDK
*/

#define WIN32_LEAN_AND_MEAN // exclude rare headers
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <mmsystem.h>
#include <mmreg.h>  // must be before other Wasapi headers
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	#include <Avrt.h>
	#define COBJMACROS
	#include <Audioclient.h>
	#include <endpointvolume.h>
	#define INITGUID //<< avoid additional linkage of static libs, uneeded code will be optimized out
	#include <mmdeviceapi.h>
	#include <functiondiscoverykeys.h>
	#undef INITGUID
#endif

#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_debugprint.h"
#include "pa_win_wasapi.h"

#ifndef NTDDI_VERSION

	#ifndef _AVRT_ //<< fix MinGW dummy compile by defining missing type: AVRT_PRIORITY
	typedef enum _AVRT_PRIORITY
	{
		AVRT_PRIORITY_LOW = -1,
		AVRT_PRIORITY_NORMAL,
		AVRT_PRIORITY_HIGH,
		AVRT_PRIORITY_CRITICAL
	} AVRT_PRIORITY, *PAVRT_PRIORITY;
	#endif

	#include <basetyps.h> // << for IID/CLSID
    #include <rpcsal.h>
    #include <sal.h>

	#ifndef __LPCGUID_DEFINED__
		#define __LPCGUID_DEFINED__
		typedef const GUID *LPCGUID;
	#endif

    #ifndef PROPERTYKEY_DEFINED
        #define PROPERTYKEY_DEFINED
        typedef struct _tagpropertykey
        {
            GUID fmtid;
            DWORD pid;
        } 	PROPERTYKEY;
    #endif

    #ifdef __midl_proxy
        #define __MIDL_CONST
    #else
        #define __MIDL_CONST const
    #endif

    #ifndef WAVE_FORMAT_IEEE_FLOAT
        #define WAVE_FORMAT_IEEE_FLOAT      0x0003 // 32-bit floating-point
    #endif

    // Speaker Positions:
    #define SPEAKER_FRONT_LEFT              0x1
    #define SPEAKER_FRONT_RIGHT             0x2
    #define SPEAKER_FRONT_CENTER            0x4
    #define SPEAKER_LOW_FREQUENCY           0x8
    #define SPEAKER_BACK_LEFT               0x10
    #define SPEAKER_BACK_RIGHT              0x20
    #define SPEAKER_FRONT_LEFT_OF_CENTER    0x40
    #define SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
    #define SPEAKER_BACK_CENTER             0x100
    #define SPEAKER_SIDE_LEFT               0x200
    #define SPEAKER_SIDE_RIGHT              0x400
    #define SPEAKER_TOP_CENTER              0x800
    #define SPEAKER_TOP_FRONT_LEFT          0x1000
    #define SPEAKER_TOP_FRONT_CENTER        0x2000
    #define SPEAKER_TOP_FRONT_RIGHT         0x4000
    #define SPEAKER_TOP_BACK_LEFT           0x8000
    #define SPEAKER_TOP_BACK_CENTER         0x10000
    #define SPEAKER_TOP_BACK_RIGHT          0x20000

    // Bit mask locations reserved for future use
    #define SPEAKER_RESERVED                0x7FFC0000

    // Used to specify that any possible permutation of speaker configurations
    #define SPEAKER_ALL                     0x80000000

    // DirectSound Speaker Config
    #if (NTDDI_VERSION >= NTDDI_WINXP)
    #define KSAUDIO_SPEAKER_DIRECTOUT       0
    #endif
    #define KSAUDIO_SPEAKER_MONO            (SPEAKER_FRONT_CENTER)
    #define KSAUDIO_SPEAKER_STEREO          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
    #define KSAUDIO_SPEAKER_QUAD            (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_BACK_LEFT  | SPEAKER_BACK_RIGHT)
    #define KSAUDIO_SPEAKER_SURROUND        (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER)
    #define KSAUDIO_SPEAKER_5POINT1         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                             SPEAKER_BACK_LEFT  | SPEAKER_BACK_RIGHT)
    #define KSAUDIO_SPEAKER_7POINT1         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                             SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | \
                                             SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER)

    #if ( (NTDDI_VERSION >= NTDDI_WINXPSP2) && (NTDDI_VERSION < NTDDI_WS03) ) || (NTDDI_VERSION >= NTDDI_WS03SP1)

    #define KSAUDIO_SPEAKER_5POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                             SPEAKER_SIDE_LEFT  | SPEAKER_SIDE_RIGHT)
    #define KSAUDIO_SPEAKER_7POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | \
                                             SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | \
                                             SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | \
                                             SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
    // The following are obsolete 5.1 and 7.1 settings (they lack side speakers).  Note this means
    // that the default 5.1 and 7.1 settings (KSAUDIO_SPEAKER_5POINT1 and KSAUDIO_SPEAKER_7POINT1 are
    // similarly obsolete but are unchanged for compatibility reasons).
    #define KSAUDIO_SPEAKER_5POINT1_BACK     KSAUDIO_SPEAKER_5POINT1
    #define KSAUDIO_SPEAKER_7POINT1_WIDE     KSAUDIO_SPEAKER_7POINT1

    #endif // XP SP2 and later (chronologically)

    // DVD Speaker Positions
    #define KSAUDIO_SPEAKER_GROUND_FRONT_LEFT   SPEAKER_FRONT_LEFT
    #define KSAUDIO_SPEAKER_GROUND_FRONT_CENTER SPEAKER_FRONT_CENTER
    #define KSAUDIO_SPEAKER_GROUND_FRONT_RIGHT  SPEAKER_FRONT_RIGHT
    #define KSAUDIO_SPEAKER_GROUND_REAR_LEFT    SPEAKER_BACK_LEFT
    #define KSAUDIO_SPEAKER_GROUND_REAR_RIGHT   SPEAKER_BACK_RIGHT
    #define KSAUDIO_SPEAKER_TOP_MIDDLE          SPEAKER_TOP_CENTER
    #define KSAUDIO_SPEAKER_SUPER_WOOFER        SPEAKER_LOW_FREQUENCY

    #ifdef WIN64
        #include <wtypes.h>
        typedef LONG NTSTATUS;
        typedef void *PKSEVENT_ENTRY;
        typedef void *PKSDEVICE;
        typedef void *PKSFILTERFACTORY;
        typedef void *PKSFILTER;
        typedef void *PIRP;
        typedef void *PCM_RESOURCE_LIST;
        typedef void *PDEVICE_CAPABILITIES;
        typedef void *PKSPIN;
        typedef void *PKSPROCESSPIN_INDEXENTRY;
        typedef void KSATTRIBUTE_LIST;
        typedef void *PKSHANDSHAKE;
        typedef void *PKTIMER;
        typedef void *PKDPC;
        typedef void *PKSSTREAM_POINTER;
        typedef void KSPROPERTY_SET;
        typedef void KSCLOCK_DISPATCH;
        typedef void KSMETHOD_SET;
        typedef void KSEVENT_SET;
        typedef void KSALLOCATOR_DISPATCH;
        typedef void KSDEVICE_DISPATCH;
        typedef void KSFILTER_DISPATCH;
        typedef void KSFILTER_DESCRIPTOR;
        typedef void KSPIN_DESCRIPTOR_EX;
        typedef void KSPIN_DISPATCH;
        typedef void KSDEVICE_DESCRIPTOR;
        typedef void *PFNKSDELETEALLOCATOR;
        typedef void *PFNKSDEFAULTALLOCATE;
        typedef void *PFNKSDEFAULTFREE;
        typedef void KSNODE_DESCRIPTOR;
        typedef char KSPIN_DESCRIPTOR;
        typedef void *PFNKSINTERSECTHANDLEREX;
        typedef void *PDEVICE_OBJECT;
        typedef void *PHYSICAL_ADDRESS;
        typedef void *PKSSTREAM_POINTER_OFFSET;
        typedef void *PKSPROCESSPIN;
        typedef void *PMDL;
        typedef ULONG KSSTREAM_POINTER_OFFSET;
        typedef void KSJACK_DESCRIPTION;
        #define FASTCALL
        #include <oleidl.h>
        #include <objidl.h>
     #else
        typedef struct _BYTE_BLOB
        {
            unsigned long clSize;
            unsigned char abData[ 1 ];
        } 	BYTE_BLOB;
        typedef /* [unique] */  __RPC_unique_pointer BYTE_BLOB *UP_BYTE_BLOB;
        typedef LONGLONG REFERENCE_TIME;
        #define NONAMELESSUNION
    #endif

    #undef WINVER
    #undef _WIN32_WINNT
    #include <sdkddkver.h>
    #include <propkeydef.h>
    #define COBJMACROS
    #define INITGUID
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <endpointvolume.h>
    #include <functiondiscoverykeys.h>
    #undef INITGUID

#endif // NTDDI_VERSION

#ifndef GUID_SECT
    #define GUID_SECT
#endif

#define __DEFINE_GUID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) static const GUID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#define __DEFINE_IID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) static const IID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#define __DEFINE_CLSID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) static const CLSID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#define PA_DEFINE_CLSID(className, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    __DEFINE_CLSID(pa_CLSID_##className, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)
#define PA_DEFINE_IID(interfaceName, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    __DEFINE_IID(pa_IID_##interfaceName, 0x##l, 0x##w1, 0x##w2, 0x##b1, 0x##b2, 0x##b3, 0x##b4, 0x##b5, 0x##b6, 0x##b7, 0x##b8)

// "1CB9AD4C-DBFA-4c32-B178-C2F568A703B2"
PA_DEFINE_IID(IAudioClient,         1cb9ad4c, dbfa, 4c32, b1, 78, c2, f5, 68, a7, 03, b2);
// "1BE09788-6894-4089-8586-9A2A6C265AC5"
PA_DEFINE_IID(IMMEndpoint,          1be09788, 6894, 4089, 85, 86, 9a, 2a, 6c, 26, 5a, c5);
// "A95664D2-9614-4F35-A746-DE8DB63617E6"
PA_DEFINE_IID(IMMDeviceEnumerator,  a95664d2, 9614, 4f35, a7, 46, de, 8d, b6, 36, 17, e6);
// "BCDE0395-E52F-467C-8E3D-C4579291692E"
PA_DEFINE_CLSID(IMMDeviceEnumerator,bcde0395, e52f, 467c, 8e, 3d, c4, 57, 92, 91, 69, 2e);
// "F294ACFC-3146-4483-A7BF-ADDCA7C260E2"
PA_DEFINE_IID(IAudioRenderClient,   f294acfc, 3146, 4483, a7, bf, ad, dc, a7, c2, 60, e2);
// "C8ADBD64-E71E-48a0-A4DE-185C395CD317"
PA_DEFINE_IID(IAudioCaptureClient,  c8adbd64, e71e, 48a0, a4, de, 18, 5c, 39, 5c, d3, 17);
// Media formats:
__DEFINE_GUID(pa_KSDATAFORMAT_SUBTYPE_PCM,        0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
__DEFINE_GUID(pa_KSDATAFORMAT_SUBTYPE_ADPCM,      0x00000002, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
__DEFINE_GUID(pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, 0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );

/* use CreateThread for CYGWIN/Windows Mobile, _beginthreadex for all others */
#if !defined(__CYGWIN__) && !defined(_WIN32_WCE)
	#define CREATE_THREAD(PROC) (HANDLE)_beginthreadex( NULL, 0, (PROC), stream, 0, &stream->dwThreadId )
	#define PA_THREAD_FUNC static unsigned WINAPI
	#define PA_THREAD_ID unsigned
#else
	#define CREATE_THREAD(PROC) CreateThread( NULL, 0, (PROC), stream, 0, &stream->dwThreadId )
	#define PA_THREAD_FUNC static DWORD WINAPI
	#define PA_THREAD_ID DWORD
#endif

// Thread function forward decl.
PA_THREAD_FUNC ProcThreadEvent(void *param);
PA_THREAD_FUNC ProcThreadPoll(void *param);

// Availabe from Windows 7
#ifndef AUDCLNT_E_BUFFER_ERROR
	#define AUDCLNT_E_BUFFER_ERROR AUDCLNT_ERR(0x018)
#endif
#ifndef AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED
	#define AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED AUDCLNT_ERR(0x019)
#endif
#ifndef AUDCLNT_E_INVALID_DEVICE_PERIOD
	#define AUDCLNT_E_INVALID_DEVICE_PERIOD AUDCLNT_ERR(0x020)
#endif

#define MAX_STR_LEN 512

enum { S_INPUT = 0, S_OUTPUT, S_COUNT };

#define STATIC_ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

#define PRINT(x) PA_DEBUG(x);

#define PA_SKELETON_SET_LAST_HOST_ERROR( errorCode, errorText ) \
    PaUtil_SetLastHostErrorInfo( paWASAPI, errorCode, errorText )

#ifndef IF_FAILED_JUMP
#define IF_FAILED_JUMP(hr, label) if(FAILED(hr)) goto label;
#endif

#define SAFE_CLOSE(h) if ((h) != NULL) { CloseHandle((h)); (h) = NULL; }
#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->lpVtbl->Release((punk)); (punk) = NULL; }

// AVRT is the new "multimedia schedulling stuff"
typedef BOOL   (WINAPI *FAvRtCreateThreadOrderingGroup)  (PHANDLE,PLARGE_INTEGER,GUID*,PLARGE_INTEGER);
typedef BOOL   (WINAPI *FAvRtDeleteThreadOrderingGroup)  (HANDLE);
typedef BOOL   (WINAPI *FAvRtWaitOnThreadOrderingGroup)  (HANDLE);
typedef HANDLE (WINAPI *FAvSetMmThreadCharacteristics)   (LPCTSTR,LPDWORD);
typedef BOOL   (WINAPI *FAvRevertMmThreadCharacteristics)(HANDLE);
typedef BOOL   (WINAPI *FAvSetMmThreadPriority)          (HANDLE,AVRT_PRIORITY);

static HMODULE hDInputDLL = 0;
FAvRtCreateThreadOrderingGroup   pAvRtCreateThreadOrderingGroup = NULL;
FAvRtDeleteThreadOrderingGroup   pAvRtDeleteThreadOrderingGroup = NULL;
FAvRtWaitOnThreadOrderingGroup   pAvRtWaitOnThreadOrderingGroup = NULL;
FAvSetMmThreadCharacteristics    pAvSetMmThreadCharacteristics = NULL;
FAvRevertMmThreadCharacteristics pAvRevertMmThreadCharacteristics = NULL;
FAvSetMmThreadPriority           pAvSetMmThreadPriority = NULL;

#define _GetProc(fun, type, name)  {                                                        \
                                        fun = (type) GetProcAddress(hDInputDLL,name);       \
                                        if (fun == NULL) {                                  \
                                            PRINT(("GetProcAddr failed for %s" ,name));     \
                                            return FALSE;                                   \
                                        }                                                   \
                                    }                                                       \

// ------------------------------------------------------------------------------------------
/* prototypes for functions declared in this file */
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
PaError PaWasapi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex index );
#ifdef __cplusplus
}
#endif /* __cplusplus */
// dummy entry point for other compilers and sdks
// currently built using RC1 SDK (5600)
//#if _MSC_VER < 1400
//PaError PaWasapi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
//{
    //return paNoError;
//}
//#else

// ------------------------------------------------------------------------------------------
static void Terminate( struct PaUtilHostApiRepresentation *hostApi );
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const PaStreamParameters *inputParameters,
                                  const PaStreamParameters *outputParameters,
                                  double sampleRate );
static PaError OpenStream( struct PaUtilHostApiRepresentation *hostApi,
                           PaStream** s,
                           const PaStreamParameters *inputParameters,
                           const PaStreamParameters *outputParameters,
                           double sampleRate,
                           unsigned long framesPerBuffer,
                           PaStreamFlags streamFlags,
                           PaStreamCallback *streamCallback,
                           void *userData );
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

// ------------------------------------------------------------------------------------------
/*
 These are fields that can be gathered from IDevice and IAudioDevice PRIOR to Initialize, and
 done in first pass i assume that neither of these will cause the Driver to "load", but again,
 who knows how they implement their stuff
 */
typedef struct PaWasapiDeviceInfo
{
    // Device
    IMMDevice *device;

	// from GetId
    WCHAR szDeviceID[MAX_STR_LEN];

	// from GetState
    DWORD state;

    // Fields filled from IMMEndpoint'sGetDataFlow
    EDataFlow flow;

    // Fields filled from IAudioDevice (_prior_ to Initialize)
    // from GetDevicePeriod(
    REFERENCE_TIME DefaultDevicePeriod;
    REFERENCE_TIME MinimumDevicePeriod;

    // from GetMixFormat
    // WAVEFORMATEX *MixFormat;//needs to be CoTaskMemFree'd after use!

	// Default format (setup through Control Panel by user)
	WAVEFORMATEXTENSIBLE DefaultFormat;

	// Formfactor
	EndpointFormFactor formFactor;
}
PaWasapiDeviceInfo;

// ------------------------------------------------------------------------------------------
/* PaWasapiHostApiRepresentation - host api datastructure specific to this implementation */
typedef struct
{
    PaUtilHostApiRepresentation inheritedHostApiRep;
    PaUtilStreamInterface callbackStreamInterface;
    PaUtilStreamInterface blockingStreamInterface;

    PaUtilAllocationGroup *allocations;

    /* implementation specific data goes here */

    //in case we later need the synch
    IMMDeviceEnumerator *enumerator;

    //this is the REAL number of devices, whether they are usefull to PA or not!
    UINT32 deviceCount;

    WCHAR defaultRenderer [MAX_STR_LEN];
    WCHAR defaultCapturer [MAX_STR_LEN];

    PaWasapiDeviceInfo *devInfo;

	// Is true when WOW64 Vista Workaround is needed
	BOOL useVistaWOW64Workaround;
}
PaWasapiHostApiRepresentation;

// ------------------------------------------------------------------------------------------
/* PaWasapiStream - a stream data structure specifically for this implementation */
typedef struct PaWasapiSubStream
{
    IAudioClient        *client;
    WAVEFORMATEXTENSIBLE wavex;
    UINT32               bufferSize;
    REFERENCE_TIME       device_latency;
    REFERENCE_TIME       period;
	double				 latency_seconds;
    UINT32				 framesPerHostCallback;
	AUDCLNT_SHAREMODE    shareMode;
	UINT32               streamFlags; // AUDCLNT_STREAMFLAGS_EVENTCALLBACK, ...
	UINT32               flags;

	// Used by blocking interface:
	UINT32               prevTime;  // time ms between calls of WriteStream
	UINT32               prevSleep; // time ms to sleep from frames written in previous call
}
PaWasapiSubStream;

// ------------------------------------------------------------------------------------------
/* PaWasapiHostProcessor - redirects processing data */
typedef struct PaWasapiHostProcessor
{
    PaWasapiHostProcessorCallback processor;
    void *userData;
}
PaWasapiHostProcessor;

// ------------------------------------------------------------------------------------------
typedef struct PaWasapiStream
{
	/* IMPLEMENT ME: rename this */
    PaUtilStreamRepresentation streamRepresentation;
    PaUtilCpuLoadMeasurer cpuLoadMeasurer;
    PaUtilBufferProcessor bufferProcessor;

    // input
	PaWasapiSubStream in;
    IAudioCaptureClient *cclient;
    IAudioEndpointVolume *inVol;

	// output
	PaWasapiSubStream out;
    IAudioRenderClient  *rclient;
	IAudioEndpointVolume *outVol;

	// must be volatile to avoid race condition on user query while
	// thread is being started
    volatile BOOL running;

    PA_THREAD_ID dwThreadId;
    HANDLE hThread;
	HANDLE hCloseRequest;
	HANDLE hThreadDone;
	HANDLE hBlockingOpStreamRD;
	HANDLE hBlockingOpStreamWR;

    // Host callback Output overrider
	PaWasapiHostProcessor hostProcessOverrideOutput;

    // Host callback Input overrider
	PaWasapiHostProcessor hostProcessOverrideInput;

	// Defines blocking/callback interface used
	BOOL bBlocking;

	// Av Task (MM thread management)
	HANDLE hAvTask;

	// Thread priority level
	PaWasapiThreadPriority nThreadPriority;
}
PaWasapiStream;

// Local stream methods
static void _OnStreamStop(PaWasapiStream *stream);
static void _FinishStream(PaWasapiStream *stream);

// Local statics
static volatile BOOL g_bCOMInitialized = FALSE;

// ------------------------------------------------------------------------------------------
#define LogHostError(HRES) __LogHostError(HRES, __FUNCTION__, __FILE__, __LINE__)
static HRESULT __LogHostError(HRESULT res, const char *func, const char *file, int line)
{
    const char *text = NULL;
    switch (res)
	{
	case S_OK: return res;
	case E_POINTER                              :text ="E_POINTER"; break;
	case E_INVALIDARG                           :text ="E_INVALIDARG"; break;

	case AUDCLNT_E_NOT_INITIALIZED              :text ="AUDCLNT_E_NOT_INITIALIZED"; break;
	case AUDCLNT_E_ALREADY_INITIALIZED          :text ="AUDCLNT_E_ALREADY_INITIALIZED"; break;
	case AUDCLNT_E_WRONG_ENDPOINT_TYPE          :text ="AUDCLNT_E_WRONG_ENDPOINT_TYPE"; break;
	case AUDCLNT_E_DEVICE_INVALIDATED           :text ="AUDCLNT_E_DEVICE_INVALIDATED"; break;
	case AUDCLNT_E_NOT_STOPPED                  :text ="AUDCLNT_E_NOT_STOPPED"; break;
	case AUDCLNT_E_BUFFER_TOO_LARGE             :text ="AUDCLNT_E_BUFFER_TOO_LARGE"; break;
	case AUDCLNT_E_OUT_OF_ORDER                 :text ="AUDCLNT_E_OUT_OF_ORDER"; break;
	case AUDCLNT_E_UNSUPPORTED_FORMAT           :text ="AUDCLNT_E_UNSUPPORTED_FORMAT"; break;
	case AUDCLNT_E_INVALID_SIZE                 :text ="AUDCLNT_E_INVALID_SIZE"; break;
	case AUDCLNT_E_DEVICE_IN_USE                :text ="AUDCLNT_E_DEVICE_IN_USE"; break;
	case AUDCLNT_E_BUFFER_OPERATION_PENDING     :text ="AUDCLNT_E_BUFFER_OPERATION_PENDING"; break;
	case AUDCLNT_E_THREAD_NOT_REGISTERED        :text ="AUDCLNT_E_THREAD_NOT_REGISTERED"; break;
	case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED   :text ="AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED"; break;
	case AUDCLNT_E_ENDPOINT_CREATE_FAILED       :text ="AUDCLNT_E_ENDPOINT_CREATE_FAILED"; break;
	case AUDCLNT_E_SERVICE_NOT_RUNNING          :text ="AUDCLNT_E_SERVICE_NOT_RUNNING"; break;
	case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED     :text ="AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED"; break;
	case AUDCLNT_E_EXCLUSIVE_MODE_ONLY          :text ="AUDCLNT_E_EXCLUSIVE_MODE_ONLY"; break;
	case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL :text ="AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL"; break;
	case AUDCLNT_E_EVENTHANDLE_NOT_SET          :text ="AUDCLNT_E_EVENTHANDLE_NOT_SET"; break;
	case AUDCLNT_E_INCORRECT_BUFFER_SIZE        :text ="AUDCLNT_E_INCORRECT_BUFFER_SIZE"; break;
	case AUDCLNT_E_BUFFER_SIZE_ERROR            :text ="AUDCLNT_E_BUFFER_SIZE_ERROR"; break;
	case AUDCLNT_E_CPUUSAGE_EXCEEDED            :text ="AUDCLNT_E_CPUUSAGE_EXCEEDED"; break;
	case AUDCLNT_E_BUFFER_ERROR					:text ="AUDCLNT_E_BUFFER_ERROR"; break;
	case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED		:text ="AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED"; break;
	case AUDCLNT_E_INVALID_DEVICE_PERIOD		:text ="AUDCLNT_E_INVALID_DEVICE_PERIOD"; break;

	case AUDCLNT_S_BUFFER_EMPTY                 :text ="AUDCLNT_S_BUFFER_EMPTY"; break;
	case AUDCLNT_S_THREAD_ALREADY_REGISTERED    :text ="AUDCLNT_S_THREAD_ALREADY_REGISTERED"; break;
	case AUDCLNT_S_POSITION_STALLED				:text ="AUDCLNT_S_POSITION_STALLED"; break;

	default:
		text = "UNKNOWN ERROR";
    }
	PRINT(("WASAPI ERROR HRESULT: 0x%X : %s\n [FUNCTION: %s FILE: %s {LINE: %d}]\n", res, text, func, file, line));
	PA_SKELETON_SET_LAST_HOST_ERROR(res, text);
	return res;
}

// ------------------------------------------------------------------------------------------
#define LogPaError(PAERR) __LogPaError(PAERR, __FUNCTION__, __FILE__, __LINE__)
static PaError __LogPaError(PaError err, const char *func, const char *file, int line)
{
	if (err == paNoError)
		return err;
	PRINT(("WASAPI ERROR PAERROR: %i : %s\n [FUNCTION: %s FILE: %s {LINE: %d}]\n", err, Pa_GetErrorText(err), func, file, line));
	return err;
}

// ------------------------------------------------------------------------------------------
static double nano100ToMillis(REFERENCE_TIME ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return ((double)ref)*0.0001;
}

// ------------------------------------------------------------------------------------------
static double nano100ToSeconds(REFERENCE_TIME ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return ((double)ref)*0.0000001;
}

// ------------------------------------------------------------------------------------------
static REFERENCE_TIME MillisTonano100(double ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return (REFERENCE_TIME)(ref/0.0001);
}

// ------------------------------------------------------------------------------------------
static REFERENCE_TIME SecondsTonano100(double ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return (REFERENCE_TIME)(ref/0.0000001);
}

// ------------------------------------------------------------------------------------------
// Makes Hns period from frames and sample rate
static REFERENCE_TIME MakeHnsPeriod(UINT32 nFrames, DWORD nSamplesPerSec)
{
	return (REFERENCE_TIME)((10000.0 * 1000 / nSamplesPerSec * nFrames) + 0.5);
}

// ------------------------------------------------------------------------------------------
// Converts PaSampleFormat to bits per sample value
static WORD PaSampleFormatToBitsPerSample(PaSampleFormat format_id)
{
	switch (format_id & ~paNonInterleaved)
	{
		case paFloat32:
		case paInt32: return 32;
		case paInt24: return 24;
		case paInt16: return 16;
		case paInt8:
		case paUInt8: return 8;
	}
	return 0;
}

// ------------------------------------------------------------------------------------------
// Converts PaSampleFormat to bits per sample value
static WORD PaSampleFormatToBytesPerSample(PaSampleFormat format_id)
{
	return PaSampleFormatToBitsPerSample(format_id) >> 3; // 'bits/8'
}

// ------------------------------------------------------------------------------------------
// Converts Hns period into number of frames
static UINT32 MakeFramesFromHns(REFERENCE_TIME hnsPeriod, UINT32 nSamplesPerSec)
{
    UINT32 nFrames = (UINT32)(	// frames =
        1.0 * hnsPeriod *		// hns *
        nSamplesPerSec /		// (frames / s) /
        1000 /					// (ms / s) /
        10000					// (hns / s) /
        + 0.5					// rounding
    );
	return nFrames;
}

// ------------------------------------------------------------------------------------------
// Aligns v backwards
static UINT32 ALIGN_BWD(UINT32 v, UINT32 align)
{
	return ((v - (align ? v % align : 0)));
}

// ------------------------------------------------------------------------------------------
// Aligns WASAPI buffer to 128 byte packet boundary. HD Audio will fail to play if buffer
// is misaligned. This problem was solved in Windows 7 were AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED
// is thrown although we must align for Vista anyway.
static UINT32 AlignFramesPerBuffer(UINT32 nFrames, UINT32 nSamplesPerSec, UINT32 nBlockAlign)
{
#define HDA_PACKET_SIZE 128

	long packets_total = 10000 * (nSamplesPerSec * nBlockAlign / HDA_PACKET_SIZE);
	long frame_bytes   = nFrames * nBlockAlign;
	long packets;

	// align to packet size
	frame_bytes  = ALIGN_BWD(frame_bytes, HDA_PACKET_SIZE);
	nFrames      = frame_bytes / nBlockAlign;
	packets      = frame_bytes / HDA_PACKET_SIZE;

	// align to packets count
	while (packets && ((packets_total % packets) != 0))
	{
		--packets;
	}

	frame_bytes = packets * HDA_PACKET_SIZE;
	nFrames     = frame_bytes / nBlockAlign;

	return nFrames;
}

// ------------------------------------------------------------------------------------------
static UINT32 GetFramesSleepTime(UINT32 nFrames, UINT32 nSamplesPerSec)
{
	REFERENCE_TIME nDuration;
	if (nSamplesPerSec == 0)
		return 0;
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
	// Calculate the actual duration of the allocated buffer.
	nDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC * nFrames / nSamplesPerSec);
	return (UINT32)(nDuration/REFTIMES_PER_MILLISEC/2);
}

// ------------------------------------------------------------------------------------------
static BOOL SetupAVRT()
{
    hDInputDLL = LoadLibraryA("avrt.dll");
    if (hDInputDLL == NULL)
        return FALSE;

    _GetProc(pAvRtCreateThreadOrderingGroup,  FAvRtCreateThreadOrderingGroup,  "AvRtCreateThreadOrderingGroup");
    _GetProc(pAvRtDeleteThreadOrderingGroup,  FAvRtDeleteThreadOrderingGroup,  "AvRtDeleteThreadOrderingGroup");
    _GetProc(pAvRtWaitOnThreadOrderingGroup,  FAvRtWaitOnThreadOrderingGroup,  "AvRtWaitOnThreadOrderingGroup");
    _GetProc(pAvSetMmThreadCharacteristics,   FAvSetMmThreadCharacteristics,   "AvSetMmThreadCharacteristicsA");
	_GetProc(pAvRevertMmThreadCharacteristics,FAvRevertMmThreadCharacteristics,"AvRevertMmThreadCharacteristics");
    _GetProc(pAvSetMmThreadPriority,          FAvSetMmThreadPriority,          "AvSetMmThreadPriority");

	return pAvRtCreateThreadOrderingGroup &&
		pAvRtDeleteThreadOrderingGroup &&
		pAvRtWaitOnThreadOrderingGroup &&
		pAvSetMmThreadCharacteristics &&
		pAvRevertMmThreadCharacteristics &&
		pAvSetMmThreadPriority;
}

// ------------------------------------------------------------------------------------------
static void CloseAVRT()
{
	if (hDInputDLL != NULL)
		FreeLibrary(hDInputDLL);
	hDInputDLL = NULL;
}

// ------------------------------------------------------------------------------------------
static BOOL IsWow64()
{
	// http://msdn.microsoft.com/en-us/library/ms684139(VS.85).aspx

	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;

    BOOL bIsWow64 = FALSE;

    // IsWow64Process is not available on all supported versions of Windows.
    // Use GetModuleHandle to get a handle to the DLL that contains the function
    // and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), TEXT("IsWow64Process"));

    if (fnIsWow64Process == NULL)
		return FALSE;

    if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		return FALSE;

    return bIsWow64;
}

// ------------------------------------------------------------------------------------------
typedef enum EWindowsVersion
{
	WINDOWS_UNKNOWN,
	WINDOWS_VISTA_SERVER2008,
	WINDOWS_7_SERVER2008R2
}
EWindowsVersion;
// The function is limited to Vista/7 mostly as we need just to find out Vista/WOW64 combination
// in order to use WASAPI WOW64 workarounds.
static EWindowsVersion GetWindowsVersion()
{
    DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuild = 0;

	typedef DWORD (WINAPI *LPFN_GETVERSION)(VOID);
	LPFN_GETVERSION fnGetVersion;

    fnGetVersion = (LPFN_GETVERSION) GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), TEXT("GetVersion"));

	if (fnGetVersion == NULL)
		return WINDOWS_UNKNOWN;

    dwVersion = fnGetVersion();

    // Get the Windows version
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    // Get the build number
    if (dwVersion < 0x80000000)
        dwBuild = (DWORD)(HIWORD(dwVersion));

	switch (dwMajorVersion)
	{
	case 6:
		switch (dwMinorVersion)
		{
		case 0:
			return WINDOWS_VISTA_SERVER2008;
		case 1:
			return WINDOWS_7_SERVER2008R2;
		}
	}

	return WINDOWS_UNKNOWN;
}

// ------------------------------------------------------------------------------------------
static BOOL UseWOW64VistaWorkaround()
{
	return (IsWow64() && (GetWindowsVersion() == WINDOWS_VISTA_SERVER2008));
}

// ------------------------------------------------------------------------------------------
PaError PaWasapi_Initialize( PaUtilHostApiRepresentation **hostApi, PaHostApiIndex hostApiIndex )
{
    PaError result = paNoError;
    PaWasapiHostApiRepresentation *paWasapi;
    PaDeviceInfo *deviceInfoArray;
    HRESULT hr = S_OK;
    IMMDeviceCollection* pEndPoints = NULL;
	UINT i;

    if (!SetupAVRT())
	{
        PRINT(("WASAPI: No AVRT! (not VISTA?)"));
        return paNoError;
    }

    /*
        If COM is already initialized CoInitialize will either return
        FALSE, or RPC_E_CHANGED_MODE if it was initialised in a different
        threading mode. In either case we shouldn't consider it an error
        but we need to be careful to not call CoUninitialize() if 
        RPC_E_CHANGED_MODE was returned.
    */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && (hr != RPC_E_CHANGED_MODE))
	{
		PRINT(("WASAPI: failed CoInitialize"));
        return paUnanticipatedHostError;
	}
    if (hr != RPC_E_CHANGED_MODE)
        g_bCOMInitialized = TRUE;

    paWasapi = (PaWasapiHostApiRepresentation *)PaUtil_AllocateMemory( sizeof(PaWasapiHostApiRepresentation) );
    if (paWasapi == NULL)
	{
        result = paInsufficientMemory;
        goto error;
    }

    paWasapi->allocations = PaUtil_CreateAllocationGroup();
    if (paWasapi->allocations == NULL)
	{
        result = paInsufficientMemory;
        goto error;
    }

    *hostApi                             = &paWasapi->inheritedHostApiRep;
    (*hostApi)->info.structVersion		 = 1;
    (*hostApi)->info.type				 = paWASAPI;
    (*hostApi)->info.name				 = "Windows WASAPI";
    (*hostApi)->info.deviceCount		 = 0;
    (*hostApi)->info.defaultInputDevice	 = paNoDevice;
    (*hostApi)->info.defaultOutputDevice = paNoDevice;

    paWasapi->enumerator = NULL;
    hr = CoCreateInstance(&pa_CLSID_IMMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER,
             &pa_IID_IMMDeviceEnumerator, (void **)&paWasapi->enumerator);
    IF_FAILED_JUMP(hr, error);

    // getting default device ids in the eMultimedia "role"
    {
        {
            IMMDevice *defaultRenderer = NULL;
            hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(paWasapi->enumerator, eRender, eMultimedia, &defaultRenderer);
            if (hr != S_OK)
			{
				if (hr != E_NOTFOUND)
					IF_FAILED_JUMP(hr, error);
			}
			else
			{
				WCHAR *pszDeviceId = NULL;
				hr = IMMDevice_GetId(defaultRenderer, &pszDeviceId);
				IF_FAILED_JUMP(hr, error);
				wcsncpy(paWasapi->defaultRenderer, pszDeviceId, MAX_STR_LEN-1);
				CoTaskMemFree(pszDeviceId);
				IMMDevice_Release(defaultRenderer);
			}
        }

        {
            IMMDevice *defaultCapturer = NULL;
            hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(paWasapi->enumerator, eCapture, eMultimedia, &defaultCapturer);
            if (hr != S_OK)
			{
				if (hr != E_NOTFOUND)
					IF_FAILED_JUMP(hr, error);
			}
			else
			{
				WCHAR *pszDeviceId = NULL;
				hr = IMMDevice_GetId(defaultCapturer, &pszDeviceId);
				IF_FAILED_JUMP(hr, error);
				wcsncpy(paWasapi->defaultCapturer, pszDeviceId, MAX_STR_LEN-1);
				CoTaskMemFree(pszDeviceId);
				IMMDevice_Release(defaultCapturer);
			}
        }
    }

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(paWasapi->enumerator, eAll, DEVICE_STATE_ACTIVE, &pEndPoints);
    IF_FAILED_JUMP(hr, error);

    hr = IMMDeviceCollection_GetCount(pEndPoints, &paWasapi->deviceCount);
    IF_FAILED_JUMP(hr, error);

    paWasapi->devInfo = (PaWasapiDeviceInfo *)malloc(sizeof(PaWasapiDeviceInfo) * paWasapi->deviceCount);
	for (i = 0; i < paWasapi->deviceCount; ++i)
		memset(&paWasapi->devInfo[i], 0, sizeof(PaWasapiDeviceInfo));

    if (paWasapi->deviceCount > 0)
    {
        (*hostApi)->deviceInfos = (PaDeviceInfo **)PaUtil_GroupAllocateMemory(
                paWasapi->allocations, sizeof(PaDeviceInfo *) * paWasapi->deviceCount);
        if ((*hostApi)->deviceInfos == NULL)
		{
            result = paInsufficientMemory;
            goto error;
        }

        /* allocate all device info structs in a contiguous block */
        deviceInfoArray = (PaDeviceInfo *)PaUtil_GroupAllocateMemory(
                paWasapi->allocations, sizeof(PaDeviceInfo) * paWasapi->deviceCount);
        if (deviceInfoArray == NULL)
		{
            result = paInsufficientMemory;
            goto error;
        }

        for (i = 0; i < paWasapi->deviceCount; ++i)
		{
			DWORD state				  = 0;
            PaDeviceInfo *deviceInfo  = &deviceInfoArray[i];
            deviceInfo->structVersion = 2;
            deviceInfo->hostApi       = hostApiIndex;

			PA_DEBUG(("WASAPI: device i: %d\n", i));

            hr = IMMDeviceCollection_Item(pEndPoints, i, &paWasapi->devInfo[i].device);
            IF_FAILED_JUMP(hr, error);

            // getting ID
            {
                WCHAR *pszDeviceId = NULL;
                hr = IMMDevice_GetId(paWasapi->devInfo[i].device, &pszDeviceId);
                IF_FAILED_JUMP(hr, error);
                wcsncpy(paWasapi->devInfo[i].szDeviceID, pszDeviceId, MAX_STR_LEN-1);
                CoTaskMemFree(pszDeviceId);

                if (lstrcmpW(paWasapi->devInfo[i].szDeviceID, paWasapi->defaultCapturer) == 0)
				{// we found the default input!
                    (*hostApi)->info.defaultInputDevice = (*hostApi)->info.deviceCount;
                }
                if (lstrcmpW(paWasapi->devInfo[i].szDeviceID, paWasapi->defaultRenderer) == 0)
				{// we found the default output!
                    (*hostApi)->info.defaultOutputDevice = (*hostApi)->info.deviceCount;
                }
            }

            hr = IMMDevice_GetState(paWasapi->devInfo[i].device, &paWasapi->devInfo[i].state);
            IF_FAILED_JUMP(hr, error);

            if (paWasapi->devInfo[i].state != DEVICE_STATE_ACTIVE)
			{
                PRINT(("WASAPI device: %d is not currently available (state:%d)\n",i,state));
            }

            {
                IPropertyStore *pProperty;
                hr = IMMDevice_OpenPropertyStore(paWasapi->devInfo[i].device, STGM_READ, &pProperty);
                IF_FAILED_JUMP(hr, error);

                // "Friendly" Name
                {
					char *deviceName;
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_Device_FriendlyName, &value);
                    IF_FAILED_JUMP(hr, error);
                    deviceInfo->name = NULL;
                    deviceName = (char *)PaUtil_GroupAllocateMemory(paWasapi->allocations, MAX_STR_LEN + 1);
                    if (deviceName == NULL)
					{
                        result = paInsufficientMemory;
                        goto error;
                    }
					if (value.pwszVal)
						wcstombs(deviceName, value.pwszVal, MAX_STR_LEN-1);
					else
						_snprintf(deviceName, MAX_STR_LEN-1, "baddev%d", i);
                    deviceInfo->name = deviceName;
                    PropVariantClear(&value);
                }

                // Default format
                {
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_AudioEngine_DeviceFormat, &value);
                    IF_FAILED_JUMP(hr, error);
					memcpy(&paWasapi->devInfo[i].DefaultFormat, value.blob.pBlobData, min(sizeof(paWasapi->devInfo[i].DefaultFormat), value.blob.cbSize));
                    // cleanup
                    PropVariantClear(&value);
                }

                // Formfactor
                {
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_AudioEndpoint_FormFactor, &value);
                    IF_FAILED_JUMP(hr, error);
					// set
					#if defined(DUMMYUNIONNAME) && defined(NONAMELESSUNION)
						// avoid breaking strict-aliasing rules in such line: (EndpointFormFactor)(*((UINT *)(((WORD *)&value.wReserved3)+1)));
						UINT v;
						memcpy(&v, (((WORD *)&value.wReserved3)+1), sizeof(v));
						paWasapi->devInfo[i].formFactor = (EndpointFormFactor)v;
					#else
						paWasapi->devInfo[i].formFactor = (EndpointFormFactor)value.uintVal;
					#endif
					PA_DEBUG(("WASAPI: device[%s] form-factor: %d\n", deviceInfo->name, paWasapi->devInfo[i].formFactor));
                    // cleanup
                    PropVariantClear(&value);
                }

				SAFE_RELEASE(pProperty);
            }


            // Endpoint data
            {
                IMMEndpoint *endpoint = NULL;
                hr = IMMDevice_QueryInterface(paWasapi->devInfo[i].device, &pa_IID_IMMEndpoint, (void **)&endpoint);
                if (SUCCEEDED(hr))
				{
                    hr = IMMEndpoint_GetDataFlow(endpoint, &paWasapi->devInfo[i].flow);
                    SAFE_RELEASE(endpoint);
                }
            }

            // Getting a temporary IAudioClient for more fields
            // we make sure NOT to call Initialize yet!
            {
                IAudioClient *tmpClient = NULL;

                hr = IMMDevice_Activate(paWasapi->devInfo[i].device, &pa_IID_IAudioClient,
					CLSCTX_INPROC_SERVER, NULL, (void **)&tmpClient);
                IF_FAILED_JUMP(hr, error);

                hr = IAudioClient_GetDevicePeriod(tmpClient,
                    &paWasapi->devInfo[i].DefaultDevicePeriod,
                    &paWasapi->devInfo[i].MinimumDevicePeriod);
                IF_FAILED_JUMP(hr, error);

                //hr = tmpClient->GetMixFormat(&paWasapi->devInfo[i].MixFormat);

				// Release client
				SAFE_RELEASE(tmpClient);

				if (hr != S_OK)
				{
					//davidv: this happened with my hardware, previously for that same device in DirectSound:
					//Digital Output (Realtek AC'97 Audio)'s GUID: {0x38f2cf50,0x7b4c,0x4740,0x86,0xeb,0xd4,0x38,0x66,0xd8,0xc8, 0x9f}
					//so something must be _really_ wrong with this device, TODO handle this better. We kind of need GetMixFormat
					LogHostError(hr);
					goto error;
				}
            }

            // we can now fill in portaudio device data
            deviceInfo->maxInputChannels  = 0;  //for now
            deviceInfo->maxOutputChannels = 0;  //for now
            switch (paWasapi->devInfo[i].flow)
			{
            case eRender:
                // WASAPI accepts exact channels count
                deviceInfo->maxOutputChannels		 = paWasapi->devInfo[i].DefaultFormat.Format.nChannels;
                deviceInfo->defaultHighOutputLatency = nano100ToSeconds(paWasapi->devInfo[i].DefaultDevicePeriod);
                deviceInfo->defaultLowOutputLatency  = nano100ToSeconds(paWasapi->devInfo[i].MinimumDevicePeriod);
            break;
            case eCapture:
                // WASAPI accepts exact channels count
                deviceInfo->maxInputChannels		= paWasapi->devInfo[i].DefaultFormat.Format.nChannels;
                deviceInfo->defaultHighInputLatency = nano100ToSeconds(paWasapi->devInfo[i].DefaultDevicePeriod);
                deviceInfo->defaultLowInputLatency  = nano100ToSeconds(paWasapi->devInfo[i].MinimumDevicePeriod);
            break;
            default:
                PRINT(("WASAPI: device %d bad Data FLow! \n",i));
                goto error;
            break;
            }

			deviceInfo->defaultSampleRate = paWasapi->devInfo[i].DefaultFormat.Format.nSamplesPerSec;

            (*hostApi)->deviceInfos[i] = deviceInfo;
            ++(*hostApi)->info.deviceCount;
        }
    }

    (*hostApi)->Terminate = Terminate;
    (*hostApi)->OpenStream = OpenStream;
    (*hostApi)->IsFormatSupported = IsFormatSupported;

    PaUtil_InitializeStreamInterface( &paWasapi->callbackStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, GetStreamCpuLoad,
                                      PaUtil_DummyRead, PaUtil_DummyWrite,
                                      PaUtil_DummyGetReadAvailable, PaUtil_DummyGetWriteAvailable );

    PaUtil_InitializeStreamInterface( &paWasapi->blockingStreamInterface, CloseStream, StartStream,
                                      StopStream, AbortStream, IsStreamStopped, IsStreamActive,
                                      GetStreamTime, PaUtil_DummyGetCpuLoad,
                                      ReadStream, WriteStream, GetStreamReadAvailable, GetStreamWriteAvailable );


	// findout if platform workaround is required
	paWasapi->useVistaWOW64Workaround = UseWOW64VistaWorkaround();

    SAFE_RELEASE(pEndPoints);

	PRINT(("WASAPI: initialized ok"));

    return paNoError;

error:

	PRINT(("WASAPI: failed %s error[%d|%s]", __FUNCTION__, result, Pa_GetErrorText(result)));

    SAFE_RELEASE(pEndPoints);

	Terminate((PaUtilHostApiRepresentation *)paWasapi);

    return result;
}

// ------------------------------------------------------------------------------------------
static void Terminate( PaUtilHostApiRepresentation *hostApi )
{
	UINT i;
    PaWasapiHostApiRepresentation *paWasapi = (PaWasapiHostApiRepresentation*)hostApi;
	if (paWasapi == NULL)
		return;

	// Release IMMDeviceEnumerator
    SAFE_RELEASE(paWasapi->enumerator);

    for (i = 0; i < paWasapi->deviceCount; ++i)
	{
        PaWasapiDeviceInfo *info = &paWasapi->devInfo[i];
        SAFE_RELEASE(info->device);

		//if (info->MixFormat)
        //    CoTaskMemFree(info->MixFormat);
    }
    free(paWasapi->devInfo);

    if (paWasapi->allocations)
	{
        PaUtil_FreeAllAllocations(paWasapi->allocations);
        PaUtil_DestroyAllocationGroup(paWasapi->allocations);
    }

    PaUtil_FreeMemory(paWasapi);

	// Close AVRT
	CloseAVRT();

	// Uninit COM
    if (g_bCOMInitialized)
        CoUninitialize();
}

// ------------------------------------------------------------------------------------------
static PaWasapiHostApiRepresentation *_GetHostApi(PaError *_error)
{
	PaError error;

	PaUtilHostApiRepresentation *pApi;
	if ((error = PaUtil_GetHostApiRepresentation(&pApi, paWASAPI)) != paNoError)
	{
		if (_error != NULL)
			(*_error) = error;

		return NULL;
	}
	return (PaWasapiHostApiRepresentation *)pApi;
}

// ------------------------------------------------------------------------------------------
int PaWasapi_GetDeviceDefaultFormat( void *pFormat, unsigned int nFormatSize, PaDeviceIndex nDevice )
{
	PaError ret;
	PaWasapiHostApiRepresentation *paWasapi;
	UINT32 size;
	PaDeviceIndex index;

	if (pFormat == NULL)
		return paBadBufferPtr;
	if (nFormatSize <= 0)
		return paBufferTooSmall;

	// Get API
	paWasapi = _GetHostApi(&ret);
	if (paWasapi == NULL)
		return ret;

	// Get device index
	ret = PaUtil_DeviceIndexToHostApiDeviceIndex(&index, nDevice, &paWasapi->inheritedHostApiRep);
    if (ret != paNoError)
        return ret;

	// Validate index
	if ((UINT32)index >= paWasapi->deviceCount)
		return paInvalidDevice;

	size = min(nFormatSize, (UINT32)sizeof(paWasapi->devInfo[ index ].DefaultFormat));
	memcpy(pFormat, &paWasapi->devInfo[ index ].DefaultFormat, size);

	return size;
}

// ------------------------------------------------------------------------------------------
int PaWasapi_GetDeviceRole( PaDeviceIndex nDevice )
{
	PaError ret;
	PaDeviceIndex index;

	// Get API
	PaWasapiHostApiRepresentation *paWasapi = _GetHostApi(&ret);
	if (paWasapi == NULL)
		return ret;

	// Get device index
	ret = PaUtil_DeviceIndexToHostApiDeviceIndex(&index, nDevice, &paWasapi->inheritedHostApiRep);
    if (ret != paNoError)
        return ret;

	// Validate index
	if ((UINT32)index >= paWasapi->deviceCount)
		return paInvalidDevice;

	return paWasapi->devInfo[ index ].formFactor;
}

// ------------------------------------------------------------------------------------------
static void LogWAVEFORMATEXTENSIBLE(const WAVEFORMATEXTENSIBLE *in)
{
    const WAVEFORMATEX *old = (WAVEFORMATEX *)in;
	switch (old->wFormatTag)
	{
	case WAVE_FORMAT_EXTENSIBLE: {

		PRINT(("wFormatTag=WAVE_FORMAT_EXTENSIBLE\n"));

		if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		{
			PRINT(("SubFormat=KSDATAFORMAT_SUBTYPE_IEEE_FLOAT\n"));
		}
		else
		if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_PCM))
		{
			PRINT(("SubFormat=KSDATAFORMAT_SUBTYPE_PCM\n"));
		}
		else
		{
			PRINT(("SubFormat=CUSTOM GUID{%d:%d:%d:%d%d%d%d%d%d%d%d}\n",
										in->SubFormat.Data1,
										in->SubFormat.Data2,
										in->SubFormat.Data3,
										(int)in->SubFormat.Data4[0],
										(int)in->SubFormat.Data4[1],
										(int)in->SubFormat.Data4[2],
										(int)in->SubFormat.Data4[3],
										(int)in->SubFormat.Data4[4],
										(int)in->SubFormat.Data4[5],
										(int)in->SubFormat.Data4[6],
										(int)in->SubFormat.Data4[7]));
		}
		PRINT(("Samples.wValidBitsPerSample=%d\n",  in->Samples.wValidBitsPerSample));
		PRINT(("dwChannelMask  =0x%X\n",in->dwChannelMask));

		break; }

	case WAVE_FORMAT_PCM:        PRINT(("wFormatTag=WAVE_FORMAT_PCM\n")); break;
	case WAVE_FORMAT_IEEE_FLOAT: PRINT(("wFormatTag=WAVE_FORMAT_IEEE_FLOAT\n")); break;
	default : PRINT(("wFormatTag=UNKNOWN(%d)\n",old->wFormatTag)); break;
	}

	PRINT(("nChannels      =%d\n",old->nChannels));
	PRINT(("nSamplesPerSec =%d\n",old->nSamplesPerSec));
	PRINT(("nAvgBytesPerSec=%d\n",old->nAvgBytesPerSec));
	PRINT(("nBlockAlign    =%d\n",old->nBlockAlign));
	PRINT(("wBitsPerSample =%d\n",old->wBitsPerSample));
	PRINT(("cbSize         =%d\n",old->cbSize));
}

// ------------------------------------------------------------------------------------------
static PaSampleFormat waveformatToPaFormat(const WAVEFORMATEXTENSIBLE *in)
{
    const WAVEFORMATEX *old = (WAVEFORMATEX *)in;

    switch (old->wFormatTag)
	{
    case WAVE_FORMAT_EXTENSIBLE: {
        if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		{
            if (in->Samples.wValidBitsPerSample == 32)
                return paFloat32;
        }
        else
		if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_PCM))
		{
            switch (old->wBitsPerSample)
			{
                case 32: return paInt32;
                case 24: return paInt24;
                case  8: return paUInt8;
                case 16: return paInt16;
            }
        }
		break; }
    
    case WAVE_FORMAT_IEEE_FLOAT: 
		return paFloat32;

    case WAVE_FORMAT_PCM: {
        switch (old->wBitsPerSample)
		{
            case 32: return paInt32;
            case 24: return paInt24;
            case  8: return paUInt8;
            case 16: return paInt16;
        }
		break; }
    }

    return paCustomFormat;
}

// ------------------------------------------------------------------------------------------
static PaError MakeWaveFormatFromParams(WAVEFORMATEXTENSIBLE *wavex, const PaStreamParameters *params,
									double sampleRate)
{
	WORD bitsPerSample;
	WAVEFORMATEX *old;
	DWORD channelMask = 0;
	PaWasapiStreamInfo *streamInfo = (PaWasapiStreamInfo *)params->hostApiSpecificStreamInfo;

	// Get user assigned channel mask
	if ((streamInfo != NULL) && (streamInfo->flags & paWinWasapiUseChannelMask))
		channelMask = streamInfo->channelMask;

	// Convert PaSampleFormat to bits per sample
	if ((bitsPerSample = PaSampleFormatToBitsPerSample(params->sampleFormat)) == 0)
		return paSampleFormatNotSupported;

    memset(wavex, 0, sizeof(*wavex));

    old					 = (WAVEFORMATEX *)wavex;
    old->nChannels       = (WORD)params->channelCount;
    old->nSamplesPerSec  = (DWORD)sampleRate;
	if ((old->wBitsPerSample = bitsPerSample) > 16)
	{
		old->wBitsPerSample = 32; // 20 or 24 bits must go in 32 bit containers (ints)
	}
    old->nBlockAlign     = (old->nChannels * (old->wBitsPerSample/8));
    old->nAvgBytesPerSec = (old->nSamplesPerSec * old->nBlockAlign);

    //WAVEFORMATEX
    /*if ((params->channelCount <= 2) && ((bitsPerSample == 16) || (bitsPerSample == 8)))
	{
        old->cbSize		= 0;
        old->wFormatTag	= WAVE_FORMAT_PCM;
    }
    //WAVEFORMATEXTENSIBLE
    else*/
	{
        old->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        old->cbSize		= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

        if ((params->sampleFormat & ~paNonInterleaved) == paFloat32)
            wavex->SubFormat = pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        else
            wavex->SubFormat = pa_KSDATAFORMAT_SUBTYPE_PCM;

        wavex->Samples.wValidBitsPerSample = bitsPerSample; //no extra padding!

		// Set channel mask
		if (channelMask != 0)
		{
			wavex->dwChannelMask = channelMask;
		}
		else
		{
			switch (params->channelCount)
			{
			case 1:  wavex->dwChannelMask = KSAUDIO_SPEAKER_MONO; break;
			case 2:  wavex->dwChannelMask = KSAUDIO_SPEAKER_STEREO; break;
			case 4:  wavex->dwChannelMask = KSAUDIO_SPEAKER_QUAD; break;
			case 6:  wavex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
			case 8:  wavex->dwChannelMask = KSAUDIO_SPEAKER_7POINT1; break;
			default: wavex->dwChannelMask = 0; break;
			}
		}
	}
    return paNoError;
}

// ------------------------------------------------------------------------------------------
static void wasapiFillWFEXT( WAVEFORMATEXTENSIBLE* pwfext, PaSampleFormat sampleFormat, double sampleRate, int channelCount)
{
    PA_DEBUG(( "sampleFormat = %lx\n" , sampleFormat ));
    PA_DEBUG(( "sampleRate = %f\n" , sampleRate ));
    PA_DEBUG(( "chanelCount = %d\n", channelCount ));

    pwfext->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    pwfext->Format.nChannels = (WORD)channelCount;
    pwfext->Format.nSamplesPerSec = (DWORD)sampleRate;
    if(channelCount == 1)
        pwfext->dwChannelMask = KSAUDIO_SPEAKER_DIRECTOUT;
    else
        pwfext->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    if(sampleFormat == paFloat32)
    {
        pwfext->Format.nBlockAlign = (WORD)(channelCount * 4);
        pwfext->Format.wBitsPerSample = 32;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 32;
        pwfext->SubFormat = pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }
    else if(sampleFormat == paInt32)
    {
        pwfext->Format.nBlockAlign = (WORD)(channelCount * 4);
        pwfext->Format.wBitsPerSample = 32;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 32;
        pwfext->SubFormat = pa_KSDATAFORMAT_SUBTYPE_PCM;
    }
    else if(sampleFormat == paInt24)
    {
        pwfext->Format.nBlockAlign = (WORD)(channelCount * 4);
        pwfext->Format.wBitsPerSample = 32; // 24-bit in 32-bit int container
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 24;
        pwfext->SubFormat = pa_KSDATAFORMAT_SUBTYPE_PCM;
    }
    else if(sampleFormat == paInt16)
    {
        pwfext->Format.nBlockAlign = (WORD)(channelCount * 2);
        pwfext->Format.wBitsPerSample = 16;
        pwfext->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        pwfext->Samples.wValidBitsPerSample = 16;
        pwfext->SubFormat = pa_KSDATAFORMAT_SUBTYPE_PCM;
    }
    pwfext->Format.nAvgBytesPerSec = pwfext->Format.nSamplesPerSec * pwfext->Format.nBlockAlign;
}

// ------------------------------------------------------------------------------------------
static PaError GetClosestFormat(IAudioClient *myClient, double sampleRate,
	const PaStreamParameters *params, AUDCLNT_SHAREMODE shareMode, WAVEFORMATEXTENSIBLE *outWavex)
{
	PaError answer = paInvalidSampleRate;
	WAVEFORMATEX *sharedClosestMatch = NULL;
	HRESULT hr = !S_OK;

    MakeWaveFormatFromParams(outWavex, params, sampleRate);

	hr = IAudioClient_IsFormatSupported(myClient, shareMode, &outWavex->Format, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
	if (hr == S_OK)
		answer = paFormatIsSupported;
    else
	if (sharedClosestMatch)
	{
		WORD bitsPerSample;
        WAVEFORMATEXTENSIBLE *ext = (WAVEFORMATEXTENSIBLE*)sharedClosestMatch;

		GUID subf_guid = GUID_NULL;
		if (sharedClosestMatch->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			memcpy(outWavex, sharedClosestMatch, sizeof(WAVEFORMATEXTENSIBLE));
			subf_guid = ext->SubFormat;
		}
		else
			memcpy(outWavex, sharedClosestMatch, sizeof(WAVEFORMATEX));

        CoTaskMemFree(sharedClosestMatch);

		// Make supported by default
		answer = paFormatIsSupported;

		// Validate SampleRate
		if ((DWORD)sampleRate != outWavex->Format.nSamplesPerSec)
			return paInvalidSampleRate;

		// Validate Channel count
		if ((WORD)params->channelCount != outWavex->Format.nChannels)
			return paInvalidChannelCount;

		// Validate Sample format
		if ((bitsPerSample = PaSampleFormatToBitsPerSample(params->sampleFormat)) == 0)
			return paSampleFormatNotSupported;

		// Validate Sample format: bit size (WASAPI does not limit 'bit size')
		//if (bitsPerSample != outWavex->Format.wBitsPerSample)
		//	return paSampleFormatNotSupported;

		// Validate Sample format: paFloat32 (WASAPI does not limit 'bit type')
		//if ((params->sampleFormat == paFloat32) && (subf_guid != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		//	return paSampleFormatNotSupported;

		// Validate Sample format: paInt32 (WASAPI does not limit 'bit type')
		//if ((params->sampleFormat == paInt32) && (subf_guid != KSDATAFORMAT_SUBTYPE_PCM))
		//	return paSampleFormatNotSupported;
	}
	else
	{
		//it doesnt suggest anything?? ok lets show it the MENU!
		int i;

#define FORMATTESTS 3
static const int BestToWorst[FORMATTESTS]={ paFloat32, paInt24, paInt16 };

		//ok fun time as with pa_win_mme, we know only a refusal of the user-requested
		//sampleRate+num Channel is disastrous, as the portaudio buffer processor converts between anything
		//so lets only use the number
		for (i = 0; i < FORMATTESTS; ++i)
		{
			WAVEFORMATEXTENSIBLE ext = { 0 };
			wasapiFillWFEXT(&ext,BestToWorst[i],sampleRate,params->channelCount);

			hr = IAudioClient_IsFormatSupported(myClient, shareMode, &ext.Format, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
			if (hr == S_OK)
			{
				memcpy(outWavex,&ext,sizeof(WAVEFORMATEXTENSIBLE));
				answer = paFormatIsSupported;
				break;
			}
		}

		if (answer != paFormatIsSupported)
		{
			// try MIX format?
			// why did it HAVE to come to this ....
			WAVEFORMATEX pcm16WaveFormat = { 0 };
			pcm16WaveFormat.wFormatTag		= WAVE_FORMAT_PCM;
			pcm16WaveFormat.nChannels		= 2;
			pcm16WaveFormat.nSamplesPerSec	= (DWORD)sampleRate;
			pcm16WaveFormat.nBlockAlign		= 4;
			pcm16WaveFormat.nAvgBytesPerSec = pcm16WaveFormat.nSamplesPerSec*pcm16WaveFormat.nBlockAlign;
			pcm16WaveFormat.wBitsPerSample	= 16;
			pcm16WaveFormat.cbSize			= 0;

			hr = IAudioClient_IsFormatSupported(myClient, shareMode, &pcm16WaveFormat, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
			if (hr == S_OK)
			{
				memcpy(outWavex,&pcm16WaveFormat,sizeof(WAVEFORMATEX));
				answer = paFormatIsSupported;
			}
		}

		LogHostError(hr);
	}

	return answer;
}

// ------------------------------------------------------------------------------------------
static PaError IsStreamParamsValid(struct PaUtilHostApiRepresentation *hostApi,
                                   const  PaStreamParameters *inputParameters,
                                   const  PaStreamParameters *outputParameters,
                                   double sampleRate)
{
	if (hostApi == NULL)
		return paHostApiNotFound;
	if ((UINT32)sampleRate == 0)
		return paInvalidSampleRate;

	if (inputParameters != NULL)
    {
        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
		if (inputParameters->sampleFormat & paCustomFormat)
            return paSampleFormatNotSupported;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if (inputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that input device can support inputChannelCount */
        if (inputParameters->channelCount > hostApi->deviceInfos[ inputParameters->device ]->maxInputChannels)
            return paInvalidChannelCount;

        /* validate inputStreamInfo */
        if (inputParameters->hostApiSpecificStreamInfo)
		{
			PaWasapiStreamInfo *inputStreamInfo = (PaWasapiStreamInfo *)inputParameters->hostApiSpecificStreamInfo;
	        if ((inputStreamInfo->size != sizeof(PaWasapiStreamInfo)) ||
	            (inputStreamInfo->version != 1) ||
                (inputStreamInfo->hostApiType != paWASAPI))
	        {
	            return paIncompatibleHostApiSpecificStreamInfo;
	        }
		}

        return paNoError;
    }

    if (outputParameters != NULL)
    {
        /* all standard sample formats are supported by the buffer adapter,
            this implementation doesn't support any custom sample formats */
        if (outputParameters->sampleFormat & paCustomFormat)
            return paSampleFormatNotSupported;

        /* unless alternate device specification is supported, reject the use of
            paUseHostApiSpecificDeviceSpecification */
        if (outputParameters->device == paUseHostApiSpecificDeviceSpecification)
            return paInvalidDevice;

        /* check that output device can support outputChannelCount */
        if (outputParameters->channelCount > hostApi->deviceInfos[ outputParameters->device ]->maxOutputChannels)
            return paInvalidChannelCount;

        /* validate outputStreamInfo */
        if(outputParameters->hostApiSpecificStreamInfo)
        {
			PaWasapiStreamInfo *outputStreamInfo = (PaWasapiStreamInfo *)outputParameters->hostApiSpecificStreamInfo;
	        if ((outputStreamInfo->size != sizeof(PaWasapiStreamInfo)) ||
	            (outputStreamInfo->version != 1) ||
                (outputStreamInfo->hostApiType != paWASAPI))
	        {
	            return paIncompatibleHostApiSpecificStreamInfo;
	        }
        }

		return paNoError;
    }

	return (inputParameters || outputParameters ? paNoError : paInternalError);
}

// ------------------------------------------------------------------------------------------
static PaError IsFormatSupported( struct PaUtilHostApiRepresentation *hostApi,
                                  const  PaStreamParameters *inputParameters,
                                  const  PaStreamParameters *outputParameters,
                                  double sampleRate )
{
	IAudioClient *tmpClient = NULL;
	PaWasapiHostApiRepresentation *paWasapi = (PaWasapiHostApiRepresentation*)hostApi;
	PaWasapiStreamInfo *inputStreamInfo = NULL, *outputStreamInfo = NULL;

	// Validate PaStreamParameters
	PaError error;
	if ((error = IsStreamParamsValid(hostApi, inputParameters, outputParameters, sampleRate)) != paNoError)
		return error;

    if (inputParameters != NULL)
    {
		WAVEFORMATEXTENSIBLE wavex;
		HRESULT hr;
		PaError answer;
		AUDCLNT_SHAREMODE shareMode = AUDCLNT_SHAREMODE_SHARED;
		inputStreamInfo = (PaWasapiStreamInfo *)inputParameters->hostApiSpecificStreamInfo;

		if (inputStreamInfo && (inputStreamInfo->flags & paWinWasapiExclusive))
			shareMode  = AUDCLNT_SHAREMODE_EXCLUSIVE;

		hr = IMMDevice_Activate(paWasapi->devInfo[inputParameters->device].device,
			&pa_IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&tmpClient);
		if (hr != S_OK)
		{
			LogHostError(hr);
			return paInvalidDevice;
		}

		answer = GetClosestFormat(tmpClient, sampleRate, inputParameters, shareMode, &wavex);
		SAFE_RELEASE(tmpClient);

		if (answer != paFormatIsSupported)
			return answer;
    }

    if (outputParameters != NULL)
    {
		HRESULT hr;
		WAVEFORMATEXTENSIBLE wavex;
		PaError answer;
		AUDCLNT_SHAREMODE shareMode = AUDCLNT_SHAREMODE_SHARED;
        outputStreamInfo = (PaWasapiStreamInfo *)outputParameters->hostApiSpecificStreamInfo;

		if (outputStreamInfo && (outputStreamInfo->flags & paWinWasapiExclusive))
			shareMode  = AUDCLNT_SHAREMODE_EXCLUSIVE;

		hr = IMMDevice_Activate(paWasapi->devInfo[outputParameters->device].device,
			&pa_IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&tmpClient);
		if (hr != S_OK)
		{
			LogHostError(hr);
			return paInvalidDevice;
		}

		answer = GetClosestFormat(tmpClient, sampleRate, outputParameters, shareMode, &wavex);
		SAFE_RELEASE(tmpClient);

		if (answer != paFormatIsSupported)
			return answer;
    }

    return paFormatIsSupported;
}

// ------------------------------------------------------------------------------------------
static HRESULT CreateAudioClient(PaWasapiSubStream *pSubStream, PaWasapiDeviceInfo *info,
	const PaStreamParameters *params, UINT32 framesPerLatency, double sampleRate, UINT32 streamFlags,
	PaError *pa_error)
{
	PaError error;
    HRESULT hr					= S_OK;
    UINT32 nFrames				= 0;
    IAudioClient *pAudioClient	= NULL;

    if (!pSubStream || !info || !params)
        return E_POINTER;
	if ((UINT32)sampleRate == 0)
        return E_INVALIDARG;

    // Get the audio client
    hr = IMMDevice_Activate(info->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&pAudioClient);
	if (hr != S_OK)
	{
		LogHostError(hr);
		goto done;
	}

	// Get closest format
	if ((error = GetClosestFormat(pAudioClient, sampleRate, params, pSubStream->shareMode, &pSubStream->wavex)) != paFormatIsSupported)
	{
		if (pa_error)
			(*pa_error) = error;
		return AUDCLNT_E_UNSUPPORTED_FORMAT;
	}

	// Add latency frames
	framesPerLatency += MakeFramesFromHns(SecondsTonano100(params->suggestedLatency), pSubStream->wavex.Format.nSamplesPerSec);

	// Align frames to HD Audio packet size of 128 bytes for Exclusive mode only.
	// Not aligning on Windows Vista will cause Event timeout, although Windows 7 will
	// return AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED error to realign buffer. Aligning is necessary
	// for Exclusive mode only! when audio data is feeded directly to hardware.
	if (pSubStream->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE)
	{
		framesPerLatency = AlignFramesPerBuffer(framesPerLatency,
			pSubStream->wavex.Format.nSamplesPerSec, pSubStream->wavex.Format.nBlockAlign);
	}

    // Calculate period
	pSubStream->period = MakeHnsPeriod(framesPerLatency, pSubStream->wavex.Format.nSamplesPerSec);

	// Enforce min/max period for device in Shared mode to avoid distorted sound.
	// Avoid doing so for Exclusive mode as alignment will suffer. Exclusive mode processes
	// big buffers without problem. Push Exclusive beyond limits if possible.
	if (pSubStream->shareMode == AUDCLNT_SHAREMODE_SHARED)
	{
		if (pSubStream->period < info->DefaultDevicePeriod)
			pSubStream->period = info->DefaultDevicePeriod;
	}

	// Open the stream and associate it with an audio session
    hr = IAudioClient_Initialize(pAudioClient,
        pSubStream->shareMode,
        streamFlags/*AUDCLNT_STREAMFLAGS_EVENTCALLBACK*/,
		pSubStream->period,
		(pSubStream->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSubStream->period : 0),
		&pSubStream->wavex.Format,
        NULL);

    // If the requested buffer size is not aligned...
    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
    {
		PRINT(("WASAPI: CreateAudioClient: aligning buffer size"));

        // Get the next aligned frame
        hr = IAudioClient_GetBufferSize(pAudioClient, &nFrames);
		if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

        // Release the previous allocations
        SAFE_RELEASE(pAudioClient);

        // Create a new audio client
        hr = IMMDevice_Activate(info->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    	if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

		// Get closest format
		if (GetClosestFormat(pAudioClient, sampleRate, params, pSubStream->shareMode, &pSubStream->wavex) != paFormatIsSupported)
			return AUDCLNT_E_UNSUPPORTED_FORMAT;

		// Calculate period
		pSubStream->period = MakeHnsPeriod(nFrames, pSubStream->wavex.Format.nSamplesPerSec);

        // Open the stream and associate it with an audio session
        hr = IAudioClient_Initialize(pAudioClient,
            pSubStream->shareMode,
            streamFlags/*AUDCLNT_STREAMFLAGS_EVENTCALLBACK*/,
			pSubStream->period,
			(pSubStream->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSubStream->period : 0),
            &pSubStream->wavex.Format,
            NULL);
    	if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}
    }
    else
	if (hr != S_OK)
    {
		LogHostError(hr);
		goto done;
    }

    // Set result
	pSubStream->client = pAudioClient;
    IAudioClient_AddRef(pSubStream->client);

done:

    // Clean up
    SAFE_RELEASE(pAudioClient);
    return hr;
}

// ------------------------------------------------------------------------------------------
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
	HRESULT hr;
    PaWasapiHostApiRepresentation *paWasapi = (PaWasapiHostApiRepresentation*)hostApi;
    PaWasapiStream *stream = NULL;
    int inputChannelCount, outputChannelCount;
    PaSampleFormat inputSampleFormat, outputSampleFormat;
    PaSampleFormat hostInputSampleFormat, hostOutputSampleFormat;
	PaWasapiStreamInfo *inputStreamInfo = NULL, *outputStreamInfo = NULL;
	PaWasapiDeviceInfo *info = NULL;
	UINT32 maxBufferSize;
	PaTime buffer_latency;
	ULONG framesPerHostCallback;
	PaUtilHostBufferSizeMode bufferMode;

	// validate PaStreamParameters
	if ((result = IsStreamParamsValid(hostApi, inputParameters, outputParameters, sampleRate)) != paNoError)
		return LogPaError(result);

    // Validate platform specific flags
    if ((streamFlags & paPlatformSpecificFlags) != 0)
	{
		LogPaError(result = paInvalidFlag); /* unexpected platform specific flag */
		goto error;
	}

	// Allocate memory for PaWasapiStream
    if ((stream = (PaWasapiStream *)PaUtil_AllocateMemory(sizeof(PaWasapiStream))) == NULL)
	{
        LogPaError(result = paInsufficientMemory);
        goto error;
    }

	// Default thread priority is Audio: for exclusive mode we will use Pro Audio.
	stream->nThreadPriority = eThreadPriorityAudio;

	// Set default number of frames: paFramesPerBufferUnspecified
	if (framesPerBuffer == paFramesPerBufferUnspecified)
	{
		UINT32 framesPerBufferIn  = 0, framesPerBufferOut = 0;
		if (inputParameters != NULL)
		{
			info = &paWasapi->devInfo[inputParameters->device];
			framesPerBufferIn = MakeFramesFromHns(info->DefaultDevicePeriod, (UINT32)sampleRate);
		}
		if (outputParameters != NULL)
		{
			info = &paWasapi->devInfo[outputParameters->device];
			framesPerBufferOut = MakeFramesFromHns(info->DefaultDevicePeriod, (UINT32)sampleRate);
		}
		// choosing maximum default size
		framesPerBuffer = max(framesPerBufferIn, framesPerBufferOut);
	}
	if (framesPerBuffer == 0)
		framesPerBuffer = ((UINT32)sampleRate / 100) * 2;

	// Try create device: Input
	if (inputParameters != NULL)
    {
        inputChannelCount = inputParameters->channelCount;
        inputSampleFormat = inputParameters->sampleFormat;
		inputStreamInfo   = (PaWasapiStreamInfo *)inputParameters->hostApiSpecificStreamInfo;
        info              = &paWasapi->devInfo[inputParameters->device];
		stream->in.flags  = (inputStreamInfo ? inputStreamInfo->flags : 0);

		// Select Exclusive/Shared mode
		stream->in.shareMode = AUDCLNT_SHAREMODE_SHARED;
        if ((inputStreamInfo != NULL) && (inputStreamInfo->flags & paWinWasapiExclusive))
		{
			// Boost thread priority
			stream->nThreadPriority = eThreadPriorityProAudio;
			// Make Exclusive
			stream->in.shareMode = AUDCLNT_SHAREMODE_EXCLUSIVE;
		}

		// If user provided explicit thread priority level, use it
        if ((inputStreamInfo != NULL) && (inputStreamInfo->flags & paWinWasapiThreadPriority))
		{
			if ((inputStreamInfo->threadPriority > eThreadPriorityNone) &&
				(inputStreamInfo->threadPriority <= eThreadPriorityWindowManager))
				stream->nThreadPriority = inputStreamInfo->threadPriority;
		}

		// Choose processing mode
		stream->in.streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (paWasapi->useVistaWOW64Workaround)
			stream->in.streamFlags = 0; // polling interface
		if (streamCallback == NULL)
			stream->in.streamFlags = 0; // polling interface
		if ((inputStreamInfo != NULL) && (inputStreamInfo->flags & paWinWasapiPolling))
			stream->in.streamFlags = 0; // polling interface

		// Create Audio client
		hr = CreateAudioClient(&stream->in, info, inputParameters, 0/*framesPerLatency*/,
			sampleRate, stream->in.streamFlags, &result);
        if (hr != S_OK)
		{
            LogHostError(hr);
			if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT)
				result = paInvalidDevice;

			LogPaError(result);
			goto error;
        }
		LogWAVEFORMATEXTENSIBLE(&stream->in.wavex);

		// Get closest format
        hostInputSampleFormat = PaUtil_SelectClosestAvailableFormat( waveformatToPaFormat(&stream->in.wavex), inputSampleFormat );

		// Create volume mgr
		stream->inVol = NULL;
        /*hr = info->device->Activate(
            __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL,
            (void**)&stream->inVol);
        if (hr != S_OK)
            return paInvalidDevice;*/

        // Set user-side custom host processor
        if ((inputStreamInfo != NULL) &&
            (inputStreamInfo->flags & paWinWasapiRedirectHostProcessor))
        {
            stream->hostProcessOverrideInput.processor = inputStreamInfo->hostProcessorInput;
            stream->hostProcessOverrideInput.userData = userData;
        }

        // Get max possible buffer size to check if it is not less than that we request
        hr = IAudioClient_GetBufferSize(stream->in.client, &maxBufferSize);
        if (hr != S_OK)
		{
			LogHostError(hr);
 			LogPaError(result = paInvalidDevice);
			goto error;
		}

        // Correct buffer to max size if it maxed out result of GetBufferSize
		stream->in.bufferSize = maxBufferSize;

		// Get interface latency (actually uneeded as we calculate latency from the size
		// of maxBufferSize).
        hr = IAudioClient_GetStreamLatency(stream->in.client, &stream->in.device_latency);
        if (hr != S_OK)
		{
			LogHostError(hr);
 			LogPaError(result = paInvalidDevice);
			goto error;
		}
		//stream->in.latency_seconds = nano100ToSeconds(stream->in.device_latency);

        // Number of frames that are required at each period
		stream->in.framesPerHostCallback = maxBufferSize;

		// Calculate buffer latency
		buffer_latency = (PaTime)maxBufferSize / stream->in.wavex.Format.nSamplesPerSec;

		// Append buffer latency to interface latency in shared mode (see GetStreamLatency notes)
		stream->in.latency_seconds += buffer_latency;

		PRINT(("PaWASAPIOpenStream(input): framesPerHostCallback[ %d ] latency[ %.02fms ]\n", (UINT32)stream->in.framesPerHostCallback, (float)(stream->in.latency_seconds*1000.0f)));
	}
    else
    {
        inputChannelCount = 0;
        inputSampleFormat = hostInputSampleFormat = paInt16; /* Surpress 'uninitialised var' warnings. */
    }

	// Try create device: Output
    if (outputParameters != NULL)
    {
        outputChannelCount = outputParameters->channelCount;
        outputSampleFormat = outputParameters->sampleFormat;
		outputStreamInfo   = (PaWasapiStreamInfo *)outputParameters->hostApiSpecificStreamInfo;
		info               = &paWasapi->devInfo[outputParameters->device];
		stream->out.flags  = (outputStreamInfo ? outputStreamInfo->flags : 0);

		// Select Exclusive/Shared mode
		stream->out.shareMode = AUDCLNT_SHAREMODE_SHARED;
        if ((outputStreamInfo != NULL) && (outputStreamInfo->flags & paWinWasapiExclusive))
		{
			// Boost thread priority
			stream->nThreadPriority = eThreadPriorityProAudio;
			// Make Exclusive
			stream->out.shareMode = AUDCLNT_SHAREMODE_EXCLUSIVE;
		}

		// If user provided explicit thread priority level, use it
        if ((outputStreamInfo != NULL) && (outputStreamInfo->flags & paWinWasapiThreadPriority))
		{
			if ((outputStreamInfo->threadPriority > eThreadPriorityNone) &&
				(outputStreamInfo->threadPriority <= eThreadPriorityWindowManager))
				stream->nThreadPriority = outputStreamInfo->threadPriority;
		}

		// Choose processing mode
		stream->out.streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (paWasapi->useVistaWOW64Workaround)
			stream->out.streamFlags = 0; // polling interface
		if (streamCallback == NULL)
			stream->out.streamFlags = 0; // polling interface
		if ((outputStreamInfo != NULL) && (outputStreamInfo->flags & paWinWasapiPolling))
			stream->out.streamFlags = 0; // polling interface

		// Create Audio client
		hr = CreateAudioClient(&stream->out, info, outputParameters, 0/*framesPerLatency*/,
			sampleRate, stream->out.streamFlags, &result);
        if (hr != S_OK)
		{
            LogHostError(hr);
			if (hr != AUDCLNT_E_UNSUPPORTED_FORMAT)
				result = paInvalidDevice;

			LogPaError(result);
			goto error;
        }
		LogWAVEFORMATEXTENSIBLE(&stream->out.wavex);

        // Get closest format
        hostOutputSampleFormat = PaUtil_SelectClosestAvailableFormat( waveformatToPaFormat(&stream->out.wavex), outputSampleFormat );

		// Activate volume
		stream->outVol = NULL;
        /*hr = info->device->Activate(
            __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL,
            (void**)&stream->outVol);
        if (hr != S_OK)
            return paInvalidDevice;*/

       // Set user-side custom host processor
        if ((outputStreamInfo != NULL) &&
            (outputStreamInfo->flags & paWinWasapiRedirectHostProcessor))
        {
            stream->hostProcessOverrideOutput.processor = outputStreamInfo->hostProcessorOutput;
            stream->hostProcessOverrideOutput.userData = userData;
        }

        // Get max possible buffer size to check if it is not less than that we request
        hr = IAudioClient_GetBufferSize(stream->out.client, &maxBufferSize);
        if (hr != S_OK)
		{
			LogHostError(hr);
 			LogPaError(result = paInvalidDevice);
			goto error;
		}

        // Correct buffer to max size if it maxed out result of GetBufferSize
		stream->out.bufferSize = maxBufferSize;

		// Get interface latency (actually uneeded as we calculate latency from the size
		// of maxBufferSize).
        hr = IAudioClient_GetStreamLatency(stream->out.client, &stream->out.device_latency);
        if (hr != S_OK)
		{
			LogHostError(hr);
 			LogPaError(result = paInvalidDevice);
			goto error;
		}
		//stream->out.latency_seconds = nano100ToSeconds(stream->out.device_latency);

        // Number of frames that are required at each period
		stream->out.framesPerHostCallback = maxBufferSize;

		// Calculate buffer latency
		buffer_latency = (PaTime)maxBufferSize / stream->out.wavex.Format.nSamplesPerSec;

		// Append buffer latency to interface latency in shared mode (see GetStreamLatency notes)
		stream->out.latency_seconds += buffer_latency;

		PRINT(("PaWASAPIOpenStream(output): framesPerHostCallback[ %d ] latency[ %.02fms ]\n", (UINT32)stream->out.framesPerHostCallback, (float)(stream->out.latency_seconds*1000.0f)));
	}
    else
    {
        outputChannelCount = 0;
        outputSampleFormat = hostOutputSampleFormat = paInt16; /* Surpress 'uninitialized var' warnings. */
    }

	// paWinWasapiPolling must be on/or not on both streams
	if ((inputParameters != NULL) && (outputParameters != NULL))
	{
		if ((inputStreamInfo != NULL) && (outputStreamInfo != NULL))
		{
			if (((inputStreamInfo->flags & paWinWasapiPolling) &&
				!(outputStreamInfo->flags & paWinWasapiPolling))
				||
				(!(inputStreamInfo->flags & paWinWasapiPolling) &&
				 (outputStreamInfo->flags & paWinWasapiPolling)))
			{
				LogPaError(result = paInvalidFlag);
				goto error;
			}
		}
	}

	// Initialize stream representation
    if (streamCallback)
    {
		stream->bBlocking = FALSE;
        PaUtil_InitializeStreamRepresentation(&stream->streamRepresentation,
                                              &paWasapi->callbackStreamInterface,
											  streamCallback, userData);
    }
    else
    {
		stream->bBlocking = TRUE;
        PaUtil_InitializeStreamRepresentation(&stream->streamRepresentation,
                                              &paWasapi->blockingStreamInterface,
											  streamCallback, userData);
    }

	// Initialize CPU measurer
    PaUtil_InitializeCpuLoadMeasurer(&stream->cpuLoadMeasurer, sampleRate);

	if (outputParameters && inputParameters)
	{
		// serious problem #1
		if (stream->in.period != stream->out.period)
		{
			PRINT(("WASAPI: OpenStream: period discrepancy\n"));
			goto error;
		}

		// serious problem #2
		if (stream->out.framesPerHostCallback != stream->in.framesPerHostCallback)
		{
			PRINT(("WASAPI: OpenStream: framesPerHostCallback discrepancy\n"));
			goto error;
		}
	}

	framesPerHostCallback = (outputParameters) ? stream->out.framesPerHostCallback:
		stream->in.framesPerHostCallback;

	// Choose correct mode of buffer processing:
	// Exclusive/Shared non paWinWasapiPolling mode: paUtilFixedHostBufferSize - always fixed
	// Exclusive/Shared paWinWasapiPolling mode: paUtilBoundedHostBufferSize - may vary
	bufferMode = paUtilFixedHostBufferSize;
	if (inputParameters &&
		(!stream->in.streamFlags || ((stream->in.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) == 0)))
		bufferMode = paUtilBoundedHostBufferSize;
	else
	if (outputParameters &&
		(!stream->out.streamFlags || ((stream->out.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) == 0)))
		bufferMode = paUtilBoundedHostBufferSize;

    // Initialize buffer processor
    result =  PaUtil_InitializeBufferProcessor(
		&stream->bufferProcessor,
        inputChannelCount,
		inputSampleFormat,
		hostInputSampleFormat,
        outputChannelCount,
		outputSampleFormat,
		hostOutputSampleFormat,
        sampleRate,
		streamFlags,
		framesPerBuffer,
        framesPerHostCallback,
		bufferMode,
        streamCallback,
		userData);
    if (result != paNoError)
	{
		LogPaError(result);
        goto error;
	}

	// Set Input latency
    stream->streamRepresentation.streamInfo.inputLatency =
            PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor)
			+ ((inputParameters)?stream->in.latency_seconds :0);

	// Set Output latency
    stream->streamRepresentation.streamInfo.outputLatency =
            PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor)
			+ ((outputParameters)?stream->out.latency_seconds :0);

	// Set SR
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

    (*s) = (PaStream *)stream;
    return result;

error:

    if (stream != NULL)
	{
		CloseStream(stream);
        PaUtil_FreeMemory(stream);
	}

    return result;
}

// ------------------------------------------------------------------------------------------
static PaError CloseStream( PaStream* s )
{
    PaError result = paNoError;
    PaWasapiStream *stream = (PaWasapiStream*)s;

	// abort active stream
	if (IsStreamActive(s))
	{
		if ((result = AbortStream(s)) != paNoError)
			return result;
	}

    SAFE_RELEASE(stream->out.client);
    SAFE_RELEASE(stream->in.client);
    SAFE_RELEASE(stream->cclient);
    SAFE_RELEASE(stream->rclient);
	SAFE_RELEASE(stream->inVol);
	SAFE_RELEASE(stream->outVol);

    SAFE_CLOSE(stream->hThread);
	SAFE_CLOSE(stream->hThreadDone);
	SAFE_CLOSE(stream->hCloseRequest);
	SAFE_CLOSE(stream->hBlockingOpStreamRD);
	SAFE_CLOSE(stream->hBlockingOpStreamWR);

    PaUtil_TerminateBufferProcessor(&stream->bufferProcessor);
    PaUtil_TerminateStreamRepresentation(&stream->streamRepresentation);
    PaUtil_FreeMemory(stream);

    return result;
}

// ------------------------------------------------------------------------------------------
static PaError StartStream( PaStream *s )
{
	HRESULT hr;
    PaWasapiStream *stream = (PaWasapiStream*)s;

	// check if stream is active already
	if (IsStreamActive(s))
		return paStreamIsNotStopped;

    PaUtil_ResetBufferProcessor(&stream->bufferProcessor);

	// Create close event
	stream->hCloseRequest = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Create thread
	if (!stream->bBlocking)
	{
		// Create thread done event
		stream->hThreadDone = CreateEvent(NULL, TRUE, FALSE, NULL);

		if ((stream->in.client && stream->in.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) ||
			(stream->out.client && stream->out.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
		{
			if ((stream->hThread = CREATE_THREAD(ProcThreadEvent)) == NULL)
			   return paUnanticipatedHostError;
		}
		else
		{
			if ((stream->hThread = CREATE_THREAD(ProcThreadPoll)) == NULL)
			   return paUnanticipatedHostError;
		}
	}
	else
	{
		// Create blocking operation events (non-signaled event means - blocking operation is pending)
		if (stream->out.client)
			stream->hBlockingOpStreamWR = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (stream->in.client)
			stream->hBlockingOpStreamRD = CreateEvent(NULL, TRUE, TRUE, NULL);

		// Initialize event & start INPUT stream
		if (stream->in.client)
		{
			if ((hr = IAudioClient_GetService(stream->in.client, &pa_IID_IAudioCaptureClient, (void **)&stream->cclient)) != S_OK)
			{
				LogHostError(hr);
				return paUnanticipatedHostError;
			}

			if ((hr = IAudioClient_Start(stream->in.client)) != S_OK)
			{
				LogHostError(hr);
				return paUnanticipatedHostError;
			}
		}


		// Initialize event & start OUTPUT stream
		if (stream->out.client)
		{
			if ((hr = IAudioClient_GetService(stream->out.client, &pa_IID_IAudioRenderClient, (void **)&stream->rclient)) != S_OK)
			{
				LogHostError(hr);
				return paUnanticipatedHostError;
			}

			// Start
			if ((hr = IAudioClient_Start(stream->out.client)) != S_OK)
			{
				LogHostError(hr);
				return paUnanticipatedHostError;
			}
		}

		// Signal: stream running
		stream->running = TRUE;

		// Set current time
		stream->out.prevTime  = timeGetTime();
		stream->out.prevSleep = 0;
	}

    return paNoError;
}

// ------------------------------------------------------------------------------------------
static void _FinishStream(PaWasapiStream *stream)
{
	// Issue command to thread to stop processing and wait for thread exit
	if (!stream->bBlocking)
	{
		SignalObjectAndWait(stream->hCloseRequest, stream->hThreadDone, INFINITE, FALSE);
	}
	else
	// Blocking mode does not own thread
	{
		// Signal close event and wait for each of 2 blocking operations to complete
		if (stream->out.client)
			SignalObjectAndWait(stream->hCloseRequest, stream->hBlockingOpStreamWR, INFINITE, TRUE);
		if (stream->out.client)
			SignalObjectAndWait(stream->hCloseRequest, stream->hBlockingOpStreamRD, INFINITE, TRUE);

		// Process stop
		_OnStreamStop(stream);
	}

	// Close thread handles to allow restart
	SAFE_CLOSE(stream->hThread);
	SAFE_CLOSE(stream->hThreadDone);
	SAFE_CLOSE(stream->hCloseRequest);
	SAFE_CLOSE(stream->hBlockingOpStreamRD);
	SAFE_CLOSE(stream->hBlockingOpStreamWR);

    stream->running = FALSE;
}

// ------------------------------------------------------------------------------------------
static PaError StopStream( PaStream *s )
{
	// Finish stream
	_FinishStream((PaWasapiStream *)s);
    return paNoError;
}

// ------------------------------------------------------------------------------------------
static PaError AbortStream( PaStream *s )
{
	// Finish stream
	_FinishStream((PaWasapiStream *)s);
    return paNoError;
}

// ------------------------------------------------------------------------------------------
static PaError IsStreamStopped( PaStream *s )
{
	return !((PaWasapiStream *)s)->running;
}

// ------------------------------------------------------------------------------------------
static PaError IsStreamActive( PaStream *s )
{
    return ((PaWasapiStream *)s)->running;
}

// ------------------------------------------------------------------------------------------
static PaTime GetStreamTime( PaStream *s )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

    /* suppress unused variable warnings */
    (void) stream;

    /* IMPLEMENT ME, see portaudio.h for required behavior*/

	//this is lame ds and mme does the same thing, quite useless method imho
	//why dont we fetch the time in the pa callbacks?
	//at least its doing to be clocked to something
    return PaUtil_GetTime();
}

// ------------------------------------------------------------------------------------------
static double GetStreamCpuLoad( PaStream* s )
{
	return PaUtil_GetCpuLoad(&((PaWasapiStream *)s)->cpuLoadMeasurer);
}

// ------------------------------------------------------------------------------------------
/* NOT TESTED */
static PaError ReadStream( PaStream* s, void *_buffer, unsigned long _frames )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	HRESULT hr = S_OK;
	UINT32 frames;
	BYTE *buffer = (BYTE *)_buffer;
	BYTE *data = NULL;
	DWORD flags = 0;
	UINT32 buffer_size;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->cclient == NULL)
		return paBadStreamPtr;

	// Notify blocking op has begun
	ResetEvent(stream->hBlockingOpStreamRD);

	while (_frames != 0)
	{
		// Get the available data in the shared buffer.
		if ((hr = IAudioCaptureClient_GetBuffer(stream->cclient, &data, &frames, &flags, NULL, NULL)) != S_OK)
		{
			if (hr == AUDCLNT_S_BUFFER_EMPTY)
			{
				// Check if blocking call must be interrupted
				if (WaitForSingleObject(stream->hCloseRequest, 1) != WAIT_TIMEOUT)
					break;
			}

			return LogHostError(hr);
			goto stream_rd_end;
		}

		// Detect silence
		// if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		//	data = NULL;

		// Check if frames <= _frames
		if (frames > _frames)
			frames = _frames;

		// Copy
		buffer_size = frames * stream->in.wavex.Format.nBlockAlign;
		memcpy(buffer, data, buffer_size);
		buffer += buffer_size;

		// Release buffer
		if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->cclient, frames)) != S_OK)
		{
			LogHostError(hr);
			goto stream_rd_end;
		}

		_frames -= frames;
	}

stream_rd_end:

	// Notify blocking op has ended
	SetEvent(stream->hBlockingOpStreamRD);

	return (hr != S_OK ? paUnanticipatedHostError : paNoError);
}

// ------------------------------------------------------------------------------------------
static PaError WriteStream( PaStream* s, const void *_buffer, unsigned long _frames )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	UINT32 frames;
	const BYTE *buffer = (BYTE *)_buffer;
	BYTE *data;
	HRESULT hr = S_OK;
	UINT32 next_rev_sleep, blocks, block_sleep_ms;
	UINT32 i;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->rclient == NULL)
		return paBadStreamPtr;

	// Notify blocking op has begun
	ResetEvent(stream->hBlockingOpStreamWR);

	// Calculate sleep time for next call
	{
		UINT32 remainder = 0;
		UINT32 sleep_ms = 0;
		DWORD elapsed_ms;
		blocks = _frames / stream->out.framesPerHostCallback;
		block_sleep_ms = GetFramesSleepTime(stream->out.framesPerHostCallback, stream->out.wavex.Format.nSamplesPerSec);
		if (blocks == 0)
		{
			blocks = 1;
			sleep_ms = GetFramesSleepTime(_frames, stream->out.wavex.Format.nSamplesPerSec); // partial
		}
		else
		{
			remainder = _frames - blocks * stream->out.framesPerHostCallback;
			sleep_ms = block_sleep_ms; // full
		}

		// Sleep for remainder
		elapsed_ms = timeGetTime() - stream->out.prevTime;
		if (sleep_ms >= elapsed_ms)
			sleep_ms -= elapsed_ms;

		next_rev_sleep = sleep_ms;
	}

	// Sleep diff from last call
	if (stream->out.prevSleep)
		Sleep(stream->out.prevSleep);
	stream->out.prevSleep = next_rev_sleep;

	// Feed engine
	for (i = 0; i < blocks; ++i)
	{
		UINT32 available;
		UINT32 buffer_size;

		// Get block frames
		frames = stream->out.framesPerHostCallback;
		if (frames > _frames)
			frames = _frames;

		if (i)
			Sleep(block_sleep_ms);

		while (frames != 0)
		{
			UINT32 padding = 0;

			// Check if blocking call must be interrupted
			if (WaitForSingleObject(stream->hCloseRequest, 0) != WAIT_TIMEOUT)
				break;

			// Get Read position
			hr = IAudioClient_GetCurrentPadding(stream->out.client, &padding);
			if (hr != S_OK)
			{
				LogHostError(hr);
				goto stream_wr_end;
			}

			// Get frames available
			if (frames >= padding)
				available = frames - padding;
			else
				available = frames;

			// Get buffer
			if ((hr = IAudioRenderClient_GetBuffer(stream->rclient, available, &data)) != S_OK)
			{
				// Buffer size is too big, waiting
				if (hr == AUDCLNT_E_BUFFER_TOO_LARGE)
					continue;
				LogHostError(hr);
				goto stream_wr_end;
			}

			// Copy
			buffer_size = available * stream->out.wavex.Format.nBlockAlign;
			memcpy(data, buffer, buffer_size);
			buffer += buffer_size;

			// Release buffer
			if ((hr = IAudioRenderClient_ReleaseBuffer(stream->rclient, available, 0)) != S_OK)
			{
				LogHostError(hr);
				goto stream_wr_end;
			}

			frames -= available;
		}

		_frames -= frames;
	}

stream_wr_end:

	// Set prev time
	stream->out.prevTime = timeGetTime();

	// Notify blocking op has ended
	SetEvent(stream->hBlockingOpStreamWR);

	return (hr != S_OK ? paUnanticipatedHostError : paNoError);
}

// ------------------------------------------------------------------------------------------
/* NOT TESTED */
static signed long GetStreamReadAvailable( PaStream* s )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;
	HRESULT hr;
	UINT32 pending = 0;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->cclient == NULL)
		return paBadStreamPtr;

	hr = IAudioClient_GetCurrentPadding(stream->in.client, &pending);
	if (hr != S_OK)
	{
		LogHostError(hr);
		return paUnanticipatedHostError;
	}

    return (long)pending;
}

// ------------------------------------------------------------------------------------------
static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	UINT32 frames = stream->out.framesPerHostCallback;
	HRESULT hr;
	UINT32 padding = 0;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->rclient == NULL)
		return paBadStreamPtr;

	hr = IAudioClient_GetCurrentPadding(stream->out.client, &padding);
	if (hr != S_OK)
	{
		LogHostError(hr);
		return paUnanticipatedHostError;
	}

	// Calculate
	frames -= padding;

    return frames;
}

// ------------------------------------------------------------------------------------------
static void WaspiHostProcessingLoop( void *inputBuffer,  long inputFrames,
                                     void *outputBuffer, long outputFrames,
                                     void *userData )
{
    PaWasapiStream *stream = (PaWasapiStream*)userData;
    PaStreamCallbackTimeInfo timeInfo = {0,0,0};
	PaStreamCallbackFlags flags = 0;
    int callbackResult;
    unsigned long framesProcessed;
	HRESULT hr;
	UINT32 pending;

    PaUtil_BeginCpuLoadMeasurement( &stream->cpuLoadMeasurer );

    /*
		Pa_GetStreamTime:
            - generate timing information
            - handle buffer slips
    */
	timeInfo.currentTime = PaUtil_GetTime();
	// Query input latency
	if (stream->in.client != NULL)
	{
		PaTime pending_time;
		if ((hr = IAudioClient_GetCurrentPadding(stream->in.client, &pending)) == S_OK)
			pending_time = (PaTime)pending / (PaTime)stream->in.wavex.Format.nSamplesPerSec;
		else
			pending_time = (PaTime)stream->in.latency_seconds;

		timeInfo.inputBufferAdcTime = timeInfo.currentTime + pending_time;
	}
	// Query output current latency
	if (stream->out.client != NULL)
	{
		PaTime pending_time;
		if ((hr = IAudioClient_GetCurrentPadding(stream->out.client, &pending)) == S_OK)
			pending_time = (PaTime)pending / (PaTime)stream->out.wavex.Format.nSamplesPerSec;
		else
			pending_time = (PaTime)stream->out.latency_seconds;

		timeInfo.outputBufferDacTime = timeInfo.currentTime + pending_time;
	}

    /*
        If you need to byte swap or shift inputBuffer to convert it into a
        portaudio format, do it here.
    */

    PaUtil_BeginBufferProcessing( &stream->bufferProcessor, &timeInfo, flags );

    /*
        depending on whether the host buffers are interleaved, non-interleaved
        or a mixture, you will want to call PaUtil_SetInterleaved*Channels(),
        PaUtil_SetNonInterleaved*Channel() or PaUtil_Set*Channel() here.
    */

    if (stream->bufferProcessor.inputChannelCount > 0)
    {
        PaUtil_SetInputFrameCount( &stream->bufferProcessor, inputFrames );
        PaUtil_SetInterleavedInputChannels( &stream->bufferProcessor,
            0, /* first channel of inputBuffer is channel 0 */
            inputBuffer,
            0 ); /* 0 - use inputChannelCount passed to init buffer processor */
    }

    if (stream->bufferProcessor.outputChannelCount > 0)
    {
        PaUtil_SetOutputFrameCount( &stream->bufferProcessor, outputFrames);
        PaUtil_SetInterleavedOutputChannels( &stream->bufferProcessor,
            0, /* first channel of outputBuffer is channel 0 */
            outputBuffer,
            0 ); /* 0 - use outputChannelCount passed to init buffer processor */
    }

    /* you must pass a valid value of callback result to PaUtil_EndBufferProcessing()
        in general you would pass paContinue for normal operation, and
        paComplete to drain the buffer processor's internal output buffer.
        You can check whether the buffer processor's output buffer is empty
        using PaUtil_IsBufferProcessorOuputEmpty( bufferProcessor )
    */
    callbackResult = paContinue;
    framesProcessed = PaUtil_EndBufferProcessing( &stream->bufferProcessor, &callbackResult );

    /*
        If you need to byte swap or shift outputBuffer to convert it to
        host format, do it here.
    */

	PaUtil_EndCpuLoadMeasurement( &stream->cpuLoadMeasurer, framesProcessed );

    if (callbackResult == paContinue)
    {
        /* nothing special to do */
    }
    else
	if (callbackResult == paAbort)
    {
		// stop stream
        SetEvent(stream->hCloseRequest);
    }
    else
    {
		// stop stream
        SetEvent(stream->hCloseRequest);
    }
}

// ------------------------------------------------------------------------------------------
HANDLE MMCSS_activate(const char *name)
{
    DWORD task_idx = 0;
    HANDLE hTask = pAvSetMmThreadCharacteristics(name, &task_idx);
    if (hTask == NULL)
	{
        PRINT(("WASAPI: AvSetMmThreadCharacteristics failed!\n"));
    }

    /*BOOL priority_ok = pAvSetMmThreadPriority(hTask, AVRT_PRIORITY_NORMAL);
    if (priority_ok == FALSE)
	{
        PRINT(("WASAPI: AvSetMmThreadPriority failed!\n"));
    }*/

	// debug
    {
        int    cur_priority		  = GetThreadPriority(GetCurrentThread());
        DWORD  cur_priority_class = GetPriorityClass(GetCurrentProcess());
		PRINT(("WASAPI: thread[ priority-0x%X class-0x%X ]\n", cur_priority, cur_priority_class));
    }

	return hTask;
}

// ------------------------------------------------------------------------------------------
void MMCSS_deactivate(HANDLE hTask)
{
	if (!hTask)
		return;

	if (pAvRevertMmThreadCharacteristics(hTask) == FALSE)
	{
        PRINT(("WASAPI: AvRevertMmThreadCharacteristics failed!\n"));
    }
}

// ------------------------------------------------------------------------------------------
PaError PaWasapi_ThreadPriorityBoost(void **hTask, PaWasapiThreadPriority nPriorityClass)
{
	static const char *mmcs_name[] =
	{
		NULL,
		"Audio",
		"Capture",
		"Distribution",
		"Games",
		"Playback",
		"Pro Audio",
		"Window Manager"
	};
	HANDLE task;

	if (hTask == NULL)
		return paUnanticipatedHostError;

	if ((UINT32)nPriorityClass >= STATIC_ARRAY_SIZE(mmcs_name))
		return paUnanticipatedHostError;

	task = MMCSS_activate(mmcs_name[nPriorityClass]);
	if (task == NULL)
		return paUnanticipatedHostError;

	(*hTask) = task;
	return paNoError;
}

// ------------------------------------------------------------------------------------------
PaError PaWasapi_ThreadPriorityRevert(void *hTask)
{
	if (hTask == NULL)
		return paUnanticipatedHostError;

	MMCSS_deactivate((HANDLE)hTask);

	return paNoError;
}

// ------------------------------------------------------------------------------------------
static HRESULT FillOutputBuffer(PaWasapiStream *stream, PaWasapiHostProcessor *processor, UINT32 frames)
{
	HRESULT hr;
	BYTE *data = NULL;

	// Get buffer
	if ((hr = IAudioRenderClient_GetBuffer(stream->rclient, frames, &data)) != S_OK)
	{
		if (stream->out.shareMode == AUDCLNT_SHAREMODE_SHARED)
		{
			// Using GetCurrentPadding to overcome AUDCLNT_E_BUFFER_TOO_LARGE in
			// shared mode results in no sound in Event-driven mode (MSDN does not
			// document this, or is it WASAPI bug?), thus we better
			// try to acquire buffer next time when GetBuffer allows to do so.
#if 0
			// Get Read position
			UINT32 padding = 0;
			hr = stream->out.client->GetCurrentPadding(&padding);
			if (hr != S_OK)
				return LogHostError(hr);

			// Get frames to write
			frames -= padding;
			if (frames == 0)
				return S_OK;

			if ((hr = stream->rclient->GetBuffer(frames, &data)) != S_OK)
				return LogHostError(hr);
#else
			if (hr == AUDCLNT_E_BUFFER_TOO_LARGE)
				return S_OK; // be silent in shared mode, try again next time
#endif
		}
		else
			return LogHostError(hr);
	}

	// Process data
	processor[S_OUTPUT].processor(NULL, 0, data, frames, processor[S_OUTPUT].userData);

	// Release buffer
	if ((hr = IAudioRenderClient_ReleaseBuffer(stream->rclient, frames, 0)) != S_OK)
		LogHostError(hr);

	return hr;
}

// ------------------------------------------------------------------------------------------
static HRESULT PollInputBuffer(PaWasapiStream *stream, PaWasapiHostProcessor *processor)
{
	HRESULT hr;
	UINT32 frames;
	BYTE *data = NULL;
	DWORD flags = 0;

	for (;;)
	{
		// Get the available data in the shared buffer.
		if ((hr = IAudioCaptureClient_GetBuffer(stream->cclient, &data, &frames, &flags, NULL, NULL)) != S_OK)
		{
			if (hr == AUDCLNT_S_BUFFER_EMPTY)
				break; // capture buffer exhausted

			return LogHostError(hr);
			break;
		}

		// Detect silence
		// if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		//	data = NULL;

 		// Process data
        processor[S_INPUT].processor(data, frames, NULL, 0, processor[S_INPUT].userData);

		// Release buffer
		if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->cclient, frames)) != S_OK)
			return LogHostError(hr);
	}

	return hr;
}

// ------------------------------------------------------------------------------------------
void _OnStreamStop(PaWasapiStream *stream)
{
	// Stop INPUT client
	if (stream->in.client != NULL)
		IAudioClient_Stop(stream->in.client);
	// Stop OUTPUT client
	if (stream->out.client != NULL)
		IAudioClient_Stop(stream->out.client);

	// Restore thread priority
	if (stream->hAvTask != NULL)
	{
		PaWasapi_ThreadPriorityRevert(stream->hAvTask);
		stream->hAvTask = NULL;
	}

    // Notify
    if (stream->streamRepresentation.streamFinishedCallback != NULL)
        stream->streamRepresentation.streamFinishedCallback(stream->streamRepresentation.userData);
}

// ------------------------------------------------------------------------------------------
PA_THREAD_FUNC ProcThreadEvent(void *param)
{
	HANDLE event[S_COUNT];
    PaWasapiHostProcessor processor[S_COUNT];
	HRESULT hr;
	DWORD dwResult;
    PaWasapiStream *stream = (PaWasapiStream *)param;
	PaWasapiHostProcessor defaultProcessor;

	// Waiting on all events in case of Full-Duplex/Exclusive mode.
	BOOL bWaitAllEvents = FALSE;
	if ((stream->in.client != NULL) && (stream->out.client != NULL))
	{
		bWaitAllEvents = (stream->in.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE) &&
			(stream->out.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE);
	}

    // Setup data processors
    defaultProcessor.processor = WaspiHostProcessingLoop;
    defaultProcessor.userData  = stream;
    processor[S_INPUT] = (stream->hostProcessOverrideInput.processor != NULL ? stream->hostProcessOverrideInput : defaultProcessor);
    processor[S_OUTPUT] = (stream->hostProcessOverrideOutput.processor != NULL ? stream->hostProcessOverrideOutput : defaultProcessor);

	// Initialize event to NULL first
	event[S_INPUT]  = CreateEvent(NULL, FALSE, FALSE, NULL);
	event[S_OUTPUT] = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Check on low resources and disallow any processing further
	if (!event[S_INPUT] || !event[S_OUTPUT])
	{
		PRINT(("WASAPI Thread: failed creating one or two event handles\n"));
		goto thread_end;
	}

	// Boost thread priority
	PaWasapi_ThreadPriorityBoost((void **)&stream->hAvTask, stream->nThreadPriority);

	// Initialize event & start INPUT stream
	if (stream->in.client)
	{
		if ((hr = IAudioClient_SetEventHandle(stream->in.client, event[S_INPUT])) != S_OK)
			LogHostError(hr);

		if ((hr = IAudioClient_GetService(stream->in.client, &pa_IID_IAudioCaptureClient, (void **)&stream->cclient)) != S_OK)
			LogHostError(hr);

		if ((hr = IAudioClient_Start(stream->in.client)) != S_OK)
			LogHostError(hr);
	}

	// Initialize event & start OUTPUT stream
	if (stream->out.client)
	{
		if ((hr = IAudioClient_SetEventHandle(stream->out.client, event[S_OUTPUT])) != S_OK)
			LogHostError(hr);

		if ((hr = IAudioClient_GetService(stream->out.client, &pa_IID_IAudioRenderClient, (void **)&stream->rclient)) != S_OK)
			LogHostError(hr);

		// Preload buffer before start
		if ((hr = FillOutputBuffer(stream, processor, stream->out.framesPerHostCallback)) != S_OK)
			LogHostError(hr);

		// Start
		if ((hr = IAudioClient_Start(stream->out.client)) != S_OK)
			LogHostError(hr);
	}

	// Signal: stream running
	stream->running = TRUE;

	// Processing Loop
	for (;;)
    {
	    // 2 sec timeout
        dwResult = WaitForMultipleObjects(S_COUNT, event, bWaitAllEvents, 10000);

		// Check for close event (after wait for buffers to avoid any calls to user
		// callback when hCloseRequest was set)
		if (WaitForSingleObject(stream->hCloseRequest, 0) != WAIT_TIMEOUT)
			break;

		// Process S_INPUT/S_OUTPUT
		switch (dwResult)
		{
		case WAIT_TIMEOUT: {
			PRINT(("WASAPI Thread: WAIT_TIMEOUT - probably bad audio driver or Vista x64 bug: use paWinWasapiPolling instead\n"));
			goto processing_stop;
			break; }

		// Input stream
		case WAIT_OBJECT_0 + S_INPUT: {
            if (stream->cclient == NULL)
                break;
			PollInputBuffer(stream, processor);
			break; }

		// Output stream
		case WAIT_OBJECT_0 + S_OUTPUT: {
            if (stream->rclient == NULL)
                break;
			FillOutputBuffer(stream, processor, stream->out.framesPerHostCallback);
			break; }
		}
	}

processing_stop:

	// Process stop
	_OnStreamStop(stream);

thread_end:

	// Close event
	CloseHandle(event[S_INPUT]);
	CloseHandle(event[S_OUTPUT]);

	// Notify thread is exiting
	SetEvent(stream->hThreadDone);

	return 0;
}

// ------------------------------------------------------------------------------------------
PA_THREAD_FUNC ProcThreadPoll(void *param)
{
    PaWasapiHostProcessor processor[S_COUNT];
	HRESULT hr;
    PaWasapiStream *stream = (PaWasapiStream *)param;
	PaWasapiHostProcessor defaultProcessor;
	INT32 i;

	// Calculate the actual duration of the allocated buffer.
	DWORD sleep_ms     = 0;
	DWORD sleep_ms_in  = GetFramesSleepTime(stream->in.framesPerHostCallback, stream->in.wavex.Format.nSamplesPerSec);
	DWORD sleep_ms_out = GetFramesSleepTime(stream->out.framesPerHostCallback, stream->out.wavex.Format.nSamplesPerSec);
	if ((sleep_ms_in != 0) && (sleep_ms_out != 0))
		sleep_ms = min(sleep_ms_in, sleep_ms_out);
	else
	{
		sleep_ms = (sleep_ms_in ? sleep_ms_in : sleep_ms_out);
	}

    // Setup data processors
    defaultProcessor.processor = WaspiHostProcessingLoop;
    defaultProcessor.userData  = stream;
    processor[S_INPUT] = (stream->hostProcessOverrideInput.processor != NULL ? stream->hostProcessOverrideInput : defaultProcessor);
    processor[S_OUTPUT] = (stream->hostProcessOverrideOutput.processor != NULL ? stream->hostProcessOverrideOutput : defaultProcessor);

	// Boost thread priority
	PaWasapi_ThreadPriorityBoost((void **)&stream->hAvTask, stream->nThreadPriority);

	// Initialize event & start INPUT stream
	if (stream->in.client)
	{
		if ((hr = IAudioClient_GetService(stream->in.client, &pa_IID_IAudioCaptureClient, (void **)&stream->cclient)) != S_OK)
			LogHostError(hr);

		if ((hr = IAudioClient_Start(stream->in.client)) != S_OK)
			LogHostError(hr);
	}


	// Initialize event & start OUTPUT stream
	if (stream->out.client)
	{
		if ((hr = IAudioClient_GetService(stream->out.client, &pa_IID_IAudioRenderClient, (void **)&stream->rclient)) != S_OK)
			LogHostError(hr);

		// Preload buffer (obligatory, othervise ->Start() will fail)
		if ((hr = FillOutputBuffer(stream, processor, stream->out.framesPerHostCallback)) != S_OK)
			LogHostError(hr);

		// Start
		if ((hr = IAudioClient_Start(stream->out.client)) != S_OK)
			LogHostError(hr);
	}

	// Signal: stream running
	stream->running = TRUE;

	// Processing Loop
	while (WaitForSingleObject(stream->hCloseRequest, sleep_ms) == WAIT_TIMEOUT)
    {
		for (i = 0; i < S_COUNT; ++i)
		{
			// Process S_INPUT/S_OUTPUT
			switch (i)
			{
			// Input stream
			case S_INPUT: {
				if (stream->cclient == NULL)
					break;
				PollInputBuffer(stream, processor);
				break; }

			// Output stream
			case S_OUTPUT: {

				UINT32 frames, padding;

				if (stream->rclient == NULL)
					break;

				frames = stream->out.framesPerHostCallback;

				// Get Read position
				padding = 0;
				hr = IAudioClient_GetCurrentPadding(stream->out.client, &padding);
				if (hr != S_OK)
				{
					LogHostError(hr);
					break;
				}

				// Fill buffer
				frames -= padding;
				if (frames != 0)
					FillOutputBuffer(stream, processor, frames);

				break; }
			}
		}
	}

	// Process stop
	_OnStreamStop(stream);

	// Notify thread is exiting
	SetEvent(stream->hThreadDone);

	return 0;
}

//#endif //VC 2005




#if 0
			if(bFirst) {
				float masteur;
				hr = stream->outVol->GetMasterVolumeLevelScalar(&masteur);
				if (hr != S_OK)
					LogHostError(hr);
				float chan1, chan2;
				hr = stream->outVol->GetChannelVolumeLevelScalar(0, &chan1);
				if (hr != S_OK)
					LogHostError(hr);
				hr = stream->outVol->GetChannelVolumeLevelScalar(1, &chan2);
				if (hr != S_OK)
					LogHostError(hr);

				BOOL bMute;
				hr = stream->outVol->GetMute(&bMute);
				if (hr != S_OK)
					LogHostError(hr);

				stream->outVol->SetMasterVolumeLevelScalar(0.5, NULL);
				stream->outVol->SetChannelVolumeLevelScalar(0, 0.5, NULL);
				stream->outVol->SetChannelVolumeLevelScalar(1, 0.5, NULL);
				stream->outVol->SetMute(FALSE, NULL);
				bFirst = FALSE;
			}
#endif
