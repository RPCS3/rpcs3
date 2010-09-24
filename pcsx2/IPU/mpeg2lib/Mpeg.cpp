/*
 * Mpeg.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * Modified by Florin for PCSX2 emu
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// [Air] Note: many functions in this module are large and only used once, so they
//	have been forced to inline since it won't bloat the program and gets rid of
//	some call overhead.

#include "PrecompiledHeader.h"

#include "Common.h"
#include "IPU/IPU.h"
#include "Mpeg.h"
#include "Vlc.h"

const int non_linear_quantizer_scale [] =
{
	0,  1,  2,  3,  4,  5,	6,	7,
	8, 10, 12, 14, 16, 18,  20,  22,
	24, 28, 32, 36, 40, 44,  48,  52,
	56, 64, 72, 80, 88, 96, 104, 112
};

/* Bitstream and buffer needs to be reallocated in order for successful
	reading of the old data. Here the old data stored in the 2nd slot
	of the internal buffer is copied to 1st slot, and the new data read
	into 1st slot is copied to the 2nd slot. Which will later be copied
	back to the 1st slot when 128bits have been read.
*/
const DCTtab * tab;
int mbaCount = 0;

int bitstream_init ()
{
	return g_BP.FillBuffer(32);
}

int get_macroblock_modes()
{
	int macroblock_modes;
	const MBtab * tab;

	switch (decoder.coding_type)
	{
		case I_TYPE:
			macroblock_modes = UBITS(2);

			if (macroblock_modes == 0) return 0;   // error

			tab = MB_I + (macroblock_modes >> 1);
			DUMPBITS(tab->len);
			macroblock_modes = tab->modes;

			if ((!(decoder.frame_pred_frame_dct)) &&
			        (decoder.picture_structure == FRAME_PICTURE))
			{
				macroblock_modes |= GETBITS(1) * DCT_TYPE_INTERLACED;
			}
			return macroblock_modes;

		case P_TYPE:
			macroblock_modes = UBITS(6);

			if (macroblock_modes == 0) return 0;   // error

			tab = MB_P + (macroblock_modes >> 1);
			DUMPBITS(tab->len);
			macroblock_modes = tab->modes;

			if (decoder.picture_structure != FRAME_PICTURE)
			{
				if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
				{
					macroblock_modes |= GETBITS(2) * MOTION_TYPE_BASE;
				}

				return macroblock_modes;
			}
			else if (decoder.frame_pred_frame_dct)
			{
				if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
					macroblock_modes |= MC_FRAME;

				return macroblock_modes;
			}
			else
			{
				if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
				{
					macroblock_modes |= GETBITS(2) * MOTION_TYPE_BASE;
				}

				if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN))
				{
					macroblock_modes |= GETBITS(1) * DCT_TYPE_INTERLACED;
				}

				return macroblock_modes;
			}

		case B_TYPE:
			macroblock_modes = UBITS(6);

			if (macroblock_modes == 0) return 0;   // error

			tab = MB_B + macroblock_modes;
			DUMPBITS(tab->len);
			macroblock_modes = tab->modes;

			if (decoder.picture_structure != FRAME_PICTURE)
			{
				if (!(macroblock_modes & MACROBLOCK_INTRA))
				{
					macroblock_modes |= GETBITS(2) * MOTION_TYPE_BASE;
				}

				return macroblock_modes;
			}
			else if (decoder.frame_pred_frame_dct)
			{
				/* if (! (macroblock_modes & MACROBLOCK_INTRA)) */
				macroblock_modes |= MC_FRAME;
				return macroblock_modes;
			}
			else
			{
				if (macroblock_modes & MACROBLOCK_INTRA) goto intra;

				macroblock_modes |= GETBITS(2) * MOTION_TYPE_BASE;

				if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN))
				{
intra:
					macroblock_modes |= GETBITS(1) * DCT_TYPE_INTERLACED;
				}

				return macroblock_modes;
			}

		case D_TYPE:
			macroblock_modes = GETBITS(1);

			if (macroblock_modes == 0) return 0;   // error
			return MACROBLOCK_INTRA;

		default:
			return 0;
	}
}

static __fi int get_quantizer_scale()
{
	int quantizer_scale_code;

	quantizer_scale_code = GETBITS(5);

	if (decoder.q_scale_type)
		return non_linear_quantizer_scale [quantizer_scale_code];
	else
		return quantizer_scale_code << 1;
}

