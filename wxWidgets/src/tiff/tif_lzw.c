/* $Header$ */

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

#include "tiffiop.h"
#ifdef LZW_SUPPORT
/*
 * TIFF Library.
 * Rev 5.0 Lempel-Ziv & Welch Compression Support
 *
 * This code is derived from the compress program whose code is
 * derived from software contributed to Berkeley by James A. Woods,
 * derived from original work by Spencer Thomas and Joseph Orost.
 *
 * The original Berkeley copyright notice appears below in its entirety.
 */
#include "tif_predict.h"

#include <assert.h>
#include <stdio.h>

/*
 * NB: The 5.0 spec describes a different algorithm than Aldus
 *     implements.  Specifically, Aldus does code length transitions
 *     one code earlier than should be done (for real LZW).
 *     Earlier versions of this library implemented the correct
 *     LZW algorithm, but emitted codes in a bit order opposite
 *     to the TIFF spec.  Thus, to maintain compatibility w/ Aldus
 *     we interpret MSB-LSB ordered codes to be images written w/
 *     old versions of this library, but otherwise adhere to the
 *     Aldus "off by one" algorithm.
 *
 * Future revisions to the TIFF spec are expected to "clarify this issue".
 */
#define	LZW_COMPAT		/* include backwards compatibility code */
/*
 * Each strip of data is supposed to be terminated by a CODE_EOI.
 * If the following #define is included, the decoder will also
 * check for end-of-strip w/o seeing this code.  This makes the
 * library more robust, but also slower.
 */
#define	LZW_CHECKEOS		/* include checks for strips w/o EOI code */

#define MAXCODE(n)	((1L<<(n))-1)
/*
 * The TIFF spec specifies that encoded bit
 * strings range from 9 to 12 bits.
 */
#define	BITS_MIN	9		/* start with 9 bits */
#define	BITS_MAX	12		/* max of 12 bit strings */
/* predefined codes */
#define	CODE_CLEAR	256		/* code to clear string table */
#define	CODE_EOI	257		/* end-of-information code */
#define CODE_FIRST	258		/* first free code entry */
#define	CODE_MAX	MAXCODE(BITS_MAX)
#define	HSIZE		9001L		/* 91% occupancy */
#define	HSHIFT		(13-8)
#ifdef LZW_COMPAT
/* NB: +1024 is for compatibility with old files */
#define	CSIZE		(MAXCODE(BITS_MAX)+1024L)
#else
#define	CSIZE		(MAXCODE(BITS_MAX)+1L)
#endif

/*
 * State block for each open TIFF file using LZW
 * compression/decompression.  Note that the predictor
 * state block must be first in this data structure.
 */
typedef	struct {
	TIFFPredictorState predict;	/* predictor super class */

	u_short		nbits;		/* # of bits/code */
	u_short		maxcode;	/* maximum code for lzw_nbits */
	u_short		free_ent;	/* next free entry in hash table */
	long		nextdata;	/* next bits of i/o */
	long		nextbits;	/* # of valid bits in lzw_nextdata */
} LZWBaseState;

#define	lzw_nbits	base.nbits
#define	lzw_maxcode	base.maxcode
#define	lzw_free_ent	base.free_ent
#define	lzw_nextdata	base.nextdata
#define	lzw_nextbits	base.nextbits

/*
 * Encoding-specific state.
 */
typedef uint16 hcode_t;			/* codes fit in 16 bits */

/*
 * Decoding-specific state.
 */
typedef struct code_ent {
	struct code_ent *next;
	u_short	length;			/* string len, including this token */
	u_char	value;			/* data value */
	u_char	firstchar;		/* first token of string */
} code_t;

typedef	int (*decodeFunc)(TIFF*, tidata_t, tsize_t, tsample_t);

typedef struct {
	LZWBaseState base;

        /* Decoding specific data */
	long	dec_nbitsmask;		/* lzw_nbits 1 bits, right adjusted */
	long	dec_restart;		/* restart count */
#ifdef LZW_CHECKEOS
	long	dec_bitsleft;		/* available bits in raw data */
#endif
	decodeFunc dec_decode;		/* regular or backwards compatible */
	code_t*	dec_codep;		/* current recognized code */
	code_t*	dec_oldcodep;		/* previously recognized code */
	code_t*	dec_free_entp;		/* next free entry */
	code_t*	dec_maxcodep;		/* max available entry */
	code_t*	dec_codetab;		/* kept separate for small machines */
} LZWCodecState;

