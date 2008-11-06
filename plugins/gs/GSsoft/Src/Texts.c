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

#include <math.h>

#include "GS.h"
#include "Texts.h"
#include "Mem.h"
#include "Cache.h"

u32 clud[256];

void copy_clut32_8(u32 *clud) {
	int x, y;
	int cy;
	int i;

	for (y=0, cy=0; y < 8; y++) {
//		GS_LOG("clut1[%d];\n", cy);
		for(x=0, i=0; x < 8; x++, i++) {
			clud[y*32 + x] = readPixel32(i, cy, tex0->cbp, 64);
//			GS_LOG("clut[%d] = %x\n", x, clud[y*32 + x]);
		}
		for(x=16; x < 24; x++, i++) {
  			clud[y*32 + x] = readPixel32(i, cy, tex0->cbp, 64);
//			GS_LOG("clut[%d] = %x\n", x, clud[y*32 + x]);
		}
		cy++;

//		GS_LOG("clut2[%d];\n", cy);
		for(x=8, i=0; x < 16; x++, i++) {
			clud[y*32 + x] = readPixel32(i, cy, tex0->cbp, 64);
//			GS_LOG("clut[%d] = %x\n", x, clud[y*32 + x]);
		}
		for(x=24; x < 32; x++, i++) {
			clud[y*32 + x] = readPixel32(i, cy, tex0->cbp, 64);
//			GS_LOG("clut[%d] = %x\n", x, clud[y*32 + x]);
		}
		cy++;
	}
}

void copy_clut32_4(u32 *clud) {
	int x, y;

	for (y = 0; y < 2; y++) {
		for(x = 0; x < 8; x++) {
			clud[y*8 + x] = readPixel32(x, y, tex0->cbp, 64);
//			GS_LOG("clut[%d] = %x\n", x, clud[y*8 + x]);
		}
	}
}

u32 _readPixel16(int x, int y, u32 bp, u32 bw) {
	u32 c = readPixel16(x, y, bp, bw);
	u32 p;

	p = ((c & 0x7c00) << 9) |
		((c & 0x03e0) << 6) |
		((c & 0x001f) << 3);
	if (gs.texa.aem) {
		if (c & 0x8000) {
			p|= gs.texa.ta[1] << 24;
		} else {
			if (p) p|= gs.texa.ta[0] << 24;
		}
	} else {
		p|= gs.texa.ta[(c & 0x8000) >> 15] << 24;
	}

	return p;
}

u32 _readPixel16S(int x, int y, u32 bp, u32 bw) {
	u32 c = readPixel16S(x, y, bp, bw);
	u32 p;

	p = ((c & 0x7c00) << 9) |
		((c & 0x03e0) << 6) |
		((c & 0x001f) << 3);
	if (gs.texa.aem) {
		if (c & 0x8000) {
			p|= gs.texa.ta[1] << 24;
		} else {
			if (p) p|= gs.texa.ta[0] << 24;
		}
	} else {
		p|= gs.texa.ta[(c & 0x8000) >> 15] << 24;
	}

	return p;
}

u32 _readPixel24(int x, int y, u32 bp, u32 bw) {
	u32 c = readPixel24(x, y, bp, bw);

	if (gs.texa.aem) {
		if (c) c|= gs.texa.ta[0] << 24;
	} else {
		c|= gs.texa.ta[0] << 24;
	}

	return c;
}

void copy_clut16S_8_1(u32 *clup) {
	int x, y;
	int cy;
	int i;

	for (y=0, cy=0; y < 8; y++) {
		for(x=0, i=0; x < 8; x++, i++) {
			clud[y*32 + x] = _readPixel16S(i, cy, tex0->cbp, 64);
		}
		for(x=16; x < 24; x++, i++) {
  			clud[y*32 + x] = _readPixel16S(i, cy, tex0->cbp, 64);
		}
		cy++;

		for(x=8, i=0; x < 16; x++, i++) {
			clud[y*32 + x] = _readPixel16S(i, cy, tex0->cbp, 64);
		}
		for(x=24; x < 32; x++, i++) {
			clud[y*32 + x] = _readPixel16S(i, cy, tex0->cbp, 64);
		}
		cy++;
	}
}

