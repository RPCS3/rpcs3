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

#include "zerospu2.h"
#include "zeroworker.h"
#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

s32 g_logsound = 0;
WavOutFile* g_pWavRecord=NULL; // used for recording

const s32 f[5][2] = {   
				{    0,     0 },
				{  60,     0 },
				{ 115, -52 },
				{   98, -55 },
				{ 122, -60 } };
				
s32 predict_nr, shift_factor, flags;
s32 s_1, s_2;

static __forceinline s32 SetPacket(s32 val)
{
	s32 ret;
	if (val & 0x8000) val |= 0xffff0000;
	ret = (val >> shift_factor);
	ret += ((s_1 * f[predict_nr][0]) >> 6) + ((s_2 * f[predict_nr][1]) >> 6);
	s_2 = s_1;
	s_1 = ret;
	
	return ret;
}

void SPU2Loop(VOICE_PROCESSED* pChannel, u32 ch)
{
	u8* start;
	u32 nSample;
	
	for (u32 ns = 0; ns < NSSIZE; ns++)
	{
		// fmod freq channel
		if (pChannel->bFMod == 1 && iFMod[ns]) pChannel->FModChangeFrequency(ns);

		while(pChannel->spos >= 0x10000 )
		{
			if (pChannel->iSBPos == 28)			// 28 reached?
			{
				start=pChannel->pCurr;		  // set up the current pos
				
				// special "stop" sign
				if (start == (u8*)-1)  //!pChannel->bOn
				{
					pChannel->bOn = false;						// -> turn everything off
					pChannel->ADSRX.lVolume = 0;
					pChannel->ADSRX.EnvelopeVol = 0;
					return;							  // -> and done for this channel
				}
					
				predict_nr = (s32)start[0];
				shift_factor = predict_nr&0xf;
				predict_nr >>= 4;
				flags=(s32)start[1];
				start += 2;
					
				pChannel->iSBPos = 0;

				// decode the 16byte packet
				s_1 = pChannel->s_1;
				s_2 = pChannel->s_2;
					
				for (nSample=0; nSample<28; ++start)
				{
					s32 d = (s32)*start;
					s32 s;
					
					s = ((d & 0xf)<<12);
					pChannel->SB[nSample++] = SetPacket(s);
					
					s = ((d & 0xf0) << 8);
					pChannel->SB[nSample++] = SetPacket(s);
				}
			
				// irq occurs no matter what core accesses the address
				for (s32 core = 0; core < 2; ++core) 
				{
					if (spu2attr(core).irq)		 // some callback and irq active?
					{
						// if irq address reached or irq on looping addr, when stop/loop flag is set 
						u8* pirq = (u8*)pSpuIrq[core];
						
						if ((pirq > (start - 16)  && pirq <= start) || 
							((flags & 1) && (pirq > (pChannel->pLoop - 16) && pirq <= pChannel->pLoop)))
						{
							IRQINFO |= 4 << core;
							SPU2_LOG("SPU2Worker:interrupt\n");
							irqCallbackSPU2();
						}
					}
				}

				// flag handler
				if ((flags & 4) && (!pChannel->bIgnoreLoop))
					pChannel->pLoop = start - 16;				// loop address

				if (flags & 1)							   // 1: stop/loop
				{
					// We play this block out first...
					dwEndChannel2[ch / 24] |= (1 << (ch % 24));
				
					if (flags != 3 || pChannel->pLoop == NULL)
					{									  // and checking if pLoop is set avoids crashes, yeah
						start = (u8*)-1;
						pChannel->bStop = true;
						pChannel->bIgnoreLoop = false;
					}
					else
					{
						start = pChannel->pLoop;
					}
				}

				pChannel->pCurr = start;					// store values for next cycle
				pChannel->s_1 = s_1;
				pChannel->s_2 = s_2;
			}

			pChannel->StoreInterpolationVal(pChannel->SB[pChannel->iSBPos++]); // get sample data
			pChannel->spos -= 0x10000;
		}
		
		s32 sval = (MixADSR(pChannel) * pChannel->iGetVal()) / 1023;   // mix adsr with noise or sample val.

		if (pChannel->bFMod == 2)						// fmod freq channel
		{
			// -> store 1T sample data, use that to do fmod on next channel
			if (!pChannel->bNoise) iFMod[ns] = sval;
		}
		else 
		{
			if (pChannel->bVolumeL)
				s_buffers[ns][0] += (sval * pChannel->leftvol) >> 14;
				
			if (pChannel->bVolumeR)
				s_buffers[ns][1] += (sval * pChannel->rightvol) >> 14;
		}

		pChannel->spos += pChannel->sinc;
	}
}
// simulate SPU2 for 1ms
void SPU2Worker()
{
	// assume s_buffers are zeroed out
	if (dwNewChannel2[0] || dwNewChannel2[1]) s_pAudioBuffers[s_nCurBuffer].newchannels++;
		
	VOICE_PROCESSED* pChannel = voices;
	for (u32 ch=0; ch < SPU_NUMBER_VOICES; ch++, pChannel++)			  // loop em all... we will collect 1 ms of sound of each playing channel
	{
		if (pChannel->bNew) 
		{
			pChannel->StartSound();						 // start new sound
			dwEndChannel2[ch / 24] &= ~(1 << (ch % 24));				  // clear end channel bit
			dwNewChannel2[ch / 24] &= ~(1 << (ch % 24));				  // clear channel bit
		}

		if (!pChannel->bOn) continue;

		if (pChannel->iActFreq != pChannel->iUsedFreq)	 // new psx frequency?
			pChannel->VoiceChangeFrequency();

		// loop until 1 ms of data is reached
		SPU2Loop(pChannel, ch);
	}

	// mix all channels
	MixChannels(0);
	MixChannels(1);

	if ( g_bPlaySound ) 
	{
		assert( s_pCurOutput != NULL);

		for (u32 ns = 0; ns < NSSIZE; ns++) 
		{
			// clamp and write
			clampandwrite16(s_pCurOutput[0],s_buffers[ns][0]);
			clampandwrite16(s_pCurOutput[1],s_buffers[ns][1]);
			
			s_pCurOutput += 2;
			s_buffers[ns][0] = 0;
			s_buffers[ns][1] = 0;
		}
		// check if end reached

		if ((uptr)s_pCurOutput - (uptr)s_pAudioBuffers[s_nCurBuffer].pbuf >= 4 * NS_TOTAL_SIZE) 
		{

			if ( conf.options & OPTION_RECORDING ) 
			{
				static s32 lastrectime = 0;
				if (timeGetTime() - lastrectime > 5000) 
				{
					WARN_LOG("ZeroSPU2: recording\n");
					lastrectime = timeGetTime();
				}
				LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf+2, 4, NS_TOTAL_SIZE);
			}

			if ( s_nQueuedBuffers >= ArraySize(s_pAudioBuffers)-1 ) 
			{
				//ZeroSPU2: dropping packets! game too fast
				s_nDropPacket += NSFRAMES;
				s_GlobalTimeStamp = GetMicroTime();
			}
			else {
				// submit to final mixer
#ifdef ZEROSPU2_DEVBUILD
				if ( g_logsound ) 
					LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf + 2, 4, NS_TOTAL_SIZE);
#endif
				if ( g_startcount == 0xffffffff ) 
				{
					g_startcount = timeGetTime();
					g_packetcount = 0;
				}

				if ( conf.options & OPTION_TIMESTRETCH ) 
				{
					u32 newtotal = s_nTotalDuration - s_nDurations[s_nCurDuration];
					u64 newtime = GetMicroTime();
					u32 duration;
					
					if (s_GlobalTimeStamp == 0) s_GlobalTimeStamp = newtime - NSFRAMES * 1000;
					duration = (u32)(newtime-s_GlobalTimeStamp);
					
					s_nDurations[s_nCurDuration] = duration;
					s_nTotalDuration = newtotal + duration;
					s_nCurDuration = (s_nCurDuration+1)%ArraySize(s_nDurations);
					s_GlobalTimeStamp = newtime;
					s_pAudioBuffers[s_nCurBuffer].timestamp = timeGetTime();
					s_pAudioBuffers[s_nCurBuffer].avgtime = s_nTotalDuration/ArraySize(s_nDurations);
				}

				s_pAudioBuffers[s_nCurBuffer].len = 4 * NS_TOTAL_SIZE;
				InterlockedExchangeAdd((long*)&s_nQueuedBuffers, 1);

				s_nCurBuffer = (s_nCurBuffer+1)%ArraySize(s_pAudioBuffers);
				s_pAudioBuffers[s_nCurBuffer].newchannels = 0; // reset
			}

			// restart
			s_pCurOutput = (s16*)s_pAudioBuffers[s_nCurBuffer].pbuf;
		}
	}
}

