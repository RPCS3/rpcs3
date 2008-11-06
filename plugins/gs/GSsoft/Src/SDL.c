/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
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
#include <SDL_gfxPrimitives.h>
//#include <SDL_framerate.h>

#include "GS.h"
#include "Draw.h"
#include "Rec.h"
#include "scale2x.h"
#if defined(__i386__) || defined(__x86_64__)
#include "ix86.h"
#endif

SDL_Surface *surf1x;
SDL_Surface *surf2x;
SDL_Surface *surf;
SDL_SysWMinfo info;
//FPSmanager fps;
char gsmsg[256];
u32 gsmsgscount;
u32 gsmsgcount;

void DXclearScr() {
	SDL_FillRect(surf, NULL, 0);
}

s32 DXopen() {
#ifdef __LINUX__
	int screen;
	int w, h;
#endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

	surf = SDL_SetVideoMode(cmode->width, cmode->height, 0, conf.fullscreen ? SDL_FULLSCREEN | SDL_HWSURFACE : SDL_HWSURFACE);
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

	surf1x = surf2x = NULL;

//	SDL_initFramerate(&fps);
//	SDL_setFramerate(&fps, 60);

#ifdef __WIN32__
	return (s32)info.window;
#else
	return (s32)info.info.x11.display;
#endif
}

void DXclose() {
	SDL_Quit();
}

void DXcreateSurface1x(int w, int h) {
	if (surf1x != NULL) {
		if ((w == surf1x->w &&
			 h == surf1x->h))
			 return;

		SDL_FreeSurface(surf1x); surf1x = NULL;
	}

/*	if (surf->flags & SDL_HWSURFACE) {
		surf1x = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 
									  0, 0, 0, 0, 0);
		if (surf1x == NULL) printf("Error allocating video memory surface, trying with normal memory\n");
	}*/
	if (surf1x == NULL) {
		surf1x = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 
									  surf->format->BitsPerPixel, surf->format->Rmask,
									  surf->format->Gmask, surf->format->Bmask,
									  surf->format->Amask);
		if (surf1x == NULL) {
			SysMessage("Error allocating surface\n");
			exit(1);
		}
	}
}

void DXcreateSurface2x(int w, int h) {
	if (surf2x != NULL) {
		if ((w == surf2x->w &&
			 h == surf2x->h))
			 return;

		SDL_FreeSurface(surf2x); surf2x = NULL;
	}

/*	if (surf->flags & SDL_HWSURFACE) {
		surf2x = SDL_CreateRGBSurface(SDL_HWSURFACE, w*2, h*2, 
									  0, 0, 0, 0, 0);
		if (surf2x == NULL) printf("Error allocating video memory surface, trying with normal memory\n");
	}*/
	if (surf2x == NULL) {
		surf2x = SDL_CreateRGBSurface(SDL_SWSURFACE, w*2, h*2, 
									  surf->format->BitsPerPixel, surf->format->Rmask,
									  surf->format->Gmask, surf->format->Bmask,
									  surf->format->Amask);
		if (surf2x == NULL) conf.filter = 0;
	}
}

void DXblit(int y) {
	if (conf.filter) {
		SDL_Rect dst;

		if (surf1x == NULL) return;

		SDL_LockSurface(surf1x);
		ScrBlit(surf1x, 0);
		SDL_UnlockSurface(surf1x);

		scale2x(surf1x, surf2x);

		dst.x = 0; dst.y = y;
		dst.w = surf->w; dst.h = surf->h - dst.y;
		SDL_SoftStretch(surf2x, NULL, surf, &dst);
	} else {
		SDL_LockSurface(surf);
		ScrBlit(surf, y);
		SDL_UnlockSurface(surf);
/*		SDL_LockSurface(surf1x);

		ScrBlit(surf1x);

		dst.x = 0; dst.y = y;
		dst.w = surf->w; dst.h = surf->h - dst.y;
//		SDL_BlitSurface(surf1x, NULL, surf, NULL);
		SDL_LockSurface(surf);
		SDL_SoftStretch(surf1x, NULL, surf, &dst);
		SDL_UnlockSurface(surf);

		SDL_UnlockSurface(surf1x);*/
	}
}

int  DXsetGSmode(int w, int h) {
	if (w <= 0 || h <= 0) return 0;

	DXcreateSurface1x(w, h);
	if (conf.filter) DXcreateSurface2x(w, h);
	return 0;
}