void copy_clut16S_8_2(u32 *clup) {
	int i;

	for (i=0; i < 256; i++) {
		clud[i] = _readPixel16S(gs.clut.cou + i, gs.clut.cov, tex0->cbp, gs.clut.cbw);
	}
}

void copy_clut16_8_1(u32 *clup) {
	int x, y;
	int cy;
	int i;

	for (y=0, cy=0; y < 8; y++) {
		for(x=0, i=0; x < 8; x++, i++) {
			clud[y*32 + x] = _readPixel16(i, cy, tex0->cbp, 64);
		}
		for(x=16; x < 24; x++, i++) {
  			clud[y*32 + x] = _readPixel16(i, cy, tex0->cbp, 64);
		}
		cy++;

		for(x=8, i=0; x < 16; x++, i++) {
			clud[y*32 + x] = _readPixel16(i, cy, tex0->cbp, 64);
		}
		for(x=24; x < 32; x++, i++) {
			clud[y*32 + x] = _readPixel16(i, cy, tex0->cbp, 64);
		}
		cy++;
	}
}

void copy_clut16_8_2(u32 *clup) {
	int i;

	for (i=0; i < 256; i++) {
		clud[i] = _readPixel16(gs.clut.cou + i, gs.clut.cov, tex0->cbp, gs.clut.cbw);
	}
}

void copy_clut16_4_1(u32 *clud) {
	int x, y;

	for (y = 0; y < 2; y++) {
		for(x = 0; x < 8; x++) {
			clud[y*8 + x] = _readPixel16(x, y, tex0->cbp, 64);
		}
	}
}

void copy_clut16_4_2(u32 *clup) {
	int i;

	for (i=0; i < 16; i++) {
		clud[i] = _readPixel16(gs.clut.cou + i, gs.clut.cov, tex0->cbp, gs.clut.cbw);
	}
}

void copy_clut16S_4_1(u32 *clud) {
	int x, y;

	for (y = 0; y < 2; y++) {
		for(x = 0; x < 8; x++) {
			clud[y*8 + x] = _readPixel16S(x, y, tex0->cbp, 64);
		}
	}
}

void copy_clut16S_4_2(u32 *clup) {
	int i;

	for (i=0; i < 16; i++) {
		clud[i] = _readPixel16S(gs.clut.cou + i, gs.clut.cov, tex0->cbp, gs.clut.cbw);
	}
}

void wrapUV(int *u, int *v) {
	switch (clamp->wms) {
		case 0: // REPEAT
			*u = *u % tex0->tw;
			break;

		case 1: // CLAMP
			if (*u < 0) *u = 0;
			if (*u >= tex0->tw) *u = tex0->tw - 1;
			break;

		case 2: // REGION_CLAMP
			if (*u < clamp->minu) *u = clamp->minu;
			if (*u > clamp->maxu) *u = clamp->maxu;
			break;

		case 3: // REGION_REPEAT
			printf("REGION_REPEAT\n");
			*u = (*u & clamp->minu) | clamp->maxu;
			break;
	}

	switch (clamp->wmt) {
		case 0: // REPEAT
			*v = *v % tex0->th;
			break;

		case 1: // CLAMP
			if (*v < 0) *v = 0;
			if (*v >= tex0->th) *v = tex0->th - 1;
			break;

		case 2: // REGION_CLAMP
			if (*v < clamp->minv) *v = clamp->minv;
			if (*v > clamp->maxv) *v = clamp->maxv;
			break;

		case 3: // REGION_REPEAT
			printf("REGION_REPEAT\n");
			*v = (*v & clamp->minv) | clamp->maxv;
			break;
	}
}