static __fi int get_coded_block_pattern()
{
	const CBPtab * tab;
	u16 code = UBITS(16);

	if (code >= 0x2000)
		tab = CBP_7 + (UBITS(7) - 16);
	else
		tab = CBP_9 + UBITS(9);

	DUMPBITS(tab->len);
	return tab->cbp;
}

int __fi get_motion_delta(const int f_code)
{
	int delta;
	int sign;
	const MVtab * tab;
	u16 code = UBITS(16);

	if ((code & 0x8000))
	{
		DUMPBITS(1);
		return 0x00010000;
	}
	else if ((code & 0xf000) || ((code & 0xfc00) == 0x0c00))
	{
		tab = MV_4 + UBITS(4);
	}
	else
	{
		tab = MV_10 + UBITS(10);
	}

	delta = tab->delta + 1;
	DUMPBITS(tab->len);

	sign = SBITS(1);
	DUMPBITS(1);
	return (delta ^ sign) - sign;
}

int __fi get_dmv()
{
	const DMVtab* tab = DMV_2 + UBITS(2);
	DUMPBITS(tab->len);
	return tab->dmv;
}

int get_macroblock_address_increment()
{
	const MBAtab *mba;
	
	u16 code = UBITS(16);

	if (code >= 4096)
		mba = MBA.mba5 + (UBITS(5) - 2);
	else if (code >= 768)
		mba = MBA.mba11 + (UBITS(11) - 24);
	else switch (UBITS(11))
	{
		case 8:		/* macroblock_escape */
			DUMPBITS(11);
			return 0x23;

		case 15:	/* macroblock_stuffing (MPEG1 only) */
			if (decoder.mpeg1)
			{
				DUMPBITS(11);
				return 0x22;
			}

		default:
			return 0;//error
	}

	DUMPBITS(mba->len);

	return mba->mba + 1;
}

static __fi int get_luma_dc_dct_diff()
{
	int size;
	int dc_diff;
	u16 code = UBITS(5);

	if (code < 31)
	{
		size = DCtable.lum0[code].size;
		DUMPBITS(DCtable.lum0[code].len);

		// 5 bits max
	}
	else
	{
		code = UBITS(9) - 0x1f0;
		size = DCtable.lum1[code].size;
		DUMPBITS(DCtable.lum1[code].len);

		// 9 bits max
	}
	
	if (size==0)
		dc_diff = 0;
	else
	{
		dc_diff = GETBITS(size);

		// 6 for tab0 and 11 for tab1
		if ((dc_diff & (1<<(size-1)))==0)
		  dc_diff-= (1<<size) - 1;
	}

	return dc_diff;
}

static __fi int get_chroma_dc_dct_diff()
{
	int size;
	int dc_diff;
	u16 code = UBITS(5);

    if (code<31)
	{
		size = DCtable.chrom0[code].size;
		DUMPBITS(DCtable.chrom0[code].len);
	}
	else
	{
	    code = UBITS(10) - 0x3e0;
	    size = DCtable.chrom1[code].size;
		DUMPBITS(DCtable.chrom1[code].len);
	}
	
	if (size==0)
	    dc_diff = 0;
	else
	{
		dc_diff = GETBITS(size);

		if ((dc_diff & (1<<(size-1)))==0)
		{
			dc_diff-= (1<<size) - 1;
		}
	}
  
	return dc_diff;
}

#define SATURATE(val)					\
do {							\
	 if (((u32)(val + 2048) > 4095))	\
	val = (((s32)val) >> 31) ^ 2047;			\
} while (0)

