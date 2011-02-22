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

#include "SDL_x11video.h"

/*#define X11MODES_DEBUG*/

static int
get_visualinfo(Display * display, int screen, XVisualInfo * vinfo)
{
    const char *visual_id = SDL_getenv("SDL_VIDEO_X11_VISUALID");
    int depth;

    /* Look for an exact visual, if requested */
    if (visual_id) {
        XVisualInfo *vi, template;
        int nvis;

        SDL_zero(template);
        template.visualid = SDL_strtol(visual_id, NULL, 0);
        vi = XGetVisualInfo(display, VisualIDMask, &template, &nvis);
        if (vi) {
            *vinfo = *vi;
            XFree(vi);
            return 0;
        }
    }

    depth = DefaultDepth(display, screen);
    if (XMatchVisualInfo(display, screen, depth, TrueColor, vinfo) ||
        XMatchVisualInfo(display, screen, depth, PseudoColor, vinfo) ||
        XMatchVisualInfo(display, screen, depth, StaticColor, vinfo)) {
        return 0;
    }
    return -1;
}

int
X11_GetVisualInfoFromVisual(Display * display, Visual * visual, XVisualInfo * vinfo)
{
    XVisualInfo *vi;
    int nvis;

    vinfo->visualid = XVisualIDFromVisual(visual);
    vi = XGetVisualInfo(display, VisualIDMask, vinfo, &nvis);
    if (vi) {
        *vinfo = *vi;
        XFree(vi);
        return 0;
    }
    return -1;
}

Uint32
X11_GetPixelFormatFromVisualInfo(Display * display, XVisualInfo * vinfo)
{
    if (vinfo->class == DirectColor || vinfo->class == TrueColor) {
        int bpp;
        Uint32 Rmask, Gmask, Bmask, Amask;

        Rmask = vinfo->visual->red_mask;
        Gmask = vinfo->visual->green_mask;
        Bmask = vinfo->visual->blue_mask;
        if (vinfo->depth == 32) {
            Amask = (0xFFFFFFFF & ~(Rmask | Gmask | Bmask));
        } else {
            Amask = 0;
        }

        bpp = vinfo->depth;
        if (bpp == 24) {
            int i, n;
            XPixmapFormatValues *p = XListPixmapFormats(display, &n);
            if (p) {
                for (i = 0; i < n; ++i) {
                    if (p[i].depth == 24) {
                        bpp = p[i].bits_per_pixel;
                        break;
                    }
                }
                XFree(p);
            }
        }

        return SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    }

    if (vinfo->class == PseudoColor || vinfo->class == StaticColor) {
        switch (vinfo->depth) {
        case 8:
            return SDL_PIXELTYPE_INDEX8;
        case 4:
            if (BitmapBitOrder(display) == LSBFirst) {
                return SDL_PIXELFORMAT_INDEX4LSB;
            } else {
                return SDL_PIXELFORMAT_INDEX4MSB;
            }
            break;
        case 1:
            if (BitmapBitOrder(display) == LSBFirst) {
                return SDL_PIXELFORMAT_INDEX1LSB;
            } else {
                return SDL_PIXELFORMAT_INDEX1MSB;
            }
            break;
        }
    }

    return SDL_PIXELFORMAT_UNKNOWN;
}

int
X11_InitModes(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int screen;

    for (screen = 0; screen < ScreenCount(data->display); ++screen) {
        XVisualInfo vinfo;
        SDL_VideoDisplay display;
        SDL_DisplayData *displaydata;
        SDL_DisplayMode mode;
        XPixmapFormatValues *pixmapFormats;
        int i, n;

        if (get_visualinfo(data->display, screen, &vinfo) < 0) {
            continue;
        }

        mode.format = X11_GetPixelFormatFromVisualInfo(data->display, &vinfo);
        if (SDL_ISPIXELFORMAT_INDEXED(mode.format)) {
            /* We don't support palettized modes now */
            continue;
        }
        mode.w = DisplayWidth(data->display, screen);
        mode.h = DisplayHeight(data->display, screen);
        mode.refresh_rate = 0;
        mode.driverdata = NULL;

        displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
        if (!displaydata) {
            continue;
        }
        displaydata->screen = screen;
        displaydata->visual = vinfo.visual;
        displaydata->depth = vinfo.depth;

        displaydata->scanline_pad = SDL_BYTESPERPIXEL(mode.format) * 8;
        pixmapFormats = XListPixmapFormats(data->display, &n);
        if (pixmapFormats) {
            for (i = 0; i < n; ++i) {
                if (pixmapFormats[i].depth == displaydata->depth) {
                    displaydata->scanline_pad = pixmapFormats[i].scanline_pad;
                    break;
                }
            }
            XFree(pixmapFormats);
        }

        SDL_zero(display);
        display.desktop_mode = mode;
        display.current_mode = mode;
        display.driverdata = displaydata;
        SDL_AddVideoDisplay(&display);
    }
    if (_this->num_displays == 0) {
        SDL_SetError("No available displays");
        return -1;
    }
    return 0;
}

