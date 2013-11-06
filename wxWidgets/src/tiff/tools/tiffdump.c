
/*
 * Copyright (c) 1988-1997 Sam Leffler
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

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#include "tiffio.h"

#ifndef O_BINARY
# define O_BINARY	0
#endif

static union
{
	TIFFHeaderClassic classic;
	TIFFHeaderBig big;
	TIFFHeaderCommon common;
} hdr;
char* appname;
char* curfile;
int swabflag;
int bigendian;
int bigtiff;
uint32 maxitems = 24;   /* maximum indirect data items to print */

const char* bytefmt = "%s%#02x";	/* BYTE */
const char* sbytefmt = "%s%d";		/* SBYTE */
const char* shortfmt = "%s%u";		/* SHORT */
const char* sshortfmt = "%s%d";		/* SSHORT */
const char* longfmt = "%s%lu";		/* LONG */
const char* slongfmt = "%s%ld";		/* SLONG */
const char* ifdfmt = "%s%#04lx";	/* IFD offset */
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
const char* long8fmt = "%s%I64u";	/* LONG8 */
const char* slong8fmt = "%s%I64d";	/* SLONG8 */
const char* ifd8fmt = "%s%#08I64x";	/* IFD offset8*/
#else
const char* long8fmt = "%s%llu";	/* LONG8 */
const char* slong8fmt = "%s%lld";	/* SLONG8 */
const char* ifd8fmt = "%s%#08llx";	/* IFD offset8*/
#endif
const char* rationalfmt = "%s%g";	/* RATIONAL */
const char* srationalfmt = "%s%g";	/* SRATIONAL */
const char* floatfmt = "%s%g";		/* FLOAT */
const char* doublefmt = "%s%g";		/* DOUBLE */

static void dump(int, uint64);
extern int optind;
extern char* optarg;

void
usage()
{
	fprintf(stderr, "usage: %s [-h] [-o offset] [-m maxitems] file.tif ...\n", appname);
	exit(-1);
}

