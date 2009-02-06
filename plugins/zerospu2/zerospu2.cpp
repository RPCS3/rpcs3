/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
#include "svnrev.h"
#endif 

#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

char libraryName[256];

FILE *spu2Log;
Config conf;

ADMA Adma4;
ADMA Adma7;

u32 MemAddr[2];
u32 g_nSpuInit = 0;
u16 interrupt = 0;
s8 *spu2regs = NULL;
u16* spu2mem = NULL;
u16* pSpuIrq[2] = {NULL};
u32 dwNewChannel2[2] = {0}; // keeps track of what channels that have been turned on
u32 dwEndChannel2[2] = {0}; // keeps track of what channels have ended
unsigned long   dwNoiseVal=1;						  // global noise generator
bool g_bPlaySound = true; // if true, will output sound, otherwise no
int iFMod[NSSIZE];
int s_buffers[NSSIZE][2]; // left and right buffers

// mixer thread variables
static bool s_bThreadExit = true;
static int s_nDropPacket = 0;
string s_strIniPath="inis/zerospu2.ini";

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

static AUDIOBUFFER s_pAudioBuffers[NSPACKETS];
static int s_nCurBuffer = 0, s_nQueuedBuffers = 0;
static s16* s_pCurOutput = NULL;
static u32 g_startcount=0xffffffff;
static u32 g_packetcount=0;

// time stretch variables
soundtouch::SoundTouch* pSoundTouch=NULL;
WavOutFile* g_pWavRecord=NULL; // used for recording

static u64 s_GlobalTimeStamp = 0;
static int s_nDurations[64]={0};
static int s_nCurDuration=0;
static int s_nTotalDuration=0;

int SPUCycles = 0, SPUWorkerCycles = 0;
int SPUStartCycle[2];
int SPUTargetCycle[2];

int g_logsound=0;

int ADMASWrite(int c);

void InitADSR();

// functions of main emu, called on spu irq
void (*irqCallbackSPU2)()=0;
void (*irqCallbackDMA4)()=0;
void (*irqCallbackDMA7)()=0;

uptr g_pDMABaseAddr=0;

const int f[5][2] = {   
				{    0,     0 },
				{  60,     0 },
				{ 115, -52 },
				{   98, -55 },
				{ 122, -60 } };

u32 RateTable[160];

// channels and voices
VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1]; // +1 for modulation

static void InitLibraryName()
{
#ifdef _WIN32
#ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "ZeroSPU2 Playground" );

#elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "ZeroSPU2 Playground"
#	ifdef _DEBUG
		"-Debug"
#	endif
		);
#else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s( libraryName, "ZeroSPU2 PG r%d%s"
#	ifdef _DEBUG
		"-Debug"
#	else
		"-Dev"
#	endif
		,SVN_REV,
		SVN_MODS ? "m" : ""
	);
