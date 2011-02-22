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
#include "SDL_config.h"

/* Initialization code for SDL */

#include "SDL.h"
#include "SDL_revision.h"
#include "SDL_fatal.h"
#include "SDL_assert_c.h"
#include "haptic/SDL_haptic_c.h"
#include "joystick/SDL_joystick_c.h"

/* Initialization/Cleanup routines */
#if !SDL_TIMERS_DISABLED
extern void SDL_StartTicks(void);
extern int SDL_TimerInit(void);
extern void SDL_TimerQuit(void);
#endif
#if defined(__WIN32__)
extern int SDL_HelperWindowCreate(void);
extern int SDL_HelperWindowDestroy(void);
#endif


/* The initialized subsystems */
static Uint32 SDL_initialized = 0;
static Uint32 ticks_started = 0;


int
SDL_InitSubSystem(Uint32 flags)
{
#if !SDL_VIDEO_DISABLED
    /* Initialize the video/event subsystem */
    if ((flags & SDL_INIT_VIDEO) && !(SDL_initialized & SDL_INIT_VIDEO)) {
        if (SDL_VideoInit(NULL) < 0) {
            return (-1);
        }
        SDL_initialized |= SDL_INIT_VIDEO;
    }
#else
    if (flags & SDL_INIT_VIDEO) {
        SDL_SetError("SDL not built with video support");
        return (-1);
    }
#endif

#if !SDL_AUDIO_DISABLED
    /* Initialize the audio subsystem */
    if ((flags & SDL_INIT_AUDIO) && !(SDL_initialized & SDL_INIT_AUDIO)) {
        if (SDL_AudioInit(NULL) < 0) {
            return (-1);
        }
        SDL_initialized |= SDL_INIT_AUDIO;
    }
#else
    if (flags & SDL_INIT_AUDIO) {
        SDL_SetError("SDL not built with audio support");
        return (-1);
    }
#endif

#if !SDL_TIMERS_DISABLED
    /* Initialize the timer subsystem */
    if (!ticks_started) {
        SDL_StartTicks();
        ticks_started = 1;
    }
    if ((flags & SDL_INIT_TIMER) && !(SDL_initialized & SDL_INIT_TIMER)) {
        if (SDL_TimerInit() < 0) {
            return (-1);
        }
        SDL_initialized |= SDL_INIT_TIMER;
    }
#else
    if (flags & SDL_INIT_TIMER) {
        SDL_SetError("SDL not built with timer support");
        return (-1);
    }
#endif

#if !SDL_JOYSTICK_DISABLED
    /* Initialize the joystick subsystem */
    if ((flags & SDL_INIT_JOYSTICK) && !(SDL_initialized & SDL_INIT_JOYSTICK)) {
        if (SDL_JoystickInit() < 0) {
            return (-1);
        }
        SDL_initialized |= SDL_INIT_JOYSTICK;
    }
#else
    if (flags & SDL_INIT_JOYSTICK) {
        SDL_SetError("SDL not built with joystick support");
        return (-1);
    }
#endif

#if !SDL_HAPTIC_DISABLED
    /* Initialize the haptic subsystem */
    if ((flags & SDL_INIT_HAPTIC) && !(SDL_initialized & SDL_INIT_HAPTIC)) {
        if (SDL_HapticInit() < 0) {
            return (-1);
        }
        SDL_initialized |= SDL_INIT_HAPTIC;
    }
#else
    if (flags & SDL_INIT_HAPTIC) {
        SDL_SetError("SDL not built with haptic (force feedback) support");
        return (-1);
    }
#endif
    return (0);
}

int
SDL_Init(Uint32 flags)
{
    if (SDL_AssertionsInit() < 0) {
        return -1;
    }

    /* Clear the error message */
    SDL_ClearError();

#if defined(__WIN32__)
    if (SDL_HelperWindowCreate() < 0) {
        return -1;
    }
#endif

    /* Initialize the desired subsystems */
    if (SDL_InitSubSystem(flags) < 0) {
        return (-1);
    }

    /* Everything is initialized */
    if (!(flags & SDL_INIT_NOPARACHUTE)) {
        SDL_InstallParachute();
    }

    return (0);
}

