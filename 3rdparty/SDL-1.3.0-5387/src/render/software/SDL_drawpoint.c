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

#if !SDL_RENDER_DISABLED

#include "SDL_draw.h"
#include "SDL_drawpoint.h"


int
SDL_DrawPoint(SDL_Surface * dst, int x, int y, Uint32 color)
{
    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_DrawPoint(): Unsupported surface format");
        return -1;
    }

    /* Perform clipping */
    if (x < dst->clip_rect.x || y < dst->clip_rect.y ||
        x >= (dst->clip_rect.x + dst->clip_rect.w) ||
        y >= (dst->clip_rect.y + dst->clip_rect.h)) {
        return 0;
    }

    switch (dst->format->BytesPerPixel) {
    case 1:
        DRAW_FASTSETPIXELXY1(x, y);
        break;
    case 2:
        DRAW_FASTSETPIXELXY2(x, y);
        break;
    case 3:
        SDL_Unsupported();
        return -1;
    case 4:
        DRAW_FASTSETPIXELXY4(x, y);
        break;
    }
    return 0;
}

int
SDL_DrawPoints(SDL_Surface * dst, const SDL_Point * points, int count,
               Uint32 color)
{
    int minx, miny;
    int maxx, maxy;
    int i;
    int x, y;

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_DrawPoints(): Unsupported surface format");
        return -1;
    }

    minx = dst->clip_rect.x;
    maxx = dst->clip_rect.x + dst->clip_rect.w - 1;
    miny = dst->clip_rect.y;
    maxy = dst->clip_rect.y + dst->clip_rect.h - 1;

    for (i = 0; i < count; ++i) {
        x = points[i].x;
        y = points[i].y;

        if (x < minx || x > maxx || y < miny || y > maxy) {
            continue;
        }

        switch (dst->format->BytesPerPixel) {
        case 1:
            DRAW_FASTSETPIXELXY1(x, y);
            break;
        case 2:
            DRAW_FASTSETPIXELXY2(x, y);
            break;
        case 3:
            SDL_Unsupported();
            return -1;
        case 4:
            DRAW_FASTSETPIXELXY4(x, y);
            break;
        }
    }
    return 0;
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
