/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/quantize.cpp
// Purpose:     wxQuantize implementation
// Author:      Julian Smart
// Modified by:
// Created:     22/6/2000
// RCS-ID:      $Id: quantize.cpp 39957 2006-07-03 19:02:54Z ABX $
// Copyright:   (c) Thomas G. Lane, Vaclav Slavik, Julian Smart
// Licence:     wxWindows licence + JPEG library licence
/////////////////////////////////////////////////////////////////////////////

/*
 * jquant2.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains 2-pass color quantization (color mapping) routines.
 * These routines provide selection of a custom color map for an image,
 * followed by mapping of the image to that color map, with optional
 * Floyd-Steinberg dithering.
 * It is also possible to use just the second pass to map to an arbitrary
 * externally-given color map.
 *
 * Note: ordered dithering is not supported, since there isn't any fast
 * way to compute intercolor distances; it's unclear that ordered dither's
 * fundamental assumptions even hold with an irregularly spaced color map.
 */

/* modified by Vaclav Slavik for use as jpeglib-independent module */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_IMAGE

#include "wx/quantize.h"

#ifndef WX_PRECOMP
    #include "wx/palette.h"
    #include "wx/image.h"
#endif

#ifdef __WXMSW__
    #include "wx/msw/private.h"
#endif

#include <stdlib.h>
#include <string.h>

#if defined(__OS2__)
#define RGB_RED_OS2   0
#define RGB_GREEN_OS2 1
#define RGB_BLUE_OS2  2
#else
#define RGB_RED       0
#define RGB_GREEN     1
#define RGB_BLUE      2
#endif
#define RGB_PIXELSIZE 3

#define MAXJSAMPLE        255
#define CENTERJSAMPLE     128
#define BITS_IN_JSAMPLE   8
#define GETJSAMPLE(value) ((int) (value))

#define RIGHT_SHIFT(x,shft) ((x) >> (shft))

typedef unsigned short UINT16;
typedef signed short INT16;
#if !(defined(__WATCOMC__) && (defined(__WXMSW__) || defined(__WXMOTIF__)))
typedef signed int INT32;
#endif

typedef unsigned char JSAMPLE;
typedef JSAMPLE *JSAMPROW;
typedef JSAMPROW *JSAMPARRAY;
typedef unsigned int JDIMENSION;

typedef struct {
        void *cquantize;
        JDIMENSION output_width;
        JSAMPARRAY colormap;
        int actual_number_of_colors;
        int desired_number_of_colors;
        JSAMPLE *sample_range_limit, *srl_orig;
} j_decompress;

#if defined(__WINDOWS__) && !defined(__WXMICROWIN__)
    #define JMETHOD(type,methodname,arglist)  type (__cdecl methodname) arglist
#else
    #define JMETHOD(type,methodname,arglist)  type (methodname) arglist
#endif

typedef j_decompress *j_decompress_ptr;
struct jpeg_color_quantizer {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo, bool is_pre_scan));
  JMETHOD(void, color_quantize, (j_decompress_ptr cinfo,
                 JSAMPARRAY input_buf, JSAMPARRAY output_buf,
                 int num_rows));
  JMETHOD(void, finish_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, new_color_map, (j_decompress_ptr cinfo));
};




/*
 * This module implements the well-known Heckbert paradigm for color
 * quantization.  Most of the ideas used here can be traced back to
 * Heckbert's seminal paper
 *   Heckbert, Paul.  "Color Image Quantization for Frame Buffer Display",
 *   Proc. SIGGRAPH '82, Computer Graphics v.16 #3 (July 1982), pp 297-304.
 *
 * In the first pass over the image, we accumulate a histogram showing the
 * usage count of each possible color.  To keep the histogram to a reasonable
 * size, we reduce the precision of the input; typical practice is to retain
 * 5 or 6 bits per color, so that 8 or 4 different input values are counted
 * in the same histogram cell.
 *
 * Next, the color-selection step begins with a box representing the whole
 * color space, and repeatedly splits the "largest" remaining box until we
 * have as many boxes as desired colors.  Then the mean color in each
 * remaining box becomes one of the possible output colors.
 *
 * The second pass over the image maps each input pixel to the closest output
 * color (optionally after applying a Floyd-Steinberg dithering correction).
 * This mapping is logically trivial, but making it go fast enough requires
 * considerable care.
 *
 * Heckbert-style quantizers vary a good deal in their policies for choosing
 * the "largest" box and deciding where to cut it.  The particular policies
 * used here have proved out well in experimental comparisons, but better ones
 * may yet be found.
 *
 * In earlier versions of the IJG code, this module quantized in YCbCr color
 * space, processing the raw upsampled data without a color conversion step.
 * This allowed the color conversion math to be done only once per colormap
 * entry, not once per pixel.  However, that optimization precluded other
 * useful optimizations (such as merging color conversion with upsampling)
 * and it also interfered with desired capabilities such as quantizing to an
 * externally-supplied colormap.  We have therefore abandoned that approach.
 * The present code works in the post-conversion color space, typically RGB.
 *
 * To improve the visual quality of the results, we actually work in scaled
 * RGB space, giving G distances more weight than R, and R in turn more than
 * B.  To do everything in integer math, we must use integer scale factors.
 * The 2/3/1 scale factors used here correspond loosely to the relative
 * weights of the colors in the NTSC grayscale equation.
 * If you want to use this code to quantize a non-RGB color space, you'll
 * probably need to change these scale factors.
 */

#define R_SCALE 2       /* scale R distances by this much */
#define G_SCALE 3       /* scale G distances by this much */
#define B_SCALE 1       /* and B by this much */

/* Relabel R/G/B as components 0/1/2, respecting the RGB ordering defined
 * in jmorecfg.h.  As the code stands, it will do the right thing for R,G,B
 * and B,G,R orders.  If you define some other weird order in jmorecfg.h,
 * you'll get compile errors until you extend this logic.  In that case
 * you'll probably want to tweak the histogram sizes too.
 */

#if defined(__OS2__)

#if RGB_RED_OS2 == 0
#define C0_SCALE R_SCALE
#endif
#if RGB_BLUE_OS2 == 0
#define C0_SCALE B_SCALE
#endif
#if RGB_GREEN_OS2 == 1
#define C1_SCALE G_SCALE
#endif
#if RGB_RED_OS2 == 2
#define C2_SCALE R_SCALE
#endif
#if RGB_BLUE_OS2 == 2
#define C2_SCALE B_SCALE
#endif

#else

#if RGB_RED == 0
#define C0_SCALE R_SCALE
#endif
#if RGB_BLUE == 0
#define C0_SCALE B_SCALE
#endif
#if RGB_GREEN == 1
#define C1_SCALE G_SCALE
#endif
#if RGB_RED == 2
#define C2_SCALE R_SCALE
#endif
#if RGB_BLUE == 2
#define C2_SCALE B_SCALE
#endif

#endif

