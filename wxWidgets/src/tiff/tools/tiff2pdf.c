 *
 * tiff2pdf - converts a TIFF image to a PDF document
 *
 * Copyright (c) 2003 Ross Finlayson
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the name of
 * Ross Finlayson may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Ross Finlayson.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL ROSS FINLAYSON BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffiop.h"
#include "tiffio.h"

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS	0
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE	1
#endif

#define TIFF2PDF_MODULE "tiff2pdf"

#define PS_UNIT_SIZE	72.0F

/* This type is of PDF color spaces. */
typedef enum {
	T2P_CS_BILEVEL = 0x01,	/* Bilevel, black and white */
	T2P_CS_GRAY = 0x02,	/* Single channel */
	T2P_CS_RGB = 0x04,	/* Three channel tristimulus RGB */
	T2P_CS_CMYK = 0x08,	/* Four channel CMYK print inkset */
	T2P_CS_LAB = 0x10,	/* Three channel L*a*b* color space */
	T2P_CS_PALETTE = 0x1000,/* One of the above with a color map */
	T2P_CS_CALGRAY = 0x20,	/* Calibrated single channel */
	T2P_CS_CALRGB = 0x40,	/* Calibrated three channel tristimulus RGB */
	T2P_CS_ICCBASED = 0x80	/* ICC profile color specification */
} t2p_cs_t;

/* This type is of PDF compression types.  */
typedef enum{
	T2P_COMPRESS_NONE=0x00
#ifdef CCITT_SUPPORT
	, T2P_COMPRESS_G4=0x01
#endif
#if defined(JPEG_SUPPORT) || defined(OJPEG_SUPPORT)
	, T2P_COMPRESS_JPEG=0x02
#endif
#ifdef ZIP_SUPPORT
	, T2P_COMPRESS_ZIP=0x04
#endif
} t2p_compress_t;

/* This type is whether TIFF image data can be used in PDF without transcoding. */
typedef enum{
	T2P_TRANSCODE_RAW=0x01, /* The raw data from the input can be used without recompressing */
	T2P_TRANSCODE_ENCODE=0x02 /* The data from the input is perhaps unencoded and reencoded */
} t2p_transcode_t;

/* This type is of information about the data samples of the input image. */
typedef enum{
	T2P_SAMPLE_NOTHING=0x0000, /* The unencoded samples are normal for the output colorspace */
	T2P_SAMPLE_ABGR_TO_RGB=0x0001, /* The unencoded samples are the result of ReadRGBAImage */
	T2P_SAMPLE_RGBA_TO_RGB=0x0002, /* The unencoded samples are contiguous RGBA */
	T2P_SAMPLE_RGBAA_TO_RGB=0x0004, /* The unencoded samples are RGBA with premultiplied alpha */
	T2P_SAMPLE_YCBCR_TO_RGB=0x0008, 
	T2P_SAMPLE_YCBCR_TO_LAB=0x0010, 
	T2P_SAMPLE_REALIZE_PALETTE=0x0020, /* The unencoded samples are indexes into the color map */
	T2P_SAMPLE_SIGNED_TO_UNSIGNED=0x0040, /* The unencoded samples are signed instead of unsignd */
	T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED=0x0040, /* The L*a*b* samples have a* and b* signed */
	T2P_SAMPLE_PLANAR_SEPARATE_TO_CONTIG=0x0100 /* The unencoded samples are separate instead of contiguous */
} t2p_sample_t;

/* This type is of error status of the T2P struct. */
typedef enum{
	T2P_ERR_OK = 0, /* This is the value of t2p->t2p_error when there is no error */
	T2P_ERR_ERROR = 1 /* This is the value of t2p->t2p_error when there was an error */
} t2p_err_t;

/* This struct defines a logical page of a TIFF. */
typedef struct {
	tdir_t page_directory;
	uint32 page_number;
	ttile_t page_tilecount;
	uint32 page_extra;
} T2P_PAGE;

/* This struct defines a PDF rectangle's coordinates. */
typedef struct {
	float x1;
	float y1;
	float x2;
	float y2;
	float mat[9];
} T2P_BOX;

/* This struct defines a tile of a PDF.  */
typedef struct {
	T2P_BOX tile_box;
} T2P_TILE;

/* This struct defines information about the tiles on a PDF page. */
typedef struct {
	ttile_t tiles_tilecount;
	uint32 tiles_tilewidth;
	uint32 tiles_tilelength;
	uint32 tiles_tilecountx;
	uint32 tiles_tilecounty;
	uint32 tiles_edgetilewidth;
	uint32 tiles_edgetilelength;
	T2P_TILE* tiles_tiles;
} T2P_TILES;

/* This struct is the context of a function to generate PDF from a TIFF. */
typedef struct {
	t2p_err_t t2p_error;
	T2P_PAGE* tiff_pages;
	T2P_TILES* tiff_tiles;
	tdir_t tiff_pagecount;
	uint16 tiff_compression;
	uint16 tiff_photometric;
	uint16 tiff_fillorder;
	uint16 tiff_bitspersample;
	uint16 tiff_samplesperpixel;
	uint16 tiff_planar;
	uint32 tiff_width;
	uint32 tiff_length;
	float tiff_xres;
	float tiff_yres;
	uint16 tiff_orientation;
	toff_t tiff_dataoffset;
	tsize_t tiff_datasize;
	uint16 tiff_resunit;
	uint16 pdf_centimeters;
	uint16 pdf_overrideres;
	uint16 pdf_overridepagesize;
	float pdf_defaultxres;
	float pdf_defaultyres;
	float pdf_xres;
	float pdf_yres;
	float pdf_defaultpagewidth;
	float pdf_defaultpagelength;
	float pdf_pagewidth;
	float pdf_pagelength;
	float pdf_imagewidth;
	float pdf_imagelength;
	int pdf_image_fillpage; /* 0 (default: no scaling, 1:scale imagesize to pagesize */
	T2P_BOX pdf_mediabox;
	T2P_BOX pdf_imagebox;
	uint16 pdf_majorversion;
	uint16 pdf_minorversion;
	uint32 pdf_catalog;
	uint32 pdf_pages;
	uint32 pdf_info;
	uint32 pdf_palettecs;
	uint16 pdf_fitwindow;
	uint32 pdf_startxref;
#define TIFF2PDF_FILEID_SIZE 33
	char pdf_fileid[TIFF2PDF_FILEID_SIZE];
#define TIFF2PDF_DATETIME_SIZE 17
	char pdf_datetime[TIFF2PDF_DATETIME_SIZE];
#define TIFF2PDF_CREATOR_SIZE 512
	char pdf_creator[TIFF2PDF_CREATOR_SIZE];
#define TIFF2PDF_AUTHOR_SIZE 512
	char pdf_author[TIFF2PDF_AUTHOR_SIZE];
#define TIFF2PDF_TITLE_SIZE 512
	char pdf_title[TIFF2PDF_TITLE_SIZE];
#define TIFF2PDF_SUBJECT_SIZE 512
	char pdf_subject[TIFF2PDF_SUBJECT_SIZE];
#define TIFF2PDF_KEYWORDS_SIZE 512
	char pdf_keywords[TIFF2PDF_KEYWORDS_SIZE];
	t2p_cs_t pdf_colorspace;
	uint16 pdf_colorspace_invert;
	uint16 pdf_switchdecode;
	uint16 pdf_palettesize;
	unsigned char* pdf_palette;
	int pdf_labrange[4];
	t2p_compress_t pdf_defaultcompression;
	uint16 pdf_defaultcompressionquality;
	t2p_compress_t pdf_compression;
	uint16 pdf_compressionquality;
	uint16 pdf_nopassthrough;
	t2p_transcode_t pdf_transcode;
	t2p_sample_t pdf_sample;
	uint32* pdf_xrefoffsets;
	uint32 pdf_xrefcount;
	tdir_t pdf_page;
#ifdef OJPEG_SUPPORT
	tdata_t pdf_ojpegdata;
	uint32 pdf_ojpegdatalength;
	uint32 pdf_ojpegiflength;
#endif
	float tiff_whitechromaticities[2];
	float tiff_primarychromaticities[6];
	float tiff_referenceblackwhite[2];
	float* tiff_transferfunction[3];
	int pdf_image_interpolate;	/* 0 (default) : do not interpolate,
					   1 : interpolate */
	uint16 tiff_transferfunctioncount;
	uint32 pdf_icccs;
	uint32 tiff_iccprofilelength;
	tdata_t tiff_iccprofile;

	/* fields for custom read/write procedures */
	FILE *outputfile;
	int outputdisable;
	tsize_t outputwritten;
} T2P;

/* These functions are called by main. */

void tiff2pdf_usage(void);
int tiff2pdf_match_paper_size(float*, float*, char*);

/* These functions are used to generate a PDF from a TIFF. */ 

#ifdef __cplusplus
extern "C" {
#endif

T2P* t2p_init(void);
void t2p_validate(T2P*);
tsize_t t2p_write_pdf(T2P*, TIFF*, TIFF*);
void t2p_free(T2P*);

#ifdef __cplusplus
}
#endif

