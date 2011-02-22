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

/* Simple error handling in SDL */

#include "SDL_log.h"
#include "SDL_error.h"
#include "SDL_error_c.h"


/* Routine to get the thread-specific error variable */
#if SDL_THREADS_DISABLED
/* The default (non-thread-safe) global error variable */
static SDL_error SDL_global_error;
#define SDL_GetErrBuf()	(&SDL_global_error)
#else
extern SDL_error *SDL_GetErrBuf(void);
#endif /* SDL_THREADS_DISABLED */

#define SDL_ERRBUFIZE	1024

/* Private functions */

static const char *
SDL_LookupString(const char *key)
{
    /* FIXME: Add code to lookup key in language string hash-table */
    return key;
}

/* Public functions */

void
SDL_SetError(const char *fmt, ...)
{
    va_list ap;
    SDL_error *error;

    /* Copy in the key, mark error as valid */
    error = SDL_GetErrBuf();
    error->error = 1;
    SDL_strlcpy((char *) error->key, fmt, sizeof(error->key));

    va_start(ap, fmt);
    error->argc = 0;
    while (*fmt) {
        if (*fmt++ == '%') {
            while (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) {
                ++fmt;
            }
            switch (*fmt++) {
            case 0:            /* Malformed format string.. */
                --fmt;
                break;
            case 'c':
            case 'i':
            case 'd':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
                error->args[error->argc++].value_i = va_arg(ap, int);
                break;
            case 'f':
                error->args[error->argc++].value_f = va_arg(ap, double);
                break;
            case 'p':
                error->args[error->argc++].value_ptr = va_arg(ap, void *);
                break;
            case 's':
                {
                    int i = error->argc;
                    const char *str = va_arg(ap, const char *);
                    if (str == NULL)
                        str = "(null)";
                    SDL_strlcpy((char *) error->args[i].buf, str,
                                ERR_MAX_STRLEN);
                    error->argc++;
                }
                break;
            default:
                break;
            }
            if (error->argc >= ERR_MAX_ARGS) {
                break;
            }
        }
    }
    va_end(ap);

    /* If we are in debug mode, print out an error message */
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
}

/* This function has a bit more overhead than most error functions
   so that it supports internationalization and thread-safe errors.
*/
static char *
SDL_GetErrorMsg(char *errstr, unsigned int maxlen)
{
    SDL_error *error;

    /* Clear the error string */
    *errstr = '\0';
    --maxlen;

    /* Get the thread-safe error, and print it out */
    error = SDL_GetErrBuf();
    if (error->error) {
        const char *fmt;
        char *msg = errstr;
        int len;
        int argi;

        fmt = SDL_LookupString(error->key);
        argi = 0;
        while (*fmt && (maxlen > 0)) {
            if (*fmt == '%') {
                char tmp[32], *spot = tmp;
                *spot++ = *fmt++;
                while ((*fmt == '.' || (*fmt >= '0' && *fmt <= '9'))
                       && spot < (tmp + SDL_arraysize(tmp) - 2)) {
                    *spot++ = *fmt++;
                }
                *spot++ = *fmt++;
                *spot++ = '\0';
                switch (spot[-2]) {
                case '%':
                    *msg++ = '%';
                    maxlen -= 1;
                    break;
                case 'c':
                case 'i':
                case 'd':
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                    len =
                        SDL_snprintf(msg, maxlen, tmp,
                                     error->args[argi++].value_i);
                    msg += len;
                    maxlen -= len;
                    break;
                case 'f':
                    len =
                        SDL_snprintf(msg, maxlen, tmp,
                                     error->args[argi++].value_f);
                    msg += len;
                    maxlen -= len;
                    break;
                case 'p':
                    len =
                        SDL_snprintf(msg, maxlen, tmp,
                                     error->args[argi++].value_ptr);
                    msg += len;
                    maxlen -= len;
                    break;
                case 's':
                    len =
                        SDL_snprintf(msg, maxlen, tmp,
                                     SDL_LookupString(error->args[argi++].
                                                      buf));
                    msg += len;
                    maxlen -= len;
                    break;
                }
            } else {
                *msg++ = *fmt++;
                maxlen -= 1;
            }
        }
        *msg = 0;               /* NULL terminate the string */
    }
    return (errstr);
}

/* Available for backwards compatibility */
const char *
SDL_GetError(void)
{
    static char errmsg[SDL_ERRBUFIZE];

    return SDL_GetErrorMsg(errmsg, SDL_ERRBUFIZE);
}

void
SDL_ClearError(void)
{
    SDL_error *error;

    error = SDL_GetErrBuf();
    error->error = 0;
}

/* Very common errors go here */
void
SDL_Error(SDL_errorcode code)
{
    switch (code) {
    case SDL_ENOMEM:
        SDL_SetError("Out of memory");
        break;
    case SDL_EFREAD:
        SDL_SetError("Error reading from datastream");
        break;
    case SDL_EFWRITE:
        SDL_SetError("Error writing to datastream");
        break;
    case SDL_EFSEEK:
        SDL_SetError("Error seeking in datastream");
        break;
    case SDL_UNSUPPORTED:
        SDL_SetError("That operation is not supported");
        break;
    default:
        SDL_SetError("Unknown SDL error");
        break;
    }
}

#ifdef TEST_ERROR
int
main(int argc, char *argv[])
{
    char buffer[BUFSIZ + 1];

    SDL_SetError("Hi there!");
    printf("Error 1: %s\n", SDL_GetError());
    SDL_ClearError();
    SDL_memset(buffer, '1', BUFSIZ);
    buffer[BUFSIZ] = 0;
    SDL_SetError("This is the error: %s (%f)", buffer, 1.0);
    printf("Error 2: %s\n", SDL_GetError());
    exit(0);
}
#endif

/* vi: set ts=4 sw=4 expandtab: */
