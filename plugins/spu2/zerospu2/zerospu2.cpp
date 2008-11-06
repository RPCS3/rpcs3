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

#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

// ADSR constants
#define ATTACK_MS      494L
#define DECAYHALF_MS   286L
#define DECAY_MS       572L
#define SUSTAIN_MS     441L
#define RELEASE_MS     437L

#define AUDIO_BUFFER 2048

#define NSSIZE      48      // ~ 1 ms of data
#define NSFRAMES    16      // gather at least NSFRAMES of NSSIZE before submitting
#define NSPACKETS 24

#ifdef _DEBUG
char *libraryName      = "ZeroSPU2 (Debug)";
#else
char *libraryName      = "ZeroSPU2 ";
#endif

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
unsigned long   dwNoiseVal=1;                          // global noise generator
bool g_bPlaySound = true; // if true, will output sound, otherwise no
static int iFMod[NSSIZE];
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

struct AUDIOBUFFER
{
    u8* pbuf;
    u32 len;

    // 1 if new channels started in this packet
    // Variable used to smooth out sound by concentrating on new voices
    u32 timestamp; // in microseconds, only used for time stretching
    u32 avgtime;
    int newchannels;
};

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

//#ifdef _DEBUG
int g_logsound=0;
//FILE* g_fLogSound=NULL;
//#endif

int ADMAS4Write();
int ADMAS7Write();

void InitADSR();

void (*irqCallbackSPU2)()=0;                  // func of main emu, called on spu irq
void (*irqCallbackDMA4)()=0;                  // func of main emu, called on spu irq
void (*irqCallbackDMA7)()=0;                  // func of main emu, called on spu irq
uptr g_pDMABaseAddr=0;

const int f[5][2] = {   {    0,  0  },
                        {   60,  0  },
                        {  115, -52 },
                        {   98, -55 },
                        {  122, -60 } };

u32 RateTable[160];


// Atomic Operations
#if defined (_WIN32)

#ifndef __x86_64__
extern "C" LONG  __cdecl _InterlockedExchangeAdd(LPLONG volatile Addend, LONG Value);
#endif

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#else

typedef void* PVOID;

__forceinline long InterlockedExchangeAdd(long volatile* Addend, long Value)
{
	__asm__ __volatile__(".intel_syntax\n"
						 "lock xadd [%0], %%eax\n"
						 ".att_syntax\n" : : "r"(Addend), "a"(Value) : "memory" );
}

#endif

// channels and voices
VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1]; // +1 for modulation

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_SPU2;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (SPU2_MINOR<<24)|(SPU2_VERSION<<16)|(SPU2_REVISION<<8)|SPU2_BUILD;
}

void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.Log || spu2Log == NULL) return;

	va_start(list, fmt);
	vfprintf(spu2Log, fmt, list);
	va_end(list);

    //fprintf(spu2Log, "c0: %x %x, c1: %x %x\n", C0_SPUADDR, C0_IRQA, C1_SPUADDR, C1_IRQA);
}

s32 CALLBACK SPU2init()
{
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
	if (spu2regs == NULL) {
		SysMessage("Error allocating Memory\n"); return -1;
	}
    memset(spu2regs, 0, 0x10000);

    spu2mem = (u16*)malloc(0x200000); // 2Mb
    if (spu2mem == NULL) {
		SysMessage("Error allocating Memory\n"); return -1;
	}
    memset(spu2mem, 0, 0x200000);
    memset(dwEndChannel2, 0, sizeof(dwEndChannel2));
    memset(dwNewChannel2, 0, sizeof(dwNewChannel2));
    memset(iFMod, 0, sizeof(iFMod));
    memset(s_buffers, 0, sizeof(s_buffers));

    InitADSR();

    memset(voices, 0, sizeof(voices));
    // last 24 channels have higher mem offset
    for(int i = 0; i < 24; ++i)
        voices[i+24].memoffset = 0x400;

    // init each channel
    for(u32 i = 0; i < ARRAYSIZE(voices); ++i) {

        voices[i].chanid = i;
        voices[i].pLoop = voices[i].pStart = voices[i].pCurr = (u8*)spu2mem;

        voices[i].pvoice = (_SPU_VOICE*)((u8*)spu2regs+voices[i].memoffset)+(i%24);
        voices[i].ADSRX.SustainLevel = 1024;                // -> init sustain
    }

	return 0;
}

s32 CALLBACK SPU2open(void *pDsp) {
#ifdef _WIN32
    hWMain = pDsp == NULL ? NULL : *(HWND*)pDsp;
    if(!IsWindow(hWMain))
        hWMain=GetActiveWindow();
#endif

    LoadConfig();

    SPUCycles = SPUWorkerCycles = 0;
    interrupt = 0;
    SPUStartCycle[0] = SPUStartCycle[1] = 0;
    SPUTargetCycle[0] = SPUTargetCycle[1] = 0;
    s_nDropPacket = 0;

    if( conf.options & OPTION_TIMESTRETCH ) {
        pSoundTouch = new soundtouch::SoundTouch();
        pSoundTouch->setSampleRate(48000);
        pSoundTouch->setChannels(2);
        pSoundTouch->setTempoChange(0);

        pSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);
        pSoundTouch->setSetting(SETTING_USE_AA_FILTER, 1);
    }

    //conf.Log = 1;

    g_bPlaySound = !(conf.options&OPTION_MUTE);
//    if( conf.options & OPTION_REALTIME )
//        SPUWorkerCycles = timeGetTime();

    if( g_bPlaySound && SetupSound() != 0 ) {
        SysMessage("ZeroSPU2: Failed to initialize sound");
        g_bPlaySound = false;
    }

    if( g_bPlaySound ) {
        // initialize the audio buffers
        for(u32 i = 0; i < ARRAYSIZE(s_pAudioBuffers); ++i) {
            s_pAudioBuffers[i].pbuf = (u8*)_aligned_malloc(4*NSSIZE*NSFRAMES, 16); // 4 bytes for each sample
            s_pAudioBuffers[i].len = 0;
        }

        s_nCurBuffer = 0;
        s_nQueuedBuffers = 0;
        s_pCurOutput = (s16*)s_pAudioBuffers[0].pbuf;
        assert( s_pCurOutput != NULL);
    
        for(int i = 0; i < ARRAYSIZE(s_nDurations); ++i) {
            s_nDurations[i] = NSFRAMES*1000;
        }
        s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
        s_nCurDuration = 0;
        
        // launch the thread
        s_bThreadExit = false;
#ifdef _WIN32
        s_threadSPU2 = CreateThread(NULL, 0, SPU2ThreadProc, NULL, 0, NULL);
        if( s_threadSPU2 == NULL ) {
            return -1;
        }
#else
        if( pthread_create(&s_threadSPU2, NULL, SPU2ThreadProc, NULL) != 0 ) {
            SysMessage("ZeroSPU2: Failed to create spu2thread\n");
            return -1;
        }
#endif
    }

    g_nSpuInit = 1;
    return 0;
}

