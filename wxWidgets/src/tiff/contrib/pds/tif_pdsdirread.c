/* $Header: /cvs/maptools/cvsroot/libtiff/contrib/pds/tif_pdsdirread.c,v 1.4 2010-06-08 18:55:15 bfriesen Exp $ */

/*
 * Copyright (c) 1988-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
 * Copyright (c( 1996 USAF Phillips Laboratory
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
 * TIFF Library.
 * 
 * These routines written by Conrad J. Poelman on a single late-night of
 * March 20-21, 1996.
 *
 * The entire purpose of this file is to provide a single external function,
 * TIFFReadPrivateDataSubDirectory(). This function is intended for use in reading a
 * private subdirectory from a TIFF file into a private structure. The
 * actual writing of data into the structure is handled by the setFieldFn(),
 * which is passed to TIFFReadPrivateDataSubDirectory() as a parameter. The idea is to
 * enable any application wishing to store private subdirectories to do so
 * easily using this function, without modifying the TIFF library.
 *
 * The astute observer will notice that only two functions are at all different
 * from the original tif_dirread.c file: TIFFReadPrivateDataSubDirectory() and
 * TIFFFetchNormalSubTag(). All the other stuff that makes this file so huge
 * is only necessary because all of those functions are declared static in
 * tif_dirread.c, so we have to totally duplicate them in order to use them.
 *
 * Oh, also note the bug fix in TIFFFetchFloat().
 *
 */

#include "tiffiop.h"

#define	IGNORE	0		/* tag placeholder used below */

#if HAVE_IEEEFP
#define	TIFFCvtIEEEFloatToNative(tif, n, fp)
#define	TIFFCvtIEEEDoubleToNative(tif, n, dp)
#else
extern	void TIFFCvtIEEEFloatToNative(TIFF*, uint32, float*);
extern	void TIFFCvtIEEEDoubleToNative(TIFF*, uint32, double*);
#endif

static	void EstimateStripByteCounts(TIFF*, TIFFDirEntry*, uint16);
static	void MissingRequired(TIFF*, const char*);
static	int CheckDirCount(TIFF*, TIFFDirEntry*, uint32);
static	tsize_t TIFFFetchData(TIFF*, TIFFDirEntry*, char*);
static	tsize_t TIFFFetchString(TIFF*, TIFFDirEntry*, char*);
static	float TIFFFetchRational(TIFF*, TIFFDirEntry*);
static	int TIFFFetchNormalSubTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*,
				  int (*getFieldFn)(TIFF *tif,ttag_t tag,...));
