/*
    headers for MMX assembler version of SDL_MixAudio
    Copyright 2002 Stephane Marchesin (stephane.marchesin@wanadoo.fr)
    This code is licensed under the LGPL (see COPYING for details)

    Assumes buffer size in bytes is a multiple of 16
    Assumes SDL_MIX_MAXVOLUME = 128
*/
#include "SDL_config.h"

#if defined(SDL_BUGGY_MMX_MIXERS) /* buggy, so we're disabling them. --ryan. */
#if defined(__GNUC__) && defined(__i386__) && defined(SDL_ASSEMBLY_ROUTINES)
void SDL_MixAudio_MMX_S16(char *, char *, unsigned int, int);
void SDL_MixAudio_MMX_S8(char *, char *, unsigned int, int);
#endif
#endif /* SDL_BUGGY_MMX_MIXERS */
/* vi: set ts=4 sw=4 expandtab: */
