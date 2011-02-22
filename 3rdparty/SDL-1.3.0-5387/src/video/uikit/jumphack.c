/*
 *  jumphack.c
 *  SDLiPhoneOS
 *
 */

#include "jumphack.h"

/* see SDL_uikitevents.m for more info */

/* stores the information we need to jump back */
jmp_buf env;

/* returns the jump environment for setting / getting purposes */
jmp_buf *
jump_env(void)
{
    return &env;
}
