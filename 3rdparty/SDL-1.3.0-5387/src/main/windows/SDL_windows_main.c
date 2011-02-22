/*
    SDL_main.c, placed in the public domain by Sam Lantinga  4/13/98

    The WinMain function -- calls your program's main() function
*/

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Include the SDL main definition header */
#include "SDL.h"
#include "SDL_main.h"

#ifdef main
# ifndef _WIN32_WCE_EMULATION
#  undef main
# endif /* _WIN32_WCE_EMULATION */
#endif /* main */

#if defined(_WIN32_WCE) && _WIN32_WCE < 300
/* seems to be undefined in Win CE although in online help */
#define isspace(a) (((CHAR)a == ' ') || ((CHAR)a == '\t'))
#endif /* _WIN32_WCE < 300 */

static void
UnEscapeQuotes(char *arg)
{
    char *last = NULL;

    while (*arg) {
        if (*arg == '"' && *last == '\\') {
            char *c_curr = arg;
            char *c_last = last;

            while (*c_curr) {
                *c_last = *c_curr;
                c_last = c_curr;
                c_curr++;
            }
            *c_last = '\0';
        }
        last = arg;
        arg++;
    }
}

/* Parse a command line buffer into arguments */
static int
ParseCommandLine(char *cmdline, char **argv)
{
    char *bufp;
    char *lastp = NULL;
    int argc, last_argc;

    argc = last_argc = 0;
    for (bufp = cmdline; *bufp;) {
        /* Skip leading whitespace */
        while (isspace(*bufp)) {
            ++bufp;
        }
        /* Skip over argument */
        if (*bufp == '"') {
            ++bufp;
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            lastp = bufp;
            while (*bufp && (*bufp != '"' || *lastp == '\\')) {
                lastp = bufp;
                ++bufp;
            }
        } else {
            if (*bufp) {
                if (argv) {
                    argv[argc] = bufp;
                }
                ++argc;
            }
            /* Skip over word */
            while (*bufp && !isspace(*bufp)) {
                ++bufp;
            }
        }
        if (*bufp) {
            if (argv) {
                *bufp = '\0';
            }
            ++bufp;
        }

        /* Strip out \ from \" sequences */
        if (argv && last_argc != argc) {
            UnEscapeQuotes(argv[last_argc]);
        }
        last_argc = argc;
    }
    if (argv) {
        argv[argc] = NULL;
    }
    return (argc);
}

/* Show an error message */
static void
ShowError(const char *title, const char *message)
{
/* If USE_MESSAGEBOX is defined, you need to link with user32.lib */
#ifdef USE_MESSAGEBOX
    MessageBox(NULL, message, title, MB_ICONEXCLAMATION | MB_OK);
#else
    fprintf(stderr, "%s: %s\n", title, message);
#endif
}

/* Pop up an out of memory message, returns to Windows */
static BOOL
OutOfMemory(void)
{
    ShowError("Fatal Error", "Out of memory - aborting");
    return FALSE;
}

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
/* The VC++ compiler needs main defined */
#define console_main main
#endif

/* This is where execution begins [console apps] */
int
console_main(int argc, char *argv[])
{
    int status;

    /* Run the application main() code */
    status = SDL_main(argc, argv);

    /* Exit cleanly, calling atexit() functions */
    exit(status);

    /* Hush little compiler, don't you cry... */
    return 0;
}

/* This is where execution begins [windowed apps] */
int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR szCmdLine, int sw)
{
    char **argv;
    int argc;
    char *cmdline;
#ifdef _WIN32_WCE
    wchar_t *bufp;
    int nLen;
#else
    char *bufp;
    size_t nLen;
#endif

#ifdef _WIN32_WCE
    nLen = wcslen(szCmdLine) + 128 + 1;
    bufp = SDL_stack_alloc(wchar_t, nLen * 2);
    wcscpy(bufp, TEXT("\""));
    GetModuleFileName(NULL, bufp + 1, 128 - 3);
    wcscpy(bufp + wcslen(bufp), TEXT("\" "));
    wcsncpy(bufp + wcslen(bufp), szCmdLine, nLen - wcslen(bufp));
    nLen = wcslen(bufp) + 1;
    cmdline = SDL_stack_alloc(char, nLen);
    if (cmdline == NULL) {
        return OutOfMemory();
    }
    WideCharToMultiByte(CP_ACP, 0, bufp, -1, cmdline, nLen, NULL, NULL);
#else
    /* Grab the command line */
    bufp = GetCommandLine();
    nLen = SDL_strlen(bufp) + 1;
    cmdline = SDL_stack_alloc(char, nLen);
    if (cmdline == NULL) {
        return OutOfMemory();
    }
    SDL_strlcpy(cmdline, bufp, nLen);
#endif

    /* Parse it into argv and argc */
    argc = ParseCommandLine(cmdline, NULL);
    argv = SDL_stack_alloc(char *, argc + 1);
    if (argv == NULL) {
        return OutOfMemory();
    }
    ParseCommandLine(cmdline, argv);

    /* Run the main program */
    console_main(argc, argv);

    /* Hush little compiler, don't you cry... */
    return 0;
}

/* vi: set ts=4 sw=4 expandtab: */
