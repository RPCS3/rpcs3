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

#ifndef _SDL_blit_h
#define _SDL_blit_h

#ifdef __MINGW32__
#include <_mingw.h>
#endif

#if defined(__MINGW32__) && defined(__MINGW64_VERSION_MAJOR)
#include <intrin.h>
#else
#ifdef __MMX__
#include <mmintrin.h>
#endif
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#endif

#include "SDL_cpuinfo.h"
#include "SDL_endian.h"
#include "SDL_surface.h"

/* SDL blit copy flags */
#define SDL_COPY_MODULATE_COLOR     0x00000001
#define SDL_COPY_MODULATE_ALPHA     0x00000002
#define SDL_COPY_BLEND              0x00000010
#define SDL_COPY_ADD                0x00000020
#define SDL_COPY_MOD                0x00000040
#define SDL_COPY_COLORKEY           0x00000100
#define SDL_COPY_NEAREST            0x00000200
#define SDL_COPY_RLE_DESIRED        0x00001000
#define SDL_COPY_RLE_COLORKEY       0x00002000
#define SDL_COPY_RLE_ALPHAKEY       0x00004000
#define SDL_COPY_RLE_MASK           (SDL_COPY_RLE_DESIRED|SDL_COPY_RLE_COLORKEY|SDL_COPY_RLE_ALPHAKEY)

/* SDL blit CPU flags */
#define SDL_CPU_ANY                 0x00000000
#define SDL_CPU_MMX                 0x00000001
#define SDL_CPU_SSE                 0x00000004
#define SDL_CPU_SSE2                0x00000008

typedef struct
{
    Uint8 *src;
    int src_w, src_h;
    int src_pitch;
    int src_skip;
    Uint8 *dst;
    int dst_w, dst_h;
    int dst_pitch;
    int dst_skip;
    SDL_PixelFormat *src_fmt;
    SDL_PixelFormat *dst_fmt;
    Uint8 *table;
    int flags;
    Uint32 colorkey;
    Uint8 r, g, b, a;
} SDL_BlitInfo;

typedef void (SDLCALL * SDL_BlitFunc) (SDL_BlitInfo * info);

typedef struct
{
    Uint32 src_format;
    Uint32 dst_format;
    int flags;
    int cpu;
    SDL_BlitFunc func;
} SDL_BlitFuncEntry;

/* Blit mapping definition */
typedef struct SDL_BlitMap
{
    SDL_Surface *dst;
    int identity;
    SDL_blit blit;
    void *data;
    SDL_BlitInfo info;

    /* the version count matches the destination; mismatch indicates
       an invalid mapping */
    Uint32 palette_version;
} SDL_BlitMap;

/* Functions found in SDL_blit.c */
extern int SDL_CalculateBlit(SDL_Surface * surface);

/* Functions found in SDL_blit_*.c */
extern SDL_BlitFunc SDL_CalculateBlit0(SDL_Surface * surface);
extern SDL_BlitFunc SDL_CalculateBlit1(SDL_Surface * surface);
extern SDL_BlitFunc SDL_CalculateBlitN(SDL_Surface * surface);
extern SDL_BlitFunc SDL_CalculateBlitA(SDL_Surface * surface);

/*
 * Useful macros for blitting routines
 */

#if defined(__GNUC__)
#define DECLARE_ALIGNED(t,v,a)  t __attribute__((aligned(a))) v
#elif defined(_MSC_VER)
#define DECLARE_ALIGNED(t,v,a)  __declspec(align(a)) t v
#else
#define DECLARE_ALIGNED(t,v,a)  t v
#endif