static	int TIFFFetchPerSampleShorts(TIFF*, TIFFDirEntry*, int*);
static	int TIFFFetchPerSampleAnys(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchShortArray(TIFF*, TIFFDirEntry*, uint16*);
static	int TIFFFetchStripThing(TIFF*, TIFFDirEntry*, long, uint32**);
static	int TIFFFetchExtraSamples(TIFF*, TIFFDirEntry*);
static	int TIFFFetchRefBlackWhite(TIFF*, TIFFDirEntry*);
static	float TIFFFetchFloat(TIFF*, TIFFDirEntry*);
static	int TIFFFetchFloatArray(TIFF*, TIFFDirEntry*, float*);
static	int TIFFFetchDoubleArray(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchAnyArray(TIFF*, TIFFDirEntry*, double*);
static	int TIFFFetchShortPair(TIFF*, TIFFDirEntry*);
#if STRIPCHOP_SUPPORT
static	void ChopUpSingleUncompressedStrip(TIFF*);
#endif

static char *
CheckMalloc(TIFF* tif, tsize_t n, const char* what)
{
	char *cp = (char*)_TIFFmalloc(n);
	if (cp == NULL)
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "No space %s", what);
	return (cp);
}

/* Just as was done with TIFFWritePrivateDataSubDirectory(), here we implement
   TIFFReadPrivateDataSubDirectory() which takes an offset into the TIFF file,
   a TIFFFieldInfo structure specifying the types of the various tags,
   and a function to use to set individual tags when they are encountered.
   The data is read from the file, translated using the TIFF library's
   built-in machine-independent conversion functions, and filled into
   private subdirectory structure.

   This code was written by copying the original TIFFReadDirectory() function
   from tif_dirread.c and paring it down to what is needed for this.

   It is the caller's responsibility to allocate and initialize the internal
   structure that setFieldFn() will be writing into. If this function is being
   called more than once before closing the file, the caller also must be
   careful to free data in the structure before re-initializing.

   It is also the caller's responsibility to verify the presence of
   any required fields after reading the directory in.
*/


int
TIFFReadPrivateDataSubDirectory(TIFF* tif, toff_t pdir_offset,
				TIFFFieldInfo *field_info,
				int (*setFieldFn)(TIFF *tif, ttag_t tag, ...))
{
	register TIFFDirEntry* dp;
	register int n;
	register TIFFDirectory* td;
	TIFFDirEntry* dir;
	int iv;
	long v;
	double dv;
	const TIFFFieldInfo* fip;
	int fix;
	uint16 dircount;
	uint32 nextdiroff;
	char* cp;
	int diroutoforderwarning = 0;

	/* Skipped part about checking for directories or compression data. */

	if (!isMapped(tif)) {
		if (!SeekOK(tif, pdir_offset)) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "Seek error accessing TIFF private subdirectory");
			return (0);
		}
		if (!ReadOK(tif, &dircount, sizeof (uint16))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "Can not read TIFF private subdirectory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		dir = (TIFFDirEntry *)CheckMalloc(tif,
		    dircount * sizeof (TIFFDirEntry), "to read TIFF private subdirectory");
		if (dir == NULL)
			return (0);
		if (!ReadOK(tif, dir, dircount*sizeof (TIFFDirEntry))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Can not read TIFF private subdirectory");
			goto bad;
		}
		/*
		 * Read offset to next directory for sequential scans.
		 */
		(void) ReadOK(tif, &nextdiroff, sizeof (uint32));
	} else {
		toff_t off = pdir_offset;

		if (off + sizeof (short) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
			    "Can not read TIFF private subdirectory count");
			return (0);
		} else
			_TIFFmemcpy(&dircount, tif->tif_base + off, sizeof (uint16));
		off += sizeof (uint16);
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		dir = (TIFFDirEntry *)CheckMalloc(tif,
		    dircount * sizeof (TIFFDirEntry), "to read TIFF private subdirectory");
		if (dir == NULL)
			return (0);
		if (off + dircount*sizeof (TIFFDirEntry) > tif->tif_size) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Can not read TIFF private subdirectory");
			goto bad;
		} else
			_TIFFmemcpy(dir, tif->tif_base + off,
			    dircount*sizeof (TIFFDirEntry));
		off += dircount* sizeof (TIFFDirEntry);
		if (off + sizeof (uint32) < tif->tif_size)
			_TIFFmemcpy(&nextdiroff, tif->tif_base+off, sizeof (uint32));
	}
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong(&nextdiroff);

	/*
	 * Setup default value and then make a pass over
	 * the fields to check type and tag information,
	 * and to extract info required to size data
	 * structures.  A second pass is made afterwards
	 * to read in everthing not taken in the first pass.
	 */
	td = &tif->tif_dir;
	
	for (fip = field_info, dp = dir, n = dircount;
	     n > 0; n--, dp++) {
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFSwabArrayOfShort(&dp->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dp->tdir_count, 2);
		}
		/*
		 * Find the field information entry for this tag.
		 */
		/*
		 * Silicon Beach (at least) writes unordered
		 * directory tags (violating the spec).  Handle
		 * it here, but be obnoxious (maybe they'll fix it?).
		 */
		if (dp->tdir_tag < fip->field_tag) {
			if (!diroutoforderwarning) {
				TIFFWarning(tif->tif_name,
	"invalid TIFF private subdirectory; tags are not sorted in ascending order");
				diroutoforderwarning = 1;
			}
			fip = field_info;    /* O(n^2) */
		}

		while (fip->field_tag && fip->field_tag < dp->tdir_tag)
			fip++;
		if (!fip->field_tag || fip->field_tag != dp->tdir_tag) {
			TIFFWarning(tif->tif_name,
			    "unknown field with tag %d (0x%x) in private subdirectory ignored",
			    dp->tdir_tag,  dp->tdir_tag);
			dp->tdir_tag = IGNORE;
			fip = field_info;/* restart search */
			continue;
		}
		/*
		 * Null out old tags that we ignore.
		 */

		/* Not implemented yet, since FIELD_IGNORE is specific to
		   the main directories. Could pass this in too... */
		if (0 /* && fip->field_bit == FIELD_IGNORE */) {
	ignore:
			dp->tdir_tag = IGNORE;
			continue;
		}

		/*
		 * Check data type.
		 */

		while (dp->tdir_type != (u_short)fip->field_type) {
			if (fip->field_type == TIFF_ANY)	/* wildcard */
				break;
			fip++;
			if (!fip->field_tag || fip->field_tag != dp->tdir_tag) {
				TIFFWarning(tif->tif_name,
				   "wrong data type %d for \"%s\"; tag ignored",
				    dp->tdir_type, fip[-1].field_name);
				goto ignore;
			}
		}
		/*
		 * Check count if known in advance.
		 */
		if (fip->field_readcount != TIFF_VARIABLE) {
			uint32 expected = (fip->field_readcount == TIFF_SPP) ?
			    (uint32) td->td_samplesperpixel :
			    (uint32) fip->field_readcount;
			if (!CheckDirCount(tif, dp, expected))
				goto ignore;
		}

		/* Now read in and process data from field. */
		if (!TIFFFetchNormalSubTag(tif, dp, fip, setFieldFn))
		    goto bad;

	}

	if (dir)
		_TIFFfree(dir);
	return (1);
