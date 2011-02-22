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

/* General quit handling code for SDL */

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "SDL_events.h"
#include "SDL_events_c.h"


#ifdef HAVE_SIGNAL_H
static void
SDL_HandleSIG(int sig)
{
    /* Reset the signal handler */
    signal(sig, SDL_HandleSIG);

    /* Signal a quit interrupt */
    SDL_SendQuit();
}
#endif /* HAVE_SIGNAL_H */

/* Public functions */
int
SDL_QuitInit(void)
{
#ifdef HAVE_SIGNAL_H
    void (*ohandler) (int);

    /* Both SIGINT and SIGTERM are translated into quit interrupts */
    ohandler = signal(SIGINT, SDL_HandleSIG);
    if (ohandler != SIG_DFL)
        signal(SIGINT, ohandler);
    ohandler = signal(SIGTERM, SDL_HandleSIG);
    if (ohandler != SIG_DFL)
        signal(SIGTERM, ohandler);
#endif /* HAVE_SIGNAL_H */

    /* That's it! */
    return (0);
}

void
SDL_QuitQuit(void)
{
#ifdef HAVE_SIGNAL_H
    void (*ohandler) (int);

    ohandler = signal(SIGINT, SIG_DFL);
    if (ohandler != SDL_HandleSIG)
        signal(SIGINT, ohandler);
    ohandler = signal(SIGTERM, SIG_DFL);
    if (ohandler != SDL_HandleSIG)
        signal(SIGTERM, ohandler);
#endif /* HAVE_SIGNAL_H */
}

/* This function returns 1 if it's okay to close the application window */
int
SDL_SendQuit(void)
{
    int posted;

    posted = 0;
    if (SDL_GetEventState(SDL_QUIT) == SDL_ENABLE) {
        SDL_Event event;
        event.type = SDL_QUIT;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

/* vi: set ts=4 sw=4 expandtab: */
