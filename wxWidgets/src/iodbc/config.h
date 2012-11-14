/*
 *
 *  config.h
 *
 *  $Id: config.h 3995 1999-10-14 17:08:31Z RR $
 *
 *  Configuration
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com> 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef	_CONFIG_H
#define	_CONFIG_H

#if	!defined(WINDOWS) && !defined(WIN32_SYSTEM)
#define	_UNIX_

#include "wx/setup.h"

/* we have these definitions from configure */
#if defined(HAVE_DLOPEN)
    #define DLDAPI_SVR4_DLFCN
#elif defined(HAVE_SHL_LOAD)
    #define DLDAPI_HP_SHL
#endif

#include	<stdlib.h>
#include	<sys/types.h>
#include        <string.h>
#include        <stdio.h>

#define	MEM_ALLOC(size)	(malloc((size_t)(size)))
#define	MEM_FREE(ptr)	{if(ptr) free(ptr);}

#define	STRCPY(t, s)	(strcpy((char*)(t), (char*)(s)))
#define	STRNCPY(t,s,n)	(strncpy((char*)(t), (char*)(s), (size_t)(n)))
#define	STRCAT(t, s)	(strcat((char*)(t), (char*)(s)))
#define	STRNCAT(t,s,n)	(strncat((char*)(t), (char*)(s), (size_t)(n)))
#define	STREQ(a, b)	(strcmp((char*)(a), (char*)(b)) == 0)
#define	STRLEN(str)	((str)? strlen((char*)(str)):0)

#define	EXPORT
#define	CALLBACK
#define	FAR

typedef signed short SSHOR;
typedef short WORD;
typedef long DWORD;

typedef WORD WPARAM;
typedef DWORD LPARAM;
typedef int BOOL;
#endif /* _UNIX_ */

#if	defined(WINDOWS) || defined(WIN32_SYSTEM)
#include	<windows.h>
#include	<windowsx.h>

#ifdef	_MSVC_
#define	MEM_ALLOC(size)	(fmalloc((size_t)(size)))
#define	MEM_FREE(ptr)	((ptr)? ffree((PTR)(ptr)):0))
#define	STRCPY(t, s)	(fstrcpy((char FAR*)(t), (char FAR*)(s)))
#define	STRNCPY(t,s,n)	(fstrncpy((char FAR*)(t), (char FAR*)(s), (size_t)(n)))
#define	STRLEN(str)	((str)? fstrlen((char FAR*)(str)):0)
#define	STREQ(a, b)	(fstrcmp((char FAR*)(a), (char FAR*)(b) == 0)
#endif

#ifdef	_BORLAND_
#define	MEM_ALLOC(size)	(farmalloc((unsigned long)(size))
#define	MEM_FREE(ptr)	((ptr)? farfree((void far*)(ptr)):0)
#define	STRCPY(t, s)	(_fstrcpy((char FAR*)(t), (char FAR*)(s)))
#define	STRNCPY(t,s,n)	(_fstrncpy((char FAR*)(t), (char FAR*)(s), (size_t)(n)))
#define      STRLEN(str)     ((str)? _fstrlen((char FAR*)(str)):0)
#define      STREQ(a, b)     (_fstrcmp((char FAR*)(a), (char FAR*)(b) == 0)
#endif

#endif /* WINDOWS */

#define	SYSERR		(-1)

#ifndef	NULL
#define	NULL		((void FAR*)0UL)
#endif

#endif