bad:
	if (dir)
		_TIFFfree(dir);
	return (0);
}

static void
EstimateStripByteCounts(TIFF* tif, TIFFDirEntry* dir, uint16 dircount)
{
	register TIFFDirEntry *dp;
	register TIFFDirectory *td = &tif->tif_dir;
	uint16 i;

	if (td->td_stripbytecount)
		_TIFFfree(td->td_stripbytecount);
	td->td_stripbytecount = (uint32*)
	    CheckMalloc(tif, td->td_nstrips * sizeof (uint32),
		"for \"StripByteCounts\" array");
	if (td->td_compression != COMPRESSION_NONE) {
		uint32 space = (uint32)(sizeof (TIFFHeader)
		    + sizeof (uint16)
		    + (dircount * sizeof (TIFFDirEntry))
		    + sizeof (uint32));
		toff_t filesize = TIFFGetFileSize(tif);
		uint16 n;

		/* calculate amount of space used by indirect values */
		for (dp = dir, n = dircount; n > 0; n--, dp++) {
			uint32 cc = dp->tdir_count*TIFFDataWidth(dp->tdir_type);
			if (cc > sizeof (uint32))
				space += cc;
		}
		space = (filesize - space) / td->td_samplesperpixel;
		for (i = 0; i < td->td_nstrips; i++)
			td->td_stripbytecount[i] = space;
		/*
		 * This gross hack handles the case were the offset to
		 * the last strip is past the place where we think the strip
		 * should begin.  Since a strip of data must be contiguous,
		 * it's safe to assume that we've overestimated the amount
		 * of data in the strip and trim this number back accordingly.
		 */ 
		i--;
		if (td->td_stripoffset[i] + td->td_stripbytecount[i] > filesize)
			td->td_stripbytecount[i] =
			    filesize - td->td_stripoffset[i];
	} else {
		uint32 rowbytes = TIFFScanlineSize(tif);
		uint32 rowsperstrip = td->td_imagelength / td->td_nstrips;
		for (i = 0; i < td->td_nstrips; i++)
			td->td_stripbytecount[i] = rowbytes*rowsperstrip;
	}
	TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
	if (!TIFFFieldSet(tif, FIELD_ROWSPERSTRIP))
		td->td_rowsperstrip = td->td_imagelength;
}

static void
MissingRequired(TIFF* tif, const char* tagname)
{
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
	    "TIFF directory is missing required \"%s\" field", tagname);
}

/*
 * Check the count field of a directory
 * entry against a known value.  The caller
 * is expected to skip/ignore the tag if
 * there is a mismatch.
 */
static int
CheckDirCount(TIFF* tif, TIFFDirEntry* dir, uint32 count)
{
	if (count != dir->tdir_count) {
		TIFFWarning(tif->tif_name,
	"incorrect count for field \"%s\" (%lu, expecting %lu); tag ignored",
		    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name,
		    dir->tdir_count, count);
		return (0);
	}
	return (1);
}

/*
 * Fetch a contiguous directory item.
 */
static tsize_t
TIFFFetchData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	int w = TIFFDataWidth(dir->tdir_type);
	tsize_t cc = dir->tdir_count * w;

	if (!isMapped(tif)) {
		if (!SeekOK(tif, dir->tdir_offset))
			goto bad;
		if (!ReadOK(tif, cp, cc))
			goto bad;
	} else {
		if (dir->tdir_offset + cc > tif->tif_size)
			goto bad;
		_TIFFmemcpy(cp, tif->tif_base + dir->tdir_offset, cc);
	}
	if (tif->tif_flags & TIFF_SWAB) {
		switch (dir->tdir_type) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((uint16*) cp, dir->tdir_count);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
			TIFFSwabArrayOfLong((uint32*) cp, dir->tdir_count);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			TIFFSwabArrayOfLong((uint32*) cp, 2*dir->tdir_count);
			break;
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*) cp, dir->tdir_count);
			break;
		}
	}
	return (cc);
