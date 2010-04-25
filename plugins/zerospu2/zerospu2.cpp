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

#ifdef _WIN32
#include "svnrev.h"
#endif

#include "Targets/SoundTargets.h"

#include <assert.h>
#include <stdlib.h>

#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

char libraryName[256];

FILE *spu2Log;
Config conf;

ADMA adma[2];

u32 MemAddr[2];
u32 g_nSpuInit = 0;
u16 interrupt = 0;
s8 *spu2regs = NULL;
u16* spu2mem = NULL;
u16* pSpuIrq[2] = {NULL};
u32 dwNewChannel2[2] = {0}; // keeps track of what channels that have been turned on
u32 dwEndChannel2[2] = {0}; // keeps track of what channels have ended
u32 dwNoiseVal=1;						  // global noise generator
bool g_bPlaySound = true; // if true, will output sound, otherwise no
s32 iFMod[NSSIZE];
s32 s_buffers[NSSIZE][2]; // left and right buffers

// mixer thread variables
bool s_bThreadExit = true;
s32 s_nDropPacket = 0;
string s_strIniPath( "inis/" );

#ifdef _WIN32
LARGE_INTEGER g_counterfreq;
extern HWND hWMain;
HANDLE s_threadSPU2 = NULL;
DWORD WINAPI SPU2ThreadProc(LPVOID);
#else
#include <pthread.h>
pthread_t s_threadSPU2;
void* SPU2ThreadProc(void*);
#endif

AUDIOBUFFER s_pAudioBuffers[NSPACKETS];
s32 s_nCurBuffer = 0, s_nQueuedBuffers = 0;
s16* s_pCurOutput = NULL;
u32 g_startcount=0xffffffff;
u32 g_packetcount=0;

// time stretch variables
soundtouch::SoundTouch* pSoundTouch=NULL;
extern WavOutFile* g_pWavRecord; // used for recording

u64 s_GlobalTimeStamp = 0;
s32 s_nDurations[64]={0};
s32 s_nCurDuration=0;
s32 s_nTotalDuration=0;

s32 SPUCycles = 0, SPUWorkerCycles = 0;
s32 SPUStartCycle[2];
s32 SPUTargetCycle[2];

int ADMASWrite(int c);

void InitADSR();

// functions of main emu, called on spu irq
void (*irqCallbackSPU2)()=0;
#ifndef ENABLE_NEW_IOPDMA_SPU2
void (*irqCallbackDMA4)()=0;
void (*irqCallbackDMA7)()=0;
#endif

uptr g_pDMABaseAddr=0;
u32 RateTable[160];

// channels and voices
VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1]; // +1 for modulation

static void InitLibraryName()
{
#ifdef _WIN32
#ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "ZeroSPU2" );

#elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "ZeroSPU2"
#	ifdef PCSX2_DEBUG
		"-Debug"
#	elif defined( ZEROSPU2_DEVBUILD )
		"-Dev"
#	endif
		);
#else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s( libraryName, "ZeroSPU2 r%d%s"
#	ifdef PCSX2_DEBUG
		"-Debug"
#	elif defined( ZEROSPU2_DEVBUILD )
		"-Dev"
#	endif
		,SVN_REV,
		SVN_MODS ? "m" : ""
	);
#endif
#else
// I'll hook in svn version code later. --arcum42

	strcpy( libraryName, "ZeroSPU2 Playground"
#	ifdef PCSX2_DEBUG
		"-Debug"
#	elif defined( ZEROSPU2_DEVBUILD )
		"-Dev"
#	endif
		);
#	endif

}

u32 CALLBACK PS2EgetLibType()
{
	return PS2E_LT_SPU2;
}

