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

#if defined(SDL_BUGGY_MMX_MIXERS) /* buggy, so we're disabling them. --ryan. */
#if ((defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)) && defined(SDL_ASSEMBLY_ROUTINES)
/* headers for MMX assembler version of SDL_MixAudio
   Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
   Converted to Intel ASM notation by Cth
   This code is licensed under the LGPL (see COPYING for details)
   
   Assumes buffer size in bytes is a multiple of 16
   Assumes SDL_MIX_MAXVOLUME = 128
*/
void SDL_MixAudio_MMX_S16_VC(char *, char *, unsigned int, int);
void SDL_MixAudio_MMX_S8_VC(char *, char *, unsigned int, int);
#endif
#endif /* SDL_BUGGY_MMX_MIXERS */

/* vi: set ts=4 sw=4 expandtab: */
