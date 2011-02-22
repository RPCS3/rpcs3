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
 *  \file SDL_version.h
 *  
 *  This header defines the current SDL version.
 */

#ifndef _SDL_version_h
#define _SDL_version_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief Information the version of SDL in use.
 *  
 *  Represents the library's version as three levels: major revision
 *  (increments with massive changes, additions, and enhancements),
 *  minor revision (increments with backwards-compatible changes to the
 *  major revision), and patchlevel (increments with fixes to the minor
 *  revision).
 *  
 *  \sa SDL_VERSION
 *  \sa SDL_GetVersion
 */
typedef struct SDL_version
{
    Uint8 major;        /**< major version */
    Uint8 minor;        /**< minor version */
    Uint8 patch;        /**< update version */
} SDL_version;

/* Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
*/
#define SDL_MAJOR_VERSION	1
#define SDL_MINOR_VERSION	3
#define SDL_PATCHLEVEL		0

/**
 *  \brief Macro to determine SDL version program was compiled against.
 *  
 *  This macro fills in a SDL_version structure with the version of the
 *  library you compiled against. This is determined by what header the
 *  compiler uses. Note that if you dynamically linked the library, you might
 *  have a slightly newer or older version at runtime. That version can be
 *  determined with SDL_GetVersion(), which, unlike SDL_VERSION(),
 *  is not a macro.
 *  
 *  \param x A pointer to a SDL_version struct to initialize.
 *  
 *  \sa SDL_version
 *  \sa SDL_GetVersion
 */
#define SDL_VERSION(x)							\
{									\
	(x)->major = SDL_MAJOR_VERSION;					\
	(x)->minor = SDL_MINOR_VERSION;					\
	(x)->patch = SDL_PATCHLEVEL;					\
}

/**
 *  This macro turns the version numbers into a numeric value:
 *  \verbatim
    (1,2,3) -> (1203)
    \endverbatim
 *  
 *  This assumes that there will never be more than 100 patchlevels.
 */
#define SDL_VERSIONNUM(X, Y, Z)						\
	((X)*1000 + (Y)*100 + (Z))

/**
 *  This is the version number macro for the current SDL version.
 */
#define SDL_COMPILEDVERSION \
	SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)

/**
 *  This macro will evaluate to true if compiled with SDL at least X.Y.Z.
 */
#define SDL_VERSION_ATLEAST(X, Y, Z) \
	(SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

/**
 *  \brief Get the version of SDL that is linked against your program.
 *
 *  If you are linking to SDL dynamically, then it is possible that the
 *  current version will be different than the version you compiled against.
 *  This function returns the current version, while SDL_VERSION() is a
 *  macro that tells you what version you compiled with.
 *  
 *  \code
 *  SDL_version compiled;
 *  SDL_version linked;
 *  
 *  SDL_VERSION(&compiled);
 *  SDL_GetVersion(&linked);
 *  printf("We compiled against SDL version %d.%d.%d ...\n",
 *         compiled.major, compiled.minor, compiled.patch);
 *  printf("But we linked against SDL version %d.%d.%d.\n",
 *         linked.major, linked.minor, linked.patch);
 *  \endcode
 *  
 *  This function may be called safely at any time, even before SDL_Init().
 *  
 *  \sa SDL_VERSION
 */
extern DECLSPEC void SDLCALL SDL_GetVersion(SDL_version * ver);

/**
 *  \brief Get the code revision of SDL that is linked against your program.
 *
 *  Returns an arbitrary string (a hash value) uniquely identifying the
 *  exact revision of the SDL library in use, and is only useful in comparing
 *  against other revisions. It is NOT an incrementing number.
 */
extern DECLSPEC const char *SDLCALL SDL_GetRevision(void);

/**
 *  \brief Get the revision number of SDL that is linked against your program.
 *
 *  Returns a number uniquely identifying the exact revision of the SDL
 *  library in use. It is an incrementing number based on commits to
 *  hg.libsdl.org.
 */
extern DECLSPEC int SDLCALL SDL_GetRevisionNumber(void);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_version_h */

/* vi: set ts=4 sw=4 expandtab: */
