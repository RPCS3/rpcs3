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

/* This is the software implementation of the YUV texture support */

/* This code was derived from code carrying the following copyright notices:

 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

 * Copyright (c) 1995 Erik Corry
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL ERIK CORRY BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ERIK CORRY HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ERIK CORRY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND ERIK CORRY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "SDL_video.h"
#include "SDL_cpuinfo.h"
#include "SDL_yuv_sw_c.h"


/* The colorspace conversion functions */

#if (__GNUC__ > 2) && defined(__i386__) && __OPTIMIZE__ && SDL_ASSEMBLY_ROUTINES
extern void Color565DitherYV12MMX1X(int *colortab, Uint32 * rgb_2_pix,
                                    unsigned char *lum, unsigned char *cr,
                                    unsigned char *cb, unsigned char *out,
                                    int rows, int cols, int mod);
extern void ColorRGBDitherYV12MMX1X(int *colortab, Uint32 * rgb_2_pix,
                                    unsigned char *lum, unsigned char *cr,
                                    unsigned char *cb, unsigned char *out,
                                    int rows, int cols, int mod);
#endif

static void
Color16DitherYV12Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned short *row1;
    unsigned short *row2;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned short *) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            *row1++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);

            L = *lum++;
            *row1++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);


            /* Now, do second row.  */

            L = *lum2++;
            *row2++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);

            L = *lum2++;
            *row2++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void
Color24DitherYV12Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int value;
    unsigned char *row1;
    unsigned char *row2;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = out;
    row2 = row1 + cols * 3 + mod * 3;
    lum2 = lum + cols;

    mod += cols + mod;
    mod *= 3;

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row1++ = (value) & 0xFF;
            *row1++ = (value >> 8) & 0xFF;
            *row1++ = (value >> 16) & 0xFF;

            L = *lum++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row1++ = (value) & 0xFF;
            *row1++ = (value >> 8) & 0xFF;
            *row1++ = (value >> 16) & 0xFF;


            /* Now, do second row.  */

            L = *lum2++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row2++ = (value) & 0xFF;
            *row2++ = (value >> 8) & 0xFF;
            *row2++ = (value >> 16) & 0xFF;

            L = *lum2++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row2++ = (value) & 0xFF;
            *row2++ = (value >> 8) & 0xFF;
            *row2++ = (value >> 16) & 0xFF;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void
