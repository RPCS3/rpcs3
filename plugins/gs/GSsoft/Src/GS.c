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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "Rec.h"
#include "Page.h"
#include "Transfer.h"
#include "Cache.h"
#include "Regs.h"
#if defined(__i386__) || defined(__x86_64__)
#include "x86/ix86.h"
#endif

#ifdef __MSCW32__
#pragma warning(disable:4244)
#endif

char *codeclist[] = { "MPEG1", /*"DIVX", "MPEG2",*/ NULL };
char *filterslist[] = { "Disabled", "Scale2x", NULL };

u8* g_pBasePS2Mem = NULL;

const unsigned char version  = PS2E_GS_VERSION;
const unsigned char revision = 1;	// revision and build gives plugin version
const unsigned char build    = 0;

static char *libraryName     = "GSsoft Plugin";

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_GS;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build;
}

#ifdef __WIN32__

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "GSsoftdx Msg", 0);
}

#endif

#ifdef GS_LOG
void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.log || gsLog == NULL) return;

	va_start(list, fmt);
	vfprintf(gsLog, fmt, list);
	va_end(list);
}
#endif

void CALLBACK GSsetBaseMem(void* pmem) {
    g_pBasePS2Mem = (u8*)pmem;
}

void gsSetCtxt(int ctxt) {
	if (gs.ctxt == ctxt) return;
	gs.ctxt = ctxt;

#ifdef GS_LOG
	GS_LOG("gsSetCtxt %d\n", ctxt);
#endif
	offset  = &gs._offset[ctxt];
	test    = &gs._test[ctxt];
	scissor = &gs._scissor[ctxt];
	tex0	= &gs._tex0[ctxt];
	tex1	= &gs._tex1[ctxt];
	tex2	= &gs._tex2[ctxt];
	clamp	= &gs._clamp[ctxt];
	miptbp0 = &gs._miptbp0[ctxt];
	miptbp1 = &gs._miptbp1[ctxt];
	gsfb    = &gs._gsfb[ctxt];
	alpha   = &gs._alpha[ctxt];
	zbuf    = &gs._zbuf[ctxt];
	fba	= &gs._fba[ctxt];
/* 	fBuf   = vRam + gsfb->fbp * 4;
 	fBufUS = (u16*)fBuf;
 	fBufUL = (u32*)fBuf;
	zBuf   = vRam + zbuf->zbp * 4;
	zBufUS = (u16*)zBuf;
	zBufUL = (u32*)zBuf;
	tBuf   = vRam + (tex0->tbp0 & ~0x1f) * 256;
	tBufUS = (u16*)tBuf;
	tBufUL = (u32*)tBuf;
	cBuf   = vRam + (tex0->cbp & ~0x1f) * 256;
	cBufUS = (u16*)cBuf;
	cBufUL = (u32*)cBuf;*/
}

void GSreset() {
	dBuf = vRam;
	dBufUL = vRamUL;

	memset(&gs, 0, sizeof(gs));

	gs.DISPLAY[0].w = 640;
	gs.DISPLAY[0].h = 480;
	gs.DISPLAY[1].w = 640;
	gs.DISPLAY[1].h = 480;

	gs.DISPFB[0].fbw = 640;
	gs.DISPFB[0].fbh = (1024*1024) / gs.DISPFB[0].fbw;
	gs.DISPFB[1].fbw = 640;
	gs.DISPFB[1].fbh = (1024*1024) / gs.DISPFB[1].fbw;

	gs._gsfb[0].fbw = 640;
	gs._gsfb[0].fbh = (1024*1024) / gs._gsfb[0].fbw;
	gs._gsfb[1].fbw = 640;
	gs._gsfb[1].fbh = (1024*1024) / gs._gsfb[1].fbw;

	gs._scissor[0].x1 = 639;
	gs._scissor[0].y1 = 479;
	gs._scissor[1].x1 = 639;
	gs._scissor[1].y1 = 479;
	gs.ctxt = -1;

	gsSetCtxt(0);

	gs.prac = 1;
	prim = &gs._prim[0];
	gs.primC = 0;

	gs._tex0[0].tbw = 64;
	gs._tex0[1].tbw = 64;

	gs.CSRr = 0x55190000;
}

