/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "GS.h"
#include "Mem.h"
#include "Mem_Swizzle.h"

u32 g_blockTable32[4][8] =
{
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

u32 g_blockTable32Z[4][8] =
{
	{ 24, 25, 28, 29,  8,  9, 12, 13},
	{ 26, 27, 30, 31, 10, 11, 14, 15},
	{ 16, 17, 20, 21,  0,  1,  4,  5},
	{ 18, 19, 22, 23,  2,  3,  6,  7}
};

u32 g_blockTable16[8][4] =
{
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

u32 g_blockTable16S[8][4] =
{
	{  0,  2, 16, 18 },
	{  1,  3, 17, 19 },
	{  8, 10, 24, 26 },
	{  9, 11, 25, 27 },
	{  4,  6, 20, 22 },
	{  5,  7, 21, 23 },
	{ 12, 14, 28, 30 },
	{ 13, 15, 29, 31 }
};

u32 g_blockTable16Z[8][4] =
{
	{ 24, 26, 16, 18 },
	{ 25, 27, 17, 19 },
	{ 28, 30, 20, 22 },
	{ 29, 31, 21, 23 },
	{  8, 10,  0,  2 },
	{  9, 11,  1,  3 },
	{ 12, 14,  4,  6 },
	{ 13, 15,  5,  7 }
};

u32 g_blockTable16SZ[8][4] =
{
	{ 24, 26,  8, 10 },
	{ 25, 27,  9, 11 },
	{ 16, 18,  0,  2 },
	{ 17, 19,  1,  3 },
	{ 28, 30, 12, 14 },
	{ 29, 31, 13, 15 },
	{ 20, 22,  4,  6 },
	{ 21, 23,  5,  7 }
};

u32 g_blockTable8[4][8] =
{
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

u32 g_blockTable4[8][4] =
{
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

u32 g_columnTable32[8][8] =
{
	{  0,  1,  4,  5,  8,  9, 12, 13 },
	{  2,  3,  6,  7, 10, 11, 14, 15 },
	{ 16, 17, 20, 21, 24, 25, 28, 29 },
	{ 18, 19, 22, 23, 26, 27, 30, 31 },
	{ 32, 33, 36, 37, 40, 41, 44, 45 },
	{ 34, 35, 38, 39, 42, 43, 46, 47 },
	{ 48, 49, 52, 53, 56, 57, 60, 61 },
	{ 50, 51, 54, 55, 58, 59, 62, 63 },
};

u32 g_columnTable16[8][16] =
{
	{   0,   2,   8,  10,  16,  18,  24,  26,
		1,   3,   9,  11,  17,  19,  25,  27 },
	{   4,   6,  12,  14,  20,  22,  28,  30,
		5,   7,  13,  15,  21,  23,  29,  31 },
	{  32,  34,  40,  42,  48,  50,  56,  58,
	   33,  35,  41,  43,  49,  51,  57,  59 },
	{  36,  38,  44,  46,  52,  54,  60,  62,
	   37,  39,  45,  47,  53,  55,  61,  63 },
	{  64,  66,  72,  74,  80,  82,  88,  90,
	   65,  67,  73,  75,  81,  83,  89,  91 },
	{  68,  70,  76,  78,  84,  86,  92,  94,
	   69,  71,  77,  79,  85,  87,  93,  95 },
	{  96,  98, 104, 106, 112, 114, 120, 122,
	   97,  99, 105, 107, 113, 115, 121, 123 },
	{ 100, 102, 108, 110, 116, 118, 124, 126,
	  101, 103, 109, 111, 117, 119, 125, 127 },
};

u32 g_columnTable8[16][16] =
{
	{   0,   4,  16,  20,  32,  36,  48,  52,   // column 0
		2,   6,  18,  22,  34,  38,  50,  54 },
	{   8,  12,  24,  28,  40,  44,  56,  60,
		10,  14,  26,  30,  42,  46,  58,  62 },
	{  33,  37,  49,  53,   1,   5,  17,  21,
	   35,  39,  51,  55,   3,   7,  19,  23 },
	{  41,  45,  57,  61,   9,  13,  25,  29,
	   43,  47,  59,  63,  11,  15,  27,  31 },
	{  96, 100, 112, 116,  64,  68,  80,  84,   // column 1
	   98, 102, 114, 118,  66,  70,  82,  86 },
	{ 104, 108, 120, 124,  72,  76,  88,  92,
	  106, 110, 122, 126,  74,  78,  90,  94 },
	{  65,  69,  81,  85,  97, 101, 113, 117,
	   67,  71,  83,  87,  99, 103, 115, 119 },
	{  73,  77,  89,  93, 105, 109, 121, 125,
	   75,  79,  91,  95, 107, 111, 123, 127 },
	{ 128, 132, 144, 148, 160, 164, 176, 180,   // column 2
	  130, 134, 146, 150, 162, 166, 178, 182 },
	{ 136, 140, 152, 156, 168, 172, 184, 188,
	  138, 142, 154, 158, 170, 174, 186, 190 },
	{ 161, 165, 177, 181, 129, 133, 145, 149,
	  163, 167, 179, 183, 131, 135, 147, 151 },
	{ 169, 173, 185, 189, 137, 141, 153, 157,
	  171, 175, 187, 191, 139, 143, 155, 159 },
	{ 224, 228, 240, 244, 192, 196, 208, 212,   // column 3
	  226, 230, 242, 246, 194, 198, 210, 214 },
	{ 232, 236, 248, 252, 200, 204, 216, 220,
	  234, 238, 250, 254, 202, 206, 218, 222 },
	{ 193, 197, 209, 213, 225, 229, 241, 245,
	  195, 199, 211, 215, 227, 231, 243, 247 },
	{ 201, 205, 217, 221, 233, 237, 249, 253,
	  203, 207, 219, 223, 235, 239, 251, 255 },
};

u32 g_columnTable4[16][32] =
{
	{   0,   8,  32,  40,  64,  72,  96, 104,   // column 0
		2,  10,  34,  42,  66,  74,  98, 106,
		4,  12,  36,  44,  68,  76, 100, 108,
		6,  14,  38,  46,  70,  78, 102, 110 },
	{  16,  24,  48,  56,  80,  88, 112, 120,
	   18,  26,  50,  58,  82,  90, 114, 122,
	   20,  28,  52,  60,  84,  92, 116, 124,
	   22,  30,  54,  62,  86,  94, 118, 126 },
	{  65,  73,  97, 105,   1,   9,  33,  41,
	   67,  75,  99, 107,   3,  11,  35,  43,
	   69,  77, 101, 109,   5,  13,  37,  45,
	   71,  79, 103, 111,   7,  15,  39,  47 },
	{  81,  89, 113, 121,  17,  25,  49,  57,
	   83,  91, 115, 123,  19,  27,  51,  59,
	   85,  93, 117, 125,  21,  29,  53,  61,
	   87,  95, 119, 127,  23,  31,  55,  63 },
	{ 192, 200, 224, 232, 128, 136, 160, 168,   // column 1
	  194, 202, 226, 234, 130, 138, 162, 170,
	  196, 204, 228, 236, 132, 140, 164, 172,
	  198, 206, 230, 238, 134, 142, 166, 174 },
	{ 208, 216, 240, 248, 144, 152, 176, 184,
	  210, 218, 242, 250, 146, 154, 178, 186,
	  212, 220, 244, 252, 148, 156, 180, 188,
	  214, 222, 246, 254, 150, 158, 182, 190 },
	{ 129, 137, 161, 169, 193, 201, 225, 233,
	  131, 139, 163, 171, 195, 203, 227, 235,
	  133, 141, 165, 173, 197, 205, 229, 237,
	  135, 143, 167, 175, 199, 207, 231, 239 },
	{ 145, 153, 177, 185, 209, 217, 241, 249,
	  147, 155, 179, 187, 211, 219, 243, 251,
	  149, 157, 181, 189, 213, 221, 245, 253,
	  151, 159, 183, 191, 215, 223, 247, 255 },
	{ 256, 264, 288, 296, 320, 328, 352, 360,   // column 2
	  258, 266, 290, 298, 322, 330, 354, 362,
	  260, 268, 292, 300, 324, 332, 356, 364,
	  262, 270, 294, 302, 326, 334, 358, 366 },
	{ 272, 280, 304, 312, 336, 344, 368, 376,
	  274, 282, 306, 314, 338, 346, 370, 378,
	  276, 284, 308, 316, 340, 348, 372, 380,
	  278, 286, 310, 318, 342, 350, 374, 382 },
	{ 321, 329, 353, 361, 257, 265, 289, 297,
	  323, 331, 355, 363, 259, 267, 291, 299,
	  325, 333, 357, 365, 261, 269, 293, 301,
	  327, 335, 359, 367, 263, 271, 295, 303 },
	{ 337, 345, 369, 377, 273, 281, 305, 313,
	  339, 347, 371, 379, 275, 283, 307, 315,
	  341, 349, 373, 381, 277, 285, 309, 317,
	  343, 351, 375, 383, 279, 287, 311, 319 },
	{ 448, 456, 480, 488, 384, 392, 416, 424,   // column 3
	  450, 458, 482, 490, 386, 394, 418, 426,
	  452, 460, 484, 492, 388, 396, 420, 428,
	  454, 462, 486, 494, 390, 398, 422, 430 },
	{ 464, 472, 496, 504, 400, 408, 432, 440,
	  466, 474, 498, 506, 402, 410, 434, 442,
	  468, 476, 500, 508, 404, 412, 436, 444,
	  470, 478, 502, 510, 406, 414, 438, 446 },
	{ 385, 393, 417, 425, 449, 457, 481, 489,
	  387, 395, 419, 427, 451, 459, 483, 491,
	  389, 397, 421, 429, 453, 461, 485, 493,
	  391, 399, 423, 431, 455, 463, 487, 495 },
	{ 401, 409, 433, 441, 465, 473, 497, 505,
	  403, 411, 435, 443, 467, 475, 499, 507,
	  405, 413, 437, 445, 469, 477, 501, 509,
	  407, 415, 439, 447, 471, 479, 503, 511 },
};

u32 g_pageTable32[32][64];
u32 g_pageTable32Z[32][64];
u32 g_pageTable16[64][64];
u32 g_pageTable16S[64][64];
u32 g_pageTable16Z[64][64];
u32 g_pageTable16SZ[64][64];
u32 g_pageTable8[64][128];
u32 g_pageTable4[128][128];

/* PSM reference array
{ 	32, 24, 16, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, 16S, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, 8, 4, NULL, NULL, NULL,
	NULL, NULL, NULL, 8H, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, 4HL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, 4HH, NULL, NULL, NULL,
	32Z, 24Z, 16Z, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, 16SZ, NULL, NULL, NULL, NULL, NULL };
*/
char* psm_name[64] =
{ 	"PSMCT32", "PSMCT24", "PSMCT16", NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, "PSMCT16S", NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "PSMT8", "PSMT4", NULL, NULL, NULL,
	NULL, NULL, NULL, "PSMT8H", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, "PSMT4HL", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, "PSMT4HH", NULL, NULL, NULL,
	"PSMT32Z", "PSMT24Z", "PSMT16Z", NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, "PSMT16SZ", NULL, NULL, NULL, NULL, NULL };
	
_SwizzleBlock swizzleBlockFun[64] =
{ 	SwizzleBlock32, SwizzleBlock24, SwizzleBlock16, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, SwizzleBlock16S, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, SwizzleBlock8, SwizzleBlock4, NULL, NULL, NULL,
	NULL, NULL, NULL, SwizzleBlock8H, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, SwizzleBlock4HL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, SwizzleBlock4HH, NULL, NULL, NULL,
	SwizzleBlock32Z, SwizzleBlock24Z, SwizzleBlock16Z, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, SwizzleBlock16SZ, NULL, NULL, NULL, NULL, NULL };
	
_SwizzleBlock swizzleBlockUnFun[64] =
{ 	SwizzleBlock32u, SwizzleBlock24u, SwizzleBlock16u, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, SwizzleBlock16Su, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, SwizzleBlock8u, SwizzleBlock4u, NULL, NULL, NULL,
	NULL, NULL, NULL, SwizzleBlock8Hu, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, SwizzleBlock4HLu, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, SwizzleBlock4HHu, NULL, NULL, NULL,
	SwizzleBlock32Zu, SwizzleBlock24Zu, SwizzleBlock16Zu, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, SwizzleBlock16SZu, NULL, NULL, NULL, NULL, NULL };
	
_getPixelAddress_0 getPixelFun_0[64] = 
{ 	
	getPixelAddress32_0, getPixelAddress24_0, getPixelAddress16_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, getPixelAddress16S_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, getPixelAddress8_0, getPixelAddress4_0, NULL, NULL, NULL,
	NULL, NULL, NULL, getPixelAddress8H_0, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, getPixelAddress4HL_0, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, getPixelAddress4HH_0, NULL, NULL, NULL,
	getPixelAddress32Z_0, getPixelAddress24Z_0, getPixelAddress16Z_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, getPixelAddress16SZ_0, NULL, NULL, NULL, NULL, NULL 
};

_writePixel_0 writePixelFun_0[64] = 
{ 	
	writePixel32_0, writePixel24_0, writePixel16_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, writePixel16S_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, writePixel8_0, writePixel4_0, NULL, NULL, NULL,
	NULL, NULL, NULL, writePixel8H_0, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, writePixel4HL_0, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, writePixel4HH_0, NULL, NULL, NULL,
	writePixel32Z_0, writePixel24Z_0, writePixel16Z_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, writePixel16SZ_0, NULL, NULL, NULL, NULL, NULL 
};

_readPixel_0 readPixelFun_0[64] = 
{ 	
	readPixel32_0, readPixel24_0, readPixel16_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, readPixel16S_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, readPixel8_0, readPixel4_0, NULL, NULL, NULL,
	NULL, NULL, NULL, readPixel8H_0, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, readPixel4HL_0, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, readPixel4HH_0, NULL, NULL, NULL,
	readPixel32Z_0, readPixel24Z_0, readPixel16Z_0, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, readPixel16SZ_0, NULL, NULL, NULL, NULL, NULL 
};

#define TD_NULL {0,0,0,0,0,0}
TransferData tData[64] =
{ 	
	{2,32,8,8,32,PSMCT32}, 
	{8,32,8,8,24,PSMCT24}, 
	{4,16,16,8,16,PSMCT16}, 
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, 
	{4,16,16,8,16,PSMCT16S}, 
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, TD_NULL, 
	{4,8,16,16,8,PSMT8}, 
	{8,4,32,16,4,PSMT4}, 
	TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, TD_NULL, 
	{4,32,8,8,8,PSMT8H}, 
	TD_NULL, TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, 
	{8,32,8,8,4,PSMT4HL}, 
	TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, 
	{8,32,8,8,4,PSMT4HH}, 
	TD_NULL, TD_NULL, TD_NULL,
	{2,32,8,8,32,PSMT32Z}, 
	{8,32,8,8,24,PSMT24Z},
	{4,16,16,8,16,PSMT16Z}, 
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, TD_NULL,
	TD_NULL, TD_NULL, 
	{4,16,16,8,16,PSMT16SZ}, 
	TD_NULL, TD_NULL, TD_NULL, TD_NULL, TD_NULL 
};