static bool get_intra_block()
{
	const u8 * scan = decoder.scantype ? mpeg2_scan.alt : mpeg2_scan.norm;
	const u8 (&quant_matrix)[64] = decoder.iq;
	int quantizer_scale = decoder.quantizer_scale;
	s16 * dest = decoder.DCTblock;
	u16 code; 

	/* decode AC coefficients */
  for (int i=1 + ipu_cmd.pos[4]; ; i++)
  {
	  switch (ipu_cmd.pos[5])
	  {
	  case 0:
		if (!GETWORD())
		{
		  ipu_cmd.pos[4] = i - 1;
		  return false;
		}

		code = UBITS(16);

		if (code >= 16384 && (!decoder.intra_vlc_format || decoder.mpeg1))
		{
		  tab = &DCT.next[(code >> 12) - 4];
		}
		else if (code >= 1024)
		{
			if (decoder.intra_vlc_format && !decoder.mpeg1)
			{
				tab = &DCT.tab0a[(code >> 8) - 4];
			}
			else
			{
				tab = &DCT.tab0[(code >> 8) - 4];
			}
		}
		else if (code >= 512)
		{
			if (decoder.intra_vlc_format && !decoder.mpeg1)
			{
				tab = &DCT.tab1a[(code >> 6) - 8];
			}
			else
			{
				tab = &DCT.tab1[(code >> 6) - 8];
			}
		}

		// [TODO] Optimization: Following codes can all be done by a single "expedited" lookup
		// that should use a single unrolled DCT table instead of five separate tables used
		// here.  Multiple conditional statements are very slow, while modern CPU data caches
		// have lots of room to spare.

		else if (code >= 256)
		{
			tab = &DCT.tab2[(code >> 4) - 16];
		}
		else if (code >= 128)
		{    
			tab = &DCT.tab3[(code >> 3) - 16];
		}
		else if (code >= 64)
		{    
			tab = &DCT.tab4[(code >> 2) - 16];
		}
		else if (code >= 32)
		{    
			tab = &DCT.tab5[(code >> 1) - 16];
		}
		else if (code >= 16)
		{    
			tab = &DCT.tab6[code - 16];
		}
		else
		{
		  ipu_cmd.pos[4] = 0;
		  return true;
		}

		DUMPBITS(tab->len);

		if (tab->run==64) /* end_of_block */
		{
			ipu_cmd.pos[4] = 0;
			return true;
		}
		
		i += (tab->run == 65) ? GETBITS(6) : tab->run;
		if (i >= 64)
		{
			ipu_cmd.pos[4] = 0;
			return true;
		}

	  case 1:
	  {
			if (!GETWORD())
			{
				ipu_cmd.pos[4] = i - 1;
				ipu_cmd.pos[5] = 1;
				return false;
			}

			uint j = scan[i];
			int val;

			if (tab->run==65) /* escape */
			{
				if(!decoder.mpeg1)
				{
				  val = (SBITS(12) * quantizer_scale * quant_matrix[i]) >> 4;
				  DUMPBITS(12);
				}
				else
				{
				  val = SBITS(8);
				  DUMPBITS(8);

				  if (!(val & 0x7f))
				  {
					val = GETBITS(8) + 2 * val;
				  }

				  val = (val * quantizer_scale * quant_matrix[i]) >> 4;
				  val = (val + ~ (((s32)val) >> 31)) | 1;
				}
			}
			else
			{
				val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;
				if(decoder.mpeg1)
				{
					/* oddification */
					val = (val - 1) | 1;
				}

				/* if (bitstream_get (1)) val = -val; */
				int bit1 = SBITS(1);
				val = (val ^ bit1) - bit1;
				DUMPBITS(1);
			}

			SATURATE(val);
			dest[j] = val;
			ipu_cmd.pos[5] = 0;
		}
	 }
  }

  ipu_cmd.pos[4] = 0;
  return true;
}

