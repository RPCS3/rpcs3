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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/

#ifndef _SDL_DirectFB_dyn_h
#define _SDL_DirectFB_dyn_h

#define DFB_SYMS \
	DFB_SYM(DFBResult, DirectFBError, (const char *msg, DFBResult result), (msg, result), return) \
	DFB_SYM(DFBResult, DirectFBErrorFatal, (const char *msg, DFBResult result), (msg, result), return) \
	DFB_SYM(const char *, DirectFBErrorString, (DFBResult result), (result), return) \
	DFB_SYM(const char *, DirectFBUsageString, ( void ), (), return) \
	DFB_SYM(DFBResult, DirectFBInit, (int *argc, char *(*argv[]) ), (argc, argv), return) \
	DFB_SYM(DFBResult, DirectFBSetOption, (const char *name, const char *value), (name, value), return) \
	DFB_SYM(DFBResult, DirectFBCreate, (IDirectFB **interface), (interface), return) \
	DFB_SYM(const char *, DirectFBCheckVersion, (unsigned int required_major, unsigned int required_minor, unsigned int required_micro), \
				(required_major, required_minor, required_micro), return)

int SDL_DirectFB_LoadLibrary(void);
void SDL_DirectFB_UnLoadLibrary(void);

#endif
