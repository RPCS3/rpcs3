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

/*
	m68k assembly mix routines

	Patrice Mandin
*/

#if defined(__M68000__) && !defined(__mcoldfire__) && defined(__GNUC__)
void
SDL_MixAudio_m68k_U8(char *dst, char *src, long len, long volume, char *mix8)
{
    __asm__ __volatile__("tstl	%2\n" "	beqs	stoploop_u8\n" "mixloop_u8:\n"
                         /* Mix a sample */
                         "	moveq	#0,%%d0\n" "	moveq	#0,%%d1\n" "	moveb	%1@+,%%d0\n"    /* d0 = *src++ */
                         "	sub	#128,%%d0\n"    /* d0 -= 128 */
                         "	muls	%3,%%d0\n"      /* d0 *= volume (0<=volume<=128) */
                         "	moveb	%0@,%%d1\n"     /* d1 = *dst */
                         "	asr	#7,%%d0\n"      /* d0 /= 128 (SDL_MIX_MAXVOLUME) */
                         "	add	#128,%%d0\n"    /* d0 += 128 */
                         "	add	%%d1,%%d0\n"
                         "	moveb	%4@(%%d0:w),%0@+\n"
                         /* Loop till done */
                         "	subql	#1,%2\n" "	bhis	mixloop_u8\n" "stoploop_u8:\n": /* no return value */
                         :      /* input */
                         "a"(dst), "a"(src), "d"(len), "d"(volume), "a"(mix8):  /* clobbered registers */
                         "d0", "d1", "cc", "memory");
}

void
SDL_MixAudio_m68k_S8(char *dst, char *src, long len, long volume)
{
    __asm__ __volatile__("tstl	%2\n"
                         "	beqs	stoploop_s8\n"
                         "	moveq	#-128,%%d2\n"
                         "	moveq	#127,%%d3\n" "mixloop_s8:\n"
                         /* Mix a sample */
                         "	moveq	#0,%%d0\n" "	moveq	#0,%%d1\n" "	moveb	%1@+,%%d0\n"    /* d0 = *src++ */
                         "	muls	%3,%%d0\n"      /* d0 *= volume (0<=volume<=128) */
                         "	moveb	%0@,%%d1\n"     /* d1 = *dst */
                         "	asr	#7,%%d0\n"      /* d0 /= 128 (SDL_MIX_MAXVOLUME) */
                         "	add	%%d1,%%d0\n"
                         "	cmp	%%d2,%%d0\n"
                         "	bges	lower_limit_s8\n"
                         "	move	%%d2,%%d0\n"
                         "lower_limit_s8:\n"
                         "	cmp	%%d3,%%d0\n"
                         "	bles	upper_limit_s8\n"
                         "	move	%%d3,%%d0\n"
                         "upper_limit_s8:\n" "	moveb	%%d0,%0@+\n"
                         /* Loop till done */
                         "	subql	#1,%2\n" "	bhis	mixloop_s8\n" "stoploop_s8:\n": /* no return value */
                         :      /* input */
                         "a"(dst), "a"(src), "d"(len), "d"(volume):     /* clobbered registers */
                         "d0", "d1", "d2", "d3", "cc", "memory");
}

void
SDL_MixAudio_m68k_S16MSB(short *dst, short *src, long len, long volume)
{
    __asm__ __volatile__("tstl	%2\n"
                         "	beqs	stoploop_s16msb\n"
                         "	movel	#-32768,%%d2\n"
                         "	movel	#32767,%%d3\n"
                         "	lsrl	#1,%2\n" "mixloop_s16msb:\n"
                         /* Mix a sample */
                         "	move	%1@+,%%d0\n"    /* d0 = *src++ */
                         "	muls	%3,%%d0\n"      /* d0 *= volume (0<=volume<=128) */
                         "	move	%0@,%%d1\n"     /* d1 = *dst */
                         "	extl	%%d1\n" /* extend d1 to 32 bits */
                         "	asrl	#7,%%d0\n"      /* d0 /= 128 (SDL_MIX_MAXVOLUME) */
                         "	addl	%%d1,%%d0\n"
                         "	cmpl	%%d2,%%d0\n"
                         "	bges	lower_limit_s16msb\n"
                         "	move	%%d2,%%d0\n"
                         "lower_limit_s16msb:\n"
                         "	cmpl	%%d3,%%d0\n"
                         "	bles	upper_limit_s16msb\n"
                         "	move	%%d3,%%d0\n"
                         "upper_limit_s16msb:\n" "	move	%%d0,%0@+\n"
                         /* Loop till done */
                         "	subql	#1,%2\n" "	bhis	mixloop_s16msb\n" "stoploop_s16msb:\n": /* no return value */
                         :      /* input */
                         "a"(dst), "a"(src), "d"(len), "d"(volume):     /* clobbered registers */
                         "d0", "d1", "d2", "d3", "cc", "memory");
}

void
SDL_MixAudio_m68k_S16LSB(short *dst, short *src, long len, long volume)
{
    __asm__ __volatile__("tstl	%2\n"
                         "	beqs	stoploop_s16lsb\n"
                         "	movel	#-32768,%%d2\n"
                         "	movel	#32767,%%d3\n"
                         "	lsrl	#1,%2\n" "mixloop_s16lsb:\n"
                         /* Mix a sample */
                         "	move	%1@+,%%d0\n"    /* d0 = *src++ */
                         "	rorw	#8,%%d0\n" "	muls	%3,%%d0\n"      /* d0 *= volume (0<=volume<=128) */
                         "	move	%0@,%%d1\n"     /* d1 = *dst */
                         "	rorw	#8,%%d1\n" "	extl	%%d1\n" /* extend d1 to 32 bits */
                         "	asrl	#7,%%d0\n"      /* d0 /= 128 (SDL_MIX_MAXVOLUME) */
                         "	addl	%%d1,%%d0\n"
                         "	cmpl	%%d2,%%d0\n"
                         "	bges	lower_limit_s16lsb\n"
                         "	move	%%d2,%%d0\n"
                         "lower_limit_s16lsb:\n"
                         "	cmpl	%%d3,%%d0\n"
                         "	bles	upper_limit_s16lsb\n"
                         "	move	%%d3,%%d0\n"
                         "upper_limit_s16lsb:\n"
                         "	rorw	#8,%%d0\n" "	move	%%d0,%0@+\n"
                         /* Loop till done */
                         "	subql	#1,%2\n" "	bhis	mixloop_s16lsb\n" "stoploop_s16lsb:\n": /* no return value */
                         :      /* input */
                         "a"(dst), "a"(src), "d"(len), "d"(volume):     /* clobbered registers */
                         "d0", "d1", "d2", "d3", "cc", "memory");
}
#endif
/* vi: set ts=4 sw=4 expandtab: */