static bool get_non_intra_block(int * last)
{
	int i;
	int j;
	int val;
	const u8 * scan = decoder.scantype ? mpeg2_scan.alt : mpeg2_scan.norm;
	const u8 (&quant_matrix)[64] = decoder.niq;
	int quantizer_scale = decoder.quantizer_scale;
	s16 * dest = decoder.DCTblock;
	u16 code;

    /* decode AC coefficients */
    for (i= ipu_cmd.pos[4] ; ; i++)
    {
		switch (ipu_cmd.pos[5])
		{
		case 0:
			if (!GETWORD())
			{
				ipu_cmd.pos[4] = i;
				return false;
			}

			code = UBITS(16);

			if (code >= 16384)
			{
				if (i==0)
				{
					tab = &DCT.first[(code >> 12) - 4];
				}
				else
				{			
					tab = &DCT.next[(code >> 12)- 4];
				}
			}
			else if (code >= 1024)
			{
				tab = &DCT.tab0[(code >> 8) - 4];
			}
			else if (code >= 512)
			{		
				tab = &DCT.tab1[(code >> 6) - 8];
			}

			// [TODO] Optimization: Following codes can all be done by a single "expedited" lookup
			// that should use a single unrolled DCT table instead of five separate tables used
			// here.  Multiple conditional statements are very slow, while modern CPU data caches
			// have lots of room to spare.

			else if (code >= 256)
			{		
				tab = &DCT.tab2[(code >> 4) - 16];
			}
			else if (code >= 128)
			{		
				tab = &DCT.tab3[(code >> 3) - 16];
			}
			else if (code >= 64)
			{		
				tab = &DCT.tab4[(code >> 2) - 16];
			}
			else if (code >= 32)
			{		
				tab = &DCT.tab5[(code >> 1) - 16];
			}
			else if (code >= 16)
			{		
				tab = &DCT.tab6[code - 16];
			}
			else
			{
				ipu_cmd.pos[4] = 0;
				return true;
			}

			DUMPBITS(tab->len);

			if (tab->run==64) /* end_of_block */
			{
				*last = i;
				ipu_cmd.pos[4] = 0;
				return true;
			}

			i += (tab->run == 65) ? GETBITS(6) : tab->run;
			if (i >= 64)
			{
				*last = i;
				ipu_cmd.pos[4] = 0;
				return true;
			}

		case 1:
			if (!GETWORD())
			{
			  ipu_cmd.pos[4] = i;
			  ipu_cmd.pos[5] = 1;
			  return false;
			}

			j = scan[i];

			if (tab->run==65) /* escape */
			{
				if (!decoder.mpeg1)
				{
					val = ((2 * (SBITS(12) + SBITS(1)) + 1) * quantizer_scale * quant_matrix[i]) >> 5;
					DUMPBITS(12);
				}
				else
				{
				  val = SBITS(8);
				  DUMPBITS(8);

				  if (!(val & 0x7f))
				  {
					val = GETBITS(8) + 2 * val;
				  }

				  val = ((2 * (val + (((s32)val) >> 31)) + 1) * quantizer_scale * quant_matrix[i]) / 32;
				  val = (val + ~ (((s32)val) >> 31)) | 1;
				}
			}
			else
			{
				int bit1 = SBITS(1);
				val = ((2 * tab->level + 1) * quantizer_scale * quant_matrix[i]) >> 5;
				val = (val ^ bit1) - bit1;
				DUMPBITS(1);
			}

			SATURATE(val);
			dest[j] = val;
			ipu_cmd.pos[5] = 0;
		}
	}

	ipu_cmd.pos[4] = 0;
	return true;
}

static __fi bool slice_intra_DCT(const int cc, u8 * const dest, const int stride, const bool skip)
{
	if (!skip || ipu_cmd.pos[3])
	{
		ipu_cmd.pos[3] = 0;
		if (!GETWORD())
		{
			ipu_cmd.pos[3] = 1;
			return false;
		}

		/* Get the intra DC coefficient and inverse quantize it */
		if (cc == 0)
			decoder.dc_dct_pred[0] += get_luma_dc_dct_diff();
		else
			decoder.dc_dct_pred[cc] += get_chroma_dc_dct_diff();

		decoder.DCTblock[0] = decoder.dc_dct_pred[cc] << (3 - decoder.intra_dc_precision);
	}

	if (!get_intra_block())
	{
		return false;
	}

	mpeg2_idct_copy(decoder.DCTblock, dest, stride);

	return true;
}

static __fi bool slice_non_intra_DCT(s16 * const dest, const int stride, const bool skip)
{
	int last;

	if (!skip)
	{
		memzero_sse_a(decoder.DCTblock);
	}

	if (!get_non_intra_block(&last))
	{
		return false;
	}

	mpeg2_idct_add(last, decoder.DCTblock, dest, stride);

	return true;
}

void __fi finishmpeg2sliceIDEC()
{
	ipuRegs.ctrl.SCD = 0;
	coded_block_pattern = decoder.coded_block_pattern;
}

