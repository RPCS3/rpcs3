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

/* Simple log messages in SDL */

#include "SDL_log.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if defined(__WIN32__)
#include "core/windows/SDL_windows.h"
#elif defined(__ANDROID__)
#include <android/log.h>
#endif

#define DEFAULT_PRIORITY                SDL_LOG_PRIORITY_CRITICAL
#define DEFAULT_APPLICATION_PRIORITY    SDL_LOG_PRIORITY_INFO

typedef struct SDL_LogLevel
{
    int category;
    SDL_LogPriority priority;
    struct SDL_LogLevel *next;
} SDL_LogLevel;

/* The default log output function */
static void SDL_LogOutput(void *userdata,
                          int category, SDL_LogPriority priority,
                          const char *message);

static SDL_LogLevel *SDL_loglevels;
static SDL_LogPriority SDL_application_priority = DEFAULT_APPLICATION_PRIORITY;
static SDL_LogPriority SDL_default_priority = DEFAULT_PRIORITY;
static SDL_LogOutputFunction SDL_log_function = SDL_LogOutput;
static void *SDL_log_userdata = NULL;

static const char *SDL_priority_prefixes[SDL_NUM_LOG_PRIORITIES] = {
    NULL,
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRITICAL"
};

#ifdef __ANDROID__
static const char *SDL_category_prefixes[SDL_LOG_CATEGORY_RESERVED1] = {
    "APP",
    "ERROR",
    "SYSTEM",
    "AUDIO",
    "VIDEO",
    "RENDER",
    "INPUT"
};

static int SDL_android_priority[SDL_NUM_LOG_PRIORITIES] = {
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL
};
#endif /* __ANDROID__ */


void
SDL_LogSetAllPriority(SDL_LogPriority priority)
{
    SDL_LogLevel *entry;

    for (entry = SDL_loglevels; entry; entry = entry->next) {
        entry->priority = priority;
    }
    SDL_application_priority = SDL_default_priority = priority;
}

void
SDL_LogSetPriority(int category, SDL_LogPriority priority)
{
    SDL_LogLevel *entry;

    for (entry = SDL_loglevels; entry; entry = entry->next) {
        if (entry->category == category) {
            entry->priority = priority;
            return;
        }
    }

    /* Create a new entry */
    entry = (SDL_LogLevel *)SDL_malloc(sizeof(*entry));
    if (entry) {
        entry->category = category;
        entry->priority = priority;
        entry->next = SDL_loglevels;
        SDL_loglevels = entry;
    }
}

SDL_LogPriority
SDL_LogGetPriority(int category)
{
    SDL_LogLevel *entry;

    for (entry = SDL_loglevels; entry; entry = entry->next) {
        if (entry->category == category) {
            return entry->priority;
        }
    }

    if (category == SDL_LOG_CATEGORY_APPLICATION) {
        return SDL_application_priority;
    } else {
        return SDL_default_priority;
    }
}

void
SDL_LogResetPriorities(void)
{
    SDL_LogLevel *entry;

    while (SDL_loglevels) {
        entry = SDL_loglevels;
        SDL_loglevels = entry->next;
        SDL_free(entry);
    }

    SDL_application_priority = DEFAULT_APPLICATION_PRIORITY;
    SDL_default_priority = DEFAULT_PRIORITY;
}

void
SDL_Log(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

void
SDL_LogVerbose(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_VERBOSE, fmt, ap);
    va_end(ap);
}

void
SDL_LogDebug(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_DEBUG, fmt, ap);
    va_end(ap);
}

void
SDL_LogInfo(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

void
SDL_LogWarn(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_WARN, fmt, ap);
    va_end(ap);
}

void
SDL_LogError(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_ERROR, fmt, ap);
    va_end(ap);
}

void
SDL_LogCritical(int category, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, SDL_LOG_PRIORITY_CRITICAL, fmt, ap);
    va_end(ap);
}

void
SDL_LogMessage(int category, SDL_LogPriority priority, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(category, priority, fmt, ap);
    va_end(ap);
}

#ifdef __ANDROID__
static const char *
GetCategoryPrefix(int category)
{
    if (category < SDL_LOG_CATEGORY_RESERVED1) {
        return SDL_category_prefixes[category];
    }
    if (category < SDL_LOG_CATEGORY_CUSTOM) {
        return "RESERVED";
    }
    return "CUSTOM";
}
#endif /* __ANDROID__ */

void
SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap)
{
    char *message;

    /* Nothing to do if we don't have an output function */
    if (!SDL_log_function) {
        return;
    }

    /* Make sure we don't exceed array bounds */
    if (priority < 0 || priority >= SDL_NUM_LOG_PRIORITIES) {
        return;
    }

    /* See if we want to do anything with this message */
    if (priority < SDL_LogGetPriority(category)) {
        return;
    }

    message = SDL_stack_alloc(char, SDL_MAX_LOG_MESSAGE);
    if (!message) {
        return;
    }
    SDL_vsnprintf(message, SDL_MAX_LOG_MESSAGE, fmt, ap);
    SDL_log_function(SDL_log_userdata, category, priority, message);
    SDL_stack_free(message);
}

static void
SDL_LogOutput(void *userdata, int category, SDL_LogPriority priority,
              const char *message)
{
#if defined(__WIN32__)
    /* Way too many allocations here, urgh */
    {
        char *output;
        size_t length;
        LPTSTR tstr;

        length = SDL_strlen(SDL_priority_prefixes[priority]) + 2 + SDL_strlen(message) + 1;
        output = SDL_stack_alloc(char, length);
        SDL_snprintf(output, length, "%s: %s", SDL_priority_prefixes[priority], message);
        tstr = WIN_UTF8ToString(output);
        OutputDebugString(tstr);
        SDL_free(tstr);
        SDL_stack_free(output);
    }
#elif defined(__ANDROID__)
    {
        char tag[32];

        SDL_snprintf(tag, SDL_arraysize(tag), "SDL/%s", GetCategoryPrefix(category));
        __android_log_write(SDL_android_priority[priority], tag, message);
    }
#endif
#if HAVE_STDIO_H
    fprintf(stderr, "%s: %s\n", SDL_priority_prefixes[priority], message);
#endif
}

void
SDL_LogGetOutputFunction(SDL_LogOutputFunction *callback, void **userdata)
{
    if (callback) {
        *callback = SDL_log_function;
    }
    if (userdata) {
        *userdata = SDL_log_userdata;
    }
}

void
SDL_LogSetOutputFunction(SDL_LogOutputFunction callback, void *userdata)
{
    SDL_log_function = callback;
    SDL_log_userdata = userdata;
}

/* vi: set ts=4 sw=4 expandtab: */