Color32DitherYV12Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row1;
    unsigned int *row2;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned int *) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            *row1++ = (rgb_2_pix[L + cr_r] |
                       rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);

            L = *lum++;
            *row1++ = (rgb_2_pix[L + cr_r] |
                       rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);


            /* Now, do second row.  */

            L = *lum2++;
            *row2++ = (rgb_2_pix[L + cr_r] |
                       rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);

            L = *lum2++;
            *row2++ = (rgb_2_pix[L + cr_r] |
                       rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

/*
 * In this function I make use of a nasty trick. The tables have the lower
 * 16 bits replicated in the upper 16. This means I can write ints and get
 * the horisontal doubling for free (almost).
 */
static void
Color16DitherYV12Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row1 = (unsigned int *) out;
    const int next_row = cols + (mod / 2);
    unsigned int *row2 = row1 + 2 * next_row;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    lum2 = lum + cols;

    mod = (next_row * 3) + (mod / 2);

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            row1[0] = row1[next_row] = (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);
            row1++;

            L = *lum++;
            row1[0] = row1[next_row] = (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);
            row1++;


            /* Now, do second row. */

            L = *lum2++;
            row2[0] = row2[next_row] = (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);
            row2++;

            L = *lum2++;
            row2[0] = row2[next_row] = (rgb_2_pix[L + cr_r] |
                                        rgb_2_pix[L + crb_g] |
                                        rgb_2_pix[L + cb_b]);
            row2++;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void
Color24DitherYV12Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int value;
    unsigned char *row1 = out;
    const int next_row = (cols * 2 + mod) * 3;
    unsigned char *row2 = row1 + 2 * next_row;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    lum2 = lum + cols;

    mod = next_row * 3 + mod * 3;

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row1[0 + 0] = row1[3 + 0] = row1[next_row + 0] =
                row1[next_row + 3 + 0] = (value) & 0xFF;
            row1[0 + 1] = row1[3 + 1] = row1[next_row + 1] =
                row1[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row1[0 + 2] = row1[3 + 2] = row1[next_row + 2] =
                row1[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row1 += 2 * 3;

            L = *lum++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row1[0 + 0] = row1[3 + 0] = row1[next_row + 0] =
                row1[next_row + 3 + 0] = (value) & 0xFF;
            row1[0 + 1] = row1[3 + 1] = row1[next_row + 1] =
                row1[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row1[0 + 2] = row1[3 + 2] = row1[next_row + 2] =
                row1[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row1 += 2 * 3;


            /* Now, do second row. */

            L = *lum2++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row2[0 + 0] = row2[3 + 0] = row2[next_row + 0] =
                row2[next_row + 3 + 0] = (value) & 0xFF;
            row2[0 + 1] = row2[3 + 1] = row2[next_row + 1] =
                row2[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row2[0 + 2] = row2[3 + 2] = row2[next_row + 2] =
                row2[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row2 += 2 * 3;

            L = *lum2++;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row2[0 + 0] = row2[3 + 0] = row2[next_row + 0] =
                row2[next_row + 3 + 0] = (value) & 0xFF;
            row2[0 + 1] = row2[3 + 1] = row2[next_row + 1] =
                row2[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row2[0 + 2] = row2[3 + 2] = row2[next_row + 2] =
                row2[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row2 += 2 * 3;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void
Color32DitherYV12Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row1 = (unsigned int *) out;
    const int next_row = cols * 2 + mod;
    unsigned int *row2 = row1 + 2 * next_row;
    unsigned char *lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    lum2 = lum + cols;

    mod = (next_row * 3) + mod;

    y = rows / 2;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            ++cr;
            ++cb;

            L = *lum++;
            row1[0] = row1[1] = row1[next_row] = row1[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row1 += 2;

            L = *lum++;
            row1[0] = row1[1] = row1[next_row] = row1[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row1 += 2;


            /* Now, do second row. */

            L = *lum2++;
            row2[0] = row2[1] = row2[next_row] = row2[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row2 += 2;

            L = *lum2++;
            row2[0] = row2[1] = row2[next_row] = row2[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row2 += 2;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

static void
Color16DitherYUY2Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned short *row;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row = (unsigned short *) out;

    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            *row++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                       rgb_2_pix[L + crb_g] |
                                       rgb_2_pix[L + cb_b]);

            L = *lum;
            lum += 2;
            *row++ = (unsigned short) (rgb_2_pix[L + cr_r] |
                                       rgb_2_pix[L + crb_g] |
                                       rgb_2_pix[L + cb_b]);

        }

        row += mod;
    }
}

static void
Color24DitherYUY2Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int value;
    unsigned char *row;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row = (unsigned char *) out;
    mod *= 3;
    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row++ = (value) & 0xFF;
            *row++ = (value >> 8) & 0xFF;
            *row++ = (value >> 16) & 0xFF;

            L = *lum;
            lum += 2;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            *row++ = (value) & 0xFF;
            *row++ = (value >> 8) & 0xFF;
            *row++ = (value >> 16) & 0xFF;

        }
        row += mod;
    }
}

static void
Color32DitherYUY2Mod1X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row = (unsigned int *) out;
    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            *row++ = (rgb_2_pix[L + cr_r] |
                      rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);

            L = *lum;
            lum += 2;
            *row++ = (rgb_2_pix[L + cr_r] |
                      rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);


        }
        row += mod;
    }
}

/*
 * In this function I make use of a nasty trick. The tables have the lower
 * 16 bits replicated in the upper 16. This means I can write ints and get
 * the horisontal doubling for free (almost).
 */
static void
Color16DitherYUY2Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row = (unsigned int *) out;
    const int next_row = cols + (mod / 2);
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            row[0] = row[next_row] = (rgb_2_pix[L + cr_r] |
                                      rgb_2_pix[L + crb_g] |
                                      rgb_2_pix[L + cb_b]);
            row++;

            L = *lum;
            lum += 2;
            row[0] = row[next_row] = (rgb_2_pix[L + cr_r] |
                                      rgb_2_pix[L + crb_g] |
                                      rgb_2_pix[L + cb_b]);
            row++;

        }
        row += next_row;
    }
}

