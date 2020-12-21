// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "utils.h"
#include <string>

void bn_print(char *name, u8 *a, u32 n)
{
	u32 i;

	printf("%s = ", name);

	for (i = 0; i < n; i++)
		printf("%02x", a[i]);

	printf("\n");
}

static void bn_zero(u8 *d, u32 n)
{
	memset(d, 0, n);
}
void bn_copy(u8 *d, u8 *a, u32 n)
{
	memcpy(d, a, n);
}

int bn_compare(u8 *a, u8 *b, u32 n)
{
	u32 i;

	for (i = 0; i < n; i++) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}

	return 0;
}

static u8 bn_add_1(u8 *d, u8 *a, u8 *b, u32 n)
{
	u32 i;
	u32 dig;
	u8 c;

	c = 0;
	for (i = n - 1; i < n; i--) {
		dig = a[i] + b[i] + c;
		c = dig >> 8;
		d[i] = dig;
	}

	return c;
}

static u8 bn_sub_1(u8 *d, u8 *a, u8 *b, u32 n)
{
	u32 i;
	u32 dig;
	u8 c;

	c = 1;
	for (i = n - 1; i < n; i--) {
		dig = a[i] + 255 - b[i] + c;
		c = dig >> 8;
		d[i] = dig;
	}

	return 1 - c;
}

void bn_reduce(u8 *d, u8 *N, u32 n)
{
	if (bn_compare(d, N, n) >= 0)
		bn_sub_1(d, d, N, n);
}

void bn_add(u8 *d, u8 *a, u8 *b, u8 *N, u32 n)
{
	if (bn_add_1(d, a, b, n))
		bn_sub_1(d, d, N, n);

	bn_reduce(d, N, n);
}

void bn_sub(u8 *d, u8 *a, u8 *b, u8 *N, u32 n)
{
	if (bn_sub_1(d, a, b, n))
		bn_add_1(d, d, N, n);
}