/* Load pixel of the specified format from a buffer and get its R-G-B values */
/* FIXME: rescale values to 0..255 here? */
#define RGB_FROM_PIXEL(Pixel, fmt, r, g, b)				\
{									\
	r = (((Pixel&fmt->Rmask)>>fmt->Rshift)<<fmt->Rloss); 		\
	g = (((Pixel&fmt->Gmask)>>fmt->Gshift)<<fmt->Gloss); 		\
	b = (((Pixel&fmt->Bmask)>>fmt->Bshift)<<fmt->Bloss); 		\
}
#define RGB_FROM_RGB565(Pixel, r, g, b)					\
{									\
	r = (((Pixel&0xF800)>>11)<<3);		 			\
	g = (((Pixel&0x07E0)>>5)<<2); 					\
	b = ((Pixel&0x001F)<<3); 					\
}
#define RGB_FROM_RGB555(Pixel, r, g, b)					\
{									\
	r = (((Pixel&0x7C00)>>10)<<3);		 			\
	g = (((Pixel&0x03E0)>>5)<<3); 					\
	b = ((Pixel&0x001F)<<3); 					\
}
#define RGB_FROM_RGB888(Pixel, r, g, b)					\
{									\
	r = ((Pixel&0xFF0000)>>16);		 			\
	g = ((Pixel&0xFF00)>>8);		 			\
	b = (Pixel&0xFF);			 			\
}
#define RETRIEVE_RGB_PIXEL(buf, bpp, Pixel)				   \
do {									   \
	switch (bpp) {							   \
		case 2:							   \
			Pixel = *((Uint16 *)(buf));			   \
		break;							   \
									   \
		case 3: {						   \
		        Uint8 *B = (Uint8 *)(buf);			   \
			if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		   \
			        Pixel = B[0] + (B[1] << 8) + (B[2] << 16); \
			} else {					   \
			        Pixel = (B[0] << 16) + (B[1] << 8) + B[2]; \
			}						   \
		}							   \
		break;							   \
									   \
		case 4:							   \
			Pixel = *((Uint32 *)(buf));			   \
		break;							   \
									   \
		default:						   \
		        Pixel; /* stop gcc complaints */		   \
		break;							   \
	}								   \
} while (0)

#define DISEMBLE_RGB(buf, bpp, fmt, Pixel, r, g, b)			   \
do {									   \
	switch (bpp) {							   \
		case 2:							   \
			Pixel = *((Uint16 *)(buf));			   \
			RGB_FROM_PIXEL(Pixel, fmt, r, g, b);		   \
		break;							   \
									   \
		case 3:	{						   \
                        if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		   \
			        r = *((buf)+fmt->Rshift/8);		   \
				g = *((buf)+fmt->Gshift/8);		   \
				b = *((buf)+fmt->Bshift/8);		   \
			} else {					   \
			        r = *((buf)+2-fmt->Rshift/8);		   \
				g = *((buf)+2-fmt->Gshift/8);		   \
				b = *((buf)+2-fmt->Bshift/8);		   \
			}						   \
		}							   \
		break;							   \
									   \
		case 4:							   \
			Pixel = *((Uint32 *)(buf));			   \
			RGB_FROM_PIXEL(Pixel, fmt, r, g, b);		   \
		break;							   \
									   \
		default:						   \
		        Pixel; /* stop gcc complaints */		   \
		break;							   \
	}								   \
} while (0)

