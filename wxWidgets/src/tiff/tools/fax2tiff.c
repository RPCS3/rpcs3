
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
 * Convert a CCITT Group 3 or 4 FAX file to TIFF Group 3 or 4 format.
 */
#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>		/* should have atof & getopt */

#ifdef HAVE_UNISTD_H
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

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS	0
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE	1
#endif

#define TIFFhowmany8(x) (((x)&0x07)?((uint32)(x)>>3)+1:(uint32)(x)>>3)

TIFF	*faxTIFF;
char	*rowbuf;
char	*refbuf;

uint32	xsize = 1728;
int	verbose;
int	stretch;
uint16	badfaxrun;
uint32	badfaxlines;

int	copyFaxFile(TIFF* tifin, TIFF* tifout);
static	void usage(void);

int
main(int argc, char* argv[])
{
	FILE *in;
	TIFF *out = NULL;
	TIFFErrorHandler whandler = NULL;
	int compression_in = COMPRESSION_CCITTFAX3;
	int compression_out = COMPRESSION_CCITTFAX3;
	int fillorder_in = FILLORDER_LSB2MSB;
	int fillorder_out = FILLORDER_LSB2MSB;
	uint32 group3options_in = 0;	/* 1d-encoded */
	uint32 group3options_out = 0;	/* 1d-encoded */
	uint32 group4options_in = 0;	/* compressed */
	uint32 group4options_out = 0;	/* compressed */
	uint32 defrowsperstrip = (uint32) 0;
	uint32 rowsperstrip;
	int photometric_in = PHOTOMETRIC_MINISWHITE;
	int photometric_out = PHOTOMETRIC_MINISWHITE;
	int mode = FAXMODE_CLASSF;
	int rows;
	int c;
	int pn, npages;
	float resY = 196.0;
	extern int optind;
	extern char* optarg;


	while ((c = getopt(argc, argv, "R:X:o:1234ABLMPUW5678abcflmprsuvwz?")) != -1)
		switch (c) {
			/* input-related options */
		case '3':		/* input is g3-encoded */
			compression_in = COMPRESSION_CCITTFAX3;
			break;
		case '4':		/* input is g4-encoded */
			compression_in = COMPRESSION_CCITTFAX4;
			break;
		case 'U':		/* input is uncompressed (g3 and g4) */
			group3options_in |= GROUP3OPT_UNCOMPRESSED;
			group4options_in |= GROUP4OPT_UNCOMPRESSED;
			break;
		case '1':		/* input is 1d-encoded (g3 only) */
			group3options_in &= ~GROUP3OPT_2DENCODING;
			break;
		case '2':		/* input is 2d-encoded (g3 only) */
			group3options_in |= GROUP3OPT_2DENCODING;
			break;
		case 'P':	/* input has not-aligned EOL (g3 only) */
			group3options_in &= ~GROUP3OPT_FILLBITS;
			break;
		case 'A':		/* input has aligned EOL (g3 only) */
			group3options_in |= GROUP3OPT_FILLBITS;
			break;
		case 'W':		/* input has 0 mean white */
			photometric_in = PHOTOMETRIC_MINISWHITE;
			break;
		case 'B':		/* input has 0 mean black */
			photometric_in = PHOTOMETRIC_MINISBLACK;
			break;
		case 'L':		/* input has lsb-to-msb fillorder */
			fillorder_in = FILLORDER_LSB2MSB;
			break;
		case 'M':		/* input has msb-to-lsb fillorder */
			fillorder_in = FILLORDER_MSB2LSB;
			break;
		case 'R':		/* input resolution */
			resY = (float) atof(optarg);
			break;
		case 'X':		/* input width */
			xsize = (uint32) atoi(optarg);
			break;

			/* output-related options */
		case '7':		/* generate g3-encoded output */
			compression_out = COMPRESSION_CCITTFAX3;
			break;
		case '8':		/* generate g4-encoded output */
			compression_out = COMPRESSION_CCITTFAX4;
			break;
		case 'u':	/* generate uncompressed output (g3 and g4) */
			group3options_out |= GROUP3OPT_UNCOMPRESSED;
			group4options_out |= GROUP4OPT_UNCOMPRESSED;
			break;
		case '5':	/* generate 1d-encoded output (g3 only) */
			group3options_out &= ~GROUP3OPT_2DENCODING;
			break;
		case '6':	/* generate 2d-encoded output (g3 only) */
			group3options_out |= GROUP3OPT_2DENCODING;
			break;
		case 'c':		/* generate "classic" g3 format */
			mode = FAXMODE_CLASSIC;
			break;
		case 'f':		/* generate Class F format */
			mode = FAXMODE_CLASSF;
			break;
		case 'm':		/* output's fillorder is msb-to-lsb */
			fillorder_out = FILLORDER_MSB2LSB;
			break;
		case 'l':		/* output's fillorder is lsb-to-msb */
			fillorder_out = FILLORDER_LSB2MSB;
			break;
		case 'o':
			out = TIFFOpen(optarg, "w");
			if (out == NULL) {
				fprintf(stderr,
				    "%s: Can not create or open %s\n",
				    argv[0], optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'a':	/* generate EOL-aligned output (g3 only) */
			group3options_out |= GROUP3OPT_FILLBITS;
			break;
		case 'p':	/* generate not EOL-aligned output (g3 only) */
			group3options_out &= ~GROUP3OPT_FILLBITS;
			break;
		case 'r':		/* rows/strip */
			defrowsperstrip = atol(optarg);
			break;
		case 's':		/* stretch image by dup'ng scanlines */
			stretch = 1;
			break;
		case 'w':		/* undocumented -- for testing */
			photometric_out = PHOTOMETRIC_MINISWHITE;
			break;
		case 'b':		/* undocumented -- for testing */
			photometric_out = PHOTOMETRIC_MINISBLACK;
			break;
		case 'z':		/* undocumented -- for testing */
			compression_out = COMPRESSION_LZW;
			break;
		case 'v':		/* -v for info */
			verbose++;
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}
	npages = argc - optind;
	if (npages < 1)
		usage();

	rowbuf = _TIFFmalloc(TIFFhowmany8(xsize));
	refbuf = _TIFFmalloc(TIFFhowmany8(xsize));
	if (rowbuf == NULL || refbuf == NULL) {
		fprintf(stderr, "%s: Not enough memory\n", argv[0]);
		return (EXIT_FAILURE);
	}

	if (out == NULL) {
		out = TIFFOpen("fax.tif", "w");
		if (out == NULL) {
			fprintf(stderr, "%s: Can not create fax.tif\n",
			    argv[0]);
			return (EXIT_FAILURE);
		}
	}
		
	faxTIFF = TIFFClientOpen("(FakeInput)", "w",
	/* TIFFClientOpen() fails if we don't set existing value here */
				 TIFFClientdata(out),
				 TIFFGetReadProc(out), TIFFGetWriteProc(out),
				 TIFFGetSeekProc(out), TIFFGetCloseProc(out),
				 TIFFGetSizeProc(out), TIFFGetMapFileProc(out),
				 TIFFGetUnmapFileProc(out));
	if (faxTIFF == NULL) {
		fprintf(stderr, "%s: Can not create fake input file\n",
		    argv[0]);
		return (EXIT_FAILURE);
	}
	TIFFSetMode(faxTIFF, O_RDONLY);
	TIFFSetField(faxTIFF, TIFFTAG_IMAGEWIDTH,	xsize);
	TIFFSetField(faxTIFF, TIFFTAG_SAMPLESPERPIXEL,	1);
	TIFFSetField(faxTIFF, TIFFTAG_BITSPERSAMPLE,	1);
	TIFFSetField(faxTIFF, TIFFTAG_FILLORDER,	fillorder_in);
	TIFFSetField(faxTIFF, TIFFTAG_PLANARCONFIG,	PLANARCONFIG_CONTIG);
	TIFFSetField(faxTIFF, TIFFTAG_PHOTOMETRIC,	photometric_in);
	TIFFSetField(faxTIFF, TIFFTAG_YRESOLUTION,	resY);
	TIFFSetField(faxTIFF, TIFFTAG_RESOLUTIONUNIT,	RESUNIT_INCH);
	
	/* NB: this must be done after directory info is setup */
	TIFFSetField(faxTIFF, TIFFTAG_COMPRESSION, compression_in);
	if (compression_in == COMPRESSION_CCITTFAX3)
		TIFFSetField(faxTIFF, TIFFTAG_GROUP3OPTIONS, group3options_in);
	else if (compression_in == COMPRESSION_CCITTFAX4)
		TIFFSetField(faxTIFF, TIFFTAG_GROUP4OPTIONS, group4options_in);
	for (pn = 0; optind < argc; pn++, optind++) {
		in = fopen(argv[optind], "rb");
		if (in == NULL) {
			fprintf(stderr,
			    "%s: %s: Can not open\n", argv[0], argv[optind]);
			continue;
		}
#if defined(_WIN32) && defined(USE_WIN32_FILEIO)
                TIFFSetClientdata(faxTIFF, (thandle_t)_get_osfhandle(fileno(in)));
#else
                TIFFSetClientdata(faxTIFF, (thandle_t)fileno(in));
#endif
		TIFFSetFileName(faxTIFF, (const char*)argv[optind]);
		TIFFSetField(out, TIFFTAG_IMAGEWIDTH, xsize);
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 1);
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression_out);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric_out);
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
		switch (compression_out) {
			/* g3 */
			case COMPRESSION_CCITTFAX3:
			TIFFSetField(out, TIFFTAG_GROUP3OPTIONS,
				     group3options_out);
			TIFFSetField(out, TIFFTAG_FAXMODE, mode);
			rowsperstrip =
				(defrowsperstrip)?defrowsperstrip:(uint32)-1L;
			break;

			/* g4 */
			case COMPRESSION_CCITTFAX4:
			TIFFSetField(out, TIFFTAG_GROUP4OPTIONS,
				     group4options_out);
			TIFFSetField(out, TIFFTAG_FAXMODE, mode);
			rowsperstrip =
				(defrowsperstrip)?defrowsperstrip:(uint32)-1L;
			break;

			default:
			rowsperstrip = (defrowsperstrip) ?
				defrowsperstrip : TIFFDefaultStripSize(out, 0);
		}
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(out, TIFFTAG_FILLORDER, fillorder_out);
		TIFFSetField(out, TIFFTAG_SOFTWARE, "fax2tiff");
		TIFFSetField(out, TIFFTAG_XRESOLUTION, 204.0);
		if (!stretch) {
			TIFFGetField(faxTIFF, TIFFTAG_YRESOLUTION, &resY);
			TIFFSetField(out, TIFFTAG_YRESOLUTION, resY);
		} else
			TIFFSetField(out, TIFFTAG_YRESOLUTION, 196.);
		TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
		TIFFSetField(out, TIFFTAG_PAGENUMBER, pn, npages);

		if (!verbose)
		    whandler = TIFFSetWarningHandler(NULL);
		rows = copyFaxFile(faxTIFF, out);
		fclose(in);
		if (!verbose)
		    (void) TIFFSetWarningHandler(whandler);

		TIFFSetField(out, TIFFTAG_IMAGELENGTH, rows);

		if (verbose) {
			fprintf(stderr, "%s:\n", argv[optind]);
			fprintf(stderr, "%d rows in input\n", rows);
			fprintf(stderr, "%ld total bad rows\n",
			    (long) badfaxlines);
			fprintf(stderr, "%d max consecutive bad rows\n", badfaxrun);
		}
		if (compression_out == COMPRESSION_CCITTFAX3 &&
		    mode == FAXMODE_CLASSF) {
			TIFFSetField(out, TIFFTAG_BADFAXLINES, badfaxlines);
			TIFFSetField(out, TIFFTAG_CLEANFAXDATA, badfaxlines ?
			    CLEANFAXDATA_REGENERATED : CLEANFAXDATA_CLEAN);
			TIFFSetField(out, TIFFTAG_CONSECUTIVEBADFAXLINES, badfaxrun);
		}
		TIFFWriteDirectory(out);
	}
	TIFFClose(out);
	_TIFFfree(rowbuf);
	_TIFFfree(refbuf);
	return (EXIT_SUCCESS);
}