/*
 * First we have the histogram data structure and routines for creating it.
 *
 * The number of bits of precision can be adjusted by changing these symbols.
 * We recommend keeping 6 bits for G and 5 each for R and B.
 * If you have plenty of memory and cycles, 6 bits all around gives marginally
 * better results; if you are short of memory, 5 bits all around will save
 * some space but degrade the results.
 * To maintain a fully accurate histogram, we'd need to allocate a "long"
 * (preferably unsigned long) for each cell.  In practice this is overkill;
 * we can get by with 16 bits per cell.  Few of the cell counts will overflow,
 * and clamping those that do overflow to the maximum value will give close-
 * enough results.  This reduces the recommended histogram size from 256Kb
 * to 128Kb, which is a useful savings on PC-class machines.
 * (In the second pass the histogram space is re-used for pixel mapping data;
 * in that capacity, each cell must be able to store zero to the number of
 * desired colors.  16 bits/cell is plenty for that too.)
 * Since the JPEG code is intended to run in small memory model on 80x86
 * machines, we can't just allocate the histogram in one chunk.  Instead
 * of a true 3-D array, we use a row of pointers to 2-D arrays.  Each
 * pointer corresponds to a C0 value (typically 2^5 = 32 pointers) and
 * each 2-D array has 2^6*2^5 = 2048 or 2^6*2^6 = 4096 entries.  Note that
 * on 80x86 machines, the pointer row is in near memory but the actual
 * arrays are in far memory (same arrangement as we use for image arrays).
 */

#define MAXNUMCOLORS  (MAXJSAMPLE+1) /* maximum size of colormap */

/* These will do the right thing for either R,G,B or B,G,R color order,
 * but you may not like the results for other color orders.
 */
#define HIST_C0_BITS  5     /* bits of precision in R/B histogram */
#define HIST_C1_BITS  6     /* bits of precision in G histogram */
#define HIST_C2_BITS  5     /* bits of precision in B/R histogram */

/* Number of elements along histogram axes. */
#define HIST_C0_ELEMS  (1<<HIST_C0_BITS)
#define HIST_C1_ELEMS  (1<<HIST_C1_BITS)
#define HIST_C2_ELEMS  (1<<HIST_C2_BITS)

/* These are the amounts to shift an input value to get a histogram index. */
#define C0_SHIFT  (BITS_IN_JSAMPLE-HIST_C0_BITS)
#define C1_SHIFT  (BITS_IN_JSAMPLE-HIST_C1_BITS)
#define C2_SHIFT  (BITS_IN_JSAMPLE-HIST_C2_BITS)


typedef UINT16 histcell;    /* histogram cell; prefer an unsigned type */

typedef histcell  * histptr;    /* for pointers to histogram cells */

typedef histcell hist1d[HIST_C2_ELEMS]; /* typedefs for the array */
typedef hist1d  * hist2d;   /* type for the 2nd-level pointers */
typedef hist2d * hist3d;    /* type for top-level pointer */


/* Declarations for Floyd-Steinberg dithering.
 *
 * Errors are accumulated into the array fserrors[], at a resolution of
 * 1/16th of a pixel count.  The error at a given pixel is propagated
 * to its not-yet-processed neighbors using the standard F-S fractions,
 *      ... (here)  7/16
 *      3/16    5/16    1/16
 * We work left-to-right on even rows, right-to-left on odd rows.
 *
 * We can get away with a single array (holding one row's worth of errors)
 * by using it to store the current row's errors at pixel columns not yet
 * processed, but the next row's errors at columns already processed.  We
 * need only a few extra variables to hold the errors immediately around the
 * current column.  (If we are lucky, those variables are in registers, but
 * even if not, they're probably cheaper to access than array elements are.)
 *
 * The fserrors[] array has (#columns + 2) entries; the extra entry at
 * each end saves us from special-casing the first and last pixels.
 * Each entry is three values long, one value for each color component.
 *
 * Note: on a wide image, we might not have enough room in a PC's near data
 * segment to hold the error array; so it is allocated with alloc_large.
 */

#if BITS_IN_JSAMPLE == 8
typedef INT16 FSERROR;      /* 16 bits should be enough */
typedef int LOCFSERROR;     /* use 'int' for calculation temps */
#else
typedef INT32 FSERROR;      /* may need more than 16 bits */
typedef INT32 LOCFSERROR;   /* be sure calculation temps are big enough */
#endif

typedef FSERROR  *FSERRPTR; /* pointer to error array (in  storage!) */


/* Private subobject */

typedef struct {

   struct {
       void (*finish_pass)(j_decompress_ptr);
       void (*color_quantize)(j_decompress_ptr, JSAMPARRAY, JSAMPARRAY, int);
       void (*start_pass)(j_decompress_ptr, bool);
       void (*new_color_map)(j_decompress_ptr);
   } pub;

  /* Space for the eventually created colormap is stashed here */
  JSAMPARRAY sv_colormap;   /* colormap allocated at init time */
  int desired;          /* desired # of colors = size of colormap */

  /* Variables for accumulating image statistics */
  hist3d histogram;     /* pointer to the histogram */

  bool needs_zeroed;        /* true if next pass must zero histogram */

  /* Variables for Floyd-Steinberg dithering */
  FSERRPTR fserrors;        /* accumulated errors */
  bool on_odd_row;      /* flag to remember which row we are on */
  int * error_limiter;      /* table for clamping the applied error */
} my_cquantizer;

typedef my_cquantizer * my_cquantize_ptr;


/*
 * Prescan some rows of pixels.
 * In this module the prescan simply updates the histogram, which has been
 * initialized to zeroes by start_pass.
 * An output_buf parameter is required by the method signature, but no data
 * is actually output (in fact the buffer controller is probably passing a
 * NULL pointer).
 */

void
prescan_quantize (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
          JSAMPARRAY WXUNUSED(output_buf), int num_rows)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register JSAMPROW ptr;
  register histptr histp;
  register hist3d histogram = cquantize->histogram;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    ptr = input_buf[row];
    for (col = width; col > 0; col--) {

      {

          /* get pixel value and index into the histogram */
          histp = & histogram[GETJSAMPLE(ptr[0]) >> C0_SHIFT]
                 [GETJSAMPLE(ptr[1]) >> C1_SHIFT]
                 [GETJSAMPLE(ptr[2]) >> C2_SHIFT];
          /* increment, check for overflow and undo increment if so. */
          if (++(*histp) <= 0)
            (*histp)--;
      }
      ptr += 3;
    }
  }
}


/*
 * Next we have the really interesting routines: selection of a colormap
 * given the completed histogram.
 * These routines work with a list of "boxes", each representing a rectangular
 * subset of the input color space (to histogram precision).
 */

typedef struct {
  /* The bounds of the box (inclusive); expressed as histogram indexes */
  int c0min, c0max;
  int c1min, c1max;
  int c2min, c2max;
  /* The volume (actually 2-norm) of the box */
  INT32 volume;
  /* The number of nonzero histogram cells within this box */
  long colorcount;
} box;

typedef box * boxptr;


boxptr
find_biggest_color_pop (boxptr boxlist, int numboxes)
/* Find the splittable box with the largest color population */
/* Returns NULL if no splittable boxes remain */
{
  register boxptr boxp;
  register int i;
  register long maxc = 0;
  boxptr which = NULL;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
    if (boxp->colorcount > maxc && boxp->volume > 0) {
      which = boxp;
      maxc = boxp->colorcount;
    }
  }
  return which;
}


boxptr
find_biggest_volume (boxptr boxlist, int numboxes)
/* Find the splittable box with the largest (scaled) volume */
/* Returns NULL if no splittable boxes remain */
{
  register boxptr boxp;
  register int i;
  register INT32 maxv = 0;
  boxptr which = NULL;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
    if (boxp->volume > maxv) {
      which = boxp;
      maxv = boxp->volume;
    }
  }
  return which;
}


