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
#include <assert.h>
#include <mmsystem.h>
#include <mmreg.h>  // must be before other Wasapi headers
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	#include <Avrt.h>
	#define COBJMACROS
	#include <Audioclient.h>
	#include <endpointvolume.h>
	#define INITGUID // Avoid additional linkage of static libs, excessive code will be optimized out by the compiler
	#include <mmdeviceapi.h>
	#include <functiondiscoverykeys.h>
    #include <devicetopology.h>	// Used to get IKsJackDescription interface
	#undef INITGUID
#endif
#ifndef __MWERKS__
#include <malloc.h>
#include <memory.h>
#endif /* __MWERKS__ */

#include "pa_util.h"
#include "pa_allocation.h"
#include "pa_hostapi.h"
#include "pa_stream.h"
#include "pa_cpuload.h"
#include "pa_process.h"
#include "pa_win_wasapi.h"
#include "pa_debugprint.h"
#include "pa_ringbuffer.h"

#ifndef NTDDI_VERSION
 
    #undef WINVER
    #undef _WIN32_WINNT
    #define WINVER       0x0600 // VISTA
	#define _WIN32_WINNT WINVER

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

    #ifdef WIN64
        #include <wtypes.h>
        typedef LONG NTSTATUS;
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
    
    #ifndef WAVE_FORMAT_IEEE_FLOAT
        #define WAVE_FORMAT_IEEE_FLOAT 0x0003 // 32-bit floating-point
    #endif    
    
    #ifndef __MINGW_EXTENSION
        #if defined(__GNUC__) || defined(__GNUG__)
            #define __MINGW_EXTENSION __extension__
        #else
            #define __MINGW_EXTENSION
        #endif
    #endif 

    #include <sdkddkver.h>
    #include <propkeydef.h>
    #define COBJMACROS
    #define INITGUID // Avoid additional linkage of static libs, excessive code will be optimized out by the compiler
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <endpointvolume.h>
    #include <functiondiscoverykeys.h>
	#include <devicetopology.h>	// Used to get IKsJackDescription interface
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
// *2A07407E-6497-4A18-9787-32F79BD0D98F*  Or this??
PA_DEFINE_IID(IDeviceTopology,      2A07407E, 6497, 4A18, 97, 87, 32, f7, 9b, d0, d9, 8f);
// *AE2DE0E4-5BCA-4F2D-AA46-5D13F8FDB3A9*
PA_DEFINE_IID(IPart,                AE2DE0E4, 5BCA, 4F2D, aa, 46, 5d, 13, f8, fd, b3, a9);
// *4509F757-2D46-4637-8E62-CE7DB944F57B*
PA_DEFINE_IID(IKsJackDescription,   4509F757, 2D46, 4637, 8e, 62, ce, 7d, b9, 44, f5, 7b);
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

enum { S_INPUT = 0, S_OUTPUT = 1, S_COUNT = 2, S_FULLDUPLEX = 0 };

// Number of packets which compose single contignous buffer. With trial and error it was calculated
// that WASAPI Input sub-system uses 6 packets per whole buffer. Please provide more information
// or corrections if available.
enum { WASAPI_PACKETS_PER_INPUT_BUFFER = 6 };

#define STATIC_ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

#define PRINT(x) PA_DEBUG(x);

#define PA_SKELETON_SET_LAST_HOST_ERROR( errorCode, errorText ) \
    PaUtil_SetLastHostErrorInfo( paWASAPI, errorCode, errorText )

#define PA_WASAPI__IS_FULLDUPLEX(STREAM) ((STREAM)->in.clientProc && (STREAM)->out.clientProc)

#ifndef IF_FAILED_JUMP
#define IF_FAILED_JUMP(hr, label) if(FAILED(hr)) goto label;
#endif

#ifndef IF_FAILED_INTERNAL_ERROR_JUMP
#define IF_FAILED_INTERNAL_ERROR_JUMP(hr, error, label) if(FAILED(hr)) { error = paInternalError; goto label; }
#endif

#define SAFE_CLOSE(h) if ((h) != NULL) { CloseHandle((h)); (h) = NULL; }
#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->lpVtbl->Release((punk)); (punk) = NULL; }

// Mixer function
typedef void (*MixMonoToStereoF) (void *__to, void *__from, UINT32 count);

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
    PaUtilStreamInterface       callbackStreamInterface;
    PaUtilStreamInterface       blockingStreamInterface;

    PaUtilAllocationGroup      *allocations;

    /* implementation specific data goes here */

    //in case we later need the synch
    IMMDeviceEnumerator *enumerator;

    //this is the REAL number of devices, whether they are usefull to PA or not!
    UINT32 deviceCount;

    WCHAR defaultRenderer [MAX_STR_LEN];
    WCHAR defaultCapturer [MAX_STR_LEN];

    PaWasapiDeviceInfo *devInfo;

	// Is true when WOW64 Vista/7 Workaround is needed
	BOOL useWOW64Workaround;
}
PaWasapiHostApiRepresentation;

// ------------------------------------------------------------------------------------------
/* PaWasapiAudioClientParams - audio client parameters */
typedef struct PaWasapiAudioClientParams
{
	PaWasapiDeviceInfo *device_info;
	PaStreamParameters  stream_params;
	PaWasapiStreamInfo  wasapi_params;
	UINT32              frames_per_buffer;
	double              sample_rate;
	BOOL                blocking;
	BOOL                full_duplex;
	BOOL                wow64_workaround;
}
PaWasapiAudioClientParams;

// ------------------------------------------------------------------------------------------
/* PaWasapiStream - a stream data structure specifically for this implementation */
typedef struct PaWasapiSubStream
{
    IAudioClient        *clientParent;
	IStream				*clientStream;
	IAudioClient		*clientProc;

    WAVEFORMATEXTENSIBLE wavex;
    UINT32               bufferSize;
    REFERENCE_TIME       deviceLatency;
    REFERENCE_TIME       period;
	double				 latencySeconds;
    UINT32				 framesPerHostCallback;
	AUDCLNT_SHAREMODE    shareMode;
	UINT32               streamFlags; // AUDCLNT_STREAMFLAGS_EVENTCALLBACK, ...
	UINT32               flags;
	PaWasapiAudioClientParams params; //!< parameters

	// Buffers
	UINT32               buffers;			//!< number of buffers used (from host side)
	UINT32               framesPerBuffer;	//!< number of frames per 1 buffer
	BOOL                 userBufferAndHostMatch;

	// Used for Mono >> Stereo workaround, if driver does not support it
	// (in Exclusive mode WASAPI usually refuses to operate with Mono (1-ch)
	void                *monoBuffer;	 //!< pointer to buffer
	UINT32               monoBufferSize; //!< buffer size in bytes
	MixMonoToStereoF     monoMixer;		 //!< pointer to mixer function

	PaUtilRingBuffer    *tailBuffer;       //!< buffer with trailing sample for blocking mode operations (only for Input)
	void                *tailBufferMemory; //!< tail buffer memory region
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
    PaUtilCpuLoadMeasurer      cpuLoadMeasurer;
    PaUtilBufferProcessor      bufferProcessor;

    // input
	PaWasapiSubStream          in;
    IAudioCaptureClient       *captureClientParent;
	IStream                   *captureClientStream;
	IAudioCaptureClient       *captureClient;
    IAudioEndpointVolume      *inVol;

	// output
	PaWasapiSubStream          out;
    IAudioRenderClient        *renderClientParent;
	IStream                   *renderClientStream;
	IAudioRenderClient        *renderClient;
	IAudioEndpointVolume      *outVol;

	// event handles for event-driven processing mode
	HANDLE event[S_COUNT];

	// buffer mode
	PaUtilHostBufferSizeMode bufferMode;

	// must be volatile to avoid race condition on user query while
	// thread is being started
    volatile BOOL running;

    PA_THREAD_ID dwThreadId;
    HANDLE hThread;
	HANDLE hCloseRequest;
	HANDLE hThreadStart;        //!< signalled by thread on start
	HANDLE hThreadExit;         //!< signalled by thread on exit
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
void _StreamOnStop(PaWasapiStream *stream);
void _StreamFinish(PaWasapiStream *stream);
void _StreamCleanup(PaWasapiStream *stream);
HRESULT _PollGetOutputFramesAvailable(PaWasapiStream *stream, UINT32 *available);
HRESULT _PollGetInputFramesAvailable(PaWasapiStream *stream, UINT32 *available);
void *PaWasapi_ReallocateMemory(void *ptr, size_t size);
void PaWasapi_FreeMemory(void *ptr);

// Local statics
static volatile BOOL  g_WasapiCOMInit    = FALSE;
static volatile DWORD g_WasapiInitThread = 0;

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

	// other windows common errors:
	case CO_E_NOTINITIALIZED                    :text ="CO_E_NOTINITIALIZED: you must call CoInitialize() before Pa_OpenStream()"; break;

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
/*! \class ThreadSleepScheduler
           Allows to emulate thread sleep of less than 1 millisecond under Windows. Scheduler
		   calculates number of times the thread must run untill next sleep of 1 millisecond.
		   It does not make thread sleeping for real number of microseconds but rather controls
		   how many of imaginary microseconds the thread task can allow thread to sleep.
*/
typedef struct ThreadIdleScheduler
{
	UINT32 m_idle_microseconds; //!< number of microseconds to sleep
	UINT32 m_next_sleep;        //!< next sleep round
	UINT32 m_i;					//!< current round iterator position
	UINT32 m_resolution;		//!< resolution in number of milliseconds
}
ThreadIdleScheduler;
//! Setup scheduler.
static void ThreadIdleScheduler_Setup(ThreadIdleScheduler *sched, UINT32 resolution, UINT32 microseconds)
{
	assert(microseconds != 0);
	assert(resolution != 0);
	assert((resolution * 1000) >= microseconds);

	memset(sched, 0, sizeof(*sched));

	sched->m_idle_microseconds = microseconds;
	sched->m_resolution        = resolution;
	sched->m_next_sleep        = (resolution * 1000) / microseconds;
}
//! Iterate and check if can sleep.
static UINT32 ThreadIdleScheduler_NextSleep(ThreadIdleScheduler *sched)
{
	// advance and check if thread can sleep
	if (++ sched->m_i == sched->m_next_sleep)
	{
		sched->m_i = 0;
		return sched->m_resolution;
	}
	return 0;
}

// ------------------------------------------------------------------------------------------
/*static double nano100ToMillis(REFERENCE_TIME ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return ((double)ref)*0.0001;
}*/

// ------------------------------------------------------------------------------------------
static double nano100ToSeconds(REFERENCE_TIME ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return ((double)ref)*0.0000001;
}

// ------------------------------------------------------------------------------------------
/*static REFERENCE_TIME MillisTonano100(double ref)
{
    //  1 nano = 0.000000001 seconds
    //100 nano = 0.0000001   seconds
    //100 nano = 0.0001   milliseconds
    return (REFERENCE_TIME)(ref/0.0001);
}*/

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
/*static WORD PaSampleFormatToBytesPerSample(PaSampleFormat format_id)
{
	return PaSampleFormatToBitsPerSample(format_id) >> 3; // 'bits/8'
}*/

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

// Aligning function type
typedef UINT32 (*ALIGN_FUNC) (UINT32 v, UINT32 align);

// ------------------------------------------------------------------------------------------
// Aligns 'v' backwards
static UINT32 ALIGN_BWD(UINT32 v, UINT32 align)
{
	return ((v - (align ? v % align : 0)));
}

// ------------------------------------------------------------------------------------------
// Aligns 'v' forward
static UINT32 ALIGN_FWD(UINT32 v, UINT32 align)
{
	UINT32 remainder = (align ? (v % align) : 0);
	if (remainder == 0)
		return v;
	return v + (align - remainder);
}

// ------------------------------------------------------------------------------------------
// Get next value power of 2
UINT32 ALIGN_NEXT_POW2(UINT32 v)
{
	UINT32 v2 = 1;
	while (v > (v2 <<= 1)) { }
	v = v2;
	return v;
}

// ------------------------------------------------------------------------------------------
// Aligns WASAPI buffer to 128 byte packet boundary. HD Audio will fail to play if buffer
// is misaligned. This problem was solved in Windows 7 were AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED
// is thrown although we must align for Vista anyway.
static UINT32 AlignFramesPerBuffer(UINT32 nFrames, UINT32 nSamplesPerSec, UINT32 nBlockAlign,
								   ALIGN_FUNC pAlignFunc)
{
#define HDA_PACKET_SIZE (128)

	long frame_bytes = nFrames * nBlockAlign;
	long packets;

	// align to packet size
	frame_bytes  = pAlignFunc(frame_bytes, HDA_PACKET_SIZE); // use ALIGN_FWD if bigger but safer period is more desired

	// atlest 1 frame must be available
	if (frame_bytes < HDA_PACKET_SIZE)
		frame_bytes = HDA_PACKET_SIZE;

	nFrames      = frame_bytes / nBlockAlign;
	packets      = frame_bytes / HDA_PACKET_SIZE;

	frame_bytes = packets * HDA_PACKET_SIZE;
	nFrames     = frame_bytes / nBlockAlign;

	return nFrames;

#undef HDA_PACKET_SIZE
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
static UINT32 GetFramesSleepTimeMicroseconds(UINT32 nFrames, UINT32 nSamplesPerSec)
{
	REFERENCE_TIME nDuration;
	if (nSamplesPerSec == 0)
		return 0;
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
	// Calculate the actual duration of the allocated buffer.
	nDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC * nFrames / nSamplesPerSec);
	return (UINT32)(nDuration/10/2);
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
	WINDOWS_UNKNOWN			 = 0,
	WINDOWS_VISTA_SERVER2008 = (1 << 0),
	WINDOWS_7_SERVER2008R2	 = (1 << 1),
	WINDOWS_FUTURE           = (1 << 2)
}
EWindowsVersion;
// Defines Windows 7/Windows Server 2008 R2 and up (future versions)
#define WINDOWS_7_SERVER2008R2_AND_UP (WINDOWS_7_SERVER2008R2|WINDOWS_FUTURE)
// The function is limited to Vista/7 mostly as we need just to find out Vista/WOW64 combination
// in order to use WASAPI WOW64 workarounds.
static UINT32 GetWindowsVersion()
{
	static UINT32 version = WINDOWS_UNKNOWN;

	if (version == WINDOWS_UNKNOWN)
	{
		DWORD dwVersion = 0;
		DWORD dwMajorVersion = 0;
		DWORD dwMinorVersion = 0;
		DWORD dwBuild = 0;

		typedef DWORD (WINAPI *LPFN_GETVERSION)(VOID);
		LPFN_GETVERSION fnGetVersion;

		fnGetVersion = (LPFN_GETVERSION) GetProcAddress(GetModuleHandle(TEXT("kernel32")), TEXT("GetVersion"));
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
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			break; // skip lower
		case 6:
			switch (dwMinorVersion)
			{
			case 0:
				version |= WINDOWS_VISTA_SERVER2008;
				break;
			case 1:
				version |= WINDOWS_7_SERVER2008R2;
				break;
			default:
				version |= WINDOWS_FUTURE;
			}
			break;
		default:
			version |= WINDOWS_FUTURE;
		}
	}

	return version;
}

// ------------------------------------------------------------------------------------------
static BOOL UseWOW64Workaround()
{
	// note: WOW64 bug is common to Windows Vista x64, thus we fall back to safe Poll-driven
	//       method. Windows 7 x64 seems has WOW64 bug fixed.

	return (IsWow64() && (GetWindowsVersion() & WINDOWS_VISTA_SERVER2008));
}

// ------------------------------------------------------------------------------------------
typedef enum EMixerDir { MIX_DIR__1TO2, MIX_DIR__2TO1, MIX_DIR__2TO1_L } EMixerDir;

// ------------------------------------------------------------------------------------------
#define _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(TYPE)\
	TYPE * __restrict to   = __to;\
	TYPE * __restrict from = __from;\
	TYPE * __restrict end  = from + count;\
	while (from != end)\
	{\
		*to ++ = *from;\
		*to ++ = *from;\
		++ from;\
	}

// ------------------------------------------------------------------------------------------
#define _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_FLT32(TYPE)\
	TYPE * __restrict to   = (TYPE *)__to;\
	TYPE * __restrict from = (TYPE *)__from;\
	TYPE * __restrict end  = to + count;\
	while (to != end)\
	{\
		*to ++ = (TYPE)((float)(from[0] + from[1]) * 0.5f);\
		from += 2;\
	}

