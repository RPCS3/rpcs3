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
 *  \file SDL_syswm.h
 *  
 *  Include file for SDL custom system window manager hooks.
 */

#ifndef _SDL_syswm_h
#define _SDL_syswm_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_version.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \file SDL_syswm.h
 *  
 *  Your application has access to a special type of event ::SDL_SYSWMEVENT,
 *  which contains window-manager specific information and arrives whenever
 *  an unhandled window event occurs.  This event is ignored by default, but
 *  you can enable it with SDL_EventState().
 */
#ifdef SDL_PROTOTYPES_ONLY
struct SDL_SysWMinfo;
#else

#if defined(SDL_VIDEO_DRIVER_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* This is the structure for custom window manager events */
#if defined(SDL_VIDEO_DRIVER_X11)
#if defined(__APPLE__) && defined(__MACH__)
/* conflicts with Quickdraw.h */
#define Cursor X11Cursor
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#if defined(__APPLE__) && defined(__MACH__)
/* matches the re-define above */
#undef Cursor
#endif

#endif /* defined(SDL_VIDEO_DRIVER_X11) */

#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
#include <directfb.h>
#endif

#if defined(SDL_VIDEO_DRIVER_COCOA)
#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#else
typedef struct _NSWindow NSWindow;
#endif
#endif

#if defined(SDL_VIDEO_DRIVER_UIKIT)
#ifdef __OBJC__
#include <UIKit/UIKit.h>
#else
typedef struct _UIWindow UIWindow;
#endif
#endif

/** 
 *  These are the various supported windowing subsystems
 */
typedef enum
{
    SDL_SYSWM_UNKNOWN,
    SDL_SYSWM_WINDOWS,
    SDL_SYSWM_X11,
    SDL_SYSWM_DIRECTFB,
    SDL_SYSWM_COCOA,
    SDL_SYSWM_UIKIT,
} SDL_SYSWM_TYPE;

/**
 *  The custom event structure.
 */
struct SDL_SysWMmsg
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct {
            HWND hwnd;                  /**< The window for the message */
            UINT msg;                   /**< The type of message */
            WPARAM wParam;              /**< WORD message parameter */
            LPARAM lParam;              /**< LONG message parameter */
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct {
            XEvent event;
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct {
            DFBEvent event;
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
            /* No Cocoa window events yet */
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
            /* No UIKit window events yet */
        } uikit;
#endif
        /* Can't have an empty union */
        int dummy;
    } msg;
};

/**
 *  The custom window manager information structure.
 *
 *  When this structure is returned, it holds information about which
 *  low level system it is using, and will be one of SDL_SYSWM_TYPE.
 */
struct SDL_SysWMinfo
{
    SDL_version version;
    SDL_SYSWM_TYPE subsystem;
    union
    {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        struct
        {
            HWND window;                /**< The window handle */
        } win;
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        struct
        {
            Display *display;           /**< The X11 display */
            Window window;              /**< The X11 window */
        } x11;
#endif
#if defined(SDL_VIDEO_DRIVER_DIRECTFB)
        struct
        {
            IDirectFB *dfb;             /**< The directfb main interface */
            IDirectFBWindow *window;    /**< The directfb window handle */
            IDirectFBSurface *surface;  /**< The directfb client surface */
        } dfb;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
        struct
        {
            NSWindow *window;           /* The Cocoa window */
        } cocoa;
#endif
#if defined(SDL_VIDEO_DRIVER_UIKIT)
        struct
        {
            UIWindow *window;           /* The UIKit window */
        } uikit;
#endif
        /* Can't have an empty union */
        int dummy;
    } info;
};

#endif /* SDL_PROTOTYPES_ONLY */

typedef struct SDL_SysWMinfo SDL_SysWMinfo;

/* Function prototypes */
/**
 *  \brief This function allows access to driver-dependent window information.
 *  
 *  \param window The window about which information is being requested
 *  \param info This structure must be initialized with the SDL version, and is 
 *              then filled in with information about the given window.
 *  
 *  \return SDL_TRUE if the function is implemented and the version member of 
 *          the \c info struct is valid, SDL_FALSE otherwise.
 *  
 *  You typically use this function like this:
 *  \code
 *  SDL_SysWMinfo info;
 *  SDL_VERSION(&info.version);
 *  if ( SDL_GetWindowWMInfo(&info) ) { ... }
 *  \endcode
 */
extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowWMInfo(SDL_Window * window,
                                                     SDL_SysWMinfo * info);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_syswm_h */

/* vi: set ts=4 sw=4 expandtab: */