__fi bool mpeg2sliceIDEC()
{
	u16 code;

	switch (ipu_cmd.pos[0])
	{
	case 0:
		decoder.dc_dct_pred[0] =
		decoder.dc_dct_pred[1] =
		decoder.dc_dct_pred[2] = 128 << decoder.intra_dc_precision;

		decoder.mbc = 0;
		ipuRegs.top = 0;
		ipuRegs.ctrl.ECD = 0;

	case 1:
		ipu_cmd.pos[0] = 1;
		if (!bitstream_init())
		{
			return false;
		}

	case 2:
		ipu_cmd.pos[0] = 2;
		while (1)
		{
			macroblock_8& mb8 = decoder.mb8;
			macroblock_rgb16& rgb16 = decoder.rgb16;
			macroblock_rgb32& rgb32 = decoder.rgb32;

			int DCT_offset, DCT_stride;
			const MBAtab * mba;

			switch (ipu_cmd.pos[1])
			{
			case 0:
				decoder.macroblock_modes = get_macroblock_modes();

				if (decoder.macroblock_modes & MACROBLOCK_QUANT) //only IDEC
				{
					decoder.quantizer_scale = get_quantizer_scale();
				}

				decoder.coded_block_pattern = 0x3F;//all 6 blocks
				memzero_sse_a(mb8);
				memzero_sse_a(rgb32);

			case 1:
				ipu_cmd.pos[1] = 1;

				if (decoder.macroblock_modes & DCT_TYPE_INTERLACED)
				{
					DCT_offset = decoder_stride;
					DCT_stride = decoder_stride * 2;
				}
				else
				{
					DCT_offset = decoder_stride * 8;
					DCT_stride = decoder_stride;
				}

				switch (ipu_cmd.pos[2])
				{
				case 0:
				case 1:
					if (!slice_intra_DCT(0, (u8*)mb8.Y, DCT_stride, ipu_cmd.pos[2] == 1))
					{
						ipu_cmd.pos[2] = 1;
						return false;
					}
				case 2:
					if (!slice_intra_DCT(0, (u8*)mb8.Y + 8, DCT_stride, ipu_cmd.pos[2] == 2))
					{
						ipu_cmd.pos[2] = 2;
						return false;
					}
				case 3:
					if (!slice_intra_DCT(0, (u8*)mb8.Y + DCT_offset, DCT_stride, ipu_cmd.pos[2] == 3))
					{
						ipu_cmd.pos[2] = 3;
						return false;
					}
				case 4:
					if (!slice_intra_DCT(0, (u8*)mb8.Y + DCT_offset + 8, DCT_stride, ipu_cmd.pos[2] == 4))
					{
						ipu_cmd.pos[2] = 4;
						return false;
					}
				case 5:
					if (!slice_intra_DCT(1, (u8*)mb8.Cb, decoder_stride >> 1, ipu_cmd.pos[2] == 5))
					{
						ipu_cmd.pos[2] = 5;
						return false;
					}
				case 6:
					if (!slice_intra_DCT(2, (u8*)mb8.Cr, decoder_stride >> 1, ipu_cmd.pos[2] == 6))
					{
						ipu_cmd.pos[2] = 6;
						return false;
					}
					break;

				jNO_DEFAULT;
				}

				// Send The MacroBlock via DmaIpuFrom
				ipu_csc(mb8, rgb32, decoder.sgn);

				if (decoder.ofm == 0)
					decoder.SetOutputTo(rgb32);
				else
				{
					ipu_dither(rgb32, rgb16, decoder.dte);
					decoder.SetOutputTo(rgb16);
				}

			case 2:
			{
				pxAssume(decoder.ipu0_data > 0);

				uint read = ipu_fifo.out.write((u32*)decoder.GetIpuDataPtr(), decoder.ipu0_data);
				decoder.AdvanceIpuDataBy(read);

				if (decoder.ipu0_data != 0)
				{
					// IPU FIFO filled up -- Will have to finish transferring later.
					ipu_cmd.pos[1] = 2;
					return false;
				}

				decoder.mbc++;
				mbaCount = 0;
			}
			
			case 3:
				while (1)
				{
					if (!GETWORD())
					{
						ipu_cmd.pos[1] = 3;
						return false;
					}

					code = UBITS(16);
					if (code >= 0x1000)
					{
						mba = MBA.mba5 + (UBITS(5) - 2);
						break;
					}
					else if (code >= 0x0300)
					{
						mba = MBA.mba11 + (UBITS(11) - 24);
						break;
					}
					else switch (UBITS(11))
					{
						case 8:		/* macroblock_escape */
							mbaCount += 33;
							/* pass through */

						case 15:	/* macroblock_stuffing (MPEG1 only) */
							DUMPBITS(11);
							continue;

						default:	/* end of slice/frame, or error? */
						{
							goto finish_idec;	
						}
					}
				}

				DUMPBITS(mba->len);
				mbaCount += mba->mba;

				if (mbaCount)
				{
					decoder.dc_dct_pred[0] =
					decoder.dc_dct_pred[1] =
					decoder.dc_dct_pred[2] = 128 << decoder.intra_dc_precision;

					decoder.mbc += mbaCount;
				}

			case 4:
				if (!GETWORD())
				{
					ipu_cmd.pos[1] = 4;
					return false;
				}

				break;

			jNO_DEFAULT;
			}

			ipu_cmd.pos[1] = 0;
			ipu_cmd.pos[2] = 0;
		}

finish_idec:
		finishmpeg2sliceIDEC();

	case 3:
	{
		u8 bit8;
		if (!getBits8((u8*)&bit8, 0))
		{
			ipu_cmd.pos[0] = 3;
			return false;
		}

		if (bit8 == 0)
		{
			g_BP.Align();
			ipuRegs.ctrl.SCD = 1;
		}
	}

	case 4:
		if (!getBits32((u8*)&ipuRegs.top, 0))
		{
			ipu_cmd.pos[0] = 4;
			return false;
		}

		ipuRegs.top = BigEndian(ipuRegs.top);
		break;

	jNO_DEFAULT;
	}

	return true;
}