void CALLBACK SPU2close() {
    g_nSpuInit = 0;

    if( g_bPlaySound && !s_bThreadExit ) {
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
    
    for(u32 i = 0; i < ARRAYSIZE(s_pAudioBuffers); ++i) {
        _aligned_free(s_pAudioBuffers[i].pbuf);
    }
    memset(s_pAudioBuffers, 0, sizeof(s_pAudioBuffers));
}

void CALLBACK SPU2shutdown()
{
    free(spu2regs); spu2regs = NULL;
    free(spu2mem); spu2mem = NULL;

    if (spu2Log) fclose(spu2Log);
}

// simulate SPU2 for 1ms
void SPU2Worker();

#define CYCLES_PER_MS (36864000/1000)

void CALLBACK SPU2async(u32 cycle)
{
    SPUCycles += cycle;
	if(interrupt & (1<<2)){
		if(SPUCycles - SPUStartCycle[1] >= SPUTargetCycle[1]){
			interrupt &= ~(1<<2);
			irqCallbackDMA7();
		}
		
	}

	if(interrupt & (1<<1)){
		if(SPUCycles - SPUStartCycle[0] >= SPUTargetCycle[0]){
			interrupt &= ~(1<<1);
			irqCallbackDMA4();
		}
	}

    if( g_nSpuInit ) {

//        if( (conf.options & OPTION_REALTIME) ) {
//            u32 iter = 0;
//            while(SPUWorkerCycles < timeGetTime()) {
//                SPU2Worker();
//                SPUWorkerCycles++;
//                if( iter++ > 1 )
//                    break;
//                //if( SPUTargetCycle[0] >= CYCLES_PER_MS ) SPUTargetCycle[0] -= CYCLES_PER_MS;
//                //if( SPUTargetCycle[1] >= CYCLES_PER_MS ) SPUTargetCycle[1] -= CYCLES_PER_MS;
//            }
//        }
//        else {
            while( SPUCycles-SPUWorkerCycles > 0 && CYCLES_PER_MS < SPUCycles-SPUWorkerCycles ) {
                SPU2Worker();
                SPUWorkerCycles += CYCLES_PER_MS;
            }
//        }
    }
    else SPUWorkerCycles = SPUCycles;
}

void InitADSR()                                    // INIT ADSR
{
    unsigned long r,rs,rd;
    int i;
    memset(RateTable,0,sizeof(unsigned long)*160);        // build the rate table according to Neill's rules (see at bottom of file)

    r=3;rs=1;rd=0;

    for(i=32;i<160;i++)                                   // we start at pos 32 with the real values... everything before is 0
    {
        if(r<0x3FFFFFFF)
        {
            r+=rs;
            rd++;if(rd==5) {rd=1;rs*=2;}
        }
        if(r>0x3FFFFFFF) r=0x3FFFFFFF;

        RateTable[i]=r;
    }
}

int MixADSR(VOICE_PROCESSED* pvoice)                             // MIX ADSR
{    
    if(pvoice->bStop)                                  // should be stopped:
    {                      
        if(pvoice->bIgnoreLoop==0){
            pvoice->ADSRX.EnvelopeVol=0;
            pvoice->bOn=false;
            pvoice->pStart= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->pLoop= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->pCurr= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->bStop=true;
            pvoice->bIgnoreLoop=false;
            return 0;
        }
        if(pvoice->ADSRX.ReleaseModeExp)// do release
        {
            switch((pvoice->ADSRX.EnvelopeVol>>28)&0x7)
            {
                case 0: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +0 + 32]; break;
                case 1: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +4 + 32]; break;
                case 2: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +6 + 32]; break;
                case 3: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +8 + 32]; break;
                case 4: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +9 + 32]; break;
                case 5: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +10+ 32]; break;
                case 6: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +11+ 32]; break;
                case 7: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +12+ 32]; break;
            }
        }
        else
        {
            pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x0C + 32];
        }

        if(pvoice->ADSRX.EnvelopeVol<0) 
        {
            pvoice->ADSRX.EnvelopeVol=0;
            pvoice->bOn=false;
            pvoice->pStart= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->pLoop= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->pCurr= (u8*)(spu2mem+pvoice->iStartAddr);
            pvoice->bStop=true;
            pvoice->bIgnoreLoop=false;
            //pvoice->bReverb=0;
            //pvoice->bNoise=0;
        }

        pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
        pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
        return pvoice->ADSRX.lVolume;
    }
    else                                                  // not stopped yet?
    {
        if(pvoice->ADSRX.State==0)                       // -> attack
        {
            if(pvoice->ADSRX.AttackModeExp)
            {
                if(pvoice->ADSRX.EnvelopeVol<0x60000000) 
                    pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x10 + 32];
                else
                    pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x18 + 32];
            }
            else
            {
                pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x10 + 32];
            }

            if(pvoice->ADSRX.EnvelopeVol<0) 
            {
                pvoice->ADSRX.EnvelopeVol=0x7FFFFFFF;
                pvoice->ADSRX.State=1;
            }

            pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
            return pvoice->ADSRX.lVolume;
        }
        //--------------------------------------------------//
        if(pvoice->ADSRX.State==1)                       // -> decay
        {
            switch((pvoice->ADSRX.EnvelopeVol>>28)&0x7)
            {
            case 0: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+0 + 32]; break;
            case 1: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+4 + 32]; break;
            case 2: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+6 + 32]; break;
            case 3: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+8 + 32]; break;
            case 4: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+9 + 32]; break;
            case 5: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+10+ 32]; break;
            case 6: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+11+ 32]; break;
            case 7: pvoice->ADSRX.EnvelopeVol-=RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+12+ 32]; break;
            }

            if(pvoice->ADSRX.EnvelopeVol<0) pvoice->ADSRX.EnvelopeVol=0;
            if(((pvoice->ADSRX.EnvelopeVol>>27)&0xF) <= pvoice->ADSRX.SustainLevel)
            {
                pvoice->ADSRX.State=2;
            }

            pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
            return pvoice->ADSRX.lVolume;
        }
        //--------------------------------------------------//
        if(pvoice->ADSRX.State==2)                       // -> sustain
        {
            if(pvoice->ADSRX.SustainIncrease)
            {
                if(pvoice->ADSRX.SustainModeExp)
                {
                    if(pvoice->ADSRX.EnvelopeVol<0x60000000) 
                        pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x10 + 32];
                    else
                        pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x18 + 32];
                }
                else
                {
                    pvoice->ADSRX.EnvelopeVol+=RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x10 + 32];
                }

                if(pvoice->ADSRX.EnvelopeVol<0) 
                {
                    pvoice->ADSRX.EnvelopeVol=0x7FFFFFFF;
                }
            }
            else
            {
                if(pvoice->ADSRX.SustainModeExp)
                {
                    switch((pvoice->ADSRX.EnvelopeVol>>28)&0x7)
                    {
                    case 0: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +0 + 32];break;
                    case 1: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +4 + 32];break;
                    case 2: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +6 + 32];break;
                    case 3: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +8 + 32];break;
                    case 4: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +9 + 32];break;
                    case 5: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +10+ 32];break;
                    case 6: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +11+ 32];break;
                    case 7: pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +12+ 32];break;
                    }
                }
                else
                {
                    pvoice->ADSRX.EnvelopeVol-=RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x0F + 32];
                }

                if(pvoice->ADSRX.EnvelopeVol<0) 
                {
                    pvoice->ADSRX.EnvelopeVol=0;
                }
            }
            pvoice->ADSRX.lVolume=pvoice->ADSRX.EnvelopeVol>>21;
            return pvoice->ADSRX.lVolume;
        }
    }
    return 0;
}

