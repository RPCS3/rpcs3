
/*
 * Copyright (c) 1992-1997 Sam Leffler
 * Copyright (c) 1992-1997 Silicon Graphics, Inc.
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

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)

#define PATH_LENGTH 8192

static const char TIFF_SUFFIX[] = ".tif";

static	char fname[PATH_LENGTH];

static	int tiffcp(TIFF*, TIFF*);
static	void newfilename(void);
static	int cpStrips(TIFF*, TIFF*);
static	int cpTiles(TIFF*, TIFF*);

int
main(int argc, char* argv[])
{
	TIFF *in, *out;

	if (argc < 2) {
                fprintf(stderr, "%s\n\n", TIFFGetVersion());
		fprintf(stderr, "usage: tiffsplit input.tif [prefix]\n");
		return (-3);
	}
	if (argc > 2) {
		strncpy(fname, argv[2], sizeof(fname));
		fname[sizeof(fname) - 1] = '\0';
	}
	in = TIFFOpen(argv[1], "r");
	if (in != NULL) {
		do {
			size_t path_len;
			char *path;
			
			newfilename();

			path_len = strlen(fname) + sizeof(TIFF_SUFFIX);
			path = (char *) _TIFFmalloc(path_len);
			strncpy(path, fname, path_len);
			path[path_len - 1] = '\0';
			strncat(path, TIFF_SUFFIX, path_len - strlen(path) - 1);
			out = TIFFOpen(path, TIFFIsBigEndian(in)?"wb":"wl");
			_TIFFfree(path);

			if (out == NULL)
				return (-2);
			if (!tiffcp(in, out))
				return (-1);
			TIFFClose(out);
		} while (TIFFReadDirectory(in));
		(void) TIFFClose(in);
	}
	return (0);
}

static void
newfilename(void)
{
	static int first = 1;
	static long lastTurn;
	static long fnum;
	static short defname;
	static char *fpnt;

	if (first) {
		if (fname[0]) {
			fpnt = fname + strlen(fname);
			defname = 0;
		} else {
			fname[0] = 'x';
			fpnt = fname + 1;
			defname = 1;
		}
		first = 0;
	}
#define	MAXFILES	17576
	if (fnum == MAXFILES) {
		if (!defname || fname[0] == 'z') {
			fprintf(stderr, "tiffsplit: too many files.\n");
			exit(1);
		}
		fname[0]++;
		fnum = 0;
	}
	if (fnum % 676 == 0) {
		if (fnum != 0) {
			/*
                         * advance to next letter every 676 pages
			 * condition for 'z'++ will be covered above
                         */
			fpnt[0]++;
		} else {
			/*
                         * set to 'a' if we are on the very first file
                         */
			fpnt[0] = 'a';
		}
		/*
                 * set the value of the last turning point
                 */
		lastTurn = fnum;
	}
	/* 
         * start from 0 every 676 times (provided by lastTurn)
         * this keeps us within a-z boundaries
         */
	fpnt[1] = (char)((fnum - lastTurn) / 26) + 'a';
	/* 
         * cycle last letter every file, from a-z, then repeat
         */
	fpnt[2] = (char)(fnum % 26) + 'a';
	fnum++;
}