int
main(int argc, char* argv[])
{
	int one = 1, fd;
	int multiplefiles = (argc > 1);
	int c;
	uint64 diroff = 0;
	bigendian = (*(char *)&one == 0);

	appname = argv[0];
	while ((c = getopt(argc, argv, "m:o:h")) != -1) {
		switch (c) {
		case 'h':			/* print values in hex */
			shortfmt = "%s%#x";
			sshortfmt = "%s%#x";
			longfmt = "%s%#lx";
			slongfmt = "%s%#lx";
			break;
		case 'o':
			diroff = (uint64) strtoul(optarg, NULL, 0);
			break;
		case 'm':
			maxitems = strtoul(optarg, NULL, 0);
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();
	for (; optind < argc; optind++) {
		fd = open(argv[optind], O_RDONLY|O_BINARY, 0);
		if (fd < 0) {
			perror(argv[0]);
			return (-1);
		}
		if (multiplefiles)
			printf("%s:\n", argv[optind]);
		curfile = argv[optind];
		swabflag = 0;
		bigtiff = 0;
		dump(fd, diroff);
		close(fd);
	}
	return (0);
}

#define ord(e) ((int)e)

static uint64 ReadDirectory(int, unsigned, uint64);
static void ReadError(char*);
static void Error(const char*, ...);
static void Fatal(const char*, ...);

static void
dump(int fd, uint64 diroff)
{
	unsigned i;

	lseek(fd, (off_t) 0, 0);
	if (read(fd, (char*) &hdr, sizeof (TIFFHeaderCommon)) != sizeof (TIFFHeaderCommon))
		ReadError("TIFF header");
	if (hdr.common.tiff_magic != TIFF_BIGENDIAN
	    && hdr.common.tiff_magic != TIFF_LITTLEENDIAN &&
#if HOST_BIGENDIAN
	    /* MDI is sensitive to the host byte order, unlike TIFF */
	    MDI_BIGENDIAN != hdr.common.tiff_magic
#else
	    MDI_LITTLEENDIAN != hdr.common.tiff_magic
#endif
	   ) {
		Fatal("Not a TIFF or MDI file, bad magic number %u (%#x)",
		    hdr.common.tiff_magic, hdr.common.tiff_magic);
	}
	if (hdr.common.tiff_magic == TIFF_BIGENDIAN
	    || hdr.common.tiff_magic == MDI_BIGENDIAN)
		swabflag = !bigendian;
	else
		swabflag = bigendian;
	if (swabflag)
		TIFFSwabShort(&hdr.common.tiff_version);
	if (hdr.common.tiff_version==42)
	{
		if (read(fd, (char*) &hdr.classic.tiff_diroff, 4) != 4)
			ReadError("TIFF header");
		if (swabflag)
			TIFFSwabLong(&hdr.classic.tiff_diroff);
		printf("Magic: %#x <%s-endian> Version: %#x <%s>\n",
		    hdr.classic.tiff_magic,
		    hdr.classic.tiff_magic == TIFF_BIGENDIAN ? "big" : "little",
		    42,"ClassicTIFF");
		if (diroff == 0)
			diroff = hdr.classic.tiff_diroff;
	}
	else if (hdr.common.tiff_version==43)
	{
		if (read(fd, (char*) &hdr.big.tiff_offsetsize, 12) != 12)
			ReadError("TIFF header");
		if (swabflag)
		{
			TIFFSwabShort(&hdr.big.tiff_offsetsize);
			TIFFSwabShort(&hdr.big.tiff_unused);
			TIFFSwabLong8(&hdr.big.tiff_diroff);
		}
		printf("Magic: %#x <%s-endian> Version: %#x <%s>\n",
		    hdr.big.tiff_magic,
		    hdr.big.tiff_magic == TIFF_BIGENDIAN ? "big" : "little",
		    43,"BigTIFF");
		printf("OffsetSize: %#x Unused: %#x\n",
		    hdr.big.tiff_offsetsize,hdr.big.tiff_unused);
		if (diroff == 0)
			diroff = hdr.big.tiff_diroff;
		bigtiff = 1;
	}
	else
		Fatal("Not a TIFF file, bad version number %u (%#x)",
		    hdr.common.tiff_version, hdr.common.tiff_version);
	for (i = 0; diroff != 0; i++) {
		if (i > 0)
			putchar('\n');
		diroff = ReadDirectory(fd, i, diroff);
	}
}

static const int datawidth[] = {
	0, /* 00 = undefined */
	1, /* 01 = TIFF_BYTE */
	1, /* 02 = TIFF_ASCII */
	2, /* 03 = TIFF_SHORT */
	4, /* 04 = TIFF_LONG */
	8, /* 05 = TIFF_RATIONAL */
	1, /* 06 = TIFF_SBYTE */
	1, /* 07 = TIFF_UNDEFINED */
	2, /* 08 = TIFF_SSHORT */
	4, /* 09 = TIFF_SLONG */
	8, /* 10 = TIFF_SRATIONAL */
	4, /* 11 = TIFF_FLOAT */
	8, /* 12 = TIFF_DOUBLE */
	4, /* 13 = TIFF_IFD */
	0, /* 14 = undefined */
	0, /* 15 = undefined */
	8, /* 16 = TIFF_LONG8 */
	8, /* 17 = TIFF_SLONG8 */
	8, /* 18 = TIFF_IFD8 */
};
#define NWIDTHS (sizeof (datawidth) / sizeof (datawidth[0]))
static void PrintTag(FILE*, uint16);
static void PrintType(FILE*, uint16);
static void PrintData(FILE*, uint16, uint32, unsigned char*);

/*
 * Read the next TIFF directory from a file
 * and convert it to the internal format.
 * We read directories sequentially.
 */
static uint64
ReadDirectory(int fd, unsigned int ix, uint64 off)
{
	uint16 dircount;
	uint32 direntrysize;
	void* dirmem = NULL;
	uint64 nextdiroff = 0;
	uint32 n;
	uint8* dp;

	if (off == 0)			/* no more directories */
		goto done;
#if defined(__WIN32__) && defined(_MSC_VER)
	if (_lseeki64(fd, (__int64)off, SEEK_SET) != (__int64)off) {
#else
	if (lseek(fd, (off_t)off, SEEK_SET) != (off_t)off) {
#endif
		Fatal("Seek error accessing TIFF directory");
		goto done;
	}
	if (!bigtiff) {
		if (read(fd, (char*) &dircount, sizeof (uint16)) != sizeof (uint16)) {
			ReadError("directory count");
			goto done;
		}
		if (swabflag)
			TIFFSwabShort(&dircount);
		direntrysize = 12;
	} else {
		uint64 dircount64 = 0;
		if (read(fd, (char*) &dircount64, sizeof (uint64)) != sizeof (uint64)) {
			ReadError("directory count");
			goto done;
		}
		if (swabflag)
			TIFFSwabLong8(&dircount64);
		if (dircount64>0xFFFF) {
			Error("Sanity check on directory count failed");
			goto done;
		}
		dircount = (uint16)dircount64;
		direntrysize = 20;
	}
	dirmem = _TIFFmalloc(dircount * direntrysize);
	if (dirmem == NULL) {
		Fatal("No space for TIFF directory");
		goto done;
	}
	n = read(fd, (char*) dirmem, dircount*direntrysize);
	if (n != dircount*direntrysize) {
		n /= direntrysize;
		Error(
#if defined(__WIN32__) && defined(_MSC_VER)
	    "Could only read %lu of %u entries in directory at offset %#I64x",
		      (unsigned long)n, dircount, (unsigned __int64) off);
#else
	    "Could only read %lu of %u entries in directory at offset %#llx",
		      (unsigned long)n, dircount, (unsigned long long) off);
#endif
		dircount = n;
		nextdiroff = 0;
	} else {
		if (!bigtiff) {
			uint32 nextdiroff32;
			if (read(fd, (char*) &nextdiroff32, sizeof (uint32)) != sizeof (uint32))
				nextdiroff32 = 0;
			if (swabflag)
				TIFFSwabLong(&nextdiroff32);
			nextdiroff = nextdiroff32;
		} else {
			if (read(fd, (char*) &nextdiroff, sizeof (uint64)) != sizeof (uint64))
				nextdiroff = 0;
			if (swabflag)
				TIFFSwabLong8(&nextdiroff);
		}
	}
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
	printf("Directory %u: offset %I64u (%#I64x) next %I64u (%#I64x)\n", ix,
	    (unsigned __int64)off, (unsigned __int64)off,
	    (unsigned __int64)nextdiroff, (unsigned __int64)nextdiroff);
#else
	printf("Directory %u: offset %llu (%#llx) next %llu (%#llx)\n", ix,
	    (unsigned long long)off, (unsigned long long)off,
	    (unsigned long long)nextdiroff, (unsigned long long)nextdiroff);
#endif
	for (dp = (uint8*)dirmem, n = dircount; n > 0; n--) {
		uint16 tag;
		uint16 type;
		uint16 typewidth;
		uint64 count;
		uint64 datasize;
		int datafits;
		void* datamem;
		uint64 dataoffset;
		int datatruncated;
		tag = *(uint16*)dp;
		if (swabflag)
			TIFFSwabShort(&tag);
		dp += sizeof(uint16);
		type = *(uint16*)dp;
		dp += sizeof(uint16);
		if (swabflag)
			TIFFSwabShort(&type);
		PrintTag(stdout, tag);
		putchar(' ');
		PrintType(stdout, type);
		putchar(' ');
		if (!bigtiff)
		{
			uint32 count32;
			count32 = *(uint32*)dp;
			if (swabflag)
				TIFFSwabLong(&count32);
			dp += sizeof(uint32);
			count = count32;
		}
		else
		{
			count = *(uint64*)dp;
			if (swabflag)
				TIFFSwabLong8(&count);
			dp += sizeof(uint64);
		}
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
		printf("%I64u<", (unsigned __int64)count);
#else
		printf("%llu<", (unsigned long long)count);
#endif
		if (type >= NWIDTHS)
			typewidth = 0;
		else
			typewidth = datawidth[type];
		datasize = count*typewidth;
		datafits = 1;
		datamem = dp;
		dataoffset = 0;
		datatruncated = 0;
		if (!bigtiff)
		{
			if (datasize>4)
			{
				uint32 dataoffset32;
				datafits = 0;
				datamem = NULL;
				dataoffset32 = *(uint32*)dp;
				if (swabflag)
					TIFFSwabLong(&dataoffset32);
				dataoffset = dataoffset32;
			}
			dp += sizeof(uint32);
		}
		else
		{
			if (datasize>8)
			{
				datafits = 0;
				datamem = NULL;
				dataoffset = *(uint64*)dp;
				if (swabflag)
					TIFFSwabLong8(&dataoffset);
			}
			dp += sizeof(uint64);
		}
		if (datasize>0x10000)
		{
			datatruncated = 1;
			count = 0x10000/typewidth;
			datasize = count*typewidth;
		}
		if (count>maxitems)
		{
			datatruncated = 1;
			count = maxitems;
			datasize = count*typewidth;
		}
		if (!datafits)
		{
			datamem = _TIFFmalloc((uint32)datasize);
			if (datamem) {
#if defined(__WIN32__) && defined(_MSC_VER)
				if (_lseeki64(fd, (__int64)dataoffset, SEEK_SET)
				    != (__int64)dataoffset)
#else
				if (lseek(fd, (off_t)dataoffset, 0) !=
				    (off_t)dataoffset)
#endif
				{
					Error(
				"Seek error accessing tag %u value", tag);
					_TIFFfree(datamem);
					datamem = NULL;
				}
				if (read(fd, datamem, (size_t)datasize) != (TIFF_SSIZE_T)datasize)
				{
					Error(
				"Read error accessing tag %u value", tag);
					_TIFFfree(datamem);
					datamem = NULL;
				}
			} else
				Error("No space for data for tag %u",tag);
		}
		if (datamem)
		{
			if (swabflag)
			{
				switch (type)
				{
					case TIFF_BYTE:
					case TIFF_ASCII:
					case TIFF_SBYTE:
					case TIFF_UNDEFINED:
						break;
					case TIFF_SHORT:
					case TIFF_SSHORT:
						TIFFSwabArrayOfShort((uint16*)datamem,(tmsize_t)count);
						break;
					case TIFF_LONG:
					case TIFF_SLONG:
					case TIFF_FLOAT:
					case TIFF_IFD:
						TIFFSwabArrayOfLong((uint32*)datamem,(tmsize_t)count);
						break;
					case TIFF_RATIONAL:
					case TIFF_SRATIONAL:
						TIFFSwabArrayOfLong((uint32*)datamem,(tmsize_t)count*2);
						break;
					case TIFF_DOUBLE:
					case TIFF_LONG8:
					case TIFF_SLONG8:
					case TIFF_IFD8:
						TIFFSwabArrayOfLong8((uint64*)datamem,(tmsize_t)count);
						break;
				}
			}
			PrintData(stdout,type,(uint32)count,datamem);
			if (datatruncated)
				printf(" ...");
			if (!datafits)
				_TIFFfree(datamem);
		}
		printf(">\n");
	}
done:
	if (dirmem)
		_TIFFfree((char *)dirmem);
	return (nextdiroff);
}

static const struct tagname {
	uint16 tag;
	const char* name;
} tagnames[] = {
    { TIFFTAG_SUBFILETYPE,	"SubFileType" },
    { TIFFTAG_OSUBFILETYPE,	"OldSubFileType" },
    { TIFFTAG_IMAGEWIDTH,	"ImageWidth" },
    { TIFFTAG_IMAGELENGTH,	"ImageLength" },
    { TIFFTAG_BITSPERSAMPLE,	"BitsPerSample" },
    { TIFFTAG_COMPRESSION,	"Compression" },
    { TIFFTAG_PHOTOMETRIC,	"Photometric" },
    { TIFFTAG_THRESHHOLDING,	"Threshholding" },
    { TIFFTAG_CELLWIDTH,	"CellWidth" },
    { TIFFTAG_CELLLENGTH,	"CellLength" },
    { TIFFTAG_FILLORDER,	"FillOrder" },
    { TIFFTAG_DOCUMENTNAME,	"DocumentName" },
    { TIFFTAG_IMAGEDESCRIPTION,	"ImageDescription" },
    { TIFFTAG_MAKE,		"Make" },
    { TIFFTAG_MODEL,		"Model" },
    { TIFFTAG_STRIPOFFSETS,	"StripOffsets" },
    { TIFFTAG_ORIENTATION,	"Orientation" },
    { TIFFTAG_SAMPLESPERPIXEL,	"SamplesPerPixel" },
    { TIFFTAG_ROWSPERSTRIP,	"RowsPerStrip" },
    { TIFFTAG_STRIPBYTECOUNTS,	"StripByteCounts" },
    { TIFFTAG_MINSAMPLEVALUE,	"MinSampleValue" },
    { TIFFTAG_MAXSAMPLEVALUE,	"MaxSampleValue" },
    { TIFFTAG_XRESOLUTION,	"XResolution" },
    { TIFFTAG_YRESOLUTION,	"YResolution" },
    { TIFFTAG_PLANARCONFIG,	"PlanarConfig" },
    { TIFFTAG_PAGENAME,		"PageName" },
    { TIFFTAG_XPOSITION,	"XPosition" },
    { TIFFTAG_YPOSITION,	"YPosition" },
    { TIFFTAG_FREEOFFSETS,	"FreeOffsets" },
    { TIFFTAG_FREEBYTECOUNTS,	"FreeByteCounts" },
    { TIFFTAG_GRAYRESPONSEUNIT,	"GrayResponseUnit" },
    { TIFFTAG_GRAYRESPONSECURVE,"GrayResponseCurve" },
    { TIFFTAG_GROUP3OPTIONS,	"Group3Options" },
    { TIFFTAG_GROUP4OPTIONS,	"Group4Options" },
    { TIFFTAG_RESOLUTIONUNIT,	"ResolutionUnit" },
    { TIFFTAG_PAGENUMBER,	"PageNumber" },
    { TIFFTAG_COLORRESPONSEUNIT,"ColorResponseUnit" },
    { TIFFTAG_TRANSFERFUNCTION,	"TransferFunction" },
    { TIFFTAG_SOFTWARE,		"Software" },
    { TIFFTAG_DATETIME,		"DateTime" },
    { TIFFTAG_ARTIST,		"Artist" },
    { TIFFTAG_HOSTCOMPUTER,	"HostComputer" },
    { TIFFTAG_PREDICTOR,	"Predictor" },
    { TIFFTAG_WHITEPOINT,	"Whitepoint" },
    { TIFFTAG_PRIMARYCHROMATICITIES,"PrimaryChromaticities" },
    { TIFFTAG_COLORMAP,		"Colormap" },
    { TIFFTAG_HALFTONEHINTS,	"HalftoneHints" },
    { TIFFTAG_TILEWIDTH,	"TileWidth" },
    { TIFFTAG_TILELENGTH,	"TileLength" },
    { TIFFTAG_TILEOFFSETS,	"TileOffsets" },
    { TIFFTAG_TILEBYTECOUNTS,	"TileByteCounts" },
    { TIFFTAG_BADFAXLINES,	"BadFaxLines" },
    { TIFFTAG_CLEANFAXDATA,	"CleanFaxData" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES, "ConsecutiveBadFaxLines" },
    { TIFFTAG_SUBIFD,		"SubIFD" },
    { TIFFTAG_INKSET,		"InkSet" },
    { TIFFTAG_INKNAMES,		"InkNames" },
    { TIFFTAG_NUMBEROFINKS,	"NumberOfInks" },
    { TIFFTAG_DOTRANGE,		"DotRange" },
    { TIFFTAG_TARGETPRINTER,	"TargetPrinter" },
    { TIFFTAG_EXTRASAMPLES,	"ExtraSamples" },
    { TIFFTAG_SAMPLEFORMAT,	"SampleFormat" },
    { TIFFTAG_SMINSAMPLEVALUE,	"SMinSampleValue" },
    { TIFFTAG_SMAXSAMPLEVALUE,	"SMaxSampleValue" },
    { TIFFTAG_JPEGPROC,		"JPEGProcessingMode" },
    { TIFFTAG_JPEGIFOFFSET,	"JPEGInterchangeFormat" },
    { TIFFTAG_JPEGIFBYTECOUNT,	"JPEGInterchangeFormatLength" },
    { TIFFTAG_JPEGRESTARTINTERVAL,"JPEGRestartInterval" },
    { TIFFTAG_JPEGLOSSLESSPREDICTORS,"JPEGLosslessPredictors" },
    { TIFFTAG_JPEGPOINTTRANSFORM,"JPEGPointTransform" },
    { TIFFTAG_JPEGTABLES,       "JPEGTables" },
    { TIFFTAG_JPEGQTABLES,	"JPEGQTables" },
    { TIFFTAG_JPEGDCTABLES,	"JPEGDCTables" },
    { TIFFTAG_JPEGACTABLES,	"JPEGACTables" },
    { TIFFTAG_YCBCRCOEFFICIENTS,"YCbCrCoefficients" },
    { TIFFTAG_YCBCRSUBSAMPLING,	"YCbCrSubsampling" },
    { TIFFTAG_YCBCRPOSITIONING,	"YCbCrPositioning" },
    { TIFFTAG_REFERENCEBLACKWHITE, "ReferenceBlackWhite" },
    { TIFFTAG_REFPTS,		"IgReferencePoints (Island Graphics)" },
    { TIFFTAG_REGIONTACKPOINT,	"IgRegionTackPoint (Island Graphics)" },
    { TIFFTAG_REGIONWARPCORNERS,"IgRegionWarpCorners (Island Graphics)" },
    { TIFFTAG_REGIONAFFINE,	"IgRegionAffine (Island Graphics)" },
    { TIFFTAG_MATTEING,		"OBSOLETE Matteing (Silicon Graphics)" },
    { TIFFTAG_DATATYPE,		"OBSOLETE DataType (Silicon Graphics)" },
    { TIFFTAG_IMAGEDEPTH,	"ImageDepth (Silicon Graphics)" },
    { TIFFTAG_TILEDEPTH,	"TileDepth (Silicon Graphics)" },
    { 32768,			"OLD BOGUS Matteing tag" },
    { TIFFTAG_COPYRIGHT,	"Copyright" },
    { TIFFTAG_ICCPROFILE,	"ICC Profile" },
    { TIFFTAG_JBIGOPTIONS,	"JBIG Options" },
    { TIFFTAG_STONITS,		"StoNits" },
};
#define	NTAGS	(sizeof (tagnames) / sizeof (tagnames[0]))

static void
PrintTag(FILE* fd, uint16 tag)
{
	const struct tagname *tp;

	for (tp = tagnames; tp < &tagnames[NTAGS]; tp++)
		if (tp->tag == tag) {
			fprintf(fd, "%s (%u)", tp->name, tag);
			return;
		}
	fprintf(fd, "%u (%#x)", tag, tag);
}

static void
PrintType(FILE* fd, uint16 type)
{
	static const char *typenames[] = {
	    "0",
	    "BYTE",
	    "ASCII",
	    "SHORT",
	    "LONG",
	    "RATIONAL",
	    "SBYTE",
	    "UNDEFINED",
	    "SSHORT",
	    "SLONG",
	    "SRATIONAL",
	    "FLOAT",
	    "DOUBLE",
	    "IFD",
	    "14",
	    "15",
	    "LONG8",
	    "SLONG8",
	    "IFD8"
	};
#define	NTYPES	(sizeof (typenames) / sizeof (typenames[0]))

	if (type < NTYPES)
		fprintf(fd, "%s (%u)", typenames[type], type);
	else
		fprintf(fd, "%u (%#x)", type, type);
}
#undef	NTYPES

#include <ctype.h>

static void
PrintASCII(FILE* fd, uint32 cc, const unsigned char* cp)
{
	for (; cc > 0; cc--, cp++) {
		const char* tp;

		if (isprint(*cp)) {
			fputc(*cp, fd);
			continue;
		}
		for (tp = "\tt\bb\rr\nn\vv"; *tp; tp++)
			if (*tp++ == *cp)
				break;
		if (*tp)
			fprintf(fd, "\\%c", *tp);
		else if (*cp)
			fprintf(fd, "\\%03o", *cp);
		else
			fprintf(fd, "\\0");
	}
}

static void
PrintData(FILE* fd, uint16 type, uint32 count, unsigned char* data)
{
	char* sep = "";

	switch (type) {
	case TIFF_BYTE:
		while (count-- > 0)
			fprintf(fd, bytefmt, sep, *data++), sep = " ";
		break;
	case TIFF_SBYTE:
		while (count-- > 0)
			fprintf(fd, sbytefmt, sep, *(char *)data++), sep = " ";
		break;
	case TIFF_UNDEFINED:
		while (count-- > 0)
			fprintf(fd, bytefmt, sep, *data++), sep = " ";
		break;
	case TIFF_ASCII:
		PrintASCII(fd, count, data);
		break;
	case TIFF_SHORT: {
		uint16 *wp = (uint16*)data;
		while (count-- > 0)
			fprintf(fd, shortfmt, sep, *wp++), sep = " ";
		break;
	}
	case TIFF_SSHORT: {
		int16 *wp = (int16*)data;
		while (count-- > 0)
			fprintf(fd, sshortfmt, sep, *wp++), sep = " ";
		break;
	}
	case TIFF_LONG: {
		uint32 *lp = (uint32*)data;
		while (count-- > 0) {
			fprintf(fd, longfmt, sep, (unsigned long) *lp++);
			sep = " ";
		}
		break;
	}
	case TIFF_SLONG: {
		int32 *lp = (int32*)data;
		while (count-- > 0)
			fprintf(fd, slongfmt, sep, (long) *lp++), sep = " ";
		break;
	}
	case TIFF_LONG8: {
		uint64 *llp = (uint64*)data;
		while (count-- > 0) {
#if defined(__WIN32__) && defined(_MSC_VER)
			fprintf(fd, long8fmt, sep, (unsigned __int64) *llp++);
#else
			fprintf(fd, long8fmt, sep, (unsigned long long) *llp++);
#endif
			sep = " ";
		}
		break;
	}
	case TIFF_SLONG8: {
		int64 *llp = (int64*)data;
		while (count-- > 0)
#if defined(__WIN32__) && defined(_MSC_VER)
			fprintf(fd, slong8fmt, sep, (__int64) *llp++), sep = " ";
#else
			fprintf(fd, slong8fmt, sep, (long long) *llp++), sep = " ";
#endif
		break;
	}
	case TIFF_RATIONAL: {
		uint32 *lp = (uint32*)data;
		while (count-- > 0) {
			if (lp[1] == 0)
				fprintf(fd, "%sNan (%lu/%lu)", sep,
				    (unsigned long) lp[0],
				    (unsigned long) lp[1]);
			else
				fprintf(fd, rationalfmt, sep,
				    (double)lp[0] / (double)lp[1]);
			sep = " ";
			lp += 2;
		}
		break;
	}
	case TIFF_SRATIONAL: {
		int32 *lp = (int32*)data;
		while (count-- > 0) {
			if (lp[1] == 0)
				fprintf(fd, "%sNan (%ld/%ld)", sep,
				    (long) lp[0], (long) lp[1]);
			else
				fprintf(fd, srationalfmt, sep,
				    (double)lp[0] / (double)lp[1]);
			sep = " ";
			lp += 2;
		}
		break;
	}
	case TIFF_FLOAT: {
		float *fp = (float *)data;
		while (count-- > 0)
			fprintf(fd, floatfmt, sep, *fp++), sep = " ";
		break;
	}
	case TIFF_DOUBLE: {
		double *dp = (double *)data;
		while (count-- > 0)
			fprintf(fd, doublefmt, sep, *dp++), sep = " ";
		break;
	}
	case TIFF_IFD: {
		uint32 *lp = (uint32*)data;
		while (count-- > 0) {
			fprintf(fd, ifdfmt, sep, (unsigned long) *lp++);
			sep = " ";
		}
		break;
	}
	case TIFF_IFD8: {
		uint64 *llp = (uint64*)data;
		while (count-- > 0) {
#if defined(__WIN32__) && defined(_MSC_VER)
			fprintf(fd, ifd8fmt, sep, (unsigned __int64) *llp++);
#else
			fprintf(fd, ifd8fmt, sep, (unsigned long long) *llp++);
#endif
			sep = " ";
		}
		break;
	}
	}
}

static void
ReadError(char* what)
{
	Fatal("Error while reading %s", what);
}

#include <stdarg.h>

static void
vError(FILE* fd, const char* fmt, va_list ap)
{
	fprintf(fd, "%s: ", curfile);
	vfprintf(fd, fmt, ap);
	fprintf(fd, ".\n");
}

static void
Error(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vError(stderr, fmt, ap);
	va_end(ap);
}

static void
Fatal(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vError(stderr, fmt, ap);
	va_end(ap);
	exit(-1);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
