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

/* This is the BeOS version of SDL YUV video overlays */

#include "SDL_video.h"
#include "SDL_sysyuv.h"
#include "../SDL_yuvfuncs.h"

extern "C"
{

/* The functions used to manipulate software video overlays */
    static struct private_yuvhwfuncs be_yuvfuncs = {
        BE_LockYUVOverlay,
        BE_UnlockYUVOverlay,
        BE_DisplayYUVOverlay,
        BE_FreeYUVOverlay
    };

    BBitmap *BE_GetOverlayBitmap(BRect bounds, color_space cs)
    {
        BBitmap *bbitmap;
        bbitmap = new BBitmap(bounds, B_BITMAP_WILL_OVERLAY, cs);
        if (!bbitmap || bbitmap->InitCheck() != B_OK) {
            delete bbitmap;
            return 0;
        }
        overlay_restrictions r;
        bbitmap->GetOverlayRestrictions(&r);
        uint32 width = bounds.IntegerWidth() + 1;
        uint32 height = bounds.IntegerHeight() + 1;
        uint32 width_padding = 0;
        uint32 height_padding = 0;
        if ((r.source.horizontal_alignment != 0) ||
            (r.source.vertical_alignment != 0)) {
            delete bbitmap;
            return 0;
        }
        if (r.source.width_alignment != 0) {
            uint32 aligned_width = r.source.width_alignment + 1;
            if (width % aligned_width > 0) {
                width_padding = aligned_width - width % aligned_width;
            }
        }
        if (r.source.height_alignment != 0) {
            uint32 aligned_height = r.source.height_alignment + 1;
            if (height % aligned_height > 0) {
                fprintf(stderr, "GetOverlayBitmap failed height alignment\n");
                fprintf(stderr, "- height = %lu, aligned_height = %lu\n",
                        height, aligned_height);
                delete bbitmap;
                return 0;
            }
        }
        if ((r.source.min_width > width) ||
            (r.source.min_height > height) ||
            (r.source.max_width < width) || (r.source.max_height < height)) {
            fprintf(stderr, "GetOverlayBitmap failed bounds tests\n");
            delete bbitmap;
            return 0;
        }
        if ((width_padding != 0) || (height_padding != 0)) {
            delete bbitmap;
            bounds.Set(bounds.left, bounds.top,
                       bounds.right + width_padding,
                       bounds.bottom + height_padding);
            bbitmap = new BBitmap(bounds, B_BITMAP_WILL_OVERLAY, cs);
            if (!bbitmap || bbitmap->InitCheck() != B_OK) {
                fprintf(stderr, "GetOverlayBitmap failed late\n");
                delete bbitmap;
                return 0;
            }
        }
        return bbitmap;
    }

// See <GraphicsDefs.h> [btw: Cb=U, Cr=V]
// See also http://www.fourcc.org/indexyuv.htm
    enum color_space convert_color_space(Uint32 format)
    {
        switch (format) {
        case SDL_YV12_OVERLAY:
            return B_YUV9;
        case SDL_IYUV_OVERLAY:
            return B_YUV12;
        case SDL_YUY2_OVERLAY:
            return B_YCbCr422;
        case SDL_UYVY_OVERLAY:
            return B_YUV422;
        case SDL_YVYU_OVERLAY: // not supported on beos?
            return B_NO_COLOR_SPACE;
        default:
            return B_NO_COLOR_SPACE;
        }
    }

// See SDL_video.h
    int count_planes(Uint32 format)
    {
        switch (format) {
        case SDL_YV12_OVERLAY:
        case SDL_IYUV_OVERLAY:
            return 3;
        case SDL_YUY2_OVERLAY:
        case SDL_UYVY_OVERLAY:
        case SDL_YVYU_OVERLAY:
            return 1;
        default:
            return 0;
        }
    }

    SDL_Overlay *BE_CreateYUVOverlay(_THIS, int width, int height,
                                     Uint32 format, SDL_Surface * display)
    {
        SDL_Overlay *overlay;
        struct private_yuvhwdata *hwdata;
        BBitmap *bbitmap;
        int planes;
        BRect bounds;
        color_space cs;

        /* find the appropriate BeOS colorspace descriptor */
        cs = convert_color_space(format);
        if (cs == B_NO_COLOR_SPACE) {
            return NULL;
        }

        /* count planes */
        planes = count_planes(format);
        if (planes == 0) {
            return NULL;
        }
        /* TODO: figure out planar modes, if anyone cares */
        if (planes == 3) {
            return NULL;
        }

        /* Create the overlay structure */
        overlay = (SDL_Overlay *) SDL_calloc(1, sizeof(SDL_Overlay));

        if (overlay == NULL) {
            SDL_OutOfMemory();
            return NULL;
        }

        /* Fill in the basic members */
        overlay->format = format;
        overlay->w = width;
        overlay->h = height;
        overlay->hwdata = NULL;

        /* Set up the YUV surface function structure */
        overlay->hwfuncs = &be_yuvfuncs;

        /* Create the pixel data and lookup tables */
        hwdata =
            (struct private_yuvhwdata *) SDL_calloc(1,
                                                    sizeof(struct
                                                           private_yuvhwdata));

        if (hwdata == NULL) {
            SDL_OutOfMemory();
            SDL_FreeYUVOverlay(overlay);
            return NULL;
        }

        overlay->hwdata = hwdata;
        overlay->hwdata->display = display;
        overlay->hwdata->bview = NULL;
        overlay->hwdata->bbitmap = NULL;
        overlay->hwdata->locked = 0;

        /* Create the BBitmap framebuffer */
        bounds.top = 0;
        bounds.left = 0;
        bounds.right = width - 1;
        bounds.bottom = height - 1;

        BView *bview =
            new BView(bounds, "overlay", B_FOLLOW_NONE, B_WILL_DRAW);
        if (!bview) {
            SDL_OutOfMemory();
            SDL_FreeYUVOverlay(overlay);
            return NULL;
        }
        overlay->hwdata->bview = bview;
        overlay->hwdata->first_display = true;
        bview->Hide();

        bbitmap = BE_GetOverlayBitmap(bounds, cs);
        if (!bbitmap) {
            overlay->hwdata->bbitmap = NULL;
            SDL_FreeYUVOverlay(overlay);
            return NULL;
        }
        overlay->hwdata->bbitmap = bbitmap;

        overlay->planes = planes;
        overlay->pitches =
            (Uint16 *) SDL_calloc(overlay->planes, sizeof(Uint16));
        overlay->pixels =
            (Uint8 **) SDL_calloc(overlay->planes, sizeof(Uint8 *));
        if (!overlay->pitches || !overlay->pixels) {
            SDL_OutOfMemory();
            SDL_FreeYUVOverlay(overlay);
            return (NULL);
        }

        overlay->pitches[0] = bbitmap->BytesPerRow();
        overlay->pixels[0] = (Uint8 *) bbitmap->Bits();
        overlay->hw_overlay = 1;

        if (SDL_Win->LockWithTimeout(1000000) != B_OK) {
            SDL_FreeYUVOverlay(overlay);
            return (NULL);
        }
        BView *view = SDL_Win->View();
        view->AddChild(bview);
        rgb_color key;
        bview->SetViewOverlay(bbitmap, bounds, bview->Bounds(), &key,
                              B_FOLLOW_ALL,
                              B_OVERLAY_FILTER_HORIZONTAL |
                              B_OVERLAY_FILTER_VERTICAL);
        bview->SetViewColor(key);
        bview->Flush();
        SDL_Win->Unlock();

        current_overlay = overlay;

        return overlay;
    }

    int BE_LockYUVOverlay(_THIS, SDL_Overlay * overlay)
    {
        if (overlay == NULL) {
            return 0;
        }

        overlay->hwdata->locked = 1;
        return 0;
    }

    void BE_UnlockYUVOverlay(_THIS, SDL_Overlay * overlay)
    {
        if (overlay == NULL) {
            return;
        }

        overlay->hwdata->locked = 0;
    }

    int BE_DisplayYUVOverlay(_THIS, SDL_Overlay * overlay, SDL_Rect * src,
                             SDL_Rect * dst)
    {
        if ((overlay == NULL) || (overlay->hwdata == NULL)
            || (overlay->hwdata->bview == NULL) || (SDL_Win->View() == NULL)) {
            return -1;
        }
        if (SDL_Win->LockWithTimeout(50000) != B_OK) {
            return 0;
        }
        BView *bview = overlay->hwdata->bview;
        if (SDL_Win->IsFullScreen()) {
            int left, top;
            SDL_Win->GetXYOffset(left, top);
            bview->MoveTo(left + dst->x, top + dst->y);
        } else {
            bview->MoveTo(dst->x, dst->y);
        }
        bview->ResizeTo(dst->w, dst->h);
        bview->Flush();
        if (overlay->hwdata->first_display) {
            bview->Show();
            overlay->hwdata->first_display = false;
        }
        SDL_Win->Unlock();

        return 0;
    }

    void BE_FreeYUVOverlay(_THIS, SDL_Overlay * overlay)
    {
        if (overlay == NULL) {
            return;
        }

        if (overlay->hwdata == NULL) {
            return;
        }

        current_overlay = NULL;

        delete overlay->hwdata->bbitmap;

        SDL_free(overlay->hwdata);
    }

};                              // extern "C"

/* vi: set ts=4 sw=4 expandtab: */