void *_align16(void *ptr) {
	return (void*)(((uptr)ptr+0x10) & ~0xf);
}

s32 CALLBACK GSinit() {
#ifdef GS_LOG
	gsLog = fopen("logs/gsLog.txt", "w");
	if (gsLog) setvbuf(gsLog, NULL,  _IONBF, 0);
	GS_LOG("GSinit\n");
#endif

	_vRam = (u8*)malloc(4*1024*1024); vRam = _align16(_vRam);
	if (vRam == NULL) return -1;

	vRamUS = (u16*)vRam;
	vRamUL = (u32*)vRam;

	GSreset();
	norender = 0;
#ifdef GS_LOG
	GS_LOG("GSinit ok\n");
#endif

	return 0;
}

void CALLBACK GSshutdown() {
	free(_vRam);
#ifdef GS_LOG
	fclose(gsLog);
#endif
}

s32 CALLBACK GSopen(void *pDsp, char *Title, int multithread) {
	s32 dsp;

#ifdef GS_LOG
	GS_LOG("GSopen\n");
#endif

	LoadConfig();

	strcpy(GStitle, Title);

	dsp = DXopen();
	if (dsp == 0) return -1;
	*(s32*)pDsp = dsp;

	if (conf.record) recOpen();
	if (conf.cache) if (CacheInit() == -1) return -1;

	DXsetGSmode(640, gs.SYNCV.vfp & 1 ? gs.SYNCV.vdp : gs.SYNCV.vdp/2);

#ifdef GS_LOG
	GS_LOG("GSopen ok\n");
#endif

	return 0;	
}

void CALLBACK GSclose() {
	DXclose();

	if (conf.record) recClose();
	if (conf.cache) CacheShutdown();

#ifdef __WIN32__
	DestroyWindow(GShwnd);
#endif
}

void CALLBACK GSirqCallback(void (*callback)()) {
	GSirq = callback;
}

void CALLBACK GSwriteCSR(u32 write)
{
    gs.CSRw = write;
}

