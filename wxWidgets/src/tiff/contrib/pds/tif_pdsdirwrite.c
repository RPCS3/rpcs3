/* $Header: /cvs/maptools/cvsroot/libtiff/contrib/pds/tif_pdsdirwrite.c,v 1.4 2010-06-08 18:55:15 bfriesen Exp $ */

/* When writing data to TIFF files, it is often useful to store application-
   specific data in a private TIFF directory so that the tags don't need to
   be registered and won't conflict with other people's user-defined tags.
   One needs to have a registered public tag which contains some amount of
   raw data. That raw data, however, is interpreted at an independent,
   separate, private tiff directory. This file provides some routines which
   will be useful for converting that data from its raw binary form into
   the proper form for your application.
*/

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
 * TIFFWritePrivateDataSubDirectory(). This function is intended for use
 * in writing a private subdirectory structure into a TIFF file. The
 * actual reading of data from the structure is handled by the getFieldFn(),
 * which is passed to TIFFWritePrivateDataSubDirectory() as a parameter. The
 * idea is to enable any application wishing to read private subdirectories to
 * do so easily using this function, without modifying the TIFF library.
 *
 * The astute observer will notice that only two functions are at all different
 * from the original tif_dirwrite.c file: TIFFWritePrivateDataSubDirectory()and
 * TIFFWriteNormalSubTag(). All the other stuff that makes this file so huge
 * is only necessary because all of those functions are declared static in
 * tif_dirwrite.c, so we have to totally duplicate them in order to use them.
 *
 * Oh, also please note the bug-fix in the routine TIFFWriteNormalSubTag(),
 * which equally should be applied to TIFFWriteNormalTag().
 *
 */
#include "tiffiop.h"

#if HAVE_IEEEFP
#define	TIFFCvtNativeToIEEEFloat(tif, n, fp)
#define	TIFFCvtNativeToIEEEDouble(tif, n, dp)
#else
extern	void TIFFCvtNativeToIEEEFloat(TIFF*, uint32, float*);
extern	void TIFFCvtNativeToIEEEDouble(TIFF*, uint32, double*);
#endif

static	int TIFFWriteNormalTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*);
static	int TIFFWriteNormalSubTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*,
				  int (*getFieldFn)(TIFF *tif,ttag_t tag,...));
static	void TIFFSetupShortLong(TIFF*, ttag_t, TIFFDirEntry*, uint32);
static	int TIFFSetupShortPair(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleShorts(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleAnys(TIFF*, TIFFDataType, ttag_t, TIFFDirEntry*);
static	int TIFFWriteShortTable(TIFF*, ttag_t, TIFFDirEntry*, uint32, uint16**);
static	int TIFFWriteShortArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, uint16*);
static	int TIFFWriteLongArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, uint32*);
static	int TIFFWriteRationalArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, float*);
static	int TIFFWriteFloatArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, float*);
static	int TIFFWriteDoubleArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
static	int TIFFWriteByteArray(TIFF*, TIFFDirEntry*, char*);
static	int TIFFWriteAnyArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
#ifdef COLORIMETRY_SUPPORT
static	int TIFFWriteTransferFunction(TIFF*, TIFFDirEntry*);
#endif
static	int TIFFWriteData(TIFF*, TIFFDirEntry*, char*);
static	int TIFFLinkDirectory(TIFF*);

#define	WriteRationalPair(type, tag1, v1, tag2, v2) {		\
	if (!TIFFWriteRational(tif, type, tag1, dir, v1))	\
		goto bad;					\
	if (!TIFFWriteRational(tif, type, tag2, dir+1, v2))	\
		goto bad;					\
	dir++;							\
}
#define	TIFFWriteRational(tif, type, tag, dir, v) \
	TIFFWriteRationalArray((tif), (type), (tag), (dir), 1, &(v))
#ifndef TIFFWriteRational
static	int TIFFWriteRational(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, float);
#endif

