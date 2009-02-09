// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_)
#define AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _GCC
#define EXPORT_GCC  __declspec (dllexport)
#else
#define EXPORT_GCC
#endif

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <commdlg.h>
#include <windowsx.h>
#include <stddef.h>

// special setup for MinGW/Dev-C++ ################### //
// (I don't want to change PS2Edefs.h/PS2Etypes.h)

#undef __WIN32__

typedef char s8;
typedef short s16;
typedef long s32;
#ifdef _GCC
typedef long long s64;
#else
typedef __int64 s64;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#ifdef _GCC
typedef unsigned long long u64;
#else
typedef unsigned __int64 u64;
#endif

#include "PS2Edefs.h"

#define __WIN32__ 

// ################################################### //


#include "wnaspi32.h"
#include "scsidefs.h"

#include "defines.h"
#include "cdda.h"
#include "cdr.h"
#include "cfg.h"
#include "generic.h"
#include "ioctrl.h"
#include "ppf.h"
#include "read.h"
#include "scsi.h"
#include "sub.h"
#include "toc.h"

//#define DBGOUT

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A412EBA3_CBB9_11D6_A315_008048C61B72__INCLUDED_)