char* CALLBACK PS2EgetLibName()
{
	InitLibraryName();
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{
	return (SPU2_MINOR<<24) | (SPU2_VERSION<<16) | (SPU2_REVISION<<8) | SPU2_BUILD;
}

void __Log(char *fmt, ...)
{
	va_list list;

	if (!conf.Log || spu2Log == NULL) return;

	va_start(list, fmt);
	vfprintf(spu2Log, fmt, list);
	va_end(list);
}

void __LogToConsole(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (!conf.Log || spu2Log == NULL)
		vfprintf(spu2Log, fmt, list);

	printf("ZeroSPU2: ");
	vprintf(fmt, list);
	va_end(list);
}

void CALLBACK SPU2setSettingsDir(const char* dir)
{
	s_strIniPath = (dir==NULL) ? "inis/" : dir;
}

void InitApi()
{
	// This is a placeholder till there is actually some way to choose Apis. For the moment, just
	// Modify this so it does whichever one you want to use.
#ifdef _WIN32
	InitDSound();
#else
#if defined(ZEROSPU2_ALSA)
	InitAlsa();
#elif defined(ZEROSPU2_OSS)
	InitOSS();
#elif defined(ZEROSPU2_PORTAUDIO)
	InitPortAudio();
#endif
#endif
}

s32 CALLBACK SPU2init()
{
	LOG_CALLBACK("SPU2init()\n");
	spu2Log = fopen("logs/spu2.txt", "w");
	if (spu2Log) setvbuf(spu2Log, NULL,  _IONBF, 0);

	SPU2_LOG("Spu2 null version %d,%d\n",SPU2_REVISION,SPU2_BUILD);
	SPU2_LOG("SPU2init\n");

#ifdef _WIN32
	QueryPerformanceFrequency(&g_counterfreq);
#endif

	InitApi();

	spu2regs = (s8*)malloc(0x10000);
	spu2mem = (u16*)malloc(0x200000); // 2Mb
	memset(spu2regs, 0, 0x10000);
	memset(spu2mem, 0, 0x200000);
	if ((spu2mem == NULL) || (spu2regs == NULL))
	{
		SysMessage("Error allocating Memory\n");
		return -1;
	}

	memset(dwEndChannel2, 0, sizeof(dwEndChannel2));
	memset(dwNewChannel2, 0, sizeof(dwNewChannel2));
	memset(iFMod, 0, sizeof(iFMod));
	memset(s_buffers, 0, sizeof(s_buffers));

	InitADSR();

	memset(voices, 0, sizeof(voices));
	// last 24 channels have higher mem offset
	for (s32 i = 0; i < 24; ++i)
		voices[i+24].memoffset = 0x400;

	// init each channel
	for (u32 i = 0; i < ArraySize(voices); ++i)
	{
		voices[i].init(i);
	}

	return 0;
}

s32 CALLBACK SPU2open(void *pDsp)
{
	LOG_CALLBACK("SPU2open()\n");
#ifdef _WIN32
	hWMain = pDsp == NULL ? NULL : *(HWND*)pDsp;
	if (!IsWindow(hWMain))
		hWMain=GetActiveWindow();
#endif

	LoadConfig();

	SPUCycles = SPUWorkerCycles = 0;
	interrupt = 0;
	SPUStartCycle[0] = SPUStartCycle[1] = 0;
	SPUTargetCycle[0] = SPUTargetCycle[1] = 0;
	s_nDropPacket = 0;

	if ( conf.options & OPTION_TIMESTRETCH )
	{
		pSoundTouch = new soundtouch::SoundTouch();
		pSoundTouch->setSampleRate(SAMPLE_RATE);
		pSoundTouch->setChannels(2);
		pSoundTouch->setTempoChange(0);

		pSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);
		pSoundTouch->setSetting(SETTING_USE_AA_FILTER, 1);
	}

	//conf.Log = 1;

	g_bPlaySound = !(conf.options&OPTION_MUTE);

	if ( g_bPlaySound && SetupSound() != 0 )
	{
		SysMessage("ZeroSPU2: Failed to initialize sound");
		g_bPlaySound = false;
	}

	if ( g_bPlaySound ) {
		// initialize the audio buffers
		for (u32 i = 0; i < ArraySize(s_pAudioBuffers); ++i)
		{
			s_pAudioBuffers[i].pbuf = (u8*)_aligned_malloc(4 * NS_TOTAL_SIZE, 16); // 4 bytes for each sample
			s_pAudioBuffers[i].len = 0;
		}

		s_nCurBuffer = 0;
		s_nQueuedBuffers = 0;
		s_pCurOutput = (s16*)s_pAudioBuffers[0].pbuf;
		assert( s_pCurOutput != NULL);

		for (s32 i = 0; i < ArraySize(s_nDurations); ++i)
		{
			s_nDurations[i] = NSFRAMES*1000;
		}
		s_nTotalDuration = ArraySize(s_nDurations)*NSFRAMES*1000;
		s_nCurDuration = 0;

		// launch the thread
		s_bThreadExit = false;
#ifdef _WIN32
		s_threadSPU2 = CreateThread(NULL, 0, SPU2ThreadProc, NULL, 0, NULL);
		if ( s_threadSPU2 == NULL )
		{
			return -1;
		}
#else
		if ( pthread_create(&s_threadSPU2, NULL, SPU2ThreadProc, NULL) != 0 )
		{
			SysMessage("ZeroSPU2: Failed to create spu2thread\n");
			return -1;
		}
#endif
	}

	g_nSpuInit = 1;
	return 0;
}

void CALLBACK SPU2close()
{
	LOG_CALLBACK("SPU2close()\n");
	g_nSpuInit = 0;

	if ( g_bPlaySound && !s_bThreadExit ) {
		s_bThreadExit = true;
		printf("ZeroSPU2: Waiting for thread... ");
#ifdef _WIN32
		WaitForSingleObject(s_threadSPU2, INFINITE);
		CloseHandle(s_threadSPU2); s_threadSPU2 = NULL;
#else
		pthread_join(s_threadSPU2, NULL);
#endif
		printf("done\n");
	}

	RemoveSound();

	delete g_pWavRecord; g_pWavRecord = NULL;
	delete pSoundTouch; pSoundTouch = NULL;

	for (u32 i = 0; i < ArraySize(s_pAudioBuffers); ++i)
	{
		_aligned_free(s_pAudioBuffers[i].pbuf);
	}
	memset(s_pAudioBuffers, 0, sizeof(s_pAudioBuffers));
}

void CALLBACK SPU2shutdown()
{
	LOG_CALLBACK("SPU2shutdown()\n");
	free(spu2regs); spu2regs = NULL;
	free(spu2mem); spu2mem = NULL;

	if (spu2Log) fclose(spu2Log);
}

