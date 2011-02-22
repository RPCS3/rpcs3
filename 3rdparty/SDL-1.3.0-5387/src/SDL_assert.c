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

#include "SDL.h"
#include "SDL_atomic.h"
#include "SDL_assert.h"
#include "SDL_assert_c.h"
#include "video/SDL_sysvideo.h"

#ifdef __WIN32__
#include "core/windows/SDL_windows.h"

#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW 0
#endif
#else  /* fprintf, _exit(), etc. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

static SDL_assert_state
SDL_PromptAssertion(const SDL_assert_data *data, void *userdata);

/*
 * We keep all triggered assertions in a singly-linked list so we can
 *  generate a report later.
 */
static SDL_assert_data assertion_list_terminator = { 0, 0, 0, 0, 0, 0, 0 };
static SDL_assert_data *triggered_assertions = &assertion_list_terminator;

static SDL_mutex *assertion_mutex = NULL;
static SDL_AssertionHandler assertion_handler = SDL_PromptAssertion;
static void *assertion_userdata = NULL;

#ifdef __GNUC__
static void
debug_print(const char *fmt, ...) __attribute__((format (printf, 1, 2)));
#endif

static void
debug_print(const char *fmt, ...)
{
#ifdef __WIN32__
    /* Format into a buffer for OutputDebugStringA(). */
    char buf[1024];
    char *startptr;
    char *ptr;
    LPTSTR tstr;
    int len;
    va_list ap;
    va_start(ap, fmt);
    len = (int) SDL_vsnprintf(buf, sizeof (buf), fmt, ap);
    va_end(ap);

    /* Visual C's vsnprintf() may not null-terminate the buffer. */
    if ((len >= sizeof (buf)) || (len < 0)) {
        buf[sizeof (buf) - 1] = '\0';
    }

    /* Write it, sorting out the Unix newlines... */
    startptr = buf;
    for (ptr = startptr; *ptr; ptr++) {
        if (*ptr == '\n') {
            *ptr = '\0';
            tstr = WIN_UTF8ToString(startptr);
            OutputDebugString(tstr);
            SDL_free(tstr);
            OutputDebugString(TEXT("\r\n"));
            startptr = ptr+1;
        }
    }

    /* catch that last piece if it didn't have a newline... */
    if (startptr != ptr) {
        tstr = WIN_UTF8ToString(startptr);
        OutputDebugString(tstr);
        SDL_free(tstr);
    }
#else
    /* Unix has it easy. Just dump it to stderr. */
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
#endif
}


#ifdef __WIN32__
static SDL_assert_state SDL_Windows_AssertChoice = SDL_ASSERTION_ABORT;
static const SDL_assert_data *SDL_Windows_AssertData = NULL;