void
update_box (j_decompress_ptr cinfo, boxptr boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its volume and population */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;
  histptr histp;
  int c0,c1,c2;
  int c0min,c0max,c1min,c1max,c2min,c2max;
  INT32 dist0,dist1,dist2;
  long ccount;

  c0min = boxp->c0min;  c0max = boxp->c0max;
  c1min = boxp->c1min;  c1max = boxp->c1max;
  c2min = boxp->c2min;  c2max = boxp->c2max;

  if (c0max > c0min)
    for (c0 = c0min; c0 <= c0max; c0++)
      for (c1 = c1min; c1 <= c1max; c1++) {
    histp = & histogram[c0][c1][c2min];
    for (c2 = c2min; c2 <= c2max; c2++)
      if (*histp++ != 0) {
        boxp->c0min = c0min = c0;
        goto have_c0min;
      }
      }
 have_c0min:
  if (c0max > c0min)
    for (c0 = c0max; c0 >= c0min; c0--)
      for (c1 = c1min; c1 <= c1max; c1++) {
    histp = & histogram[c0][c1][c2min];
    for (c2 = c2min; c2 <= c2max; c2++)
      if (*histp++ != 0) {
        boxp->c0max = c0max = c0;
        goto have_c0max;
      }
      }
 have_c0max:
  if (c1max > c1min)
    for (c1 = c1min; c1 <= c1max; c1++)
      for (c0 = c0min; c0 <= c0max; c0++) {
    histp = & histogram[c0][c1][c2min];
    for (c2 = c2min; c2 <= c2max; c2++)
      if (*histp++ != 0) {
        boxp->c1min = c1min = c1;
        goto have_c1min;
      }
      }
 have_c1min:
  if (c1max > c1min)
    for (c1 = c1max; c1 >= c1min; c1--)
      for (c0 = c0min; c0 <= c0max; c0++) {
    histp = & histogram[c0][c1][c2min];
    for (c2 = c2min; c2 <= c2max; c2++)
      if (*histp++ != 0) {
        boxp->c1max = c1max = c1;
        goto have_c1max;
      }
      }
 have_c1max:
  if (c2max > c2min)
    for (c2 = c2min; c2 <= c2max; c2++)
      for (c0 = c0min; c0 <= c0max; c0++) {
    histp = & histogram[c0][c1min][c2];
    for (c1 = c1min; c1 <= c1max; c1++, histp += HIST_C2_ELEMS)
      if (*histp != 0) {
        boxp->c2min = c2min = c2;
        goto have_c2min;
      }
      }
 have_c2min:
  if (c2max > c2min)
    for (c2 = c2max; c2 >= c2min; c2--)
      for (c0 = c0min; c0 <= c0max; c0++) {
    histp = & histogram[c0][c1min][c2];
    for (c1 = c1min; c1 <= c1max; c1++, histp += HIST_C2_ELEMS)
      if (*histp != 0) {
        boxp->c2max = c2max = c2;
        goto have_c2max;
      }
      }
 have_c2max:

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0.
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist0 = ((c0max - c0min) << C0_SHIFT) * C0_SCALE;
  dist1 = ((c1max - c1min) << C1_SHIFT) * C1_SCALE;
  dist2 = ((c2max - c2min) << C2_SHIFT) * C2_SCALE;
  boxp->volume = dist0*dist0 + dist1*dist1 + dist2*dist2;

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (c0 = c0min; c0 <= c0max; c0++)
    for (c1 = c1min; c1 <= c1max; c1++) {
      histp = & histogram[c0][c1][c2min];
      for (c2 = c2min; c2 <= c2max; c2++, histp++)
    if (*histp != 0) {
      ccount++;
    }
    }
  boxp->colorcount = ccount;
}


int
median_cut (j_decompress_ptr cinfo, boxptr boxlist, int numboxes,
        int desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
  int n,lb;
  int c0,c1,c2,cmax;
  register boxptr b1,b2;

  while (numboxes < desired_colors) {
    /* Select box to split.
     * Current algorithm: by population for first half, then by volume.
     */
    if ((numboxes*2) <= desired_colors) {
      b1 = find_biggest_color_pop(boxlist, numboxes);
    } else {
      b1 = find_biggest_volume(boxlist, numboxes);
    }
    if (b1 == NULL)     /* no splittable boxes left! */
      break;
    b2 = &boxlist[numboxes];    /* where new box will go */
    /* Copy the color bounds to the new box. */
    b2->c0max = b1->c0max; b2->c1max = b1->c1max; b2->c2max = b1->c2max;
    b2->c0min = b1->c0min; b2->c1min = b1->c1min; b2->c2min = b1->c2min;
    /* Choose which axis to split the box on.
     * Current algorithm: longest scaled axis.
     * See notes in update_box about scaling distances.
     */
    c0 = ((b1->c0max - b1->c0min) << C0_SHIFT) * C0_SCALE;
    c1 = ((b1->c1max - b1->c1min) << C1_SHIFT) * C1_SCALE;
    c2 = ((b1->c2max - b1->c2min) << C2_SHIFT) * C2_SCALE;
    /* We want to break any ties in favor of green, then red, blue last.
     * This code does the right thing for R,G,B or B,G,R color orders only.
     */
#if defined(__VISAGECPP__)

#if RGB_RED_OS2 == 0
    cmax = c1; n = 1;
    if (c0 > cmax) { cmax = c0; n = 0; }
    if (c2 > cmax) { n = 2; }
#else
    cmax = c1; n = 1;
    if (c2 > cmax) { cmax = c2; n = 2; }
    if (c0 > cmax) { n = 0; }
#endif

#else

#if RGB_RED == 0
    cmax = c1; n = 1;
    if (c0 > cmax) { cmax = c0; n = 0; }
    if (c2 > cmax) { n = 2; }
#else
    cmax = c1; n = 1;
    if (c2 > cmax) { cmax = c2; n = 2; }
    if (c0 > cmax) { n = 0; }
#endif

#endif
    /* Choose split point along selected axis, and update box bounds.
     * Current algorithm: split at halfway point.
     * (Since the box has been shrunk to minimum volume,
     * any split will produce two nonempty subboxes.)
     * Note that lb value is max for lower box, so must be < old max.
     */
    switch (n) {
    case 0:
      lb = (b1->c0max + b1->c0min) / 2;
      b1->c0max = lb;
      b2->c0min = lb+1;
      break;
    case 1:
      lb = (b1->c1max + b1->c1min) / 2;
      b1->c1max = lb;
      b2->c1min = lb+1;
      break;
    case 2:
      lb = (b1->c2max + b1->c2min) / 2;
      b1->c2max = lb;
      b2->c2min = lb+1;
      break;
    }
    /* Update stats for boxes */
    update_box(cinfo, b1);
    update_box(cinfo, b2);
    numboxes++;
  }
  return numboxes;
}


