/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdlib.h>

#include "zerospu2.h"

#include "soundtouch/SoundTouch.h"


// VOICE_PROCESSED definitions
tSPU_ATTR* VOICE_PROCESSED::GetCtrl()
{
	return &spu2attr(memchannel);
}

void VOICE_PROCESSED::SetVolume(s32 iProcessRight)
{
	u16 vol = iProcessRight ? pvoice->right.word : pvoice->left.word;

	if (vol&0x8000) // sweep not working
	{
		short sInc=1;							// -> sweep up?

		if (vol&0x2000) sInc=-1;					// -> or down?
		if (vol&0x1000) vol^=0xffff;				// -> mmm... phase inverted? have to investigate this

		vol=((vol&0x7f)+1)/2;					// -> sweep: 0..127 -> 0..64
		vol+=vol/(2*sInc);						 // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
		vol*=128;
	}
	else								         // no sweep:
	{
		if (vol&0x4000) vol=0x3fff-(vol&0x3fff);		 // -> mmm... phase inverted? have to investigate this
	}

	if ( iProcessRight )
		rightvol = vol&0x3fff;
	else
		leftvol = vol&0x3fff;

	bVolChanged = true;
}

void VOICE_PROCESSED::StartSound()
{
	ADSRX.lVolume=1; // and init some adsr vars
	ADSRX.State=0;
	ADSRX.EnvelopeVol=0;

	if (bReverb && GetCtrl()->reverb)
	{
		// setup the reverb effects
	}

	pCurr=pStart;   // set sample start

	s_1=0;		  // init mixing vars
	s_2=0;
	iSBPos=28;

	bNew=false;		 // init channel flags
	bStop=false;
	bOn=true;
	SB[29]=0;	   // init our interpolation helpers
	SB[30]=0;

	spos=0x10000L;
	SB[31]=0;
}

void VOICE_PROCESSED::VoiceChangeFrequency()
{
	iUsedFreq=iActFreq;			   // -> take it and calc steps
	sinc=(u32)pvoice->pitch<<4;

	if (!sinc) sinc=1;

	// -> freq change in simle imterpolation mode: set flag
	SB[32]=1;
}

void VOICE_PROCESSED::InterpolateUp()
{
	if (SB[32]==1)							   // flag == 1? calc step and set flag... and don't change the value in this pass
	{
		const s32 id1=SB[30]-SB[29];	// curr delta to next val
		const s32 id2=SB[31]-SB[30];	// and next delta to next-next val :)

		SB[32]=0;

		if (id1>0)										   // curr delta positive
		{
			if (id2<id1)
			{
				SB[28]=id1;
				SB[32]=2;
			}
			else if (id2<(id1<<1))
				SB[28]=(id1*sinc)/0x10000L;
			else
				SB[28]=(id1*sinc)/0x20000L;
		}
		else												// curr delta negative
		{
			if (id2>id1)
			{
				SB[28]=id1;
				SB[32]=2;
			}
			else if (id2>(id1<<1))
				SB[28]=(id1*sinc)/0x10000L;
			else
				SB[28]=(id1*sinc)/0x20000L;
		}
	}
	else if (SB[32]==2)							   // flag 1: calc step and set flag... and don't change the value in this pass
	{
		SB[32]=0;

		SB[28]=(SB[28]*sinc)/0x20000L;
		if (sinc<=0x8000)
			SB[29]=SB[30]-(SB[28]*((0x10000/sinc)-1));
		else
			SB[29]+=SB[28];
	}
	else												  // no flags? add bigger val (if possible), calc smaller step, set flag1
		SB[29]+=SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

void VOICE_PROCESSED::InterpolateDown()
{
	if (sinc>=0x20000L)								// we would skip at least one val?
	{
		SB[29]+=(SB[30]-SB[29])/2;  // add easy weight
		if (sinc>=0x30000L)							  // we would skip even more vals?
			SB[29]+=(SB[31]-SB[30])/2; // add additional next weight
	}
}

void VOICE_PROCESSED::FModChangeFrequency(s32 ns)
{
	s32 NP=pvoice->pitch;

	NP=((32768L+iFMod[ns])*NP)/32768L;

	if (NP>0x3fff) NP=0x3fff;
	if (NP<0x1)	NP=0x1;

	NP = (SAMPLE_RATE * NP) / (4096L);							   // calc frequency

	iActFreq=NP;
	iUsedFreq=NP;
	sinc=(((NP/10)<<16)/4800);
	if (!sinc) sinc=1;

	// freq change in simple interpolation mode
	SB[32]=1;

	iFMod[ns]=0;
}

static void __forceinline GetNoiseValues(s32& VD)
{
	static s32 Seed = 0x41595321;

	if(Seed&0x100)
		VD = (s32)((Seed&0xff)<<8);
	else if (!(Seed&0xffff))
		VD = (s32)0x8000;
	else
		VD = (s32)0x7fff;

#ifdef _WIN32
	__asm {
		MOV eax,Seed
		ROR eax,5
		XOR eax,0x9a
		MOV ebx,eax
		ROL eax,2
		ADD eax,ebx
		XOR eax,ebx
		ROR eax,3
		MOV Seed,eax
	}
#else
	__asm__ (
		".intel_syntax\n"
		"MOV %%eax,%1\n"
		"ROR %%eax,5\n"
		"XOR %%eax,0x9a\n"
		"MOV %%ebx,%%eax\n"
		"ROL %%eax,2\n"
		"ADD %%eax,%%ebx\n"
		"XOR %%eax,%%ebx\n"
		"ROR %%eax,3\n"
		"MOV %0,%%eax\n"
		".att_syntax\n" : "=r"(Seed) :"r"(Seed));
#endif
}

// fixme - noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff
int VOICE_PROCESSED::iGetNoiseVal()
{
	s32 fa;

	/*if ((dwNoiseVal<<=1)&0x80000000L)
	{
		dwNoiseVal^=0x0040001L;
		fa = ((dwNoiseVal>>2)&0x7fff);
		fa = -fa;
	}
	else
		fa=(dwNoiseVal>>2)&0x7fff;*/
	GetNoiseValues(fa);

	// mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
	fa=iOldNoise + ((fa - iOldNoise) / ((0x001f - (GetCtrl()->noiseFreq)) + 1));

	clamp16(fa);

	iOldNoise=fa;
	SB[29] = fa;							   // -> store noise val in "current sample" slot
	return fa;
}

void VOICE_PROCESSED::StoreInterpolationVal(s32 fa)
{
	if (bFMod==2)								// fmod freq channel
		SB[29]=fa;
	else
	{
		if (!GetCtrl()->spuUnmute)
			fa=0;					   // muted?
		else												// else adjust
		{
			clamp16(fa);
		}

		SB[28] = 0;
		SB[29] = SB[30];			  // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
		SB[30] = SB[31];
		SB[31] = fa;
		SB[32] = 1;							 // -> flag: calc new interolation
	}
}

s32 VOICE_PROCESSED::iGetInterpolationVal()
{
	s32 fa;

	if (bFMod==2) return SB[29];

	if (sinc<0x10000L)					   // -> upsampling?
		InterpolateUp();					 // --> interpolate up
	else
		InterpolateDown();				   // --> else down

	fa=SB[29];
	return fa;
}

s32 VOICE_PROCESSED::iGetVal()
{
	if (bNoise)
		return iGetNoiseVal();
	else
		return iGetInterpolationVal();
}

void VOICE_PROCESSED::Stop()
{
}