#endif
#else
// I'll hook in svn version code later. --arcum42

	strcpy( libraryName, "ZeroSPU2 Playground"
#	ifdef _DEBUG
		"-Debug"
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

s32 CALLBACK SPU2init()
{
	LOG_CALLBACK("SPU2init()\n");
	spu2Log = fopen("logs/spu2.txt", "w");
	if (spu2Log) setvbuf(spu2Log, NULL,  _IONBF, 0);
	
	SPU2_LOG("Spu2 null version %d,%d\n",SPU2_REVISION,SPU2_BUILD);  
	SPU2_LOG("SPU2init\n");
	
#ifdef _WIN32
	QueryPerformanceFrequency(&g_counterfreq);
#else
	char strcurdir[256];
	getcwd(strcurdir, 256);
	s_strIniPath = strcurdir;
	s_strIniPath += "/inis/zerospu2.ini";
#endif

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
	for (int i = 0; i < 24; ++i)
		voices[i+24].memoffset = 0x400;

	// init each channel
	for (u32 i = 0; i < ARRAYSIZE(voices); ++i) {
		voices[i].chanid = i;
		voices[i].pLoop = voices[i].pStart = voices[i].pCurr = (u8*)spu2mem;

		voices[i].pvoice = (_SPU_VOICE*)((u8*)spu2regs+voices[i].memoffset)+(i%24);
		voices[i].ADSRX.SustainLevel = 1024;				// -> init sustain
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
		for (u32 i = 0; i < ARRAYSIZE(s_pAudioBuffers); ++i) 
		{
			s_pAudioBuffers[i].pbuf = (u8*)_aligned_malloc(4*NSSIZE*NSFRAMES, 16); // 4 bytes for each sample
			s_pAudioBuffers[i].len = 0;
		}

		s_nCurBuffer = 0;
		s_nQueuedBuffers = 0;
		s_pCurOutput = (s16*)s_pAudioBuffers[0].pbuf;
		assert( s_pCurOutput != NULL);
	
		for (int i = 0; i < ARRAYSIZE(s_nDurations); ++i) 
		{
			s_nDurations[i] = NSFRAMES*1000;
		}
		s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
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
	
	for (u32 i = 0; i < ARRAYSIZE(s_pAudioBuffers); ++i) 
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
			irqCallbackDMA7();
		}
		
	}

	if (interrupt & (1<<1))
	{
		if (SPUCycles - SPUStartCycle[0] >= SPUTargetCycle[0])
		{
			interrupt &= ~(1<<1);
			irqCallbackDMA4();
		}
	}

	if ( g_nSpuInit ) 
	{
			while( SPUCycles-SPUWorkerCycles > 0 && CYCLES_PER_MS < SPUCycles-SPUWorkerCycles ) 
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
	int i;
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

int MixADSR(VOICE_PROCESSED* pvoice)							 // MIX ADSR
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

void MixChannels(int core)
{
	// mix all channels
	int c_offset = 0x0400 * core;
	int dma;
	ADMA *Adma;
	
	if (core == 0)
	{
		Adma = &Adma4;
		dma = 4;
	}
	else
	{
		Adma = &Adma7;
		dma = 7;
	}
	
	if ((spu2Ru16(REG_C0_MMIX + c_offset) & 0xF0) && (spu2Ru16(REG_C0_ADMAS + c_offset) & (0x1 + core))) 
	{
		for (int ns=0;ns<NSSIZE;ns++) 
		{ 
			if ((spu2Ru16(REG_C0_MMIX + c_offset) & 0x80)) 
				s_buffers[ns][0] += (((short*)spu2mem)[0x2000 + c_offset +Adma->Index]*(int)spu2Ru16(REG_C0_BVOLL + c_offset))>>16;
			if ((spu2Ru16(REG_C0_MMIX + c_offset) & 0x40)) 
				s_buffers[ns][1] += (((short*)spu2mem)[0x2200 + c_offset +Adma->Index]*(int)spu2Ru16(REG_C0_BVOLR + c_offset))>>16;
			
			Adma->Index +=1;
			MemAddr[core] += 4;
			
			if (Adma->Index == 128 || Adma->Index == 384)
			{
				if (ADMASWrite(core))
				{
					if (interrupt & (0x2 * (core + 1)))
					{
						interrupt &= ~(0x2 * (core + 1));
						printf("Stopping double interrupt DMA7\n");
					}
					if (core == 0)
						irqCallbackDMA4();
					else
						irqCallbackDMA7();
					
				}
				if (core == 1) Adma->Enabled = 2;
			}

			if (Adma->Index == 512) 
			{
				if ( Adma->Enabled == 2 ) Adma->Enabled = 0;
				Adma->Index = 0;
			}
		}
	}
}

// simulate SPU2 for 1ms
void SPU2Worker()
{
	int s_1,s_2,fa;
	u8* start;
	u32 nSample;
	int ch,predict_nr,shift_factor,flags,d,s;

	// assume s_buffers are zeroed out
	if ( dwNewChannel2[0] || dwNewChannel2[1] )
		s_pAudioBuffers[s_nCurBuffer].newchannels++;
		
	VOICE_PROCESSED* pChannel=voices;
	for (ch=0;ch<SPU_NUMBER_VOICES;ch++,pChannel++)			  // loop em all... we will collect 1 ms of sound of each playing channel
	{
		if (pChannel->bNew) 
		{
			pChannel->StartSound();						 // start new sound
			dwEndChannel2[ch/24]&=~(1<<(ch%24));				  // clear end channel bit
			dwNewChannel2[ch/24]&=~(1<<(ch%24));				  // clear channel bit
		}

		if (!pChannel->bOn) continue;

		if (pChannel->iActFreq!=pChannel->iUsedFreq)	 // new psx frequency?
			pChannel->VoiceChangeFrequency();

		// loop until 1 ms of data is reached
		int ns = 0;
		
		while(ns<NSSIZE)
		{
			if (pChannel->bFMod==1 && iFMod[ns])		   // fmod freq channel
				pChannel->FModChangeFrequency(ns);

			while(pChannel->spos >= 0x10000 )
			{
				if (pChannel->iSBPos == 28)			// 28 reached?
				{
					start=pChannel->pCurr;		  // set up the current pos

					// special "stop" sign - fixme - an *unsigned* -1?
					if (start == (u8*)-1)  //!pChannel->bOn 
					{
						pChannel->bOn=false;						// -> turn everything off
						pChannel->ADSRX.lVolume=0;
						pChannel->ADSRX.EnvelopeVol=0;
						goto ENDX;							  // -> and done for this channel
					}

					pChannel->iSBPos=0;

					// decode the 16byte packet
					s_1=pChannel->s_1;
					s_2=pChannel->s_2;

					predict_nr=(s32)start[0];
					shift_factor=predict_nr&0xf;
					predict_nr >>= 4;
					flags=(s32)start[1];
					start += 2;

					for (nSample=0;nSample<28; ++start)
					{
						d = (int)*start;
						s = ((d & 0xf)<<12);
						if (s & 0x8000) s |= 0xffff0000;

						fa = (s >> shift_factor);
						fa += ((s_1 * f[predict_nr][0]) >> 6) + ((s_2 * f[predict_nr][1]) >> 6);
						s_2 = s_1;
						s_1 = fa;
						s = ((d & 0xf0) << 8);

						pChannel->SB[nSample++]=fa;

						if (s & 0x8000) s|=0xffff0000;
						fa = (s>>shift_factor);			  
						fa += ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1]) >> 6);
						s_2 = s_1;
						s_1 = fa;

						pChannel->SB[nSample++]=fa;
					}
			
					// irq occurs no matter what core access the address
					for (int core = 0; core < 2; ++core) 
					{
						if (((SPU_CONTROL_*)(spu2regs + 0x400 * core + REG_C0_CTRL))->irq)		 // some callback and irq active?
						{
							// if irq address reached or irq on looping addr, when stop/loop flag is set 
							u8* pirq = (u8*)pSpuIrq[core];
							if ((pirq > (start - 16)  && pirq <= start) || ((flags & 1) && (pirq > (pChannel->pLoop - 16) && pirq <= pChannel->pLoop)))
							{
								IRQINFO |= 4<<core;
								SPU2_LOG("SPU2Worker:interrupt\n");
								irqCallbackSPU2();
							}
						}
					}

					// flag handler
					if ((flags&4) && (!pChannel->bIgnoreLoop))
						pChannel->pLoop=start-16;				// loop adress

					if (flags&1)							   // 1: stop/loop
					{
						// We play this block out first...
						dwEndChannel2[ch/24]|=(1<<(ch%24));
				
						if (flags!=3 || pChannel->pLoop==NULL)
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

					pChannel->pCurr=start;					// store values for next cycle
					pChannel->s_1=s_1;
					pChannel->s_2=s_2;
				}

				fa=pChannel->SB[pChannel->iSBPos++];		// get sample data
				pChannel->StoreInterpolationVal(fa);
				pChannel->spos -= 0x10000;
			}

			if (pChannel->bNoise)
				fa=pChannel->iGetNoiseVal();			   // get noise val
			else
				fa=pChannel->iGetInterpolationVal();	   // get sample val

			int sval = (MixADSR(pChannel) * fa) / 1023;   // mix adsr

			if (pChannel->bFMod == 2)						// fmod freq channel
			{
				iFMod[ns] = sval;					// -> store 1T sample data, use that to do fmod on next channel
			}
			else 
			{
				if (pChannel->bVolumeL)
					s_buffers[ns][0]+=(sval * pChannel->leftvol)>>14;
				
				if (pChannel->bVolumeR)
					s_buffers[ns][1]+=(sval * pChannel->rightvol)>>14;
			}

			// go to the next packet
			ns++;
			pChannel->spos += pChannel->sinc;
		}
ENDX:
		;
	}

	// mix all channels
	/*MixChannels(0);
	MixChannels(1);*/
	
	
        // mix all channels
	// This code is temporarily being put back, till I work out why the replacement is breaking things, --arcum42
        if ((spu2Ru16(REG_C0_MMIX) & 0xF0) && (spu2Ru16(REG_C0_ADMAS) & 0x1) /*&& !(spu2Ru16(REG_C0_CTRL) & 0x30)*/) 
        {
                ADMA *Adma = &Adma4;
                
                for (int ns=0;ns<NSSIZE;ns++) 
                { 
                
                        if ((spu2Ru16(REG_C0_MMIX) & 0x80)) 
                                s_buffers[ns][0] += (((short*)spu2mem)[0x2000+Adma->Index]*(int)spu2Ru16(REG_C0_BVOLL))>>16;
                        if ((spu2Ru16(REG_C0_MMIX) & 0x40)) 
                                s_buffers[ns][1] += (((short*)spu2mem)[0x2200+Adma->Index]*(int)spu2Ru16(REG_C0_BVOLR))>>16;

                        Adma->Index +=1;
                        // just add after every sample, it is better than adding 1024 all at once (games like Genji don't like it)
                        MemAddr[0] += 4;

                        if ((Adma->Index == 128) || (Adma->Index == 384))
                        {
                                if (ADMASWrite(0))
                                {
                                        if (interrupt & 0x2)
                                        {
                                                interrupt &= ~0x2;
                                                printf("Stopping double interrupt DMA4\n");
                                        }
                                        irqCallbackDMA4();
                                        
                                }
                        }
                        
                        if (Adma->Index == 512) 
                        {
                                if ( Adma->Enabled == 2 ) 
                                {
                                        Adma->Enabled = 0;
                                }
                                Adma->Index = 0;
                        }
                }
        }

        // Let's do the same bloody mixing code again, only for C1. 
        // fixme - There is way too much duplication of code between C0 & C1, and Adma4 & Adma7.
        // arcum42
        if ((spu2Ru16(REG_C1_MMIX) & 0xF0) && (spu2Ru16(REG_C1_ADMAS) & 0x2)) 
        {
                ADMA *Adma = &Adma7;

                for (int ns=0;ns<NSSIZE;ns++) 
                { 
                        if ((spu2Ru16(REG_C1_MMIX) & 0x80)) 
                                s_buffers[ns][0] += (((short*)spu2mem)[0x2400+Adma->Index]*(int)spu2Ru16(REG_C1_BVOLL))>>16;
                        if ((spu2Ru16(REG_C1_MMIX) & 0x40)) 
                                s_buffers[ns][1] += (((short*)spu2mem)[0x2600+Adma->Index]*(int)spu2Ru16(REG_C1_BVOLR))>>16;
                        
                        Adma->Index +=1;
                        MemAddr[1] += 4;
                        
                        if (Adma->Index == 128 || Adma->Index == 384)
                        {
                                if (ADMASWrite(1))
                                {
                                        if (interrupt & 0x4)
                                        {
                                                interrupt &= ~0x4;
                                                printf("Stopping double interrupt DMA7\n");
                                        }
                                        irqCallbackDMA7();
                                        
                                }
                                Adma->Enabled = 2;
                        }

                        if (Adma->Index == 512) 
                        {
                                if ( Adma->Enabled == 2 ) Adma->Enabled = 0;
                                Adma->Index = 0;
                        }
                }
        }

	if ( g_bPlaySound ) 
	{
		assert( s_pCurOutput != NULL);

		for (int ns=0; ns<NSSIZE; ns++) 
		{
			// clamp and write
			clampandwrite16(s_pCurOutput[0],s_buffers[ns][0]);
			clampandwrite16(s_pCurOutput[1],s_buffers[ns][1]);
			
			s_pCurOutput += 2;
			s_buffers[ns][0] = 0;
			s_buffers[ns][1] = 0;
		}
		// check if end reached

		if ((uptr)s_pCurOutput - (uptr)s_pAudioBuffers[s_nCurBuffer].pbuf >= 4 * NSSIZE * NSFRAMES) 
		{

			if ( conf.options & OPTION_RECORDING ) 
			{
				static int lastrectime = 0;
				if (timeGetTime() - lastrectime > 5000) 
				{
					printf("ZeroSPU2: recording\n");
					lastrectime = timeGetTime();
				}
				LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf+2, 4, NSSIZE*NSFRAMES);
			}

			if ( s_nQueuedBuffers >= ARRAYSIZE(s_pAudioBuffers)-1 ) 
			{
				//ZeroSPU2: dropping packets! game too fast
				s_nDropPacket += NSFRAMES;
				s_GlobalTimeStamp = GetMicroTime();
			}
			else {
				// submit to final mixer
#ifdef _DEBUG
				if ( g_logsound ) 
					LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf+2, 4, NSSIZE*NSFRAMES);
#endif
				if ( g_startcount == 0xffffffff ) 
				{
					g_startcount = timeGetTime();
					g_packetcount = 0;
				}

				if ( conf.options & OPTION_TIMESTRETCH ) 
				{
					u64 newtime = GetMicroTime();
					if ( s_GlobalTimeStamp == 0 )
						s_GlobalTimeStamp = newtime-NSFRAMES*1000;
					u32 newtotal = s_nTotalDuration-s_nDurations[s_nCurDuration];
					u32 duration = (u32)(newtime-s_GlobalTimeStamp);
					s_nDurations[s_nCurDuration] = duration;
					s_nTotalDuration = newtotal + duration;
					s_nCurDuration = (s_nCurDuration+1)%ARRAYSIZE(s_nDurations);
					s_GlobalTimeStamp = newtime;
					s_pAudioBuffers[s_nCurBuffer].timestamp = timeGetTime();
					s_pAudioBuffers[s_nCurBuffer].avgtime = s_nTotalDuration/ARRAYSIZE(s_nDurations);
				}

				s_pAudioBuffers[s_nCurBuffer].len = 4*NSSIZE*NSFRAMES;
				InterlockedExchangeAdd((long*)&s_nQueuedBuffers, 1);

				s_nCurBuffer = (s_nCurBuffer+1)%ARRAYSIZE(s_pAudioBuffers);
				s_pAudioBuffers[s_nCurBuffer].newchannels = 0; // reset
			}

			// restart
			s_pCurOutput = (s16*)s_pAudioBuffers[s_nCurBuffer].pbuf;
		}
	}
}