void t2p_read_tiff_init(T2P*, TIFF*);
int t2p_cmp_t2p_page(const void*, const void*);
void t2p_read_tiff_data(T2P*, TIFF*);
void t2p_read_tiff_size(T2P*, TIFF*);
void t2p_read_tiff_size_tile(T2P*, TIFF*, ttile_t);
int t2p_tile_is_right_edge(T2P_TILES, ttile_t);
int t2p_tile_is_bottom_edge(T2P_TILES, ttile_t);
int t2p_tile_is_edge(T2P_TILES, ttile_t);
int t2p_tile_is_corner_edge(T2P_TILES, ttile_t);
tsize_t t2p_readwrite_pdf_image(T2P*, TIFF*, TIFF*);
tsize_t t2p_readwrite_pdf_image_tile(T2P*, TIFF*, TIFF*, ttile_t);
#ifdef OJPEG_SUPPORT
int t2p_process_ojpeg_tables(T2P*, TIFF*);
#endif
#ifdef JPEG_SUPPORT
int t2p_process_jpeg_strip(unsigned char*, tsize_t*, unsigned char*, tsize_t*, tstrip_t, uint32);
#endif
void t2p_tile_collapse_left(tdata_t, tsize_t, uint32, uint32, uint32);
void t2p_write_advance_directory(T2P*, TIFF*);
tsize_t t2p_sample_planar_separate_to_contig(T2P*, unsigned char*, unsigned char*, tsize_t);
tsize_t t2p_sample_realize_palette(T2P*, unsigned char*);
tsize_t t2p_sample_abgr_to_rgb(tdata_t, uint32);
tsize_t t2p_sample_rgba_to_rgb(tdata_t, uint32);
tsize_t t2p_sample_rgbaa_to_rgb(tdata_t, uint32);
tsize_t t2p_sample_lab_signed_to_unsigned(tdata_t, uint32);
tsize_t t2p_write_pdf_header(T2P*, TIFF*);
tsize_t t2p_write_pdf_obj_start(uint32, TIFF*);
tsize_t t2p_write_pdf_obj_end(TIFF*);
tsize_t t2p_write_pdf_name(unsigned char*, TIFF*);
tsize_t t2p_write_pdf_string(char*, TIFF*);
tsize_t t2p_write_pdf_stream(tdata_t, tsize_t, TIFF*);
tsize_t t2p_write_pdf_stream_start(TIFF*);
tsize_t t2p_write_pdf_stream_end(TIFF*);
tsize_t t2p_write_pdf_stream_dict(tsize_t, uint32, TIFF*);
tsize_t t2p_write_pdf_stream_dict_start(TIFF*);
tsize_t t2p_write_pdf_stream_dict_end(TIFF*);
tsize_t t2p_write_pdf_stream_length(tsize_t, TIFF*);
tsize_t t2p_write_pdf_catalog(T2P*, TIFF*);
tsize_t t2p_write_pdf_info(T2P*, TIFF*, TIFF*);
void t2p_pdf_currenttime(T2P*);
void t2p_pdf_tifftime(T2P*, TIFF*);
tsize_t t2p_write_pdf_pages(T2P*, TIFF*);
tsize_t t2p_write_pdf_page(uint32, T2P*, TIFF*);
void t2p_compose_pdf_page(T2P*);
void t2p_compose_pdf_page_orient(T2P_BOX*, uint16);
void t2p_compose_pdf_page_orient_flip(T2P_BOX*, uint16);
tsize_t t2p_write_pdf_page_content(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_stream_dict(ttile_t, T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_cs(T2P*, TIFF*);
tsize_t t2p_write_pdf_transfer(T2P*, TIFF*);
tsize_t t2p_write_pdf_transfer_dict(T2P*, TIFF*, uint16);
tsize_t t2p_write_pdf_transfer_stream(T2P*, TIFF*, uint16);
tsize_t t2p_write_pdf_xobject_calcs(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_icccs(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_icccs_dict(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_icccs_stream(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_cs_stream(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_decode(T2P*, TIFF*);
tsize_t t2p_write_pdf_xobject_stream_filter(ttile_t, T2P*, TIFF*);
tsize_t t2p_write_pdf_xreftable(T2P*, TIFF*);
tsize_t t2p_write_pdf_trailer(T2P*, TIFF*);

static void
t2p_disable(TIFF *tif)
{
	T2P *t2p = (T2P*) TIFFClientdata(tif);
	t2p->outputdisable = 1;
}

static void
t2p_enable(TIFF *tif)
{
	T2P *t2p = (T2P*) TIFFClientdata(tif);
	t2p->outputdisable = 0;
}

/*
 * Procs for TIFFClientOpen
 */

static tmsize_t 
t2pReadFile(TIFF *tif, tdata_t data, tmsize_t size)
{
	thandle_t client = TIFFClientdata(tif);
	TIFFReadWriteProc proc = TIFFGetReadProc(tif);
	if (proc)
		return proc(client, data, size);
	return -1;
}

static tmsize_t 
t2pWriteFile(TIFF *tif, tdata_t data, tmsize_t size)
{
	thandle_t client = TIFFClientdata(tif);
	TIFFReadWriteProc proc = TIFFGetWriteProc(tif);
	if (proc)
		return proc(client, data, size);
	return -1;
}

static uint64
t2pSeekFile(TIFF *tif, toff_t offset, int whence)
{
	thandle_t client = TIFFClientdata(tif);
	TIFFSeekProc proc = TIFFGetSeekProc(tif);
	if (proc)
		return proc(client, offset, whence);
	return -1;
}

static tmsize_t 
t2p_readproc(thandle_t handle, tdata_t data, tmsize_t size) 
{
	(void) handle, (void) data, (void) size;
	return -1;
}

static tmsize_t 
t2p_writeproc(thandle_t handle, tdata_t data, tmsize_t size) 
{
	T2P *t2p = (T2P*) handle;
	if (t2p->outputdisable <= 0 && t2p->outputfile) {
		tsize_t written = fwrite(data, 1, size, t2p->outputfile);
		t2p->outputwritten += written;
		return written;
	}
	return size; 
}

static uint64 
t2p_seekproc(thandle_t handle, uint64 offset, int whence) 
{ 
	T2P *t2p = (T2P*) handle;
	if (t2p->outputdisable <= 0 && t2p->outputfile)
		return fseek(t2p->outputfile, (long) offset, whence);
	return offset;
}

static int 
t2p_closeproc(thandle_t handle)
{ 
	(void) handle;
	return 0; 
}

static uint64 
t2p_sizeproc(thandle_t handle) 
{
	(void) handle;
	return -1;
}

static int 
t2p_mapproc(thandle_t handle, void **data, toff_t *offset) 
{ 
	(void) handle, (void) data, (void) offset;
	return -1; 
}

static void 
t2p_unmapproc(thandle_t handle, void *data, toff_t offset)
{ 
	(void) handle, (void) data, (void) offset;
}

static uint64
checkAdd64(uint64 summand1, uint64 summand2, T2P* t2p)
{
	uint64 bytes = summand1 + summand2;

	if (bytes - summand1 != summand2) {
		TIFFError(TIFF2PDF_MODULE, "Integer overflow");
		t2p->t2p_error = T2P_ERR_ERROR;
		bytes = 0;
	}

	return bytes;
}

static uint64
checkMultiply64(uint64 first, uint64 second, T2P* t2p)
{
	uint64 bytes = first * second;

	if (second && bytes / second != first) {
		TIFFError(TIFF2PDF_MODULE, "Integer overflow");
		t2p->t2p_error = T2P_ERR_ERROR;
		bytes = 0;
	}

	return bytes;
}

/*

  This is the main function.

  The program converts one TIFF file to one PDF file, including multiple page 
  TIFF files, tiled TIFF files, black and white. grayscale, and color TIFF 
  files that contain data of TIFF photometric interpretations of bilevel, 
  grayscale, RGB, YCbCr, CMYK separation, and ICC L*a*b* as supported by 
  libtiff and PDF.

  If you have multiple TIFF files to convert into one PDF file then use tiffcp 
  or other program to concatenate the files into a multiple page TIFF file.  
  If the input TIFF file is of huge dimensions (greater than 10000 pixels height
  or width) convert the input image to a tiled TIFF if it is not already.

  The standard output is standard output.  Set the output file name with the 
  "-o output.pdf" option.

  All black and white files are compressed into a single strip CCITT G4 Fax 
  compressed PDF, unless tiled, where tiled black and white images are 
  compressed into tiled CCITT G4 Fax compressed PDF, libtiff CCITT support 
  is assumed.

  Color and grayscale data can be compressed using either JPEG compression, 
  ITU-T T.81, or Zip/Deflate LZ77 compression, per PNG 1.2 and RFC 1951.  Set 
  the compression type using the -j or -z options.  JPEG compression support 
  requires that libtiff be configured with JPEG support, and Zip/Deflate 
  compression support requires that libtiff is configured with Zip support, 
  in tiffconf.h.  Use only one or the other of -j and -z.  The -q option 
  sets the image compression quality, that is 1-100 with libjpeg JPEG 
  compression and one of 1, 10, 11, 12, 13, 14, or 15 for PNG group compression 
  predictor methods, add 100, 200, ..., 900 to set zlib compression quality 1-9.
  PNG Group differencing predictor methods are not currently implemented.

  If the input TIFF contains single strip CCITT G4 Fax compressed information, 
  then that is written to the PDF file without transcoding, unless the options 
  of no compression and no passthrough are set, -d and -n.

  If the input TIFF contains JPEG or single strip Zip/Deflate compressed 
  information, and they are configured, then that is written to the PDF file 
  without transcoding, unless the options of no compression and no passthrough 
  are set.

  The default page size upon which the TIFF image is placed is determined by 
  the resolution and extent of the image data.  Default values for the TIFF 
  image resolution can be set using the -x and -y options.  The page size can 
  be set using the -p option for paper size, or -w and -l for paper width and 
  length, then each page of the TIFF image is centered on its page.  The 
  distance unit for default resolution and page width and length can be set 
  by the -u option, the default unit is inch.

  Various items of the output document information can be set with the -e, -c, 
  -a, -t, -s, and -k tags.  Setting the argument of the option to "" for these 
  tags causes the relevant document information field to be not written.  Some 
  of the document information values otherwise get their information from the 
  input TIFF image, the software, author, document name, and image description.

  The output PDF file conforms to the PDF 1.1 specification or PDF 1.2 if using 
  Zip/Deflate compression.  
  
  The Portable Document Format (PDF) specification is copyrighted by Adobe 
  Systems, Incorporated.  Todos derechos reservados.

  Here is a listing of the usage example and the options to the tiff2pdf 
  program that is part of the libtiff distribution.  Options followed by 
  a colon have a required argument.
  
    usage:  tiff2pdf [options] input.tif

    options:
    -o: output to file name

    -j: compress with JPEG (requires libjpeg configured with libtiff)
    -z: compress with Zip/Deflate (requires zlib configured with libtiff)
    -q: compression quality
    -n: no compressed data passthrough
    -d: do not compress (decompress)
    -i: invert colors
    -u: set distance unit, 'i' for inch, 'm' for centimeter
    -x: set x resolution default
    -y: set y resolution default
    -w: width in units
    -l: length in units
    -r: 'd' for resolution default, 'o' for resolution override
    -p: paper size, eg "letter", "legal", "a4"
    -F: make the tiff fill the PDF page
    -f: set pdf "fit window" user preference
    -b:	set PDF "Interpolate" user preference
    -e: date, overrides image or current date/time default, YYYYMMDDHHMMSS
    -c: creator, overrides image software default
    -a: author, overrides image artist default
    -t: title, overrides image document name default
    -s: subject, overrides image image description default
    -k: keywords

    -h: usage

    examples:

        tiff2pdf -o output.pdf input.tiff

    The above example would generate the file output.pdf from input.tiff.

        tiff2pdf input.tiff

    The above example would generate PDF output from input.tiff and write it
    to standard output.

        tiff2pdf -j -p letter -o output.pdf input.tiff

    The above example would generate the file output.pdf from input.tiff,
    putting the image pages on a letter sized page, compressing the output
    with JPEG.

	Please report bugs through:

	http://bugzilla.remotesensing.org/buglist.cgi?product=libtiff

    See also libtiff.3t, tiffcp.
  */

int main(int argc, char** argv){

	extern char *optarg;
	extern int optind;
	const char *outfilename = NULL;
	T2P *t2p = NULL;
	TIFF *input = NULL, *output = NULL;
	int c, ret = EXIT_SUCCESS;

	t2p = t2p_init();

	if (t2p == NULL){
		TIFFError(TIFF2PDF_MODULE, "Can't initialize context");
		goto fail;
	}

	while (argv &&
	       (c = getopt(argc, argv,
			   "o:q:u:x:y:w:l:r:p:e:c:a:t:s:k:jzndifbhF")) != -1){
		switch (c) {
			case 'o':
				outfilename = optarg;
				break;
#ifdef JPEG_SUPPORT
			case 'j':  
				t2p->pdf_defaultcompression=T2P_COMPRESS_JPEG;
				break;
#endif
#ifndef JPEG_SUPPORT
			case 'j':  
				TIFFWarning(
					TIFF2PDF_MODULE, 
					"JPEG support in libtiff required for JPEG compression, ignoring option");
				break;
#endif
#ifdef ZIP_SUPPORT
			case 'z':  
				t2p->pdf_defaultcompression=T2P_COMPRESS_ZIP;
				break;
#endif
#ifndef ZIP_SUPPORT
			case 'z':  
				TIFFWarning(
					TIFF2PDF_MODULE, 
					"Zip support in libtiff required for Zip compression, ignoring option");
				break;
#endif
			case 'q': 
				t2p->pdf_defaultcompressionquality=atoi(optarg);
				break;
			case 'n': 
				t2p->pdf_nopassthrough=1;
				break;
			case 'd': 
				t2p->pdf_defaultcompression=T2P_COMPRESS_NONE;
				break;
			case 'u': 
				if(optarg[0]=='m'){
					t2p->pdf_centimeters=1;
				}
				break;
			case 'x': 
				t2p->pdf_defaultxres = 
					(float)atof(optarg) / (t2p->pdf_centimeters?2.54F:1.0F);
				break;
			case 'y': 
				t2p->pdf_defaultyres = 
					(float)atof(optarg) / (t2p->pdf_centimeters?2.54F:1.0F);
				break;
			case 'w': 
				t2p->pdf_overridepagesize=1;
				t2p->pdf_defaultpagewidth = 
					((float)atof(optarg) * PS_UNIT_SIZE) / (t2p->pdf_centimeters?2.54F:1.0F);
				break;
			case 'l': 
				t2p->pdf_overridepagesize=1;
				t2p->pdf_defaultpagelength = 
					((float)atof(optarg) * PS_UNIT_SIZE) / (t2p->pdf_centimeters?2.54F:1.0F);
				break;
			case 'r': 
				if(optarg[0]=='o'){
					t2p->pdf_overrideres=1;
				}
				break;
			case 'p': 
				if(tiff2pdf_match_paper_size(
					&(t2p->pdf_defaultpagewidth), 
					&(t2p->pdf_defaultpagelength), 
					optarg)){
					t2p->pdf_overridepagesize=1;
				} else {
					TIFFWarning(TIFF2PDF_MODULE, 
					"Unknown paper size %s, ignoring option",
						optarg);
				}
				break;
			case 'i':
				t2p->pdf_colorspace_invert=1;
				break;
			case 'F':
				t2p->pdf_image_fillpage = 1;
				break;
			case 'f': 
				t2p->pdf_fitwindow=1;
				break;
			case 'e':
				if (strlen(optarg) == 0) {
					t2p->pdf_datetime[0] = '\0';
				} else {
					t2p->pdf_datetime[0] = 'D';
					t2p->pdf_datetime[1] = ':';
					strncpy(t2p->pdf_datetime + 2, optarg,
						sizeof(t2p->pdf_datetime) - 3);
					t2p->pdf_datetime[sizeof(t2p->pdf_datetime) - 1] = '\0';
				}
				break;
			case 'c': 
				strncpy(t2p->pdf_creator, optarg, sizeof(t2p->pdf_creator) - 1);
				t2p->pdf_creator[sizeof(t2p->pdf_creator) - 1] = '\0';
				break;
			case 'a': 
				strncpy(t2p->pdf_author, optarg, sizeof(t2p->pdf_author) - 1);
				t2p->pdf_author[sizeof(t2p->pdf_author) - 1] = '\0';
				break;
			case 't': 
				strncpy(t2p->pdf_title, optarg, sizeof(t2p->pdf_title) - 1);
				t2p->pdf_title[sizeof(t2p->pdf_title) - 1] = '\0';
				break;
			case 's': 
				strncpy(t2p->pdf_subject, optarg, sizeof(t2p->pdf_subject) - 1);
				t2p->pdf_subject[sizeof(t2p->pdf_subject) - 1] = '\0';
				break;
			case 'k': 
				strncpy(t2p->pdf_keywords, optarg, sizeof(t2p->pdf_keywords) - 1);
				t2p->pdf_keywords[sizeof(t2p->pdf_keywords) - 1] = '\0';
				break;
			case 'b':
				t2p->pdf_image_interpolate = 1;
				break;
			case 'h': 
			case '?': 
				tiff2pdf_usage();
				goto success;
				break;
		}
	}

	/*
	 * Input
	 */
	if(argc > optind) {
		input = TIFFOpen(argv[optind++], "r");
		if (input==NULL) {
			TIFFError(TIFF2PDF_MODULE, 
				  "Can't open input file %s for reading", 
				  argv[optind-1]);
			goto fail;
		}
	} else {
		TIFFError(TIFF2PDF_MODULE, "No input file specified"); 
		tiff2pdf_usage();
		goto fail;
	}

	if(argc > optind) {
		TIFFError(TIFF2PDF_MODULE, 
			  "No support for multiple input files"); 
		tiff2pdf_usage();
		goto fail;
	}

	/*
	 * Output
	 */
	t2p->outputdisable = 0;
	if (outfilename) {
		t2p->outputfile = fopen(outfilename, "wb");
		if (t2p->outputfile == NULL) {
			TIFFError(TIFF2PDF_MODULE,
				  "Can't open output file %s for writing",
				  outfilename);
			goto fail;
		}
	} else {
		outfilename = "-";
		t2p->outputfile = stdout;
	}

	output = TIFFClientOpen(outfilename, "w", (thandle_t) t2p,
				t2p_readproc, t2p_writeproc, t2p_seekproc, 
				t2p_closeproc, t2p_sizeproc, 
				t2p_mapproc, t2p_unmapproc);
	if (output == NULL) {
		TIFFError(TIFF2PDF_MODULE,
			  "Can't initialize output descriptor");
		goto fail;
	}
	
	/*
	 * Validate
	 */
	t2p_validate(t2p);
	t2pSeekFile(output, (toff_t) 0, SEEK_SET);

	/*
	 * Write
	 */
	t2p_write_pdf(t2p, input, output);
	if (t2p->t2p_error != 0) {
		TIFFError(TIFF2PDF_MODULE,
			  "An error occurred creating output PDF file");
		goto fail;
	}

	goto success;
fail:
	ret = EXIT_FAILURE;
success:
	if(input != NULL)
		TIFFClose(input);
	if (output != NULL)
		TIFFClose(output);
	if (t2p != NULL)
		t2p_free(t2p);
	return ret;
  
}

void tiff2pdf_usage(){
	char* lines[]={
	"usage:  tiff2pdf [options] input.tiff",
	"options:",
	" -o: output to file name",
#ifdef JPEG_SUPPORT
	" -j: compress with JPEG", 
#endif
#ifdef ZIP_SUPPORT
	" -z: compress with Zip/Deflate",
#endif
	" -q: compression quality",
	" -n: no compressed data passthrough",
	" -d: do not compress (decompress)",
	" -i: invert colors",
	" -u: set distance unit, 'i' for inch, 'm' for centimeter",
	" -x: set x resolution default in dots per unit",
	" -y: set y resolution default in dots per unit",
	" -w: width in units",
	" -l: length in units",
	" -r: 'd' for resolution default, 'o' for resolution override",
	" -p: paper size, eg \"letter\", \"legal\", \"A4\"",
  " -F: make the tiff fill the PDF page",
	" -f: set PDF \"Fit Window\" user preference",
	" -e: date, overrides image or current date/time default, YYYYMMDDHHMMSS",
	" -c: sets document creator, overrides image software default",
	" -a: sets document author, overrides image artist default",
	" -t: sets document title, overrides image document name default",
	" -s: sets document subject, overrides image image description default",
	" -k: sets document keywords",
	" -b: set PDF \"Interpolate\" user preference",
	" -h: usage",
	NULL
	};
	int i=0;

	fprintf(stderr, "%s\n\n", TIFFGetVersion());
	for (i=0;lines[i]!=NULL;i++){
		fprintf(stderr, "%s\n", lines[i]);
	}

	return;
}

int tiff2pdf_match_paper_size(float* width, float* length, char* papersize){

	size_t i, len;
	const char* sizes[]={
		"LETTER", "A4", "LEGAL",
		"EXECUTIVE", "LETTER", "LEGAL", "LEDGER", "TABLOID", 
		"A", "B", "C", "D", "E", "F", "G", "H", "J", "K", 
		"A10", "A9", "A8", "A7", "A6", "A5", "A4", "A3", "A2", "A1", "A0", 
		"2A0", "4A0", "2A", "4A", 
		"B10", "B9", "B8", "B7", "B6", "B5", "B4", "B3", "B2", "B1", "B0", 
		"JISB10", "JISB9", "JISB8", "JISB7", "JISB6", "JISB5", "JISB4", 
		"JISB3", "JISB2", "JISB1", "JISB0", 
		"C10", "C9", "C8", "C7", "C6", "C5", "C4", "C3", "C2", "C1", "C0", 
		"RA2", "RA1", "RA0", "SRA4", "SRA3", "SRA2", "SRA1", "SRA0", 
		"A3EXTRA", "A4EXTRA", 
		"STATEMENT", "FOLIO", "QUARTO", 
		NULL
	} ;
	const int widths[]={
		612, 595, 612,
		522, 612,612,792,792,
		612,792,1224,1584,2448,2016,792,2016,2448,2880,
		74,105,147,210,298,420,595,842,1191,1684,2384,3370,4768,3370,4768,
		88,125,176,249,354,499,709,1001,1417,2004,2835,
		91,128,181,258,363,516,729,1032,1460,2064,2920,
		79,113,162,230,323,459,649,918,1298,1298,2599,
		1219,1729,2438,638,907,1276,1814,2551,
		914,667,
		396, 612, 609, 
		0
	};
	const int lengths[]={
		792,842,1008,
		756,792,1008,1224,1224,
		792,1224,1584,2448,3168,2880,6480,10296,12672,10296,
		105,147,210,298,420,595,842,1191,1684,2384,3370,4768,6741,4768,6741,
		125,176,249,354,499,709,1001,1417,2004,2835,4008,
		128,181,258,363,516,729,1032,1460,2064,2920,4127,
		113,162,230,323,459,649,918,1298,1837,1837,3677,
		1729,2438,3458,907,1276,1814,2551,3628,
		1262,914,
		612, 936, 780, 
		0
	};

	len=strlen(papersize);
	for(i=0;i<len;i++){
		papersize[i]=toupper(papersize[i]);
	}
	for(i=0;sizes[i]!=NULL; i++){
		if (strcmp( (const char*)papersize, sizes[i])==0){
			*width=(float)widths[i];
			*length=(float)lengths[i];
			return(1);
		}
	}

	return(0);
}

/*
 * This function allocates and initializes a T2P context struct pointer.
 */

T2P* t2p_init()
{
	T2P* t2p = (T2P*) _TIFFmalloc(sizeof(T2P));
	if(t2p==NULL){
		TIFFError(
			TIFF2PDF_MODULE, 
			"Can't allocate %lu bytes of memory for t2p_init", 
			(unsigned long) sizeof(T2P));
		return( (T2P*) NULL );
	}
	_TIFFmemset(t2p, 0x00, sizeof(T2P));
	t2p->pdf_majorversion=1;
	t2p->pdf_minorversion=1;
	t2p->pdf_defaultxres=300.0;
	t2p->pdf_defaultyres=300.0;
	t2p->pdf_defaultpagewidth=612.0;
	t2p->pdf_defaultpagelength=792.0;
	t2p->pdf_xrefcount=3; /* Catalog, Info, Pages */
	
	return(t2p);
}

/*
 * This function frees a T2P context struct pointer and any allocated data fields of it.
 */

void t2p_free(T2P* t2p)
{
	int i = 0;

	if (t2p != NULL) {
		if(t2p->pdf_xrefoffsets != NULL){
			_TIFFfree( (tdata_t) t2p->pdf_xrefoffsets);
		}
		if(t2p->tiff_pages != NULL){
			_TIFFfree( (tdata_t) t2p->tiff_pages);
		}
		for(i=0;i<t2p->tiff_pagecount;i++){
			if(t2p->tiff_tiles[i].tiles_tiles != NULL){
				_TIFFfree( (tdata_t) t2p->tiff_tiles[i].tiles_tiles);
			}
		}
		if(t2p->tiff_tiles != NULL){
			_TIFFfree( (tdata_t) t2p->tiff_tiles);
		}
		if(t2p->pdf_palette != NULL){
			_TIFFfree( (tdata_t) t2p->pdf_palette);
		}
#ifdef OJPEG_SUPPORT
		if(t2p->pdf_ojpegdata != NULL){
			_TIFFfree( (tdata_t) t2p->pdf_ojpegdata);
		}
#endif
		_TIFFfree( (tdata_t) t2p );
	}

	return;
}

/*
	This function validates the values of a T2P context struct pointer
        before calling t2p_write_pdf with it.
*/

void t2p_validate(T2P* t2p){

#ifdef JPEG_SUPPORT
	if(t2p->pdf_defaultcompression==T2P_COMPRESS_JPEG){
		if(t2p->pdf_defaultcompressionquality>100 ||
			t2p->pdf_defaultcompressionquality<1){
			t2p->pdf_defaultcompressionquality=0;
		}
	}
#endif
#ifdef ZIP_SUPPORT
	if(t2p->pdf_defaultcompression==T2P_COMPRESS_ZIP){
 		uint16 m=t2p->pdf_defaultcompressionquality%100;
 		if(t2p->pdf_defaultcompressionquality/100 > 9 ||
 			(m>1 && m<10) || m>15){
 			t2p->pdf_defaultcompressionquality=0;
		}
		if(t2p->pdf_defaultcompressionquality%100 !=0){
 			t2p->pdf_defaultcompressionquality/=100;
 			t2p->pdf_defaultcompressionquality*=100;
			TIFFError(
				TIFF2PDF_MODULE, 
				"PNG Group predictor differencing not implemented, assuming compression quality %u", 
				t2p->pdf_defaultcompressionquality);
		}
		t2p->pdf_defaultcompressionquality%=100;
		if(t2p->pdf_minorversion<2){t2p->pdf_minorversion=2;}
	}
#endif
	(void)0;

	return;
}


/*
	This function scans the input TIFF file for pages.  It attempts
        to determine which IFD's of the TIFF file contain image document
        pages.  For each, it gathers some information that has to do
        with the output of the PDF document as a whole.  
*/

void t2p_read_tiff_init(T2P* t2p, TIFF* input){

	tdir_t directorycount=0;
	tdir_t i=0;
	uint16 pagen=0;
	uint16 paged=0;
	uint16 xuint16=0;

	directorycount=TIFFNumberOfDirectories(input);
	t2p->tiff_pages = (T2P_PAGE*) _TIFFmalloc(directorycount * sizeof(T2P_PAGE));
	if(t2p->tiff_pages==NULL){
		TIFFError(
			TIFF2PDF_MODULE, 
			"Can't allocate %lu bytes of memory for tiff_pages array, %s", 
			(unsigned long) directorycount * sizeof(T2P_PAGE), 
			TIFFFileName(input));
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}
	_TIFFmemset( t2p->tiff_pages, 0x00, directorycount * sizeof(T2P_PAGE));
	t2p->tiff_tiles = (T2P_TILES*) _TIFFmalloc(directorycount * sizeof(T2P_TILES));
	if(t2p->tiff_tiles==NULL){
		TIFFError(
			TIFF2PDF_MODULE, 
			"Can't allocate %lu bytes of memory for tiff_tiles array, %s", 
			(unsigned long) directorycount * sizeof(T2P_TILES), 
			TIFFFileName(input));
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}
	_TIFFmemset( t2p->tiff_tiles, 0x00, directorycount * sizeof(T2P_TILES));
	for(i=0;i<directorycount;i++){
		uint32 subfiletype = 0;
		
		if(!TIFFSetDirectory(input, i)){
			TIFFError(
				TIFF2PDF_MODULE, 
				"Can't set directory %u of input file %s", 
				i,
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return;
		}
		if(TIFFGetField(input, TIFFTAG_PAGENUMBER, &pagen, &paged)){
			if((pagen>paged) && (paged != 0)){
				t2p->tiff_pages[t2p->tiff_pagecount].page_number = 
					paged;
			} else {
				t2p->tiff_pages[t2p->tiff_pagecount].page_number = 
					pagen;
			}
			goto ispage2;
		}
		if(TIFFGetField(input, TIFFTAG_SUBFILETYPE, &subfiletype)){
			if ( ((subfiletype & FILETYPE_PAGE) != 0)
                             || (subfiletype == 0)){
				goto ispage;
			} else {
				goto isnotpage;
			}
		}
		if(TIFFGetField(input, TIFFTAG_OSUBFILETYPE, &subfiletype)){
			if ((subfiletype == OFILETYPE_IMAGE) 
				|| (subfiletype == OFILETYPE_PAGE)
				|| (subfiletype == 0) ){
				goto ispage;
			} else {
				goto isnotpage;
			}
		}
		ispage:
		t2p->tiff_pages[t2p->tiff_pagecount].page_number=t2p->tiff_pagecount;
		ispage2:
		t2p->tiff_pages[t2p->tiff_pagecount].page_directory=i;
		if(TIFFIsTiled(input)){
			t2p->tiff_pages[t2p->tiff_pagecount].page_tilecount = 
				TIFFNumberOfTiles(input);
		}
		t2p->tiff_pagecount++;
		isnotpage:
		(void)0;
	}
	
	qsort((void*) t2p->tiff_pages, t2p->tiff_pagecount,
              sizeof(T2P_PAGE), t2p_cmp_t2p_page);

	for(i=0;i<t2p->tiff_pagecount;i++){
		t2p->pdf_xrefcount += 5;
		TIFFSetDirectory(input, t2p->tiff_pages[i].page_directory );
		if((TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &xuint16)
                    && (xuint16==PHOTOMETRIC_PALETTE))
		   || TIFFGetField(input, TIFFTAG_INDEXED, &xuint16)) {
			t2p->tiff_pages[i].page_extra++;
			t2p->pdf_xrefcount++;
		}
#ifdef ZIP_SUPPORT
		if (TIFFGetField(input, TIFFTAG_COMPRESSION, &xuint16)) {
                        if( (xuint16== COMPRESSION_DEFLATE ||
                             xuint16== COMPRESSION_ADOBE_DEFLATE) && 
                            ((t2p->tiff_pages[i].page_tilecount != 0) 
                             || TIFFNumberOfStrips(input)==1) &&
                            (t2p->pdf_nopassthrough==0)	){
                                if(t2p->pdf_minorversion<2){t2p->pdf_minorversion=2;}
                        }
                }
#endif
		if (TIFFGetField(input, TIFFTAG_TRANSFERFUNCTION,
                                 &(t2p->tiff_transferfunction[0]),
                                 &(t2p->tiff_transferfunction[1]),
                                 &(t2p->tiff_transferfunction[2]))) {
			if(t2p->tiff_transferfunction[1] !=
			   t2p->tiff_transferfunction[0]) {
				t2p->tiff_transferfunctioncount = 3;
				t2p->tiff_pages[i].page_extra += 4;
				t2p->pdf_xrefcount += 4;
			} else {
				t2p->tiff_transferfunctioncount = 1;
				t2p->tiff_pages[i].page_extra += 2;
				t2p->pdf_xrefcount += 2;
			}
			if(t2p->pdf_minorversion < 2)
				t2p->pdf_minorversion = 2;
                } else {
			t2p->tiff_transferfunctioncount=0;
		}
		if( TIFFGetField(
			input, 
			TIFFTAG_ICCPROFILE, 
			&(t2p->tiff_iccprofilelength), 
			&(t2p->tiff_iccprofile)) != 0){
			t2p->tiff_pages[i].page_extra++;
			t2p->pdf_xrefcount++;
			if(t2p->pdf_minorversion<3){t2p->pdf_minorversion=3;}
		}
		t2p->tiff_tiles[i].tiles_tilecount=
			t2p->tiff_pages[i].page_tilecount;
		if( (TIFFGetField(input, TIFFTAG_PLANARCONFIG, &xuint16) != 0)
			&& (xuint16 == PLANARCONFIG_SEPARATE ) ){
				TIFFGetField(input, TIFFTAG_SAMPLESPERPIXEL, &xuint16);
				t2p->tiff_tiles[i].tiles_tilecount/= xuint16;
		}
		if( t2p->tiff_tiles[i].tiles_tilecount > 0){
			t2p->pdf_xrefcount += 
				(t2p->tiff_tiles[i].tiles_tilecount -1)*2;
			TIFFGetField(input, 
				TIFFTAG_TILEWIDTH, 
				&( t2p->tiff_tiles[i].tiles_tilewidth) );
			TIFFGetField(input, 
				TIFFTAG_TILELENGTH, 
				&( t2p->tiff_tiles[i].tiles_tilelength) );
			t2p->tiff_tiles[i].tiles_tiles = 
			(T2P_TILE*) _TIFFmalloc(
				t2p->tiff_tiles[i].tiles_tilecount 
				* sizeof(T2P_TILE) );
			if( t2p->tiff_tiles[i].tiles_tiles == NULL){
				TIFFError(
					TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory for t2p_read_tiff_init, %s", 
					(unsigned long) t2p->tiff_tiles[i].tiles_tilecount * sizeof(T2P_TILE), 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
		}
	}

	return;
}

/*
 * This function is used by qsort to sort a T2P_PAGE* array of page structures
 * by page number.
 */

int t2p_cmp_t2p_page(const void* e1, const void* e2){

	return( ((T2P_PAGE*)e1)->page_number - ((T2P_PAGE*)e2)->page_number );
}

/*
	This function sets the input directory to the directory of a given
	page and determines information about the image.  It checks
	the image characteristics to determine if it is possible to convert
	the image data into a page of PDF output, setting values of the T2P
	struct for this page.  It determines what color space is used in
	the output PDF to represent the image.
	
	It determines if the image can be converted as raw data without
	requiring transcoding of the image data.
*/

void t2p_read_tiff_data(T2P* t2p, TIFF* input){

	int i=0;
	uint16* r;
	uint16* g;
	uint16* b;
	uint16* a;
	uint16 xuint16;
	uint16* xuint16p;
	float* xfloatp;

	t2p->pdf_transcode = T2P_TRANSCODE_ENCODE;
	t2p->pdf_sample = T2P_SAMPLE_NOTHING;
        t2p->pdf_switchdecode = t2p->pdf_colorspace_invert;
        
	
	TIFFSetDirectory(input, t2p->tiff_pages[t2p->pdf_page].page_directory);

	TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &(t2p->tiff_width));
	if(t2p->tiff_width == 0){
		TIFFError(
			TIFF2PDF_MODULE, 
			"No support for %s with zero width", 
			TIFFFileName(input)	);
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}

	TIFFGetField(input, TIFFTAG_IMAGELENGTH, &(t2p->tiff_length));
	if(t2p->tiff_length == 0){
		TIFFError(
			TIFF2PDF_MODULE, 
			"No support for %s with zero length", 
			TIFFFileName(input)	);
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}

        if(TIFFGetField(input, TIFFTAG_COMPRESSION, &(t2p->tiff_compression)) == 0){
                TIFFError(
                        TIFF2PDF_MODULE, 
                        "No support for %s with no compression tag", 
                        TIFFFileName(input)     );
                t2p->t2p_error = T2P_ERR_ERROR;
                return;

        }
        if( TIFFIsCODECConfigured(t2p->tiff_compression) == 0){
		TIFFError(
			TIFF2PDF_MODULE, 
			"No support for %s with compression type %u:  not configured", 
			TIFFFileName(input), 
			t2p->tiff_compression	
			);
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	
	}

	TIFFGetFieldDefaulted(input, TIFFTAG_BITSPERSAMPLE, &(t2p->tiff_bitspersample));
	switch(t2p->tiff_bitspersample){
		case 1:
		case 2:
		case 4:
		case 8:
			break;
		case 0:
			TIFFWarning(
				TIFF2PDF_MODULE, 
				"Image %s has 0 bits per sample, assuming 1",
				TIFFFileName(input));
			t2p->tiff_bitspersample=1;
			break;
		default:
			TIFFError(
				TIFF2PDF_MODULE, 
				"No support for %s with %u bits per sample",
				TIFFFileName(input),
				t2p->tiff_bitspersample);
			t2p->t2p_error = T2P_ERR_ERROR;
			return;
	}

	TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &(t2p->tiff_samplesperpixel));
	if(t2p->tiff_samplesperpixel>4){
		TIFFError(
			TIFF2PDF_MODULE, 
			"No support for %s with %u samples per pixel",
			TIFFFileName(input),
			t2p->tiff_samplesperpixel);
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}
	if(t2p->tiff_samplesperpixel==0){
		TIFFWarning(
			TIFF2PDF_MODULE, 
			"Image %s has 0 samples per pixel, assuming 1",
			TIFFFileName(input));
		t2p->tiff_samplesperpixel=1;
	}
	
	if(TIFFGetField(input, TIFFTAG_SAMPLEFORMAT, &xuint16) != 0 ){
		switch(xuint16){
			case 0:
			case 1:
			case 4:
				break;
			default:
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for %s with sample format %u",
					TIFFFileName(input),
					xuint16);
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
				break;
		}
	}
	
	TIFFGetFieldDefaulted(input, TIFFTAG_FILLORDER, &(t2p->tiff_fillorder));
	
        if(TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &(t2p->tiff_photometric)) == 0){
                TIFFError(
                        TIFF2PDF_MODULE, 
                        "No support for %s with no photometric interpretation tag", 
                        TIFFFileName(input)     );
                t2p->t2p_error = T2P_ERR_ERROR;
                return;

        }
        
	switch(t2p->tiff_photometric){
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK: 
			if (t2p->tiff_bitspersample==1){
				t2p->pdf_colorspace=T2P_CS_BILEVEL;
				if(t2p->tiff_photometric==PHOTOMETRIC_MINISWHITE){
					t2p->pdf_switchdecode ^= 1;
				}
			} else {
				t2p->pdf_colorspace=T2P_CS_GRAY;
				if(t2p->tiff_photometric==PHOTOMETRIC_MINISWHITE){
					t2p->pdf_switchdecode ^= 1;
				} 
			}
			break;
		case PHOTOMETRIC_RGB: 
			t2p->pdf_colorspace=T2P_CS_RGB;
			if(t2p->tiff_samplesperpixel == 3){
				break;
			}
			if(TIFFGetField(input, TIFFTAG_INDEXED, &xuint16)){
				if(xuint16==1)
					goto photometric_palette;
			}
			if(t2p->tiff_samplesperpixel > 3) {
				if(t2p->tiff_samplesperpixel == 4) {
					t2p->pdf_colorspace = T2P_CS_RGB;
					if(TIFFGetField(input,
							TIFFTAG_EXTRASAMPLES,
							&xuint16, &xuint16p)
					   && xuint16 == 1) {
						if(xuint16p[0] == EXTRASAMPLE_ASSOCALPHA){
							t2p->pdf_sample=T2P_SAMPLE_RGBAA_TO_RGB;
							break;
						}
						if(xuint16p[0] == EXTRASAMPLE_UNASSALPHA){
							t2p->pdf_sample=T2P_SAMPLE_RGBA_TO_RGB;
							break;
						}
						TIFFWarning(
							TIFF2PDF_MODULE, 
							"RGB image %s has 4 samples per pixel, assuming RGBA",
							TIFFFileName(input));
							break;
					}
					t2p->pdf_colorspace=T2P_CS_CMYK;
					t2p->pdf_switchdecode ^= 1;
					TIFFWarning(
						TIFF2PDF_MODULE, 
						"RGB image %s has 4 samples per pixel, assuming inverse CMYK",
					TIFFFileName(input));
					break;
				} else {
					TIFFError(
						TIFF2PDF_MODULE, 
						"No support for RGB image %s with %u samples per pixel", 
						TIFFFileName(input), 
						t2p->tiff_samplesperpixel);
					t2p->t2p_error = T2P_ERR_ERROR;
					break;
				}
			} else {
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for RGB image %s with %u samples per pixel", 
					TIFFFileName(input), 
					t2p->tiff_samplesperpixel);
				t2p->t2p_error = T2P_ERR_ERROR;
				break;
			}
		case PHOTOMETRIC_PALETTE: 
			photometric_palette:
			if(t2p->tiff_samplesperpixel!=1){
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for palettized image %s with not one sample per pixel", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			t2p->pdf_colorspace=T2P_CS_RGB | T2P_CS_PALETTE;
			t2p->pdf_palettesize=0x0001<<t2p->tiff_bitspersample;
			if(!TIFFGetField(input, TIFFTAG_COLORMAP, &r, &g, &b)){
				TIFFError(
					TIFF2PDF_MODULE, 
					"Palettized image %s has no color map", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			} 
			if(t2p->pdf_palette != NULL){
				_TIFFfree(t2p->pdf_palette);
				t2p->pdf_palette=NULL;
			}
			t2p->pdf_palette = (unsigned char*)
				_TIFFmalloc(t2p->pdf_palettesize*3);
			if(t2p->pdf_palette==NULL){
				TIFFError(
					TIFF2PDF_MODULE, 
					"Can't allocate %u bytes of memory for t2p_read_tiff_image, %s", 
					t2p->pdf_palettesize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			for(i=0;i<t2p->pdf_palettesize;i++){
				t2p->pdf_palette[(i*3)]  = (unsigned char) (r[i]>>8);
				t2p->pdf_palette[(i*3)+1]= (unsigned char) (g[i]>>8);
				t2p->pdf_palette[(i*3)+2]= (unsigned char) (b[i]>>8);
			}
			t2p->pdf_palettesize *= 3;
			break;
		case PHOTOMETRIC_SEPARATED:
			if(TIFFGetField(input, TIFFTAG_INDEXED, &xuint16)){
				if(xuint16==1){
						goto photometric_palette_cmyk;
				}
			}
			if( TIFFGetField(input, TIFFTAG_INKSET, &xuint16) ){
				if(xuint16 != INKSET_CMYK){
					TIFFError(
						TIFF2PDF_MODULE, 
						"No support for %s because its inkset is not CMYK",
						TIFFFileName(input) );
					t2p->t2p_error = T2P_ERR_ERROR;
					return;
				}
			}
			if(t2p->tiff_samplesperpixel==4){
				t2p->pdf_colorspace=T2P_CS_CMYK;
			} else {
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for %s because it has %u samples per pixel",
					TIFFFileName(input), 
					t2p->tiff_samplesperpixel);
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			break;
			photometric_palette_cmyk:
			if(t2p->tiff_samplesperpixel!=1){
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for palettized CMYK image %s with not one sample per pixel", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			t2p->pdf_colorspace=T2P_CS_CMYK | T2P_CS_PALETTE;
			t2p->pdf_palettesize=0x0001<<t2p->tiff_bitspersample;
			if(!TIFFGetField(input, TIFFTAG_COLORMAP, &r, &g, &b, &a)){
				TIFFError(
					TIFF2PDF_MODULE, 
					"Palettized image %s has no color map", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			} 
			if(t2p->pdf_palette != NULL){
				_TIFFfree(t2p->pdf_palette);
				t2p->pdf_palette=NULL;
			}
			t2p->pdf_palette = (unsigned char*) 
				_TIFFmalloc(t2p->pdf_palettesize*4);
			if(t2p->pdf_palette==NULL){
				TIFFError(
					TIFF2PDF_MODULE, 
					"Can't allocate %u bytes of memory for t2p_read_tiff_image, %s", 
					t2p->pdf_palettesize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			for(i=0;i<t2p->pdf_palettesize;i++){
				t2p->pdf_palette[(i*4)]  = (unsigned char) (r[i]>>8);
				t2p->pdf_palette[(i*4)+1]= (unsigned char) (g[i]>>8);
				t2p->pdf_palette[(i*4)+2]= (unsigned char) (b[i]>>8);
				t2p->pdf_palette[(i*4)+3]= (unsigned char) (a[i]>>8);
			}
			t2p->pdf_palettesize *= 4;
			break;
		case PHOTOMETRIC_YCBCR:
			t2p->pdf_colorspace=T2P_CS_RGB;
			if(t2p->tiff_samplesperpixel==1){
				t2p->pdf_colorspace=T2P_CS_GRAY;
				t2p->tiff_photometric=PHOTOMETRIC_MINISBLACK;
				break;
			}
			t2p->pdf_sample=T2P_SAMPLE_YCBCR_TO_RGB;
#ifdef JPEG_SUPPORT
			if(t2p->pdf_defaultcompression==T2P_COMPRESS_JPEG){
				t2p->pdf_sample=T2P_SAMPLE_NOTHING;
			}
#endif
			break;
		case PHOTOMETRIC_CIELAB:
			t2p->pdf_labrange[0]= -127;
			t2p->pdf_labrange[1]= 127;
			t2p->pdf_labrange[2]= -127;
			t2p->pdf_labrange[3]= 127;
			t2p->pdf_sample=T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED;
			t2p->pdf_colorspace=T2P_CS_LAB;
			break;
		case PHOTOMETRIC_ICCLAB:
			t2p->pdf_labrange[0]= 0;
			t2p->pdf_labrange[1]= 255;
			t2p->pdf_labrange[2]= 0;
			t2p->pdf_labrange[3]= 255;
			t2p->pdf_colorspace=T2P_CS_LAB;
			break;
		case PHOTOMETRIC_ITULAB:
			t2p->pdf_labrange[0]=-85;
			t2p->pdf_labrange[1]=85;
			t2p->pdf_labrange[2]=-75;
			t2p->pdf_labrange[3]=124;
			t2p->pdf_sample=T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED;
			t2p->pdf_colorspace=T2P_CS_LAB;
			break;
		case PHOTOMETRIC_LOGL:
		case PHOTOMETRIC_LOGLUV:
			TIFFError(
				TIFF2PDF_MODULE, 
				"No support for %s with photometric interpretation LogL/LogLuv", 
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return;
		default:
			TIFFError(
				TIFF2PDF_MODULE, 
				"No support for %s with photometric interpretation %u", 
				TIFFFileName(input),
				t2p->tiff_photometric);
			t2p->t2p_error = T2P_ERR_ERROR;
			return;
	}

	if(TIFFGetField(input, TIFFTAG_PLANARCONFIG, &(t2p->tiff_planar))){
		switch(t2p->tiff_planar){
			case 0:
				TIFFWarning(
					TIFF2PDF_MODULE, 
					"Image %s has planar configuration 0, assuming 1", 
					TIFFFileName(input));
				t2p->tiff_planar=PLANARCONFIG_CONTIG;
			case PLANARCONFIG_CONTIG:
				break;
			case PLANARCONFIG_SEPARATE:
				t2p->pdf_sample=T2P_SAMPLE_PLANAR_SEPARATE_TO_CONTIG;
				if(t2p->tiff_bitspersample!=8){
					TIFFError(
						TIFF2PDF_MODULE, 
						"No support for %s with separated planar configuration and %u bits per sample", 
						TIFFFileName(input),
						t2p->tiff_bitspersample);
					t2p->t2p_error = T2P_ERR_ERROR;
					return;
				}
				break;
			default:
				TIFFError(
					TIFF2PDF_MODULE, 
					"No support for %s with planar configuration %u", 
					TIFFFileName(input),
					t2p->tiff_planar);
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
		}
	}

        TIFFGetFieldDefaulted(input, TIFFTAG_ORIENTATION,
                              &(t2p->tiff_orientation));
        if(t2p->tiff_orientation>8){
                TIFFWarning(TIFF2PDF_MODULE,
                            "Image %s has orientation %u, assuming 0",
                            TIFFFileName(input), t2p->tiff_orientation);
                t2p->tiff_orientation=0;
        }

        if(TIFFGetField(input, TIFFTAG_XRESOLUTION, &(t2p->tiff_xres) ) == 0){
                t2p->tiff_xres=0.0;
        }
        if(TIFFGetField(input, TIFFTAG_YRESOLUTION, &(t2p->tiff_yres) ) == 0){
                t2p->tiff_yres=0.0;
        }
	TIFFGetFieldDefaulted(input, TIFFTAG_RESOLUTIONUNIT,
			      &(t2p->tiff_resunit));
	if(t2p->tiff_resunit == RESUNIT_CENTIMETER) {
		t2p->tiff_xres *= 2.54F;
		t2p->tiff_yres *= 2.54F;
	} else if (t2p->tiff_resunit != RESUNIT_INCH
		   && t2p->pdf_centimeters != 0) {
		t2p->tiff_xres *= 2.54F;
		t2p->tiff_yres *= 2.54F;
	}

	t2p_compose_pdf_page(t2p);

	t2p->pdf_transcode = T2P_TRANSCODE_ENCODE;
	if(t2p->pdf_nopassthrough==0){
#ifdef CCITT_SUPPORT
		if(t2p->tiff_compression==COMPRESSION_CCITTFAX4  
			){
			if(TIFFIsTiled(input) || (TIFFNumberOfStrips(input)==1) ){
				t2p->pdf_transcode = T2P_TRANSCODE_RAW;
				t2p->pdf_compression=T2P_COMPRESS_G4;
			}
		}
#endif
#ifdef ZIP_SUPPORT
		if(t2p->tiff_compression== COMPRESSION_ADOBE_DEFLATE 
			|| t2p->tiff_compression==COMPRESSION_DEFLATE){
			if(TIFFIsTiled(input) || (TIFFNumberOfStrips(input)==1) ){
				t2p->pdf_transcode = T2P_TRANSCODE_RAW;
				t2p->pdf_compression=T2P_COMPRESS_ZIP;
			}
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression==COMPRESSION_OJPEG){
			t2p->pdf_transcode = T2P_TRANSCODE_RAW;
			t2p->pdf_compression=T2P_COMPRESS_JPEG;
			t2p_process_ojpeg_tables(t2p, input);
		}
#endif
#ifdef JPEG_SUPPORT
		if(t2p->tiff_compression==COMPRESSION_JPEG){
			t2p->pdf_transcode = T2P_TRANSCODE_RAW;
			t2p->pdf_compression=T2P_COMPRESS_JPEG;
		}
#endif
		(void)0;
	}

	if(t2p->pdf_transcode!=T2P_TRANSCODE_RAW){
		t2p->pdf_compression = t2p->pdf_defaultcompression;
	}

#ifdef JPEG_SUPPORT
	if(t2p->pdf_defaultcompression==T2P_COMPRESS_JPEG){
		if(t2p->pdf_colorspace & T2P_CS_PALETTE){
			t2p->pdf_sample|=T2P_SAMPLE_REALIZE_PALETTE;
			t2p->pdf_colorspace ^= T2P_CS_PALETTE;
			t2p->tiff_pages[t2p->pdf_page].page_extra--;
		}
	}
	if(t2p->tiff_compression==COMPRESSION_JPEG){
		if(t2p->tiff_planar==PLANARCONFIG_SEPARATE){
			TIFFError(
				TIFF2PDF_MODULE, 
				"No support for %s with JPEG compression and separated planar configuration", 
				TIFFFileName(input));
				t2p->t2p_error=T2P_ERR_ERROR;
			return;
		}
	}
#endif
#ifdef OJPEG_SUPPORT
	if(t2p->tiff_compression==COMPRESSION_OJPEG){
		if(t2p->tiff_planar==PLANARCONFIG_SEPARATE){
			TIFFError(
				TIFF2PDF_MODULE, 
				"No support for %s with OJPEG compression and separated planar configuration", 
				TIFFFileName(input));
				t2p->t2p_error=T2P_ERR_ERROR;
			return;
		}
	}
#endif

	if(t2p->pdf_sample & T2P_SAMPLE_REALIZE_PALETTE){
		if(t2p->pdf_colorspace & T2P_CS_CMYK){
			t2p->tiff_samplesperpixel=4;
			t2p->tiff_photometric=PHOTOMETRIC_SEPARATED;
		} else {
			t2p->tiff_samplesperpixel=3;
			t2p->tiff_photometric=PHOTOMETRIC_RGB;
		}
	}

	if (TIFFGetField(input, TIFFTAG_TRANSFERFUNCTION,
			 &(t2p->tiff_transferfunction[0]),
			 &(t2p->tiff_transferfunction[1]),
			 &(t2p->tiff_transferfunction[2]))) {
		if(t2p->tiff_transferfunction[1] !=
		   t2p->tiff_transferfunction[0]) {
			t2p->tiff_transferfunctioncount=3;
		} else {
			t2p->tiff_transferfunctioncount=1;
		}
	} else {
		t2p->tiff_transferfunctioncount=0;
	}
	if(TIFFGetField(input, TIFFTAG_WHITEPOINT, &xfloatp)!=0){
		t2p->tiff_whitechromaticities[0]=xfloatp[0];
		t2p->tiff_whitechromaticities[1]=xfloatp[1];
		if(t2p->pdf_colorspace & T2P_CS_GRAY){
			t2p->pdf_colorspace |= T2P_CS_CALGRAY;
		}
		if(t2p->pdf_colorspace & T2P_CS_RGB){
			t2p->pdf_colorspace |= T2P_CS_CALRGB;
		}
	}
	if(TIFFGetField(input, TIFFTAG_PRIMARYCHROMATICITIES, &xfloatp)!=0){
		t2p->tiff_primarychromaticities[0]=xfloatp[0];
		t2p->tiff_primarychromaticities[1]=xfloatp[1];
		t2p->tiff_primarychromaticities[2]=xfloatp[2];
		t2p->tiff_primarychromaticities[3]=xfloatp[3];
		t2p->tiff_primarychromaticities[4]=xfloatp[4];
		t2p->tiff_primarychromaticities[5]=xfloatp[5];
		if(t2p->pdf_colorspace & T2P_CS_RGB){
			t2p->pdf_colorspace |= T2P_CS_CALRGB;
		}
	}
	if(t2p->pdf_colorspace & T2P_CS_LAB){
		if(TIFFGetField(input, TIFFTAG_WHITEPOINT, &xfloatp) != 0){
			t2p->tiff_whitechromaticities[0]=xfloatp[0];
			t2p->tiff_whitechromaticities[1]=xfloatp[1];
		} else {
			t2p->tiff_whitechromaticities[0]=0.3457F; /* 0.3127F; */
			t2p->tiff_whitechromaticities[1]=0.3585F; /* 0.3290F; */
		}
	}
	if(TIFFGetField(input, 
		TIFFTAG_ICCPROFILE, 
		&(t2p->tiff_iccprofilelength), 
		&(t2p->tiff_iccprofile))!=0){
		t2p->pdf_colorspace |= T2P_CS_ICCBASED;
	} else {
		t2p->tiff_iccprofilelength=0;
		t2p->tiff_iccprofile=NULL;
	}
	
#ifdef CCITT_SUPPORT
	if( t2p->tiff_bitspersample==1 &&
		t2p->tiff_samplesperpixel==1){
		t2p->pdf_compression = T2P_COMPRESS_G4;
	}
#endif


	return;
}

/*
	This function returns the necessary size of a data buffer to contain the raw or 
	uncompressed image data from the input TIFF for a page.
*/

void t2p_read_tiff_size(T2P* t2p, TIFF* input){

	uint64* sbc=NULL;
#if defined(JPEG_SUPPORT) || defined (OJPEG_SUPPORT)
	unsigned char* jpt=NULL;
	tstrip_t i=0;
	tstrip_t stripcount=0;
#endif
        uint64 k = 0;

	if(t2p->pdf_transcode == T2P_TRANSCODE_RAW){
#ifdef CCITT_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_G4 ){
			TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc);
			t2p->tiff_datasize=(tmsize_t)sbc[0];
			return;
		}
#endif
#ifdef ZIP_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_ZIP){
			TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc);
			t2p->tiff_datasize=(tmsize_t)sbc[0];
			return;
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_OJPEG){
			if(!TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc)){
				TIFFError(TIFF2PDF_MODULE, 
					"Input file %s missing field: TIFFTAG_STRIPBYTECOUNTS",
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			stripcount=TIFFNumberOfStrips(input);
			for(i=0;i<stripcount;i++){
				k = checkAdd64(k, sbc[i], t2p);
			}
			if(TIFFGetField(input, TIFFTAG_JPEGIFOFFSET, &(t2p->tiff_dataoffset))){
				if(t2p->tiff_dataoffset != 0){
					if(TIFFGetField(input, TIFFTAG_JPEGIFBYTECOUNT, &(t2p->tiff_datasize))!=0){
						if((uint64)t2p->tiff_datasize < k) {
							TIFFWarning(TIFF2PDF_MODULE, 
								"Input file %s has short JPEG interchange file byte count", 
								TIFFFileName(input));
							t2p->pdf_ojpegiflength=t2p->tiff_datasize;
							k = checkAdd64(k, t2p->tiff_datasize, t2p);
							k = checkAdd64(k, 6, t2p);
							k = checkAdd64(k, stripcount, t2p);
							k = checkAdd64(k, stripcount, t2p);
							t2p->tiff_datasize = (tsize_t) k;
							if ((uint64) t2p->tiff_datasize != k) {
								TIFFError(TIFF2PDF_MODULE, "Integer overflow");
								t2p->t2p_error = T2P_ERR_ERROR;
							}
							return;
						}
						return;
					}else {
						TIFFError(TIFF2PDF_MODULE, 
							"Input file %s missing field: TIFFTAG_JPEGIFBYTECOUNT",
							TIFFFileName(input));
							t2p->t2p_error = T2P_ERR_ERROR;
							return;
					}
				}
			}
			k = checkAdd64(k, stripcount, t2p);
			k = checkAdd64(k, stripcount, t2p);
			k = checkAdd64(k, 2048, t2p);
			t2p->tiff_datasize = (tsize_t) k;
			if ((uint64) t2p->tiff_datasize != k) {
				TIFFError(TIFF2PDF_MODULE, "Integer overflow");
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		}
#endif
#ifdef JPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_JPEG) {
			uint32 count = 0;
			if(TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt) != 0 ){
				if(count > 4){
					k += count;
					k -= 2; /* don't use EOI of header */
				}
			} else {
				k = 2; /* SOI for first strip */
			}
			stripcount=TIFFNumberOfStrips(input);
			if(!TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc)){
				TIFFError(TIFF2PDF_MODULE, 
					"Input file %s missing field: TIFFTAG_STRIPBYTECOUNTS",
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return;
			}
			for(i=0;i<stripcount;i++){
				k = checkAdd64(k, sbc[i], t2p);
				k -=4; /* don't use SOI or EOI of strip */
			}
			k = checkAdd64(k, 2, t2p); /* use EOI of last strip */
			t2p->tiff_datasize = (tsize_t) k;
			if ((uint64) t2p->tiff_datasize != k) {
				TIFFError(TIFF2PDF_MODULE, "Integer overflow");
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		}
#endif
		(void) 0;
	}
	k = checkMultiply64(TIFFScanlineSize(input), t2p->tiff_length, t2p);
	if(t2p->tiff_planar==PLANARCONFIG_SEPARATE){
		k = checkMultiply64(k, t2p->tiff_samplesperpixel, t2p);
	}
	if (k == 0) {
		/* Assume we had overflow inside TIFFScanlineSize */
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	t2p->tiff_datasize = (tsize_t) k;
	if ((uint64) t2p->tiff_datasize != k) {
		TIFFError(TIFF2PDF_MODULE, "Integer overflow");
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	return;
}

/*
	This function returns the necessary size of a data buffer to contain the raw or 
	uncompressed image data from the input TIFF for a tile of a page.
*/

void t2p_read_tiff_size_tile(T2P* t2p, TIFF* input, ttile_t tile){

	uint64* tbc = NULL;
	uint16 edge=0;
#ifdef JPEG_SUPPORT
	unsigned char* jpt;
#endif
        uint64 k;

	edge |= t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile);
	edge |= t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile);
	
	if(t2p->pdf_transcode==T2P_TRANSCODE_RAW){
		if(edge
#if defined(JPEG_SUPPORT) || defined(OJPEG_SUPPORT)
		&& !(t2p->pdf_compression==T2P_COMPRESS_JPEG)
#endif
		){
			t2p->tiff_datasize=TIFFTileSize(input);
			if (t2p->tiff_datasize == 0) {
				/* Assume we had overflow inside TIFFTileSize */
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		} else {
			TIFFGetField(input, TIFFTAG_TILEBYTECOUNTS, &tbc);
			k=tbc[tile];
#ifdef OJPEG_SUPPORT
			if(t2p->tiff_compression==COMPRESSION_OJPEG){
			  	k = checkAdd64(k, 2048, t2p);
			}
#endif
#ifdef JPEG_SUPPORT
			if(t2p->tiff_compression==COMPRESSION_JPEG) {
				uint32 count = 0;
				if(TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt)!=0){
					if(count > 4){
						k = checkAdd64(k, count, t2p);
						k -= 2; /* don't use EOI of header or SOI of tile */
					}
				}
			}
#endif
			t2p->tiff_datasize = (tsize_t) k;
			if ((uint64) t2p->tiff_datasize != k) {
				TIFFError(TIFF2PDF_MODULE, "Integer overflow");
				t2p->t2p_error = T2P_ERR_ERROR;
			}
			return;
		}
	}
	k = TIFFTileSize(input);
	if(t2p->tiff_planar==PLANARCONFIG_SEPARATE){
		k = checkMultiply64(k, t2p->tiff_samplesperpixel, t2p);
	}
	if (k == 0) {
		/* Assume we had overflow inside TIFFTileSize */
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	t2p->tiff_datasize = (tsize_t) k;
	if ((uint64) t2p->tiff_datasize != k) {
		TIFFError(TIFF2PDF_MODULE, "Integer overflow");
		t2p->t2p_error = T2P_ERR_ERROR;
	}

	return;
}

/*
 * This functions returns a non-zero value when the tile is on the right edge
 * and does not have full imaged tile width.
 */

int t2p_tile_is_right_edge(T2P_TILES tiles, ttile_t tile){

	if( ((tile+1) % tiles.tiles_tilecountx == 0) 
		&& (tiles.tiles_edgetilewidth != 0) ){
		return(1);
	} else {
		return(0);
	}
}

/*
 * This functions returns a non-zero value when the tile is on the bottom edge
 * and does not have full imaged tile length.
 */

int t2p_tile_is_bottom_edge(T2P_TILES tiles, ttile_t tile){

	if( ((tile+1) > (tiles.tiles_tilecount-tiles.tiles_tilecountx) )
		&& (tiles.tiles_edgetilelength != 0) ){
		return(1);
	} else {
		return(0);
	}
}

/*
 * This function returns a non-zero value when the tile is a right edge tile
 * or a bottom edge tile.
 */

int t2p_tile_is_edge(T2P_TILES tiles, ttile_t tile){

	return(t2p_tile_is_right_edge(tiles, tile) | t2p_tile_is_bottom_edge(tiles, tile) );
}

/*
	This function returns a non-zero value when the tile is a right edge tile and a bottom 
	edge tile.
*/

int t2p_tile_is_corner_edge(T2P_TILES tiles, ttile_t tile){

	return(t2p_tile_is_right_edge(tiles, tile) & t2p_tile_is_bottom_edge(tiles, tile) );
}


/*
	This function reads the raster image data from the input TIFF for an image and writes 
	the data to the output PDF XObject image dictionary stream.  It returns the amount written 
	or zero on error.
*/

tsize_t t2p_readwrite_pdf_image(T2P* t2p, TIFF* input, TIFF* output){

	tsize_t written=0;
	unsigned char* buffer=NULL;
	unsigned char* samplebuffer=NULL;
	tsize_t bufferoffset=0;
	tsize_t samplebufferoffset=0;
	tsize_t read=0;
	tstrip_t i=0;
	tstrip_t j=0;
	tstrip_t stripcount=0;
	tsize_t stripsize=0;
	tsize_t sepstripcount=0;
	tsize_t sepstripsize=0;
#ifdef OJPEG_SUPPORT
	toff_t inputoffset=0;
	uint16 h_samp=1;
	uint16 v_samp=1;
	uint16 ri=1;
	uint32 rows=0;
#endif
#ifdef JPEG_SUPPORT
	unsigned char* jpt;
	float* xfloatp;
	uint64* sbc;
	unsigned char* stripbuffer;
	tsize_t striplength=0;
	uint32 max_striplength=0;
#endif

	/* Fail if prior error (in particular, can't trust tiff_datasize) */
	if (t2p->t2p_error != T2P_ERR_OK)
		return(0);

	if(t2p->pdf_transcode == T2P_TRANSCODE_RAW){
#ifdef CCITT_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_G4){
			buffer = (unsigned char*)
				_TIFFmalloc(t2p->tiff_datasize);
			if (buffer == NULL) {
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			TIFFReadRawStrip(input, 0, (tdata_t) buffer,
					 t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB){
					/*
					 * make sure is lsb-to-msb
					 * bit-endianness fill order
					 */
					TIFFReverseBits(buffer,
							t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer,
				      t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef ZIP_SUPPORT
		if (t2p->pdf_compression == T2P_COMPRESS_ZIP) {
			buffer = (unsigned char*)
				_TIFFmalloc(t2p->tiff_datasize);
			if(buffer == NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
                        memset(buffer, 0, t2p->tiff_datasize);
			TIFFReadRawStrip(input, 0, (tdata_t) buffer,
					 t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB) {
					TIFFReverseBits(buffer,
							t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer,
				      t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_OJPEG) {

			if(t2p->tiff_dataoffset != 0) {
				buffer = (unsigned char*)
					_TIFFmalloc(t2p->tiff_datasize);
				if(buffer == NULL) {
					TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
						(unsigned long) t2p->tiff_datasize, 
						TIFFFileName(input));
					t2p->t2p_error = T2P_ERR_ERROR;
					return(0);
				}
                                memset(buffer, 0, t2p->tiff_datasize);
				if(t2p->pdf_ojpegiflength==0){
					inputoffset=t2pSeekFile(input, 0,
								 SEEK_CUR);
					t2pSeekFile(input,
						     t2p->tiff_dataoffset,
						     SEEK_SET);
					t2pReadFile(input, (tdata_t) buffer,
						     t2p->tiff_datasize);
					t2pSeekFile(input, inputoffset,
						     SEEK_SET);
					t2pWriteFile(output, (tdata_t) buffer,
						      t2p->tiff_datasize);
					_TIFFfree(buffer);
					return(t2p->tiff_datasize);
				} else {
					inputoffset=t2pSeekFile(input, 0,
								 SEEK_CUR);
					t2pSeekFile(input,
						     t2p->tiff_dataoffset,
						     SEEK_SET);
					bufferoffset = t2pReadFile(input,
						(tdata_t) buffer,
						t2p->pdf_ojpegiflength);
					t2p->pdf_ojpegiflength = 0;
					t2pSeekFile(input, inputoffset,
						     SEEK_SET);
					TIFFGetField(input,
						     TIFFTAG_YCBCRSUBSAMPLING,
						     &h_samp, &v_samp);
					buffer[bufferoffset++]= 0xff;
					buffer[bufferoffset++]= 0xdd;
					buffer[bufferoffset++]= 0x00;
					buffer[bufferoffset++]= 0x04;
					h_samp*=8;
					v_samp*=8;
					ri=(t2p->tiff_width+h_samp-1) / h_samp;
					TIFFGetField(input,
						     TIFFTAG_ROWSPERSTRIP,
						     &rows);
					ri*=(rows+v_samp-1)/v_samp;
					buffer[bufferoffset++]= (ri>>8) & 0xff;
					buffer[bufferoffset++]= ri & 0xff;
					stripcount=TIFFNumberOfStrips(input);
					for(i=0;i<stripcount;i++){
						if(i != 0 ){ 
							buffer[bufferoffset++]=0xff;
							buffer[bufferoffset++]=(0xd0 | ((i-1)%8));
						}
						bufferoffset+=TIFFReadRawStrip(input, 
							i, 
							(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
							-1);
					}
					t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
					_TIFFfree(buffer);
					return(bufferoffset);
				}
			} else {
				if(! t2p->pdf_ojpegdata){
					TIFFError(TIFF2PDF_MODULE, 
				"No support for OJPEG image %s with bad tables", 
						TIFFFileName(input));
					t2p->t2p_error = T2P_ERR_ERROR;
					return(0);
				}
				buffer = (unsigned char*)
					_TIFFmalloc(t2p->tiff_datasize);
				if(buffer==NULL){
					TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
						(unsigned long) t2p->tiff_datasize, 
						TIFFFileName(input));
					t2p->t2p_error = T2P_ERR_ERROR;
					return(0);
				}
                                memset(buffer, 0, t2p->tiff_datasize);
				_TIFFmemcpy(buffer, t2p->pdf_ojpegdata, t2p->pdf_ojpegdatalength);
				bufferoffset=t2p->pdf_ojpegdatalength;
				stripcount=TIFFNumberOfStrips(input);
				for(i=0;i<stripcount;i++){
					if(i != 0){
						buffer[bufferoffset++]=0xff;
						buffer[bufferoffset++]=(0xd0 | ((i-1)%8));
					}
					bufferoffset+=TIFFReadRawStrip(input, 
						i, 
						(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
						-1);
				}
				if( ! ( (buffer[bufferoffset-1]==0xd9) && (buffer[bufferoffset-2]==0xff) ) ){
						buffer[bufferoffset++]=0xff;
						buffer[bufferoffset++]=0xd9;
				}
				t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
				_TIFFfree(buffer);
				return(bufferoffset);
				TIFFError(TIFF2PDF_MODULE, 
	"No support for OJPEG image %s with no JPEG File Interchange offset", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			return(t2p->tiff_datasize);
		}
#endif
#ifdef JPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_JPEG) {
			uint32 count = 0;
			buffer = (unsigned char*)
				_TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
                        memset(buffer, 0, t2p->tiff_datasize);
			if (TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt) != 0) {
				if(count > 4) {
					_TIFFmemcpy(buffer, jpt, count);
					bufferoffset += count - 2;
				}
			}
			stripcount=TIFFNumberOfStrips(input);
			TIFFGetField(input, TIFFTAG_STRIPBYTECOUNTS, &sbc);
			for(i=0;i<stripcount;i++){
				if(sbc[i]>max_striplength) max_striplength=sbc[i];
			}
			stripbuffer = (unsigned char*)
				_TIFFmalloc(max_striplength);
			if(stripbuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %u bytes of memory for t2p_readwrite_pdf_image, %s", 
					max_striplength, 
					TIFFFileName(input));
				_TIFFfree(buffer);
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			for(i=0;i<stripcount;i++){
				striplength=TIFFReadRawStrip(input, i, (tdata_t) stripbuffer, -1);
				if(!t2p_process_jpeg_strip(
					stripbuffer, 
					&striplength, 
					buffer, 
					&bufferoffset, 
					i, 
					t2p->tiff_length)){
						TIFFError(TIFF2PDF_MODULE, 
				"Can't process JPEG data in input file %s", 
							TIFFFileName(input));
						_TIFFfree(samplebuffer);
						_TIFFfree(buffer);
						t2p->t2p_error = T2P_ERR_ERROR;
						return(0);
				}
			}
			buffer[bufferoffset++]=0xff; 
			buffer[bufferoffset++]=0xd9;
			t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
			_TIFFfree(stripbuffer);
			_TIFFfree(buffer);
			return(bufferoffset);
		}
#endif
		(void)0;
	}

	if(t2p->pdf_sample==T2P_SAMPLE_NOTHING){
		buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
		if(buffer==NULL){
			TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
				(unsigned long) t2p->tiff_datasize, 
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}
                memset(buffer, 0, t2p->tiff_datasize);
		stripsize=TIFFStripSize(input);
		stripcount=TIFFNumberOfStrips(input);
		for(i=0;i<stripcount;i++){
			read = 
				TIFFReadEncodedStrip(input, 
				i, 
				(tdata_t) &buffer[bufferoffset], 
				stripsize);
			if(read==-1){
				TIFFError(TIFF2PDF_MODULE, 
					"Error on decoding strip %u of %s", 
					i, 
					TIFFFileName(input));
				_TIFFfree(buffer);
				t2p->t2p_error=T2P_ERR_ERROR;
				return(0);
			}
			bufferoffset+=read;
		}
	} else {
		if(t2p->pdf_sample & T2P_SAMPLE_PLANAR_SEPARATE_TO_CONTIG){
		
			sepstripsize=TIFFStripSize(input);
			sepstripcount=TIFFNumberOfStrips(input);
		
			stripsize=sepstripsize*t2p->tiff_samplesperpixel;
			stripcount=sepstripcount/t2p->tiff_samplesperpixel;
			
			buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
                        memset(buffer, 0, t2p->tiff_datasize);
			samplebuffer = (unsigned char*) _TIFFmalloc(stripsize);
			if(samplebuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			for(i=0;i<stripcount;i++){
				samplebufferoffset=0;
				for(j=0;j<t2p->tiff_samplesperpixel;j++){
					read = 
						TIFFReadEncodedStrip(input, 
							i + j*stripcount, 
							(tdata_t) &(samplebuffer[samplebufferoffset]), 
							sepstripsize);
					if(read==-1){
						TIFFError(TIFF2PDF_MODULE, 
					"Error on decoding strip %u of %s", 
							i + j*stripcount, 
							TIFFFileName(input));
							_TIFFfree(buffer);
						t2p->t2p_error=T2P_ERR_ERROR;
						return(0);
					}
					samplebufferoffset+=read;
				}
				t2p_sample_planar_separate_to_contig(
					t2p,
					&(buffer[bufferoffset]),
					samplebuffer, 
					samplebufferoffset); 
				bufferoffset+=samplebufferoffset;
			}
			_TIFFfree(samplebuffer);
			goto dataready;
		}

		buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
		if(buffer==NULL){
			TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
				(unsigned long) t2p->tiff_datasize, 
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}
                memset(buffer, 0, t2p->tiff_datasize);
		stripsize=TIFFStripSize(input);
		stripcount=TIFFNumberOfStrips(input);
		for(i=0;i<stripcount;i++){
			read = 
				TIFFReadEncodedStrip(input, 
				i, 
				(tdata_t) &buffer[bufferoffset], 
				stripsize);
			if(read==-1){
				TIFFError(TIFF2PDF_MODULE, 
					"Error on decoding strip %u of %s", 
					i, 
					TIFFFileName(input));
				_TIFFfree(samplebuffer);
				_TIFFfree(buffer);
				t2p->t2p_error=T2P_ERR_ERROR;
				return(0);
			}
			bufferoffset+=read;
		}

		if(t2p->pdf_sample & T2P_SAMPLE_REALIZE_PALETTE){
			// FIXME: overflow?
			samplebuffer=(unsigned char*)_TIFFrealloc( 
				(tdata_t) buffer, 
				t2p->tiff_datasize * t2p->tiff_samplesperpixel);
			if(samplebuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
			  _TIFFfree(buffer);
			} else {
				buffer=samplebuffer;
				t2p->tiff_datasize *= t2p->tiff_samplesperpixel;
			}
			t2p_sample_realize_palette(t2p, buffer);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgba_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_width*t2p->tiff_length);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBAA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgbaa_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_width*t2p->tiff_length);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_YCBCR_TO_RGB){
			samplebuffer=(unsigned char*)_TIFFrealloc(
				(tdata_t)buffer, 
				t2p->tiff_width*t2p->tiff_length*4);
			if(samplebuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't allocate %lu bytes of memory for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				_TIFFfree(buffer);
				return(0);
			} else {
				buffer=samplebuffer;
			}
			if(!TIFFReadRGBAImageOriented(
				input, 
				t2p->tiff_width, 
				t2p->tiff_length, 
				(uint32*)buffer, 
				ORIENTATION_TOPLEFT,
				0)){
				TIFFError(TIFF2PDF_MODULE, 
	"Can't use TIFFReadRGBAImageOriented to extract RGB image from %s", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			t2p->tiff_datasize=t2p_sample_abgr_to_rgb(
				(tdata_t) buffer, 
				t2p->tiff_width*t2p->tiff_length);

		}

		if(t2p->pdf_sample & T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED){
			t2p->tiff_datasize=t2p_sample_lab_signed_to_unsigned(
				(tdata_t)buffer, 
				t2p->tiff_width*t2p->tiff_length);
		}
	}

dataready:

	t2p_disable(output);
	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, t2p->tiff_photometric);
	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, t2p->tiff_bitspersample);
	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, t2p->tiff_samplesperpixel);
	TIFFSetField(output, TIFFTAG_IMAGEWIDTH, t2p->tiff_width);
	TIFFSetField(output, TIFFTAG_IMAGELENGTH, t2p->tiff_length);
	TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, t2p->tiff_length);
	TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(output, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);

	switch(t2p->pdf_compression){
	case T2P_COMPRESS_NONE:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		break;
#ifdef CCITT_SUPPORT
	case T2P_COMPRESS_G4:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
		break;
#endif
#ifdef JPEG_SUPPORT
	case T2P_COMPRESS_JPEG:
		if(t2p->tiff_photometric==PHOTOMETRIC_YCBCR) {
			uint16 hor = 0, ver = 0;
			if (TIFFGetField(input, TIFFTAG_YCBCRSUBSAMPLING, &hor, &ver) !=0 ) {
				if(hor != 0 && ver != 0){
					TIFFSetField(output, TIFFTAG_YCBCRSUBSAMPLING, hor, ver);
				}
			}
			if(TIFFGetField(input, TIFFTAG_REFERENCEBLACKWHITE, &xfloatp)!=0){
				TIFFSetField(output, TIFFTAG_REFERENCEBLACKWHITE, xfloatp);
			}
		}
		if(TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_JPEG)==0){
			TIFFError(TIFF2PDF_MODULE, 
		"Unable to use JPEG compression for input %s and output %s", 
				TIFFFileName(input),
				TIFFFileName(output));
			_TIFFfree(buffer);
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}
		TIFFSetField(output, TIFFTAG_JPEGTABLESMODE, 0);

		if(t2p->pdf_colorspace & (T2P_CS_RGB | T2P_CS_LAB)){
			TIFFSetField(output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
			if(t2p->tiff_photometric != PHOTOMETRIC_YCBCR){
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
			} else {
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW);
			}
		}
		if(t2p->pdf_colorspace & T2P_CS_GRAY){
			(void)0;
		}
		if(t2p->pdf_colorspace & T2P_CS_CMYK){
			(void)0;
		}
		if(t2p->pdf_defaultcompressionquality != 0){
			TIFFSetField(output, 
				TIFFTAG_JPEGQUALITY, 
				t2p->pdf_defaultcompressionquality);
		}
	
		break;
#endif
#ifdef ZIP_SUPPORT
	case T2P_COMPRESS_ZIP:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
		if(t2p->pdf_defaultcompressionquality%100 != 0){
			TIFFSetField(output, 
				TIFFTAG_PREDICTOR, 
				t2p->pdf_defaultcompressionquality % 100);
		}
		if(t2p->pdf_defaultcompressionquality/100 != 0){
			TIFFSetField(output, 
				TIFFTAG_ZIPQUALITY, 
				(t2p->pdf_defaultcompressionquality / 100));
		}
		break;
#endif
	default:
		break;
	}

	t2p_enable(output);
	t2p->outputwritten = 0;
#ifdef JPEG_SUPPORT
	if(t2p->pdf_compression == T2P_COMPRESS_JPEG
	   && t2p->tiff_photometric == PHOTOMETRIC_YCBCR){
		bufferoffset = TIFFWriteEncodedStrip(output, (tstrip_t)0,
						     buffer,
						     stripsize * stripcount); 
	} else
#endif
        {
		bufferoffset = TIFFWriteEncodedStrip(output, (tstrip_t)0,
						     buffer,
						     t2p->tiff_datasize); 
	}
	if (buffer != NULL) {
		_TIFFfree(buffer);
		buffer=NULL;
	}

	if (bufferoffset == (tsize_t)-1) {
		TIFFError(TIFF2PDF_MODULE, 
			  "Error writing encoded strip to output PDF %s", 
			  TIFFFileName(output));
		t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	
	written = t2p->outputwritten;
	return(written);
}

/*
 * This function reads the raster image data from the input TIFF for an image
 * tile and writes the data to the output PDF XObject image dictionary stream
 * for the tile.  It returns the amount written or zero on error.
 */

tsize_t t2p_readwrite_pdf_image_tile(T2P* t2p, TIFF* input, TIFF* output, ttile_t tile){

	uint16 edge=0;
	tsize_t written=0;
	unsigned char* buffer=NULL;
	tsize_t bufferoffset=0;
	unsigned char* samplebuffer=NULL;
	tsize_t samplebufferoffset=0;
	tsize_t read=0;
	uint16 i=0;
	ttile_t tilecount=0;
	tsize_t tilesize=0;
	ttile_t septilecount=0;
	tsize_t septilesize=0;
#ifdef JPEG_SUPPORT
	unsigned char* jpt;
	float* xfloatp;
	uint32 xuint32=0;
#endif

	/* Fail if prior error (in particular, can't trust tiff_datasize) */
	if (t2p->t2p_error != T2P_ERR_OK)
		return(0);

	edge |= t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile);
	edge |= t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile);

	if( (t2p->pdf_transcode == T2P_TRANSCODE_RAW) && ((edge == 0)
#if defined(JPEG_SUPPORT) || defined(OJPEG_SUPPORT)
		|| (t2p->pdf_compression == T2P_COMPRESS_JPEG)
#endif
	)
	){
#ifdef CCITT_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_G4){
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			TIFFReadRawTile(input, tile, (tdata_t) buffer, t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB){
					TIFFReverseBits(buffer, t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer, t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef ZIP_SUPPORT
		if(t2p->pdf_compression == T2P_COMPRESS_ZIP){
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			TIFFReadRawTile(input, tile, (tdata_t) buffer, t2p->tiff_datasize);
			if (t2p->tiff_fillorder==FILLORDER_LSB2MSB){
					TIFFReverseBits(buffer, t2p->tiff_datasize);
			}
			t2pWriteFile(output, (tdata_t) buffer, t2p->tiff_datasize);
			_TIFFfree(buffer);
			return(t2p->tiff_datasize);
		}
#endif
#ifdef OJPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_OJPEG){
			if(! t2p->pdf_ojpegdata){
				TIFFError(TIFF2PDF_MODULE, 
					"No support for OJPEG image %s with "
                                        "bad tables", 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			buffer=(unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			_TIFFmemcpy(buffer, t2p->pdf_ojpegdata, t2p->pdf_ojpegdatalength);
			if(edge!=0){
				if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile)){
					buffer[7]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength >> 8) & 0xff;
					buffer[8]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength ) & 0xff;
				}
				if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile)){
					buffer[9]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth >> 8) & 0xff;
					buffer[10]=
						(t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth ) & 0xff;
				}
			}
			bufferoffset=t2p->pdf_ojpegdatalength;
			bufferoffset+=TIFFReadRawTile(input, 
					tile, 
					(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
					-1);
			((unsigned char*)buffer)[bufferoffset++]=0xff;
			((unsigned char*)buffer)[bufferoffset++]=0xd9;
			t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
			_TIFFfree(buffer);
			return(bufferoffset);
		}
#endif
#ifdef JPEG_SUPPORT
		if(t2p->tiff_compression == COMPRESSION_JPEG){
			unsigned char table_end[2];
			uint32 count = 0;
			buffer= (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			if(TIFFGetField(input, TIFFTAG_JPEGTABLES, &count, &jpt) != 0) {
				if (count > 0) {
					_TIFFmemcpy(buffer, jpt, count);
					bufferoffset += count - 2;
					table_end[0] = buffer[bufferoffset-2];
					table_end[1] = buffer[bufferoffset-1];
				}
				if (count > 0) {
					xuint32 = bufferoffset;
					bufferoffset += TIFFReadRawTile(
						input, 
						tile, 
						(tdata_t) &(((unsigned char*)buffer)[bufferoffset-2]), 
						-1);
						buffer[xuint32-2]=table_end[0];
						buffer[xuint32-1]=table_end[1];
				} else {
					bufferoffset += TIFFReadRawTile(
						input, 
						tile, 
						(tdata_t) &(((unsigned char*)buffer)[bufferoffset]), 
						-1);
				}
			}
			t2pWriteFile(output, (tdata_t) buffer, bufferoffset);
			_TIFFfree(buffer);
			return(bufferoffset);
		}
#endif
		(void)0;
	}

	if(t2p->pdf_sample==T2P_SAMPLE_NOTHING){
		buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
		if(buffer==NULL){
			TIFFError(TIFF2PDF_MODULE, 
				"Can't allocate %lu bytes of memory for "
                                "t2p_readwrite_pdf_image_tile, %s", 
				(unsigned long) t2p->tiff_datasize, 
				TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}

		read = TIFFReadEncodedTile(
			input, 
			tile, 
			(tdata_t) &buffer[bufferoffset], 
			t2p->tiff_datasize);
		if(read==-1){
			TIFFError(TIFF2PDF_MODULE, 
				"Error on decoding tile %u of %s", 
				tile, 
				TIFFFileName(input));
			_TIFFfree(buffer);
			t2p->t2p_error=T2P_ERR_ERROR;
			return(0);
		}

	} else {

		if(t2p->pdf_sample == T2P_SAMPLE_PLANAR_SEPARATE_TO_CONTIG){
			septilesize=TIFFTileSize(input);
			septilecount=TIFFNumberOfTiles(input);
			tilesize=septilesize*t2p->tiff_samplesperpixel;
			tilecount=septilecount/t2p->tiff_samplesperpixel;
			buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			samplebuffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(samplebuffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			samplebufferoffset=0;
			for(i=0;i<t2p->tiff_samplesperpixel;i++){
				read = 
					TIFFReadEncodedTile(input, 
						tile + i*tilecount, 
						(tdata_t) &(samplebuffer[samplebufferoffset]), 
						septilesize);
				if(read==-1){
					TIFFError(TIFF2PDF_MODULE, 
						"Error on decoding tile %u of %s", 
						tile + i*tilecount, 
						TIFFFileName(input));
						_TIFFfree(samplebuffer);
						_TIFFfree(buffer);
					t2p->t2p_error=T2P_ERR_ERROR;
					return(0);
				}
				samplebufferoffset+=read;
			}
			t2p_sample_planar_separate_to_contig(
				t2p,
				&(buffer[bufferoffset]),
				samplebuffer, 
				samplebufferoffset); 
			bufferoffset+=samplebufferoffset;
			_TIFFfree(samplebuffer);
		}

		if(buffer==NULL){
			buffer = (unsigned char*) _TIFFmalloc(t2p->tiff_datasize);
			if(buffer==NULL){
				TIFFError(TIFF2PDF_MODULE, 
					"Can't allocate %lu bytes of memory "
                                        "for t2p_readwrite_pdf_image_tile, %s", 
					(unsigned long) t2p->tiff_datasize, 
					TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
			}
			read = TIFFReadEncodedTile(
				input, 
				tile, 
				(tdata_t) &buffer[bufferoffset], 
				t2p->tiff_datasize);
			if(read==-1){
				TIFFError(TIFF2PDF_MODULE, 
					"Error on decoding tile %u of %s", 
					tile, 
					TIFFFileName(input));
				_TIFFfree(buffer);
				t2p->t2p_error=T2P_ERR_ERROR;
				return(0);
			}
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgba_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_RGBAA_TO_RGB){
			t2p->tiff_datasize=t2p_sample_rgbaa_to_rgb(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_YCBCR_TO_RGB){
			TIFFError(TIFF2PDF_MODULE, 
				"No support for YCbCr to RGB in tile for %s", 
				TIFFFileName(input));
			_TIFFfree(buffer);
			t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}

		if(t2p->pdf_sample & T2P_SAMPLE_LAB_SIGNED_TO_UNSIGNED){
			t2p->tiff_datasize=t2p_sample_lab_signed_to_unsigned(
				(tdata_t)buffer, 
				t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth
				*t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}
	}

	if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile) != 0){
		t2p_tile_collapse_left(
			buffer, 
			TIFFTileRowSize(input),
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth,
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
	}


	t2p_disable(output);
	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, t2p->tiff_photometric);
	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, t2p->tiff_bitspersample);
	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, t2p->tiff_samplesperpixel);
	if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile) == 0){
		TIFFSetField(
			output, 
			TIFFTAG_IMAGEWIDTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth);
	} else {
		TIFFSetField(
			output, 
			TIFFTAG_IMAGEWIDTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth);
	}
	if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile) == 0){
		TIFFSetField(
			output, 
			TIFFTAG_IMAGELENGTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		TIFFSetField(
			output, 
			TIFFTAG_ROWSPERSTRIP, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
	} else {
		TIFFSetField(
			output, 
			TIFFTAG_IMAGELENGTH, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
		TIFFSetField(
			output, 
			TIFFTAG_ROWSPERSTRIP, 
			t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
	}
	TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(output, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);

	switch(t2p->pdf_compression){
	case T2P_COMPRESS_NONE:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		break;
#ifdef CCITT_SUPPORT
	case T2P_COMPRESS_G4:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
		break;
#endif
#ifdef JPEG_SUPPORT
	case T2P_COMPRESS_JPEG:
		if (t2p->tiff_photometric==PHOTOMETRIC_YCBCR) {
			uint16 hor = 0, ver = 0;
			if (TIFFGetField(input, TIFFTAG_YCBCRSUBSAMPLING, &hor, &ver)!=0) {
				if (hor != 0 && ver != 0) {
					TIFFSetField(output, TIFFTAG_YCBCRSUBSAMPLING, hor, ver);
				}
			}
			if(TIFFGetField(input, TIFFTAG_REFERENCEBLACKWHITE, &xfloatp)!=0){
				TIFFSetField(output, TIFFTAG_REFERENCEBLACKWHITE, xfloatp);
			}
		}
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
		TIFFSetField(output, TIFFTAG_JPEGTABLESMODE, 0); /* JPEGTABLESMODE_NONE */
		if(t2p->pdf_colorspace & (T2P_CS_RGB | T2P_CS_LAB)){
			TIFFSetField(output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
			if(t2p->tiff_photometric != PHOTOMETRIC_YCBCR){
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
			} else {
				TIFFSetField(output, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW);
			}
		}
		if(t2p->pdf_colorspace & T2P_CS_GRAY){
			(void)0;
		}
		if(t2p->pdf_colorspace & T2P_CS_CMYK){
			(void)0;
		}
		if(t2p->pdf_defaultcompressionquality != 0){
			TIFFSetField(output, 
				TIFFTAG_JPEGQUALITY, 
				t2p->pdf_defaultcompressionquality);
		}
		break;
#endif
#ifdef ZIP_SUPPORT
	case T2P_COMPRESS_ZIP:
		TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
		if(t2p->pdf_defaultcompressionquality%100 != 0){
			TIFFSetField(output, 
				TIFFTAG_PREDICTOR, 
				t2p->pdf_defaultcompressionquality % 100);
		}
		if(t2p->pdf_defaultcompressionquality/100 != 0){
			TIFFSetField(output, 
				TIFFTAG_ZIPQUALITY, 
				(t2p->pdf_defaultcompressionquality / 100));
		}
		break;
#endif
	default:
		break;
	}

	t2p_enable(output);
	t2p->outputwritten = 0;
	bufferoffset = TIFFWriteEncodedStrip(output, (tstrip_t) 0, buffer,
					     TIFFStripSize(output)); 
	if (buffer != NULL) {
		_TIFFfree(buffer);
		buffer = NULL;
	}
	if (bufferoffset == -1) {
		TIFFError(TIFF2PDF_MODULE, 
			  "Error writing encoded tile to output PDF %s", 
			  TIFFFileName(output));
		t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	
	written = t2p->outputwritten;
	
	return(written);
}

#ifdef OJPEG_SUPPORT
int t2p_process_ojpeg_tables(T2P* t2p, TIFF* input){
	uint16 proc=0;
	void* q;
	uint32 q_length=0;
	void* dc;
	uint32 dc_length=0;
	void* ac;
	uint32 ac_length=0;
	uint16* lp;
	uint16* pt;
	uint16 h_samp=1;
	uint16 v_samp=1;
	unsigned char* ojpegdata;
	uint16 table_count;
	uint32 offset_table;
	uint32 offset_ms_l;
	uint32 code_count;
	uint32 i=0;
	uint32 dest=0;
	uint16 ri=0;
	uint32 rows=0;
	
	if(!TIFFGetField(input, TIFFTAG_JPEGPROC, &proc)){
		TIFFError(TIFF2PDF_MODULE, 
			"Missing JPEGProc field in OJPEG image %s", 
			TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	if(proc!=JPEGPROC_BASELINE && proc!=JPEGPROC_LOSSLESS){
		TIFFError(TIFF2PDF_MODULE, 
			"Bad JPEGProc field in OJPEG image %s", 
			TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	if(!TIFFGetField(input, TIFFTAG_JPEGQTABLES, &q_length, &q)){
		TIFFError(TIFF2PDF_MODULE, 
			"Missing JPEGQTables field in OJPEG image %s", 
			TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	if(q_length < (64U * t2p->tiff_samplesperpixel)){
		TIFFError(TIFF2PDF_MODULE, 
			"Bad JPEGQTables field in OJPEG image %s", 
			TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	} 
	if(!TIFFGetField(input, TIFFTAG_JPEGDCTABLES, &dc_length, &dc)){
		TIFFError(TIFF2PDF_MODULE, 
			"Missing JPEGDCTables field in OJPEG image %s", 
			TIFFFileName(input));
			t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	if(proc==JPEGPROC_BASELINE){
		if(!TIFFGetField(input, TIFFTAG_JPEGACTABLES, &ac_length, &ac)){
			TIFFError(TIFF2PDF_MODULE, 
				"Missing JPEGACTables field in OJPEG image %s", 
				TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}
	} else {
		if(!TIFFGetField(input, TIFFTAG_JPEGLOSSLESSPREDICTORS, &lp)){
			TIFFError(TIFF2PDF_MODULE, 
				"Missing JPEGLosslessPredictors field in OJPEG image %s", 
				TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
				return(0);
		}
		if(!TIFFGetField(input, TIFFTAG_JPEGPOINTTRANSFORM, &pt)){
			TIFFError(TIFF2PDF_MODULE, 
				"Missing JPEGPointTransform field in OJPEG image %s", 
				TIFFFileName(input));
				t2p->t2p_error = T2P_ERR_ERROR;
			return(0);
		}
	}
	if(!TIFFGetField(input, TIFFTAG_YCBCRSUBSAMPLING, &h_samp, &v_samp)){
		h_samp=1;
		v_samp=1;
	}
	if(t2p->pdf_ojpegdata != NULL){
		_TIFFfree(t2p->pdf_ojpegdata);
		t2p->pdf_ojpegdata=NULL;
	} 
	t2p->pdf_ojpegdata = _TIFFmalloc(2048);
	if(t2p->pdf_ojpegdata == NULL){
		TIFFError(TIFF2PDF_MODULE, 
			"Can't allocate %u bytes of memory for t2p_process_ojpeg_tables, %s", 
			2048, 
			TIFFFileName(input));
		t2p->t2p_error = T2P_ERR_ERROR;
		return(0);
	}
	_TIFFmemset(t2p->pdf_ojpegdata, 0x00, 2048);
	t2p->pdf_ojpegdatalength = 0;
	table_count=t2p->tiff_samplesperpixel;
	if(proc==JPEGPROC_BASELINE){
		if(table_count>2) table_count=2;
	}
	ojpegdata=(unsigned char*)t2p->pdf_ojpegdata;
	ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
	ojpegdata[t2p->pdf_ojpegdatalength++]=0xd8;
	ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
	if(proc==JPEGPROC_BASELINE){
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xc0;
	} else {
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xc3;
	}
	ojpegdata[t2p->pdf_ojpegdatalength++]=0x00;
	ojpegdata[t2p->pdf_ojpegdatalength++]=(8 + 3*t2p->tiff_samplesperpixel);
	ojpegdata[t2p->pdf_ojpegdatalength++]=(t2p->tiff_bitspersample & 0xff);
	if(TIFFIsTiled(input)){
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength >> 8) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength ) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth >> 8) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth ) & 0xff;
	} else {
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_length >> 8) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_length ) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_width >> 8) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=
			(t2p->tiff_width ) & 0xff;
	}
	ojpegdata[t2p->pdf_ojpegdatalength++]=(t2p->tiff_samplesperpixel & 0xff);
	for(i=0;i<t2p->tiff_samplesperpixel;i++){
		ojpegdata[t2p->pdf_ojpegdatalength++]=i;
		if(i==0){
			ojpegdata[t2p->pdf_ojpegdatalength] |= h_samp<<4 & 0xf0;;
			ojpegdata[t2p->pdf_ojpegdatalength++] |= v_samp & 0x0f;
		} else {
				ojpegdata[t2p->pdf_ojpegdatalength++]= 0x11;
		}
		ojpegdata[t2p->pdf_ojpegdatalength++]=i;
	}
	for(dest=0;dest<t2p->tiff_samplesperpixel;dest++){
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xdb;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0x00;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0x43;
		ojpegdata[t2p->pdf_ojpegdatalength++]=dest;
		_TIFFmemcpy( &(ojpegdata[t2p->pdf_ojpegdatalength++]), 
			&(((unsigned char*)q)[64*dest]), 64);
		t2p->pdf_ojpegdatalength+=64;
	}
	offset_table=0;
	for(dest=0;dest<table_count;dest++){
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xc4;
		offset_ms_l=t2p->pdf_ojpegdatalength;
		t2p->pdf_ojpegdatalength+=2;
		ojpegdata[t2p->pdf_ojpegdatalength++]=dest & 0x0f;
		_TIFFmemcpy( &(ojpegdata[t2p->pdf_ojpegdatalength]), 
			&(((unsigned char*)dc)[offset_table]), 16);
		code_count=0;
		offset_table+=16;
		for(i=0;i<16;i++){
			code_count+=ojpegdata[t2p->pdf_ojpegdatalength++];
		}
		ojpegdata[offset_ms_l]=((19+code_count)>>8) & 0xff;
		ojpegdata[offset_ms_l+1]=(19+code_count) & 0xff;
		_TIFFmemcpy( &(ojpegdata[t2p->pdf_ojpegdatalength]), 
			&(((unsigned char*)dc)[offset_table]), code_count);
		offset_table+=code_count;
		t2p->pdf_ojpegdatalength+=code_count;
	}
	if(proc==JPEGPROC_BASELINE){
	offset_table=0;
		for(dest=0;dest<table_count;dest++){
			ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
			ojpegdata[t2p->pdf_ojpegdatalength++]=0xc4;
			offset_ms_l=t2p->pdf_ojpegdatalength;
			t2p->pdf_ojpegdatalength+=2;
			ojpegdata[t2p->pdf_ojpegdatalength] |= 0x10;
			ojpegdata[t2p->pdf_ojpegdatalength++] |=dest & 0x0f;
			_TIFFmemcpy( &(ojpegdata[t2p->pdf_ojpegdatalength]), 
				&(((unsigned char*)ac)[offset_table]), 16);
			code_count=0;
			offset_table+=16;
			for(i=0;i<16;i++){
				code_count+=ojpegdata[t2p->pdf_ojpegdatalength++];
			}	
			ojpegdata[offset_ms_l]=((19+code_count)>>8) & 0xff;
			ojpegdata[offset_ms_l+1]=(19+code_count) & 0xff;
			_TIFFmemcpy( &(ojpegdata[t2p->pdf_ojpegdatalength]), 
				&(((unsigned char*)ac)[offset_table]), code_count);
			offset_table+=code_count;
			t2p->pdf_ojpegdatalength+=code_count;
		}
	}
	if(TIFFNumberOfStrips(input)>1){
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0xdd;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0x00;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0x04;
		h_samp*=8;
		v_samp*=8;
		ri=(t2p->tiff_width+h_samp-1) / h_samp;
		TIFFGetField(input, TIFFTAG_ROWSPERSTRIP, &rows);
		ri*=(rows+v_samp-1)/v_samp;
		ojpegdata[t2p->pdf_ojpegdatalength++]= (ri>>8) & 0xff;
		ojpegdata[t2p->pdf_ojpegdatalength++]= ri & 0xff;
	}
	ojpegdata[t2p->pdf_ojpegdatalength++]=0xff;
	ojpegdata[t2p->pdf_ojpegdatalength++]=0xda;
	ojpegdata[t2p->pdf_ojpegdatalength++]=0x00;
	ojpegdata[t2p->pdf_ojpegdatalength++]=(6 + 2*t2p->tiff_samplesperpixel);
	ojpegdata[t2p->pdf_ojpegdatalength++]=t2p->tiff_samplesperpixel & 0xff;
	for(i=0;i<t2p->tiff_samplesperpixel;i++){
		ojpegdata[t2p->pdf_ojpegdatalength++]= i & 0xff;
		if(proc==JPEGPROC_BASELINE){
			ojpegdata[t2p->pdf_ojpegdatalength] |= 
				( ( (i>(table_count-1U)) ? (table_count-1U) : i) << 4U) & 0xf0;
			ojpegdata[t2p->pdf_ojpegdatalength++] |= 
				( (i>(table_count-1U)) ? (table_count-1U) : i) & 0x0f;
		} else {
			ojpegdata[t2p->pdf_ojpegdatalength++] =  (i << 4) & 0xf0;
		}
	}
	if(proc==JPEGPROC_BASELINE){
		t2p->pdf_ojpegdatalength++;
		ojpegdata[t2p->pdf_ojpegdatalength++]=0x3f;
		t2p->pdf_ojpegdatalength++;
	} else {
		ojpegdata[t2p->pdf_ojpegdatalength++]= (lp[0] & 0xff);
		t2p->pdf_ojpegdatalength++;
		ojpegdata[t2p->pdf_ojpegdatalength++]= (pt[0] & 0x0f);
	}

	return(1);
}
#endif

#ifdef JPEG_SUPPORT
int t2p_process_jpeg_strip(
	unsigned char* strip, 
	tsize_t* striplength, 
	unsigned char* buffer, 
	tsize_t* bufferoffset, 
	tstrip_t no, 
	uint32 height){

	tsize_t i=0;
	uint16 ri =0;
	uint16 v_samp=1;
	uint16 h_samp=1;
	int j=0;
	
	i++;
	
	while(i<(*striplength)){
		switch( strip[i] ){
			case 0xd8:
				/* SOI - start of image */
				_TIFFmemcpy(&(buffer[*bufferoffset]), &(strip[i-1]), 2);
				*bufferoffset+=2;
				i+=2;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc3:
			case 0xc9:
			case 0xca:
				if(no==0){
					_TIFFmemcpy(&(buffer[*bufferoffset]), &(strip[i-1]), strip[i+2]+2);
					for(j=0;j<buffer[*bufferoffset+9];j++){
						if( (buffer[*bufferoffset+11+(2*j)]>>4) > h_samp) 
							h_samp = (buffer[*bufferoffset+11+(2*j)]>>4);
						if( (buffer[*bufferoffset+11+(2*j)] & 0x0f) > v_samp) 
							v_samp = (buffer[*bufferoffset+11+(2*j)] & 0x0f);
					}
					v_samp*=8;
					h_samp*=8;
					ri=((( ((uint16)(buffer[*bufferoffset+5])<<8) | 
					(uint16)(buffer[*bufferoffset+6]) )+v_samp-1)/ 
					v_samp);
					ri*=((( ((uint16)(buffer[*bufferoffset+7])<<8) | 
					(uint16)(buffer[*bufferoffset+8]) )+h_samp-1)/ 
					h_samp);
					buffer[*bufferoffset+5]=
                                          (unsigned char) ((height>>8) & 0xff);
					buffer[*bufferoffset+6]=
                                            (unsigned char) (height & 0xff);
					*bufferoffset+=strip[i+2]+2;
					i+=strip[i+2]+2;

					buffer[(*bufferoffset)++]=0xff;
					buffer[(*bufferoffset)++]=0xdd;
					buffer[(*bufferoffset)++]=0x00;
					buffer[(*bufferoffset)++]=0x04;
					buffer[(*bufferoffset)++]=(ri >> 8) & 0xff;
					buffer[(*bufferoffset)++]= ri & 0xff;
				} else {
					i+=strip[i+2]+2;
				}
				break;
			case 0xc4:
			case 0xdb:
				_TIFFmemcpy(&(buffer[*bufferoffset]), &(strip[i-1]), strip[i+2]+2);
				*bufferoffset+=strip[i+2]+2;
				i+=strip[i+2]+2;
				break;
			case 0xda:
				if(no==0){
					_TIFFmemcpy(&(buffer[*bufferoffset]), &(strip[i-1]), strip[i+2]+2);
					*bufferoffset+=strip[i+2]+2;
					i+=strip[i+2]+2;
				} else {
					buffer[(*bufferoffset)++]=0xff;
					buffer[(*bufferoffset)++]=
                                            (unsigned char)(0xd0 | ((no-1)%8));
					i+=strip[i+2]+2;
				}
				_TIFFmemcpy(&(buffer[*bufferoffset]), &(strip[i-1]), (*striplength)-i-1);
				*bufferoffset+=(*striplength)-i-1;
				return(1);
			default:
				i+=strip[i+2]+2;
		}
	}
	

	return(0);
}
#endif

/*
	This functions converts a tilewidth x tilelength buffer of samples into an edgetilewidth x 
	tilelength buffer of samples.
*/
void t2p_tile_collapse_left(
	tdata_t buffer, 
	tsize_t scanwidth, 
	uint32 tilewidth, 
	uint32 edgetilewidth, 
	uint32 tilelength){
	
	uint32 i;
	tsize_t edgescanwidth=0;
	
	edgescanwidth = (scanwidth * edgetilewidth + (tilewidth - 1))/ tilewidth;
	for(i=0;i<tilelength;i++){
		_TIFFmemcpy( 
			&(((char*)buffer)[edgescanwidth*i]), 
			&(((char*)buffer)[scanwidth*i]), 
			edgescanwidth);
	}
	
	return;
}


/*
 * This function calls TIFFWriteDirectory on the output after blanking its
 * output by replacing the read, write, and seek procedures with empty
 * implementations, then it replaces the original implementations.
 */

void
t2p_write_advance_directory(T2P* t2p, TIFF* output)
{
	t2p_disable(output);
	if(!TIFFWriteDirectory(output)){
		TIFFError(TIFF2PDF_MODULE, 
			"Error writing virtual directory to output PDF %s", 
			TIFFFileName(output));
		t2p->t2p_error = T2P_ERR_ERROR;
		return;
	}
	t2p_enable(output);
	return;
}

tsize_t t2p_sample_planar_separate_to_contig(
											T2P* t2p, 
											unsigned char* buffer, 
											unsigned char* samplebuffer, 
											tsize_t samplebuffersize){

	tsize_t stride=0;
	tsize_t i=0;
	tsize_t j=0;
	
	stride=samplebuffersize/t2p->tiff_samplesperpixel;
	for(i=0;i<stride;i++){
		for(j=0;j<t2p->tiff_samplesperpixel;j++){
			buffer[i*t2p->tiff_samplesperpixel + j] = samplebuffer[i + j*stride];
		}
	}

	return(samplebuffersize);
}

tsize_t t2p_sample_realize_palette(T2P* t2p, unsigned char* buffer){

	uint32 sample_count=0;
	uint16 component_count=0;
	uint32 palette_offset=0;
	uint32 sample_offset=0;
	uint32 i=0;
	uint32 j=0;
	sample_count=t2p->tiff_width*t2p->tiff_length;
	component_count=t2p->tiff_samplesperpixel;
	
	for(i=sample_count;i>0;i--){
		palette_offset=buffer[i-1] * component_count;
		sample_offset= (i-1) * component_count;
		for(j=0;j<component_count;j++){
			buffer[sample_offset+j]=t2p->pdf_palette[palette_offset+j];
		}
	}

	return(0);
}

/*
	This functions converts in place a buffer of ABGR interleaved data
	into RGB interleaved data, discarding A.
*/

tsize_t t2p_sample_abgr_to_rgb(tdata_t data, uint32 samplecount)
{
	uint32 i=0;
	uint32 sample=0;
	
	for(i=0;i<samplecount;i++){
		sample=((uint32*)data)[i];
		((char*)data)[i*3]= (char) (sample & 0xff);
		((char*)data)[i*3+1]= (char) ((sample>>8) & 0xff);
		((char*)data)[i*3+2]= (char) ((sample>>16) & 0xff);
	}

	return(i*3);
}

/*
 * This functions converts in place a buffer of RGBA interleaved data
 * into RGB interleaved data, discarding A.
 */

tsize_t
t2p_sample_rgbaa_to_rgb(tdata_t data, uint32 samplecount)
{
	uint32 i;
	
	for(i = 0; i < samplecount; i++)
		memcpy((uint8*)data + i * 3, (uint8*)data + i * 4, 3);

	return(i * 3);
}

/*
 * This functions converts in place a buffer of RGBA interleaved data
 * into RGB interleaved data, adding 255-A to each component sample.
 */

tsize_t
t2p_sample_rgba_to_rgb(tdata_t data, uint32 samplecount)
{
	uint32 i = 0;
	uint32 sample = 0;
	uint8 alpha = 0;
	
	for (i = 0; i < samplecount; i++) {
		sample=((uint32*)data)[i];
		alpha=(uint8)((255 - ((sample >> 24) & 0xff)));
		((uint8 *)data)[i * 3] = (uint8) ((sample >> 16) & 0xff) + alpha;
		((uint8 *)data)[i * 3 + 1] = (uint8) ((sample >> 8) & 0xff) + alpha;
		((uint8 *)data)[i * 3 + 2] = (uint8) (sample & 0xff) + alpha;
	}

	return (i * 3);
}

/*
	This function converts the a and b samples of Lab data from signed
	to unsigned.
*/

tsize_t t2p_sample_lab_signed_to_unsigned(tdata_t buffer, uint32 samplecount){

	uint32 i=0;

	for(i=0;i<samplecount;i++){
		if( (((unsigned char*)buffer)[(i*3)+1] & 0x80) !=0){
			((unsigned char*)buffer)[(i*3)+1] =
				(unsigned char)(0x80 + ((char*)buffer)[(i*3)+1]);
		} else {
			((unsigned char*)buffer)[(i*3)+1] |= 0x80;
		}
		if( (((unsigned char*)buffer)[(i*3)+2] & 0x80) !=0){
			((unsigned char*)buffer)[(i*3)+2] =
				(unsigned char)(0x80 + ((char*)buffer)[(i*3)+2]);
		} else {
			((unsigned char*)buffer)[(i*3)+2] |= 0x80;
		}
	}

	return(samplecount*3);
}

/* 
	This function writes the PDF header to output.
*/

tsize_t t2p_write_pdf_header(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;
	
	buflen=sprintf(buffer, "%%PDF-%u.%u ", t2p->pdf_majorversion&0xff, t2p->pdf_minorversion&0xff);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t)"\n%\342\343\317\323\n", 7);

	return(written);
}

/*
	This function writes the beginning of a PDF object to output.
*/

tsize_t t2p_write_pdf_obj_start(uint32 number, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	buflen=sprintf(buffer, "%lu", (unsigned long)number);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen );
	written += t2pWriteFile(output, (tdata_t) " 0 obj\n", 7);

	return(written);
}

/*
	This function writes the end of a PDF object to output.
*/

tsize_t t2p_write_pdf_obj_end(TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) "endobj\n", 7);

	return(written);
}

/*
	This function writes a PDF name object to output.
*/

tsize_t t2p_write_pdf_name(unsigned char* name, TIFF* output){

	tsize_t written=0;
	uint32 i=0;
	char buffer[64];
	uint16 nextchar=0;
	size_t namelen=0;
	
	namelen = strlen((char *)name);
	if (namelen>126) {
		namelen=126;
	}
	written += t2pWriteFile(output, (tdata_t) "/", 1);
	for (i=0;i<namelen;i++){
		if ( ((unsigned char)name[i]) < 0x21){
			sprintf(buffer, "#%.2X", name[i]);
			buffer[sizeof(buffer) - 1] = '\0';
			written += t2pWriteFile(output, (tdata_t) buffer, 3);
			nextchar=1;
		}
		if ( ((unsigned char)name[i]) > 0x7E){
			sprintf(buffer, "#%.2X", name[i]);
			buffer[sizeof(buffer) - 1] = '\0';
			written += t2pWriteFile(output, (tdata_t) buffer, 3);
			nextchar=1;
		}
		if (nextchar==0){
			switch (name[i]){
				case 0x23:
					sprintf(buffer, "#%.2X", name[i]);
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x25:
					sprintf(buffer, "#%.2X", name[i]);
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x28:
					sprintf(buffer, "#%.2X", name[i]);
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x29:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x2F:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x3C:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x3E:
					sprintf(buffer, "#%.2X", name[i]);
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x5B:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x5D:
					sprintf(buffer, "#%.2X", name[i]);
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x7B:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				case 0x7D:
					sprintf(buffer, "#%.2X", name[i]); 
					buffer[sizeof(buffer) - 1] = '\0';
					written += t2pWriteFile(output, (tdata_t) buffer, 3);
					break;
				default:
					written += t2pWriteFile(output, (tdata_t) &name[i], 1);
			}
		}
		nextchar=0;
	}
	written += t2pWriteFile(output, (tdata_t) " ", 1);

	return(written);
}

/*
 * This function writes a PDF string object to output.
 */
	
tsize_t t2p_write_pdf_string(char* pdfstr, TIFF* output)
{
	tsize_t written = 0;
	uint32 i = 0;
	char buffer[64];
	size_t len = 0;
	
	len = strlen(pdfstr);
	written += t2pWriteFile(output, (tdata_t) "(", 1);
	for (i=0; i<len; i++) {
		if((pdfstr[i]&0x80) || (pdfstr[i]==127) || (pdfstr[i]<32)){
			snprintf(buffer, sizeof(buffer), "\\%.3o", ((unsigned char)pdfstr[i]));
			written += t2pWriteFile(output, (tdata_t)buffer, 4);
		} else {
			switch (pdfstr[i]){
				case 0x08:
					written += t2pWriteFile(output, (tdata_t) "\\b", 2);
					break;
				case 0x09:
					written += t2pWriteFile(output, (tdata_t) "\\t", 2);
					break;
				case 0x0A:
					written += t2pWriteFile(output, (tdata_t) "\\n", 2);
					break;
				case 0x0C:
					written += t2pWriteFile(output, (tdata_t) "\\f", 2);
					break;
				case 0x0D:
					written += t2pWriteFile(output, (tdata_t) "\\r", 2);
					break;
				case 0x28:
					written += t2pWriteFile(output, (tdata_t) "\\(", 2);
					break;
				case 0x29:
					written += t2pWriteFile(output, (tdata_t) "\\)", 2);
					break;
				case 0x5C:
					written += t2pWriteFile(output, (tdata_t) "\\\\", 2);
					break;
				default:
					written += t2pWriteFile(output, (tdata_t) &pdfstr[i], 1);
			}
		}
	}
	written += t2pWriteFile(output, (tdata_t) ") ", 1);

	return(written);
}


/*
	This function writes a buffer of data to output.
*/

tsize_t t2p_write_pdf_stream(tdata_t buffer, tsize_t len, TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) buffer, len);

	return(written);
}

/*
	This functions writes the beginning of a PDF stream to output.
*/

tsize_t t2p_write_pdf_stream_start(TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) "stream\n", 7);

	return(written);
}

/*
	This function writes the end of a PDF stream to output. 
*/

tsize_t t2p_write_pdf_stream_end(TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) "\nendstream\n", 11);

	return(written);
}

/*
	This function writes a stream dictionary for a PDF stream to output.
*/

tsize_t t2p_write_pdf_stream_dict(tsize_t len, uint32 number, TIFF* output){
	
	tsize_t written=0;
	char buffer[16];
	int buflen=0;
	
	written += t2pWriteFile(output, (tdata_t) "/Length ", 8);
	if(len!=0){
		written += t2p_write_pdf_stream_length(len, output);
	} else {
		buflen=sprintf(buffer, "%lu", (unsigned long)number);
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R \n", 6);
	}
	
	return(written);
}

/*
	This functions writes the beginning of a PDF stream dictionary to output.
*/

tsize_t t2p_write_pdf_stream_dict_start(TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) "<< \n", 4);

	return(written);
}

/*
	This function writes the end of a PDF stream dictionary to output. 
*/

tsize_t t2p_write_pdf_stream_dict_end(TIFF* output){

	tsize_t written=0;

	written += t2pWriteFile(output, (tdata_t) " >>\n", 4);

	return(written);
}

/*
	This function writes a number to output.
*/

tsize_t t2p_write_pdf_stream_length(tsize_t len, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	buflen=sprintf(buffer, "%lu", (unsigned long)len);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "\n", 1);

	return(written);
}

/*
 * This function writes the PDF Catalog structure to output.
 */

tsize_t t2p_write_pdf_catalog(T2P* t2p, TIFF* output)
{
	tsize_t written = 0;
	char buffer[16];
	int buflen = 0;

	written += t2pWriteFile(output, 
		(tdata_t)"<< \n/Type /Catalog \n/Pages ", 
		27);
	buflen = snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)t2p->pdf_pages);
	written += t2pWriteFile(output, (tdata_t) buffer,
				TIFFmin((size_t)buflen, sizeof(buffer) - 1));
	written += t2pWriteFile(output, (tdata_t) " 0 R \n", 6);
	if(t2p->pdf_fitwindow){
		written += t2pWriteFile(output, 
			(tdata_t) "/ViewerPreferences <</FitWindow true>>\n", 
			39);
	}
	written += t2pWriteFile(output, (tdata_t)">>\n", 3);

	return(written);
}

/*
	This function writes the PDF Info structure to output.
*/

tsize_t t2p_write_pdf_info(T2P* t2p, TIFF* input, TIFF* output)
{
	tsize_t written = 0;
	char* info;
	char buffer[512];

	if(t2p->pdf_datetime[0] == '\0')
		t2p_pdf_tifftime(t2p, input);
	if (strlen(t2p->pdf_datetime) > 0) {
		written += t2pWriteFile(output, (tdata_t) "<< \n/CreationDate ", 18);
		written += t2p_write_pdf_string(t2p->pdf_datetime, output);
		written += t2pWriteFile(output, (tdata_t) "\n/ModDate ", 10);
		written += t2p_write_pdf_string(t2p->pdf_datetime, output);
	}
	written += t2pWriteFile(output, (tdata_t) "\n/Producer ", 11);
	_TIFFmemset((tdata_t)buffer, 0x00, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "libtiff / tiff2pdf - %d", TIFFLIB_VERSION);
	written += t2p_write_pdf_string(buffer, output);
	written += t2pWriteFile(output, (tdata_t) "\n", 1);
	if (t2p->pdf_creator[0] != '\0') {
		written += t2pWriteFile(output, (tdata_t) "/Creator ", 9);
		written += t2p_write_pdf_string(t2p->pdf_creator, output);
		written += t2pWriteFile(output, (tdata_t) "\n", 1);
	} else {
		if (TIFFGetField(input, TIFFTAG_SOFTWARE, &info) != 0 && info) {
			if(strlen(info) >= sizeof(t2p->pdf_creator))
				info[sizeof(t2p->pdf_creator) - 1] = '\0';
			written += t2pWriteFile(output, (tdata_t) "/Creator ", 9);
			written += t2p_write_pdf_string(info, output);
			written += t2pWriteFile(output, (tdata_t) "\n", 1);
		}
	}
	if (t2p->pdf_author[0] != '\0') {
		written += t2pWriteFile(output, (tdata_t) "/Author ", 8);
		written += t2p_write_pdf_string(t2p->pdf_author, output);
		written += t2pWriteFile(output, (tdata_t) "\n", 1);
	} else {
		if ((TIFFGetField(input, TIFFTAG_ARTIST, &info) != 0
		     || TIFFGetField(input, TIFFTAG_COPYRIGHT, &info) != 0)
		    && info) {
			if (strlen(info) >= sizeof(t2p->pdf_author))
				info[sizeof(t2p->pdf_author) - 1] = '\0';
			written += t2pWriteFile(output, (tdata_t) "/Author ", 8);
			written += t2p_write_pdf_string(info, output);
			written += t2pWriteFile(output, (tdata_t) "\n", 1);
		}
	}
	if (t2p->pdf_title[0] != '\0') {
		written += t2pWriteFile(output, (tdata_t) "/Title ", 7);
		written += t2p_write_pdf_string(t2p->pdf_title, output);
		written += t2pWriteFile(output, (tdata_t) "\n", 1);
	} else {
		if (TIFFGetField(input, TIFFTAG_DOCUMENTNAME, &info) != 0){
			if(strlen(info) > 511) {
				info[512] = '\0';
			}
			written += t2pWriteFile(output, (tdata_t) "/Title ", 7);
			written += t2p_write_pdf_string(info, output);
			written += t2pWriteFile(output, (tdata_t) "\n", 1);
		}
	}
	if (t2p->pdf_subject[0] != '\0') {
		written += t2pWriteFile(output, (tdata_t) "/Subject ", 9);
		written += t2p_write_pdf_string(t2p->pdf_subject, output);
		written += t2pWriteFile(output, (tdata_t) "\n", 1);
	} else {
		if (TIFFGetField(input, TIFFTAG_IMAGEDESCRIPTION, &info) != 0 && info) {
			if (strlen(info) >= sizeof(t2p->pdf_subject))
				info[sizeof(t2p->pdf_subject) - 1] = '\0';
			written += t2pWriteFile(output, (tdata_t) "/Subject ", 9);
			written += t2p_write_pdf_string(info, output);
			written += t2pWriteFile(output, (tdata_t) "\n", 1);
		}
	}
	if (t2p->pdf_keywords[0] != '\0') {
		written += t2pWriteFile(output, (tdata_t) "/Keywords ", 10);
		written += t2p_write_pdf_string(t2p->pdf_keywords, output);
		written += t2pWriteFile(output, (tdata_t) "\n", 1);
	}
	written += t2pWriteFile(output, (tdata_t) ">> \n", 4);

	return(written);
}

/*
 * This function fills a string of a T2P struct with the current time as a PDF
 * date string, it is called by t2p_pdf_tifftime.
 */

void t2p_pdf_currenttime(T2P* t2p)
{
	struct tm* currenttime;
	time_t timenow;

	if (time(&timenow) == (time_t) -1) {
		TIFFError(TIFF2PDF_MODULE,
			  "Can't get the current time: %s", strerror(errno));
		timenow = (time_t) 0;
	}

	currenttime = localtime(&timenow);
	snprintf(t2p->pdf_datetime, sizeof(t2p->pdf_datetime),
		 "D:%.4d%.2d%.2d%.2d%.2d%.2d",
		 (currenttime->tm_year + 1900) % 65536,
		 (currenttime->tm_mon + 1) % 256,
		 (currenttime->tm_mday) % 256,
		 (currenttime->tm_hour) % 256,
		 (currenttime->tm_min) % 256,
		 (currenttime->tm_sec) % 256);

	return;
}

/*
 * This function fills a string of a T2P struct with the date and time of a
 * TIFF file if it exists or the current time as a PDF date string.
 */

void t2p_pdf_tifftime(T2P* t2p, TIFF* input)
{
	char* datetime;

	if (TIFFGetField(input, TIFFTAG_DATETIME, &datetime) != 0
	    && (strlen(datetime) >= 19) ){
		t2p->pdf_datetime[0]='D';
		t2p->pdf_datetime[1]=':';
		t2p->pdf_datetime[2]=datetime[0];
		t2p->pdf_datetime[3]=datetime[1];
		t2p->pdf_datetime[4]=datetime[2];
		t2p->pdf_datetime[5]=datetime[3];
		t2p->pdf_datetime[6]=datetime[5];
		t2p->pdf_datetime[7]=datetime[6];
		t2p->pdf_datetime[8]=datetime[8];
		t2p->pdf_datetime[9]=datetime[9];
		t2p->pdf_datetime[10]=datetime[11];
		t2p->pdf_datetime[11]=datetime[12];
		t2p->pdf_datetime[12]=datetime[14];
		t2p->pdf_datetime[13]=datetime[15];
		t2p->pdf_datetime[14]=datetime[17];
		t2p->pdf_datetime[15]=datetime[18];
		t2p->pdf_datetime[16] = '\0';
	} else {
		t2p_pdf_currenttime(t2p);
	}

	return;
}

/*
 * This function writes a PDF Pages Tree structure to output.
 */

tsize_t t2p_write_pdf_pages(T2P* t2p, TIFF* output)
{
	tsize_t written=0;
	tdir_t i=0;
	char buffer[16];
	int buflen=0;

	int page=0;
	written += t2pWriteFile(output,
		(tdata_t) "<< \n/Type /Pages \n/Kids [ ", 26);
	page = t2p->pdf_pages+1;
	for (i=0;i<t2p->tiff_pagecount;i++){
		buflen=sprintf(buffer, "%d", page);
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		if ( ((i+1)%8)==0 ) {
			written += t2pWriteFile(output, (tdata_t) "\n", 1);
		}
		page +=3;
		page += t2p->tiff_pages[i].page_extra;
		if(t2p->tiff_pages[i].page_tilecount>0){
			page += (2 * t2p->tiff_pages[i].page_tilecount);
		} else {
			page +=2;
		}
	}
	written += t2pWriteFile(output, (tdata_t) "] \n/Count ", 10);
	_TIFFmemset(buffer, 0x00, 16);
	buflen=sprintf(buffer, "%d", t2p->tiff_pagecount);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " \n>> \n", 6);

	return(written);
}