// ------------------------------------------------------------------------------------------
#define _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT32(TYPE)\
	TYPE * __restrict to   = (TYPE *)__to;\
	TYPE * __restrict from = (TYPE *)__from;\
	TYPE * __restrict end  = to + count;\
	while (to != end)\
	{\
		*to ++ = (TYPE)(((INT32)from[0] + (INT32)from[1]) >> 1);\
		from += 2;\
	}

// ------------------------------------------------------------------------------------------
#define _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT64(TYPE)\
	TYPE * __restrict to   = (TYPE *)__to;\
	TYPE * __restrict from = (TYPE *)__from;\
	TYPE * __restrict end  = to + count;\
	while (to != end)\
	{\
		*to ++ = (TYPE)(((INT64)from[0] + (INT64)from[1]) >> 1);\
		from += 2;\
	}

// ------------------------------------------------------------------------------------------
#define _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(TYPE)\
	TYPE * __restrict to   = (TYPE *)__to;\
	TYPE * __restrict from = (TYPE *)__from;\
	TYPE * __restrict end  = to + count;\
	while (to != end)\
	{\
		*to ++ = from[0];\
		from += 2;\
	}

// ------------------------------------------------------------------------------------------
static void _MixMonoToStereo_1TO2_8(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(BYTE); }
static void _MixMonoToStereo_1TO2_16(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(short); }
static void _MixMonoToStereo_1TO2_24(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(int); /* !!! int24 data is contained in 32-bit containers*/ }
static void _MixMonoToStereo_1TO2_32(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(int); }
static void _MixMonoToStereo_1TO2_32f(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_1_TO_2(float); }

// ------------------------------------------------------------------------------------------
static void _MixMonoToStereo_2TO1_8(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT32(BYTE); }
static void _MixMonoToStereo_2TO1_16(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT32(short); }
static void _MixMonoToStereo_2TO1_24(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT32(int); /* !!! int24 data is contained in 32-bit containers*/ }
static void _MixMonoToStereo_2TO1_32(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_INT64(int); }
static void _MixMonoToStereo_2TO1_32f(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_FLT32(float); }

// ------------------------------------------------------------------------------------------
static void _MixMonoToStereo_2TO1_8_L(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(BYTE); }
static void _MixMonoToStereo_2TO1_16_L(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(short); }
static void _MixMonoToStereo_2TO1_24_L(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(int); /* !!! int24 data is contained in 32-bit containers*/ }
static void _MixMonoToStereo_2TO1_32_L(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(int); }
static void _MixMonoToStereo_2TO1_32f_L(void *__to, void *__from, UINT32 count) { _WASAPI_MONO_TO_STEREO_MIXER_2_TO_1_L(float); }

// ------------------------------------------------------------------------------------------
static MixMonoToStereoF _GetMonoToStereoMixer(PaSampleFormat format, EMixerDir dir)
{
	switch (dir)
	{
	case MIX_DIR__1TO2:
		switch (format & ~paNonInterleaved)
		{
		case paUInt8:	return _MixMonoToStereo_1TO2_8;
		case paInt16:	return _MixMonoToStereo_1TO2_16;
		case paInt24:	return _MixMonoToStereo_1TO2_24;
		case paInt32:	return _MixMonoToStereo_1TO2_32;
		case paFloat32: return _MixMonoToStereo_1TO2_32f;
		}
		break;

	case MIX_DIR__2TO1:
		switch (format & ~paNonInterleaved)
		{
		case paUInt8:	return _MixMonoToStereo_2TO1_8;
		case paInt16:	return _MixMonoToStereo_2TO1_16;
		case paInt24:	return _MixMonoToStereo_2TO1_24;
		case paInt32:	return _MixMonoToStereo_2TO1_32;
		case paFloat32: return _MixMonoToStereo_2TO1_32f;
		}
		break;

	case MIX_DIR__2TO1_L:
		switch (format & ~paNonInterleaved)
		{
		case paUInt8:	return _MixMonoToStereo_2TO1_8_L;
		case paInt16:	return _MixMonoToStereo_2TO1_16_L;
		case paInt24:	return _MixMonoToStereo_2TO1_24_L;
		case paInt32:	return _MixMonoToStereo_2TO1_32_L;
		case paFloat32: return _MixMonoToStereo_2TO1_32f_L;
		}
		break;
	}

	return NULL;
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
        g_WasapiCOMInit = TRUE;

	// Memorize calling thread id and report warning on Uninitialize if calling thread is different
	// as CoInitialize must match CoUninitialize in the same thread.
	g_WasapiInitThread = GetCurrentThreadId();

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
    
	// We need to set the result to a value otherwise we will return paNoError
	// [IF_FAILED_JUMP(hResult, error);]
	IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

    // getting default device ids in the eMultimedia "role"
    {
        {
            IMMDevice *defaultRenderer = NULL;
            hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(paWasapi->enumerator, eRender, eMultimedia, &defaultRenderer);
            if (hr != S_OK)
			{
				if (hr != E_NOTFOUND) {
					// We need to set the result to a value otherwise we will return paNoError
					// [IF_FAILED_JUMP(hResult, error);]
					IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
				}
			}
			else
			{
				WCHAR *pszDeviceId = NULL;
				hr = IMMDevice_GetId(defaultRenderer, &pszDeviceId);
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hResult, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
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
				if (hr != E_NOTFOUND) {
					// We need to set the result to a value otherwise we will return paNoError
					// [IF_FAILED_JUMP(hResult, error);]
					IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
				}
			}
			else
			{
				WCHAR *pszDeviceId = NULL;
				hr = IMMDevice_GetId(defaultCapturer, &pszDeviceId);
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hResult, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
				wcsncpy(paWasapi->defaultCapturer, pszDeviceId, MAX_STR_LEN-1);
				CoTaskMemFree(pszDeviceId);
				IMMDevice_Release(defaultCapturer);
			}
        }
    }

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(paWasapi->enumerator, eAll, DEVICE_STATE_ACTIVE, &pEndPoints);
	// We need to set the result to a value otherwise we will return paNoError
	// [IF_FAILED_JUMP(hResult, error);]
	IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

    hr = IMMDeviceCollection_GetCount(pEndPoints, &paWasapi->deviceCount);
	// We need to set the result to a value otherwise we will return paNoError
	// [IF_FAILED_JUMP(hResult, error);]
	IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

    paWasapi->devInfo = (PaWasapiDeviceInfo *)PaUtil_AllocateMemory(sizeof(PaWasapiDeviceInfo) * paWasapi->deviceCount);
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

			PA_DEBUG(("WASAPI: device idx: %02d\n", i));
			PA_DEBUG(("WASAPI: ---------------\n"));

            hr = IMMDeviceCollection_Item(pEndPoints, i, &paWasapi->devInfo[i].device);
			// We need to set the result to a value otherwise we will return paNoError
			// [IF_FAILED_JUMP(hResult, error);]
			IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

            // getting ID
            {
                WCHAR *pszDeviceId = NULL;
                hr = IMMDevice_GetId(paWasapi->devInfo[i].device, &pszDeviceId);
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hr, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
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
			// We need to set the result to a value otherwise we will return paNoError
			// [IF_FAILED_JUMP(hResult, error);]
			IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

            if (paWasapi->devInfo[i].state != DEVICE_STATE_ACTIVE)
			{
                PRINT(("WASAPI device: %d is not currently available (state:%d)\n",i,state));
            }

            {
                IPropertyStore *pProperty;
                hr = IMMDevice_OpenPropertyStore(paWasapi->devInfo[i].device, STGM_READ, &pProperty);
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hResult, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

                // "Friendly" Name
                {
					char *deviceName;
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_Device_FriendlyName, &value);
					// We need to set the result to a value otherwise we will return paNoError
					// [IF_FAILED_JUMP(hResult, error);]
					IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
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
					PA_DEBUG(("WASAPI:%d| name[%s]\n", i, deviceInfo->name));
                }

                // Default format
                {
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_AudioEngine_DeviceFormat, &value);
					// We need to set the result to a value otherwise we will return paNoError
					// [IF_FAILED_JUMP(hResult, error);]
					IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
					memcpy(&paWasapi->devInfo[i].DefaultFormat, value.blob.pBlobData, min(sizeof(paWasapi->devInfo[i].DefaultFormat), value.blob.cbSize));
                    // cleanup
                    PropVariantClear(&value);
                }

                // Formfactor
                {
                    PROPVARIANT value;
                    PropVariantInit(&value);
                    hr = IPropertyStore_GetValue(pProperty, &PKEY_AudioEndpoint_FormFactor, &value);
					// We need to set the result to a value otherwise we will return paNoError
					// [IF_FAILED_JUMP(hResult, error);]
					IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);
					// set
					#if defined(DUMMYUNIONNAME) && defined(NONAMELESSUNION)
						// avoid breaking strict-aliasing rules in such line: (EndpointFormFactor)(*((UINT *)(((WORD *)&value.wReserved3)+1)));
						UINT v;
						memcpy(&v, (((WORD *)&value.wReserved3)+1), sizeof(v));
						paWasapi->devInfo[i].formFactor = (EndpointFormFactor)v;
					#else
						paWasapi->devInfo[i].formFactor = (EndpointFormFactor)value.uintVal;
					#endif
					PA_DEBUG(("WASAPI:%d| form-factor[%d]\n", i, paWasapi->devInfo[i].formFactor));
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
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hResult, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

                hr = IAudioClient_GetDevicePeriod(tmpClient,
                    &paWasapi->devInfo[i].DefaultDevicePeriod,
                    &paWasapi->devInfo[i].MinimumDevicePeriod);
				// We need to set the result to a value otherwise we will return paNoError
				// [IF_FAILED_JUMP(hResult, error);]
				IF_FAILED_INTERNAL_ERROR_JUMP(hr, result, error);

                //hr = tmpClient->GetMixFormat(&paWasapi->devInfo[i].MixFormat);

				// Release client
				SAFE_RELEASE(tmpClient);

				if (hr != S_OK)
				{
					//davidv: this happened with my hardware, previously for that same device in DirectSound:
					//Digital Output (Realtek AC'97 Audio)'s GUID: {0x38f2cf50,0x7b4c,0x4740,0x86,0xeb,0xd4,0x38,0x66,0xd8,0xc8, 0x9f}
					//so something must be _really_ wrong with this device, TODO handle this better. We kind of need GetMixFormat
					LogHostError(hr);
					// We need to set the result to a value otherwise we will return paNoError
					result = paInternalError;
					goto error;
				}
            }

            // we can now fill in portaudio device data
            deviceInfo->maxInputChannels  = 0;
            deviceInfo->maxOutputChannels = 0;
			deviceInfo->defaultSampleRate = paWasapi->devInfo[i].DefaultFormat.Format.nSamplesPerSec;
            switch (paWasapi->devInfo[i].flow)
			{
			case eRender: {
                deviceInfo->maxOutputChannels		 = paWasapi->devInfo[i].DefaultFormat.Format.nChannels;
                deviceInfo->defaultHighOutputLatency = nano100ToSeconds(paWasapi->devInfo[i].DefaultDevicePeriod);
                deviceInfo->defaultLowOutputLatency  = nano100ToSeconds(paWasapi->devInfo[i].MinimumDevicePeriod);
				PA_DEBUG(("WASAPI:%d| def.SR[%d] max.CH[%d] latency{hi[%f] lo[%f]}\n", i, (UINT32)deviceInfo->defaultSampleRate,
					deviceInfo->maxOutputChannels, (float)deviceInfo->defaultHighOutputLatency, (float)deviceInfo->defaultLowOutputLatency));
				break;}
			case eCapture: {
                deviceInfo->maxInputChannels		= paWasapi->devInfo[i].DefaultFormat.Format.nChannels;
                deviceInfo->defaultHighInputLatency = nano100ToSeconds(paWasapi->devInfo[i].DefaultDevicePeriod);
                deviceInfo->defaultLowInputLatency  = nano100ToSeconds(paWasapi->devInfo[i].MinimumDevicePeriod);
				PA_DEBUG(("WASAPI:%d| def.SR[%d] max.CH[%d] latency{hi[%f] lo[%f]}\n", i, (UINT32)deviceInfo->defaultSampleRate,
					deviceInfo->maxInputChannels, (float)deviceInfo->defaultHighInputLatency, (float)deviceInfo->defaultLowInputLatency));
				break; }
            default:
                PRINT(("WASAPI:%d| bad Data Flow!\n", i));
				// We need to set the result to a value otherwise we will return paNoError
				result = paInternalError;
                //continue; // do not skip from list, allow to initialize
            break;
            }

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
	paWasapi->useWOW64Workaround = UseWOW64Workaround();

    SAFE_RELEASE(pEndPoints);

	PRINT(("WASAPI: initialized ok\n"));

    return paNoError;

error:

	PRINT(("WASAPI: failed %s error[%d|%s]\n", __FUNCTION__, result, Pa_GetErrorText(result)));

    SAFE_RELEASE(pEndPoints);

	Terminate((PaUtilHostApiRepresentation *)paWasapi);

	// Safety if error was not set so that we do not think initialize was a success
	if (result == paNoError) {
		result = paInternalError;
	}

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
    PaUtil_FreeMemory(paWasapi->devInfo);

    if (paWasapi->allocations)
	{
        PaUtil_FreeAllAllocations(paWasapi->allocations);
        PaUtil_DestroyAllocationGroup(paWasapi->allocations);
    }

    PaUtil_FreeMemory(paWasapi);

	// Close AVRT
	CloseAVRT();

	// Uninit COM (checking calling thread we won't unitialize user's COM if one is calling
	//             Pa_Unitialize by mistake from not initializing thread)
    if (g_WasapiCOMInit)
	{
		DWORD calling_thread_id = GetCurrentThreadId();
		if (g_WasapiInitThread != calling_thread_id)
		{
			PRINT(("WASAPI: failed CoUninitializes calling thread[%d] does not match initializing thread[%d]\n",
				calling_thread_id, g_WasapiInitThread));
		}
		else
		{
			CoUninitialize();
		}
	}
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
		return paNotInitialized;

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
PaError PaWasapi_GetFramesPerHostBuffer( PaStream *pStream, unsigned int *nInput, unsigned int *nOutput )
{
    PaWasapiStream *stream = (PaWasapiStream *)pStream;
	if (stream == NULL)
		return paBadStreamPtr;

	if (nInput != NULL)
		(*nInput) = stream->in.framesPerHostCallback;

	if (nOutput != NULL)
		(*nOutput) = stream->out.framesPerHostCallback;

	return paNoError;
}

