/*  GSsoft
 *  Copyright (C) 2002-2004  GSsoft Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
//#include <SDL_gfxPrimitives.h>

#include "GS.h"
#include "Draw.h"
#include "Win32.h"
#include "Rec.h"

SDL_Surface *surf;
SDL_SysWMinfo info;

void DXclearScr() {
	SDL_FillRect(surf, NULL, 0);
}

s32 DXopen() {
#ifdef __LINUX__
	int screen;
	int w, h;
#endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

	surf = SDL_SetVideoMode(cmode->width, cmode->height, 0, conf.fullscreen ? SDL_FULLSCREEN : 0);
	if (surf == NULL) return -1;

	SDL_VERSION(&info.version);
	if (SDL_GetWMInfo(&info) == 0) return -1;

#ifdef __LINUX__
	info.info.x11.lock_func();
	screen = DefaultScreen(info.info.x11.display);
	w = DisplayWidth(info.info.x11.display, screen);
	h = DisplayHeight(info.info.x11.display, screen);
	XMoveWindow(info.info.x11.display, info.info.x11.wmwindow,
				cmode->width  < w ? (w - cmode->width)  / 2 : 0,
				cmode->height < h ? (h - cmode->height) / 2 : 0);
	info.info.x11.unlock_func();
#endif

	SDL_WM_SetCaption(GStitle, NULL);

	SETScrBlit(surf->format->BitsPerPixel);
	DXclearScr();

#ifdef __WIN32__
	return (s32)info.window;
#else
	return (s32)info.info.x11.display;
#endif
}

void DXclose() {
	SDL_Quit();
}

#include <time.h>

void DXupdate() {
	SDL_Rect rect;
	static time_t to;
	static int fpscount, fpsc, fc;
	int lPitch;
	char *sbuff;

	fc++;

	if (conf.fps || conf.frameskip) {
		if (time(NULL) > to) {
			time(&to);
			fpsc = fpscount;
			fpscount = 0;
		}
		fpscount++;
	}

	if (conf.frameskip) {
		if ((fpscount % 3) == 0) {
			norender = 1;
		} else if (norender) {
			norender = 0;
		}
	}

	if (norender == 0) {
		SDL_LockSurface(surf);
		sbuff = surf->pixels;
		lPitch = surf->pitch;

		ScrBlit(sbuff, lPitch);
		if (conf.record) recFrame(sbuff, lPitch, surf->format->BitsPerPixel);

		SDL_UnlockSurface(surf);
	}

	if (conf.fps) {
		char title[256];
		char tmp[256];

		if (fpspos < 0) fpspos = 0;
#ifndef GS_LOG
		if (fpspos > 6) fpspos = 6;
#else
		if (fpspos > 7) fpspos = 7;
#endif
		switch (fpspos) {
			case 0: // FrameSkip
				sprintf(tmp, "Frameskip %s", conf.frameskip == 1 ? "On" : "Off");
				norender = 0;
				break;
			case 1: // Stretch
				sprintf(tmp, "Stretch %s", conf.stretch == 1 ? "On" : "Off");
				break;
			/*case 2: // Misc stuff
				sprintf(tmp, "GSmode %dx%d", gsmode->w, gsmode->h);
				break;*/
			case 3: // ShowFullVRam
				sprintf(tmp, "ShowFullVRam %s", showfullvram == 1 ? "On" : "Off");
				break;
			case 4: // NoAlphaBlending
				sprintf(tmp, "NoAlphaBlending %s", noabe == 1 ? "On" : "Off");
				break;
			case 5: // DisableRendering
				sprintf(tmp, "DisableRendering %s", norender == 1 ? "On" : "Off");
				break;
			case 6: // WireFrame
				sprintf(tmp, "WireFrame %s", wireframe == 1 ? "On" : "Off");
				break;
#ifdef GS_LOG
			case 7: // Log
				sprintf(tmp, "Log %s", conf.log == 1 ? "On" : "Off");
				break;
#endif
		}
		if (fpspress) {
			switch (fpspos) {
				case 0:
					conf.frameskip = 1 - conf.frameskip;
					norender = 0;
					break;
				case 1:
					conf.stretch = 1 - conf.stretch;
					SETScrBlit(surf->format->BitsPerPixel);
					DXclearScr();
					fpspress = 0;
					return;
				case 3:
					DXclearScr();
					showfullvram = 1 - showfullvram;
					break;
				case 4:
					noabe = 1 - noabe;
					break;
				case 5:
					norender = 1 - norender;
					break;
				case 6:
					wireframe = 1 - wireframe;
					break;
#ifdef GS_LOG
				case 7:
					conf.log = 1 - conf.log;
					break;
#endif
			}
			fpspress = 0;
		}

		sprintf(title,"FPS %d -- %s - FC: %d", fpsc, tmp, fc);

		rect.x = 0; rect.y = 0;
		rect.w = cmode->width;
		rect.h = 15;
		SDL_FillRect(surf, &rect, 0);
//		stringRGBA(surf, 4, 4, title, 0, 0xff, 0, 0xff);
	}

	SDL_UpdateRect(surf, 0, 0, 0, 0);
	SDL_PumpEvents();
}

int DXgetModes() {
	SDL_Rect **rects;
	int i;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

	memset(modes, 0, sizeof(modes));
	rects = SDL_ListModes(NULL, SDL_FULLSCREEN);
	if (rects == NULL) return -1;

	for (i=0; i<64; i++) {
		if (rects[i] == NULL) break;
		modes[i].width  = rects[i]->w;
		modes[i].height = rects[i]->h;
	}

	SDL_Quit();

	return i;
}