// simulate SPU2 for 1ms
void SPU2Worker()
{
    int s_1,s_2,fa;
    u8* start;
    unsigned int nSample;
    int ch,predict_nr,shift_factor,flags,d,s;
    //int bIRQReturn=0;

    // assume s_buffers are zeroed out
    if( dwNewChannel2[0] || dwNewChannel2[1] )
        s_pAudioBuffers[s_nCurBuffer].newchannels++;
        
    VOICE_PROCESSED* pChannel=voices;
    for(ch=0;ch<SPU_NUMBER_VOICES;ch++,pChannel++)              // loop em all... we will collect 1 ms of sound of each playing channel
    {
        if(pChannel->bNew) {
            pChannel->StartSound();                         // start new sound
            dwEndChannel2[ch/24]&=~(1<<(ch%24));                  // clear end channel bit
            dwNewChannel2[ch/24]&=~(1<<(ch%24));                  // clear channel bit
        }

        if(!pChannel->bOn)
            continue;

        if(pChannel->iActFreq!=pChannel->iUsedFreq)     // new psx frequency?
            pChannel->VoiceChangeFrequency();

        // loop until 1 ms of data is reached
        int ns = 0;
        while(ns<NSSIZE)
        {
            if(pChannel->bFMod==1 && iFMod[ns])           // fmod freq channel
                pChannel->FModChangeFrequency(ns);

            while(pChannel->spos >= 0x10000 )
            {
                if(pChannel->iSBPos==28)            // 28 reached?
                {
                    start=pChannel->pCurr;          // set up the current pos

                    // special "stop" sign
                    if( start == (u8*)-1 ) //!pChannel->bOn 
                    {
                        pChannel->bOn=false;                        // -> turn everything off
                        pChannel->ADSRX.lVolume=0;
                        pChannel->ADSRX.EnvelopeVol=0;
                        goto ENDX;                              // -> and done for this channel
                    }

                    pChannel->iSBPos=0;

                    // decode the 16byte packet
                    s_1=pChannel->s_1;
                    s_2=pChannel->s_2;

                    predict_nr=(int)start[0];
                    shift_factor=predict_nr&0xf;
                    predict_nr >>= 4;
                    flags=(int)start[1];
                    start += 2;

                    for(nSample=0;nSample<28; ++start)
                    {
                        d=(int)*start;
                        s=((d&0xf)<<12);
                        if(s&0x8000) s|=0xffff0000;

                        fa=(s >> shift_factor);
                        fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
                        s_2=s_1;s_1=fa;
                        s=((d & 0xf0) << 8);

                        pChannel->SB[nSample++]=fa;

                        if(s&0x8000) s|=0xffff0000;
                        fa=(s>>shift_factor);              
                        fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
                        s_2=s_1;s_1=fa;

                        pChannel->SB[nSample++]=fa;
                    }

//                    if(pChannel->GetCtrl()->irq)         // some callback and irq active?
//                    {
//                        // if irq address reached or irq on looping addr, when stop/loop flag is set 
//                        u8* pirq = (u8*)pSpuIrq[ch>=24];
//                        if( (pirq > start-16  && pirq <= start)
//                            || ((flags&1) && (pirq >  pChannel->pLoop-16 && pirq <= pChannel->pLoop)))
//                        {
//                            IRQINFO |= 4<<(int)(ch>=24);
//                            SPU2_LOG("SPU2Worker:interrupt\n");
//                            irqCallbackSPU2();
//                        }
//                    }
                    // irq occurs no matter what core access the address
                    for(int core = 0; core < 2; ++core) {
                        if(((SPU_CONTROL_*)(spu2regs+0x400*core+REG_C0_CTRL))->irq)         // some callback and irq active?
                        {
                            // if irq address reached or irq on looping addr, when stop/loop flag is set 
                            u8* pirq = (u8*)pSpuIrq[core];
                            if( (pirq > start-16  && pirq <= start)
                                || ((flags&1) && (pirq >  pChannel->pLoop-16 && pirq <= pChannel->pLoop)))
                            {
                                IRQINFO |= 4<<core;
                                SPU2_LOG("SPU2Worker:interrupt\n");
                                irqCallbackSPU2();
                            }
                        }
                    }

                    // flag handler
                    if((flags&4) && (!pChannel->bIgnoreLoop))
                        pChannel->pLoop=start-16;                // loop adress

                    if(flags&1)                               // 1: stop/loop
                    {
                        // We play this block out first...
                        dwEndChannel2[ch/24]|=(1<<(ch%24));
                        //if(!(flags&2))                          // 1+2: do loop... otherwise: stop
                        if(flags!=3 || pChannel->pLoop==NULL)
                        {                                      // and checking if pLoop is set avoids crashes, yeah
                            start = (u8*)-1;
                            pChannel->bStop = true;
                            pChannel->bIgnoreLoop = false;
                        }
                        else
                        {
                            start = pChannel->pLoop;
//                            if(conf.options&OPTION_FFXHACK)
//                                start += 16;
                        }
                    }

                    pChannel->pCurr=start;                    // store values for next cycle
                    pChannel->s_1=s_1;
                    pChannel->s_2=s_2;

//                            if(bIRQReturn)                            // special return for "spu irq - wait for cpu action"
//                            {
//                                bIRQReturn=0;
//                                DWORD dwWatchTime=GetTickCount()+1500;
//
//                                while(iSpuAsyncWait && !bEndThread && GetTickCount()<dwWatchTime)
//                                    Sleep(1);
//                            }
                }

                fa=pChannel->SB[pChannel->iSBPos++];        // get sample data
                pChannel->StoreInterpolationVal(fa);
                pChannel->spos -= 0x10000;
            }

            if(pChannel->bNoise)
                fa=pChannel->iGetNoiseVal();               // get noise val
            else
                fa=pChannel->iGetInterpolationVal();       // get sample val

            int sval = (MixADSR(pChannel)*fa)/1023;   // mix adsr

            if(pChannel->bFMod==2)                        // fmod freq channel
            {
                iFMod[ns]=sval;                    // -> store 1T sample data, use that to do fmod on next channel
            }
            else {
                if(pChannel->bVolumeL)
                    s_buffers[ns][0]+=(sval*pChannel->leftvol)>>14;
                
                if(pChannel->bVolumeR)
                    s_buffers[ns][1]+=(sval*pChannel->rightvol)>>14;
            }

            // go to the next packet
            ns++;
            pChannel->spos += pChannel->sinc;
        }
ENDX:
        ;
    }

    // mix all channels
    if( (spu2Ru16(REG_C0_MMIX) & 0xF0) && (spu2Ru16(REG_C0_ADMAS) & 0x1) /*&& !(spu2Ru16(REG_C0_CTRL) & 0x30)*/) {
        for(int ns=0;ns<NSSIZE;ns++) { 
        
            if((spu2Ru16(REG_C0_MMIX) & 0x80)) s_buffers[ns][0] += (((short*)spu2mem)[0x2000+Adma4.Index]*(int)spu2Ru16(REG_C0_BVOLL))>>16;
            if((spu2Ru16(REG_C0_MMIX) & 0x40)) s_buffers[ns][1] += (((short*)spu2mem)[0x2200+Adma4.Index]*(int)spu2Ru16(REG_C0_BVOLR))>>16;

		    Adma4.Index +=1;
            // just add after every sample, it is better than adding 1024 all at once (games like Genji don't like it)
            MemAddr[0] += 4;

		    if(Adma4.Index == 128 || Adma4.Index == 384)
		    {
			    if(ADMAS4Write())
			    {
				    //if( Adma4.AmountLeft == 0 )
                    //spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
					//printf("ADMA4 end spu cycle %x\n", SPUCycles);
					if(interrupt & 0x2){
						interrupt &= ~0x2;
						printf("Stopping double interrupt DMA4\n");
					}
                    irqCallbackDMA4();
					
			    }
                //else {
                    Adma4.Enabled = 2;
//                    if( Adma4.AmountLeft > 0 )
//                        MemAddr[0] += 1024;
               // }
		    }
        	
            if(Adma4.Index == 512) {
                if( Adma4.Enabled == 2 ) {
//                    if( Adma4.AmountLeft == 0 )
//                        MemAddr[0] += 1024;
                    Adma4.Enabled = 0;
                }
			    Adma4.Index = 0;
            }
	    }

        //LogRawSound(s_buffers, 4, &s_buffers[0][1], 4, NSSIZE);
    }

    if( (spu2Ru16(REG_C1_MMIX) & 0xF0) && (spu2Ru16(REG_C1_ADMAS) & 0x2) /*&& !(spu2Ru16(REG_C1_CTRL) & 0x30)*/) {

        for(int ns=0;ns<NSSIZE;ns++) { 
            if((spu2Ru16(REG_C1_MMIX) & 0x80)) s_buffers[ns][0] += (((short*)spu2mem)[0x2400+Adma7.Index]*(int)spu2Ru16(REG_C1_BVOLL))>>16;
            if((spu2Ru16(REG_C1_MMIX) & 0x40)) s_buffers[ns][1] += (((short*)spu2mem)[0x2600+Adma7.Index]*(int)spu2Ru16(REG_C1_BVOLR))>>16;

		    Adma7.Index +=1;
            MemAddr[1] += 4;
        	
		    if(Adma7.Index == 128 || Adma7.Index == 384)
		    {
			    if(ADMAS7Write())
			    {
				   // spu2Ru16(REG_C1_SPUSTAT)&=~0x80;
					//printf("ADMA7 end spu cycle %x\n", SPUCycles);
					if(interrupt & 0x4){
						interrupt &= ~0x4;
						printf("Stopping double interrupt DMA7\n");
					}
				    irqCallbackDMA7();
					
			    }
                //else {
                    Adma7.Enabled = 2;
                    //MemAddr[1] += 1024;
               // }
		    }

            if(Adma7.Index == 512) {
                if( Adma7.Enabled == 2 )
                    Adma7.Enabled = 0;
                Adma7.Index = 0;
            }
	    }
    }

    if( g_bPlaySound ) {

        assert( s_pCurOutput != NULL);

        for(int ns=0;ns<NSSIZE;ns++) {
            // clamp and write
            if( s_buffers[ns][0] < -32767 ) s_pCurOutput[0] = -32767;
            else if( s_buffers[ns][0] > 32767 ) s_pCurOutput[0] = 32767;
            else s_pCurOutput[0] = (s16)s_buffers[ns][0];
            
            if( s_buffers[ns][1] < -32767 ) s_pCurOutput[1] = -32767;
            else if( s_buffers[ns][1] > 32767 ) s_pCurOutput[1] = 32767;
            else s_pCurOutput[1] = (s16)s_buffers[ns][1];
            s_pCurOutput += 2;

            
            s_buffers[ns][0] = 0;
            s_buffers[ns][1] = 0;
        }
        // check if end reached

        if( (uptr)s_pCurOutput - (uptr)s_pAudioBuffers[s_nCurBuffer].pbuf >= 4 * NSSIZE * NSFRAMES ) {

            if( conf.options & OPTION_RECORDING ) {
                static int lastrectime=0;
                if( timeGetTime()-lastrectime > 5000 ) {
                    printf("ZeroSPU2: recording\n");
                    lastrectime = timeGetTime();
                }
                LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf+2, 4, NSSIZE*NSFRAMES);
            }

            if( s_nQueuedBuffers >= ARRAYSIZE(s_pAudioBuffers)-1 ) {
                s_nDropPacket += NSFRAMES;
                s_GlobalTimeStamp = GetMicroTime();
//#ifdef _DEBUG
                //printf("ZeroSPU2: dropping packets! game too fast\n");
//#endif
            }
            else {
                // submit to final mixer
#ifdef _DEBUG
                if( g_logsound ) {
                    LogRawSound(s_pAudioBuffers[s_nCurBuffer].pbuf, 4, s_pAudioBuffers[s_nCurBuffer].pbuf+2, 4, NSSIZE*NSFRAMES);
                }
#endif
                if( g_startcount == 0xffffffff ) {
                    g_startcount = timeGetTime();
                    g_packetcount = 0;
                }

                if( conf.options & OPTION_TIMESTRETCH ) {
                    u64 newtime = GetMicroTime();
                    if( s_GlobalTimeStamp == 0 )
                        s_GlobalTimeStamp = newtime-NSFRAMES*1000;
                    u32 newtotal = s_nTotalDuration-s_nDurations[s_nCurDuration];
                    u32 duration = (u32)(newtime-s_GlobalTimeStamp);
                    s_nDurations[s_nCurDuration] = duration;
                    s_nTotalDuration = newtotal + duration;
                    s_nCurDuration = (s_nCurDuration+1)%ARRAYSIZE(s_nDurations);
                    s_GlobalTimeStamp = newtime;
                    s_pAudioBuffers[s_nCurBuffer].timestamp = timeGetTime();
                    s_pAudioBuffers[s_nCurBuffer].avgtime = s_nTotalDuration/ARRAYSIZE(s_nDurations);
                    //fprintf(spu2Log, "%d %d\n", duration, s_pAudioBuffers[s_nCurBuffer].avgtime);
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
    for(int i = 0; i < newsamples; ++i) {
        int io = i * oldsamples;
        int old = io / newsamples;
        int rem = io - old * newsamples;

        // newsamp = [old] * (1-rem/newsamp) + [old+1] * (rem/newsamp)
        old *= 2;
        int newsampL = pStereoSamples[old] * (newsamples - rem) + pStereoSamples[old+2] * rem;
        int newsampR = pStereoSamples[old+1] * (newsamples - rem) + pStereoSamples[old+3] * rem;
        pNewSamples[2*i] = newsampL / newsamples;
        pNewSamples[2*i+1] = newsampR / newsamples;
    }
}

static PCSX2_ALIGNED16(s16 s_ThreadBuffer[NSSIZE*NSFRAMES*2*5]);

// communicates with the audio hardware
#ifdef _WIN32
DWORD WINAPI SPU2ThreadProc(LPVOID)
#else
void* SPU2ThreadProc(void* lpParam)
#endif
{
    int nReadBuf = 0;

    while(!s_bThreadExit) {

        if( !(conf.options&OPTION_REALTIME) ) {
            while(s_nQueuedBuffers< 3 && !s_bThreadExit) {
                //printf("sleeping!!!!\n");
                Sleep(1);
                if( s_bThreadExit )
                    return NULL;
            }

            while( SoundGetBytesBuffered() > 72000 ) {
                //printf("bytes buffered\n");
                Sleep(1);

                if( s_bThreadExit )
                    return NULL;
            }
        }
        else {
            while(s_nQueuedBuffers< 1 && !s_bThreadExit) {
                //printf("sleeping!!!!\n");
                Sleep(1);
            }
        }


        int ps2delay = timeGetTime() - s_pAudioBuffers[nReadBuf].timestamp;
        int NewSamples = s_pAudioBuffers[nReadBuf].avgtime;

        if( (conf.options & OPTION_TIMESTRETCH) ) {

            int bytesbuf = SoundGetBytesBuffered();
            if( bytesbuf < 8000 )
                NewSamples += 1000;
            // check the current timestamp, if too far apart, speed up audio
            else if( bytesbuf > 40000 ) {
                //printf("making faster %d\n", timeGetTime() - s_pAudioBuffers[nReadBuf].timestamp);
                NewSamples -= (bytesbuf-40000)/10;//*(ps2delay-NewSamples*8/1000);
            }

            if( s_nDropPacket > 0 ) {
                s_nDropPacket--;
                NewSamples -= 1000;
            }

            NewSamples *= NSSIZE;
            NewSamples /= 1000;

            NewSamples = min(NewSamples, NSFRAMES*NSSIZE*3);

            //ResampleTimeStretch((s16*)s_pAudioBuffers[nReadBuf].pbuf, s_pAudioBuffers[nReadBuf].len/4, s_ThreadBuffer, NewSamples);

            int oldsamples = s_pAudioBuffers[nReadBuf].len/4;

            /*if( NewSamples < oldsamples/2 )
                NewSamples = oldsamples/2;*/

            if( (nReadBuf&3)==0 ) { // wow, this if statement makes the whole difference
                pSoundTouch->setTempoChange(100.0f*(float)oldsamples/(float)NewSamples - 100.0f);
            }

            pSoundTouch->putSamples((s16*)s_pAudioBuffers[nReadBuf].pbuf, oldsamples);

            // extract 2*NSFRAMES ms at a time
            int nOutSamples;
            do
            {
                nOutSamples = pSoundTouch->receiveSamples(s_ThreadBuffer, NSSIZE*NSFRAMES*5);
                if( nOutSamples > 0 ) {
                    //LogRawSound(s_ThreadBuffer, 4, s_ThreadBuffer+1, 4, nOutSamples);
                    //printf("%d %d\n", timeGetTime(), nOutSamples);
                    SoundFeedVoiceData((u8*)s_ThreadBuffer, nOutSamples*4);
                    //g_packetcount += nOutSamples;
                }
            } while (nOutSamples != 0);

            
            //printf("ave: %d %d, played: %d, queued: %d, delay: %d, proc: %d\n", NewSamples, s_pAudioBuffers[nReadBuf].avgtime, SoundGetBytesBuffered(), s_nQueuedBuffers, ps2delay, GetMicroTime()-starttime);
        }
        else {
            SoundFeedVoiceData(s_pAudioBuffers[nReadBuf].pbuf, s_pAudioBuffers[nReadBuf].len);
        }

        // don't go to the next buffer unless there is more data buffered
        nReadBuf = (nReadBuf+1)%ARRAYSIZE(s_pAudioBuffers);
        InterlockedExchangeAdd((long*)&s_nQueuedBuffers, -1);

        if( s_bThreadExit )
            break;
    }

    return NULL;
}

void CALLBACK SPU2readDMA4Mem(u16 *pMem, int size)
{
    u32 spuaddr = C0_SPUADDR;
    int i;

    SPU2_LOG("SPU2 readDMA4Mem size %x, addr: %x\n", size, pMem);

    for(i=0;i<size;i++)
    {
        *pMem++ = *(u16*)(spu2mem+spuaddr);
        if((spu2Rs16(REG_C0_CTRL)&0x40) && C0_IRQA == spuaddr){
            //spu2Ru16(SPDIF_OUT) |= 0x4;
            C0_SPUADDR_SET(spuaddr);
            IRQINFO |= 4;
            SPU2_LOG("SPU2readDMA4Mem:interrupt\n");
            irqCallbackSPU2();
        }

        spuaddr++;           // inc spu addr
        if(spuaddr>0x0fffff) // wrap at 2Mb
            spuaddr=0;             // wrap
    }

    spuaddr+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
    C0_SPUADDR_SET(spuaddr);

    // got from J.F. and Kanodin... is it needed?
    spu2Ru16(REG_C0_SPUSTAT) &=~0x80;                                     // DMA complete
    SPUStartCycle[0] = SPUCycles;
    SPUTargetCycle[0] = size;
    interrupt |= (1<<1);
}

void CALLBACK SPU2readDMA7Mem(u16* pMem, int size)
{
    u32 spuaddr = C1_SPUADDR;
    int i;

    SPU2_LOG("SPU2 readDMA7Mem size %x, addr: %x\n", size, pMem);

    for(i=0;i<size;i++)
    {
        *pMem++ = *(u16*)(spu2mem+spuaddr);
        if((spu2Rs16(REG_C1_CTRL)&0x40) && C1_IRQA == spuaddr ){
           //spu2Ru16(SPDIF_OUT) |= 0x8;
           C1_SPUADDR_SET(spuaddr);
           IRQINFO |= 8;
           SPU2_LOG("SPU2readDMA7Mem:interrupt\n");
           irqCallbackSPU2();
        }
        spuaddr++;                            // inc spu addr
        if(spuaddr>0x0fffff) // wrap at 2Mb
            spuaddr=0;             // wrap
    }

    spuaddr+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
    C1_SPUADDR_SET(spuaddr);

    // got from J.F. and Kanodin... is it needed?
    spu2Ru16(REG_C1_SPUSTAT)&=~0x80;                                     // DMA complete
    SPUStartCycle[1] = SPUCycles;
    SPUTargetCycle[1] = size;
    interrupt |= (1<<2);
}

// WRITE

// AutoDMA's are used to transfer to the DIRECT INPUT area of the spu2 memory
// Left and Right channels are always interleaved together in the transfer so 
// the AutoDMA's deinterleaves them and transfers them. An interrupt is
// generated when half of the buffer (256 short-words for left and 256 
// short-words for right ) has been transferred. Another interrupt occurs at 
// the end of the transfer.
int ADMAS4Write()
{
    u32 spuaddr;
    if(interrupt & 0x2){
		printf("4 returning for interrupt\n");
        return 0;
	}
	if(Adma4.AmountLeft <= 0){
		printf("4 amount left is 0\n");
        return 1;
	}

    assert( Adma4.AmountLeft >= 512 );
    spuaddr = C0_SPUADDR;
    // SPU2 Deinterleaves the Left and Right Channels
    memcpy((short*)(spu2mem + spuaddr + 0x2000),(short*)Adma4.MemAddr,512);
    Adma4.MemAddr += 256;
    memcpy((short*)(spu2mem + spuaddr + 0x2200),(short*)Adma4.MemAddr,512);
    Adma4.MemAddr += 256;
	if( (spu2Ru16(REG_C0_CTRL)&0x40) && ((spuaddr + 0x2400) <= C0_IRQA &&  (spuaddr + 0x2400 + 256) >= C0_IRQA)){
        //spu2Ru16(SPDIF_OUT) |= 0x4;
        IRQINFO |= 4;
        printf("ADMA 4 Mem access:interrupt\n");
        irqCallbackSPU2();
    }
	if( (spu2Ru16(REG_C0_CTRL)&0x40) && ((spuaddr + 0x2600) <= C0_IRQA &&  (spuaddr + 0x2600 + 256) >= C0_IRQA)){
        //spu2Ru16(SPDIF_OUT) |= 0x4;
        IRQINFO |= 4;
        printf("ADMA 4 Mem access:interrupt\n");
        irqCallbackSPU2();
    }

    spuaddr = (spuaddr + 256) & 511;
    C0_SPUADDR_SET(spuaddr);
	
    Adma4.AmountLeft-=512;

    
	
    if(Adma4.AmountLeft > 0) return 0;
	else return 1;
}

int ADMAS7Write()
{
    u32 spuaddr;
	if(interrupt & 0x4){
		printf("7 returning for interrupt\n");
        return 0;
	}
	if(Adma7.AmountLeft <= 0){
		printf("7 amount left is 0\n");
        return 1;
	}

    assert( Adma7.AmountLeft >= 512 );
    spuaddr = C1_SPUADDR;
    // SPU2 Deinterleaves the Left and Right Channels
    memcpy((short*)(spu2mem + spuaddr + 0x2400),(short*)Adma7.MemAddr,512);
    Adma7.MemAddr += 256;
    memcpy((short*)(spu2mem + spuaddr + 0x2600),(short*)Adma7.MemAddr,512);
    Adma7.MemAddr += 256;
	if( (spu2Ru16(REG_C1_CTRL)&0x40) && ((spuaddr + 0x2400) <= C1_IRQA &&  (spuaddr + 0x2400 + 256) >= C1_IRQA)){
        //spu2Ru16(SPDIF_OUT) |= 0x4;
        IRQINFO |= 8;
       printf("ADMA 7 Mem access:interrupt\n");
        irqCallbackSPU2();
    }
	if( (spu2Ru16(REG_C1_CTRL)&0x40) && ((spuaddr + 0x2600) <= C1_IRQA &&  (spuaddr + 0x2600 + 256) >= C1_IRQA)){
        //spu2Ru16(SPDIF_OUT) |= 0x4;
        IRQINFO |= 8;
       printf("ADMA 7 Mem access:interrupt\n");
        irqCallbackSPU2();
    }
    spuaddr = (spuaddr + 256) & 511;
    C1_SPUADDR_SET(spuaddr);
	
    Adma7.AmountLeft-=512;
   
    assert( Adma7.AmountLeft >= 0 );

    if(Adma7.AmountLeft > 0) return 0;
	else return 1;
}

void CALLBACK SPU2writeDMA4Mem(u16* pMem, int size)
{
    u32 spuaddr;

    SPU2_LOG("SPU2 writeDMA4Mem size %x, addr: %x(spu2:%x), ctrl: %x, adma: %x\n", size, pMem, C0_SPUADDR, spu2Ru16(REG_C0_CTRL), spu2Ru16(REG_C0_ADMAS));

    if((spu2Ru16(REG_C0_ADMAS) & 0x1) && (spu2Ru16(REG_C0_CTRL) & 0x30) == 0 && size)
    {
//        u16* ptempmem = pMem;
//        for(int i = 0; i < size/512; ++i) {
//            LogRawSound(ptempmem, 2, ptempmem+256, 2, 256);
//            ptempmem += 512;
//        }
    
		//printf("ADMA4 size %x\n", size);
        // if still active, don't destroy adma4
        if( !Adma4.Enabled )
            Adma4.Index = 0;

        //memset(&Adma4,0,sizeof(ADMA));
        Adma4.MemAddr = pMem;
        Adma4.AmountLeft = size;
		SPUTargetCycle[0] = size;
		spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
        if( !Adma4.Enabled || Adma4.Index > 384 ) {
            C0_SPUADDR_SET(0);
			if(ADMAS4Write()){
				SPUStartCycle[0] = SPUCycles;
			   // SPUTargetCycle[0] = 512;//512*48000;
				//spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
				interrupt |= (1<<1);
			}
        }

		//if(interrupt & 0x2) printf("start ADMA4 interrupt target cycle %x start cycle %x spu cycle %x\n", SPUTargetCycle[0], SPUStartCycle[0], SPUCycles);
        Adma4.Enabled = 1;
        return;
    }

    spuaddr = C0_SPUADDR;
    memcpy((unsigned char*)(spu2mem + spuaddr),(unsigned char*)pMem,size<<1);
    spuaddr += size;
    C0_SPUADDR_SET(spuaddr);
    
    if( (spu2Ru16(REG_C0_CTRL)&0x40) && (spuaddr < C0_IRQA && C0_IRQA <= spuaddr+0x20)){
        //spu2Ru16(SPDIF_OUT) |= 0x4;
        IRQINFO |= 4;
        SPU2_LOG("SPU2writeDMA4Mem:interrupt\n");
        irqCallbackSPU2();
    }
    if(spuaddr>0xFFFFE)
        spuaddr = 0x2800;
    C0_SPUADDR_SET(spuaddr);

    MemAddr[0] += size<<1;
    spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
    SPUStartCycle[0] = SPUCycles;
    SPUTargetCycle[0] = size;
    interrupt |= (1<<1);
}

void CALLBACK SPU2writeDMA7Mem(u16* pMem, int size)
{
    u32 spuaddr;

    SPU2_LOG("SPU2 writeDMA7Mem size %x, addr: %x(spu2:%x), ctrl: %x, adma: %x\n", size, pMem, C1_SPUADDR, spu2Ru16(REG_C1_CTRL), spu2Ru16(REG_C1_ADMAS));

    if((spu2Ru16(REG_C1_ADMAS) & 0x2) && (spu2Ru16(REG_C1_CTRL) & 0x30) == 0 && size)
    {
//        u16* ptempmem = pMem;
//        for(int i = 0; i < size/512; ++i) {
//            LogRawSound(ptempmem, 2, ptempmem+256, 2, 256);
//            ptempmem += 512;
//        }

		//printf("ADMA7 size %x\n", size);
        if( !Adma7.Enabled )
            Adma7.Index = 0;

        //memset(&Adma7,0,sizeof(ADMA));
        Adma7.MemAddr = pMem;
        Adma7.AmountLeft = size;
		SPUTargetCycle[1] = size;
		spu2Ru16(REG_C1_SPUSTAT)&=~0x80;
        if( !Adma7.Enabled || Adma7.Index > 384 ) {
            C1_SPUADDR_SET(0);
			if(ADMAS7Write()){
				SPUStartCycle[1] = SPUCycles;
			   // SPUTargetCycle[0] = 512;//512*48000;
				interrupt |= (1<<2);
			}
        }
		//if(interrupt & 0x4) printf("start ADMA7 interrupt target cycle %x start cycle %x spu cycle %x\n", SPUTargetCycle[1], SPUStartCycle[1], SPUCycles);
        Adma7.Enabled = 1;

        return;
    }

#ifdef _DEBUG
    if( conf.Log && conf.options & OPTION_RECORDING )
        LogPacketSound(pMem, 0x8000);
#endif

    spuaddr = C1_SPUADDR;
    memcpy((unsigned char*)(spu2mem + spuaddr),(unsigned char*)pMem,size<<1);
    spuaddr += size;
    C1_SPUADDR_SET(spuaddr);
    
    if( (spu2Ru16(REG_C1_CTRL)&0x40) && (spuaddr < C1_IRQA && C1_IRQA <= spuaddr+0x20)){
        //spu2Ru16(SPDIF_OUT) |= 0x8;
        IRQINFO |= 8;
        SPU2_LOG("SPU2writeDMA7Mem:interrupt\n");
        irqCallbackSPU2();
    }
    if(spuaddr>0xFFFFE)
        spuaddr = 0x2800;
    C1_SPUADDR_SET(spuaddr);

    MemAddr[1] += size<<1;
    spu2Ru16(REG_C1_SPUSTAT)&=~0x80;
    SPUStartCycle[1] = SPUCycles;
    SPUTargetCycle[1] = size;
    interrupt |= (1<<2);
}

void CALLBACK SPU2interruptDMA4()
{
	SPU2_LOG("SPU2 interruptDMA4\n");

//	spu2Rs16(REG_C0_CTRL)&= ~0x30;
//	spu2Rs16(REG__1B0) = 0;
//	spu2Rs16(SPU2_STATX_WRDY_M)|= 0x80;
    spu2Rs16(REG_C0_CTRL)&=~0x30;
	spu2Ru16(REG_C0_SPUSTAT)|=0x80;
}

void CALLBACK SPU2interruptDMA7()
{
	SPU2_LOG("SPU2 interruptDMA7\n");

//	spu2Rs16(REG_C1_CTRL)&= ~0x30;
//	//spu2Rs16(REG__5B0) = 0;
//	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;
    spu2Rs16(REG_C1_CTRL)&=~0x30;
	spu2Ru16(REG_C1_SPUSTAT)|=0x80;
}

// turn channels on
void SoundOn(int start,int end,unsigned short val)     // SOUND ON PSX COMAND
{
    for(int ch=start;ch<end;ch++,val>>=1)                     // loop channels
    {
        if((val&1) && voices[ch].pStart)                    // mmm... start has to be set before key on !?!
        {
            voices[ch].bNew=true;
            voices[ch].bIgnoreLoop = false;
            dwNewChannel2[ch/24]|=(1<<(ch%24));                  // clear end channel bit
        }
    }
}

// turn channels off
void SoundOff(int start,int end,unsigned short val)    // SOUND OFF PSX COMMAND
{
    for(int ch=start;ch<end;ch++,val>>=1) {                     // loop channels
        if(val&1)                                           // && s_chan[i].bOn)  mmm...
            voices[ch].bStop=true;                                               
    }
}

void FModOn(int start,int end,unsigned short val)      // FMOD ON PSX COMMAND
{
    int ch;

    for(ch=start;ch<end;ch++,val>>=1) {                     // loop channels
        if(val&1)  {                                          // -> fmod on/off
            if(ch>0) {
                voices[ch].bFMod=1;                             // --> sound channel
                voices[ch-1].bFMod=2;                           // --> freq channel
            }
        }
        else
            voices[ch].bFMod=0;                               // --> turn off fmod
    }
}

void VolumeOn(int start,int end,unsigned short val,int iRight)  // VOLUME ON PSX COMMAND
{
    int ch;
    
    for(ch=start;ch<end;ch++,val>>=1) {                    // loop channels
        if(val&1) {                                           // -> reverb on/off
            
            if(iRight) voices[ch].bVolumeR=1;
            else       voices[ch].bVolumeL=1;
        }
        else {
            if(iRight) voices[ch].bVolumeR=0;
            else       voices[ch].bVolumeL=0;
        }
    }
}

void CALLBACK SPU2write(u32 mem, u16 value)
{
    u32 spuaddr;
	SPU2_LOG("SPU2 write mem %x value %x\n", mem, value);

    assert( C0_SPUADDR < 0x100000);
    assert( C1_SPUADDR < 0x100000);

    spu2Ru16(mem) = value;
    u32 r = mem&0xffff;

    // channel info
    if((r>=0x0000 && r<0x0180)||(r>=0x0400 && r<0x0580))  // some channel info?
    {
        int ch=0;
        if(r>=0x400) ch=((r-0x400)>>4)+24;
        else ch=(r>>4);

        VOICE_PROCESSED* pvoice = &voices[ch];

        switch(r&0x0f)
        {
            case 0:
            case 2:
                pvoice->SetVolume(mem&0x2);
                break;
            case 4:
            {
                int NP;
                if(value>0x3fff) NP=0x3fff;                             // get pitch val
                else           NP=value;

                pvoice->pvoice->pitch = NP;

                NP=(48000L*NP)/4096L;                                 // calc frequency
                if(NP<1) NP=1;                                        // some security
                pvoice->iActFreq=NP;                               // store frequency
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
    if((r>=0x01c0 && r<0x02E0)||(r>=0x05c0 && r<0x06E0))
    {
        int ch=0;
        unsigned long rx=r;
        if(rx>=0x400)
        {
            ch=24;
            rx-=0x400;
        }

        ch+=((rx-0x1c0)/12);
        rx-=(ch%24)*12;
        VOICE_PROCESSED* pvoice = &voices[ch];

        switch(rx)
        {
            case 0x1C0:
                pvoice->iStartAddr=(((unsigned long)value&0x3f)<<16)|(pvoice->iStartAddr&0xFFFF);
                pvoice->pStart=(u8*)(spu2mem+pvoice->iStartAddr);
                break;
            case 0x1C2:
                pvoice->iStartAddr=(pvoice->iStartAddr & 0x3f0000) | (value & 0xFFFF);
                pvoice->pStart=(u8*)(spu2mem+pvoice->iStartAddr);
                break;
            case 0x1C4:
                pvoice->iLoopAddr =(((unsigned long)value&0x3f)<<16)|(pvoice->iLoopAddr&0xFFFF);
                pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
                pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
                break;
            case 0x1C6:
                pvoice->iLoopAddr=(pvoice->iLoopAddr& 0x3f0000) | (value & 0xFFFF);
                pvoice->pLoop=(u8*)(spu2mem+pvoice->iLoopAddr);
                pvoice->bIgnoreLoop=pvoice->iLoopAddr>0;
                break;
            case 0x1C8:
                // unused... check if it gets written as well
                pvoice->iNextAddr=(((unsigned long)value&0x3f)<<16)|(pvoice->iNextAddr&0xFFFF);
                break;
            case 0x1CA:
                // unused... check if it gets written as well
                pvoice->iNextAddr=(pvoice->iNextAddr & 0x3f0000) | (value & 0xFFFF);
                break;
        }

        return;
    }

    // process non-channel data
    switch(mem&0xffff) {
        case REG_C0_SPUDATA:
            spuaddr = C0_SPUADDR;
            spu2mem[spuaddr] = value;
            spuaddr++;
            if( (spu2Ru16(REG_C0_CTRL)&0x40) && C0_IRQA == spuaddr){
                //spu2Ru16(SPDIF_OUT) |= 0x4;
                IRQINFO |= 4;
                SPU2_LOG("SPU2write:C0_CPUDATA interrupt\n");
                irqCallbackSPU2();
            }
            if(spuaddr>0xFFFFE)
                spuaddr = 0x2800;
            C0_SPUADDR_SET(spuaddr);
            spu2Ru16(REG_C0_SPUSTAT)&=~0x80;
            spu2Ru16(REG_C0_CTRL)&=~0x30;
            break;
        case REG_C1_SPUDATA:
            spuaddr = C1_SPUADDR;
            spu2mem[spuaddr] = value;
            spuaddr++;
            if( (spu2Ru16(REG_C1_CTRL)&0x40) && C1_IRQA == spuaddr){
                //spu2Ru16(SPDIF_OUT) |= 0x8;
                IRQINFO |= 8;
                SPU2_LOG("SPU2write:C1_CPUDATA interrupt\n");
                irqCallbackSPU2();
            }
            if(spuaddr>0xFFFFE)
                spuaddr = 0x2800;
            C1_SPUADDR_SET(spuaddr);
            spu2Ru16(REG_C1_SPUSTAT)&=~0x80;
            spu2Ru16(REG_C1_CTRL)&=~0x30;
            break;
        case REG_C0_IRQA_HI:
        case REG_C0_IRQA_LO:
            pSpuIrq[0]=spu2mem+C0_IRQA;
            break;
        case REG_C1_IRQA_HI:
        case REG_C1_IRQA_LO:
            pSpuIrq[1]=spu2mem+C1_IRQA;
            break;

        case REG_C0_SPUADDR_HI:
        case REG_C1_SPUADDR_HI:
            spu2Ru16(mem) = value&0xf;
            break;

        case REG_C0_CTRL:
            spu2Ru16(mem) = value;
            // clear interrupt
            if( !(value & 0x40) )
                IRQINFO &= ~0x4;
            break;
        case REG_C1_CTRL:
            spu2Ru16(mem) = value;
            // clear interrupt
            if( !(value & 0x40) )
                IRQINFO &= ~0x8;
            break;
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

    assert( C0_SPUADDR < 0x100000);
    assert( C1_SPUADDR < 0x100000);
}

u16  CALLBACK SPU2read(u32 mem)
{
    u32 spuaddr;
	u16 ret;
    u32 r = mem&0xffff;

    if((r>=0x0000 && r<0x0180)||(r>=0x0400 && r<0x0580))  // some channel info?
    {
        int ch=0;
        if(r>=0x400) ch=((r-0x400)>>4)+24;
        else ch=(r>>4);

        VOICE_PROCESSED* pvoice = &voices[ch];
                
        switch(r&0x0f) {
            case 10:
//                if( pvoice->bNew ) return 1;
//                if( pvoice->ADSRX.lVolume && !pvoice->ADSRX.EnvelopeVol ) return 1;
                return (unsigned short)(pvoice->ADSRX.EnvelopeVol>>16);
        }
    }

    if((r>=0x01c0 && r<0x02E0)||(r>=0x05c0 && r<0x06E0))  // some channel info?
    {
        int ch=0;
        unsigned long rx=r;
        if(rx>=0x400)
        {
            ch=24;
            rx-=0x400;
        }

        ch+=((rx-0x1c0)/12);
        rx-=(ch%24)*12;
        VOICE_PROCESSED* pvoice = &voices[ch];

        switch(rx) {
            case 0x1C0:
                ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>17)&0x3F);
                break;
            case 0x1C2:
                ret = ((((uptr)pvoice->pStart-(uptr)spu2mem)>>1)&0xFFFF);
                break;
            case 0x1C4:
                ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>17)&0x3F);
                break;
            case 0x1C6:
                ret = ((((uptr)pvoice->pLoop-(uptr)spu2mem)>>1)&0xFFFF);
                break;
            case 0x1C8:
                ret = ((((uptr)pvoice->pCurr-(uptr)spu2mem)>>17)&0x3F);
                break;
            case 0x1CA:
                ret = ((((uptr)pvoice->pCurr-(uptr)spu2mem)>>1)&0xFFFF);
                break;
        }

	    SPU2_LOG("SPU2 channel read mem %x: %x\n", mem, ret);
        return ret;
    }

	switch(mem&0xffff) {
        case REG_C0_SPUDATA:
            spuaddr = C0_SPUADDR;
            ret =spu2mem[spuaddr];
            spuaddr++;
            if(spuaddr>0xfffff)
                spuaddr=0;
            C0_SPUADDR_SET(spuaddr);
            break;
        case REG_C1_SPUDATA:
            spuaddr = C1_SPUADDR;
            ret = spu2mem[spuaddr];
            spuaddr++;
            if(spuaddr>0xfffff)
                spuaddr=0;
            C1_SPUADDR_SET(spuaddr);
            break;

        case REG_C0_END1: ret = (dwEndChannel2[0]&0xffff); break;
        case REG_C0_END2: ret = (dwEndChannel2[0]>>16); break;
        case REG_C1_END1: ret = (dwEndChannel2[1]&0xffff); break;
        case REG_C1_END2: ret = (dwEndChannel2[1]>>16); break;

        case REG_IRQINFO: 
            ret = IRQINFO;
            //IRQINFO = 0; // clear once done
            break;

        default:
			ret = spu2Ru16(mem);
	}

	SPU2_LOG("SPU2 read mem %x: %x\n", mem, ret);

	return ret;
}

void CALLBACK SPU2WriteMemAddr(int core, u32 value)
{
	MemAddr[core] = g_pDMABaseAddr+value;
}

u32 CALLBACK SPU2ReadMemAddr(int core)
{
    return MemAddr[core]-g_pDMABaseAddr;
}

void CALLBACK SPU2setDMABaseAddr(uptr baseaddr)
{
    g_pDMABaseAddr = baseaddr;
}

void CALLBACK SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)())
{
    irqCallbackSPU2 = SPU2callback;
    irqCallbackDMA4 = DMA4callback;
    irqCallbackDMA7 = DMA7callback;
}

// VOICE_PROCESSED definitions
SPU_CONTROL_* VOICE_PROCESSED::GetCtrl()
{
    return ((SPU_CONTROL_*)(spu2regs+memoffset+REG_C0_CTRL));
}

void VOICE_PROCESSED::SetVolume(int iProcessRight)
{
    u16 vol = iProcessRight ? pvoice->right.word : pvoice->left.word;

    if(vol&0x8000) // sweep not working
    {
        short sInc=1;                                       // -> sweep up?
        if(vol&0x2000) sInc=-1;                             // -> or down?
        if(vol&0x1000) vol^=0xffff;                         // -> mmm... phase inverted? have to investigate this
        vol=((vol&0x7f)+1)/2;                               // -> sweep: 0..127 -> 0..64
        vol+=vol/(2*sInc);                                  // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
        vol*=128;
    }
    else                                                  // no sweep:
    {
        if(vol&0x4000)                                      // -> mmm... phase inverted? have to investigate this
        vol=0x3fff-(vol&0x3fff);
    }

    if( iProcessRight )
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
                            
    if(bReverb && GetCtrl()->reverb )
    {
        // setup the reverb effects
    }

    pCurr=pStart;   // set sample start
                            
    s_1=0;          // init mixing vars
    s_2=0;
    iSBPos=28;

    bNew=false;         // init channel flags
    bStop=false;
    bOn=true;
    SB[29]=0;       // init our interpolation helpers
    SB[30]=0;

    spos=0x10000L;
    SB[31]=0;
}

void VOICE_PROCESSED::VoiceChangeFrequency()
{
    iUsedFreq=iActFreq;               // -> take it and calc steps
    sinc=(u32)pvoice->pitch<<4;
    if(!sinc)
        sinc=1;
    
    // -> freq change in simle imterpolation mode: set flag
    SB[32]=1;
}

void VOICE_PROCESSED::InterpolateUp()
{
    if(SB[32]==1)                               // flag == 1? calc step and set flag... and don't change the value in this pass
    {
        const int id1=SB[30]-SB[29];    // curr delta to next val
        const int id2=SB[31]-SB[30];    // and next delta to next-next val :)

        SB[32]=0;

        if(id1>0)                                           // curr delta positive
        {
            if(id2<id1)
            {SB[28]=id1;SB[32]=2;}
            else
            if(id2<(id1<<1))
            SB[28]=(id1*sinc)/0x10000L;
            else
            SB[28]=(id1*sinc)/0x20000L; 
        }
        else                                                // curr delta negative
        {
            if(id2>id1)
            {SB[28]=id1;SB[32]=2;}
            else
            if(id2>(id1<<1))
            SB[28]=(id1*sinc)/0x10000L;
            else
            SB[28]=(id1*sinc)/0x20000L; 
        }
    }
    else if(SB[32]==2)                               // flag 1: calc step and set flag... and don't change the value in this pass
    {
        SB[32]=0;

        SB[28]=(SB[28]*sinc)/0x20000L;
        if(sinc<=0x8000)
            SB[29]=SB[30]-(SB[28]*((0x10000/sinc)-1));
        else
            SB[29]+=SB[28];
    }
    else                                                  // no flags? add bigger val (if possible), calc smaller step, set flag1
        SB[29]+=SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

void VOICE_PROCESSED::InterpolateDown()
{
    if(sinc>=0x20000L)                                // we would skip at least one val?
    {
        SB[29]+=(SB[30]-SB[29])/2;  // add easy weight
        if(sinc>=0x30000L)                              // we would skip even more vals?
            SB[29]+=(SB[31]-SB[30])/2; // add additional next weight
    }
}

void VOICE_PROCESSED::FModChangeFrequency(int ns)
{
    int NP=pvoice->pitch;

    NP=((32768L+iFMod[ns])*NP)/32768L;

    if(NP>0x3fff) NP=0x3fff;
    if(NP<0x1)    NP=0x1;

    NP=(48000L*NP)/(4096L);                               // calc frequency

    iActFreq=NP;
    iUsedFreq=NP;
    sinc=(((NP/10)<<16)/4800);
    if(!sinc)
        sinc=1;

    // freq change in simple interpolation mode
    SB[32]=1;

    iFMod[ns]=0;
}

// noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff
int VOICE_PROCESSED::iGetNoiseVal()
{
    int fa;

    if((dwNoiseVal<<=1)&0x80000000L)
    {
        dwNoiseVal^=0x0040001L;
        fa=((dwNoiseVal>>2)&0x7fff);
        fa=-fa;
    }
    else
        fa=(dwNoiseVal>>2)&0x7fff;

    // mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
    fa=iOldNoise+((fa-iOldNoise)/((0x001f-(GetCtrl()->noiseFreq))+1));
    if(fa>32767L)
        fa=32767L;
    if(fa<-32767L)
        fa=-32767L;

    iOldNoise=fa;
    SB[29] = fa;                               // -> store noise val in "current sample" slot
    return fa;
}                                 

void VOICE_PROCESSED::StoreInterpolationVal(int fa)
{
    if(bFMod==2)                                // fmod freq channel
        SB[29]=fa;
    else
    {
        if(!GetCtrl()->spuUnmute)
            fa=0;                       // muted?
        else                                                // else adjust
        {
            if(fa>32767L)
                fa=32767L;
            if(fa<-32767L)
                fa=-32767L;              
        }

        SB[28] = 0;                    
        SB[29] = SB[30];              // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
        SB[30] = SB[31];
        SB[31] = fa;
        SB[32] = 1;                             // -> flag: calc new interolation
    }
}

int VOICE_PROCESSED::iGetInterpolationVal()
{
    int fa;
    if(bFMod==2)
        return SB[29];

    if(sinc<0x10000L)                       // -> upsampling?
        InterpolateUp();                     // --> interpolate up
    else InterpolateDown();                   // --> else down
    fa=SB[29];
    
    return fa;
}

void VOICE_PROCESSED::Stop()
{
}


s32 CALLBACK SPU2test()
{
	return 0;
}

// size is in bytes
void LogPacketSound(void* packet, int memsize)
{
    u16 buf[28];

    u8* pstart = (u8*)packet;
    int s_1 = 0;
    int s_2=0;
    for(int i = 0; i < memsize; i += 16) {
        int predict_nr=(int)pstart[0];
        int shift_factor=predict_nr&0xf;
        predict_nr >>= 4;
        int flags=(int)pstart[1];
        pstart += 2;

        for(int nSample=0;nSample<28; ++pstart)
        {
            int d=(int)*pstart;
            int s=((d&0xf)<<12);
            if(s&0x8000) s|=0xffff0000;

            int fa=(s >> shift_factor);
            fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
            s_2=s_1;s_1=fa;
            s=((d & 0xf0) << 8);

            buf[nSample++]=fa;

            if(s&0x8000) s|=0xffff0000;
            fa=(s>>shift_factor);              
            fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
            s_2=s_1;s_1=fa;

            buf[nSample++]=fa;
        }

        LogRawSound(buf, 2, buf, 2, 28);
    }
}

#define RECORD_FILENAME "zerospu2.wav"

void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples)
{
//#ifdef _DEBUG
//    if( g_fLogSound == NULL ) {
//        g_fLogSound = fopen("rawsndbuf.pcm", "wb");
//        if( g_fLogSound == NULL )
//            return;
//    }
    if( g_pWavRecord == NULL ) {
        g_pWavRecord = new WavOutFile(RECORD_FILENAME, 48000, 16, 2);
    }

    u8* left = (u8*)pleft;
    u8* right = (u8*)pright;
    static vector<s16> tempbuf;

    tempbuf.resize(2*numsamples);

    for(int i = 0; i < numsamples; ++i) {
        tempbuf[2*i+0] = *(s16*)left;
        tempbuf[2*i+1] = *(s16*)right;
        left += leftstride;
        right += rightstride;
    }

    g_pWavRecord->write(&tempbuf[0], numsamples*2);

    //fwrite(&tempbuf[0], 4*numsamples, 1, g_fLogSound);
//#endif
}

int CALLBACK SPU2setupRecording(int start, void* pData)
{
    if( start ) {
        conf.options |= OPTION_RECORDING;
        printf("ZeroSPU2: started recording at %s\n", RECORD_FILENAME);
    }
    else {
        conf.options &= ~OPTION_RECORDING;
        printf("ZeroSPU2: stopped recording\n");
    }
    
    return 1;
}

struct SPU2freezeData
{
    u32 version;
	u8 spu2regs[0x10000];
    u8 spu2mem[0x200000];
    u16 interrupt;
    int nSpuIrq[2];
    u32 dwNewChannel2[2], dwEndChannel2[2];
    u32 dwNoiseVal;
    int iFMod[NSSIZE];
    u32 MemAddr[2];
    ADMA adma[2];
    u32 Adma4MemAddr, Adma7MemAddr;

    int SPUCycles, SPUWorkerCycles;
    int SPUStartCycle[2];
    int SPUTargetCycle[2];

    int voicesize;
    VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1];
};