/*
	This function writes a PDF Page structure to output.
*/

tsize_t t2p_write_pdf_page(uint32 object, T2P* t2p, TIFF* output){

	unsigned int i=0;
	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	written += t2pWriteFile(output, (tdata_t) "<<\n/Type /Page \n/Parent ", 24);
	buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_pages);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " 0 R \n", 6);
	written += t2pWriteFile(output, (tdata_t) "/MediaBox [", 11); 
	buflen=sprintf(buffer, "%.4f",t2p->pdf_mediabox.x1);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " ", 1); 
	buflen=sprintf(buffer, "%.4f",t2p->pdf_mediabox.y1);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " ", 1); 
	buflen=sprintf(buffer, "%.4f",t2p->pdf_mediabox.x2);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " ", 1); 
	buflen=sprintf(buffer, "%.4f",t2p->pdf_mediabox.y2);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "] \n", 3); 
	written += t2pWriteFile(output, (tdata_t) "/Contents ", 10);
	buflen=sprintf(buffer, "%lu", (unsigned long)(object + 1));
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " 0 R \n", 6);
	written += t2pWriteFile(output, (tdata_t) "/Resources << \n", 15);
	if( t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount != 0 ){
		written += t2pWriteFile(output, (tdata_t) "/XObject <<\n", 12);
		for(i=0;i<t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount;i++){
			written += t2pWriteFile(output, (tdata_t) "/Im", 3);
			buflen = sprintf(buffer, "%u", t2p->pdf_page+1);
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) "_", 1);
			buflen = sprintf(buffer, "%u", i+1);
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) " ", 1);
			buflen = sprintf(
				buffer, 
				"%lu", 
				(unsigned long)(object+3+(2*i)+t2p->tiff_pages[t2p->pdf_page].page_extra)); 
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
			if(i%4==3){
				written += t2pWriteFile(output, (tdata_t) "\n", 1);
			}
		}
		written += t2pWriteFile(output, (tdata_t) ">>\n", 3);
	} else {
			written += t2pWriteFile(output, (tdata_t) "/XObject <<\n", 12);
			written += t2pWriteFile(output, (tdata_t) "/Im", 3);
			buflen = sprintf(buffer, "%u", t2p->pdf_page+1);
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) " ", 1);
			buflen = sprintf(
				buffer, 
				"%lu", 
				(unsigned long)(object+3+(2*i)+t2p->tiff_pages[t2p->pdf_page].page_extra)); 
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		written += t2pWriteFile(output, (tdata_t) ">>\n", 3);
	}
	if(t2p->tiff_transferfunctioncount != 0) {
		written += t2pWriteFile(output, (tdata_t) "/ExtGState <<", 13);
		t2pWriteFile(output, (tdata_t) "/GS1 ", 5);
		buflen = sprintf(
			buffer, 
			"%lu", 
			(unsigned long)(object + 3)); 
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		written += t2pWriteFile(output, (tdata_t) ">> \n", 4);
	}
	written += t2pWriteFile(output, (tdata_t) "/ProcSet [ ", 11);
	if(t2p->pdf_colorspace == T2P_CS_BILEVEL 
		|| t2p->pdf_colorspace == T2P_CS_GRAY
		){
		written += t2pWriteFile(output, (tdata_t) "/ImageB ", 8);
	} else {
		written += t2pWriteFile(output, (tdata_t) "/ImageC ", 8);
		if(t2p->pdf_colorspace & T2P_CS_PALETTE){
			written += t2pWriteFile(output, (tdata_t) "/ImageI ", 8);
		}
	}
	written += t2pWriteFile(output, (tdata_t) "]\n>>\n>>\n", 8);

	return(written);
}

