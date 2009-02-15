/*  SPU2-X
 *  A plugin for Emulating the Sound Processing Unit of the Playstation 2
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

#ifndef _DPLII_H_
#define _DPLII_H_

#include "PS2Etypes.h"
#include "lowpass.h"

class DPLII
{
public:
	static const bool UseAveraging = false;

protected:
	s32 LAccum;
	s32 RAccum;
	s32 ANum;

	LPF_data lpf_l;
	LPF_data lpf_r;

	u8 bufdone;
	s32 Gfl,Gfr;

	s32 spdif_data[6];
	s32 LMax,RMax;

	s32 LBuff[128];
	s32 RBuff[128];

public:
	DPLII( s32 lowpass_freq, s32 samplerate );
	void Convert( s16 *obuffer, s32 ValL, s32 ValR );
};

#endif