int
copyFaxFile(TIFF* tifin, TIFF* tifout)
{
	uint32 row;
	uint32 linesize = TIFFhowmany8(xsize);
	uint16 badrun;
	int ok;

	tifin->tif_rawdatasize = (tmsize_t)TIFFGetFileSize(tifin);
	tifin->tif_rawdata = _TIFFmalloc(tifin->tif_rawdatasize);
	if (tifin->tif_rawdata == NULL) {
		TIFFError(tifin->tif_name, "Not enough memory");
		return (0);
	}
	if (!ReadOK(tifin, tifin->tif_rawdata, tifin->tif_rawdatasize)) {
		TIFFError(tifin->tif_name, "Read error at scanline 0");
		return (0);
	}
	tifin->tif_rawcp = tifin->tif_rawdata;
	tifin->tif_rawcc = tifin->tif_rawdatasize;

	(*tifin->tif_setupdecode)(tifin);
	(*tifin->tif_predecode)(tifin, (tsample_t) 0);
	tifin->tif_row = 0;
	badfaxlines = 0;
	badfaxrun = 0;

	_TIFFmemset(refbuf, 0, linesize);
	row = 0;
	badrun = 0;		/* current run of bad lines */
	while (tifin->tif_rawcc > 0) {
		ok = (*tifin->tif_decoderow)(tifin, (tdata_t) rowbuf, 
					     linesize, 0);
		if (!ok) {
			badfaxlines++;
			badrun++;
			/* regenerate line from previous good line */
			_TIFFmemcpy(rowbuf, refbuf, linesize);
		} else {
			if (badrun > badfaxrun)
				badfaxrun = badrun;
			badrun = 0;
			_TIFFmemcpy(refbuf, rowbuf, linesize);
		}
		tifin->tif_row++;

		if (TIFFWriteScanline(tifout, rowbuf, row, 0) < 0) {
			fprintf(stderr, "%s: Write error at row %ld.\n",
			    tifout->tif_name, (long) row);
			break;
		}
		row++;
		if (stretch) {
			if (TIFFWriteScanline(tifout, rowbuf, row, 0) < 0) {
				fprintf(stderr, "%s: Write error at row %ld.\n",
				    tifout->tif_name, (long) row);
				break;
			}
			row++;
		}
	}
	if (badrun > badfaxrun)
		badfaxrun = badrun;
	_TIFFfree(tifin->tif_rawdata);
	return (row);
}