void
compute_color (j_decompress_ptr cinfo, boxptr boxp, int icolor)
/* Compute representative color for a box, put it in colormap[icolor] */
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;
  histptr histp;
  int c0,c1,c2;
  int c0min,c0max,c1min,c1max,c2min,c2max;
  long count;
  long total = 0;
  long c0total = 0;
  long c1total = 0;
  long c2total = 0;

  c0min = boxp->c0min;  c0max = boxp->c0max;
  c1min = boxp->c1min;  c1max = boxp->c1max;
  c2min = boxp->c2min;  c2max = boxp->c2max;

  for (c0 = c0min; c0 <= c0max; c0++)
    for (c1 = c1min; c1 <= c1max; c1++) {
      histp = & histogram[c0][c1][c2min];
      for (c2 = c2min; c2 <= c2max; c2++) {
    if ((count = *histp++) != 0) {
      total += count;
      c0total += ((c0 << C0_SHIFT) + ((1<<C0_SHIFT)>>1)) * count;
      c1total += ((c1 << C1_SHIFT) + ((1<<C1_SHIFT)>>1)) * count;
      c2total += ((c2 << C2_SHIFT) + ((1<<C2_SHIFT)>>1)) * count;
    }
      }
    }

  cinfo->colormap[0][icolor] = (JSAMPLE) ((c0total + (total>>1)) / total);
  cinfo->colormap[1][icolor] = (JSAMPLE) ((c1total + (total>>1)) / total);
  cinfo->colormap[2][icolor] = (JSAMPLE) ((c2total + (total>>1)) / total);
}


static void
select_colors (j_decompress_ptr cinfo, int desired_colors)
/* Master routine for color selection */
{
  boxptr boxlist;
  int numboxes;
  int i;

  /* Allocate workspace for box list */
  boxlist = (boxptr) malloc(desired_colors * sizeof(box));
  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].c0min = 0;
  boxlist[0].c0max = MAXJSAMPLE >> C0_SHIFT;
  boxlist[0].c1min = 0;
  boxlist[0].c1max = MAXJSAMPLE >> C1_SHIFT;
  boxlist[0].c2min = 0;
  boxlist[0].c2max = MAXJSAMPLE >> C2_SHIFT;
  /* Shrink it to actually-used volume and set its statistics */
  update_box(cinfo, & boxlist[0]);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut(cinfo, boxlist, numboxes, desired_colors);
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    compute_color(cinfo, & boxlist[i], i);
  cinfo->actual_number_of_colors = numboxes;

  free(boxlist); //FIXME?? I don't know if this is correct - VS
}


/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be INT32s, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.
 *
 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  Furthermore, we need not fill subboxes that are never
 * referenced in pass2; many images use only part of the color gamut, so a
 * fair amount of work is saved.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */


/* log2(histogram cells in update box) for each axis; this can be adjusted */
#define BOX_C0_LOG  (HIST_C0_BITS-3)
#define BOX_C1_LOG  (HIST_C1_BITS-3)
#define BOX_C2_LOG  (HIST_C2_BITS-3)

#define BOX_C0_ELEMS  (1<<BOX_C0_LOG) /* # of hist cells in update box */
#define BOX_C1_ELEMS  (1<<BOX_C1_LOG)
#define BOX_C2_ELEMS  (1<<BOX_C2_LOG)

#define BOX_C0_SHIFT  (C0_SHIFT + BOX_C0_LOG)
#define BOX_C1_SHIFT  (C1_SHIFT + BOX_C1_LOG)
#define BOX_C2_SHIFT  (C2_SHIFT + BOX_C2_LOG)


/*
 * The next three routines implement inverse colormap filling.  They could
 * all be folded into one big routine, but splitting them up this way saves
 * some stack space (the mindist[] and bestdist[] arrays need not coexist)
 * and may allow some compilers to produce better code by registerizing more
 * inner-loop variables.
 */

static int
find_nearby_colors (j_decompress_ptr cinfo, int minc0, int minc1, int minc2,
            JSAMPLE colorlist[])
/* Locate the colormap entries close enough to an update box to be candidates
 * for the nearest entry to some cell(s) in the update box.  The update box
 * is specified by the center coordinates of its first cell.  The number of
 * candidate colormap entries is returned, and their colormap indexes are
 * placed in colorlist[].
 * This routine uses Heckbert's "locally sorted search" criterion to select
 * the colors that need further consideration.
 */
{
  int numcolors = cinfo->actual_number_of_colors;
  int maxc0, maxc1, maxc2;
  int centerc0, centerc1, centerc2;
  int i, x, ncolors;
  INT32 minmaxdist, min_dist, max_dist, tdist;
  INT32 mindist[MAXNUMCOLORS];  /* min distance to colormap entry i */

  /* Compute true coordinates of update box's upper corner and center.
   * Actually we compute the coordinates of the center of the upper-corner
   * histogram cell, which are the upper bounds of the volume we care about.
   * Note that since ">>" rounds down, the "center" values may be closer to
   * min than to max; hence comparisons to them must be "<=", not "<".
   */
  maxc0 = minc0 + ((1 << BOX_C0_SHIFT) - (1 << C0_SHIFT));
  centerc0 = (minc0 + maxc0) >> 1;
  maxc1 = minc1 + ((1 << BOX_C1_SHIFT) - (1 << C1_SHIFT));
  centerc1 = (minc1 + maxc1) >> 1;
  maxc2 = minc2 + ((1 << BOX_C2_SHIFT) - (1 << C2_SHIFT));
  centerc2 = (minc2 + maxc2) >> 1;

  /* For each color in colormap, find:
   *  1. its minimum squared-distance to any point in the update box
   *     (zero if color is within update box);
   *  2. its maximum squared-distance to any point in the update box.
   * Both of these can be found by considering only the corners of the box.
   * We save the minimum distance for each color in mindist[];
   * only the smallest maximum distance is of interest.
   */
  minmaxdist = 0x7FFFFFFFL;

  for (i = 0; i < numcolors; i++) {
    /* We compute the squared-c0-distance term, then add in the other two. */
    x = GETJSAMPLE(cinfo->colormap[0][i]);
    if (x < minc0) {
      tdist = (x - minc0) * C0_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - maxc0) * C0_SCALE;
      max_dist = tdist*tdist;
    } else if (x > maxc0) {
      tdist = (x - maxc0) * C0_SCALE;
      min_dist = tdist*tdist;
      tdist = (x - minc0) * C0_SCALE;
      max_dist = tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      min_dist = 0;
      if (x <= centerc0) {
    tdist = (x - maxc0) * C0_SCALE;
    max_dist = tdist*tdist;
      } else {
    tdist = (x - minc0) * C0_SCALE;
    max_dist = tdist*tdist;
      }
    }

    x = GETJSAMPLE(cinfo->colormap[1][i]);
    if (x < minc1) {
      tdist = (x - minc1) * C1_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - maxc1) * C1_SCALE;
      max_dist += tdist*tdist;
    } else if (x > maxc1) {
      tdist = (x - maxc1) * C1_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - minc1) * C1_SCALE;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerc1) {
    tdist = (x - maxc1) * C1_SCALE;
    max_dist += tdist*tdist;
      } else {
    tdist = (x - minc1) * C1_SCALE;
    max_dist += tdist*tdist;
      }
    }

    x = GETJSAMPLE(cinfo->colormap[2][i]);
    if (x < minc2) {
      tdist = (x - minc2) * C2_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - maxc2) * C2_SCALE;
      max_dist += tdist*tdist;
    } else if (x > maxc2) {
      tdist = (x - maxc2) * C2_SCALE;
      min_dist += tdist*tdist;
      tdist = (x - minc2) * C2_SCALE;
      max_dist += tdist*tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerc2) {
    tdist = (x - maxc2) * C2_SCALE;
    max_dist += tdist*tdist;
      } else {
    tdist = (x - minc2) * C2_SCALE;
    max_dist += tdist*tdist;
      }
    }

    mindist[i] = min_dist;  /* save away the results */
    if (max_dist < minmaxdist)
      minmaxdist = max_dist;
  }

  /* Now we know that no cell in the update box is more than minmaxdist
   * away from some colormap entry.  Therefore, only colors that are
   * within minmaxdist of some part of the box need be considered.
   */
  ncolors = 0;
  for (i = 0; i < numcolors; i++) {
    if (mindist[i] <= minmaxdist)
      colorlist[ncolors++] = (JSAMPLE) i;
  }
  return ncolors;
}