s32  CALLBACK SPU2freeze(int mode, freezeData *data)
{
	SPU2freezeData *spud;
    int i;
    assert( g_pDMABaseAddr != 0 );

	if (mode == FREEZE_LOAD) {
		spud = (SPU2freezeData*)data->data; 
        if( spud->version != 0x70000001 ) {
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

        for(i = 0; i < ARRAYSIZE(voices); ++i) {
            memcpy(&voices[i], &spud->voices[i], min((int)SPU_VOICE_STATE_SIZE, spud->voicesize));
            voices[i].pStart = (u8*)((uptr)spud->voices[i].pStart+(uptr)spu2mem);
            voices[i].pLoop = (u8*)((uptr)spud->voices[i].pLoop+(uptr)spu2mem);
            voices[i].pCurr = (u8*)((uptr)spud->voices[i].pCurr+(uptr)spu2mem);
        }

        //conf.Log = 1;
        s_GlobalTimeStamp = 0;
        g_startcount=0xffffffff;
        for(int i = 0; i < ARRAYSIZE(s_nDurations); ++i) {
            s_nDurations[i] = NSFRAMES*1000;
        }
        s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
        s_nCurDuration = 0;
        s_nQueuedBuffers = 0;
        s_nDropPacket = 0;
	}
    else if (mode == FREEZE_SAVE) {
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

//        if( conf.options & OPTION_REALTIME )
//            SPUWorkerCycles = SPUCycles;
//        else
        spud->SPUWorkerCycles = SPUWorkerCycles;
        memcpy(spud->SPUStartCycle, SPUStartCycle, sizeof(SPUStartCycle));
        memcpy(spud->SPUTargetCycle, SPUTargetCycle, sizeof(SPUTargetCycle));

        for(i = 0; i < ARRAYSIZE(s_nDurations); ++i) {
            s_nDurations[i] = NSFRAMES*1000;
        }
        s_nTotalDuration = ARRAYSIZE(s_nDurations)*NSFRAMES*1000;
        s_nCurDuration = 0;

        spud->voicesize = SPU_VOICE_STATE_SIZE;
        for(i = 0; i < ARRAYSIZE(voices); ++i) {
            memcpy(&spud->voices[i], &voices[i], SPU_VOICE_STATE_SIZE);
            spud->voices[i].pStart = (u8*)((uptr)voices[i].pStart-(uptr)spu2mem);
            spud->voices[i].pLoop = (u8*)((uptr)voices[i].pLoop-(uptr)spu2mem);
            spud->voices[i].pCurr = (u8*)((uptr)voices[i].pCurr-(uptr)spu2mem);
        }

        g_startcount=0xffffffff;
        s_GlobalTimeStamp = 0;
        s_nDropPacket = 0;
	}
    else if (mode == FREEZE_SIZE) {
		data->size = sizeof(SPU2freezeData);
	}

//    if( conf.options & OPTION_REALTIME )
//        SPUWorkerCycles = timeGetTime();
        
	return 0;
}
