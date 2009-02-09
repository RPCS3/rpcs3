/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
//============================================
//=== Audio XA decoding
//=== Kazzuya
//============================================
//=== Modified by linuzappz
//============================================

#include <stdio.h>

#include "Decode_XA.h"

#ifdef __WIN32__
#pragma warning(disable:4244)
#endif

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

#define NOT(_X_)				(!(_X_))
#define CLAMP(_X_,_MI_,_MA_)	{if(_X_<_MI_)_X_=_MI_;if(_X_>_MA_)_X_=_MA_;}

//============================================
//===  ADPCM DECODING ROUTINES
//============================================

static double K0[4] = {
    0.0,
    0.9375,
    1.796875,
    1.53125
};

static double K1[4] = {
    0.0,
    0.0,
    -0.8125,
    -0.859375
};

#define BLKSIZ 28       /* block size (32 - 4 nibbles) */

//===========================================
void ADPCM_InitDecode( ADPCM_Decode_t *decp )
{
	decp->y0 = 0;
	decp->y1 = 0;
}

//===========================================
#define SH	4
#define SHC	10

#define IK0(fid)	((int)((-K0[fid]) * (1<<SHC)))
#define IK1(fid)	((int)((-K1[fid]) * (1<<SHC)))

void ADPCM_DecodeBlock16( ADPCM_Decode_t *decp, U8 filter_range, const void *vblockp, short *destp, int inc ) {
	int i;
	int range, filterid;
	long fy0, fy1;
	const U16 *blockp;

	blockp = (const unsigned short *)vblockp;
	filterid = (filter_range >>  4) & 0x0f;
	range    = (filter_range >>  0) & 0x0f;

	fy0 = decp->y0;
	fy1 = decp->y1;

	for (i = BLKSIZ/4; i; --i) {
		long y;
		long x0, x1, x2, x3;

		y = *blockp++;
		x3 = (short)( y        & 0xf000) >> range; x3 <<= SH;
		x2 = (short)((y <<  4) & 0xf000) >> range; x2 <<= SH;
		x1 = (short)((y <<  8) & 0xf000) >> range; x1 <<= SH;
		x0 = (short)((y << 12) & 0xf000) >> range; x0 <<= SH;

		x0 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC; fy1 = fy0; fy0 = x0;
		x1 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC; fy1 = fy0; fy0 = x1;
		x2 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC; fy1 = fy0; fy0 = x2;
		x3 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC; fy1 = fy0; fy0 = x3;

		CLAMP( x0, -32768<<SH, 32767<<SH ); *destp = x0 >> SH; destp += inc;
		CLAMP( x1, -32768<<SH, 32767<<SH ); *destp = x1 >> SH; destp += inc;
		CLAMP( x2, -32768<<SH, 32767<<SH ); *destp = x2 >> SH; destp += inc;
		CLAMP( x3, -32768<<SH, 32767<<SH ); *destp = x3 >> SH; destp += inc;
	}
	decp->y0 = fy0;
	decp->y1 = fy1;
}

static int headtable[4] = {0,2,8,10};