bad:
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error fetching data for field \"%s\"",
	    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
	return ((tsize_t) 0);
}

/*
 * Fetch an ASCII item from the file.
 */
static tsize_t
TIFFFetchString(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (dir->tdir_count <= 4) {
		uint32 l = dir->tdir_offset;
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&l);
		_TIFFmemcpy(cp, &l, dir->tdir_count);
		return (1);
	}
	return (TIFFFetchData(tif, dir, cp));
}

/*
 * Convert numerator+denominator to float.
 */
static int
cvtRational(TIFF* tif, TIFFDirEntry* dir, uint32 num, uint32 denom, float* rv)
{
	if (denom == 0) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "%s: Rational with zero denominator (num = %lu)",
		    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name, num);
		return (0);
	} else {
		if (dir->tdir_type == TIFF_RATIONAL)
			*rv = ((float)num / (float)denom);
		else
			*rv = ((float)(int32)num / (float)(int32)denom);
		return (1);
	}
}

/*
 * Fetch a rational item from the file
 * at offset off and return the value
 * as a floating point number.
 */
static float
TIFFFetchRational(TIFF* tif, TIFFDirEntry* dir)
{
	uint32 l[2];
	float v;

	return (!TIFFFetchData(tif, dir, (char *)l) ||
	    !cvtRational(tif, dir, l[0], l[1], &v) ? 1.0f : v);
}

/*
 * Fetch a single floating point value
 * from the offset field and return it
 * as a native float.
 */
static float
TIFFFetchFloat(TIFF* tif, TIFFDirEntry* dir)
{
	/* This appears to be a flagrant bug in the TIFF library, yet I
	   actually don't understand how it could have ever worked the old
	   way. Look at the comments in my new code and you'll understand. */
#if (0)
	float v = (float)
	    TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
	TIFFCvtIEEEFloatToNative(tif, 1, &v);
#else
	float v;
	/* This is a little bit tricky - if we just cast the uint32 to a float,
	   C will perform a numerical conversion, which is not what we want.
	   We want to take the actual bit pattern in the uint32 and interpret
	   it as a float. Thus we cast a uint32 * into a float * and then
	   dereference to get v. */
	uint32 l = (uint32)
	    TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
	v = * (float *) &l;
	TIFFCvtIEEEFloatToNative(tif, 1, &v);
#endif
	return (v);

}

/*
 * Fetch an array of BYTE or SBYTE values.
 */
static int
TIFFFetchByteArray(TIFF* tif, TIFFDirEntry* dir, uint16* v)
{
	if (dir->tdir_count <= 4) {
		/*
		 * Extract data from offset field.
		 */
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			switch (dir->tdir_count) {
			case 4: v[3] = dir->tdir_offset & 0xff;
			case 3: v[2] = (dir->tdir_offset >> 8) & 0xff;
			case 2: v[1] = (dir->tdir_offset >> 16) & 0xff;
			case 1: v[0] = dir->tdir_offset >> 24;
			}
		} else {
			switch (dir->tdir_count) {
			case 4: v[3] = dir->tdir_offset >> 24;
			case 3: v[2] = (dir->tdir_offset >> 16) & 0xff;
			case 2: v[1] = (dir->tdir_offset >> 8) & 0xff;
			case 1: v[0] = dir->tdir_offset & 0xff;
			}
		}
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char*) v) != 0);	/* XXX */
}

/*
 * Fetch an array of SHORT or SSHORT values.
 */
static int
TIFFFetchShortArray(TIFF* tif, TIFFDirEntry* dir, uint16* v)
{
	if (dir->tdir_count <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			switch (dir->tdir_count) {
			case 2: v[1] = dir->tdir_offset & 0xffff;
			case 1: v[0] = dir->tdir_offset >> 16;
			}
		} else {
			switch (dir->tdir_count) {
			case 2: v[1] = dir->tdir_offset >> 16;
			case 1: v[0] = dir->tdir_offset & 0xffff;
			}
		}
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char *)v) != 0);
}

