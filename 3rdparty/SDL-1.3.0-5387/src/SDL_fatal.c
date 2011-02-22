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

/* General fatal signal handling code for SDL */

#ifdef HAVE_SIGNAL_H

#include <signal.h>

#include "SDL.h"
#include "SDL_fatal.h"

/* This installs some signal handlers for the more common fatal signals,
   so that if the programmer is lazy, the app doesn't die so horribly if
   the program crashes.
*/

static void
SDL_Parachute(int sig)
{
    signal(sig, SIG_DFL);
    SDL_Quit();
    raise(sig);
}

static const int SDL_fatal_signals[] = {
    SIGSEGV,
#ifdef SIGBUS
    SIGBUS,
#endif
#ifdef SIGFPE
    SIGFPE,
#endif
#ifdef SIGQUIT
    SIGQUIT,
#endif
    0
};

void
SDL_InstallParachute(void)
{
    /* Set a handler for any fatal signal not already handled */
    int i;
#ifdef HAVE_SIGACTION
    struct sigaction action;

    for (i = 0; SDL_fatal_signals[i]; ++i) {
        sigaction(SDL_fatal_signals[i], NULL, &action);
        if (action.sa_handler == SIG_DFL) {
            action.sa_handler = SDL_Parachute;
            sigaction(SDL_fatal_signals[i], &action, NULL);
        }
    }
#ifdef SIGALRM
    /* Set SIGALRM to be ignored -- necessary on Solaris */
    sigaction(SIGALRM, NULL, &action);
    if (action.sa_handler == SIG_DFL) {
        action.sa_handler = SIG_IGN;
        sigaction(SIGALRM, &action, NULL);
    }
#endif
#else
    void (*ohandler) (int);

    for (i = 0; SDL_fatal_signals[i]; ++i) {
        ohandler = signal(SDL_fatal_signals[i], SDL_Parachute);
        if (ohandler != SIG_DFL) {
            signal(SDL_fatal_signals[i], ohandler);
        }
    }
#endif /* HAVE_SIGACTION */
    return;
}

void
SDL_UninstallParachute(void)
{
    /* Remove a handler for any fatal signal handled */
    int i;
#ifdef HAVE_SIGACTION
    struct sigaction action;

    for (i = 0; SDL_fatal_signals[i]; ++i) {
        sigaction(SDL_fatal_signals[i], NULL, &action);
        if (action.sa_handler == SDL_Parachute) {
            action.sa_handler = SIG_DFL;
            sigaction(SDL_fatal_signals[i], &action, NULL);
        }
    }
#else
    void (*ohandler) (int);

    for (i = 0; SDL_fatal_signals[i]; ++i) {
        ohandler = signal(SDL_fatal_signals[i], SIG_DFL);
        if (ohandler != SDL_Parachute) {
            signal(SDL_fatal_signals[i], ohandler);
        }
    }
#endif /* HAVE_SIGACTION */
}

#else

/* No signals on this platform, nothing to do.. */

void
SDL_InstallParachute(void)
{
    return;
}

void
SDL_UninstallParachute(void)
{
    return;
}

#endif /* HAVE_SIGNAL_H */
/* vi: set ts=4 sw=4 expandtab: */