// resamples pStereoSamples
void ResampleLinear(s16* pStereoSamples, int oldsamples, s16* pNewSamples, int newsamples)
{
	for (int i = 0; i < newsamples; ++i) 
	{
		int io = i * oldsamples;
		int old = io / newsamples;
		int rem = io - old * newsamples;

		old *= 2;
		int newsampL = pStereoSamples[old] * (newsamples - rem) + pStereoSamples[old+2] * rem;
		int newsampR = pStereoSamples[old+1] * (newsamples - rem) + pStereoSamples[old+3] * rem;
		pNewSamples[2 * i] = newsampL / newsamples;
		pNewSamples[2 * i + 1] = newsampR / newsamples;
	}
}

static PCSX2_ALIGNED16(s16 s_ThreadBuffer[NSSIZE*NSFRAMES*2*5]);

// SoundTouch's INTEGER system is broken these days, so we'll need this to do float conversions...
static PCSX2_ALIGNED16(float s_floatBuffer[NSSIZE*NSFRAMES*2*5]);

// communicates with the audio hardware
#ifdef _WIN32
DWORD WINAPI SPU2ThreadProc(LPVOID)
#else
void* SPU2ThreadProc(void* lpParam)
#endif
{
	int nReadBuf = 0;

	while (!s_bThreadExit) 
	{

		if (!(conf.options&OPTION_REALTIME)) 
		{
			while(s_nQueuedBuffers< 3 && !s_bThreadExit) 
			{
				//Sleeping
				Sleep(1);
				if ( s_bThreadExit )
					return NULL;
			}

			while( SoundGetBytesBuffered() > 72000 ) 
			{
				//Bytes buffered
				Sleep(1);

				if ( s_bThreadExit ) return NULL;
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


		//int ps2delay = timeGetTime() - s_pAudioBuffers[nReadBuf].timestamp;
		int NewSamples = s_pAudioBuffers[nReadBuf].avgtime;

		if ( (conf.options & OPTION_TIMESTRETCH) ) 
		{

			int bytesbuf = SoundGetBytesBuffered();
			if ( bytesbuf < 8000 )
				NewSamples += 1000;
			// check the current timestamp, if too far apart, speed up audio
			else if ( bytesbuf > 40000 ) 
			{
				//printf("making faster %d\n", timeGetTime() - s_pAudioBuffers[nReadBuf].timestamp);
				NewSamples -= (bytesbuf-40000)/10;//*(ps2delay-NewSamples*8/1000);
			}

			if ( s_nDropPacket > 0 ) 
			{
				s_nDropPacket--;
				NewSamples -= 1000;
			}

			NewSamples *= NSSIZE;
			NewSamples /= 1000;

			NewSamples = min(NewSamples, NSFRAMES * NSSIZE * 3);

			int oldsamples = s_pAudioBuffers[nReadBuf].len / 4;

			if ((nReadBuf & 3) == 0)  // wow, this if statement makes the whole difference
				pSoundTouch->setTempoChange(100.0f*(float)oldsamples/(float)NewSamples - 100.0f);

			for( int sx=0; sx<oldsamples*2; sx++ )
				s_floatBuffer[sx] = ((s16*)s_pAudioBuffers[nReadBuf].pbuf)[sx]/65536.0f;

			pSoundTouch->putSamples(s_floatBuffer, oldsamples);

			// extract 2*NSFRAMES ms at a time
			int nOutSamples;
			
			do
			{
				nOutSamples = pSoundTouch->receiveSamples(s_floatBuffer, NSSIZE * NSFRAMES * 5);
				if ( nOutSamples > 0 )
				{
					for( int sx=0; sx<nOutSamples*2; sx++ )
						s_ThreadBuffer[sx] = (s16)(s_floatBuffer[sx]*65536.0f);

					SoundFeedVoiceData((u8*)s_ThreadBuffer, nOutSamples * 4);
				}
				
			} while (nOutSamples != 0);

		}
		else
			SoundFeedVoiceData(s_pAudioBuffers[nReadBuf].pbuf, s_pAudioBuffers[nReadBuf].len);

		// don't go to the next buffer unless there is more data buffered
		nReadBuf = (nReadBuf+1)%ARRAYSIZE(s_pAudioBuffers);
		InterlockedExchangeAdd((long*)&s_nQueuedBuffers, -1);

		if ( s_bThreadExit ) break;
	}

	return NULL;
}

// turn channels on
void SoundOn(int start,int end,unsigned short val)	 // SOUND ON PSX COMAND
{
	for (int ch=start;ch<end;ch++,val>>=1)					 // loop channels
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
void SoundOff(int start,int end,unsigned short val)	// SOUND OFF PSX COMMAND
{
	for (int ch=start;ch<end;ch++,val>>=1)  // loop channels
	{					
		if (val&1) voices[ch].bStop=true; // && s_chan[i].bOn)  mmm...				   
	}
}

void FModOn(int start,int end,unsigned short val)	  // FMOD ON PSX COMMAND
{
	int ch;

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

void VolumeOn(int start,int end,unsigned short val,int iRight)  // VOLUME ON PSX COMMAND
{
	int ch;
	
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
		int ch=0;
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
				int NP;
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
		int ch=0;
		unsigned long rx=r;
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
			case 0x1C2:
				pvoice->iStartAddr=(pvoice->iStartAddr & 0x3f0000) | (value & 0xFFFF);
				pvoice->pStart=(u8*)(spu2mem+pvoice->iStartAddr);
				break;
			case REG_VA_LSAX:
				pvoice->iLoopAddr =(((u32)value&0x3f)<<16)|(pvoice->iLoopAddr&0xFFFF);
				pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
				pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
				break;
			case 0x1C6:
				pvoice->iLoopAddr=(pvoice->iLoopAddr& 0x3f0000) | (value & 0xFFFF);
				pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
				pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
				break;
			case REG_VA_NAX:
				// unused... check if it gets written as well
				pvoice->iNextAddr=(((u32)value&0x3f)<<16)|(pvoice->iNextAddr&0xFFFF);
				break;
			case 0x1CA:
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
		
			if ((spu2Ru16(REG_C0_CTRL)&0x40) && (C0_IRQA() == spuaddr))
			{
				IRQINFO |= 4;
				SPU2_LOG("SPU2write:C0_CPUDATA interrupt\n");
				irqCallbackSPU2();
			}
			
			if (spuaddr>0xFFFFE) spuaddr = 0x2800;
			
			C0_SPUADDR_SET(spuaddr);
			spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
			spu2Ru16(REG_C0_CTRL)&=~0x30;
			break;
			
		case REG_C1_SPUDATA:
			spuaddr = C1_SPUADDR();
			spu2mem[spuaddr] = value;
			spuaddr++;
		
			if ((spu2Ru16(REG_C1_CTRL)&0x40) && (C1_IRQA() == spuaddr))
			{
				IRQINFO |= 8;
				SPU2_LOG("SPU2write:C1_CPUDATA interrupt\n");
				irqCallbackSPU2();
			}
			
			if (spuaddr>0xFFFFE) spuaddr = 0x2800;
			
			C1_SPUADDR_SET(spuaddr);
			spu2Ru16(REG_C1_SPUSTAT)&=~0x80;
			spu2Ru16(REG_C1_CTRL)&=~0x30;
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
		int ch = 0;
		
		if (r >= 0x400) 
			ch=((r - 0x400) >> 4) + 24;
		else 
			ch = (r >> 4);

		VOICE_PROCESSED* pvoice = &voices[ch];
				
		if ((r&0x0f) == 10) return (u16)(pvoice->ADSRX.EnvelopeVol >> 16);
	}

	
	if ((r>=REG_VA_SSA && r<REG_A_ESA) || (r>=0x05c0 && r<0x06E0))  // some channel info?
	{
		int ch=0;
		unsigned long rx = r;
		
		if (rx >=0x400)
		{
			ch=24;
			rx-=0x400;
		}

		ch+=((rx-0x1c0)/12);
		rx-=(ch%24)*12;
		VOICE_PROCESSED* pvoice = &voices[ch];

		// Note - can we generalize this?
		switch(rx) 
		{
			case REG_VA_SSA:
				ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>17)&0x3F);
				break;
			case 0x1C2:
				ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>1)&0xFFFF);
				break;
			case REG_VA_LSAX:
				ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>17)&0x3F);
				break;
			case 0x1C6:
				ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>1)&0xFFFF);
				break;
			case REG_VA_NAX:
				ret = ((((uptr)pvoice->pCurr-(uptr)spu2mem)>>17)&0x3F);
				break;
			case 0x1CA:
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

