/* @(#)s_fabs.c 5.1 93/09/24 */
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
    "$NetBSD: s_fabs.c,v 1.7 1995/05/10 20:47:13 jtc Exp $";
#endif

/*
 * fabs(x) returns the absolute value of x.
 */

#include "math.h"
#include "math_private.h"

libm_hidden_proto(fabs)
#ifdef __STDC__
     double fabs(double x)
#else
     double fabs(x)
     double x;
#endif
{
    u_int32_t high;
    GET_HIGH_WORD(high, x);
    SET_HIGH_WORD(x, high & 0x7fffffff);
    return x;
}

libm_hidden_def(fabs)
