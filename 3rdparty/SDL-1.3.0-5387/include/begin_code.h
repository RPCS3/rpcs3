/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/**
 *  \file begin_code.h
 *
 *  This file sets things up for C dynamic library function definitions,
 *  static inlined functions, and structures aligned at 4-byte alignment.
 *  If you don't like ugly C preprocessor code, don't look at this file. :)
 */

/* This shouldn't be nested -- included it around code only. */
#ifdef _begin_code_h
#error Nested inclusion of begin_code.h
#endif
#define _begin_code_h

/* Some compilers use a special export keyword */
#ifndef DECLSPEC
# if defined(__BEOS__) || defined(__HAIKU__)
#  if defined(__GNUC__)
#   define DECLSPEC	__declspec(dllexport)
#  else
#   define DECLSPEC	__declspec(export)
#  endif
# elif defined(__WIN32__)
#  ifdef __BORLANDC__
#   ifdef BUILD_SDL
#    define DECLSPEC
#   else
#    define DECLSPEC	__declspec(dllimport)
#   endif
#  else
#   define DECLSPEC	__declspec(dllexport)
#  endif
# else
#  if defined(__GNUC__) && __GNUC__ >= 4
#   define DECLSPEC	__attribute__ ((visibility("default")))
#  else
#   define DECLSPEC
#  endif
# endif
#endif

/* By default SDL uses the C calling convention */
#ifndef SDLCALL
#if defined(__WIN32__) && !defined(__GNUC__)
#define SDLCALL __cdecl
#else
#define SDLCALL
#endif
#endif /* SDLCALL */

/* Removed DECLSPEC on Symbian OS because SDL cannot be a DLL in EPOC */
#ifdef __SYMBIAN32__
#undef DECLSPEC
#define DECLSPEC
#endif /* __SYMBIAN32__ */

/* Force structure packing at 4 byte alignment.
   This is necessary if the header is included in code which has structure
   packing set to an alternate value, say for loading structures from disk.
   The packing is reset to the previous value in close_code.h
 */
#if defined(_MSC_VER) || defined(__MWERKS__) || defined(__BORLANDC__)
#ifdef _MSC_VER
#pragma warning(disable: 4103)
#endif
#ifdef __BORLANDC__
#pragma nopackwarning
#endif
#pragma pack(push,4)
#endif /* Compiler needs structure packing set */

/* Set up compiler-specific options for inlining functions */
#ifndef SDL_INLINE_OKAY
#ifdef __GNUC__
#define SDL_INLINE_OKAY
#else
/* Add any special compiler-specific cases here */
#if defined(_MSC_VER) || defined(__BORLANDC__) || \
    defined(__DMC__) || defined(__SC__) || \
    defined(__WATCOMC__) || defined(__LCC__) || \
    defined(__DECC)
#ifndef __inline__
#define __inline__	__inline
#endif
#define SDL_INLINE_OKAY
#else
#if !defined(__MRC__) && !defined(_SGI_SOURCE)
#ifndef __inline__
#define __inline__ inline
#endif
#define SDL_INLINE_OKAY
#endif /* Not a funky compiler */
#endif /* Visual C++ */
#endif /* GNU C */
#endif /* SDL_INLINE_OKAY */

/* If inlining isn't supported, remove "__inline__", turning static
   inlined functions into static functions (resulting in code bloat
   in all files which include the offending header files)
*/
#ifndef SDL_INLINE_OKAY
#define __inline__
#endif

/* Apparently this is needed by several Windows compilers */
#if !defined(__MACH__)
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif /* NULL */
#endif /* ! Mac OS X - breaks precompiled headers */
