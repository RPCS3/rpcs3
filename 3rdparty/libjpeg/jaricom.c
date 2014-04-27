/*
 * jaricom.c
 *
 * Developed 1997 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains probability estimation tables for common use in
 * arithmetic entropy encoding and decoding routines.
 *
 * This data represents Table D.2 in the JPEG spec (ISO/IEC IS 10918-1
 * and CCITT Recommendation ITU-T T.81) and Table 24 in the JBIG spec
 * (ISO/IEC IS 11544 and CCITT Recommendation ITU-T T.82).
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

/* The following #define specifies the packing of the four components
 * into the compact INT32 representation.
 * Note that this formula must match the actual arithmetic encoder
 * and decoder implementation.  The implementation has to be changed
 * if this formula is changed.
 * The current organization is leaned on Markus Kuhn's JBIG
 * implementation (jbig_tab.c).
 */

#define V(a,b,c,d) (((INT32)a << 16) | ((INT32)c << 8) | ((INT32)d << 7) | b)

const INT32 jaritab[113] = {
/*
 * Index, Qe_Value, Next_Index_LPS, Next_Index_MPS, Switch_MPS
 */
/*   0 */  V( 0x5a1d,   1,   1, 1 ),
/*   1 */  V( 0x2586,  14,   2, 0 ),
/*   2 */  V( 0x1114,  16,   3, 0 ),
/*   3 */  V( 0x080b,  18,   4, 0 ),
/*   4 */  V( 0x03d8,  20,   5, 0 ),
/*   5 */  V( 0x01da,  23,   6, 0 ),
/*   6 */  V( 0x00e5,  25,   7, 0 ),
/*   7 */  V( 0x006f,  28,   8, 0 ),
/*   8 */  V( 0x0036,  30,   9, 0 ),
/*   9 */  V( 0x001a,  33,  10, 0 ),
/*  10 */  V( 0x000d,  35,  11, 0 ),
/*  11 */  V( 0x0006,   9,  12, 0 ),
/*  12 */  V( 0x0003,  10,  13, 0 ),
/*  13 */  V( 0x0001,  12,  13, 0 ),
/*  14 */  V( 0x5a7f,  15,  15, 1 ),
/*  15 */  V( 0x3f25,  36,  16, 0 ),
/*  16 */  V( 0x2cf2,  38,  17, 0 ),
/*  17 */  V( 0x207c,  39,  18, 0 ),
/*  18 */  V( 0x17b9,  40,  19, 0 ),
/*  19 */  V( 0x1182,  42,  20, 0 ),
/*  20 */  V( 0x0cef,  43,  21, 0 ),
/*  21 */  V( 0x09a1,  45,  22, 0 ),
/*  22 */  V( 0x072f,  46,  23, 0 ),
/*  23 */  V( 0x055c,  48,  24, 0 ),
/*  24 */  V( 0x0406,  49,  25, 0 ),
/*  25 */  V( 0x0303,  51,  26, 0 ),
/*  26 */  V( 0x0240,  52,  27, 0 ),
/*  27 */  V( 0x01b1,  54,  28, 0 ),
/*  28 */  V( 0x0144,  56,  29, 0 ),
/*  29 */  V( 0x00f5,  57,  30, 0 ),
/*  30 */  V( 0x00b7,  59,  31, 0 ),
/*  31 */  V( 0x008a,  60,  32, 0 ),
/*  32 */  V( 0x0068,  62,  33, 0 ),
/*  33 */  V( 0x004e,  63,  34, 0 ),
/*  34 */  V( 0x003b,  32,  35, 0 ),
/*  35 */  V( 0x002c,  33,   9, 0 ),
/*  36 */  V( 0x5ae1,  37,  37, 1 ),
/*  37 */  V( 0x484c,  64,  38, 0 ),
/*  38 */  V( 0x3a0d,  65,  39, 0 ),
/*  39 */  V( 0x2ef1,  67,  40, 0 ),
/*  40 */  V( 0x261f,  68,  41, 0 ),
/*  41 */  V( 0x1f33,  69,  42, 0 ),
/*  42 */  V( 0x19a8,  70,  43, 0 ),
/*  43 */  V( 0x1518,  72,  44, 0 ),
/*  44 */  V( 0x1177,  73,  45, 0 ),
/*  45 */  V( 0x0e74,  74,  46, 0 ),
/*  46 */  V( 0x0bfb,  75,  47, 0 ),
/*  47 */  V( 0x09f8,  77,  48, 0 ),
/*  48 */  V( 0x0861,  78,  49, 0 ),
/*  49 */  V( 0x0706,  79,  50, 0 ),
/*  50 */  V( 0x05cd,  48,  51, 0 ),
/*  51 */  V( 0x04de,  50,  52, 0 ),
/*  52 */  V( 0x040f,  50,  53, 0 ),
/*  53 */  V( 0x0363,  51,  54, 0 ),
/*  54 */  V( 0x02d4,  52,  55, 0 ),
/*  55 */  V( 0x025c,  53,  56, 0 ),
/*  56 */  V( 0x01f8,  54,  57, 0 ),
/*  57 */  V( 0x01a4,  55,  58, 0 ),
/*  58 */  V( 0x0160,  56,  59, 0 ),
/*  59 */  V( 0x0125,  57,  60, 0 ),
/*  60 */  V( 0x00f6,  58,  61, 0 ),
/*  61 */  V( 0x00cb,  59,  62, 0 ),
/*  62 */  V( 0x00ab,  61,  63, 0 ),
/*  63 */  V( 0x008f,  61,  32, 0 ),
/*  64 */  V( 0x5b12,  65,  65, 1 ),
/*  65 */  V( 0x4d04,  80,  66, 0 ),
/*  66 */  V( 0x412c,  81,  67, 0 ),
/*  67 */  V( 0x37d8,  82,  68, 0 ),
/*  68 */  V( 0x2fe8,  83,  69, 0 ),
/*  69 */  V( 0x293c,  84,  70, 0 ),
/*  70 */  V( 0x2379,  86,  71, 0 ),
/*  71 */  V( 0x1edf,  87,  72, 0 ),
/*  72 */  V( 0x1aa9,  87,  73, 0 ),
/*  73 */  V( 0x174e,  72,  74, 0 ),
/*  74 */  V( 0x1424,  72,  75, 0 ),
/*  75 */  V( 0x119c,  74,  76, 0 ),
/*  76 */  V( 0x0f6b,  74,  77, 0 ),
/*  77 */  V( 0x0d51,  75,  78, 0 ),
/*  78 */  V( 0x0bb6,  77,  79, 0 ),
/*  79 */  V( 0x0a40,  77,  48, 0 ),
/*  80 */  V( 0x5832,  80,  81, 1 ),
/*  81 */  V( 0x4d1c,  88,  82, 0 ),
/*  82 */  V( 0x438e,  89,  83, 0 ),
/*  83 */  V( 0x3bdd,  90,  84, 0 ),
/*  84 */  V( 0x34ee,  91,  85, 0 ),
/*  85 */  V( 0x2eae,  92,  86, 0 ),
/*  86 */  V( 0x299a,  93,  87, 0 ),
/*  87 */  V( 0x2516,  86,  71, 0 ),
/*  88 */  V( 0x5570,  88,  89, 1 ),
/*  89 */  V( 0x4ca9,  95,  90, 0 ),
/*  90 */  V( 0x44d9,  96,  91, 0 ),
/*  91 */  V( 0x3e22,  97,  92, 0 ),
/*  92 */  V( 0x3824,  99,  93, 0 ),
/*  93 */  V( 0x32b4,  99,  94, 0 ),
/*  94 */  V( 0x2e17,  93,  86, 0 ),
/*  95 */  V( 0x56a8,  95,  96, 1 ),
/*  96 */  V( 0x4f46, 101,  97, 0 ),
/*  97 */  V( 0x47e5, 102,  98, 0 ),
/*  98 */  V( 0x41cf, 103,  99, 0 ),
/*  99 */  V( 0x3c3d, 104, 100, 0 ),
/* 100 */  V( 0x375e,  99,  93, 0 ),
/* 101 */  V( 0x5231, 105, 102, 0 ),
/* 102 */  V( 0x4c0f, 106, 103, 0 ),
/* 103 */  V( 0x4639, 107, 104, 0 ),
/* 104 */  V( 0x415e, 103,  99, 0 ),
/* 105 */  V( 0x5627, 105, 106, 1 ),
/* 106 */  V( 0x50e7, 108, 107, 0 ),
/* 107 */  V( 0x4b85, 109, 103, 0 ),
/* 108 */  V( 0x5597, 110, 109, 0 ),
/* 109 */  V( 0x504f, 111, 107, 0 ),
/* 110 */  V( 0x5a10, 110, 111, 1 ),
/* 111 */  V( 0x5522, 112, 109, 0 ),
/* 112 */  V( 0x59eb, 112, 111, 1 )
};