void CALLBACK SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)())
{
	LOG_CALLBACK("SPU2irqCallback()\n");
	irqCallbackSPU2 = SPU2callback;
	irqCallbackDMA4 = DMA4callback;
	irqCallbackDMA7 = DMA7callback;
}

s32 CALLBACK SPU2test()
{
	LOG_CALLBACK("SPU2test()\n");
	return 0;
}

#define SetPacket(s) \
{ \
	if (s & 0x8000) s|=0xffff0000; \
	fa = (s >> shift_factor); \
	fa += ((s_1 * f[predict_nr][0]) >> 6) + ((s_2 * f[predict_nr][1]) >> 6); \
	s_2 = s_1; \
	s_1 = fa; \
	buf[nSample++] = fa; \
}

// size is in bytes
void LogPacketSound(void* packet, int memsize)
{
	u16 buf[28];

	u8* pstart = (u8*)packet;
	int s_1 = 0, s_2=0;
	
	for (int i = 0; i < memsize; i += 16) 
	{
		int predict_nr=(int)pstart[0];
		int shift_factor=predict_nr&0xf;
		predict_nr >>= 4;
		pstart += 2;

		for (int nSample=0;nSample<28; ++pstart)
		{
			int d=(int)*pstart;
			int s, fa;
			
			s =((d & 0xf) << 12);
			SetPacket(s);
			
			s=((d & 0xf0) << 8);
			SetPacket(s);
		}

		LogRawSound(buf, 2, buf, 2, 28);
	}
}

