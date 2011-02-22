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

/* This a stretch blit implementation based on ideas given to me by
   Tomasz Cejner - thanks! :)

   April 27, 2000 - Sam Lantinga
*/

#include "SDL_video.h"
#include "SDL_blit.h"

/* This isn't ready for general consumption yet - it should be folded
   into the general blitting mechanism.
*/

#if ((defined(_MFC_VER) && defined(_M_IX86)/* && !defined(_WIN32_WCE) still needed? */) || \
     defined(__WATCOMC__) || \
     (defined(__GNUC__) && defined(__i386__))) && SDL_ASSEMBLY_ROUTINES
/* There's a bug with gcc 4.4.1 and -O2 where srcp doesn't get the correct
 * value after the first scanline.  FIXME? */
/*#define USE_ASM_STRETCH*/
#endif

#ifdef USE_ASM_STRETCH

#ifdef HAVE_MPROTECT
#include <sys/types.h>
#include <sys/mman.h>
#endif
#ifdef __GNUC__
#define PAGE_ALIGNED __attribute__((__aligned__(4096)))
#else
#define PAGE_ALIGNED
#endif

#if defined(_M_IX86) || defined(i386)
#define PREFIX16	0x66
#define STORE_BYTE	0xAA
#define STORE_WORD	0xAB
#define LOAD_BYTE	0xAC
#define LOAD_WORD	0xAD
#define RETURN		0xC3
#else
#error Need assembly opcodes for this architecture
#endif

static unsigned char copy_row[4096] PAGE_ALIGNED;

static int
generate_rowbytes(int src_w, int dst_w, int bpp)
{
    static struct
    {
        int bpp;
        int src_w;
        int dst_w;
        int status;
    } last;

    int i;
    int pos, inc;
    unsigned char *eip, *fence;
    unsigned char load, store;

    /* See if we need to regenerate the copy buffer */
    if ((src_w == last.src_w) && (dst_w == last.dst_w) && (bpp == last.bpp)) {
        return (last.status);
    }
    last.bpp = bpp;
    last.src_w = src_w;
    last.dst_w = dst_w;
    last.status = -1;

    switch (bpp) {
    case 1:
        load = LOAD_BYTE;
        store = STORE_BYTE;
        break;
    case 2:
    case 4:
        load = LOAD_WORD;
        store = STORE_WORD;
        break;
    default:
        SDL_SetError("ASM stretch of %d bytes isn't supported\n", bpp);
        return (-1);
    }
#ifdef HAVE_MPROTECT
    /* Make the code writeable */
    if (mprotect(copy_row, sizeof(copy_row), PROT_READ | PROT_WRITE) < 0) {
        SDL_SetError("Couldn't make copy buffer writeable");
        return (-1);
    }
#endif
    pos = 0x10000;
    inc = (src_w << 16) / dst_w;
    eip = copy_row;
    fence = copy_row + sizeof(copy_row)-2;
    for (i = 0; i < dst_w; ++i) {
        while (pos >= 0x10000L) {
            if (eip == fence) {
                return -1;
            }
            if (bpp == 2) {
                *eip++ = PREFIX16;
            }
            *eip++ = load;
            pos -= 0x10000L;
        }
        if (eip == fence) {
            return -1;
        }
        if (bpp == 2) {
            *eip++ = PREFIX16;
        }
        *eip++ = store;
        pos += inc;
    }
    *eip++ = RETURN;

#ifdef HAVE_MPROTECT
    /* Make the code executable but not writeable */
    if (mprotect(copy_row, sizeof(copy_row), PROT_READ | PROT_EXEC) < 0) {
        SDL_SetError("Couldn't make copy buffer executable");
        return (-1);
    }
#endif
    last.status = 0;
    return (0);
}

#endif /* USE_ASM_STRETCH */

#define DEFINE_COPY_ROW(name, type)			\
static void name(type *src, int src_w, type *dst, int dst_w)	\
{							\
	int i;						\
	int pos, inc;					\
	type pixel = 0;					\
							\
	pos = 0x10000;					\
	inc = (src_w << 16) / dst_w;			\
	for ( i=dst_w; i>0; --i ) {			\
		while ( pos >= 0x10000L ) {		\
			pixel = *src++;			\
			pos -= 0x10000L;		\
		}					\
		*dst++ = pixel;				\
		pos += inc;				\
	}						\
}
/* *INDENT-OFF* */
DEFINE_COPY_ROW(copy_row1, Uint8)
DEFINE_COPY_ROW(copy_row2, Uint16)
DEFINE_COPY_ROW(copy_row4, Uint32)
/* *INDENT-ON* */

/* The ASM code doesn't handle 24-bpp stretch blits */
static void
copy_row3(Uint8 * src, int src_w, Uint8 * dst, int dst_w)
{
    int i;
    int pos, inc;
    Uint8 pixel[3] = { 0, 0, 0 };

    pos = 0x10000;
    inc = (src_w << 16) / dst_w;
    for (i = dst_w; i > 0; --i) {
        while (pos >= 0x10000L) {
            pixel[0] = *src++;
            pixel[1] = *src++;
            pixel[2] = *src++;
            pos -= 0x10000L;
        }
        *dst++ = pixel[0];
        *dst++ = pixel[1];
        *dst++ = pixel[2];
        pos += inc;
    }
}