// ------------------------------------------------------------------------------------------
static void LogWAVEFORMATEXTENSIBLE(const WAVEFORMATEXTENSIBLE *in)
{
    const WAVEFORMATEX *old = (WAVEFORMATEX *)in;
	switch (old->wFormatTag)
	{
	case WAVE_FORMAT_EXTENSIBLE: {

		PRINT(("wFormatTag     =WAVE_FORMAT_EXTENSIBLE\n"));

		if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		{
			PRINT(("SubFormat      =KSDATAFORMAT_SUBTYPE_IEEE_FLOAT\n"));
		}
		else
		if (IsEqualGUID(&in->SubFormat, &pa_KSDATAFORMAT_SUBTYPE_PCM))
		{
			PRINT(("SubFormat      =KSDATAFORMAT_SUBTYPE_PCM\n"));
		}
		else
		{
			PRINT(("SubFormat      =CUSTOM GUID{%d:%d:%d:%d%d%d%d%d%d%d%d}\n",
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
		PRINT(("Samples.wValidBitsPerSample =%d\n",  in->Samples.wValidBitsPerSample));
		PRINT(("dwChannelMask  =0x%X\n",in->dwChannelMask));

		break; }

	case WAVE_FORMAT_PCM:        PRINT(("wFormatTag     =WAVE_FORMAT_PCM\n")); break;
	case WAVE_FORMAT_IEEE_FLOAT: PRINT(("wFormatTag     =WAVE_FORMAT_IEEE_FLOAT\n")); break;
	default: 
		PRINT(("wFormatTag     =UNKNOWN(%d)\n",old->wFormatTag)); break;
	}

	PRINT(("nChannels      =%d\n",old->nChannels));
	PRINT(("nSamplesPerSec =%d\n",old->nSamplesPerSec));
	PRINT(("nAvgBytesPerSec=%d\n",old->nAvgBytesPerSec));
	PRINT(("nBlockAlign    =%d\n",old->nBlockAlign));
	PRINT(("wBitsPerSample =%d\n",old->wBitsPerSample));
	PRINT(("cbSize         =%d\n",old->cbSize));
}

// ------------------------------------------------------------------------------------------
static PaSampleFormat WaveToPaFormat(const WAVEFORMATEXTENSIBLE *in)
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

    // WAVEFORMATEX
    if ((params->channelCount <= 2) && ((bitsPerSample == 16) || (bitsPerSample == 8)))
	{
        old->cbSize		= 0;
        old->wFormatTag	= WAVE_FORMAT_PCM;
    }
    // WAVEFORMATEXTENSIBLE
    else
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
			case 3:  wavex->dwChannelMask = KSAUDIO_SPEAKER_STEREO|SPEAKER_LOW_FREQUENCY; break;
			case 4:  wavex->dwChannelMask = KSAUDIO_SPEAKER_QUAD; break;
			case 5:  wavex->dwChannelMask = KSAUDIO_SPEAKER_QUAD|SPEAKER_LOW_FREQUENCY; break;
#ifdef KSAUDIO_SPEAKER_5POINT1_SURROUND
			case 6:  wavex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND; break;
#else
			case 6:  wavex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
#endif
#ifdef KSAUDIO_SPEAKER_5POINT1_SURROUND
			case 7:  wavex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND|SPEAKER_BACK_CENTER; break;
#else
			case 7:  wavex->dwChannelMask = KSAUDIO_SPEAKER_5POINT1|SPEAKER_BACK_CENTER; break;
#endif	
#ifdef KSAUDIO_SPEAKER_7POINT1_SURROUND
			case 8:  wavex->dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND; break;
#else
			case 8:  wavex->dwChannelMask = KSAUDIO_SPEAKER_7POINT1; break;
#endif

			default: wavex->dwChannelMask = 0;
			}
		}
	}
    return paNoError;
}

// ------------------------------------------------------------------------------------------
/*static void wasapiFillWFEXT( WAVEFORMATEXTENSIBLE* pwfext, PaSampleFormat sampleFormat, double sampleRate, int channelCount)
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
}*/

// ------------------------------------------------------------------------------------------
static PaError GetClosestFormat(IAudioClient *myClient, double sampleRate,
	const PaStreamParameters *_params, AUDCLNT_SHAREMODE shareMode, WAVEFORMATEXTENSIBLE *outWavex,
	BOOL output)
{
	PaError answer                   = paInvalidSampleRate;
	WAVEFORMATEX *sharedClosestMatch = NULL;
	HRESULT hr                       = !S_OK;
	PaStreamParameters params       = (*_params);

	/* It was not noticed that 24-bit Input producing no output while device accepts this format.
	   To fix this issue let's ask for 32-bits and let PA converters convert host 32-bit data
	   to 24-bit for user-space. The bug concerns Vista, if Windows 7 supports 24-bits for Input
	   please report to PortAudio developers to exclude Windows 7.
	*/
	/*if ((params.sampleFormat == paInt24) && (output == FALSE))
		params.sampleFormat = paFloat32;*/ // <<< The silence was due to missing Int32_To_Int24_Dither implementation

    MakeWaveFormatFromParams(outWavex, &params, sampleRate);

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
		if ((WORD)params.channelCount != outWavex->Format.nChannels)
		{
			// If mono, then driver does not support 1 channel, we use internal workaround
			// of tiny software mixing functionality, e.g. we provide to user buffer 1 channel
			// but then mix into 2 for device buffer
			if ((params.channelCount == 1) && (outWavex->Format.nChannels == 2))
				return paFormatIsSupported;
			else
				return paInvalidChannelCount;
		}

		// Validate Sample format
		if ((bitsPerSample = PaSampleFormatToBitsPerSample(params.sampleFormat)) == 0)
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
		static const int BestToWorst[] = { paFloat32, paInt24, paInt16 };
		int i;

		// Try combination stereo and we will use built-in mono-stereo mixer then
		if (params.channelCount == 1)
		{
			WAVEFORMATEXTENSIBLE stereo = { 0 };

			PaStreamParameters stereo_params = params;
			stereo_params.channelCount = 2;

			MakeWaveFormatFromParams(&stereo, &stereo_params, sampleRate);

			hr = IAudioClient_IsFormatSupported(myClient, shareMode, &stereo.Format, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
			if (hr == S_OK)
			{
				memcpy(outWavex, &stereo, sizeof(WAVEFORMATEXTENSIBLE));
				CoTaskMemFree(sharedClosestMatch);
				return (answer = paFormatIsSupported);
			}

			// Try selecting suitable sample type
			for (i = 0; i < STATIC_ARRAY_SIZE(BestToWorst); ++i)
			{
				WAVEFORMATEXTENSIBLE sample = { 0 };

				PaStreamParameters sample_params = stereo_params;
				sample_params.sampleFormat = BestToWorst[i];

				MakeWaveFormatFromParams(&sample, &sample_params, sampleRate);

				hr = IAudioClient_IsFormatSupported(myClient, shareMode, &sample.Format, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
				if (hr == S_OK)
				{
					memcpy(outWavex, &sample, sizeof(WAVEFORMATEXTENSIBLE));
					CoTaskMemFree(sharedClosestMatch);
					return (answer = paFormatIsSupported);
				}
			}
		}

		// Try selecting suitable sample type
		for (i = 0; i < STATIC_ARRAY_SIZE(BestToWorst); ++i)
		{
			WAVEFORMATEXTENSIBLE spfmt = { 0 };

			PaStreamParameters spfmt_params = params;
			spfmt_params.sampleFormat = BestToWorst[i];

			MakeWaveFormatFromParams(&spfmt, &spfmt_params, sampleRate);

			hr = IAudioClient_IsFormatSupported(myClient, shareMode, &spfmt.Format, (shareMode == AUDCLNT_SHAREMODE_SHARED ? &sharedClosestMatch : NULL));
			if (hr == S_OK)
			{
				memcpy(outWavex, &spfmt, sizeof(WAVEFORMATEXTENSIBLE));
				CoTaskMemFree(sharedClosestMatch);
				answer = paFormatIsSupported;
				break;
			}
		}

		// Nothing helped
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

		answer = GetClosestFormat(tmpClient, sampleRate, inputParameters, shareMode, &wavex, FALSE);
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

		answer = GetClosestFormat(tmpClient, sampleRate, outputParameters, shareMode, &wavex, TRUE);
		SAFE_RELEASE(tmpClient);

		if (answer != paFormatIsSupported)
			return answer;
    }

    return paFormatIsSupported;
}

// ------------------------------------------------------------------------------------------
static PaUint32 PaUtil_GetFramesPerHostBuffer(PaUint32 userFramesPerBuffer, PaTime suggestedLatency, double sampleRate, PaUint32 TimerJitterMs)
{
	PaUint32 frames = userFramesPerBuffer + max( userFramesPerBuffer, (PaUint32)(suggestedLatency * sampleRate) );
    frames += (PaUint32)((sampleRate * 0.001) * TimerJitterMs);
	return frames;
}

// ------------------------------------------------------------------------------------------
static void _RecalculateBuffersCount(PaWasapiSubStream *sub, UINT32 userFramesPerBuffer, UINT32 framesPerLatency, BOOL fullDuplex)
{
	// Count buffers (must be at least 1)
	sub->buffers = (userFramesPerBuffer ? framesPerLatency / userFramesPerBuffer : 0);
	if (sub->buffers == 0)
		sub->buffers = 1;

	// Determine amount of buffers used:
	// - Full-duplex mode will lead to period difference, thus only 1.
	// - Input mode, only 1, as WASAPI allows extraction of only 1 packet.
	// - For Shared mode we use double buffering.
	if ((sub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE) || fullDuplex)
	{
		// Exclusive mode does not allow >1 buffers be used for Event interface, e.g. GetBuffer
		// call must acquire max buffer size and it all must be processed.
		if (sub->streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)
			sub->userBufferAndHostMatch = 1;

		// Use paUtilBoundedHostBufferSize because exclusive mode will starve and produce
		// bad quality of audio
		sub->buffers = 1;
	}
}

// ------------------------------------------------------------------------------------------
static void _CalculateAlignedPeriod(PaWasapiSubStream *pSub, UINT32 *nFramesPerLatency,
									ALIGN_FUNC pAlignFunc)
{
	// Align frames to HD Audio packet size of 128 bytes for Exclusive mode only.
	// Not aligning on Windows Vista will cause Event timeout, although Windows 7 will
	// return AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED error to realign buffer. Aligning is necessary
	// for Exclusive mode only! when audio data is feeded directly to hardware.
	if (pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE)
	{
		(*nFramesPerLatency) = AlignFramesPerBuffer((*nFramesPerLatency),
			pSub->wavex.Format.nSamplesPerSec, pSub->wavex.Format.nBlockAlign, pAlignFunc);
	}

	// Calculate period
	pSub->period = MakeHnsPeriod((*nFramesPerLatency), pSub->wavex.Format.nSamplesPerSec);
}

// ------------------------------------------------------------------------------------------
static HRESULT CreateAudioClient(PaWasapiStream *pStream, PaWasapiSubStream *pSub, BOOL output, PaError *pa_error)
{
	PaError error;
    HRESULT hr;

	const PaWasapiDeviceInfo *pInfo  = pSub->params.device_info;
	const PaStreamParameters *params = &pSub->params.stream_params;
	UINT32 framesPerLatency          = pSub->params.frames_per_buffer;
	double sampleRate                = pSub->params.sample_rate;
	BOOL blocking                    = pSub->params.blocking;
	BOOL fullDuplex                  = pSub->params.full_duplex;

	const UINT32 userFramesPerBuffer = framesPerLatency;
    IAudioClient *audioClient	     = NULL;

	// Validate parameters
    if (!pSub || !pInfo || !params)
        return E_POINTER;
	if ((UINT32)sampleRate == 0)
        return E_INVALIDARG;

    // Get the audio client
    hr = IMMDevice_Activate(pInfo->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&audioClient);
	if (hr != S_OK)
	{
		LogHostError(hr);
		goto done;
	}

	// Get closest format
	if ((error = GetClosestFormat(audioClient, sampleRate, params, pSub->shareMode, &pSub->wavex, output)) != paFormatIsSupported)
	{
		if (pa_error)
			(*pa_error) = error;

		LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT);
		goto done; // fail, format not supported
	}

	// Check for Mono <<>> Stereo workaround
	if ((params->channelCount == 1) && (pSub->wavex.Format.nChannels == 2))
	{
		/*if (blocking)
		{
			LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT);
			goto done; // fail, blocking mode not supported
		}*/

		// select mixer
		pSub->monoMixer = _GetMonoToStereoMixer(WaveToPaFormat(&pSub->wavex), (pInfo->flow == eRender ? MIX_DIR__1TO2 : MIX_DIR__2TO1_L));
		if (pSub->monoMixer == NULL)
		{
			LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT);
			goto done; // fail, no mixer for format
		}
	}

#if 0
	// Add suggestd latency
	framesPerLatency += MakeFramesFromHns(SecondsTonano100(params->suggestedLatency), pSub->wavex.Format.nSamplesPerSec);
#else
	// Calculate host buffer size
	if ((pSub->shareMode != AUDCLNT_SHAREMODE_EXCLUSIVE) &&
		(!pSub->streamFlags || ((pSub->streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) == 0)))
	{
		framesPerLatency = PaUtil_GetFramesPerHostBuffer(userFramesPerBuffer,
			params->suggestedLatency, pSub->wavex.Format.nSamplesPerSec, 0/*,
			(pSub->streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK ? 0 : 1)*/);
	}
	else
	{
		REFERENCE_TIME overall;

		// Work 1:1 with user buffer (only polling allows to use >1)
		framesPerLatency += MakeFramesFromHns(SecondsTonano100(params->suggestedLatency), pSub->wavex.Format.nSamplesPerSec);

		// Use Polling if overall latency is > 5ms as it allows to use 100% CPU in a callback,
		// or user specified latency parameter
		overall = MakeHnsPeriod(framesPerLatency, pSub->wavex.Format.nSamplesPerSec);
		if ((overall >= (106667*2)/*21.33ms*/) || ((INT32)(params->suggestedLatency*100000.0) != 0/*0.01 msec granularity*/))
		{
			framesPerLatency = PaUtil_GetFramesPerHostBuffer(userFramesPerBuffer,
				params->suggestedLatency, pSub->wavex.Format.nSamplesPerSec, 0/*,
				(streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK ? 0 : 1)*/);

			// Use Polling interface
			pSub->streamFlags &= ~AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
			PRINT(("WASAPI: CreateAudioClient: forcing POLL mode\n"));
		}
	}
