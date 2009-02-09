/*
 * yuv2rgb.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * Modified by Florin for PCSX2 emu
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "System.h"
#include "mpeg2lib/Mpeg.h"
#include "yuv2rgb.h"

//#include "convert_internal.h" //START
struct convert_rgb_t {
    u8 * rgb_ptr;
    int width;
    int uv_stride, uv_stride_frame;
    int rgb_stride, rgb_stride_frame;
    void (__fastcall * yuv2rgb) (u8 *, u8 *, u8 *, u8 *,
		      void *, void *, int);
};

typedef void __fastcall yuv2rgb_copy (void * id, u8 * const * src,
			   unsigned int v_offset);

yuv2rgb_copy __fastcall * yuv2rgb_init_mmxext (int bpp, int mode);
yuv2rgb_copy __fastcall * yuv2rgb_init_mmx (int bpp, int mode);
yuv2rgb_copy __fastcall * yuv2rgb_init_mlib (int bpp, int mode);
//#include "convert_internal.h" //END

static u32 matrix_coefficients = 6;

const s32 Inverse_Table_6_9[8][4] = {
    {117504, 138453, 13954, 34903}, /*0 no sequence_display_extension */
    {117504, 138453, 13954, 34903}, /*1 ITU-R Rec. 709 (1990) */
    {104597, 132201, 25675, 53279}, /*2 unspecified */
    {104597, 132201, 25675, 53279}, /*3 reserved */
    {104448, 132798, 24759, 53109}, /*4 FCC */
    {104597, 132201, 25675, 53279}, /*5 ITU-R Rec. 624-4 System B, G */
    {104597, 132201, 25675, 53279}, /*6 SMPTE 170M */
    {117579, 136230, 16907, 35559}  /*7 SMPTE 240M (1987) */
};

typedef void __fastcall yuv2rgb_c_internal (u8 *, u8 *, u8 *, u8 *,
				 void *, void *, int);

void * table_rV[256];
void * table_gU[256];
int table_gV[256];
void * table_bU[256];

#define _RGB(type,i)						\
	U = pu[i];						\
	V = pv[i];						\
	r = (type *) table_rV[V];				\
	g = (type *) (((u8 *)table_gU[U]) + table_gV[V]);	\
	b = (type *) table_bU[U];

#define DST(py,dst,i)				\
	Y = py[2*i];				\
	dst[2*i] = r[Y] + g[Y] + b[Y];		\
	Y = py[2*i+1];				\
	dst[2*i+1] = r[Y] + g[Y] + b[Y];

#define DSTRGB(py,dst,i)						\
	Y = py[2*i];							\
	dst[6*i] = r[Y]; dst[6*i+1] = g[Y]; dst[6*i+2] = b[Y];		\
	Y = py[2*i+1];							\
	dst[6*i+3] = r[Y]; dst[6*i+4] = g[Y]; dst[6*i+5] = b[Y];

#define DSTBGR(py,dst,i)						\
	Y = py[2*i];							\
	dst[6*i] = b[Y]; dst[6*i+1] = g[Y]; dst[6*i+2] = r[Y];		\
	Y = py[2*i+1];							\
	dst[6*i+3] = b[Y]; dst[6*i+4] = g[Y]; dst[6*i+5] = r[Y];

static void __fastcall yuv2rgb_c_32 (u8 * py_1, u8 * py_2,
			  u8 * pu, u8 * pv,
			  void * _dst_1, void * _dst_2, int width)
{
    int U, V, Y;
    u32 * r, * g, * b;
    u32 * dst_1, * dst_2;

    width >>= 3;
    dst_1 = (u32 *) _dst_1;
    dst_2 = (u32 *) _dst_2;

    do {
		_RGB (u32, 0);
		DST (py_1, dst_1, 0);
		DST (py_2, dst_2, 0);

		_RGB (u32, 1);
		DST (py_2, dst_2, 1);
		DST (py_1, dst_1, 1);

		_RGB (u32, 2);
		DST (py_1, dst_1, 2);
		DST (py_2, dst_2, 2);

		_RGB (u32, 3);
		DST (py_2, dst_2, 3);
		DST (py_1, dst_1, 3);

		pu += 4;
		pv += 4;
		py_1 += 8;
		py_2 += 8;
		dst_1 += 8;
		dst_2 += 8;
    } while (--width);
}