/*
	This function composes the page size and image and tile locations on a page.
*/

void t2p_compose_pdf_page(T2P* t2p){

	uint32 i=0;
	uint32 i2=0;
	T2P_TILE* tiles=NULL;
	T2P_BOX* boxp=NULL;
	uint32 tilecountx=0;
	uint32 tilecounty=0;
	uint32 tilewidth=0;
	uint32 tilelength=0;
	int istiled=0;
	float f=0;
	float width_ratio=0;
	float length_ratio=0;
	
	t2p->pdf_xres = t2p->tiff_xres;
	t2p->pdf_yres = t2p->tiff_yres;
	if(t2p->pdf_overrideres) {
		t2p->pdf_xres = t2p->pdf_defaultxres;
		t2p->pdf_yres = t2p->pdf_defaultyres;
	}
	if(t2p->pdf_xres == 0.0)
		t2p->pdf_xres = t2p->pdf_defaultxres;
	if(t2p->pdf_yres == 0.0)
		t2p->pdf_yres = t2p->pdf_defaultyres;
	if (t2p->pdf_image_fillpage) {
		width_ratio = t2p->pdf_defaultpagewidth/t2p->tiff_width;
		length_ratio = t2p->pdf_defaultpagelength/t2p->tiff_length;
		if (width_ratio < length_ratio ) {
			t2p->pdf_imagewidth = t2p->pdf_defaultpagewidth;
			t2p->pdf_imagelength = t2p->tiff_length * width_ratio;
		} else {
			t2p->pdf_imagewidth = t2p->tiff_width * length_ratio;
			t2p->pdf_imagelength = t2p->pdf_defaultpagelength;
		}
	} else if (t2p->tiff_resunit != RESUNIT_CENTIMETER	/* RESUNIT_NONE and */
		&& t2p->tiff_resunit != RESUNIT_INCH) {	/* other cases */
		t2p->pdf_imagewidth = ((float)(t2p->tiff_width))/t2p->pdf_xres;
		t2p->pdf_imagelength = ((float)(t2p->tiff_length))/t2p->pdf_yres;
	} else {
		t2p->pdf_imagewidth = 
			((float)(t2p->tiff_width))*PS_UNIT_SIZE/t2p->pdf_xres;
		t2p->pdf_imagelength = 
			((float)(t2p->tiff_length))*PS_UNIT_SIZE/t2p->pdf_yres;
	}
	if(t2p->pdf_overridepagesize != 0) {
		t2p->pdf_pagewidth = t2p->pdf_defaultpagewidth;
		t2p->pdf_pagelength = t2p->pdf_defaultpagelength;
	} else {
		t2p->pdf_pagewidth = t2p->pdf_imagewidth;
		t2p->pdf_pagelength = t2p->pdf_imagelength;
	}
	t2p->pdf_mediabox.x1=0.0;
	t2p->pdf_mediabox.y1=0.0;
	t2p->pdf_mediabox.x2=t2p->pdf_pagewidth;
	t2p->pdf_mediabox.y2=t2p->pdf_pagelength;
	t2p->pdf_imagebox.x1=0.0;
	t2p->pdf_imagebox.y1=0.0;
	t2p->pdf_imagebox.x2=t2p->pdf_imagewidth;
	t2p->pdf_imagebox.y2=t2p->pdf_imagelength;
	if(t2p->pdf_overridepagesize!=0){
		t2p->pdf_imagebox.x1+=((t2p->pdf_pagewidth-t2p->pdf_imagewidth)/2.0F);
		t2p->pdf_imagebox.y1+=((t2p->pdf_pagelength-t2p->pdf_imagelength)/2.0F);
		t2p->pdf_imagebox.x2+=((t2p->pdf_pagewidth-t2p->pdf_imagewidth)/2.0F);
		t2p->pdf_imagebox.y2+=((t2p->pdf_pagelength-t2p->pdf_imagelength)/2.0F);
	}
	if(t2p->tiff_orientation > 4){
		f=t2p->pdf_mediabox.x2;
		t2p->pdf_mediabox.x2=t2p->pdf_mediabox.y2;
		t2p->pdf_mediabox.y2=f;
	}
	istiled=((t2p->tiff_tiles[t2p->pdf_page]).tiles_tilecount==0) ? 0 : 1;
	if(istiled==0){
		t2p_compose_pdf_page_orient(&(t2p->pdf_imagebox), t2p->tiff_orientation);
		return;
	} else {
		tilewidth=(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilewidth;
		tilelength=(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilelength;
		tilecountx=(t2p->tiff_width + 
			tilewidth -1)/ 
			tilewidth;
		(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilecountx=tilecountx;
		tilecounty=(t2p->tiff_length + 
			tilelength -1)/ 
			tilelength;
		(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilecounty=tilecounty;
		(t2p->tiff_tiles[t2p->pdf_page]).tiles_edgetilewidth=
			t2p->tiff_width % tilewidth;
		(t2p->tiff_tiles[t2p->pdf_page]).tiles_edgetilelength=
			t2p->tiff_length % tilelength;
		tiles=(t2p->tiff_tiles[t2p->pdf_page]).tiles_tiles;
		for(i2=0;i2<tilecounty-1;i2++){
			for(i=0;i<tilecountx-1;i++){
				boxp=&(tiles[i2*tilecountx+i].tile_box);
				boxp->x1 = 
					t2p->pdf_imagebox.x1 
					+ ((float)(t2p->pdf_imagewidth * i * tilewidth)
					/ (float)t2p->tiff_width);
				boxp->x2 = 
					t2p->pdf_imagebox.x1 
					+ ((float)(t2p->pdf_imagewidth * (i+1) * tilewidth)
					/ (float)t2p->tiff_width);
				boxp->y1 = 
					t2p->pdf_imagebox.y2 
					- ((float)(t2p->pdf_imagelength * (i2+1) * tilelength)
					/ (float)t2p->tiff_length);
				boxp->y2 = 
					t2p->pdf_imagebox.y2 
					- ((float)(t2p->pdf_imagelength * i2 * tilelength)
					/ (float)t2p->tiff_length);
			}
			boxp=&(tiles[i2*tilecountx+i].tile_box);
			boxp->x1 = 
				t2p->pdf_imagebox.x1 
				+ ((float)(t2p->pdf_imagewidth * i * tilewidth)
				/ (float)t2p->tiff_width);
			boxp->x2 = t2p->pdf_imagebox.x2;
			boxp->y1 = 
				t2p->pdf_imagebox.y2 
				- ((float)(t2p->pdf_imagelength * (i2+1) * tilelength)
				/ (float)t2p->tiff_length);
			boxp->y2 = 
				t2p->pdf_imagebox.y2 
				- ((float)(t2p->pdf_imagelength * i2 * tilelength)
				/ (float)t2p->tiff_length);
		}
		for(i=0;i<tilecountx-1;i++){
			boxp=&(tiles[i2*tilecountx+i].tile_box);
			boxp->x1 = 
				t2p->pdf_imagebox.x1 
				+ ((float)(t2p->pdf_imagewidth * i * tilewidth)
				/ (float)t2p->tiff_width);
			boxp->x2 = 
				t2p->pdf_imagebox.x1 
				+ ((float)(t2p->pdf_imagewidth * (i+1) * tilewidth)
				/ (float)t2p->tiff_width);
			boxp->y1 = t2p->pdf_imagebox.y1;
			boxp->y2 = 
				t2p->pdf_imagebox.y2 
				- ((float)(t2p->pdf_imagelength * i2 * tilelength)
				/ (float)t2p->tiff_length);
		}
		boxp=&(tiles[i2*tilecountx+i].tile_box);
		boxp->x1 = 
			t2p->pdf_imagebox.x1 
			+ ((float)(t2p->pdf_imagewidth * i * tilewidth)
			/ (float)t2p->tiff_width);
		boxp->x2 = t2p->pdf_imagebox.x2;
		boxp->y1 = t2p->pdf_imagebox.y1;
		boxp->y2 = 
			t2p->pdf_imagebox.y2 
			- ((float)(t2p->pdf_imagelength * i2 * tilelength)
			/ (float)t2p->tiff_length);
	}
	if(t2p->tiff_orientation==0 || t2p->tiff_orientation==1){
		for(i=0;i<(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilecount;i++){
			t2p_compose_pdf_page_orient( &(tiles[i].tile_box) , 0);
		}
		return;
	}
	for(i=0;i<(t2p->tiff_tiles[t2p->pdf_page]).tiles_tilecount;i++){
		boxp=&(tiles[i].tile_box);
		boxp->x1 -= t2p->pdf_imagebox.x1;
		boxp->x2 -= t2p->pdf_imagebox.x1;
		boxp->y1 -= t2p->pdf_imagebox.y1;
		boxp->y2 -= t2p->pdf_imagebox.y1;
		if(t2p->tiff_orientation==2 || t2p->tiff_orientation==3){
			boxp->x1 = t2p->pdf_imagebox.x2 - t2p->pdf_imagebox.x1 - boxp->x1;
			boxp->x2 = t2p->pdf_imagebox.x2 - t2p->pdf_imagebox.x1 - boxp->x2;
		}
		if(t2p->tiff_orientation==3 || t2p->tiff_orientation==4){
			boxp->y1 = t2p->pdf_imagebox.y2 - t2p->pdf_imagebox.y1 - boxp->y1;
			boxp->y2 = t2p->pdf_imagebox.y2 - t2p->pdf_imagebox.y1 - boxp->y2;
		}
		if(t2p->tiff_orientation==8 || t2p->tiff_orientation==5){
			boxp->y1 = t2p->pdf_imagebox.y2 - t2p->pdf_imagebox.y1 - boxp->y1;
			boxp->y2 = t2p->pdf_imagebox.y2 - t2p->pdf_imagebox.y1 - boxp->y2;
		}
		if(t2p->tiff_orientation==5 || t2p->tiff_orientation==6){
			boxp->x1 = t2p->pdf_imagebox.x2 - t2p->pdf_imagebox.x1 - boxp->x1;
			boxp->x2 = t2p->pdf_imagebox.x2 - t2p->pdf_imagebox.x1 - boxp->x2;
		}
		if(t2p->tiff_orientation > 4){
			f=boxp->x1;
			boxp->x1 = boxp->y1;
			boxp->y1 = f;
			f=boxp->x2;
			boxp->x2 = boxp->y2;
			boxp->y2 = f; 
			t2p_compose_pdf_page_orient_flip(boxp, t2p->tiff_orientation);
		} else {
			t2p_compose_pdf_page_orient(boxp, t2p->tiff_orientation);
		}
		
	}

	return;
}

void t2p_compose_pdf_page_orient(T2P_BOX* boxp, uint16 orientation){

	float m1[9];
	float f=0.0;
	
	if( boxp->x1 > boxp->x2){
		f=boxp->x1;
		boxp->x1=boxp->x2;
		boxp->x2 = f;
	}
	if( boxp->y1 > boxp->y2){
		f=boxp->y1;
		boxp->y1=boxp->y2;
		boxp->y2 = f;
	}
	boxp->mat[0]=m1[0]=boxp->x2-boxp->x1;
	boxp->mat[1]=m1[1]=0.0;
	boxp->mat[2]=m1[2]=0.0;
	boxp->mat[3]=m1[3]=0.0;
	boxp->mat[4]=m1[4]=boxp->y2-boxp->y1;
	boxp->mat[5]=m1[5]=0.0;
	boxp->mat[6]=m1[6]=boxp->x1;
	boxp->mat[7]=m1[7]=boxp->y1;
	boxp->mat[8]=m1[8]=1.0;
	switch(orientation){
		case 0:
		case 1:
			break;
		case 2:
			boxp->mat[0]=0.0F-m1[0];
			boxp->mat[6]+=m1[0];
			break;
		case 3:
			boxp->mat[0]=0.0F-m1[0];
			boxp->mat[4]=0.0F-m1[4];
			boxp->mat[6]+=m1[0];
			boxp->mat[7]+=m1[4];
			break;
		case 4:
			boxp->mat[4]=0.0F-m1[4];
			boxp->mat[7]+=m1[4];
			break;
		case 5:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=0.0F-m1[0];
			boxp->mat[3]=0.0F-m1[4];
			boxp->mat[4]=0.0F;
			boxp->mat[6]+=m1[4];
			boxp->mat[7]+=m1[0];
			break;
		case 6:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=0.0F-m1[0];
			boxp->mat[3]=m1[4];
			boxp->mat[4]=0.0F;
			boxp->mat[7]+=m1[0];
			break;
		case 7:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=m1[0];
			boxp->mat[3]=m1[4];
			boxp->mat[4]=0.0F;
			break;
		case 8:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=m1[0];
			boxp->mat[3]=0.0F-m1[4];
			boxp->mat[4]=0.0F;
			boxp->mat[6]+=m1[4];
			break;
	}

	return;
}

void t2p_compose_pdf_page_orient_flip(T2P_BOX* boxp, uint16 orientation){

	float m1[9];
	float f=0.0;
	
	if( boxp->x1 > boxp->x2){
		f=boxp->x1;
		boxp->x1=boxp->x2;
		boxp->x2 = f;
	}
	if( boxp->y1 > boxp->y2){
		f=boxp->y1;
		boxp->y1=boxp->y2;
		boxp->y2 = f;
	}
	boxp->mat[0]=m1[0]=boxp->x2-boxp->x1;
	boxp->mat[1]=m1[1]=0.0F;
	boxp->mat[2]=m1[2]=0.0F;
	boxp->mat[3]=m1[3]=0.0F;
	boxp->mat[4]=m1[4]=boxp->y2-boxp->y1;
	boxp->mat[5]=m1[5]=0.0F;
	boxp->mat[6]=m1[6]=boxp->x1;
	boxp->mat[7]=m1[7]=boxp->y1;
	boxp->mat[8]=m1[8]=1.0F;
	switch(orientation){
		case 5:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=0.0F-m1[4];
			boxp->mat[3]=0.0F-m1[0];
			boxp->mat[4]=0.0F;
			boxp->mat[6]+=m1[0];
			boxp->mat[7]+=m1[4];
			break;
		case 6:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=0.0F-m1[4];
			boxp->mat[3]=m1[0];
			boxp->mat[4]=0.0F;
			boxp->mat[7]+=m1[4];
			break;
		case 7:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=m1[4];
			boxp->mat[3]=m1[0];
			boxp->mat[4]=0.0F;
			break;
		case 8:
			boxp->mat[0]=0.0F;
			boxp->mat[1]=m1[4];
			boxp->mat[3]=0.0F-m1[0];
			boxp->mat[4]=0.0F;
			boxp->mat[6]+=m1[0];
			break;
	}

	return;
}

/*
	This function writes a PDF Contents stream to output.
*/

tsize_t t2p_write_pdf_page_content_stream(T2P* t2p, TIFF* output){

	tsize_t written=0;
	ttile_t i=0;
	char buffer[512];
	int buflen=0;
	T2P_BOX box;
	
	if(t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount>0){ 
		for(i=0;i<t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount; i++){
			box=t2p->tiff_tiles[t2p->pdf_page].tiles_tiles[i].tile_box;
			buflen=sprintf(buffer, 
				"q %s %.4f %.4f %.4f %.4f %.4f %.4f cm /Im%d_%ld Do Q\n", 
				t2p->tiff_transferfunctioncount?"/GS1 gs ":"",
				box.mat[0],
				box.mat[1],
				box.mat[3],
				box.mat[4],
				box.mat[6],
				box.mat[7],
				t2p->pdf_page + 1, 
				(long)(i + 1));
			written += t2p_write_pdf_stream(buffer, buflen, output);
		}
	} else {
		box=t2p->pdf_imagebox;
		buflen=sprintf(buffer, 
			"q %s %.4f %.4f %.4f %.4f %.4f %.4f cm /Im%d Do Q\n", 
			t2p->tiff_transferfunctioncount?"/GS1 gs ":"",
			box.mat[0],
			box.mat[1],
			box.mat[3],
			box.mat[4],
			box.mat[6],
			box.mat[7],
			t2p->pdf_page+1);
		written += t2p_write_pdf_stream(buffer, buflen, output);
	}

	return(written);
}

/*
	This function writes a PDF Image XObject stream dictionary to output. 
*/

tsize_t t2p_write_pdf_xobject_stream_dict(ttile_t tile, 
												T2P* t2p, 
												TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	written += t2p_write_pdf_stream_dict(0, t2p->pdf_xrefcount+1, output); 
	written += t2pWriteFile(output, 
		(tdata_t) "/Type /XObject \n/Subtype /Image \n/Name /Im", 
		42);
	buflen=sprintf(buffer, "%u", t2p->pdf_page+1);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	if(tile != 0){
		written += t2pWriteFile(output, (tdata_t) "_", 1);
		buflen=sprintf(buffer, "%lu", (unsigned long)tile);
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	}
	written += t2pWriteFile(output, (tdata_t) "\n/Width ", 8);
	_TIFFmemset((tdata_t)buffer, 0x00, 16);
	if(tile==0){
		buflen=sprintf(buffer, "%lu", (unsigned long)t2p->tiff_width);
	} else {
		if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile-1)!=0){
			buflen=sprintf(
				buffer, 
				"%lu", 
				(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth);
		} else {
			buflen=sprintf(
				buffer, 
				"%lu", 
				(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth);
		}
	}
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "\n/Height ", 9);
	_TIFFmemset((tdata_t)buffer, 0x00, 16);
	if(tile==0){
		buflen=sprintf(buffer, "%lu", (unsigned long)t2p->tiff_length);
	} else {
		if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile-1)!=0){
			buflen=sprintf(
				buffer, 
				"%lu", 
				(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
		} else {
			buflen=sprintf(
				buffer, 
				"%lu", 
				(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
		}
	}
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "\n/BitsPerComponent ", 19);
	_TIFFmemset((tdata_t)buffer, 0x00, 16);
	buflen=sprintf(buffer, "%u", t2p->tiff_bitspersample);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "\n/ColorSpace ", 13);
	written += t2p_write_pdf_xobject_cs(t2p, output);
	if (t2p->pdf_image_interpolate)
		written += t2pWriteFile(output,
					 (tdata_t) "\n/Interpolate true", 18);
	if( (t2p->pdf_switchdecode != 0)
#ifdef CCITT_SUPPORT
		&& ! (t2p->pdf_colorspace == T2P_CS_BILEVEL 
		&& t2p->pdf_compression == T2P_COMPRESS_G4)
#endif
		){
		written += t2p_write_pdf_xobject_decode(t2p, output);
	}
	written += t2p_write_pdf_xobject_stream_filter(tile, t2p, output);
	
	return(written);
}

/*
 * 	This function writes a PDF Image XObject Colorspace name to output.
 */


tsize_t t2p_write_pdf_xobject_cs(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[128];
	int buflen=0;

	float X_W=1.0;
	float Y_W=1.0;
	float Z_W=1.0;
	
	if( (t2p->pdf_colorspace & T2P_CS_ICCBASED) != 0){
		written += t2p_write_pdf_xobject_icccs(t2p, output);
		return(written);
	}
	if( (t2p->pdf_colorspace & T2P_CS_PALETTE) != 0){
		written += t2pWriteFile(output, (tdata_t) "[ /Indexed ", 11);
		t2p->pdf_colorspace ^= T2P_CS_PALETTE;
		written += t2p_write_pdf_xobject_cs(t2p, output);
		t2p->pdf_colorspace |= T2P_CS_PALETTE;
		buflen=sprintf(buffer, "%u", (0x0001 << t2p->tiff_bitspersample)-1 );
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " ", 1);
		_TIFFmemset(buffer, 0x00, 16);
		buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_palettecs ); 
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ]\n", 7);
		return(written);
	}
	if(t2p->pdf_colorspace & T2P_CS_BILEVEL){
			written += t2pWriteFile(output, (tdata_t) "/DeviceGray \n", 13);
	}
	if(t2p->pdf_colorspace & T2P_CS_GRAY){
			if(t2p->pdf_colorspace & T2P_CS_CALGRAY){
				written += t2p_write_pdf_xobject_calcs(t2p, output);
			} else {
				written += t2pWriteFile(output, (tdata_t) "/DeviceGray \n", 13);
			}
	}
	if(t2p->pdf_colorspace & T2P_CS_RGB){
			if(t2p->pdf_colorspace & T2P_CS_CALRGB){
				written += t2p_write_pdf_xobject_calcs(t2p, output);
			} else {
				written += t2pWriteFile(output, (tdata_t) "/DeviceRGB \n", 12);
			}
	}
	if(t2p->pdf_colorspace & T2P_CS_CMYK){
			written += t2pWriteFile(output, (tdata_t) "/DeviceCMYK \n", 13);
	}
	if(t2p->pdf_colorspace & T2P_CS_LAB){
			written += t2pWriteFile(output, (tdata_t) "[/Lab << \n", 10);
			written += t2pWriteFile(output, (tdata_t) "/WhitePoint ", 12);
			X_W = t2p->tiff_whitechromaticities[0];
			Y_W = t2p->tiff_whitechromaticities[1];
			Z_W = 1.0F - (X_W + Y_W);
			X_W /= Y_W;
			Z_W /= Y_W;
			Y_W = 1.0F;
			buflen=sprintf(buffer, "[%.4f %.4f %.4f] \n", X_W, Y_W, Z_W);
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) "/Range ", 7);
			buflen=sprintf(buffer, "[%d %d %d %d] \n", 
				t2p->pdf_labrange[0], 
				t2p->pdf_labrange[1], 
				t2p->pdf_labrange[2], 
				t2p->pdf_labrange[3]);
			written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			written += t2pWriteFile(output, (tdata_t) ">>] \n", 5);
			
	}
	
	return(written);
}

