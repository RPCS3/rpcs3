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

#ifndef _SDL_sysvideo_h
#define _SDL_sysvideo_h

#include "SDL_shape.h"

/* The SDL video driver */

typedef struct SDL_WindowShaper SDL_WindowShaper;
typedef struct SDL_ShapeDriver SDL_ShapeDriver;
typedef struct SDL_VideoDisplay SDL_VideoDisplay;
typedef struct SDL_VideoDevice SDL_VideoDevice;

/* Define the SDL window-shaper structure */
struct SDL_WindowShaper
{   
    /* The window associated with the shaper */
    SDL_Window *window;
    
    /* The user's specified coordinates for the window, for once we give it a shape. */
    Uint32 userx,usery;
    
    /* The parameters for shape calculation. */
    SDL_WindowShapeMode mode;
    
    /* Has this window been assigned a shape? */
    SDL_bool hasshape;
    
    void *driverdata;
};

/* Define the SDL shape driver structure */
struct SDL_ShapeDriver
{
    SDL_WindowShaper *(*CreateShaper)(SDL_Window * window);
    int (*SetWindowShape)(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode);
    int (*ResizeWindowShape)(SDL_Window *window);
};

typedef struct SDL_WindowUserData
{
    char *name;
    void *data;
    struct SDL_WindowUserData *next;
} SDL_WindowUserData;

/* Define the SDL window structure, corresponding to toplevel windows */
struct SDL_Window
{
    const void *magic;
    Uint32 id;
    char *title;
    int x, y;
    int w, h;
    Uint32 flags;

    SDL_DisplayMode fullscreen_mode;
    
    SDL_Surface *surface;
    SDL_bool surface_valid;

    SDL_WindowShaper *shaper;

    SDL_WindowUserData *data;

    void *driverdata;

    SDL_Window *prev;
    SDL_Window *next;
};
#define FULLSCREEN_VISIBLE(W) \
    (((W)->flags & SDL_WINDOW_FULLSCREEN) && \
     ((W)->flags & SDL_WINDOW_SHOWN) && \
     !((W)->flags & SDL_WINDOW_MINIMIZED))

/*
 * Define the SDL display structure This corresponds to physical monitors
 * attached to the system.
 */
struct SDL_VideoDisplay
{
    int max_display_modes;
    int num_display_modes;
    SDL_DisplayMode *display_modes;
    SDL_DisplayMode desktop_mode;
    SDL_DisplayMode current_mode;

    SDL_Window *fullscreen_window;

    SDL_VideoDevice *device;

    void *driverdata;
};

/* Forward declaration */
struct SDL_SysWMinfo;

/* Define the SDL video driver structure */
#define _THIS	SDL_VideoDevice *_this

struct SDL_VideoDevice
{
    /* * * */
    /* The name of this video driver */
    const char *name;

    /* * * */
    /* Initialization/Query functions */

    /*
     * Initialize the native video subsystem, filling in the list of
     * displays for this driver, returning 0 or -1 if there's an error.
     */
    int (*VideoInit) (_THIS);

    /*
     * Reverse the effects VideoInit() -- called if VideoInit() fails or
     * if the application is shutting down the video subsystem.
     */
    void (*VideoQuit) (_THIS);

    /* * * */
    /*
     * Display functions
     */

    /*
     * Get the bounds of a display
     */
    int (*GetDisplayBounds) (_THIS, SDL_VideoDisplay * display, SDL_Rect * rect);

    /*
     * Get a list of the available display modes for a display.
     */
    void (*GetDisplayModes) (_THIS, SDL_VideoDisplay * display);

    /*
     * Setting the display mode is independent of creating windows, so
     * when the display mode is changed, all existing windows should have
     * their data updated accordingly, including the display surfaces
     * associated with them.
     */
    int (*SetDisplayMode) (_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);

    /* * * */
    /*
     * Window functions
     */
    int (*CreateWindow) (_THIS, SDL_Window * window);
    int (*CreateWindowFrom) (_THIS, SDL_Window * window, const void *data);
    void (*SetWindowTitle) (_THIS, SDL_Window * window);
    void (*SetWindowIcon) (_THIS, SDL_Window * window, SDL_Surface * icon);
    void (*SetWindowPosition) (_THIS, SDL_Window * window);
    void (*SetWindowSize) (_THIS, SDL_Window * window);
    void (*ShowWindow) (_THIS, SDL_Window * window);
    void (*HideWindow) (_THIS, SDL_Window * window);
    void (*RaiseWindow) (_THIS, SDL_Window * window);
    void (*MaximizeWindow) (_THIS, SDL_Window * window);
    void (*MinimizeWindow) (_THIS, SDL_Window * window);
    void (*RestoreWindow) (_THIS, SDL_Window * window);
    void (*SetWindowFullscreen) (_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen);
    void (*SetWindowGrab) (_THIS, SDL_Window * window);
    void (*DestroyWindow) (_THIS, SDL_Window * window);
    int (*CreateWindowFramebuffer) (_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch);
    int (*UpdateWindowFramebuffer) (_THIS, SDL_Window * window, SDL_Rect * rects, int numrects);
    void (*DestroyWindowFramebuffer) (_THIS, SDL_Window * window);