void CALLBACK GSmakeSnapshot(char *path) {
	FILE *bmpfile;
	char filename[256];     
	unsigned char header[0x36];
	long size;
	unsigned char line[1024*3];
	int w,h;
	short i,j;
	unsigned char empty[2]={0,0};
	u32 color;
	u32 snapshotnr = 0;

//	w = dispfb->fbw;
//	h = dispfb->fbh - 20;
	size = w*h*3 + 0x38;
 
	// fill in proper values for BMP
 
	// hardcoded BMP header
	memset(header,0,0x36);
	header[0]='B';
	header[1]='M';
	header[2]=size&0xff;
	header[3]=(size>>8)&0xff;
	header[4]=(size>>16)&0xff;	
	header[5]=(size>>24)&0xff;
	header[0x0a]=0x36;
	header[0x0e]=0x28;
	header[0x12]=w%256;
	header[0x13]=w/256;
	header[0x16]=h%256;
	header[0x17]=h/256;
	header[0x1a]=0x01;
	header[0x1c]=0x18;
	header[0x26]=0x12;
	header[0x27]=0x0B;
	header[0x2A]=0x12;
	header[0x2B]=0x0B;
 
 // increment snapshot value & try to get filename
	for (;;) {
		snapshotnr++;

		sprintf(filename,"%ssnap%03d.bmp", path, snapshotnr);

		bmpfile=fopen(filename,"rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	}

	// try opening new snapshot file
	if((bmpfile=fopen(filename,"wb"))==NULL)
		return;

	fwrite(header,0x36,1,bmpfile);
	for(i=h-1;i>=0;i--) {
		for(j=0;j<w;j++) {
//			color = readPixel32(j, i, 0, dispfb->fbw);
			line[j*3+2]=(color    )&0xff;
			line[j*3+1]=(color>> 8)&0xff;
			line[j*3+0]=(color>>16)&0xff;
		}
		fwrite(line,w*3,1,bmpfile);
	}
	fwrite(empty,0x2,1,bmpfile);
	fclose(bmpfile);  
}

void CALLBACK GSvsync(int interlace) {
#ifdef GS_LOG
	GS_LOG("\nGSvsync\n\n");
#endif
	gs.interlace = 1 - gs.interlace;

	DXupdate();

	ppf = 0;
	bpf = 0;
}

#define _FBmoveImage(spsm, dpsm) { \
	int x, y; \
	u32 pixel; \
	if (s->x < d->x) { \
		for(y=0; y<h; y++) { \
			for(x=w-1; x>=0; x--) { \
				pixel = readPixel##spsm(s->x + x, s->y + y, gs.srcbuf.bp, gs.srcbuf.bw); \
				writePixel##dpsm(d->x + x, d->y + y, pixel, gs.dstbuf.bp, gs.dstbuf.bw); \
			} \
		} \
	} else { \
		for(y=0; y<h; y++) { \
			for(x=0; x<w; x++) { \
				pixel = readPixel##spsm(s->x + x, s->y + y, gs.srcbuf.bp, gs.srcbuf.bw); \
				writePixel##dpsm(d->x + x, d->y + y, pixel, gs.dstbuf.bp, gs.dstbuf.bw); \
			} \
		} \
	} \
}

#define __FBmoveImage(bpp) \
void FBmoveImage##bpp##to32(Point *s, Point *d, int w, int h)  { _FBmoveImage(bpp,  32); } \
void FBmoveImage##bpp##to24(Point *s, Point *d, int w, int h)  { _FBmoveImage(bpp,  24); } \
void FBmoveImage##bpp##to16(Point *s, Point *d, int w, int h)  { _FBmoveImage(bpp,  16); } \
void FBmoveImage##bpp##to16S(Point *s, Point *d, int w, int h) { _FBmoveImage(bpp,  16S); } \
void FBmoveImage##bpp##to8(Point *s, Point *d, int w, int h)   { _FBmoveImage(bpp,  8); } \
void FBmoveImage##bpp##to4(Point *s, Point *d, int w, int h)   { _FBmoveImage(bpp,  4); } \
void FBmoveImage##bpp##to8H(Point *s, Point *d, int w, int h)  { _FBmoveImage(bpp,  8H); } \
void FBmoveImage##bpp##to4HL(Point *s, Point *d, int w, int h) { _FBmoveImage(bpp,  4HL); } \
void FBmoveImage##bpp##to4HH(Point *s, Point *d, int w, int h) { _FBmoveImage(bpp,  4HH); }

__FBmoveImage(32);
__FBmoveImage(24);
__FBmoveImage(16);
__FBmoveImage(16S);
__FBmoveImage(8);
__FBmoveImage(4);
__FBmoveImage(8H);
__FBmoveImage(4HL);
__FBmoveImage(4HH);

#define _FBmoveImageTO(bpp) \
	switch (gs.srcbuf.psm) { \
		case 0x0:  FBmoveImage32to##bpp(&s, &d, w, h); return; \
		case 0x1:  FBmoveImage24to##bpp(&s, &d, w, h); return; \
		case 0x2:  FBmoveImage16to##bpp(&s, &d, w, h); return; \
		case 0xA:  FBmoveImage16to##bpp(&s, &d, w, h); return; \
		case 0x13: FBmoveImage8to##bpp(&s, &d, w, h); return; \
		case 0x14: FBmoveImage4to##bpp(&s, &d, w, h); return; \
		case 0x1B: FBmoveImage8Hto##bpp(&s, &d, w, h); return; \
		case 0x24: FBmoveImage4HLto##bpp(&s, &d, w, h); return; \
		case 0x2C: FBmoveImage4HHto##bpp(&s, &d, w, h); return; \
	}

void FBmoveImage() {
	Point s, d;
	int sw, sh, dw, dh;
	int w, h;

	s.x = gs.trxpos.sx;
	s.y = gs.trxpos.sy;
	d.x = gs.trxpos.dx;
	d.y = gs.trxpos.dy;
	sw = dw = gs.imageW;
	sh = dh = gs.imageH;

	if (d.x >= gs.dstbuf.bw || d.y >= gs.dstbuf.bh) return;
	if (s.x >= gs.srcbuf.bw || s.y >= gs.srcbuf.bh) return;

	if ((d.x + dw) < 0 || (d.y + dh) < 0) return;
	if ((s.x + sw) < 0 || (s.y + sh) < 0) return;

	if (d.x < 0) { dw-= -d.x; d.x = 0; }
	if (d.y < 0) { dh-= -d.y; d.y = 0; }

	if (s.x < 0) { sw-= -s.x; s.x = 0; }
	if (s.y < 0) { sh-= -s.y; s.y = 0; }

	if ((d.x + dw) > gs.dstbuf.bw) dw = gs.dstbuf.bw;
	if ((d.y + dh) > gs.dstbuf.bh) dh = gs.dstbuf.bh;

	if ((s.x + sw) > gs.srcbuf.bw) sw = gs.srcbuf.bw;
	if ((s.y + sh) > gs.srcbuf.bh) sh = gs.srcbuf.bh;

	w = min(dw, sw);
	h = min(dh, sh);

	switch (gs.dstbuf.psm) {
		case 0x0:  _FBmoveImageTO(32); break;
		case 0x1:  _FBmoveImageTO(24); break;
		case 0x2:  _FBmoveImageTO(16); break;
		case 0xA:  _FBmoveImageTO(16S); break;
		case 0x13: _FBmoveImageTO(8); break;
		case 0x14: _FBmoveImageTO(4); break;
		case 0x1B: _FBmoveImageTO(8H); break;
		case 0x24: _FBmoveImageTO(4HL); break;
		case 0x2C: _FBmoveImageTO(4HH); break;
	}

	printf("invalid FBmoveImage psm configuraton: %x to %x\n", gs.dstbuf.psm, gs.srcbuf.psm);
}

u32 primP[2];

void GIFtag(pathInfo *path, u32 *data) {
	path->tag.nloop = data[0] & 0x7fff;
	path->tag.eop   = (data[0] >> 15) & 0x1;
	path->tag.pre   = (data[1] >> 14) & 0x1;
	path->tag.prim  = (data[1] >> 15) & 0x7ff;
	path->tag.flg   = (data[1] >> 26) & 0x3;
	path->tag.nreg  = data[1] >> 28;
	if (path->tag.nreg == 0) path->tag.nreg = 16;

#ifdef GS_LOG
	GS_LOG("GIFtag: %8.8lx_%8.8lx_%8.8lx_%8.8lx: EOP=%d, NLOOP=%x, FLG=%x, NREG=%d, PRE=%d\n",
		    data[3], data[2], data[1], data[0],
			path->tag.eop, path->tag.nloop, path->tag.flg, path->tag.nreg, path->tag.pre);
#endif

	switch (path->tag.flg) {
		case 0x0:
			gs.regs = *(u64 *)(data+2);
			gs.regn = 0;
			path->mode = 1;
			if (path->tag.pre) {
				primP[0] = path->tag.prim;
				primP[1] = 0;

				GSwrite(primP, 0);
			}
			break;

		case 0x1:
			gs.regs = *(u64 *)(data+2);
			gs.regn = 0;
			path->mode = 2;
			break;

		case 0x3:
		case 0x2:
//			if (gs.dstbuf.bw == 0) printf("gs.dstbuf == 0!!!\n");
			if (gs.imageTransfer == 0x2) {
/*#ifdef GS_LOG
				GS_LOG("moveImage %dx%d %dx%d %dx%d (dir=%d)\n",
					   gs.trxpos.sx, gs.trxpos.sy, gs.trxpos.dx, gs.trxpos.dy, gs.imageW, gs.imageH, gs.trxpos.dir);
#endif
				FBmoveImage();*/
				break;
			}
			path->mode = 3;
#ifdef GS_LOG
			GS_LOG("imageTransfer size %lx, %dx%d %dx%d (psm=%x, bp=%x)\n",
				   path->tag.nloop, gs.trxpos.dx, gs.trxpos.dy, gs.imageW, gs.imageH, gs.dstbuf.psm, gs.dstbuf.bp);
#endif
			break;
	}
}


void GIFprocessReg(u32 *data, int reg) {
	u32 out[2];
//	GS_LOG("GIFprocessReg %d %8.8x_%8.8x_%8.8x_%8.8x\n", reg, data[3], data[2], data[1], data[0]);

	switch (reg) {
		case 0x0: // prim
			out[0] = data[0];
			GSwrite(out, 0x0);
			break;

		case 0x1: // rgbaq
			out[0] = (data[0] & 0xff) | 
					((data[1] & 0xff) <<  8) |
					((data[2] & 0xff) << 16) |
					((data[3] & 0xff) << 24);
			out[1] = gs.q;
			GSwrite(out, 0x1);
			break;

		case 0x2: // st
			GSwrite(data, 0x2);
			out[0] = gs.rgba;
			out[1] = data[2];
			GSwrite(out, 0x1);
			break;

		case 0x3: // uv
			out[0] = (data[0] & 0x7fff) |
					((data[1] & 0x7fff) << 16);
			GSwrite(out, 0x3);
			break;

		case 0x4: // xyzf2
			out[0] = (data[0] & 0xffff) | 
				     ((data[1] & 0xffff) << 16);
			out[1] = ((data[2] >> 4) & 0xffffff) | 
				     ((data[3] & 0xff0) << 20);
			if ((data[3] >> 15) & 0x1)
				 GSwrite(out, 0xc);
			else GSwrite(out, 0x4);
			break;

		case 0x5: //xyz2
			out[0] = (data[0] & 0xffff) | 
				    ((data[1] & 0xffff) << 16);
			out[1] = data[2];
			if ((data[3] >> 15) & 0x1)
				 GSwrite(out, 0xd);
			else GSwrite(out, 0x5);
			break;

		case 0xe: // ad
			GSwrite(data, data[2] & 0xff);
			break;

		case 0xf: // nop
			break;

		default:
#ifdef GS_LOG
			GS_LOG("UNHANDLED GIFprocessReg %d %8.8x_%8.8x_%8.8x_%8.8x\n", reg, data[3], data[2], data[1], data[0]);
//			GS_LOG("UNHANDLED %x!!!\n",reg);
#endif
//			printf("UNHANDLED GIFprocessReg %d %8.8x_%8.8x_%8.8x_%8.8x\n", reg, data[3], data[2], data[1], data[0]);
			GSwrite(data, reg);
			break;
	}
}

void _GSgifPacket(pathInfo *path, u32 *pMem) { // 128bit
	int reg = (int)((gs.regs >> (gs.regn*4)) & 0xf);

	GIFprocessReg(pMem, reg);
	gs.regn++;
	if (path->tag.nreg == gs.regn) {
		gs.regn = 0;
		path->tag.nloop--;
	}
}

void _GSgifRegList(pathInfo *path, u32 *pMem) { // 64bit
	int reg;

	reg = (int)((gs.regs >> (gs.regn*4)) & 0xf);
	GSwrite(pMem, reg);
	gs.regn++;
	if (path->tag.nreg == gs.regn) {
		gs.regn = 0;
		path->tag.nloop--;
	}
}

void _GSgifTransfer(pathInfo *path, u32 *pMem, u32 size) {

	while (size > 0) {
		switch (path->mode) {
			case 0: /* GIF TAG */
/*				if (path->tag.eop == 1) {
					path->tag.eop = 0;
					return;
				}*/
				GIFtag(path, pMem); pMem+= 4; size--;
				break;

			case 1: /* GIF PACKET */
				while (size > 0 && path->tag.nloop > 0) {
					_GSgifPacket(path, pMem);
					pMem+= 4; size--;
				}
				if (path->tag.nloop == 0 &&
					path->mode == 1) path->mode = 0;
				break;

			case 2: /* GIF REGLIST */
				size*= 2;
				while (size > 0 && path->tag.nloop > 0) {
					_GSgifRegList(path, pMem);
					pMem+= 2; size--;
				}
				if (size & 0x1) pMem+= 2;
				size/= 2;
				if (path->tag.nloop == 0 &&
					path->mode == 2) path->mode = 0;
				break;

			case 3: /* GIF IMAGE */
				if (size < path->tag.nloop) {
					FBtransferImage(pMem, size);
					path->tag.nloop-= size;
					return;
				} else {
					FBtransferImage(pMem, path->tag.nloop);
					pMem+= path->tag.nloop*4; size-= path->tag.nloop;
					path->tag.nloop = 0; path->mode = 0;
				}
				break;

			case 4: /* GIF IMAGE (FROM VRAM) */
//				if (size < path->tag.nloop) {
					FBtransferImageSrc(pMem, size);
					pMem+= size*4; path->tag.nloop-= size;
					size = 0; path->mode = 0;
/*				} else {
					FBtransferImageSrc(pMem, path->tag.nloop);
					pMem+= path->tag.nloop*4; size-= path->tag.nloop;
					path->tag.nloop = 0; path->mode = 0;
				}*/
				break;
		}
	}
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size) {
#if defined(__i386__) || defined(__x86_64__)
	u64 tick;

	tick = GetCPUTick();
#endif

#ifdef GS_LOG
	GS_LOG("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d)\n", size, gs.path2.mode, gs.path2.tag.nloop);
#endif

	_GSgifTransfer(&gs.path2, pMem, size);

#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size) {
#if defined(__i386__) || defined(__x86_64__)
	u64 tick;

	tick = GetCPUTick();
#endif

#ifdef GS_LOG
	GS_LOG("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d)\n", size, gs.path3.mode, gs.path3.tag.nloop);
#endif

	_GSgifTransfer(&gs.path3, pMem, size);

#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
}

void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr) {
	u32 *data;
	pathInfo *path = &gs.path1;
#if defined(__i386__) || defined(__x86_64__)
	u64 tick;

	tick = GetCPUTick();
#endif


#ifdef GS_LOG
	GS_LOG("GSgifTransfer1 0x%x (mode %d)\n", addr, path->mode);
#endif

	path->tag.eop = 0;
	path->mode = 0;
	for (;;) {
		data = ((u8*)pMem)+(addr&0x3fff);
		switch (path->mode) {
			case 0: /* GIF TAG */
				if (path->tag.eop == 1) { /*if (addr > 0x4000) printf("meh %x\n", addr);*/ path->mode = 0; return; }
				GIFtag(&gs.path1, data); addr+= 4*4;
				if (path->tag.nloop == 0) { /*if (addr > 0x4000) printf("meh %x\n", addr);*/ path->mode = 0; return; }
				break;

			case 1: /* GIF PACKET */
				_GSgifPacket(&gs.path1, data);
				addr+= 4*4;
				if (path->tag.nloop == 0 &&
					path->mode == 1) path->mode = 0;
				break;

			case 2: /* GIF REGLIST */
				_GSgifRegList(&gs.path1, data);
				addr+= 2*4;
				if (path->tag.nloop == 0 &&
					path->mode == 2) {
					if (addr & 2) addr+= 2*4;
					path->mode = 0;
				}
				break;

			case 3: /* GIF IMAGE */
				FBtransferImage(pMem, 1);
				addr+= 4*4; path->tag.nloop-= 1;
				if (path->tag.nloop == 0) path->mode = 0;
				break;
		}
	}
#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
}