/* Perform a stretch blit between two surfaces of the same format.
   NOTE:  This function is not safe to call from multiple threads!
*/
int
SDL_SoftStretch(SDL_Surface * src, const SDL_Rect * srcrect,
                SDL_Surface * dst, const SDL_Rect * dstrect)
{
    int src_locked;
    int dst_locked;
    int pos, inc;
    int dst_width;
    int dst_maxrow;
    int src_row, dst_row;
    Uint8 *srcp = NULL;
    Uint8 *dstp;
    SDL_Rect full_src;
    SDL_Rect full_dst;
#ifdef USE_ASM_STRETCH
    SDL_bool use_asm = SDL_TRUE;
#ifdef __GNUC__
    int u1, u2;
#endif
#endif /* USE_ASM_STRETCH */
    const int bpp = dst->format->BytesPerPixel;

    if (src->format->BitsPerPixel != dst->format->BitsPerPixel) {
        SDL_SetError("Only works with same format surfaces");
        return (-1);
    }

    /* Verify the blit rectangles */
    if (srcrect) {
        if ((srcrect->x < 0) || (srcrect->y < 0) ||
            ((srcrect->x + srcrect->w) > src->w) ||
            ((srcrect->y + srcrect->h) > src->h)) {
            SDL_SetError("Invalid source blit rectangle");
            return (-1);
        }
    } else {
        full_src.x = 0;
        full_src.y = 0;
        full_src.w = src->w;
        full_src.h = src->h;
        srcrect = &full_src;
    }
    if (dstrect) {
        if ((dstrect->x < 0) || (dstrect->y < 0) ||
            ((dstrect->x + dstrect->w) > dst->w) ||
            ((dstrect->y + dstrect->h) > dst->h)) {
            SDL_SetError("Invalid destination blit rectangle");
            return (-1);
        }
    } else {
        full_dst.x = 0;
        full_dst.y = 0;
        full_dst.w = dst->w;
        full_dst.h = dst->h;
        dstrect = &full_dst;
    }

    /* Lock the destination if it's in hardware */
    dst_locked = 0;
    if (SDL_MUSTLOCK(dst)) {
        if (SDL_LockSurface(dst) < 0) {
            SDL_SetError("Unable to lock destination surface");
            return (-1);
        }
        dst_locked = 1;
    }
    /* Lock the source if it's in hardware */
    src_locked = 0;
    if (SDL_MUSTLOCK(src)) {
        if (SDL_LockSurface(src) < 0) {
            if (dst_locked) {
                SDL_UnlockSurface(dst);
            }
            SDL_SetError("Unable to lock source surface");
            return (-1);
        }
        src_locked = 1;
    }

    /* Set up the data... */
    pos = 0x10000;
    inc = (srcrect->h << 16) / dstrect->h;
    src_row = srcrect->y;
    dst_row = dstrect->y;
    dst_width = dstrect->w * bpp;

#ifdef USE_ASM_STRETCH
    /* Write the opcodes for this stretch */
    if ((bpp == 3) || (generate_rowbytes(srcrect->w, dstrect->w, bpp) < 0)) {
        use_asm = SDL_FALSE;
    }
#endif

    /* Perform the stretch blit */
    for (dst_maxrow = dst_row + dstrect->h; dst_row < dst_maxrow; ++dst_row) {
        dstp = (Uint8 *) dst->pixels + (dst_row * dst->pitch)
            + (dstrect->x * bpp);
        while (pos >= 0x10000L) {
            srcp = (Uint8 *) src->pixels + (src_row * src->pitch)
                + (srcrect->x * bpp);
            ++src_row;
            pos -= 0x10000L;
        }
#ifdef USE_ASM_STRETCH
        if (use_asm) {
#ifdef __GNUC__
            __asm__ __volatile__("call *%4":"=&D"(u1), "=&S"(u2)
                                 :"0"(dstp), "1"(srcp), "r"(copy_row)
                                 :"memory");
#elif defined(_MSC_VER) || defined(__WATCOMC__)
            /* *INDENT-OFF* */
            {
                void *code = copy_row;
                __asm {
                    push edi
                    push esi
                    mov edi, dstp
                    mov esi, srcp
                    call dword ptr code
                    pop esi
                    pop edi
                }
            }
            /* *INDENT-ON* */
#else
#error Need inline assembly for this compiler
#endif
        } else
#endif
            switch (bpp) {
            case 1:
                copy_row1(srcp, srcrect->w, dstp, dstrect->w);
                break;
            case 2:
                copy_row2((Uint16 *) srcp, srcrect->w,
                          (Uint16 *) dstp, dstrect->w);
                break;
            case 3:
                copy_row3(srcp, srcrect->w, dstp, dstrect->w);
                break;
            case 4:
                copy_row4((Uint32 *) srcp, srcrect->w,
                          (Uint32 *) dstp, dstrect->w);
                break;
            }
        pos += inc;
    }

    /* We need to unlock the surfaces if they're locked */
    if (dst_locked) {
        SDL_UnlockSurface(dst);
    }
    if (src_locked) {
        SDL_UnlockSurface(src);
    }
    return (0);
}

/* vi: set ts=4 sw=4 expandtab: */