static void
find_best_colors (j_decompress_ptr cinfo, int minc0, int minc1, int minc2,
          int numcolors, JSAMPLE colorlist[], JSAMPLE bestcolor[])
/* Find the closest colormap entry for each cell in the update box,
 * given the list of candidate colors prepared by find_nearby_colors.
 * Return the indexes of the closest entries in the bestcolor[] array.
 * This routine uses Thomas' incremental distance calculation method to
 * find the distance from a colormap entry to successive cells in the box.
 */
{
  int ic0, ic1, ic2;
  int i, icolor;
  register INT32 * bptr;    /* pointer into bestdist[] array */
  JSAMPLE * cptr;       /* pointer into bestcolor[] array */
  INT32 dist0, dist1;       /* initial distance values */
  register INT32 dist2;     /* current distance in inner loop */
  INT32 xx0, xx1;       /* distance increments */
  register INT32 xx2;
  INT32 inc0, inc1, inc2;   /* initial values for increments */
  /* This array holds the distance to the nearest-so-far color for each cell */
  INT32 bestdist[BOX_C0_ELEMS * BOX_C1_ELEMS * BOX_C2_ELEMS];

  /* Initialize best-distance for each cell of the update box */
  bptr = bestdist;
  for (i = BOX_C0_ELEMS*BOX_C1_ELEMS*BOX_C2_ELEMS-1; i >= 0; i--)
    *bptr++ = 0x7FFFFFFFL;

  /* For each color selected by find_nearby_colors,
   * compute its distance to the center of each cell in the box.
   * If that's less than best-so-far, update best distance and color number.
   */

  /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_C0  ((1 << C0_SHIFT) * C0_SCALE)
#define STEP_C1  ((1 << C1_SHIFT) * C1_SCALE)
#define STEP_C2  ((1 << C2_SHIFT) * C2_SCALE)

  for (i = 0; i < numcolors; i++) {
    icolor = GETJSAMPLE(colorlist[i]);
    /* Compute (square of) distance from minc0/c1/c2 to this color */
    inc0 = (minc0 - GETJSAMPLE(cinfo->colormap[0][icolor])) * C0_SCALE;
    dist0 = inc0*inc0;
    inc1 = (minc1 - GETJSAMPLE(cinfo->colormap[1][icolor])) * C1_SCALE;
    dist0 += inc1*inc1;
    inc2 = (minc2 - GETJSAMPLE(cinfo->colormap[2][icolor])) * C2_SCALE;
    dist0 += inc2*inc2;
    /* Form the initial difference increments */
    inc0 = inc0 * (2 * STEP_C0) + STEP_C0 * STEP_C0;
    inc1 = inc1 * (2 * STEP_C1) + STEP_C1 * STEP_C1;
    inc2 = inc2 * (2 * STEP_C2) + STEP_C2 * STEP_C2;
    /* Now loop over all cells in box, updating distance per Thomas method */
    bptr = bestdist;
    cptr = bestcolor;
    xx0 = inc0;
    for (ic0 = BOX_C0_ELEMS-1; ic0 >= 0; ic0--) {
      dist1 = dist0;
      xx1 = inc1;
      for (ic1 = BOX_C1_ELEMS-1; ic1 >= 0; ic1--) {
    dist2 = dist1;
    xx2 = inc2;
    for (ic2 = BOX_C2_ELEMS-1; ic2 >= 0; ic2--) {
      if (dist2 < *bptr) {
        *bptr = dist2;
        *cptr = (JSAMPLE) icolor;
      }
      dist2 += xx2;
      xx2 += 2 * STEP_C2 * STEP_C2;
      bptr++;
      cptr++;
    }
    dist1 += xx1;
    xx1 += 2 * STEP_C1 * STEP_C1;
      }
      dist0 += xx0;
      xx0 += 2 * STEP_C0 * STEP_C0;
    }
  }
}


static void
fill_inverse_cmap (j_decompress_ptr cinfo, int c0, int c1, int c2)
/* Fill the inverse-colormap entries in the update box that contains */
/* histogram cell c0/c1/c2.  (Only that one cell MUST be filled, but */
/* we can fill as many others as we wish.) */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;
  int minc0, minc1, minc2;  /* lower left corner of update box */
  int ic0, ic1, ic2;
  register JSAMPLE * cptr;  /* pointer into bestcolor[] array */
  register histptr cachep;  /* pointer into main cache array */
  /* This array lists the candidate colormap indexes. */
  JSAMPLE colorlist[MAXNUMCOLORS];
  int numcolors;        /* number of candidate colors */
  /* This array holds the actually closest colormap index for each cell. */
  JSAMPLE bestcolor[BOX_C0_ELEMS * BOX_C1_ELEMS * BOX_C2_ELEMS];

  /* Convert cell coordinates to update box ID */
  c0 >>= BOX_C0_LOG;
  c1 >>= BOX_C1_LOG;
  c2 >>= BOX_C2_LOG;

  /* Compute true coordinates of update box's origin corner.
   * Actually we compute the coordinates of the center of the corner
   * histogram cell, which are the lower bounds of the volume we care about.
   */
  minc0 = (c0 << BOX_C0_SHIFT) + ((1 << C0_SHIFT) >> 1);
  minc1 = (c1 << BOX_C1_SHIFT) + ((1 << C1_SHIFT) >> 1);
  minc2 = (c2 << BOX_C2_SHIFT) + ((1 << C2_SHIFT) >> 1);

  /* Determine which colormap entries are close enough to be candidates
   * for the nearest entry to some cell in the update box.
   */
  numcolors = find_nearby_colors(cinfo, minc0, minc1, minc2, colorlist);

  /* Determine the actually nearest colors. */
  find_best_colors(cinfo, minc0, minc1, minc2, numcolors, colorlist,
           bestcolor);

  /* Save the best color numbers (plus 1) in the main cache array */
  c0 <<= BOX_C0_LOG;        /* convert ID back to base cell indexes */
  c1 <<= BOX_C1_LOG;
  c2 <<= BOX_C2_LOG;
  cptr = bestcolor;
  for (ic0 = 0; ic0 < BOX_C0_ELEMS; ic0++) {
    for (ic1 = 0; ic1 < BOX_C1_ELEMS; ic1++) {
      cachep = & histogram[c0+ic0][c1+ic1][c2];
      for (ic2 = 0; ic2 < BOX_C2_ELEMS; ic2++) {
    *cachep++ = (histcell) (GETJSAMPLE(*cptr++) + 1);
      }
    }
  }
}


/*
 * Map some rows of pixels to the output colormapped representation.
 */

void
pass2_no_dither (j_decompress_ptr cinfo,
         JSAMPARRAY input_buf, JSAMPARRAY output_buf, int num_rows)