tsize_t t2p_write_pdf_transfer(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	written += t2pWriteFile(output, (tdata_t) "<< /Type /ExtGState \n/TR ", 25);
	if(t2p->tiff_transferfunctioncount == 1){
		buflen=sprintf(buffer, "%lu",
			       (unsigned long)(t2p->pdf_xrefcount + 1));
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
	} else {
		written += t2pWriteFile(output, (tdata_t) "[ ", 2);
		buflen=sprintf(buffer, "%lu",
			       (unsigned long)(t2p->pdf_xrefcount + 1));
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		buflen=sprintf(buffer, "%lu",
			       (unsigned long)(t2p->pdf_xrefcount + 2));
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		buflen=sprintf(buffer, "%lu",
			       (unsigned long)(t2p->pdf_xrefcount + 3));
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) " 0 R ", 5);
		written += t2pWriteFile(output, (tdata_t) "/Identity ] ", 12);
	}

	written += t2pWriteFile(output, (tdata_t) " >> \n", 5);

	return(written);
}

tsize_t t2p_write_pdf_transfer_dict(T2P* t2p, TIFF* output, uint16 i){

	tsize_t written=0;
	char buffer[32];
	int buflen=0;
	(void)i; /* XXX */

	written += t2pWriteFile(output, (tdata_t) "/FunctionType 0 \n", 17);
	written += t2pWriteFile(output, (tdata_t) "/Domain [0.0 1.0] \n", 19);
	written += t2pWriteFile(output, (tdata_t) "/Range [0.0 1.0] \n", 18);
	buflen=sprintf(buffer, "/Size [%u] \n", (1<<t2p->tiff_bitspersample));
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "/BitsPerSample 16 \n", 19);
	written += t2p_write_pdf_stream_dict(((tsize_t)1)<<(t2p->tiff_bitspersample+1), 0, output);

	return(written);
}

