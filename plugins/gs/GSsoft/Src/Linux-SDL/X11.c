#include <stdlib.h>

#include "../gs.h"
#include "../Draw.h"


SDL_Surface *display;
SDL_Surface *visual;
SDL_Surface* gc;

int screen;
int depth;

char *buffd;
SDL_Surface *buffer;


SDL_Surface * TextGC;
char *textd;

SDL_Surface *text;

static int ShowFullVRam=0;

void (*ScrBlit) (char *, char *, Rect *, int);

int DXinit = 0;

int DXswitchScrMode() {
	DXclose();
	conf.fullscreen = 1 - conf.fullscreen;
	return DXopen();
}

void DXclearScr() {
	if (conf.fullscreen) {

	} else {
		memset(buffd, 0, conf.mode.width * conf.mode.height * 4);
		
		SDL_BlitSurface ( GShwnd , NULL , buffer, NULL );
		
	}
}

void DXupdScrBlit() {
	//printf("DXupdScrBlit : %d s:%d\n",depth,conf.stretch);
	switch (depth) {
		case 15:
			if (conf.stretch) ScrBlit = ScrBlit15S;
			else ScrBlit = ScrBlit15;
			break;
		case 16:
			if (conf.stretch) ScrBlit = ScrBlit16S;
			else ScrBlit = ScrBlit16;
			break;
		case 24:
			if (conf.stretch) ScrBlit = ScrBlit24S;
			else ScrBlit = ScrBlit24;
			break;
		case 32:
			if (conf.stretch) ScrBlit = ScrBlit32S;
			else ScrBlit = ScrBlit32;
			break;
	}
}

int DXopen() {


	u32 rmask, gmask, bmask, amask;

	if (DXinit == 1) return 0;



	
	//printf("DXopen : %d \n",conf.fullscreen);

	if (conf.fullscreen) {

		return -1;

	} else {
		

		buffd = malloc(conf.mode.width * conf.mode.height * 4);
		if (buffd == NULL) return -1;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
   gmask = 0x00ff0000;
   bmask = 0x0000ff00;
   amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif	

		depth = conf.mode.bpp;

		buffer = SDL_GetVideoSurface();
		
		if ( buffer == NULL ) printf("DXopenError : Create buffer\n");
		//else  printf("DXopenOK : Create buffer\n");
		
		
	}

	DXinit = 1;

	DXupdScrBlit();
	DXclearScr();

	return 0;
}

void DXclose() {
	DXinit = 0;

	if (conf.fullscreen) {

	} else {
		SDL_FreeSurface ( buffer );
	}
}

#include <time.h>
int fpspos;
int fpspress;

void DXupdate() {
	Rect gsScr;
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
			return;
		}
	}

	if (conf.fullscreen) {
		conf.fps = 0;
	//	sbuff = dgaDev->data;
	//	lPitch = dgaDev->mode.imageWidth * (dgaDev->mode.bitsPerPixel / 8);
		
		sbuff = buffer->pixels;
		lPitch = buffer->pitch;
	
	} else {
		//sbuff = buffd;
		//lPitch = buffer->bytes_per_line;
		sbuff = (char * )buffer->pixels;
		lPitch =  buffer->pitch ;
		
	}

	if (ShowFullVRam) {
		gsScr.x = 0;
		gsScr.y = 0;
		gsScr.w = gsdspfb->fbw;
		gsScr.h = gsdspfb->fbh;

		ScrBlit(sbuff, vRam, &gsScr, lPitch);
	
	} else {
		gsScr.x = gsmode->x + gsdspfb->dbx;
		gsScr.y = gsmode->y + gsdspfb->dby;
		gsScr.w = gsmode->w;
		gsScr.h = gsmode->h;

			
		//SDL_LockSurface( buffer );	
		ScrBlit(sbuff, dBuf, &gsScr, lPitch);
		//SDL_UnlockSurface ( buffer );
		

	}

	if (conf.fps && conf.fullscreen == 0) {
		char title[256];
		char tmp[256];

		if (fpspos < 0) fpspos = 0;
#ifndef GS_LOG
		if (fpspos > 4) fpspos = 4;
#else
		if (fpspos > 5) fpspos = 5;
#endif
		switch (fpspos) {
			case 0: // FrameSkip
				sprintf(tmp, "Frameskip %s", conf.frameskip == 1 ? "On" : "Off");
				norender = 0;
				break;
			case 1: // FullScreen
				sprintf(tmp, "Fullscreen %s", conf.fullscreen == 1 ? "On" : "Off");
				break;
			case 2: // Stretch
				sprintf(tmp, "Stretch %s", conf.stretch == 1 ? "On" : "Off");
				break;
			case 3: // Misc stuff
				sprintf(tmp, "GSmode: %dx%d - FC: %d", gsmode->w, gsmode->h, fc);
				break;
			case 4: // ShowFullVRam
				sprintf(tmp, "ShowFullVRam %s", ShowFullVRam == 1 ? "On" : "Off");
				break;
#ifdef GS_LOG
			case 5: // Log
				sprintf(tmp, "Log %s - FC: %d", Log == 1 ? "On" : "Off", fc);
				break;
#endif
		}
		if (fpspress) {
			switch (fpspos) {
				case 0:
					conf.frameskip = 1 - conf.frameskip;
					break;
				case 1:
					DXswitchScrMode();
					fpspress = 0;
					return;
				case 2:
					conf.stretch = 1 - conf.stretch;
					DXupdScrBlit();
					DXclearScr();
					fpspress = 0;
					return;
				case 4:
					DXclearScr();
					ShowFullVRam = 1 - ShowFullVRam;
					break;
#ifdef GS_LOG
				case 5:
					Log = 1 - Log;
					break;
#endif
			}
			fpspress = 0;
		}

		sprintf (title,"  FPS %d -- %s", fpsc, tmp);
		
	

//		XPutImage(display, GShwnd, gc, text, 0, 0, 0, 0, conf.mode.width, 15);
//		XDrawString(display, GShwnd, TextGC, 0, 12, title, strlen(title));
	}

	if (conf.fullscreen == 0) {
		//int i = conf.fps ? 15 : 0;

		SDL_Flip(buffer );
		
		
	}

}
	