/* This function will write an entire directory to the disk, and return the
   offset value indicating where in the file it wrote the beginning of the
   directory structure. This is NOT the same as the offset value before
   calling this function, because some of the fields may have caused various
   data items to be written out BEFORE writing the directory structure.

   This code was basically written by ripping of the TIFFWriteDirectory() 
   code and generalizing it, using RPS's TIFFWritePliIfd() code for
   inspiration.  My original goal was to make this code general enough that
   the original TIFFWriteDirectory() could be rewritten to just call this
   function with the appropriate field and field-accessing arguments.

   However, now I realize that there's a lot of code that gets executed for
   the main, standard TIFF directories that does not apply to special
   private subdirectories, so such a reimplementation for the sake of
   eliminating redundant or duplicate code is probably not possible,
   unless we also pass in a Main flag to indiciate which type of handling
   to do, which would be kind of a hack. I've marked those places where I
   changed or ripped out code which would have to be re-inserted to
   generalize this function. If it can be done in a clean and graceful way,
   it would be a great way to generalize the TIFF library. Otherwise, I'll
   just leave this code here where it duplicates but remains on top of and
   hopefully mostly independent of the main TIFF library.

   The caller will probably want to free the sub directory structure after
   returning from this call, since otherwise once written out, the user
   is likely to forget about it and leave data lying around.
*/
toff_t
TIFFWritePrivateDataSubDirectory(TIFF* tif,
				 uint32 pdir_fieldsset[], int pdir_fields_last,
				 TIFFFieldInfo *field_info,
				 int (*getFieldFn)(TIFF *tif, ttag_t tag, ...))
{
	uint16 dircount;
	uint32 diroff, nextdiroff;
	ttag_t tag;
	uint32 nfields;
	tsize_t dirsize;
	char* data;
	TIFFDirEntry* dir;
	u_long b, *fields, fields_size;
	toff_t directory_offset;
	TIFFFieldInfo* fip;

	/*
	 * Deleted out all of the encoder flushing and such code from here -
	 * not necessary for subdirectories.
	 */

	/* Finish writing out any image data. */
	TIFFFlushData(tif);

	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= pdir_fields_last; b++)
		if (FieldSet(pdir_fieldsset, b))
			/* Deleted code to make size of first 4 tags 2
			   instead of 1. */
			nfields += 1;
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = (char*) _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		    "Cannot write private subdirectory, out of space");
		return (0);
	}
	/*
	 * Place directory in data section of the file. If there isn't one
	 * yet, place it at the end of the file. The directory is treated as
	 * data, so we don't link it into the directory structure at all.
	 */
	if (tif->tif_dataoff == 0)
	    tif->tif_dataoff =(TIFFSeekFile(tif, (toff_t) 0, SEEK_END)+1) &~ 1;
	diroff = tif->tif_dataoff;
	tif->tif_dataoff = (toff_t)(
	    diroff + sizeof (uint16) + dirsize + sizeof (toff_t));
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	/*tif->tif_curdir++;*/
	dir = (TIFFDirEntry*) data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	/*
	 * We make a local copy of the fieldsset here so that we don't mess
	 * up the original one when we call ResetFieldBit(). But I'm not sure
	 * why the original code calls ResetFieldBit(), since we're already
	 * going through the fields in order...
	 *
	 * fields_size is the number of uint32's we will need to hold the
	 * bit-mask for all of the fields. If our highest field number is
	 * 100, then we'll need 100 / (8*4)+1 == 4 uint32's to hold the
	 * fieldset.
	 *
	 * Unlike the original code, we allocate fields dynamically based
	 * on the requested pdir_fields_last value, allowing private
	 * data subdirectories to contain more than the built-in code's limit
	 * of 95 tags in a directory.
	 */
	fields_size = pdir_fields_last / (8*sizeof(uint32)) + 1;
	fields = _TIFFmalloc(fields_size*sizeof(uint32));
	_TIFFmemcpy(fields, pdir_fieldsset, fields_size * sizeof(uint32));

	/* Deleted "write out extra samples tag" code here. */

	/* Deleted code for checking a billion little special cases for the
	 * standard TIFF tags. Should add a general mechanism for overloading
	 * write function for each field, just like Brian kept telling me!!!
	 */
	for (fip = field_info; fip->field_tag; fip++) {
		/* Deleted code to check for FIELD_IGNORE!! */
		if (/* fip->field_bit == FIELD_IGNORE || */
		    !FieldSet(fields, fip->field_bit))
			continue;
		if (!TIFFWriteNormalSubTag(tif, dir, fip, getFieldFn))
			goto bad;
		dir++;
		ResetFieldBit(fields, fip->field_bit);
	}

	/* Now we've written all of the referenced data, and are about to
	   write the main directory structure, so grab the tif_dataoff value
	   now so we can remember where we wrote the directory. */
	directory_offset = tif->tif_dataoff;

	/*
	 * Write directory.
	 */
	dircount = (uint16) nfields;
	/* Deleted code to link to the next directory - we set it to zero! */
	nextdiroff = 0;
	if (tif->tif_flags & TIFF_SWAB) {
		/*
		 * The file's byte order is opposite to the
		 * native machine architecture.  We overwrite
		 * the directory information with impunity
		 * because it'll be released below after we
		 * write it to the file.  Note that all the
		 * other tag construction routines assume that
		 * we do this byte-swapping; i.e. they only
		 * byte-swap indirect data.
		 */
		for (dir = (TIFFDirEntry*) data; dircount; dir++, dircount--) {
			TIFFSwabArrayOfShort(&dir->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dir->tdir_count, 2);
		}
		dircount = (uint16) nfields;
		TIFFSwabShort(&dircount);
		TIFFSwabLong(&nextdiroff);
	}

	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	if (!WriteOK(tif, &dircount, sizeof (dircount))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error writing private subdirectory count");
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error writing private subdirectory contents");
		goto bad;
	}
	if (!WriteOK(tif, &nextdiroff, sizeof (nextdiroff))) {
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error writing private subdirectory link");
		goto bad;
	}
	tif->tif_dataoff += sizeof(dircount) + dirsize + sizeof(nextdiroff);

	_TIFFfree(data);
	_TIFFfree(fields);
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;

