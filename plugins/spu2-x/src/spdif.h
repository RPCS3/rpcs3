//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#ifndef SPDIF_H_INCLUDED
#define SPDIF_H_INCLUDED

#ifndef u32
typedef unsigned int u32;
#endif

/*
   Preamble    cell-order         cell-order
            (last cell "0")    (last cell "1")
   ----------------------------------------------
   "B"         11101000           00010111
   "M"         11100010           00011101
   "W"         11100100           00011011

	Only the lower 4 bits are used.

Preamble B: Marks a word containing data for channel A (left)
    at the start of the data-block.

Preamble M: Marks a word with data for channel A that isn't
    at the start of the data-block.

Preamble W: Marks a word containing data for channel B.
    (right, for stereo). When using more than 2
    channels, this could also be any other channel
    (except for A).

   bits           meaning
   ----------------------------------------------------------
   0-3            Preamble (see above; special structure)

   4-7            Auxillary-audio-databits

   8-27           Sample
					(A 24-bit sample can be used (using bits 4-27).
					 A CD-player uses only 16 bits, so only bits
					 13 (LSB) to 27 (MSB) are used. Bits 4-12 are
					 set to 0).

   28             Validity
					(When this bit is set, the sample should not
					 be used by the receiver. A CD-player uses
					 the 'error-flag' to set this bit).

   29             Subcode-data

   30             Channel-status-information

   31             Parity (bit 0-3 are not included)

*/

typedef struct _subframe
{
	u32 preamble:4;
	u32 aux_data:4;
	u32 snd_data:20;
	u32 validity:1;
	u32 subcode:1;
	u32 chstatus:1;
	u32 parity:1;
} subframe;

/*
   bit            meaning
   -------------------------------------------------------------
   0-3            controlbits:
						bit 0: 0 (is set to 1 during 4 channel transmission)
						bit 1: 0=Digital audio, 1=Non-audio   (reserved to be 0 on old S/PDIF specs)
						bit 2: copy-protection. Copying is allowed when this bit is set.
						bit 3: is set when pre-emphasis is used.

   4-7            0 (reserved)

   9-15           catagory-code:
						0 = common 2-channel format
						1 = 2-channel CD-format
							(set by a CD-player when a subcode is transmitted)
						2 = 2-channel PCM-encoder-decoder format

						others are not used

   19-191         0 (reserved)
*/

typedef struct _chstatus
{
	u8 ctrlbits:4;
	u8 reservd1:4;
	u8 category;
	u8 reservd2[22];
} chstatus:

#endif//SPDIF_H_INCLUDED