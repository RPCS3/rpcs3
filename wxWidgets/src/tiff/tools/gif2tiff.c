
/*
 * Copyright (c) 1990-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
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
 *	convert a GIF file into a TIFF file.
 *	based on Paul Haeberli's fromgif program which in turn is
 *	based on a GIF file reader by Marcel J.E. Mol March 23 1989 
 *
 *	if input is 320 by 200 pixel aspect is probably 1.2
 *	if input is 640 350 pixel aspect is probably 1.37
 *
 */
#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#define	GIFGAMMA	(1.5)		/* smaller makes output img brighter */
#define	IMAX		0xffff		/* max intensity value */
#define EXTRAFUDGE	128		/* some people write BAD .gif files */

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

unsigned short gamtab[256];

void
makegamtab(float gam)
{
    int i;

    for(i=0; i<256; i++) 
	gamtab[i] = (unsigned short) (IMAX*pow(i/255.0,gam)+0.5);
}

char* stuff[] = {
"usage: gif2tiff [options] input.gif output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
"",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding",
" -c none	use no compression algorithm on output",
"",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
NULL
};

static void
usage(void)
{
	char buf[BUFSIZ];
	int i;

	setbuf(stderr, buf);
        fprintf(stderr, "%s\n\n", TIFFGetVersion());
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(stderr, "%s\n", stuff[i]);
	exit(-1);
}

#define COLSIZE 256

unsigned char *stackp;
unsigned int prefix[4096];
unsigned char suffix[4096];
unsigned char stack[4096];
int datasize,codesize,codemask;     /* Decoder working variables */
int clear,eoi;                      /* Special code values */
int avail, oldcode;

FILE *infile;
int global;                        /* Is there a global color map? */
int globalbits;                     /* Number of bits of global colors */
unsigned char globalmap[COLSIZE][3];/* RGB values for global color map */
unsigned char *raster;              /* Decoded image data */
unsigned long width, height;
unsigned short red[COLSIZE];
unsigned short green[COLSIZE];
unsigned short blue[COLSIZE];
char *filename, *imagename;

static	uint16 compression = COMPRESSION_PACKBITS;
static	uint16 predictor = 0;
static	uint32 rowsperstrip = (uint32) -1;
static	int processCompressOptions(char*);

int	convert(void);
int	checksignature(void);
void	readscreen(void);
int	readgifimage(char*);
void	readextension(void);
int	readraster(void);
int	process(int, unsigned char**);
void	initcolors(unsigned char [COLSIZE][3], int);
void	rasterize(int, char*);

