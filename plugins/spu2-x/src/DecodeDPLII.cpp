/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Originally based on SPU2ghz v1.9 beta, by David Quintana.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Spu2.h"
#include "DPLII.h"
#include <string.h>

static const u8 sLogTable[256] = {
	0x00,0x3C,0x60,0x78,0x8C,0x9C,0xA8,0xB4,0xBE,0xC8,0xD0,0xD8,0xDE,0xE4,0xEA,0xF0,
	0xF6,0xFA,0xFE,0x04,0x08,0x0C,0x10,0x14,0x16,0x1A,0x1E,0x20,0x24,0x26,0x2A,0x2C,
	0x2E,0x32,0x34,0x36,0x38,0x3A,0x3E,0x40,0x42,0x44,0x46,0x48,0x4A,0x4C,0x4E,0x50,
	0x50,0x52,0x54,0x56,0x58,0x5A,0x5A,0x5C,0x5E,0x60,0x60,0x62,0x64,0x66,0x66,0x68,
	0x6A,0x6A,0x6C,0x6E,0x6E,0x70,0x70,0x72,0x74,0x74,0x76,0x76,0x78,0x7A,0x7A,0x7C,
	0x7C,0x7E,0x7E,0x80,0x80,0x82,0x82,0x84,0x84,0x86,0x86,0x88,0x88,0x8A,0x8A,0x8C,
	0x8C,0x8C,0x8E,0x8E,0x90,0x90,0x92,0x92,0x92,0x94,0x94,0x96,0x96,0x96,0x98,0x98,
	0x9A,0x9A,0x9A,0x9C,0x9C,0x9C,0x9E,0x9E,0xA0,0xA0,0xA0,0xA2,0xA2,0xA2,0xA4,0xA4,
	0xA4,0xA6,0xA6,0xA6,0xA8,0xA8,0xA8,0xAA,0xAA,0xAA,0xAC,0xAC,0xAC,0xAC,0xAE,0xAE,
	0xAE,0xB0,0xB0,0xB0,0xB2,0xB2,0xB2,0xB2,0xB4,0xB4,0xB4,0xB6,0xB6,0xB6,0xB6,0xB8,
	0xB8,0xB8,0xB8,0xBA,0xBA,0xBA,0xBC,0xBC,0xBC,0xBC,0xBE,0xBE,0xBE,0xBE,0xC0,0xC0,
	0xC0,0xC0,0xC2,0xC2,0xC2,0xC2,0xC2,0xC4,0xC4,0xC4,0xC4,0xC6,0xC6,0xC6,0xC6,0xC8,
	0xC8,0xC8,0xC8,0xC8,0xCA,0xCA,0xCA,0xCA,0xCC,0xCC,0xCC,0xCC,0xCC,0xCE,0xCE,0xCE,
	0xCE,0xCE,0xD0,0xD0,0xD0,0xD0,0xD0,0xD2,0xD2,0xD2,0xD2,0xD2,0xD4,0xD4,0xD4,0xD4,
	0xD4,0xD6,0xD6,0xD6,0xD6,0xD6,0xD8,0xD8,0xD8,0xD8,0xD8,0xD8,0xDA,0xDA,0xDA,0xDA,
	0xDA,0xDC,0xDC,0xDC,0xDC,0xDC,0xDC,0xDE,0xDE,0xDE,0xDE,0xDE,0xDE,0xE0,0xE0,0xE0,
};

DPLII::DPLII( s32 lowpass_freq, s32 samplerate ) :
	LAccum( 0 ),
	RAccum( 0 ),
	ANum( 0 ),
	lpf_l( lowpass_freq, samplerate ),
	lpf_r( lowpass_freq, samplerate ),
	bufdone( 1 ),
	Gfl( 0 ),
	Gfr( 0 ),
	LMax( 0 ),
	RMax( 0 )
{
	memset( LBuff, 0, sizeof( LBuff ) );
	memset( RBuff, 0, sizeof( RBuff ) );
	memset( spdif_data, 0, sizeof( spdif_data ) );
}