void CALLBACK SPU2async(u32 cycle)
{
	//LOG_CALLBACK("SPU2async()\n");
	SPUCycles += cycle;

	if (interrupt & (1<<2))
	{
		if (SPUCycles - SPUStartCycle[1] >= SPUTargetCycle[1])
		{
			interrupt &= ~(1<<2);
#ifndef ENABLE_NEW_IOPDMA_SPU2
			irqCallbackDMA7();
#endif
		}

	}

	if (interrupt & (1<<1))
	{
		if (SPUCycles - SPUStartCycle[0] >= SPUTargetCycle[0])
		{
			interrupt &= ~(1<<1);
#ifndef ENABLE_NEW_IOPDMA_SPU2
			irqCallbackDMA4();
#endif
		}
	}

	if ( g_nSpuInit )
	{
			while((SPUCycles-SPUWorkerCycles > 0) && (CYCLES_PER_MS < (SPUCycles - SPUWorkerCycles)))
			{
				SPU2Worker();
				SPUWorkerCycles += CYCLES_PER_MS;
			}
	}
	else
		SPUWorkerCycles = SPUCycles;
}

void InitADSR()									// INIT ADSR
{
	u32 r,rs,rd;
	s32 i;
	memset(RateTable,0,sizeof(u32)*160);		// build the rate table according to Neill's rules (see at bottom of file)

	r=3;rs=1;rd=0;

	for (i=32;i<160;i++)								   // we start at pos 32 with the real values... everything before is 0
	{
		if (r<0x3FFFFFFF)
		{
			r+=rs;
			rd++;

			if (rd==5)
			{
				rd=1;
				rs*=2;
			}
		}

		if (r>0x3FFFFFFF) r=0x3FFFFFFF;
		RateTable[i]=r;
	}
}

s32 MixADSR(VOICE_PROCESSED* pvoice)							 // MIX ADSR
{
	u32 rateadd[8] = { 0, 4, 6, 8, 9, 10, 11, 12 };

	if (pvoice->bStop)								  // should be stopped:
	{
		if (pvoice->ADSRX.ReleaseModeExp) // do release
		{
			s32 temp = ((pvoice->ADSRX.EnvelopeVol>>28)&0x7);
			pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F)) - 0x18 + rateadd[temp] + 32];
		}
		else
		{
			pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F)) - 0x0C + 32];
		}

		// bIgnoreLoop sets EnvelopeVol to 0 anyways, so we can use one if statement rather then two.
		if ((pvoice->ADSRX.EnvelopeVol<0) || (pvoice->bIgnoreLoop == 0))
		{
			pvoice->ADSRX.EnvelopeVol=0;
			pvoice->bOn=false;
			pvoice->pStart= (u8*)(spu2mem+pvoice->iStartAddr);
			pvoice->pLoop= (u8*)(spu2mem+pvoice->iStartAddr);
			pvoice->pCurr= (u8*)(spu2mem+pvoice->iStartAddr);
			pvoice->bStop = true;
			pvoice->bIgnoreLoop = false;
			//pvoice->bReverb=0;
			//pvoice->bNoise=0;
		}

		pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;

		return pvoice->ADSRX.lVolume;
	}
	else												  // not stopped yet?
	{
		s32 temp = ((pvoice->ADSRX.EnvelopeVol>>28)&0x7);

		switch (pvoice->ADSRX.State)
		{
		case 0:				   // -> attack
			if (pvoice->ADSRX.AttackModeExp)
			{
				if (pvoice->ADSRX.EnvelopeVol<0x60000000)
					pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F) - 0x10 + 32];
				else
					pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F) - 0x18 + 32];
			}
			else
			{
				pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F) - 0x10 + 32];
			}

			if (pvoice->ADSRX.EnvelopeVol<0)
			{
				pvoice->ADSRX.EnvelopeVol=0x7FFFFFFF;
				pvoice->ADSRX.State=1;
			}
			break;

		case 1:				   // -> decay
			pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F)) - 0x18+ rateadd[temp] + 32];

			if (pvoice->ADSRX.EnvelopeVol<0) pvoice->ADSRX.EnvelopeVol=0;

			if (((pvoice->ADSRX.EnvelopeVol>>27)&0xF) <= pvoice->ADSRX.SustainLevel)
				pvoice->ADSRX.State=2;
			break;

		case 2:				    // -> sustain
			if (pvoice->ADSRX.SustainIncrease)
			{
				if ((pvoice->ADSRX.SustainModeExp) && (pvoice->ADSRX.EnvelopeVol>=0x60000000))
					pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.SustainRate^0x7F) - 0x18 + 32];
				else
					pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.SustainRate^0x7F) - 0x10 + 32];

				if (pvoice->ADSRX.EnvelopeVol<0) pvoice->ADSRX.EnvelopeVol=0x7FFFFFFF;
			}
			else
			{
				if (pvoice->ADSRX.SustainModeExp)
					pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F)) - 0x1B +rateadd[temp] + 32];
				else
					pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F)) - 0x0F + 32];

				if (pvoice->ADSRX.EnvelopeVol<0) pvoice->ADSRX.EnvelopeVol=0;
			}
			break;

		default:
			// This should never happen.
			return 0;
		}

		pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
		return pvoice->ADSRX.lVolume;
	}

	return 0;
}