static int
tiffcp(TIFF* in, TIFF* out)
{
	uint16 bitspersample, samplesperpixel, compression, shortv, *shortav;
	uint32 w, l;
	float floatv;
	char *stringv;
	uint32 longv;

	CopyField(TIFFTAG_SUBFILETYPE, longv);
	CopyField(TIFFTAG_TILEWIDTH, w);
	CopyField(TIFFTAG_TILELENGTH, l);
	CopyField(TIFFTAG_IMAGEWIDTH, w);
	CopyField(TIFFTAG_IMAGELENGTH, l);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_COMPRESSION, compression);
	if (compression == COMPRESSION_JPEG) {
		uint32 count = 0;
		void *table = NULL;
		if (TIFFGetField(in, TIFFTAG_JPEGTABLES, &count, &table)
		    && count > 0 && table) {
		    TIFFSetField(out, TIFFTAG_JPEGTABLES, count, table);
		}
	}
        CopyField(TIFFTAG_PHOTOMETRIC, shortv);
	CopyField(TIFFTAG_PREDICTOR, shortv);
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_GROUP3OPTIONS, longv);
	CopyField(TIFFTAG_GROUP4OPTIONS, longv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, shortv);
	CopyField(TIFFTAG_ROWSPERSTRIP, longv);
	CopyField(TIFFTAG_XPOSITION, floatv);
	CopyField(TIFFTAG_YPOSITION, floatv);
	CopyField(TIFFTAG_IMAGEDEPTH, longv);
	CopyField(TIFFTAG_TILEDEPTH, longv);
	CopyField(TIFFTAG_SAMPLEFORMAT, shortv);
	CopyField2(TIFFTAG_EXTRASAMPLES, shortv, shortav);
	{ uint16 *red, *green, *blue;
	  CopyField3(TIFFTAG_COLORMAP, red, green, blue);
	}
	{ uint16 shortv2;
	  CopyField2(TIFFTAG_PAGENUMBER, shortv, shortv2);
	}
	CopyField(TIFFTAG_ARTIST, stringv);
	CopyField(TIFFTAG_IMAGEDESCRIPTION, stringv);
	CopyField(TIFFTAG_MAKE, stringv);
	CopyField(TIFFTAG_MODEL, stringv);
	CopyField(TIFFTAG_SOFTWARE, stringv);
	CopyField(TIFFTAG_DATETIME, stringv);
	CopyField(TIFFTAG_HOSTCOMPUTER, stringv);
	CopyField(TIFFTAG_PAGENAME, stringv);
	CopyField(TIFFTAG_DOCUMENTNAME, stringv);
	CopyField(TIFFTAG_BADFAXLINES, longv);
	CopyField(TIFFTAG_CLEANFAXDATA, longv);
	CopyField(TIFFTAG_CONSECUTIVEBADFAXLINES, longv);
	CopyField(TIFFTAG_FAXRECVPARAMS, longv);
	CopyField(TIFFTAG_FAXRECVTIME, longv);
	CopyField(TIFFTAG_FAXSUBADDRESS, stringv);
	CopyField(TIFFTAG_FAXDCS, stringv);
	if (TIFFIsTiled(in))
		return (cpTiles(in, out));
	else
		return (cpStrips(in, out));
}

static int
cpStrips(TIFF* in, TIFF* out)
{
	tmsize_t bufsize  = TIFFStripSize(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		tstrip_t s, ns = TIFFNumberOfStrips(in);
		uint64 *bytecounts;

		if (!TIFFGetField(in, TIFFTAG_STRIPBYTECOUNTS, &bytecounts)) {
			fprintf(stderr, "tiffsplit: strip byte counts are missing\n");
			return (0);
		}
		for (s = 0; s < ns; s++) {
			if (bytecounts[s] > (uint64)bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, (tmsize_t)bytecounts[s]);
				if (!buf)
					return (0);
				bufsize = (tmsize_t)bytecounts[s];
			}
			if (TIFFReadRawStrip(in, s, buf, (tmsize_t)bytecounts[s]) < 0 ||
			    TIFFWriteRawStrip(out, s, buf, (tmsize_t)bytecounts[s]) < 0) {
				_TIFFfree(buf);
				return (0);
			}
		}
		_TIFFfree(buf);
		return (1);
	}
	return (0);
}

static int
cpTiles(TIFF* in, TIFF* out)
{
	tmsize_t bufsize = TIFFTileSize(in);
	unsigned char *buf = (unsigned char *)_TIFFmalloc(bufsize);

	if (buf) {
		ttile_t t, nt = TIFFNumberOfTiles(in);
		uint64 *bytecounts;

		if (!TIFFGetField(in, TIFFTAG_TILEBYTECOUNTS, &bytecounts)) {
			fprintf(stderr, "tiffsplit: tile byte counts are missing\n");
			return (0);
		}
		for (t = 0; t < nt; t++) {
			if (bytecounts[t] > (uint64) bufsize) {
				buf = (unsigned char *)_TIFFrealloc(buf, (tmsize_t)bytecounts[t]);
				if (!buf)
					return (0);
				bufsize = (tmsize_t)bytecounts[t];
			}
			if (TIFFReadRawTile(in, t, buf, (tmsize_t)bytecounts[t]) < 0 ||
			    TIFFWriteRawTile(out, t, buf, (tmsize_t)bytecounts[t]) < 0) {
				_TIFFfree(buf);
				return (0);
			}
		}
		_TIFFfree(buf);
		return (1);
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