u32 _GetTexturePixel32_T32B(int u, int v) {
	wrapUV(&u, &v);
	return readPixel32(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_T24B(int u, int v) {
	wrapUV(&u, &v);
	return _readPixel24(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_T16B(int u, int v) {
	wrapUV(&u, &v);
	return _readPixel16(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_T16SB(int u, int v) {
	wrapUV(&u, &v);
	return _readPixel16S(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_T8B(int u, int v) {
	wrapUV(&u, &v);
	return clud[readPixel8(u, v, tex0->tbp0, tex0->tbw)];
}

u32 _GetTexturePixel32_T4B(int u, int v) {
	wrapUV(&u, &v);
	return clud[readPixel4(u, v, tex0->tbp0, tex0->tbw)];
}

u32 _GetTexturePixel32_T8HB(int u, int v) {
	wrapUV(&u, &v);
	return clud[readPixel8H(u, v, tex0->tbp0, tex0->tbw)];
}

u32 _GetTexturePixel32_T4HLB(int u, int v) {
	wrapUV(&u, &v);
	return clud[readPixel8H(u, v, tex0->tbp0, tex0->tbw) & 0xf];
}

u32 _GetTexturePixel32_T4HHB(int u, int v) {
	wrapUV(&u, &v);
	return clud[readPixel8H(u, v, tex0->tbp0, tex0->tbw) >> 4];
}

u32 _GetTexturePixel32_Z32B(int u, int v) {
	wrapUV(&u, &v);
	return readPixel32Z(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_Z24B(int u, int v) {
	wrapUV(&u, &v);
	return readPixel24Z(u, v, tex0->tbp0, tex0->tbw);
}

/*u32 _GetTexturePixel32_Z16B(int u, int v) {
	wrapUV(&u, &v);
	return readPixel24Z(u, v, tex0->tbp0, tex0->tbw);
}

u32 _GetTexturePixel32_Z16SB(int u, int v) {
	wrapUV(&u, &v);
	return readPixel24Z(u, v, tex0->tbp0, tex0->tbw);
}*/

void _SetGetTexturePixelB() {
	switch (tex0->psm) {
		case 0x00: // PSMCT32
			GetTexturePixel32 = _GetTexturePixel32_T32B; break;
		case 0x01: // PSMCT24
			GetTexturePixel32 = _GetTexturePixel32_T24B; break;
		case 0x02: // PSMCT16
			GetTexturePixel32 = _GetTexturePixel32_T16B; break;
		case 0x0A: // PSMCT16S
			GetTexturePixel32 = _GetTexturePixel32_T16SB;break;
		case 0x13: // PSMT8
			GetTexturePixel32 = _GetTexturePixel32_T8B;  break;
		case 0x14: // PSMT4
			GetTexturePixel32 = _GetTexturePixel32_T4B;  break;
		case 0x1B: // PSMT8H
			GetTexturePixel32 = _GetTexturePixel32_T8HB; break;
		case 0x24: // PSMT4HL
			GetTexturePixel32 = _GetTexturePixel32_T4HLB; break;
		case 0x2C: // PSMT4HH
			GetTexturePixel32 = _GetTexturePixel32_T4HHB; break;
		case 0x30: // PSMZ32
			GetTexturePixel32 = _GetTexturePixel32_Z32B; break;
/*		case 0x31: // PSMZ24
			GetTexturePixel32 = _GetTexturePixel32_Z24B; break;
		case 0x32: // PSMZ16
			GetTexturePixel32 = _GetTexturePixel32_Z16B; break;
		case 0x3A: // PSMZ16S
			GetTexturePixel32 = _GetTexturePixel32_Z16SB; break;*/
		default:
			printf("unhandled psm : %x\n", tex0->psm);
			GetTexturePixel32 = _GetTexturePixel32_T32B; break;
	}
}

void SetTexture() {
	int clutf=0;

	switch (tex0->psm) {
		case 0x13: // PSMT8
		case 0x1B: // PSMT8H
			clutf = 8; break;
		case 0x14: // PSMT4
		case 0x24: // PSMT4HL
		case 0x2C: // PSMT4HH
			clutf = 4; break;
	}

	if (clutf == 8) {
		if (tex0->cpsm == 0) {
			copy_clut32_8(clud);
		}
		if (tex0->cpsm == 0x2) {
			if (tex0->csm == 0) {
				copy_clut16_8_1(clud);
			} else {
				copy_clut16_8_2(clud);
			}
		}
		if (tex0->cpsm == 0xa) {
			if (tex0->csm == 0) {
				copy_clut16S_8_1(clud);
			} else {
				copy_clut16S_8_2(clud);
			}
		}
	}

	if (clutf == 4) {
		if (tex0->cpsm == 0) {
			copy_clut32_4(clud);
		}
		if (tex0->cpsm == 0x2) {
			if (tex0->csm == 0) {
				copy_clut16_4_1(clud);
			} else {
				copy_clut16_4_2(clud);
			}
		}
		if (tex0->cpsm == 0xa) {
			if (tex0->csm == 0) {
				copy_clut16S_4_1(clud);
			} else {
				copy_clut16S_4_2(clud);
			}
		}
	}

/*	if (tex0->tbp0 & 0x1f)*/ {
		_SetGetTexturePixelB();
	} /*else
	switch (tex0->psm) {
		case 0x00: // PSMCT32
			GetTexturePixel32 = _GetTexturePixel32_T32; break;
		case 0x01: // PSMCT24
			GetTexturePixel32 = _GetTexturePixel32_T24; break;
		case 0x02: // PSMCT16
		case 0x0A: // PSMCT16S
			GetTexturePixel32 = _GetTexturePixel32_T16; break;
		case 0x13: // PSMT8
			GetTexturePixel32 = _GetTexturePixel32_T8;  break;
		case 0x14: // PSMT4
			GetTexturePixel32 = _GetTexturePixel32_T4;  break;
		case 0x1B: // PSMT8H
			GetTexturePixel32 = _GetTexturePixel32_T8H; break;
		case 0x24: // PSMT4HL
			GetTexturePixel32 = _GetTexturePixel32_T4HL;break;
		case 0x2C: // PSMT4HH
			GetTexturePixel32 = _GetTexturePixel32_T4HH;break;
		default:
			printf("unhandled psm : %x\n", tex0->psm);
			GetTexturePixel32 = _GetTexturePixel32_T32; break;
	}*/

	if (conf.cache) {
		if (CacheSetTexture() == 0) {
			GetTexturePixel32 = CacheGetTexturePixel32;
		}
	}
//	if (tex0->tbp0 == 0x75680)
	if (conf.dumptexts) DumpTexture();
}

u32 TextureSizeGS(int width, int height, int psm) {
	switch (psm) {
		case 0x00: // PSMCT32
		case 0x01: // PSMCT24
		case 0x1B: // PSMT8H
		case 0x24: // PSMT4HL
		case 0x2C: // PSMT4HH
			return (width*height*4);
		case 0x02: // PSMCT16
		case 0x0A: // PSMCT16S
			return (width*height*2);
		case 0x13: // PSMT8
			return (width*height  );
		case 0x14: // PSMT4
			return (width*height/2);
		default:
			printf("unsupported PSM %d\n", psm);
			return 0;
	}
}

void DumpTexture() {
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

	w = tex0->tw;
	h = tex0->th-20;
	size = w*h*3 + 0x38;
	printf("DumpTexture %d, %d\n", w, h);
 
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

		sprintf(filename,"tex%03ld_%x.bmp", snapshotnr, tex0->tbp0);

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
			color = GetTexturePixel32(j, i);
			line[j*3+2]=(color    )&0xff;
			line[j*3+1]=(color>> 8)&0xff;
			line[j*3+0]=(color>>16)&0xff;
		}
		fwrite(line,w*3,1,bmpfile);
	}
	fwrite(empty,0x2,1,bmpfile);
	fclose(bmpfile);  
}