/* This is very near from the yuv2rgb_c_32 code */
static void __fastcall yuv2rgb_c_24_rgb (u8 * py_1, u8 * py_2,
			      u8 * pu, u8 * pv,
			      void * _dst_1, void * _dst_2, int width)
{
    int U, V, Y;
    u8 * r, * g, * b;
    u8 * dst_1, * dst_2;

    width >>= 3;
    dst_1 = (u8 *) _dst_1;
    dst_2 = (u8 *) _dst_2;

    do {
		_RGB (u8, 0);
		DSTRGB (py_1, dst_1, 0);
		DSTRGB (py_2, dst_2, 0);

		_RGB (u8, 1);
		DSTRGB (py_2, dst_2, 1);
		DSTRGB (py_1, dst_1, 1);

		_RGB (u8, 2);
		DSTRGB (py_1, dst_1, 2);
		DSTRGB (py_2, dst_2, 2);

		_RGB (u8, 3);
		DSTRGB (py_2, dst_2, 3);
		DSTRGB (py_1, dst_1, 3);

		pu += 4;
		pv += 4;
		py_1 += 8;
		py_2 += 8;
		dst_1 += 24;
		dst_2 += 24;
    } while (--width);
}

/* only trivial mods from yuv2rgb_c_24_rgb */
static void __fastcall yuv2rgb_c_24_bgr (u8 * py_1, u8 * py_2,
			      u8 * pu, u8 * pv,
			      void * _dst_1, void * _dst_2, int width)
{
    int U, V, Y;
    u8 * r, * g, * b;
    u8 * dst_1, * dst_2;

    width >>= 3;
    dst_1 = (u8 *) _dst_1;
    dst_2 = (u8 *) _dst_2;

    do {
		_RGB (u8, 0);
		DSTBGR (py_1, dst_1, 0);
		DSTBGR (py_2, dst_2, 0);

		_RGB (u8, 1);
		DSTBGR (py_2, dst_2, 1);
		DSTBGR (py_1, dst_1, 1);

		_RGB (u8, 2);
		DSTBGR (py_1, dst_1, 2);
		DSTBGR (py_2, dst_2, 2);

		_RGB (u8, 3);
		DSTBGR (py_2, dst_2, 3);
		DSTBGR (py_1, dst_1, 3);

		pu += 4;
		pv += 4;
		py_1 += 8;
		py_2 += 8;
		dst_1 += 24;
		dst_2 += 24;
    } while (--width);
}

/* This is exactly the same code as yuv2rgb_c_32 except for the types of */
/* r, g, b, dst_1, dst_2 */
static void __fastcall yuv2rgb_c_16 (u8 * py_1, u8 * py_2,
			  u8 * pu, u8 * pv,
			  void * _dst_1, void * _dst_2, int width)
{
    int U, V, Y;
    u16 * r, * g, * b;
    u16 * dst_1, * dst_2;

    width >>= 3;
    dst_1 = (u16 *) _dst_1;
    dst_2 = (u16 *) _dst_2;

    do {
		_RGB (u16, 0);
		DST (py_1, dst_1, 0);
		DST (py_2, dst_2, 0);

		_RGB (u16, 1);
		DST (py_2, dst_2, 1);
		DST (py_1, dst_1, 1);

		_RGB (u16, 2);
		DST (py_1, dst_1, 2);
		DST (py_2, dst_2, 2);

		_RGB (u16, 3);
		DST (py_2, dst_2, 3);
		DST (py_1, dst_1, 3);

		pu += 4;
		pv += 4;
		py_1 += 8;
		py_2 += 8;
		dst_1 += 8;
		dst_2 += 8;
    } while (--width);
}

static int div_round (int dividend, int divisor)
{
    if (dividend > 0)
		return (dividend + (divisor>>1)) / divisor;
    else
		return -((-dividend + (divisor>>1)) / divisor);
}

