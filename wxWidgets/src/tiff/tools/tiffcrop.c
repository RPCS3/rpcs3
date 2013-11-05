
/* tiffcrop.c -- a port of tiffcp.c extended to include manipulations of
 * the image data through additional options listed below
 *
 * Original code:
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 * Additions (c) Richard Nolde 2006-2010 
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
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS OR ANY OTHER COPYRIGHT  
 * HOLDERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL 
 * DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
 * DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND 
 * ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Some portions of the current code are derived from tiffcp, primarly in 
 * the areas of lowlevel reading and writing of TAGS, scanlines and tiles though
 * some of the original functions have been extended to support arbitrary bit
 * depths. These functions are presented at the top of this file.
 *
 * Add support for the options below to extract sections of image(s) 
 * and to modify the whole image or selected portions of each image by
 * rotations, mirroring, and colorscale/colormap inversion of selected
 * types of TIFF images when appropriate. Some color model dependent 
 * functions are restricted to bilevel or 8 bit per sample data.
 * See the man page for the full explanations.
 *
 * New Options: 
 * -h             Display the syntax guide.
 * -v             Report the version and last build date for tiffcrop and libtiff.
 * -z x1,y1,x2,y2:x3,y3,x4,y4:..xN,yN,xN + 1, yN + 1 
 *                Specify a series of coordinates to define rectangular
 *                regions by the top left and lower right corners.
 * -e c|d|i|m|s   export mode for images and selections from input images
 *   combined     All images and selections are written to a single file (default)
 *                with multiple selections from one image combined into a single image
 *   divided      All images and selections are written to a single file
 *                with each selection from one image written to a new image
 *   image        Each input image is written to a new file (numeric filename sequence)
 *                with multiple selections from the image combined into one image
 *   multiple     Each input image is written to a new file (numeric filename sequence)
 *                with each selection from the image written to a new image
 *   separated    Individual selections from each image are written to separate files
 * -U units       [in, cm, px ] inches, centimeters or pixels
 * -H #           Set horizontal resolution of output images to #
 * -V #           Set vertical resolution of output images to #
 * -J #           Horizontal margin of output page to # expressed in current
 *                units when sectioning image into columns x rows 
 *                using the -S cols:rows option.
 * -K #           Vertical margin of output page to # expressed in current
 *                units when sectioning image into columns x rows
 *                using the -S cols:rows option.
 * -X #           Horizontal dimension of region to extract expressed in current
 *                units
 * -Y #           Vertical dimension of region to extract expressed in current
 *                units
 * -O orient      Orientation for output image, portrait, landscape, auto
 * -P page        Page size for output image segments, eg letter, legal, tabloid,
 *                etc.
 * -S cols:rows   Divide the image into equal sized segments using cols across
 *                and rows down
 * -E t|l|r|b     Edge to use as origin
 * -m #,#,#,#     Margins from edges for selection: top, left, bottom, right
 *                (commas separated)
 * -Z #:#,#:#     Zones of the image designated as zone X of Y, 
 *                eg 1:3 would be first of three equal portions measured
 *                from reference edge
 * -N odd|even|#,#-#,#|last 
 *                Select sequences and/or ranges of images within file
 *                to process. The words odd or even may be used to specify
 *                all odd or even numbered images the word last may be used
 *                in place of a number in the sequence to indicate the final
 *                image in the file without knowing how many images there are.
 * -R #           Rotate image or crop selection by 90,180,or 270 degrees
 *                clockwise  
 * -F h|v         Flip (mirror) image or crop selection horizontally
 *                or vertically 
 * -I [black|white|data|both]
 *                Invert color space, eg dark to light for bilevel and grayscale images
 *                If argument is white or black, set the PHOTOMETRIC_INTERPRETATION 
 *                tag to MinIsBlack or MinIsWhite without altering the image data
 *                If the argument is data or both, the image data are modified:
 *                both inverts the data and the PHOTOMETRIC_INTERPRETATION tag,
 *                data inverts the data but not the PHOTOMETRIC_INTERPRETATION tag
 * -D input:<filename1>,output:<filename2>,format:<raw|txt>,level:N,debug:N
 *                Dump raw data for input and/or output images to individual files
 *                in raw (binary) format or text (ASCII) representing binary data
 *                as strings of 1s and 0s. The filename arguments are used as stems
 *                from which individual files are created for each image. Text format
 *                includes annotations for image parameters and scanline info. Level
 *                selects which functions dump data, with higher numbers selecting
 *                lower level, scanline level routines. Debug reports a limited set
 *                of messages to monitor progess without enabling dump logs.
 */

static   char tiffcrop_version_id[] = "2.4";
static   char tiffcrop_rev_date[] = "12-13-2010";

#include "tif_config.h"
#include "tiffiop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#if defined(VMS)
# define unlink delete
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef streq
#define	streq(a,b)	(strcmp((a),(b)) == 0)
#endif
#define	strneq(a,b,n)	(strncmp((a),(b),(n)) == 0)

#define	TRUE	1
#define	FALSE	0

#ifndef TIFFhowmany
#define TIFFhowmany(x, y) ((((uint32)(x))+(((uint32)(y))-1))/((uint32)(y)))
#define TIFFhowmany8(x) (((x)&0x07)?((uint32)(x)>>3)+1:(uint32)(x)>>3)
#endif

/*
 * Definitions and data structures required to support cropping and image
 * manipulations.
 */

#define EDGE_TOP      1
#define EDGE_LEFT     2
#define EDGE_BOTTOM   3
#define EDGE_RIGHT    4
#define EDGE_CENTER   5

#define MIRROR_HORIZ  1
#define MIRROR_VERT   2
#define MIRROR_BOTH   3
#define ROTATECW_90   8
#define ROTATECW_180 16
#define ROTATECW_270 32
#define ROTATE_ANY ROTATECW_90 || ROTATECW_180 || ROTATECW_270 

#define CROP_NONE     0
#define CROP_MARGINS  1
#define CROP_WIDTH    2
#define CROP_LENGTH   4
#define CROP_ZONES    8
#define CROP_REGIONS 16
#define CROP_ROTATE  32
#define CROP_MIRROR  64
#define CROP_INVERT 128

/* Modes for writing out images and selections */
#define ONE_FILE_COMPOSITE       0 /* One file, sections combined sections */
#define ONE_FILE_SEPARATED       1 /* One file, sections to new IFDs */
#define FILE_PER_IMAGE_COMPOSITE 2 /* One file per image, combined sections */
#define FILE_PER_IMAGE_SEPARATED 3 /* One file per input image */
#define FILE_PER_SELECTION       4 /* One file per selection */

#define COMPOSITE_IMAGES         0 /* Selections combined into one image */  
#define SEPARATED_IMAGES         1 /* Selections saved to separate images */

#define STRIP    1
#define TILE     2

#define MAX_REGIONS   8  /* number of regions to extract from a single page */
#define MAX_OUTBUFFS  8  /* must match larger of zones or regions */
#define MAX_SECTIONS 32  /* number of sections per page to write to output */
#define MAX_IMAGES 2048  /* number of images in descrete list, not in the file */
#define MAX_SAMPLES   8  /* maximum number of samples per pixel supported */
#define MAX_BITS_PER_SAMPLE 64 /* maximum bit depth supported */
#define MAX_EXPORT_PAGES 999999  /* maximum number of export pages per file */

#define DUMP_NONE   0
#define DUMP_TEXT   1
#define DUMP_RAW    2

/* Offsets into buffer for margins and fixed width and length segments */
struct offset {
  uint32  tmargin;
  uint32  lmargin;
  uint32  bmargin;
  uint32  rmargin;
  uint32  crop_width;
  uint32  crop_length;
  uint32  startx;
  uint32  endx;
  uint32  starty;
  uint32  endy;
};

/* Description of a zone within the image. Position 1 of 3 zones would be 
 * the first third of the image. These are computed after margins and 
 * width/length requests are applied so that you can extract multiple 
 * zones from within a larger region for OCR or barcode recognition.
 */

struct  buffinfo {
  uint32 size;           /* size of this buffer */
  unsigned char *buffer; /* address of the allocated buffer */
};

struct  zone {
  int   position;  /* ordinal of segment to be extracted */
  int   total;     /* total equal sized divisions of crop area */
  };

struct  pageseg {
  uint32 x1;        /* index of left edge */
  uint32 x2;        /* index of right edge */
  uint32 y1;        /* index of top edge */
  uint32 y2;        /* index of bottom edge */
  int    position;  /* ordinal of segment to be extracted */
  int    total;     /* total equal sized divisions of crop area */
  uint32 buffsize;  /* size of buffer needed to hold the cropped zone */
};

struct  coordpairs {
  double X1;        /* index of left edge in current units */
  double X2;        /* index of right edge in current units */
  double Y1;        /* index of top edge in current units */
  double Y2;        /* index of bottom edge in current units */
};

struct  region {
  uint32 x1;        /* pixel offset of left edge */
  uint32 x2;        /* pixel offset of right edge */
  uint32 y1;        /* pixel offset of top edge */
  uint32 y2;        /* picel offset of bottom edge */
  uint32 width;     /* width in pixels */
  uint32 length;    /* length in pixels */
  uint32 buffsize;  /* size of buffer needed to hold the cropped region */
  unsigned char *buffptr; /* address of start of the region */
};

/* Cropping parameters from command line and image data 
 * Note: This should be renamed to proc_opts and expanded to include all current globals
 * if possible, but each function that accesses global variables will have to be redone.
 */
struct crop_mask {
  double width;           /* Selection width for master crop region in requested units */
  double length;          /* Selection length for master crop region in requesed units */
  double margins[4];      /* Top, left, bottom, right margins */
  float  xres;            /* Horizontal resolution read from image*/
  float  yres;            /* Vertical resolution read from image */
  uint32 combined_width;  /* Width of combined cropped zones */
  uint32 combined_length; /* Length of combined cropped zones */
  uint32 bufftotal;       /* Size of buffer needed to hold all the cropped region */
  uint16 img_mode;        /* Composite or separate images created from zones or regions */
  uint16 exp_mode;        /* Export input images or selections to one or more files */
  uint16 crop_mode;       /* Crop options to be applied */
  uint16 res_unit;        /* Resolution unit for margins and selections */
  uint16 edge_ref;        /* Reference edge for sections extraction and combination */
  uint16 rotation;        /* Clockwise rotation of the extracted region or image */
  uint16 mirror;          /* Mirror extracted region or image horizontally or vertically */
  uint16 invert;          /* Invert the color map of image or region */
  uint16 photometric;     /* Status of photometric interpretation for inverted image */
  uint16 selections;      /* Number of regions or zones selected */
  uint16 regions;         /* Number of regions delimited by corner coordinates */
  struct region regionlist[MAX_REGIONS]; /* Regions within page or master crop region */
  uint16 zones;           /* Number of zones delimited by Ordinal:Total requested */
  struct zone zonelist[MAX_REGIONS]; /* Zones indices to define a region */
  struct coordpairs corners[MAX_REGIONS]; /* Coordinates of upper left and lower right corner */
};

#define MAX_PAPERNAMES 49
#define MAX_PAPERNAME_LENGTH 15
#define DEFAULT_RESUNIT      RESUNIT_INCH
#define DEFAULT_PAGE_HEIGHT   14.0
#define DEFAULT_PAGE_WIDTH     8.5
#define DEFAULT_RESOLUTION   300
#define DEFAULT_PAPER_SIZE  "legal"

#define ORIENTATION_NONE       0
#define ORIENTATION_PORTRAIT   1
#define ORIENTATION_LANDSCAPE  2
#define ORIENTATION_SEASCAPE   4
#define ORIENTATION_AUTO      16

#define PAGE_MODE_NONE         0
#define PAGE_MODE_RESOLUTION   1
#define PAGE_MODE_PAPERSIZE    2
#define PAGE_MODE_MARGINS      4
#define PAGE_MODE_ROWSCOLS     8

#define INVERT_DATA_ONLY      10
#define INVERT_DATA_AND_TAG   11

struct paperdef {
  char   name[MAX_PAPERNAME_LENGTH];
  double width;
  double length;
  double asratio;
  };

/* European page sizes corrected from update sent by 
 * thomas . jarosch @ intra2net . com on 5/7/2010
 * Paper Size       Width   Length  Aspect Ratio */
struct paperdef PaperTable[MAX_PAPERNAMES] = {
  {"default",         8.500,  14.000,  0.607},
  {"pa4",             8.264,  11.000,  0.751},
  {"letter",          8.500,  11.000,  0.773},
  {"legal",           8.500,  14.000,  0.607},
  {"half-letter",     8.500,   5.514,  1.542},
  {"executive",       7.264,  10.528,  0.690},
  {"tabloid",        11.000,  17.000,  0.647},
  {"11x17",          11.000,  17.000,  0.647},
  {"ledger",         17.000,  11.000,  1.545},
  {"archa",           9.000,  12.000,  0.750},
  {"archb",          12.000,  18.000,  0.667},
  {"archc",          18.000,  24.000,  0.750},
  {"archd",          24.000,  36.000,  0.667},
  {"arche",          36.000,  48.000,  0.750},
  {"csheet",         17.000,  22.000,  0.773},
  {"dsheet",         22.000,  34.000,  0.647},
  {"esheet",         34.000,  44.000,  0.773},
  {"superb",         11.708,  17.042,  0.687},
  {"commercial",      4.139,   9.528,  0.434},
  {"monarch",         3.889,   7.528,  0.517},
  {"envelope-dl",     4.333,   8.681,  0.499},
  {"envelope-c5",     6.389,   9.028,  0.708},
  {"europostcard",    4.139,   5.833,  0.710},
  {"a0",             33.110,  46.811,  0.707},
  {"a1",             23.386,  33.110,  0.706},
  {"a2",             16.535,  23.386,  0.707},
  {"a3",             11.693,  16.535,  0.707},
  {"a4",              8.268,  11.693,  0.707},
  {"a5",              5.827,   8.268,  0.705},
  {"a6",              4.134,   5.827,  0.709},
  {"a7",              2.913,   4.134,  0.705},
  {"a8",              2.047,   2.913,  0.703},
  {"a9",              1.457,   2.047,  0.712},
  {"a10",             1.024,   1.457,  0.703},
  {"b0",             39.370,  55.669,  0.707},
  {"b1",             27.835,  39.370,  0.707},
  {"b2",             19.685,  27.835,  0.707},
  {"b3",             13.898,  19.685,  0.706},
  {"b4",              9.843,  13.898,  0.708},
  {"b5",              6.929,   9.843,  0.704},
  {"b6",              4.921,   6.929,  0.710},
  {"c0",             36.102,  51.063,  0.707},
  {"c1",             25.512,  36.102,  0.707},
  {"c2",             18.031,  25.512,  0.707},
  {"c3",             12.756,  18.031,  0.707},
  {"c4",              9.016,  12.756,  0.707},
  {"c5",              6.378,   9.016,  0.707},
  {"c6",              4.488,   6.378,  0.704},
  {"",                0.000,   0.000,  1.000}
};

/* Structure to define input image parameters */
struct image_data {
  float  xres;
  float  yres;
  uint32 width;
  uint32 length;
  uint16 res_unit;
  uint16 bps;
  uint16 spp;
  uint16 planar;
  uint16 photometric;
  uint16 orientation;
  uint16 compression;
  uint16 adjustments;
};

/* Structure to define the output image modifiers */
struct pagedef {
  char          name[16];
  double        width;    /* width in pixels */
  double        length;   /* length in pixels */
  double        hmargin;  /* margins to subtract from width of sections */
  double        vmargin;  /* margins to subtract from height of sections */
  double        hres;     /* horizontal resolution for output */
  double        vres;     /* vertical resolution for output */
  uint32        mode;     /* bitmask of modifiers to page format */
  uint16        res_unit; /* resolution unit for output image */
  unsigned int  rows;     /* number of section rows */
  unsigned int  cols;     /* number of section cols */
  unsigned int  orient;   /* portrait, landscape, seascape, auto */
};

struct dump_opts {
  int  debug;
  int  format;
  int  level;
  char mode[4];
  char infilename[PATH_MAX + 1];
  char outfilename[PATH_MAX + 1];
  FILE *infile;
  FILE *outfile;
  };

/* globals */
static int    outtiled = -1;
static uint32 tilewidth = 0;
static uint32 tilelength = 0;

static uint16 config = 0;
static uint16 compression = 0;
static uint16 predictor = 0;
static uint16 fillorder = 0;
static uint32 rowsperstrip = 0;
static uint32 g3opts = 0;
static int    ignore = FALSE;		/* if true, ignore read errors */
static uint32 defg3opts = (uint32) -1;
static int    quality = 100;		/* JPEG quality */
/* static int    jpegcolormode = -1;        was JPEGCOLORMODE_RGB;  */
static int    jpegcolormode = JPEGCOLORMODE_RGB;
static uint16 defcompression = (uint16) -1;
static uint16 defpredictor = (uint16) -1;
static int    pageNum = 0;
static int    little_endian = 1;

/* Functions adapted from tiffcp with additions or significant modifications */
static int  readContigStripsIntoBuffer   (TIFF*, uint8*);
static int  readSeparateStripsIntoBuffer (TIFF*, uint8*, uint32, uint32, tsample_t, struct dump_opts *);
static int  readContigTilesIntoBuffer    (TIFF*, uint8*, uint32, uint32, uint32, uint32, tsample_t, uint16);
static int  readSeparateTilesIntoBuffer  (TIFF*, uint8*, uint32, uint32, uint32, uint32, tsample_t, uint16);
static int  writeBufferToContigStrips    (TIFF*, uint8*, uint32);
static int  writeBufferToContigTiles     (TIFF*, uint8*, uint32, uint32, tsample_t, struct dump_opts *);
static int  writeBufferToSeparateStrips  (TIFF*, uint8*, uint32, uint32, tsample_t, struct dump_opts *);
static int  writeBufferToSeparateTiles   (TIFF*, uint8*, uint32, uint32, tsample_t, struct dump_opts *);
static int  extractContigSamplesToBuffer (uint8 *, uint8 *, uint32, uint32, tsample_t, 
                                         uint16, uint16, struct dump_opts *);
static int processCompressOptions(char*);
static void usage(void);

/* All other functions by Richard Nolde,  not found in tiffcp */
static void initImageData (struct image_data *);
static void initCropMasks (struct crop_mask *);
static void initPageSetup (struct pagedef *, struct pageseg *, struct buffinfo []);
static void initDumpOptions(struct dump_opts *);

/* Command line and file naming functions */
void  process_command_opts (int, char *[], char *, char *, uint32 *,
	                    uint16 *, uint16 *, uint32 *, uint32 *, uint32 *,
		            struct crop_mask *, struct pagedef *, 
                            struct dump_opts *, 
                            unsigned int *, unsigned int *);
static  int update_output_file (TIFF **, char *, int, char *, unsigned int *);


/*  * High level functions for whole image manipulation */
static int  get_page_geometry (char *, struct pagedef*);
static int  computeInputPixelOffsets(struct crop_mask *, struct image_data *, 
                                     struct offset *);
static int  computeOutputPixelOffsets (struct crop_mask *, struct image_data *,
				       struct pagedef *, struct pageseg *,
                                       struct dump_opts *);
static int  loadImage(TIFF *, struct image_data *, struct dump_opts *, unsigned char **);
static int  correct_orientation(struct image_data *, unsigned char **);
static int  getCropOffsets(struct image_data *, struct crop_mask *, struct dump_opts *);
static int  processCropSelections(struct image_data *, struct crop_mask *, 
                                  unsigned char **, struct buffinfo []);
static int  writeSelections(TIFF *, TIFF **, struct crop_mask *, struct image_data *,
                            struct dump_opts *, struct buffinfo [],
                            char *, char *, unsigned int*, unsigned int);

/* Section functions */
static int  createImageSection(uint32, unsigned char **);
static int  extractImageSection(struct image_data *, struct pageseg *, 
                                unsigned char *, unsigned char *);
static int  writeSingleSection(TIFF *, TIFF *, struct image_data *,
                               struct dump_opts *, uint32, uint32,
			       double, double, unsigned char *);
static int  writeImageSections(TIFF *, TIFF *, struct image_data *,
                               struct pagedef *, struct pageseg *, 
                               struct dump_opts *, unsigned char *, 
                               unsigned char **);
/* Whole image functions */
static int  createCroppedImage(struct image_data *, struct crop_mask *, 
                               unsigned char **, unsigned char **);
static int  writeCroppedImage(TIFF *, TIFF *, struct image_data *image,
                              struct dump_opts * dump,
                              uint32, uint32, unsigned char *, int, int);

/* Image manipulation functions */
static int rotateContigSamples8bits(uint16, uint16, uint16, uint32, 
                                    uint32,   uint32, uint8 *, uint8 *);
static int rotateContigSamples16bits(uint16, uint16, uint16, uint32, 
                                     uint32,   uint32, uint8 *, uint8 *);
static int rotateContigSamples24bits(uint16, uint16, uint16, uint32, 
                                     uint32,   uint32, uint8 *, uint8 *);
static int rotateContigSamples32bits(uint16, uint16, uint16, uint32, 
                                     uint32,   uint32, uint8 *, uint8 *);
static int rotateImage(uint16, struct image_data *, uint32 *, uint32 *,
 		       unsigned char **);
static int mirrorImage(uint16, uint16, uint16, uint32, uint32,
		       unsigned char *);
static int invertImage(uint16, uint16, uint16, uint32, uint32,
		       unsigned char *);

/* Functions to reverse the sequence of samples in a scanline */
static int reverseSamples8bits  (uint16, uint16, uint32, uint8 *, uint8 *);
static int reverseSamples16bits (uint16, uint16, uint32, uint8 *, uint8 *);
static int reverseSamples24bits (uint16, uint16, uint32, uint8 *, uint8 *);
static int reverseSamples32bits (uint16, uint16, uint32, uint8 *, uint8 *);
static int reverseSamplesBytes  (uint16, uint16, uint32, uint8 *, uint8 *);

/* Functions for manipulating individual samples in an image */
static int extractSeparateRegion(struct image_data *, struct crop_mask *,
		 		 unsigned char *, unsigned char *, int);
static int extractCompositeRegions(struct image_data *,  struct crop_mask *,
				   unsigned char *, unsigned char *);
static int extractContigSamples8bits (uint8 *, uint8 *, uint32,
 	                             tsample_t, uint16, uint16, 
                                     tsample_t, uint32, uint32);
static int extractContigSamples16bits (uint8 *, uint8 *, uint32,
 	                              tsample_t, uint16, uint16, 
                                      tsample_t, uint32, uint32);
static int extractContigSamples24bits (uint8 *, uint8 *, uint32,
 	                              tsample_t, uint16, uint16, 
                                      tsample_t, uint32, uint32);
static int extractContigSamples32bits (uint8 *, uint8 *, uint32,
	                              tsample_t, uint16, uint16, 
                                      tsample_t, uint32, uint32);
static int extractContigSamplesBytes (uint8 *, uint8 *, uint32, 
                                      tsample_t, uint16, uint16, 
				      tsample_t, uint32, uint32);
static int extractContigSamplesShifted8bits (uint8 *, uint8 *, uint32,
 	                                     tsample_t, uint16, uint16,
                                             tsample_t, uint32, uint32,
                                             int);
static int extractContigSamplesShifted16bits (uint8 *, uint8 *, uint32,
 	                                      tsample_t, uint16, uint16, 
				              tsample_t, uint32, uint32,
                                              int);
static int extractContigSamplesShifted24bits (uint8 *, uint8 *, uint32,
 	                                      tsample_t, uint16, uint16, 
				              tsample_t, uint32, uint32,
                                              int);
static int extractContigSamplesShifted32bits (uint8 *, uint8 *, uint32,
	                                      tsample_t, uint16, uint16, 
				              tsample_t, uint32, uint32,
                                              int);
static int extractContigSamplesToTileBuffer(uint8 *, uint8 *, uint32, uint32,
  	                                    uint32, uint32, tsample_t, uint16,
					    uint16, uint16, struct dump_opts *);

/* Functions to combine separate planes into interleaved planes */
static int combineSeparateSamples8bits (uint8 *[], uint8 *, uint32, uint32,
                                        uint16, uint16, FILE *, int, int);
static int combineSeparateSamples16bits (uint8 *[], uint8 *, uint32, uint32,
                                         uint16, uint16, FILE *, int, int);
static int combineSeparateSamples24bits (uint8 *[], uint8 *, uint32, uint32,
                                         uint16, uint16, FILE *, int, int);
static int combineSeparateSamples32bits (uint8 *[], uint8 *, uint32, uint32,
                                         uint16, uint16, FILE *, int, int);
static int combineSeparateSamplesBytes (unsigned char *[], unsigned char *,
					uint32, uint32, tsample_t, uint16,
                                        FILE *, int, int);

static int combineSeparateTileSamples8bits (uint8 *[], uint8 *, uint32, uint32,
                                            uint32, uint32, uint16, uint16, 
                                            FILE *, int, int);
static int combineSeparateTileSamples16bits (uint8 *[], uint8 *, uint32, uint32,
                                             uint32, uint32, uint16, uint16,
                                             FILE *, int, int);
static int combineSeparateTileSamples24bits (uint8 *[], uint8 *, uint32, uint32,
                                             uint32, uint32, uint16, uint16,
                                             FILE *, int, int);
static int combineSeparateTileSamples32bits (uint8 *[], uint8 *, uint32, uint32,
                                             uint32, uint32, uint16, uint16,
                                             FILE *, int, int);
static int combineSeparateTileSamplesBytes (unsigned char *[], unsigned char *,
			  		    uint32, uint32, uint32, uint32, 
                                            tsample_t, uint16, FILE *, int, int);

/* Dump functions for debugging */
static void dump_info  (FILE *, int, char *, char *, ...);
static int  dump_data  (FILE *, int, char *, unsigned char *, uint32);
static int  dump_byte  (FILE *, int, char *, unsigned char);
static int  dump_short (FILE *, int, char *, uint16);
static int  dump_long  (FILE *, int, char *, uint32);
static int  dump_wide  (FILE *, int, char *, uint64);
static int  dump_buffer (FILE *, int, uint32, uint32, uint32, unsigned char *);

/* End function declarations */
/* Functions derived in whole or in part from tiffcp */
/* The following functions are taken largely intact from tiffcp */

static   char* usage_info[] = {
"usage: tiffcrop [options] source1 ... sourceN  destination",
"where options are:",
" -h		Print this syntax listing",
" -v		Print tiffcrop version identifier and last revision date",
" ",
" -a		Append to output instead of overwriting",
" -d offset	Set initial directory offset, counting first image as one, not zero",
" -p contig	Pack samples contiguously (e.g. RGBRGB...)",
" -p separate	Store samples separately (e.g. RRR...GGG...BBB...)",
" -s		Write output in strips",
" -t		Write output in tiles",
" -i		Ignore read errors",
" ",
" -r #		Make each strip have no more than # rows",
" -w #		Set output tile width (pixels)",
" -l #		Set output tile length (pixels)",
" ",
" -f lsb2msb	Force lsb-to-msb FillOrder for output",
" -f msb2lsb	Force msb-to-lsb FillOrder for output",
"",
" -c lzw[:opts]	 Compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	 Compress output with deflate encoding",
" -c jpeg[:opts] Compress output with JPEG encoding",
" -c packbits	 Compress output with packbits encoding",
" -c g3[:opts]	 Compress output with CCITT Group 3 encoding",
" -c g4		 Compress output with CCITT Group 4 encoding",
" -c none	 Use no compression algorithm on output",
" ",
"Group 3 options:",
" 1d		Use default CCITT Group 3 1D-encoding",
" 2d		Use optional CCITT Group 3 2D-encoding",
" fill		Byte-align EOL codes",
"For example, -c g3:2d:fill to get G3-2D-encoded data with byte-aligned EOLs",
" ",
"JPEG options:",
" #		Set compression quality level (0-100, default 100)",
" raw		Output color image as raw YCbCr",
" rgb		Output color image as RGB",
"For example, -c jpeg:rgb:50 to get JPEG-encoded RGB data with 50% comp. quality",
" ",
"LZW and deflate options:",
" #		Set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
" ",
"Page and selection options:",
" -N odd|even|#,#-#,#|last         sequences and ranges of images within file to process",
"             The words odd or even may be used to specify all odd or even numbered images.",
"             The word last may be used in place of a number in the sequence to indicate.",
"             The final image in the file without knowing how many images there are.",
"             Numbers are counted from one even though TIFF IFDs are counted from zero.",
" ",
" -E t|l|r|b  edge to use as origin for width and length of crop region",
" -U units    [in, cm, px ] inches, centimeters or pixels",
" ",
" -m #,#,#,#  margins from edges for selection: top, left, bottom, right separated by commas",
" -X #        horizontal dimension of region to extract expressed in current units",
" -Y #        vertical dimension of region to extract expressed in current units",
" -Z #:#,#:#  zones of the image designated as position X of Y,",
"             eg 1:3 would be first of three equal portions measured from reference edge",
" -z x1,y1,x2,y2:...:xN,yN,xN+1,yN+1",
"             regions of the image designated by upper left and lower right coordinates",
"",
"Export grouping options:",
" -e c|d|i|m|s    export mode for images and selections from input images.",
"                 When exporting a composite image from multiple zones or regions",
"                 (combined and image modes), the selections must have equal sizes",
"                 for the axis perpendicular to the edge specified with -E.",
"    c|combined   All images and selections are written to a single file (default).",
"                 with multiple selections from one image combined into a single image.",
"    d|divided    All images and selections are written to a single file",
"                 with each selection from one image written to a new image.",
"    i|image      Each input image is written to a new file (numeric filename sequence)",
"                 with multiple selections from the image combined into one image.",
"    m|multiple   Each input image is written to a new file (numeric filename sequence)",
"                 with each selection from the image written to a new image.",
"    s|separated  Individual selections from each image are written to separate files.",
"",
"Output options:",
" -H #        Set horizontal resolution of output images to #",
" -V #        Set vertical resolution of output images to #",
" -J #        Set horizontal margin of output page to # expressed in current units",
"             when sectioning image into columns x rows using the -S cols:rows option",
" -K #        Set verticalal margin of output page to # expressed in current units",
"             when sectioning image into columns x rows using the -S cols:rows option",
" ",
" -O orient    orientation for output image, portrait, landscape, auto",
" -P page      page size for output image segments, eg letter, legal, tabloid, etc",
"              use #.#x#.# to specify a custom page size in the currently defined units",
"              where #.# represents the width and length",        
" -S cols:rows Divide the image into equal sized segments using cols across and rows down.",
" ",
" -F hor|vert|both",
"             flip (mirror) image or region horizontally, vertically, or both",
" -R #        [90,180,or 270] degrees clockwise rotation of image or extracted region",
" -I [black|white|data|both]",
"             invert color space, eg dark to light for bilevel and grayscale images",
"             If argument is white or black, set the PHOTOMETRIC_INTERPRETATION ",
"             tag to MinIsBlack or MinIsWhite without altering the image data",
"             If the argument is data or both, the image data are modified:",
"             both inverts the data and the PHOTOMETRIC_INTERPRETATION tag,",
"             data inverts the data but not the PHOTOMETRIC_INTERPRETATION tag",
" ",
"-D opt1:value1,opt2:value2,opt3:value3:opt4:value4",
"             Debug/dump program progress and/or data to non-TIFF files.",
"             Options include the following and must be joined as a comma",
"             separate list. The use of this option is generally limited to",
"             program debugging and development of future options.",
" ",
"   debug:N   Display limited program progress indicators where larger N",
"             increase the level of detail. Note: Tiffcrop may be compiled with",
"             -DDEVELMODE to enable additional very low level debug reporting.",
"",
"   Format:txt|raw  Format any logged data as ASCII text or raw binary ",
"             values. ASCII text dumps include strings of ones and zeroes",
"             representing the binary values in the image data plus identifying headers.",
" ",
"   level:N   Specify the level of detail presented in the dump files.",
"             This can vary from dumps of the entire input or output image data to dumps",
"             of data processed by specific functions. Current range of levels is 1 to 3.",
" ",
"   input:full-path-to-directory/input-dumpname",
" ",
"   output:full-path-to-directory/output-dumpnaem",
" ",
"             When dump files are being written, each image will be written to a separate",
"             file with the name built by adding a numeric sequence value to the dumpname",
"             and an extension of .txt for ASCII dumps or .bin for binary dumps.",
" ",
"             The four debug/dump options are independent, though it makes little sense to",
"             specify a dump file without specifying a detail level.",
" ",
NULL
};

/* This function could be modified to pass starting sample offset 
 * and number of samples as args to select fewer than spp
 * from input image. These would then be passed to individual 
 * extractContigSampleXX routines.
 */
