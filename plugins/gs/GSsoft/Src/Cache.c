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

#include <stdlib.h>
#include <string.h>

#include "Draw.h"
#include "Mem.h"
#include "Page.h"
#include "Texts.h"

#ifdef __MSCW32__
#pragma warning(disable:4244)
#endif

typedef struct {
	u32 *_data;
	u32 *data;
	tex0Info tex0;
} Cache;

Cache *Texts;
Cache *Texture;
int TextureCount;

int  CacheInit() {
	Texts = (Cache*)malloc(conf.cachesize * sizeof(Cache));
	memset(Texts, 0, conf.cachesize * sizeof(Cache));
	TextureCount = 0;

	return 0;
}

void _CacheLoadTexture32(Cache *tex) {
	s32 otick;
	s32 tick;
	u32 *mem;
	u32 *ptr;
	int u, v;

#ifdef __GNUC__
	__asm__ __volatile__ (
		"rdtsc\n"
		 : "=a"(otick) :
	);
#else
	__asm {
		rdtsc
		mov otick, eax
	}
#endif

	ptr = tex->data;
	for (v=0; v<tex->tex0.th; v++) {
		for (u=0; u<tex->tex0.tw; u+=8) {
			mem = &vRamUL[getPixelAddress32(u, v, tex->tex0.tbp0, tex->tex0.tbw)];
#ifdef __GNUC__
			__asm__ __volatile__ (
				"movsd %1, %%xmm0\n"
				"movhps %2, %%xmm0\n"
				"movaps %%xmm0, %0\n"
				: "=m"(&ptr[0]) : "m"(&mem[0]), "m"(&mem[4])
			);
#else
			ptr[0] = mem[0]; ptr[1] = mem[1];
			ptr[2] = mem[4]; ptr[3] = mem[5];
			ptr[4] = mem[8]; ptr[5] = mem[9];
			ptr[6] = mem[12]; ptr[7] = mem[13];
#endif
			ptr+= 8;
		}

		for (; u<tex->tex0.tw; u++) {
			*ptr++ = readPixel32(u, v, tex->tex0.tbp0, tex->tex0.tbw);
		}
	}

#ifdef __GNUC__
	__asm__ __volatile__ (
		"rdtsc\n"
		 : "=a"(tick) :
	);
#else
	__asm {
		rdtsc
		mov tick, eax
	}
#endif

	printf("%d\n", tick - otick);
}

void _CacheLoadTexture(Cache *tex) {
	u32 *ptr;
	int u, v;

	ptr = tex->data;
	for (v=0; v<tex->tex0.th; v++) {
		for (u=0; u<tex->tex0.tw; u++) {
			*ptr++ = GetTexturePixel32(u, v);
		}
	}
}

void CacheLoadTexture(int i) {
	Texts[i].tex0 = *tex0;
	Texts[i]._data = malloc(Texts[i].tex0.th*Texts[i].tex0.tw*4+16);
	Texts[i].data = _align16(Texts[i]._data);
/*	if (Texts[i].tex0.psm == 0) {
		_CacheLoadTexture32(&Texts[i]);
		return;
	}*/
	_CacheLoadTexture(&Texts[i]);
}

void CacheFreeTexture(int i) {
	free(Texts[i]._data); Texts[i]._data = NULL;
}

int  CacheSetTexture() {
	int i;

#ifdef GS_LOG
	GS_LOG("CacheSetTexture\n");
#endif
	if (tex0->tbp0 == gs._gsfb[0].fbp ||
		tex0->tbp0 == gs._gsfb[1].fbp) {
		return -1;
	}

	for (i=0; i<conf.cachesize; i++) {
		if (Texts[i]._data == NULL) continue;
		if (memcmp(&Texts[i].tex0, tex0, sizeof(tex0Info)) == 0) {
#ifdef GS_LOG
			GS_LOG("CacheSetTexture found %d\n", i);
#endif
			Texture = &Texts[i];
			return 0;
		}
	}
	if (i == conf.cachesize) {
		i = TextureCount++;
		if (TextureCount >= conf.cachesize) TextureCount = 0;
		CacheFreeTexture(i);
	}

#ifdef GS_LOG
	GS_LOG("CacheSetTexture: %dx%d: %d\n", tex0->tw, tex0->th, i);
#endif
	printf("CacheSetTexture: %x, %dx%d: %d\n", tex0->tbp0, tex0->tw, tex0->th, i);
	CacheLoadTexture(i);
	Texture = &Texts[i];
	return 0;
}

u32  CacheGetTexturePixel32(int u, int v) {
	wrapUV(&u, &v);
	if (u < 0 || u >= Texture->tex0.tw) return 0;
	if (v < 0 || v >= Texture->tex0.th) return 0;
#ifdef GS_LOG
//	GS_LOG("CacheGetTexturePixel32 %dx%d\n", u, v);
#endif
	return Texture->data[v*Texture->tex0.tw+u];
}

void CacheClear(u32 bp, int size) {
	int i;

	for (i=0; i<conf.cachesize; i++) {
		if (Texts[i].data == NULL) break;
		if (Texts[i].tex0.tbp0 == bp) {
			CacheFreeTexture(i);
		}
	}
}

void CacheShutdown() {
	int i;

	for (i=0; i<conf.cachesize; i++) {
		if (Texts[i].data) CacheFreeTexture(i);
	}
	free(Texts);
}


