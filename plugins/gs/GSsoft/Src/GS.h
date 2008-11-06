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

#ifndef __GS_H__
#define __GS_H__

#include <stdio.h>
#include <SDL/SDL.h>

#define GSdefs
#include "PS2Edefs.h"

#ifdef __WIN32__

#include <windows.h>
#include <windowsx.h>

HWND GShwnd;

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <gtk/gtk.h>

#define __inline inline

typedef int BOOL;

//#define TRUE  1
//#define FALSE 0

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#endif

/////////////////////

#ifndef _RELEASE
#define GS_LOG __Log
#endif

char GStitle[256];

typedef struct {
	int x, y, w, h;
} Rect;

typedef struct {
	int x, y;
} Point;

typedef struct {
	int x0, y0;
	int x1, y1;
} Rect2;

typedef struct {
	int x, y, c;
} PointC;

typedef struct {
	int width;
	int height;
	int bpp;
} DXmode;

typedef struct {
	DXmode fmode;
	DXmode wmode;
	int fullscreen;
	int fps;
	int frameskip;
	int record;
	int cache;
	int cachesize;
	int codec;
	int filter;
#ifdef GS_LOG
	u32 log;
#endif
	int dumptexts;
} GSconf;

typedef struct {
	int x, y, f;
	u32 z;
	int u, v;
	u32 s, t, q;
	u32 rgba;
} Vertex;

DXmode modes[64];
DXmode *cmode;
GSconf conf;
int norender;
int wireframe;
int showfullvram;
int fpspos;
int fpspress;
int ppf;
int bpf;
u64 gsticks;

u8  *vRam;	// Video Ram
u16 *vRamUS;
u32 *vRamUL;
u8  *_vRam;

u8  *fBuf;	// Frame Buffer
u16 *fBufUS;
u32 *fBufUL;

u8  *dBuf;	// Display Buffer
u16 *dBufUS;
u32 *dBufUL;

u8  *zBuf;	// Z Buffer
u16 *zBufUS;
u32 *zBufUL;

u8  *iBuf;	// Image Buffer
u16 *iBufUS;
u32 *iBufUL;

u8  *tBuf;	// Texture Buffer
u16 *tBufUS;
u32 *tBufUL;

u8  *cBuf;	// Clut Buffer
u16 *cBufUS;
u32 *cBufUL;

extern char *codeclist[];
extern char *filterslist[];

typedef struct {
	int n;
	int x, y;
} Block;

typedef struct {
	int nloop;
	int eop;
	int flg;
	int nreg;
	int prim;
	int pre;
} tagInfo;

typedef union {
	s64 SD;
	u64 UD;
	s32 SL[2];
	u32 UL[2];
	s16 SS[4];
	u16 US[4];
	s8  SC[8];
	u8  UC[8];
} reg64;

/* privileged regs structs */


typedef struct {
	u32 fbp;
	int fbw;
	int fbh;
	int psm;
	int dbx;
	int dby;
} dispfbInfo;

typedef struct {
	int inter;
	int ffmd;
	int dpms;
} smode2Info;

typedef struct {
	int rc;
	int lc;
	int t1248;
	int slck;
	int cmod;
	int ex;
	int prst;
	int sint;
	int xpck;
	int pck2;
	int spml;
	int gcont;
	int phs;
	int pvs;
	int pehs;
	int pevs;
	int clksel;
	int nvck;
	int slck2;
	int vcksel;
	int vhp;
} smode1Info;

typedef struct {
	int hfp;
	int fbp;
	int hseq;
	int hsvs;
	int hs;
} synch1Info;

typedef struct {
	int hb;
	int hf;
} synch2Info;

typedef struct {
	int vfp;
	int vfpe;
	int vbp;
	int vbpe;
	int vdp;
	int vs;
} syncvInfo;

typedef struct {
	int en[2];
	int crtmd;
	int mmod;
	int amod;
	int slbg;
	u32 alp;
} pmodeInfo;

typedef struct {
	u32 exbp;
	u32 exbw;
	int fbin;
	int wffmd;
	int emoda;
	int emodc;
	int wdx;
	int wdy;
} extbufInfo;

typedef struct {
	u32 sigid;
	u32 lblid;
} siglblidInfo;

/* general purpose regs structs */

typedef struct {
	int fbp;
	int fbw;
	int fbh;
	int psm;
	int fbm;
} frameInfo;

frameInfo *gsfb;
Point *offset;
Rect2 *scissor;

typedef struct {
	int prim;
	int iip;
	int tme;
	int fge;
	int abe;
	int aa1;
	int fst;
	int ctxt;
	int fix;
} primInfo;

primInfo *prim;

typedef struct {
	int ate;
	int atst;
	u32 aref;
	int afail;
	int date;
	int datm;
	int zte;
	int ztst;
} pixTest;

pixTest *test;

#define ZTST_NEVER   0
#define ZTST_ALWAYS  1
#define ZTST_GEQUAL  2
#define ZTST_GREATER 3

#define ATST_NEVER    0
#define ATST_ALWAYS   1
#define ATST_LESS     2
#define ATST_LEQUAL   3
#define ATST_EQUAL    4
#define ATST_GEQUAL   5
#define ATST_GREATER  6
#define ATST_NOTEQUAL 7

#define AFAIL_KEEP     0
#define AFAIL_FB_ONLY  1
#define AFAIL_ZB_ONLY  2
#define AFAIL_RGB_ONLY 3


typedef struct {
	int bp;
	int bw;
	int bh;
	int psm;
} bufInfo;