void MixChannels(s32 channel)
{
	// mix all channels
	s32 left_vol, right_vol;
	ADMA *Adma;

	if (channel == 0)
	{
		Adma = &adma[0];
		left_vol = REG_C0_BVOLL;
		right_vol = REG_C0_BVOLR;
	}
	else
	{
		Adma = &adma[1];
		left_vol = REG_C1_BVOLL;
		right_vol = REG_C1_BVOLR;
	}

	if ((spu2mmix(channel) & 0xF0) && spu2admas(channel))
	{
		for (u32 ns = 0; ns < NSSIZE; ns++)
		{
			u16 left_reg = 0x2000 + c_offset(channel) + Adma->Index;
			u16 right_reg = left_reg + 0x200;

			if ((spu2mmix(channel) & 0x80))
				s_buffers[ns][0] += (((s16*)spu2mem)[left_reg]*(s32)spu2Ru16(left_vol))>>16;
			if ((spu2mmix(channel) & 0x40))
				s_buffers[ns][1] += (((s16*)spu2mem)[right_reg]*(s32)spu2Ru16(right_vol))>>16;

			Adma->Index +=1;
			MemAddr[channel] += 4;

			if (Adma->Index == 128 || Adma->Index == 384)
			{
				if (ADMASWrite(channel))
				{
					if (interrupt & (0x2 * (channel + 1)))
					{
						interrupt &= ~(0x2 * (channel + 1));
						WARN_LOG("Stopping double interrupt DMA7\n");
					}
#ifndef ENABLE_NEW_IOPDMA_SPU2
					if (channel == 0)
						irqCallbackDMA4();
					else
						irqCallbackDMA7();
#endif

				}
				if (channel == 1) Adma->Enabled = 2;
			}

			if (Adma->Index == 512)
			{
				if ( Adma->Enabled == 2 ) Adma->Enabled = 0;
				Adma->Index = 0;
			}
		}
	}
}

// resamples pStereoSamples
void ResampleLinear(s16* pStereoSamples, s32 oldsamples, s16* pNewSamples, s32 newsamples)
{
	for (s32 i = 0; i < newsamples; ++i)
	{
		s32 io = i * oldsamples;
		s32 old = io / newsamples;
		s32 rem = io - old * newsamples;

		old *= 2;
		s32 newsampL = pStereoSamples[old] * (newsamples - rem) + pStereoSamples[old+2] * rem;
		s32 newsampR = pStereoSamples[old+1] * (newsamples - rem) + pStereoSamples[old+3] * rem;
		pNewSamples[2 * i] = newsampL / newsamples;
		pNewSamples[2 * i + 1] = newsampR / newsamples;
	}
}

static __aligned16 s16 s_ThreadBuffer[NS_TOTAL_SIZE * 2 * 5];

// SoundTouch's INTEGER system is broken these days, so we'll need this to do float conversions...
static __aligned16 float s_floatBuffer[NS_TOTAL_SIZE * 2 * 5];

static __forceinline u32 GetNewSamples(s32 nReadBuf)
{
	u32 NewSamples = s_pAudioBuffers[nReadBuf].avgtime / 1000;
	s32 bytesbuf = SoundGetBytesBuffered() * 10;

	if (bytesbuf < MaxBuffer)
	{
		NewSamples += 1;
	}
	else if (bytesbuf > MaxBuffer * 5)
	{
		// check the current timestamp, if too far apart, speed up audio
		//WARN_LOG("making faster %d\n", timeGetTime() - s_pAudioBuffers[nReadBuf].timestamp);
		NewSamples = NewSamples - ((bytesbuf - 400000) / 100000);
	}

	if (s_nDropPacket > 0)
	{
		s_nDropPacket--;
		NewSamples -= 1;
	}

	return min(NewSamples * NSSIZE, NS_TOTAL_SIZE * 3);
}

static __forceinline void ExtractSamples()
{
	// extract 2*NSFRAMES ms at a time
	s32 nOutSamples;

	do
	{
		nOutSamples = pSoundTouch->receiveSamples(s_floatBuffer, NS_TOTAL_SIZE * 5);
		if ( nOutSamples > 0 )
		{
			for( s32 sx=0; sx<nOutSamples*2; sx++ )
				s_ThreadBuffer[sx] = (s16)(s_floatBuffer[sx]*65536.0f);

			SoundFeedVoiceData((u8*)s_ThreadBuffer, nOutSamples * 4);
		}
	} while (nOutSamples != 0);
}

// communicates with the audio hardware
#ifdef _WIN32
DWORD WINAPI SPU2ThreadProc(LPVOID)
#else
void* SPU2ThreadProc(void* lpParam)
#endif
{
	s32 nReadBuf = 0;

	while (!s_bThreadExit)
	{

		if (!(conf.options&OPTION_REALTIME))
		{
			while(s_nQueuedBuffers< 3 && !s_bThreadExit)
			{
				//Sleeping
				Sleep(1);
				if (s_bThreadExit) return NULL;
			}

			while( SoundGetBytesBuffered() > 72000 )
			{
				//Bytes buffered
				Sleep(1);

				if (s_bThreadExit) return NULL;
			}
		}
		else
		{
			while(s_nQueuedBuffers< 1 && !s_bThreadExit)
			{
				//Sleeping
				Sleep(1);
			}
		}

		if (conf.options & OPTION_TIMESTRETCH)
		{
			u32 NewSamples = GetNewSamples(nReadBuf);
			s32 OldSamples = s_pAudioBuffers[nReadBuf].len / 4;

			if ((nReadBuf & 3) == 0)  // wow, this if statement makes the whole difference
			{
				//SPU2_LOG("OldSamples = %d; NewSamples = %d/n", OldSamples, NewSamples);
				float percent = ((float)OldSamples/(float)NewSamples - 1.0f) * 100.0f;
				pSoundTouch->setTempoChange(percent);
			}

			for( s32 sx = 0; sx < OldSamples * 2; sx++ )
				s_floatBuffer[sx] = ((s16*)s_pAudioBuffers[nReadBuf].pbuf)[sx]/65536.0f;

			pSoundTouch->putSamples(s_floatBuffer, OldSamples);
			ExtractSamples();
		}
		else
		{
			SoundFeedVoiceData(s_pAudioBuffers[nReadBuf].pbuf, s_pAudioBuffers[nReadBuf].len);
		}

		// don't go to the next buffer unless there is more data buffered
		nReadBuf = (nReadBuf+1)%ArraySize(s_pAudioBuffers);
		InterlockedExchangeAdd((long*)&s_nQueuedBuffers, -1);

		if ( s_bThreadExit ) break;
	}

	return NULL;
}