#endif

	// For full-duplex output resize buffer to be the same as for input
	if (output && fullDuplex)
		framesPerLatency = pStream->in.framesPerHostCallback;

	// Avoid 0 frames
	if (framesPerLatency == 0)
		framesPerLatency = MakeFramesFromHns(pInfo->DefaultDevicePeriod, pSub->wavex.Format.nSamplesPerSec);

	//! Exclusive Input stream renders data in 6 packets, we must set then the size of
	//! single packet, total buffer size, e.g. required latency will be PacketSize * 6
	if (!output && (pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE))
	{
		framesPerLatency /= WASAPI_PACKETS_PER_INPUT_BUFFER;
	}

	// Calculate aligned period
	_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_BWD);

	/*! Enforce min/max period for device in Shared mode to avoid bad audio quality.
        Avoid doing so for Exclusive mode as alignment will suffer.
	*/
	if (pSub->shareMode == AUDCLNT_SHAREMODE_SHARED)
	{
		if (pSub->period < pInfo->DefaultDevicePeriod)
		{
			pSub->period = pInfo->DefaultDevicePeriod;
			// Recalculate aligned period
			framesPerLatency = MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec);
			_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_BWD);
		}
	}
	else
	{
		if (pSub->period < pInfo->MinimumDevicePeriod)
		{
			pSub->period = pInfo->MinimumDevicePeriod;
			// Recalculate aligned period
			framesPerLatency = MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec);
			_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_FWD);
		}
	}

	/*! Windows 7 does not allow to set latency lower than minimal device period and will
	    return error: AUDCLNT_E_INVALID_DEVICE_PERIOD. Under Vista we enforce the same behavior
	    manually for unified behavior on all platforms.
	*/
	{
		/*!	AUDCLNT_E_BUFFER_SIZE_ERROR: Applies to Windows 7 and later.
			Indicates that the buffer duration value requested by an exclusive-mode client is
			out of range. The requested duration value for pull mode must not be greater than
			500 milliseconds; for push mode the duration value must not be greater than 2 seconds.
		*/
		if (pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE)
		{
			static const REFERENCE_TIME MAX_BUFFER_EVENT_DURATION = 500  * 10000;
			static const REFERENCE_TIME MAX_BUFFER_POLL_DURATION  = 2000 * 10000;

			if (pSub->streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)	// pull mode, max 500ms
			{
				if (pSub->period > MAX_BUFFER_EVENT_DURATION)
				{
					pSub->period = MAX_BUFFER_EVENT_DURATION;
					// Recalculate aligned period
					framesPerLatency = MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec);
					_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_BWD);
				}
			}
			else														// push mode, max 2000ms
			{
				if (pSub->period > MAX_BUFFER_POLL_DURATION)
				{
					pSub->period = MAX_BUFFER_POLL_DURATION;
					// Recalculate aligned period
					framesPerLatency = MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec);
					_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_BWD);
				}
			}
		}
	}

	// Open the stream and associate it with an audio session
    hr = IAudioClient_Initialize(audioClient,
        pSub->shareMode,
        pSub->streamFlags,
		pSub->period,
		(pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSub->period : 0),
		&pSub->wavex.Format,
        NULL);

	/*! WASAPI is tricky on large device buffer, sometimes 2000ms can be allocated sometimes
	    less. There is no known guaranteed level thus we make subsequent tries by decreasing
		buffer by 100ms per try.
	*/
	while ((hr == E_OUTOFMEMORY) && (pSub->period > (100 * 10000)))
	{
		PRINT(("WASAPI: CreateAudioClient: decreasing buffer size to %d milliseconds\n", (pSub->period / 10000)));

		// Decrease by 100ms and try again
		pSub->period -= (100 * 10000);

		// Recalculate aligned period
		framesPerLatency = MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec);
		_CalculateAlignedPeriod(pSub, &framesPerLatency, ALIGN_BWD);

        // Release the previous allocations
        SAFE_RELEASE(audioClient);

        // Create a new audio client
        hr = IMMDevice_Activate(pInfo->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audioClient);
    	if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

		// Open the stream and associate it with an audio session
		hr = IAudioClient_Initialize(audioClient,
			pSub->shareMode,
			pSub->streamFlags,
			pSub->period,
			(pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSub->period : 0),
			&pSub->wavex.Format,
			NULL);
	}

	/*! WASAPI buffer size failure. Fallback to using default size.
	*/
	if (hr == AUDCLNT_E_BUFFER_SIZE_ERROR)
	{
		// Use default
		pSub->period = pInfo->DefaultDevicePeriod;

		PRINT(("WASAPI: CreateAudioClient: correcting buffer size to device default\n"));

        // Release the previous allocations
        SAFE_RELEASE(audioClient);

        // Create a new audio client
        hr = IMMDevice_Activate(pInfo->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audioClient);
    	if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

		// Open the stream and associate it with an audio session
		hr = IAudioClient_Initialize(audioClient,
			pSub->shareMode,
			pSub->streamFlags,
			pSub->period,
			(pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSub->period : 0),
			&pSub->wavex.Format,
			NULL);
	}

    /*! If the requested buffer size is not aligned. Can be triggered by Windows 7 and up.
	    Should not be be triggered ever as we do align buffers always with _CalculateAlignedPeriod.
	*/
    if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
    {
		UINT32 frames = 0;

        // Get the next aligned frame
        hr = IAudioClient_GetBufferSize(audioClient, &frames);
		if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

		PRINT(("WASAPI: CreateAudioClient: aligning buffer size to % frames\n", frames));

        // Release the previous allocations
        SAFE_RELEASE(audioClient);

        // Create a new audio client
        hr = IMMDevice_Activate(pInfo->device, &pa_IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audioClient);
    	if (hr != S_OK)
		{
			LogHostError(hr);
			goto done;
		}

		// Get closest format
		if ((error = GetClosestFormat(audioClient, sampleRate, params, pSub->shareMode, &pSub->wavex, output)) != paFormatIsSupported)
		{
			if (pa_error)
				(*pa_error) = error;

			LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT); // fail, format not supported
			goto done;
		}

		// Check for Mono >> Stereo workaround
		if ((params->channelCount == 1) && (pSub->wavex.Format.nChannels == 2))
		{
			/*if (blocking)
			{
				LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT);
				goto done; // fail, blocking mode not supported
			}*/

			// Select mixer
			pSub->monoMixer = _GetMonoToStereoMixer(WaveToPaFormat(&pSub->wavex), (pInfo->flow == eRender ? MIX_DIR__1TO2 : MIX_DIR__2TO1_L));
			if (pSub->monoMixer == NULL)
			{
				LogHostError(hr = AUDCLNT_E_UNSUPPORTED_FORMAT);
				goto done; // fail, no mixer for format
			}
		}

		// Calculate period
		pSub->period = MakeHnsPeriod(frames, pSub->wavex.Format.nSamplesPerSec);

        // Open the stream and associate it with an audio session
        hr = IAudioClient_Initialize(audioClient,
            pSub->shareMode,
            pSub->streamFlags,
			pSub->period,
			(pSub->shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? pSub->period : 0),
            &pSub->wavex.Format,
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

    // Set client
	pSub->clientParent = audioClient;
    IAudioClient_AddRef(pSub->clientParent);

	// Recalculate buffers count
	_RecalculateBuffersCount(pSub,
		userFramesPerBuffer,
		MakeFramesFromHns(pSub->period, pSub->wavex.Format.nSamplesPerSec),
		fullDuplex);

done:

    // Clean up
    SAFE_RELEASE(audioClient);
    return hr;
}

// ------------------------------------------------------------------------------------------
static PaError ActivateAudioClientOutput(PaWasapiStream *stream)
{
	HRESULT hr;
	PaError result;

	UINT32 maxBufferSize   = 0;
	PaTime buffer_latency  = 0;
	UINT32 framesPerBuffer = stream->out.params.frames_per_buffer;

	// Create Audio client
	hr = CreateAudioClient(stream, &stream->out, TRUE, &result);
    if (hr != S_OK)
	{
		LogPaError(result = paInvalidDevice);
		goto error;
    }
	LogWAVEFORMATEXTENSIBLE(&stream->out.wavex);

	// Activate volume
	stream->outVol = NULL;
    /*hr = info->device->Activate(
        __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL,
        (void**)&stream->outVol);
    if (hr != S_OK)
        return paInvalidDevice;*/

    // Get max possible buffer size to check if it is not less than that we request
    hr = IAudioClient_GetBufferSize(stream->out.clientParent, &maxBufferSize);
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
    hr = IAudioClient_GetStreamLatency(stream->out.clientParent, &stream->out.deviceLatency);
    if (hr != S_OK)
	{
		LogHostError(hr);
		LogPaError(result = paInvalidDevice);
		goto error;
	}
	//stream->out.latencySeconds = nano100ToSeconds(stream->out.deviceLatency);

    // Number of frames that are required at each period
	stream->out.framesPerHostCallback = maxBufferSize;

	// Calculate frames per single buffer, if buffers > 1 then always framesPerBuffer
	stream->out.framesPerBuffer =
		(stream->out.userBufferAndHostMatch ? stream->out.framesPerHostCallback : framesPerBuffer);

	// Calculate buffer latency
	buffer_latency = (PaTime)maxBufferSize / stream->out.wavex.Format.nSamplesPerSec;

	// Append buffer latency to interface latency in shared mode (see GetStreamLatency notes)
	stream->out.latencySeconds = buffer_latency;

	PRINT(("WASAPI::OpenStream(output): framesPerUser[ %d ] framesPerHost[ %d ] latency[ %.02fms ] exclusive[ %s ] wow64_fix[ %s ] mode[ %s ]\n", (UINT32)framesPerBuffer, (UINT32)stream->out.framesPerHostCallback, (float)(stream->out.latencySeconds*1000.0f), (stream->out.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? "YES" : "NO"), (stream->out.params.wow64_workaround ? "YES" : "NO"), (stream->out.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK ? "EVENT" : "POLL")));

	return paNoError;

error:

	return result;
}

// ------------------------------------------------------------------------------------------
static PaError ActivateAudioClientInput(PaWasapiStream *stream)
{
	HRESULT hr;
	PaError result;

	UINT32 maxBufferSize   = 0;
	PaTime buffer_latency  = 0;
	UINT32 framesPerBuffer = stream->in.params.frames_per_buffer;

	// Create Audio client
	hr = CreateAudioClient(stream, &stream->in, FALSE, &result);
    if (hr != S_OK)
	{
		LogPaError(result = paInvalidDevice);
		goto error;
    }
	LogWAVEFORMATEXTENSIBLE(&stream->in.wavex);

	// Create volume mgr
	stream->inVol = NULL;
    /*hr = info->device->Activate(
        __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL,
        (void**)&stream->inVol);
    if (hr != S_OK)
        return paInvalidDevice;*/

    // Get max possible buffer size to check if it is not less than that we request
    hr = IAudioClient_GetBufferSize(stream->in.clientParent, &maxBufferSize);
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
    hr = IAudioClient_GetStreamLatency(stream->in.clientParent, &stream->in.deviceLatency);
    if (hr != S_OK)
	{
		LogHostError(hr);
		LogPaError(result = paInvalidDevice);
		goto error;
	}
	//stream->in.latencySeconds = nano100ToSeconds(stream->in.deviceLatency);

    // Number of frames that are required at each period
	stream->in.framesPerHostCallback = maxBufferSize;

	// Calculate frames per single buffer, if buffers > 1 then always framesPerBuffer
	stream->in.framesPerBuffer =
		(stream->in.userBufferAndHostMatch ? stream->in.framesPerHostCallback : framesPerBuffer);

	// Calculate buffer latency
	buffer_latency = (PaTime)maxBufferSize / stream->in.wavex.Format.nSamplesPerSec;

	// Append buffer latency to interface latency in shared mode (see GetStreamLatency notes)
	stream->in.latencySeconds = buffer_latency;

	PRINT(("WASAPI::OpenStream(input): framesPerUser[ %d ] framesPerHost[ %d ] latency[ %.02fms ] exclusive[ %s ] wow64_fix[ %s ] mode[ %s ]\n", (UINT32)framesPerBuffer, (UINT32)stream->in.framesPerHostCallback, (float)(stream->in.latencySeconds*1000.0f), (stream->in.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? "YES" : "NO"), (stream->in.params.wow64_workaround ? "YES" : "NO"), (stream->in.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK ? "EVENT" : "POLL")));

	return paNoError;

error:

	return result;
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
	ULONG framesPerHostCallback;
	PaUtilHostBufferSizeMode bufferMode;
	const BOOL fullDuplex = ((inputParameters != NULL) && (outputParameters != NULL));

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
		stream->in.streamFlags = (stream->in.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0);
		if (paWasapi->useWOW64Workaround)
			stream->in.streamFlags = 0; // polling interface
		else
		if (streamCallback == NULL)
			stream->in.streamFlags = 0; // polling interface
		else
		if ((inputStreamInfo != NULL) && (inputStreamInfo->flags & paWinWasapiPolling))
			stream->in.streamFlags = 0; // polling interface
		else
		if (fullDuplex)
			stream->in.streamFlags = 0; // polling interface is implemented for full-duplex mode also

		// Fill parameters for Audio Client creation
		stream->in.params.device_info       = info;
		stream->in.params.stream_params     = (*inputParameters);
		if (inputStreamInfo != NULL)
		{
			stream->in.params.wasapi_params = (*inputStreamInfo);
			stream->in.params.stream_params.hostApiSpecificStreamInfo = &stream->in.params.wasapi_params;
		}
		stream->in.params.frames_per_buffer = framesPerBuffer;
		stream->in.params.sample_rate       = sampleRate;
		stream->in.params.blocking          = (streamCallback == NULL);
		stream->in.params.full_duplex       = fullDuplex;
		stream->in.params.wow64_workaround  = paWasapi->useWOW64Workaround;

		// Create and activate audio client
		hr = ActivateAudioClientInput(stream);
        if (hr != S_OK)
		{
			LogPaError(result = paInvalidDevice);
			goto error;
        }

		// Get closest format
        hostInputSampleFormat = PaUtil_SelectClosestAvailableFormat( WaveToPaFormat(&stream->in.wavex), inputSampleFormat );

        // Set user-side custom host processor
        if ((inputStreamInfo != NULL) &&
            (inputStreamInfo->flags & paWinWasapiRedirectHostProcessor))
        {
            stream->hostProcessOverrideInput.processor = inputStreamInfo->hostProcessorInput;
            stream->hostProcessOverrideInput.userData = userData;
        }

		// Only get IAudioCaptureClient input once here instead of getting it at multiple places based on the use
		hr = IAudioClient_GetService(stream->in.clientParent, &pa_IID_IAudioCaptureClient, (void **)&stream->captureClientParent);
		if (hr != S_OK)
		{
			LogHostError(hr);
			LogPaError(result = paUnanticipatedHostError);
			goto error;
		}

		// Create ring buffer for blocking mode (It is needed because we fetch Input packets, not frames,
		// and thus we have to save partial packet if such remains unread)
		if (stream->in.params.blocking == TRUE)
		{
			UINT32 bufferFrames = ALIGN_NEXT_POW2((stream->in.framesPerHostCallback / WASAPI_PACKETS_PER_INPUT_BUFFER) * 2);
			UINT32 frameSize    = stream->in.wavex.Format.nBlockAlign;

			// buffer
			if ((stream->in.tailBuffer = PaUtil_AllocateMemory(sizeof(PaUtilRingBuffer))) == NULL)
			{
				LogPaError(result = paInsufficientMemory);
				goto error;
			}
			memset(stream->in.tailBuffer, 0, sizeof(PaUtilRingBuffer));

			// buffer memory region
			stream->in.tailBufferMemory = PaUtil_AllocateMemory(frameSize * bufferFrames);
			if (stream->in.tailBufferMemory == NULL)
			{
				LogPaError(result = paInsufficientMemory);
				goto error;
			}

			// initialize
			if (PaUtil_InitializeRingBuffer(stream->in.tailBuffer, frameSize, bufferFrames,	stream->in.tailBufferMemory) != 0)
			{
				LogPaError(result = paInternalError);
				goto error;
			}
		}
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
		stream->out.streamFlags = (stream->out.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0);
		if (paWasapi->useWOW64Workaround)
			stream->out.streamFlags = 0; // polling interface
		else
		if (streamCallback == NULL)
			stream->out.streamFlags = 0; // polling interface
		else
		if ((outputStreamInfo != NULL) && (outputStreamInfo->flags & paWinWasapiPolling))
			stream->out.streamFlags = 0; // polling interface
		else
		if (fullDuplex)
			stream->out.streamFlags = 0; // polling interface is implemented for full-duplex mode also

		// Fill parameters for Audio Client creation
		stream->out.params.device_info       = info;
		stream->out.params.stream_params     = (*outputParameters);
		if (inputStreamInfo != NULL)
		{
			stream->out.params.wasapi_params = (*outputStreamInfo);
			stream->out.params.stream_params.hostApiSpecificStreamInfo = &stream->out.params.wasapi_params;
		}
		stream->out.params.frames_per_buffer = framesPerBuffer;
		stream->out.params.sample_rate       = sampleRate;
		stream->out.params.blocking          = (streamCallback == NULL);
		stream->out.params.full_duplex       = fullDuplex;
		stream->out.params.wow64_workaround  = paWasapi->useWOW64Workaround;

		// Create and activate audio client
		hr = ActivateAudioClientOutput(stream);
        if (hr != S_OK)
		{
			LogPaError(result = paInvalidDevice);
			goto error;
        }

		// Get closest format
        hostOutputSampleFormat = PaUtil_SelectClosestAvailableFormat( WaveToPaFormat(&stream->out.wavex), outputSampleFormat );

        // Set user-side custom host processor
        if ((outputStreamInfo != NULL) &&
            (outputStreamInfo->flags & paWinWasapiRedirectHostProcessor))
        {
            stream->hostProcessOverrideOutput.processor = outputStreamInfo->hostProcessorOutput;
            stream->hostProcessOverrideOutput.userData = userData;
        }

		// Only get IAudioCaptureClient output once here instead of getting it at multiple places based on the use
		hr = IAudioClient_GetService(stream->out.clientParent, &pa_IID_IAudioRenderClient, (void **)&stream->renderClientParent);
		if (hr != S_OK)
		{
			LogHostError(hr);
			LogPaError(result = paUnanticipatedHostError);
			goto error;
		}
	}
    else
    {
        outputChannelCount = 0;
        outputSampleFormat = hostOutputSampleFormat = paInt16; /* Surpress 'uninitialized var' warnings. */
    }

	// log full-duplex
	if (fullDuplex)
		PRINT(("WASAPI::OpenStream: full-duplex mode\n"));

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
		// serious problem #1 - No, Not a problem, especially concerning Exclusive mode.
		// Input device in exclusive mode somehow is getting large buffer always, thus we
		// adjust Output latency to reflect it, thus period will differ but playback will be
		// normal.
		/*if (stream->in.period != stream->out.period)
		{
			PRINT(("WASAPI: OpenStream: period discrepancy\n"));
			LogPaError(result = paBadIODeviceCombination);
			goto error;
		}*/

		// serious problem #2 - No, Not a problem, as framesPerHostCallback take into account
		// sample size while it is not a problem for PA full-duplex, we must care of
		// preriod only!
		/*if (stream->out.framesPerHostCallback != stream->in.framesPerHostCallback)
		{
			PRINT(("WASAPI: OpenStream: framesPerHostCallback discrepancy\n"));
			goto error;
		}*/
	}

	// Calculate frames per host for processor
	framesPerHostCallback = (outputParameters ? stream->out.framesPerBuffer : stream->in.framesPerBuffer);

	// Choose correct mode of buffer processing:
	// Exclusive/Shared non paWinWasapiPolling mode: paUtilFixedHostBufferSize - always fixed
	// Exclusive/Shared paWinWasapiPolling mode: paUtilBoundedHostBufferSize - may vary for Exclusive or Full-duplex
	bufferMode = paUtilFixedHostBufferSize;
	if (inputParameters) // !!! WASAPI IAudioCaptureClient::GetBuffer extracts not number of frames but 1 packet, thus we always must adapt
		bufferMode = paUtilBoundedHostBufferSize;
	else
	if (outputParameters)
	{
		if ((stream->out.buffers == 1) &&
			(!stream->out.streamFlags || ((stream->out.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) == 0)))
			bufferMode = paUtilBoundedHostBufferSize;
	}
	stream->bufferMode = bufferMode;

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
            ((double)PaUtil_GetBufferProcessorInputLatency(&stream->bufferProcessor) / sampleRate)
			+ ((inputParameters)?stream->in.latencySeconds : 0);

	// Set Output latency
    stream->streamRepresentation.streamInfo.outputLatency =
            ((double)PaUtil_GetBufferProcessorOutputLatency(&stream->bufferProcessor) / sampleRate)
			+ ((outputParameters)?stream->out.latencySeconds : 0);

	// Set SR
    stream->streamRepresentation.streamInfo.sampleRate = sampleRate;

    (*s) = (PaStream *)stream;
    return result;

