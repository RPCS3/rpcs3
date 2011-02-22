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

/* BWindow based framebuffer implementation */

#include <unistd.h>

#include "SDL_BWin.h"
#include "SDL_timer.h"

extern "C"
{

#include "../SDL_sysvideo.h"
#include "../../events/SDL_events_c.h"
#include "SDL_sysevents_c.h"
#include "SDL_sysmouse_c.h"
#include "SDL_syswm_c.h"
#include "SDL_lowvideo.h"
#include "../SDL_yuvfuncs.h"
#include "SDL_sysyuv.h"
#include "../blank_cursor.h"

#define BEOS_HIDDEN_SIZE	32      /* starting hidden window size */

/* Initialization/Query functions */
    static int BE_VideoInit(_THIS, SDL_PixelFormat * vformat);
    static SDL_Rect **BE_ListModes(_THIS, SDL_PixelFormat * format,
                                   Uint32 flags);
    static SDL_Surface *BE_SetVideoMode(_THIS, SDL_Surface * current,
                                        int width, int height, int bpp,
                                        Uint32 flags);
    static void BE_UpdateMouse(_THIS);
    static int BE_SetColors(_THIS, int firstcolor, int ncolors,
                            SDL_Color * colors);
    static void BE_VideoQuit(_THIS);

/* Hardware surface functions */
    static int BE_AllocHWSurface(_THIS, SDL_Surface * surface);
    static int BE_LockHWSurface(_THIS, SDL_Surface * surface);
    static void BE_UnlockHWSurface(_THIS, SDL_Surface * surface);
    static void BE_FreeHWSurface(_THIS, SDL_Surface * surface);

    static int BE_ToggleFullScreen(_THIS, int fullscreen);
    static SDL_Overlay *BE_CreateYUVOverlay(_THIS, int width, int height,
                                            Uint32 format,
                                            SDL_Surface * display);

/* OpenGL functions */
#if SDL_VIDEO_OPENGL
    static int BE_GL_LoadLibrary(_THIS, const char *path);
    static void *BE_GL_GetProcAddress(_THIS, const char *proc);
    static int BE_GL_GetAttribute(_THIS, SDL_GLattr attrib, int *value);
    static int BE_GL_MakeCurrent(_THIS);
    static void BE_GL_SwapBuffers(_THIS);
#endif

/* FB driver bootstrap functions */

    static int BE_Available(void)
    {
        return (1);
    }

    static void BE_DeleteDevice(SDL_VideoDevice * device)
    {
        SDL_free(device->hidden);
        SDL_free(device);
    }

    static SDL_VideoDevice *BE_CreateDevice(int devindex)
    {
        SDL_VideoDevice *device;

        /* Initialize all variables that we clean on shutdown */
        device = (SDL_VideoDevice *) SDL_malloc(sizeof(SDL_VideoDevice));
        if (device) {
            SDL_memset(device, 0, (sizeof *device));
            device->hidden = (struct SDL_PrivateVideoData *)
                SDL_malloc((sizeof *device->hidden));
        }
        if ((device == NULL) || (device->hidden == NULL)) {
            SDL_OutOfMemory();
            if (device) {
                SDL_free(device);
            }
            return (0);
        }
        SDL_memset(device->hidden, 0, (sizeof *device->hidden));

        /* Set the function pointers */
        /* Initialization/Query functions */
        device->VideoInit = BE_VideoInit;
        device->ListModes = BE_ListModes;
        device->SetVideoMode = BE_SetVideoMode;
        device->ToggleFullScreen = BE_ToggleFullScreen;
        device->UpdateMouse = BE_UpdateMouse;
        device->CreateYUVOverlay = BE_CreateYUVOverlay;
        device->SetColors = BE_SetColors;
        device->UpdateRects = NULL;
        device->VideoQuit = BE_VideoQuit;
        /* Hardware acceleration functions */
        device->AllocHWSurface = BE_AllocHWSurface;
        device->CheckHWBlit = NULL;
        device->FillHWRect = NULL;
        device->SetHWColorKey = NULL;
        device->SetHWAlpha = NULL;
        device->LockHWSurface = BE_LockHWSurface;
        device->UnlockHWSurface = BE_UnlockHWSurface;
        device->FlipHWSurface = NULL;
        device->FreeHWSurface = BE_FreeHWSurface;
#if SDL_VIDEO_OPENGL
        /* OpenGL support */
        device->GL_LoadLibrary = BE_GL_LoadLibrary;
        device->GL_GetProcAddress = BE_GL_GetProcAddress;
        device->GL_GetAttribute = BE_GL_GetAttribute;
        device->GL_MakeCurrent = BE_GL_MakeCurrent;
        device->GL_SwapBuffers = BE_GL_SwapBuffers;
#endif
        /* Window manager functions */
        device->SetCaption = BE_SetWMCaption;
        device->SetIcon = NULL;
        device->IconifyWindow = BE_IconifyWindow;
        device->GrabInput = NULL;
        device->GetWMInfo = BE_GetWMInfo;
        /* Cursor manager functions */
        device->FreeWMCursor = BE_FreeWMCursor;
        device->CreateWMCursor = BE_CreateWMCursor;
        device->ShowWMCursor = BE_ShowWMCursor;
        device->WarpWMCursor = BE_WarpWMCursor;
        device->MoveWMCursor = NULL;
        device->CheckMouseMode = NULL;
        /* Event manager functions */
        device->InitOSKeymap = BE_InitOSKeymap;
        device->PumpEvents = BE_PumpEvents;

        device->free = BE_DeleteDevice;

        /* Set the driver flags */
        device->handles_any_size = 1;

        return device;
    }

    VideoBootStrap BWINDOW_bootstrap = {
        "bwindow", "BDirectWindow graphics",
        BE_Available, BE_CreateDevice
    };

    static inline int ColorSpaceToBitsPerPixel(uint32 colorspace)
    {
        int bitsperpixel;

        bitsperpixel = 0;
        switch (colorspace) {
        case B_CMAP8:
            bitsperpixel = 8;
            break;
        case B_RGB15:
        case B_RGBA15:
        case B_RGB15_BIG:
        case B_RGBA15_BIG:
            bitsperpixel = 15;
            break;
        case B_RGB16:
        case B_RGB16_BIG:
            bitsperpixel = 16;
            break;
        case B_RGB32:
        case B_RGBA32:
        case B_RGB32_BIG:
        case B_RGBA32_BIG:
            bitsperpixel = 32;
            break;
        default:
            break;
        }
        return (bitsperpixel);
    }

/* Function to sort the display_list in bscreen */
    static int CompareModes(const void *A, const void *B)
    {
        const display_mode *a = (display_mode *) A;
        const display_mode *b = (display_mode *) B;

        if (a->space == b->space) {
            return ((b->virtual_width * b->virtual_height) -
                    (a->virtual_width * a->virtual_height));
        } else {
            return (ColorSpaceToBitsPerPixel(b->space) -
                    ColorSpaceToBitsPerPixel(a->space));
        }
    }

/* Yes, this isn't the fastest it could be, but it works nicely */
    static int BE_AddMode(_THIS, int index, unsigned int w, unsigned int h)
    {
        SDL_Rect *mode;
        int i;
        int next_mode;

        /* Check to see if we already have this mode */
        if (SDL_nummodes[index] > 0) {
            for (i = SDL_nummodes[index] - 1; i >= 0; --i) {
                mode = SDL_modelist[index][i];
                if ((mode->w == w) && (mode->h == h)) {
#ifdef BWINDOW_DEBUG
                    fprintf(stderr,
                            "We already have mode %dx%d at %d bytes per pixel\n",
                            w, h, index + 1);
#endif
                    return (0);
                }
            }
        }

        /* Set up the new video mode rectangle */
        mode = (SDL_Rect *) SDL_malloc(sizeof *mode);
        if (mode == NULL) {
            SDL_OutOfMemory();
            return (-1);
        }
        mode->x = 0;
        mode->y = 0;
        mode->w = w;
        mode->h = h;
#ifdef BWINDOW_DEBUG
        fprintf(stderr, "Adding mode %dx%d at %d bytes per pixel\n", w, h,
                index + 1);
#endif

        /* Allocate the new list of modes, and fill in the new mode */
        next_mode = SDL_nummodes[index];
        SDL_modelist[index] = (SDL_Rect **)
            SDL_realloc(SDL_modelist[index],
                        (1 + next_mode + 1) * sizeof(SDL_Rect *));
        if (SDL_modelist[index] == NULL) {
            SDL_OutOfMemory();
            SDL_nummodes[index] = 0;
            SDL_free(mode);
            return (-1);
        }
        SDL_modelist[index][next_mode] = mode;
        SDL_modelist[index][next_mode + 1] = NULL;
        SDL_nummodes[index]++;

        return (0);
    }

    int BE_VideoInit(_THIS, SDL_PixelFormat * vformat)
    {
        display_mode *modes;
        uint32 i, nmodes;
        int bpp;
        BRect bounds;

        /* Initialize the Be Application for appserver interaction */
        if (SDL_InitBeApp() < 0) {
            return (-1);
        }

        /* It is important that this be created after SDL_InitBeApp() */
        BScreen bscreen;

        /* Save the current display mode */
        bscreen.GetMode(&saved_mode);
        _this->info.current_w = saved_mode.virtual_width;
        _this->info.current_h = saved_mode.virtual_height;

        /* Determine the screen depth */
        vformat->BitsPerPixel =
            ColorSpaceToBitsPerPixel(bscreen.ColorSpace());
        if (vformat->BitsPerPixel == 0) {
            SDL_SetError("Unknown BScreen colorspace: 0x%x",
                         bscreen.ColorSpace());
            return (-1);
        }

        /* Get the video modes we can switch to in fullscreen mode */
        bscreen.GetModeList(&modes, &nmodes);
        SDL_qsort(modes, nmodes, sizeof *modes, CompareModes);
        for (i = 0; i < nmodes; ++i) {
            bpp = ColorSpaceToBitsPerPixel(modes[i].space);
            //if ( bpp != 0 ) { // There are bugs in changing colorspace
            if (modes[i].space == saved_mode.space) {
                BE_AddMode(_this, ((bpp + 7) / 8) - 1,
                           modes[i].virtual_width, modes[i].virtual_height);
            }
        }

        /* Create the window and view */
        bounds.top = 0;
        bounds.left = 0;
        bounds.right = BEOS_HIDDEN_SIZE;
        bounds.bottom = BEOS_HIDDEN_SIZE;
        SDL_Win = new SDL_BWin(bounds);

#if SDL_VIDEO_OPENGL
        /* testgl application doesn't load library, just tries to load symbols */
        /* is it correct? if so we have to load library here */
        BE_GL_LoadLibrary(_this, NULL);
#endif

        /* Create the clear cursor */
        SDL_BlankCursor = BE_CreateWMCursor(_this, blank_cdata, blank_cmask,
                                            BLANK_CWIDTH, BLANK_CHEIGHT,
                                            BLANK_CHOTX, BLANK_CHOTY);

        /* Fill in some window manager capabilities */
        _this->info.wm_available = 1;

        /* We're done! */
        return (0);
    }

/* We support any dimension at our bit-depth */
    SDL_Rect **BE_ListModes(_THIS, SDL_PixelFormat * format, Uint32 flags)
    {
        SDL_Rect **modes;

        modes = ((SDL_Rect **) 0);
        if ((flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
            modes = SDL_modelist[((format->BitsPerPixel + 7) / 8) - 1];
        } else {
            if (format->BitsPerPixel == _this->screen->format->BitsPerPixel) {
                modes = ((SDL_Rect **) - 1);
            }
        }
        return (modes);
    }

/* Various screen update functions available */
    static void BE_NormalUpdate(_THIS, int numrects, SDL_Rect * rects);


/* Find the closest display mode for fullscreen */
    static bool BE_FindClosestFSMode(_THIS, int width, int height, int bpp,
                                     display_mode * mode)
    {
        BScreen bscreen;
        uint32 i, nmodes;
        SDL_Rect **modes;
        display_mode *dmodes;
        display_mode current;
        float current_refresh;
        bscreen.GetMode(&current);
        current_refresh = (1000 * current.timing.pixel_clock) /
            (current.timing.h_total * current.timing.v_total);

        modes = SDL_modelist[((bpp + 7) / 8) - 1];

        bool exactmatch = false;
        for (uint32 x = 0; modes[x]; x++) {
            if (modes[x]->w == width && modes[x]->h == height) {
                exactmatch = true;
                i = x;
                break;
            }
        }
        if (!exactmatch) {
            for (i = 0; modes[i] && (modes[i]->w > width) &&
                 (modes[i]->h > height); ++i) {
                /* still looking */
            }
            if (!modes[i] || (modes[i]->w < width) || (modes[i]->h < height)) {
                /* We went too far */
                --i;
            }
        }
        width = modes[i]->w;
        height = modes[i]->h;
        bscreen.GetModeList(&dmodes, &nmodes);
        for (i = 0; i < nmodes; ++i) {
            if ((bpp == ColorSpaceToBitsPerPixel(dmodes[i].space)) &&
                (width == dmodes[i].virtual_width) &&
                (height == dmodes[i].virtual_height)) {
                break;
            }
        }
        if (i != nmodes) {
            *mode = dmodes[i];
            if ((mode->virtual_width <= current.virtual_width) &&
                (mode->virtual_height <= current.virtual_height)) {
                float new_refresh = (1000 * mode->timing.pixel_clock) /
                    (mode->timing.h_total * mode->timing.v_total);
                if (new_refresh < current_refresh) {
                    mode->timing.pixel_clock =
                        (uint32) ((mode->timing.h_total *
                                   mode->timing.v_total) *
                                  current_refresh / 1000);
                }
            }
            return true;
        } else {
            return false;
        }
    }

    static int BE_SetFullScreen(_THIS, SDL_Surface * screen, int fullscreen)
    {
        int was_fullscreen;
        bool needs_unlock;
        BScreen bscreen;
        BRect bounds;
        display_mode mode;
        int width, height, bpp;

        /* Set the fullscreen mode */
        was_fullscreen = SDL_Win->IsFullScreen();
        SDL_Win->SetFullScreen(fullscreen);
        fullscreen = SDL_Win->IsFullScreen();

        width = screen->w;
        height = screen->h;

        /* Set the appropriate video mode */
        if (fullscreen) {
            bpp = screen->format->BitsPerPixel;
            bscreen.GetMode(&mode);
            if ((bpp != ColorSpaceToBitsPerPixel(mode.space)) ||
                (width != mode.virtual_width) ||
                (height != mode.virtual_height)) {
                if (BE_FindClosestFSMode(_this, width, height, bpp, &mode)) {
                    bscreen.SetMode(&mode);
                    /* This simply stops the next resize event from being
                     * sent to the SDL handler.
                     */
                    SDL_Win->InhibitResize();
                } else {
                    fullscreen = 0;
                    SDL_Win->SetFullScreen(fullscreen);
                }
            }
        }
        if (was_fullscreen && !fullscreen) {
            bscreen.SetMode(&saved_mode);
        }

        if (SDL_Win->Lock()) {
            int xoff, yoff;
            if (SDL_Win->Shown()) {
                needs_unlock = 1;
                SDL_Win->Hide();
            } else {
                needs_unlock = 0;
            }
            /* This resizes the window and view area, but inhibits resizing
             * of the BBitmap due to the InhibitResize call above. Thus the
             * bitmap (pixel data) never changes.
             */
            SDL_Win->ResizeTo(width, height);
            bounds = bscreen.Frame();
            /* Calculate offsets - used either to center window
             * (windowed mode) or to set drawing offsets (fullscreen mode)
             */
            xoff = (bounds.IntegerWidth() - width) / 2;
            yoff = (bounds.IntegerHeight() - height) / 2;
            if (fullscreen) {
                /* Set offset for drawing */
                SDL_Win->SetXYOffset(xoff, yoff);
            } else {
                /* Center window and reset the drawing offset */
                SDL_Win->SetXYOffset(0, 0);
            }
            if (!needs_unlock || was_fullscreen) {
                /* Center the window the first time */
                SDL_Win->MoveTo(xoff > 0 ? (float) xoff : 0.0f,
                                yoff > 0 ? (float) yoff : 0.0f);
            }
            SDL_Win->Show();

            /* Unlock the window manually after the first Show() */
            if (needs_unlock) {
                SDL_Win->Unlock();
            }
        }

        /* Set the fullscreen flag in the screen surface */
        if (fullscreen) {
            screen->flags |= SDL_FULLSCREEN;
        } else {
            screen->flags &= ~SDL_FULLSCREEN;
        }
        return (1);
    }

    static int BE_ToggleFullScreen(_THIS, int fullscreen)
    {
        return BE_SetFullScreen(_this, _this->screen, fullscreen);
    }

/* FIXME: check return values and cleanup here */
    SDL_Surface *BE_SetVideoMode(_THIS, SDL_Surface * current,
                                 int width, int height, int bpp, Uint32 flags)
    {
        BScreen bscreen;
        BBitmap *bbitmap;
        BRect bounds;
        Uint32 gl_flags = 0;

        /* Only RGB works on r5 currently */
        gl_flags = BGL_RGB;
        if (_this->gl_config.double_buffer)
            gl_flags |= BGL_DOUBLE;
        else
            gl_flags |= BGL_SINGLE;
        if (_this->gl_config.alpha_size > 0 || bpp == 32)
            gl_flags |= BGL_ALPHA;
        if (_this->gl_config.depth_size > 0)
            gl_flags |= BGL_DEPTH;
        if (_this->gl_config.stencil_size > 0)
            gl_flags |= BGL_STENCIL;
        if (_this->gl_config.accum_red_size > 0
            || _this->gl_config.accum_green_size > 0
            || _this->gl_config.accum_blue_size > 0
            || _this->gl_config.accum_alpha_size > 0)
            gl_flags |= BGL_ACCUM;

        /* Create the view for this window, using found flags */
        if (SDL_Win->CreateView(flags, gl_flags) < 0) {
            return (NULL);
        }

        current->flags = 0;     /* Clear flags */
        current->w = width;
        current->h = height;
        SDL_Win->SetType(B_TITLED_WINDOW);
        if (flags & SDL_NOFRAME) {
            current->flags |= SDL_NOFRAME;
            SDL_Win->SetLook(B_NO_BORDER_WINDOW_LOOK);
        } else {
            if ((flags & SDL_RESIZABLE) && !(flags & SDL_INTERNALOPENGL)) {
                current->flags |= SDL_RESIZABLE;
                /* We don't want opaque resizing (TM). :-) */
                SDL_Win->SetFlags(B_OUTLINE_RESIZE);
            } else {
                SDL_Win->SetFlags(B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
            }
        }

        if (flags & SDL_INTERNALOPENGL) {
            current->flags |= SDL_INTERNALOPENGL;
            current->pitch = 0;
            current->pixels = NULL;
            _this->UpdateRects = NULL;
        } else {
            /* Create the BBitmap framebuffer */
            bounds.top = 0;
            bounds.left = 0;
            bounds.right = width - 1;
            bounds.bottom = height - 1;
            bbitmap = new BBitmap(bounds, bscreen.ColorSpace());
            if (!bbitmap->IsValid()) {
                SDL_SetError("Couldn't create screen bitmap");
                delete bbitmap;
                return (NULL);
            }
            current->pitch = bbitmap->BytesPerRow();
            current->pixels = (void *) bbitmap->Bits();
            SDL_Win->SetBitmap(bbitmap);
            _this->UpdateRects = BE_NormalUpdate;
        }

        /* Set the correct fullscreen mode */
        BE_SetFullScreen(_this, current, flags & SDL_FULLSCREEN ? 1 : 0);

        /* We're done */
        return (current);
    }

/* Update the current mouse state and position */
    void BE_UpdateMouse(_THIS)
    {
        BPoint point;
        uint32 buttons;

        if (SDL_Win->Lock()) {
            /* Get new input state, if still active */
            if (SDL_Win->IsActive()) {
                (SDL_Win->View())->GetMouse(&point, &buttons, true);
            } else {
                point.x = -1;
                point.y = -1;
            }
            SDL_Win->Unlock();

            if ((point.x >= 0) && (point.x < SDL_VideoSurface->w) &&
                (point.y >= 0) && (point.y < SDL_VideoSurface->h)) {
                SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
                SDL_PrivateMouseMotion(0, 0,
                                       (Sint16) point.x, (Sint16) point.y);
            } else {
                SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
            }
        }
    }

/* We don't actually allow hardware surfaces other than the main one */
    static int BE_AllocHWSurface(_THIS, SDL_Surface * surface)
    {
        return (-1);
    }
    static void BE_FreeHWSurface(_THIS, SDL_Surface * surface)
    {
        return;
    }
    static int BE_LockHWSurface(_THIS, SDL_Surface * surface)
    {
        return (0);
    }
    static void BE_UnlockHWSurface(_THIS, SDL_Surface * surface)
    {
        return;
    }

    static void BE_NormalUpdate(_THIS, int numrects, SDL_Rect * rects)
    {
        if (SDL_Win->BeginDraw()) {
            int i;

            for (i = 0; i < numrects; ++i) {
                BRect rect;

                rect.top = rects[i].y;
                rect.left = rects[i].x;
                rect.bottom = rect.top + rects[i].h - 1;
                rect.right = rect.left + rects[i].w - 1;
                SDL_Win->DrawAsync(rect);
            }
            SDL_Win->EndDraw();
        }
    }

#if SDL_VIDEO_OPENGL
/* Passing a NULL path means load pointers from the application */
    int BE_GL_LoadLibrary(_THIS, const char *path)
    {
        if (path == NULL) {
            if (_this->gl_config.dll_handle == NULL) {
                image_info info;
                int32 cookie = 0;
                while (get_next_image_info(0, &cookie, &info) == B_OK) {
                    void *location = NULL;
                    if (get_image_symbol
                        ((image_id) cookie, "glBegin",
                         B_SYMBOL_TYPE_ANY, &location) == B_OK) {
                        _this->gl_config.dll_handle = (void *) cookie;
                        _this->gl_config.driver_loaded = 1;
                        SDL_strlcpy(_this->gl_config.driver_path,
                                    "libGL.so",
                                    SDL_arraysize(_this->
                                                  gl_config.driver_path));
                    }
                }
            }
        } else {
            /*
               FIXME None of BeOS libGL.so implementations have exported functions 
               to load BGLView, which should be reloaded from new lib.
               So for now just "load" linked libGL.so :(
             */
            if (_this->gl_config.dll_handle == NULL) {
                return BE_GL_LoadLibrary(_this, NULL);
            }

            /* Unload old first */
            /*if (_this->gl_config.dll_handle != NULL) { */
            /* Do not try to unload application itself (if LoadLibrary was called before with NULL ;) */
            /*      image_info info;
               if (get_image_info((image_id)_this->gl_config.dll_handle, &info) == B_OK) {
               if (info.type != B_APP_IMAGE) {
               unload_add_on((image_id)_this->gl_config.dll_handle);
               }
               }

               }

               if ((_this->gl_config.dll_handle = (void*)load_add_on(path)) != (void*)B_ERROR) {
               _this->gl_config.driver_loaded = 1;
               SDL_strlcpy(_this->gl_config.driver_path, path, SDL_arraysize(_this->gl_config.driver_path));
               } */
        }

        if (_this->gl_config.dll_handle != NULL) {
            return 0;
        } else {
            _this->gl_config.dll_handle = NULL;
            _this->gl_config.driver_loaded = 0;
            *_this->gl_config.driver_path = '\0';
            return -1;
        }
    }

    void *BE_GL_GetProcAddress(_THIS, const char *proc)
    {
        if (_this->gl_config.dll_handle != NULL) {
            void *location = NULL;
            status_t err;
            if ((err =
                 get_image_symbol((image_id) _this->gl_config.dll_handle,
                                  proc, B_SYMBOL_TYPE_ANY,
                                  &location)) == B_OK) {
                return location;
            } else {
                SDL_SetError("Couldn't find OpenGL symbol");
                return NULL;
            }
        } else {
            SDL_SetError("OpenGL library not loaded");
            return NULL;
        }
    }

    int BE_GL_GetAttribute(_THIS, SDL_GLattr attrib, int *value)
    {
        /*
           FIXME? Right now BE_GL_GetAttribute shouldn't be called between glBegin() and glEnd() - it doesn't use "cached" values
         */
        switch (attrib) {
        case SDL_GL_RED_SIZE:
            glGetIntegerv(GL_RED_BITS, (GLint *) value);
            break;
        case SDL_GL_GREEN_SIZE:
            glGetIntegerv(GL_GREEN_BITS, (GLint *) value);
            break;
        case SDL_GL_BLUE_SIZE:
            glGetIntegerv(GL_BLUE_BITS, (GLint *) value);
            break;
        case SDL_GL_ALPHA_SIZE:
            glGetIntegerv(GL_ALPHA_BITS, (GLint *) value);
            break;
        case SDL_GL_DOUBLEBUFFER:
            glGetBooleanv(GL_DOUBLEBUFFER, (GLboolean *) value);
            break;
        case SDL_GL_BUFFER_SIZE:
            int v;
            glGetIntegerv(GL_RED_BITS, (GLint *) & v);
            *value = v;
            glGetIntegerv(GL_GREEN_BITS, (GLint *) & v);
            *value += v;
            glGetIntegerv(GL_BLUE_BITS, (GLint *) & v);
            *value += v;
            glGetIntegerv(GL_ALPHA_BITS, (GLint *) & v);
            *value += v;
            break;
        case SDL_GL_DEPTH_SIZE:
            glGetIntegerv(GL_DEPTH_BITS, (GLint *) value);      /* Mesa creates 16 only? r5 always 32 */
            break;
        case SDL_GL_STENCIL_SIZE:
            glGetIntegerv(GL_STENCIL_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_RED_SIZE:
            glGetIntegerv(GL_ACCUM_RED_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_GREEN_SIZE:
            glGetIntegerv(GL_ACCUM_GREEN_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_BLUE_SIZE:
            glGetIntegerv(GL_ACCUM_BLUE_BITS, (GLint *) value);
            break;
        case SDL_GL_ACCUM_ALPHA_SIZE:
            glGetIntegerv(GL_ACCUM_ALPHA_BITS, (GLint *) value);
            break;
        case SDL_GL_STEREO:
        case SDL_GL_MULTISAMPLEBUFFERS:
        case SDL_GL_MULTISAMPLESAMPLES:
        default:
            *value = 0;
            return (-1);
        }
        return 0;
    }

    int BE_GL_MakeCurrent(_THIS)
    {
        /* FIXME: should we glview->unlock and then glview->lock()? */
        return 0;
    }

    void BE_GL_SwapBuffers(_THIS)
    {
        SDL_Win->SwapBuffers();
    }
#endif

/* Is the system palette settable? */
    int BE_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color * colors)
    {
        int i;
        SDL_Palette *palette;
        const color_map *cmap = BScreen().ColorMap();

        /* Get the screen colormap */
        palette = _this->screen->format->palette;
        for (i = 0; i < 256; ++i) {
            palette->colors[i].r = cmap->color_list[i].red;
            palette->colors[i].g = cmap->color_list[i].green;
            palette->colors[i].b = cmap->color_list[i].blue;
        }
        return (0);
    }

    void BE_VideoQuit(_THIS)
    {
        int i, j;

        SDL_Win->Quit();
        SDL_Win = NULL;

        if (SDL_BlankCursor != NULL) {
            BE_FreeWMCursor(_this, SDL_BlankCursor);
            SDL_BlankCursor = NULL;
        }
        for (i = 0; i < NUM_MODELISTS; ++i) {
            if (SDL_modelist[i]) {
                for (j = 0; SDL_modelist[i][j]; ++j) {
                    SDL_free(SDL_modelist[i][j]);
                }
                SDL_free(SDL_modelist[i]);
                SDL_modelist[i] = NULL;
            }
        }
        /* Restore the original video mode */
        if (_this->screen) {
            if ((_this->screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) {
                BScreen bscreen;
                bscreen.SetMode(&saved_mode);
            }
            _this->screen->pixels = NULL;
        }
#if SDL_VIDEO_OPENGL
        if (_this->gl_config.dll_handle != NULL)
            unload_add_on((image_id) _this->gl_config.dll_handle);
#endif

        SDL_QuitBeApp();
    }

};                              /* Extern C */

/* vi: set ts=4 sw=4 expandtab: */