/* Assemble R-G-B values into a specified pixel format and store them */
#define PIXEL_FROM_RGB(Pixel, fmt, r, g, b)				\
{									\
	Pixel = ((r>>fmt->Rloss)<<fmt->Rshift)|				\
		((g>>fmt->Gloss)<<fmt->Gshift)|				\
		((b>>fmt->Bloss)<<fmt->Bshift);				\
}
#define RGB565_FROM_RGB(Pixel, r, g, b)					\
{									\
	Pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);			\
}
#define RGB555_FROM_RGB(Pixel, r, g, b)					\
{									\
	Pixel = ((r>>3)<<10)|((g>>3)<<5)|(b>>3);			\
}
#define RGB888_FROM_RGB(Pixel, r, g, b)					\
{									\
	Pixel = (r<<16)|(g<<8)|b;					\
}
#define ARGB8888_FROM_RGBA(Pixel, r, g, b, a)				\
{									\
	Pixel = (a<<24)|(r<<16)|(g<<8)|b;				\
}
#define RGBA8888_FROM_RGBA(Pixel, r, g, b, a)				\
{									\
	Pixel = (r<<24)|(g<<16)|(b<<8)|a;				\
}
#define ABGR8888_FROM_RGBA(Pixel, r, g, b, a)				\
{									\
	Pixel = (a<<24)|(b<<16)|(g<<8)|r;				\
}
#define BGRA8888_FROM_RGBA(Pixel, r, g, b, a)				\
{									\
	Pixel = (b<<24)|(g<<16)|(r<<8)|a;				\
}
#define ASSEMBLE_RGB(buf, bpp, fmt, r, g, b) 				\
{									\
	switch (bpp) {							\
		case 2: {						\
			Uint16 Pixel;					\
									\
			PIXEL_FROM_RGB(Pixel, fmt, r, g, b);		\
			*((Uint16 *)(buf)) = Pixel;			\
		}							\
		break;							\
									\
		case 3: {						\
                        if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		\
			        *((buf)+fmt->Rshift/8) = r;		\
				*((buf)+fmt->Gshift/8) = g;		\
				*((buf)+fmt->Bshift/8) = b;		\
			} else {					\
			        *((buf)+2-fmt->Rshift/8) = r;		\
				*((buf)+2-fmt->Gshift/8) = g;		\
				*((buf)+2-fmt->Bshift/8) = b;		\
			}						\
		}							\
		break;							\
									\
		case 4: {						\
			Uint32 Pixel;					\
									\
			PIXEL_FROM_RGB(Pixel, fmt, r, g, b);		\
			*((Uint32 *)(buf)) = Pixel;			\
		}							\
		break;							\
	}								\
}
#define ASSEMBLE_RGB_AMASK(buf, bpp, fmt, r, g, b, Amask)		\
{									\
	switch (bpp) {							\
		case 2: {						\
			Uint16 *bufp;					\
			Uint16 Pixel;					\
									\
			bufp = (Uint16 *)buf;				\
			PIXEL_FROM_RGB(Pixel, fmt, r, g, b);		\
			*bufp = Pixel | (*bufp & Amask);		\
		}							\
		break;							\
									\
		case 3: {						\
                        if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		\
			        *((buf)+fmt->Rshift/8) = r;		\
				*((buf)+fmt->Gshift/8) = g;		\
				*((buf)+fmt->Bshift/8) = b;		\
			} else {					\
			        *((buf)+2-fmt->Rshift/8) = r;		\
				*((buf)+2-fmt->Gshift/8) = g;		\
				*((buf)+2-fmt->Bshift/8) = b;		\
			}						\
		}							\
		break;							\
									\
		case 4: {						\
			Uint32 *bufp;					\
			Uint32 Pixel;					\
									\
			bufp = (Uint32 *)buf;				\
			PIXEL_FROM_RGB(Pixel, fmt, r, g, b);		\
			*bufp = Pixel | (*bufp & Amask);		\
		}							\
		break;							\
	}								\
}