error:

    if (stream != NULL)
		CloseStream(stream);

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
		result = AbortStream(s);
	}

    SAFE_RELEASE(stream->captureClientParent);
    SAFE_RELEASE(stream->renderClientParent);
    SAFE_RELEASE(stream->out.clientParent);
    SAFE_RELEASE(stream->in.clientParent);
	SAFE_RELEASE(stream->inVol);
	SAFE_RELEASE(stream->outVol);

	CloseHandle(stream->event[S_INPUT]);
	CloseHandle(stream->event[S_OUTPUT]);

	_StreamCleanup(stream);

	PaWasapi_FreeMemory(stream->in.monoBuffer);
	PaWasapi_FreeMemory(stream->out.monoBuffer);

	PaUtil_FreeMemory(stream->in.tailBuffer);
	PaUtil_FreeMemory(stream->in.tailBufferMemory);

	PaUtil_FreeMemory(stream->out.tailBuffer);
	PaUtil_FreeMemory(stream->out.tailBufferMemory);

    PaUtil_TerminateBufferProcessor(&stream->bufferProcessor);
    PaUtil_TerminateStreamRepresentation(&stream->streamRepresentation);
    PaUtil_FreeMemory(stream);

    return result;
}

// ------------------------------------------------------------------------------------------
HRESULT UnmarshalSubStreamComPointers(PaWasapiSubStream *substream) 
{
	HRESULT hResult = S_OK;
	HRESULT hFirstBadResult = S_OK;
	substream->clientProc = NULL;

	// IAudioClient
	hResult = CoGetInterfaceAndReleaseStream(substream->clientStream, &pa_IID_IAudioClient, (LPVOID*)&substream->clientProc);
	substream->clientStream = NULL;
	if (hResult != S_OK) 
	{
		hFirstBadResult = (hFirstBadResult == S_OK) ? hResult : hFirstBadResult;
	}

	return hFirstBadResult;
}

// ------------------------------------------------------------------------------------------
HRESULT UnmarshalStreamComPointers(PaWasapiStream *stream) 
{
	HRESULT hResult = S_OK;
	HRESULT hFirstBadResult = S_OK;
	stream->captureClient = NULL;
	stream->renderClient = NULL;
	stream->in.clientProc = NULL;
	stream->out.clientProc = NULL;

	if (NULL != stream->in.clientParent) 
	{
		// SubStream pointers
		hResult = UnmarshalSubStreamComPointers(&stream->in);
		if (hResult != S_OK) 
		{
			hFirstBadResult = (hFirstBadResult == S_OK) ? hResult : hFirstBadResult;
		}

		// IAudioCaptureClient
		hResult = CoGetInterfaceAndReleaseStream(stream->captureClientStream, &pa_IID_IAudioCaptureClient, (LPVOID*)&stream->captureClient);
		stream->captureClientStream = NULL;
		if (hResult != S_OK) 
		{
			hFirstBadResult = (hFirstBadResult == S_OK) ? hResult : hFirstBadResult;
		}
	}

	if (NULL != stream->out.clientParent) 
	{
		// SubStream pointers
		hResult = UnmarshalSubStreamComPointers(&stream->out);
		if (hResult != S_OK) 
		{
			hFirstBadResult = (hFirstBadResult == S_OK) ? hResult : hFirstBadResult;
		}

		// IAudioRenderClient
		hResult = CoGetInterfaceAndReleaseStream(stream->renderClientStream, &pa_IID_IAudioRenderClient, (LPVOID*)&stream->renderClient);
		stream->renderClientStream = NULL;
		if (hResult != S_OK) 
		{
			hFirstBadResult = (hFirstBadResult == S_OK) ? hResult : hFirstBadResult;
		}
	}

	return hFirstBadResult;
}

// -----------------------------------------------------------------------------------------
void ReleaseUnmarshaledSubComPointers(PaWasapiSubStream *substream) 
{
	SAFE_RELEASE(substream->clientProc);
}

// -----------------------------------------------------------------------------------------
void ReleaseUnmarshaledComPointers(PaWasapiStream *stream) 
{
	// Release AudioClient services first
	SAFE_RELEASE(stream->captureClient);
	SAFE_RELEASE(stream->renderClient);

	// Release AudioClients
	ReleaseUnmarshaledSubComPointers(&stream->in);
	ReleaseUnmarshaledSubComPointers(&stream->out);
}

// ------------------------------------------------------------------------------------------
HRESULT MarshalSubStreamComPointers(PaWasapiSubStream *substream) 
{
	HRESULT hResult;
	substream->clientStream = NULL;

	// IAudioClient
	hResult = CoMarshalInterThreadInterfaceInStream(&pa_IID_IAudioClient, (LPUNKNOWN)substream->clientParent, &substream->clientStream);
	if (hResult != S_OK)
		goto marshal_sub_error;

	return hResult;

	// If marshaling error occurred, make sure to release everything.
marshal_sub_error:

	UnmarshalSubStreamComPointers(substream);
	ReleaseUnmarshaledSubComPointers(substream);
	return hResult;
}

// ------------------------------------------------------------------------------------------
HRESULT MarshalStreamComPointers(PaWasapiStream *stream) 
{
	HRESULT hResult = S_OK;
	stream->captureClientStream = NULL;
	stream->in.clientStream = NULL;
	stream->renderClientStream = NULL;
	stream->out.clientStream = NULL;

	if (NULL != stream->in.clientParent) 
	{
		// SubStream pointers
		hResult = MarshalSubStreamComPointers(&stream->in);
		if (hResult != S_OK) 
			goto marshal_error;

		// IAudioCaptureClient
		hResult = CoMarshalInterThreadInterfaceInStream(&pa_IID_IAudioCaptureClient, (LPUNKNOWN)stream->captureClientParent, &stream->captureClientStream);
		if (hResult != S_OK) 
			goto marshal_error;
	}

	if (NULL != stream->out.clientParent) 
	{
		// SubStream pointers
		hResult = MarshalSubStreamComPointers(&stream->out);
		if (hResult != S_OK) 
			goto marshal_error;

		// IAudioRenderClient
		hResult = CoMarshalInterThreadInterfaceInStream(&pa_IID_IAudioRenderClient, (LPUNKNOWN)stream->renderClientParent, &stream->renderClientStream);
		if (hResult != S_OK) 
			goto marshal_error;
	}

	return hResult;

	// If marshaling error occurred, make sure to release everything.
marshal_error:

	UnmarshalStreamComPointers(stream);
	ReleaseUnmarshaledComPointers(stream);
	return hResult;
}

// ------------------------------------------------------------------------------------------
static PaError StartStream( PaStream *s )
{
	HRESULT hr;
    PaWasapiStream *stream = (PaWasapiStream*)s;
	PaError result = paNoError;

	// check if stream is active already
	if (IsStreamActive(s))
		return paStreamIsNotStopped;

    PaUtil_ResetBufferProcessor(&stream->bufferProcessor);

	// Cleanup handles (may be necessary if stream was stopped by itself due to error)
	_StreamCleanup(stream);

	// Create close event
	if ((stream->hCloseRequest = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) 
	{
		result = paInsufficientMemory;
		goto start_error;
	}

	// Create thread
	if (!stream->bBlocking)
	{
		// Create thread events
		stream->hThreadStart = CreateEvent(NULL, TRUE, FALSE, NULL);
		stream->hThreadExit  = CreateEvent(NULL, TRUE, FALSE, NULL);
		if ((stream->hThreadStart == NULL) || (stream->hThreadExit == NULL))
		{
			result = paInsufficientMemory;
			goto start_error;
		}

		// Marshal WASAPI interface pointers for safe use in thread created below.
		if ((hr = MarshalStreamComPointers(stream)) != S_OK) 
		{
			PRINT(("Failed marshaling stream COM pointers."));
			result = paUnanticipatedHostError;
			goto nonblocking_start_error;
		}

		if ((stream->in.clientParent  && (stream->in.streamFlags  & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)) ||
			(stream->out.clientParent && (stream->out.streamFlags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)))
		{
			if ((stream->hThread = CREATE_THREAD(ProcThreadEvent)) == NULL) 
			{
				PRINT(("Failed creating thread: ProcThreadEvent."));
				result = paUnanticipatedHostError;
				goto nonblocking_start_error;
			}
		}
		else
		{
			if ((stream->hThread = CREATE_THREAD(ProcThreadPoll)) == NULL) 
			{
				PRINT(("Failed creating thread: ProcThreadPoll."));
				result = paUnanticipatedHostError;
				goto nonblocking_start_error;
			}
		}

		// Wait for thread to start
		if (WaitForSingleObject(stream->hThreadStart, 60*1000) == WAIT_TIMEOUT) 
		{
			PRINT(("Failed starting thread: timeout."));
			result = paUnanticipatedHostError;
			goto nonblocking_start_error;
		}
	}
	else
	{
		// Create blocking operation events (non-signaled event means - blocking operation is pending)
		if (stream->out.clientParent != NULL) 
		{
			if ((stream->hBlockingOpStreamWR = CreateEvent(NULL, TRUE, TRUE, NULL)) == NULL) 
			{
				result = paInsufficientMemory;
				goto start_error;
			}
		}
		if (stream->in.clientParent != NULL) 
		{
			if ((stream->hBlockingOpStreamRD = CreateEvent(NULL, TRUE, TRUE, NULL)) == NULL) 
			{
				result = paInsufficientMemory;
				goto start_error;
			}
		}

		// Initialize event & start INPUT stream
		if (stream->in.clientParent != NULL)
		{
			if ((hr = IAudioClient_Start(stream->in.clientParent)) != S_OK)
			{
				LogHostError(hr);
				result = paUnanticipatedHostError;
				goto start_error;
			}
		}

		// Initialize event & start OUTPUT stream
		if (stream->out.clientParent != NULL)
		{
			// Start
			if ((hr = IAudioClient_Start(stream->out.clientParent)) != S_OK)
			{
				LogHostError(hr);
				result = paUnanticipatedHostError;
				goto start_error;
			}
		}

		// Set parent to working pointers to use shared functions.
		stream->captureClient  = stream->captureClientParent;
		stream->renderClient   = stream->renderClientParent;
		stream->in.clientProc  = stream->in.clientParent;
		stream->out.clientProc = stream->out.clientParent;

		// Signal: stream running.
		stream->running = TRUE;
	}

    return result;

nonblocking_start_error:

	// Set hThreadExit event to prevent blocking during cleanup
	SetEvent(stream->hThreadExit);
	UnmarshalStreamComPointers(stream);
	ReleaseUnmarshaledComPointers(stream);

start_error:

	StopStream(s);
	return result;
}

// ------------------------------------------------------------------------------------------
void _StreamFinish(PaWasapiStream *stream)
{
	// Issue command to thread to stop processing and wait for thread exit
	if (!stream->bBlocking)
	{
		SignalObjectAndWait(stream->hCloseRequest, stream->hThreadExit, INFINITE, FALSE);
	}
	else
	// Blocking mode does not own thread
	{
		// Signal close event and wait for each of 2 blocking operations to complete
		if (stream->out.clientParent)
			SignalObjectAndWait(stream->hCloseRequest, stream->hBlockingOpStreamWR, INFINITE, TRUE);
		if (stream->out.clientParent)
			SignalObjectAndWait(stream->hCloseRequest, stream->hBlockingOpStreamRD, INFINITE, TRUE);

		// Process stop
		_StreamOnStop(stream);
	}

	// Cleanup handles
	_StreamCleanup(stream);

    stream->running = FALSE;
}

// ------------------------------------------------------------------------------------------
void _StreamCleanup(PaWasapiStream *stream)
{
	// Close thread handles to allow restart
	SAFE_CLOSE(stream->hThread);
	SAFE_CLOSE(stream->hThreadStart);
	SAFE_CLOSE(stream->hThreadExit);
	SAFE_CLOSE(stream->hCloseRequest);
	SAFE_CLOSE(stream->hBlockingOpStreamRD);
	SAFE_CLOSE(stream->hBlockingOpStreamWR);
}

// ------------------------------------------------------------------------------------------
static PaError StopStream( PaStream *s )
{
	// Finish stream
	_StreamFinish((PaWasapiStream *)s);
    return paNoError;
}

// ------------------------------------------------------------------------------------------
static PaError AbortStream( PaStream *s )
{
	// Finish stream
	_StreamFinish((PaWasapiStream *)s);
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

    return PaUtil_GetTime();
}

// ------------------------------------------------------------------------------------------
static double GetStreamCpuLoad( PaStream* s )
{
	return PaUtil_GetCpuLoad(&((PaWasapiStream *)s)->cpuLoadMeasurer);
}

// ------------------------------------------------------------------------------------------
static PaError ReadStream( PaStream* s, void *_buffer, unsigned long frames )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	HRESULT hr = S_OK;
	BYTE *user_buffer = (BYTE *)_buffer;
	BYTE *wasapi_buffer = NULL;
	DWORD flags = 0;
	UINT32 i, available, sleep = 0;
	unsigned long processed;
	ThreadIdleScheduler sched;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->captureClient == NULL)
		return paBadStreamPtr;

	// Notify blocking op has begun
	ResetEvent(stream->hBlockingOpStreamRD);

	// Use thread scheduling for 500 microseconds (emulated) when wait time for frames is less than
	// 1 milliseconds, emulation helps to normalize CPU consumption and avoids too busy waiting
	ThreadIdleScheduler_Setup(&sched, 1, 250/* microseconds */);

    // Make a local copy of the user buffer pointer(s), this is necessary
	// because PaUtil_CopyOutput() advances these pointers every time it is called
    if (!stream->bufferProcessor.userInputIsInterleaved)
    {
		user_buffer = (BYTE *)alloca(sizeof(BYTE *) * stream->bufferProcessor.inputChannelCount);
        if (user_buffer == NULL)
            return paInsufficientMemory;

        for (i = 0; i < stream->bufferProcessor.inputChannelCount; ++i)
            ((BYTE **)user_buffer)[i] = ((BYTE **)_buffer)[i];
    }

	// Findout if there are tail frames, flush them all before reading hardware
	if ((available = PaUtil_GetRingBufferReadAvailable(stream->in.tailBuffer)) != 0)
	{
		ring_buffer_size_t buf1_size = 0, buf2_size = 0, read, desired;
		void *buf1 = NULL, *buf2 = NULL;

		// Limit desired to amount of requested frames
		desired = available;
		if (desired > frames)
			desired = frames;
		
		// Get pointers to read regions
		read = PaUtil_GetRingBufferReadRegions(stream->in.tailBuffer, desired, &buf1, &buf1_size, &buf2, &buf2_size);

		if (buf1 != NULL)
		{
			// Register available frames to processor
			PaUtil_SetInputFrameCount(&stream->bufferProcessor, buf1_size);

			// Register host buffer pointer to processor
			PaUtil_SetInterleavedInputChannels(&stream->bufferProcessor, 0, buf1, stream->bufferProcessor.inputChannelCount);

			// Copy user data to host buffer (with conversion if applicable)
			processed = PaUtil_CopyInput(&stream->bufferProcessor, (void **)&user_buffer, buf1_size);
			frames -= processed;
		}

		if (buf2 != NULL)
		{
			// Register available frames to processor
			PaUtil_SetInputFrameCount(&stream->bufferProcessor, buf2_size);

			// Register host buffer pointer to processor
			PaUtil_SetInterleavedInputChannels(&stream->bufferProcessor, 0, buf2, stream->bufferProcessor.inputChannelCount);

			// Copy user data to host buffer (with conversion if applicable)
			processed = PaUtil_CopyInput(&stream->bufferProcessor, (void **)&user_buffer, buf2_size);
			frames -= processed;
		}

		// Advance
		PaUtil_AdvanceRingBufferReadIndex(stream->in.tailBuffer, read);
	}

	// Read hardware
	while (frames != 0)
	{
		// Check if blocking call must be interrupted
		if (WaitForSingleObject(stream->hCloseRequest, sleep) != WAIT_TIMEOUT)
			break;

		// Get available frames (must be finding out available frames before call to IAudioCaptureClient_GetBuffer
		// othervise audio glitches will occur inExclusive mode as it seems that WASAPI has some scheduling/
		// processing problems when such busy polling with IAudioCaptureClient_GetBuffer occurs)
		if ((hr = _PollGetInputFramesAvailable(stream, &available)) != S_OK)
		{
			LogHostError(hr);
			return paUnanticipatedHostError;
		}

		// Wait for more frames to become available
		if (available == 0)
		{
			// Exclusive mode may require latency of 1 millisecond, thus we shall sleep
			// around 500 microseconds (emulated) to collect packets in time
			if (stream->in.shareMode != AUDCLNT_SHAREMODE_EXCLUSIVE)
			{
				UINT32 sleep_frames = (frames < stream->in.framesPerHostCallback ? frames : stream->in.framesPerHostCallback);

				sleep  = GetFramesSleepTime(sleep_frames, stream->in.wavex.Format.nSamplesPerSec);
				sleep /= 4; // wait only for 1/4 of the buffer

				// WASAPI input provides packets, thus expiring packet will result in bad audio
				// limit waiting time to 2 seconds (will always work for smallest buffer in Shared)
				if (sleep > 2)
					sleep = 2;

				// Avoid busy waiting, schedule next 1 millesecond wait
				if (sleep == 0)
					sleep = ThreadIdleScheduler_NextSleep(&sched);
			}
			else
			{
				if ((sleep = ThreadIdleScheduler_NextSleep(&sched)) != 0)
				{
					Sleep(sleep);
					sleep = 0;
				}
			}

			continue;
		}

		// Get the available data in the shared buffer.
		if ((hr = IAudioCaptureClient_GetBuffer(stream->captureClient, &wasapi_buffer, &available, &flags, NULL, NULL)) != S_OK)
		{
			// Buffer size is too small, waiting
			if (hr != AUDCLNT_S_BUFFER_EMPTY)
			{
				LogHostError(hr);
				goto end;
			}

			continue;
		}

		// Register available frames to processor
        PaUtil_SetInputFrameCount(&stream->bufferProcessor, available);

		// Register host buffer pointer to processor
        PaUtil_SetInterleavedInputChannels(&stream->bufferProcessor, 0, wasapi_buffer, stream->bufferProcessor.inputChannelCount);

		// Copy user data to host buffer (with conversion if applicable)
		processed = PaUtil_CopyInput(&stream->bufferProcessor, (void **)&user_buffer, frames);
		frames -= processed;

		// Save tail into buffer
		if ((frames == 0) && (available > processed))
		{
			UINT32 bytes_processed = processed * stream->in.wavex.Format.nBlockAlign;
			UINT32 frames_to_save  = available - processed;

			PaUtil_WriteRingBuffer(stream->in.tailBuffer, wasapi_buffer + bytes_processed, frames_to_save);
		}

		// Release host buffer
		if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->captureClient, available)) != S_OK)
		{
			LogHostError(hr);
			goto end;
		}
	}

