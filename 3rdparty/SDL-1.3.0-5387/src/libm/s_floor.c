/* @(#)s_floor.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static const char rcsid[] =
    "$NetBSD: s_floor.c,v 1.8 1995/05/10 20:47:20 jtc Exp $";
#endif

/*
 * floor(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to floor(x).
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
static const double huge_val = 1.0e300;
#else
static double huge_val = 1.0e300;
#endif

libm_hidden_proto(floor)
#ifdef __STDC__
     double floor(double x)
#else
     double floor(x)
     double x;
#endif
{
    int32_t i0, i1, j0;
    u_int32_t i, j;
    EXTRACT_WORDS(i0, i1, x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if (j0 < 20) {
        if (j0 < 0) {           /* raise inexact if x != 0 */
            if (huge_val + x > 0.0) {       /* return 0*sign(x) if |x|<1 */
                if (i0 >= 0) {
                    i0 = i1 = 0;
                } else if (((i0 & 0x7fffffff) | i1) != 0) {
                    i0 = 0xbff00000;
                    i1 = 0;
                }
            }
        } else {
            i = (0x000fffff) >> j0;
            if (((i0 & i) | i1) == 0)
                return x;       /* x is integral */
            if (huge_val + x > 0.0) {       /* raise inexact flag */
                if (i0 < 0)
                    i0 += (0x00100000) >> j0;
                i0 &= (~i);
                i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400)
            return x + x;       /* inf or NaN */
        else
            return x;           /* x is integral */
    } else {
        i = ((u_int32_t) (0xffffffff)) >> (j0 - 20);
        if ((i1 & i) == 0)
            return x;           /* x is integral */
        if (huge_val + x > 0.0) {   /* raise inexact flag */
            if (i0 < 0) {
                if (j0 == 20)
                    i0 += 1;
                else {
                    j = i1 + (1 << (52 - j0));
                    if (j < (u_int32_t) i1)
                        i0 += 1;        /* got a carry */
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    INSERT_WORDS(x, i0, i1);
    return x;
}

libm_hidden_def(floor)