typedef struct {
	int tbp0;
	int tbw;
	int psm;
	int tw, th;
	int tcc;
	int tfx;
	int cbp;
	int cpsm;
	int csm;
} tex0Info;

tex0Info *tex0;

#define TEX_MODULATE 0
#define TEX_DECAL 1
#define TEX_HIGHLIGHT 2
#define TEX_HIGHLIGHT2 3

typedef struct {
	int lcm;
	int mxl;
	int mmag;
	int mmin;
	int mtba;
	int l;
	int k;
} tex1Info;

tex1Info *tex1;

typedef struct {
	int psm;
	int cbp;
	int cpsm;
	int csm;
	int csa;
	int cld;
} tex2Info;

tex2Info *tex2;

typedef struct {
	int u, v;
} uvInfo;

typedef struct {
	int s, t;
} stInfo;

typedef struct {
	int wms;
	int wmt;
	int minu;
	int maxu;
	int minv;
	int maxv;
} clampInfo;

clampInfo *clamp;

typedef struct {
	int cbw;
	int cou;
	int cov;
} clutInfo;

typedef struct {
	int tbp[3];
	int tbw[3];
} miptbpInfo;

miptbpInfo *miptbp0;
miptbpInfo *miptbp1;

typedef struct {
	int ta[2];
	int aem;
} texaInfo;

typedef struct {
	int sx;
	int sy;
	int dx;
	int dy;
	int dir;
} trxposInfo;

typedef struct {
	int a, b, c, d;
	int fix;
} alphaInfo;

alphaInfo *alpha;

typedef struct {
	int zbp;
	int psm;
	int zmsk;
} zbufInfo;

zbufInfo *zbuf;

typedef struct {
	int fba;
} fbaInfo;

fbaInfo *fba;

typedef struct {
	int mode;
	tagInfo tag;
} pathInfo;

typedef struct {	
	Vertex gsvertex[3];
	u32 rgba;
	u32 q;
	int primC;
	int psm;
	int prac;
	int dthe;
	int colclamp;
	int fogf;
	int fogcol;
	int smask;
	int pabe;
	u64 regs;
	int regn;
	u64 buff[2];
	int buffsize;
	int ctxt;

	pmodeInfo PMODE;
	smode1Info SMODE1;
	smode2Info SMODE2;
	u64 SRFSH;
	synch1Info SYNCH1;
	synch2Info SYNCH2;
	syncvInfo SYNCV;
	dispfbInfo DISPFB[2];
	Rect DISPLAY[2];
	extbufInfo EXTBUF;
	Rect EXTDATA;
	u32 EXTWRITE;
	u32 BGCOLOR;
	u32 CSRr;
	u32 CSRw;
	u32 IMR;
	u32 BUSDIR;
	siglblidInfo SIGLBLID;

	frameInfo _gsfb[2];
	Point _offset[2];
	Rect2 _scissor[2];
	primInfo _prim[2];
	pixTest _test[2];
	bufInfo srcbuf;
	bufInfo dstbuf;
	tex0Info _tex0[2];
	tex1Info _tex1[2];
	tex2Info _tex2[2];
	uvInfo uv;
	stInfo st;
	clampInfo _clamp[2];
	clutInfo clut;
	miptbpInfo _miptbp0[2];
	miptbpInfo _miptbp1[2];
	texaInfo texa;
	trxposInfo trxpos;
	alphaInfo _alpha[2];
	zbufInfo _zbuf[2];
	fbaInfo _fba[2];

	int imageTransfer;
	int imageX, imageY, imageW, imageH;

	pathInfo path1;
	pathInfo path2;
	pathInfo path3;

	int interlace;
} GSinternal;

GSinternal gs;


FILE *gsLog;

#ifdef GS_LOG
#define RDWR_LOG /*if (conf.log & 0x00000001)*/ __Log
#define TRAN_LOG /*if (conf.log & 0x00000002)*/ __Log
#define PREG_LOG /*if (conf.log & 0x00000004)*/ __Log
#define GREG_LOG /*if (conf.log & 0x00000008)*/ __Log
#define PRIM_LOG /*if (conf.log & 0x00000010)*/ __Log
#define DRAW_LOG /*if (conf.log & 0x00000020)*/ __Log
#define WARN_LOG /*if (conf.log & 0x00000040)*/ __Log
#endif

void __Log(char *fmt, ...);

void LoadConfig();
void SaveConfig();

s32  DXopen();
void DXclose();
int  DXsetGSmode(int w, int h);
void DXupdate();
int  DXgetModes();
int  DXgetDrvs();
void DXclearScr();

void GSreset();

void (*GSirq)();

void *_align16(void *ptr);
void *SysLoadLibrary(char *lib);		// Loads Library
void *SysLoadSym(void *lib, char *sym);	// Loads Symbol from Library
char *SysLibError();					// Gets previous error loading sysbols
void SysCloseLibrary(void *lib);		// Closes Library
void SysMessage(char *fmt, ...);

#define RGBA32to16(c) \
	(u16)((((c) & 0x000000f8) >>  3) | \
	(((c) & 0x0000f800) >>  6) | \
	(((c) & 0x00f80000) >>  9) | \
	(((c) & 0x80000000) >> 16)) \

#define RGBA16to32(c) \
	(((c) & 0x001f) <<  3) | \
	(((c) & 0x03e0) <<  6) | \
	(((c) & 0x7c00) <<  9) | \
	(((c) & 0x8000) << 16) \

void gsSetCtxt(int ctxt);
void FBmoveImage();
u32  TextureSizeGS(int width, int height, int psm);

#endif