void CALLBACK GSreadFIFO(u64 *pMem) {
#if defined(__i386__) || defined(__x86_64__)
	u64 tick;

	tick = GetCPUTick();
#endif

#ifdef GS_LOG
	GS_LOG("GSreadFIFO\n");
#endif

	FBtransferImageSrc(pMem, 1);

#if defined(__i386__) || defined(__x86_64__)
	gsticks+= GetCPUTick() - tick;
#endif
}

#if 0
void CALLBACK GSgifTransfer64(u32 *pMem) {
#ifdef GS_LOG
	GS_LOG("GSgifTransfer64 (mode %d)\n", gs.gifMode);
#endif
	GS_LOG("GSgifTransfer64 %8.8x+%8.8x\n", pMem[1], pMem[0]);

	switch (gs.gifMode) {
		case 0: /* GIF TAG */
			printf("GIFTAG over Transfer64!!\n");
//			GIFtag(pMem); pMem+= 4; size--;
			break;

		case 1: /* GIF PACKET */
			if (gs.buffsize == 1) {
				gs.buffsize = 0;
				memcpy(&gs.buff[1], pMem, 2*4);
				_GSgifPacket((u32*)gs.buff);
			} else {
				gs.buffsize = 1;
				memcpy(&gs.buff[0], pMem, 2*4);
			}
			break;

		case 2: /* GIF REGLIST */
			_GSgifRegList(pMem);
			break;

		case 3: /* GIF IMAGE */
/*			if (size < gs.gtag.nloop) {
				FBtransferImage(pMem, size);
				gs.gtag.nloop-= size;
				return;
			} else {
				FBtransferImage(pMem, gs.gtag.nloop);
				pMem+= gs.gtag.nloop*4; size-= gs.gtag.nloop;
				gs.gtag.nloop = 0; gs.gifMode = 0;
			}
*/			break;

		case 4: /* GIF IMAGE (FROM VRAM) */
//			if (size < gs.gtag.nloop) {
/*				FBtransferImageSrc(pMem, size);
				pMem+= size*4; gs.gtag.nloop-= size;
				size = 0; gs.gifMode = 0;*/
/*			} else {
				FBtransferImageSrc(pMem, gs.gtag.nloop);
				pMem+= gs.gtag.nloop*4; size-= gs.gtag.nloop;
				gs.gtag.nloop = 0; gs.gifMode = 0;
			}*/
			break;
	}
}
#endif