static const u8 inv256[0x80] = {
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

static void bn_mon_muladd_dig(u8 *d, u8 *a, u8 b, u8 *N, u32 n)
{
	u32 dig;
	u32 i;

	u8 z = -(d[n-1] + a[n-1]*b) * inv256[N[n-1]/2];

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

void bn_mon_mul(u8 *d, u8 *a, u8 *b, u8 *N, u32 n)
{
	u8 t[512];
	u32 i;

	bn_zero(t, n);

	for (i = n - 1; i < n; i--)
		bn_mon_muladd_dig(t, a, b[i], N, n);

	bn_copy(d, t, n);
}

void bn_to_mon(u8 *d, u8 *N, u32 n)
{
	u32 i;

	for (i = 0; i < 8*n; i++)
		bn_add(d, d, d, N, n);
}

void bn_from_mon(u8 *d, u8 *N, u32 n)
{
	u8 t[512];

	bn_zero(t, n);
	t[n-1] = 1;
	bn_mon_mul(d, d, t, N, n);
}

static void bn_mon_exp(u8 *d, u8 *a, u8 *N, u32 n, u8 *e, u32 en)
{
	u8 t[512];
	u32 i;
	u8 mask;

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

void bn_mon_inv(u8 *d, u8 *a, u8 *N, u32 n)
{
	u8 t[512], s[512];

	bn_zero(s, n);
	s[n-1] = 2;
	bn_sub_1(t, N, s, n);
	bn_mon_exp(d, a, N, n, t, n);
}

void bn_copy(u8 *d, u8 *a, u32 n);
int bn_compare(u8 *a, u8 *b, u32 n);
void bn_reduce(u8 *d, u8 *N, u32 n);
void bn_add(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_sub(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_to_mon(u8 *d, u8 *N, u32 n);
void bn_from_mon(u8 *d, u8 *N, u32 n);
void bn_mon_mul(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_mon_inv(u8 *d, u8 *a, u8 *N, u32 n);

struct point {
	u8 x[20];
	u8 y[20];
};

static u8 ec_p[20];
static u8 ec_a[20];	// mon
static u8 ec_b[20];	// mon
static u8 ec_N[21];
static struct point ec_G;	// mon
static struct point ec_Q;	// mon
static u8 ec_k[21];

static void elt_copy(u8 *d, u8 *a)
{
	memcpy(d, a, 20);
}

static void elt_zero(u8 *d)
{
	memset(d, 0, 20);
}

static int elt_is_zero(u8 *d)
{
	u32 i;

	for (i = 0; i < 20; i++)
		if (d[i] != 0)
			return 0;

	return 1;
}

static void elt_add(u8 *d, u8 *a, u8 *b)
{
	bn_add(d, a, b, ec_p, 20);
}

static void elt_sub(u8 *d, u8 *a, u8 *b)
{
	bn_sub(d, a, b, ec_p, 20);
}

static void elt_mul(u8 *d, u8 *a, u8 *b)
{
	bn_mon_mul(d, a, b, ec_p, 20);
}

static void elt_square(u8 *d, u8 *a)
{
	elt_mul(d, a, a);
}

static void elt_inv(u8 *d, u8 *a)
{
	u8 s[20];
	elt_copy(s, a);
	bn_mon_inv(d, s, ec_p, 20);
}

static void point_to_mon(struct point *p)
{
	bn_to_mon(p->x, ec_p, 20);
	bn_to_mon(p->y, ec_p, 20);
}

static void point_from_mon(struct point *p)
{
	bn_from_mon(p->x, ec_p, 20);
	bn_from_mon(p->y, ec_p, 20);
}

#if 0
static int point_is_on_curve(u8 *p)
{
	u8 s[20], t[20];
	u8 *x, *y;

	x = p;
	y = p + 20;

	elt_square(t, x);
	elt_mul(s, t, x);

	elt_mul(t, x, ec_a);
	elt_add(s, s, t);

	elt_add(s, s, ec_b);

	elt_square(t, y);
	elt_sub(s, s, t);

	return elt_is_zero(s);
}
#endif

static void point_zero(struct point *p)
{
	elt_zero(p->x);
	elt_zero(p->y);
}

static int point_is_zero(struct point *p)
{
	return elt_is_zero(p->x) && elt_is_zero(p->y);
}

static void point_double(struct point *r, struct point *p)
{
	u8 s[20], t[20];
	struct point pp;
	u8 *px, *py, *rx, *ry;

	pp = *p;

	px = pp.x;
	py = pp.y;
	rx = r->x;
	ry = r->y;

	if (elt_is_zero(py)) {
		point_zero(r);
		return;
	}

	elt_square(t, px);	// t = px*px
	elt_add(s, t, t);	// s = 2*px*px
	elt_add(s, s, t);	// s = 3*px*px
	elt_add(s, s, ec_a);	// s = 3*px*px + a
	elt_add(t, py, py);	// t = 2*py
	elt_inv(t, t);		// t = 1/(2*py)
	elt_mul(s, s, t);	// s = (3*px*px+a)/(2*py)

	elt_square(rx, s);	// rx = s*s
	elt_add(t, px, px);	// t = 2*px
	elt_sub(rx, rx, t);	// rx = s*s - 2*px

	elt_sub(t, px, rx);	// t = -(rx-px)
	elt_mul(ry, s, t);	// ry = -s*(rx-px)
	elt_sub(ry, ry, py);	// ry = -s*(rx-px) - py
}

static void point_add(struct point *r, struct point *p, struct point *q)
{
	u8 s[20], t[20], u[20];
	u8 *px, *py, *qx, *qy, *rx, *ry;
	struct point pp, qq;

	pp = *p;
	qq = *q;

	px = pp.x;
	py = pp.y;
	qx = qq.x;
	qy = qq.y;
	rx = r->x;
	ry = r->y;

	if (point_is_zero(&pp)) {
		elt_copy(rx, qx);
		elt_copy(ry, qy);
		return;
	}

	if (point_is_zero(&qq)) {
		elt_copy(rx, px);
		elt_copy(ry, py);
		return;
	}

	elt_sub(u, qx, px);

	if (elt_is_zero(u)) {
		elt_sub(u, qy, py);
		if (elt_is_zero(u))
			point_double(r, &pp);
		else
			point_zero(r);

		return;
	}

	elt_inv(t, u);		// t = 1/(qx-px)
	elt_sub(u, qy, py);	// u = qy-py
	elt_mul(s, t, u);	// s = (qy-py)/(qx-px)

	elt_square(rx, s);	// rx = s*s
	elt_add(t, px, qx);	// t = px+qx
	elt_sub(rx, rx, t);	// rx = s*s - (px+qx)

	elt_sub(t, px, rx);	// t = -(rx-px)
	elt_mul(ry, s, t);	// ry = -s*(rx-px)
	elt_sub(ry, ry, py);	// ry = -s*(rx-px) - py
}

static void point_mul(struct point *d, u8 *a, struct point *b)	// a is bignum
{
	u32 i;
	u8 mask;

	point_zero(d);

	for (i = 0; i < 21; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			point_double(d, d);
			if ((a[i] & mask) != 0)
				point_add(d, d, b);
		}
}

static int check_ecdsa(struct point *Q, u8 *R, u8 *S, u8 *hash)
{
	u8 Sinv[21];
	u8 e[21];
	u8 w1[21], w2[21];
	struct point r1, r2;
	u8 rr[21];

	e[0] = 0;
	memcpy(e + 1, hash, 20);
	bn_reduce(e, ec_N, 21);

	bn_to_mon(R, ec_N, 21);
	bn_to_mon(S, ec_N, 21);
	bn_to_mon(e, ec_N, 21);

	bn_mon_inv(Sinv, S, ec_N, 21);

	bn_mon_mul(w1, e, Sinv, ec_N, 21);
	bn_mon_mul(w2, R, Sinv, ec_N, 21);

	bn_from_mon(w1, ec_N, 21);
	bn_from_mon(w2, ec_N, 21);

	point_mul(&r1, w1, &ec_G);
	point_mul(&r2, w2, Q);

	point_add(&r1, &r1, &r2);

	point_from_mon(&r1);

	rr[0] = 0;
	memcpy(rr + 1, r1.x, 20);
	bn_reduce(rr, ec_N, 21);

	bn_from_mon(R, ec_N, 21);
	bn_from_mon(S, ec_N, 21);

	return (bn_compare(rr, R, 21) == 0);
}

#if 0
static void ec_priv_to_pub(u8 *k, u8 *Q)
{
	point_mul(Q, k, ec_G);
}
#endif

int ecdsa_set_curve(const u8* p, const u8* a, const u8* b, const u8* N, const u8* Gx, const u8* Gy)
{
	memcpy(ec_p, p, 20);
	memcpy(ec_a, a, 20);
	memcpy(ec_b, b, 20);
	memcpy(ec_N, N, 21);
	memcpy(ec_G.x, Gx, 20);
	memcpy(ec_G.y, Gy, 20);

	bn_to_mon(ec_a, ec_p, 20);
	bn_to_mon(ec_b, ec_p, 20);

	point_to_mon(&ec_G);

	return 0;
}

void ecdsa_set_pub(const u8* Q)
{
	memcpy(ec_Q.x, Q, 20);
	memcpy(ec_Q.y, Q+20, 20);
	point_to_mon(&ec_Q);
}

void ecdsa_set_priv(const u8* k)
{
	memcpy(ec_k, k, sizeof ec_k);
}

int ecdsa_verify(u8 *hash, u8 *R, u8 *S)
{
	return check_ecdsa(&ec_Q, R, S, hash);
}