/*
 * Fetch a pair of SHORT or BYTE values.
 */
static int
TIFFFetchShortPair(TIFF* tif, TIFFDirEntry* dir)
{
	uint16 v[2];
	int ok = 0;

	switch (dir->tdir_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		ok = TIFFFetchShortArray(tif, dir, v);
		break;
	case TIFF_BYTE:
	case TIFF_SBYTE:
		ok  = TIFFFetchByteArray(tif, dir, v);
		break;
	}
	if (ok)
		TIFFSetField(tif, dir->tdir_tag, v[0], v[1]);
	return (ok);
}

/*
 * Fetch an array of LONG or SLONG values.
 */
static int
TIFFFetchLongArray(TIFF* tif, TIFFDirEntry* dir, uint32* v)
{
	if (dir->tdir_count == 1) {
		v[0] = dir->tdir_offset;
		return (1);
	} else
		return (TIFFFetchData(tif, dir, (char*) v) != 0);
}

/*
 * Fetch an array of RATIONAL or SRATIONAL values.
 */
static int
TIFFFetchRationalArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{
	int ok = 0;
	uint32* l;

	l = (uint32*)CheckMalloc(tif,
	    dir->tdir_count*TIFFDataWidth(dir->tdir_type),
	    "to fetch array of rationals");
	if (l) {
		if (TIFFFetchData(tif, dir, (char *)l)) {
			uint32 i;
			for (i = 0; i < dir->tdir_count; i++) {
				ok = cvtRational(tif, dir,
				    l[2*i+0], l[2*i+1], &v[i]);
				if (!ok)
					break;
			}
		}
		_TIFFfree((char *)l);
	}
	return (ok);
}

/*
 * Fetch an array of FLOAT values.
 */
static int
TIFFFetchFloatArray(TIFF* tif, TIFFDirEntry* dir, float* v)
{

	if (dir->tdir_count == 1) {
		v[0] = *(float*) &dir->tdir_offset;
		TIFFCvtIEEEFloatToNative(tif, dir->tdir_count, v);
		return (1);
	} else	if (TIFFFetchData(tif, dir, (char*) v)) {
		TIFFCvtIEEEFloatToNative(tif, dir->tdir_count, v);
		return (1);
	} else
		return (0);
}

/*
 * Fetch an array of DOUBLE values.
 */
static int
TIFFFetchDoubleArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	if (TIFFFetchData(tif, dir, (char*) v)) {
		TIFFCvtIEEEDoubleToNative(tif, dir->tdir_count, v);
		return (1);
	} else
		return (0);
}

/*
 * Fetch an array of ANY values.  The actual values are
 * returned as doubles which should be able hold all the
 * types.  Yes, there really should be an tany_t to avoid
 * this potential non-portability ...  Note in particular
 * that we assume that the double return value vector is
 * large enough to read in any fundamental type.  We use
 * that vector as a buffer to read in the base type vector
 * and then convert it in place to double (from end
 * to front of course).
 */