#define	LZWState(tif)		((LZWBaseState*) (tif)->tif_data)
#define	DecoderState(tif)	((LZWCodecState*) LZWState(tif))

static	int LZWDecode(TIFF*, tidata_t, tsize_t, tsample_t);
#ifdef LZW_COMPAT
static	int LZWDecodeCompat(TIFF*, tidata_t, tsize_t, tsample_t);
#endif

/*
 * LZW Decoder.
 */

#ifdef LZW_CHECKEOS
/*
 * This check shouldn't be necessary because each
 * strip is suppose to be terminated with CODE_EOI.
 */
#define	NextCode(_tif, _sp, _bp, _code, _get) {				\
	if ((_sp)->dec_bitsleft < nbits) {				\
		TIFFWarning(_tif->tif_name,				\
		    "LZWDecode: Strip %d not terminated with EOI code", \
		    _tif->tif_curstrip);				\
		_code = CODE_EOI;					\
	} else {							\
		_get(_sp,_bp,_code);					\
		(_sp)->dec_bitsleft -= nbits;				\
	}								\
}
#else
#define	NextCode(tif, sp, bp, code, get) get(sp, bp, code)
#endif

static int
LZWSetupDecode(TIFF* tif)
{
	LZWCodecState* sp = DecoderState(tif);
	static const char module[] = "LZWSetupDecode";
	int code;

	assert(sp != NULL);

	if (sp->dec_codetab == NULL) {
		sp->dec_codetab = (code_t*)_TIFFmalloc(CSIZE*sizeof (code_t));
		if (sp->dec_codetab == NULL) {
			TIFFError(module, "No space for LZW code table");
			return (0);
		}
		/*
		 * Pre-load the table.
		 */
                code = 255;
                do {
                    sp->dec_codetab[code].value = (u_char) code;
                    sp->dec_codetab[code].firstchar = (u_char) code;
                    sp->dec_codetab[code].length = 1;
                    sp->dec_codetab[code].next = NULL;
                } while (code--);
	}
	return (1);
}

/*
 * Setup state for decoding a strip.
 */
static int
LZWPreDecode(TIFF* tif, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);

	(void) s;
	assert(sp != NULL);
	/*
	 * Check for old bit-reversed codes.
	 */
	if (tif->tif_rawdata[0] == 0 && (tif->tif_rawdata[1] & 0x1)) {
#ifdef LZW_COMPAT
		if (!sp->dec_decode) {
			TIFFWarning(tif->tif_name,
			    "Old-style LZW codes, convert file");
			/*
			 * Override default decoding methods with
			 * ones that deal with the old coding.
			 * Otherwise the predictor versions set
			 * above will call the compatibility routines
			 * through the dec_decode method.
			 */
			tif->tif_decoderow = LZWDecodeCompat;
			tif->tif_decodestrip = LZWDecodeCompat;
			tif->tif_decodetile = LZWDecodeCompat;
			/*
			 * If doing horizontal differencing, must
			 * re-setup the predictor logic since we
			 * switched the basic decoder methods...
			 */
			(*tif->tif_setupdecode)(tif);
			sp->dec_decode = LZWDecodeCompat;
		}
		sp->lzw_maxcode = MAXCODE(BITS_MIN);
#else /* !LZW_COMPAT */
		if (!sp->dec_decode) {
			TIFFError(tif->tif_name,
			    "Old-style LZW codes not supported");
			sp->dec_decode = LZWDecode;
		}
		return (0);
#endif/* !LZW_COMPAT */
	} else {
		sp->lzw_maxcode = MAXCODE(BITS_MIN)-1;
		sp->dec_decode = LZWDecode;
	}
	sp->lzw_nbits = BITS_MIN;
	sp->lzw_nextbits = 0;
	sp->lzw_nextdata = 0;

	sp->dec_restart = 0;
	sp->dec_nbitsmask = MAXCODE(BITS_MIN);
#ifdef LZW_CHECKEOS
	sp->dec_bitsleft = tif->tif_rawcc << 3;
#endif
	sp->dec_free_entp = sp->dec_codetab + CODE_FIRST;
	/*
	 * Zero entries that are not yet filled in.  We do
	 * this to guard against bogus input data that causes
	 * us to index into undefined entries.  If you can
	 * come up with a way to safely bounds-check input codes
	 * while decoding then you can remove this operation.
	 */
	_TIFFmemset(sp->dec_free_entp, 0, (CSIZE-CODE_FIRST)*sizeof (code_t));
	sp->dec_oldcodep = &sp->dec_codetab[-1];
	sp->dec_maxcodep = &sp->dec_codetab[sp->dec_nbitsmask-1];
	return (1);
}