tsize_t t2p_write_pdf_transfer_stream(T2P* t2p, TIFF* output, uint16 i){

	tsize_t written=0;

	written += t2p_write_pdf_stream(
		t2p->tiff_transferfunction[i], 
		(((tsize_t)1)<<(t2p->tiff_bitspersample+1)), 
		output);

	return(written);
}

/*
	This function writes a PDF Image XObject Colorspace array to output.
*/

tsize_t t2p_write_pdf_xobject_calcs(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[128];
	int buflen=0;
	
	float X_W=0.0;
	float Y_W=0.0;
	float Z_W=0.0;
	float X_R=0.0;
	float Y_R=0.0;
	float Z_R=0.0;
	float X_G=0.0;
	float Y_G=0.0;
	float Z_G=0.0;
	float X_B=0.0;
	float Y_B=0.0;
	float Z_B=0.0;
	float x_w=0.0;
	float y_w=0.0;
	float z_w=0.0;
	float x_r=0.0;
	float y_r=0.0;
	float x_g=0.0;
	float y_g=0.0;
	float x_b=0.0;
	float y_b=0.0;
	float R=1.0;
	float G=1.0;
	float B=1.0;
	
	written += t2pWriteFile(output, (tdata_t) "[", 1);
	if(t2p->pdf_colorspace & T2P_CS_CALGRAY){
		written += t2pWriteFile(output, (tdata_t) "/CalGray ", 9);
		X_W = t2p->tiff_whitechromaticities[0];
		Y_W = t2p->tiff_whitechromaticities[1];
		Z_W = 1.0F - (X_W + Y_W);
		X_W /= Y_W;
		Z_W /= Y_W;
		Y_W = 1.0F;
	}
	if(t2p->pdf_colorspace & T2P_CS_CALRGB){
		written += t2pWriteFile(output, (tdata_t) "/CalRGB ", 8);
		x_w = t2p->tiff_whitechromaticities[0];
		y_w = t2p->tiff_whitechromaticities[1];
		x_r = t2p->tiff_primarychromaticities[0];
		y_r = t2p->tiff_primarychromaticities[1];
		x_g = t2p->tiff_primarychromaticities[2];
		y_g = t2p->tiff_primarychromaticities[3];
		x_b = t2p->tiff_primarychromaticities[4];
		y_b = t2p->tiff_primarychromaticities[5];
		z_w = y_w * ((x_g - x_b)*y_r - (x_r-x_b)*y_g + (x_r-x_g)*y_b);
		Y_R = (y_r/R) * ((x_g-x_b)*y_w - (x_w-x_b)*y_g + (x_w-x_g)*y_b) / z_w;
		X_R = Y_R * x_r / y_r;
		Z_R = Y_R * (((1-x_r)/y_r)-1);
		Y_G = ((0.0F-(y_g))/G) * ((x_r-x_b)*y_w - (x_w-x_b)*y_r + (x_w-x_r)*y_b) / z_w;
		X_G = Y_G * x_g / y_g;
		Z_G = Y_G * (((1-x_g)/y_g)-1);
		Y_B = (y_b/B) * ((x_r-x_g)*y_w - (x_w-x_g)*y_r + (x_w-x_r)*y_g) / z_w;
		X_B = Y_B * x_b / y_b;
		Z_B = Y_B * (((1-x_b)/y_b)-1);
		X_W = (X_R * R) + (X_G * G) + (X_B * B);
		Y_W = (Y_R * R) + (Y_G * G) + (Y_B * B);
		Z_W = (Z_R * R) + (Z_G * G) + (Z_B * B);
		X_W /= Y_W;
		Z_W /= Y_W;
		Y_W = 1.0;
	}
	written += t2pWriteFile(output, (tdata_t) "<< \n", 4);
	if(t2p->pdf_colorspace & T2P_CS_CALGRAY){
		written += t2pWriteFile(output, (tdata_t) "/WhitePoint ", 12);
		buflen=sprintf(buffer, "[%.4f %.4f %.4f] \n", X_W, Y_W, Z_W);
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) "/Gamma 2.2 \n", 12);
	}
	if(t2p->pdf_colorspace & T2P_CS_CALRGB){
		written += t2pWriteFile(output, (tdata_t) "/WhitePoint ", 12);
		buflen=sprintf(buffer, "[%.4f %.4f %.4f] \n", X_W, Y_W, Z_W);
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) "/Matrix ", 8);
		buflen=sprintf(buffer, "[%.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f] \n", 
			X_R, Y_R, Z_R, 
			X_G, Y_G, Z_G, 
			X_B, Y_B, Z_B); 
		written += t2pWriteFile(output, (tdata_t) buffer, buflen);
		written += t2pWriteFile(output, (tdata_t) "/Gamma [2.2 2.2 2.2] \n", 22);
	}
	written += t2pWriteFile(output, (tdata_t) ">>] \n", 5);

	return(written);
}

