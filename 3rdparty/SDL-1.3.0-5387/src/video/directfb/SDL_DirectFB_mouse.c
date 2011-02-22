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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/

#include "SDL_config.h"

#include "SDL_DirectFB_video.h"

#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"

#if USE_MULTI_API
static SDL_Cursor *DirectFB_CreateCursor(SDL_Surface * surface,
                                         int hot_x, int hot_y);
static int DirectFB_ShowCursor(SDL_Cursor * cursor);
static void DirectFB_MoveCursor(SDL_Cursor * cursor);
static void DirectFB_FreeCursor(SDL_Cursor * cursor);
static void DirectFB_WarpMouse(SDL_Mouse * mouse, SDL_Window * window,
                               int x, int y);
static void DirectFB_FreeMouse(SDL_Mouse * mouse);

static int id_mask;

static DFBEnumerationResult
EnumMice(DFBInputDeviceID device_id, DFBInputDeviceDescription desc,
         void *callbackdata)
{
    DFB_DeviceData *devdata = callbackdata;

    if ((desc.type & DIDTF_MOUSE) && (device_id & id_mask)) {
        SDL_Mouse mouse;

        SDL_zero(mouse);
        mouse.id = device_id;
        mouse.CreateCursor = DirectFB_CreateCursor;
        mouse.ShowCursor = DirectFB_ShowCursor;
        mouse.MoveCursor = DirectFB_MoveCursor;
        mouse.FreeCursor = DirectFB_FreeCursor;
        mouse.WarpMouse = DirectFB_WarpMouse;
        mouse.FreeMouse = DirectFB_FreeMouse;
        mouse.cursor_shown = 1;

        SDL_AddMouse(&mouse, desc.name, 0, 0, 1);
        devdata->mouse_id[devdata->num_mice++] = device_id;
    }
    return DFENUM_OK;
}

void
DirectFB_InitMouse(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    devdata->num_mice = 0;
    if (devdata->use_linux_input) {
        /* try non-core devices first */
        id_mask = 0xF0;
        devdata->dfb->EnumInputDevices(devdata->dfb, EnumMice, devdata);
        if (devdata->num_mice == 0) {
            /* try core devices */
            id_mask = 0x0F;
            devdata->dfb->EnumInputDevices(devdata->dfb, EnumMice, devdata);
        }
    }
    if (devdata->num_mice == 0) {
        SDL_Mouse mouse;

        SDL_zero(mouse);
        mouse.CreateCursor = DirectFB_CreateCursor;
        mouse.ShowCursor = DirectFB_ShowCursor;
        mouse.MoveCursor = DirectFB_MoveCursor;
        mouse.FreeCursor = DirectFB_FreeCursor;
        mouse.WarpMouse = DirectFB_WarpMouse;
        mouse.FreeMouse = DirectFB_FreeMouse;
        mouse.cursor_shown = 1;

        SDL_AddMouse(&mouse, "Mouse", 0, 0, 1);
        devdata->num_mice = 1;
    }
}

void
DirectFB_QuitMouse(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    if (devdata->use_linux_input) {
        SDL_MouseQuit();
    } else {
        SDL_DelMouse(0);
    }
}

/* Create a cursor from a surface */
static SDL_Cursor *
DirectFB_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
    SDL_VideoDevice *dev = SDL_GetVideoDevice();

    SDL_DFB_DEVICEDATA(dev);
    DFB_CursorData *curdata;
    DFBResult ret;
    DFBSurfaceDescription dsc;
    SDL_Cursor *cursor;
    Uint32 *dest;
    Uint32 *p;
    int pitch, i;

    SDL_DFB_ALLOC_CLEAR(cursor, 1, sizeof(*cursor));
    SDL_DFB_ALLOC_CLEAR(curdata, 1, sizeof(*curdata));

    dsc.flags =
        DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
    dsc.caps = DSCAPS_VIDEOONLY;
    dsc.width = surface->w;
    dsc.height = surface->h;
    dsc.pixelformat = DSPF_ARGB;

    SDL_DFB_CHECKERR(devdata->dfb->CreateSurface(devdata->dfb, &dsc,
                                                 &curdata->surf));
    curdata->hotx = hot_x;
    curdata->hoty = hot_y;
    cursor->driverdata = curdata;

    SDL_DFB_CHECKERR(curdata->surf->Lock(curdata->surf, DSLF_WRITE,
                                         (void *) &dest, &pitch));

    /* Relies on the fact that this is only called with ARGB surface. */
    p = surface->pixels;
    for (i = 0; i < surface->h; i++)
        memcpy((char *) dest + i * pitch,
               (char *) p + i * surface->pitch, 4 * surface->w);

    curdata->surf->Unlock(curdata->surf);
    return cursor;
  error:
    return NULL;
}

/* Show the specified cursor, or hide if cursor is NULL */
static int
DirectFB_ShowCursor(SDL_Cursor * cursor)
{
    SDL_DFB_CURSORDATA(cursor);
    DFBResult ret;
    SDL_Window *window;

    window = SDL_GetFocusWindow();
    if (!window)
        return -1;
    else {
        SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);

        if (display) {
            DFB_DisplayData *dispdata =
                (DFB_DisplayData *) display->driverdata;
            DFB_WindowData *windata = (DFB_WindowData *) window->driverdata;

            if (cursor)
                SDL_DFB_CHECKERR(windata->window->
                                 SetCursorShape(windata->dfbwin,
                                                curdata->surf, curdata->hotx,
                                                curdata->hoty));

            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCooperativeLevel(dispdata->layer,
                                                 DLSCL_ADMINISTRATIVE));
            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCursorOpacity(dispdata->layer,
                                              cursor ? 0xC0 : 0x00));
            SDL_DFB_CHECKERR(dispdata->layer->
                             SetCooperativeLevel(dispdata->layer,
                                                 DLSCL_SHARED));
        }
    }

    return 0;
  error:
    return -1;
}

/* This is called when a mouse motion event occurs */
static void
DirectFB_MoveCursor(SDL_Cursor * cursor)
{

}

/* Free a window manager cursor */
static void
DirectFB_FreeCursor(SDL_Cursor * cursor)
{
    SDL_DFB_CURSORDATA(cursor);

    SDL_DFB_RELEASE(curdata->surf);
    SDL_DFB_FREE(cursor->driverdata);
    SDL_DFB_FREE(cursor);
}

/* Warp the mouse to (x,y) */
static void
DirectFB_WarpMouse(SDL_Mouse * mouse, SDL_Window * window, int x, int y)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
    DFB_WindowData *windata = (DFB_WindowData *) window->driverdata;
    DFBResult ret;
    int cx, cy;

    SDL_DFB_CHECKERR(windata->dfbwin->GetPosition(windata->dfbwin, &cx, &cy));
    SDL_DFB_CHECKERR(dispdata->layer->WarpCursor(dispdata->layer,
                                                 cx + x + windata->client.x,
                                                 cy + y + windata->client.y));

  error:
    return;
}

/* Free the mouse when it's time */
static void
DirectFB_FreeMouse(SDL_Mouse * mouse)
{
    /* nothing yet */
}

#else /* USE_MULTI_API */

void
DirectFB_InitMouse(_THIS)
{
    SDL_DFB_DEVICEDATA(_this);

    devdata->num_mice = 1;
}

void
DirectFB_QuitMouse(_THIS)
{
}


#endif

/* vi: set ts=4 sw=4 expandtab: */