char* stuff[] = {
"usage: fax2tiff [options] input.raw...",
"where options are:",
" -3		input data is G3-encoded		[default]",
" -4		input data is G4-encoded",
" -U		input data is uncompressed (G3 or G4)",
" -1		input data is 1D-encoded (G3 only)	[default]",
" -2		input data is 2D-encoded (G3 only)",
" -P		input is not EOL-aligned (G3 only)	[default]",
" -A		input is EOL-aligned (G3 only)",
" -M		input data has MSB2LSB bit order",
" -L		input data has LSB2MSB bit order	[default]",
" -B		input data has min 0 means black",
" -W		input data has min 0 means white	[default]",
" -R #		input data has # resolution (lines/inch) [default is 196]",
" -X #		input data has # width			[default is 1728]",
"",
" -o out.tif	write output to out.tif",
" -7		generate G3-encoded output		[default]",
" -8		generate G4-encoded output",
" -u		generate uncompressed output (G3 or G4)",
" -5		generate 1D-encoded output (G3 only)",
" -6		generate 2D-encoded output (G3 only)	[default]",
" -p		generate not EOL-aligned output (G3 only)",
" -a		generate EOL-aligned output (G3 only)	[default]",
" -c		generate \"classic\" TIFF format",
" -f		generate TIFF Class F (TIFF/F) format	[default]",
" -m		output fill order is MSB2LSB",
" -l		output fill order is LSB2MSB		[default]",
" -r #		make each strip have no more than # rows",
" -s		stretch image by duplicating scanlines",
" -v		print information about conversion work",
" -z		generate LZW compressed output",
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
	exit(EXIT_FAILURE);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