static void
Color24DitherYUY2Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int value;
    unsigned char *row = out;
    const int next_row = (cols * 2 + mod) * 3;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;
    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row[0 + 0] = row[3 + 0] = row[next_row + 0] =
                row[next_row + 3 + 0] = (value) & 0xFF;
            row[0 + 1] = row[3 + 1] = row[next_row + 1] =
                row[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row[0 + 2] = row[3 + 2] = row[next_row + 2] =
                row[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row += 2 * 3;

            L = *lum;
            lum += 2;
            value = (rgb_2_pix[L + cr_r] |
                     rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row[0 + 0] = row[3 + 0] = row[next_row + 0] =
                row[next_row + 3 + 0] = (value) & 0xFF;
            row[0 + 1] = row[3 + 1] = row[next_row + 1] =
                row[next_row + 3 + 1] = (value >> 8) & 0xFF;
            row[0 + 2] = row[3 + 2] = row[next_row + 2] =
                row[next_row + 3 + 2] = (value >> 16) & 0xFF;
            row += 2 * 3;

        }
        row += next_row;
    }
}

static void
Color32DitherYUY2Mod2X(int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod)
{
    unsigned int *row = (unsigned int *) out;
    const int next_row = cols * 2 + mod;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;
    mod += mod;
    y = rows;
    while (y--) {
        x = cols_2;
        while (x--) {
            register int L;

            cr_r = 0 * 768 + 256 + colortab[*cr + 0 * 256];
            crb_g = 1 * 768 + 256 + colortab[*cr + 1 * 256]
                + colortab[*cb + 2 * 256];
            cb_b = 2 * 768 + 256 + colortab[*cb + 3 * 256];
            cr += 4;
            cb += 4;

            L = *lum;
            lum += 2;
            row[0] = row[1] = row[next_row] = row[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row += 2;

            L = *lum;
            lum += 2;
            row[0] = row[1] = row[next_row] = row[next_row + 1] =
                (rgb_2_pix[L + cr_r] |
                 rgb_2_pix[L + crb_g] | rgb_2_pix[L + cb_b]);
            row += 2;


        }

        row += next_row;
    }
}

/*
 * How many 1 bits are there in the Uint32.
 * Low performance, do not call often.
 */
static int
number_of_bits_set(Uint32 a)
{
    if (!a)
        return 0;
    if (a & 1)
        return 1 + number_of_bits_set(a >> 1);
    return (number_of_bits_set(a >> 1));
}

/*
 * How many 0 bits are there at least significant end of Uint32.
 * Low performance, do not call often.
 */
static int
free_bits_at_bottom(Uint32 a)
{
    /* assume char is 8 bits */
    if (!a)
        return sizeof(Uint32) * 8;
    if (((Sint32) a) & 1l)
        return 0;
    return 1 + free_bits_at_bottom(a >> 1);
}

static int
SDL_SW_SetupYUVDisplay(SDL_SW_YUVTexture * swdata, Uint32 target_format)
{
    Uint32 *r_2_pix_alloc;
    Uint32 *g_2_pix_alloc;
    Uint32 *b_2_pix_alloc;
    int i;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (target_format, &bpp, &Rmask, &Gmask, &Bmask, &Amask) || bpp < 15) {
        SDL_SetError("Unsupported YUV destination format");
        return -1;
    }

    swdata->target_format = target_format;
    r_2_pix_alloc = &swdata->rgb_2_pix[0 * 768];
    g_2_pix_alloc = &swdata->rgb_2_pix[1 * 768];
    b_2_pix_alloc = &swdata->rgb_2_pix[2 * 768];

    /* 
     * Set up entries 0-255 in rgb-to-pixel value tables.
     */
    for (i = 0; i < 256; ++i) {
        r_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Rmask));
        r_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Rmask);
        r_2_pix_alloc[i + 256] |= Amask;
        g_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Gmask));
        g_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Gmask);
        g_2_pix_alloc[i + 256] |= Amask;
        b_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Bmask));
        b_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Bmask);
        b_2_pix_alloc[i + 256] |= Amask;
    }

    /*
     * If we have 16-bit output depth, then we double the value
     * in the top word. This means that we can write out both
     * pixels in the pixel doubling mode with one op. It is 
     * harmless in the normal case as storing a 32-bit value
     * through a short pointer will lose the top bits anyway.
     */
    if (SDL_BYTESPERPIXEL(target_format) == 2) {
        for (i = 0; i < 256; ++i) {
            r_2_pix_alloc[i + 256] |= (r_2_pix_alloc[i + 256]) << 16;
            g_2_pix_alloc[i + 256] |= (g_2_pix_alloc[i + 256]) << 16;
            b_2_pix_alloc[i + 256] |= (b_2_pix_alloc[i + 256]) << 16;
        }
    }

    /*
     * Spread out the values we have to the rest of the array so that
     * we do not need to check for overflow.
     */
    for (i = 0; i < 256; ++i) {
        r_2_pix_alloc[i] = r_2_pix_alloc[256];
        r_2_pix_alloc[i + 512] = r_2_pix_alloc[511];
        g_2_pix_alloc[i] = g_2_pix_alloc[256];
        g_2_pix_alloc[i + 512] = g_2_pix_alloc[511];
        b_2_pix_alloc[i] = b_2_pix_alloc[256];
        b_2_pix_alloc[i + 512] = b_2_pix_alloc[511];
    }

    /* You have chosen wisely... */
    switch (swdata->format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
        if (SDL_BYTESPERPIXEL(target_format) == 2) {
#if (__GNUC__ > 2) && defined(__i386__) && __OPTIMIZE__ && SDL_ASSEMBLY_ROUTINES
            /* inline assembly functions */
            if (SDL_HasMMX() && (Rmask == 0xF800) &&
                (Gmask == 0x07E0) && (Bmask == 0x001F)
                && (swdata->w & 15) == 0) {
/*printf("Using MMX 16-bit 565 dither\n");*/
                swdata->Display1X = Color565DitherYV12MMX1X;
            } else {
/*printf("Using C 16-bit dither\n");*/
                swdata->Display1X = Color16DitherYV12Mod1X;
            }
#else
            swdata->Display1X = Color16DitherYV12Mod1X;
#endif
            swdata->Display2X = Color16DitherYV12Mod2X;
        }
        if (SDL_BYTESPERPIXEL(target_format) == 3) {
            swdata->Display1X = Color24DitherYV12Mod1X;
            swdata->Display2X = Color24DitherYV12Mod2X;
        }
        if (SDL_BYTESPERPIXEL(target_format) == 4) {
#if (__GNUC__ > 2) && defined(__i386__) && __OPTIMIZE__ && SDL_ASSEMBLY_ROUTINES
            /* inline assembly functions */
            if (SDL_HasMMX() && (Rmask == 0x00FF0000) &&
                (Gmask == 0x0000FF00) &&
                (Bmask == 0x000000FF) && (swdata->w & 15) == 0) {
/*printf("Using MMX 32-bit dither\n");*/
                swdata->Display1X = ColorRGBDitherYV12MMX1X;
            } else {
/*printf("Using C 32-bit dither\n");*/
                swdata->Display1X = Color32DitherYV12Mod1X;
            }
#else
            swdata->Display1X = Color32DitherYV12Mod1X;
#endif
            swdata->Display2X = Color32DitherYV12Mod2X;
        }
        break;
    case SDL_PIXELFORMAT_YUY2:
    case SDL_PIXELFORMAT_UYVY:
    case SDL_PIXELFORMAT_YVYU:
        if (SDL_BYTESPERPIXEL(target_format) == 2) {
            swdata->Display1X = Color16DitherYUY2Mod1X;
            swdata->Display2X = Color16DitherYUY2Mod2X;
        }
        if (SDL_BYTESPERPIXEL(target_format) == 3) {
            swdata->Display1X = Color24DitherYUY2Mod1X;
            swdata->Display2X = Color24DitherYUY2Mod2X;
        }
        if (SDL_BYTESPERPIXEL(target_format) == 4) {
            swdata->Display1X = Color32DitherYUY2Mod1X;
            swdata->Display2X = Color32DitherYUY2Mod2X;
        }
        break;
    default:
        /* We should never get here (caught above) */
        break;
    }

    if (swdata->display) {
        SDL_FreeSurface(swdata->display);
        swdata->display = NULL;
    }
    return 0;
}