/*
 * Decode a "hunk of data".
 */
#define	GetNextCode(sp, bp, code) {				\
	nextdata = (nextdata<<8) | *(bp)++;			\
	nextbits += 8;						\
	if (nextbits < nbits) {					\
		nextdata = (nextdata<<8) | *(bp)++;		\
		nextbits += 8;					\
	}							\
	code = (hcode_t)((nextdata >> (nextbits-nbits)) & nbitsmask);	\
	nextbits -= nbits;					\
}

static void
codeLoop(TIFF* tif)
{
	TIFFError(tif->tif_name,
	    "LZWDecode: Bogus encoding, loop in the code table; scanline %d",
	    tif->tif_row);
}

static int
LZWDecode(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);
	char *op = (char*) op0;
	long occ = (long) occ0;
	char *tp;
	u_char *bp;
	hcode_t code;
	int len;
	long nbits, nextbits, nextdata, nbitsmask;
	code_t *codep, *free_entp, *maxcodep, *oldcodep;

	(void) s;
	assert(sp != NULL);
	/*
	 * Restart interrupted output operation.
	 */
	if (sp->dec_restart) {
		long residue;

		codep = sp->dec_codep;
		residue = codep->length - sp->dec_restart;
		if (residue > occ) {
			/*
			 * Residue from previous decode is sufficient
			 * to satisfy decode request.  Skip to the
			 * start of the decoded string, place decoded
			 * values in the output buffer, and return.
			 */
			sp->dec_restart += occ;
			do {
				codep = codep->next;
			} while (--residue > occ && codep);
			if (codep) {
				tp = op + occ;
				do {
					*--tp = codep->value;
					codep = codep->next;
				} while (--occ && codep);
			}
			return (1);
		}
		/*
		 * Residue satisfies only part of the decode request.
		 */
		op += residue, occ -= residue;
		tp = op;
		do {
			int t;
			--tp;
			t = codep->value;
			codep = codep->next;
			*tp = (char) t;
		} while (--residue && codep);
		sp->dec_restart = 0;
	}

	bp = (u_char *)tif->tif_rawcp;
	nbits = sp->lzw_nbits;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	nbitsmask = sp->dec_nbitsmask;
	oldcodep = sp->dec_oldcodep;
	free_entp = sp->dec_free_entp;
	maxcodep = sp->dec_maxcodep;

	while (occ > 0) {
		NextCode(tif, sp, bp, code, GetNextCode);
		if (code == CODE_EOI)
			break;
		if (code == CODE_CLEAR) {
			free_entp = sp->dec_codetab + CODE_FIRST;
			nbits = BITS_MIN;
			nbitsmask = MAXCODE(BITS_MIN);
			maxcodep = sp->dec_codetab + nbitsmask-1;
			NextCode(tif, sp, bp, code, GetNextCode);
			if (code == CODE_EOI)
				break;
			*op++ = (char)code, occ--;
			oldcodep = sp->dec_codetab + code;
			continue;
		}
		codep = sp->dec_codetab + code;

		/*
	 	 * Add the new entry to the code table.
	 	 */
		if (free_entp < &sp->dec_codetab[0] ||
			free_entp >= &sp->dec_codetab[CSIZE]) {
			TIFFError(tif->tif_name,
			"LZWDecode: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}

		free_entp->next = oldcodep;
		if (free_entp->next < &sp->dec_codetab[0] ||
			free_entp->next >= &sp->dec_codetab[CSIZE]) {
			TIFFError(tif->tif_name,
			"LZWDecode: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}
		free_entp->firstchar = free_entp->next->firstchar;
		free_entp->length = free_entp->next->length+1;
		free_entp->value = (codep < free_entp) ?
		    codep->firstchar : free_entp->firstchar;
		if (++free_entp > maxcodep) {
			if (++nbits > BITS_MAX)		/* should not happen */
				nbits = BITS_MAX;
			nbitsmask = MAXCODE(nbits);
			maxcodep = sp->dec_codetab + nbitsmask-1;
		}
		oldcodep = codep;
		if (code >= 256) {
			/*
		 	 * Code maps to a string, copy string
			 * value to output (written in reverse).
		 	 */
			if(codep->length == 0) {
			    TIFFError(tif->tif_name,
	    		    "LZWDecode: Wrong length of decoded string: "
			    "data probably corrupted at scanline %d",
			    tif->tif_row);	
			    return (0);
			}
			if (codep->length > occ) {
				/*
				 * String is too long for decode buffer,
				 * locate portion that will fit, copy to
				 * the decode buffer, and setup restart
				 * logic for the next decoding call.
				 */
				sp->dec_codep = codep;
				do {
					codep = codep->next;
				} while (codep && codep->length > occ);
				if (codep) {
					sp->dec_restart = occ;
					tp = op + occ;
					do  {
						*--tp = codep->value;
						codep = codep->next;
					}  while (--occ && codep);
					if (codep)
						codeLoop(tif);
				}
				break;
			}
			len = codep->length;
			tp = op + len;
			do {
				int t;
				--tp;
				t = codep->value;
				codep = codep->next;
				*tp = (char) t;
			} while (codep && tp > op);
			if (codep) {
			    codeLoop(tif);
			    break;
			}
			op += len, occ -= len;
		} else
			*op++ = (char)code, occ--;
	}

	tif->tif_rawcp = (tidata_t) bp;
	sp->lzw_nbits = (u_short) nbits;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->dec_nbitsmask = nbitsmask;
	sp->dec_oldcodep = oldcodep;
	sp->dec_free_entp = free_entp;
	sp->dec_maxcodep = maxcodep;

	if (occ > 0) {
		TIFFError(tif->tif_name,
		"LZWDecode: Not enough data at scanline %d (short %d bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}

#ifdef LZW_COMPAT
/*
 * Decode a "hunk of data" for old images.
 */
#define	GetNextCodeCompat(sp, bp, code) {			\
	nextdata |= (u_long) *(bp)++ << nextbits;		\
	nextbits += 8;						\
	if (nextbits < nbits) {					\
		nextdata |= (u_long) *(bp)++ << nextbits;	\
		nextbits += 8;					\
	}							\
	code = (hcode_t)(nextdata & nbitsmask);			\
	nextdata >>= nbits;					\
	nextbits -= nbits;					\
}

static int
LZWDecodeCompat(TIFF* tif, tidata_t op0, tsize_t occ0, tsample_t s)
{
	LZWCodecState *sp = DecoderState(tif);
	char *op = (char*) op0;
	long occ = (long) occ0;
	char *tp;
	u_char *bp;
	int code, nbits;
	long nextbits, nextdata, nbitsmask;
	code_t *codep, *free_entp, *maxcodep, *oldcodep;

	(void) s;
	assert(sp != NULL);
	/*
	 * Restart interrupted output operation.
	 */
	if (sp->dec_restart) {
		long residue;

		codep = sp->dec_codep;
		residue = codep->length - sp->dec_restart;
		if (residue > occ) {
			/*
			 * Residue from previous decode is sufficient
			 * to satisfy decode request.  Skip to the
			 * start of the decoded string, place decoded
			 * values in the output buffer, and return.
			 */
			sp->dec_restart += occ;
			do {
				codep = codep->next;
			} while (--residue > occ);
			tp = op + occ;
			do {
				*--tp = codep->value;
				codep = codep->next;
			} while (--occ);
			return (1);
		}
		/*
		 * Residue satisfies only part of the decode request.
		 */
		op += residue, occ -= residue;
		tp = op;
		do {
			*--tp = codep->value;
			codep = codep->next;
		} while (--residue);
		sp->dec_restart = 0;
	}

	bp = (u_char *)tif->tif_rawcp;
	nbits = sp->lzw_nbits;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	nbitsmask = sp->dec_nbitsmask;
	oldcodep = sp->dec_oldcodep;
	free_entp = sp->dec_free_entp;
	maxcodep = sp->dec_maxcodep;

	while (occ > 0) {
		NextCode(tif, sp, bp, code, GetNextCodeCompat);
		if (code == CODE_EOI)
			break;
		if (code == CODE_CLEAR) {
			free_entp = sp->dec_codetab + CODE_FIRST;
			nbits = BITS_MIN;
			nbitsmask = MAXCODE(BITS_MIN);
			maxcodep = sp->dec_codetab + nbitsmask;
			NextCode(tif, sp, bp, code, GetNextCodeCompat);
			if (code == CODE_EOI)
				break;
			*op++ = (char) code, occ--;
			oldcodep = sp->dec_codetab + code;
			continue;
		}
		codep = sp->dec_codetab + code;

		/*
	 	 * Add the new entry to the code table.
	 	 */
		if (free_entp < &sp->dec_codetab[0] ||
			free_entp >= &sp->dec_codetab[CSIZE]) {
			TIFFError(tif->tif_name,
			"LZWDecodeCompat: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}

		free_entp->next = oldcodep;
		if (free_entp->next < &sp->dec_codetab[0] ||
			free_entp->next >= &sp->dec_codetab[CSIZE]) {
			TIFFError(tif->tif_name,
			"LZWDecodeCompat: Corrupted LZW table at scanline %d",
			tif->tif_row);
			return (0);
		}
		free_entp->firstchar = free_entp->next->firstchar;
		free_entp->length = free_entp->next->length+1;
		free_entp->value = (codep < free_entp) ?
		    codep->firstchar : free_entp->firstchar;
		if (++free_entp > maxcodep) {
			if (++nbits > BITS_MAX)		/* should not happen */
				nbits = BITS_MAX;
			nbitsmask = MAXCODE(nbits);
			maxcodep = sp->dec_codetab + nbitsmask;
		}
		oldcodep = codep;
		if (code >= 256) {
			/*
		 	 * Code maps to a string, copy string
			 * value to output (written in reverse).
		 	 */
			if(codep->length == 0) {
			    TIFFError(tif->tif_name,
	    		    "LZWDecodeCompat: Wrong length of decoded "
			    "string: data probably corrupted at scanline %d",
			    tif->tif_row);	
			    return (0);
			}
			if (codep->length > occ) {
				/*
				 * String is too long for decode buffer,
				 * locate portion that will fit, copy to
				 * the decode buffer, and setup restart
				 * logic for the next decoding call.
				 */
				sp->dec_codep = codep;
				do {
					codep = codep->next;
				} while (codep->length > occ);
				sp->dec_restart = occ;
				tp = op + occ;
				do  {
					*--tp = codep->value;
					codep = codep->next;
				}  while (--occ);
				break;
			}
			op += codep->length, occ -= codep->length;
			tp = op;
			do {
				*--tp = codep->value;
			} while( (codep = codep->next) != NULL);
		} else
			*op++ = (char) code, occ--;
	}

	tif->tif_rawcp = (tidata_t) bp;
	sp->lzw_nbits = (u_short) nbits;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->dec_nbitsmask = nbitsmask;
	sp->dec_oldcodep = oldcodep;
	sp->dec_free_entp = free_entp;
	sp->dec_maxcodep = maxcodep;

	if (occ > 0) {
		TIFFError(tif->tif_name,
	    "LZWDecodeCompat: Not enough data at scanline %d (short %d bytes)",
		    tif->tif_row, occ);
		return (0);
	}
	return (1);
}
#endif /* LZW_COMPAT */



static void
LZWCleanup(TIFF* tif)
{
    if (tif->tif_data) {
        if (DecoderState(tif)->dec_codetab)
            _TIFFfree(DecoderState(tif)->dec_codetab);
        _TIFFfree(tif->tif_data);
        tif->tif_data = NULL;
    }
}

static int
LZWSetupEncode(TIFF* tif)
{
    TIFFError(tif->tif_name,
              "LZW compression is not available to due to Unisys patent enforcement");
    return (0);
}

int
TIFFInitLZW(TIFF* tif, int scheme)
{
	assert(scheme == COMPRESSION_LZW);

	/*
	 * Allocate state block so tag methods have storage to record values.
	 */
	tif->tif_data = (tidata_t) _TIFFmalloc(sizeof (LZWCodecState));
	if (tif->tif_data == NULL)
		goto bad;
	DecoderState(tif)->dec_codetab = NULL;
	DecoderState(tif)->dec_decode = NULL;

	/*
	 * Install codec methods.
	 */
	tif->tif_setupencode = LZWSetupEncode;
	tif->tif_setupdecode = LZWSetupDecode;
	tif->tif_predecode = LZWPreDecode;
	tif->tif_decoderow = LZWDecode;
	tif->tif_decodestrip = LZWDecode;
	tif->tif_decodetile = LZWDecode;
	tif->tif_cleanup = LZWCleanup;

	/*
	 * Setup predictor setup.
	 */
	(void) TIFFPredictorInit(tif);

	return (1);

bad:
	TIFFError("TIFFInitLZW", "No space for LZW state block");
	return (0);
}

/*
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#endif /* LZW_SUPPORT */