/* This version performs no dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;
  register JSAMPROW inptr, outptr;
  register histptr cachep;
  register int c0, c1, c2;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    inptr = input_buf[row];
    outptr = output_buf[row];
    for (col = width; col > 0; col--) {
      /* get pixel value and index into the cache */
      c0 = GETJSAMPLE(*inptr++) >> C0_SHIFT;
      c1 = GETJSAMPLE(*inptr++) >> C1_SHIFT;
      c2 = GETJSAMPLE(*inptr++) >> C2_SHIFT;
      cachep = & histogram[c0][c1][c2];
      /* If we have not seen this color before, find nearest colormap entry */
      /* and update the cache */
      if (*cachep == 0)
    fill_inverse_cmap(cinfo, c0,c1,c2);
      /* Now emit the colormap index for this cell */
      *outptr++ = (JSAMPLE) (*cachep - 1);
    }
  }
}


void
pass2_fs_dither (j_decompress_ptr cinfo,
         JSAMPARRAY input_buf, JSAMPARRAY output_buf, int num_rows)
/* This version performs Floyd-Steinberg dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;
  register LOCFSERROR cur0, cur1, cur2; /* current error or pixel value */
  LOCFSERROR belowerr0, belowerr1, belowerr2; /* error for pixel below cur */
  LOCFSERROR bpreverr0, bpreverr1, bpreverr2; /* error for below/prev col */
  register FSERRPTR errorptr;   /* => fserrors[] at column before current */
  JSAMPROW inptr;       /* => current input pixel */
  JSAMPROW outptr;      /* => current output pixel */
  histptr cachep;
  int dir;          /* +1 or -1 depending on direction */
  int dir3;         /* 3*dir, for advancing inptr & errorptr */
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  int *error_limit = cquantize->error_limiter;
  JSAMPROW colormap0 = cinfo->colormap[0];
  JSAMPROW colormap1 = cinfo->colormap[1];
  JSAMPROW colormap2 = cinfo->colormap[2];


  for (row = 0; row < num_rows; row++) {
    inptr = input_buf[row];
    outptr = output_buf[row];
    if (cquantize->on_odd_row) {
      /* work right to left in this row */
      inptr += (width-1) * 3;   /* so point to rightmost pixel */
      outptr += width-1;
      dir = -1;
      dir3 = -3;
      errorptr = cquantize->fserrors + (width+1)*3; /* => entry after last column */
      cquantize->on_odd_row = false; /* flip for next time */
    } else {
      /* work left to right in this row */
      dir = 1;
      dir3 = 3;
      errorptr = cquantize->fserrors; /* => entry before first real column */
      cquantize->on_odd_row = true; /* flip for next time */
    }
    /* Preset error values: no error propagated to first pixel from left */
    cur0 = cur1 = cur2 = 0;
    /* and no error propagated to row below yet */
    belowerr0 = belowerr1 = belowerr2 = 0;
    bpreverr0 = bpreverr1 = bpreverr2 = 0;

    for (col = width; col > 0; col--) {
      /* curN holds the error propagated from the previous pixel on the
       * current line.  Add the error propagated from the previous line
       * to form the complete error correction term for this pixel, and
       * round the error term (which is expressed * 16) to an integer.
       * RIGHT_SHIFT rounds towards minus infinity, so adding 8 is correct
       * for either sign of the error value.
       * Note: errorptr points to *previous* column's array entry.
       */
      cur0 = RIGHT_SHIFT(cur0 + errorptr[dir3+0] + 8, 4);
      cur1 = RIGHT_SHIFT(cur1 + errorptr[dir3+1] + 8, 4);
      cur2 = RIGHT_SHIFT(cur2 + errorptr[dir3+2] + 8, 4);
      /* Limit the error using transfer function set by init_error_limit.
       * See comments with init_error_limit for rationale.
       */
      cur0 = error_limit[cur0];
      cur1 = error_limit[cur1];
      cur2 = error_limit[cur2];
      /* Form pixel value + error, and range-limit to 0..MAXJSAMPLE.
       * The maximum error is +- MAXJSAMPLE (or less with error limiting);
       * this sets the required size of the range_limit array.
       */
      cur0 += GETJSAMPLE(inptr[0]);
      cur1 += GETJSAMPLE(inptr[1]);
      cur2 += GETJSAMPLE(inptr[2]);
      cur0 = GETJSAMPLE(range_limit[cur0]);
      cur1 = GETJSAMPLE(range_limit[cur1]);
      cur2 = GETJSAMPLE(range_limit[cur2]);
      /* Index into the cache with adjusted pixel value */
      cachep = & histogram[cur0>>C0_SHIFT][cur1>>C1_SHIFT][cur2>>C2_SHIFT];
      /* If we have not seen this color before, find nearest colormap */
      /* entry and update the cache */
      if (*cachep == 0)
    fill_inverse_cmap(cinfo, cur0>>C0_SHIFT,cur1>>C1_SHIFT,cur2>>C2_SHIFT);
      /* Now emit the colormap index for this cell */
      { register int pixcode = *cachep - 1;
    *outptr = (JSAMPLE) pixcode;
    /* Compute representation error for this pixel */
    cur0 -= GETJSAMPLE(colormap0[pixcode]);
    cur1 -= GETJSAMPLE(colormap1[pixcode]);
    cur2 -= GETJSAMPLE(colormap2[pixcode]);
      }
      /* Compute error fractions to be propagated to adjacent pixels.
       * Add these into the running sums, and simultaneously shift the
       * next-line error sums left by 1 column.
       */
      { register LOCFSERROR bnexterr, delta;

    bnexterr = cur0;    /* Process component 0 */
    delta = cur0 * 2;
    cur0 += delta;      /* form error * 3 */
    errorptr[0] = (FSERROR) (bpreverr0 + cur0);
    cur0 += delta;      /* form error * 5 */
    bpreverr0 = belowerr0 + cur0;
    belowerr0 = bnexterr;
    cur0 += delta;      /* form error * 7 */
    bnexterr = cur1;    /* Process component 1 */
    delta = cur1 * 2;
    cur1 += delta;      /* form error * 3 */
    errorptr[1] = (FSERROR) (bpreverr1 + cur1);
    cur1 += delta;      /* form error * 5 */
    bpreverr1 = belowerr1 + cur1;
    belowerr1 = bnexterr;
    cur1 += delta;      /* form error * 7 */
    bnexterr = cur2;    /* Process component 2 */
    delta = cur2 * 2;
    cur2 += delta;      /* form error * 3 */
    errorptr[2] = (FSERROR) (bpreverr2 + cur2);
    cur2 += delta;      /* form error * 5 */
    bpreverr2 = belowerr2 + cur2;
    belowerr2 = bnexterr;
    cur2 += delta;      /* form error * 7 */
      }
      /* At this point curN contains the 7/16 error value to be propagated
       * to the next pixel on the current line, and all the errors for the
       * next line have been shifted over.  We are therefore ready to move on.
       */
      inptr += dir3;        /* Advance pixel pointers to next column */
      outptr += dir;
      errorptr += dir3;     /* advance errorptr to current column */
    }
    /* Post-loop cleanup: we must unload the final error values into the
     * final fserrors[] entry.  Note we need not unload belowerrN because
     * it is for the dummy column before or after the actual array.
     */
    errorptr[0] = (FSERROR) bpreverr0; /* unload prev errs into array */
    errorptr[1] = (FSERROR) bpreverr1;
    errorptr[2] = (FSERROR) bpreverr2;
  }
}


