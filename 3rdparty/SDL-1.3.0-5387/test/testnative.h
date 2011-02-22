
/* Definitions for platform dependent windowing functions to test SDL
   integration with native windows
*/

#include "SDL.h"

/* This header includes all the necessary system headers for native windows */
#include "SDL_syswm.h"

typedef struct
{
    const char *tag;
    void *(*CreateNativeWindow) (int w, int h);
    void (*DestroyNativeWindow) (void *window);
} NativeWindowFactory;

#ifdef SDL_VIDEO_DRIVER_WINDOWS
#define TEST_NATIVE_WINDOWS
extern NativeWindowFactory WindowsWindowFactory;
#endif

#ifdef SDL_VIDEO_DRIVER_X11
#define TEST_NATIVE_X11
extern NativeWindowFactory X11WindowFactory;
#endif

#ifdef SDL_VIDEO_DRIVER_COCOA
/* Actually, we don't really do this, since it involves adding Objective C
   support to the build system, which is a little tricky.  You can uncomment
   it manually though and link testnativecocoa.m into the test application.
*/
#if 1
#define TEST_NATIVE_COCOA
extern NativeWindowFactory CocoaWindowFactory;
#endif
#endif