// turn channels on
void SoundOn(s32 start,s32 end,u16 val)	 // SOUND ON PSX COMAND
{
	for (s32 ch=start;ch<end;ch++,val>>=1)					 // loop channels
	{
		if ((val&1) && voices[ch].pStart)					// mmm... start has to be set before key on !?!
		{
			voices[ch].bNew=true;
			voices[ch].bIgnoreLoop = false;
			dwNewChannel2[ch/24]|=(1<<(ch%24));				  // clear end channel bit
		}
	}
}

// turn channels off
void SoundOff(s32 start,s32 end,u16 val)	// SOUND OFF PSX COMMAND
{
	for (s32 ch=start;ch<end;ch++,val>>=1)  // loop channels
	{
		if (val&1) voices[ch].bStop=true; // && s_chan[i].bOn)  mmm...
	}
}

void FModOn(s32 start,s32 end,u16 val)	  // FMOD ON PSX COMMAND
{
	s32 ch;

	for (ch=start;ch<end;ch++,val>>=1) // loop channels
	{
		if (val&1)
		{										  // -> fmod on/off
			if (ch>0)
			{
				voices[ch].bFMod=1;							 // --> sound channel
				voices[ch-1].bFMod=2;						   // --> freq channel
			}
		}
		else
			voices[ch].bFMod=0;							   // --> turn off fmod
	}
}

void VolumeOn(s32 start,s32 end,u16 val,s32 iRight)  // VOLUME ON PSX COMMAND
{
	s32 ch;

	for (ch=start;ch<end;ch++,val>>=1) // loop channels
	{
		if (val&1)
		{										   // -> reverb on/off
			if (iRight)
				voices[ch].bVolumeR = true;
			else
				voices[ch].bVolumeL = true;
		}
		else
		{
			if (iRight)
				voices[ch].bVolumeR = false;
			else
				voices[ch].bVolumeL = false;
		}
	}
}