void
SDL_QuitSubSystem(Uint32 flags)
{
    /* Shut down requested initialized subsystems */
#if !SDL_JOYSTICK_DISABLED
    if ((flags & SDL_initialized & SDL_INIT_JOYSTICK)) {
        SDL_JoystickQuit();
        SDL_initialized &= ~SDL_INIT_JOYSTICK;
    }
#endif
#if !SDL_HAPTIC_DISABLED
    if ((flags & SDL_initialized & SDL_INIT_HAPTIC)) {
        SDL_HapticQuit();
        SDL_initialized &= ~SDL_INIT_HAPTIC;
    }
#endif
#if !SDL_TIMERS_DISABLED
    if ((flags & SDL_initialized & SDL_INIT_TIMER)) {
        SDL_TimerQuit();
        SDL_initialized &= ~SDL_INIT_TIMER;
    }
#endif
#if !SDL_AUDIO_DISABLED
    if ((flags & SDL_initialized & SDL_INIT_AUDIO)) {
        SDL_AudioQuit();
        SDL_initialized &= ~SDL_INIT_AUDIO;
    }
#endif
#if !SDL_VIDEO_DISABLED
    if ((flags & SDL_initialized & SDL_INIT_VIDEO)) {
        SDL_VideoQuit();
        SDL_initialized &= ~SDL_INIT_VIDEO;
    }
#endif
}

Uint32
SDL_WasInit(Uint32 flags)
{
    if (!flags) {
        flags = SDL_INIT_EVERYTHING;
    }
    return (SDL_initialized & flags);
}

void
SDL_Quit(void)
{
    /* Quit all subsystems */
#if defined(__WIN32__)
    SDL_HelperWindowDestroy();
#endif
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);

    /* Uninstall any parachute signal handlers */
    SDL_UninstallParachute();

    SDL_ClearHints();
    SDL_AssertionsQuit();
    SDL_LogResetPriorities();
}

/* Get the library version number */
void
SDL_GetVersion(SDL_version * ver)
{
    SDL_VERSION(ver);
}

/* Get the library source revision */
const char *
SDL_GetRevision(void)
{
    return SDL_REVISION;
}

/* Get the library source revision number */
int
SDL_GetRevisionNumber(void)
{
    return SDL_REVISION_NUMBER;
}

/* Get the name of the platform */
const char *
SDL_GetPlatform()
{
#if __AIX__
    return "AIX";
#elif __HAIKU__
/* Haiku must appear here before BeOS, since it also defines __BEOS__ */
    return "Haiku";
#elif __BEOS__
    return "BeOS";
#elif __BSDI__
    return "BSDI";
#elif __DREAMCAST__
    return "Dreamcast";
#elif __FREEBSD__
    return "FreeBSD";
#elif __HPUX__
    return "HP-UX";
#elif __IRIX__
    return "Irix";
#elif __LINUX__
    return "Linux";
#elif __MINT__
    return "Atari MiNT";
#elif __MACOS__
    return "MacOS Classic";
#elif __MACOSX__
    return "Mac OS X";
#elif __NETBSD__
    return "NetBSD";
#elif __NDS__
    return "Nintendo DS";
#elif __OPENBSD__
    return "OpenBSD";
#elif __OS2__
    return "OS/2";
#elif __OSF__
    return "OSF/1";
#elif __QNXNTO__
    return "QNX Neutrino";
#elif __RISCOS__
    return "RISC OS";
#elif __SOLARIS__
    return "Solaris";
#elif __WIN32__
#ifdef _WIN32_WCE
    return "Windows CE";
#else
    return "Windows";
#endif
#elif __IPHONEOS__
    return "iPhone OS";
#else
    return "Unknown (see SDL_platform.h)";
#endif
}

#if defined(__WIN32__)

#if !defined(HAVE_LIBC) || (defined(__WATCOMC__) && defined(BUILD_DLL))
/* Need to include DllMain() on Watcom C for some reason.. */
#include "core/windows/SDL_windows.h"

BOOL APIENTRY
_DllMainCRTStartup(HANDLE hModule,
                   DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif /* building DLL with Watcom C */

#endif /* __WIN32__ */

/* vi: set ts=4 sw=4 expandtab: */
