//------------------------------------------------------------------------------
// File: COMLite.h
//
// Desc: This header file is to provide a migration path for users of 
//       ActiveMovie betas 1 and 2.
//
// Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef _INC_COMLITE_
#define _INC_COMLITE_

#define QzInitialize            CoInitialize
#define QzUninitialize          CoUninitialize
#define QzFreeUnusedLibraries   CoFreeUnusedLibraries

#define QzGetMalloc             CoGetMalloc
#define QzTaskMemAlloc          CoTaskMemAlloc
#define QzTaskMemRealloc        CoTaskMemRealloc
#define QzTaskMemFree           CoTaskMemFree
#define QzCreateFilterObject    CoCreateInstance
#define QzCLSIDFromString       CLSIDFromString
#define QzStringFromGUID2       StringFromGUID2

#endif  // _INC_COMLITE_