    /* * * */
    /*
     * Shaped-window functions
     */
    SDL_ShapeDriver shape_driver;

    /* Get some platform dependent window information */
      SDL_bool(*GetWindowWMInfo) (_THIS, SDL_Window * window,
                                  struct SDL_SysWMinfo * info);

    /* * * */
    /*
     * OpenGL support
     */
    int (*GL_LoadLibrary) (_THIS, const char *path);
    void *(*GL_GetProcAddress) (_THIS, const char *proc);
    void (*GL_UnloadLibrary) (_THIS);
      SDL_GLContext(*GL_CreateContext) (_THIS, SDL_Window * window);
    int (*GL_MakeCurrent) (_THIS, SDL_Window * window, SDL_GLContext context);
    int (*GL_SetSwapInterval) (_THIS, int interval);
    int (*GL_GetSwapInterval) (_THIS);
    void (*GL_SwapWindow) (_THIS, SDL_Window * window);
    void (*GL_DeleteContext) (_THIS, SDL_GLContext context);

    /* * * */
    /*
     * Event manager functions
     */
    void (*PumpEvents) (_THIS);

    /* Suspend the screensaver */
    void (*SuspendScreenSaver) (_THIS);

    /* Text input */
    void (*StartTextInput) (_THIS);
    void (*StopTextInput) (_THIS);
    void (*SetTextInputRect) (_THIS, SDL_Rect *rect);

    /* Clipboard */
    int (*SetClipboardText) (_THIS, const char *text);
    char * (*GetClipboardText) (_THIS);
    SDL_bool (*HasClipboardText) (_THIS);

    /* * * */
    /* Data common to all drivers */
    SDL_bool suspend_screensaver;
    int num_displays;
    SDL_VideoDisplay *displays;
    SDL_Window *windows;
    Uint8 window_magic;
    Uint32 next_object_id;
    char * clipboard_text;

    /* * * */
    /* Data used by the GL drivers */
    struct
    {
        int red_size;
        int green_size;
        int blue_size;
        int alpha_size;
        int depth_size;
        int buffer_size;
        int stencil_size;
        int double_buffer;
        int accum_red_size;
        int accum_green_size;
        int accum_blue_size;
        int accum_alpha_size;
        int stereo;
        int multisamplebuffers;
        int multisamplesamples;
        int accelerated;
        int major_version;
        int minor_version;
        int retained_backing;
        int driver_loaded;
        char driver_path[256];
        void *dll_handle;
    } gl_config;

    /* * * */
    /* Data private to this driver */
    void *driverdata;
    struct SDL_GLDriverData *gl_data;

#if SDL_VIDEO_DRIVER_PANDORA
    struct SDL_PrivateGLESData *gles_data;
#endif

    /* * * */
    /* The function used to dispose of this structure */
    void (*free) (_THIS);
};

typedef struct VideoBootStrap
{
    const char *name;
    const char *desc;
    int (*available) (void);
    SDL_VideoDevice *(*create) (int devindex);
} VideoBootStrap;

#if SDL_VIDEO_DRIVER_COCOA
extern VideoBootStrap COCOA_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_X11
extern VideoBootStrap X11_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_DIRECTFB
extern VideoBootStrap DirectFB_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_WINDOWS
extern VideoBootStrap WINDOWS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_BWINDOW
extern VideoBootStrap BWINDOW_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_PANDORA
extern VideoBootStrap PND_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_NDS
extern VideoBootStrap NDS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_UIKIT
extern VideoBootStrap UIKIT_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_ANDROID
extern VideoBootStrap Android_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_DUMMY
extern VideoBootStrap DUMMY_bootstrap;
#endif

extern SDL_VideoDevice *SDL_GetVideoDevice(void);
extern int SDL_AddBasicVideoDisplay(const SDL_DisplayMode * desktop_mode);
extern int SDL_AddVideoDisplay(const SDL_VideoDisplay * display);
extern SDL_bool SDL_AddDisplayMode(SDL_VideoDisplay *display, const SDL_DisplayMode * mode);
extern SDL_VideoDisplay *SDL_GetDisplayForWindow(SDL_Window *window);

extern int SDL_RecreateWindow(SDL_Window * window, Uint32 flags);

extern void SDL_OnWindowShown(SDL_Window * window);
extern void SDL_OnWindowHidden(SDL_Window * window);
extern void SDL_OnWindowResized(SDL_Window * window);
extern void SDL_OnWindowMinimized(SDL_Window * window);
extern void SDL_OnWindowRestored(SDL_Window * window);
extern void SDL_OnWindowFocusGained(SDL_Window * window);
extern void SDL_OnWindowFocusLost(SDL_Window * window);
extern SDL_Window * SDL_GetFocusWindow(void);

#endif /* _SDL_sysvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
