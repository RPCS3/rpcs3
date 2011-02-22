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

#include "SDL_hints.h"


/* Assuming there aren't many hints set and they aren't being queried in
   critical performance paths, we'll just use a linked list here.
 */
typedef struct SDL_Hint {
    char *name;
    char *value;
    SDL_HintPriority priority;
    struct SDL_Hint *next;
} SDL_Hint;

static SDL_Hint *SDL_hints;


SDL_bool
SDL_SetHintWithPriority(const char *name, const char *value,
                        SDL_HintPriority priority)
{
    const char *env;
    SDL_Hint *prev, *hint;

    if (!name || !value) {
        return SDL_FALSE;
    }

    env = SDL_getenv(name);
    if (env && priority < SDL_HINT_OVERRIDE) {
        return SDL_FALSE;
    }

    prev = NULL;
    for (hint = SDL_hints; hint; prev = hint, hint = hint->next) {
        if (SDL_strcmp(name, hint->name) == 0) {
            if (priority < hint->priority) {
                return SDL_FALSE;
            }
            if (SDL_strcmp(hint->value, value) != 0) {
                SDL_free(hint->value);
                hint->value = SDL_strdup(value);
            }
            hint->priority = priority;
            return SDL_TRUE;
        }
    }

    /* Couldn't find the hint, add a new one */
    hint = (SDL_Hint *)SDL_malloc(sizeof(*hint));
    if (!hint) {
        return SDL_FALSE;
    }
    hint->name = SDL_strdup(name);
    hint->value = SDL_strdup(value);
    hint->priority = priority;
    hint->next = SDL_hints;
    SDL_hints = hint;
    return SDL_TRUE;
}

SDL_bool
SDL_SetHint(const char *name, const char *value)
{
    return SDL_SetHintWithPriority(name, value, SDL_HINT_NORMAL);
}

const char *
SDL_GetHint(const char *name)
{
    const char *env;
    SDL_Hint *hint;

    env = SDL_getenv(name);
    for (hint = SDL_hints; hint; hint = hint->next) {
        if (SDL_strcmp(name, hint->name) == 0) {
            if (!env || hint->priority == SDL_HINT_OVERRIDE) {
                return hint->value;
            }
            break;
        }
    }
    return env;
}

void SDL_ClearHints(void)
{
    SDL_Hint *hint;

    while (SDL_hints) {
        hint = SDL_hints;
        SDL_hints = hint->next;

        SDL_free(hint->name);
        SDL_free(hint->value);
        SDL_free(hint);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
