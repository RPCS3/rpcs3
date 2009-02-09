//------------------------------------------------------------------------------
// File: Streams.h
//
// Desc: DirectShow base classes - defines overall streams architecture.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __STREAMS__
#define __STREAMS__

#ifdef	_MSC_VER
// disable some level-4 warnings, use #pragma warning(enable:###) to re-enable
#pragma warning(disable:4100) // warning C4100: unreferenced formal parameter
#pragma warning(disable:4127) // warning C4127: conditional expression is constant
#pragma warning(disable:4189) // warning C4189: local variable is initialized but not referenced
#pragma warning(disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning(disable:4511) // warning C4511: copy constructor could not be generated
#pragma warning(disable:4512) // warning C4512: assignment operator could not be generated
#pragma warning(disable:4514) // warning C4514: unreferenced inline function has been removed
#pragma warning(disable:4710) // warning C4710: 'function' not inlined

#if _MSC_VER>=1100
#define AM_NOVTABLE __declspec(novtable)
#else
#define AM_NOVTABLE
#endif
#endif	// MSC_VER

// Because of differences between Visual C++ and older Microsoft SDKs,
// you may have defined _DEBUG without defining DEBUG.  This logic
// ensures that both will be set if Visual C++ sets _DEBUG.
#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <ddraw.h>

// Disable warning message for C4201 - use of nameless struct/union
// Otherwise, strmif.h will generate warnings for Win32 debug builds
#pragma warning( disable : 4201 )

#include <mmsystem.h>

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

///////////////////////////////////////////////////////////////////////////
// The following definitions come from the Platform SDK and are required if
// the applicaiton is being compiled with the headers from Visual C++ 6.0.
///////////////////////////////////////////////////////////////////////////
#ifndef InterlockedExchangePointer
	#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))
#endif

#ifndef _WAVEFORMATEXTENSIBLE_
#define _WAVEFORMATEXTENSIBLE_
typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to zero. */
    } Samples;
    DWORD           dwChannelMask;      /* which channels are */
                                        /* present in stream  */
    GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#endif // !_WAVEFORMATEXTENSIBLE_

#if !defined(WAVE_FORMAT_EXTENSIBLE)
#define  WAVE_FORMAT_EXTENSIBLE                 0xFFFE
#endif // !defined(WAVE_FORMAT_EXTENSIBLE)

#ifndef GetWindowLongPtr
  #define GetWindowLongPtrA   GetWindowLongA
  #define GetWindowLongPtrW   GetWindowLongW
  #ifdef UNICODE
    #define GetWindowLongPtr  GetWindowLongPtrW
  #else
    #define GetWindowLongPtr  GetWindowLongPtrA
  #endif // !UNICODE
#endif // !GetWindowLongPtr

#ifndef SetWindowLongPtr
  #define SetWindowLongPtrA   SetWindowLongA
  #define SetWindowLongPtrW   SetWindowLongW
  #ifdef UNICODE
    #define SetWindowLongPtr  SetWindowLongPtrW
  #else
    #define SetWindowLongPtr  SetWindowLongPtrA
  #endif // !UNICODE
#endif // !SetWindowLongPtr

#ifndef GWLP_WNDPROC
  #define GWLP_WNDPROC        (-4)
#endif
#ifndef GWLP_HINSTANCE
  #define GWLP_HINSTANCE      (-6)
#endif
#ifndef GWLP_HWNDPARENT
  #define GWLP_HWNDPARENT     (-8)
#endif
#ifndef GWLP_USERDATA
  #define GWLP_USERDATA       (-21)
#endif
#ifndef GWLP_ID
  #define GWLP_ID             (-12)
#endif
#ifndef DWLP_MSGRESULT
  #define DWLP_MSGRESULT  0
#endif
#ifndef DWLP_DLGPROC
  #define DWLP_DLGPROC    DWLP_MSGRESULT + sizeof(LRESULT)
#endif
#ifndef DWLP_USER
  #define DWLP_USER       DWLP_DLGPROC + sizeof(DLGPROC)
#endif
///////////////////////////////////////////////////////////////////////////
// End Platform SDK definitions
///////////////////////////////////////////////////////////////////////////

#include <initguid.h>

#pragma warning(disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#include <strmif.h>     // Generated IDL header file for streams interfaces