end:

	// Notify blocking op has ended
	SetEvent(stream->hBlockingOpStreamRD);

	return (hr != S_OK ? paUnanticipatedHostError : paNoError);
}

// ------------------------------------------------------------------------------------------
static PaError WriteStream( PaStream* s, const void *_buffer, unsigned long frames )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	//UINT32 frames;
	const BYTE *user_buffer = (const BYTE *)_buffer;
	BYTE *wasapi_buffer;
	HRESULT hr = S_OK;
	UINT32 i, available, sleep = 0;
	unsigned long processed;
	ThreadIdleScheduler sched;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->renderClient == NULL)
		return paBadStreamPtr;

	// Notify blocking op has begun
	ResetEvent(stream->hBlockingOpStreamWR);

	// Use thread scheduling for 500 microseconds (emulated) when wait time for frames is less than
	// 1 milliseconds, emulation helps to normalize CPU consumption and avoids too busy waiting
	ThreadIdleScheduler_Setup(&sched, 1, 500/* microseconds */);

    // Make a local copy of the user buffer pointer(s), this is necessary
	// because PaUtil_CopyOutput() advances these pointers every time it is called
    if (!stream->bufferProcessor.userOutputIsInterleaved)
    {
        user_buffer = (const BYTE *)alloca(sizeof(const BYTE *) * stream->bufferProcessor.outputChannelCount);
        if (user_buffer == NULL)
            return paInsufficientMemory;

        for (i = 0; i < stream->bufferProcessor.outputChannelCount; ++i)
            ((const BYTE **)user_buffer)[i] = ((const BYTE **)_buffer)[i];
    }

	// Blocking (potentially, untill 'frames' are consumed) loop
	while (frames != 0)
	{
		// Check if blocking call must be interrupted
		if (WaitForSingleObject(stream->hCloseRequest, sleep) != WAIT_TIMEOUT)
			break;

		// Get frames available
		if ((hr = _PollGetOutputFramesAvailable(stream, &available)) != S_OK)
		{
			LogHostError(hr);
			goto end;
		}

		// Wait for more frames to become available
		if (available == 0)
		{
			UINT32 sleep_frames = (frames < stream->out.framesPerHostCallback ? frames : stream->out.framesPerHostCallback);

			sleep  = GetFramesSleepTime(sleep_frames, stream->out.wavex.Format.nSamplesPerSec);
			sleep /= 2; // wait only for half of the buffer

			// Avoid busy waiting, schedule next 1 millesecond wait
			if (sleep == 0)
				sleep = ThreadIdleScheduler_NextSleep(&sched);

			continue;
		}

		// Keep in 'frmaes' range
		if (available > frames)
			available = frames;

		// Get pointer to host buffer
		if ((hr = IAudioRenderClient_GetBuffer(stream->renderClient, available, &wasapi_buffer)) != S_OK)
		{
			// Buffer size is too big, waiting
			if (hr == AUDCLNT_E_BUFFER_TOO_LARGE)
				continue;

			LogHostError(hr);
			goto end;
		}

		// Keep waiting again (on Vista it was noticed that WASAPI could SOMETIMES return NULL pointer 
		// to buffer without returning AUDCLNT_E_BUFFER_TOO_LARGE instead)
		if (wasapi_buffer == NULL)
			continue;

		// Register available frames to processor
        PaUtil_SetOutputFrameCount(&stream->bufferProcessor, available);

		// Register host buffer pointer to processor
        PaUtil_SetInterleavedOutputChannels(&stream->bufferProcessor, 0, wasapi_buffer,	stream->bufferProcessor.outputChannelCount);

		// Copy user data to host buffer (with conversion if applicable), this call will advance
		// pointer 'user_buffer' to consumed portion of data
		processed = PaUtil_CopyOutput(&stream->bufferProcessor, (const void **)&user_buffer, frames);
		frames -= processed;

		// Release host buffer
		if ((hr = IAudioRenderClient_ReleaseBuffer(stream->renderClient, available, 0)) != S_OK)
		{
			LogHostError(hr);
			goto end;
		}
	}

end:

	// Notify blocking op has ended
	SetEvent(stream->hBlockingOpStreamWR);

	return (hr != S_OK ? paUnanticipatedHostError : paNoError);
}

unsigned long PaUtil_GetOutputFrameCount( PaUtilBufferProcessor* bp )
{
	return bp->hostOutputFrameCount[0];
}

// ------------------------------------------------------------------------------------------
static signed long GetStreamReadAvailable( PaStream* s )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;

	HRESULT hr;
	UINT32  available = 0;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->captureClient == NULL)
		return paBadStreamPtr;

	// available in hardware buffer
	if ((hr = _PollGetInputFramesAvailable(stream, &available)) != S_OK)
	{
		LogHostError(hr);
		return paUnanticipatedHostError;
	}

	// available in software tail buffer
	available += PaUtil_GetRingBufferReadAvailable(stream->in.tailBuffer);

    return available;
}

