//------------------------------------------------------------------------------
// File: DShow.h
//
// Desc: DirectShow top-level include file
//
// Copyright (c) 2000-2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef __DSHOW_INCLUDED__
#define __DSHOW_INCLUDED__

///////////////////////////////////////////////////////////////////////////
// Set up constants & pragmas for the compiler
///////////////////////////////////////////////////////////////////////////
#ifdef  _MSC_VER
// disable some level-4 warnings, use #pragma warning(default:###) to re-enable
#pragma warning(disable:4100) // warning C4100: unreferenced formal parameter
#pragma warning(disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning(disable:4511) // warning C4511: copy constructor could not be generated
#pragma warning(disable:4512) // warning C4512: assignment operator could not be generated
#pragma warning(disable:4514) // warning C4514: "unreferenced inline function has been removed"

#if _MSC_VER>=1100
#define AM_NOVTABLE __declspec(novtable)
#else
#define AM_NOVTABLE
#endif
#endif  // MSC_VER

///////////////////////////////////////////////////////////////////////////
// Include standard Windows files
///////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <ddraw.h>
#include <mmsystem.h>

#ifndef NO_DSHOW_STRSAFE
#define NO_SHLWAPI_STRFCNS
#include <strsafe.h>  
#endif

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

///////////////////////////////////////////////////////////////////////////
// Include DirectShow include files
///////////////////////////////////////////////////////////////////////////
#include <strmif.h>     // Generated IDL header file for streams interfaces
#include <amvideo.h>    // ActiveMovie video interfaces and definitions
#include <amaudio.h>    // ActiveMovie audio interfaces and definitions
#include <control.h>    // generated from control.odl
#include <evcode.h>     // event code definitions
#include <uuids.h>      // declaration of type GUIDs and well-known clsids
#include <errors.h>     // HRESULT status and error definitions
#include <edevdefs.h>   // External device control interface defines
#include <audevcod.h>   // audio filter device error event codes
#include <dvdevcod.h>   // DVD error event codes

///////////////////////////////////////////////////////////////////////////
// Define OLE Automation constants
///////////////////////////////////////////////////////////////////////////
#ifndef OATRUE
#define OATRUE (-1)
#endif // OATRUE
#ifndef OAFALSE
#define OAFALSE (0)
#endif // OAFALSE

///////////////////////////////////////////////////////////////////////////
// Define Win64 interfaces if not already defined
///////////////////////////////////////////////////////////////////////////

// InterlockedExchangePointer
#ifndef InterlockedExchangePointer
#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))
#endif 


#endif // __DSHOW_INCLUDED__