// size is in bytes
void LogPacketSound(void* packet, s32 memsize)
{
	u16 buf[28];

	u8* pstart = (u8*)packet;
	s_1 = s_2 = 0;
	
	for (s32 i = 0; i < memsize; i += 16) 
	{
		predict_nr = (s32)pstart[0];
		shift_factor = predict_nr&0xf;
		predict_nr >>= 4;
		pstart += 2;

		for (s32 nSample = 0;nSample < 28; ++pstart)
		{
			s32 d = (s32)*pstart;
			
			s32 temp;
			
			temp = ((d & 0xf) << 12);
			buf[nSample++] = SetPacket(temp);
			temp = ((d & 0xf0) << 8);
			buf[nSample++] = SetPacket(temp);
		}

		LogRawSound(buf, 2, buf, 2, 28);
	}
}

void LogRawSound(void* pleft, s32 leftstride, void* pright, s32 rightstride, s32 numsamples)
{
	if (g_pWavRecord == NULL ) 
		g_pWavRecord = new WavOutFile(RECORD_FILENAME, SAMPLE_RATE, 16, 2);

	u8* left = (u8*)pleft;
	u8* right = (u8*)pright;
	static vector<s16> tempbuf;

	tempbuf.resize(2 * numsamples);

	for (s32 i = 0; i < numsamples; ++i) 
	{
		tempbuf[2*i+0] = *(s16*)left;
		tempbuf[2*i+1] = *(s16*)right;
		left += leftstride;
		right += rightstride;
	}

	g_pWavRecord->write(&tempbuf[0], numsamples*2);
}