void CALLBACK SPU2write(u32 mem, u16 value)
{
	LOG_CALLBACK("SPU2write()\n");
	u32 spuaddr;
	SPU2_LOG("SPU2 write mem %x value %x\n", mem, value);

	assert(C0_SPUADDR() < 0x100000);
	assert(C1_SPUADDR() < 0x100000);

	spu2Ru16(mem) = value;
	u32 r = mem & 0xffff;

	// channel info
	if ((r<0x0180) || (r>=0x0400 && r<0x0580))  //  u32s are always >= 0.
	{
		s32 ch=0;
		if (r >= 0x400)
			ch = ((r - 0x400) >> 4) + 24;
		else
			ch = (r >> 4);

		VOICE_PROCESSED* pvoice = &voices[ch];

		switch(r & 0x0f)
		{
			case 0:
			case 2:
				pvoice->SetVolume(mem & 0x2);
				break;
			case 4:
			{
				s32 NP;
				if (value> 0x3fff)
					NP=0x3fff;							 // get pitch val
				else
					NP=value;

				pvoice->pvoice->pitch = NP;

				NP = (SAMPLE_RATE * NP) / 4096L;								 // calc frequency
				if (NP<1) NP = 1;										// some security
				pvoice->iActFreq = NP;							   // store frequency
				break;
			}
			case 6:
			{
				pvoice->ADSRX.AttackModeExp=(value&0x8000)?1:0;
				pvoice->ADSRX.AttackRate = ((value>>8) & 0x007f);
				pvoice->ADSRX.DecayRate = (((value>>4) & 0x000f));
				pvoice->ADSRX.SustainLevel = (value & 0x000f);
				break;
			}
			case 8:
				pvoice->ADSRX.SustainModeExp = (value&0x8000)?1:0;
				pvoice->ADSRX.SustainIncrease= (value&0x4000)?0:1;
				pvoice->ADSRX.SustainRate = ((value>>6) & 0x007f);
				pvoice->ADSRX.ReleaseModeExp = (value&0x0020)?1:0;
				pvoice->ADSRX.ReleaseRate = ((value & 0x001f));
				break;
		}

		return;
	}

	// more channel info
	if ((r>=0x01c0 && r<0x02E0)||(r>=0x05c0 && r<0x06E0))
	{
		s32 ch=0;
		u32 rx=r;
		if (rx>=0x400)
		{
			ch=24;
			rx-=0x400;
		}

		ch += ((rx-0x1c0)/12);
		rx -= (ch%24)*12;
		VOICE_PROCESSED* pvoice = &voices[ch];

		switch(rx)
		{
			case REG_VA_SSA:
				pvoice->iStartAddr=(((u32)value&0x3f)<<16)|(pvoice->iStartAddr&0xFFFF);
				pvoice->pStart=(u8*)(spu2mem+pvoice->iStartAddr);
				break;
			case REG_VA_SSA + 2:
				pvoice->iStartAddr=(pvoice->iStartAddr & 0x3f0000) | (value & 0xFFFF);
				pvoice->pStart=(u8*)(spu2mem+pvoice->iStartAddr);
				break;
			case REG_VA_LSAX:
				pvoice->iLoopAddr =(((u32)value&0x3f)<<16)|(pvoice->iLoopAddr&0xFFFF);
				pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
				pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
				break;
			case REG_VA_LSAX + 2:
				pvoice->iLoopAddr=(pvoice->iLoopAddr& 0x3f0000) | (value & 0xFFFF);
				pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
				pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
				break;
			case REG_VA_NAX:
				// unused... check if it gets written as well
				pvoice->iNextAddr=(((u32)value&0x3f)<<16)|(pvoice->iNextAddr&0xFFFF);
				break;
			case REG_VA_NAX + 2:
				// unused... check if it gets written as well
				pvoice->iNextAddr=(pvoice->iNextAddr & 0x3f0000) | (value & 0xFFFF);
				break;
		}

		return;
	}

	// process non-channel data
	switch(mem & 0xffff)
	{
		case REG_C0_SPUDATA:
			spuaddr = C0_SPUADDR();
			spu2mem[spuaddr] = value;
			spuaddr++;

			if (spu2attr0.irq && (C0_IRQA() == spuaddr))
			{
				IRQINFO |= 4;
				SPU2_LOG("SPU2write:C0_CPUDATA interrupt\n");
				irqCallbackSPU2();
			}

			if (spuaddr>0xFFFFE) spuaddr = 0x2800;

			C0_SPUADDR_SET(spuaddr);
			spu2stat_clear_80(0);
			spu2attr1.irq = 0;
			break;

		case REG_C1_SPUDATA:
			spuaddr = C1_SPUADDR();
			spu2mem[spuaddr] = value;
			spuaddr++;

			if (spu2attr1.irq && (C1_IRQA() == spuaddr))
			{
				IRQINFO |= 8;
				SPU2_LOG("SPU2write:C1_CPUDATA interrupt\n");
				irqCallbackSPU2();
			}

			if (spuaddr>0xFFFFE) spuaddr = 0x2800;

			C1_SPUADDR_SET(spuaddr);
			spu2stat_clear_80(1);
			spu2attr0.irq = 0;
			break;

		case REG_C0_IRQA_HI:
		case REG_C0_IRQA_LO:
			pSpuIrq[0] = spu2mem + C0_IRQA();
			break;

		case REG_C1_IRQA_HI:
		case REG_C1_IRQA_LO:
			pSpuIrq[1] = spu2mem + C1_IRQA();
			break;

		case REG_C0_SPUADDR_HI:
		case REG_C1_SPUADDR_HI:
			spu2Ru16(mem) = value&0xf;
			break;

		case REG_C0_CTRL:
			spu2Ru16(mem) = value;
			// clear interrupt
			if (!(value & 0x40)) IRQINFO &= ~0x4;
			break;

		case REG_C1_CTRL:
			spu2Ru16(mem) = value;
			// clear interrupt
			if (!(value & 0x40)) IRQINFO &= ~0x8;
			break;

		// Could probably simplify
		case REG_C0_SPUON1: SoundOn(0,16,value); break;
		case REG_C0_SPUON2: SoundOn(16,24,value); break;
		case REG_C1_SPUON1: SoundOn(24,40,value); break;
		case REG_C1_SPUON2: SoundOn(40,48,value); break;
		case REG_C0_SPUOFF1: SoundOff(0,16,value); break;
		case REG_C0_SPUOFF2: SoundOff(16,24,value); break;
		case REG_C1_SPUOFF1: SoundOff(24,40,value); break;
		case REG_C1_SPUOFF2: SoundOff(40,48,value); break;

		// According to manual all bits are cleared by writing an arbitary value
		case REG_C0_END1: dwEndChannel2[0] &= 0x00ff0000; break;
		case REG_C0_END2: dwEndChannel2[0] &= 0x0000ffff; break;
		case REG_C1_END1: dwEndChannel2[1] &= 0x00ff0000; break;
		case REG_C1_END2: dwEndChannel2[1] &= 0x0000ffff; break;
		case REG_C0_FMOD1: FModOn(0,16,value); break;
		case REG_C0_FMOD2: FModOn(16,24,value); break;
		case REG_C1_FMOD1: FModOn(24,40,value); break;
		case REG_C1_FMOD2: FModOn(40,48,value); break;
		case REG_C0_VMIXL1: VolumeOn(0,16,value,0); break;
		case REG_C0_VMIXL2: VolumeOn(16,24,value,0); break;
		case REG_C1_VMIXL1: VolumeOn(24,40,value,0); break;
		case REG_C1_VMIXL2: VolumeOn(40,48,value,0); break;
		case REG_C0_VMIXR1: VolumeOn(0,16,value,1); break;
		case REG_C0_VMIXR2: VolumeOn(16,24,value,1); break;
		case REG_C1_VMIXR1: VolumeOn(24,40,value,1); break;
		case REG_C1_VMIXR2: VolumeOn(40,48,value,1); break;
	}

	assert( C0_SPUADDR() < 0x100000);
	assert( C1_SPUADDR() < 0x100000);
}

