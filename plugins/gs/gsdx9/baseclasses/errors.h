//------------------------------------------------------------------------------
// File: Errors.h
//
// Desc:  ActiveMovie error defines.
//
// Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __ERRORS__
#define __ERRORS__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef _AMOVIE_
#define AMOVIEAPI   DECLSPEC_IMPORT
#else
#define AMOVIEAPI
#endif

// codes 0-01ff are reserved for OLE
#define VFW_FIRST_CODE   0x200
#define MAX_ERROR_TEXT_LEN 160

#include <VFWMSGS.H>                    // includes all message definitions

typedef BOOL (WINAPI* AMGETERRORTEXTPROCA)(HRESULT, char *, DWORD);
typedef BOOL (WINAPI* AMGETERRORTEXTPROCW)(HRESULT, WCHAR *, DWORD);

AMOVIEAPI DWORD WINAPI AMGetErrorTextA( HRESULT hr , char *pbuffer , DWORD MaxLen);
AMOVIEAPI DWORD WINAPI AMGetErrorTextW( HRESULT hr , WCHAR *pbuffer , DWORD MaxLen);


#ifdef UNICODE
#define AMGetErrorText  AMGetErrorTextW
typedef AMGETERRORTEXTPROCW AMGETERRORTEXTPROC;
#else
#define AMGetErrorText  AMGetErrorTextA
typedef AMGETERRORTEXTPROCA AMGETERRORTEXTPROC;
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __ERRORS__
