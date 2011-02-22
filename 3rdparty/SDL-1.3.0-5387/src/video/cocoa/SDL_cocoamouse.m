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

#include "SDL_events.h"
#include "SDL_cocoavideo.h"

#include "../../events/SDL_mouse_c.h"


static SDL_Cursor *
Cocoa_CreateDefaultCursor()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSCursor *nscursor;
    SDL_Cursor *cursor = NULL;

    nscursor = [NSCursor arrowCursor];

    if (nscursor) {
        cursor = SDL_calloc(1, sizeof(*cursor));
        if (cursor) {
            cursor->driverdata = nscursor;
            [nscursor retain];
        }
    }

    [pool release];

    return cursor;
}

static SDL_Cursor *
Cocoa_CreateCursor(SDL_Surface * surface, int hot_x, int hot_y)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSImage *nsimage;
    NSCursor *nscursor = NULL;
    SDL_Cursor *cursor = NULL;

    nsimage = Cocoa_CreateImage(surface);
    if (nsimage) {
        nscursor = [[NSCursor alloc] initWithImage: nsimage hotSpot: NSMakePoint(hot_x, hot_y)];
    }

    if (nscursor) {
        cursor = SDL_calloc(1, sizeof(*cursor));
        if (cursor) {
            cursor->driverdata = nscursor;
        }
    }

    [pool release];

    return cursor;
}

static void
Cocoa_FreeCursor(SDL_Cursor * cursor)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSCursor *nscursor = (NSCursor *)cursor->driverdata;

    [nscursor release];

    [pool release];
}

static int
Cocoa_ShowCursor(SDL_Cursor * cursor)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (SDL_GetMouseFocus()) {
        if (cursor) {
            NSCursor *nscursor = (NSCursor *)cursor->driverdata;

            [nscursor set];
            [NSCursor unhide];
        } else {
            [NSCursor hide];
        }
    }

    [pool release];

    return 0;
}

static void
Cocoa_WarpMouse(SDL_Window * window, int x, int y)
{
    CGPoint point;

    point.x = (CGFloat)window->x + x;
    point.y = (CGFloat)window->y + y;
    CGWarpMouseCursorPosition(point);
}

void
Cocoa_InitMouse(_THIS)
{
    SDL_Mouse *mouse = SDL_GetMouse();
    SDL_Cursor *cursor;

    mouse->CreateCursor = Cocoa_CreateCursor;
    mouse->ShowCursor = Cocoa_ShowCursor;
    mouse->WarpMouse = Cocoa_WarpMouse;
    mouse->FreeCursor = Cocoa_FreeCursor;

    cursor = Cocoa_CreateDefaultCursor();
    mouse->cursors = mouse->cur_cursor = cursor;
}

static int
ConvertMouseButtonToSDL(int button)
{
    switch (button)
    {
        case 0:
            return(SDL_BUTTON_LEFT);   /* 1 */
        case 1:
            return(SDL_BUTTON_RIGHT);  /* 3 */
        case 2:
            return(SDL_BUTTON_MIDDLE); /* 2 */
    }
    return button+1;
}

void
Cocoa_HandleMouseEvent(_THIS, NSEvent *event)
{
    /* We're correctly using views even in fullscreen mode now */
}

void
Cocoa_HandleMouseWheel(SDL_Window *window, NSEvent *event)
{
    float x = [event deltaX];
    float y = [event deltaY];

    if (x > 0) {
        x += 0.9f;
    } else if (x < 0) {
        x -= 0.9f;
    }
    if (y > 0) {
        y += 0.9f;
    } else if (y < 0) {
        y -= 0.9f;
    }
    SDL_SendMouseWheel(window, (int)x, (int)y);
}

void
Cocoa_QuitMouse(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