SDL_SW_YUVTexture *
SDL_SW_CreateYUVTexture(Uint32 format, int w, int h)
{
    SDL_SW_YUVTexture *swdata;
    int *Cr_r_tab;
    int *Cr_g_tab;
    int *Cb_g_tab;
    int *Cb_b_tab;
    int i;
    int CR, CB;

    swdata = (SDL_SW_YUVTexture *) SDL_calloc(1, sizeof(*swdata));
    if (!swdata) {
        SDL_OutOfMemory();
        return NULL;
    }

    switch (format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
    case SDL_PIXELFORMAT_YUY2:
    case SDL_PIXELFORMAT_UYVY:
    case SDL_PIXELFORMAT_YVYU:
        break;
    default:
        SDL_SetError("Unsupported YUV format");
        return NULL;
    }

    swdata->format = format;
    swdata->target_format = SDL_PIXELFORMAT_UNKNOWN;
    swdata->w = w;
    swdata->h = h;
    swdata->pixels = (Uint8 *) SDL_malloc(w * h * 2);
    swdata->colortab = (int *) SDL_malloc(4 * 256 * sizeof(int));
    swdata->rgb_2_pix = (Uint32 *) SDL_malloc(3 * 768 * sizeof(Uint32));
    if (!swdata->pixels || !swdata->colortab || !swdata->rgb_2_pix) {
        SDL_OutOfMemory();
        SDL_SW_DestroyYUVTexture(swdata);
        return NULL;
    }

    /* Generate the tables for the display surface */
    Cr_r_tab = &swdata->colortab[0 * 256];
    Cr_g_tab = &swdata->colortab[1 * 256];
    Cb_g_tab = &swdata->colortab[2 * 256];
    Cb_b_tab = &swdata->colortab[3 * 256];
    for (i = 0; i < 256; i++) {
        /* Gamma correction (luminescence table) and chroma correction
           would be done here.  See the Berkeley mpeg_play sources.
         */
        CB = CR = (i - 128);
        Cr_r_tab[i] = (int) ((0.419 / 0.299) * CR);
        Cr_g_tab[i] = (int) (-(0.299 / 0.419) * CR);
        Cb_g_tab[i] = (int) (-(0.114 / 0.331) * CB);
        Cb_b_tab[i] = (int) ((0.587 / 0.331) * CB);
    }

    /* Find the pitch and offset values for the overlay */
    switch (format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
        swdata->pitches[0] = w;
        swdata->pitches[1] = swdata->pitches[0] / 2;
        swdata->pitches[2] = swdata->pitches[0] / 2;
        swdata->planes[0] = swdata->pixels;
        swdata->planes[1] = swdata->planes[0] + swdata->pitches[0] * h;
        swdata->planes[2] = swdata->planes[1] + swdata->pitches[1] * h / 2;
        break;
    case SDL_PIXELFORMAT_YUY2:
    case SDL_PIXELFORMAT_UYVY:
    case SDL_PIXELFORMAT_YVYU:
        swdata->pitches[0] = w * 2;
        swdata->planes[0] = swdata->pixels;
        break;
    default:
        /* We should never get here (caught above) */
        break;
    }

    /* We're all done.. */
    return (swdata);
}

