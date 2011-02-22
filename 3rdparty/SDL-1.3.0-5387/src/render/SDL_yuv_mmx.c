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

#if (__GNUC__ > 2) && defined(__i386__) && __OPTIMIZE__ && SDL_ASSEMBLY_ROUTINES

#include "SDL_stdinc.h"

#include "mmx.h"

/* *INDENT-OFF* */

static mmx_t MMX_0080w    = { .ud = {0x00800080, 0x00800080} };
static mmx_t MMX_00FFw    = { .ud = {0x00ff00ff, 0x00ff00ff} };
static mmx_t MMX_FF00w    = { .ud = {0xff00ff00, 0xff00ff00} };

static mmx_t MMX_Ycoeff   = { .uw = {0x004a, 0x004a, 0x004a, 0x004a} };

static mmx_t MMX_UbluRGB  = { .uw = {0x0072, 0x0072, 0x0072, 0x0072} };
static mmx_t MMX_VredRGB  = { .uw = {0x0059, 0x0059, 0x0059, 0x0059} };
static mmx_t MMX_UgrnRGB  = { .uw = {0xffea, 0xffea, 0xffea, 0xffea} };
static mmx_t MMX_VgrnRGB  = { .uw = {0xffd2, 0xffd2, 0xffd2, 0xffd2} };

static mmx_t MMX_Ublu5x5  = { .uw = {0x0081, 0x0081, 0x0081, 0x0081} };
static mmx_t MMX_Vred5x5  = { .uw = {0x0066, 0x0066, 0x0066, 0x0066} };
static mmx_t MMX_Ugrn565  = { .uw = {0xffe8, 0xffe8, 0xffe8, 0xffe8} };
static mmx_t MMX_Vgrn565  = { .uw = {0xffcd, 0xffcd, 0xffcd, 0xffcd} };

static mmx_t MMX_red565   = { .uw = {0xf800, 0xf800, 0xf800, 0xf800} };
static mmx_t MMX_grn565   = { .uw = {0x07e0, 0x07e0, 0x07e0, 0x07e0} };

/**
   This MMX assembler is my first assembler/MMX program ever.
   Thus it maybe buggy.
   Send patches to:
   mvogt@rhrk.uni-kl.de

   After it worked fine I have "obfuscated" the code a bit to have
   more parallism in the MMX units. This means I moved
   initilisation around and delayed other instruction.
   Performance measurement did not show that this brought any advantage
   but in theory it _should_ be faster this way.

   The overall performanve gain to the C based dither was 30%-40%.
   The MMX routine calculates 256bit=8RGB values in each cycle
   (4 for row1 & 4 for row2)

   The red/green/blue.. coefficents are taken from the mpeg_play 
   player. They look nice, but I dont know if you can have
   better values, to avoid integer rounding errors.
   

   IMPORTANT:
   ==========

   It is a requirement that the cr/cb/lum are 8 byte aligned and
   the out are 16byte aligned or you will/may get segfaults

*/

