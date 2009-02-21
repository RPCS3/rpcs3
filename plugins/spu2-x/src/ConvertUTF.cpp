/*
 * Copyright 2001-2004 Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

/* ---------------------------------------------------------------------

    Conversions between UTF32, UTF-16, and UTF-8. Source code file.
    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
    Sept 2001: fixed const & error conditions per
	mods suggested by S. Parent & A. Lillich.
    June 2002: Tim Dodd added detection and handling of incomplete
	source sequences, enhanced error detection, added casts
	to eliminate compiler warnings.
    July 2003: slight mods to back out aggressive FFFE detection.
    Jan 2004: updated switches in from-UTF8 conversions.
    Oct 2004: updated to use UNI_MAX_LEGAL_UTF32 in UTF-32 conversions.

    See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */

#include "PS2Etypes.h"

#include <string>
#include "ConvertUTF.h"


using std::string;
using std::wstring;


#ifdef CVTUTF_DEBUG
#include <stdio.h>
#endif

namespace Unicode
{
	/* Some fundamental constants */
	#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
	#define UNI_MAX_BMP (UTF32)0x0000FFFF
	#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
	#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
	#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

	#define UNI_SUR_HIGH_START  (UTF32)0xD800
	#define UNI_SUR_HIGH_END    (UTF32)0xDBFF
	#define UNI_SUR_LOW_START   (UTF32)0xDC00
	#define UNI_SUR_LOW_END     (UTF32)0xDFFF

	static const int halfShift  = 10; /* used for shifting by 10 bits */

	static const UTF32 halfBase = 0x0010000UL;
	static const UTF32 halfMask = 0x3FFUL;



	/* --------------------------------------------------------------------- */

	/*
	 * Index into the table below with the first byte of a UTF-8 sequence to
	 * get the number of trailing bytes that are supposed to follow it.
	 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
	 * left as-is for anyone who may want to do such conversion, which was
	 * allowed in earlier algorithms.
	 */
	static const char trailingBytesForUTF8[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};

	/*
	 * Magic values subtracted from a buffer value during UTF8 conversion.
	 * This table contains as many values as there might be trailing bytes
	 * in a UTF-8 sequence.
	 */
	static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
				 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

	/*
	 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
	 * into the first byte, depending on how many bytes follow.  There are
	 * as many entries in this table as there are UTF-8 sequence types.
	 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
	 * for *legal* UTF-8 will be 4 or fewer bytes total.
	 */
	static const UTF8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	/* --------------------------------------------------------------------- */

	/* The interface converts a whole buffer to avoid function-call overhead.
	 * Constants have been gathered. Loops & conditionals have been removed as
	 * much as possible for efficiency, in favor of drop-through switches.
	 * (See "Note A" at the bottom of the file for equivalent code.)
	 * If your compiler supports it, the "isLegalUTF8" call can be turned
	 * into an inline function.
	 */

	/* --------------------------------------------------------------------- */