/*
	This function writes a PDF Image XObject Colorspace array to output.
*/

tsize_t t2p_write_pdf_xobject_icccs(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;
	
	written += t2pWriteFile(output, (tdata_t) "[/ICCBased ", 11);
	buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_icccs);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " 0 R] \n", 7);

	return(written);
}

tsize_t t2p_write_pdf_xobject_icccs_dict(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;
	
	written += t2pWriteFile(output, (tdata_t) "/N ", 3);
	buflen=sprintf(buffer, "%u \n", t2p->tiff_samplesperpixel);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) "/Alternate ", 11);
	t2p->pdf_colorspace ^= T2P_CS_ICCBASED;
	written += t2p_write_pdf_xobject_cs(t2p, output);
	t2p->pdf_colorspace |= T2P_CS_ICCBASED;
	written += t2p_write_pdf_stream_dict(t2p->tiff_iccprofilelength, 0, output);
	
	return(written);
}

tsize_t t2p_write_pdf_xobject_icccs_stream(T2P* t2p, TIFF* output){

	tsize_t written=0;

	written += t2p_write_pdf_stream(
				(tdata_t) t2p->tiff_iccprofile, 
				(tsize_t) t2p->tiff_iccprofilelength, 
				output);
	
	return(written);
}

/*
	This function writes a palette stream for an indexed color space to output.
*/

