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

#include "SDL_stdinc.h"

#ifndef HAVE_GETENV

#if defined(__WIN32__) && !defined(_WIN32_WCE)

#include "../core/windows/SDL_windows.h"

/* Note this isn't thread-safe! */

static char *SDL_envmem = NULL; /* Ugh, memory leak */
static size_t SDL_envmemlen = 0;

/* Put a variable into the environment */
int
SDL_setenv(const char *name, const char *value, int overwrite)
{
    if (!overwrite) {
        char ch = 0;
        const size_t len = GetEnvironmentVariableA(name, &ch, sizeof (ch));
        if (len > 0) {
            return 0;  /* asked not to overwrite existing value. */
        }
    }
    if (!SetEnvironmentVariableA(name, *value ? value : NULL)) {
        return -1;
    }
    return 0;
}

/* Retrieve a variable named "name" from the environment */
char *
SDL_getenv(const char *name)
{
    size_t bufferlen;

    bufferlen =
        GetEnvironmentVariableA(name, SDL_envmem, (DWORD) SDL_envmemlen);
    if (bufferlen == 0) {
        return NULL;
    }
    if (bufferlen > SDL_envmemlen) {
        char *newmem = (char *) SDL_realloc(SDL_envmem, bufferlen);
        if (newmem == NULL) {
            return NULL;
        }
        SDL_envmem = newmem;
        SDL_envmemlen = bufferlen;
        GetEnvironmentVariableA(name, SDL_envmem, (DWORD) SDL_envmemlen);
    }
    return SDL_envmem;
}

#else /* roll our own */

static char **SDL_env = (char **) 0;

/* Put a variable into the environment */
int
SDL_setenv(const char *name, const char *value, int overwrite)
{
    int added;
    int len, i;
    char **new_env;
    char *new_variable;

    /* A little error checking */
    if (!name || !value) {
        return (-1);
    }

    /* See if it already exists */
    if (!overwrite && SDL_getenv(name)) {
        return 0;
    }

    /* Allocate memory for the variable */
    len = SDL_strlen(name) + SDL_strlen(value) + 2;
    new_variable = (char *) SDL_malloc(len);
    if (!new_variable) {
        return (-1);
    }

    SDL_snprintf(new_variable, len, "%s=%s", name, value);
    value = new_variable + SDL_strlen(name) + 1;
    name = new_variable;

    /* Actually put it into the environment */
    added = 0;
    i = 0;
    if (SDL_env) {
        /* Check to see if it's already there... */
        len = (value - name);
        for (; SDL_env[i]; ++i) {
            if (SDL_strncmp(SDL_env[i], name, len) == 0) {
                break;
            }
        }
        /* If we found it, just replace the entry */
        if (SDL_env[i]) {
            SDL_free(SDL_env[i]);
            SDL_env[i] = new_variable;
            added = 1;
        }
    }

    /* Didn't find it in the environment, expand and add */
    if (!added) {
        new_env = SDL_realloc(SDL_env, (i + 2) * sizeof(char *));
        if (new_env) {
            SDL_env = new_env;
            SDL_env[i++] = new_variable;
            SDL_env[i++] = (char *) 0;
            added = 1;
        } else {
            SDL_free(new_variable);
        }
    }
    return (added ? 0 : -1);
}

/* Retrieve a variable named "name" from the environment */
char *
SDL_getenv(const char *name)
{
    int len, i;
    char *value;

    value = (char *) 0;
    if (SDL_env) {
        len = SDL_strlen(name);
        for (i = 0; SDL_env[i] && !value; ++i) {
            if ((SDL_strncmp(SDL_env[i], name, len) == 0) &&
                (SDL_env[i][len] == '=')) {
                value = &SDL_env[i][len + 1];
            }
        }
    }
    return value;
}

#endif /* __WIN32__ */

#endif /* !HAVE_GETENV */


/* We have a real environment table, but no real setenv? Fake it w/ putenv. */
#if (defined(HAVE_GETENV) && defined(HAVE_PUTENV) && !defined(HAVE_SETENV))
int
SDL_setenv(const char *name, const char *value, int overwrite)
{
    size_t len;
    char *new_variable;

    if (getenv(name) != NULL) {
        if (overwrite) {
            unsetenv(name);
        } else {
            return 0;  /* leave the existing one there. */
        }
    }

    /* This leaks. Sorry. Get a better OS so we don't have to do this. */
    len = SDL_strlen(name) + SDL_strlen(value) + 2;
    new_variable = (char *) SDL_malloc(len);
    if (!new_variable) {
        return (-1);
    }

    SDL_snprintf(new_variable, len, "%s=%s", name, value);
    return putenv(new_variable);
}
#endif


#ifdef TEST_MAIN
#include <stdio.h>

int
main(int argc, char *argv[])
{
    char *value;

    printf("Checking for non-existent variable... ");
    fflush(stdout);
    if (!SDL_getenv("EXISTS")) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Setting FIRST=VALUE1 in the environment... ");
    fflush(stdout);
    if (SDL_setenv("FIRST", "VALUE1", 0) == 0) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Getting FIRST from the environment... ");
    fflush(stdout);
    value = SDL_getenv("FIRST");
    if (value && (SDL_strcmp(value, "VALUE1") == 0)) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Setting SECOND=VALUE2 in the environment... ");
    fflush(stdout);
    if (SDL_setenv("SECOND", "VALUE2", 0) == 0) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Getting SECOND from the environment... ");
    fflush(stdout);
    value = SDL_getenv("SECOND");
    if (value && (SDL_strcmp(value, "VALUE2") == 0)) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Setting FIRST=NOVALUE in the environment... ");
    fflush(stdout);
    if (SDL_setenv("FIRST", "NOVALUE", 1) == 0) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Getting FIRST from the environment... ");
    fflush(stdout);
    value = SDL_getenv("FIRST");
    if (value && (SDL_strcmp(value, "NOVALUE") == 0)) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    printf("Checking for non-existent variable... ");
    fflush(stdout);
    if (!SDL_getenv("EXISTS")) {
        printf("okay\n");
    } else {
        printf("failed\n");
    }
    return (0);
}
#endif /* TEST_MAIN */

/* vi: set ts=4 sw=4 expandtab: */