// ------------------------------------------------------------------------------------------
static signed long GetStreamWriteAvailable( PaStream* s )
{
    PaWasapiStream *stream = (PaWasapiStream*)s;
	HRESULT hr;
	UINT32  available = 0;

	// validate
	if (!stream->running)
		return paStreamIsStopped;
	if (stream->renderClient == NULL)
		return paBadStreamPtr;

	if ((hr = _PollGetOutputFramesAvailable(stream, &available)) != S_OK)
	{
		LogHostError(hr);
		return paUnanticipatedHostError;
	}

	return (signed long)available;
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
	if (stream->in.clientProc != NULL)
	{
		PaTime pending_time;
		if ((hr = IAudioClient_GetCurrentPadding(stream->in.clientProc, &pending)) == S_OK)
			pending_time = (PaTime)pending / (PaTime)stream->in.wavex.Format.nSamplesPerSec;
		else
			pending_time = (PaTime)stream->in.latencySeconds;

		timeInfo.inputBufferAdcTime = timeInfo.currentTime + pending_time;
	}
	// Query output current latency
	if (stream->out.clientProc != NULL)
	{
		PaTime pending_time;
		if ((hr = IAudioClient_GetCurrentPadding(stream->out.clientProc, &pending)) == S_OK)
			pending_time = (PaTime)pending / (PaTime)stream->out.wavex.Format.nSamplesPerSec;
		else
			pending_time = (PaTime)stream->out.latencySeconds;

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
// Described at:
// http://msdn.microsoft.com/en-us/library/dd371387(v=VS.85).aspx

PaError PaWasapi_GetJackCount(PaDeviceIndex nDevice, int *jcount)
{
	PaError ret;
	HRESULT hr = S_OK;
	PaDeviceIndex index;
    IDeviceTopology *pDeviceTopology = NULL;
    IConnector *pConnFrom = NULL;
    IConnector *pConnTo = NULL;
    IPart *pPart = NULL;
    IKsJackDescription *pJackDesc = NULL;
	UINT jackCount = 0;

	PaWasapiHostApiRepresentation *paWasapi = _GetHostApi(&ret);
	if (paWasapi == NULL)
		return paNotInitialized;

	// Get device index.
	ret = PaUtil_DeviceIndexToHostApiDeviceIndex(&index, nDevice, &paWasapi->inheritedHostApiRep);
    if (ret != paNoError)
        return ret;

	// Validate index.
	if ((UINT32)index >= paWasapi->deviceCount)
		return paInvalidDevice;

	// Get the endpoint device's IDeviceTopology interface.
	hr = IMMDevice_Activate(paWasapi->devInfo[index].device, &pa_IID_IDeviceTopology,
		CLSCTX_INPROC_SERVER, NULL, (void**)&pDeviceTopology);
	IF_FAILED_JUMP(hr, error);

    // The device topology for an endpoint device always contains just one connector (connector number 0).
	hr = IDeviceTopology_GetConnector(pDeviceTopology, 0, &pConnFrom);
	IF_FAILED_JUMP(hr, error);

    // Step across the connection to the jack on the adapter.
	hr = IConnector_GetConnectedTo(pConnFrom, &pConnTo);
    if (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
    {
        // The adapter device is not currently active.
        hr = E_NOINTERFACE;
    }
	IF_FAILED_JUMP(hr, error);

	// Get the connector's IPart interface.
	hr = IConnector_QueryInterface(pConnTo, &pa_IID_IPart, (void**)&pPart);
	IF_FAILED_JUMP(hr, error);

	// Activate the connector's IKsJackDescription interface.
	hr = IPart_Activate(pPart, CLSCTX_INPROC_SERVER, &pa_IID_IKsJackDescription, (void**)&pJackDesc);
	IF_FAILED_JUMP(hr, error);

	// Return jack count for this device.
	hr = IKsJackDescription_GetJackCount(pJackDesc, &jackCount);
	IF_FAILED_JUMP(hr, error);

	// Set.
	(*jcount) = jackCount;

	// Ok.
	ret = paNoError;

error:

	SAFE_RELEASE(pDeviceTopology);
	SAFE_RELEASE(pConnFrom);
	SAFE_RELEASE(pConnTo);
	SAFE_RELEASE(pPart);
	SAFE_RELEASE(pJackDesc);

	LogHostError(hr);
	return paNoError;
}

// ------------------------------------------------------------------------------------------
static PaWasapiJackConnectionType ConvertJackConnectionTypeWASAPIToPA(int connType)
{
	switch (connType)
	{
		case eConnTypeUnknown:			return eJackConnTypeUnknown;
#ifdef _KS_
		case eConnType3Point5mm:		return eJackConnType3Point5mm;
#else
		case eConnTypeEighth:		    return eJackConnType3Point5mm;
#endif
		case eConnTypeQuarter:			return eJackConnTypeQuarter;
		case eConnTypeAtapiInternal:	return eJackConnTypeAtapiInternal;
		case eConnTypeRCA:				return eJackConnTypeRCA;
		case eConnTypeOptical:			return eJackConnTypeOptical;
		case eConnTypeOtherDigital:		return eJackConnTypeOtherDigital;
		case eConnTypeOtherAnalog:		return eJackConnTypeOtherAnalog;
		case eConnTypeMultichannelAnalogDIN: return eJackConnTypeMultichannelAnalogDIN;
		case eConnTypeXlrProfessional:	return eJackConnTypeXlrProfessional;
		case eConnTypeRJ11Modem:		return eJackConnTypeRJ11Modem;
		case eConnTypeCombination:		return eJackConnTypeCombination;
	}
	return eJackConnTypeUnknown;
}

// ------------------------------------------------------------------------------------------
static PaWasapiJackGeoLocation ConvertJackGeoLocationWASAPIToPA(int geoLoc)
{
	switch (geoLoc)
	{
	case eGeoLocRear:				return eJackGeoLocRear;
	case eGeoLocFront:				return eJackGeoLocFront;
	case eGeoLocLeft:				return eJackGeoLocLeft;
	case eGeoLocRight:				return eJackGeoLocRight;
	case eGeoLocTop:				return eJackGeoLocTop;
	case eGeoLocBottom:				return eJackGeoLocBottom;
#ifdef _KS_
	case eGeoLocRearPanel:			return eJackGeoLocRearPanel;
#else
	case eGeoLocRearOPanel:         return eJackGeoLocRearPanel;
#endif
	case eGeoLocRiser:				return eJackGeoLocRiser;
	case eGeoLocInsideMobileLid:	return eJackGeoLocInsideMobileLid;
	case eGeoLocDrivebay:			return eJackGeoLocDrivebay;
	case eGeoLocHDMI:				return eJackGeoLocHDMI;
	case eGeoLocOutsideMobileLid:	return eJackGeoLocOutsideMobileLid;
	case eGeoLocATAPI:				return eJackGeoLocATAPI;
	}
	return eJackGeoLocUnk;
}

// ------------------------------------------------------------------------------------------
static PaWasapiJackGenLocation ConvertJackGenLocationWASAPIToPA(int genLoc)
{
	switch (genLoc)
	{
	case eGenLocPrimaryBox:	return eJackGenLocPrimaryBox;
	case eGenLocInternal:	return eJackGenLocInternal;
#ifdef _KS_
	case eGenLocSeparate:	return eJackGenLocSeparate;
#else
	case eGenLocSeperate:	return eJackGenLocSeparate;
#endif
	case eGenLocOther:		return eJackGenLocOther;
	}
	return eJackGenLocPrimaryBox;
}

// ------------------------------------------------------------------------------------------
static PaWasapiJackPortConnection ConvertJackPortConnectionWASAPIToPA(int portConn)
{
	switch (portConn)
	{
	case ePortConnJack:					return eJackPortConnJack;
	case ePortConnIntegratedDevice:		return eJackPortConnIntegratedDevice;
	case ePortConnBothIntegratedAndJack:return eJackPortConnBothIntegratedAndJack;
	case ePortConnUnknown:				return eJackPortConnUnknown;
	}
	return eJackPortConnJack;
}

// ------------------------------------------------------------------------------------------
// Described at:
// http://msdn.microsoft.com/en-us/library/dd371387(v=VS.85).aspx

PaError PaWasapi_GetJackDescription(PaDeviceIndex nDevice, int jindex, PaWasapiJackDescription *pJackDescription)
{
	PaError ret;
	HRESULT hr = S_OK;
	PaDeviceIndex index;
    IDeviceTopology *pDeviceTopology = NULL;
    IConnector *pConnFrom = NULL;
    IConnector *pConnTo = NULL;
    IPart *pPart = NULL;
    IKsJackDescription *pJackDesc = NULL;
	KSJACK_DESCRIPTION jack = { 0 };

	PaWasapiHostApiRepresentation *paWasapi = _GetHostApi(&ret);
	if (paWasapi == NULL)
		return paNotInitialized;

	// Get device index.
	ret = PaUtil_DeviceIndexToHostApiDeviceIndex(&index, nDevice, &paWasapi->inheritedHostApiRep);
    if (ret != paNoError)
        return ret;

	// Validate index.
	if ((UINT32)index >= paWasapi->deviceCount)
		return paInvalidDevice;

	// Get the endpoint device's IDeviceTopology interface.
	hr = IMMDevice_Activate(paWasapi->devInfo[index].device, &pa_IID_IDeviceTopology,
		CLSCTX_INPROC_SERVER, NULL, (void**)&pDeviceTopology);
	IF_FAILED_JUMP(hr, error);

    // The device topology for an endpoint device always contains just one connector (connector number 0).
	hr = IDeviceTopology_GetConnector(pDeviceTopology, 0, &pConnFrom);
	IF_FAILED_JUMP(hr, error);

    // Step across the connection to the jack on the adapter.
	hr = IConnector_GetConnectedTo(pConnFrom, &pConnTo);
    if (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
    {
        // The adapter device is not currently active.
        hr = E_NOINTERFACE;
    }
	IF_FAILED_JUMP(hr, error);

	// Get the connector's IPart interface.
	hr = IConnector_QueryInterface(pConnTo, &pa_IID_IPart, (void**)&pPart);
	IF_FAILED_JUMP(hr, error);

	// Activate the connector's IKsJackDescription interface.
	hr = IPart_Activate(pPart, CLSCTX_INPROC_SERVER, &pa_IID_IKsJackDescription, (void**)&pJackDesc);
	IF_FAILED_JUMP(hr, error);

	// Test to return jack description struct for index 0.
	hr = IKsJackDescription_GetJackDescription(pJackDesc, jindex, &jack);
	IF_FAILED_JUMP(hr, error);

	// Convert WASAPI values to PA format.
	pJackDescription->channelMapping = jack.ChannelMapping;
	pJackDescription->color          = jack.Color;
	pJackDescription->connectionType = ConvertJackConnectionTypeWASAPIToPA(jack.ConnectionType);
	pJackDescription->genLocation    = ConvertJackGenLocationWASAPIToPA(jack.GenLocation);
	pJackDescription->geoLocation    = ConvertJackGeoLocationWASAPIToPA(jack.GeoLocation);
	pJackDescription->isConnected    = jack.IsConnected;
	pJackDescription->portConnection = ConvertJackPortConnectionWASAPIToPA(jack.PortConnection);

	// Ok.
	ret = paNoError;

error:

	SAFE_RELEASE(pDeviceTopology);
	SAFE_RELEASE(pConnFrom);
	SAFE_RELEASE(pConnTo);
	SAFE_RELEASE(pPart);
	SAFE_RELEASE(pJackDesc);

	LogHostError(hr);
	return ret;
}

// ------------------------------------------------------------------------------------------
HRESULT _PollGetOutputFramesAvailable(PaWasapiStream *stream, UINT32 *available)
{
	HRESULT hr;
	UINT32 frames  = stream->out.framesPerHostCallback,
		   padding = 0;

	(*available) = 0;

	// get read position
	if ((hr = IAudioClient_GetCurrentPadding(stream->out.clientProc, &padding)) != S_OK)
		return LogHostError(hr);

	// get available
	frames -= padding;

	// set
	(*available) = frames;
	return hr;
}

// ------------------------------------------------------------------------------------------
HRESULT _PollGetInputFramesAvailable(PaWasapiStream *stream, UINT32 *available)
{
	HRESULT hr;

	(*available) = 0;

	// GetCurrentPadding() has opposite meaning to Output stream 
	if ((hr = IAudioClient_GetCurrentPadding(stream->in.clientProc, available)) != S_OK)
		return LogHostError(hr);

	return hr;
}

// ------------------------------------------------------------------------------------------
HRESULT ProcessOutputBuffer(PaWasapiStream *stream, PaWasapiHostProcessor *processor, UINT32 frames)
{
	HRESULT hr;
	BYTE *data = NULL;

	// Get buffer
	if ((hr = IAudioRenderClient_GetBuffer(stream->renderClient, frames, &data)) != S_OK)
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
			hr = IAudioClient_GetCurrentPadding(stream->out.clientProc, &padding);
			if (hr != S_OK)
				return LogHostError(hr);

			// Get frames to write
			frames -= padding;
			if (frames == 0)
				return S_OK;

			if ((hr = IAudioRenderClient_GetBuffer(stream->renderClient, frames, &data)) != S_OK)
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
	if (stream->out.monoMixer != NULL)
	{
		// expand buffer
		UINT32 mono_frames_size = frames * (stream->out.wavex.Format.wBitsPerSample / 8);
		if (mono_frames_size > stream->out.monoBufferSize)
			stream->out.monoBuffer = PaWasapi_ReallocateMemory(stream->out.monoBuffer, (stream->out.monoBufferSize = mono_frames_size));

		// process
		processor[S_OUTPUT].processor(NULL, 0, (BYTE *)stream->out.monoBuffer, frames, processor[S_OUTPUT].userData);

		// mix 1 to 2 channels
		stream->out.monoMixer(data, stream->out.monoBuffer, frames);
	}
	else
	{
		processor[S_OUTPUT].processor(NULL, 0, data, frames, processor[S_OUTPUT].userData);
	}

	// Release buffer
	if ((hr = IAudioRenderClient_ReleaseBuffer(stream->renderClient, frames, 0)) != S_OK)
		LogHostError(hr);

	return hr;
}

// ------------------------------------------------------------------------------------------
HRESULT ProcessInputBuffer(PaWasapiStream *stream, PaWasapiHostProcessor *processor)
{
	HRESULT hr = S_OK;
	UINT32 frames;
	BYTE *data = NULL;
	DWORD flags = 0;

	for (;;)
	{
		// Check if blocking call must be interrupted
		if (WaitForSingleObject(stream->hCloseRequest, 0) != WAIT_TIMEOUT)
			break;

		// Findout if any frames available
		frames = 0;
		if ((hr = _PollGetInputFramesAvailable(stream, &frames)) != S_OK)
			return hr;

		// Empty/consumed buffer
		if (frames == 0)
			break;

		// Get the available data in the shared buffer.
		if ((hr = IAudioCaptureClient_GetBuffer(stream->captureClient, &data, &frames, &flags, NULL, NULL)) != S_OK)
		{
			if (hr == AUDCLNT_S_BUFFER_EMPTY)
			{
				hr = S_OK;
				break; // Empty/consumed buffer
			}

			return LogHostError(hr);
			break;
		}

		// Detect silence
		// if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		//	data = NULL;

		// Process data
		if (stream->in.monoMixer != NULL)
		{
			// expand buffer
			UINT32 mono_frames_size = frames * (stream->in.wavex.Format.wBitsPerSample / 8);
			if (mono_frames_size > stream->in.monoBufferSize)
				stream->in.monoBuffer = PaWasapi_ReallocateMemory(stream->in.monoBuffer, (stream->in.monoBufferSize = mono_frames_size));

			// mix 1 to 2 channels
			stream->in.monoMixer(stream->in.monoBuffer, data, frames);

			// process
			processor[S_INPUT].processor((BYTE *)stream->in.monoBuffer, frames, NULL, 0, processor[S_INPUT].userData);
		}
		else
		{
			processor[S_INPUT].processor(data, frames, NULL, 0, processor[S_INPUT].userData);
		}

		// Release buffer
		if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->captureClient, frames)) != S_OK)
			return LogHostError(hr);

		//break;
	}

	return hr;
}

// ------------------------------------------------------------------------------------------
void _StreamOnStop(PaWasapiStream *stream)
{
	// Stop INPUT/OUTPUT clients
	if (!stream->bBlocking) 
	{
		if (stream->in.clientProc != NULL)
			IAudioClient_Stop(stream->in.clientProc);
		if (stream->out.clientProc != NULL)
			IAudioClient_Stop(stream->out.clientProc);
	} 
	else 
	{
		if (stream->in.clientParent != NULL)
			IAudioClient_Stop(stream->in.clientParent);
		if (stream->out.clientParent != NULL)
			IAudioClient_Stop(stream->out.clientParent);
	}

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
    PaWasapiHostProcessor processor[S_COUNT];
	HRESULT hr;
	DWORD dwResult;
    PaWasapiStream *stream = (PaWasapiStream *)param;
	PaWasapiHostProcessor defaultProcessor;
	BOOL set_event[S_COUNT] = { FALSE, FALSE };
	BOOL bWaitAllEvents = FALSE;
	BOOL bThreadComInitialized = FALSE;

	/*
	If COM is already initialized CoInitialize will either return
	FALSE, or RPC_E_CHANGED_MODE if it was initialized in a different
	threading mode. In either case we shouldn't consider it an error
	but we need to be careful to not call CoUninitialize() if 
	RPC_E_CHANGED_MODE was returned.
	*/
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr) && (hr != RPC_E_CHANGED_MODE))
	{
		PRINT(("WASAPI: failed ProcThreadEvent CoInitialize"));
		return paUnanticipatedHostError;
	}
	if (hr != RPC_E_CHANGED_MODE)
		bThreadComInitialized = TRUE;

	// Unmarshal stream pointers for safe COM operation
	hr = UnmarshalStreamComPointers(stream);
	if (hr != S_OK) {
		PRINT(("Error unmarshaling stream COM pointers. HRESULT: %i\n", hr));
		goto thread_end;
	}

	// Waiting on all events in case of Full-Duplex/Exclusive mode.
	if ((stream->in.clientProc != NULL) && (stream->out.clientProc != NULL))
	{
		bWaitAllEvents = (stream->in.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE) &&
			(stream->out.shareMode == AUDCLNT_SHAREMODE_EXCLUSIVE);
	}

    // Setup data processors
    defaultProcessor.processor = WaspiHostProcessingLoop;
    defaultProcessor.userData  = stream;
    processor[S_INPUT] = (stream->hostProcessOverrideInput.processor != NULL ? stream->hostProcessOverrideInput : defaultProcessor);
    processor[S_OUTPUT] = (stream->hostProcessOverrideOutput.processor != NULL ? stream->hostProcessOverrideOutput : defaultProcessor);

	// Boost thread priority
	PaWasapi_ThreadPriorityBoost((void **)&stream->hAvTask, stream->nThreadPriority);

	// Create events
	if (stream->event[S_OUTPUT] == NULL)
	{
		stream->event[S_OUTPUT] = CreateEvent(NULL, FALSE, FALSE, NULL);
		set_event[S_OUTPUT] = TRUE;
	}
	if (stream->event[S_INPUT] == NULL)
	{
		stream->event[S_INPUT]  = CreateEvent(NULL, FALSE, FALSE, NULL);
		set_event[S_INPUT] = TRUE;
	}
	if ((stream->event[S_OUTPUT] == NULL) || (stream->event[S_INPUT] == NULL))
	{
		PRINT(("WASAPI Thread: failed creating Input/Output event handle\n"));
		goto thread_error;
	}

	// Initialize event & start INPUT stream
	if (stream->in.clientProc)
	{
		// Create & set handle
		if (set_event[S_INPUT])
		{
			if ((hr = IAudioClient_SetEventHandle(stream->in.clientProc, stream->event[S_INPUT])) != S_OK)
			{
				LogHostError(hr);
				goto thread_error;
			}
		}

		// Start
		if ((hr = IAudioClient_Start(stream->in.clientProc)) != S_OK)
		{
			LogHostError(hr);
			goto thread_error;
		}
	}

	// Initialize event & start OUTPUT stream
	if (stream->out.clientProc)
	{
		// Create & set handle
		if (set_event[S_OUTPUT])
		{
			if ((hr = IAudioClient_SetEventHandle(stream->out.clientProc, stream->event[S_OUTPUT])) != S_OK)
			{
				LogHostError(hr);
				goto thread_error;
			}
		}

		// Preload buffer before start
		if ((hr = ProcessOutputBuffer(stream, processor, stream->out.framesPerBuffer)) != S_OK)
		{
			LogHostError(hr);
			goto thread_error;
		}

		// Start
		if ((hr = IAudioClient_Start(stream->out.clientProc)) != S_OK)
		{
			LogHostError(hr);
			goto thread_error;
		}

	}

	// Signal: stream running
	stream->running = TRUE;

	// Notify: thread started
	SetEvent(stream->hThreadStart);

	// Processing Loop
	for (;;)
    {
	    // 10 sec timeout (on timeout stream will auto-stop when processed by WAIT_TIMEOUT case)
        dwResult = WaitForMultipleObjects(S_COUNT, stream->event, bWaitAllEvents, 10*1000);

		// Check for close event (after wait for buffers to avoid any calls to user
		// callback when hCloseRequest was set)
		if (WaitForSingleObject(stream->hCloseRequest, 0) != WAIT_TIMEOUT)
			break;

		// Process S_INPUT/S_OUTPUT
		switch (dwResult)
		{
		case WAIT_TIMEOUT: {
			PRINT(("WASAPI Thread: WAIT_TIMEOUT - probably bad audio driver or Vista x64 bug: use paWinWasapiPolling instead\n"));
			goto thread_end;
			break; }

		// Input stream
		case WAIT_OBJECT_0 + S_INPUT: {

            if (stream->captureClient == NULL)
                break;

			if ((hr = ProcessInputBuffer(stream, processor)) != S_OK)
			{
				LogHostError(hr);
				goto thread_error;
			}

			break; }

		// Output stream
		case WAIT_OBJECT_0 + S_OUTPUT: {

            if (stream->renderClient == NULL)
                break;

			if ((hr = ProcessOutputBuffer(stream, processor, stream->out.framesPerBuffer)) != S_OK)
			{
				LogHostError(hr);
				goto thread_error;
			}

			break; }
		}
	}

thread_end:

	// Process stop
	_StreamOnStop(stream);

	// Release unmarshaled COM pointers
	ReleaseUnmarshaledComPointers(stream);

	// Cleanup COM for this thread
	if (bThreadComInitialized == TRUE)
		CoUninitialize();

	// Notify: not running
	stream->running = FALSE;

	// Notify: thread exited
	SetEvent(stream->hThreadExit);

	return 0;

thread_error:

	// Prevent deadlocking in Pa_StreamStart
	SetEvent(stream->hThreadStart);

	// Exit
	goto thread_end;
}