	void Convert( const wstring& src, string& dest )
	{
		const UTF16* source = (UTF16*)src.c_str();
		const UTF16* sourceEnd = &source[ src.length() ];
		
		// initialize a four-char packet:
		std::wstring packet( L"    " );

		while (source < sourceEnd)
		{
			UTF32 ch;
			unsigned short bytesToWrite = 0;
			const UTF32 byteMask = 0xBF;
			const UTF32 byteMark = 0x80; 
			const UTF16* oldSource = source; /* In case we have to back up because of target overflow. */

			ch = *source++;

			/* If we have a surrogate pair, convert to UTF32 first. */
			if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
			{
				/* If the 16 bits following the high surrogate are in the source buffer... */
				if (source < sourceEnd)
				{
					UTF32 ch2 = *source;
					/* If it's a low surrogate, convert to UTF32. */
					if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
					{
						ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
							+ (ch2 - UNI_SUR_LOW_START) + halfBase;
						++source;
					}
					#ifdef UTF_STRICT
					else if (flags == strictConversion)
					{
						/* it's an unpaired high surrogate */
						--source; /* return to the illegal value itself */
						result = SourceIllegal;
						break;
					}
					#endif
				}
				else
				{
					/* We don't have the 16 bits following the high surrogate. */
					//--source; /* return to the high surrogate */
					//result = SourceExhausted;
					//break;
					throw Exception::UTFConversion<string>( dest, "UTF16->UTF8 conversion failure: Unexpected end of source string!" );
				}
			}
			#ifdef UTF_STRICT
			else if (flags == strictConversion)
			{
				/* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
				{
					--source; /* return to the illegal value itself */
					result = sourceIllegal;
					break;
				}
			}
			#endif
			
			/* Figure out how many bytes the result will require */
			if (ch < (UTF32)0x80) bytesToWrite = 1;
			else if (ch < (UTF32)0x800) bytesToWrite = 2;
			else if (ch < (UTF32)0x10000) bytesToWrite = 3;
			else if (ch < (UTF32)0x110000) bytesToWrite = 4;
			else
			{
				bytesToWrite = 3;
				ch = UNI_REPLACEMENT_CHAR;
			}

			packet.clear();
			switch( bytesToWrite )
			{
				/* note: everything falls through. */
				case 4: packet += (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
				case 3: packet += (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
				case 2: packet += (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
				case 1: packet += (UTF8) (ch | firstByteMark[bytesToWrite]);
			}
			dest.append( packet.rbegin(), packet.rend() );
		}
	}

	/* --------------------------------------------------------------------- */

	/*
	 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
	 * This must be called with the length pre-determined by the first byte.
	 * If not calling this from ConvertUTF8to*, then the length can be set by:
	 *  length = trailingBytesForUTF8[*source]+1;
	 * and the sequence is illegal right away if there aren't that many bytes
	 * available.
	 * If presented with a length > 4, this returns false.  The Unicode
	 * definition of UTF-8 goes up to 4-byte sequences.
	 */
	static bool isLegalUTF8(const UTF8 *source, int length)
	{
		UTF8 a;
		const UTF8 *srcptr = source+length;

		switch (length) 
		{
			default: return false;
			/* Everything else falls through when "true"... */
			case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
			case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
			case 2: if ((a = (*--srcptr)) > 0xBF) return false;

			switch (*source)
			{
				/* no fall-through in this inner switch */
				case 0xE0: if (a < 0xA0) return false; break;
				case 0xED: if (a > 0x9F) return false; break;
				case 0xF0: if (a < 0x90) return false; break;
				case 0xF4: if (a > 0x8F) return false; break;
				default:   if (a < 0x80) return false;
			}

			case 1: if (*source >= 0x80 && *source < 0xC2) return false;
		}
		if (*source > 0xF4) return false;
		return true;
	}

	/* --------------------------------------------------------------------- */

	/*
	 * Exported function to return whether a UTF-8 sequence is legal or not.
	 * This is not used here; it's just exported.
	 */
	bool isLegalUTF8Sequence( const UTF8 *source, const UTF8 *sourceEnd )
	{
		int length = trailingBytesForUTF8[*source]+1;
		if (source+length > sourceEnd)
			return false;
		return isLegalUTF8(source, length);
	}

	/* --------------------------------------------------------------------- */

	void Convert( const std::string& src, std::wstring& dest )
	{
		const UTF8* source = (UTF8*)src.c_str();
		const UTF8* sourceEnd = &source[ src.length() ];
	    
		while (source < sourceEnd)
		{
			UTF32 ch = 0;
			u16 extraBytesToRead = trailingBytesForUTF8[*source];
		
			if( source + extraBytesToRead >= sourceEnd )
			{
				throw Exception::UTFConversion<wstring>( dest, "UTF8->UTF16 String Conversion failed: Unexpected end of string data." );
			}

			/* Do this check whether lenient or strict */
			if (!isLegalUTF8(source, extraBytesToRead+1))
			{
				throw Exception::UTFConversion<wstring>( dest, "UTF8->UTF16 String Conversion failed: Illegal UTF8 data encountered!" );
			}

			/*
			 * The cases all fall through. See "Note A" below.
			 */
			switch (extraBytesToRead)
			{
				case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
				case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
				case 3: ch += *source++; ch <<= 6;
				case 2: ch += *source++; ch <<= 6;
				case 1: ch += *source++; ch <<= 6;
				case 0: ch += *source++;
			}
			ch -= offsetsFromUTF8[extraBytesToRead];

			if (ch <= UNI_MAX_BMP)
			{
				/* Target is a character <= 0xFFFF */
				/* UTF-16 surrogate values are illegal in UTF-32 */
				if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
				{
					#ifdef UTF_STRICT
					if (flags == strictConversion)
					{
						source -= (extraBytesToRead+1); /* return to the illegal value itself */
						result = sourceIllegal;
						break;
					}
					else
					#endif
					{
						dest += UNI_REPLACEMENT_CHAR;
					}
				}
				else 
				{
					dest += (UTF16)ch; /* normal case */
				}
			} else if (ch > UNI_MAX_UTF16)
			{
				#ifdef UTF_STRICT
				if (flags == strictConversion)
				{
					result = sourceIllegal;
					source -= (extraBytesToRead+1); /* return to the start */
					break; /* Bail out; shouldn't continue */
				}
				else
				#endif
				{
					dest += UNI_REPLACEMENT_CHAR;
				}
			} else
			{
				ch -= halfBase;
				dest += (UTF16)((ch >> halfShift) + UNI_SUR_HIGH_START);
				dest += (UTF16)((ch & halfMask) + UNI_SUR_LOW_START);
			}
		}
	}


	/* ---------------------------------------------------------------------

		Note A.
		The fall-through switches in UTF-8 reading code save a
		temp variable, some decrements & conditionals.  The switches
		are equivalent to the following loop:
		{
			int tmpBytesToRead = extraBytesToRead+1;
			do {
			ch += *source++;
			--tmpBytesToRead;
			if (tmpBytesToRead) ch <<= 6;
			} while (tmpBytesToRead > 0);
		}
		In UTF-8 writing code, the switches on "bytesToWrite" are
		similarly unrolled loops.

	   --------------------------------------------------------------------- */
	   
	wstring Convert( const string& src )
	{
		wstring dest;
		Convert( src, dest );
		return dest;
	}

	string Convert( const wstring& src )
	{
		string dest;
		Convert( src, dest );
		return dest;
	}
}