static LRESULT CALLBACK
SDL_Assertion_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            /* !!! FIXME: all this code stinks. */
            const SDL_assert_data *data = SDL_Windows_AssertData;
            char buf[1024];
            LPTSTR tstr;
            const int w = 100;
            const int h = 25;
            const int gap = 10;
            int x = gap;
            int y = 50;
            int len;
            int i;
            static const struct { 
                LPCTSTR name;
                SDL_assert_state state;
            } buttons[] = {
                {TEXT("Abort"), SDL_ASSERTION_ABORT },
                {TEXT("Break"), SDL_ASSERTION_BREAK },
                {TEXT("Retry"), SDL_ASSERTION_RETRY },
                {TEXT("Ignore"), SDL_ASSERTION_IGNORE },
                {TEXT("Always Ignore"), SDL_ASSERTION_ALWAYS_IGNORE },
            };

            len = (int) SDL_snprintf(buf, sizeof (buf), 
                         "Assertion failure at %s (%s:%d), triggered %u time%s:\r\n  '%s'",
                         data->function, data->filename, data->linenum,
                         data->trigger_count, (data->trigger_count == 1) ? "" : "s",
                         data->condition);
            if ((len < 0) || (len >= sizeof (buf))) {
                buf[sizeof (buf) - 1] = '\0';
            }

            tstr = WIN_UTF8ToString(buf);
            CreateWindow(TEXT("STATIC"), tstr,
                         WS_VISIBLE | WS_CHILD | SS_LEFT,
                         x, y, 550, 100,
                         hwnd, (HMENU) 1, NULL, NULL);
            SDL_free(tstr);
            y += 110;

            for (i = 0; i < (sizeof (buttons) / sizeof (buttons[0])); i++) {
                CreateWindow(TEXT("BUTTON"), buttons[i].name,
                         WS_VISIBLE | WS_CHILD,
                         x, y, w, h,
                         hwnd, (HMENU) buttons[i].state, NULL, NULL);
                x += w + gap;
            }
            break;
        }

        case WM_COMMAND:
            SDL_Windows_AssertChoice = ((SDL_assert_state) (LOWORD(wParam)));
            SDL_Windows_AssertData = NULL;
            break;

        case WM_DESTROY:
            SDL_Windows_AssertData = NULL;
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static SDL_assert_state
SDL_PromptAssertion_windows(const SDL_assert_data *data)
{
    HINSTANCE hInstance = 0;  /* !!! FIXME? */
    HWND hwnd;
    MSG msg;
    WNDCLASS wc = {0};

    SDL_Windows_AssertChoice = SDL_ASSERTION_ABORT;
    SDL_Windows_AssertData = data;

    wc.lpszClassName = TEXT("SDL_assert");
    wc.hInstance = hInstance ;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.lpfnWndProc = SDL_Assertion_WndProc;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
  
    RegisterClass(&wc);
    hwnd = CreateWindow(wc.lpszClassName, TEXT("SDL assertion failure"),
                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                 150, 150, 570, 260, 0, 0, hInstance, 0);  

    while (GetMessage(&msg, NULL, 0, 0) && (SDL_Windows_AssertData != NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, hInstance);
    return SDL_Windows_AssertChoice;
}
#endif


static void SDL_AddAssertionToReport(SDL_assert_data *data)
{
    /* (data) is always a static struct defined with the assert macros, so
       we don't have to worry about copying or allocating them. */
    if (data->next == NULL) {  /* not yet added? */
        data->next = triggered_assertions;
        triggered_assertions = data;
    }
}


static void SDL_GenerateAssertionReport(void)
{
    const SDL_assert_data *item;

    /* only do this if the app hasn't assigned an assertion handler. */
    if (assertion_handler != SDL_PromptAssertion)
        return;

    item = SDL_GetAssertionReport();
    if (item->condition)
    {
        debug_print("\n\nSDL assertion report.\n");
        debug_print("All SDL assertions between last init/quit:\n\n");

        while (item->condition) {
            debug_print(
                "'%s'\n"
                "    * %s (%s:%d)\n"
                "    * triggered %u time%s.\n"
                "    * always ignore: %s.\n",
                item->condition, item->function, item->filename,
                item->linenum, item->trigger_count,
                (item->trigger_count == 1) ? "" : "s",
                item->always_ignore ? "yes" : "no");
            item = item->next;
        }
        debug_print("\n");

        SDL_ResetAssertionReport();
    }
}

static void SDL_ExitProcess(int exitcode)
{
#ifdef __WIN32__
    ExitProcess(42);
#else
    _exit(42);
#endif
}

static void SDL_AbortAssertion(void)
{
    SDL_Quit();
    SDL_ExitProcess(42);
}


static SDL_assert_state
SDL_PromptAssertion(const SDL_assert_data *data, void *userdata)
{
    const char *envr;
    SDL_assert_state state = SDL_ASSERTION_ABORT;
    SDL_Window *window;

    (void) userdata;  /* unused in default handler. */

    debug_print("\n\n"
                "Assertion failure at %s (%s:%d), triggered %u time%s:\n"
                "  '%s'\n"
                "\n",
                data->function, data->filename, data->linenum,
                data->trigger_count, (data->trigger_count == 1) ? "" : "s",
                data->condition);

    /* let env. variable override, so unit tests won't block in a GUI. */
    envr = SDL_getenv("SDL_ASSERT");
    if (envr != NULL) {
        if (SDL_strcmp(envr, "abort") == 0) {
            return SDL_ASSERTION_ABORT;
        } else if (SDL_strcmp(envr, "break") == 0) {
            return SDL_ASSERTION_BREAK;
        } else if (SDL_strcmp(envr, "retry") == 0) {
            return SDL_ASSERTION_RETRY;
        } else if (SDL_strcmp(envr, "ignore") == 0) {
            return SDL_ASSERTION_IGNORE;
        } else if (SDL_strcmp(envr, "always_ignore") == 0) {
            return SDL_ASSERTION_ALWAYS_IGNORE;
        } else {
            return SDL_ASSERTION_ABORT;  /* oh well. */
        }
    }

    /* Leave fullscreen mode, if possible (scary!) */
    window = SDL_GetFocusWindow();
    if (window) {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
            SDL_MinimizeWindow(window);
        } else {
            /* !!! FIXME: ungrab the input if we're not fullscreen? */
            /* No need to mess with the window */
            window = 0;
        }
    }

    /* platform-specific UI... */

#ifdef __WIN32__
    state = SDL_PromptAssertion_windows(data);

#elif __MACOSX__
    /* This has to be done in an Objective-C (*.m) file, so we call out. */
    extern SDL_assert_state SDL_PromptAssertion_cocoa(const SDL_assert_data *);
    state = SDL_PromptAssertion_cocoa(data);

#else
    /* this is a little hacky. */
    for ( ; ; ) {
        char buf[32];
        fprintf(stderr, "Abort/Break/Retry/Ignore/AlwaysIgnore? [abriA] : ");
        fflush(stderr);
        if (fgets(buf, sizeof (buf), stdin) == NULL) {
            break;
        }

        if (SDL_strcmp(buf, "a") == 0) {
            state = SDL_ASSERTION_ABORT;
            break;
        } else if (SDL_strcmp(envr, "b") == 0) {
            state = SDL_ASSERTION_BREAK;
            break;
        } else if (SDL_strcmp(envr, "r") == 0) {
            state = SDL_ASSERTION_RETRY;
            break;
        } else if (SDL_strcmp(envr, "i") == 0) {
            state = SDL_ASSERTION_IGNORE;
            break;
        } else if (SDL_strcmp(envr, "A") == 0) {
            state = SDL_ASSERTION_ALWAYS_IGNORE;
            break;
        }
    }
#endif

    /* Re-enter fullscreen mode */
    if (window) {
        SDL_RestoreWindow(window);
    }

    return state;
}