static int readContigTilesIntoBuffer (TIFF* in, uint8* buf, 
                                      uint32 imagelength, 
                                      uint32 imagewidth, 
                                      uint32 tw, uint32 tl,
                                      tsample_t spp, uint16 bps)
  {
  int status = 1;
  tsample_t sample = 0;
  tsample_t count = spp; 
  uint32 row, col, trow;
  uint32 nrow, ncol;
  uint32 dst_rowsize, shift_width;
  uint32 bytes_per_sample, bytes_per_pixel;
  uint32 trailing_bits, prev_trailing_bits;
  uint32 tile_rowsize  = TIFFTileRowSize(in);
  uint32 src_offset, dst_offset;
  uint32 row_offset, col_offset;
  uint8 *bufp = (uint8*) buf;
  unsigned char *src = NULL;
  unsigned char *dst = NULL;
  tsize_t tbytes = 0, tile_buffsize = 0;
  tsize_t tilesize = TIFFTileSize(in);
  unsigned char *tilebuf = NULL;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;

  if ((bps % 8) == 0)
    shift_width = 0;
  else
    {
    if (bytes_per_pixel < (bytes_per_sample + 1))
      shift_width = bytes_per_pixel;
    else
      shift_width = bytes_per_sample + 1;
    }

  tile_buffsize = tilesize;

  if (tilesize < (tsize_t)(tl * tile_rowsize))
    {
#ifdef DEBUG2
    TIFFError("readContigTilesIntoBuffer",
	      "Tilesize %lu is too small, using alternate calculation %u",
              tilesize, tl * tile_rowsize);
#endif
    tile_buffsize = tl * tile_rowsize;
    } 

  tilebuf = _TIFFmalloc(tile_buffsize);
  if (tilebuf == 0)
    return 0;

  dst_rowsize = ((imagewidth * bps * spp) + 7) / 8;  
  for (row = 0; row < imagelength; row += tl)
    {
    nrow = (row + tl > imagelength) ? imagelength - row : tl;
    for (col = 0; col < imagewidth; col += tw)
      {
      tbytes = TIFFReadTile(in, tilebuf, col, row, 0, 0);
      if (tbytes < tilesize  && !ignore)
        {
	TIFFError(TIFFFileName(in),
		  "Error, can't read tile at row %lu col %lu, Read %lu bytes of %lu",
		  (unsigned long) col, (unsigned long) row, (unsigned long)tbytes,
                  (unsigned long)tilesize);
		  status = 0;
                  _TIFFfree(tilebuf);
		  return status;
	}
      
      row_offset = row * dst_rowsize;
      col_offset = ((col * bps * spp) + 7)/ 8;
      bufp = buf + row_offset + col_offset;

      if (col + tw > imagewidth)
	ncol = imagewidth - col;
      else
        ncol = tw;

      /* Each tile scanline will start on a byte boundary but it
       * has to be merged into the scanline for the entire
       * image buffer and the previous segment may not have
       * ended on a byte boundary
       */
      /* Optimization for common bit depths, all samples */
      if (((bps % 8) == 0) && (count == spp))
        {
	for (trow = 0; trow < nrow; trow++)
          {
	  src_offset = trow * tile_rowsize;
	  _TIFFmemcpy (bufp, tilebuf + src_offset, (ncol * spp * bps) / 8);
          bufp += (imagewidth * bps * spp) / 8;
	  }
        }
      else
        {
	/* Bit depths not a multiple of 8 and/or extract fewer than spp samples */
        prev_trailing_bits = trailing_bits = 0;
        trailing_bits = (ncol * bps * spp) % 8;

	/*	for (trow = 0; tl < nrow; trow++) */
	for (trow = 0; trow < nrow; trow++)
          {
	  src_offset = trow * tile_rowsize;
          src = tilebuf + src_offset;
	  dst_offset = (row + trow) * dst_rowsize;
          dst = buf + dst_offset + col_offset;
          switch (shift_width)
            {
            case 0: if (extractContigSamplesBytes (src, dst, ncol, sample,
                                                   spp, bps, count, 0, ncol))
                      {
		      TIFFError("readContigTilesIntoBuffer",
                                "Unable to extract row %d from tile %lu", 
				row, (unsigned long)TIFFCurrentTile(in));
		      return 1;
		      }
		    break;
            case 1: if (bps == 1)
                      { 
                      if (extractContigSamplesShifted8bits (src, dst, ncol,
                                                            sample, spp,
                                                            bps, count,
                                                            0, ncol,
                                                            prev_trailing_bits))
                        {
		        TIFFError("readContigTilesIntoBuffer",
                                  "Unable to extract row %d from tile %lu", 
				  row, (unsigned long)TIFFCurrentTile(in));
		        return 1;
		        }
		      break;
		      }
                    else
                      if (extractContigSamplesShifted16bits (src, dst, ncol,
                                                             sample, spp,
                                                             bps, count,
                                                             0, ncol,
                                                             prev_trailing_bits))
                        {
		        TIFFError("readContigTilesIntoBuffer",
                                  "Unable to extract row %d from tile %lu", 
			  	  row, (unsigned long)TIFFCurrentTile(in));
		        return 1;
		        }
	            break;
            case 2: if (extractContigSamplesShifted24bits (src, dst, ncol,
                                                           sample, spp,
                                                           bps, count,
                                                           0, ncol,
                                                           prev_trailing_bits))
                      {
		      TIFFError("readContigTilesIntoBuffer",
                                "Unable to extract row %d from tile %lu", 
		  	        row, (unsigned long)TIFFCurrentTile(in));
		      return 1;
		      }
		    break;
            case 3:
            case 4:
            case 5: if (extractContigSamplesShifted32bits (src, dst, ncol,
                                                           sample, spp,
                                                           bps, count,
                                                           0, ncol,
                                                           prev_trailing_bits))
                      {
		      TIFFError("readContigTilesIntoBuffer",
                                "Unable to extract row %d from tile %lu", 
			        row, (unsigned long)TIFFCurrentTile(in));
		      return 1;
		      }
		    break;
            default: TIFFError("readContigTilesIntoBuffer", "Unsupported bit depth %d", bps);
		     return 1;
	    }
          }
        prev_trailing_bits += trailing_bits;
        if (prev_trailing_bits > 7)
	  prev_trailing_bits-= 8;
	}
      }
    }

  _TIFFfree(tilebuf);
  return status;
  }

static int  readSeparateTilesIntoBuffer (TIFF* in, uint8 *obuf, 
					 uint32 imagelength, uint32 imagewidth, 
                                         uint32 tw, uint32 tl,
                                         uint16 spp, uint16 bps)
  {
  int     i, status = 1, sample;
  int     shift_width, bytes_per_pixel;
  uint16  bytes_per_sample;
  uint32  row, col;     /* Current row and col of image */
  uint32  nrow, ncol;   /* Number of rows and cols in current tile */
  uint32  row_offset, col_offset; /* Output buffer offsets */
  tsize_t tbytes = 0, tilesize = TIFFTileSize(in);
  tsample_t s;
  uint8*  bufp = (uint8*)obuf;
  unsigned char *srcbuffs[MAX_SAMPLES];
  unsigned char *tbuff = NULL;

  bytes_per_sample = (bps + 7) / 8;

  for (sample = 0; (sample < spp) && (sample < MAX_SAMPLES); sample++)
    {
    srcbuffs[sample] = NULL;
    tbuff = (unsigned char *)_TIFFmalloc(tilesize + 8);
    if (!tbuff)
      {
      TIFFError ("readSeparateTilesIntoBuffer", 
                 "Unable to allocate tile read buffer for sample %d", sample);
      for (i = 0; i < sample; i++)
        _TIFFfree (srcbuffs[i]);
      return 0;
      }
    srcbuffs[sample] = tbuff;
    } 
  /* Each tile contains only the data for a single plane
   * arranged in scanlines of tw * bytes_per_sample bytes.
   */
  for (row = 0; row < imagelength; row += tl)
    {
    nrow = (row + tl > imagelength) ? imagelength - row : tl;
    for (col = 0; col < imagewidth; col += tw)
      {
      for (s = 0; s < spp; s++)
        {  /* Read each plane of a tile set into srcbuffs[s] */
	tbytes = TIFFReadTile(in, srcbuffs[s], col, row, 0, s);
        if (tbytes < 0  && !ignore)
          {
	  TIFFError(TIFFFileName(in),
                 "Error, can't read tile for row %lu col %lu, "
		 "sample %lu",
		 (unsigned long) col, (unsigned long) row,
		 (unsigned long) s);
		 status = 0;
          for (sample = 0; (sample < spp) && (sample < MAX_SAMPLES); sample++)
            {
            tbuff = srcbuffs[sample];
            if (tbuff != NULL)
              _TIFFfree(tbuff);
            }
          return status;
	  }
	}
     /* Tiles on the right edge may be padded out to tw 
      * which must be a multiple of 16.
      * Ncol represents the visible (non padding) portion.  
      */
      if (col + tw > imagewidth)
        ncol = imagewidth - col;
      else
        ncol = tw;

      row_offset = row * (((imagewidth * spp * bps) + 7) / 8);
      col_offset = ((col * spp * bps) + 7) / 8;
      bufp = obuf + row_offset + col_offset;

      if ((bps % 8) == 0)
        {
        if (combineSeparateTileSamplesBytes(srcbuffs, bufp, ncol, nrow, imagewidth,
					    tw, spp, bps, NULL, 0, 0))
	  {
          status = 0;
          break;
      	  }
	}
      else
        {
        bytes_per_pixel  = ((bps * spp) + 7) / 8;
        if (bytes_per_pixel < (bytes_per_sample + 1))
          shift_width = bytes_per_pixel;
        else
          shift_width = bytes_per_sample + 1;

        switch (shift_width)
          {
          case 1: if (combineSeparateTileSamples8bits (srcbuffs, bufp, ncol, nrow,
                                                       imagewidth, tw, spp, bps, 
						       NULL, 0, 0))
	            {
                    status = 0;
                    break;
      	            }
	          break;
          case 2: if (combineSeparateTileSamples16bits (srcbuffs, bufp, ncol, nrow,
                                                       imagewidth, tw, spp, bps, 
						       NULL, 0, 0))
	            {
                    status = 0;
                    break;
		    }
	          break;
          case 3: if (combineSeparateTileSamples24bits (srcbuffs, bufp, ncol, nrow,
                                                       imagewidth, tw, spp, bps, 
						       NULL, 0, 0))
	            {
                    status = 0;
                    break;
       	            }
                  break;
          case 4: 
          case 5:
          case 6:
          case 7:
          case 8: if (combineSeparateTileSamples32bits (srcbuffs, bufp, ncol, nrow,
                                                       imagewidth, tw, spp, bps, 
						       NULL, 0, 0))
	            {
                    status = 0;
                    break;
		    }
	          break;
          default: TIFFError ("readSeparateTilesIntoBuffer", "Unsupported bit depth: %d", bps);
                  status = 0;
                  break;
          }
        }
      }
    }

  for (sample = 0; (sample < spp) && (sample < MAX_SAMPLES); sample++)
    {
    tbuff = srcbuffs[sample];
    if (tbuff != NULL)
      _TIFFfree(tbuff);
    }
 
  return status;
  }

static int writeBufferToContigStrips(TIFF* out, uint8* buf, uint32 imagelength)
  {
  uint32 row, nrows, rowsperstrip;
  tstrip_t strip = 0;
  tsize_t stripsize;

  TIFFGetFieldDefaulted(out, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
  for (row = 0; row < imagelength; row += rowsperstrip)
    {
    nrows = (row + rowsperstrip > imagelength) ?
	     imagelength - row : rowsperstrip;
    stripsize = TIFFVStripSize(out, nrows);
    if (TIFFWriteEncodedStrip(out, strip++, buf, stripsize) < 0)
      {
      TIFFError(TIFFFileName(out), "Error, can't write strip %u", strip - 1);
      return 1;
      }
    buf += stripsize;
    }

  return 0;
  }

/* Abandon plans to modify code so that plannar orientation separate images
 * do not have all samples for each channel written before all samples
 * for the next channel have been abandoned.
 * Libtiff internals seem to depend on all data for a given sample
 * being contiguous within a strip or tile when PLANAR_CONFIG is 
 * separate. All strips or tiles of a given plane are written
 * before any strips or tiles of a different plane are stored.
 */
static int 
writeBufferToSeparateStrips (TIFF* out, uint8* buf, 
			     uint32 length, uint32 width, uint16 spp,
			     struct dump_opts *dump)
  {
  uint8   *src;
  uint16   bps;
  uint32   row, nrows, rowsize, rowsperstrip;
  uint32   bytes_per_sample;
  tsample_t s;
  tstrip_t strip = 0;
  tsize_t  stripsize = TIFFStripSize(out);
  tsize_t  rowstripsize,  scanlinesize = TIFFScanlineSize(out);
  tsize_t  total_bytes = 0;
  tdata_t  obuf;

  (void) TIFFGetFieldDefaulted(out, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
  (void) TIFFGetField(out, TIFFTAG_BITSPERSAMPLE, &bps);
  bytes_per_sample = (bps + 7) / 8;
  rowsize = ((bps * spp * width) + 7) / 8; /* source has interleaved samples */
  rowstripsize = rowsperstrip * bytes_per_sample * (width + 1); 

  obuf = _TIFFmalloc (rowstripsize);
  if (obuf == NULL)
    return 1;
  
  for (s = 0; s < spp; s++)
    {
    for (row = 0; row < length; row += rowsperstrip)
      {
      nrows = (row + rowsperstrip > length) ? length - row : rowsperstrip;

      stripsize = TIFFVStripSize(out, nrows);
      src = buf + (row * rowsize);
      total_bytes += stripsize;
      memset (obuf, '\0', rowstripsize);
      if (extractContigSamplesToBuffer(obuf, src, nrows, width, s, spp, bps, dump))
        {
        _TIFFfree(obuf);
        return 1;
	}
      if ((dump->outfile != NULL) && (dump->level == 1))
        {
        dump_info(dump->outfile, dump->format,"", 
                  "Sample %2d, Strip: %2d, bytes: %4d, Row %4d, bytes: %4d, Input offset: %6d", 
                  s + 1, strip + 1, stripsize, row + 1, scanlinesize, src - buf);
        dump_buffer(dump->outfile, dump->format, nrows, scanlinesize, row, obuf);
	}

      if (TIFFWriteEncodedStrip(out, strip++, obuf, stripsize) < 0)
        {
	TIFFError(TIFFFileName(out), "Error, can't write strip %u", strip - 1);
	_TIFFfree(obuf);
	return 1;
	}
      }
    }      

  _TIFFfree(obuf);
  return 0;
}

/* Extract all planes from contiguous buffer into a single tile buffer 
 * to be written out as a tile.
 */
static int writeBufferToContigTiles (TIFF* out, uint8* buf, uint32 imagelength,
				       uint32 imagewidth, tsample_t spp, 
                                       struct dump_opts* dump)
  {
  uint16 bps;
  uint32 tl, tw;
  uint32 row, col, nrow, ncol;
  uint32 src_rowsize, col_offset;
  uint32 tile_rowsize  = TIFFTileRowSize(out);
  uint8* bufp = (uint8*) buf;
  tsize_t tile_buffsize = 0;
  tsize_t tilesize = TIFFTileSize(out);
  unsigned char *tilebuf = NULL;

  TIFFGetField(out, TIFFTAG_TILELENGTH, &tl);
  TIFFGetField(out, TIFFTAG_TILEWIDTH, &tw);
  TIFFGetField(out, TIFFTAG_BITSPERSAMPLE, &bps);

  tile_buffsize = tilesize;
  if (tilesize < (tsize_t)(tl * tile_rowsize))
    {
#ifdef DEBUG2
    TIFFError("writeBufferToContigTiles",
	      "Tilesize %lu is too small, using alternate calculation %u",
              tilesize, tl * tile_rowsize);
#endif
    tile_buffsize = tl * tile_rowsize;
    }

  tilebuf = _TIFFmalloc(tile_buffsize);
  if (tilebuf == 0)
    return 1;

  src_rowsize = ((imagewidth * spp * bps) + 7) / 8;
  for (row = 0; row < imagelength; row += tl)
    {
    nrow = (row + tl > imagelength) ? imagelength - row : tl;
    for (col = 0; col < imagewidth; col += tw)
      {
      /* Calculate visible portion of tile. */
      if (col + tw > imagewidth)
	ncol = imagewidth - col;
      else
        ncol = tw;

      col_offset = (((col * bps * spp) + 7) / 8);
      bufp = buf + (row * src_rowsize) + col_offset;
      if (extractContigSamplesToTileBuffer(tilebuf, bufp, nrow, ncol, imagewidth,
					   tw, 0, spp, spp, bps, dump) > 0)
        {
	TIFFError("writeBufferToContigTiles", 
                  "Unable to extract data to tile for row %lu, col %lu",
                  (unsigned long) row, (unsigned long)col);
	_TIFFfree(tilebuf);
	return 1;
        }

      if (TIFFWriteTile(out, tilebuf, col, row, 0, 0) < 0)
        {
	TIFFError("writeBufferToContigTiles",
	          "Cannot write tile at %lu %lu",
	          (unsigned long) col, (unsigned long) row);
	 _TIFFfree(tilebuf);
	return 1;
	}
      }
    }
  _TIFFfree(tilebuf);

  return 0;
  } /* end writeBufferToContigTiles */

/* Extract each plane from contiguous buffer into a single tile buffer 
 * to be written out as a tile.
 */
static int writeBufferToSeparateTiles (TIFF* out, uint8* buf, uint32 imagelength,
				       uint32 imagewidth, tsample_t spp, 
                                       struct dump_opts * dump)
  {
  tdata_t obuf = _TIFFmalloc(TIFFTileSize(out));
  uint32 tl, tw;
  uint32 row, col, nrow, ncol;
  uint32 src_rowsize, col_offset;
  uint16 bps;
  tsample_t s;
  uint8* bufp = (uint8*) buf;

  if (obuf == NULL)
    return 1;

  TIFFGetField(out, TIFFTAG_TILELENGTH, &tl);
  TIFFGetField(out, TIFFTAG_TILEWIDTH, &tw);
  TIFFGetField(out, TIFFTAG_BITSPERSAMPLE, &bps);
  src_rowsize = ((imagewidth * spp * bps) + 7) / 8;
         
  for (row = 0; row < imagelength; row += tl)
    {
    nrow = (row + tl > imagelength) ? imagelength - row : tl;
    for (col = 0; col < imagewidth; col += tw)
      {
      /* Calculate visible portion of tile. */
      if (col + tw > imagewidth)
	ncol = imagewidth - col;
      else
        ncol = tw;

      col_offset = (((col * bps * spp) + 7) / 8);
      bufp = buf + (row * src_rowsize) + col_offset;

      for (s = 0; s < spp; s++)
        {
	if (extractContigSamplesToTileBuffer(obuf, bufp, nrow, ncol, imagewidth,
					     tw, s, 1, spp, bps, dump) > 0)
          {
	  TIFFError("writeBufferToSeparateTiles", 
                    "Unable to extract data to tile for row %lu, col %lu sample %d",
                    (unsigned long) row, (unsigned long)col, (int)s);
	  _TIFFfree(obuf);
	  return 1;
          }

	if (TIFFWriteTile(out, obuf, col, row, 0, s) < 0)
          {
	   TIFFError("writeBufferToseparateTiles",
	             "Cannot write tile at %lu %lu sample %lu",
	             (unsigned long) col, (unsigned long) row,
	             (unsigned long) s);
	   _TIFFfree(obuf);
	   return 1;
	  }
	}
      }
    }
  _TIFFfree(obuf);

  return 0;
  } /* end writeBufferToSeparateTiles */

static void
processG3Options(char* cp)
{
	if( (cp = strchr(cp, ':')) ) {
		if (defg3opts == (uint32) -1)
			defg3opts = 0;
		do {
			cp++;
			if (strneq(cp, "1d", 2))
				defg3opts &= ~GROUP3OPT_2DENCODING;
			else if (strneq(cp, "2d", 2))
				defg3opts |= GROUP3OPT_2DENCODING;
			else if (strneq(cp, "fill", 4))
				defg3opts |= GROUP3OPT_FILLBITS;
			else
				usage();
		} while( (cp = strchr(cp, ':')) );
	}
}

static int
processCompressOptions(char* opt)
  {
  char* cp = NULL;

  if (strneq(opt, "none",4))
    {
    defcompression = COMPRESSION_NONE;
    }
  else if (streq(opt, "packbits"))
    {
    defcompression = COMPRESSION_PACKBITS;
    }
  else if (strneq(opt, "jpeg", 4))
    {
    cp = strchr(opt, ':');
    defcompression = COMPRESSION_JPEG;

    while (cp)
      {
      if (isdigit((int)cp[1]))
	quality = atoi(cp + 1);
      else if (strneq(cp + 1, "raw", 3 ))
	jpegcolormode = JPEGCOLORMODE_RAW;
      else if (strneq(cp + 1, "rgb", 3 ))
	jpegcolormode = JPEGCOLORMODE_RGB;
      else
	usage();
      cp = strchr(cp + 1, ':');
      }
    }
  else if (strneq(opt, "g3", 2))
    {
    processG3Options(opt);
    defcompression = COMPRESSION_CCITTFAX3;
    }
  else if (streq(opt, "g4"))
    {
    defcompression = COMPRESSION_CCITTFAX4;
    }
  else if (strneq(opt, "lzw", 3))
    {
    cp = strchr(opt, ':');
    if (cp)
      defpredictor = atoi(cp+1);
    defcompression = COMPRESSION_LZW;
    }
  else if (strneq(opt, "zip", 3))
    {
    cp = strchr(opt, ':');
    if (cp)
      defpredictor = atoi(cp+1);
    defcompression = COMPRESSION_ADOBE_DEFLATE;
   }
  else
    return (0);

  return (1);
  }

static void
usage(void)
  {
  int i;

  fprintf(stderr, "\n%s\n", TIFFGetVersion());
  for (i = 0; usage_info[i] != NULL; i++)
    fprintf(stderr, "%s\n", usage_info[i]);
  exit(-1);
  }

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)
#define	CopyField4(tag, v1, v2, v3, v4) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3, &v4)) TIFFSetField(out, tag, v1, v2, v3, v4)

static void
cpTag(TIFF* in, TIFF* out, uint16 tag, uint16 count, TIFFDataType type)
{
	switch (type) {
	case TIFF_SHORT:
		if (count == 1) {
			uint16 shortv;
			CopyField(tag, shortv);
		} else if (count == 2) {
			uint16 shortv1, shortv2;
			CopyField2(tag, shortv1, shortv2);
		} else if (count == 4) {
			uint16 *tr, *tg, *tb, *ta;
			CopyField4(tag, tr, tg, tb, ta);
		} else if (count == (uint16) -1) {
			uint16 shortv1;
			uint16* shortav;
			CopyField2(tag, shortv1, shortav);
		}
		break;
	case TIFF_LONG:
		{ uint32 longv;
		  CopyField(tag, longv);
		}
		break;
	case TIFF_RATIONAL:
		if (count == 1) {
			float floatv;
			CopyField(tag, floatv);
		} else if (count == (uint16) -1) {
			float* floatav;
			CopyField(tag, floatav);
		}
		break;
	case TIFF_ASCII:
		{ char* stringv;
		  CopyField(tag, stringv);
		}
		break;
	case TIFF_DOUBLE:
		if (count == 1) {
			double doublev;
			CopyField(tag, doublev);
		} else if (count == (uint16) -1) {
			double* doubleav;
			CopyField(tag, doubleav);
		}
		break;
          default:
                TIFFError(TIFFFileName(in),
                          "Data type %d is not supported, tag %d skipped",
                          tag, type);
	}
}

static struct cpTag {
	uint16	tag;
	uint16	count;
	TIFFDataType type;
} tags[] = {
	{ TIFFTAG_SUBFILETYPE,		1, TIFF_LONG },
	{ TIFFTAG_THRESHHOLDING,	1, TIFF_SHORT },
	{ TIFFTAG_DOCUMENTNAME,		1, TIFF_ASCII },
	{ TIFFTAG_IMAGEDESCRIPTION,	1, TIFF_ASCII },
	{ TIFFTAG_MAKE,			1, TIFF_ASCII },
	{ TIFFTAG_MODEL,		1, TIFF_ASCII },
	{ TIFFTAG_MINSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_MAXSAMPLEVALUE,	1, TIFF_SHORT },
	{ TIFFTAG_XRESOLUTION,		1, TIFF_RATIONAL },
	{ TIFFTAG_YRESOLUTION,		1, TIFF_RATIONAL },
	{ TIFFTAG_PAGENAME,		1, TIFF_ASCII },
	{ TIFFTAG_XPOSITION,		1, TIFF_RATIONAL },
	{ TIFFTAG_YPOSITION,		1, TIFF_RATIONAL },
	{ TIFFTAG_RESOLUTIONUNIT,	1, TIFF_SHORT },
	{ TIFFTAG_SOFTWARE,		1, TIFF_ASCII },
	{ TIFFTAG_DATETIME,		1, TIFF_ASCII },
	{ TIFFTAG_ARTIST,		1, TIFF_ASCII },
	{ TIFFTAG_HOSTCOMPUTER,		1, TIFF_ASCII },
	{ TIFFTAG_WHITEPOINT,		(uint16) -1, TIFF_RATIONAL },
	{ TIFFTAG_PRIMARYCHROMATICITIES,(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_HALFTONEHINTS,	2, TIFF_SHORT },
	{ TIFFTAG_INKSET,		1, TIFF_SHORT },
	{ TIFFTAG_DOTRANGE,		2, TIFF_SHORT },
	{ TIFFTAG_TARGETPRINTER,	1, TIFF_ASCII },
	{ TIFFTAG_SAMPLEFORMAT,		1, TIFF_SHORT },
	{ TIFFTAG_YCBCRCOEFFICIENTS,	(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_YCBCRSUBSAMPLING,	2, TIFF_SHORT },
	{ TIFFTAG_YCBCRPOSITIONING,	1, TIFF_SHORT },
	{ TIFFTAG_REFERENCEBLACKWHITE,	(uint16) -1,TIFF_RATIONAL },
	{ TIFFTAG_EXTRASAMPLES,		(uint16) -1, TIFF_SHORT },
	{ TIFFTAG_SMINSAMPLEVALUE,	1, TIFF_DOUBLE },
	{ TIFFTAG_SMAXSAMPLEVALUE,	1, TIFF_DOUBLE },
	{ TIFFTAG_STONITS,		1, TIFF_DOUBLE },
};
#define	NTAGS	(sizeof (tags) / sizeof (tags[0]))

#define	CopyTag(tag, count, type)	cpTag(in, out, tag, count, type)

/* Functions written by Richard Nolde, with exceptions noted. */
void  process_command_opts (int argc, char *argv[], char *mp, char *mode, uint32 *dirnum,
	                    uint16 *defconfig, uint16 *deffillorder, uint32 *deftilewidth,
                            uint32 *deftilelength, uint32 *defrowsperstrip,
		            struct crop_mask *crop_data, struct pagedef *page, 
                            struct dump_opts *dump,
                            unsigned int     *imagelist, unsigned int   *image_count )
    {
    int   c, good_args = 0;
    char *opt_offset   = NULL;    /* Position in string of value sought */
    char *opt_ptr      = NULL;    /* Pointer to next token in option set */
    char *sep          = NULL;    /* Pointer to a token separator */
    unsigned int  i, j, start, end;
    extern int   optind;
    extern char* optarg;

    *mp++ = 'w';
    *mp = '\0';
    while ((c = getopt(argc, argv,
       "ac:d:e:f:hil:m:p:r:stvw:z:BCD:E:F:H:I:J:K:LMN:O:P:R:S:U:V:X:Y:Z:")) != -1)
      {
    good_args++;
    switch (c) {
      case 'a': mode[0] = 'a';	/* append to output */
		break;
      case 'c':	if (!processCompressOptions(optarg)) /* compression scheme */
		  {
		  TIFFError ("Unknown compression option", "%s", optarg);
                  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);
                  }
		break;
      case 'd':	start = strtoul(optarg, NULL, 0); /* initial IFD offset */
	        if (start == 0)
                  {
		  TIFFError ("","Directory offset must be greater than zero");
		  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);
		  }
	        *dirnum = start - 1;
		break;
      case 'e': switch (tolower(optarg[0])) /* image export modes*/
                  {
		  case 'c': crop_data->exp_mode = ONE_FILE_COMPOSITE;
 		            crop_data->img_mode = COMPOSITE_IMAGES;
		            break; /* Composite */
		  case 'd': crop_data->exp_mode = ONE_FILE_SEPARATED;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Divided */
		  case 'i': crop_data->exp_mode = FILE_PER_IMAGE_COMPOSITE;
 		            crop_data->img_mode = COMPOSITE_IMAGES;
		            break; /* Image */
		  case 'm': crop_data->exp_mode = FILE_PER_IMAGE_SEPARATED;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Multiple */
		  case 's': crop_data->exp_mode = FILE_PER_SELECTION;
 		            crop_data->img_mode = SEPARATED_IMAGES;
		            break; /* Sections */
		  default:  TIFFError ("Unknown export mode","%s", optarg);
                            TIFFError ("For valid options type", "tiffcrop -h");
                            exit (-1);
                  }
	        break;
      case 'f':	if (streq(optarg, "lsb2msb"))	   /* fill order */
		  *deffillorder = FILLORDER_LSB2MSB;
		else if (streq(optarg, "msb2lsb"))
		  *deffillorder = FILLORDER_MSB2LSB;
		else
		  {
		  TIFFError ("Unknown fill order", "%s", optarg);
                  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);
                  }
		break;
      case 'h':	usage();
		break;
      case 'i':	ignore = TRUE;		/* ignore errors */
		break;
      case 'l':	outtiled = TRUE;	 /* tile length */
		*deftilelength = atoi(optarg);
		break;
      case 'p': /* planar configuration */
		if (streq(optarg, "separate"))
		  *defconfig = PLANARCONFIG_SEPARATE;
		else if (streq(optarg, "contig"))
		  *defconfig = PLANARCONFIG_CONTIG;
		else
		  {
		  TIFFError ("Unkown planar configuration", "%s", optarg);
                  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);
                  }
		break;
      case 'r':	/* rows/strip */
		*defrowsperstrip = atol(optarg);
		break;
      case 's':	/* generate stripped output */
		outtiled = FALSE;
		break;
      case 't':	/* generate tiled output */
		outtiled = TRUE;
		break;
      case 'v': TIFFError("Library Release", "%s", TIFFGetVersion());
                TIFFError ("Tiffcrop version", "%s, last updated: %s", 
			   tiffcrop_version_id, tiffcrop_rev_date);
 	        TIFFError ("Tiffcp code", "Copyright (c) 1988-1997 Sam Leffler");
		TIFFError ("           ", "Copyright (c) 1991-1997 Silicon Graphics, Inc");
                TIFFError ("Tiffcrop additions", "Copyright (c) 2007-2010 Richard Nolde");
	        exit (0);
		break;
      case 'w':	/* tile width */
		outtiled = TRUE;
		*deftilewidth = atoi(optarg);
		break;
      case 'z': /* regions of an image specified as x1,y1,x2,y2:x3,y3,x4,y4 etc */
	        crop_data->crop_mode |= CROP_REGIONS;
		for (i = 0, opt_ptr = strtok (optarg, ":");
                   ((opt_ptr != NULL) &&  (i < MAX_REGIONS));
                    (opt_ptr = strtok (NULL, ":")), i++)
                    {
		    crop_data->regions++;
                    if (sscanf(opt_ptr, "%lf,%lf,%lf,%lf",
			       &crop_data->corners[i].X1, &crop_data->corners[i].Y1,
			       &crop_data->corners[i].X2, &crop_data->corners[i].Y2) != 4)
                      {
                      TIFFError ("Unable to parse coordinates for region", "%d %s", i, optarg);
		      TIFFError ("For valid options type", "tiffcrop -h");
                      exit (-1);
		      }
                    }
                /*  check for remaining elements over MAX_REGIONS */
                if ((opt_ptr != NULL) && (i >= MAX_REGIONS))
                  {
		  TIFFError ("Region list exceeds limit of", "%d regions %s", MAX_REGIONS, optarg);
		  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);;
                  }
		break;
      /* options for file open modes */
      case 'B': *mp++ = 'b'; *mp = '\0';
		break;
      case 'L': *mp++ = 'l'; *mp = '\0';
		break;
      case 'M': *mp++ = 'm'; *mp = '\0';
		break;
      case 'C': *mp++ = 'c'; *mp = '\0';
		break;
      /* options for Debugging / data dump */
      case 'D': for (i = 0, opt_ptr = strtok (optarg, ",");
                    (opt_ptr != NULL);
                    (opt_ptr = strtok (NULL, ",")), i++)
                    {
		    opt_offset = strpbrk(opt_ptr, ":=");
                    if (opt_offset == NULL)
                      {
                      TIFFError("Invalid dump option", "%s", optarg);
                      TIFFError ("For valid options type", "tiffcrop -h");
                      exit (-1);
		      }
                      
                    *opt_offset = '\0';
                    /* convert option to lowercase */
                    end = strlen (opt_ptr);
                    for (i = 0; i < end; i++)
                      *(opt_ptr + i) = tolower(*(opt_ptr + i));
                    /* Look for dump format specification */
                    if (strncmp(opt_ptr, "for", 3) == 0)
                      {
		      /* convert value to lowercase */
                      end = strlen (opt_offset + 1);
                      for (i = 1; i <= end; i++)
                        *(opt_offset + i) = tolower(*(opt_offset + i));
                      /* check dump format value */
		      if (strncmp (opt_offset + 1, "txt", 3) == 0)
                        {
                        dump->format = DUMP_TEXT;
                        strcpy (dump->mode, "w");
                        }
                      else
                        {
		        if (strncmp(opt_offset + 1, "raw", 3) == 0)
                          {
                          dump->format = DUMP_RAW;
                          strcpy (dump->mode, "wb");
                          }
                        else
                          {
                          TIFFError("parse_command_opts", "Unknown dump format %s", opt_offset + 1);
                          TIFFError ("For valid options type", "tiffcrop -h");
                          exit (-1);
		          }
			}
                      }
		    else
                      { /* Look for dump level specification */
                      if (strncmp (opt_ptr, "lev", 3) == 0)
                        dump->level = atoi(opt_offset + 1);
                        /* Look for input data dump file name */
                      if (strncmp (opt_ptr, "in", 2) == 0)
		        {
                        strncpy (dump->infilename, opt_offset + 1, PATH_MAX - 20);
                        dump->infilename[PATH_MAX - 20] = '\0';
                        }
                        /* Look for output data dump file name */
                      if (strncmp (opt_ptr, "out", 3) == 0)
			{
                        strncpy (dump->outfilename, opt_offset + 1, PATH_MAX - 20);
                        dump->outfilename[PATH_MAX - 20] = '\0';
                        }
                      if (strncmp (opt_ptr, "deb", 3) == 0)
			dump->debug = atoi(opt_offset + 1);
		      }
                    }
	        if ((strlen(dump->infilename)) || (strlen(dump->outfilename)))
                  {
		  if (dump->level == 1)
                    TIFFError("","Defaulting to dump level 1, no data.");
	          if (dump->format == DUMP_NONE)
                    {
		    TIFFError("", "You must specify a dump format for dump files");
		    TIFFError ("For valid options type", "tiffcrop -h");
		    exit (-1);
		    }
                  }
	        break;

      /* image manipulation routine options */
      case 'm': /* margins to exclude from selection, uppercase M was already used */
		/* order of values must be TOP, LEFT, BOTTOM, RIGHT */
		crop_data->crop_mode |= CROP_MARGINS;
                for (i = 0, opt_ptr = strtok (optarg, ",:");
                    ((opt_ptr != NULL) &&  (i < 4));
                     (opt_ptr = strtok (NULL, ",:")), i++)
                    {
		    crop_data->margins[i] = atof(opt_ptr);
                    }
		break;
      case 'E':	/* edge reference */
		switch (tolower(optarg[0]))
                  {
		  case 't': crop_data->edge_ref = EDGE_TOP;
                            break;
                  case 'b': crop_data->edge_ref = EDGE_BOTTOM;
                             break;
                  case 'l': crop_data->edge_ref = EDGE_LEFT;
                            break;
                  case 'r': crop_data->edge_ref = EDGE_RIGHT;
                            break;
		  default:  TIFFError ("Edge reference must be top, bottom, left, or right", "%s", optarg);
			    TIFFError ("For valid options type", "tiffcrop -h");
                            exit (-1);
		  }
		break;
      case 'F': /* flip eg mirror image or cropped segment, M was already used */
		crop_data->crop_mode |= CROP_MIRROR;
		switch (tolower(optarg[0]))
                  {
		  case  'h': crop_data->mirror = MIRROR_HORIZ;
                             break;
                  case  'v': crop_data->mirror = MIRROR_VERT;
                             break;
                  case  'b': crop_data->mirror = MIRROR_BOTH;
                             break;
		  default:   TIFFError ("Flip mode must be horiz, vert, or both", "%s", optarg);
			     TIFFError ("For valid options type", "tiffcrop -h");
                             exit (-1);
		  }
		break;
      case 'H': /* set horizontal resolution to new value */
		page->hres = atof (optarg);
                page->mode |= PAGE_MODE_RESOLUTION;
		break;
      case 'I': /* invert the color space, eg black to white */
		crop_data->crop_mode |= CROP_INVERT;
                /* The PHOTOMETIC_INTERPRETATION tag may be updated */
                if (streq(optarg, "black"))
                  {
		  crop_data->photometric = PHOTOMETRIC_MINISBLACK;
		  continue;
                  }
                if (streq(optarg, "white"))
                  {
		  crop_data->photometric = PHOTOMETRIC_MINISWHITE;
                  continue;
                  }
                if (streq(optarg, "data")) 
                  {
		  crop_data->photometric = INVERT_DATA_ONLY;
                  continue;
                  }
                if (streq(optarg, "both"))
                  {
		  crop_data->photometric = INVERT_DATA_AND_TAG;
                  continue;
                  }

		TIFFError("Missing or unknown option for inverting PHOTOMETRIC_INTERPRETATION", "%s", optarg);
		TIFFError ("For valid options type", "tiffcrop -h");
                exit (-1);
		break;
      case 'J': /* horizontal margin for sectioned ouput pages */ 
		page->hmargin = atof(optarg);
                page->mode |= PAGE_MODE_MARGINS;
		break;
      case 'K': /* vertical margin for sectioned ouput pages*/ 
                page->vmargin = atof(optarg);
                page->mode |= PAGE_MODE_MARGINS;
		break;
      case 'N':	/* list of images to process */
                for (i = 0, opt_ptr = strtok (optarg, ",");
                    ((opt_ptr != NULL) &&  (i < MAX_IMAGES));
                     (opt_ptr = strtok (NULL, ",")))
                     { /* We do not know how many images are in file yet 
			* so we build a list to include the maximum allowed
                        * and follow it until we hit the end of the file.
                        * Image count is not accurate for odd, even, last
                        * so page numbers won't be valid either.
                        */
		     if (streq(opt_ptr, "odd"))
                       {
		       for (j = 1; j <= MAX_IMAGES; j += 2)
			 imagelist[i++] = j;
                       *image_count = (MAX_IMAGES - 1) / 2;
                       break;
		       }
		     else
                       {
		       if (streq(opt_ptr, "even"))
                         {
			 for (j = 2; j <= MAX_IMAGES; j += 2)
			   imagelist[i++] = j;
                         *image_count = MAX_IMAGES / 2;
                         break;
			 }
		       else
                         {
			 if (streq(opt_ptr, "last"))
			   imagelist[i++] = MAX_IMAGES;
			 else  /* single value between commas */
			   {
			   sep = strpbrk(opt_ptr, ":-");
			   if (!sep)
			     imagelist[i++] = atoi(opt_ptr);
                           else
                             {
			     *sep = '\0';
                             start = atoi (opt_ptr);
                             if (!strcmp((sep + 1), "last"))
			       end = MAX_IMAGES;
                             else
                               end = atoi (sep + 1);
                             for (j = start; j <= end && j - start + i < MAX_IMAGES; j++)
			       imagelist[i++] = j;
			     }
			   }
			 }
		      }
		    }
                *image_count = i;
		break;
      case 'O': /* page orientation */ 
		switch (tolower(optarg[0]))
                  {
		  case  'a': page->orient = ORIENTATION_AUTO;
                             break;
		  case  'p': page->orient = ORIENTATION_PORTRAIT;
                             break;
		  case  'l': page->orient = ORIENTATION_LANDSCAPE;
                             break;
		  default:  TIFFError ("Orientation must be portrait, landscape, or auto.", "%s", optarg);
			    TIFFError ("For valid options type", "tiffcrop -h");
                            exit (-1);
		  }
		break;
      case 'P': /* page size selection */ 
	        if (sscanf(optarg, "%lfx%lf", &page->width, &page->length) == 2)
                  {
                  strcpy (page->name, "Custom"); 
                  page->mode |= PAGE_MODE_PAPERSIZE;
                  break;
                  }
                if (get_page_geometry (optarg, page))
                  {
		  if (!strcmp(optarg, "list"))
                    {
		    TIFFError("", "Name            Width   Length (in inches)");
                    for (i = 0; i < MAX_PAPERNAMES - 1; i++)
                      TIFFError ("", "%-15.15s %5.2f   %5.2f", 
			       PaperTable[i].name, PaperTable[i].width, 
                               PaperTable[i].length);
		    exit (-1);                   
                    }
     
		  TIFFError ("Invalid paper size", "%s", optarg);
                  TIFFError ("", "Select one of:");
		  TIFFError("", "Name            Width   Length (in inches)");
                  for (i = 0; i < MAX_PAPERNAMES - 1; i++)
                    TIFFError ("", "%-15.15s %5.2f   %5.2f", 
			       PaperTable[i].name, PaperTable[i].width, 
                               PaperTable[i].length);
		  exit (-1);
		  }
		else
                  {
                  page->mode |= PAGE_MODE_PAPERSIZE;
		  }
		break;
      case 'R': /* rotate image or cropped segment */
		crop_data->crop_mode |= CROP_ROTATE;
		switch (strtoul(optarg, NULL, 0))
                  {
		  case  90:  crop_data->rotation = (uint16)90;
                             break;
                  case  180: crop_data->rotation = (uint16)180;
                             break;
                  case  270: crop_data->rotation = (uint16)270;
                             break;
		  default:   TIFFError ("Rotation must be 90, 180, or 270 degrees clockwise", "%s", optarg);
			     TIFFError ("For valid options type", "tiffcrop -h");
                             exit (-1);
		  }
		break;
      case 'S':	/* subdivide into Cols:Rows sections, eg 3:2 would be 3 across and 2 down */
		sep = strpbrk(optarg, ",:");
		if (sep)
                  {
                  *sep = '\0';
                  page->cols = atoi(optarg);
                  page->rows = atoi(sep +1);
		  }
                else
                  {
                  page->cols = atoi(optarg);
                  page->rows = atoi(optarg);
		  }
                if ((page->cols * page->rows) > MAX_SECTIONS)
                  {
		  TIFFError ("Limit for subdivisions, ie rows x columns, exceeded", "%d", MAX_SECTIONS);
		  exit (-1);
                  }
                page->mode |= PAGE_MODE_ROWSCOLS;
		break;
      case 'U':	/* units for measurements and offsets */
		if (streq(optarg, "in"))
                  {
		  crop_data->res_unit = RESUNIT_INCH;
		  page->res_unit = RESUNIT_INCH;
		  }
		else if (streq(optarg, "cm"))
		  {
		  crop_data->res_unit = RESUNIT_CENTIMETER;
		  page->res_unit = RESUNIT_CENTIMETER;
		  }
		else if (streq(optarg, "px"))
		  {
		  crop_data->res_unit = RESUNIT_NONE;
		  page->res_unit = RESUNIT_NONE;
		  }
		else
                  {
		  TIFFError ("Illegal unit of measure","%s", optarg);
		  TIFFError ("For valid options type", "tiffcrop -h");
                  exit (-1);
		  }
		break;
      case 'V': /* set vertical resolution to new value */
		page->vres = atof (optarg);
                page->mode |= PAGE_MODE_RESOLUTION;
		break;
      case 'X':	/* selection width */
		crop_data->crop_mode |= CROP_WIDTH;
		crop_data->width = atof(optarg);
		break;
      case 'Y':	/* selection length */
		crop_data->crop_mode |= CROP_LENGTH;
		crop_data->length = atof(optarg);
		break;
      case 'Z': /* zones of an image X:Y read as zone X of Y */
		crop_data->crop_mode |= CROP_ZONES;
		for (i = 0, opt_ptr = strtok (optarg, ",");
                   ((opt_ptr != NULL) &&  (i < MAX_REGIONS));
                    (opt_ptr = strtok (NULL, ",")), i++)
                    {
		    crop_data->zones++;
		    opt_offset = strchr(opt_ptr, ':');
                    *opt_offset = '\0';
                    crop_data->zonelist[i].position = atoi(opt_ptr);
                    crop_data->zonelist[i].total    = atoi(opt_offset + 1);
                    }
                /*  check for remaining elements over MAX_REGIONS */
                if ((opt_ptr != NULL) && (i >= MAX_REGIONS))
                  {
		  TIFFError("Zone list exceeds region limit", "%d",  MAX_REGIONS);
		  exit (-1);
                  }
		break;
    case '?':	TIFFError ("For valid options type", "tiffcrop -h");
                exit (-1);
		/*NOTREACHED*/
      }
    }
  }  /* end process_command_opts */