u16  CALLBACK SPU2read(u32 mem)
{
	LOG_CALLBACK("SPU2read()\n");
	u32 spuaddr;
	u16 ret = 0;
	u32 r = mem & 0xffff; // register

	// channel info
	// if the register is any of the regs before core 0, or is somewhere between core 0 and 1...
	if ((r < 0x0180) || (r >= 0x0400 && r < 0x0580))  //  u32s are always >= 0.
	{
		s32 ch = 0;

		if (r >= 0x400)
			ch=((r - 0x400) >> 4) + 24;
		else
			ch = (r >> 4);

		VOICE_PROCESSED* pvoice = &voices[ch];

		if ((r&0x0f) == 10) return (u16)(pvoice->ADSRX.EnvelopeVol >> 16);
	}


	if ((r>=REG_VA_SSA && r<REG_A_ESA) || (r>=0x05c0 && r<0x06E0))  // some channel info?
	{
		s32 ch = 0;
		u32 rx = r;

		if (rx >= 0x400)
		{
			ch = 24;
			rx -= 0x400;
		}

		ch += ((rx - 0x1c0) / 12);
		rx -= (ch % 24) * 12;
		VOICE_PROCESSED* pvoice = &voices[ch];

		switch(rx)
		{
			case REG_VA_SSA:
				ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>17)&0x3F);
				break;
			case REG_VA_SSA + 2:
				ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>1)&0xFFFF);
				break;
			case REG_VA_LSAX:
				ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>17)&0x3F);
				break;
			case REG_VA_LSAX + 2:
				ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>1)&0xFFFF);
				break;
			case REG_VA_NAX:
				ret = ((((uptr)pvoice->pCurr-(uptr)spu2mem)>>17)&0x3F);
				break;
			case REG_VA_NAX + 2:
				ret = ((((uptr)pvoice->pCurr-(uptr)spu2mem)>>1)&0xFFFF);
				break;
		}

		SPU2_LOG("SPU2 channel read mem %x: %x\n", mem, ret);
		return ret;
	}

	switch(mem & 0xffff)
	{
		case REG_C0_SPUDATA:
			spuaddr = C0_SPUADDR();
			ret =spu2mem[spuaddr];
			spuaddr++;
			if (spuaddr > 0xfffff) spuaddr=0;
			C0_SPUADDR_SET(spuaddr);
			break;

		case REG_C1_SPUDATA:
			spuaddr = C1_SPUADDR();
			ret = spu2mem[spuaddr];
			spuaddr++;
			if (spuaddr > 0xfffff) spuaddr=0;
			C1_SPUADDR_SET(spuaddr);
			break;

		case REG_C0_END1: ret = (dwEndChannel2[0]&0xffff); break;
		case REG_C1_END1: ret = (dwEndChannel2[1]&0xffff); break;
		case REG_C0_END2: ret = (dwEndChannel2[0]>>16); break;
		case REG_C1_END2: ret = (dwEndChannel2[1]>>16); break;

		case REG_IRQINFO:
			ret = IRQINFO;
			break;

		default:
			ret = spu2Ru16(mem);
	}

	SPU2_LOG("SPU2 read mem %x: %x\n", mem, ret);

	return ret;
}

void CALLBACK SPU2WriteMemAddr(int core, u32 value)
{
	LOG_CALLBACK("SPU2WriteMemAddr(%d, %d)\n", core, value);
	MemAddr[core] = g_pDMABaseAddr + value;
}

u32 CALLBACK SPU2ReadMemAddr(int core)
{
	LOG_CALLBACK("SPU2ReadMemAddr(%d)\n", core);
	return MemAddr[core] - g_pDMABaseAddr;
}

void CALLBACK SPU2setDMABaseAddr(uptr baseaddr)
{
	LOG_CALLBACK("SPU2setDMABaseAddr()\n");
	g_pDMABaseAddr = baseaddr;
}

#ifdef ENABLE_NEW_IOPDMA_SPU2
void CALLBACK SPU2irqCallback(void (*SPU2callback)())
{
	LOG_CALLBACK("SPU2irqCallback()\n");
	irqCallbackSPU2 = SPU2callback;
}
#else
void CALLBACK SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)())
{
	LOG_CALLBACK("SPU2irqCallback()\n");
	irqCallbackSPU2 = SPU2callback;
	irqCallbackDMA4 = DMA4callback;
	irqCallbackDMA7 = DMA7callback;
}
#endif

s32 CALLBACK SPU2test()
{
	LOG_CALLBACK("SPU2test()\n");
	return 0;
}

int CALLBACK SPU2setupRecording(int start, void* pData)
{
	LOG_CALLBACK("SPU2setupRecording()\n");
	if ( start )
	{
		conf.options |= OPTION_RECORDING;
		WARN_LOG("ZeroSPU2: started recording at %s\n", RECORD_FILENAME);
	}
	else
	{
		conf.options &= ~OPTION_RECORDING;
		WARN_LOG("ZeroSPU2: stopped recording\n");
	}

	return 1;
}