#if (0)
	/* This stuff commented out because I don't think we want it for
	   subdirectories, but I could be wrong. */
	(*tif->tif_cleanup)(tif);

	/*
	 * Reset directory-related state for subsequent
	 * directories.
	 */
	TIFFDefaultDirectory(tif);
	tif->tif_curoff = 0;
	tif->tif_row = (uint32) -1;
	tif->tif_curstrip = (tstrip_t) -1;
#endif

	return (directory_offset);
bad:
	_TIFFfree(data);
	_TIFFfree(fields);
	return (0);
}
#undef WriteRationalPair

/*
 * Process tags that are not special cased.
 */
/* The standard function TIFFWriteNormalTag() could definitely be replaced
   with a simple call to this function, just adding TIFFGetField() as the
   last argument. */
static int
TIFFWriteNormalSubTag(TIFF* tif, TIFFDirEntry* dir, const TIFFFieldInfo* fip,
		      int (*getFieldFn)(TIFF *tif, ttag_t tag, ...))
{
	u_short wc = (u_short) fip->field_writecount;

	dir->tdir_tag = fip->field_tag;
	dir->tdir_type = (u_short) fip->field_type;
	dir->tdir_count = wc;
#define	WRITEF(x,y)	x(tif, fip->field_type, fip->field_tag, dir, wc, y)
	switch (fip->field_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (wc > 1) {
			uint16* wp;
			if (wc == (u_short) TIFF_VARIABLE) {
				(*getFieldFn)(tif, fip->field_tag, &wc, &wp);
				dir->tdir_count = wc;
			} else
				(*getFieldFn)(tif, fip->field_tag, &wp);
			if (!WRITEF(TIFFWriteShortArray, wp))
				return (0);
		} else {
			uint16 sv;
			(*getFieldFn)(tif, fip->field_tag, &sv);
			dir->tdir_offset =
			    TIFFInsertData(tif, dir->tdir_type, sv);
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
		if (wc > 1) {
			uint32* lp;
			if (wc == (u_short) TIFF_VARIABLE) {
				(*getFieldFn)(tif, fip->field_tag, &wc, &lp);
				dir->tdir_count = wc;
			} else
				(*getFieldFn)(tif, fip->field_tag, &lp);
			if (!WRITEF(TIFFWriteLongArray, lp))
				return (0);
		} else {
			/* XXX handle LONG->SHORT conversion */
			(*getFieldFn)(tif, fip->field_tag, &dir->tdir_offset);
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (wc > 1) {
			float* fp;
			if (wc == (u_short) TIFF_VARIABLE) {
				(*getFieldFn)(tif, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			} else
				(*getFieldFn)(tif, fip->field_tag, &fp);
			if (!WRITEF(TIFFWriteRationalArray, fp))
				return (0);
		} else {
			float fv;
			(*getFieldFn)(tif, fip->field_tag, &fv);
			if (!WRITEF(TIFFWriteRationalArray, &fv))
				return (0);
		}
		break;
	case TIFF_FLOAT:
		if (wc > 1) {
			float* fp;
			if (wc == (u_short) TIFF_VARIABLE) {
				(*getFieldFn)(tif, fip->field_tag, &wc, &fp);
				dir->tdir_count = wc;
			} else
				(*getFieldFn)(tif, fip->field_tag, &fp);
			if (!WRITEF(TIFFWriteFloatArray, fp))
				return (0);
		} else {
			float fv;
			(*getFieldFn)(tif, fip->field_tag, &fv);
			if (!WRITEF(TIFFWriteFloatArray, &fv))
				return (0);
		}
		break;
	case TIFF_DOUBLE:
		/* Hey - I think this is a bug, or at least a "gross
		   inconsistency", in the TIFF library. Look at the original
		   TIFF library code below within the "#if (0) ... #else".
		   Just from the type of *dp, you can see that this code
		   expects TIFFGetField() to be handed a double ** for
		   any TIFF_DOUBLE tag, even for the constant wc==1 case.
		   This is totally inconsistent with other fields (like
		   TIFF_FLOAT, above) and is also inconsistent with the
		   TIFFSetField() function for TIFF_DOUBLEs, which expects
		   to be passed a single double by value for the wc==1 case.
		   (See the handling of TIFFFetchNormalTag() in tif_dirread.c
		   for an example.) Maybe this function was written before
		   TIFFWriteDoubleArray() was written, not that that's an
		   excuse. Anyway, the new code below is a trivial modification
		   of the TIFF_FLOAT code above. The fact that even single
		   doubles get written out in the data segment and get an
		   offset value stored is irrelevant here - that is all
		   handled by TIFFWriteDoubleArray(). */
#if (0)
		{ double* dp;
		  if (wc == (u_short) TIFF_VARIABLE) {
			(*getFieldFn)(tif, fip->field_tag, &wc, &dp);
			dir->tdir_count = wc;
		  } else
			(*getFieldFn)(tif, fip->field_tag, &dp);
		  TIFFCvtNativeToIEEEDouble(tif, wc, dp);
		  if (!TIFFWriteData(tif, dir, (char*) dp))
			return (0);
		}
#else
		if (wc > 1) {
			double* dp;
			if (wc == (u_short) TIFF_VARIABLE) {
				(*getFieldFn)(tif, fip->field_tag, &wc, &dp);
				dir->tdir_count = wc;
			} else
				(*getFieldFn)(tif, fip->field_tag, &dp);
			if (!WRITEF(TIFFWriteDoubleArray, dp))
				return (0);
		} else {
			double dv;
			(*getFieldFn)(tif, fip->field_tag, &dv);
			if (!WRITEF(TIFFWriteDoubleArray, &dv))
				return (0);
		}
#endif
		break;
	case TIFF_ASCII:
		{ char* cp;
		  (*getFieldFn)(tif, fip->field_tag, &cp);
		  dir->tdir_count = (uint32) (strlen(cp) + 1);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;
	case TIFF_UNDEFINED:
		{ char* cp;
		  if (wc == (u_short) TIFF_VARIABLE) {
			(*getFieldFn)(tif, fip->field_tag, &wc, &cp);
			dir->tdir_count = wc;
		  } else 
			(*getFieldFn)(tif, fip->field_tag, &cp);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;
	}
	return (1);
}
#undef WRITEF

/* Everything after this is exactly duplicated from the standard tif_dirwrite.c
   file, necessitated by the fact that they are declared static there so
   we can't call them!
*/
/*
 * Setup a directory entry with either a SHORT
 * or LONG type according to the value.
 */
static void
TIFFSetupShortLong(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint32 v)
{
	dir->tdir_tag = tag;
	dir->tdir_count = 1;
	if (v > 0xffffL) {
		dir->tdir_type = (short) TIFF_LONG;
		dir->tdir_offset = v;
	} else {
		dir->tdir_type = (short) TIFF_SHORT;
		dir->tdir_offset = TIFFInsertData(tif, (int) TIFF_SHORT, v);
	}
}
#undef MakeShortDirent

#ifndef TIFFWriteRational
/*
 * Setup a RATIONAL directory entry and
 * write the associated indirect value.
 */
static int
TIFFWriteRational(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, float v)
{
	return (TIFFWriteRationalArray(tif, type, tag, dir, 1, &v));
}
#endif

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry that references a
 * samples/pixel array of SHORT values and
 * (potentially) write the associated indirect
 * values.
 */
static int
TIFFWritePerSampleShorts(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 buf[10], v;
	uint16* w = buf;
	int i, status, samples = tif->tif_dir.td_samplesperpixel;

	if (samples > NITEMS(buf))
		w = (uint16*) _TIFFmalloc(samples * sizeof (uint16));
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree((char*) w);
	return (status);
}

/*
 * Setup a directory entry that references a samples/pixel array of ``type''
 * values and (potentially) write the associated indirect values.  The source
 * data from TIFFGetField() for the specified tag must be returned as double.
 */
static int
TIFFWritePerSampleAnys(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir)
{
	double buf[10], v;
	double* w = buf;
	int i, status;
	int samples = (int) tif->tif_dir.td_samplesperpixel;

	if (samples > NITEMS(buf))
		w = (double*) _TIFFmalloc(samples * sizeof (double));
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteAnyArray(tif, type, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree(w);
	return (status);
}
#undef NITEMS

/*
 * Setup a pair of shorts that are returned by
 * value, rather than as a reference to an array.
 */
static int
TIFFSetupShortPair(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 v[2];

	TIFFGetField(tif, tag, &v[0], &v[1]);
	return (TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, 2, v));
}

/*
 * Setup a directory entry for an NxM table of shorts,
 * where M is known to be 2**bitspersample, and write
 * the associated indirect data.
 */
static int
TIFFWriteShortTable(TIFF* tif,
    ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16** table)
{
	uint32 i, off;

	dir->tdir_tag = tag;
	dir->tdir_type = (short) TIFF_SHORT;
	/* XXX -- yech, fool TIFFWriteData */
	dir->tdir_count = (uint32) (1L<<tif->tif_dir.td_bitspersample);
	off = tif->tif_dataoff;
	for (i = 0; i < n; i++)
		if (!TIFFWriteData(tif, dir, (char *)table[i]))
			return (0);
	dir->tdir_count *= n;
	dir->tdir_offset = off;
	return (1);
}

/*
 * Write/copy data associated with an ASCII or opaque tag value.
 */
static int
TIFFWriteByteArray(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (dir->tdir_count > 4) {
		if (!TIFFWriteData(tif, dir, cp))
			return (0);
	} else
		_TIFFmemcpy(&dir->tdir_offset, cp, dir->tdir_count);
	return (1);
}

/*
 * Setup a directory entry of an array of SHORT
 * or SSHORT and write the associated indirect values.
 */
static int
TIFFWriteShortArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16* v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	if (n <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (uint32) ((long) v[0] << 16);
			if (n == 2)
				dir->tdir_offset |= v[1] & 0xffff;
		} else {
			dir->tdir_offset = v[0] & 0xffff;
			if (n == 2)
				dir->tdir_offset |= (long) v[1] << 16;
		}
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of LONG
 * or SLONG and write the associated indirect values.
 */
static int
TIFFWriteLongArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, uint32* v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	if (n == 1) {
		dir->tdir_offset = v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of RATIONAL
 * or SRATIONAL and write the associated indirect values.
 */
static int
TIFFWriteRationalArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, float* v)
{
	uint32 i;
	uint32* t;
	int status;

	dir->tdir_tag = tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	t = (uint32*) _TIFFmalloc(2*n * sizeof (uint32));
	for (i = 0; i < n; i++) {
		float fv = v[i];
		int sign = 1;
		uint32 den;

		if (fv < 0) {
			if (type == TIFF_RATIONAL) {
				TIFFWarning(tif->tif_name,
	"\"%s\": Information lost writing value (%g) as (unsigned) RATIONAL",
				_TIFFFieldWithTag(tif,tag)->field_name, v);
				fv = 0;
			} else
				fv = -fv, sign = -1;
		}
		den = 1L;
		if (fv > 0) {
			while (fv < 1L<<(31-3) && den < 1L<<(31-3))
				fv *= 1<<3, den *= 1L<<3;
		}
		t[2*i+0] = sign * (fv + 0.5);
		t[2*i+1] = den;
	}
	status = TIFFWriteData(tif, dir, (char *)t);
	_TIFFfree((char*) t);
	return (status);
}

static int
TIFFWriteFloatArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, float* v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	TIFFCvtNativeToIEEEFloat(tif, n, v);
	if (n == 1) {
		dir->tdir_offset = *(uint32*) &v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

static int
TIFFWriteDoubleArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	TIFFCvtNativeToIEEEDouble(tif, n, v);
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Write an array of ``type'' values for a specified tag (i.e. this is a tag
 * which is allowed to have different types, e.g. SMaxSampleType).
 * Internally the data values are represented as double since a double can
 * hold any of the TIFF tag types (yes, this should really be an abstract
 * type tany_t for portability).  The data is converted into the specified
 * type in a temporary buffer and then handed off to the appropriate array
 * writer.
 */
static int
TIFFWriteAnyArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	char buf[10 * sizeof(double)];
	char* w = buf;
	int i, status = 0;

	if (n * TIFFDataWidth(type) > sizeof buf)
		w = (char*) _TIFFmalloc(n * TIFFDataWidth(type));
	switch (type) {
	case TIFF_BYTE:
		{ unsigned char* bp = (unsigned char*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (unsigned char) v[i];
		  dir->tdir_tag = tag;
		  dir->tdir_type = (short) type;
		  dir->tdir_count = n;
		  if (!TIFFWriteByteArray(tif, dir, (char*) bp))
			goto out;
		}
		break;
	case TIFF_SBYTE:
		{ signed char* bp = (signed char*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (signed char) v[i];
		  dir->tdir_tag = tag;
		  dir->tdir_type = (short) type;
		  dir->tdir_count = n;
		  if (!TIFFWriteByteArray(tif, dir, (char*) bp))
			goto out;
		}
		break;
	case TIFF_SHORT:
		{ uint16* bp = (uint16*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (uint16) v[i];
		  if (!TIFFWriteShortArray(tif, type, tag, dir, n, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_SSHORT:
		{ int16* bp = (int16*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (int16) v[i];
		  if (!TIFFWriteShortArray(tif, type, tag, dir, n, (uint16*)bp))
			goto out;
		}
		break;
	case TIFF_LONG:
		{ uint32* bp = (uint32*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (uint32) v[i];
		  if (!TIFFWriteLongArray(tif, type, tag, dir, n, bp))
			goto out;
		}
		break;
	case TIFF_SLONG:
		{ int32* bp = (int32*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (int32) v[i];
		  if (!TIFFWriteLongArray(tif, type, tag, dir, n, (uint32*) bp))
			goto out;
		}
		break;
	case TIFF_FLOAT:
		{ float* bp = (float*) w;
		  for (i = 0; i < n; i++)
			bp[i] = (float) v[i];
		  if (!TIFFWriteFloatArray(tif, type, tag, dir, n, bp))
			goto out;
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFWriteDoubleArray(tif, type, tag, dir, n, v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		/* TIFF_RATIONAL */
		/* TIFF_SRATIONAL */
		goto out;
	}
	status = 1;
 out:
	if (w != buf)
		_TIFFfree(w);
	return (status);
}

#ifdef COLORIMETRY_SUPPORT
static int
TIFFWriteTransferFunction(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;
	tsize_t n = (1L<<td->td_bitspersample) * sizeof (uint16);
	uint16** tf = td->td_transferfunction;
	int ncols;

	/*
	 * Check if the table can be written as a single column,
	 * or if it must be written as 3 columns.  Note that we
	 * write a 3-column tag if there are 2 samples/pixel and
	 * a single column of data won't suffice--hmm.
	 */
	switch (td->td_samplesperpixel - td->td_extrasamples) {
	default:	if (_TIFFmemcmp(tf[0], tf[2], n)) { ncols = 3; break; }
	case 2:		if (_TIFFmemcmp(tf[0], tf[1], n)) { ncols = 3; break; }
	case 1: case 0:	ncols = 1;
	}
	return (TIFFWriteShortTable(tif,
	    TIFFTAG_TRANSFERFUNCTION, dir, ncols, tf));
}
#endif

/*
 * Write a contiguous directory item.
 */
static int
TIFFWriteData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	tsize_t cc;

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
	dir->tdir_offset = tif->tif_dataoff;
	cc = dir->tdir_count * TIFFDataWidth(dir->tdir_type);
	if (SeekOK(tif, dir->tdir_offset) &&
	    WriteOK(tif, cp, cc)) {
		tif->tif_dataoff += (cc + 1) & ~1;
		return (1);
	}
	TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error writing data for field \"%s\"",
	    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Link the current directory into the
 * directory chain for the file.
 */
static int
TIFFLinkDirectory(TIFF* tif)
{
	static const char module[] = "TIFFLinkDirectory";
	uint32 nextdir;
	uint32 diroff;

	tif->tif_diroff = (TIFFSeekFile(tif, (toff_t) 0, SEEK_END)+1) &~ 1;
	diroff = (uint32) tif->tif_diroff;
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong(&diroff);
#if SUBIFD_SUPPORT
	if (tif->tif_flags & TIFF_INSUBIFD) {
		(void) TIFFSeekFile(tif, tif->tif_subifdoff, SEEK_SET);
		if (!WriteOK(tif, &diroff, sizeof (diroff))) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Error writing SubIFD directory link",
			    tif->tif_name);
			return (0);
		}
		/*
		 * Advance to the next SubIFD or, if this is
		 * the last one configured, revert back to the
		 * normal directory linkage.
		 */
		if (--tif->tif_nsubifd)
			tif->tif_subifdoff += sizeof (diroff);
		else
			tif->tif_flags &= ~TIFF_INSUBIFD;
		return (1);
	}
#endif
	if (tif->tif_header.tiff_diroff == 0) {
		/*
		 * First directory, overwrite offset in header.
		 */
		tif->tif_header.tiff_diroff = (uint32) tif->tif_diroff;
#define	HDROFF(f)	((toff_t) &(((TIFFHeader*) 0)->f))
		(void) TIFFSeekFile(tif, HDROFF(tiff_diroff), SEEK_SET);
		if (!WriteOK(tif, &diroff, sizeof (diroff))) {
			TIFFErrorExt(tif->tif_clientdata, tif->tif_name, "Error writing TIFF header");
			return (0);
		}
		return (1);
	}
	/*
	 * Not the first directory, search to the last and append.
	 */
	nextdir = tif->tif_header.tiff_diroff;
	do {
		uint16 dircount;

		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (dircount))) {
			TIFFErrorExt(tif->tif_clientdata, module, "Error fetching directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		(void) TIFFSeekFile(tif,
		    dircount * sizeof (TIFFDirEntry), SEEK_CUR);
		if (!ReadOK(tif, &nextdir, sizeof (nextdir))) {
			TIFFErrorExt(tif->tif_clientdata, module, "Error fetching directory link");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&nextdir);
	} while (nextdir != 0);
	(void) TIFFSeekFile(tif, -(toff_t) sizeof (nextdir), SEEK_CUR);
	if (!WriteOK(tif, &diroff, sizeof (diroff))) {
		TIFFErrorExt(tif->tif_clientdata, module, "Error writing directory link");
		return (0);
	}
	return (1);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