static yuv2rgb_c_internal __fastcall * yuv2rgb_c_init (int order, int bpp)
{
    int i;
    u8 table_Y[1024];
    u32 * table_32 = 0;
    u16 * table_16 = 0;
    u8 * table_8 = 0;
    int entry_size = 0;
    void * table_r = 0;
    void * table_g = 0;
    void * table_b = 0;
    yuv2rgb_c_internal * yuv2rgb;

    int crv = Inverse_Table_6_9[matrix_coefficients][0];
    int cbu = Inverse_Table_6_9[matrix_coefficients][1];
    int cgu = -Inverse_Table_6_9[matrix_coefficients][2];
    int cgv = -Inverse_Table_6_9[matrix_coefficients][3];

    for (i = 0; i < 1024; i++)
	{
		int j;

		j = (76309 * (i - 384 - 16) + 32768) >> 16;
		j = (j < 0) ? 0 : ((j > 255) ? 255 : j);
		table_Y[i] = j;
    }

    switch (bpp)
	{
    case 32:
		yuv2rgb = yuv2rgb_c_32;

		table_32 = (u32 *) malloc ((197 + 2*682 + 256 + 132) *
						sizeof (u32));

		entry_size = sizeof (u32);
		table_r = table_32 + 197;
		table_b = table_32 + 197 + 685;
		table_g = table_32 + 197 + 2*682;

		for (i = -197; i < 256+197; i++)
			((u32 *) table_r)[i] =
			table_Y[i+384] << ((order == CONVERT_RGB) ? 16 : 0);
		for (i = -132; i < 256+132; i++)
			((u32 *) table_g)[i] = table_Y[i+384] << 8;
		for (i = -232; i < 256+232; i++)
			((u32 *) table_b)[i] =
			table_Y[i+384] << ((order == CONVERT_RGB) ? 0 : 16);
	break;

    case 24:
		yuv2rgb = (order == CONVERT_RGB) ? yuv2rgb_c_24_rgb : yuv2rgb_c_24_bgr;

		table_8 = (u8 *) malloc ((256 + 2*232) * sizeof (u8));

		entry_size = sizeof (u8);
		table_r = table_g = table_b = table_8 + 232;

		for (i = -232; i < 256+232; i++)
			((u8 * )table_b)[i] = table_Y[i+384];
	break;

    case 15:
    case 16:
		yuv2rgb = yuv2rgb_c_16;

		table_16 = (u16 *) malloc ((197 + 2*682 + 256 + 132) *
						sizeof (u16));

		entry_size = sizeof (u16);
		table_r = table_16 + 197;
		table_b = table_16 + 197 + 685;
		table_g = table_16 + 197 + 2*682;

		for (i = -197; i < 256+197; i++) {
			int j = table_Y[i+384] >> 3;

			if (order == CONVERT_RGB)
			j <<= ((bpp==16) ? 11 : 10);

			((u16 *)table_r)[i] = j;
		}
		for (i = -132; i < 256+132; i++) {
			int j = table_Y[i+384] >> ((bpp==16) ? 2 : 3);

			((u16 *)table_g)[i] = j << 5;
		}
		for (i = -232; i < 256+232; i++) {
			int j = table_Y[i+384] >> 3;

			if (order == CONVERT_RGB)
			j <<= ((bpp==16) ? 11 : 10);

			((u16 *)table_b)[i] = j;
		}
	break;

#ifdef PCSX2_DEVBUILD
    default:
		DevCon::Error( "IPU Panic!  %ibpp not supported by yuv2rgb", params bpp );
#else
		jNO_DEFAULT
#endif
	}

    for (i = 0; i < 256; i++) {
		table_rV[i] = (((u8 *)table_r) +
			entry_size * div_round (crv * (i-128), 76309));
		table_gU[i] = (((u8 *)table_g) +
			entry_size * div_round (cgu * (i-128), 76309));
		table_gV[i] = entry_size * div_round (cgv * (i-128), 76309);
		table_bU[i] = (((u8 *)table_b) +
			entry_size * div_round (cbu * (i-128), 76309));
    }

    return yuv2rgb;
}