void save_data(freezeData *data)
{
	SPU2freezeData *spud;

	spud = (SPU2freezeData*)data->data;
	spud->version = 0x70000002;

	memcpy(spud->spu2regs, spu2regs, 0x10000);
	memcpy(spud->spu2mem, spu2mem, 0x200000);

	spud->nSpuIrq[0] = (s32)(pSpuIrq[0] - spu2mem);
	spud->nSpuIrq[1] = (s32)(pSpuIrq[1] - spu2mem);

	memcpy(spud->dwNewChannel2, dwNewChannel2, sizeof(dwNewChannel2));
	memcpy(spud->dwEndChannel2, dwEndChannel2, sizeof(dwEndChannel2));

	spud->dwNoiseVal = dwNoiseVal;
	spud->interrupt = interrupt;

	memcpy(spud->iFMod, iFMod, sizeof(iFMod));
	memcpy(spud->MemAddr, MemAddr, sizeof(MemAddr));

	spud->adma[0] = adma[0];
	spud->adma[1] = adma[1];
	spud->AdmaMemAddr[0] = (u32)((uptr)adma[0].MemAddr - g_pDMABaseAddr);
	spud->AdmaMemAddr[1] = (u32)((uptr)adma[1].MemAddr - g_pDMABaseAddr);

	spud->SPUCycles = SPUCycles;
	spud->SPUWorkerCycles = SPUWorkerCycles;

	memcpy(spud->SPUStartCycle, SPUStartCycle, sizeof(SPUStartCycle));
	memcpy(spud->SPUTargetCycle, SPUTargetCycle, sizeof(SPUTargetCycle));

	for (u32 i = 0; i < ArraySize(s_nDurations); ++i)
	{
		s_nDurations[i] = NSFRAMES*1000;
	}

	s_nTotalDuration = ArraySize(s_nDurations)*NSFRAMES*1000;
	s_nCurDuration = 0;

	spud->voicesize = SPU_VOICE_STATE_SIZE;
	for (u32 i = 0; i < ArraySize(voices); ++i)
	{
		memcpy(&spud->voices[i], &voices[i], SPU_VOICE_STATE_SIZE);
		spud->voices[i].pStart = (u8*)((uptr)voices[i].pStart-(uptr)spu2mem);
		spud->voices[i].pLoop = (u8*)((uptr)voices[i].pLoop-(uptr)spu2mem);
		spud->voices[i].pCurr = (u8*)((uptr)voices[i].pCurr-(uptr)spu2mem);
	}

	g_startcount=0xffffffff;
	s_GlobalTimeStamp = 0;
	s_nDropPacket = 0;
}

void load_data(freezeData *data)
{
	SPU2freezeData *spud;

	spud = (SPU2freezeData*)data->data;

	if (spud->version != 0x70000002)
	{
		ERROR_LOG("zerospu2: Sound data either corrupted or from another plugin. Ignoring.\n");
		return;
	}

	memcpy(spu2regs, spud->spu2regs, 0x10000);
	memcpy(spu2mem, spud->spu2mem, 0x200000);

	pSpuIrq[0] = spu2mem + spud->nSpuIrq[0];
	pSpuIrq[1] = spu2mem + spud->nSpuIrq[1];

	memcpy(dwNewChannel2, spud->dwNewChannel2, sizeof(dwNewChannel2));
	memcpy(dwEndChannel2, spud->dwEndChannel2, sizeof(dwEndChannel2));

	dwNoiseVal = spud->dwNoiseVal;
	interrupt = spud->interrupt;

	memcpy(iFMod, spud->iFMod, sizeof(iFMod));
	memcpy(MemAddr, spud->MemAddr, sizeof(MemAddr));

	adma[0] = spud->adma[0];
	adma[1] = spud->adma[1];
	adma[0].MemAddr = (u16*)(g_pDMABaseAddr+spud->AdmaMemAddr[0]);
	adma[1].MemAddr = (u16*)(g_pDMABaseAddr+spud->AdmaMemAddr[1]);

	SPUCycles = spud->SPUCycles;
	SPUWorkerCycles = spud->SPUWorkerCycles;

	memcpy(SPUStartCycle, spud->SPUStartCycle, sizeof(SPUStartCycle));
	memcpy(SPUTargetCycle, spud->SPUTargetCycle, sizeof(SPUTargetCycle));

	for (u32 i = 0; i < ArraySize(voices); ++i)
	{
		memcpy(&voices[i], &spud->voices[i], min((s32)SPU_VOICE_STATE_SIZE, spud->voicesize));
		voices[i].pStart = (u8*)((uptr)spud->voices[i].pStart+(uptr)spu2mem);
		voices[i].pLoop = (u8*)((uptr)spud->voices[i].pLoop+(uptr)spu2mem);
		voices[i].pCurr = (u8*)((uptr)spud->voices[i].pCurr+(uptr)spu2mem);
	}

	s_GlobalTimeStamp = 0;
	g_startcount = 0xffffffff;

	for (u32 i = 0; i < ArraySize(s_nDurations); ++i)
	{
		s_nDurations[i] = NSFRAMES*1000;
	}

	s_nTotalDuration = ArraySize(s_nDurations)*NSFRAMES*1000;
	s_nCurDuration = 0;
	s_nQueuedBuffers = 0;
	s_nDropPacket = 0;
}

s32  CALLBACK SPU2freeze(int mode, freezeData *data)
{
	LOG_CALLBACK("SPU2freeze()\n");
	assert( g_pDMABaseAddr != 0 );

	switch (mode)
	{
		case FREEZE_LOAD:
			load_data(data);
			break;
		case FREEZE_SAVE:
			save_data(data);
			break;
		case FREEZE_SIZE:
			data->size = sizeof(SPU2freezeData);
			break;
		default:
			break;
	}

	return 0;
}