int
main(int argc, char* argv[])
{
    extern int optind;
    extern char *optarg;
    int c, status;

    while ((c = getopt(argc, argv, "c:r:")) != -1)
	    switch (c) {
	    case 'c':		/* compression scheme */
		    if (!processCompressOptions(optarg))
			    usage();
		    break;
	    case 'r':		/* rows/strip */
		    rowsperstrip = atoi(optarg);
		    break;
	    case '?':
		    usage();
		    /*NOTREACHED*/
	    }
    if (argc - optind != 2)
	    usage();

    makegamtab(GIFGAMMA);
    filename = argv[optind];
    imagename = argv[optind+1];
    if ((infile = fopen(imagename, "rb")) != NULL) {
	int c;
	fclose(infile);
	printf("overwrite %s? ", imagename); fflush(stdout);
	c = getc(stdin);
	if (c != 'y' && c != 'Y')
	    return (1);
    }
    if ((infile = fopen(filename, "rb")) == NULL) {
	perror(filename);
	return (1);
    }
    status = convert();
    fclose(infile);
    return (status);
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none"))
		compression = COMPRESSION_NONE;
	else if (streq(opt, "packbits"))
		compression = COMPRESSION_PACKBITS;
	else if (strneq(opt, "lzw", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_LZW;
	} else if (strneq(opt, "zip", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_DEFLATE;
	} else
		return (0);
	return (1);
}

int
convert(void)
{
    int ch;
    char* mode = "w";

    if (!checksignature())
        return (-1);
    readscreen();
    while ((ch = getc(infile)) != ';' && ch != EOF) {
        switch (ch) {
            case '\0':  break;  /* this kludge for non-standard files */
            case ',':   if (!readgifimage(mode))
                           return (-1);
			mode = "a";		/* subsequent images append */
                        break;
            case '!':   readextension();
                        break;
            default:    fprintf(stderr, "illegal GIF block type\n");
                        return (-1);
        }
    }
    return (0);
}

int
checksignature(void)
{
    char buf[6];

    fread(buf,1,6,infile);
    if (strncmp(buf,"GIF",3)) {
        fprintf(stderr, "file is not a GIF file\n");
        return 0;
    }
    if (strncmp(&buf[3],"87a",3)) {
        fprintf(stderr, "unknown GIF version number\n");
        return 0;
    }
    return 1;
}

/*
 * 	readscreen - 
 *		Get information which is global to all the images stored 
 *	in the file
 */
void
readscreen(void)
{
    unsigned char buf[7];

    fread(buf,1,7,infile);
    global = buf[4] & 0x80;
    if (global) {
        globalbits = (buf[4] & 0x07) + 1;
        fread(globalmap,3,((size_t)1)<<globalbits,infile);
    }
}

int
readgifimage(char* mode)
{
    unsigned char buf[9];
    int local, interleaved;
    unsigned char localmap[256][3];
    int localbits;
    int status;

    if (fread(buf, 1, 9, infile) == 0) {
        perror(filename);
	return (0);
    }
    width = buf[4] + (buf[5] << 8);
    height = buf[6] + (buf[7] << 8);
    local = buf[8] & 0x80;
    interleaved = buf[8] & 0x40;

    if (local == 0 && global == 0) {
        fprintf(stderr, "no colormap present for image\n");
        return (0);
    }
    if ((raster = (unsigned char*) _TIFFmalloc(width*height+EXTRAFUDGE)) == NULL) {
        fprintf(stderr, "not enough memory for image\n");
        return (0);
    }
    if (local) {
        localbits = (buf[8] & 0x7) + 1;

        fprintf(stderr, "   local colors: %d\n", 1<<localbits);

        fread(localmap, 3, ((size_t)1)<<localbits, infile);
        initcolors(localmap, 1<<localbits);
    } else if (global) {
        initcolors(globalmap, 1<<globalbits);
    }
    if ((status = readraster()))
	rasterize(interleaved, mode);
    _TIFFfree(raster);
    return status;
}

/*
 * 	readextension -
 *		Read a GIF extension block (and do nothing with it).
 *
 */
void
readextension(void)
{
    int count;
    char buf[255];

    (void) getc(infile);
    while ((count = getc(infile)))
        fread(buf, 1, count, infile);
}

/*
 * 	readraster -
 *		Decode a raster image
 *
 */
int
readraster(void)
{
    unsigned char *fill = raster;
    unsigned char buf[255];
    register int bits=0;
    register unsigned long datum=0;
    register unsigned char *ch;
    register int count, code;
    int status = 1;

    datasize = getc(infile);
    clear = 1 << datasize;
    eoi = clear + 1;
    avail = clear + 2;
    oldcode = -1;
    codesize = datasize + 1;
    codemask = (1 << codesize) - 1;
    for (code = 0; code < clear; code++) {
	prefix[code] = 0;
	suffix[code] = code;
    }
    stackp = stack;
    for (count = getc(infile); count > 0; count = getc(infile)) {
	fread(buf,1,count,infile);
	for (ch=buf; count-- > 0; ch++) {
	    datum += (unsigned long) *ch << bits;
	    bits += 8;
	    while (bits >= codesize) {
		code = datum & codemask;
		datum >>= codesize;
		bits -= codesize;
		if (code == eoi) {               /* This kludge put in */
		    goto exitloop;               /* because some GIF files*/
		}                                /* aren't standard */
		if (!process(code, &fill)) {
		    status = 0;
		    goto exitloop;
		}
	    }
	}
	if (fill >= raster + width*height) {
	    fprintf(stderr, "raster full before eoi code\n");
	    break;
	}
    }
exitloop:
    if (fill != raster + width*height)  {
	fprintf(stderr, "warning: wrong rastersize: %ld bytes\n",
						      (long) (fill-raster));
	fprintf(stderr, "         instead of %ld bytes\n",
						      (long) width*height);
    }
    return status;
}

/*
 * 	process - 
 *		Process a compression code.  "clear" resets the code table.  
 *	Otherwise make a new code table entry, and output the bytes 
 *	associated with the code.
 */
int
process(register int code, unsigned char** fill)
{
    int incode;
    static unsigned char firstchar;

    if (code == clear) {
	codesize = datasize + 1;
	codemask = (1 << codesize) - 1;
	avail = clear + 2;
	oldcode = -1;
	return 1;
    }

    if (oldcode == -1) {
	*(*fill)++ = suffix[code];
	firstchar = oldcode = code;
	return 1;
    }
    if (code > avail) {
	fprintf(stderr, "code %d too large for %d\n", code, avail);
	return 0; 
    }

    incode = code;
    if (code == avail) {      /* the first code is always < avail */
	*stackp++ = firstchar;
	code = oldcode;
    }
    while (code > clear) {
	*stackp++ = suffix[code];
	code = prefix[code];
    }

    *stackp++ = firstchar = suffix[code];
    prefix[avail] = oldcode;
    suffix[avail] = firstchar;
    avail++;

    if (((avail & codemask) == 0) && (avail < 4096)) {
	codesize++;
	codemask += avail;
    }
    oldcode = incode;
    do {
	*(*fill)++ = *--stackp;
    } while (stackp > stack);
    return 1;
}

/*
 * 	initcolors -
 *		Convert a color map (local or global) to arrays with R, G and B
 * 	values. 
 *
 */
void
initcolors(unsigned char colormap[COLSIZE][3], int ncolors)
{
    register int i;

    for (i = 0; i < ncolors; i++) {
        red[i]   = gamtab[colormap[i][0]];
        green[i] = gamtab[colormap[i][1]];
        blue[i]  = gamtab[colormap[i][2]];
    }
}

void
rasterize(int interleaved, char* mode)
{
    register unsigned long row;
    unsigned char *newras;
    unsigned char *ras;
    TIFF *tif;
    tstrip_t strip;
    tsize_t stripsize;

    if ((newras = (unsigned char*) _TIFFmalloc(width*height+EXTRAFUDGE)) == NULL) {
        fprintf(stderr, "not enough memory for image\n");
        return;
    }
#define DRAWSEGMENT(offset, step) {			\
        for (row = offset; row < height; row += step) {	\
            _TIFFmemcpy(newras + row*width, ras, width);\
            ras += width;                            	\
        }						\
    }
    ras = raster;
    if (interleaved) {
        DRAWSEGMENT(0, 8);
        DRAWSEGMENT(4, 8);
        DRAWSEGMENT(2, 4);
        DRAWSEGMENT(1, 2);
    } else 
        DRAWSEGMENT(0, 1);
#undef DRAWSEGMENT

    tif = TIFFOpen(imagename, mode);
    if (!tif) {
	TIFFError(imagename,"Can not open output image");
	exit(-1);
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32) width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32) height);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 
	rowsperstrip = TIFFDefaultStripSize(tif, rowsperstrip));
    TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);
    switch (compression) {
    case COMPRESSION_LZW:
    case COMPRESSION_DEFLATE:
	    if (predictor != 0)
		    TIFFSetField(tif, TIFFTAG_PREDICTOR, predictor);
	    break;
    }
    TIFFSetField(tif, TIFFTAG_COLORMAP, red, green, blue);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    strip = 0;
    stripsize = TIFFStripSize(tif);
    for (row=0; row<height; row += rowsperstrip) {
	if (rowsperstrip > height-row) {
	    rowsperstrip = height-row;
	    stripsize = TIFFVStripSize(tif, rowsperstrip);
	}
	if (TIFFWriteEncodedStrip(tif, strip, newras+row*width, stripsize) < 0)
	    break;
	strip++;
    }
    TIFFClose(tif);

    _TIFFfree(newras);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