int
SDL_SW_QueryYUVTexturePixels(SDL_SW_YUVTexture * swdata, void **pixels,
                             int *pitch)
{
    *pixels = swdata->planes[0];
    *pitch = swdata->pitches[0];
    return 0;
}

int
SDL_SW_UpdateYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                        const void *pixels, int pitch)
{
    switch (swdata->format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
        if (rect
            && (rect->x != 0 || rect->y != 0 || rect->w != swdata->w
                || rect->h != swdata->h)) {
            SDL_SetError
                ("YV12 and IYUV textures only support full surface updates");
            return -1;
        }
        SDL_memcpy(swdata->pixels, pixels,
                   (swdata->h * swdata->w) + (swdata->h * swdata->w) / 2);
        break;
    case SDL_PIXELFORMAT_YUY2:
    case SDL_PIXELFORMAT_UYVY:
    case SDL_PIXELFORMAT_YVYU:
        {
            Uint8 *src, *dst;
            int row;
            size_t length;

            src = (Uint8 *) pixels;
            dst =
                swdata->planes[0] + rect->y * swdata->pitches[0] +
                rect->x * 2;
            length = rect->w * 2;
            for (row = 0; row < rect->h; ++row) {
                SDL_memcpy(dst, src, length);
                src += pitch;
                dst += swdata->pitches[0];
            }
        }
        break;
    }
    return 0;
}

int
SDL_SW_LockYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                      void **pixels, int *pitch)
{
    switch (swdata->format) {
    case SDL_PIXELFORMAT_YV12:
    case SDL_PIXELFORMAT_IYUV:
        if (rect
            && (rect->x != 0 || rect->y != 0 || rect->w != swdata->w
                || rect->h != swdata->h)) {
            SDL_SetError
                ("YV12 and IYUV textures only support full surface locks");
            return -1;
        }
        break;
    }

    *pixels = swdata->planes[0] + rect->y * swdata->pitches[0] + rect->x * 2;
    *pitch = swdata->pitches[0];
    return 0;
}

void
SDL_SW_UnlockYUVTexture(SDL_SW_YUVTexture * swdata)
{
}