SDL_assert_state
SDL_ReportAssertion(SDL_assert_data *data, const char *func, const char *file,
                    int line)
{
    static int assertion_running = 0;
    static SDL_SpinLock spinlock = 0;
    SDL_assert_state state = SDL_ASSERTION_IGNORE;

    SDL_AtomicLock(&spinlock);
    if (assertion_mutex == NULL) { /* never called SDL_Init()? */
        assertion_mutex = SDL_CreateMutex();
        if (assertion_mutex == NULL) {
            SDL_AtomicUnlock(&spinlock);
            return SDL_ASSERTION_IGNORE;   /* oh well, I guess. */
        }
    }
    SDL_AtomicUnlock(&spinlock);

    if (SDL_LockMutex(assertion_mutex) < 0) {
        return SDL_ASSERTION_IGNORE;   /* oh well, I guess. */
    }

    /* doing this because Visual C is upset over assigning in the macro. */
    if (data->trigger_count == 0) {
        data->function = func;
        data->filename = file;
        data->linenum = line;
    }

    SDL_AddAssertionToReport(data);

    data->trigger_count++;

    assertion_running++;
    if (assertion_running > 1) {   /* assert during assert! Abort. */
        if (assertion_running == 2) {
            SDL_AbortAssertion();
        } else if (assertion_running == 3) {  /* Abort asserted! */
            SDL_ExitProcess(42);
        } else {
            while (1) { /* do nothing but spin; what else can you do?! */ }
        }
    }

    if (!data->always_ignore) {
        state = assertion_handler(data, assertion_userdata);
    }

    switch (state)
    {
        case SDL_ASSERTION_ABORT:
            SDL_AbortAssertion();
            return SDL_ASSERTION_IGNORE;  /* shouldn't return, but oh well. */

        case SDL_ASSERTION_ALWAYS_IGNORE:
            state = SDL_ASSERTION_IGNORE;
            data->always_ignore = 1;
            break;

        case SDL_ASSERTION_IGNORE:
        case SDL_ASSERTION_RETRY:
        case SDL_ASSERTION_BREAK:
            break;  /* macro handles these. */
    }

    assertion_running--;
    SDL_UnlockMutex(assertion_mutex);

    return state;
}


int SDL_AssertionsInit(void)
{
    /* this is a no-op at the moment. */
    return 0;
}

void SDL_AssertionsQuit(void)
{
    SDL_GenerateAssertionReport();
    if (assertion_mutex != NULL) {
        SDL_DestroyMutex(assertion_mutex);
        assertion_mutex = NULL;
    }
}

void SDL_SetAssertionHandler(SDL_AssertionHandler handler, void *userdata)
{
    if (handler != NULL) {
        assertion_handler = handler;
        assertion_userdata = userdata;
    } else {
        assertion_handler = SDL_PromptAssertion;
        assertion_userdata = NULL;
    }
}

const SDL_assert_data *SDL_GetAssertionReport(void)
{
    return triggered_assertions;
}

void SDL_ResetAssertionReport(void)
{
    SDL_assert_data *item = triggered_assertions;
    SDL_assert_data *next = NULL;
    for (item = triggered_assertions; item->condition; item = next) {
        next = (SDL_assert_data *) item->next;
        item->always_ignore = SDL_FALSE;
        item->trigger_count = 0;
        item->next = NULL;
    }

    triggered_assertions = &assertion_list_terminator;
}

/* vi: set ts=4 sw=4 expandtab: */