/* Global for the error handler */
int vm_event, vm_error = -1;

#if SDL_VIDEO_DRIVER_X11_XINERAMA
static SDL_bool
CheckXinerama(Display * display, int *major, int *minor)
{
    const char *env;

    /* Default the extension not available */
    *major = *minor = 0;

    /* Allow environment override */
    env = getenv("SDL_VIDEO_X11_XINERAMA");
    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    /* Query the extension version */
    if (!SDL_NAME(XineramaQueryExtension) (display, major, minor) ||
        !SDL_NAME(XineramaIsActive) (display)) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
#endif /* SDL_VIDEO_DRIVER_X11_XINERAMA */

#if SDL_VIDEO_DRIVER_X11_XRANDR
static SDL_bool
CheckXRandR(Display * display, int *major, int *minor)
{
    const char *env;

    /* Default the extension not available */
    *major = *minor = 0;

    /* Allow environment override */
    env = getenv("SDL_VIDEO_X11_XRANDR");
    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    if (!SDL_X11_HAVE_XRANDR) {
        return SDL_FALSE;
    }

    /* Query the extension version */
    if (!XRRQueryVersion(display, major, minor)) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
#endif /* SDL_VIDEO_DRIVER_X11_XRANDR */

#if SDL_VIDEO_DRIVER_X11_VIDMODE
static SDL_bool
CheckVidMode(Display * display, int *major, int *minor)
{
    const char *env;

    /* Default the extension not available */
    *major = *minor = 0;

    /* Allow environment override */
    env = getenv("SDL_VIDEO_X11_VIDMODE");
    if (env && !SDL_atoi(env)) {
        return SDL_FALSE;
    }

    /* Query the extension version */
    vm_error = -1;
    if (!SDL_NAME(XF86VidModeQueryExtension) (display, &vm_event, &vm_error)
        || !SDL_NAME(XF86VidModeQueryVersion) (display, major, minor)) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static
Bool SDL_NAME(XF86VidModeGetModeInfo) (Display * dpy, int scr,
                                       SDL_NAME(XF86VidModeModeInfo) * info)
{
    Bool retval;
    int dotclock;
    SDL_NAME(XF86VidModeModeLine) l;
    SDL_zerop(info);
    SDL_zero(l);
    retval = SDL_NAME(XF86VidModeGetModeLine) (dpy, scr, &dotclock, &l);
    info->dotclock = dotclock;
    info->hdisplay = l.hdisplay;
    info->hsyncstart = l.hsyncstart;
    info->hsyncend = l.hsyncend;
    info->htotal = l.htotal;
    info->hskew = l.hskew;
    info->vdisplay = l.vdisplay;
    info->vsyncstart = l.vsyncstart;
    info->vsyncend = l.vsyncend;
    info->vtotal = l.vtotal;
    info->flags = l.flags;
    info->privsize = l.privsize;
    info->private = l.private;
    return retval;
}

static int
calculate_rate(SDL_NAME(XF86VidModeModeInfo) * info)
{
    return (info->htotal
            && info->vtotal) ? (1000 * info->dotclock / (info->htotal *
                                                         info->vtotal)) : 0;
}

static void
save_mode(Display * display, SDL_DisplayData * data)
{
    SDL_NAME(XF86VidModeGetModeInfo) (display, data->screen,
                                      &data->saved_mode);
    SDL_NAME(XF86VidModeGetViewPort) (display, data->screen,
                                      &data->saved_view.x,
                                      &data->saved_view.y);
}

/*
static void
restore_mode(Display * display, SDL_DisplayData * data)
{
    SDL_NAME(XF86VidModeModeInfo) mode;

    if (SDL_NAME(XF86VidModeGetModeInfo) (display, data->screen, &mode)) {
        if (SDL_memcmp(&mode, &data->saved_mode, sizeof(mode)) != 0) {
            SDL_NAME(XF86VidModeSwitchToMode) (display, data->screen,
                                               &data->saved_mode);
        }
    }
    if ((data->saved_view.x != 0) || (data->saved_view.y != 0)) {
        SDL_NAME(XF86VidModeSetViewPort) (display, data->screen,
                                          data->saved_view.x,
                                          data->saved_view.y);
    }
}
*/
#endif /* SDL_VIDEO_DRIVER_X11_VIDMODE */

void
X11_GetDisplayModes(_THIS, SDL_VideoDisplay * sdl_display)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    SDL_DisplayData *data = (SDL_DisplayData *) sdl_display->driverdata;
#if SDL_VIDEO_DRIVER_X11_XINERAMA
    int xinerama_major, xinerama_minor;
    int screens;
    SDL_NAME(XineramaScreenInfo) * xinerama;
#endif
#if SDL_VIDEO_DRIVER_X11_XRANDR
    int xrandr_major, xrandr_minor;
    int nsizes, nrates;
    XRRScreenSize *sizes;
    short *rates;
#endif
#if SDL_VIDEO_DRIVER_X11_VIDMODE
    int vm_major, vm_minor;
    int nmodes;
    SDL_NAME(XF86VidModeModeInfo) ** modes;
#endif
    int screen_w;
    int screen_h;
    SDL_DisplayMode mode;

    /* Unfortunately X11 requires the window to be created with the correct
     * visual and depth ahead of time, but the SDL API allows you to create
     * a window before setting the fullscreen display mode.  This means that
     * we have to use the same format for all windows and all display modes.
     * (or support recreating the window with a new visual behind the scenes)
     */
    mode.format = sdl_display->current_mode.format;
    mode.driverdata = NULL;

    data->use_xinerama = 0;
    data->use_xrandr = 0;
    data->use_vidmode = 0;
    screen_w = DisplayWidth(display, data->screen);
    screen_h = DisplayHeight(display, data->screen);

#if SDL_VIDEO_DRIVER_X11_XINERAMA
    /* Query Xinerama extention */
    if (CheckXinerama(display, &xinerama_major, &xinerama_minor)) {
#ifdef X11MODES_DEBUG
        printf("X11 detected Xinerama:\n");
#endif
        xinerama = SDL_NAME(XineramaQueryScreens) (display, &screens);
        if (xinerama) {
            int i;
            for (i = 0; i < screens; i++) {
#ifdef X11MODES_DEBUG
                printf("xinerama %d: %dx%d+%d+%d\n",
                       xinerama[i].screen_number,
                       xinerama[i].width, xinerama[i].height,
                       xinerama[i].x_org, xinerama[i].y_org);
#endif
                if (xinerama[i].screen_number == data->screen) {
                    data->use_xinerama =
                        xinerama_major * 100 + xinerama_minor;
                    data->xinerama_info = xinerama[i];
                }
            }
            XFree(xinerama);
        }

        if (data->use_xinerama) {
            /* Add the full xinerama mode */
            if (screen_w > data->xinerama_info.width ||
                screen_h > data->xinerama_info.height) {
                mode.w = screen_w;
                mode.h = screen_h;
                mode.refresh_rate = 0;
                SDL_AddDisplayMode(sdl_display, &mode);
            }

            /* Add the head xinerama mode */
            mode.w = data->xinerama_info.width;
            mode.h = data->xinerama_info.height;
            mode.refresh_rate = 0;
            SDL_AddDisplayMode(sdl_display, &mode);
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_XINERAMA */

#if SDL_VIDEO_DRIVER_X11_XRANDR
    /* XRandR */
    /* require at least XRandR v1.0 (arbitrary) */
    if (CheckXRandR(display, &xrandr_major, &xrandr_minor)
        && xrandr_major >= 1) {
#ifdef X11MODES_DEBUG
        fprintf(stderr, "XRANDR: XRRQueryVersion: V%d.%d\n",
                xrandr_major, xrandr_minor);
#endif

        /* save the screen configuration since we must reference it
           each time we toggle modes.
         */
        data->screen_config =
            XRRGetScreenInfo(display, RootWindow(display, data->screen));

        /* retrieve the list of resolution */
        sizes = XRRConfigSizes(data->screen_config, &nsizes);
        if (nsizes > 0) {
            int i, j;
            for (i = 0; i < nsizes; i++) {
                mode.w = sizes[i].width;
                mode.h = sizes[i].height;

                rates = XRRConfigRates(data->screen_config, i, &nrates);
                for (j = 0; j < nrates; ++j) {
                    mode.refresh_rate = rates[j];
#ifdef X11MODES_DEBUG
                    fprintf(stderr,
                            "XRANDR: mode = %4d[%d], w = %4d, h = %4d, rate = %4d\n",
                            i, j, mode.w, mode.h, mode.refresh_rate);
#endif
                    SDL_AddDisplayMode(sdl_display, &mode);
                }
            }

            data->use_xrandr = xrandr_major * 100 + xrandr_minor;
            data->saved_size =
                XRRConfigCurrentConfiguration(data->screen_config,
                                              &data->saved_rotation);
            data->saved_rate = XRRConfigCurrentRate(data->screen_config);
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_XRANDR */

#if SDL_VIDEO_DRIVER_X11_VIDMODE
    /* XVidMode */
    if (!data->use_xrandr &&
#if SDL_VIDEO_DRIVER_X11_XINERAMA
        (!data->use_xinerama || data->xinerama_info.screen_number == 0) &&
#endif
        CheckVidMode(display, &vm_major, &vm_minor) &&
        SDL_NAME(XF86VidModeGetAllModeLines) (display, data->screen, &nmodes,
                                              &modes)) {
        int i;

#ifdef X11MODES_DEBUG
        printf("VidMode modes: (unsorted)\n");
        for (i = 0; i < nmodes; ++i) {
            printf("Mode %d: %d x %d @ %d\n", i,
                   modes[i]->hdisplay, modes[i]->vdisplay,
                   calculate_rate(modes[i]));
        }
#endif
        for (i = 0; i < nmodes; ++i) {
            mode.w = modes[i]->hdisplay;
            mode.h = modes[i]->vdisplay;
            mode.refresh_rate = calculate_rate(modes[i]);
            SDL_AddDisplayMode(sdl_display, &mode);
        }
        XFree(modes);

        data->use_vidmode = vm_major * 100 + vm_minor;
        save_mode(display, data);
    }
#endif /* SDL_VIDEO_DRIVER_X11_VIDMODE */

    if (!data->use_xrandr && !data->use_vidmode) {
        mode.w = screen_w;
        mode.h = screen_h;
        mode.refresh_rate = 0;
        SDL_AddDisplayMode(sdl_display, &mode);
    }
#ifdef X11MODES_DEBUG
    if (data->use_xinerama) {
        printf("Xinerama is enabled\n");
    }

    if (data->use_xrandr) {
        printf("XRandR is enabled\n");
    }

    if (data->use_vidmode) {
        printf("VidMode is enabled\n");
    }
#endif /* X11MODES_DEBUG */
}

static void
get_real_resolution(Display * display, SDL_DisplayData * data, int *w, int *h,
                    int *rate)
{
#if SDL_VIDEO_DRIVER_X11_XRANDR
    if (data->use_xrandr) {
        int nsizes;
        XRRScreenSize *sizes;

        sizes = XRRConfigSizes(data->screen_config, &nsizes);
        if (nsizes > 0) {
            int cur_size;
            Rotation cur_rotation;

            cur_size =
                XRRConfigCurrentConfiguration(data->screen_config,
                                              &cur_rotation);
            *w = sizes[cur_size].width;
            *h = sizes[cur_size].height;
            *rate = XRRConfigCurrentRate(data->screen_config);
#ifdef X11MODES_DEBUG
            fprintf(stderr,
                    "XRANDR: get_real_resolution: w = %d, h = %d, rate = %d\n",
                    *w, *h, *rate);
#endif
            return;
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_XRANDR */

#if SDL_VIDEO_DRIVER_X11_VIDMODE
    if (data->use_vidmode) {
        SDL_NAME(XF86VidModeModeInfo) mode;

        if (SDL_NAME(XF86VidModeGetModeInfo) (display, data->screen, &mode)) {
            *w = mode.hdisplay;
            *h = mode.vdisplay;
            *rate = calculate_rate(&mode);
            return;
        }
    }
#endif /* SDL_VIDEO_DRIVER_X11_VIDMODE */

#if SDL_VIDEO_DRIVER_X11_XINERAMA
    if (data->use_xinerama) {
        *w = data->xinerama_info.width;
        *h = data->xinerama_info.height;
        *rate = 0;
        return;
    }
#endif /* SDL_VIDEO_DRIVER_X11_XINERAMA */

    *w = DisplayWidth(display, data->screen);
    *h = DisplayHeight(display, data->screen);
    *rate = 0;
}

static void
set_best_resolution(Display * display, SDL_DisplayData * data, int w, int h,
                    int rate)
{
    int real_w, real_h, real_rate;

    /* check current mode so we can avoid uneccessary mode changes */
    get_real_resolution(display, data, &real_w, &real_h, &real_rate);
    if (w == real_w && h == real_h && (!rate || rate == real_rate)) {
        return;
    }
#if SDL_VIDEO_DRIVER_X11_XRANDR
    if (data->use_xrandr) {
#ifdef X11MODES_DEBUG
        fprintf(stderr, "XRANDR: set_best_resolution(): w = %d, h = %d\n",
                w, h);
#endif
        int i, nsizes, nrates;
        int best;
        int best_rate;
        XRRScreenSize *sizes;
        short *rates;

        /* find the smallest resolution that is at least as big as the user requested */
        best = -1;
        sizes = XRRConfigSizes(data->screen_config, &nsizes);
        for (i = 0; i < nsizes; ++i) {
            if (sizes[i].width < w || sizes[i].height < h) {
                continue;
            }
            if (sizes[i].width == w && sizes[i].height == h) {
                best = i;
                break;
            }
            if (best == -1 ||
                (sizes[i].width < sizes[best].width) ||
                (sizes[i].width == sizes[best].width
                 && sizes[i].height < sizes[best].height)) {
                best = i;
            }
        }

        if (best >= 0) {
            best_rate = 0;
            rates = XRRConfigRates(data->screen_config, best, &nrates);
            for (i = 0; i < nrates; ++i) {
                if (rates[i] == rate) {
                    best_rate = rate;
                    break;
                }
                if (!rate) {
                    /* Higher is better, right? */
                    if (rates[i] > best_rate) {
                        best_rate = rates[i];
                    }
                } else {
                    if (SDL_abs(rates[i] - rate) < SDL_abs(best_rate - rate)) {
                        best_rate = rates[i];
                    }
                }
            }
            XRRSetScreenConfigAndRate(display, data->screen_config,
                                      RootWindow(display, data->screen), best,
                                      data->saved_rotation, best_rate,
                                      CurrentTime);
        }
        return;
    }
#endif /* SDL_VIDEO_DRIVER_X11_XRANDR */

#if SDL_VIDEO_DRIVER_X11_VIDMODE
    if (data->use_vidmode) {
        SDL_NAME(XF86VidModeModeInfo) ** modes;
        int i, nmodes;
        int best;

        if (SDL_NAME(XF86VidModeGetAllModeLines)
            (display, data->screen, &nmodes, &modes)) {
            best = -1;
            for (i = 0; i < nmodes; ++i) {
                if (modes[i]->hdisplay < w || modes[i]->vdisplay < h) {
                    continue;
                }
                if (best == -1 ||
                    (modes[i]->hdisplay < modes[best]->hdisplay) ||
                    (modes[i]->hdisplay == modes[best]->hdisplay
                     && modes[i]->vdisplay < modes[best]->vdisplay)) {
                    best = i;
                    continue;
                }
                if ((modes[i]->hdisplay == modes[best]->hdisplay) &&
                    (modes[i]->vdisplay == modes[best]->vdisplay)) {
                    if (!rate) {
                        /* Higher is better, right? */
                        if (calculate_rate(modes[i]) >
                            calculate_rate(modes[best])) {
                            best = i;
                        }
                    } else {
                        if (SDL_abs(calculate_rate(modes[i]) - rate) <
                            SDL_abs(calculate_rate(modes[best]) - rate)) {
                            best = i;
                        }
                    }
                }
            }
            if (best >= 0) {
#ifdef X11MODES_DEBUG
                printf("Best Mode %d: %d x %d @ %d\n", best,
                       modes[best]->hdisplay, modes[best]->vdisplay,
                       calculate_rate(modes[best]));
#endif
                SDL_NAME(XF86VidModeSwitchToMode) (display, data->screen,
                                                   modes[best]);
            }
            XFree(modes);
        }
        return;
    }
#endif /* SDL_VIDEO_DRIVER_X11_VIDMODE */
}

int
X11_SetDisplayMode(_THIS, SDL_VideoDisplay * sdl_display, SDL_DisplayMode * mode)
{
    Display *display = ((SDL_VideoData *) _this->driverdata)->display;
    SDL_DisplayData *data = (SDL_DisplayData *) sdl_display->driverdata;

    set_best_resolution(display, data, mode->w, mode->h, mode->refresh_rate);
    return 0;
}

void
X11_QuitModes(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
