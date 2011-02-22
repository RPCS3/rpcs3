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

#ifndef _SDL_thread_c_h
#define _SDL_thread_c_h

/* Need the definitions of SYS_ThreadHandle */
#if SDL_THREADS_DISABLED
#include "generic/SDL_systhread_c.h"
#elif SDL_THREAD_BEOS
#include "beos/SDL_systhread_c.h"
#elif SDL_THREAD_EPOC
#include "epoc/SDL_systhread_c.h"
#elif SDL_THREAD_PTHREAD
#include "pthread/SDL_systhread_c.h"
#elif SDL_THREAD_SPROC
#include "irix/SDL_systhread_c.h"
#elif SDL_THREAD_WINDOWS
#include "windows/SDL_systhread_c.h"
#elif SDL_THREAD_NDS
#include "nds/SDL_systhread_c.h"
#else
#error Need thread implementation for this platform
#include "generic/SDL_systhread_c.h"
#endif
#include "../SDL_error_c.h"

/* This is the system-independent thread info structure */
struct SDL_Thread
{
    SDL_threadID threadid;
    SYS_ThreadHandle handle;
    int status;
    SDL_error errbuf;
    void *data;
};

/* This is the function called to run a thread */
extern void SDL_RunThread(void *data);

#endif /* _SDL_thread_c_h */
/* vi: set ts=4 sw=4 expandtab: */