static int
TIFFFetchAnyArray(TIFF* tif, TIFFDirEntry* dir, double* v)
{
	int i;

	switch (dir->tdir_type) {
	case TIFF_BYTE:
	case TIFF_SBYTE:
		if (!TIFFFetchByteArray(tif, dir, (uint16*) v))
			return (0);
		if (dir->tdir_type == TIFF_BYTE) {
			uint16* vp = (uint16*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int16* vp = (int16*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (!TIFFFetchShortArray(tif, dir, (uint16*) v))
			return (0);
		if (dir->tdir_type == TIFF_SHORT) {
			uint16* vp = (uint16*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int16* vp = (int16*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
		if (!TIFFFetchLongArray(tif, dir, (uint32*) v))
			return (0);
		if (dir->tdir_type == TIFF_LONG) {
			uint32* vp = (uint32*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		} else {
			int32* vp = (int32*) v;
			for (i = dir->tdir_count-1; i >= 0; i--)
				v[i] = vp[i];
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (!TIFFFetchRationalArray(tif, dir, (float*) v))
			return (0);
		{ float* vp = (float*) v;
		  for (i = dir->tdir_count-1; i >= 0; i--)
			v[i] = vp[i];
		}
		break;
	case TIFF_FLOAT:
		if (!TIFFFetchFloatArray(tif, dir, (float*) v))
			return (0);
		{ float* vp = (float*) v;
		  for (i = dir->tdir_count-1; i >= 0; i--)
			v[i] = vp[i];
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFFetchDoubleArray(tif, dir, (double*) v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "Cannot read TIFF_ANY type %d for field \"%s\"",
		    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
		return (0);
	}
	return (1);
}


/*
 * Fetch a tag that is not handled by special case code.
 */
/* The standard function TIFFFetchNormalTag() could definitely be replaced
   with a simple call to this function, just adding TIFFSetField() as the
   last argument. */
static int
TIFFFetchNormalSubTag(TIFF* tif, TIFFDirEntry* dp, const TIFFFieldInfo* fip,
		      int (*setFieldFn)(TIFF *tif, ttag_t tag, ...))
{
	static char mesg[] = "to fetch tag value";
	int ok = 0;

	if (dp->tdir_count > 1) {		/* array of values */
		char* cp = NULL;

		switch (dp->tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
			/* NB: always expand BYTE values to shorts */
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (uint16), mesg);
			ok = cp && TIFFFetchByteArray(tif, dp, (uint16*) cp);
			break;
		case TIFF_SHORT:
		case TIFF_SSHORT:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (uint16), mesg);
			ok = cp && TIFFFetchShortArray(tif, dp, (uint16*) cp);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (uint32), mesg);
			ok = cp && TIFFFetchLongArray(tif, dp, (uint32*) cp);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (float), mesg);
			ok = cp && TIFFFetchRationalArray(tif, dp, (float*) cp);
			break;
		case TIFF_FLOAT:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (float), mesg);
			ok = cp && TIFFFetchFloatArray(tif, dp, (float*) cp);
			break;
		case TIFF_DOUBLE:
			cp = CheckMalloc(tif,
			    dp->tdir_count * sizeof (double), mesg);
			ok = cp && TIFFFetchDoubleArray(tif, dp, (double*) cp);
			break;
		case TIFF_ASCII:
		case TIFF_UNDEFINED:		/* bit of a cheat... */
			/*
			 * Some vendors write strings w/o the trailing
			 * NULL byte, so always append one just in case.
			 */
			cp = CheckMalloc(tif, dp->tdir_count+1, mesg);
			if (ok = (cp && TIFFFetchString(tif, dp, cp)))
				cp[dp->tdir_count] = '\0';	/* XXX */
			break;
		}
		if (ok) {
			ok = (fip->field_passcount ?
			    (*setFieldFn)(tif, dp->tdir_tag, dp->tdir_count, cp)
			  : (*setFieldFn)(tif, dp->tdir_tag, cp));
		}
		if (cp != NULL)
			_TIFFfree(cp);
	} else if (CheckDirCount(tif, dp, 1)) {	/* singleton value */
		switch (dp->tdir_type) {
		case TIFF_BYTE:
		case TIFF_SBYTE:
		case TIFF_SHORT:
		case TIFF_SSHORT:
			/*
			 * If the tag is also acceptable as a LONG or SLONG
			 * then (*setFieldFn) will expect an uint32 parameter
			 * passed to it (through varargs).  Thus, for machines
			 * where sizeof (int) != sizeof (uint32) we must do
			 * a careful check here.  It's hard to say if this
			 * is worth optimizing.
			 *
			 * NB: We use TIFFFieldWithTag here knowing that
			 *     it returns us the first entry in the table
			 *     for the tag and that that entry is for the
			 *     widest potential data type the tag may have.
			 */
			{ TIFFDataType type = fip->field_type;
			  if (type != TIFF_LONG && type != TIFF_SLONG) {
				uint16 v = (uint16)
			   TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset);
				ok = (fip->field_passcount ?
				    (*setFieldFn)(tif, dp->tdir_tag, 1, &v)
				  : (*setFieldFn)(tif, dp->tdir_tag, v));
				break;
			  }
			}
			/* fall thru... */
		case TIFF_LONG:
		case TIFF_SLONG:
			{ uint32 v32 =
		    TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset);
			  ok = (fip->field_passcount ? 
			      (*setFieldFn)(tif, dp->tdir_tag, 1, &v32)
			    : (*setFieldFn)(tif, dp->tdir_tag, v32));
			}
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		case TIFF_FLOAT:
			{ float v = (dp->tdir_type == TIFF_FLOAT ? 
			      TIFFFetchFloat(tif, dp)
			    : TIFFFetchRational(tif, dp));
			  ok = (fip->field_passcount ?
			      (*setFieldFn)(tif, dp->tdir_tag, 1, &v)
			    : (*setFieldFn)(tif, dp->tdir_tag, v));
			}
			break;
		case TIFF_DOUBLE:
			{ double v;
			  ok = (TIFFFetchDoubleArray(tif, dp, &v) &&
			    (fip->field_passcount ?
			      (*setFieldFn)(tif, dp->tdir_tag, 1, &v)
			    : (*setFieldFn)(tif, dp->tdir_tag, v))
			  );
			}
			break;
		case TIFF_ASCII:
		case TIFF_UNDEFINED:		/* bit of a cheat... */
			{ char c[2];
			  if (ok = (TIFFFetchString(tif, dp, c) != 0)) {
				c[1] = '\0';		/* XXX paranoid */
				ok = (*setFieldFn)(tif, dp->tdir_tag, c);
			  }
			}
			break;
		}
	}
	return (ok);
}

/* Everything after this is exactly duplicated from the standard tif_dirread.c
   file, necessitated by the fact that they are declared static there so
   we can't call them!
*/
#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Fetch samples/pixel short values for 
 * the specified tag and verify that
 * all values are the same.
 */
static int
TIFFFetchPerSampleShorts(TIFF* tif, TIFFDirEntry* dir, int* pl)
{
	int samples = tif->tif_dir.td_samplesperpixel;
	int status = 0;

	if (CheckDirCount(tif, dir, (uint32) samples)) {
		uint16 buf[10];
		uint16* v = buf;

		if (samples > NITEMS(buf))
			v = (uint16*) _TIFFmalloc(samples * sizeof (uint16));
		if (TIFFFetchShortArray(tif, dir, v)) {
			int i;
			for (i = 1; i < samples; i++)
				if (v[i] != v[0]) {
					TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"Cannot handle different per-sample values for field \"%s\"",
			   _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
					goto bad;
				}
			*pl = v[0];
			status = 1;
		}
	bad:
		if (v != buf)
			_TIFFfree((char*) v);
	}
	return (status);
}

/*
 * Fetch samples/pixel ANY values for 
 * the specified tag and verify that
 * all values are the same.
 */
static int
TIFFFetchPerSampleAnys(TIFF* tif, TIFFDirEntry* dir, double* pl)
{
	int samples = (int) tif->tif_dir.td_samplesperpixel;
	int status = 0;

	if (CheckDirCount(tif, dir, (uint32) samples)) {
		double buf[10];
		double* v = buf;

		if (samples > NITEMS(buf))
			v = (double*) _TIFFmalloc(samples * sizeof (double));
		if (TIFFFetchAnyArray(tif, dir, v)) {
			int i;
			for (i = 1; i < samples; i++)
				if (v[i] != v[0]) {
					TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		"Cannot handle different per-sample values for field \"%s\"",
			   _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
					goto bad;
				}
			*pl = v[0];
			status = 1;
		}
	bad:
		if (v != buf)
			_TIFFfree(v);
	}
	return (status);
}
#undef NITEMS

/*
 * Fetch a set of offsets or lengths.
 * While this routine says "strips",
 * in fact it's also used for tiles.
 */
static int
TIFFFetchStripThing(TIFF* tif, TIFFDirEntry* dir, long nstrips, uint32** lpp)
{
	register uint32* lp;
	int status;

	if (!CheckDirCount(tif, dir, (uint32) nstrips))
		return (0);
	/*
	 * Allocate space for strip information.
	 */
	if (*lpp == NULL &&
	    (*lpp = (uint32 *)CheckMalloc(tif,
	      nstrips * sizeof (uint32), "for strip array")) == NULL)
		return (0);
	lp = *lpp;
	if (dir->tdir_type == (int)TIFF_SHORT) {
		/*
		 * Handle uint16->uint32 expansion.
		 */
		uint16* dp = (uint16*) CheckMalloc(tif,
		    dir->tdir_count* sizeof (uint16), "to fetch strip tag");
		if (dp == NULL)
			return (0);
		if (status = TIFFFetchShortArray(tif, dir, dp)) {
			register uint16* wp = dp;
			while (nstrips-- > 0)
				*lp++ = *wp++;
		}
		_TIFFfree((char*) dp);
	} else
		status = TIFFFetchLongArray(tif, dir, lp);
	return (status);
}

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Fetch and set the ExtraSamples tag.
 */
static int
TIFFFetchExtraSamples(TIFF* tif, TIFFDirEntry* dir)
{
	uint16 buf[10];
	uint16* v = buf;
	int status;

	if (dir->tdir_count > NITEMS(buf))
		v = (uint16*) _TIFFmalloc(dir->tdir_count * sizeof (uint16));
	if (dir->tdir_type == TIFF_BYTE)
		status = TIFFFetchByteArray(tif, dir, v);
	else
		status = TIFFFetchShortArray(tif, dir, v);
	if (status)
		status = TIFFSetField(tif, dir->tdir_tag, dir->tdir_count, v);
	if (v != buf)
		_TIFFfree((char*) v);
	return (status);
}
#undef NITEMS

#ifdef COLORIMETRY_SUPPORT
/*
 * Fetch and set the RefBlackWhite tag.
 */
static int
TIFFFetchRefBlackWhite(TIFF* tif, TIFFDirEntry* dir)
{
	static char mesg[] = "for \"ReferenceBlackWhite\" array";
	char* cp;
	int ok;

	if (dir->tdir_type == TIFF_RATIONAL)
		return (1/*TIFFFetchNormalTag(tif, dir) just so linker won't complain - this part of the code is never used anyway */);
	/*
	 * Handle LONG's for backward compatibility.
	 */
	cp = CheckMalloc(tif, dir->tdir_count * sizeof (uint32), mesg);
	if (ok = (cp && TIFFFetchLongArray(tif, dir, (uint32*) cp))) {
		float* fp = (float*)
		    CheckMalloc(tif, dir->tdir_count * sizeof (float), mesg);
		if (ok = (fp != NULL)) {
			uint32 i;
			for (i = 0; i < dir->tdir_count; i++)
				fp[i] = (float)((uint32*) cp)[i];
			ok = TIFFSetField(tif, dir->tdir_tag, fp);
			_TIFFfree((char*) fp);
		}
	}
	if (cp)
		_TIFFfree(cp);
	return (ok);
}
#endif

#if STRIPCHOP_SUPPORT
/*
 * Replace a single strip (tile) of uncompressed data by
 * multiple strips (tiles), each approximately 8Kbytes.
 * This is useful for dealing with large images or
 * for dealing with machines with a limited amount
 * memory.
 */
static void
ChopUpSingleUncompressedStrip(TIFF* tif)
{
	register TIFFDirectory *td = &tif->tif_dir;
	uint32 bytecount = td->td_stripbytecount[0];
	uint32 offset = td->td_stripoffset[0];
	tsize_t rowbytes = TIFFVTileSize(tif, 1), stripbytes;
	tstrip_t strip, nstrips, rowsperstrip;
	uint32* newcounts;
	uint32* newoffsets;

	/*
	 * Make the rows hold at least one
	 * scanline, but fill 8k if possible.
	 */
	if (rowbytes > 8192) {
		stripbytes = rowbytes;
		rowsperstrip = 1;
	} else {
		rowsperstrip = 8192 / rowbytes;
		stripbytes = rowbytes * rowsperstrip;
	}
	/* never increase the number of strips in an image */
	if (rowsperstrip >= td->td_rowsperstrip)
		return;
	nstrips = (tstrip_t) TIFFhowmany(bytecount, stripbytes);
	newcounts = (uint32*) CheckMalloc(tif, nstrips * sizeof (uint32),
				"for chopped \"StripByteCounts\" array");
	newoffsets = (uint32*) CheckMalloc(tif, nstrips * sizeof (uint32),
				"for chopped \"StripOffsets\" array");
	if (newcounts == NULL || newoffsets == NULL) {
	        /*
		 * Unable to allocate new strip information, give
		 * up and use the original one strip information.
		 */
		if (newcounts != NULL)
			_TIFFfree(newcounts);
		if (newoffsets != NULL)
			_TIFFfree(newoffsets);
		return;
	}
	/*
	 * Fill the strip information arrays with
	 * new bytecounts and offsets that reflect
	 * the broken-up format.
	 */
	for (strip = 0; strip < nstrips; strip++) {
		if (stripbytes > bytecount)
			stripbytes = bytecount;
		newcounts[strip] = stripbytes;
		newoffsets[strip] = offset;
		offset += stripbytes;
		bytecount -= stripbytes;
	}
	/*
	 * Replace old single strip info with multi-strip info.
	 */
	td->td_stripsperimage = td->td_nstrips = nstrips;
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);

	_TIFFfree(td->td_stripbytecount);
	_TIFFfree(td->td_stripoffset);
	td->td_stripbytecount = newcounts;
	td->td_stripoffset = newoffsets;
}
#endif /* STRIPCHOP_SUPPORT */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