/* Start a new output file if one has not been previously opened or
 * autoindex is set to non-zero. Update page and file counters
 * so TIFFTAG PAGENUM will be correct in image.
 */
static int 
update_output_file (TIFF **tiffout, char *mode, int autoindex,
                    char *outname, unsigned int *page)
  {
  static int findex = 0;    /* file sequence indicator */
  char  *sep;
  char   filenum[16];
  char   export_ext[16];
  char   exportname[PATH_MAX];

  if (autoindex && (*tiffout != NULL))
    {   
    /* Close any export file that was previously opened */
    TIFFClose (*tiffout);
    *tiffout = NULL;
    }

  strcpy (export_ext, ".tiff");
  memset (exportname, '\0', PATH_MAX);

  /* Leave room for page number portion of the new filename */
  strncpy (exportname, outname, PATH_MAX - 16);
  if (*tiffout == NULL)   /* This is a new export file */
    {
    if (autoindex)
      { /* create a new filename for each export */
      findex++;
      if ((sep = strstr(exportname, ".tif")) || (sep = strstr(exportname, ".TIF")))
        {
        strncpy (export_ext, sep, 5);
        *sep = '\0';
        }
      else
        strncpy (export_ext, ".tiff", 5);
      export_ext[5] = '\0';

      /* MAX_EXPORT_PAGES limited to 6 digits to prevent string overflow of pathname */
      if (findex > MAX_EXPORT_PAGES)
	{
	TIFFError("update_output_file", "Maximum of %d pages per file exceeded", MAX_EXPORT_PAGES);
        return 1;
        }

      sprintf (filenum, "-%03d%s", findex, export_ext);
      filenum[14] = '\0';
      strncat (exportname, filenum, 15);
      }
    exportname[PATH_MAX - 1] = '\0';

    *tiffout = TIFFOpen(exportname, mode);
    if (*tiffout == NULL)
      {
      TIFFError("update_output_file", "Unable to open output file %s", exportname);
      return 1;
      }
    *page = 0; 

    return 0;
    }
  else 
    (*page)++;

  return 0;
  } /* end update_output_file */


int
main(int argc, char* argv[])
  {
  extern int optind;
  uint16 defconfig = (uint16) -1;
  uint16 deffillorder = 0;
  uint32 deftilewidth = (uint32) 0;
  uint32 deftilelength = (uint32) 0;
  uint32 defrowsperstrip = (uint32) 0;
  uint32 dirnum = 0;

  TIFF *in = NULL;
  TIFF *out = NULL;
  char  mode[10];
  char *mp = mode;

  /** RJN additions **/
  struct image_data image;     /* Image parameters for one image */
  struct crop_mask  crop;      /* Cropping parameters for all images */
  struct pagedef    page;      /* Page definition for output pages */
  struct pageseg    sections[MAX_SECTIONS];  /* Sections of one output page */
  struct buffinfo   seg_buffs[MAX_SECTIONS]; /* Segment buffer sizes and pointers */
  struct dump_opts  dump;                  /* Data dump options */
  unsigned char *read_buff    = NULL;      /* Input image data buffer */
  unsigned char *crop_buff    = NULL;      /* Crop area buffer */
  unsigned char *sect_buff    = NULL;      /* Image section buffer */
  unsigned char *sect_src     = NULL;      /* Image section buffer pointer */
  unsigned int  imagelist[MAX_IMAGES + 1]; /* individually specified images */
  unsigned int  image_count  = 0;
  unsigned int  dump_images  = 0;
  unsigned int  next_image   = 0;
  unsigned int  next_page    = 0;
  unsigned int  total_pages  = 0;
  unsigned int  total_images = 0;
  unsigned int  end_of_input = FALSE;
  int    seg, length;
  char   temp_filename[PATH_MAX + 1];

  little_endian = *((unsigned char *)&little_endian) & '1';

  initImageData(&image);
  initCropMasks(&crop);
  initPageSetup(&page, sections, seg_buffs);
  initDumpOptions(&dump);

  process_command_opts (argc, argv, mp, mode, &dirnum, &defconfig, 
                        &deffillorder, &deftilewidth, &deftilelength, &defrowsperstrip,
	                &crop, &page, &dump, imagelist, &image_count);

  if (argc - optind < 2)
    usage();

  if ((argc - optind) == 2)
    pageNum = -1;
  else
    total_images = 0;
  /* read multiple input files and write to output file(s) */
  while (optind < argc - 1)
    {
    in = TIFFOpen (argv[optind], "r");
    if (in == NULL)
      return (-3);

    /* If only one input file is specified, we can use directory count */
    total_images = TIFFNumberOfDirectories(in); 
    if (image_count == 0)
      {
      dirnum = 0;
      total_pages = total_images; /* Only valid with single input file */
      }
    else
      {
      dirnum = (tdir_t)(imagelist[next_image] - 1);
      next_image++;

      /* Total pages only valid for enumerated list of pages not derived
       * using odd, even, or last keywords.
       */
      if (image_count >  total_images)
	image_count = total_images;
      
      total_pages = image_count;
      }

    /* MAX_IMAGES is used for special case "last" in selection list */
    if (dirnum == (MAX_IMAGES - 1))
      dirnum = total_images - 1;

    if (dirnum > (total_images))
      {
      TIFFError (TIFFFileName(in), 
      "Invalid image number %d, File contains only %d images", 
		 (int)dirnum + 1, total_images);
      if (out != NULL)
        (void) TIFFClose(out);
      return (1);
      }

    if (dirnum != 0 && !TIFFSetDirectory(in, (tdir_t)dirnum))
      {
      TIFFError(TIFFFileName(in),"Error, setting subdirectory at %d", dirnum);
      if (out != NULL)
        (void) TIFFClose(out);
      return (1);
      }

    end_of_input = FALSE;
    while (end_of_input == FALSE)
      {
      config = defconfig;
      compression = defcompression;
      predictor = defpredictor;
      fillorder = deffillorder;
      rowsperstrip = defrowsperstrip;
      tilewidth = deftilewidth;
      tilelength = deftilelength;
      g3opts = defg3opts;

      if (dump.format != DUMP_NONE)
        {
        /* manage input and/or output dump files here */
	dump_images++;
        length = strlen(dump.infilename);
        if (length > 0)
          {
          if (dump.infile != NULL)
            fclose (dump.infile);

          /* dump.infilename is guaranteed to be NUL termimated and have 20 bytes 
             fewer than PATH_MAX */ 
          memset (temp_filename, '\0', PATH_MAX + 1);              
          sprintf (temp_filename, "%s-read-%03d.%s", dump.infilename, dump_images,
                  (dump.format == DUMP_TEXT) ? "txt" : "raw");
          if ((dump.infile = fopen(temp_filename, dump.mode)) == NULL)
            {
	    TIFFError ("Unable to open dump file for writing", "%s", temp_filename);
	    exit (-1);
            }
          dump_info(dump.infile, dump.format, "Reading image","%d from %s", 
                    dump_images, TIFFFileName(in));
          } 
        length = strlen(dump.outfilename);
        if (length > 0)
          {
          if (dump.outfile != NULL)
            fclose (dump.outfile);

          /* dump.outfilename is guaranteed to be NUL termimated and have 20 bytes 
             fewer than PATH_MAX */ 
          memset (temp_filename, '\0', PATH_MAX + 1);              
          sprintf (temp_filename, "%s-write-%03d.%s", dump.outfilename, dump_images,
                  (dump.format == DUMP_TEXT) ? "txt" : "raw");
          if ((dump.outfile = fopen(temp_filename, dump.mode)) == NULL)
            {
	      TIFFError ("Unable to open dump file for writing", "%s", temp_filename);
	    exit (-1);
            }
          dump_info(dump.outfile, dump.format, "Writing image","%d from %s", 
                    dump_images, TIFFFileName(in));
          } 
        }

      if (dump.debug)
         TIFFError("main", "Reading image %4d of %4d total pages.", dirnum + 1, total_pages);

      if (loadImage(in, &image, &dump, &read_buff))
        {
        TIFFError("main", "Unable to load source image");
        exit (-1);
        }

      /* Correct the image orientation if it was not ORIENTATION_TOPLEFT.
       */
      if (image.adjustments != 0)
        {
	if (correct_orientation(&image, &read_buff))
	    TIFFError("main", "Unable to correct image orientation");
        }

      if (getCropOffsets(&image, &crop, &dump))
        {
        TIFFError("main", "Unable to define crop regions");
        exit (-1);
	}

      if (crop.selections > 0)
        {
        if (processCropSelections(&image, &crop, &read_buff, seg_buffs))
          {
          TIFFError("main", "Unable to process image selections");
          exit (-1);
	  }
	}
      else  /* Single image segment without zones or regions */
        {
        if (createCroppedImage(&image, &crop, &read_buff, &crop_buff))
          {
          TIFFError("main", "Unable to create output image");
          exit (-1);
	  }
	}
      if (page.mode == PAGE_MODE_NONE)
        {  /* Whole image or sections not based on output page size */
        if (crop.selections > 0)
          {
	  writeSelections(in, &out, &crop, &image, &dump, seg_buffs,
                          mp, argv[argc - 1], &next_page, total_pages);
          }
	else  /* One file all images and sections */
          {
	  if (update_output_file (&out, mp, crop.exp_mode, argv[argc - 1],
                                  &next_page))
             exit (1);
          if (writeCroppedImage(in, out, &image, &dump,crop.combined_width, 
                                crop.combined_length, crop_buff, next_page, total_pages))
            {
             TIFFError("main", "Unable to write new image");
             exit (-1);
	    }
          }
	}
      else
        {
	/* If we used a crop buffer, our data is there, otherwise it is
         * in the read_buffer
         */
	if (crop_buff != NULL)  
	  sect_src = crop_buff;
        else
          sect_src = read_buff;
        /* Break input image into pages or rows and columns */
        if (computeOutputPixelOffsets(&crop, &image, &page, sections, &dump))
          {
          TIFFError("main", "Unable to compute output section data");
          exit (-1);
	  }
        /* If there are multiple files on the command line, the final one is assumed 
         * to be the output filename into which the images are written.
         */
	if (update_output_file (&out, mp, crop.exp_mode, argv[argc - 1], &next_page))
          exit (1);

	if (writeImageSections(in, out, &image, &page, sections, &dump, sect_src, &sect_buff))
          {
          TIFFError("main", "Unable to write image sections");
          exit (-1);
	  }
        }

      /* No image list specified, just read the next image */
      if (image_count == 0)
        dirnum++;
      else
        {
	dirnum = (tdir_t)(imagelist[next_image] - 1);
        next_image++;
        }

      if (dirnum == MAX_IMAGES - 1)
        dirnum = TIFFNumberOfDirectories(in) - 1;

      if (!TIFFSetDirectory(in, (tdir_t)dirnum))
        end_of_input = TRUE;
      }
    TIFFClose(in);
    optind++;
    }

  /* If we did not use the read buffer as the crop buffer */
  if (read_buff)
    _TIFFfree(read_buff);

  if (crop_buff)
    _TIFFfree(crop_buff);

  if (sect_buff)
    _TIFFfree(sect_buff);

   /* Clean up any segment buffers used for zones or regions */
  for (seg = 0; seg < crop.selections; seg++)
    _TIFFfree (seg_buffs[seg].buffer);

  if (dump.format != DUMP_NONE)
    {
    if (dump.infile != NULL)
     fclose (dump.infile);

    if (dump.outfile != NULL)
      {
      dump_info (dump.outfile, dump.format, "", "Completed run for %s", TIFFFileName(out));
      fclose (dump.outfile);
      }
    }

  TIFFClose(out);

  return (0);
  } /* end main */


/* Debugging functions */
static int dump_data (FILE *dumpfile, int format, char *dump_tag, unsigned char *data, uint32 count)
  {
  int j, k;
  uint32 i;
  char  dump_array[10];
  unsigned char bitset;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  if (format == DUMP_TEXT)
    {
    fprintf (dumpfile," %s  ", dump_tag);
    for (i = 0; i < count; i++)
      {
      for (j = 0, k = 7; j < 8; j++, k--)
        {
	bitset = (*(data + i)) & (((unsigned char)1 << k)) ? 1 : 0;
        sprintf(&dump_array[j], (bitset) ? "1" : "0");
        }
      dump_array[8] = '\0';
      fprintf (dumpfile," %s", dump_array);
      }
    fprintf (dumpfile,"\n");
    }
  else
    {
    if ((fwrite (data, 1, count, dumpfile)) != count)
      {
      TIFFError ("", "Unable to write binary data to dump file");
      return (1);
      }
    }

  return (0);
  }

static int dump_byte (FILE *dumpfile, int format, char *dump_tag, unsigned char data)
  {
  int j, k;
  char  dump_array[10];
  unsigned char bitset;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  if (format == DUMP_TEXT)
    {
    fprintf (dumpfile," %s  ", dump_tag);
    for (j = 0, k = 7; j < 8; j++, k--)
      {
      bitset = data & (((unsigned char)1 << k)) ? 1 : 0;
      sprintf(&dump_array[j], (bitset) ? "1" : "0");
      }
    dump_array[8] = '\0';
    fprintf (dumpfile," %s\n", dump_array);
    }
  else
    {
    if ((fwrite (&data, 1, 1, dumpfile)) != 1)
      {
      TIFFError ("", "Unable to write binary data to dump file");
      return (1);
      }
    }

  return (0);
  }

static int dump_short (FILE *dumpfile, int format, char *dump_tag, uint16 data)
  {
  int j, k;
  char  dump_array[20];
  unsigned char bitset;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  if (format == DUMP_TEXT)
    {
    fprintf (dumpfile," %s  ", dump_tag);
    for (j = 0, k = 15; k >= 0; j++, k--)
      {
      bitset = data & (((unsigned char)1 << k)) ? 1 : 0;
      sprintf(&dump_array[j], (bitset) ? "1" : "0");
      if ((k % 8) == 0)
          sprintf(&dump_array[++j], " ");
      }
    dump_array[17] = '\0';
    fprintf (dumpfile," %s\n", dump_array);
    }
  else
    {
    if ((fwrite (&data, 2, 1, dumpfile)) != 2)
      {
      TIFFError ("", "Unable to write binary data to dump file");
      return (1);
      }
    }

  return (0);
  }

static int dump_long (FILE *dumpfile, int format, char *dump_tag, uint32 data)
  {
  int j, k;
  char  dump_array[40];
  unsigned char bitset;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  if (format == DUMP_TEXT)
    {
    fprintf (dumpfile," %s  ", dump_tag);
    for (j = 0, k = 31; k >= 0; j++, k--)
      {
      bitset = data & (((uint32)1 << k)) ? 1 : 0;
      sprintf(&dump_array[j], (bitset) ? "1" : "0");
      if ((k % 8) == 0)
          sprintf(&dump_array[++j], " ");
      }
    dump_array[35] = '\0';
    fprintf (dumpfile," %s\n", dump_array);
    }
  else
    {
    if ((fwrite (&data, 4, 1, dumpfile)) != 4)
      {
      TIFFError ("", "Unable to write binary data to dump file");
      return (1);
      }
    }
  return (0);
  }

static int dump_wide (FILE *dumpfile, int format, char *dump_tag, uint64 data)
  {
  int j, k;
  char  dump_array[80];
  unsigned char bitset;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  if (format == DUMP_TEXT)
    {
    fprintf (dumpfile," %s  ", dump_tag);
    for (j = 0, k = 63; k >= 0; j++, k--)
      {
      bitset = data & (((uint64)1 << k)) ? 1 : 0;
      sprintf(&dump_array[j], (bitset) ? "1" : "0");
      if ((k % 8) == 0)
          sprintf(&dump_array[++j], " ");
      }
    dump_array[71] = '\0';
    fprintf (dumpfile," %s\n", dump_array);
    }
  else
    {
    if ((fwrite (&data, 8, 1, dumpfile)) != 8)
      {
      TIFFError ("", "Unable to write binary data to dump file");
      return (1);
      }
    }

  return (0);
  }

static void dump_info(FILE *dumpfile, int format, char *prefix, char *msg, ...)
  {
  if (format == DUMP_TEXT)
    {
    va_list ap;
    va_start(ap, msg);
    fprintf(dumpfile, "%s ", prefix);
    vfprintf(dumpfile, msg, ap);
    fprintf(dumpfile, "\n");
    }
  }

static int dump_buffer (FILE* dumpfile, int format, uint32 rows, uint32 width, 
                 uint32 row, unsigned char *buff)
  {
  int j, k;
  uint32 i;
  unsigned char * dump_ptr;

  if (dumpfile == NULL)
    {
    TIFFError ("", "Invalid FILE pointer for dump file");
    return (1);
    }

  for (i = 0; i < rows; i++)
    {
    dump_ptr = buff + (i * width);
    if (format == DUMP_TEXT)
      dump_info (dumpfile, format, "", 
                 "Row %4d, %d bytes at offset %d",
	         row + i + 1, width, row * width);
     
    for (j = 0, k = width; k >= 10; j += 10, k -= 10, dump_ptr += 10)
      dump_data (dumpfile, format, "", dump_ptr, 10);
    if (k > 0)
      dump_data (dumpfile, format, "", dump_ptr, k);
    }
  return (0);
  }

/* Extract one or more samples from an interleaved buffer. If count == 1,
 * only the sample plane indicated by sample will be extracted.  If count > 1, 
 * count samples beginning at sample will be extracted. Portions of a 
 * scanline can be extracted by specifying a start and end value.
 */

static int 
extractContigSamplesBytes (uint8 *in, uint8 *out, uint32 cols, 
                           tsample_t sample, uint16 spp, uint16 bps, 
                           tsample_t count, uint32 start, uint32 end)
  {
  int i, bytes_per_sample, sindex;
  uint32 col, dst_rowsize, bit_offset;
  uint32 src_byte, src_bit;
  uint8 *src = in;
  uint8 *dst = out;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("extractContigSamplesBytes","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamplesBytes", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamplesBytes", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  dst_rowsize = (bps * (end - start) * count) / 8;

  bytes_per_sample = (bps + 7) / 8; 
  /* Optimize case for copying all samples */
  if (count == spp)
    {
    src = in + (start * spp * bytes_per_sample);
    _TIFFmemcpy (dst, src, dst_rowsize);
    }
  else
    {
    for (col = start; col < end; col++)
      {
      for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
        {
        bit_offset = col * bps * spp;
        if (sindex == 0)
          {
          src_byte = bit_offset / 8;
          src_bit  = bit_offset % 8;
          }
        else
          {
          src_byte = (bit_offset + (sindex * bps)) / 8;
          src_bit  = (bit_offset + (sindex * bps)) % 8;
          }
        src = in + src_byte;
        for (i = 0; i < bytes_per_sample; i++)
            *dst++ = *src++;
        }
      }
    }

  return (0);
  } /* end extractContigSamplesBytes */

static int
extractContigSamples8bits (uint8 *in, uint8 *out, uint32 cols,
                           tsample_t sample, uint16 spp, uint16 bps, 
                           tsample_t count, uint32 start, uint32 end)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint8  maskbits = 0, matchbits = 0;
  uint8  buff1 = 0, buff2 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("extractContigSamples8bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamples8bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamples8bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }
  
  ready_bits = 0;
  maskbits =  (uint8)-1 >> ( 8 - bps);
  buff1 = buff2 = 0;
  for (col = start; col < end; col++)
    {    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (8 - src_bit - bps); 
      buff1 = ((*src) & matchbits) << (src_bit);

      /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 8)
        {
        *dst++ = buff2;
        buff2 = buff1;
        ready_bits -= 8;
        }
      else
        buff2 = (buff2 | (buff1 >> ready_bits));
      ready_bits += bps;
      }
    }

  while (ready_bits > 0)
    {
    buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
    *dst++ = buff1;
    ready_bits -= 8;
    }

  return (0);
  } /* end extractContigSamples8bits */

static int
extractContigSamples16bits (uint8 *in, uint8 *out, uint32 cols, 
                            tsample_t sample, uint16 spp, uint16 bps, 
                            tsample_t count, uint32 start, uint32 end)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint16 maskbits = 0, matchbits = 0;
  uint16 buff1 = 0, buff2 = 0;
  uint8  bytebuff = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("extractContigSamples16bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamples16bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamples16bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  ready_bits = 0;
  maskbits = (uint16)-1 >> (16 - bps);

  for (col = start; col < end; col++)
    {    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (16 - src_bit - bps); 

      if (little_endian)
        buff1 = (src[0] << 8) | src[1];
      else
        buff1 = (src[1] << 8) | src[0];

      buff1 = (buff1 & matchbits) << (src_bit);
      if (ready_bits < 8) /* add another bps bits to the buffer */
        { 
        bytebuff = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff = (buff2 >> 8);
        *dst++ = bytebuff;
        ready_bits -= 8;
        /* shift in new bits */
        buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff = (buff2 >> 8);
    *dst++ = bytebuff;
    ready_bits -= 8;
    }
  
  return (0);
  } /* end extractContigSamples16bits */


static int
extractContigSamples24bits (uint8 *in, uint8 *out, uint32 cols,
 	                    tsample_t sample, uint16 spp, uint16 bps, 
                            tsample_t count, uint32 start, uint32 end)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint32 maskbits = 0, matchbits = 0;
  uint32 buff1 = 0, buff2 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((in == NULL) || (out == NULL))
    {
    TIFFError("extractContigSamples24bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamples24bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamples24bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  ready_bits = 0;
  maskbits =  (uint32)-1 >> ( 32 - bps);
  for (col = start; col < end; col++)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (32 - src_bit - bps); 
      if (little_endian)
	buff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      else
	buff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
      buff1 = (buff1 & matchbits) << (src_bit);

      if (ready_bits < 16) /* add another bps bits to the buffer */
        {
        bytebuff1 = bytebuff2 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 24);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 16);
        *dst++ = bytebuff2;
        ready_bits -= 16;

        /* shift in new bits */
        buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 24);
    *dst++ = bytebuff1;

    buff2 = (buff2 << 8);
    bytebuff2 = bytebuff1;
    ready_bits -= 8;
    } 
  
  return (0);
  } /* end extractContigSamples24bits */

static int
extractContigSamples32bits (uint8 *in, uint8 *out, uint32 cols,
                            tsample_t sample, uint16 spp, uint16 bps, 
 			    tsample_t count, uint32 start, uint32 end)
  {
  int    ready_bits = 0, sindex = 0, shift_width = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 maskbits = 0, matchbits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((in == NULL) || (out == NULL))
    {
    TIFFError("extractContigSamples32bits","Invalid input or output buffer");
    return (1);
    }


  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamples32bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamples32bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  shift_width = ((bps + 7) / 8) + 1; 
  ready_bits = 0;
  maskbits =  (uint64)-1 >> ( 64 - bps);
  for (col = start; col < end; col++)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (64 - src_bit - bps); 
      if (little_endian)
        {
	longbuff1 = (src[0] << 24) | (src[1] << 16)  | (src[2] << 8) | src[3];
	longbuff2 = longbuff1;
        }
      else
        {
	longbuff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
	longbuff2 = longbuff1;
	}

      buff3 = ((uint64)longbuff1 << 32) | longbuff2;
      buff1 = (buff3 & matchbits) << (src_bit);

      /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 32)
        {
        bytebuff1 = (buff2 >> 56);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 48);
        *dst++ = bytebuff2;
        bytebuff3 = (buff2 >> 40);
        *dst++ = bytebuff3;
        bytebuff4 = (buff2 >> 32);
        *dst++ = bytebuff4;
        ready_bits -= 32;
                    
        /* shift in new bits */
        buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
        }
      else
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 56);
    *dst++ = bytebuff1;
    buff2 = (buff2 << 8);
    ready_bits -= 8;
    }
  
  return (0);
  } /* end extractContigSamples32bits */

static int
extractContigSamplesShifted8bits (uint8 *in, uint8 *out, uint32 cols,
                                  tsample_t sample, uint16 spp, uint16 bps, 
			          tsample_t count, uint32 start, uint32 end,
 	                          int shift)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint8  maskbits = 0, matchbits = 0;
  uint8  buff1 = 0, buff2 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("extractContigSamplesShifted8bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamplesShifted8bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamplesShifted8bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  ready_bits = shift;
  maskbits =  (uint8)-1 >> ( 8 - bps);
  buff1 = buff2 = 0;
  for (col = start; col < end; col++)
    {    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (8 - src_bit - bps); 
      buff1 = ((*src) & matchbits) << (src_bit);
      if ((col == start) && (sindex == sample))
        buff2 = *src & ((uint8)-1) << (shift);

      /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 8)
        {
        *dst++ |= buff2;
        buff2 = buff1;
        ready_bits -= 8;
        }
      else
	buff2 = buff2 | (buff1 >> ready_bits);
      ready_bits += bps;
      }
    }

  while (ready_bits > 0)
    {
    buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
    *dst++ = buff1;
    ready_bits -= 8;
    }

  return (0);
  } /* end extractContigSamplesShifted8bits */

