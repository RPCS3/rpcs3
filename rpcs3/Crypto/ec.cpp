// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "util/types.hpp"
#include <cstring>

static inline int bn_compare(const u8* a, const u8* b, u32 n)
{
	for (u32 i = 0; i < n; i++)
	{
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}

	return 0;
}

static u8 bn_add_1(u8* d, const u8* a, const u8* b, u32 n)
{
	u8 c = 0;
	for (u32 i = n - 1; i != umax; i--)
	{
		const u32 dig = a[i] + b[i] + c;
		c    = dig >> 8;
		d[i] = dig;
	}

	return c;
}

static u8 bn_sub_1(u8* d, const u8* a, const u8* b, u32 n)
{
	u8 c = 1;
	for (u32 i = n - 1; i != umax; i--)
	{
		const u32 dig = a[i] + 255 - b[i] + c;
		c    = dig >> 8;
		d[i] = dig;
	}

	return 1 - c;
}

static void bn_reduce(u8* d, const u8* N, u32 n)
{
	if (bn_compare(d, N, n) >= 0)
		bn_sub_1(d, d, N, n);
}

static void bn_add(u8* d, const u8* a, const u8* b, const u8* N, u32 n)
{
	if (bn_add_1(d, a, b, n))
		bn_sub_1(d, d, N, n);

	bn_reduce(d, N, n);
}

static void bn_sub(u8* d, const u8* a, const u8* b, const u8* N, u32 n)
{
	if (bn_sub_1(d, a, b, n))
		bn_add_1(d, d, N, n);
}