void ColorRGBDitherYV12MMX1X( int *colortab, Uint32 *rgb_2_pix,
                              unsigned char *lum, unsigned char *cr,
                              unsigned char *cb, unsigned char *out,
                              int rows, int cols, int mod )
{
    Uint32 *row1;
    Uint32 *row2;

    unsigned char* y = lum +cols*rows;    // Pointer to the end
    int x = 0;
    row1 = (Uint32 *)out;                 // 32 bit target
    row2 = (Uint32 *)out+cols+mod;        // start of second row
    mod = (mod+cols+mod)*4;               // increment for row1 in byte

    __asm__ __volatile__ (
        // tap dance to workaround the inability to use %%ebx at will...
        //  move one thing to the stack...
        "pushl $0\n"  // save a slot on the stack.
        "pushl %%ebx\n"  // save %%ebx.
        "movl %0, %%ebx\n"  // put the thing in ebx.
        "movl %%ebx,4(%%esp)\n"  // put the thing in the stack slot.
        "popl %%ebx\n"  // get back %%ebx (the PIC register).

        ".align 8\n"
        "1:\n"

        // create Cr (result in mm1)
        "pushl %%ebx\n"
        "movl 4(%%esp),%%ebx\n"
        "movd (%%ebx),%%mm1\n"   //         0  0  0  0  v3 v2 v1 v0
        "popl %%ebx\n"
        "pxor %%mm7,%%mm7\n"      //         00 00 00 00 00 00 00 00
        "movd (%2), %%mm2\n"           //    0  0  0  0 l3 l2 l1 l0
        "punpcklbw %%mm7,%%mm1\n" //         0  v3 0  v2 00 v1 00 v0
        "punpckldq %%mm1,%%mm1\n" //         00 v1 00 v0 00 v1 00 v0
        "psubw %9,%%mm1\n"        // mm1-128:r1 r1 r0 r0 r1 r1 r0 r0

        // create Cr_g (result in mm0)
        "movq %%mm1,%%mm0\n"           // r1 r1 r0 r0 r1 r1 r0 r0
        "pmullw %10,%%mm0\n"           // red*-46dec=0.7136*64
        "pmullw %11,%%mm1\n"           // red*89dec=1.4013*64
        "psraw  $6, %%mm0\n"           // red=red/64
        "psraw  $6, %%mm1\n"           // red=red/64

        // create L1 L2 (result in mm2,mm4)
        // L2=lum+cols
        "movq (%2,%4),%%mm3\n"         //    0  0  0  0 L3 L2 L1 L0
        "punpckldq %%mm3,%%mm2\n"      //   L3 L2 L1 L0 l3 l2 l1 l0
        "movq %%mm2,%%mm4\n"           //   L3 L2 L1 L0 l3 l2 l1 l0
        "pand %12,%%mm2\n"             //   L3 0  L1  0 l3  0 l1  0
        "pand %13,%%mm4\n"             //   0  L2  0 L0  0 l2  0 l0
        "psrlw $8,%%mm2\n"             //   0  L3  0 L1  0 l3  0 l1

        // create R (result in mm6)
        "movq %%mm2,%%mm5\n"           //   0 L3  0 L1  0 l3  0 l1
        "movq %%mm4,%%mm6\n"           //   0 L2  0 L0  0 l2  0 l0
        "paddsw  %%mm1, %%mm5\n"       // lum1+red:x R3 x R1 x r3 x r1
        "paddsw  %%mm1, %%mm6\n"       // lum1+red:x R2 x R0 x r2 x r0
        "packuswb %%mm5,%%mm5\n"       //  R3 R1 r3 r1 R3 R1 r3 r1
        "packuswb %%mm6,%%mm6\n"       //  R2 R0 r2 r0 R2 R0 r2 r0
        "pxor %%mm7,%%mm7\n"      //         00 00 00 00 00 00 00 00
        "punpcklbw %%mm5,%%mm6\n"      //  R3 R2 R1 R0 r3 r2 r1 r0

        // create Cb (result in mm1)
        "movd (%1), %%mm1\n"      //         0  0  0  0  u3 u2 u1 u0
        "punpcklbw %%mm7,%%mm1\n" //         0  u3 0  u2 00 u1 00 u0
        "punpckldq %%mm1,%%mm1\n" //         00 u1 00 u0 00 u1 00 u0
        "psubw %9,%%mm1\n"        // mm1-128:u1 u1 u0 u0 u1 u1 u0 u0

        // create Cb_g (result in mm5)
        "movq %%mm1,%%mm5\n"            // u1 u1 u0 u0 u1 u1 u0 u0
        "pmullw %14,%%mm5\n"            // blue*-109dec=1.7129*64
        "pmullw %15,%%mm1\n"            // blue*114dec=1.78125*64
        "psraw  $6, %%mm5\n"            // blue=red/64
        "psraw  $6, %%mm1\n"            // blue=blue/64

        // create G (result in mm7)
        "movq %%mm2,%%mm3\n"      //   0  L3  0 L1  0 l3  0 l1
        "movq %%mm4,%%mm7\n"      //   0  L2  0 L0  0 l2  0 l1
        "paddsw  %%mm5, %%mm3\n"  // lum1+Cb_g:x G3t x G1t x g3t x g1t
        "paddsw  %%mm5, %%mm7\n"  // lum1+Cb_g:x G2t x G0t x g2t x g0t
        "paddsw  %%mm0, %%mm3\n"  // lum1+Cr_g:x G3  x G1  x g3  x g1
        "paddsw  %%mm0, %%mm7\n"  // lum1+blue:x G2  x G0  x g2  x g0
        "packuswb %%mm3,%%mm3\n"  // G3 G1 g3 g1 G3 G1 g3 g1
        "packuswb %%mm7,%%mm7\n"  // G2 G0 g2 g0 G2 G0 g2 g0
        "punpcklbw %%mm3,%%mm7\n" // G3 G2 G1 G0 g3 g2 g1 g0

        // create B (result in mm5)
        "movq %%mm2,%%mm3\n"         //   0  L3  0 L1  0 l3  0 l1
        "movq %%mm4,%%mm5\n"         //   0  L2  0 L0  0 l2  0 l1
        "paddsw  %%mm1, %%mm3\n"     // lum1+blue:x B3 x B1 x b3 x b1
        "paddsw  %%mm1, %%mm5\n"     // lum1+blue:x B2 x B0 x b2 x b0
        "packuswb %%mm3,%%mm3\n"     // B3 B1 b3 b1 B3 B1 b3 b1
        "packuswb %%mm5,%%mm5\n"     // B2 B0 b2 b0 B2 B0 b2 b0
        "punpcklbw %%mm3,%%mm5\n"    // B3 B2 B1 B0 b3 b2 b1 b0

        // fill destination row1 (needed are mm6=Rr,mm7=Gg,mm5=Bb)

        "pxor %%mm2,%%mm2\n"           //  0  0  0  0  0  0  0  0
        "pxor %%mm4,%%mm4\n"           //  0  0  0  0  0  0  0  0
        "movq %%mm6,%%mm1\n"           // R3 R2 R1 R0 r3 r2 r1 r0
        "movq %%mm5,%%mm3\n"           // B3 B2 B1 B0 b3 b2 b1 b0

        // process lower lum
        "punpcklbw %%mm4,%%mm1\n"      //  0 r3  0 r2  0 r1  0 r0
        "punpcklbw %%mm4,%%mm3\n"      //  0 b3  0 b2  0 b1  0 b0
        "movq %%mm1,%%mm2\n"           //  0 r3  0 r2  0 r1  0 r0
        "movq %%mm3,%%mm0\n"           //  0 b3  0 b2  0 b1  0 b0
        "punpcklwd %%mm1,%%mm3\n"      //  0 r1  0 b1  0 r0  0 b0
        "punpckhwd %%mm2,%%mm0\n"      //  0 r3  0 b3  0 r2  0 b2

        "pxor %%mm2,%%mm2\n"           //  0  0  0  0  0  0  0  0
        "movq %%mm7,%%mm1\n"           // G3 G2 G1 G0 g3 g2 g1 g0
        "punpcklbw %%mm1,%%mm2\n"      // g3  0 g2  0 g1  0 g0  0
        "punpcklwd %%mm4,%%mm2\n"      //  0  0 g1  0  0  0 g0  0
        "por %%mm3, %%mm2\n"          //  0 r1 g1 b1  0 r0 g0 b0
        "movq %%mm2,(%3)\n"          // wrote out ! row1

        "pxor %%mm2,%%mm2\n"           //  0  0  0  0  0  0  0  0
        "punpcklbw %%mm1,%%mm4\n"      // g3  0 g2  0 g1  0 g0  0
        "punpckhwd %%mm2,%%mm4\n"      //  0  0 g3  0  0  0 g2  0
        "por %%mm0, %%mm4\n"          //  0 r3 g3 b3  0 r2 g2 b2
        "movq %%mm4,8(%3)\n"         // wrote out ! row1

        // fill destination row2 (needed are mm6=Rr,mm7=Gg,mm5=Bb)
        // this can be done "destructive"
        "pxor %%mm2,%%mm2\n"           //  0  0  0  0  0  0  0  0
        "punpckhbw %%mm2,%%mm6\n"      //  0 R3  0 R2  0 R1  0 R0
        "punpckhbw %%mm1,%%mm5\n"      // G3 B3 G2 B2 G1 B1 G0 B0
        "movq %%mm5,%%mm1\n"           // G3 B3 G2 B2 G1 B1 G0 B0
        "punpcklwd %%mm6,%%mm1\n"      //  0 R1 G1 B1  0 R0 G0 B0
        "movq %%mm1,(%5)\n"          // wrote out ! row2
        "punpckhwd %%mm6,%%mm5\n"      //  0 R3 G3 B3  0 R2 G2 B2
        "movq %%mm5,8(%5)\n"         // wrote out ! row2

        "addl $4,%2\n"            // lum+4
        "leal 16(%3),%3\n"        // row1+16
        "leal 16(%5),%5\n"        // row2+16
        "addl $2,(%%esp)\n"        // cr+2
        "addl $2,%1\n"           // cb+2

        "addl $4,%6\n"            // x+4
        "cmpl %4,%6\n"

        "jl 1b\n"
        "addl %4,%2\n" // lum += cols
        "addl %8,%3\n" // row1+= mod
        "addl %8,%5\n" // row2+= mod
        "movl $0,%6\n" // x=0
        "cmpl %7,%2\n"
        "jl 1b\n"

        "addl $4,%%esp\n"  // get rid of the stack slot we reserved.
        "emms\n"  // reset MMX registers.
        :
        : "m" (cr), "r"(cb),"r"(lum),
          "r"(row1),"r"(cols),"r"(row2),"m"(x),"m"(y),"m"(mod),
          "m"(MMX_0080w),"m"(MMX_VgrnRGB),"m"(MMX_VredRGB),
          "m"(MMX_FF00w),"m"(MMX_00FFw),"m"(MMX_UgrnRGB),
          "m"(MMX_UbluRGB)
    );
}