static int
extractContigSamplesShifted16bits (uint8 *in, uint8 *out, uint32 cols, 
                                   tsample_t sample, uint16 spp, uint16 bps, 
  			           tsample_t count, uint32 start, uint32 end,
 	                           int shift)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint16 maskbits = 0, matchbits = 0;
  uint16 buff1 = 0, buff2 = 0;
  uint8  bytebuff = 0;
  uint8 *src = in;
  uint8 *dst = out;
  
  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("extractContigSamplesShifted16bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamplesShifted16bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamplesShifted16bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  ready_bits = shift;
  maskbits = (uint16)-1 >> (16 - bps);
  for (col = start; col < end; col++)
    {    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (16 - src_bit - bps); 
      if (little_endian)
        buff1 = (src[0] << 8) | src[1];
      else
        buff1 = (src[1] << 8) | src[0];

      if ((col == start) && (sindex == sample))
        buff2 = buff1 & ((uint16)-1) << (8 - shift);

      buff1 = (buff1 & matchbits) << (src_bit);

      if (ready_bits < 8) /* add another bps bits to the buffer */
        buff2 = buff2 | (buff1 >> ready_bits);
      else  /* If we have a full buffer's worth, write it out */
        {
        bytebuff = (buff2 >> 8);
        *dst++ = bytebuff;
        ready_bits -= 8;
        /* shift in new bits */
        buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
        }

      ready_bits += bps;
      }
    }

  /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff = (buff2 >> 8);
    *dst++ = bytebuff;
    ready_bits -= 8;
    }
  
  return (0);
  } /* end extractContigSamplesShifted16bits */


static int
extractContigSamplesShifted24bits (uint8 *in, uint8 *out, uint32 cols,
 	                           tsample_t sample, uint16 spp, uint16 bps, 
                                   tsample_t count, uint32 start, uint32 end,
	                           int shift)
  {
  int    ready_bits = 0, sindex = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint32 maskbits = 0, matchbits = 0;
  uint32 buff1 = 0, buff2 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((in == NULL) || (out == NULL))
    {
    TIFFError("extractContigSamplesShifted24bits","Invalid input or output buffer");
    return (1);
    }

  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamplesShifted24bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamplesShifted24bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  ready_bits = shift;
  maskbits =  (uint32)-1 >> ( 32 - bps);
  for (col = start; col < end; col++)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (32 - src_bit - bps); 
      if (little_endian)
	buff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      else
	buff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

      if ((col == start) && (sindex == sample))
        buff2 = buff1 & ((uint32)-1) << (16 - shift);

      buff1 = (buff1 & matchbits) << (src_bit);

      if (ready_bits < 16)  /* add another bps bits to the buffer */
        {
        bytebuff1 = bytebuff2 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 24);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 16);
        *dst++ = bytebuff2;
        ready_bits -= 16;

        /* shift in new bits */
        buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 24);
    *dst++ = bytebuff1;

    buff2 = (buff2 << 8);
    bytebuff2 = bytebuff1;
    ready_bits -= 8;
    }
   
  return (0);
  } /* end extractContigSamplesShifted24bits */

static int
extractContigSamplesShifted32bits (uint8 *in, uint8 *out, uint32 cols,
                                   tsample_t sample, uint16 spp, uint16 bps, 
 			           tsample_t count, uint32 start, uint32 end,
	                           int shift)
  {
  int    ready_bits = 0, sindex = 0, shift_width = 0;
  uint32 col, src_byte, src_bit, bit_offset;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 maskbits = 0, matchbits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  uint8 *src = in;
  uint8 *dst = out;

  if ((in == NULL) || (out == NULL))
    {
    TIFFError("extractContigSamplesShifted32bits","Invalid input or output buffer");
    return (1);
    }


  if ((start > end) || (start > cols))
    {
    TIFFError ("extractContigSamplesShifted32bits", 
               "Invalid start column value %d ignored", start);
    start = 0;
    }
  if ((end == 0) || (end > cols))
    {
    TIFFError ("extractContigSamplesShifted32bits", 
               "Invalid end column value %d ignored", end);
    end = cols;
    }

  shift_width = ((bps + 7) / 8) + 1; 
  ready_bits = shift;
  maskbits =  (uint64)-1 >> ( 64 - bps);
  for (col = start; col < end; col++)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = col * bps * spp;
    for (sindex = sample; (sindex < spp) && (sindex < (sample + count)); sindex++)
      {
      if (sindex == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sindex * bps)) / 8;
        src_bit  = (bit_offset + (sindex * bps)) % 8;
        }

      src = in + src_byte;
      matchbits = maskbits << (64 - src_bit - bps); 
      if (little_endian)
        {
	longbuff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
	longbuff2 = longbuff1;
        }
      else
        {
	longbuff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
	longbuff2 = longbuff1;
	}

      buff3 = ((uint64)longbuff1 << 32) | longbuff2;
      if ((col == start) && (sindex == sample))
        buff2 = buff3 & ((uint64)-1) << (32 - shift);

      buff1 = (buff3 & matchbits) << (src_bit);

      if (ready_bits < 32)
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else  /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 56);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 48);
        *dst++ = bytebuff2;
        bytebuff3 = (buff2 >> 40);
        *dst++ = bytebuff3;
        bytebuff4 = (buff2 >> 32);
        *dst++ = bytebuff4;
        ready_bits -= 32;
                    
        /* shift in new bits */
        buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 56);
    *dst++ = bytebuff1;
    buff2 = (buff2 << 8);
    ready_bits -= 8;
    }
  
  return (0);
  } /* end extractContigSamplesShifted32bits */

static int
extractContigSamplesToBuffer(uint8 *out, uint8 *in, uint32 rows, uint32 cols,
  	                     tsample_t sample, uint16 spp, uint16 bps, 
                             struct dump_opts *dump)
  {
  int    shift_width, bytes_per_sample, bytes_per_pixel;
  uint32 src_rowsize, src_offset, row, first_col = 0;
  uint32 dst_rowsize, dst_offset;
  tsample_t count = 1;
  uint8 *src, *dst;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if ((bps % 8) == 0)
    shift_width = 0;
  else
    {
    if (bytes_per_pixel < (bytes_per_sample + 1))
      shift_width = bytes_per_pixel;
    else
      shift_width = bytes_per_sample + 1;
    }
  src_rowsize = ((bps * spp * cols) + 7) / 8;
  dst_rowsize = ((bps * cols) + 7) / 8;

  if ((dump->outfile != NULL) && (dump->level == 4))
    {
    dump_info  (dump->outfile, dump->format, "extractContigSamplesToBuffer", 
                "Sample %d, %d rows", sample + 1, rows + 1);
    }
  for (row = 0; row < rows; row++)
    {
    src_offset = row * src_rowsize;
    dst_offset = row * dst_rowsize;
    src = in + src_offset;
    dst = out + dst_offset;

    /* pack the data into the scanline */
    switch (shift_width)
      {  
      case 0: if (extractContigSamplesBytes (src, dst, cols, sample,
                                             spp, bps,  count, first_col, cols))  
                return (1);
 	      break;
      case 1: if (bps == 1)
                {
                if (extractContigSamples8bits (src, dst, cols, sample,
                                               spp, bps, count, first_col, cols))
	          return (1);
	        break;
		}
	      else
                 if (extractContigSamples16bits (src, dst, cols, sample,
                                                 spp, bps, count, first_col, cols))
	         return (1);
	      break;
      case 2: if (extractContigSamples24bits (src, dst, cols, sample,
                                              spp, bps,  count, first_col, cols))
	         return (1);
	      break;
      case 3:
      case 4: 
      case 5: if (extractContigSamples32bits (src, dst, cols, sample,
                                              spp, bps,  count, first_col, cols))
	         return (1);
	      break;
      default: TIFFError ("extractContigSamplesToBuffer", "Unsupported bit depth: %d", bps);
	       return (1);
      }
    if ((dump->outfile != NULL) && (dump->level == 4))
      dump_buffer(dump->outfile, dump->format, 1, dst_rowsize, row, dst);
    }

  return (0);
  } /* end extractContigSamplesToBuffer */

static int
extractContigSamplesToTileBuffer(uint8 *out, uint8 *in, uint32 rows, uint32 cols,
  	                         uint32 imagewidth, uint32 tilewidth, tsample_t sample,
				 uint16 count, uint16 spp, uint16 bps, struct dump_opts *dump)
  {
  int    shift_width, bytes_per_sample, bytes_per_pixel;
  uint32 src_rowsize, src_offset, row;
  uint32 dst_rowsize, dst_offset;
  uint8 *src, *dst;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if ((bps % 8) == 0)
    shift_width = 0;
  else
    {
    if (bytes_per_pixel < (bytes_per_sample + 1))
      shift_width = bytes_per_pixel;
    else
      shift_width = bytes_per_sample + 1;
    }

  if ((dump->outfile != NULL) && (dump->level == 4))
    {
    dump_info  (dump->outfile, dump->format, "extractContigSamplesToTileBuffer", 
                "Sample %d, %d rows", sample + 1, rows + 1);
    }

  src_rowsize = ((bps * spp * imagewidth) + 7) / 8;
  dst_rowsize = ((bps * tilewidth * count) + 7) / 8;

  for (row = 0; row < rows; row++)
    {
    src_offset = row * src_rowsize;
    dst_offset = row * dst_rowsize;
    src = in + src_offset;
    dst = out + dst_offset;

    /* pack the data into the scanline */
    switch (shift_width)
      {  
      case 0: if (extractContigSamplesBytes (src, dst, cols, sample,
                                             spp, bps,  count, 0, cols))  
                return (1);
 	      break;
      case 1: if (bps == 1)
                {
                if (extractContigSamples8bits (src, dst, cols, sample,
                                               spp, bps, count, 0, cols))
	          return (1);
	        break;
		}
	      else
                 if (extractContigSamples16bits (src, dst, cols, sample,
                                                 spp, bps, count, 0, cols))
	         return (1);
	      break;
      case 2: if (extractContigSamples24bits (src, dst, cols, sample,
                                              spp, bps,  count, 0, cols))
	         return (1);
	      break;
      case 3:
      case 4: 
      case 5: if (extractContigSamples32bits (src, dst, cols, sample,
                                              spp, bps,  count, 0, cols))
	         return (1);
	      break;
      default: TIFFError ("extractContigSamplesToTileBuffer", "Unsupported bit depth: %d", bps);
	       return (1);
      }
    if ((dump->outfile != NULL) && (dump->level == 4))
      dump_buffer(dump->outfile, dump->format, 1, dst_rowsize, row, dst);
    }

  return (0);
  } /* end extractContigSamplesToTileBuffer */

static int readContigStripsIntoBuffer (TIFF* in, uint8* buf)
  {
  uint8* bufp = buf;
  int32  bytes_read = 0;
  uint16 strip, nstrips   = TIFFNumberOfStrips(in);
  uint32 stripsize = TIFFStripSize(in);
  uint32 rows = 0;
  uint32 rps = TIFFGetFieldDefaulted(in, TIFFTAG_ROWSPERSTRIP, &rps);
  tsize_t scanline_size = TIFFScanlineSize(in);

  for (strip = 0; strip < nstrips; strip++)
    {
    bytes_read = TIFFReadEncodedStrip (in, strip, bufp, -1);
    rows = bytes_read / scanline_size;
    if ((strip < (nstrips - 1)) && (bytes_read != (int32)stripsize))
      TIFFError("", "Strip %d: read %lu bytes, strip size %lu",
		(int)strip + 1, (unsigned long) bytes_read, (unsigned long)stripsize);

    if (bytes_read < 0 && !ignore)
      {
      TIFFError("", "Error reading strip %lu after %lu rows",
		(unsigned long) strip, (unsigned long)rows);
      return 0;
      }
    bufp += bytes_read;
    }

 return 1;
  } /* end readContigStripsIntoBuffer */