static constexpr u8 inv256[0x80] = {
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

static void bn_mon_muladd_dig(u8* d, const u8* a, u8 b, const u8* N, u32 n)
{
	const u8 z = -(d[n - 1] + a[n - 1] * b) * inv256[N[n - 1] / 2];

	u32 dig = d[n - 1] + a[n - 1] * b + N[n - 1] * z;
	dig >>= 8;

	for (u32 i = n - 2; i < n; i--)
	{
		dig += d[i] + a[i] * b + N[i] * z;
		d[i + 1] = dig;
		dig >>= 8;
	}

	d[0] = dig;
	dig >>= 8;

	if (dig)
		bn_sub_1(d, d, N, n);

	bn_reduce(d, N, n);
}

static void bn_mon_mul(u8* d, const u8* a, const u8* b, const u8* N, u32 n)
{
	u8 t[512];
	memset(t, 0, n);

	for (u32 i = n - 1; i != umax; i--)
		bn_mon_muladd_dig(t, a, b[i], N, n);

	memcpy(d, t, n);
}

static void bn_to_mon(u8* d, const u8* N, u32 n)
{
	for (u32 i = 0; i < 8 * n; i++)
		bn_add(d, d, d, N, n);
}

static void bn_from_mon(u8* d, const u8* N, u32 n)
{
	u8 t[512];

	memset(t, 0, n);
	t[n - 1] = 1;
	bn_mon_mul(d, d, t, N, n);
}

static void bn_mon_exp(u8* d, const u8* a, const u8* N, u32 n, const u8* e, u32 en)
{
	u8 t[512];

	memset(d, 0, n);
	d[n - 1] = 1;
	bn_to_mon(d, N, n);

	for (u32 i = 0; i < en; i++)
	{
		for (u8 mask = 0x80; mask != 0; mask >>= 1)
		{
			bn_mon_mul(t, d, d, N, n);
			if ((e[i] & mask) != 0)
				bn_mon_mul(d, t, a, N, n);
			else
				memcpy(d, t, n);
		}
	}
}

static void bn_mon_inv(u8* d, const u8* a, const u8* N, u32 n)
{
	u8 t[512], s[512];

	memset(s, 0, n);
	s[n - 1] = 2;
	bn_sub_1(t, N, s, n);
	bn_mon_exp(d, a, N, n, t, n);
}

struct point
{
	u8 x[20];
	u8 y[20];
};

static thread_local u8 ec_p[20]{};
static thread_local u8 ec_a[20]{}; // mon
static thread_local u8 ec_b[20]{}; // mon
static thread_local u8 ec_N[21]{};
static thread_local point ec_G{}; // mon
static thread_local point ec_Q{}; // mon
static thread_local u8 ec_k[21]{};
static thread_local bool ec_curve_initialized{};
static thread_local bool ec_pub_initialized{};

static inline bool elt_is_zero(const u8* d)
{
	for (u32 i = 0; i < 20; i++)
		if (d[i] != 0)
			return false;

	return true;
}

static void elt_add(u8* d, const u8* a, const u8* b)
{
	bn_add(d, a, b, ec_p, 20);
}

static void elt_sub(u8* d, const u8* a, const u8* b)
{
	bn_sub(d, a, b, ec_p, 20);
}

static void elt_mul(u8* d, const u8* a, const u8* b)
{
	bn_mon_mul(d, a, b, ec_p, 20);
}

static void elt_square(u8* d, const u8* a)
{
	elt_mul(d, a, a);
}

static void elt_inv(u8* d, const u8* a)
{
	u8 s[20];
	memcpy(s, a, 20);
	bn_mon_inv(d, s, ec_p, 20);
}

static void point_to_mon(point* p)
{
	bn_to_mon(p->x, ec_p, 20);
	bn_to_mon(p->y, ec_p, 20);
}

static void point_from_mon(point* p)
{
	bn_from_mon(p->x, ec_p, 20);
	bn_from_mon(p->y, ec_p, 20);
}

static inline void point_zero(point* p)
{
	memset(p->x, 0, 20);
	memset(p->y, 0, 20);
}

static inline bool point_is_zero(const point* p)
{
	return elt_is_zero(p->x) && elt_is_zero(p->y);
}

static void point_double(point* r, const point* p)
{
	u8 s[20], t[20];

	point pp = *p;

	const u8* px = pp.x;
	const u8* py = pp.y;
	u8* rx = r->x;
	u8* ry = r->y;

	if (elt_is_zero(py))
	{
		point_zero(r);
		return;
	}

	elt_square(t, px);   // t = px*px
	elt_add(s, t, t);    // s = 2*px*px
	elt_add(s, s, t);    // s = 3*px*px
	elt_add(s, s, ec_a); // s = 3*px*px + a
	elt_add(t, py, py);  // t = 2*py
	elt_inv(t, t);       // t = 1/(2*py)
	elt_mul(s, s, t);    // s = (3*px*px+a)/(2*py)

	elt_square(rx, s);  // rx = s*s
	elt_add(t, px, px); // t = 2*px
	elt_sub(rx, rx, t); // rx = s*s - 2*px

	elt_sub(t, px, rx);  // t = -(rx-px)
	elt_mul(ry, s, t);   // ry = -s*(rx-px)
	elt_sub(ry, ry, py); // ry = -s*(rx-px) - py
}

static void point_add(point* r, const point* p, const point* q)
{
	u8 s[20], t[20], u[20];

	point pp = *p;
	point qq = *q;

	const u8* px = pp.x;
	const u8* py = pp.y;
	const u8* qx = qq.x;
	const u8* qy = qq.y;
	u8* rx = r->x;
	u8* ry = r->y;

	if (point_is_zero(&pp))
	{
		memcpy(rx, qx, 20);
		memcpy(ry, qy, 20);
		return;
	}

	if (point_is_zero(&qq))
	{
		memcpy(rx, px, 20);
		memcpy(ry, py, 20);
		return;
	}

	elt_sub(u, qx, px);

	if (elt_is_zero(u))
	{
		elt_sub(u, qy, py);
		if (elt_is_zero(u))
			point_double(r, &pp);
		else
			point_zero(r);

		return;
	}

	elt_inv(t, u);      // t = 1/(qx-px)
	elt_sub(u, qy, py); // u = qy-py
	elt_mul(s, t, u);   // s = (qy-py)/(qx-px)

	elt_square(rx, s);  // rx = s*s
	elt_add(t, px, qx); // t = px+qx
	elt_sub(rx, rx, t); // rx = s*s - (px+qx)

	elt_sub(t, px, rx);  // t = -(rx-px)
	elt_mul(ry, s, t);   // ry = -s*(rx-px)
	elt_sub(ry, ry, py); // ry = -s*(rx-px) - py
}

static void point_mul(point* d, const u8* a, const point* b) // a is bignum
{
	point_zero(d);

	for (u32 i = 0; i < 21; i++)
	{
		for (u8 mask = 0x80; mask != 0; mask >>= 1)
		{
			point_double(d, d);
			if ((a[i] & mask) != 0)
				point_add(d, d, b);
		}
	}
}

static bool check_ecdsa(const struct point* Q, u8* R, u8* S, const u8* hash)
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

void ecdsa_set_curve(const u8* p, const u8* a, const u8* b, const u8* N, const u8* Gx, const u8* Gy)
{
	if (ec_curve_initialized) return;

	memcpy(ec_p, p, 20);
	memcpy(ec_a, a, 20);
	memcpy(ec_b, b, 20);
	memcpy(ec_N, N, 21);
	memcpy(ec_G.x, Gx, 20);
	memcpy(ec_G.y, Gy, 20);

	bn_to_mon(ec_a, ec_p, 20);
	bn_to_mon(ec_b, ec_p, 20);

	point_to_mon(&ec_G);

	ec_curve_initialized = true;
}

void ecdsa_set_pub(const u8* Q)
{
	if (ec_pub_initialized) return;

	memcpy(ec_Q.x, Q, 20);
	memcpy(ec_Q.y, Q + 20, 20);
	point_to_mon(&ec_Q);

	ec_pub_initialized = true;
}

void ecdsa_set_priv(const u8* k)
{
	memcpy(ec_k, k, sizeof ec_k);
}

bool ecdsa_verify(const u8* hash, u8* R, u8* S)
{
	return check_ecdsa(&ec_Q, R, S, hash);
}