__fi bool mpeg2_slice()
{
	int DCT_offset, DCT_stride;

	macroblock_8& mb8 = decoder.mb8;
	macroblock_16& mb16 = decoder.mb16;

	switch (ipu_cmd.pos[0])
	{
	case 0:
		if (decoder.dcr)
		{
			decoder.dc_dct_pred[0] =
			decoder.dc_dct_pred[1] =
			decoder.dc_dct_pred[2] = 128 << decoder.intra_dc_precision;
		}
			
		ipuRegs.ctrl.ECD = 0;
		ipuRegs.top = 0;
		memzero_sse_a(mb8);
		memzero_sse_a(mb16);
	case 1:
		if (!bitstream_init())
		{
			ipu_cmd.pos[0] = 1;
			return false;
		}

	case 2:
		ipu_cmd.pos[0] = 2;

		if (decoder.macroblock_modes & DCT_TYPE_INTERLACED)
		{
			DCT_offset = decoder_stride;
			DCT_stride = decoder_stride * 2;
		}
		else
		{
			DCT_offset = decoder_stride * 8;
			DCT_stride = decoder_stride;
		}

		if (decoder.macroblock_modes & MACROBLOCK_INTRA)
		{
			switch(ipu_cmd.pos[1])
			{
			case 0:
				decoder.coded_block_pattern = 0x3F;
			case 1:
				if (!slice_intra_DCT(0, (u8*)mb8.Y, DCT_stride, ipu_cmd.pos[1] == 1))
				{
					ipu_cmd.pos[1] = 1;
					return false;
				}
			case 2:
				if (!slice_intra_DCT(0, (u8*)mb8.Y + 8, DCT_stride, ipu_cmd.pos[1] == 2))
				{
					ipu_cmd.pos[1] = 2;
					return false;
				}
			case 3:
				if (!slice_intra_DCT(0, (u8*)mb8.Y + DCT_offset, DCT_stride, ipu_cmd.pos[1] == 3))
				{
					ipu_cmd.pos[1] = 3;
					return false;
				}
			case 4:
				if (!slice_intra_DCT(0, (u8*)mb8.Y + DCT_offset + 8, DCT_stride, ipu_cmd.pos[1] == 4))
				{
					ipu_cmd.pos[1] = 4;
					return false;
				}
			case 5:
				if (!slice_intra_DCT(1, (u8*)mb8.Cb, decoder_stride >> 1, ipu_cmd.pos[1] == 5))
				{
					ipu_cmd.pos[1] = 5;
					return false;
				}
			case 6:
				if (!slice_intra_DCT(2, (u8*)mb8.Cr, decoder_stride >> 1, ipu_cmd.pos[1] == 6))
				{
					ipu_cmd.pos[1] = 6;
					return false;
				}
				break;

			jNO_DEFAULT;
			}

			// Copy macroblock8 to macroblock16 - without sign extension.
			// Manually inlined due to MSVC refusing to inline the SSE-optimized version.
			{
				const u8	*s = (const u8*)&mb8;
				u16			*d = (u16*)&mb16;

				//Y  bias	- 16 * 16
				//Cr bias	- 8 * 8
				//Cb bias	- 8 * 8

				__m128i zeroreg = _mm_setzero_si128();

				for (uint i = 0; i < (256+64+64) / 32; ++i)
				{
					//*d++ = *s++;
					__m128i woot1 = _mm_load_si128((__m128i*)s);
					__m128i woot2 = _mm_load_si128((__m128i*)s+1);
					_mm_store_si128((__m128i*)d,	_mm_unpacklo_epi8(woot1, zeroreg));
					_mm_store_si128((__m128i*)d+1,	_mm_unpackhi_epi8(woot1, zeroreg));
					_mm_store_si128((__m128i*)d+2,	_mm_unpacklo_epi8(woot2, zeroreg));
					_mm_store_si128((__m128i*)d+3,	_mm_unpackhi_epi8(woot2, zeroreg));
					s += 32;
					d += 32;
				}
			}
		}
		else
		{
			if (decoder.macroblock_modes & MACROBLOCK_PATTERN)
			{
				switch(ipu_cmd.pos[1])
				{
				case 0:
					decoder.coded_block_pattern = get_coded_block_pattern();  // max 9bits
				case 1:
					if (decoder.coded_block_pattern & 0x20)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Y, DCT_stride, ipu_cmd.pos[1] == 1))
						{
							ipu_cmd.pos[1] = 1;
							return false;
						}
					}
				case 2:
					if (decoder.coded_block_pattern & 0x10)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Y + 8, DCT_stride, ipu_cmd.pos[1] == 2))
						{
							ipu_cmd.pos[1] = 2;
							return false;
						}
					}
				case 3:
					if (decoder.coded_block_pattern & 0x08)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Y + DCT_offset, DCT_stride, ipu_cmd.pos[1] == 3))
						{
							ipu_cmd.pos[1] = 3;
							return false;
						}
					}
				case 4:
					if (decoder.coded_block_pattern & 0x04)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Y + DCT_offset + 8, DCT_stride, ipu_cmd.pos[1] == 4))
						{
							ipu_cmd.pos[1] = 4;
							return false;
						}
					}
				case 5:
					if (decoder.coded_block_pattern & 0x2)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Cb, decoder_stride >> 1, ipu_cmd.pos[1] == 5))
						{
							ipu_cmd.pos[1] = 5;
							return false;
						}
					}
				case 6:
					if (decoder.coded_block_pattern & 0x1)
					{
						if (!slice_non_intra_DCT((s16*)mb16.Cr, decoder_stride >> 1, ipu_cmd.pos[1] == 6))
						{
							ipu_cmd.pos[1] = 6;
							return false;
						}
					}
					break;

				jNO_DEFAULT;
				}
			}
		}

		// Send The MacroBlock via DmaIpuFrom
		ipuRegs.ctrl.SCD = 0;
		coded_block_pattern = decoder.coded_block_pattern;

		decoder.mbc = 1;
		decoder.SetOutputTo(mb16);

	case 3:
	{
		pxAssume(decoder.ipu0_data > 0);

		uint read = ipu_fifo.out.write((u32*)decoder.GetIpuDataPtr(), decoder.ipu0_data);
		decoder.AdvanceIpuDataBy(read);

		if (decoder.ipu0_data != 0)
		{
			// IPU FIFO filled up -- Will have to finish transferring later.
			ipu_cmd.pos[0] = 3;
			return false;
		}

		decoder.mbc++;
		mbaCount = 0;
	}
	
	case 4:
	{
		u8 bit8;
		if (!getBits8((u8*)&bit8, 0))
		{
			ipu_cmd.pos[0] = 4;
			return false;
		}

		if (bit8 == 0)
		{
			g_BP.Align();
			ipuRegs.ctrl.SCD = 1;
		}
	}
	
	case 5:
		if (!getBits32((u8*)&ipuRegs.top, 0))
		{
			ipu_cmd.pos[0] = 5;
			return false;
		}

		ipuRegs.top = BigEndian(ipuRegs.top);
		break;
	}

	return true;
}