/*
 * Initialize the error-limiting transfer function (lookup table).
 * The raw F-S error computation can potentially compute error values of up to
 * +- MAXJSAMPLE.  But we want the maximum correction applied to a pixel to be
 * much less, otherwise obviously wrong pixels will be created.  (Typical
 * effects include weird fringes at color-area boundaries, isolated bright
 * pixels in a dark area, etc.)  The standard advice for avoiding this problem
 * is to ensure that the "corners" of the color cube are allocated as output
 * colors; then repeated errors in the same direction cannot cause cascading
 * error buildup.  However, that only prevents the error from getting
 * completely out of hand; Aaron Giles reports that error limiting improves
 * the results even with corner colors allocated.
 * A simple clamping of the error values to about +- MAXJSAMPLE/8 works pretty
 * well, but the smoother transfer function used below is even better.  Thanks
 * to Aaron Giles for this idea.
 */

static void
init_error_limit (j_decompress_ptr cinfo)
/* Allocate and fill in the error_limiter table */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  int * table;
  int in, out;

  table = (int *) malloc((MAXJSAMPLE*2+1) * sizeof(int));
  table += MAXJSAMPLE;      /* so can index -MAXJSAMPLE .. +MAXJSAMPLE */
  cquantize->error_limiter = table;

#define STEPSIZE ((MAXJSAMPLE+1)/16)
  /* Map errors 1:1 up to +- MAXJSAMPLE/16 */
  out = 0;
  for (in = 0; in < STEPSIZE; in++, out++) {
    table[in] = out; table[-in] = -out;
  }
  /* Map errors 1:2 up to +- 3*MAXJSAMPLE/16 */
  for (; in < STEPSIZE*3; in++, out += (in&1) ? 0 : 1) {
    table[in] = out; table[-in] = -out;
  }
  /* Clamp the rest to final out value (which is (MAXJSAMPLE+1)/8) */
  for (; in <= MAXJSAMPLE; in++) {
    table[in] = out; table[-in] = -out;
  }
#undef STEPSIZE
}


/*
 * Finish up at the end of each pass.
 */

void
finish_pass1 (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;

  /* Select the representative colors and fill in cinfo->colormap */
  cinfo->colormap = cquantize->sv_colormap;
  select_colors(cinfo, cquantize->desired);
  /* Force next pass to zero the color index table */
  cquantize->needs_zeroed = true;
}


void
finish_pass2 (j_decompress_ptr WXUNUSED(cinfo))
{
  /* no work */
}


/*
 * Initialize for each processing pass.
 */

void
start_pass_2_quant (j_decompress_ptr cinfo, bool is_pre_scan)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  hist3d histogram = cquantize->histogram;

  if (is_pre_scan) {
    /* Set up method pointers */
    cquantize->pub.color_quantize = prescan_quantize;
    cquantize->pub.finish_pass = finish_pass1;
    cquantize->needs_zeroed = true; /* Always zero histogram */
  } else {
    /* Set up method pointers */
    cquantize->pub.color_quantize = pass2_fs_dither;
    cquantize->pub.finish_pass = finish_pass2;

    {
      size_t arraysize = (size_t) ((cinfo->output_width + 2) *
                   (3 * sizeof(FSERROR)));
      /* Allocate Floyd-Steinberg workspace if we didn't already. */
      if (cquantize->fserrors == NULL)
    cquantize->fserrors = (INT16*) malloc(arraysize);
      /* Initialize the propagated errors to zero. */
      memset((void  *) cquantize->fserrors, 0, arraysize);
      /* Make the error-limit table if we didn't already. */
      if (cquantize->error_limiter == NULL)
    init_error_limit(cinfo);
      cquantize->on_odd_row = false;
    }

  }
  /* Zero the histogram or inverse color map, if necessary */
  if (cquantize->needs_zeroed) {
    for (int i = 0; i < HIST_C0_ELEMS; i++) {
      memset((void  *) histogram[i], 0,
        HIST_C1_ELEMS*HIST_C2_ELEMS * sizeof(histcell));
    }
    cquantize->needs_zeroed = false;
  }
}


/*
 * Switch to a new external colormap between output passes.
 */

void
new_color_map_2_quant (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;

  /* Reset the inverse color map */
  cquantize->needs_zeroed = true;
}


/*
 * Module initialization routine for 2-pass color quantization.
 */

void
jinit_2pass_quantizer (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize;
  int i;

  cquantize = (my_cquantize_ptr) malloc(sizeof(my_cquantizer));
  cinfo->cquantize = (jpeg_color_quantizer *) cquantize;
  cquantize->pub.start_pass = start_pass_2_quant;
  cquantize->pub.new_color_map = new_color_map_2_quant;
  cquantize->fserrors = NULL;   /* flag optional arrays not allocated */
  cquantize->error_limiter = NULL;


  /* Allocate the histogram/inverse colormap storage */
  cquantize->histogram = (hist3d) malloc(HIST_C0_ELEMS * sizeof(hist2d));
  for (i = 0; i < HIST_C0_ELEMS; i++) {
    cquantize->histogram[i] = (hist2d) malloc(HIST_C1_ELEMS*HIST_C2_ELEMS * sizeof(histcell));
  }
  cquantize->needs_zeroed = true; /* histogram is garbage now */

  /* Allocate storage for the completed colormap, if required.
   * We do this now since it is  storage and may affect
   * the memory manager's space calculations.
   */
  {
    /* Make sure color count is acceptable */
    int desired = cinfo->desired_number_of_colors;

    cquantize->sv_colormap = (JSAMPARRAY) malloc(sizeof(JSAMPROW) * 3);
    cquantize->sv_colormap[0] = (JSAMPROW) malloc(sizeof(JSAMPLE) * desired);
    cquantize->sv_colormap[1] = (JSAMPROW) malloc(sizeof(JSAMPLE) * desired);
    cquantize->sv_colormap[2] = (JSAMPROW) malloc(sizeof(JSAMPLE) * desired);

    cquantize->desired = desired;
  }

  /* Allocate Floyd-Steinberg workspace if necessary.
   * This isn't really needed until pass 2, but again it is  storage.
   * Although we will cope with a later change in dither_mode,
   * we do not promise to honor max_memory_to_use if dither_mode changes.
   */
  {
    cquantize->fserrors = (FSERRPTR) malloc(
       (size_t) ((cinfo->output_width + 2) * (3 * sizeof(FSERROR))));
    /* Might as well create the error-limiting table too. */
    init_error_limit(cinfo);
  }
}