static int 
combineSeparateSamplesBytes (unsigned char *srcbuffs[], unsigned char *out,
                             uint32 cols, uint32 rows, uint16 spp, uint16 bps,
                             FILE *dumpfile, int format, int level)
  {
  int i, bytes_per_sample;
  uint32 row, col, col_offset, src_rowsize, dst_rowsize, row_offset;
  unsigned char *src;
  unsigned char *dst;
  tsample_t s;

  src = srcbuffs[0];
  dst = out;
  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateSamplesBytes","Invalid buffer address");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 

  src_rowsize = ((bps * cols) + 7) / 8;
  dst_rowsize = ((bps * spp * cols) + 7) / 8;
  for (row = 0; row < rows; row++)
    {
    if ((dumpfile != NULL) && (level == 2))
      {
      for (s = 0; s < spp; s++)
        {
        dump_info (dumpfile, format, "combineSeparateSamplesBytes","Input data, Sample %d", s);
        dump_buffer(dumpfile, format, 1, cols, row, srcbuffs[s] + (row * src_rowsize));
        }
      }
    dst = out + (row * dst_rowsize);
    row_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      col_offset = row_offset + (col * (bps / 8)); 
      for (s = 0; (s < spp) && (s < MAX_SAMPLES); s++)
        {
        src = srcbuffs[s] + col_offset; 
        for (i = 0; i < bytes_per_sample; i++)
          *(dst + i) = *(src + i);
        src += bytes_per_sample;
        dst += bytes_per_sample;
        }   
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateSamplesBytes","Output data, combined samples");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateSamplesBytes */

static int
combineSeparateSamples8bits (uint8 *in[], uint8 *out, uint32 cols,
                            uint32 rows, uint16 spp, uint16 bps, 
 	                    FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0;
  int    bytes_per_sample = 0;
  uint32 src_rowsize, dst_rowsize, src_offset; 
  uint32 bit_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint8  maskbits = 0, matchbits = 0;
  uint8  buff1 = 0, buff2 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[32];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateSamples8bits","Invalid input or output buffer");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 
  src_rowsize = ((bps * cols) + 7) / 8;
  dst_rowsize = ((bps * cols * spp) + 7) / 8;
  maskbits =  (uint8)-1 >> ( 8 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (8 - src_bit - bps); 
      /* load up next sample from each plane */
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        buff1 = ((*src) & matchbits) << (src_bit);

        /* If we have a full buffer's worth, write it out */
        if (ready_bits >= 8)
          {
          *dst++ = buff2;
          buff2 = buff1;
          ready_bits -= 8;
          strcpy (action, "Flush");
          }
        else
          {
          buff2 = (buff2 | (buff1 >> ready_bits));
          strcpy (action, "Update");
          }
        ready_bits += bps;
 
        if ((dumpfile != NULL) && (level == 3))
          {
          dump_info (dumpfile, format, "",
                   "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		   row + 1, col + 1, s, src_byte, src_bit, dst - out);
          dump_byte (dumpfile, format, "Match bits", matchbits);
          dump_byte (dumpfile, format, "Src   bits", *src);
          dump_byte (dumpfile, format, "Buff1 bits", buff1);
          dump_byte (dumpfile, format, "Buff2 bits", buff2);
          dump_info (dumpfile, format, "","%s", action); 
	  }
        }
      }

    if (ready_bits > 0)
      {
      buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
      *dst++ = buff1;
      if ((dumpfile != NULL) && (level == 3))
        {
        dump_info (dumpfile, format, "",
	         "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
	         row + 1, col + 1, src_byte, src_bit, dst - out);
                 dump_byte (dumpfile, format, "Final bits", buff1);
        }
      }

    if ((dumpfile != NULL) && (level >= 2))
      {
      dump_info (dumpfile, format, "combineSeparateSamples8bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateSamples8bits */

static int
combineSeparateSamples16bits (uint8 *in[], uint8 *out, uint32 cols,
                              uint32 rows, uint16 spp, uint16 bps, 
 	                      FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0, bytes_per_sample = 0;
  uint32 src_rowsize, dst_rowsize; 
  uint32 bit_offset, src_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint16 maskbits = 0, matchbits = 0;
  uint16 buff1 = 0, buff2 = 0;
  uint8  bytebuff = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateSamples16bits","Invalid input or output buffer");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 
  src_rowsize = ((bps * cols) + 7) / 8;
  dst_rowsize = ((bps * cols * spp) + 7) / 8;
  maskbits = (uint16)-1 >> (16 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (16 - src_bit - bps); 
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        if (little_endian)
          buff1 = (src[0] << 8) | src[1];
        else
          buff1 = (src[1] << 8) | src[0];

	buff1 = (buff1 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 8)
	  {
	    bytebuff = (buff2 >> 8);
	    *dst++ = bytebuff;
	    ready_bits -= 8;
	    /* shift in new bits */
	    buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
	    strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	    bytebuff = 0;
	    buff2 = (buff2 | (buff1 >> ready_bits));
	    strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  {
	  dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, s, src_byte, src_bit, dst - out);

	  dump_short (dumpfile, format, "Match bits", matchbits);
	  dump_data  (dumpfile, format, "Src   bits", src, 2);
	  dump_short (dumpfile, format, "Buff1 bits", buff1);
	  dump_short (dumpfile, format, "Buff2 bits", buff2);
	  dump_byte  (dumpfile, format, "Write byte", bytebuff);
	  dump_info  (dumpfile, format, "","Ready bits:  %d, %s", ready_bits, action); 
	  }
	}
      }

    /* catch any trailing bits at the end of the line */
    if (ready_bits > 0)
      {
      bytebuff = (buff2 >> 8);
      *dst++ = bytebuff;
      if ((dumpfile != NULL) && (level == 3))
	{
	dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, src_byte, src_bit, dst - out);
	dump_byte (dumpfile, format, "Final bits", bytebuff);
	}
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateSamples16bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateSamples16bits */

static int
combineSeparateSamples24bits (uint8 *in[], uint8 *out, uint32 cols,
                              uint32 rows, uint16 spp, uint16 bps, 
	                      FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0, bytes_per_sample = 0;
  uint32 src_rowsize, dst_rowsize; 
  uint32 bit_offset, src_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint32 maskbits = 0, matchbits = 0;
  uint32 buff1 = 0, buff2 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateSamples24bits","Invalid input or output buffer");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 
  src_rowsize = ((bps * cols) + 7) / 8;
  dst_rowsize = ((bps * cols * spp) + 7) / 8;
  maskbits =  (uint32)-1 >> ( 32 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (32 - src_bit - bps); 
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        if (little_endian)
	  buff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
        else
	  buff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
	buff1 = (buff1 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 16)
	  {
	    bytebuff1 = (buff2 >> 24);
	    *dst++ = bytebuff1;
	    bytebuff2 = (buff2 >> 16);
	    *dst++ = bytebuff2;
	    ready_bits -= 16;

	    /* shift in new bits */
	    buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
	    strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	    bytebuff1 = bytebuff2 = 0;
	    buff2 = (buff2 | (buff1 >> ready_bits));
	    strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  {
	  dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, s, src_byte, src_bit, dst - out);
	  dump_long (dumpfile, format, "Match bits ", matchbits);
	  dump_data (dumpfile, format, "Src   bits ", src, 4);
	  dump_long (dumpfile, format, "Buff1 bits ", buff1);
	  dump_long (dumpfile, format, "Buff2 bits ", buff2);
	  dump_byte (dumpfile, format, "Write bits1", bytebuff1);
	  dump_byte (dumpfile, format, "Write bits2", bytebuff2);
	  dump_info (dumpfile, format, "","Ready bits:   %d, %s", ready_bits, action); 
	  }
	}
      }

    /* catch any trailing bits at the end of the line */
    while (ready_bits > 0)
      {
	bytebuff1 = (buff2 >> 24);
	*dst++ = bytebuff1;

	buff2 = (buff2 << 8);
	bytebuff2 = bytebuff1;
	ready_bits -= 8;
      }
 
    if ((dumpfile != NULL) && (level == 3))
      {
      dump_info (dumpfile, format, "",
		   "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		   row + 1, col + 1, src_byte, src_bit, dst - out);

      dump_long (dumpfile, format, "Match bits ", matchbits);
      dump_data (dumpfile, format, "Src   bits ", src, 4);
      dump_long (dumpfile, format, "Buff1 bits ", buff1);
      dump_long (dumpfile, format, "Buff2 bits ", buff2);
      dump_byte (dumpfile, format, "Write bits1", bytebuff1);
      dump_byte (dumpfile, format, "Write bits2", bytebuff2);
      dump_info (dumpfile, format, "", "Ready bits:  %2d", ready_bits); 
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateSamples24bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }
  
  return (0);
  } /* end combineSeparateSamples24bits */

static int
combineSeparateSamples32bits (uint8 *in[], uint8 *out, uint32 cols,
                              uint32 rows, uint16 spp, uint16 bps, 
	                      FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0, bytes_per_sample = 0, shift_width = 0;
  uint32 src_rowsize, dst_rowsize, bit_offset, src_offset;
  uint32 src_byte = 0, src_bit = 0;
  uint32 row, col;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 maskbits = 0, matchbits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateSamples32bits","Invalid input or output buffer");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 
  src_rowsize = ((bps * cols) + 7) / 8;
  dst_rowsize = ((bps * cols * spp) + 7) / 8;
  maskbits =  (uint64)-1 >> ( 64 - bps);
  shift_width = ((bps + 7) / 8) + 1; 

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (64 - src_bit - bps); 
      for (s = 0; s < spp; s++)
	{
	src = in[s] + src_offset + src_byte;
	if (little_endian)
	  {
	  longbuff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
          longbuff2 = longbuff1;
	  }
	else
	  {
	  longbuff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
          longbuff2 = longbuff1;
	  }
	buff3 = ((uint64)longbuff1 << 32) | longbuff2;
	buff1 = (buff3 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 32)
	  {
	  bytebuff1 = (buff2 >> 56);
	  *dst++ = bytebuff1;
	  bytebuff2 = (buff2 >> 48);
	  *dst++ = bytebuff2;
	  bytebuff3 = (buff2 >> 40);
	  *dst++ = bytebuff3;
	  bytebuff4 = (buff2 >> 32);
	  *dst++ = bytebuff4;
	  ready_bits -= 32;
                    
	  /* shift in new bits */
	  buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
	  strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	  bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
	  buff2 = (buff2 | (buff1 >> ready_bits));
	  strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  { 
	  dump_info (dumpfile, format, "",
		     "Row %3d, Col %3d, Sample %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		     row + 1, col + 1, s, src_byte, src_bit, dst - out);
	  dump_wide (dumpfile, format, "Match bits ", matchbits);
	  dump_data (dumpfile, format, "Src   bits ", src, 8);
	  dump_wide (dumpfile, format, "Buff1 bits ", buff1);
	  dump_wide (dumpfile, format, "Buff2 bits ", buff2);
	  dump_info (dumpfile, format, "", "Ready bits:   %d, %s", ready_bits, action); 
	  }
	}
      }
    while (ready_bits > 0)
      {
      bytebuff1 = (buff2 >> 56);
      *dst++ = bytebuff1;
      buff2 = (buff2 << 8);
      ready_bits -= 8;
      }

    if ((dumpfile != NULL) && (level == 3))
      {
      dump_info (dumpfile, format, "",
	         "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		 row + 1, col + 1, src_byte, src_bit, dst - out);

      dump_long (dumpfile, format, "Match bits ", matchbits);
      dump_data (dumpfile, format, "Src   bits ", src, 4);
      dump_long (dumpfile, format, "Buff1 bits ", buff1);
      dump_long (dumpfile, format, "Buff2 bits ", buff2);
      dump_byte (dumpfile, format, "Write bits1", bytebuff1);
      dump_byte (dumpfile, format, "Write bits2", bytebuff2);
      dump_info (dumpfile, format, "", "Ready bits:  %2d", ready_bits); 
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateSamples32bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out);
      }
    }
  
  return (0);
  } /* end combineSeparateSamples32bits */

static int 
combineSeparateTileSamplesBytes (unsigned char *srcbuffs[], unsigned char *out,
                                 uint32 cols, uint32 rows, uint32 imagewidth,
                                 uint32 tw, uint16 spp, uint16 bps,
                                 FILE *dumpfile, int format, int level)
  {
  int i, bytes_per_sample;
  uint32 row, col, col_offset, src_rowsize, dst_rowsize, src_offset;
  unsigned char *src;
  unsigned char *dst;
  tsample_t s;

  src = srcbuffs[0];
  dst = out;
  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateTileSamplesBytes","Invalid buffer address");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8; 
  src_rowsize = ((bps * tw) + 7) / 8;
  dst_rowsize = imagewidth * bytes_per_sample * spp;
  for (row = 0; row < rows; row++)
    {
    if ((dumpfile != NULL) && (level == 2))
      {
      for (s = 0; s < spp; s++)
        {
        dump_info (dumpfile, format, "combineSeparateTileSamplesBytes","Input data, Sample %d", s);
        dump_buffer(dumpfile, format, 1, cols, row, srcbuffs[s] + (row * src_rowsize));
        }
      }
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
#ifdef DEVELMODE
    TIFFError("","Tile row %4d, Src offset %6d   Dst offset %6d", 
              row, src_offset, dst - out);
#endif
    for (col = 0; col < cols; col++)
      {
      col_offset = src_offset + (col * (bps / 8)); 
      for (s = 0; (s < spp) && (s < MAX_SAMPLES); s++)
        {
        src = srcbuffs[s] + col_offset; 
        for (i = 0; i < bytes_per_sample; i++)
          *(dst + i) = *(src + i);
        dst += bytes_per_sample;
        }   
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateTileSamplesBytes","Output data, combined samples");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateTileSamplesBytes */

static int
combineSeparateTileSamples8bits (uint8 *in[], uint8 *out, uint32 cols,
                                 uint32 rows, uint32 imagewidth, 
                                 uint32 tw, uint16 spp, uint16 bps, 
 	                         FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0;
  uint32 src_rowsize, dst_rowsize, src_offset; 
  uint32 bit_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint8  maskbits = 0, matchbits = 0;
  uint8  buff1 = 0, buff2 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[32];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateTileSamples8bits","Invalid input or output buffer");
    return (1);
    }

  src_rowsize = ((bps * tw) + 7) / 8;
  dst_rowsize = ((imagewidth * bps * spp) + 7) / 8;
  maskbits =  (uint8)-1 >> ( 8 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (8 - src_bit - bps); 
      /* load up next sample from each plane */
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        buff1 = ((*src) & matchbits) << (src_bit);

        /* If we have a full buffer's worth, write it out */
        if (ready_bits >= 8)
          {
          *dst++ = buff2;
          buff2 = buff1;
          ready_bits -= 8;
          strcpy (action, "Flush");
          }
        else
          {
          buff2 = (buff2 | (buff1 >> ready_bits));
          strcpy (action, "Update");
          }
        ready_bits += bps;
 
        if ((dumpfile != NULL) && (level == 3))
          {
          dump_info (dumpfile, format, "",
                   "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		   row + 1, col + 1, s, src_byte, src_bit, dst - out);
          dump_byte (dumpfile, format, "Match bits", matchbits);
          dump_byte (dumpfile, format, "Src   bits", *src);
          dump_byte (dumpfile, format, "Buff1 bits", buff1);
          dump_byte (dumpfile, format, "Buff2 bits", buff2);
          dump_info (dumpfile, format, "","%s", action); 
	  }
        }
      }

    if (ready_bits > 0)
      {
      buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
      *dst++ = buff1;
      if ((dumpfile != NULL) && (level == 3))
        {
        dump_info (dumpfile, format, "",
	         "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
	         row + 1, col + 1, src_byte, src_bit, dst - out);
                 dump_byte (dumpfile, format, "Final bits", buff1);
        }
      }

    if ((dumpfile != NULL) && (level >= 2))
      {
      dump_info (dumpfile, format, "combineSeparateTileSamples8bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateTileSamples8bits */

static int
combineSeparateTileSamples16bits (uint8 *in[], uint8 *out, uint32 cols,
                                  uint32 rows, uint32 imagewidth, 
                                  uint32 tw, uint16 spp, uint16 bps, 
 	                          FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0;
  uint32 src_rowsize, dst_rowsize; 
  uint32 bit_offset, src_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint16 maskbits = 0, matchbits = 0;
  uint16 buff1 = 0, buff2 = 0;
  uint8  bytebuff = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateTileSamples16bits","Invalid input or output buffer");
    return (1);
    }

  src_rowsize = ((bps * tw) + 7) / 8;
  dst_rowsize = ((imagewidth * bps * spp) + 7) / 8;
  maskbits = (uint16)-1 >> (16 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (16 - src_bit - bps); 
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        if (little_endian)
          buff1 = (src[0] << 8) | src[1];
        else
          buff1 = (src[1] << 8) | src[0];
	buff1 = (buff1 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 8)
	  {
	    bytebuff = (buff2 >> 8);
	    *dst++ = bytebuff;
	    ready_bits -= 8;
	    /* shift in new bits */
	    buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
	    strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	    bytebuff = 0;
	    buff2 = (buff2 | (buff1 >> ready_bits));
	    strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  {
	  dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, s, src_byte, src_bit, dst - out);

	  dump_short (dumpfile, format, "Match bits", matchbits);
	  dump_data  (dumpfile, format, "Src   bits", src, 2);
	  dump_short (dumpfile, format, "Buff1 bits", buff1);
	  dump_short (dumpfile, format, "Buff2 bits", buff2);
	  dump_byte  (dumpfile, format, "Write byte", bytebuff);
	  dump_info  (dumpfile, format, "","Ready bits:  %d, %s", ready_bits, action); 
	  }
	}
      }

    /* catch any trailing bits at the end of the line */
    if (ready_bits > 0)
      {
      bytebuff = (buff2 >> 8);
      *dst++ = bytebuff;
      if ((dumpfile != NULL) && (level == 3))
	{
	dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, src_byte, src_bit, dst - out);
	dump_byte (dumpfile, format, "Final bits", bytebuff);
	}
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateTileSamples16bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }

  return (0);
  } /* end combineSeparateTileSamples16bits */

static int
combineSeparateTileSamples24bits (uint8 *in[], uint8 *out, uint32 cols,
                                  uint32 rows, uint32 imagewidth, 
                                  uint32 tw, uint16 spp, uint16 bps, 
 	                          FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0;
  uint32 src_rowsize, dst_rowsize; 
  uint32 bit_offset, src_offset;
  uint32 row, col, src_byte = 0, src_bit = 0;
  uint32 maskbits = 0, matchbits = 0;
  uint32 buff1 = 0, buff2 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateTileSamples24bits","Invalid input or output buffer");
    return (1);
    }

  src_rowsize = ((bps * tw) + 7) / 8;
  dst_rowsize = ((imagewidth * bps * spp) + 7) / 8;
  maskbits =  (uint32)-1 >> ( 32 - bps);

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (32 - src_bit - bps); 
      for (s = 0; s < spp; s++)
        {
	src = in[s] + src_offset + src_byte;
        if (little_endian)
	  buff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
        else
	  buff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
	buff1 = (buff1 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 16)
	  {
	    bytebuff1 = (buff2 >> 24);
	    *dst++ = bytebuff1;
	    bytebuff2 = (buff2 >> 16);
	    *dst++ = bytebuff2;
	    ready_bits -= 16;

	    /* shift in new bits */
	    buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
	    strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	    bytebuff1 = bytebuff2 = 0;
	    buff2 = (buff2 | (buff1 >> ready_bits));
	    strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  {
	  dump_info (dumpfile, format, "",
		       "Row %3d, Col %3d, Samples %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		       row + 1, col + 1, s, src_byte, src_bit, dst - out);
	  dump_long (dumpfile, format, "Match bits ", matchbits);
	  dump_data (dumpfile, format, "Src   bits ", src, 4);
	  dump_long (dumpfile, format, "Buff1 bits ", buff1);
	  dump_long (dumpfile, format, "Buff2 bits ", buff2);
	  dump_byte (dumpfile, format, "Write bits1", bytebuff1);
	  dump_byte (dumpfile, format, "Write bits2", bytebuff2);
	  dump_info (dumpfile, format, "","Ready bits:   %d, %s", ready_bits, action); 
	  }
	}
      }

    /* catch any trailing bits at the end of the line */
    while (ready_bits > 0)
      {
	bytebuff1 = (buff2 >> 24);
	*dst++ = bytebuff1;

	buff2 = (buff2 << 8);
	bytebuff2 = bytebuff1;
	ready_bits -= 8;
      }
 
    if ((dumpfile != NULL) && (level == 3))
      {
      dump_info (dumpfile, format, "",
		   "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		   row + 1, col + 1, src_byte, src_bit, dst - out);

      dump_long (dumpfile, format, "Match bits ", matchbits);
      dump_data (dumpfile, format, "Src   bits ", src, 4);
      dump_long (dumpfile, format, "Buff1 bits ", buff1);
      dump_long (dumpfile, format, "Buff2 bits ", buff2);
      dump_byte (dumpfile, format, "Write bits1", bytebuff1);
      dump_byte (dumpfile, format, "Write bits2", bytebuff2);
      dump_info (dumpfile, format, "", "Ready bits:  %2d", ready_bits); 
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateTileSamples24bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out + (row * dst_rowsize));
      }
    }
  
  return (0);
  } /* end combineSeparateTileSamples24bits */

static int
combineSeparateTileSamples32bits (uint8 *in[], uint8 *out, uint32 cols,
                                  uint32 rows, uint32 imagewidth, 
                                  uint32 tw, uint16 spp, uint16 bps, 
 	                          FILE *dumpfile, int format, int level)
  {
  int    ready_bits = 0, shift_width = 0;
  uint32 src_rowsize, dst_rowsize, bit_offset, src_offset;
  uint32 src_byte = 0, src_bit = 0;
  uint32 row, col;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 maskbits = 0, matchbits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  tsample_t s;
  unsigned char *src = in[0];
  unsigned char *dst = out;
  char           action[8];

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("combineSeparateTileSamples32bits","Invalid input or output buffer");
    return (1);
    }

  src_rowsize = ((bps * tw) + 7) / 8;
  dst_rowsize = ((imagewidth * bps * spp) + 7) / 8;
  maskbits =  (uint64)-1 >> ( 64 - bps);
  shift_width = ((bps + 7) / 8) + 1; 

  for (row = 0; row < rows; row++)
    {
    ready_bits = 0;
    buff1 = buff2 = 0;
    dst = out + (row * dst_rowsize);
    src_offset = row * src_rowsize;
    for (col = 0; col < cols; col++)
      {
      /* Compute src byte(s) and bits within byte(s) */
      bit_offset = col * bps;
      src_byte = bit_offset / 8;
      src_bit  = bit_offset % 8;

      matchbits = maskbits << (64 - src_bit - bps); 
      for (s = 0; s < spp; s++)
	{
	src = in[s] + src_offset + src_byte;
	if (little_endian)
	  {
	  longbuff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
	  longbuff2 = longbuff1;
	  }
	else
	  {
	  longbuff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
          longbuff2 = longbuff1;
	  }

	buff3 = ((uint64)longbuff1 << 32) | longbuff2;
	buff1 = (buff3 & matchbits) << (src_bit);

	/* If we have a full buffer's worth, write it out */
	if (ready_bits >= 32)
	  {
	  bytebuff1 = (buff2 >> 56);
	  *dst++ = bytebuff1;
	  bytebuff2 = (buff2 >> 48);
	  *dst++ = bytebuff2;
	  bytebuff3 = (buff2 >> 40);
	  *dst++ = bytebuff3;
	  bytebuff4 = (buff2 >> 32);
	  *dst++ = bytebuff4;
	  ready_bits -= 32;
                    
	  /* shift in new bits */
	  buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
	  strcpy (action, "Flush");
	  }
	else
	  { /* add another bps bits to the buffer */
	  bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
	  buff2 = (buff2 | (buff1 >> ready_bits));
	  strcpy (action, "Update");
	  }
	ready_bits += bps;

	if ((dumpfile != NULL) && (level == 3))
	  { 
	  dump_info (dumpfile, format, "",
		     "Row %3d, Col %3d, Sample %d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		     row + 1, col + 1, s, src_byte, src_bit, dst - out);
	  dump_wide (dumpfile, format, "Match bits ", matchbits);
	  dump_data (dumpfile, format, "Src   bits ", src, 8);
	  dump_wide (dumpfile, format, "Buff1 bits ", buff1);
	  dump_wide (dumpfile, format, "Buff2 bits ", buff2);
	  dump_info (dumpfile, format, "", "Ready bits:   %d, %s", ready_bits, action); 
	  }
	}
      }
    while (ready_bits > 0)
      {
      bytebuff1 = (buff2 >> 56);
      *dst++ = bytebuff1;
      buff2 = (buff2 << 8);
      ready_bits -= 8;
      }

    if ((dumpfile != NULL) && (level == 3))
      {
      dump_info (dumpfile, format, "",
	         "Row %3d, Col %3d, Src byte offset %3d  bit offset %2d  Dst offset %3d",
		 row + 1, col + 1, src_byte, src_bit, dst - out);

      dump_long (dumpfile, format, "Match bits ", matchbits);
      dump_data (dumpfile, format, "Src   bits ", src, 4);
      dump_long (dumpfile, format, "Buff1 bits ", buff1);
      dump_long (dumpfile, format, "Buff2 bits ", buff2);
      dump_byte (dumpfile, format, "Write bits1", bytebuff1);
      dump_byte (dumpfile, format, "Write bits2", bytebuff2);
      dump_info (dumpfile, format, "", "Ready bits:  %2d", ready_bits); 
      }

    if ((dumpfile != NULL) && (level == 2))
      {
      dump_info (dumpfile, format, "combineSeparateTileSamples32bits","Output data");
      dump_buffer(dumpfile, format, 1, dst_rowsize, row, out);
      }
    }
  
  return (0);
  } /* end combineSeparateTileSamples32bits */


static int readSeparateStripsIntoBuffer (TIFF *in, uint8 *obuf, uint32 length, 
                                         uint32 width, uint16 spp,
                                         struct dump_opts *dump)
  {
  int i, j, bytes_per_sample, bytes_per_pixel, shift_width, result = 1;
  int32  bytes_read = 0;
  uint16 bps, nstrips, planar, strips_per_sample;
  uint32 src_rowsize, dst_rowsize, rows_processed, rps;
  uint32 rows_this_strip = 0;
  tsample_t s;
  tstrip_t  strip;
  tsize_t scanlinesize = TIFFScanlineSize(in);
  tsize_t stripsize    = TIFFStripSize(in);
  unsigned char *srcbuffs[MAX_SAMPLES];
  unsigned char *buff = NULL;
  unsigned char *dst = NULL;

  if (obuf == NULL)
    {
    TIFFError("readSeparateStripsIntoBuffer","Invalid buffer argument");
    return (0);
    }

  memset (srcbuffs, '\0', sizeof(srcbuffs));
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetFieldDefaulted(in, TIFFTAG_ROWSPERSTRIP, &rps);
  if (rps > length)
    rps = length;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if (bytes_per_pixel < (bytes_per_sample + 1))
    shift_width = bytes_per_pixel;
  else
    shift_width = bytes_per_sample + 1;

  src_rowsize = ((bps * width) + 7) / 8;
  dst_rowsize = ((bps * width * spp) + 7) / 8;
  dst = obuf;

  if ((dump->infile != NULL) && (dump->level == 3))
    {
    dump_info  (dump->infile, dump->format, "", 
                "Image width %d, length %d, Scanline size, %4d bytes",
                width, length,  scanlinesize);
    dump_info  (dump->infile, dump->format, "", 
                "Bits per sample %d, Samples per pixel %d, Shift width %d",
		bps, spp, shift_width);
    }

  /* Libtiff seems to assume/require that data for separate planes are 
   * written one complete plane after another and not interleaved in any way.
   * Multiple scanlines and possibly strips of the same plane must be 
   * written before data for any other plane.
   */
  nstrips = TIFFNumberOfStrips(in);
  strips_per_sample = nstrips /spp;

  for (s = 0; (s < spp) && (s < MAX_SAMPLES); s++)
    {
    srcbuffs[s] = NULL;
    buff = _TIFFmalloc(stripsize);
    if (!buff)
      {
      TIFFError ("readSeparateStripsIntoBuffer", 
                 "Unable to allocate strip read buffer for sample %d", s);
      for (i = 0; i < s; i++)
        _TIFFfree (srcbuffs[i]);
      return 0;
      }
    srcbuffs[s] = buff;
    }

  rows_processed = 0;
  for (j = 0; (j < strips_per_sample) && (result == 1); j++)
    {
    for (s = 0; (s < spp) && (s < MAX_SAMPLES); s++)
      {
      buff = srcbuffs[s];
      strip = (s * strips_per_sample) + j; 
      bytes_read = TIFFReadEncodedStrip (in, strip, buff, stripsize);
      rows_this_strip = bytes_read / src_rowsize;
      if (bytes_read < 0 && !ignore)
        {
        TIFFError(TIFFFileName(in),
	          "Error, can't read strip %lu for sample %d",
         	   (unsigned long) strip, s + 1);
        result = 0;
        break;
        }
#ifdef DEVELMODE
      TIFFError("", "Strip %2d, read %5d bytes for %4d scanlines, shift width %d", 
		strip, bytes_read, rows_this_strip, shift_width);
#endif
      }

    if (rps > rows_this_strip)
      rps = rows_this_strip;
    dst = obuf + (dst_rowsize * rows_processed);
    if ((bps % 8) == 0)
      {
      if (combineSeparateSamplesBytes (srcbuffs, dst, width, rps,
                                       spp, bps, dump->infile, 
                                       dump->format, dump->level))
        {
        result = 0;
        break;
	}
      }
    else
      {
      switch (shift_width)
        {
        case 1: if (combineSeparateSamples8bits (srcbuffs, dst, width, rps,
                                                 spp, bps, dump->infile,
                                                 dump->format, dump->level))
	          {
                  result = 0;
                  break;
      	          }
	        break;
        case 2: if (combineSeparateSamples16bits (srcbuffs, dst, width, rps,
                                                  spp, bps, dump->infile,
                                                  dump->format, dump->level))
	          {
                  result = 0;
                  break;
		  }
	        break;
        case 3: if (combineSeparateSamples24bits (srcbuffs, dst, width, rps,
                                                  spp, bps, dump->infile,
                                                  dump->format, dump->level))
	          {
                  result = 0;
                  break;
       	          }
                break;
        case 4: 
        case 5:
        case 6:
        case 7:
        case 8: if (combineSeparateSamples32bits (srcbuffs, dst, width, rps,
                                                  spp, bps, dump->infile,
                                                  dump->format, dump->level))
	          {
                  result = 0;
                  break;
		  }
	        break;
        default: TIFFError ("readSeparateStripsIntoBuffer", "Unsupported bit depth: %d", bps);
                  result = 0;
                  break;
        }
      }
 
    if ((rows_processed + rps) > length)
      {
      rows_processed = length;
      rps = length - rows_processed;
      }
    else
      rows_processed += rps;
    }

  /* free any buffers allocated for each plane or scanline and 
   * any temporary buffers 
   */
  for (s = 0; (s < spp) && (s < MAX_SAMPLES); s++)
    {
    buff = srcbuffs[s];
    if (buff != NULL)
      _TIFFfree(buff);
    }

  return (result);
  } /* end readSeparateStripsIntoBuffer */

static int
get_page_geometry (char *name, struct pagedef *page)
    {
    char *ptr;
    int n; 

    for (ptr = name; *ptr; ptr++)
      *ptr = (char)tolower((int)*ptr);

    for (n = 0; n < MAX_PAPERNAMES; n++)
      {
      if (strcmp(name, PaperTable[n].name) == 0)
        {
	page->width = PaperTable[n].width;
	page->length = PaperTable[n].length;
        strncpy (page->name, PaperTable[n].name, 15);
        page->name[15] = '\0';
        return (0);
        }
      }

  return (1);
  }


static void
initPageSetup (struct pagedef *page, struct pageseg *pagelist, 
               struct buffinfo seg_buffs[])
   {
   int i; 

   strcpy (page->name, "");
   page->mode = PAGE_MODE_NONE;
   page->res_unit = RESUNIT_NONE;
   page->hres = 0.0;
   page->vres = 0.0;
   page->width = 0.0;
   page->length = 0.0;
   page->hmargin = 0.0;
   page->vmargin = 0.0;
   page->rows = 0;
   page->cols = 0;
   page->orient = ORIENTATION_NONE;

   for (i = 0; i < MAX_SECTIONS; i++)
     {
     pagelist[i].x1 = (uint32)0;
     pagelist[i].x2 = (uint32)0;
     pagelist[i].y1 = (uint32)0;
     pagelist[i].y2 = (uint32)0;
     pagelist[i].buffsize = (uint32)0;
     pagelist[i].position = 0;
     pagelist[i].total = 0;
     }

   for (i = 0; i < MAX_OUTBUFFS; i++)
     {
     seg_buffs[i].size = 0;
     seg_buffs[i].buffer = NULL;
     }
   }

static void
initImageData (struct image_data *image)
  {
  image->xres = 0.0;
  image->yres = 0.0;
  image->width = 0;
  image->length = 0;
  image->res_unit = RESUNIT_NONE;
  image->bps = 0;
  image->spp = 0;
  image->planar = 0;
  image->photometric = 0;
  image->orientation = 0;
  image->compression = COMPRESSION_NONE;
  image->adjustments = 0;
  }

static void
initCropMasks (struct crop_mask *cps)
   {
   int i;

   cps->crop_mode = CROP_NONE;
   cps->res_unit  = RESUNIT_NONE;
   cps->edge_ref  = EDGE_TOP;
   cps->width = 0;
   cps->length = 0;
   for (i = 0; i < 4; i++)
     cps->margins[i] = 0.0;
   cps->bufftotal = (uint32)0;
   cps->combined_width = (uint32)0;
   cps->combined_length = (uint32)0;
   cps->rotation = (uint16)0;
   cps->photometric = INVERT_DATA_AND_TAG;
   cps->mirror   = (uint16)0;
   cps->invert   = (uint16)0;
   cps->zones    = (uint32)0;
   cps->regions  = (uint32)0;
   for (i = 0; i < MAX_REGIONS; i++)
     {
     cps->corners[i].X1 = 0.0;
     cps->corners[i].X2 = 0.0;
     cps->corners[i].Y1 = 0.0;
     cps->corners[i].Y2 = 0.0;
     cps->regionlist[i].x1 = 0;
     cps->regionlist[i].x2 = 0;
     cps->regionlist[i].y1 = 0;
     cps->regionlist[i].y2 = 0;
     cps->regionlist[i].width = 0;
     cps->regionlist[i].length = 0;
     cps->regionlist[i].buffsize = 0;
     cps->regionlist[i].buffptr = NULL;
     cps->zonelist[i].position = 0;
     cps->zonelist[i].total = 0;
     }
   cps->exp_mode = ONE_FILE_COMPOSITE;
   cps->img_mode = COMPOSITE_IMAGES;
   }

static void initDumpOptions(struct dump_opts *dump)
  {
  dump->debug  = 0;
  dump->format = DUMP_NONE;
  dump->level  = 1;
  sprintf (dump->mode, "w");
  memset (dump->infilename, '\0', PATH_MAX + 1);
  memset (dump->outfilename, '\0',PATH_MAX + 1);
  dump->infile = NULL;
  dump->outfile = NULL;
  }

/* Compute pixel offsets into the image for margins and fixed regions */
static int
computeInputPixelOffsets(struct crop_mask *crop, struct image_data *image,
                         struct offset *off)
  {
  double scale;
  float xres, yres;
  /* Values for these offsets are in pixels from start of image, not bytes,
   * and are indexed from zero to width - 1 or length - 1 */
  uint32 tmargin, bmargin, lmargin, rmargin;
  uint32 startx, endx;   /* offsets of first and last columns to extract */
  uint32 starty, endy;   /* offsets of first and last row to extract */
  uint32 width, length, crop_width, crop_length; 
  uint32 i, max_width, max_length, zwidth, zlength, buffsize;
  uint32 x1, x2, y1, y2;

  if (image->res_unit != RESUNIT_INCH && image->res_unit != RESUNIT_CENTIMETER)
    {
    xres = 1.0;
    yres = 1.0;
    }
  else
    {
    if (((image->xres == 0) || (image->yres == 0)) && 
         (crop->res_unit != RESUNIT_NONE) &&
	((crop->crop_mode & CROP_REGIONS) || (crop->crop_mode & CROP_MARGINS) ||
 	 (crop->crop_mode & CROP_LENGTH)  || (crop->crop_mode & CROP_WIDTH)))
      {
      TIFFError("computeInputPixelOffsets", "Cannot compute margins or fixed size sections without image resolution");
      TIFFError("computeInputPixelOffsets", "Specify units in pixels and try again");
      return (-1);
      }
    xres = image->xres;
    yres = image->yres;
    }

  /* Translate user units to image units */
  scale = 1.0;
  switch (crop->res_unit) {
    case RESUNIT_CENTIMETER:
         if (image->res_unit == RESUNIT_INCH)
	   scale = 1.0/2.54;
	 break;
    case RESUNIT_INCH:
	 if (image->res_unit == RESUNIT_CENTIMETER)
	     scale = 2.54;
	 break;
    case RESUNIT_NONE: /* Dimensions in pixels */
    default:
    break;
    }

  if (crop->crop_mode & CROP_REGIONS)
    {
    max_width = max_length = 0;
    for (i = 0; i < crop->regions; i++)
      {
      if ((crop->res_unit == RESUNIT_INCH) || (crop->res_unit == RESUNIT_CENTIMETER))
        {
	x1 = (uint32) (crop->corners[i].X1 * scale * xres);
	x2 = (uint32) (crop->corners[i].X2 * scale * xres);
	y1 = (uint32) (crop->corners[i].Y1 * scale * yres);
	y2 = (uint32) (crop->corners[i].Y2 * scale * yres);
        }
      else
        {
	x1 = (uint32) (crop->corners[i].X1);
	x2 = (uint32) (crop->corners[i].X2);
	y1 = (uint32) (crop->corners[i].Y1);
	y2 = (uint32) (crop->corners[i].Y2);       
	}
      if (x1 < 1)
        crop->regionlist[i].x1 = 0;
      else
        crop->regionlist[i].x1 = (uint32) (x1 - 1);

      if (x2 > image->width - 1)
        crop->regionlist[i].x2 = image->width - 1;
      else
        crop->regionlist[i].x2 = (uint32) (x2 - 1);
      zwidth  = crop->regionlist[i].x2 - crop->regionlist[i].x1 + 1; 

      if (y1 < 1)
        crop->regionlist[i].y1 = 0;
      else
        crop->regionlist[i].y1 = (uint32) (y1 - 1);

      if (y2 > image->length - 1)
        crop->regionlist[i].y2 = image->length - 1;
      else
        crop->regionlist[i].y2 = (uint32) (y2 - 1);

      zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1; 

      if (zwidth > max_width)
        max_width = zwidth;
      if (zlength > max_length)
        max_length = zlength;

      buffsize = (uint32)
          (((zwidth * image->bps * image->spp + 7 ) / 8) * (zlength + 1));

      crop->regionlist[i].buffsize = buffsize;
      crop->bufftotal += buffsize;
      if (crop->img_mode == COMPOSITE_IMAGES)
        {
        switch (crop->edge_ref)
          {
          case EDGE_LEFT:
          case EDGE_RIGHT:
               crop->combined_length = zlength;
               crop->combined_width += zwidth;
               break;
          case EDGE_BOTTOM:
          case EDGE_TOP:  /* width from left, length from top */
          default:
               crop->combined_width = zwidth;
               crop->combined_length += zlength;
	       break;
          }
	}
      }
    return (0);
    }
  
  /* Convert crop margins into offsets into image
   * Margins are expressed as pixel rows and columns, not bytes
   */
  if (crop->crop_mode & CROP_MARGINS)
    {
    if (crop->res_unit != RESUNIT_INCH && crop->res_unit != RESUNIT_CENTIMETER)
      { /* User has specified pixels as reference unit */
      tmargin = (uint32)(crop->margins[0]);
      lmargin = (uint32)(crop->margins[1]);
      bmargin = (uint32)(crop->margins[2]);
      rmargin = (uint32)(crop->margins[3]);
      }
    else
      { /* inches or centimeters specified */
      tmargin = (uint32)(crop->margins[0] * scale * yres);
      lmargin = (uint32)(crop->margins[1] * scale * xres);
      bmargin = (uint32)(crop->margins[2] * scale * yres);
      rmargin = (uint32)(crop->margins[3] * scale * xres);
      }

    if ((lmargin + rmargin) > image->width)
      {
      TIFFError("computeInputPixelOffsets", "Combined left and right margins exceed image width");
      lmargin = (uint32) 0;
      rmargin = (uint32) 0;
      return (-1);
      }
    if ((tmargin + bmargin) > image->length)
      {
      TIFFError("computeInputPixelOffsets", "Combined top and bottom margins exceed image length"); 
      tmargin = (uint32) 0; 
      bmargin = (uint32) 0;
      return (-1);
      }
    }
  else
    { /* no margins requested */
    tmargin = (uint32) 0;
    lmargin = (uint32) 0;
    bmargin = (uint32) 0;
    rmargin = (uint32) 0;
    }

  /* Width, height, and margins are expressed as pixel offsets into image */
  if (crop->res_unit != RESUNIT_INCH && crop->res_unit != RESUNIT_CENTIMETER)
    {
    if (crop->crop_mode & CROP_WIDTH)
      width = (uint32)crop->width;
    else
      width = image->width - lmargin - rmargin;

    if (crop->crop_mode & CROP_LENGTH)
      length  = (uint32)crop->length;
    else
      length = image->length - tmargin - bmargin;
    }
  else
    {
    if (crop->crop_mode & CROP_WIDTH)
      width = (uint32)(crop->width * scale * image->xres);
    else
      width = image->width - lmargin - rmargin;

    if (crop->crop_mode & CROP_LENGTH)
      length  = (uint32)(crop->length * scale * image->yres);
    else
      length = image->length - tmargin - bmargin;
    }

  off->tmargin = tmargin;
  off->bmargin = bmargin;
  off->lmargin = lmargin;
  off->rmargin = rmargin;

  /* Calculate regions defined by margins, width, and length. 
   * Coordinates expressed as 0 to imagewidth - 1, imagelength - 1,
   * since they are used to compute offsets into buffers */
  switch (crop->edge_ref) {
    case EDGE_BOTTOM:
         startx = lmargin;
         if ((startx + width) >= (image->width - rmargin))
           endx = image->width - rmargin - 1;
         else
           endx = startx + width - 1;

         endy = image->length - bmargin - 1;
         if ((endy - length) <= tmargin)
           starty = tmargin;
         else
           starty = endy - length + 1;
         break;
    case EDGE_RIGHT:
         endx = image->width - rmargin - 1;
         if ((endx - width) <= lmargin)
           startx = lmargin;
         else
           startx = endx - width + 1;

         starty = tmargin;
         if ((starty + length) >= (image->length - bmargin))
           endy = image->length - bmargin - 1;
         else
           endy = starty + length - 1;
         break;
    case EDGE_TOP:  /* width from left, length from top */
    case EDGE_LEFT:
    default:
         startx = lmargin;
         if ((startx + width) >= (image->width - rmargin))
           endx = image->width - rmargin - 1;
         else
           endx = startx + width - 1;

         starty = tmargin;
         if ((starty + length) >= (image->length - bmargin))
           endy = image->length - bmargin - 1;
         else
           endy = starty + length - 1;
         break;
    }
  off->startx = startx;
  off->starty = starty;
  off->endx   = endx;
  off->endy   = endy;

  crop_width  = endx - startx + 1;
  crop_length = endy - starty + 1;

  if (crop_width <= 0)
    {
    TIFFError("computeInputPixelOffsets", 
               "Invalid left/right margins and /or image crop width requested");
    return (-1);
    }
  if (crop_width > image->width)
    crop_width = image->width;

  if (crop_length <= 0)
    {
    TIFFError("computeInputPixelOffsets", 
              "Invalid top/bottom margins and /or image crop length requested");
    return (-1);
    }
  if (crop_length > image->length)
    crop_length = image->length;

  off->crop_width = crop_width;
  off->crop_length = crop_length;

  return (0);
  } /* end computeInputPixelOffsets */

/* 
 * Translate crop options into pixel offsets for one or more regions of the image.
 * Options are applied in this order: margins, specific width and length, zones,
 * but all are optional. Margins are relative to each edge. Width, length and
 * zones are relative to the specified reference edge. Zones are expressed as
 * X:Y where X is the ordinal value in a set of Y equal sized portions. eg.
 * 2:3 would indicate the middle third of the region qualified by margins and
 * any explicit width and length specified. Regions are specified by coordinates
 * of the top left and lower right corners with range 1 to width or height.
 */

static int
getCropOffsets(struct image_data *image, struct crop_mask *crop, struct dump_opts *dump)
  {
  struct offset offsets;
  int    i;
  int32  test;
  uint32 seg, total, need_buff = 0;
  uint32 buffsize;
  uint32 zwidth, zlength;

  memset(&offsets, '\0', sizeof(struct offset));
  crop->bufftotal = 0;
  crop->combined_width  = (uint32)0;
  crop->combined_length = (uint32)0;
  crop->selections = 0;

  /* Compute pixel offsets if margins or fixed width or length specified */
  if ((crop->crop_mode & CROP_MARGINS) ||
      (crop->crop_mode & CROP_REGIONS) ||
      (crop->crop_mode & CROP_LENGTH)  || 
      (crop->crop_mode & CROP_WIDTH))
    {
    if (computeInputPixelOffsets(crop, image, &offsets))
      {
      TIFFError ("getCropOffsets", "Unable to compute crop margins");
      return (-1);
      }
    need_buff = TRUE;
    crop->selections = crop->regions;
    /* Regions are only calculated from top and left edges with no margins */
    if (crop->crop_mode & CROP_REGIONS)
      return (0);
    }
  else
    { /* cropped area is the full image */
    offsets.tmargin = 0;
    offsets.lmargin = 0;
    offsets.bmargin = 0;
    offsets.rmargin = 0;
    offsets.crop_width = image->width;
    offsets.crop_length = image->length;
    offsets.startx = 0;
    offsets.endx = image->width - 1;
    offsets.starty = 0;
    offsets.endy = image->length - 1;
    need_buff = FALSE;
    }

  if (dump->outfile != NULL)
    {
    dump_info (dump->outfile, dump->format, "", "Margins: Top: %d  Left: %d  Bottom: %d  Right: %d", 
           offsets.tmargin, offsets.lmargin, offsets.bmargin, offsets.rmargin); 
    dump_info (dump->outfile, dump->format, "", "Crop region within margins: Adjusted Width:  %6d  Length: %6d", 
           offsets.crop_width, offsets.crop_length);
    }

  if (!(crop->crop_mode & CROP_ZONES)) /* no crop zones requested */
    {
    if (need_buff == FALSE)  /* No margins or fixed width or length areas */
      {
      crop->selections = 0;
      crop->combined_width  = image->width;
      crop->combined_length = image->length;
      return (0);
      }
    else 
      {
      /* Use one region for margins and fixed width or length areas
       * even though it was not formally declared as a region.
       */
      crop->selections = 1;
      crop->zones = 1;
      crop->zonelist[0].total = 1;
      crop->zonelist[0].position = 1;
      }
    }     
  else
    crop->selections = crop->zones;

  for (i = 0; i < crop->zones; i++)
    {
    seg = crop->zonelist[i].position;
    total = crop->zonelist[i].total;

    switch (crop->edge_ref) 
      {
      case EDGE_LEFT: /* zones from left to right, length from top */
           zlength = offsets.crop_length;
	   crop->regionlist[i].y1 = offsets.starty;
           crop->regionlist[i].y2 = offsets.endy;

           crop->regionlist[i].x1 = offsets.startx + 
                                  (uint32)(offsets.crop_width * 1.0 * (seg - 1) / total);
           test = (int32)offsets.startx + 
                  (int32)(offsets.crop_width * 1.0 * seg / total);
           if (test < 1 )
             crop->regionlist[i].x2 = 0;
           else
	     {
	     if (test > (int32)(image->width - 1))
               crop->regionlist[i].x2 = image->width - 1;
             else
	       crop->regionlist[i].x2 = test - 1;
             }
           zwidth = crop->regionlist[i].x2 - crop->regionlist[i].x1  + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           crop->combined_length = (uint32)zlength;
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_width += (uint32)zwidth;
           else
             crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_BOTTOM: /* width from left, zones from bottom to top */
           zwidth = offsets.crop_width;
	   crop->regionlist[i].x1 = offsets.startx;
           crop->regionlist[i].x2 = offsets.endx;

           test = offsets.endy - (uint32)(offsets.crop_length * 1.0 * seg / total);
           if (test < 1 )
	     crop->regionlist[i].y1 = 0;
           else
	     crop->regionlist[i].y1 = test + 1;

           test = offsets.endy - (offsets.crop_length * 1.0 * (seg - 1) / total);
           if (test < 1 )
             crop->regionlist[i].y2 = 0;
           else
	     {
             if (test > (int32)(image->length - 1))
               crop->regionlist[i].y2 = image->length - 1;
             else 
               crop->regionlist[i].y2 = test;
	     }
           zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_length += (uint32)zlength;
           else
             crop->combined_length = (uint32)zlength;
           crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_RIGHT: /* zones from right to left, length from top */
           zlength = offsets.crop_length;
	   crop->regionlist[i].y1 = offsets.starty;
           crop->regionlist[i].y2 = offsets.endy;

           crop->regionlist[i].x1 = offsets.startx +
                                  (uint32)(offsets.crop_width  * (total - seg) * 1.0 / total);
           test = offsets.startx + 
	          (offsets.crop_width * (total - seg + 1) * 1.0 / total);
           if (test < 1 )
             crop->regionlist[i].x2 = 0;
           else
	     {
	     if (test > (int32)(image->width - 1))
               crop->regionlist[i].x2 = image->width - 1;
             else
               crop->regionlist[i].x2 = test - 1;
             }
           zwidth = crop->regionlist[i].x2 - crop->regionlist[i].x1  + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           crop->combined_length = (uint32)zlength;
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_width += (uint32)zwidth;
           else
             crop->combined_width = (uint32)zwidth;
           break;
      case EDGE_TOP: /* width from left, zones from top to bottom */
      default:
           zwidth = offsets.crop_width;
	   crop->regionlist[i].x1 = offsets.startx;
           crop->regionlist[i].x2 = offsets.endx;

           crop->regionlist[i].y1 = offsets.starty + (uint32)(offsets.crop_length * 1.0 * (seg - 1) / total);
           test = offsets.starty + (uint32)(offsets.crop_length * 1.0 * seg / total);
           if (test < 1 )
             crop->regionlist[i].y2 = 0;
           else
	     {
	     if (test > (int32)(image->length - 1))
	       crop->regionlist[i].y2 = image->length - 1;
             else
	       crop->regionlist[i].y2 = test - 1;
	     }
           zlength = crop->regionlist[i].y2 - crop->regionlist[i].y1 + 1;

	   /* This is passed to extractCropZone or extractCompositeZones */
           if (crop->exp_mode == COMPOSITE_IMAGES)
             crop->combined_length += (uint32)zlength;
           else
             crop->combined_length = (uint32)zlength;
           crop->combined_width = (uint32)zwidth;
           break;
      } /* end switch statement */

    buffsize = (uint32)
          ((((zwidth * image->bps * image->spp) + 7 ) / 8) * (zlength + 1));
    crop->regionlist[i].width = (uint32) zwidth;
    crop->regionlist[i].length = (uint32) zlength;
    crop->regionlist[i].buffsize = buffsize;
    crop->bufftotal += buffsize;


  if (dump->outfile != NULL)
    dump_info (dump->outfile, dump->format, "",  "Zone %d, width: %4d, length: %4d, x1: %4d  x2: %4d  y1: %4d  y2: %4d",
                    i + 1, (uint32)zwidth, (uint32)zlength,
		    crop->regionlist[i].x1, crop->regionlist[i].x2, 
                    crop->regionlist[i].y1, crop->regionlist[i].y2);
    }

  return (0);
  } /* end getCropOffsets */


static int
computeOutputPixelOffsets (struct crop_mask *crop, struct image_data *image,
                           struct pagedef *page, struct pageseg *sections,
                           struct dump_opts* dump)
  {
  double scale;
  double pwidth, plength;          /* Output page width and length in user units*/
  uint32 iwidth, ilength;          /* Input image width and length in pixels*/
  uint32 owidth, olength;          /* Output image width and length in pixels*/
  uint32 orows, ocols;             /* rows and cols for output */
  uint32 hmargin, vmargin;         /* Horizontal and vertical margins */
  uint32 x1, x2, y1, y2, line_bytes;
  unsigned int orientation;
  uint32 i, j, k;
 
  scale = 1.0;
  if (page->res_unit == RESUNIT_NONE)
    page->res_unit = image->res_unit;

  switch (image->res_unit) {
    case RESUNIT_CENTIMETER:
         if (page->res_unit == RESUNIT_INCH)
	   scale = 1.0/2.54;
	 break;
    case RESUNIT_INCH:
	 if (page->res_unit == RESUNIT_CENTIMETER)
	     scale = 2.54;
	 break;
    case RESUNIT_NONE: /* Dimensions in pixels */
    default:
    break;
    }

  /* get width, height, resolutions of input image selection */
  if (crop->combined_width > 0)
    iwidth = crop->combined_width;
  else
    iwidth = image->width;
  if (crop->combined_length > 0)
    ilength = crop->combined_length;
  else
    ilength = image->length;

  if (page->hres <= 1.0)
    page->hres = image->xres;
  if (page->vres <= 1.0)
    page->vres = image->yres;

  if ((page->hres < 1.0) || (page->vres < 1.0))
    {
    TIFFError("computeOutputPixelOffsets",
    "Invalid horizontal or vertical resolution specified or read from input image");
    return (1);
    }

  /* If no page sizes are being specified, we just use the input image size to
   * calculate maximum margins that can be taken from image.
   */
  if (page->width <= 0)
    pwidth = iwidth;
  else
    pwidth = page->width;

  if (page->length <= 0)
    plength = ilength;
  else
    plength = page->length;

  if (dump->debug)
    {
    TIFFError("", "Page size: %s, Vres: %3.2f, Hres: %3.2f, "
                   "Hmargin: %3.2f, Vmargin: %3.2f",
	     page->name, page->vres, page->hres,
             page->hmargin, page->vmargin);
    TIFFError("", "Res_unit: %d, Scale: %3.2f, Page width: %3.2f, length: %3.2f", 
           page->res_unit, scale, pwidth, plength);
    }

  /* compute margins at specified unit and resolution */
  if (page->mode & PAGE_MODE_MARGINS)
    {
    if (page->res_unit == RESUNIT_INCH || page->res_unit == RESUNIT_CENTIMETER)
      { /* inches or centimeters specified */
      hmargin = (uint32)(page->hmargin * scale * page->hres * ((image->bps + 7)/ 8));
      vmargin = (uint32)(page->vmargin * scale * page->vres * ((image->bps + 7)/ 8));
      }
    else
      { /* Otherwise user has specified pixels as reference unit */
      hmargin = (uint32)(page->hmargin * scale * ((image->bps + 7)/ 8));
      vmargin = (uint32)(page->vmargin * scale * ((image->bps + 7)/ 8));
      }

    if ((hmargin * 2.0) > (pwidth * page->hres))
      {
      TIFFError("computeOutputPixelOffsets", 
                "Combined left and right margins exceed page width");
      hmargin = (uint32) 0;
      return (-1);
      }
    if ((vmargin * 2.0) > (plength * page->vres))
      {
      TIFFError("computeOutputPixelOffsets", 
                "Combined top and bottom margins exceed page length"); 
      vmargin = (uint32) 0; 
      return (-1);
      }
    }
  else
    {
    hmargin = 0;
    vmargin = 0;
    }

  if (page->mode & PAGE_MODE_ROWSCOLS )
    {
    /* Maybe someday but not for now */
    if (page->mode & PAGE_MODE_MARGINS)
      TIFFError("computeOutputPixelOffsets", 
      "Output margins cannot be specified with rows and columns"); 

    owidth  = TIFFhowmany(iwidth, page->cols);
    olength = TIFFhowmany(ilength, page->rows);
    }
  else
    {
    if (page->mode & PAGE_MODE_PAPERSIZE )
      {
      owidth  = (uint32)((pwidth * page->hres) - (hmargin * 2));
      olength = (uint32)((plength * page->vres) - (vmargin * 2));
      }
    else
      {
      owidth = (uint32)(iwidth - (hmargin * 2 * page->hres));
      olength = (uint32)(ilength - (vmargin * 2 * page->vres));
      }
    }

  if (owidth > iwidth)
    owidth = iwidth;
  if (olength > ilength)
    olength = ilength;

  /* Compute the number of pages required for Portrait or Landscape */
  switch (page->orient)
    {
    case ORIENTATION_NONE:
    case ORIENTATION_PORTRAIT:
         ocols = TIFFhowmany(iwidth, owidth);
         orows = TIFFhowmany(ilength, olength);
         orientation = ORIENTATION_PORTRAIT;
         break;

    case ORIENTATION_LANDSCAPE:
         ocols = TIFFhowmany(iwidth, olength);
         orows = TIFFhowmany(ilength, owidth);
         x1 = olength;
         olength = owidth;
         owidth = x1;
         orientation = ORIENTATION_LANDSCAPE;
         break;

    case ORIENTATION_AUTO:
    default:
         x1 = TIFFhowmany(iwidth, owidth);
         x2 = TIFFhowmany(ilength, olength); 
         y1 = TIFFhowmany(iwidth, olength);
         y2 = TIFFhowmany(ilength, owidth); 

         if ( (x1 * x2) < (y1 * y2))
           { /* Portrait */
           ocols = x1;
           orows = x2;
           orientation = ORIENTATION_PORTRAIT;
	   }
         else
           { /* Landscape */
           ocols = y1;
           orows = y2;
           x1 = olength;
           olength = owidth;
           owidth = x1;
           orientation = ORIENTATION_LANDSCAPE;
           }
    }

  if (ocols < 1)
    ocols = 1;
  if (orows < 1)
    orows = 1;

  /* If user did not specify rows and cols, set them from calcuation */
  if (page->rows < 1)
    page->rows = orows;
  if (page->cols < 1)
    page->cols = ocols;

  line_bytes = TIFFhowmany8(owidth * image->bps) * image->spp;

  if ((page->rows * page->cols) > MAX_SECTIONS)
   {
   TIFFError("computeOutputPixelOffsets",
	     "Rows and Columns exceed maximum sections\nIncrease resolution or reduce sections");
   return (-1);
   }

  /* build the list of offsets for each output section */
  for (k = 0, i = 0 && k <= MAX_SECTIONS; i < orows; i++)
    {
    y1 = (uint32)(olength * i);
    y2 = (uint32)(olength * (i +  1) - 1);
    if (y2 >= ilength)
      y2 = ilength - 1;
    for (j = 0; j < ocols; j++, k++)
      {
      x1 = (uint32)(owidth * j); 
      x2 = (uint32)(owidth * (j + 1) - 1);
      if (x2 >= iwidth)
        x2 = iwidth - 1;
      sections[k].x1 = x1;
      sections[k].x2 = x2;
      sections[k].y1 = y1;
      sections[k].y2 = y2;
      sections[k].buffsize = line_bytes * olength;
      sections[k].position = k + 1;
      sections[k].total = orows * ocols;
      } 
    } 
  return (0);
  } /* end computeOutputPixelOffsets */

static int
loadImage(TIFF* in, struct image_data *image, struct dump_opts *dump, unsigned char **read_ptr)
  {
  uint32   i;
  float    xres = 0.0, yres = 0.0;
  uint16   nstrips = 0, ntiles = 0, planar = 0;
  uint16   bps = 0, spp = 0, res_unit = 0;
  uint16   orientation = 0;
  uint16   input_compression = 0, input_photometric = 0;
  uint16   subsampling_horiz, subsampling_vert;
  uint32   width = 0, length = 0;
  uint32   stsize = 0, tlsize = 0, buffsize = 0, scanlinesize = 0;
  uint32   tw = 0, tl = 0;       /* Tile width and length */
  uint32   tile_rowsize = 0;
  unsigned char *read_buff = NULL;
  unsigned char *new_buff  = NULL;
  int      readunit = 0;
  static   uint32  prev_readsize = 0;

  TIFFGetFieldDefaulted(in, TIFFTAG_BITSPERSAMPLE, &bps);
  TIFFGetFieldDefaulted(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &planar);
  TIFFGetFieldDefaulted(in, TIFFTAG_ORIENTATION, &orientation);
  if (! TIFFGetFieldDefaulted(in, TIFFTAG_PHOTOMETRIC, &input_photometric))
    TIFFError("loadImage","Image lacks Photometric interpreation tag");
  if (! TIFFGetField(in, TIFFTAG_IMAGEWIDTH,  &width))
    TIFFError("loadimage","Image lacks image width tag");
  if(! TIFFGetField(in, TIFFTAG_IMAGELENGTH, &length))
    TIFFError("loadimage","Image lacks image length tag");
  TIFFGetFieldDefaulted(in, TIFFTAG_XRESOLUTION, &xres);
  TIFFGetFieldDefaulted(in, TIFFTAG_YRESOLUTION, &yres);
  if (!TIFFGetFieldDefaulted(in, TIFFTAG_RESOLUTIONUNIT, &res_unit))
    res_unit = RESUNIT_INCH;
  if (!TIFFGetField(in, TIFFTAG_COMPRESSION, &input_compression))
    input_compression = COMPRESSION_NONE;

#ifdef DEBUG2
  char compressionid[16];

  switch (input_compression)
    {
    case COMPRESSION_NONE:	/* 1  dump mode */
	 strcpy (compressionid, "None/dump");
         break;         
    case COMPRESSION_CCITTRLE:	  /* 2 CCITT modified Huffman RLE */
	 strcpy (compressionid, "Huffman RLE");
         break;         
    case COMPRESSION_CCITTFAX3:	  /* 3 CCITT Group 3 fax encoding */
	 strcpy (compressionid, "Group3 Fax");
         break;         
    case COMPRESSION_CCITTFAX4:	  /* 4 CCITT Group 4 fax encoding */
	 strcpy (compressionid, "Group4 Fax");
         break;         
    case COMPRESSION_LZW:	  /* 5 Lempel-Ziv  & Welch */
	 strcpy (compressionid, "LZW");
         break;         
    case COMPRESSION_OJPEG:	  /* 6 !6.0 JPEG */
	 strcpy (compressionid, "Old Jpeg");
         break;         
    case COMPRESSION_JPEG:	  /* 7 %JPEG DCT compression */
	 strcpy (compressionid, "New Jpeg");
         break;         
    case COMPRESSION_NEXT:	  /* 32766 NeXT 2-bit RLE */
	 strcpy (compressionid, "Next RLE");
         break;         
    case COMPRESSION_CCITTRLEW:   /* 32771 #1 w/ word alignment */
	 strcpy (compressionid, "CITTRLEW");
         break;         
    case COMPRESSION_PACKBITS:	  /* 32773 Macintosh RLE */
	 strcpy (compressionid, "Mac Packbits");
         break;         
    case COMPRESSION_THUNDERSCAN: /* 32809 ThunderScan RLE */
	 strcpy (compressionid, "Thunderscan");
         break;         
    case COMPRESSION_IT8CTPAD:	  /* 32895 IT8 CT w/padding */
	 strcpy (compressionid, "IT8 padded");
         break;         
    case COMPRESSION_IT8LW:	  /* 32896 IT8 Linework RLE */
	 strcpy (compressionid, "IT8 RLE");
         break;         
    case COMPRESSION_IT8MP:	  /* 32897 IT8 Monochrome picture */
	 strcpy (compressionid, "IT8 mono");
         break;         
    case COMPRESSION_IT8BL:	  /* 32898 IT8 Binary line art */
	 strcpy (compressionid, "IT8 lineart");
         break;         
    case COMPRESSION_PIXARFILM:	  /* 32908 Pixar companded 10bit LZW */
	 strcpy (compressionid, "Pixar 10 bit");
         break;         
    case COMPRESSION_PIXARLOG:	  /* 32909 Pixar companded 11bit ZIP */
	 strcpy (compressionid, "Pixar 11bit");
         break;         
    case COMPRESSION_DEFLATE:	  /* 32946 Deflate compression */
	 strcpy (compressionid, "Deflate");
         break;         
    case COMPRESSION_ADOBE_DEFLATE: /* 8 Deflate compression */
	 strcpy (compressionid, "Adobe deflate");
         break;         
    default:
	 strcpy (compressionid, "None/unknown");
         break;         
    }
  TIFFError("loadImage", "Input compression %s", compressionid);
#endif

  scanlinesize = TIFFScanlineSize(in);
  image->bps = bps;
  image->spp = spp;
  image->planar = planar;
  image->width = width;
  image->length = length;
  image->xres = xres;
  image->yres = yres;
  image->res_unit = res_unit;
  image->compression = input_compression;
  image->photometric = input_photometric;
#ifdef DEBUG2
  char photometricid[12];

  switch (input_photometric)
    {
    case PHOTOMETRIC_MINISWHITE:
         strcpy (photometricid, "MinIsWhite");
         break;
    case PHOTOMETRIC_MINISBLACK:
         strcpy (photometricid, "MinIsBlack");
         break;
    case PHOTOMETRIC_RGB:
         strcpy (photometricid, "RGB");
         break;
    case PHOTOMETRIC_PALETTE:
         strcpy (photometricid, "Palette");
         break;
    case PHOTOMETRIC_MASK:
         strcpy (photometricid, "Mask");
         break;
    case PHOTOMETRIC_SEPARATED:
         strcpy (photometricid, "Separated");
         break;
    case PHOTOMETRIC_YCBCR:
         strcpy (photometricid, "YCBCR");
         break;
    case PHOTOMETRIC_CIELAB:
         strcpy (photometricid, "CIELab");
         break;
    case PHOTOMETRIC_ICCLAB:
         strcpy (photometricid, "ICCLab");
         break;
    case PHOTOMETRIC_ITULAB:
         strcpy (photometricid, "ITULab");
         break;
    case PHOTOMETRIC_LOGL:
         strcpy (photometricid, "LogL");
         break;
    case PHOTOMETRIC_LOGLUV:
         strcpy (photometricid, "LOGLuv");
         break;
    default:
         strcpy (photometricid, "Unknown");
         break;
    }
  TIFFError("loadImage", "Input photometric interpretation %s", photometricid);

#endif
  image->orientation = orientation;
  switch (orientation)
    {
    case 0:
    case ORIENTATION_TOPLEFT:
         image->adjustments = 0;
	 break;
    case ORIENTATION_TOPRIGHT:
         image->adjustments = MIRROR_HORIZ;
	 break;
    case ORIENTATION_BOTRIGHT:
         image->adjustments = ROTATECW_180;
	 break;
    case ORIENTATION_BOTLEFT:
         image->adjustments = MIRROR_VERT; 
	 break;
    case ORIENTATION_LEFTTOP:
         image->adjustments = MIRROR_VERT | ROTATECW_90;
	 break;
    case ORIENTATION_RIGHTTOP:
         image->adjustments = ROTATECW_90;
	 break;
    case ORIENTATION_RIGHTBOT:
         image->adjustments = MIRROR_VERT | ROTATECW_270;
	 break; 
    case ORIENTATION_LEFTBOT:
         image->adjustments = ROTATECW_270;
	 break;
    default:
         image->adjustments = 0;
         image->orientation = ORIENTATION_TOPLEFT;
   }

  if ((bps == 0) || (spp == 0))
    {
    TIFFError("loadImage", "Invalid samples per pixel (%d) or bits per sample (%d)",
	       spp, bps);
    return (-1);
    }

  if (TIFFIsTiled(in))
    {
    readunit = TILE;
    tlsize = TIFFTileSize(in);
    ntiles = TIFFNumberOfTiles(in);
    TIFFGetField(in, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(in, TIFFTAG_TILELENGTH, &tl);

    tile_rowsize  = TIFFTileRowSize(in);      
    buffsize = tlsize * ntiles;

        
    if (buffsize < (uint32)(ntiles * tl * tile_rowsize))
      {
      buffsize = ntiles * tl * tile_rowsize;
#ifdef DEBUG2
      TIFFError("loadImage",
	        "Tilesize %u is too small, using ntiles * tilelength * tilerowsize %lu",
                tlsize, (unsigned long)buffsize);
#endif
      }
    
    if (dump->infile != NULL)
      dump_info (dump->infile, dump->format, "", 
                 "Tilesize: %u, Number of Tiles: %u, Tile row size: %u",
                 tlsize, ntiles, tile_rowsize);
    }
  else
    {
    readunit = STRIP;
    TIFFGetFieldDefaulted(in, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    stsize = TIFFStripSize(in);
    nstrips = TIFFNumberOfStrips(in);
    buffsize = stsize * nstrips;
    
    if (buffsize < (uint32) (((length * width * spp * bps) + 7) / 8))
      {
      buffsize =  ((length * width * spp * bps) + 7) / 8;
#ifdef DEBUG2
      TIFFError("loadImage",
	        "Stripsize %u is too small, using imagelength * width * spp * bps / 8 = %lu",
                stsize, (unsigned long)buffsize);
#endif
      }
    
    if (dump->infile != NULL)
      dump_info (dump->infile, dump->format, "",
                 "Stripsize: %u, Number of Strips: %u, Rows per Strip: %u, Scanline size: %u",
		 stsize, nstrips, rowsperstrip, scanlinesize);
    }
  
  if (input_compression == COMPRESSION_JPEG)
    {  /* Force conversion to RGB */
    jpegcolormode = JPEGCOLORMODE_RGB;
    TIFFSetField(in, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
    }
  /* The clause up to the read statement is taken from Tom Lane's tiffcp patch */
  else 
    {   /* Otherwise, can't handle subsampled input */
    if (input_photometric == PHOTOMETRIC_YCBCR)
      {
      TIFFGetFieldDefaulted(in, TIFFTAG_YCBCRSUBSAMPLING,
 		           &subsampling_horiz, &subsampling_vert);
      if (subsampling_horiz != 1 || subsampling_vert != 1)
        {
	TIFFError("loadImage", 
		"Can't copy/convert subsampled image with subsampling %d horiz %d vert",
                subsampling_horiz, subsampling_vert);
        return (-1);
        }
	}
    }
 
  read_buff = *read_ptr;
  if (!read_buff)
    read_buff = (unsigned char *)_TIFFmalloc(buffsize);
  else
    {
    if (prev_readsize < buffsize)
      {
      new_buff = _TIFFrealloc(read_buff, buffsize);
      if (!new_buff)
        {
	free (read_buff);
        read_buff = (unsigned char *)_TIFFmalloc(buffsize);
        }
      else
        read_buff = new_buff;
      }
    }

  if (!read_buff)
    {
    TIFFError("loadImage", "Unable to allocate/reallocate read buffer");
    return (-1);
    }

  prev_readsize = buffsize;
  *read_ptr = read_buff;

  /* N.B. The read functions used copy separate plane data into a buffer as interleaved
   * samples rather than separate planes so the same logic works to extract regions
   * regardless of the way the data are organized in the input file.
   */
  switch (readunit) {
    case STRIP:
         if (planar == PLANARCONFIG_CONTIG)
           {
	     if (!(readContigStripsIntoBuffer(in, read_buff)))
	     {
	     TIFFError("loadImage", "Unable to read contiguous strips into buffer");
	     return (-1);
             }
           }
         else
           {
	   if (!(readSeparateStripsIntoBuffer(in, read_buff, length, width, spp, dump)))
	     {
	     TIFFError("loadImage", "Unable to read separate strips into buffer");
	     return (-1);
             }
           }
         break;

    case TILE:
         if (planar == PLANARCONFIG_CONTIG)
           {
	   if (!(readContigTilesIntoBuffer(in, read_buff, length, width, tw, tl, spp, bps)))
	     {
	     TIFFError("loadImage", "Unable to read contiguous tiles into buffer");
	     return (-1);
             }
           }
         else
           {
	   if (!(readSeparateTilesIntoBuffer(in, read_buff, length, width, tw, tl, spp, bps)))
	     {
	     TIFFError("loadImage", "Unable to read separate tiles into buffer");
	     return (-1);
             }
           }
         break;
    default: TIFFError("loadImage", "Unsupported image file format");
          return (-1);
          break;
    }
  if ((dump->infile != NULL) && (dump->level == 2))
    {
    dump_info  (dump->infile, dump->format, "loadImage", 
                "Image width %d, length %d, Raw image data, %4d bytes",
                width, length,  buffsize);
    dump_info  (dump->infile, dump->format, "", 
                "Bits per sample %d, Samples per pixel %d", bps, spp);

    for (i = 0; i < length; i++)
      dump_buffer(dump->infile, dump->format, 1, scanlinesize, 
                  i, read_buff + (i * scanlinesize));
    }
  return (0);
  }   /* end loadImage */

static int  correct_orientation(struct image_data *image, unsigned char **work_buff_ptr)
  {
  uint16 mirror, rotation;
  unsigned char *work_buff;

  work_buff = *work_buff_ptr;
  if ((image == NULL) || (work_buff == NULL))
    {
    TIFFError ("correct_orientatin", "Invalid image or buffer pointer");
    return (-1);
    }

  if ((image->adjustments & MIRROR_HORIZ) || (image->adjustments & MIRROR_VERT))
    {
    mirror = (uint16)(image->adjustments & MIRROR_BOTH);
    if (mirrorImage(image->spp, image->bps, mirror, 
        image->width, image->length, work_buff))
      {
      TIFFError ("correct_orientation", "Unable to mirror image");
      return (-1);
      }
    }

  if (image->adjustments & ROTATE_ANY)
    {
    if (image->adjustments & ROTATECW_90)
      rotation = (uint16) 90;
    else
    if (image->adjustments & ROTATECW_180)
      rotation = (uint16) 180;
    else
    if (image->adjustments & ROTATECW_270)
      rotation = (uint16) 270;
    else
      {
      TIFFError ("correct_orientation", "Invalid rotation value: %d", 
                  image->adjustments & ROTATE_ANY);
      return (-1);
      }
 
    if (rotateImage(rotation, image, &image->width, &image->length, work_buff_ptr))
      {
      TIFFError ("correct_orientation", "Unable to rotate image");
      return (-1);
      }
    image->orientation = ORIENTATION_TOPLEFT;
    }

  return (0);
  } /* end correct_orientation */


/* Extract multiple zones from an image and combine into a single composite image */
static int
extractCompositeRegions(struct image_data *image,  struct crop_mask *crop, 
                        unsigned char *read_buff, unsigned char *crop_buff)
  {
  int       shift_width, bytes_per_sample, bytes_per_pixel;
  uint32    i, trailing_bits, prev_trailing_bits;
  uint32    row, first_row, last_row, first_col, last_col;
  uint32    src_rowsize, dst_rowsize, src_offset, dst_offset;
  uint32    crop_width, crop_length, img_width, img_length;
  uint32    prev_length, prev_width, composite_width;
  uint16    bps, spp;
  uint8    *src, *dst;
  tsample_t count, sample = 0;   /* Update to extract one or more samples */

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;
  count = spp;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if ((bps % 8) == 0)
    shift_width = 0;
  else
    {
    if (bytes_per_pixel < (bytes_per_sample + 1))
      shift_width = bytes_per_pixel;
    else
      shift_width = bytes_per_sample + 1;
    }
  src = read_buff;
  dst = crop_buff;

  /* These are setup for adding additional sections */
  prev_width = prev_length = 0;
  prev_trailing_bits = trailing_bits = 0;
  composite_width = crop->combined_width;
  crop->combined_width = 0;
  crop->combined_length = 0;

  for (i = 0; i < crop->selections; i++)
    {
    /* rows, columns, width, length are expressed in pixels */
    first_row = crop->regionlist[i].y1;
    last_row  = crop->regionlist[i].y2;
    first_col = crop->regionlist[i].x1;
    last_col  = crop->regionlist[i].x2;

    crop_width = last_col - first_col + 1;
    crop_length = last_row - first_row + 1;

    /* These should not be needed for composite images */
    crop->regionlist[i].width = crop_width;
    crop->regionlist[i].length = crop_length;
    crop->regionlist[i].buffptr = crop_buff;

    src_rowsize = ((img_width * bps * spp) + 7) / 8;
    dst_rowsize = (((crop_width * bps * count) + 7) / 8);

    switch (crop->edge_ref)
      {
      default:
      case EDGE_TOP:
      case EDGE_BOTTOM:
	   if ((i > 0) && (crop_width != crop->regionlist[i - 1].width))
             {
	     TIFFError ("extractCompositeRegions", 
                          "Only equal width regions can be combined for -E top or bottom");
	     return (1);
             }

           crop->combined_width = crop_width;
           crop->combined_length += crop_length;

           for (row = first_row; row <= last_row; row++)
             {
	     src_offset = row * src_rowsize;
	     dst_offset = (row - first_row) * dst_rowsize;
             src = read_buff + src_offset;
             dst = crop_buff + dst_offset + (prev_length * dst_rowsize);
             switch (shift_width)
               {
               case 0: if (extractContigSamplesBytes (src, dst, img_width, sample,
                                                      spp, bps, count, first_col,
                                                      last_col + 1))
                         {
		         TIFFError("extractCompositeRegions",
                                   "Unable to extract row %d", row);
		         return (1);
		         }
		       break;
               case 1: if (bps == 1)
                         { 
                         if (extractContigSamplesShifted8bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                           {
		           TIFFError("extractCompositeRegions",
                                     "Unable to extract row %d", row);
		           return (1);
		           }
		         break;
			 }
                       else
                         if (extractContigSamplesShifted16bits (src, dst, img_width,
                                                                sample, spp, bps, count, 
                                                                first_col, last_col + 1,
                                                                prev_trailing_bits))
                           {
		           TIFFError("extractCompositeRegions",
                                     "Unable to extract row %d", row);
		           return (1);
		           }
		        break;
               case 2:  if (extractContigSamplesShifted24bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                          {
		          TIFFError("extractCompositeRegions",
                                    "Unable to extract row %d", row);
		          return (1);
		          }
		        break;
               case 3:
               case 4:
               case 5:  if (extractContigSamplesShifted32bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                          {
		          TIFFError("extractCompositeRegions",
                                    "Unable to extract row %d", row);
		          return (1);
		          }
		        break;
               default: TIFFError("extractCompositeRegions", "Unsupported bit depth %d", bps);
		        return (1);
	       }
             }
           prev_length += crop_length;
	   break;
      case EDGE_LEFT:  /* splice the pieces of each row together, side by side */
      case EDGE_RIGHT:
	   if ((i > 0) && (crop_length != crop->regionlist[i - 1].length))
             {
	     TIFFError ("extractCompositeRegions", 
                          "Only equal length regions can be combined for -E left or right");
	     return (1);
             }
           crop->combined_width += crop_width;
           crop->combined_length = crop_length;
           dst_rowsize = (((composite_width * bps * count) + 7) / 8);
           trailing_bits = (crop_width * bps * count) % 8;
           for (row = first_row; row <= last_row; row++)
             {
	     src_offset = row * src_rowsize;
	     dst_offset = (row - first_row) * dst_rowsize;
             src = read_buff + src_offset;
             dst = crop_buff + dst_offset + prev_width;

             switch (shift_width)
               {
               case 0: if (extractContigSamplesBytes (src, dst, img_width,
                                                      sample, spp, bps, count,
                                                      first_col, last_col + 1))
                         {
		         TIFFError("extractCompositeRegions",
                                   "Unable to extract row %d", row);
		         return (1);
		         }
		       break;
               case 1: if (bps == 1)
                         { 
                         if (extractContigSamplesShifted8bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                           {
		           TIFFError("extractCompositeRegions",
                                     "Unable to extract row %d", row);
		           return (1);
		           }
		         break;
			 }
                       else
                         if (extractContigSamplesShifted16bits (src, dst, img_width,
                                                                sample, spp, bps, count, 
                                                                first_col, last_col + 1,
                                                                prev_trailing_bits))
                           {
		           TIFFError("extractCompositeRegions",
                                     "Unable to extract row %d", row);
		           return (1);
		           }
		        break;
              case 2:  if (extractContigSamplesShifted24bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                          {
		          TIFFError("extractCompositeRegions",
                                    "Unable to extract row %d", row);
		          return (1);
		          }
		        break;
               case 3:
               case 4:
               case 5:  if (extractContigSamplesShifted32bits (src, dst, img_width,
                                                               sample, spp, bps, count, 
                                                               first_col, last_col + 1,
                                                               prev_trailing_bits))
                          {
		          TIFFError("extractCompositeRegions",
                                    "Unable to extract row %d", row);
		          return (1);
		          }
		        break;
               default: TIFFError("extractCompositeRegions", "Unsupported bit depth %d", bps);
		        return (1);
	       }
	     }
	   prev_width += (crop_width * bps * count) / 8;
           prev_trailing_bits += trailing_bits;
           if (prev_trailing_bits > 7)
	     prev_trailing_bits-= 8;
	   break;
      }
    }
  if (crop->combined_width != composite_width)
    TIFFError("combineSeparateRegions","Combined width does not match composite width");
      
  return (0);
  }  /* end extractCompositeRegions */

/* Copy a single region of input buffer to an output buffer. 
 * The read functions used copy separate plane data into a buffer 
 * as interleaved samples rather than separate planes so the same
 * logic works to extract regions regardless of the way the data 
 * are organized in the input file. This function can be used to
 * extract one or more samples from the input image by updating the 
 * parameters for starting sample and number of samples to copy in the
 * fifth and eighth arguments of the call to extractContigSamples.
 * They would be passed as new elements of the crop_mask struct.
 */

static int
extractSeparateRegion(struct image_data *image,  struct crop_mask *crop,
                      unsigned char *read_buff, unsigned char *crop_buff,
                      int region)
  {
  int     shift_width, prev_trailing_bits = 0;
  uint32  bytes_per_sample, bytes_per_pixel;
  uint32  src_rowsize, dst_rowsize;
  uint32  row, first_row, last_row, first_col, last_col;
  uint32  src_offset, dst_offset;
  uint32  crop_width, crop_length, img_width, img_length;
  uint16  bps, spp;
  uint8  *src, *dst;
  tsample_t count, sample = 0;   /* Update to extract more or more samples */

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;
  count = spp;

  bytes_per_sample = (bps + 7) / 8; 
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if ((bps % 8) == 0)
    shift_width = 0; /* Byte aligned data only */
  else
    {
    if (bytes_per_pixel < (bytes_per_sample + 1))
      shift_width = bytes_per_pixel;
    else
      shift_width = bytes_per_sample + 1;
    }

  /* rows, columns, width, length are expressed in pixels */
  first_row = crop->regionlist[region].y1;
  last_row  = crop->regionlist[region].y2;
  first_col = crop->regionlist[region].x1;
  last_col  = crop->regionlist[region].x2;

  crop_width = last_col - first_col + 1;
  crop_length = last_row - first_row + 1;

  crop->regionlist[region].width = crop_width;
  crop->regionlist[region].length = crop_length;
  crop->regionlist[region].buffptr = crop_buff;

  src = read_buff;
  dst = crop_buff;
  src_rowsize = ((img_width * bps * spp) + 7) / 8;
  dst_rowsize = (((crop_width * bps * spp) + 7) / 8);

  for (row = first_row; row <= last_row; row++)
    {
    src_offset = row * src_rowsize;
    dst_offset = (row  - first_row) * dst_rowsize;
    src = read_buff + src_offset;
    dst = crop_buff + dst_offset;

    switch (shift_width)
      {
      case 0: if (extractContigSamplesBytes (src, dst, img_width, sample,
                                             spp, bps, count, first_col,
                                             last_col + 1))
                {
	        TIFFError("extractSeparateRegion",
                          "Unable to extract row %d", row);
	        return (1);
	        }
	      break;
      case 1: if (bps == 1)
                { 
                if (extractContigSamplesShifted8bits (src, dst, img_width,
                                                      sample, spp, bps, count, 
                                                      first_col, last_col + 1,
                                                      prev_trailing_bits))
                  {
		  TIFFError("extractSeparateRegion",
                            "Unable to extract row %d", row);
		  return (1);
		  }
		  break;
		}
              else
                if (extractContigSamplesShifted16bits (src, dst, img_width,
                                                       sample, spp, bps, count, 
                                                       first_col, last_col + 1,
                                                       prev_trailing_bits))
                  {
		  TIFFError("extractSeparateRegion",
                            "Unable to extract row %d", row);
		  return (1);
		  }
	      break;
      case 2:  if (extractContigSamplesShifted24bits (src, dst, img_width,
                                                     sample, spp, bps, count, 
                                                     first_col, last_col + 1,
                                                     prev_trailing_bits))
                {
		TIFFError("extractSeparateRegion",
                          "Unable to extract row %d", row);
		return (1);
		}
	      break;
      case 3:
      case 4:
      case 5:  if (extractContigSamplesShifted32bits (src, dst, img_width,
                                                     sample, spp, bps, count, 
                                                     first_col, last_col + 1,
                                                     prev_trailing_bits))
                {
		TIFFError("extractSeparateRegion",
                          "Unable to extract row %d", row);
		return (1);
		}
	      break;
      default: TIFFError("extractSeparateRegion", "Unsupported bit depth %d", bps);
	       return (1);
      }
    }
          
  return (0);
  }  /* end extractSeparateRegion */

static int
extractImageSection(struct image_data *image, struct pageseg *section, 
                    unsigned char *src_buff, unsigned char *sect_buff)
  {
  unsigned  char  bytebuff1, bytebuff2;
  unsigned  char *src, *dst;

  uint32    img_width, img_length, img_rowsize;
  uint32    j, shift1, shift2, trailing_bits;
  uint32    row, first_row, last_row, first_col, last_col;
  uint32    src_offset, dst_offset, row_offset, col_offset;
  uint32    offset1, offset2, full_bytes;
  uint32    sect_width, sect_length;
  uint16    bps, spp;

#ifdef DEVELMODE
  int      k;
  unsigned char bitset;
  static char *bitarray = NULL;
#endif

  img_width = image->width;
  img_length = image->length;
  bps = image->bps;
  spp = image->spp;

  src = src_buff;
  dst = sect_buff;
  src_offset = 0;
  dst_offset = 0;

#ifdef DEVELMODE
  if (bitarray == NULL)
    {
    if ((bitarray = (char *)malloc(img_width)) == NULL)
      {
      TIFFError ("", "DEBUG: Unable to allocate debugging bitarray");
      return (-1);
      }
    }
#endif

  /* rows, columns, width, length are expressed in pixels */
  first_row = section->y1;
  last_row  = section->y2;
  first_col = section->x1;
  last_col  = section->x2;

  sect_width = last_col - first_col + 1;
  sect_length = last_row - first_row + 1;
  img_rowsize = ((img_width * bps + 7) / 8) * spp;
  full_bytes = (sect_width * spp * bps) / 8;   /* number of COMPLETE bytes per row in section */
  trailing_bits = (sect_width * bps) % 8;

#ifdef DEVELMODE
    TIFFError ("", "First row: %d, last row: %d, First col: %d, last col: %d\n",
           first_row, last_row, first_col, last_col);
    TIFFError ("", "Image width: %d, Image length: %d, bps: %d, spp: %d\n",
	   img_width, img_length, bps, spp);
    TIFFError ("", "Sect  width: %d,  Sect length: %d, full bytes: %d trailing bits %d\n", 
           sect_width, sect_length, full_bytes, trailing_bits);
#endif

  if ((bps % 8) == 0)
    {
    col_offset = first_col * spp * bps / 8;
    for (row = first_row; row <= last_row; row++)
      {
      /* row_offset = row * img_width * spp * bps / 8; */
      row_offset = row * img_rowsize;
      src_offset = row_offset + col_offset;

#ifdef DEVELMODE
        TIFFError ("", "Src offset: %8d, Dst offset: %8d", src_offset, dst_offset); 
#endif
      _TIFFmemcpy (sect_buff + dst_offset, src_buff + src_offset, full_bytes);
      dst_offset += full_bytes;
      }        
    }
  else
    { /* bps != 8 */
    shift1  = spp * ((first_col * bps) % 8);
    shift2  = spp * ((last_col * bps) % 8);
    for (row = first_row; row <= last_row; row++)
      {
      /* pull out the first byte */
      row_offset = row * img_rowsize;
      offset1 = row_offset + (first_col * bps / 8);
      offset2 = row_offset + (last_col * bps / 8);

#ifdef DEVELMODE
      for (j = 0, k = 7; j < 8; j++, k--)
        {
        bitset = *(src_buff + offset1) & (((unsigned char)1 << k)) ? 1 : 0;
        sprintf(&bitarray[j], (bitset) ? "1" : "0");
        }
      sprintf(&bitarray[8], " ");
      sprintf(&bitarray[9], " ");
      for (j = 10, k = 7; j < 18; j++, k--)
        {
        bitset = *(src_buff + offset2) & (((unsigned char)1 << k)) ? 1 : 0;
        sprintf(&bitarray[j], (bitset) ? "1" : "0");
        }
      bitarray[18] = '\0';
      TIFFError ("", "Row: %3d Offset1: %d,  Shift1: %d,    Offset2: %d,  Shift2:  %d\n", 
                 row, offset1, shift1, offset2, shift2); 
#endif

      bytebuff1 = bytebuff2 = 0;
      if (shift1 == 0) /* the region is byte and sample alligned */
        {
	_TIFFmemcpy (sect_buff + dst_offset, src_buff + offset1, full_bytes);

#ifdef DEVELMODE
	TIFFError ("", "        Alligned data src offset1: %8d, Dst offset: %8d\n", offset1, dst_offset); 
	sprintf(&bitarray[18], "\n");
	sprintf(&bitarray[19], "\t");
        for (j = 20, k = 7; j < 28; j++, k--)
          {
          bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
          sprintf(&bitarray[j], (bitset) ? "1" : "0");
          }
        bitarray[28] = ' ';
        bitarray[29] = ' ';
#endif
        dst_offset += full_bytes;

        if (trailing_bits != 0)
          {
	  bytebuff2 = src_buff[offset2] & ((unsigned char)255 << (7 - shift2));
          sect_buff[dst_offset] = bytebuff2;
#ifdef DEVELMODE
	  TIFFError ("", "        Trailing bits src offset:  %8d, Dst offset: %8d\n", 
                              offset2, dst_offset); 
          for (j = 30, k = 7; j < 38; j++, k--)
            {
            bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
            sprintf(&bitarray[j], (bitset) ? "1" : "0");
            }
          bitarray[38] = '\0';
          TIFFError ("", "\tFirst and last bytes before and after masking:\n\t%s\n\n", bitarray);
#endif
          dst_offset++;
          }
        }
      else   /* each destination byte will have to be built from two source bytes*/
        {
#ifdef DEVELMODE
	  TIFFError ("", "        Unalligned data src offset: %8d, Dst offset: %8d\n", offset1 , dst_offset); 
#endif
        for (j = 0; j <= full_bytes; j++) 
          {
	  bytebuff1 = src_buff[offset1 + j] & ((unsigned char)255 >> shift1);
	  bytebuff2 = src_buff[offset1 + j + 1] & ((unsigned char)255 << (7 - shift1));
          sect_buff[dst_offset + j] = (bytebuff1 << shift1) | (bytebuff2 >> (8 - shift1));
          }
#ifdef DEVELMODE
	sprintf(&bitarray[18], "\n");
	sprintf(&bitarray[19], "\t");
        for (j = 20, k = 7; j < 28; j++, k--)
          {
          bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
          sprintf(&bitarray[j], (bitset) ? "1" : "0");
          }
        bitarray[28] = ' ';
        bitarray[29] = ' ';
#endif
        dst_offset += full_bytes;

        if (trailing_bits != 0)
          {
#ifdef DEVELMODE
	    TIFFError ("", "        Trailing bits   src offset: %8d, Dst offset: %8d\n", offset1 + full_bytes, dst_offset); 
#endif
	  if (shift2 > shift1)
            {
	    bytebuff1 = src_buff[offset1 + full_bytes] & ((unsigned char)255 << (7 - shift2));
            bytebuff2 = bytebuff1 & ((unsigned char)255 << shift1);
            sect_buff[dst_offset] = bytebuff2;
#ifdef DEVELMODE
	    TIFFError ("", "        Shift2 > Shift1\n"); 
#endif
            }
          else
            {
	    if (shift2 < shift1)
              {
              bytebuff2 = ((unsigned char)255 << (shift1 - shift2 - 1));
	      sect_buff[dst_offset] &= bytebuff2;
#ifdef DEVELMODE
	      TIFFError ("", "        Shift2 < Shift1\n"); 
#endif
              }
#ifdef DEVELMODE
            else
	      TIFFError ("", "        Shift2 == Shift1\n"); 
#endif
            }
	  }
#ifdef DEVELMODE
	  sprintf(&bitarray[28], " ");
	  sprintf(&bitarray[29], " ");
          for (j = 30, k = 7; j < 38; j++, k--)
            {
            bitset = *(sect_buff + dst_offset) & (((unsigned char)1 << k)) ? 1 : 0;
            sprintf(&bitarray[j], (bitset) ? "1" : "0");
            }
          bitarray[38] = '\0';
          TIFFError ("", "\tFirst and last bytes before and after masking:\n\t%s\n\n", bitarray);
#endif
        dst_offset++;
        }
      }
    }

  return (0);
  } /* end extractImageSection */

static int 
writeSelections(TIFF *in, TIFF **out, struct crop_mask *crop, 
                struct image_data *image, struct dump_opts *dump,
                struct buffinfo seg_buffs[], char *mp, char *filename, 
                unsigned int *page, unsigned int total_pages)
  {
  int i, page_count;
  int autoindex = 0;
  unsigned char *crop_buff = NULL;

  /* Where we open a new file depends on the export mode */  
  switch (crop->exp_mode)
    {
    case ONE_FILE_COMPOSITE: /* Regions combined into single image */
         autoindex = 0;
         crop_buff = seg_buffs[0].buffer;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
         page_count = total_pages;
         if (writeCroppedImage(in, *out, image, dump,
                               crop->combined_width, 
                               crop->combined_length,
                               crop_buff, *page, total_pages))
            {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
	 break;
    case ONE_FILE_SEPARATED: /* Regions as separated images */
         autoindex = 0;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
         page_count = crop->selections * total_pages;
         for (i = 0; i < crop->selections; i++)
           {
           crop_buff = seg_buffs[i].buffer;
           if (writeCroppedImage(in, *out, image, dump,
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
	   }
         break;
    case FILE_PER_IMAGE_COMPOSITE: /* Regions as composite image */
         autoindex = 1;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);

         crop_buff = seg_buffs[0].buffer;
         if (writeCroppedImage(in, *out, image, dump,
                               crop->combined_width, 
                               crop->combined_length, 
                               crop_buff, *page, total_pages))
           {
           TIFFError("writeRegions", "Unable to write new image");
           return (-1);
           }
         break;
    case FILE_PER_IMAGE_SEPARATED: /* Regions as separated images */
         autoindex = 1;
         page_count = crop->selections;
         if (update_output_file (out, mp, autoindex, filename, page))
           return (1);
                
         for (i = 0; i < crop->selections; i++)
           {
           crop_buff = seg_buffs[i].buffer;
           /* Write the current region to the current file */
           if (writeCroppedImage(in, *out, image, dump,
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
           }
         break;
    case FILE_PER_SELECTION:
         autoindex = 1;
	 page_count = 1;
         for (i = 0; i < crop->selections; i++)
           {
           if (update_output_file (out, mp, autoindex, filename, page))
             return (1);

           crop_buff = seg_buffs[i].buffer;
           /* Write the current region to the current file */
           if (writeCroppedImage(in, *out, image, dump,
                                 crop->regionlist[i].width, 
                                 crop->regionlist[i].length, 
                                 crop_buff, *page, page_count))
             {
             TIFFError("writeRegions", "Unable to write new image");
             return (-1);
             }
           }
	 break;
    default: return (1);
    }

  return (0);
  } /* end writeRegions */

static int
writeImageSections(TIFF *in, TIFF *out, struct image_data *image,
		   struct pagedef *page, struct pageseg *sections,
		   struct dump_opts * dump, unsigned char *src_buff,
                   unsigned char **sect_buff_ptr)
  {
  double  hres, vres;
  uint32  i, k, width, length, sectsize;
  unsigned char *sect_buff = *sect_buff_ptr;

  hres = page->hres;
  vres = page->vres;

  k = page->cols * page->rows;
  if ((k < 1) || (k > MAX_SECTIONS))
   {
   TIFFError("writeImageSections",
	     "%d Rows and Columns exceed maximum sections\nIncrease resolution or reduce sections", k);
   return (-1);
   }

  for (i = 0; i < k; i++)
    {
    width  = sections[i].x2 - sections[i].x1 + 1;
    length = sections[i].y2 - sections[i].y1 + 1;
    sectsize = (uint32)
	    ceil((width * image->bps + 7) / (double)8) * image->spp * length;
    /* allocate a buffer if we don't have one already */
    if (createImageSection(sectsize, sect_buff_ptr))
      {
      TIFFError("writeImageSections", "Unable to allocate section buffer");
      exit (-1);
      }
    sect_buff = *sect_buff_ptr;

    if (extractImageSection (image, &sections[i], src_buff, sect_buff))
      {
      TIFFError("writeImageSections", "Unable to extract image sections");
      exit (-1);
      }

  /* call the write routine here instead of outside the loop */
    if (writeSingleSection(in, out, image, dump, width, length, hres, vres, sect_buff))
      {
      TIFFError("writeImageSections", "Unable to write image section");
      exit (-1);
      }
    }

  return (0);
  } /* end writeImageSections */

/* Code in this function is heavily indebted to code in tiffcp
 * with modifications by Richard Nolde to handle orientation correctly.
 * It will have to be updated significantly if support is added to
 * extract one or more samples from original image since the 
 * original code assumes we are always copying all samples.
 */
static int  
writeSingleSection(TIFF *in, TIFF *out, struct image_data *image,
                   struct dump_opts *dump, uint32 width, uint32 length,
                   double hres, double vres,
                   unsigned char *sect_buff)
  {
  uint16 bps, spp;
  uint16 input_compression, input_photometric;
  uint16 input_planar;
  struct cpTag* p;

  /*  Calling this seems to reset the compression mode on the TIFF *in file.
  TIFFGetField(in, TIFFTAG_JPEGCOLORMODE, &input_jpeg_colormode);
  */
  input_compression = image->compression;
  input_photometric = image->photometric;

  spp = image->spp;
  bps = image->bps;
  TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
  TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bps);
  TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);

#ifdef DEBUG2
  TIFFError("writeSingleSection", "Input compression: %s",
	    (input_compression == COMPRESSION_OJPEG) ? "Old Jpeg" :
	    ((input_compression == COMPRESSION_JPEG) ?  "New Jpeg" : "Non Jpeg"));
#endif
  /* This is the global variable compression which is set 
   * if the user has specified a command line option for 
   * a compression option.  Should be passed around in one
   * of the parameters instead of as a global. If no user
   * option specified it will still be (uint16) -1. */
  if (compression != (uint16)-1)
    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
  else
    { /* OJPEG is no longer supported for writing so upgrade to JPEG */
    if (input_compression == COMPRESSION_OJPEG)
      {
      compression = COMPRESSION_JPEG;
      jpegcolormode = JPEGCOLORMODE_RAW;
      TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
      }
    else /* Use the compression from the input file */
      TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
    }

  if (compression == COMPRESSION_JPEG)
    {
    if ((input_photometric == PHOTOMETRIC_PALETTE) ||  /* color map indexed */
        (input_photometric == PHOTOMETRIC_MASK))       /* holdout mask */
      {
      TIFFError ("writeSingleSection",
                 "JPEG compression cannot be used with %s image data",
		 (input_photometric == PHOTOMETRIC_PALETTE) ?
                 "palette" : "mask");
      return (-1);
      }
    if ((input_photometric == PHOTOMETRIC_RGB) &&
	(jpegcolormode == JPEGCOLORMODE_RGB))
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
    else
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, input_photometric);
    }
  else
    {
    if (compression == COMPRESSION_SGILOG || compression == COMPRESSION_SGILOG24)
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, spp == 1 ?
			PHOTOMETRIC_LOGL : PHOTOMETRIC_LOGLUV);
    else
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, image->photometric);
    }

#ifdef DEBUG2
  TIFFError("writeSingleSection", "Input photometric: %s",
	    (input_photometric == PHOTOMETRIC_RGB) ? "RGB" :
	    ((input_photometric == PHOTOMETRIC_YCBCR) ?  "YCbCr" : "Not RGB or YCbCr"));
#endif

  if (((input_photometric == PHOTOMETRIC_LOGL) ||
       (input_photometric ==  PHOTOMETRIC_LOGLUV)) &&
      ((compression != COMPRESSION_SGILOG) && 
       (compression != COMPRESSION_SGILOG24)))
    {
    TIFFError("writeSingleSection",
              "LogL and LogLuv source data require SGI_LOG or SGI_LOG24 compression");
    return (-1);
    }

  if (fillorder != 0)
    TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
  else
    CopyTag(TIFFTAG_FILLORDER, 1, TIFF_SHORT);

  /* The loadimage function reads input orientation and sets
   * image->orientation. The correct_image_orientation function
   * applies the required rotation and mirror operations to 
   * present the data in TOPLEFT orientation and updates 
   * image->orientation if any transforms are performed, 
   * as per EXIF standard.
   */
  TIFFSetField(out, TIFFTAG_ORIENTATION, image->orientation);

  /*
   * Choose tiles/strip for the output image according to
   * the command line arguments (-tiles, -strips) and the
   * structure of the input image.
   */
  if (outtiled == -1)
    outtiled = TIFFIsTiled(in);
  if (outtiled) {
    /*
     * Setup output file's tile width&height.  If either
     * is not specified, use either the value from the
     * input image or, if nothing is defined, use the
     * library default.
     */
    if (tilewidth == (uint32) 0)
      TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth);
    if (tilelength == (uint32) 0)
      TIFFGetField(in, TIFFTAG_TILELENGTH, &tilelength);

    if (tilewidth == 0 || tilelength == 0)
      TIFFDefaultTileSize(out, &tilewidth, &tilelength);
    TIFFDefaultTileSize(out, &tilewidth, &tilelength);
    TIFFSetField(out, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(out, TIFFTAG_TILELENGTH, tilelength);
    } else {
       /*
	* RowsPerStrip is left unspecified: use either the
	* value from the input image or, if nothing is defined,
	* use the library default.
	*/
	if (rowsperstrip == (uint32) 0)
          {
	  if (!TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
	    rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
          if (compression != COMPRESSION_JPEG)
            {
  	    if (rowsperstrip > length)
	      rowsperstrip = length;
	    }
	  }
	else 
          if (rowsperstrip == (uint32) -1)
	    rowsperstrip = length;
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}

  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &input_planar);
  if (config != (uint16) -1)
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
  else
    CopyField(TIFFTAG_PLANARCONFIG, config);
  if (spp <= 4)
    CopyTag(TIFFTAG_TRANSFERFUNCTION, 4, TIFF_SHORT);
  CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);

/* SMinSampleValue & SMaxSampleValue */
  switch (compression) {
    /* These are references to GLOBAL variables set by defaults
     * and /or the compression flag
     */
    case COMPRESSION_JPEG:
         if (((bps % 8) == 0) || ((bps % 12) == 0))
	   {
           TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
	   TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
           }
         else
           {
	   TIFFError("writeSingleSection",
                     "JPEG compression requires 8 or 12 bits per sample");
           return (-1);
           }
	 break;
   case COMPRESSION_LZW:
   case COMPRESSION_ADOBE_DEFLATE:
   case COMPRESSION_DEFLATE:
	if (predictor != (uint16)-1)
          TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
	else
	  CopyField(TIFFTAG_PREDICTOR, predictor);
	break;
   case COMPRESSION_CCITTFAX3:
   case COMPRESSION_CCITTFAX4:
	if (compression == COMPRESSION_CCITTFAX3) {
          if (g3opts != (uint32) -1)
	    TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, g3opts);
	  else
	    CopyField(TIFFTAG_GROUP3OPTIONS, g3opts);
	} else
	    CopyTag(TIFFTAG_GROUP4OPTIONS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_BADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CLEANFAXDATA, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CONSECUTIVEBADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVPARAMS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVTIME, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXSUBADDRESS, 1, TIFF_ASCII);
	break;
   }
   { uint32 len32;
     void** data;
     if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &len32, &data))
       TIFFSetField(out, TIFFTAG_ICCPROFILE, len32, data);
   }
   { uint16 ninks;
     const char* inknames;
     if (TIFFGetField(in, TIFFTAG_NUMBEROFINKS, &ninks)) {
       TIFFSetField(out, TIFFTAG_NUMBEROFINKS, ninks);
       if (TIFFGetField(in, TIFFTAG_INKNAMES, &inknames)) {
	 int inknameslen = strlen(inknames) + 1;
	 const char* cp = inknames;
	 while (ninks > 1) {
	   cp = strchr(cp, '\0');
	   if (cp) {
	     cp++;
	     inknameslen += (strlen(cp) + 1);
	   }
	   ninks--;
         }
	 TIFFSetField(out, TIFFTAG_INKNAMES, inknameslen, inknames);
       }
     }
   }
   {
   unsigned short pg0, pg1;
   if (TIFFGetField(in, TIFFTAG_PAGENUMBER, &pg0, &pg1)) {
     if (pageNum < 0) /* only one input file */
	TIFFSetField(out, TIFFTAG_PAGENUMBER, pg0, pg1);
     else 
	TIFFSetField(out, TIFFTAG_PAGENUMBER, pageNum++, 0);
     }
   }

  for (p = tags; p < &tags[NTAGS]; p++)
		CopyTag(p->tag, p->count, p->type);

  /* Update these since they are overwritten from input res by loop above */
  TIFFSetField(out, TIFFTAG_XRESOLUTION, (float)hres);
  TIFFSetField(out, TIFFTAG_YRESOLUTION, (float)vres);

  /* Compute the tile or strip dimensions and write to disk */
  if (outtiled)
    {
    if (config == PLANARCONFIG_CONTIG)
      writeBufferToContigTiles (out, sect_buff, length, width, spp, dump);
    else
      writeBufferToSeparateTiles (out, sect_buff, length, width, spp, dump);
    }
  else
    {
    if (config == PLANARCONFIG_CONTIG)
      writeBufferToContigStrips (out, sect_buff, length);
    else
      writeBufferToSeparateStrips(out, sect_buff, length, width, spp, dump);
    }

  if (!TIFFWriteDirectory(out))
    {
    TIFFClose(out);
    return (-1);
    }

  return (0);
  } /* end writeSingleSection */


/* Create a buffer to write one section at a time */
static int
createImageSection(uint32 sectsize, unsigned char **sect_buff_ptr)
  {
  unsigned  char *sect_buff = NULL;
  unsigned  char *new_buff  = NULL;
  static    uint32  prev_sectsize = 0;
  
  sect_buff = *sect_buff_ptr;

  if (!sect_buff)
    {
    sect_buff = (unsigned char *)_TIFFmalloc(sectsize);
    *sect_buff_ptr = sect_buff;
    _TIFFmemset(sect_buff, 0, sectsize);
    }
  else
    {
    if (prev_sectsize < sectsize)
      {
      new_buff = _TIFFrealloc(sect_buff, sectsize);
      if (!new_buff)
        {
	free (sect_buff);
        sect_buff = (unsigned char *)_TIFFmalloc(sectsize);
        }
      else
        sect_buff = new_buff;

      _TIFFmemset(sect_buff, 0, sectsize);
      }
    }

  if (!sect_buff)
    {
    TIFFError("createImageSection", "Unable to allocate/reallocate section buffer");
    return (-1);
    }
  prev_sectsize = sectsize;
  *sect_buff_ptr = sect_buff;

  return (0);
  }  /* end createImageSection */


/* Process selections defined by regions, zones, margins, or fixed sized areas */
static int
processCropSelections(struct image_data *image, struct crop_mask *crop, 
                      unsigned char **read_buff_ptr, struct buffinfo seg_buffs[])
  {
  int       i;
  uint32    width, length, total_width, total_length;
  tsize_t   cropsize;
  unsigned  char *crop_buff = NULL;
  unsigned  char *read_buff = NULL;
  unsigned  char *next_buff = NULL;
  tsize_t   prev_cropsize = 0;

  read_buff = *read_buff_ptr;

  if (crop->img_mode == COMPOSITE_IMAGES)
    {
    cropsize = crop->bufftotal;
    crop_buff = seg_buffs[0].buffer; 
    if (!crop_buff)
      crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
    else
      {
      prev_cropsize = seg_buffs[0].size;
      if (prev_cropsize < cropsize)
        {
        next_buff = _TIFFrealloc(crop_buff, cropsize);
        if (! next_buff)
          {
          _TIFFfree (crop_buff);
          crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
          }
        else
          crop_buff = next_buff;
        }
      }

    if (!crop_buff)
      {
      TIFFError("processCropSelections", "Unable to allocate/reallocate crop buffer");
      return (-1);
      }
 
    _TIFFmemset(crop_buff, 0, cropsize);
    seg_buffs[0].buffer = crop_buff;
    seg_buffs[0].size = cropsize;

    /* Checks for matching width or length as required */
    if (extractCompositeRegions(image, crop, read_buff, crop_buff) != 0)
      return (1);

    if (crop->crop_mode & CROP_INVERT)
      {
      switch (crop->photometric)
        {
        /* Just change the interpretation */
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
	     image->photometric = crop->photometric;
	     break;
        case INVERT_DATA_ONLY:
        case INVERT_DATA_AND_TAG:
             if (invertImage(image->photometric, image->spp, image->bps, 
                             crop->combined_width, crop->combined_length, crop_buff))
               {
               TIFFError("processCropSelections", 
                         "Failed to invert colorspace for composite regions");
               return (-1);
               }
             if (crop->photometric == INVERT_DATA_AND_TAG)
               {
               switch (image->photometric)
                 {
                 case PHOTOMETRIC_MINISWHITE:
 	              image->photometric = PHOTOMETRIC_MINISBLACK;
	              break;
                 case PHOTOMETRIC_MINISBLACK:
 	              image->photometric = PHOTOMETRIC_MINISWHITE;
	              break;
                 default:
	              break;
	         }
	       }
             break;
        default: break;
        }
      }

    /* Mirror and Rotate will not work with multiple regions unless they are the same width */
    if (crop->crop_mode & CROP_MIRROR)
      {
      if (mirrorImage(image->spp, image->bps, crop->mirror, 
                      crop->combined_width, crop->combined_length, crop_buff))
        {
        TIFFError("processCropSelections", "Failed to mirror composite regions %s", 
	         (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
        return (-1);
        }
      }

    if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
      {
      if (rotateImage(crop->rotation, image, &crop->combined_width, 
                      &crop->combined_length, &crop_buff))
        {
        TIFFError("processCropSelections", 
                  "Failed to rotate composite regions by %d degrees", crop->rotation);
        return (-1);
        }
      seg_buffs[0].buffer = crop_buff;
      seg_buffs[0].size = (((crop->combined_width * image->bps + 7 ) / 8)
                            * image->spp) * crop->combined_length; 
      }
    }
  else  /* Separated Images */
    {
    total_width = total_length = 0;
    for (i = 0; i < crop->selections; i++)
      {
      cropsize = crop->bufftotal;
      crop_buff = seg_buffs[i].buffer; 
      if (!crop_buff)
        crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
      else
        {
        prev_cropsize = seg_buffs[0].size;
        if (prev_cropsize < cropsize)
          {
          next_buff = _TIFFrealloc(crop_buff, cropsize);
          if (! next_buff)
            {
            _TIFFfree (crop_buff);
            crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
            }
          else
            crop_buff = next_buff;
          }
        }

      if (!crop_buff)
        {
        TIFFError("processCropSelections", "Unable to allocate/reallocate crop buffer");
        return (-1);
        }
 
      _TIFFmemset(crop_buff, 0, cropsize);
      seg_buffs[i].buffer = crop_buff;
      seg_buffs[i].size = cropsize;

      if (extractSeparateRegion(image, crop, read_buff, crop_buff, i))
        {
	TIFFError("processCropSelections", "Unable to extract cropped region %d from image", i);
        return (-1);
        }
    
      width  = crop->regionlist[i].width;
      length = crop->regionlist[i].length;

      if (crop->crop_mode & CROP_INVERT)
        {
        switch (crop->photometric)
          {
          /* Just change the interpretation */
          case PHOTOMETRIC_MINISWHITE:
          case PHOTOMETRIC_MINISBLACK:
	       image->photometric = crop->photometric;
	       break;
          case INVERT_DATA_ONLY:
          case INVERT_DATA_AND_TAG:
               if (invertImage(image->photometric, image->spp, image->bps, 
                               width, length, crop_buff))
                 {
                 TIFFError("processCropSelections", 
                           "Failed to invert colorspace for region");
                 return (-1);
                 }
               if (crop->photometric == INVERT_DATA_AND_TAG)
                 {
                 switch (image->photometric)
                   {
                   case PHOTOMETRIC_MINISWHITE:
 	                image->photometric = PHOTOMETRIC_MINISBLACK;
	                break;
                   case PHOTOMETRIC_MINISBLACK:
 	                image->photometric = PHOTOMETRIC_MINISWHITE;
	                break;
                   default:
	                break;
	           }
	         }
               break;
          default: break;
          }
        }

      if (crop->crop_mode & CROP_MIRROR)
        {
        if (mirrorImage(image->spp, image->bps, crop->mirror, 
                        width, length, crop_buff))
          {
          TIFFError("processCropSelections", "Failed to mirror crop region %s", 
	           (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
          return (-1);
          }
        }

      if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
        {
	if (rotateImage(crop->rotation, image, &crop->regionlist[i].width, 
			&crop->regionlist[i].length, &crop_buff))
          {
          TIFFError("processCropSelections", 
                    "Failed to rotate crop region by %d degrees", crop->rotation);
          return (-1);
          }
        total_width  += crop->regionlist[i].width;
        total_length += crop->regionlist[i].length;
        crop->combined_width = total_width;
        crop->combined_length = total_length;
        seg_buffs[i].buffer = crop_buff;
        seg_buffs[i].size = (((crop->regionlist[i].width * image->bps + 7 ) / 8)
                               * image->spp) * crop->regionlist[i].length; 
        }
      }
    }
  return (0);
  } /* end processCropSelections */

/* Copy the crop section of the data from the current image into a buffer
 * and adjust the IFD values to reflect the new size. If no cropping is
 * required, use the origial read buffer as the crop buffer.
 *
 * There is quite a bit of redundancy between this routine and the more
 * specialized processCropSelections, but this provides
 * the most optimized path when no Zones or Regions are required.
 */
static int
createCroppedImage(struct image_data *image, struct crop_mask *crop, 
                   unsigned char **read_buff_ptr, unsigned char **crop_buff_ptr)
  {
  tsize_t   cropsize;
  unsigned  char *read_buff = NULL;
  unsigned  char *crop_buff = NULL;
  unsigned  char *new_buff  = NULL;
  static    tsize_t  prev_cropsize = 0;

  read_buff = *read_buff_ptr;

  /* process full image, no crop buffer needed */
  crop_buff = read_buff;
  *crop_buff_ptr = read_buff;
  crop->combined_width = image->width;
  crop->combined_length = image->length;

  cropsize = crop->bufftotal;
  crop_buff = *crop_buff_ptr;
  if (!crop_buff)
    {
    crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
    *crop_buff_ptr = crop_buff;
    _TIFFmemset(crop_buff, 0, cropsize);
    prev_cropsize = cropsize;
    }
  else
    {
    if (prev_cropsize < cropsize)
      {
      new_buff = _TIFFrealloc(crop_buff, cropsize);
      if (!new_buff)
        {
	free (crop_buff);
        crop_buff = (unsigned char *)_TIFFmalloc(cropsize);
        }
      else
        crop_buff = new_buff;
      _TIFFmemset(crop_buff, 0, cropsize);
      }
    }

  if (!crop_buff)
    {
    TIFFError("createCroppedImage", "Unable to allocate/reallocate crop buffer");
    return (-1);
    }
  *crop_buff_ptr = crop_buff;

  if (crop->crop_mode & CROP_INVERT)
    {
    switch (crop->photometric)
      {
      /* Just change the interpretation */
      case PHOTOMETRIC_MINISWHITE:
      case PHOTOMETRIC_MINISBLACK:
	   image->photometric = crop->photometric;
	   break;
      case INVERT_DATA_ONLY:
      case INVERT_DATA_AND_TAG:
           if (invertImage(image->photometric, image->spp, image->bps, 
                           crop->combined_width, crop->combined_length, crop_buff))
             {
             TIFFError("createCroppedImage", 
                       "Failed to invert colorspace for image or cropped selection");
             return (-1);
             }
           if (crop->photometric == INVERT_DATA_AND_TAG)
             {
             switch (image->photometric)
               {
               case PHOTOMETRIC_MINISWHITE:
 	            image->photometric = PHOTOMETRIC_MINISBLACK;
	            break;
               case PHOTOMETRIC_MINISBLACK:
 	            image->photometric = PHOTOMETRIC_MINISWHITE;
	            break;
               default:
	            break;
	       }
	     }
           break;
      default: break;
      }
    }

  if (crop->crop_mode & CROP_MIRROR)
    {
    if (mirrorImage(image->spp, image->bps, crop->mirror, 
                    crop->combined_width, crop->combined_length, crop_buff))
      {
      TIFFError("createCroppedImage", "Failed to mirror image or cropped selection %s", 
	       (crop->rotation == MIRROR_HORIZ) ? "horizontally" : "vertically");
      return (-1);
      }
    }

  if (crop->crop_mode & CROP_ROTATE) /* rotate should be last as it can reallocate the buffer */
    {
    if (rotateImage(crop->rotation, image, &crop->combined_width, 
                    &crop->combined_length, crop_buff_ptr))
      {
      TIFFError("createCroppedImage", 
                "Failed to rotate image or cropped selection by %d degrees", crop->rotation);
      return (-1);
      }
    }

  if (crop_buff == read_buff) /* we used the read buffer for the crop buffer */
    *read_buff_ptr = NULL;    /* so we don't try to free it later */

  return (0);
  } /* end createCroppedImage */


/* Code in this function is heavily indebted to code in tiffcp
 * with modifications by Richard Nolde to handle orientation correctly.
 * It will have to be updated significantly if support is added to
 * extract one or more samples from original image since the 
 * original code assumes we are always copying all samples.
 * Use of global variables for config, compression and others
 * should be replaced by addition to the crop_mask struct (which
 * will be renamed to proc_opts indicating that is controlls
 * user supplied processing options, not just cropping) and 
 * then passed in as an argument.
 */
static int  
writeCroppedImage(TIFF *in, TIFF *out, struct image_data *image, 
                  struct dump_opts *dump, uint32 width, uint32 length, 
                  unsigned char *crop_buff, int pagenum, int total_pages)
  {
  uint16 bps, spp;
  uint16 input_compression, input_photometric;
  uint16 input_planar;
  struct cpTag* p;

  input_compression = image->compression;
  input_photometric = image->photometric;
  spp = image->spp;
  bps = image->bps;

  TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
  TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bps);
  TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);

#ifdef DEBUG2
  TIFFError("writeCroppedImage", "Input compression: %s",
	    (input_compression == COMPRESSION_OJPEG) ? "Old Jpeg" :
	    ((input_compression == COMPRESSION_JPEG) ?  "New Jpeg" : "Non Jpeg"));
#endif

  if (compression != (uint16)-1)
    TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
  else
    {
    if (input_compression == COMPRESSION_OJPEG)
      {
      compression = COMPRESSION_JPEG;
      jpegcolormode = JPEGCOLORMODE_RAW;
      TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
      }
    else
      CopyField(TIFFTAG_COMPRESSION, compression);
    }

  if (compression == COMPRESSION_JPEG)
    {
    if ((input_photometric == PHOTOMETRIC_PALETTE) ||  /* color map indexed */
        (input_photometric == PHOTOMETRIC_MASK))       /* $holdout mask */
      {
      TIFFError ("writeCroppedImage",
                 "JPEG compression cannot be used with %s image data",
      	        (input_photometric == PHOTOMETRIC_PALETTE) ?
                 "palette" : "mask");
      return (-1);
      }
    if ((input_photometric == PHOTOMETRIC_RGB) &&
	(jpegcolormode == JPEGCOLORMODE_RGB))
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
    else
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, input_photometric);
    }
  else
    {
    if (compression == COMPRESSION_SGILOG || compression == COMPRESSION_SGILOG24)
      {
      TIFFSetField(out, TIFFTAG_PHOTOMETRIC, spp == 1 ?
			PHOTOMETRIC_LOGL : PHOTOMETRIC_LOGLUV);
      }
    else
      {
      if (input_compression == COMPRESSION_SGILOG ||
          input_compression == COMPRESSION_SGILOG24)
        {
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, spp == 1 ?
			  PHOTOMETRIC_LOGL : PHOTOMETRIC_LOGLUV);
        }
      else
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, image->photometric);
      }
    }

  if (((input_photometric == PHOTOMETRIC_LOGL) ||
       (input_photometric ==  PHOTOMETRIC_LOGLUV)) &&
      ((compression != COMPRESSION_SGILOG) && 
       (compression != COMPRESSION_SGILOG24)))
    {
    TIFFError("writeCroppedImage",
              "LogL and LogLuv source data require SGI_LOG or SGI_LOG24 compression");
    return (-1);
    }

  if (fillorder != 0)
    TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
  else
    CopyTag(TIFFTAG_FILLORDER, 1, TIFF_SHORT);

  /* The loadimage function reads input orientation and sets
   * image->orientation. The correct_image_orientation function
   * applies the required rotation and mirror operations to 
   * present the data in TOPLEFT orientation and updates 
   * image->orientation if any transforms are performed, 
   * as per EXIF standard. 
   */
  TIFFSetField(out, TIFFTAG_ORIENTATION, image->orientation);
	
  /*
   * Choose tiles/strip for the output image according to
   * the command line arguments (-tiles, -strips) and the
   * structure of the input image.
   */
  if (outtiled == -1)
    outtiled = TIFFIsTiled(in);
  if (outtiled) {
    /*
     * Setup output file's tile width&height.  If either
     * is not specified, use either the value from the
     * input image or, if nothing is defined, use the
     * library default.
     */
    if (tilewidth == (uint32) 0)
      TIFFGetField(in, TIFFTAG_TILEWIDTH, &tilewidth);
    if (tilelength == (uint32) 0)
      TIFFGetField(in, TIFFTAG_TILELENGTH, &tilelength);

    if (tilewidth == 0 || tilelength == 0)
      TIFFDefaultTileSize(out, &tilewidth, &tilelength);
    TIFFSetField(out, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(out, TIFFTAG_TILELENGTH, tilelength);
    } else {
       /*
	* RowsPerStrip is left unspecified: use either the
	* value from the input image or, if nothing is defined,
	* use the library default.
	*/
	if (rowsperstrip == (uint32) 0)
          {
	  if (!TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &rowsperstrip))
	    rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
          if (compression != COMPRESSION_JPEG)
            {
  	    if (rowsperstrip > length)
	      rowsperstrip = length;
	    }
	  }
	else 
          if (rowsperstrip == (uint32) -1)
	    rowsperstrip = length;
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}

  TIFFGetFieldDefaulted(in, TIFFTAG_PLANARCONFIG, &input_planar);
  if (config != (uint16) -1)
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
  else
    CopyField(TIFFTAG_PLANARCONFIG, config);
  if (spp <= 4)
    CopyTag(TIFFTAG_TRANSFERFUNCTION, 4, TIFF_SHORT);
  CopyTag(TIFFTAG_COLORMAP, 4, TIFF_SHORT);

/* SMinSampleValue & SMaxSampleValue */
  switch (compression) {
    case COMPRESSION_JPEG:
         if (((bps % 8) == 0) || ((bps % 12) == 0))
	   {
           TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
	   TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
           }
         else
           {
	   TIFFError("writeCroppedImage",
                     "JPEG compression requires 8 or 12 bits per sample");
           return (-1);
           }
	 break;
   case COMPRESSION_LZW:
   case COMPRESSION_ADOBE_DEFLATE:
   case COMPRESSION_DEFLATE:
	if (predictor != (uint16)-1)
          TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
	else
	  CopyField(TIFFTAG_PREDICTOR, predictor);
	break;
   case COMPRESSION_CCITTFAX3:
   case COMPRESSION_CCITTFAX4:
        if (bps != 1)
          {
	  TIFFError("writeCroppedImage",
            "Group 3/4 compression is not usable with bps > 1");
          return (-1);
	  }
	if (compression == COMPRESSION_CCITTFAX3) {
          if (g3opts != (uint32) -1)
	    TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, g3opts);
	  else
	    CopyField(TIFFTAG_GROUP3OPTIONS, g3opts);
	} else
	    CopyTag(TIFFTAG_GROUP4OPTIONS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_BADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CLEANFAXDATA, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_CONSECUTIVEBADFAXLINES, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVPARAMS, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXRECVTIME, 1, TIFF_LONG);
	    CopyTag(TIFFTAG_FAXSUBADDRESS, 1, TIFF_ASCII);
	 break;
    case COMPRESSION_NONE:
         break;
    default: break;
   }
   { uint32 len32;
     void** data;
     if (TIFFGetField(in, TIFFTAG_ICCPROFILE, &len32, &data))
       TIFFSetField(out, TIFFTAG_ICCPROFILE, len32, data);
   }
   { uint16 ninks;
     const char* inknames;
     if (TIFFGetField(in, TIFFTAG_NUMBEROFINKS, &ninks)) {
       TIFFSetField(out, TIFFTAG_NUMBEROFINKS, ninks);
       if (TIFFGetField(in, TIFFTAG_INKNAMES, &inknames)) {
	 int inknameslen = strlen(inknames) + 1;
	 const char* cp = inknames;
	 while (ninks > 1) {
	   cp = strchr(cp, '\0');
	   if (cp) {
	     cp++;
	     inknameslen += (strlen(cp) + 1);
	   }
	   ninks--;
         }
	 TIFFSetField(out, TIFFTAG_INKNAMES, inknameslen, inknames);
       }
     }
   }
   {
   unsigned short pg0, pg1;
   if (TIFFGetField(in, TIFFTAG_PAGENUMBER, &pg0, &pg1)) {
     TIFFSetField(out, TIFFTAG_PAGENUMBER, pagenum, total_pages);
     }
   }

  for (p = tags; p < &tags[NTAGS]; p++)
		CopyTag(p->tag, p->count, p->type);

  /* Compute the tile or strip dimensions and write to disk */
  if (outtiled)
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      if (writeBufferToContigTiles (out, crop_buff, length, width, spp, dump))
        TIFFError("","Unable to write contiguous tile data for page %d", pagenum);
      }
    else
      {
      if (writeBufferToSeparateTiles (out, crop_buff, length, width, spp, dump))
        TIFFError("","Unable to write separate tile data for page %d", pagenum);
      }
    }
  else
    {
    if (config == PLANARCONFIG_CONTIG)
      {
      if (writeBufferToContigStrips (out, crop_buff, length))
        TIFFError("","Unable to write contiguous strip data for page %d", pagenum);
      }
    else
      {
      if (writeBufferToSeparateStrips(out, crop_buff, length, width, spp, dump))
        TIFFError("","Unable to write separate strip data for page %d", pagenum);
      }
    }

  if (!TIFFWriteDirectory(out))
    {
    TIFFError("","Failed to write IFD for page number %d", pagenum);
    TIFFClose(out);
    return (-1);
    }

  return (0);
  } /* end writeCroppedImage */

static int
rotateContigSamples8bits(uint16 rotation, uint16 spp, uint16 bps, uint32 width, 
                         uint32 length,   uint32 col, uint8 *src, uint8 *dst)
  {
  int      ready_bits = 0;
  uint32   src_byte = 0, src_bit = 0;
  uint32   row, rowsize = 0, bit_offset = 0;
  uint8    matchbits = 0, maskbits = 0;
  uint8    buff1 = 0, buff2 = 0;
  uint8   *next;
  tsample_t sample;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("rotateContigSamples8bits","Invalid src or destination buffer");
    return (1);
    }

  rowsize = ((bps * spp * width) + 7) / 8;
  ready_bits = 0;
  maskbits =  (uint8)-1 >> ( 8 - bps);
  buff1 = buff2 = 0;

  for (row = 0; row < length ; row++)
    {
    bit_offset = col * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        src_bit  = (bit_offset + (sample * bps)) % 8;
        }

      switch (rotation)
	{
        case  90: next = src + src_byte - (row * rowsize);
                  break;
        case 270: next = src + src_byte + (row * rowsize);
	          break;
	default:  TIFFError("rotateContigSamples8bits", "Invalid rotation %d", rotation);
                  return (1);
        }
      matchbits = maskbits << (8 - src_bit - bps); 
      buff1 = ((*next) & matchbits) << (src_bit);

       /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 8)
        {
        *dst++ = buff2;
        buff2 = buff1;
        ready_bits -= 8;
        }
      else
        {
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  if (ready_bits > 0)
    {
    buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
    *dst++ = buff1;
    }

  return (0);
  }  /* end rotateContigSamples8bits */


static int
rotateContigSamples16bits(uint16 rotation, uint16 spp, uint16 bps, uint32 width, 
                         uint32 length,   uint32 col, uint8 *src, uint8 *dst)
  {
  int      ready_bits = 0;
  uint32   row, rowsize, bit_offset;
  uint32   src_byte = 0, src_bit = 0;
  uint16   matchbits = 0, maskbits = 0;
  uint16   buff1 = 0, buff2 = 0;
  uint8    bytebuff = 0;
  uint8   *next;
  tsample_t sample;

  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("rotateContigSamples16bits","Invalid src or destination buffer");
    return (1);
    }

  rowsize = ((bps * spp * width) + 7) / 8;
  ready_bits = 0;
  maskbits =  (uint16)-1 >> (16 - bps);
  buff1 = buff2 = 0;
  for (row = 0; row < length; row++)
    {
    bit_offset = col * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        src_bit  = (bit_offset + (sample * bps)) % 8;
        }

      switch (rotation)
	{
        case  90: next = src + src_byte - (row * rowsize);
                  break;
        case 270: next = src + src_byte + (row * rowsize);
	          break;
	default:  TIFFError("rotateContigSamples8bits", "Invalid rotation %d", rotation);
                  return (1);
        }
      matchbits = maskbits << (16 - src_bit - bps); 
      if (little_endian)
        buff1 = (next[0] << 8) | next[1];
      else
        buff1 = (next[1] << 8) | next[0];

      buff1 = (buff1 & matchbits) << (src_bit);

      /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 8)
        {
        bytebuff = (buff2 >> 8);
        *dst++ = bytebuff;
        ready_bits -= 8;
        /* shift in new bits */
        buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
        }
      else
        { /* add another bps bits to the buffer */
        bytebuff = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  if (ready_bits > 0)
    {
    bytebuff = (buff2 >> 8);
    *dst++ = bytebuff;
    }

  return (0);
  }  /* end rotateContigSamples16bits */

static int
rotateContigSamples24bits(uint16 rotation, uint16 spp, uint16 bps, uint32 width, 
                          uint32 length,   uint32 col, uint8 *src, uint8 *dst)
  {
  int      ready_bits = 0;
  uint32   row, rowsize, bit_offset;
  uint32   src_byte = 0, src_bit = 0;
  uint32   matchbits = 0, maskbits = 0;
  uint32   buff1 = 0, buff2 = 0;
  uint8    bytebuff1 = 0, bytebuff2 = 0;
  uint8   *next;
  tsample_t sample;


  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("rotateContigSamples24bits","Invalid src or destination buffer");
    return (1);
    }

  rowsize = ((bps * spp * width) + 7) / 8;
  ready_bits = 0;
  maskbits =  (uint32)-1 >> (32 - bps);
  buff1 = buff2 = 0;
  for (row = 0; row < length; row++)
    {
    bit_offset = col * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        src_bit  = (bit_offset + (sample * bps)) % 8;
        }

      switch (rotation)
	{
        case  90: next = src + src_byte - (row * rowsize);
                  break;
        case 270: next = src + src_byte + (row * rowsize);
	          break;
	default:  TIFFError("rotateContigSamples8bits", "Invalid rotation %d", rotation);
                  return (1);
        }
      matchbits = maskbits << (32 - src_bit - bps); 
      if (little_endian)
	buff1 = (next[0] << 24) | (next[1] << 16) | (next[2] << 8) | next[3];
      else
	buff1 = (next[3] << 24) | (next[2] << 16) | (next[1] << 8) | next[0];
      buff1 = (buff1 & matchbits) << (src_bit);

      /* If we have a full buffer's worth, write it out */
      if (ready_bits >= 16)
        {
        bytebuff1 = (buff2 >> 24);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 16);
        *dst++ = bytebuff2;
        ready_bits -= 16;

        /* shift in new bits */
        buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
        }
      else
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

 /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 24);
    *dst++ = bytebuff1;

    buff2 = (buff2 << 8);
    bytebuff2 = bytebuff1;
    ready_bits -= 8;
    }
 
  return (0);
  }  /* end rotateContigSamples24bits */

