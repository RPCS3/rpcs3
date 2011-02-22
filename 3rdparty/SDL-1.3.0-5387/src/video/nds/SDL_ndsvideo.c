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

/* SDL Nintendo DS video driver implementation
 * based on dummy driver:
 *  Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include <nds/arm9/video.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "SDL_render.h"
#include "SDL_ndsvideo.h"
#include "SDL_ndsevents_c.h"

#define NDSVID_DRIVER_NAME "nds"

/* Per Window information. */
struct NDS_WindowData {
    int hw_index;               /* index of sprite in OAM or bg from libnds */
	int bg;						/* which bg is that attached to (2 or 3) */
    int pitch, bpp;             /* useful information about the texture */
    struct {
        int x, y;
    } scale;                    /* x/y stretch (24.8 fixed point) */
    struct {
        int x, y;
    } scroll;                   /* x/y offset */
    int rotate;                 /* -32768 to 32767, texture rotation */
    u16 *vram_pixels;           /* where the pixel data is stored (a pointer into VRAM) */
};

/* Per device information. */
struct NDS_DeviceData {
	int has_bg2;				/* backgroud 2 has been attached */
	int has_bg3;				/* backgroud 3 has been attached */
    int sub;
};

/* Initialization/Query functions */
static int NDS_VideoInit(_THIS);
static int NDS_SetDisplayMode(_THIS, SDL_VideoDisplay *display,
							  SDL_DisplayMode *mode);
static void NDS_VideoQuit(_THIS);


/* SDL NDS driver bootstrap functions */
static int
NDS_Available(void)
{
    return (1);                 /* always here */
}

static int NDS_CreateWindowFramebuffer(_THIS, SDL_Window * window,
									   Uint32 * format, void ** pixels,
									   int *pitch)
{
	struct NDS_DeviceData *data = _this->driverdata;
	struct NDS_WindowData *wdata;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;
	int whichbg = -1;

	*format = SDL_PIXELFORMAT_BGR555;

    if (!SDL_PixelFormatEnumToMasks
        (*format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

	if (!data->has_bg2)
		whichbg = 2;
	else if (!data->has_bg3)
		whichbg = 3;
	else {
		SDL_SetError("Out of NDS backgrounds.");
		return -1;
	}

	wdata = SDL_calloc(1, sizeof(struct NDS_WindowData));
	if (!wdata) {
		SDL_OutOfMemory();
		return -1;
	}

	if (!data->sub) {
		if (bpp == 8) {
			wdata->hw_index =
				bgInit(whichbg, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
		} else {
			wdata->hw_index =
				bgInit(whichbg, BgType_Bmp16, BgSize_B16_256x256, 0,
					   0);
		}
	} else {
		if (bpp == 8) {
			wdata->hw_index =
				bgInitSub(whichbg, BgType_Bmp8, BgSize_B8_256x256, 0,
						  0);
		} else {
			wdata->hw_index =
				bgInitSub(whichbg, BgType_Bmp16, BgSize_B16_256x256,
						  0, 0);
		}
	}

	wdata->bg = whichbg;
	wdata->pitch = (window->w) * ((bpp+1) / 8);
	wdata->bpp = bpp;
	wdata->rotate = 0;
	wdata->scale.x = 0x100;
	wdata->scale.y = 0x100;
	wdata->scroll.x = 0;
	wdata->scroll.y = 0;
	wdata->vram_pixels = (u16 *) bgGetGfxPtr(wdata->hw_index);

	bgSetCenter(wdata->hw_index, 0, 0);
	bgSetRotateScale(wdata->hw_index, wdata->rotate, wdata->scale.x,
					 wdata->scale.y);
	bgSetScroll(wdata->hw_index, wdata->scroll.x, wdata->scroll.y);
	bgUpdate();

	*pixels = wdata->vram_pixels;
	*pitch = wdata->pitch;

	if (!data->has_bg2)
		data->has_bg2 = 1;
	else 
		data->has_bg3 = 1;

	window->driverdata = wdata;

	return 0;
}

static int NDS_UpdateWindowFramebuffer(_THIS, SDL_Window * window,
									   SDL_Rect * rects, int numrects)
{
	/* Nothing to do because writes are done directly into the
	 * framebuffer. */
    return 0;
}

static void NDS_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
	struct NDS_DeviceData *data = _this->driverdata;
    struct NDS_WindowData *wdata = window->driverdata;

	if (wdata->bg == 2)
		data->has_bg2 = 0;
	else
		data->has_bg3 = 0;

    SDL_free(wdata);
}

static void
NDS_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
NDS_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

	device->driverdata = SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
		SDL_free(device);
        SDL_OutOfMemory();
        return NULL;
    }

    /* Set the function pointers */
    device->VideoInit = NDS_VideoInit;
    device->VideoQuit = NDS_VideoQuit;
    device->SetDisplayMode = NDS_SetDisplayMode;
    device->PumpEvents = NDS_PumpEvents;
	device->CreateWindowFramebuffer = NDS_CreateWindowFramebuffer;
	device->UpdateWindowFramebuffer = NDS_UpdateWindowFramebuffer;
	device->DestroyWindowFramebuffer = NDS_DestroyWindowFramebuffer;

    device->num_displays = 2;   /* DS = dual screens */

    device->free = NDS_DeleteDevice;

    return device;
}

VideoBootStrap NDS_bootstrap = {
    NDSVID_DRIVER_NAME, "SDL NDS video driver",
    NDS_Available, NDS_CreateDevice
};

int
NDS_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    /* simple 256x192x16x60 for now */
    mode.w = 256;
    mode.h = 192;
    mode.format = SDL_PIXELFORMAT_ABGR1555;
    mode.refresh_rate = 60;
    mode.driverdata = NULL;

    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_zero(mode);
	SDL_AddDisplayMode(&_this->displays[0], &mode);

    powerOn(POWER_ALL_2D);
    irqEnable(IRQ_VBLANK);
    NDS_SetDisplayMode(_this, &_this->displays[0], &mode);

    return 0;
}

static int
NDS_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    /* right now this function is just hard-coded for 256x192 ABGR1555 */
    videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE | DISPLAY_BG_EXT_PALETTE | DISPLAY_SPR_1D_LAYOUT | DISPLAY_SPR_1D_BMP | DISPLAY_SPR_1D_BMP_SIZE_256 |      /* (try 128 if 256 is trouble.) */
                 DISPLAY_SPR_ACTIVE | DISPLAY_SPR_EXT_PALETTE); /* display on main core
                                                                   with lots of flags set for
                                                                   flexibility/capacity to render */

    /* hopefully these cover all the various things we might need to do */
    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);
    vramSetBankD(VRAM_D_MAIN_BG_0x06040000);    /* not a typo. vram d can't sub */
    vramSetBankE(VRAM_E_MAIN_SPRITE);
    vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
    vramSetBankG(VRAM_G_BG_EXT_PALETTE);
    vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
    vramSetBankI(VRAM_I_SUB_SPRITE);

    videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);    /* debug text on sub
                                                           TODO: this will change
                                                           when multi-head is
                                                           introduced in render */

    return 0;
}

void
NDS_VideoQuit(_THIS)
{
    videoSetMode(DISPLAY_SCREEN_OFF);
    videoSetModeSub(DISPLAY_SCREEN_OFF);
    vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
    vramSetBankE(VRAM_E_LCD);
    vramSetBankF(VRAM_F_LCD);
    vramSetBankG(VRAM_G_LCD);
    vramSetBankH(VRAM_H_LCD);
    vramSetBankI(VRAM_I_LCD);
}

/* vi: set ts=4 sw=4 expandtab: */