// Takes a single stereo input sample and translates it into six output samples
// for 5.1 audio support on non-DPL2 hardware.
void DPLII::Convert( s16 *obuffer, s32 ValL, s32 ValR )
{
	ValL >>= 2;
	ValR >>= 2;
	if(PlayMode&4)
	{
		spdif_get_samples(spdif_data);
	}
	else
	{
		spdif_data[0]=0;
		spdif_data[1]=0;
		spdif_data[2]=0;
		spdif_data[3]=0;
		spdif_data[4]=0;
		spdif_data[5]=0;
	}

	//const u8 shift = SndOutVolumeShift;

	s32 XL = abs(ValL>>8);
	s32 XR = abs(ValR>>8);

	if(XL>LMax) LMax = XL;
	if(XR>RMax) RMax = XR;

	ANum++;
	if(ANum>=128)
	{
		ANum=0;
		LAccum = 1+((LAccum * 224 + LMax * 31)>>8);
		RAccum = 1+((RAccum * 224 + RMax * 31)>>8);

		LMax = 0;
		RMax = 0;

		s32 Tfl=(RAccum)*255/(LAccum);
		s32 Tfr=(LAccum)*255/(RAccum);

		int gMax = max(Tfl,Tfr);
		Tfl=Tfl*255/gMax;
		Tfr=Tfr*255/gMax;

		if(Tfl>255) Tfl=255;
		if(Tfr>255) Tfr=255;
		if(Tfl<1) Tfl=1;
		if(Tfr<1) Tfr=1;

		Gfl = (Gfl * 200 + Tfl * 56)>>8;
		Gfr = (Gfr * 200 + Tfr * 56)>>8;

	}

	s32 L,R,C,LFE,SL,SR,LL,LR;

	extern double pow_2_31;
	LL = (s32)(lpf_l.sample((ValL>>4)/pow_2_31)*pow_2_31);
	LR = (s32)(lpf_r.sample((ValR>>4)/pow_2_31)*pow_2_31);
	LFE = (LL + LR)>>4;

	C=(ValL+ValR)>>1; //16.8

	ValL-=C;//16.8
	ValR-=C;//16.8

	L=ValL>>8; //16.0
	R=ValR>>8; //16.0
	C=C>>8;    //16.0

	const s32 Cfl = 1 + sLogTable[Gfl];
	const s32 Cfr = 1 + sLogTable[Gfr];

	const s32 VL = (ValL>>4) * Cfl; //16.12
	const s32 VR = (ValR>>4) * Cfr;

	const s32 SC = (VL-VR)>>15;

	SL = (((VR/148 - VL/209)>>4)*Cfr)>>8;
	SR = (((VR/209 - VL/148)>>4)*Cfl)>>8;

	int AddCX  = (C * Config_DSound51.AddCLR)>>8;

	obuffer[0]=spdif_data[0] + (((L   * Config_DSound51.GainL  ))>>8) + AddCX;
	obuffer[1]=spdif_data[1] + (((R   * Config_DSound51.GainR  ))>>8) + AddCX;
	obuffer[2]=spdif_data[2] + (((C   * Config_DSound51.GainC  ))>>8); // - AddCX;
	obuffer[3]=spdif_data[3] + (((LFE * Config_DSound51.GainLFE))>>8);
	obuffer[4]=spdif_data[4] + (((SL  * Config_DSound51.GainSL ))>>8);
	obuffer[5]=spdif_data[5] + (((SR  * Config_DSound51.GainSR ))>>8);

#if 0
	if( UseAveraging )
	{
		LAccum+=abs(ValL);
		RAccum+=abs(ValR);
		ANum++;

		if(ANum>=512)
		{
			LMax=0;RMax=0;

			LAccum/=ANum;
			RAccum/=ANum;
			ANum=0;

			for(int i=0;i<127;i++)
			{
				LMax+=LBuff[i];
				RMax+=RBuff[i];
				LBuff[i]=LBuff[i+1];
				RBuff[i]=RBuff[i+1];
			}
			LBuff[127]=LAccum;
			RBuff[127]=RAccum;
			LMax+=LAccum;
			RMax+=RAccum;

			s32 TL = (LMax>>15)+1;
			s32 TR = (RMax>>15)+1;

			Gfl=(RMax)/(TL);
			Gfr=(LMax)/(TR);

			if(Gfl>255) Gfl=255;
			if(Gfr>255) Gfr=255;
		}
	}
	else
	{
		if(ValL>LMax) LMax = ValL;
		if(-ValL>LMax) LMax = -ValL;
		if(ValR>RMax) RMax = ValR;
		if(-ValR>RMax) RMax = -ValR;

		ANum++;
		if(ANum>=128)
		{
			// shift into a 21 bit value
			const u8 shift = SndOutVolumeShift-5;

			ANum=0;
			LAccum = ((LAccum * 224) + (LMax>>shift))>>8;
			RAccum = ((RAccum * 224) + (RMax>>shift))>>8;

			LMax=0;
			RMax=0;

			if(LAccum<1) LAccum=1;
			if(RAccum<1) RAccum=1;

			Gfl=(RAccum*256)/LAccum;
			Gfr=(LAccum*256)/RAccum;

			int gMax = max(Gfl,Gfr);

			Gfl=(Gfl*256)/gMax;
			Gfr=(Gfr*256)/gMax;

			if(Gfl>255) Gfl=255;
			if(Gfr>255) Gfr=255;
			if(Gfl<1) Gfl=1;
			if(Gfr<1) Gfr=1;
		}
	}

	Gfr = 1; Gfl = 1;

	extern double pow_2_31;

	// shift Values into 12 bits:
	u8 shift2 = SndOutVolumeShift + 4;

	const s32 LL = (s32)(lpf_l.sample((ValL>>shift2)/pow_2_31)*pow_2_31);
	const s32 LR = (s32)(lpf_r.sample((ValR>>shift2)/pow_2_31)*pow_2_31);
	const s32 LFE = (LL + LR)>>4;

	s32 C = (ValL+ValR)>>1; //16.8

	ValL -= C;//16.8
	ValR -= C;//16.8

	const s32 L = ValL>>SndOutVolumeShift; //16.0
	const s32 R = ValR>>SndOutVolumeShift; //16.0
	C >>= SndOutVolumeShift;    //16.0

	const s32 VL = (ValL>>4) * Gfl; //16.12
	const s32 VR = (ValR>>4) * Gfr;

	s32 SL = (VL/209 - VR/148)>>4; //16.0 (?)
	s32 SR = (VL/148 - VR/209)>>4; //16.0 (?)

	// increase surround stereo separation
	const int SC = (SL+SR)>>1; //16.0
	const int SLd = SL - SC;   //16.0
	const int SRd = SL - SC;   //16.0

	SL = (SLd * 209 + SC * 148)>>8; //16.0
	SR = (SRd * 209 + SC * 148)>>8; //16.0

	obuffer[0]=spdif_data[0] + (((L   * Config_DSound51.GainL  ) + (C * Config_DSound51.AddCLR))>>8);
	obuffer[1]=spdif_data[1] + (((R   * Config_DSound51.GainR  ) + (C * Config_DSound51.AddCLR))>>8);
	obuffer[2]=spdif_data[2] + (((C   * Config_DSound51.GainC  ))>>8);
	obuffer[3]=spdif_data[3] + (((LFE * Config_DSound51.GainLFE))>>8);
	obuffer[4]=spdif_data[4] + (((SL  * Config_DSound51.GainSL ))>>8);
	obuffer[5]=spdif_data[5] + (((SR  * Config_DSound51.GainSR ))>>8);
#endif
}