void Color565DitherYV12MMX1X( int *colortab, Uint32 *rgb_2_pix,
                             unsigned char *lum, unsigned char *cr,
                             unsigned char *cb, unsigned char *out,
                             int rows, int cols, int mod )
{
    Uint16 *row1;
    Uint16 *row2;

    unsigned char* y = lum +cols*rows;    /* Pointer to the end */
    int x = 0;
    row1 = (Uint16 *)out;                 /* 16 bit target */
    row2 = (Uint16 *)out+cols+mod;        /* start of second row  */
    mod = (mod+cols+mod)*2;               /* increment for row1 in byte */

    __asm__ __volatile__(
        // tap dance to workaround the inability to use %%ebx at will...
        //  move one thing to the stack...
        "pushl $0\n"  // save a slot on the stack.
        "pushl %%ebx\n"  // save %%ebx.
        "movl %0, %%ebx\n"  // put the thing in ebx.
        "movl %%ebx, 4(%%esp)\n"  // put the thing in the stack slot.
        "popl %%ebx\n"  // get back %%ebx (the PIC register).

        ".align 8\n"
        "1:\n"

        "movd           (%1),                   %%mm0\n" // 4 Cb         0  0  0  0 u3 u2 u1 u0
        "pxor           %%mm7,                  %%mm7\n"
        "pushl %%ebx\n"
        "movl 4(%%esp), %%ebx\n"
        "movd (%%ebx), %%mm1\n"   // 4 Cr                0  0  0  0 v3 v2 v1 v0
        "popl %%ebx\n"

        "punpcklbw      %%mm7,                  %%mm0\n" // 4 W cb   0 u3  0 u2  0 u1  0 u0
        "punpcklbw      %%mm7,                  %%mm1\n" // 4 W cr   0 v3  0 v2  0 v1  0 v0
        "psubw          %9,                     %%mm0\n"
        "psubw          %9,                     %%mm1\n"
        "movq           %%mm0,                  %%mm2\n" // Cb                   0 u3  0 u2  0 u1  0 u0
        "movq           %%mm1,                  %%mm3\n" // Cr
        "pmullw         %10,                    %%mm2\n" // Cb2green 0 R3  0 R2  0 R1  0 R0
        "movq           (%2),                   %%mm6\n" // L1      l7 L6 L5 L4 L3 L2 L1 L0
        "pmullw         %11,                    %%mm0\n" // Cb2blue
        "pand           %12,                    %%mm6\n" // L1      00 L6 00 L4 00 L2 00 L0
        "pmullw         %13,                    %%mm3\n" // Cr2green
        "movq           (%2),                   %%mm7\n" // L2
        "pmullw         %14,                    %%mm1\n" // Cr2red
        "psrlw          $8,                     %%mm7\n"        // L2           00 L7 00 L5 00 L3 00 L1
        "pmullw         %15,                    %%mm6\n" // lum1
        "paddw          %%mm3,                  %%mm2\n" // Cb2green + Cr2green == green
        "pmullw         %15,                    %%mm7\n" // lum2

        "movq           %%mm6,                  %%mm4\n" // lum1
        "paddw          %%mm0,                  %%mm6\n" // lum1 +blue 00 B6 00 B4 00 B2 00 B0
        "movq           %%mm4,                  %%mm5\n" // lum1
        "paddw          %%mm1,                  %%mm4\n" // lum1 +red  00 R6 00 R4 00 R2 00 R0
        "paddw          %%mm2,                  %%mm5\n" // lum1 +green 00 G6 00 G4 00 G2 00 G0
        "psraw          $6,                     %%mm4\n" // R1 0 .. 64
        "movq           %%mm7,                  %%mm3\n" // lum2                       00 L7 00 L5 00 L3 00 L1
        "psraw          $6,                     %%mm5\n" // G1  - .. +
        "paddw          %%mm0,                  %%mm7\n" // Lum2 +blue 00 B7 00 B5 00 B3 00 B1
        "psraw          $6,                     %%mm6\n" // B1         0 .. 64
        "packuswb       %%mm4,                  %%mm4\n" // R1 R1
        "packuswb       %%mm5,                  %%mm5\n" // G1 G1
        "packuswb       %%mm6,                  %%mm6\n" // B1 B1
        "punpcklbw      %%mm4,                  %%mm4\n"
        "punpcklbw      %%mm5,                  %%mm5\n"

        "pand           %16,                    %%mm4\n"
        "psllw          $3,                     %%mm5\n" // GREEN       1
        "punpcklbw      %%mm6,                  %%mm6\n"
        "pand           %17,                    %%mm5\n"
        "pand           %16,                    %%mm6\n"
        "por            %%mm5,                  %%mm4\n" //
        "psrlw          $11,                    %%mm6\n" // BLUE        1
        "movq           %%mm3,                  %%mm5\n" // lum2
        "paddw          %%mm1,                  %%mm3\n" // lum2 +red      00 R7 00 R5 00 R3 00 R1
        "paddw          %%mm2,                  %%mm5\n" // lum2 +green 00 G7 00 G5 00 G3 00 G1
        "psraw          $6,                     %%mm3\n" // R2
        "por            %%mm6,                  %%mm4\n" // MM4
        "psraw          $6,                     %%mm5\n" // G2
        "movq           (%2, %4),               %%mm6\n" // L3 load lum2
        "psraw          $6,                     %%mm7\n"
        "packuswb       %%mm3,                  %%mm3\n"
        "packuswb       %%mm5,                  %%mm5\n"
        "packuswb       %%mm7,                  %%mm7\n"
        "pand           %12,                    %%mm6\n" // L3
        "punpcklbw      %%mm3,                  %%mm3\n"
        "punpcklbw      %%mm5,                  %%mm5\n"
        "pmullw         %15,                    %%mm6\n" // lum3
        "punpcklbw      %%mm7,                  %%mm7\n"
        "psllw          $3,                     %%mm5\n" // GREEN 2
        "pand           %16,                    %%mm7\n"
        "pand           %16,                    %%mm3\n"
        "psrlw          $11,                    %%mm7\n" // BLUE  2
        "pand           %17,                    %%mm5\n"
        "por            %%mm7,                  %%mm3\n"
        "movq           (%2,%4),                %%mm7\n" // L4 load lum2
        "por            %%mm5,                  %%mm3\n" //
        "psrlw          $8,                     %%mm7\n" // L4
        "movq           %%mm4,                  %%mm5\n"
        "punpcklwd      %%mm3,                  %%mm4\n"
        "pmullw         %15,                    %%mm7\n" // lum4
        "punpckhwd      %%mm3,                  %%mm5\n"

        "movq           %%mm4,                  (%3)\n"  // write row1
        "movq           %%mm5,                  8(%3)\n" // write row1

        "movq           %%mm6,                  %%mm4\n" // Lum3
        "paddw          %%mm0,                  %%mm6\n" // Lum3 +blue

        "movq           %%mm4,                  %%mm5\n" // Lum3
        "paddw          %%mm1,                  %%mm4\n" // Lum3 +red
        "paddw          %%mm2,                  %%mm5\n" // Lum3 +green
        "psraw          $6,                     %%mm4\n"
        "movq           %%mm7,                  %%mm3\n" // Lum4
        "psraw          $6,                     %%mm5\n"
        "paddw          %%mm0,                  %%mm7\n" // Lum4 +blue
        "psraw          $6,                     %%mm6\n" // Lum3 +blue
        "movq           %%mm3,                  %%mm0\n" // Lum4
        "packuswb       %%mm4,                  %%mm4\n"
        "paddw          %%mm1,                  %%mm3\n" // Lum4 +red
        "packuswb       %%mm5,                  %%mm5\n"
        "paddw          %%mm2,                  %%mm0\n" // Lum4 +green
        "packuswb       %%mm6,                  %%mm6\n"
        "punpcklbw      %%mm4,                  %%mm4\n"
        "punpcklbw      %%mm5,                  %%mm5\n"
        "punpcklbw      %%mm6,                  %%mm6\n"
        "psllw          $3,                     %%mm5\n" // GREEN 3
        "pand           %16,                    %%mm4\n"
        "psraw          $6,                     %%mm3\n" // psr 6
        "psraw          $6,                     %%mm0\n"
        "pand           %16,                    %%mm6\n" // BLUE
        "pand           %17,                    %%mm5\n"
        "psrlw          $11,                    %%mm6\n" // BLUE  3
        "por            %%mm5,                  %%mm4\n"
        "psraw          $6,                     %%mm7\n"
        "por            %%mm6,                  %%mm4\n"
        "packuswb       %%mm3,                  %%mm3\n"
        "packuswb       %%mm0,                  %%mm0\n"
        "packuswb       %%mm7,                  %%mm7\n"
        "punpcklbw      %%mm3,                  %%mm3\n"
        "punpcklbw      %%mm0,                  %%mm0\n"
        "punpcklbw      %%mm7,                  %%mm7\n"
        "pand           %16,                    %%mm3\n"
        "pand           %16,                    %%mm7\n" // BLUE
        "psllw          $3,                     %%mm0\n" // GREEN 4
        "psrlw          $11,                    %%mm7\n"
        "pand           %17,                    %%mm0\n"
        "por            %%mm7,                  %%mm3\n"
        "por            %%mm0,                  %%mm3\n"

        "movq           %%mm4,                  %%mm5\n"

        "punpcklwd      %%mm3,                  %%mm4\n"
        "punpckhwd      %%mm3,                  %%mm5\n"

        "movq           %%mm4,                  (%5)\n"
        "movq           %%mm5,                  8(%5)\n"

        "addl           $8,                     %6\n"
        "addl           $8,                     %2\n"
        "addl           $4,                     (%%esp)\n"
        "addl           $4,                     %1\n"
        "cmpl           %4,                     %6\n"
        "leal           16(%3),                 %3\n"
        "leal           16(%5),%5\n" // row2+16

        "jl             1b\n"
        "addl           %4,     %2\n" // lum += cols
        "addl           %8,     %3\n" // row1+= mod
        "addl           %8,     %5\n" // row2+= mod
        "movl           $0,     %6\n" // x=0
        "cmpl           %7,     %2\n"
        "jl             1b\n"
        "addl $4, %%esp\n"  // get rid of the stack slot we reserved.
        "emms\n"
        :
        : "m" (cr), "r"(cb),"r"(lum),
          "r"(row1),"r"(cols),"r"(row2),"m"(x),"m"(y),"m"(mod),
          "m"(MMX_0080w),"m"(MMX_Ugrn565),"m"(MMX_Ublu5x5),
          "m"(MMX_00FFw),"m"(MMX_Vgrn565),"m"(MMX_Vred5x5),
          "m"(MMX_Ycoeff),"m"(MMX_red565),"m"(MMX_grn565)
    );
}

/* *INDENT-ON* */

#endif /* GCC3 i386 inline assembly */

/* vi: set ts=4 sw=4 expandtab: */