/* FIXME: Should we rescale alpha into 0..255 here? */
#define RGBA_FROM_PIXEL(Pixel, fmt, r, g, b, a)				\
{									\
	r = ((Pixel&fmt->Rmask)>>fmt->Rshift)<<fmt->Rloss; 		\
	g = ((Pixel&fmt->Gmask)>>fmt->Gshift)<<fmt->Gloss; 		\
	b = ((Pixel&fmt->Bmask)>>fmt->Bshift)<<fmt->Bloss; 		\
	a = ((Pixel&fmt->Amask)>>fmt->Ashift)<<fmt->Aloss;	 	\
}
#define RGBA_FROM_8888(Pixel, fmt, r, g, b, a)	\
{						\
	r = (Pixel&fmt->Rmask)>>fmt->Rshift;	\
	g = (Pixel&fmt->Gmask)>>fmt->Gshift;	\
	b = (Pixel&fmt->Bmask)>>fmt->Bshift;	\
	a = (Pixel&fmt->Amask)>>fmt->Ashift;	\
}
#define RGBA_FROM_RGBA8888(Pixel, r, g, b, a)				\
{									\
	r = (Pixel>>24);						\
	g = ((Pixel>>16)&0xFF);						\
	b = ((Pixel>>8)&0xFF);						\
	a = (Pixel&0xFF);						\
}
#define RGBA_FROM_ARGB8888(Pixel, r, g, b, a)				\
{									\
	r = ((Pixel>>16)&0xFF);						\
	g = ((Pixel>>8)&0xFF);						\
	b = (Pixel&0xFF);						\
	a = (Pixel>>24);						\
}
#define RGBA_FROM_ABGR8888(Pixel, r, g, b, a)				\
{									\
	r = (Pixel&0xFF);						\
	g = ((Pixel>>8)&0xFF);						\
	b = ((Pixel>>16)&0xFF);						\
	a = (Pixel>>24);						\
}
#define RGBA_FROM_BGRA8888(Pixel, r, g, b, a)				\
{									\
	r = ((Pixel>>8)&0xFF);						\
	g = ((Pixel>>16)&0xFF);						\
	b = (Pixel>>24);						\
	a = (Pixel&0xFF);						\
}
#define DISEMBLE_RGBA(buf, bpp, fmt, Pixel, r, g, b, a)			   \
do {									   \
	switch (bpp) {							   \
		case 2:							   \
			Pixel = *((Uint16 *)(buf));			   \
			RGBA_FROM_PIXEL(Pixel, fmt, r, g, b, a);	   \
		break;							   \
									   \
		case 3:	{						   \
                        if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		   \
			        r = *((buf)+fmt->Rshift/8);		   \
				g = *((buf)+fmt->Gshift/8);		   \
				b = *((buf)+fmt->Bshift/8);		   \
			} else {					   \
			        r = *((buf)+2-fmt->Rshift/8);		   \
				g = *((buf)+2-fmt->Gshift/8);		   \
				b = *((buf)+2-fmt->Bshift/8);		   \
			}						   \
			a = 0xFF;					   \
		}							   \
		break;							   \
									   \
		case 4:							   \
			Pixel = *((Uint32 *)(buf));			   \
			RGBA_FROM_PIXEL(Pixel, fmt, r, g, b, a);	   \
		break;							   \
									   \
		default:						   \
		        Pixel; /* stop gcc complaints */		   \
		break;							   \
	}								   \
} while (0)

/* FIXME: this isn't correct, especially for Alpha (maximum != 255) */
#define PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a)				\
{									\
	Pixel = ((r>>fmt->Rloss)<<fmt->Rshift)|				\
		((g>>fmt->Gloss)<<fmt->Gshift)|				\
		((b>>fmt->Bloss)<<fmt->Bshift)|				\
		((a>>fmt->Aloss)<<fmt->Ashift);				\
}
#define ASSEMBLE_RGBA(buf, bpp, fmt, r, g, b, a)			\
{									\
	switch (bpp) {							\
		case 2: {						\
			Uint16 Pixel;					\
									\
			PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a);	\
			*((Uint16 *)(buf)) = Pixel;			\
		}							\
		break;							\
									\
		case 3: {						\
                        if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {		\
			        *((buf)+fmt->Rshift/8) = r;		\
				*((buf)+fmt->Gshift/8) = g;		\
				*((buf)+fmt->Bshift/8) = b;		\
			} else {					\
			        *((buf)+2-fmt->Rshift/8) = r;		\
				*((buf)+2-fmt->Gshift/8) = g;		\
				*((buf)+2-fmt->Bshift/8) = b;		\
			}						\
		}							\
		break;							\
									\
		case 4: {						\
			Uint32 Pixel;					\
									\
			PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a);	\
			*((Uint32 *)(buf)) = Pixel;			\
		}							\
		break;							\
	}								\
}