static int
rotateContigSamples32bits(uint16 rotation, uint16 spp, uint16 bps, uint32 width, 
                          uint32 length,   uint32 col, uint8 *src, uint8 *dst)
  {
  int    ready_bits = 0, shift_width = 0;
  int    bytes_per_sample, bytes_per_pixel;
  uint32 row, rowsize, bit_offset;
  uint32 src_byte, src_bit;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 maskbits = 0, matchbits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  uint8   *next;
  tsample_t sample;


  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("rotateContigSamples24bits","Invalid src or destination buffer");
    return (1);
    }

  bytes_per_sample = (bps + 7) / 8;
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if (bytes_per_pixel < (bytes_per_sample + 1))
    shift_width = bytes_per_pixel;
  else
    shift_width = bytes_per_sample + 1;

  rowsize = ((bps * spp * width) + 7) / 8;
  ready_bits = 0;
  maskbits =  (uint64)-1 >> (64 - bps);
  buff1 = buff2 = 0;
  for (row = 0; row < length; row++)
    {
    bit_offset = col * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        src_bit  = (bit_offset + (sample * bps)) % 8;
        }

      switch (rotation)
	{
        case  90: next = src + src_byte - (row * rowsize);
                  break;
        case 270: next = src + src_byte + (row * rowsize);
	          break;
	default:  TIFFError("rotateContigSamples8bits", "Invalid rotation %d", rotation);
                  return (1);
        }
      matchbits = maskbits << (64 - src_bit - bps); 
      if (little_endian)
        {
	longbuff1 = (next[0] << 24) | (next[1] << 16) | (next[2] << 8) | next[3];
        longbuff2 = longbuff1;
        }
      else
        {
	longbuff1 = (next[3] << 24) | (next[2] << 16) | (next[1] << 8) | next[0];
        longbuff2 = longbuff1;
	}

      buff3 = ((uint64)longbuff1 << 32) | longbuff2;
      buff1 = (buff3 & matchbits) << (src_bit);

      if (ready_bits < 32)
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 56);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 48);
        *dst++ = bytebuff2;
        bytebuff3 = (buff2 >> 40);
        *dst++ = bytebuff3;
        bytebuff4 = (buff2 >> 32);
        *dst++ = bytebuff4;
        ready_bits -= 32;
                    
        /* shift in new bits */
        buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 56);
    *dst++ = bytebuff1;
    buff2 = (buff2 << 8);
    ready_bits -= 8;
    }

  return (0);
  } /* end rotateContigSamples32bits */