tsize_t t2p_write_pdf_xobject_palettecs_stream(T2P* t2p, TIFF* output){

	tsize_t written=0;

	written += t2p_write_pdf_stream(
				(tdata_t) t2p->pdf_palette, 
				(tsize_t) t2p->pdf_palettesize, 
				output);
	
	return(written);
}

/*
	This function writes a PDF Image XObject Decode array to output.
*/

tsize_t t2p_write_pdf_xobject_decode(T2P* t2p, TIFF* output){

	tsize_t written=0;
	int i=0;

	written += t2pWriteFile(output, (tdata_t) "/Decode [ ", 10);
	for (i=0;i<t2p->tiff_samplesperpixel;i++){
		written += t2pWriteFile(output, (tdata_t) "1 0 ", 4);
	}
	written += t2pWriteFile(output, (tdata_t) "]\n", 2);

	return(written);
}

/*
	This function writes a PDF Image XObject stream filter name and parameters to 
	output.
*/

tsize_t t2p_write_pdf_xobject_stream_filter(ttile_t tile, T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[16];
	int buflen=0;

	if(t2p->pdf_compression==T2P_COMPRESS_NONE){
		return(written);
	}
	written += t2pWriteFile(output, (tdata_t) "/Filter ", 8);
	switch(t2p->pdf_compression){
#ifdef CCITT_SUPPORT
		case T2P_COMPRESS_G4:
			written += t2pWriteFile(output, (tdata_t) "/CCITTFaxDecode ", 16);
			written += t2pWriteFile(output, (tdata_t) "/DecodeParms ", 13);
			written += t2pWriteFile(output, (tdata_t) "<< /K -1 ", 9);
			if(tile==0){
				written += t2pWriteFile(output, (tdata_t) "/Columns ", 9);
				buflen=sprintf(buffer, "%lu",
					       (unsigned long)t2p->tiff_width);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				written += t2pWriteFile(output, (tdata_t) " /Rows ", 7);
				buflen=sprintf(buffer, "%lu",
					       (unsigned long)t2p->tiff_length);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
			} else {
				if(t2p_tile_is_right_edge(t2p->tiff_tiles[t2p->pdf_page], tile-1)==0){
					written += t2pWriteFile(output, (tdata_t) "/Columns ", 9);
					buflen=sprintf(
						buffer, 
						"%lu", 
						(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_tilewidth);
					written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				} else {
					written += t2pWriteFile(output, (tdata_t) "/Columns ", 9);
					buflen=sprintf(
						buffer, 
						"%lu", 
						(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilewidth);
					written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				}
				if(t2p_tile_is_bottom_edge(t2p->tiff_tiles[t2p->pdf_page], tile-1)==0){
					written += t2pWriteFile(output, (tdata_t) " /Rows ", 7);
					buflen=sprintf(
						buffer, 
						"%lu", 
						(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_tilelength);
					written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				} else {
					written += t2pWriteFile(output, (tdata_t) " /Rows ", 7);
					buflen=sprintf(
						buffer, 
						"%lu", 
						(unsigned long)t2p->tiff_tiles[t2p->pdf_page].tiles_edgetilelength);
					written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				}
			}
			if(t2p->pdf_switchdecode == 0){
				written += t2pWriteFile(output, (tdata_t) " /BlackIs1 true ", 16);
			}
			written += t2pWriteFile(output, (tdata_t) ">>\n", 3);
			break;
#endif
#ifdef JPEG_SUPPORT
		case T2P_COMPRESS_JPEG:
			written += t2pWriteFile(output, (tdata_t) "/DCTDecode ", 11);

			if(t2p->tiff_photometric != PHOTOMETRIC_YCBCR) {
				written += t2pWriteFile(output, (tdata_t) "/DecodeParms ", 13);
				written += t2pWriteFile(output, (tdata_t) "<< /ColorTransform 0 >>\n", 24);
			}
			break;
#endif
#ifdef ZIP_SUPPORT
		case T2P_COMPRESS_ZIP:
			written += t2pWriteFile(output, (tdata_t) "/FlateDecode ", 13);
			if(t2p->pdf_compressionquality%100){
				written += t2pWriteFile(output, (tdata_t) "/DecodeParms ", 13);
				written += t2pWriteFile(output, (tdata_t) "<< /Predictor ", 14);
				_TIFFmemset(buffer, 0x00, 16);
				buflen=sprintf(buffer, "%u", t2p->pdf_compressionquality%100);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				written += t2pWriteFile(output, (tdata_t) " /Columns ", 10);
				_TIFFmemset(buffer, 0x00, 16);
				buflen = sprintf(buffer, "%lu",
						 (unsigned long)t2p->tiff_width);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				written += t2pWriteFile(output, (tdata_t) " /Colors ", 9);
				_TIFFmemset(buffer, 0x00, 16);
				buflen=sprintf(buffer, "%u", t2p->tiff_samplesperpixel);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				written += t2pWriteFile(output, (tdata_t) " /BitsPerComponent ", 19);
				_TIFFmemset(buffer, 0x00, 16);
				buflen=sprintf(buffer, "%u", t2p->tiff_bitspersample);
				written += t2pWriteFile(output, (tdata_t) buffer, buflen);
				written += t2pWriteFile(output, (tdata_t) ">>\n", 3);
			}
			break;
#endif
		default:
			break;
	}

	return(written);
}

/*
	This function writes a PDF xref table to output.
*/

tsize_t t2p_write_pdf_xreftable(T2P* t2p, TIFF* output){

	tsize_t written=0;
	char buffer[21];
	int buflen=0;
	uint32 i=0;

	written += t2pWriteFile(output, (tdata_t) "xref\n0 ", 7);
	buflen=sprintf(buffer, "%lu", (unsigned long)(t2p->pdf_xrefcount + 1));
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	written += t2pWriteFile(output, (tdata_t) " \n0000000000 65535 f \n", 22);
	for (i=0;i<t2p->pdf_xrefcount;i++){
		sprintf(buffer, "%.10lu 00000 n \n",
			(unsigned long)t2p->pdf_xrefoffsets[i]);
		written += t2pWriteFile(output, (tdata_t) buffer, 20);
	}

	return(written);
}

/*
 * This function writes a PDF trailer to output.
 */

tsize_t t2p_write_pdf_trailer(T2P* t2p, TIFF* output)
{

	tsize_t written = 0;
	char buffer[32];
	int buflen = 0;
	size_t i = 0;

	for (i = 0; i < sizeof(t2p->pdf_fileid) - 8; i += 8)
		snprintf(t2p->pdf_fileid + i, 9, "%.8X", rand());

	written += t2pWriteFile(output, (tdata_t) "trailer\n<<\n/Size ", 17);
	buflen = sprintf(buffer, "%lu", (unsigned long)(t2p->pdf_xrefcount+1));
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	_TIFFmemset(buffer, 0x00, 32);	
	written += t2pWriteFile(output, (tdata_t) "\n/Root ", 7);
	buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_catalog);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	_TIFFmemset(buffer, 0x00, 32);	
	written += t2pWriteFile(output, (tdata_t) " 0 R \n/Info ", 12);
	buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_info);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	_TIFFmemset(buffer, 0x00, 32);	
	written += t2pWriteFile(output, (tdata_t) " 0 R \n/ID[<", 11);
	written += t2pWriteFile(output, (tdata_t) t2p->pdf_fileid,
				sizeof(t2p->pdf_fileid) - 1);
	written += t2pWriteFile(output, (tdata_t) "><", 2);
	written += t2pWriteFile(output, (tdata_t) t2p->pdf_fileid,
				sizeof(t2p->pdf_fileid) - 1);
	written += t2pWriteFile(output, (tdata_t) ">]\n>>\nstartxref\n", 16);
	buflen=sprintf(buffer, "%lu", (unsigned long)t2p->pdf_startxref);
	written += t2pWriteFile(output, (tdata_t) buffer, buflen);
	_TIFFmemset(buffer, 0x00, 32);	
	written += t2pWriteFile(output, (tdata_t) "\n%%EOF\n", 7);

	return(written);
}
 
/*

  This function writes a PDF to a file given a pointer to a TIFF.

  The idea with using a TIFF* as output for a PDF file is that the file 
  can be created with TIFFClientOpen for memory-mapped use within the TIFF 
  library, and TIFFWriteEncodedStrip can be used to write compressed data to 
  the output.  The output is not actually a TIFF file, it is a PDF file.  

  This function uses only t2pWriteFile and TIFFWriteEncodedStrip to write to 
  the output TIFF file.  When libtiff would otherwise be writing data to the 
  output file, the write procedure of the TIFF structure is replaced with an 
  empty implementation.

  The first argument to the function is an initialized and validated T2P 
  context struct pointer.

  The second argument to the function is the TIFF* that is the input that has 
  been opened for reading and no other functions have been called upon it.

  The third argument to the function is the TIFF* that is the output that has 
  been opened for writing.  It has to be opened so that it hasn't written any 
  data to the output.  If the output is seekable then it's OK to seek to the 
  beginning of the file.  The function only writes to the output PDF and does 
  not seek.  See the example usage in the main() function.

	TIFF* output = TIFFOpen("output.pdf", "w");
	assert(output != NULL);

	if(output->tif_seekproc != NULL){
		t2pSeekFile(output, (toff_t) 0, SEEK_SET);
	}

  This function returns the file size of the output PDF file.  On error it 
  returns zero and the t2p->t2p_error variable is set to T2P_ERR_ERROR.

  After this function completes, call t2p_free on t2p, TIFFClose on input, 
  and TIFFClose on output.
*/

tsize_t t2p_write_pdf(T2P* t2p, TIFF* input, TIFF* output){

	tsize_t written=0;
	ttile_t i2=0;
	tsize_t streamlen=0;
	uint16 i=0;

	t2p_read_tiff_init(t2p, input);
	if(t2p->t2p_error!=T2P_ERR_OK){return(0);}
	t2p->pdf_xrefoffsets= (uint32*) _TIFFmalloc(t2p->pdf_xrefcount * sizeof(uint32) );
	if(t2p->pdf_xrefoffsets==NULL){
		TIFFError(
			TIFF2PDF_MODULE, 
			"Can't allocate %u bytes of memory for t2p_write_pdf", 
			(unsigned int) (t2p->pdf_xrefcount * sizeof(uint32)) );
		t2p->t2p_error = T2P_ERR_ERROR;
		return(written);
	}
	t2p->pdf_xrefcount=0;
	t2p->pdf_catalog=1;
	t2p->pdf_info=2;
	t2p->pdf_pages=3;
	written += t2p_write_pdf_header(t2p, output);
	t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
	t2p->pdf_catalog=t2p->pdf_xrefcount;
	written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
	written += t2p_write_pdf_catalog(t2p, output);
	written += t2p_write_pdf_obj_end(output);
	t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
	t2p->pdf_info=t2p->pdf_xrefcount;
	written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
	written += t2p_write_pdf_info(t2p, input, output);
	written += t2p_write_pdf_obj_end(output);
	t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
	t2p->pdf_pages=t2p->pdf_xrefcount;
	written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
	written += t2p_write_pdf_pages(t2p, output);
	written += t2p_write_pdf_obj_end(output);
	for(t2p->pdf_page=0;t2p->pdf_page<t2p->tiff_pagecount;t2p->pdf_page++){
		t2p_read_tiff_data(t2p, input);
		if(t2p->t2p_error!=T2P_ERR_OK){return(0);}
		t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
		written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
		written += t2p_write_pdf_page(t2p->pdf_xrefcount, t2p, output);
		written += t2p_write_pdf_obj_end(output);
		t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
		written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
		written += t2p_write_pdf_stream_dict_start(output);
		written += t2p_write_pdf_stream_dict(0, t2p->pdf_xrefcount+1, output);
		written += t2p_write_pdf_stream_dict_end(output);
		written += t2p_write_pdf_stream_start(output);
		streamlen=written;
		written += t2p_write_pdf_page_content_stream(t2p, output);
		streamlen=written-streamlen;
		written += t2p_write_pdf_stream_end(output);
		written += t2p_write_pdf_obj_end(output);
		t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
		written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
		written += t2p_write_pdf_stream_length(streamlen, output);
		written += t2p_write_pdf_obj_end(output);
		if(t2p->tiff_transferfunctioncount != 0){
			t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
			written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
			written += t2p_write_pdf_transfer(t2p, output);
			written += t2p_write_pdf_obj_end(output);
			for(i=0; i < t2p->tiff_transferfunctioncount; i++){
				t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
				written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
				written += t2p_write_pdf_stream_dict_start(output);
				written += t2p_write_pdf_transfer_dict(t2p, output, i);
				written += t2p_write_pdf_stream_dict_end(output);
				written += t2p_write_pdf_stream_start(output);
				streamlen=written;
				written += t2p_write_pdf_transfer_stream(t2p, output, i);
				streamlen=written-streamlen;
				written += t2p_write_pdf_stream_end(output);
				written += t2p_write_pdf_obj_end(output);
			}
		}
		if( (t2p->pdf_colorspace & T2P_CS_PALETTE) != 0){
			t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
			t2p->pdf_palettecs=t2p->pdf_xrefcount;
			written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
			written += t2p_write_pdf_stream_dict_start(output);
			written += t2p_write_pdf_stream_dict(t2p->pdf_palettesize, 0, output);
			written += t2p_write_pdf_stream_dict_end(output);
			written += t2p_write_pdf_stream_start(output);
			streamlen=written;
			written += t2p_write_pdf_xobject_palettecs_stream(t2p, output);
			streamlen=written-streamlen;
			written += t2p_write_pdf_stream_end(output);
			written += t2p_write_pdf_obj_end(output);
		}
		if( (t2p->pdf_colorspace & T2P_CS_ICCBASED) != 0){
			t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
			t2p->pdf_icccs=t2p->pdf_xrefcount;
			written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
			written += t2p_write_pdf_stream_dict_start(output);
			written += t2p_write_pdf_xobject_icccs_dict(t2p, output);
			written += t2p_write_pdf_stream_dict_end(output);
			written += t2p_write_pdf_stream_start(output);
			streamlen=written;
			written += t2p_write_pdf_xobject_icccs_stream(t2p, output);
			streamlen=written-streamlen;
			written += t2p_write_pdf_stream_end(output);
			written += t2p_write_pdf_obj_end(output);
		}
		if(t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount !=0){
			for(i2=0;i2<t2p->tiff_tiles[t2p->pdf_page].tiles_tilecount;i2++){
				t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
				written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
				written += t2p_write_pdf_stream_dict_start(output);
				written += t2p_write_pdf_xobject_stream_dict(
					i2+1, 
					t2p, 
					output);
				written += t2p_write_pdf_stream_dict_end(output);
				written += t2p_write_pdf_stream_start(output);
				streamlen=written;
				t2p_read_tiff_size_tile(t2p, input, i2);
				written += t2p_readwrite_pdf_image_tile(t2p, input, output, i2);
				t2p_write_advance_directory(t2p, output);
				if(t2p->t2p_error!=T2P_ERR_OK){return(0);}
				streamlen=written-streamlen;
				written += t2p_write_pdf_stream_end(output);
				written += t2p_write_pdf_obj_end(output);
				t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
				written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
				written += t2p_write_pdf_stream_length(streamlen, output);
				written += t2p_write_pdf_obj_end(output);
			}
		} else {
			t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
			written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
			written += t2p_write_pdf_stream_dict_start(output);
			written += t2p_write_pdf_xobject_stream_dict(
				0, 
				t2p, 
				output);
			written += t2p_write_pdf_stream_dict_end(output);
			written += t2p_write_pdf_stream_start(output);
			streamlen=written;
			t2p_read_tiff_size(t2p, input);
			written += t2p_readwrite_pdf_image(t2p, input, output);
			t2p_write_advance_directory(t2p, output);
			if(t2p->t2p_error!=T2P_ERR_OK){return(0);}
			streamlen=written-streamlen;
			written += t2p_write_pdf_stream_end(output);
			written += t2p_write_pdf_obj_end(output);
			t2p->pdf_xrefoffsets[t2p->pdf_xrefcount++]=written;
			written += t2p_write_pdf_obj_start(t2p->pdf_xrefcount, output);
			written += t2p_write_pdf_stream_length(streamlen, output);
			written += t2p_write_pdf_obj_end(output);
		}
	}
	t2p->pdf_startxref = written;
	written += t2p_write_pdf_xreftable(t2p, output);
	written += t2p_write_pdf_trailer(t2p, output);
	t2p_disable(output);

	return(written);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
