/*****************************************************************************
** Name:        extended.c
** Purpose:     IEEE Extended<->Double routines to save floats to file
** Maintainer:  Ryan Norton
** Modified by:
** Created:     11/24/04
** RCS-ID:      $Id: extended.c 36952 2006-01-18 10:25:04Z JS $
*****************************************************************************/


#if defined(_WIN32_WCE)
    /* eVC cause warnings in its own headers: stdlib.h and winnt.h */
    #pragma warning (disable:4115)
    #pragma warning (disable:4214)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "wx/defs.h"

#if defined(_WIN32_WCE)
    #pragma warning (default:4115)
    #pragma warning (default:4214)
#endif

#if wxUSE_APPLE_IEEE

#include "wx/math.h"

/* Copyright (C) 1989-1991 Ken Turkowski. <turk@computer.org>
 *
 * All rights reserved.
 *
 * Warranty Information
 *  Even though I have reviewed this software, I make no warranty
 *  or representation, either express or implied, with respect to this
 *  software, its quality, accuracy, merchantability, or fitness for a
 *  particular purpose.  As a result, this software is provided "as is,"
 *  and you, its user, are assuming the entire risk as to its quality
 *  and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the above warranty information.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *    Sequent Balance (Multiprocesor 386)
 *    NeXT
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/


/****************************************************************
 * The following two routines make up for deficiencies in many
 * compilers to convert properly between unsigned integers and
 * floating-point.  Some compilers which have this bug are the
 * THINK_C compiler for the Macintosh and the C compiler for the
 * Silicon Graphics MIPS-based Iris.
 ****************************************************************/

#ifdef applec    /* The Apple C compiler works */
# define FloatToUnsigned(f)   ((wxUint32)(f))
# define UnsignedToFloat(u)   ((wxFloat64)(u))
#else /*applec*/
# define FloatToUnsigned(f)   ((wxUint32)(((wxInt32)((f) - 2147483648.0)) + 2147483647L) + 1)
# define UnsignedToFloat(u)   (((wxFloat64)((wxInt32)((u) - 2147483647L - 1))) + 2147483648.0)
#endif /*applec*/



/****************************************************************
 * Extended precision IEEE floating-point conversion routines.
 * Extended is an 80-bit number as defined by Motorola,
 * with a sign bit, 15 bits of exponent (offset 16383?),
 * and a 64-bit mantissa, with no hidden bit.
 ****************************************************************/

wxFloat64 ConvertFromIeeeExtended(const wxInt8 *bytes)
{
    wxFloat64 f;
    wxInt32 expon;
    wxUint32 hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((wxUint32)(bytes[2] & 0xFF) << 24)
            | ((wxUint32)(bytes[3] & 0xFF) << 16)
            | ((wxUint32)(bytes[4] & 0xFF) << 8)
            | ((wxUint32)(bytes[5] & 0xFF));
    loMant = ((wxUint32)(bytes[6] & 0xFF) << 24)
            | ((wxUint32)(bytes[7] & 0xFF) << 16)
            | ((wxUint32)(bytes[8] & 0xFF) << 8)
            | ((wxUint32)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) { /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
            f += ldexp(UnsignedToFloat(loMant), expon-=32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}


/****************************************************************/


void ConvertToIeeeExtended(wxFloat64 num, wxInt8 *bytes)
{
    wxInt32 sign;
    wxInt32 expon;
    wxFloat64 fMant, fsMant;
    wxUint32 hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) { /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else { /* Finite */
            expon += 16382;
            if (expon < 0) { /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);          fsMant = floor(fMant); hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32); fsMant = floor(fMant); loMant = FloatToUnsigned(fsMant);
        }
    }

    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}



#endif /* wxUSE_APPLE_IEEE */