static void __fastcall convert_yuv2rgb_c (void * _id, u8 * Y, u8 * Cr, u8 * Cb,
			       unsigned int v_offset)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;
    u8 * dst;
    u8 * py;
    u8 * pu;
    u8 * pv;
    int loop;

    dst = id->rgb_ptr + id->rgb_stride * v_offset;
    py = Y; pu = Cr; pv = Cb;

    loop = 8;
    do {
		id->yuv2rgb (py, py + (id->uv_stride << 1), pu, pv,
		     dst, dst + id->rgb_stride, id->width);
		py += id->uv_stride << 2;
		pu += id->uv_stride;
		pv += id->uv_stride;
		dst += 2 * id->rgb_stride;
    } while (--loop);
}

static void __fastcall convert_start (void * _id, u8 * dest, int flags)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;
    id->rgb_ptr = dest;
    switch (flags) {
    case CONVERT_BOTTOM_FIELD:
		id->rgb_ptr += id->rgb_stride_frame;
		/* break thru */
    case CONVERT_TOP_FIELD:
		id->uv_stride = id->uv_stride_frame << 1;
		id->rgb_stride = id->rgb_stride_frame << 1;
	break;
    default:
		id->uv_stride = id->uv_stride_frame;
		id->rgb_stride = id->rgb_stride_frame;
    }
}

static void __fastcall convert_internal (int order, int bpp, int width, int height,
			      u32 accel, void * arg, convert_init_t * result)
{
    convert_rgb_t * id = (convert_rgb_t *) result->id;

    if (!id) {
		result->id_size = sizeof (convert_rgb_t);
    } else {
		id->width = width;
		id->uv_stride_frame = width >> 1;
		id->rgb_stride_frame = ((bpp + 7) >> 3) * width;

		result->buf_size[0] = id->rgb_stride_frame * height;
		result->buf_size[1] = result->buf_size[2] = 0;
		result->start = convert_start;

		result->copy = NULL;
	#ifdef ARCH_X86
		if ((result->copy == NULL) && (accel & MPEG2_ACCEL_X86_MMXEXT)) {
			result->copy = yuv2rgb_init_mmxext (order, bpp);
		}
		if ((result->copy == NULL) && (accel & MPEG2_ACCEL_X86_MMX)) {
			result->copy = yuv2rgb_init_mmx (order, bpp);
		}
	#endif
	#ifdef LIBVO_MLIB
		if ((result->copy == NULL) && (accel & MPEG2_ACCEL_MLIB)) {
			result->copy = yuv2rgb_init_mlib (order, bpp);
		}
	#endif
		if (result->copy == NULL) {
			result->copy = convert_yuv2rgb_c;
			id->yuv2rgb = yuv2rgb_c_init (order, bpp);
		}
    }
}

void __fastcall convert_rgb32 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_RGB, 32, width, height, accel, arg, result);
}

void __fastcall convert_rgb24 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_RGB, 24, width, height, accel, arg, result);
}

void __fastcall convert_rgb16 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_RGB, 16, width, height, accel, arg, result);
}

void __fastcall convert_rgb15 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_RGB, 15, width, height, accel, arg, result);
}

void __fastcall convert_bgr32 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_BGR, 32, width, height, accel, arg, result);
}

void __fastcall convert_bgr24 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_BGR, 24, width, height, accel, arg, result);
}

void __fastcall convert_bgr16 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_BGR, 16, width, height, accel, arg, result);
}

void __fastcall convert_bgr15 (int width, int height, u32 accel, void * arg,
		    convert_init_t * result)
{
    convert_internal (CONVERT_BGR, 15, width, height, accel, arg, result);
}

__forceinline convert_t* convert_rgb (int order, int bpp)
{
    if (order == CONVERT_RGB || order == CONVERT_BGR)
	switch (bpp) {
	case 32: return (order == CONVERT_RGB) ? convert_rgb32 : convert_bgr32;
	case 24: return (order == CONVERT_RGB) ? convert_rgb24 : convert_bgr24;
	case 16: return (order == CONVERT_RGB) ? convert_rgb16 : convert_bgr16;
	case 15: return (order == CONVERT_RGB) ? convert_rgb15 : convert_bgr15;
	}
    return NULL;
}