//===========================================
static void xa_decode_data( xa_decode_t *xdp, unsigned char *srcp ) {
	const U8    *sound_groupsp;
	const U8    *sound_datap, *sound_datap2;
	int         i, j, k, nbits;
	U16			data[4096], *datap;
	short		*destp;

	destp = xdp->pcm;
	nbits = xdp->nbits == 4 ? 4 : 2;

	if (xdp->stereo) { // stereo
		for (j=0; j < 18; j++) {
			sound_groupsp = srcp + j * 128;		// sound groups header
			sound_datap = sound_groupsp + 16;	// sound data just after the header

			for (i=0; i < nbits; i++) {
    			datap = data;
    			sound_datap2 = sound_datap + i;
				if ((xdp->nbits == 8) && (xdp->freq == 37800)) { // level A
        			for (k=0; k < 14; k++, sound_datap2 += 8) {
           					*(datap++) = (U16)sound_datap2[0] |
                       				     (U16)(sound_datap2[4] << 8);
					}
				} else { // level B/C
        			for (k=0; k < 7; k++, sound_datap2 += 16) {
           					*(datap++) = (U16)(sound_datap2[ 0] & 0x0f) |
                       				    ((U16)(sound_datap2[ 4] & 0x0f) <<  4) |
                       				    ((U16)(sound_datap2[ 8] & 0x0f) <<  8) |
                       				    ((U16)(sound_datap2[12] & 0x0f) << 12);
					}
        		}
    			ADPCM_DecodeBlock16( &xdp->left,  sound_groupsp[headtable[i]+0], data,
                   				    destp+0, 2 );

        		datap = data;
        		sound_datap2 = sound_datap + i;
				if ((xdp->nbits == 8) && (xdp->freq == 37800)) { // level A
        			for (k=0; k < 14; k++, sound_datap2 += 8) {
           					*(datap++) = (U16)sound_datap2[0] |
                       				     (U16)(sound_datap2[4] << 8);
					}
				} else { // level B/C
        			for (k=0; k < 7; k++, sound_datap2 += 16) {
           					*(datap++) = (U16)(sound_datap2[ 0] >> 4) |
                       	    			((U16)(sound_datap2[ 4] >> 4) <<  4) |
                       				    ((U16)(sound_datap2[ 8] >> 4) <<  8) |
                       				    ((U16)(sound_datap2[12] >> 4) << 12);
        			}
				}
				ADPCM_DecodeBlock16( &xdp->right,  sound_groupsp[headtable[i]+1], data,
                           			    destp+1, 2 );

        		destp += 28*2;
			}
    	}
	}
	else { // mono
		for (j=0; j < 18; j++) {
    		sound_groupsp = srcp + j * 128;		// sound groups header
    		sound_datap = sound_groupsp + 16;	// sound data just after the header

    		for (i=0; i < nbits; i++) {
        		datap = data;
        		sound_datap2 = sound_datap + i;
				if ((xdp->nbits == 8) && (xdp->freq == 37800)) { // level A
        			for (k=0; k < 14; k++, sound_datap2 += 8) {
           					*(datap++) = (U16)sound_datap2[0] |
                       				     (U16)(sound_datap2[4] << 8);
					}
				} else { // level B/C
        			for (k=0; k < 7; k++, sound_datap2 += 16) {
           					*(datap++) = (U16)(sound_datap2[ 0] & 0x0f) |
                       				    ((U16)(sound_datap2[ 4] & 0x0f) <<  4) |
                       				    ((U16)(sound_datap2[ 8] & 0x0f) <<  8) |
                       				    ((U16)(sound_datap2[12] & 0x0f) << 12);
					}
        		}
        		ADPCM_DecodeBlock16( &xdp->left,  sound_groupsp[headtable[i]+0], data,
                           			    destp, 1 );

        		destp += 28;

        		datap = data;
        		sound_datap2 = sound_datap + i;
				if ((xdp->nbits == 8) && (xdp->freq == 37800)) { // level A
        			for (k=0; k < 14; k++, sound_datap2 += 8) {
           					*(datap++) = (U16)sound_datap2[0] |
                       				     (U16)(sound_datap2[4] << 8);
					}
				} else { // level B/C
        			for (k=0; k < 7; k++, sound_datap2 += 16) {
            				*(datap++) = (U16)(sound_datap2[ 0] >> 4) |
                       	    		    ((U16)(sound_datap2[ 4] >> 4) <<  4) |
                        				((U16)(sound_datap2[ 8] >> 4) <<  8) |
                        				((U16)(sound_datap2[12] >> 4) << 12);
        				}
				}
       			ADPCM_DecodeBlock16( &xdp->left,  sound_groupsp[headtable[i]+1], data,
                           			    destp, 1 );

				destp += 28;
			}
    	}
	}
}

