// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <string.h>
#include <stdio.h>

#include "types.h"

void bn_print(char *name, scetool::u8 *a, scetool::u32 n)
{
	scetool::u32 i;

	printf("%s = ", name);

	for (i = 0; i < n; i++)
		printf("%02x", a[i]);

	printf("\n");
}

static void bn_zero(scetool::u8 *d, scetool::u32 n)
{
	memset(d, 0, n);
}

void bn_copy(scetool::u8 *d, scetool::u8 *a, scetool::u32 n)
{
	memcpy(d, a, n);
}

int bn_compare(scetool::u8 *a, scetool::u8 *b, scetool::u32 n)
{
	scetool::u32 i;

	for (i = 0; i < n; i++) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}

	return 0;
}

static scetool::u8 bn_add_1(scetool::u8 *d, scetool::u8 *a, scetool::u8 *b, scetool::u32 n)
{
	scetool::u32 i;
	scetool::u32 dig;
	scetool::u8 c;

	c = 0;
	for (i = n - 1; i < n; i--) {
		dig = a[i] + b[i] + c;
		c = dig >> 8;
		d[i] = dig;
	}

	return c;
}

static scetool::u8 bn_sub_1(scetool::u8 *d, scetool::u8 *a, scetool::u8 *b, scetool::u32 n)
{
	scetool::u32 i;
	scetool::u32 dig;
	scetool::u8 c;

	c = 1;
	for (i = n - 1; i < n; i--) {
		dig = a[i] + 255 - b[i] + c;
		c = dig >> 8;
		d[i] = dig;
	}

	return 1 - c;
}

void bn_reduce(scetool::u8 *d, scetool::u8 *N, scetool::u32 n)
{
	if (bn_compare(d, N, n) >= 0)
		bn_sub_1(d, d, N, n);
}

void bn_add(scetool::u8 *d, scetool::u8 *a, scetool::u8 *b, scetool::u8 *N, scetool::u32 n)
{
	if (bn_add_1(d, a, b, n))
		bn_sub_1(d, d, N, n);

	bn_reduce(d, N, n);
}

void bn_sub(scetool::u8 *d, scetool::u8 *a, scetool::u8 *b, scetool::u8 *N, scetool::u32 n)
{
	if (bn_sub_1(d, a, b, n))
		bn_add_1(d, d, N, n);
}

static const scetool::u8 inv256[0x80] = {
	0x01, 0xab, 0xcd, 0xb7, 0x39, 0xa3, 0xc5, 0xef,
	0xf1, 0x1b, 0x3d, 0xa7, 0x29, 0x13, 0x35, 0xdf,
	0xe1, 0x8b, 0xad, 0x97, 0x19, 0x83, 0xa5, 0xcf,
	0xd1, 0xfb, 0x1d, 0x87, 0x09, 0xf3, 0x15, 0xbf,
	0xc1, 0x6b, 0x8d, 0x77, 0xf9, 0x63, 0x85, 0xaf,
	0xb1, 0xdb, 0xfd, 0x67, 0xe9, 0xd3, 0xf5, 0x9f,
	0xa1, 0x4b, 0x6d, 0x57, 0xd9, 0x43, 0x65, 0x8f,
	0x91, 0xbb, 0xdd, 0x47, 0xc9, 0xb3, 0xd5, 0x7f,
	0x81, 0x2b, 0x4d, 0x37, 0xb9, 0x23, 0x45, 0x6f,
	0x71, 0x9b, 0xbd, 0x27, 0xa9, 0x93, 0xb5, 0x5f,
	0x61, 0x0b, 0x2d, 0x17, 0x99, 0x03, 0x25, 0x4f,
	0x51, 0x7b, 0x9d, 0x07, 0x89, 0x73, 0x95, 0x3f,
	0x41, 0xeb, 0x0d, 0xf7, 0x79, 0xe3, 0x05, 0x2f,
	0x31, 0x5b, 0x7d, 0xe7, 0x69, 0x53, 0x75, 0x1f,
	0x21, 0xcb, 0xed, 0xd7, 0x59, 0xc3, 0xe5, 0x0f,
	0x11, 0x3b, 0x5d, 0xc7, 0x49, 0x33, 0x55, 0xff,
};

static void bn_mon_muladd_dig(scetool::u8 *d, scetool::u8 *a, scetool::u8 b, scetool::u8 *N, scetool::u32 n)
{
	scetool::u32 dig;
	scetool::u32 i;

	scetool::u8 z = -(d[n-1] + a[n-1]*b) * inv256[N[n-1]/2];

	dig = d[n-1] + a[n-1]*b + N[n-1]*z;
	dig >>= 8;

	for (i = n - 2; i < n; i--) {
		dig += d[i] + a[i]*b + N[i]*z;
		d[i+1] = dig;
		dig >>= 8;
	}

	d[0] = dig;
	dig >>= 8;

	if (dig)
		bn_sub_1(d, d, N, n);

	bn_reduce(d, N, n);
}

void bn_mon_mul(scetool::u8 *d, scetool::u8 *a, scetool::u8 *b, scetool::u8 *N, scetool::u32 n)
{
	scetool::u8 t[512];
	scetool::u32 i;

	bn_zero(t, n);

	for (i = n - 1; i < n; i--)
		bn_mon_muladd_dig(t, a, b[i], N, n);

	bn_copy(d, t, n);
}

void bn_to_mon(scetool::u8 *d, scetool::u8 *N, scetool::u32 n)
{
	scetool::u32 i;

	for (i = 0; i < 8*n; i++)
		bn_add(d, d, d, N, n);
}

void bn_from_mon(scetool::u8 *d, scetool::u8 *N, scetool::u32 n)
{
	scetool::u8 t[512];

	bn_zero(t, n);
	t[n-1] = 1;
	bn_mon_mul(d, d, t, N, n);
}

static void bn_mon_exp(scetool::u8 *d, scetool::u8 *a, scetool::u8 *N, scetool::u32 n, scetool::u8 *e, scetool::u32 en)
{
	scetool::u8 t[512];
	scetool::u32 i;
	scetool::u8 mask;

	bn_zero(d, n);
	d[n-1] = 1;
	bn_to_mon(d, N, n);

	for (i = 0; i < en; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			bn_mon_mul(t, d, d, N, n);
			if ((e[i] & mask) != 0)
				bn_mon_mul(d, t, a, N, n);
			else
				bn_copy(d, t, n);
		}
}

void bn_mon_inv(scetool::u8 *d, scetool::u8 *a, scetool::u8 *N, scetool::u32 n)
{
	scetool::u8 t[512], s[512];

	bn_zero(s, n);
	s[n-1] = 2;
	bn_sub_1(t, N, s, n);
	bn_mon_exp(d, a, N, n, t, n);
}