/* Rotate an image by a multiple of 90 degrees clockwise */
static int
rotateImage(uint16 rotation, struct image_data *image, uint32 *img_width, 
            uint32 *img_length, unsigned char **ibuff_ptr)
  {
  int      shift_width;
  uint32   bytes_per_pixel, bytes_per_sample;
  uint32   row, rowsize, src_offset, dst_offset;
  uint32   i, col, width, length;
  uint32   colsize, buffsize, col_offset, pix_offset;
  unsigned char *ibuff;
  unsigned char *src;
  unsigned char *dst;
  uint16   spp, bps;
  float    res_temp;
  unsigned char *rbuff = NULL;

  width  = *img_width;
  length = *img_length;
  spp = image->spp;
  bps = image->bps;

  rowsize = ((bps * spp * width) + 7) / 8;
  colsize = ((bps * spp * length) + 7) / 8;
  if ((colsize * width) > (rowsize * length))
    buffsize = (colsize + 1) * width;
  else
    buffsize = (rowsize + 1) * length;

  bytes_per_sample = (bps + 7) / 8;
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if (bytes_per_pixel < (bytes_per_sample + 1))
    shift_width = bytes_per_pixel;
  else
    shift_width = bytes_per_sample + 1;

  switch (rotation)
    {
    case 0:
    case 360: return (0);
    case 90:
    case 180:
    case 270: break;
    default:  TIFFError("rotateImage", "Invalid rotation angle %d", rotation);
              return (-1);
    }

  if (!(rbuff = (unsigned char *)_TIFFmalloc(buffsize)))
    {
    TIFFError("rotateImage", "Unable to allocate rotation buffer of %1u bytes", buffsize);
    return (-1);
    }
  _TIFFmemset(rbuff, '\0', buffsize);

  ibuff = *ibuff_ptr;
  switch (rotation)
    {
    case 180: if ((bps % 8) == 0) /* byte alligned data */
                { 
                src = ibuff;
                pix_offset = (spp * bps) / 8;
                for (row = 0; row < length; row++)
                   {
		   dst_offset = (length - row - 1) * rowsize;
                   for (col = 0; col < width; col++)
                     { 
		     col_offset = (width - col - 1) * pix_offset;
                     dst = rbuff + dst_offset + col_offset;

		     for (i = 0; i  < bytes_per_pixel; i++)
		       *dst++ = *src++;
                     }
                   }
                }
	      else
                { /* non 8 bit per sample data */ 
                for (row = 0; row < length; row++)
                  {
		  src_offset = row * rowsize;
		  dst_offset = (length - row - 1) * rowsize;
		  src = ibuff + src_offset;
                  dst = rbuff + dst_offset;
                  switch (shift_width)
                    {
                    case 1: if (bps == 1)
			      {
                              if (reverseSamples8bits(spp, bps, width, src, dst))
                                {
		                _TIFFfree(rbuff);
                                return (-1);
                                }
                              break;
                              }
                            if (reverseSamples16bits(spp, bps, width, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    case 2: if (reverseSamples24bits(spp, bps, width, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    case 3: 
                    case 4: 
                    case 5: if (reverseSamples32bits(spp, bps, width, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    default: TIFFError("rotateImage","Unsupported bit depth %d", bps);
		             _TIFFfree(rbuff);
                             return (-1);      
                    }
		  }
		}
              _TIFFfree(ibuff);
              *(ibuff_ptr) = rbuff;
              break;

    case 90:  if ((bps % 8) == 0) /* byte aligned data */
                {
                for (col = 0; col < width; col++)
                  {
		  src_offset = ((length - 1) * rowsize) + (col * bytes_per_pixel);
                  dst_offset = col * colsize;
		  src = ibuff + src_offset;
		  dst = rbuff + dst_offset;
                  for (row = length; row > 0; row--)
                    {
                    for (i = 0; i < bytes_per_pixel; i++)
                      *dst++ = *(src + i);
		    src -= rowsize;
                    }
		  }
		}
              else
                { /* non 8 bit per sample data */ 
                for (col = 0; col < width; col++)
                  {
		  src_offset = (length - 1) * rowsize;
                  dst_offset = col * colsize;
		  src = ibuff + src_offset;
		  dst = rbuff + dst_offset;
                  switch (shift_width)
                    {
                    case 1: if (bps == 1)
			      {
                              if (rotateContigSamples8bits(rotation, spp, bps, width, 
				   	                 length, col, src, dst))
                                {
		                _TIFFfree(rbuff);
                                return (-1);
                                }
                              break;
                              }
                            if (rotateContigSamples16bits(rotation, spp, bps, width, 
				   	                 length, col, src, dst))
                              {
	                      _TIFFfree(rbuff);
                              return (-1);
		              }
		            break;
                    case 2: if (rotateContigSamples24bits(rotation, spp, bps, width, 
					                  length, col, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    case 3: 
                    case 4: 
                    case 5: if (rotateContigSamples32bits(rotation, spp, bps, width, 
					                  length, col, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    default: TIFFError("rotateImage","Unsupported bit depth %d", bps);
		             _TIFFfree(rbuff);
                             return (-1);      
		    }
		  }
		}
              _TIFFfree(ibuff);
              *(ibuff_ptr) = rbuff;

              *img_width = length;
              *img_length = width;
              image->width = length;
              image->length = width;
              res_temp = image->xres;
              image->xres = image->yres;
              image->yres = res_temp;
	      break;

    case 270: if ((bps % 8) == 0) /* byte aligned data */
                {
                for (col = 0; col < width; col++)
                  {
		  src_offset = col * bytes_per_pixel;
                  dst_offset = (width - col - 1) * colsize;
		  src = ibuff + src_offset;
		  dst = rbuff + dst_offset;
                  for (row = length; row > 0; row--)
                    {
                    for (i = 0; i < bytes_per_pixel; i++)
                      *dst++ = *(src + i);
		    src += rowsize;
                    }
		  }
		}
              else
                { /* non 8 bit per sample data */ 
                for (col = 0; col < width; col++)
                  {
		  src_offset = 0;
                  dst_offset = (width - col - 1) * colsize;
		  src = ibuff + src_offset;
		  dst = rbuff + dst_offset;
                  switch (shift_width)
                    {
                    case 1: if (bps == 1)
			      {
                              if (rotateContigSamples8bits(rotation, spp, bps, width, 
				   	                 length, col, src, dst))
                                {
		                _TIFFfree(rbuff);
                                return (-1);
                                }
                              break;
                              }
                            if (rotateContigSamples16bits(rotation, spp, bps, width, 
				   	                 length, col, src, dst))
                              {
	                      _TIFFfree(rbuff);
                              return (-1);
		              }
		            break;
                    case 2: if (rotateContigSamples24bits(rotation, spp, bps, width, 
					                  length, col, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    case 3: 
                    case 4: 
                    case 5: if (rotateContigSamples32bits(rotation, spp, bps, width, 
					                  length, col, src, dst))
                              {
		              _TIFFfree(rbuff);
                              return (-1);
                              }
                             break;
                    default: TIFFError("rotateImage","Unsupported bit depth %d", bps);
		             _TIFFfree(rbuff);
                             return (-1);      
		    }
		  }
		}
              _TIFFfree(ibuff);
              *(ibuff_ptr) = rbuff;

              *img_width = length;
              *img_length = width;
              image->width = length;
              image->length = width;
              res_temp = image->xres;
              image->xres = image->yres;
              image->yres = res_temp;
              break;
    default:
              break;
    }

  return (0);
  } /* end rotateImage */

static int
reverseSamples8bits (uint16 spp, uint16 bps, uint32 width, 
                     uint8 *ibuff, uint8 *obuff)
  {
  int      ready_bits = 0;
  uint32   col;
  uint32   src_byte, src_bit;
  uint32   bit_offset = 0;
  uint8    match_bits = 0, mask_bits = 0;
  uint8    buff1 = 0, buff2 = 0;
  unsigned char *src;
  unsigned char *dst;
  tsample_t sample;

  if ((ibuff == NULL) || (obuff == NULL))
    {
    TIFFError("reverseSamples8bits","Invalid image or work buffer");
    return (1);
    }

  ready_bits = 0;
  mask_bits =  (uint8)-1 >> ( 8 - bps);
  dst = obuff;
  for (col = width; col > 0; col--)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = (col - 1) * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        src_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        src_bit  = (bit_offset + (sample * bps)) % 8;
        }

      src = ibuff + src_byte;
      match_bits = mask_bits << (8 - src_bit - bps); 
      buff1 = ((*src) & match_bits) << (src_bit);

      if (ready_bits < 8)
        buff2 = (buff2 | (buff1 >> ready_bits));
      else  /* If we have a full buffer's worth, write it out */
        {
        *dst++ = buff2;
        buff2 = buff1;
        ready_bits -= 8;
        }
      ready_bits += bps;
      }
    }
  if (ready_bits > 0)
    {
    buff1 = (buff2 & ((unsigned int)255 << (8 - ready_bits)));
    *dst++ = buff1;
    }

  return (0);
  } /* end reverseSamples8bits */


static int
reverseSamples16bits (uint16 spp, uint16 bps, uint32 width, 
                      uint8 *ibuff, uint8 *obuff)
  {
  int      ready_bits = 0;
  uint32   col;
  uint32   src_byte = 0, high_bit = 0;
  uint32   bit_offset = 0;
  uint16   match_bits = 0, mask_bits = 0;
  uint16   buff1 = 0, buff2 = 0;
  uint8    bytebuff = 0;
  unsigned char *src;
  unsigned char *dst;
  tsample_t sample;

  if ((ibuff == NULL) || (obuff == NULL))
    {
    TIFFError("reverseSample16bits","Invalid image or work buffer");
    return (1);
    }

  ready_bits = 0;
  mask_bits =  (uint16)-1 >> (16 - bps);
  dst = obuff;
  for (col = width; col > 0; col--)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = (col - 1) * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        high_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        high_bit  = (bit_offset + (sample * bps)) % 8;
        }

      src = ibuff + src_byte;
      match_bits = mask_bits << (16 - high_bit - bps); 
      if (little_endian)
        buff1 = (src[0] << 8) | src[1];
      else
        buff1 = (src[1] << 8) | src[0];
      buff1 = (buff1 & match_bits) << (high_bit);
      
      if (ready_bits < 8)
        { /* add another bps bits to the buffer */
        bytebuff = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff = (buff2 >> 8);
        *dst++ = bytebuff;
        ready_bits -= 8;
        /* shift in new bits */
        buff2 = ((buff2 << 8) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

  if (ready_bits > 0)
    {
    bytebuff = (buff2 >> 8);
    *dst++ = bytebuff;
    }

  return (0);
  } /* end reverseSamples16bits */

static int
reverseSamples24bits (uint16 spp, uint16 bps, uint32 width, 
                      uint8 *ibuff, uint8 *obuff)
  {
  int      ready_bits = 0;
  uint32   col;
  uint32   src_byte = 0, high_bit = 0;
  uint32   bit_offset = 0;
  uint32   match_bits = 0, mask_bits = 0;
  uint32   buff1 = 0, buff2 = 0;
  uint8    bytebuff1 = 0, bytebuff2 = 0;
  unsigned char *src;
  unsigned char *dst;
  tsample_t sample;

  if ((ibuff == NULL) || (obuff == NULL))
    {
    TIFFError("reverseSamples24bits","Invalid image or work buffer");
    return (1);
    }

  ready_bits = 0;
  mask_bits =  (uint32)-1 >> (32 - bps);
  dst = obuff;
  for (col = width; col > 0; col--)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = (col - 1) * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        high_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        high_bit  = (bit_offset + (sample * bps)) % 8;
        }

      src = ibuff + src_byte;
      match_bits = mask_bits << (32 - high_bit - bps); 
      if (little_endian)
	buff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
      else
	buff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
      buff1 = (buff1 & match_bits) << (high_bit);

      if (ready_bits < 16)
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 24);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 16);
        *dst++ = bytebuff2;
        ready_bits -= 16;

        /* shift in new bits */
        buff2 = ((buff2 << 16) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }

 /* catch any trailing bits at the end of the line */
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 24);
    *dst++ = bytebuff1;

    buff2 = (buff2 << 8);
    bytebuff2 = bytebuff1;
    ready_bits -= 8;
    }
 
  return (0);
  } /* end reverseSamples24bits */


static int
reverseSamples32bits (uint16 spp, uint16 bps, uint32 width, 
                      uint8 *ibuff, uint8 *obuff)
  {
  int    ready_bits = 0, shift_width = 0;
  int    bytes_per_sample, bytes_per_pixel;
  uint32 bit_offset;
  uint32 src_byte = 0, high_bit = 0;
  uint32 col;
  uint32 longbuff1 = 0, longbuff2 = 0;
  uint64 mask_bits = 0, match_bits = 0;
  uint64 buff1 = 0, buff2 = 0, buff3 = 0;
  uint8  bytebuff1 = 0, bytebuff2 = 0, bytebuff3 = 0, bytebuff4 = 0;
  unsigned char *src;
  unsigned char *dst;
  tsample_t sample;

  if ((ibuff == NULL) || (obuff == NULL))
    {
    TIFFError("reverseSamples32bits","Invalid image or work buffer");
    return (1);
    }

  ready_bits = 0;
  mask_bits =  (uint64)-1 >> (64 - bps);
  dst = obuff;

  bytes_per_sample = (bps + 7) / 8;
  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  if (bytes_per_pixel < (bytes_per_sample + 1))
    shift_width = bytes_per_pixel;
  else
    shift_width = bytes_per_sample + 1;

  for (col = width; col > 0; col--)
    {
    /* Compute src byte(s) and bits within byte(s) */
    bit_offset = (col - 1) * bps * spp;
    for (sample = 0; sample < spp; sample++)
      {
      if (sample == 0)
        {
        src_byte = bit_offset / 8;
        high_bit  = bit_offset % 8;
        }
      else
        {
        src_byte = (bit_offset + (sample * bps)) / 8;
        high_bit  = (bit_offset + (sample * bps)) % 8;
        }

      src = ibuff + src_byte;
      match_bits = mask_bits << (64 - high_bit - bps); 
      if (little_endian)
        {
	longbuff1 = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
        longbuff2 = longbuff1;
        }
      else
        {
	longbuff1 = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
        longbuff2 = longbuff1;
	}
      buff3 = ((uint64)longbuff1 << 32) | longbuff2;
      buff1 = (buff3 & match_bits) << (high_bit);

      if (ready_bits < 32)
        { /* add another bps bits to the buffer */
        bytebuff1 = bytebuff2 = bytebuff3 = bytebuff4 = 0;
        buff2 = (buff2 | (buff1 >> ready_bits));
        }
      else /* If we have a full buffer's worth, write it out */
        {
        bytebuff1 = (buff2 >> 56);
        *dst++ = bytebuff1;
        bytebuff2 = (buff2 >> 48);
        *dst++ = bytebuff2;
        bytebuff3 = (buff2 >> 40);
        *dst++ = bytebuff3;
        bytebuff4 = (buff2 >> 32);
        *dst++ = bytebuff4;
        ready_bits -= 32;
                    
        /* shift in new bits */
        buff2 = ((buff2 << 32) | (buff1 >> ready_bits));
        }
      ready_bits += bps;
      }
    }
  while (ready_bits > 0)
    {
    bytebuff1 = (buff2 >> 56);
    *dst++ = bytebuff1;
    buff2 = (buff2 << 8);
    ready_bits -= 8;
    }

  return (0);
  } /* end reverseSamples32bits */

static int
reverseSamplesBytes (uint16 spp, uint16 bps, uint32 width, 
                     uint8 *src, uint8 *dst)
  {
  int i;
  uint32  col, bytes_per_pixel, col_offset;
  uint8   bytebuff1;
  unsigned char swapbuff[32];
  
  if ((src == NULL) || (dst == NULL))
    {
    TIFFError("reverseSamplesBytes","Invalid input or output buffer");
    return (1);
    }

  bytes_per_pixel  = ((bps * spp) + 7) / 8;
  switch (bps / 8)
     {
     case 8:  /* Use memcpy for multiple bytes per sample data */
     case 4:
     case 3:
     case 2: for (col = 0; col < (width / 2); col++)
               {
	       col_offset = col * bytes_per_pixel;                     
	       _TIFFmemcpy (swapbuff, src + col_offset, bytes_per_pixel);
	       _TIFFmemcpy (src + col_offset, dst - col_offset - bytes_per_pixel, bytes_per_pixel);
	       _TIFFmemcpy (dst - col_offset - bytes_per_pixel, swapbuff, bytes_per_pixel);
               }
	     break;
     case 1: /* Use byte copy only for single byte per sample data */
             for (col = 0; col < (width / 2); col++)
               { 
	       for (i = 0; i < spp; i++)
                  {
		  bytebuff1 = *src;
		  *src++ = *(dst - spp + i);
                  *(dst - spp + i) = bytebuff1;
		  }
		dst -= spp;
                }
	     break;
     default: TIFFError("reverseSamplesBytes","Unsupported bit depth %d", bps);
       return (1);
     }
  return (0);
  } /* end reverseSamplesBytes */


/* Mirror an image horizontally or vertically */
static int
mirrorImage(uint16 spp, uint16 bps, uint16 mirror, uint32 width, uint32 length, unsigned char *ibuff)
  {
  int      shift_width;
  uint32   bytes_per_pixel, bytes_per_sample;
  uint32   row, rowsize, row_offset;
  unsigned char *line_buff = NULL;
  unsigned char *src;
  unsigned char *dst;

  src = ibuff;
  rowsize = ((width * bps * spp) + 7) / 8;
  switch (mirror)
    {
    case MIRROR_BOTH:
    case MIRROR_VERT: 
             line_buff = (unsigned char *)_TIFFmalloc(rowsize);
             if (line_buff == NULL)
               {
	       TIFFError ("mirrorImage", "Unable to allocate mirror line buffer of %1u bytes", rowsize);
               return (-1);
               }

             dst = ibuff + (rowsize * (length - 1));
             for (row = 0; row < length / 2; row++)
               {
	      _TIFFmemcpy(line_buff, src, rowsize);
	      _TIFFmemcpy(src, dst,  rowsize);
	      _TIFFmemcpy(dst, line_buff, rowsize);
               src += (rowsize);
               dst -= (rowsize);                                 
               }
             if (line_buff)
               _TIFFfree(line_buff);
             if (mirror == MIRROR_VERT)
               break;
    case MIRROR_HORIZ :
              if ((bps % 8) == 0) /* byte alligned data */
                { 
                for (row = 0; row < length; row++)
                  {
		  row_offset = row * rowsize;
                  src = ibuff + row_offset;
                  dst = ibuff + row_offset + rowsize;
                  if (reverseSamplesBytes(spp, bps, width, src, dst))
                    {
		    return (-1);
                    }
		  }
		}
	      else
                { /* non 8 bit per sample  data */
                if (!(line_buff = (unsigned char *)_TIFFmalloc(rowsize + 1)))
                  {
                  TIFFError("mirrorImage", "Unable to allocate mirror line buffer");
                  return (-1);
                  }
                bytes_per_sample = (bps + 7) / 8;
                bytes_per_pixel  = ((bps * spp) + 7) / 8;
                if (bytes_per_pixel < (bytes_per_sample + 1))
                  shift_width = bytes_per_pixel;
                else
                  shift_width = bytes_per_sample + 1;

                for (row = 0; row < length; row++)
                  {
		  row_offset = row * rowsize;
                  src = ibuff + row_offset;
                  _TIFFmemset (line_buff, '\0', rowsize);
                  switch (shift_width)
                    {
                    case 1: if (reverseSamples16bits(spp, bps, width, src, line_buff))
                              {
		              _TIFFfree(line_buff);
                              return (-1);
                              }
                             _TIFFmemcpy (src, line_buff, rowsize);
                             break;
                    case 2: if (reverseSamples24bits(spp, bps, width, src, line_buff))
                              {
		              _TIFFfree(line_buff);
                              return (-1);
                              }
                             _TIFFmemcpy (src, line_buff, rowsize);
                             break;
                    case 3: 
                    case 4: 
                    case 5: if (reverseSamples32bits(spp, bps, width, src, line_buff))
                              {
		              _TIFFfree(line_buff);
                              return (-1);
                              }
                             _TIFFmemcpy (src, line_buff, rowsize);
                             break;
                    default: TIFFError("mirrorImage","Unsupported bit depth %d", bps);
		             _TIFFfree(line_buff);
                             return (-1);      
                    }
		  }
                if (line_buff)
                  _TIFFfree(line_buff);
		}
             break;

    default: TIFFError ("mirrorImage", "Invalid mirror axis %d", mirror);
             return (-1);
             break;
    }

  return (0);
  }

/* Invert the light and dark values for a bilevel or grayscale image */
static int
invertImage(uint16 photometric, uint16 spp, uint16 bps, uint32 width, uint32 length, unsigned char *work_buff)
  {
  uint32   row, col;
  unsigned char  bytebuff1, bytebuff2, bytebuff3, bytebuff4;
  unsigned char *src;
  uint16        *src_uint16;
  uint32        *src_uint32;

  if (spp != 1)
    {
    TIFFError("invertImage", "Image inversion not supported for more than one sample per pixel");
    return (-1);
    }

  if (photometric !=  PHOTOMETRIC_MINISWHITE && photometric !=  PHOTOMETRIC_MINISBLACK)
    {
    TIFFError("invertImage", "Only black and white and grayscale images can be inverted");
    return (-1);
    }

  src = work_buff;
  if (src == NULL)
    {
    TIFFError ("invertImage", "Invalid crop buffer passed to invertImage");
    return (-1);
    }

  switch (bps)
    {
    case 32: src_uint32 = (uint32 *)src;
             for (row = 0; row < length; row++)
               for (col = 0; col < width; col++)
                 {
		 *src_uint32 = (uint32)0xFFFFFFFF - *src_uint32;
                  src_uint32++;
                 }
            break;
    case 16: src_uint16 = (uint16 *)src;
             for (row = 0; row < length; row++)
               for (col = 0; col < width; col++)
                 {
		 *src_uint16 = (uint16)0xFFFF - *src_uint16;
                  src_uint16++;
                 }
            break;
    case 8: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
		*src = (uint8)255 - *src;
                 src++;
                }
            break;
    case 4: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
		bytebuff1 = 16 - (uint8)(*src & 240 >> 4);
		bytebuff2 = 16 - (*src & 15);
		*src = bytebuff1 << 4 & bytebuff2;
                src++;
                }
            break;
    case 2: for (row = 0; row < length; row++)
              for (col = 0; col < width; col++)
                {
		bytebuff1 = 4 - (uint8)(*src & 192 >> 6);
		bytebuff2 = 4 - (uint8)(*src & 48  >> 4);
		bytebuff3 = 4 - (uint8)(*src & 12  >> 2);
		bytebuff4 = 4 - (uint8)(*src & 3);
		*src = (bytebuff1 << 6) || (bytebuff2 << 4) || (bytebuff3 << 2) || bytebuff4;
                src++;
                }
            break;
    case 1: for (row = 0; row < length; row++)
              for (col = 0; col < width; col += 8 /(spp * bps))
                {
                *src = ~(*src);
                src++;
                }
            break;
    default: TIFFError("invertImage", "Unsupported bit depth %d", bps);
      return (-1);
    }

  return (0);
  }

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