// ------------------------------------------------------------------------------------------
PA_THREAD_FUNC ProcThreadPoll(void *param)
{
    PaWasapiHostProcessor processor[S_COUNT];
	HRESULT hr;
    PaWasapiStream *stream = (PaWasapiStream *)param;
	PaWasapiHostProcessor defaultProcessor;
	INT32 i;
	ThreadIdleScheduler scheduler;

	// Calculate the actual duration of the allocated buffer.
	DWORD sleep_ms     = 0;
	DWORD sleep_ms_in;
	DWORD sleep_ms_out;

	BOOL bThreadComInitialized = FALSE;

	/*
	If COM is already initialized CoInitialize will either return
	FALSE, or RPC_E_CHANGED_MODE if it was initialized in a different
	threading mode. In either case we shouldn't consider it an error
	but we need to be careful to not call CoUninitialize() if 
	RPC_E_CHANGED_MODE was returned.
	*/
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr) && (hr != RPC_E_CHANGED_MODE))
	{
		PRINT(("WASAPI: failed ProcThreadPoll CoInitialize"));
		return paUnanticipatedHostError;
	}
	if (hr != RPC_E_CHANGED_MODE)
		bThreadComInitialized = TRUE;

	// Unmarshal stream pointers for safe COM operation
	hr = UnmarshalStreamComPointers(stream);
	if (hr != S_OK) 
	{
		PRINT(("Error unmarshaling stream COM pointers. HRESULT: %i\n", hr));
		return 0;
	}

	// Calculate timeout for next polling attempt.
	sleep_ms_in  = GetFramesSleepTime(stream->in.framesPerHostCallback/WASAPI_PACKETS_PER_INPUT_BUFFER, stream->in.wavex.Format.nSamplesPerSec);
	sleep_ms_out = GetFramesSleepTime(stream->out.framesPerBuffer, stream->out.wavex.Format.nSamplesPerSec);

	// WASAPI Input packets tend to expire very easily, let's limit sleep time to 2 milliseconds
	// for all cases. Please propose better solution if any.
	if (sleep_ms_in > 2)
		sleep_ms_in = 2;

	// Adjust polling time for non-paUtilFixedHostBufferSize. Input stream is not adjustable as it is being
	// polled according its packet length.
	if (stream->bufferMode != paUtilFixedHostBufferSize)
	{
		//sleep_ms_in = GetFramesSleepTime(stream->bufferProcessor.framesPerUserBuffer, stream->in.wavex.Format.nSamplesPerSec);
		sleep_ms_out = GetFramesSleepTime(stream->bufferProcessor.framesPerUserBuffer, stream->out.wavex.Format.nSamplesPerSec);
	}

	// Choose smallest
	if ((sleep_ms_in != 0) && (sleep_ms_out != 0))
		sleep_ms = min(sleep_ms_in, sleep_ms_out);
	else
	{
		sleep_ms = (sleep_ms_in ? sleep_ms_in : sleep_ms_out);
	}
	// Make sure not 0, othervise use ThreadIdleScheduler
	if (sleep_ms == 0)
	{
		sleep_ms_in  = GetFramesSleepTimeMicroseconds(stream->in.framesPerHostCallback/WASAPI_PACKETS_PER_INPUT_BUFFER, stream->in.wavex.Format.nSamplesPerSec);
		sleep_ms_out = GetFramesSleepTimeMicroseconds(stream->bufferProcessor.framesPerUserBuffer, stream->out.wavex.Format.nSamplesPerSec);

		// Choose smallest
		if ((sleep_ms_in != 0) && (sleep_ms_out != 0))
			sleep_ms = min(sleep_ms_in, sleep_ms_out);
		else
		{
			sleep_ms = (sleep_ms_in ? sleep_ms_in : sleep_ms_out);
		}

		// Setup thread sleep scheduler
		ThreadIdleScheduler_Setup(&scheduler, 1, sleep_ms/* microseconds here */);
		sleep_ms = 0;
	}

    // Setup data processors
    defaultProcessor.processor = WaspiHostProcessingLoop;
    defaultProcessor.userData  = stream;
    processor[S_INPUT] = (stream->hostProcessOverrideInput.processor != NULL ? stream->hostProcessOverrideInput : defaultProcessor);
    processor[S_OUTPUT] = (stream->hostProcessOverrideOutput.processor != NULL ? stream->hostProcessOverrideOutput : defaultProcessor);

	// Boost thread priority
	PaWasapi_ThreadPriorityBoost((void **)&stream->hAvTask, stream->nThreadPriority);

	// Initialize event & start INPUT stream
	if (stream->in.clientProc)
	{
		if ((hr = IAudioClient_Start(stream->in.clientProc)) != S_OK)
		{
			LogHostError(hr);
			goto thread_error;
		}
	}

	// Initialize event & start OUTPUT stream
	if (stream->out.clientProc)
	{
		// Preload buffer (obligatory, othervise ->Start() will fail), avoid processing
		// when in full-duplex mode as it requires input processing as well
		if (!PA_WASAPI__IS_FULLDUPLEX(stream))
		{
			UINT32 frames = 0;
			if ((hr = _PollGetOutputFramesAvailable(stream, &frames)) == S_OK)
            {
				if (stream->bufferMode == paUtilFixedHostBufferSize)
				{
					if (frames >= stream->out.framesPerBuffer)
					{
						frames = stream->out.framesPerBuffer;

						if ((hr = ProcessOutputBuffer(stream, processor, frames)) != S_OK)
						{
							LogHostError(hr); // not fatal, just log
						}
					}
				}
				else
				{
					if (frames != 0)
					{
						if ((hr = ProcessOutputBuffer(stream, processor, frames)) != S_OK)
						{
							LogHostError(hr); // not fatal, just log
						}
					}
				}
            }
            else
			{
				LogHostError(hr); // not fatal, just log
			}
		}

		// Start
		if ((hr = IAudioClient_Start(stream->out.clientProc)) != S_OK)
		{
			LogHostError(hr);
			goto thread_error;
		}
	}

	// Signal: stream running
	stream->running = TRUE;

	// Notify: thread started
	SetEvent(stream->hThreadStart);

	if (!PA_WASAPI__IS_FULLDUPLEX(stream))
	{
		// Processing Loop
		UINT32 next_sleep = sleep_ms;
		while (WaitForSingleObject(stream->hCloseRequest, next_sleep) == WAIT_TIMEOUT)
		{
			// Get next sleep time
			if (sleep_ms == 0)
			{
				next_sleep = ThreadIdleScheduler_NextSleep(&scheduler);
			}

			for (i = 0; i < S_COUNT; ++i)
			{
				// Process S_INPUT/S_OUTPUT
				switch (i)
				{
				// Input stream
				case S_INPUT: {

					if (stream->captureClient == NULL)
						break;

					if ((hr = ProcessInputBuffer(stream, processor)) != S_OK)
					{
						LogHostError(hr);
						goto thread_error;
					}

					break; }

				// Output stream
				case S_OUTPUT: {

					UINT32 frames;
					if (stream->renderClient == NULL)
						break;

					// get available frames
					if ((hr = _PollGetOutputFramesAvailable(stream, &frames)) != S_OK)
					{
						LogHostError(hr);
						goto thread_error;
					}

					// output
					if (stream->bufferMode == paUtilFixedHostBufferSize)
					{
						if (frames >= stream->out.framesPerBuffer)
						{
							if ((hr = ProcessOutputBuffer(stream, processor, stream->out.framesPerBuffer)) != S_OK)
							{
								LogHostError(hr);
								goto thread_error;
							}
						}
					}
					else
					{
						if (frames != 0)
						{
							if ((hr = ProcessOutputBuffer(stream, processor, frames)) != S_OK)
							{
								LogHostError(hr);
								goto thread_error;
							}
						}
					}

					break; }
				}
			}
		}
	}
	else
	{
#if 0
		// Processing Loop
		while (WaitForSingleObject(stream->hCloseRequest, 1) == WAIT_TIMEOUT)
		{
			UINT32 i_frames = 0, i_processed = 0;
			BYTE *i_data = NULL, *o_data = NULL, *o_data_host = NULL;
			DWORD i_flags = 0;
			UINT32 o_frames = 0;

			// get host input buffer
			if ((hr = IAudioCaptureClient_GetBuffer(stream->captureClient, &i_data, &i_frames, &i_flags, NULL, NULL)) != S_OK)
			{
				if (hr == AUDCLNT_S_BUFFER_EMPTY)
					continue; // no data in capture buffer

				LogHostError(hr);
				break;
			}

			// get available frames
			if ((hr = _PollGetOutputFramesAvailable(stream, &o_frames)) != S_OK)
			{
				// release input buffer
				IAudioCaptureClient_ReleaseBuffer(stream->captureClient, 0);

				LogHostError(hr);
				break;
			}

			// process equal ammount of frames
			if (o_frames >= i_frames)
			{
				// process input ammount of frames
				UINT32 o_processed = i_frames;

				// get host output buffer
				if ((hr = IAudioRenderClient_GetBuffer(stream->procRCClient, o_processed, &o_data)) == S_OK)
				{
					// processed amount of i_frames
					i_processed = i_frames;
					o_data_host = o_data;

					// convert output mono
					if (stream->out.monoMixer)
					{
						UINT32 mono_frames_size = o_processed * (stream->out.wavex.Format.wBitsPerSample / 8);
						// expand buffer
						if (mono_frames_size > stream->out.monoBufferSize)
						{
							stream->out.monoBuffer = PaWasapi_ReallocateMemory(stream->out.monoBuffer, (stream->out.monoBufferSize = mono_frames_size));
							if (stream->out.monoBuffer == NULL)
							{
								// release input buffer
								IAudioCaptureClient_ReleaseBuffer(stream->captureClient, 0);
								// release output buffer
								IAudioRenderClient_ReleaseBuffer(stream->renderClient, 0, 0);

								LogPaError(paInsufficientMemory);
								break;
							}
						}

						// replace buffer pointer
						o_data = (BYTE *)stream->out.monoBuffer;
					}

					// convert input mono
					if (stream->in.monoMixer)
					{
						UINT32 mono_frames_size = i_processed * (stream->in.wavex.Format.wBitsPerSample / 8);
						// expand buffer
						if (mono_frames_size > stream->in.monoBufferSize)
						{
							stream->in.monoBuffer = PaWasapi_ReallocateMemory(stream->in.monoBuffer, (stream->in.monoBufferSize = mono_frames_size));
							if (stream->in.monoBuffer == NULL)
							{
								// release input buffer
								IAudioCaptureClient_ReleaseBuffer(stream->captureClient, 0);
								// release output buffer
								IAudioRenderClient_ReleaseBuffer(stream->renderClient, 0, 0);

								LogPaError(paInsufficientMemory);
								break;
							}
						}

						// mix 2 to 1 input channels
						stream->in.monoMixer(stream->in.monoBuffer, i_data, i_processed);

						// replace buffer pointer
						i_data = (BYTE *)stream->in.monoBuffer;
					}

					// process
					processor[S_FULLDUPLEX].processor(i_data, i_processed, o_data, o_processed, processor[S_FULLDUPLEX].userData);

					// mix 1 to 2 output channels
					if (stream->out.monoBuffer)
						stream->out.monoMixer(o_data_host, stream->out.monoBuffer, o_processed);

					// release host output buffer
					if ((hr = IAudioRenderClient_ReleaseBuffer(stream->renderClient, o_processed, 0)) != S_OK)
						LogHostError(hr);
				}
				else
				{
					if (stream->out.shareMode != AUDCLNT_SHAREMODE_SHARED)
						LogHostError(hr); // be silent in shared mode, try again next time
				}
			}

			// release host input buffer
			if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->captureClient, i_processed)) != S_OK)
			{
				LogHostError(hr);
				break;
			}
		}
#else
		// Processing Loop
		UINT32 next_sleep = sleep_ms;
		while (WaitForSingleObject(stream->hCloseRequest, next_sleep) == WAIT_TIMEOUT)
		{
			UINT32 i_frames = 0, i_processed = 0;
			BYTE *i_data = NULL, *o_data = NULL, *o_data_host = NULL;
			DWORD i_flags = 0;
			UINT32 o_frames = 0;

			// Get next sleep time
			if (sleep_ms == 0)
			{
				next_sleep = ThreadIdleScheduler_NextSleep(&scheduler);
			}

			// get available frames
			if ((hr = _PollGetOutputFramesAvailable(stream, &o_frames)) != S_OK)
			{
				LogHostError(hr);
				break;
			}

			while (o_frames != 0)
			{
				// get host input buffer
				if ((hr = IAudioCaptureClient_GetBuffer(stream->captureClient, &i_data, &i_frames, &i_flags, NULL, NULL)) != S_OK)
				{
					if (hr == AUDCLNT_S_BUFFER_EMPTY)
						break; // no data in capture buffer

					LogHostError(hr);
					break;
				}

				// process equal ammount of frames
				if (o_frames >= i_frames)
				{
					// process input ammount of frames
					UINT32 o_processed = i_frames;

					// get host output buffer
					if ((hr = IAudioRenderClient_GetBuffer(stream->renderClient, o_processed, &o_data)) == S_OK)
					{
						// processed amount of i_frames
						i_processed = i_frames;
						o_data_host = o_data;

						// convert output mono
						if (stream->out.monoMixer)
						{
							UINT32 mono_frames_size = o_processed * (stream->out.wavex.Format.wBitsPerSample / 8);
							// expand buffer
							if (mono_frames_size > stream->out.monoBufferSize)
							{
								stream->out.monoBuffer = PaWasapi_ReallocateMemory(stream->out.monoBuffer, (stream->out.monoBufferSize = mono_frames_size));
								if (stream->out.monoBuffer == NULL)
								{
									// release input buffer
									IAudioCaptureClient_ReleaseBuffer(stream->captureClient, 0);
									// release output buffer
									IAudioRenderClient_ReleaseBuffer(stream->renderClient, 0, 0);

									LogPaError(paInsufficientMemory);
									goto thread_error;
								}
							}

							// replace buffer pointer
							o_data = (BYTE *)stream->out.monoBuffer;
						}

						// convert input mono
						if (stream->in.monoMixer)
						{
							UINT32 mono_frames_size = i_processed * (stream->in.wavex.Format.wBitsPerSample / 8);
							// expand buffer
							if (mono_frames_size > stream->in.monoBufferSize)
							{
								stream->in.monoBuffer = PaWasapi_ReallocateMemory(stream->in.monoBuffer, (stream->in.monoBufferSize = mono_frames_size));
								if (stream->in.monoBuffer == NULL)
								{
									// release input buffer
									IAudioCaptureClient_ReleaseBuffer(stream->captureClient, 0);
									// release output buffer
									IAudioRenderClient_ReleaseBuffer(stream->renderClient, 0, 0);

									LogPaError(paInsufficientMemory);
									goto thread_error;
								}
							}

							// mix 2 to 1 input channels
							stream->in.monoMixer(stream->in.monoBuffer, i_data, i_processed);

							// replace buffer pointer
							i_data = (BYTE *)stream->in.monoBuffer;
						}

						// process
						processor[S_FULLDUPLEX].processor(i_data, i_processed, o_data, o_processed, processor[S_FULLDUPLEX].userData);

						// mix 1 to 2 output channels
						if (stream->out.monoBuffer)
							stream->out.monoMixer(o_data_host, stream->out.monoBuffer, o_processed);

						// release host output buffer
						if ((hr = IAudioRenderClient_ReleaseBuffer(stream->renderClient, o_processed, 0)) != S_OK)
							LogHostError(hr);

						o_frames -= o_processed;
					}
					else
					{
						if (stream->out.shareMode != AUDCLNT_SHAREMODE_SHARED)
							LogHostError(hr); // be silent in shared mode, try again next time
					}
				}
				else
				{
					i_processed = 0;
					goto fd_release_buffer_in;
				}

fd_release_buffer_in:

				// release host input buffer
				if ((hr = IAudioCaptureClient_ReleaseBuffer(stream->captureClient, i_processed)) != S_OK)
				{
					LogHostError(hr);
					break;
				}

				// break processing, input hasn't been accumulated yet
				if (i_processed == 0)
					break;
			}
		}
#endif
	}

thread_end:

	// Process stop
	_StreamOnStop(stream);

	// Release unmarshaled COM pointers
	ReleaseUnmarshaledComPointers(stream);

	// Cleanup COM for this thread
	if (bThreadComInitialized == TRUE)
		CoUninitialize();

	// Notify: not running
	stream->running = FALSE;

	// Notify: thread exited
	SetEvent(stream->hThreadExit);

	return 0;

thread_error:

	// Prevent deadlocking in Pa_StreamStart
	SetEvent(stream->hThreadStart);

	// Exit
	goto thread_end;
}

// ------------------------------------------------------------------------------------------
void *PaWasapi_ReallocateMemory(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

// ------------------------------------------------------------------------------------------
void PaWasapi_FreeMemory(void *ptr)
{
	free(ptr);
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