typedef struct {
	u8 id[32];
	u8 vRam[4*1024*1024];
	GSinternal gs;
} GSfreezeData;

s32 CALLBACK GSfreeze(int mode, freezeData *data) {
	GSfreezeData *gsd;

	if (mode == FREEZE_LOAD) {
		gsd = (GSfreezeData*)data->data;
		if (data->size != sizeof(GSfreezeData)) {
			SysMessage("GSsoft: freeze data size incorrect, %d != %d\n", data->size, sizeof(GSfreezeData));
			return 0;
//			return -1;
		}
		if (strcmp(gsd->id, libraryName)) {
			SysMessage("GSsoft: freeze data incorrect plugin\n");
			return -1;
		}
		memcpy(vRam, gsd->vRam, 4*1024*1024);
		memcpy(&gs, &gsd->gs, sizeof(GSinternal));
	} else
	if (mode == FREEZE_SAVE) {
		data->size = sizeof(GSfreezeData);
		gsd = (GSfreezeData*)data->data;
		strcpy(gsd->id, libraryName);
		memcpy(gsd->vRam, vRam, 4*1024*1024);
		memcpy(&gsd->gs, &gs, sizeof(GSinternal));
	}
	if (mode == FREEZE_SIZE) {
		data->size = sizeof(GSfreezeData);
	}

	return 0;
}