//============================================
//===  XA SPECIFIC ROUTINES
//============================================
typedef struct {
U8  filenum;
U8  channum;
U8  submode;
U8  coding;

U8  filenum2;
U8  channum2;
U8  submode2;
U8  coding2;
} xa_subheader_t;

#define SUB_SUB_EOF     (1<<7)  // end of file
#define SUB_SUB_RT      (1<<6)  // real-time sector
#define SUB_SUB_FORM    (1<<5)  // 0 form1  1 form2
#define SUB_SUB_TRIGGER (1<<4)  // used for interrupt
#define SUB_SUB_DATA    (1<<3)  // contains data
#define SUB_SUB_AUDIO   (1<<2)  // contains audio
#define SUB_SUB_VIDEO   (1<<1)  // contains video
#define SUB_SUB_EOR     (1<<0)  // end of record

#define AUDIO_CODING_GET_STEREO(_X_)    ( (_X_) & 3)
#define AUDIO_CODING_GET_FREQ(_X_)      (((_X_) >> 2) & 3)
#define AUDIO_CODING_GET_BPS(_X_)       (((_X_) >> 4) & 3)
#define AUDIO_CODING_GET_EMPHASIS(_X_)  (((_X_) >> 6) & 1)

#define SUB_UNKNOWN 0
#define SUB_VIDEO   1
#define SUB_AUDIO   2

//============================================
static int parse_xa_audio_sector( xa_decode_t *xdp, 
								  xa_subheader_t *subheadp,
								  unsigned char *sectorp,
								  int is_first_sector ) {
    if ( is_first_sector ) {
		switch ( AUDIO_CODING_GET_FREQ(subheadp->coding) ) {
			case 0: xdp->freq = 37800;   break;
			case 1: xdp->freq = 18900;   break;
			default: xdp->freq = 0;      break;
		}
		switch ( AUDIO_CODING_GET_BPS(subheadp->coding) ) {
			case 0: xdp->nbits = 4; break;
			case 1: xdp->nbits = 8; break;
			default: xdp->nbits = 0; break;
		}
		switch ( AUDIO_CODING_GET_STEREO(subheadp->coding) ) {
			case 0: xdp->stereo = 0; break;
			case 1: xdp->stereo = 1; break;
			default: xdp->stereo = 0; break;
		}

		if ( xdp->freq == 0 )
			return -1;

		ADPCM_InitDecode( &xdp->left );
		ADPCM_InitDecode( &xdp->right );

		xdp->nsamples = 18 * 28 * 8;
		if (xdp->stereo == 1) xdp->nsamples /= 2;
    }
	xa_decode_data( xdp, sectorp );

	return 0;
}

//================================================================
//=== THIS IS WHAT YOU HAVE TO CALL
//=== xdp              - structure were all important data are returned
//=== sectorp          - data in input
//=== pcmp             - data in output
//=== is_first_sector  - 1 if it's the 1st sector of the stream
//===                  - 0 for any other successive sector
//=== return -1 if error
//================================================================
long xa_decode_sector( xa_decode_t *xdp,
					   unsigned char *sectorp, int is_first_sector ) {
	if (parse_xa_audio_sector(xdp, (xa_subheader_t *)sectorp, sectorp + sizeof(xa_subheader_t), is_first_sector))
		return -1;

	return 0;
}

/* EXAMPLE:
"nsamples" is the number of 16 bit samples
every sample is 2 bytes in mono and 4 bytes in stereo

xa_decode_t	xa;

	sectorp = read_first_sector();
	xa_decode_sector( &xa, sectorp, 1 );
	play_wave( xa.pcm, xa.freq, xa.nsamples );

	while ( --n_sectors )
	{
		sectorp = read_next_sector();
		xa_decode_sector( &xa, sectorp, 0 );
		play_wave( xa.pcm, xa.freq, xa.nsamples );
	}
*/