void DXupdate() {
	SDL_Rect rect;
	static int to;
	static int fpsc, fc;
	static u64 cto;
	int ticks;
	u64 cticks;
	int i;
#if defined(__i386__) || defined(__x86_64__)
	u64 tick;

	tick = GetCPUTick();
#endif

	fc++;

	if (conf.fps || conf.frameskip) {
		ticks = SDL_GetTicks();
		fpsc = ticks - to;
		to = ticks;
	}
#if defined(__i386__) || defined(__x86_64__)
	cticks = GetCPUTick() - cto;
	cto = GetCPUTick();
#endif

/*	if (conf.frameskip) {
		if ((fpscount % 3) == 0) {
			norender = 1;
		} else if (norender) {
			norender = 0;
		}
	}*/

#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
	rect.x = 0; rect.y = 0;
	rect.w = cmode->width;
	rect.h = 15;
	if (conf.fps || norender) {
		char title[256];
		char tmp[256];

		if (fpspos < 0) fpspos = 0;
#ifndef GS_LOG
		if (fpspos > 5) fpspos = 5;
#else
		if (fpspos > 6) fpspos = 6;
#endif
		switch (fpspos) {
			case 0: // NoRender
				sprintf(tmp, "NoRender %s", norender == 1 ? "On" : "Off");
				break;
			case 1: // WireFrame
				sprintf(tmp, "WireFrame %s", wireframe == 1 ? "On" : "Off");
				break;
			case 2: // ShowFullVRam
				sprintf(tmp, "ShowFullVRam %s", showfullvram == 1 ? "On" : "Off");
				break;
			case 3: // FrameSkip
				sprintf(tmp, "Frameskip %s", conf.frameskip == 1 ? "On" : "Off");
				break;
			case 4: // GSmode
				if (gs.PMODE.en[0] && gs.PMODE.en[1]) {
					sprintf(tmp, "GSmode %dx%d CRT1 %dx%d CRT2 %dx%d", surf1x->w, surf1x->h, gs.DISPLAY[0].w,gs.DISPLAY[0].h, gs.DISPLAY[1].w, gs.DISPLAY[1].h);
				} else 
				if (gs.PMODE.en[0]) {
					sprintf(tmp, "GSmode %dx%d CRT1 %dx%d", surf1x->w, surf1x->h, gs.DISPLAY[0].w,gs.DISPLAY[0].h);
				} else {
					sprintf(tmp, "GSmode %dx%d CRT2 %dx%d", surf1x->w, surf1x->h, gs.DISPLAY[1].w, gs.DISPLAY[1].h);
				}
				break;
			case 5: // DumpTexts
				sprintf(tmp, "DumpTexts %s", conf.dumptexts == 1 ? "On" : "Off");
				break;
#ifdef GS_LOG
			case 6: // Log
				sprintf(tmp, "Log %s", conf.log == 1 ? "On" : "Off");
				break;
#endif
		}
		if (fpspress) {
			switch (fpspos) {
				case 0:
					norender = 1 - norender;
					break;
				case 1:
					wireframe = 1 - wireframe;
					break;
				case 2:
					showfullvram = 1 - showfullvram;
					break;
				case 3:
					conf.frameskip = 1 - conf.frameskip;
					norender = 0;
					break;
				case 4:
					break;
				case 5:
					conf.dumptexts = 1 - conf.dumptexts;
					break;
#ifdef GS_LOG
				case 6:
					conf.log = 1 - conf.log;
					break;
#endif
			}
			fpspress = 0;
		}

		if (cmode->width <= 320) {
			sprintf(title, "FPS %.2f; %s; FC %d", 1000.0 / fpsc, tmp, fc);

			SDL_FillRect(surf, &rect, 0);
			stringRGBA(surf, 4, rect.y+4, title, 0, 0xff, 0, 0xff);
			rect.y+= 15;

			sprintf(title, "PPF %d; BPF %d; CPU %d%% - TICKS %lld", ppf, bpf, (u32)((gsticks*100)/cticks), cticks);

			SDL_FillRect(surf, &rect, 0);
			stringRGBA(surf, 4, rect.y+4, title, 0, 0xff, 0, 0xff);
			rect.y+= 15;
		} else {
			sprintf(title, "FPS %.2f; %s; FC %d; PPF %d; BPF %d; CPU %d%% - TICKS %lld", 1000.0 / fpsc, tmp, fc, ppf, bpf, (u32)((gsticks*100)/cticks), cticks);

			SDL_FillRect(surf, &rect, 0);
			stringRGBA(surf, 4, rect.y+4, title, 0, 0xff, 0, 0xff);
			rect.y+= 15;
		}
	}
	gsticks = 0;

#if defined(__i386__) || defined(__x86_64__)
	tick = GetCPUTick();
#endif
	if (gsmsg[0]) {
		char str[256];
		char *s = gsmsg;
		int len = strlen(gsmsg);

		while (len>0) {
			for (i=0; i<255; i++) {
				if (len <= 0) break;
				if (*s == '\n' || *s == '\0') { s++; break; }
				str[i] = *s++;
			}
			if (i == 0) break;
			str[i] = 0;

			SDL_FillRect(surf, &rect, 0);
			stringRGBA(surf, 4, rect.y+4, str, 0, 0xff, 0, 0xff);
			rect.y+= 15;
		}

		if ((SDL_GetTicks() - gsmsgscount) >= gsmsgcount) gsmsg[0] = 0;
	}

	if (norender == 0) {
		DXblit(rect.y);
	}

	if (conf.record) {
		SDL_LockSurface(surf);
		recFrame(surf->pixels, surf->pitch, surf->format->BitsPerPixel);
		SDL_UnlockSurface(surf);
	}
	SDL_UpdateRect(surf, 0, 0, 0, 0);
#ifdef __WIN32__
	SDL_PumpEvents();
#endif
#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
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

void CALLBACK GSprintf(int timeout, char *fmt, ...) {
	va_list list;

	va_start(list,fmt);
	vsprintf(gsmsg,fmt,list);
	va_end(list);
	gsmsgscount = SDL_GetTicks();
	gsmsgcount  = timeout*1000;
}

void CALLBACK GSgetDriverInfo(GSdriverInfo *info) {
	strcpy(info->name, "sdl");
	info->common = surf;
}