void
prepare_range_limit_table (j_decompress_ptr cinfo)
/* Allocate and fill in the sample_range_limit table */
{
  JSAMPLE * table;
  int i;

  table = (JSAMPLE *) malloc((5 * (MAXJSAMPLE+1) + CENTERJSAMPLE) * sizeof(JSAMPLE));
  cinfo->srl_orig = table;
  table += (MAXJSAMPLE+1);  /* allow negative subscripts of simple table */
  cinfo->sample_range_limit = table;
  /* First segment of "simple" table: limit[x] = 0 for x < 0 */
  memset(table - (MAXJSAMPLE+1), 0, (MAXJSAMPLE+1) * sizeof(JSAMPLE));
  /* Main part of "simple" table: limit[x] = x */
  for (i = 0; i <= MAXJSAMPLE; i++)
    table[i] = (JSAMPLE) i;
  table += CENTERJSAMPLE;   /* Point to where post-IDCT table starts */
  /* End of simple table, rest of first half of post-IDCT table */
  for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
    table[i] = MAXJSAMPLE;
  /* Second half of post-IDCT table */
  memset(table + (2 * (MAXJSAMPLE+1)), 0,
      (2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * sizeof(JSAMPLE));
  memcpy(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE),
      cinfo->sample_range_limit, CENTERJSAMPLE * sizeof(JSAMPLE));
}




/*
 * wxQuantize
 */

IMPLEMENT_DYNAMIC_CLASS(wxQuantize, wxObject)

void wxQuantize::DoQuantize(unsigned w, unsigned h, unsigned char **in_rows, unsigned char **out_rows,
    unsigned char *palette, int desiredNoColours)
{
    j_decompress dec;
    my_cquantize_ptr cquantize;

    dec.output_width = w;
    dec.desired_number_of_colors = desiredNoColours;
    prepare_range_limit_table(&dec);
    jinit_2pass_quantizer(&dec);
    cquantize = (my_cquantize_ptr) dec.cquantize;


    cquantize->pub.start_pass(&dec, true);
    cquantize->pub.color_quantize(&dec, in_rows, out_rows, h);
    cquantize->pub.finish_pass(&dec);

    cquantize->pub.start_pass(&dec, false);
    cquantize->pub.color_quantize(&dec, in_rows, out_rows, h);
    cquantize->pub.finish_pass(&dec);


    for (int i = 0; i < dec.desired_number_of_colors; i++) {
        palette[3 * i + 0] = dec.colormap[0][i];
        palette[3 * i + 1] = dec.colormap[1][i];
        palette[3 * i + 2] = dec.colormap[2][i];
    }

    for (int ii = 0; ii < HIST_C0_ELEMS; ii++) free(cquantize->histogram[ii]);
    free(cquantize->histogram);
    free(dec.colormap[0]);
    free(dec.colormap[1]);
    free(dec.colormap[2]);
    free(dec.colormap);
    free(dec.srl_orig);

    //free(cquantize->error_limiter);
    free((void*)(cquantize->error_limiter - MAXJSAMPLE)); // To reverse what was done to it

    free(cquantize->fserrors);
    free(cquantize);
}

// TODO: somehow make use of the Windows system colours, rather than ignoring them for the
// purposes of quantization.

bool wxQuantize::Quantize(const wxImage& src, wxImage& dest,
                          wxPalette** pPalette,
                          int desiredNoColours,
                          unsigned char** eightBitData,
                          int flags)

{
    int i;

    int windowsSystemColourCount = 20;

    int paletteShift = 0;

    // Shift the palette up by the number of Windows system colours,
    // if necessary
    if (flags & wxQUANTIZE_INCLUDE_WINDOWS_COLOURS)
        paletteShift = windowsSystemColourCount;

    // Make room for the Windows system colours
#ifdef __WXMSW__
    if ((flags & wxQUANTIZE_INCLUDE_WINDOWS_COLOURS) && (desiredNoColours > (256 - windowsSystemColourCount)))
        desiredNoColours = 256 - windowsSystemColourCount;
#endif

    // create rows info:
    int h = src.GetHeight();
    int w = src.GetWidth();
    unsigned char **rows = new unsigned char *[h];
    unsigned char *imgdt = src.GetData();
    for (i = 0; i < h; i++)
        rows[i] = imgdt + 3/*RGB*/ * w * i;

    unsigned char palette[3*256];

    // This is the image as represented by palette indexes.
    unsigned char *data8bit = new unsigned char[w * h];
    unsigned char **outrows = new unsigned char *[h];
    for (i = 0; i < h; i++)
        outrows[i] = data8bit + w * i;

    //RGB->palette
    DoQuantize(w, h, rows, outrows, palette, desiredNoColours);

    delete[] rows;
    delete[] outrows;

    // palette->RGB(max.256)

    if (flags & wxQUANTIZE_FILL_DESTINATION_IMAGE)
    {
        if (!dest.Ok())
            dest.Create(w, h);

        imgdt = dest.GetData();
        for (i = 0; i < w * h; i++)
        {
            unsigned char c = data8bit[i];
            imgdt[3 * i + 0/*R*/] = palette[3 * c + 0];
            imgdt[3 * i + 1/*G*/] = palette[3 * c + 1];
            imgdt[3 * i + 2/*B*/] = palette[3 * c + 2];
        }
    }

    if (eightBitData && (flags & wxQUANTIZE_RETURN_8BIT_DATA))
    {
#ifdef __WXMSW__
        if (flags & wxQUANTIZE_INCLUDE_WINDOWS_COLOURS)
        {
            // We need to shift the palette entries up
            // to make room for the Windows system colours.
            for (i = 0; i < w * h; i++)
                data8bit[i] = (unsigned char)(data8bit[i] + paletteShift);
        }
#endif
        *eightBitData = data8bit;
    }
    else
        delete[] data8bit;

#if wxUSE_PALETTE
    // Make a wxWidgets palette
    if (pPalette)
    {
        unsigned char* r = new unsigned char[256];
        unsigned char* g = new unsigned char[256];
        unsigned char* b = new unsigned char[256];

#ifdef __WXMSW__
        // Fill the first 20 entries with Windows system colours
        if (flags & wxQUANTIZE_INCLUDE_WINDOWS_COLOURS)
        {
            HDC hDC = ::GetDC(NULL);
            PALETTEENTRY* entries = new PALETTEENTRY[windowsSystemColourCount];
            ::GetSystemPaletteEntries(hDC, 0, windowsSystemColourCount, entries);
            ::ReleaseDC(NULL, hDC);

            for (i = 0; i < windowsSystemColourCount; i++)
            {
                r[i] = entries[i].peRed;
                g[i] = entries[i].peGreen;
                b[i] = entries[i].peBlue;
            }
            delete[] entries;
        }
#endif

        for (i = 0; i < desiredNoColours; i++)
        {
            r[i+paletteShift] = palette[i*3 + 0];
            g[i+paletteShift] = palette[i*3 + 1];
            b[i+paletteShift] = palette[i*3 + 2];
        }

        // Blank out any remaining palette entries
        for (i = desiredNoColours+paletteShift; i < 256; i++)
        {
            r[i] = 0;
            g[i] = 0;
            b[i] = 0;
        }
        *pPalette = new wxPalette(256, r, g, b);
        delete[] r;
        delete[] g;
        delete[] b;
    }
#endif // wxUSE_PALETTE

    return true;
}

// This version sets a palette in the destination image so you don't
// have to manage it yourself.

bool wxQuantize::Quantize(const wxImage& src,
                          wxImage& dest,
                          int desiredNoColours,
                          unsigned char** eightBitData,
                          int flags)
{
    wxPalette* palette = NULL;
    if ( !Quantize(src, dest, & palette, desiredNoColours, eightBitData, flags) )
        return false;

#if wxUSE_PALETTE
    if (palette)
    {
        dest.SetPalette(* palette);
        delete palette;
    }
#endif // wxUSE_PALETTE

    return true;
}

#endif
    // wxUSE_IMAGE
