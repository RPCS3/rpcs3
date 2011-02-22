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

/**
 *  \file SDL_log.h
 *  
 *  Simple log messages with categories and priorities.
 *
 *  By default logs are quiet, but if you're debugging SDL you might want:
 *
 *      SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
 *
 *  Here's where the messages go on different platforms:
 *      Windows: debug output stream
 *      Android: log output
 *      Others: standard error output (stderr)
 */

#ifndef _SDL_log_h
#define _SDL_log_h

#include "SDL_stdinc.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif


/**
 *  \brief The maximum size of a log message
 *
 *  Messages longer than the maximum size will be truncated
 */
#define SDL_MAX_LOG_MESSAGE 4096

/**
 *  \brief The predefined log categories
 *
 *  By default the application category is enabled at the INFO level,
 *  and all other categories are enabled at the CRITICAL level.
 */
enum
{
    SDL_LOG_CATEGORY_APPLICATION,
    SDL_LOG_CATEGORY_ERROR,
    SDL_LOG_CATEGORY_SYSTEM,
    SDL_LOG_CATEGORY_AUDIO,
    SDL_LOG_CATEGORY_VIDEO,
    SDL_LOG_CATEGORY_RENDER,
    SDL_LOG_CATEGORY_INPUT,

    /* Reserved for future SDL library use */
    SDL_LOG_CATEGORY_RESERVED1,
    SDL_LOG_CATEGORY_RESERVED2,
    SDL_LOG_CATEGORY_RESERVED3,
    SDL_LOG_CATEGORY_RESERVED4,
    SDL_LOG_CATEGORY_RESERVED5,
    SDL_LOG_CATEGORY_RESERVED6,
    SDL_LOG_CATEGORY_RESERVED7,
    SDL_LOG_CATEGORY_RESERVED8,
    SDL_LOG_CATEGORY_RESERVED9,
    SDL_LOG_CATEGORY_RESERVED10,

    /* Beyond this point is reserved for application use, e.g.
       enum {
           MYAPP_CATEGORY_AWESOME1 = SDL_LOG_CATEGORY_CUSTOM,
           MYAPP_CATEGORY_AWESOME2,
           MYAPP_CATEGORY_AWESOME3,
           ...
       };
     */
    SDL_LOG_CATEGORY_CUSTOM
};

/**
 *  \brief The predefined log priorities
 */
typedef enum
{
    SDL_LOG_PRIORITY_VERBOSE = 1,
    SDL_LOG_PRIORITY_DEBUG,
    SDL_LOG_PRIORITY_INFO,
    SDL_LOG_PRIORITY_WARN,
    SDL_LOG_PRIORITY_ERROR,
    SDL_LOG_PRIORITY_CRITICAL,
    SDL_NUM_LOG_PRIORITIES
} SDL_LogPriority;


/**
 *  \brief Set the priority of all log categories
 */
extern DECLSPEC void SDLCALL SDL_LogSetAllPriority(SDL_LogPriority priority);

/**
 *  \brief Set the priority of a particular log category
 */
extern DECLSPEC void SDLCALL SDL_LogSetPriority(int category,
                                                SDL_LogPriority priority);

/**
 *  \brief Set the priority of a particular log category
 */
extern DECLSPEC SDL_LogPriority SDLCALL SDL_LogGetPriority(int category);

/**
 *  \brief Reset all priorities to default.
 *
 *  \note This is called in SDL_Quit().
 */
extern DECLSPEC void SDLCALL SDL_LogResetPriorities(void);

/**
 *  \brief Log a message with SDL_LOG_CATEGORY_APPLICATION and SDL_LOG_PRIORITY_INFO
 */
extern DECLSPEC void SDLCALL SDL_Log(const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_VERBOSE
 */
extern DECLSPEC void SDLCALL SDL_LogVerbose(int category, const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_DEBUG
 */
extern DECLSPEC void SDLCALL SDL_LogDebug(int category, const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_INFO
 */
extern DECLSPEC void SDLCALL SDL_LogInfo(int category, const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_WARN
 */
extern DECLSPEC void SDLCALL SDL_LogWarn(int category, const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_ERROR
 */
extern DECLSPEC void SDLCALL SDL_LogError(int category, const char *fmt, ...);

/**
 *  \brief Log a message with SDL_LOG_PRIORITY_CRITICAL
 */
extern DECLSPEC void SDLCALL SDL_LogCritical(int category, const char *fmt, ...);

/**
 *  \brief Log a message with the specified category and priority.
 */
extern DECLSPEC void SDLCALL SDL_LogMessage(int category,
                                            SDL_LogPriority priority,
                                            const char *fmt, ...);

/**
 *  \brief Log a message with the specified category and priority.
 */
extern DECLSPEC void SDLCALL SDL_LogMessageV(int category,
                                             SDL_LogPriority priority,
                                             const char *fmt, va_list ap);

/**
 *  \brief The prototype for the log output function
 */
typedef void (*SDL_LogOutputFunction)(void *userdata, int category, SDL_LogPriority priority, const char *message);

/**
 *  \brief Get the current log output function.
 */
extern DECLSPEC void SDLCALL SDL_LogGetOutputFunction(SDL_LogOutputFunction *callback, void **userdata);

/**
 *  \brief This function allows you to replace the default log output
 *         function with one of your own.
 */
extern DECLSPEC void SDLCALL SDL_LogSetOutputFunction(SDL_LogOutputFunction callback, void *userdata);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_log_h */

/* vi: set ts=4 sw=4 expandtab: */
