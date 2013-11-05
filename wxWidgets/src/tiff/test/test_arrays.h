
/*
 * Copyright (c) 2004, Andrey Kiselev  <dron@ak4719.spb.edu>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Few declarations for the test numerical arrays.
 */

#ifndef _TEST_ARRAYS_ 
#define _TEST_ARRAYS_

#include <stddef.h>

#define XSIZE 37
#define YSIZE 23

extern const unsigned char byte_array1[];
extern const size_t byte_array1_size;

extern const unsigned char byte_array2[];
extern const size_t byte_array2_size;

extern const unsigned char byte_array3[];
extern const size_t byte_array3_size;

extern const float array_float1[];
extern const size_t array_float1_size;

extern const float array_float2[];
extern const size_t array_float2_size;

extern const double array_double1[];
extern const size_t array_double1_size;

extern const double array_double2[];
extern const size_t array_double2_size;

#endif /* _TEST_ARRAYS_ */

/* vim: set ts=8 sts=8 sw=8 noet: */