/* Blend the RGB values of two Pixels based on a source alpha value */
#define ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB)	\
do {						\
	dR = ((((int)(sR-dR)*(int)A)/255)+dR);	\
	dG = ((((int)(sG-dG)*(int)A)/255)+dG);	\
	dB = ((((int)(sB-dB)*(int)A)/255)+dB);	\
} while(0)


/* This is a very useful loop for optimizing blitters */
#if defined(_MSC_VER) && (_MSC_VER == 1300)
/* There's a bug in the Visual C++ 7 optimizer when compiling this code */
#else
#define USE_DUFFS_LOOP
#endif
#ifdef USE_DUFFS_LOOP

/* 8-times unrolled loop */
#define DUFFS_LOOP8(pixel_copy_increment, width)			\
{ int n = (width+7)/8;							\
	switch (width & 7) {						\
	case 0: do {	pixel_copy_increment;				\
	case 7:		pixel_copy_increment;				\
	case 6:		pixel_copy_increment;				\
	case 5:		pixel_copy_increment;				\
	case 4:		pixel_copy_increment;				\
	case 3:		pixel_copy_increment;				\
	case 2:		pixel_copy_increment;				\
	case 1:		pixel_copy_increment;				\
		} while ( --n > 0 );					\
	}								\
}

/* 4-times unrolled loop */
#define DUFFS_LOOP4(pixel_copy_increment, width)			\
{ int n = (width+3)/4;							\
	switch (width & 3) {						\
	case 0: do {	pixel_copy_increment;				\
	case 3:		pixel_copy_increment;				\
	case 2:		pixel_copy_increment;				\
	case 1:		pixel_copy_increment;				\
		} while (--n > 0);					\
	}								\
}

/* Use the 8-times version of the loop by default */
#define DUFFS_LOOP(pixel_copy_increment, width)				\
	DUFFS_LOOP8(pixel_copy_increment, width)

/* Special version of Duff's device for even more optimization */
#define DUFFS_LOOP_124(pixel_copy_increment1,				\
                       pixel_copy_increment2,				\
                       pixel_copy_increment4, width)			\
{ int n = width;							\
	if (n & 1) {							\
		pixel_copy_increment1; n -= 1;				\
	}								\
	if (n & 2) {							\
		pixel_copy_increment2; n -= 2;				\
	}								\
	if (n) {							\
		n = (n+7)/ 8;						\
		switch (n & 4) {					\
		case 0: do {	pixel_copy_increment4;			\
		case 4:		pixel_copy_increment4;			\
			} while (--n > 0);				\
		}							\
	}								\
}

#else

/* Don't use Duff's device to unroll loops */
#define DUFFS_LOOP(pixel_copy_increment, width)				\
{ int n;								\
	for ( n=width; n > 0; --n ) {					\
		pixel_copy_increment;					\
	}								\
}
#define DUFFS_LOOP8(pixel_copy_increment, width)			\
	DUFFS_LOOP(pixel_copy_increment, width)
#define DUFFS_LOOP4(pixel_copy_increment, width)			\
	DUFFS_LOOP(pixel_copy_increment, width)
#define DUFFS_LOOP_124(pixel_copy_increment1,				\
                       pixel_copy_increment2,				\
                       pixel_copy_increment4, width)			\
	DUFFS_LOOP(pixel_copy_increment1, width)

#endif /* USE_DUFFS_LOOP */

/* Prevent Visual C++ 6.0 from printing out stupid warnings */
#if defined(_MSC_VER) && (_MSC_VER >= 600)
#pragma warning(disable: 4550)
#endif

#endif /* _SDL_blit_h */

/* vi: set ts=4 sw=4 expandtab: */