#include "reftime.h"    // Helper class for REFERENCE_TIME management
#include "wxdebug.h"    // Debug support for logging and ASSERTs
#include "amvideo.h"    // ActiveMovie video interfaces and definitions
//include amaudio.h explicitly if you need it.  it requires the DirectX SDK.
//#include "amaudio.h"    // ActiveMovie audio interfaces and definitions
#include "wxutil.h"     // General helper classes for threads etc
#include "combase.h"    // Base COM classes to support IUnknown
//#include "dllsetup.h"   // Filter registration support functions
#include "measure.h"    // Performance measurement
#include "comlite.h"    // Light weight com function prototypes

#include "cache.h"      // Simple cache container class
#include "wxlist.h"     // Non MFC generic list class
#include "msgthrd.h"	// CMsgThread
#include "mtype.h"      // Helper class for managing media types
#include "fourcc.h"     // conversions between FOURCCs and GUIDs
#include "control.h"    // generated from control.odl
#include "ctlutil.h"    // control interface utility classes
#include "evcode.h"     // event code definitions
#include "amfilter.h"   // Main streams architecture class hierachy
#include "transfrm.h"   // Generic transform filter
#include "transip.h"    // Generic transform-in-place filter
#include "uuids.h"      // declaration of type GUIDs and well-known clsids
#include "source.h"	// Generic source filter
#include "outputq.h"    // Output pin queueing
#include "errors.h"     // HRESULT status and error definitions
#include "renbase.h"    // Base class for writing ActiveX renderers
//#include "winutil.h"    // Helps with filters that manage windows
//#include "winctrl.h"    // Implements the IVideoWindow interface
//#include "videoctl.h"   // Specifically video related classes
#include "refclock.h"	// Base clock class
#include "sysclock.h"	// System clock
#include "pstream.h"    // IPersistStream helper class
#include "vtrans.h"     // Video Transform Filter base class
#include "amextra.h"
//#include "cprop.h"      // Base property page class
#include "strmctl.h"    // IAMStreamControl support
#include "edevdefs.h"   // External device control interface defines
#include "audevcod.h"   // audio filter device error event codes

#include <tchar.h>

#define NO_SHLWAPI_STRFCNS
#include <atlbase.h>
#include <strsafe.h>

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

class CPinInfo : public PIN_INFO
{
public:
	CPinInfo() {pFilter = NULL;}
	~CPinInfo() {if(pFilter) pFilter->Release();}
};

class CFilterInfo : public FILTER_INFO
{
public:
	CFilterInfo() {pGraph = NULL;}
	~CFilterInfo() {if(pGraph) pGraph->Release();}
};

#define BeginEnumFilters(pFilterGraph, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if(pFilterGraph && SUCCEEDED(pFilterGraph->EnumFilters(&pEnumFilters))) \
	{ \
		for(CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
		{ \

#define EndEnumFilters }}}

#define BeginEnumCachedFilters(pGraphConfig, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if(pGraphConfig && SUCCEEDED(pGraphConfig->EnumCacheFilter(&pEnumFilters))) \
	{ \
		for(CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
		{ \

#define EndEnumCachedFilters }}}

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
	{CComPtr<IEnumPins> pEnumPins; \
	if(pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
	{ \
		for(CComPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = NULL) \
		{ \

#define EndEnumPins }}}

#define BeginEnumMediaTypes(pPin, pEnumMediaTypes, pMediaType) \
	{CComPtr<IEnumMediaTypes> pEnumMediaTypes; \
	if(pPin && SUCCEEDED(pPin->EnumMediaTypes(&pEnumMediaTypes))) \
	{ \
		AM_MEDIA_TYPE* pMediaType = NULL; \
		for(; S_OK == pEnumMediaTypes->Next(1, &pMediaType, NULL); DeleteMediaType(pMediaType), pMediaType = NULL) \
		{ \

#define EndEnumMediaTypes(pMediaType) } if(pMediaType) DeleteMediaType(pMediaType); }}

#define BeginEnumSysDev(clsid, pMoniker) \
	{CComPtr<ICreateDevEnum> pDevEnum4$##clsid; \
	pDevEnum4$##clsid.CoCreateInstance(CLSID_SystemDeviceEnum); \
	CComPtr<IEnumMoniker> pClassEnum4$##clsid; \
	if(SUCCEEDED(pDevEnum4$##clsid->CreateClassEnumerator(clsid, &pClassEnum4$##clsid, 0)) \
	&& pClassEnum4$##clsid) \
	{ \
		for(CComPtr<IMoniker> pMoniker; pClassEnum4$##clsid->Next(1, &pMoniker, 0) == S_OK; pMoniker = NULL) \
		{ \

#define EndEnumSysDev }}}

#else
    #ifdef DEBUG
    #pragma message("STREAMS.H included TWICE")
    #endif
#endif // __STREAMS__