int
SDL_SW_CopyYUVToRGB(SDL_SW_YUVTexture * swdata, const SDL_Rect * srcrect,
                    Uint32 target_format, int w, int h, void *pixels,
                    int pitch)
{
    int stretch;
    int scale_2x;
    Uint8 *lum, *Cr, *Cb;
    int mod;

    /* Make sure we're set up to display in the desired format */
    if (target_format != swdata->target_format) {
        if (SDL_SW_SetupYUVDisplay(swdata, target_format) < 0) {
            return -1;
        }
    }

    stretch = 0;
    scale_2x = 0;
    if (srcrect->x || srcrect->y || srcrect->w < swdata->w
        || srcrect->h < swdata->h) {
        /* The source rectangle has been clipped.
           Using a scratch surface is easier than adding clipped
           source support to all the blitters, plus that would
           slow them down in the general unclipped case.
         */
        stretch = 1;
    } else if ((srcrect->w != w) || (srcrect->h != h)) {
        if ((w == 2 * srcrect->w) && (h == 2 * srcrect->h)) {
            scale_2x = 1;
        } else {
            stretch = 1;
        }
    }
    if (stretch) {
        int bpp;
        Uint32 Rmask, Gmask, Bmask, Amask;

        if (swdata->display) {
            swdata->display->w = w;
            swdata->display->h = h;
            swdata->display->pixels = pixels;
            swdata->display->pitch = pitch;
        } else {
            /* This must have succeeded in SDL_SW_SetupYUVDisplay() earlier */
            SDL_PixelFormatEnumToMasks(target_format, &bpp, &Rmask, &Gmask,
                                       &Bmask, &Amask);
            swdata->display =
                SDL_CreateRGBSurfaceFrom(pixels, w, h, bpp, pitch, Rmask,
                                         Gmask, Bmask, Amask);
            if (!swdata->display) {
                return (-1);
            }
        }
        if (!swdata->stretch) {
            /* This must have succeeded in SDL_SW_SetupYUVDisplay() earlier */
            SDL_PixelFormatEnumToMasks(target_format, &bpp, &Rmask, &Gmask,
                                       &Bmask, &Amask);
            swdata->stretch =
                SDL_CreateRGBSurface(0, swdata->w, swdata->h, bpp, Rmask,
                                     Gmask, Bmask, Amask);
            if (!swdata->stretch) {
                return (-1);
            }
        }
        pixels = swdata->stretch->pixels;
        pitch = swdata->stretch->pitch;
    }
    switch (swdata->format) {
    case SDL_PIXELFORMAT_YV12:
        lum = swdata->planes[0];
        Cr = swdata->planes[1];
        Cb = swdata->planes[2];
        break;
    case SDL_PIXELFORMAT_IYUV:
        lum = swdata->planes[0];
        Cr = swdata->planes[2];
        Cb = swdata->planes[1];
        break;
    case SDL_PIXELFORMAT_YUY2:
        lum = swdata->planes[0];
        Cr = lum + 3;
        Cb = lum + 1;
        break;
    case SDL_PIXELFORMAT_UYVY:
        lum = swdata->planes[0] + 1;
        Cr = lum + 1;
        Cb = lum - 1;
        break;
    case SDL_PIXELFORMAT_YVYU:
        lum = swdata->planes[0];
        Cr = lum + 1;
        Cb = lum + 3;
        break;
    default:
        SDL_SetError("Unsupported YUV format in copy");
        return (-1);
    }
    mod = (pitch / SDL_BYTESPERPIXEL(target_format));

    if (scale_2x) {
        mod -= (swdata->w * 2);
        swdata->Display2X(swdata->colortab, swdata->rgb_2_pix,
                          lum, Cr, Cb, pixels, swdata->h, swdata->w, mod);
    } else {
        mod -= swdata->w;
        swdata->Display1X(swdata->colortab, swdata->rgb_2_pix,
                          lum, Cr, Cb, pixels, swdata->h, swdata->w, mod);
    }
    if (stretch) {
        SDL_Rect rect = *srcrect;
        SDL_SoftStretch(swdata->stretch, &rect, swdata->display, NULL);
    }
    return 0;
}

void
SDL_SW_DestroyYUVTexture(SDL_SW_YUVTexture * swdata)
{
    if (swdata) {
        if (swdata->pixels) {
            SDL_free(swdata->pixels);
        }
        if (swdata->colortab) {
            SDL_free(swdata->colortab);
        }
        if (swdata->rgb_2_pix) {
            SDL_free(swdata->rgb_2_pix);
        }
        if (swdata->stretch) {
            SDL_FreeSurface(swdata->stretch);
        }
        if (swdata->display) {
            SDL_FreeSurface(swdata->display);
        }
        SDL_free(swdata);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