void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples)
{
	if (g_pWavRecord == NULL ) 
		g_pWavRecord = new WavOutFile(RECORD_FILENAME, SAMPLE_RATE, 16, 2);

	u8* left = (u8*)pleft;
	u8* right = (u8*)pright;
	static vector<s16> tempbuf;

	tempbuf.resize(2 * numsamples);

	for (int i = 0; i < numsamples; ++i) 
	{
		tempbuf[2*i+0] = *(s16*)left;
		tempbuf[2*i+1] = *(s16*)right;
		left += leftstride;
		right += rightstride;
	}

	g_pWavRecord->write(&tempbuf[0], numsamples*2);
}

int CALLBACK SPU2setupRecording(int start, void* pData)
{
	LOG_CALLBACK("SPU2setupRecording()\n");
	if ( start ) 
	{
		conf.options |= OPTION_RECORDING;
		printf("ZeroSPU2: started recording at %s\n", RECORD_FILENAME);
	}
	else 
	{
		conf.options &= ~OPTION_RECORDING;
		printf("ZeroSPU2: stopped recording\n");
	}
	
	return 1;
}

s32  CALLBACK SPU2freeze(int mode, freezeData *data)
{
	LOG_CALLBACK("SPU2freeze()\n");
	SPU2freezeData *spud;
	int i;
	assert( g_pDMABaseAddr != 0 );

	if (mode == FREEZE_LOAD) 
	{
		spud = (SPU2freezeData*)data->data; 
		if (spud->version != 0x70000001) 
		{
			printf("zerospu2: data wrong format\n");
			return 0;
		}

		memcpy(spu2regs, spud->spu2regs, 0x10000);
		memcpy(spu2mem, spud->spu2mem, 0x200000);
		pSpuIrq[0] = spu2mem + spud->nSpuIrq[0];
		pSpuIrq[1] = spu2mem + spud->nSpuIrq[1];
		memcpy(dwNewChannel2, spud->dwNewChannel2, 4*2);
		memcpy(dwEndChannel2, spud->dwEndChannel2, 4*2);
		dwNoiseVal = spud->dwNoiseVal;
		memcpy(iFMod, spud->iFMod, sizeof(iFMod));
		interrupt = spud->interrupt;
		memcpy(MemAddr, spud->MemAddr, sizeof(MemAddr));
		Adma4 = spud->adma[0];
		Adma4.MemAddr = (u16*)(g_pDMABaseAddr+spud->Adma4MemAddr);
		Adma7 = spud->adma[1];
		Adma7.MemAddr = (u16*)(g_pDMABaseAddr+spud->Adma7MemAddr);

		SPUCycles = spud->SPUCycles;
		SPUWorkerCycles = spud->SPUWorkerCycles;
		memcpy(SPUStartCycle, spud->SPUStartCycle, sizeof(SPUStartCycle));
		memcpy(SPUTargetCycle, spud->SPUTargetCycle, sizeof(SPUTargetCycle));

		for (i = 0; i < ARRAYSIZE(voices); ++i) 
		{
			memcpy(&voices[i], &spud->voices[i], min((int)SPU_VOICE_STATE_SIZE, spud->voicesize));
			voices[i].pStart = (u8*)((uptr)spud->voices[i].pStart+(uptr)spu2mem);
			voices[i].pLoop = (u8*)((uptr)spud->voices[i].pLoop+(uptr)spu2mem);
			voices[i].pCurr = (u8*)((uptr)spud->voices[i].pCurr+(uptr)spu2mem);
		}

		s_GlobalTimeStamp = 0;
		g_startcount = 0xffffffff;
		
		for (int i = 0; i < ARRAYSIZE(s_nDurations); ++i) 
		{
			s_nDurations[i] = NSFRAMES*1000;
		}
		
		s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
		s_nCurDuration = 0;
		s_nQueuedBuffers = 0;
		s_nDropPacket = 0;
	}
	else if (mode == FREEZE_SAVE) 
	{
		spud = (SPU2freezeData*)data->data;
		spud->version = 0x70000001;

		memcpy(spud->spu2regs, spu2regs, 0x10000);
		memcpy(spud->spu2mem, spu2mem, 0x200000);
		spud->nSpuIrq[0] = (int)(pSpuIrq[0] - spu2mem);
		spud->nSpuIrq[1] = (int)(pSpuIrq[1] - spu2mem);
		memcpy(spud->dwNewChannel2, dwNewChannel2, 4*2);
		memcpy(spud->dwEndChannel2, dwEndChannel2, 4*2);
		spud->dwNoiseVal = dwNoiseVal;
		memcpy(spud->iFMod, iFMod, sizeof(iFMod));
		spud->interrupt = interrupt;
		memcpy(spud->MemAddr, MemAddr, sizeof(MemAddr));

		spud->adma[0] = Adma4;
		spud->Adma4MemAddr = (u32)((uptr)Adma4.MemAddr - g_pDMABaseAddr);
		spud->adma[1] = Adma7;
		spud->Adma7MemAddr = (u32)((uptr)Adma7.MemAddr - g_pDMABaseAddr);

		spud->SPUCycles = SPUCycles;
		spud->SPUWorkerCycles = SPUWorkerCycles;
	
		memcpy(spud->SPUStartCycle, SPUStartCycle, sizeof(SPUStartCycle));
		memcpy(spud->SPUTargetCycle, SPUTargetCycle, sizeof(SPUTargetCycle));

		for (i = 0; i < ARRAYSIZE(s_nDurations); ++i) 
		{
			s_nDurations[i] = NSFRAMES*1000;
		}
		s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
		s_nCurDuration = 0;

		spud->voicesize = SPU_VOICE_STATE_SIZE;
		for (i = 0; i < ARRAYSIZE(voices); ++i) 
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
	else if (mode == FREEZE_SIZE) 
	{
		data->size = sizeof(SPU2freezeData);
	}
		
	return 0;
}
