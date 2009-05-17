/*  SPU2null
 *  Copyright (C) 2002-2005  SPU2null Team
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

#include "SPU2.h"

#include <assert.h>
#include <stdlib.h>
#include <string>
using namespace std;
 
const u8 version  = PS2E_SPU2_VERSION;
const u8 revision = 0;
const u8 build    = 8;    // increase that with each version
const u32 minor = 0;    // increase that with each version

// ADSR constants
#define ATTACK_MS      494L
#define DECAYHALF_MS   286L
#define DECAY_MS       572L
#define SUSTAIN_MS     441L
#define RELEASE_MS     437L

#ifdef _DEBUG
char *libraryName      = "SPU2null (Debug)";
#else
char *libraryName      = "SPU2null ";
#endif
string s_strIniPath="inis/SPU2null.ini";

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
u32 dwEndChannel2[2] = {0}; // keeps track of what channels have ended
u32 dwNoiseVal = 1;                        // global noise generator

s32 SPUCycles = 0, SPUWorkerCycles = 0;
s32 SPUStartCycle[2];
s32 SPUTargetCycle[2];

int ADMAS4Write();
int ADMAS7Write();

void InitADSR();

void (*irqCallbackSPU2)();                  // func of main emu, called on spu irq
void (*irqCallbackDMA4)() = 0;                // func of main emu, called on spu irq
void (*irqCallbackDMA7)() = 0;                // func of main emu, called on spu irq

const s32 f[5][2] = {   
	{    0,  0  },
	{   60,  0  },
	{  115, -52 },
	{   98, -55 },
	{  122, -60 }
};

u32 RateTable[160];

// channels and voices
VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1]; // +1 for modulation

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_SPU2;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build | (minor << 24);
}

void __Log(char *fmt, ...)
{
	va_list list;

	if (!conf.Log || spu2Log == NULL) return;

	va_start(list, fmt);
	vfprintf(spu2Log, fmt, list);
	va_end(list);
}

EXPORT_C_(s32) SPU2init()
{
#ifdef SPU2_LOG
	spu2Log = fopen("logs/spu2.txt", "w");
	if (spu2Log) setvbuf(spu2Log, NULL,  _IONBF, 0);
	SPU2_LOG("Spu2 null version %d,%d\n", revision, build);
	SPU2_LOG("SPU2init\n");
#endif
	spu2regs = (s8*)malloc(0x10000);
	if (spu2regs == NULL)
	{
		SysMessage("Error allocating Memory\n");
		return -1;
	}
	memset(spu2regs, 0, 0x10000);

	spu2mem = (u16*)malloc(0x200000); // 2Mb
	if (spu2mem == NULL)
	{
		SysMessage("Error allocating Memory\n");
		return -1;
	}
	memset(spu2mem, 0, 0x200000);
	memset(dwEndChannel2, 0, sizeof(dwEndChannel2));

	InitADSR();

	memset(voices, 0, sizeof(voices));
	// last 24 channels have higher mem offset
	for (int i = 0; i < 24; ++i)
		voices[i+24].memoffset = 0x400;

	// init each channel
	for (u32 i = 0; i < ArraySize(voices); ++i)
	{

		voices[i].pLoop = voices[i].pStart = voices[i].pCurr = (u8*)spu2mem;

		voices[i].pvoice = (_SPU_VOICE*)((u8*)spu2regs + voices[i].memoffset) + (i % 24);
		voices[i].ADSRX.SustainLevel = 1024;                // -> init sustain
	}

	return 0;
}

EXPORT_C_(s32) SPU2open(void *pDsp)
{
	LoadConfig();
	SPUCycles = SPUWorkerCycles = 0;
	interrupt = 0;
	SPUStartCycle[0] = SPUStartCycle[1] = 0;
	SPUTargetCycle[0] = SPUTargetCycle[1] = 0;
	g_nSpuInit = 1;
	return 0;
}

EXPORT_C_(void) SPU2close()
{
	g_nSpuInit = 0;
}

EXPORT_C_(void) SPU2shutdown()
{
	free(spu2regs);
	spu2regs = NULL;
	free(spu2mem);
	spu2mem = NULL;
#ifdef SPU2_LOG
	if (spu2Log) fclose(spu2Log);
#endif
}

// simulate SPU2 for 1ms
void SPU2Worker();

#define CYCLES_PER_MS (36864000/1000)

EXPORT_C_(void) SPU2async(u32 cycle)
{
	SPUCycles += cycle;
	if (interrupt & (1 << 2))
	{
		if (SPUCycles - SPUStartCycle[1] >= SPUTargetCycle[1])
		{
			interrupt &= ~(1 << 2);
			irqCallbackDMA7();
		}

	}

	if (interrupt & (1 << 1))
	{
		if (SPUCycles - SPUStartCycle[0] >= SPUTargetCycle[0])
		{
			interrupt &= ~(1 << 1);
			irqCallbackDMA4();
		}
	}

	if (g_nSpuInit)
	{

		while (SPUCycles - SPUWorkerCycles > 0 && CYCLES_PER_MS < SPUCycles - SPUWorkerCycles)
		{
			SPU2Worker();
			SPUWorkerCycles += CYCLES_PER_MS;
		}
	}
}

void InitADSR()                                    // INIT ADSR
{
	u32 r, rs, rd;
	s32 i;
	memset(RateTable, 0, sizeof(u32)*160);      // build the rate table according to Neill's rules (see at bottom of file)

	r = 3;
	rs = 1;
	rd = 0;

	for (i = 32;i < 160;i++)                              // we start at pos 32 with the real values... everything before is 0
	{
		if (r < 0x3FFFFFFF)
		{
			r += rs;
			rd++;
			if (rd == 5)
			{
				rd = 1;
				rs *= 2;
			}
		}
		if (r > 0x3FFFFFFF) r = 0x3FFFFFFF;

		RateTable[i] = r;
	}
}

int MixADSR(VOICE_PROCESSED* pvoice)                             // MIX ADSR
{
	if (pvoice->bStop)                                 // should be stopped:
	{
		if (pvoice->bIgnoreLoop == 0)
		{
			pvoice->ADSRX.EnvelopeVol = 0;
			pvoice->bOn = false;
			pvoice->pStart = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->pLoop = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->pCurr = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->bStop = true;
			pvoice->bIgnoreLoop = false;
			return 0;
		}
		if (pvoice->ADSRX.ReleaseModeExp)// do release
		{
			switch ((pvoice->ADSRX.EnvelopeVol >> 28)&0x7)
			{
				case 0:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +0 + 32];
					break;
				case 1:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +4 + 32];
					break;
				case 2:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +6 + 32];
					break;
				case 3:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +8 + 32];
					break;
				case 4:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +9 + 32];
					break;
				case 5:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +10+ 32];
					break;
				case 6:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +11+ 32];
					break;
				case 7:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x18 +12+ 32];
					break;
			}
		}
		else
		{
			pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.ReleaseRate^0x1F))-0x0C + 32];
		}

		if (pvoice->ADSRX.EnvelopeVol < 0)
		{
			pvoice->ADSRX.EnvelopeVol = 0;
			pvoice->bOn = false;
			pvoice->pStart = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->pLoop = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->pCurr = (u8*)(spu2mem + pvoice->iStartAddr);
			pvoice->bStop = true;
			pvoice->bIgnoreLoop = false;
			//pvoice->bReverb=0;
			//pvoice->bNoise=0;
		}

		pvoice->ADSRX.lVolume = pvoice->ADSRX.EnvelopeVol >> 21;
		pvoice->ADSRX.lVolume = pvoice->ADSRX.EnvelopeVol >> 21;
		return pvoice->ADSRX.lVolume;
	}
	else                                                  // not stopped yet?
	{
		if (pvoice->ADSRX.State == 0)                    // -> attack
		{
			if (pvoice->ADSRX.AttackModeExp)
			{
				if (pvoice->ADSRX.EnvelopeVol < 0x60000000)
					pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x10 + 32];
				else
					pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x18 + 32];
			}
			else
			{
				pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.AttackRate^0x7F)-0x10 + 32];
			}

			if (pvoice->ADSRX.EnvelopeVol < 0)
			{
				pvoice->ADSRX.EnvelopeVol = 0x7FFFFFFF;
				pvoice->ADSRX.State = 1;
			}

			pvoice->ADSRX.lVolume = pvoice->ADSRX.EnvelopeVol >> 21;
			return pvoice->ADSRX.lVolume;
		}
		//--------------------------------------------------//
		if (pvoice->ADSRX.State == 1)                    // -> decay
		{
			switch ((pvoice->ADSRX.EnvelopeVol >> 28)&0x7)
			{
				case 0:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+0 + 32];
					break;
				case 1:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+4 + 32];
					break;
				case 2:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+6 + 32];
					break;
				case 3:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+8 + 32];
					break;
				case 4:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+9 + 32];
					break;
				case 5:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+10+ 32];
					break;
				case 6:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+11+ 32];
					break;
				case 7:
					pvoice->ADSRX.EnvelopeVol -= RateTable[(4*(pvoice->ADSRX.DecayRate^0x1F))-0x18+12+ 32];
					break;
			}

			if (pvoice->ADSRX.EnvelopeVol < 0) pvoice->ADSRX.EnvelopeVol = 0;
			if (((pvoice->ADSRX.EnvelopeVol >> 27)&0xF) <= pvoice->ADSRX.SustainLevel)
			{
				pvoice->ADSRX.State = 2;
			}

			pvoice->ADSRX.lVolume = pvoice->ADSRX.EnvelopeVol >> 21;
			return pvoice->ADSRX.lVolume;
		}
		//--------------------------------------------------//
		if (pvoice->ADSRX.State == 2)                    // -> sustain
		{
			if (pvoice->ADSRX.SustainIncrease)
			{
				if (pvoice->ADSRX.SustainModeExp)
				{
					if (pvoice->ADSRX.EnvelopeVol < 0x60000000)
						pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x10 + 32];
					else
						pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x18 + 32];
				}
				else
				{
					pvoice->ADSRX.EnvelopeVol += RateTable[(pvoice->ADSRX.SustainRate^0x7F)-0x10 + 32];
				}

				if (pvoice->ADSRX.EnvelopeVol < 0)
				{
					pvoice->ADSRX.EnvelopeVol = 0x7FFFFFFF;
				}
			}
			else
			{
				if (pvoice->ADSRX.SustainModeExp)
				{
					switch ((pvoice->ADSRX.EnvelopeVol >> 28)&0x7)
					{
						case 0:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +0 + 32];
							break;
						case 1:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +4 + 32];
							break;
						case 2:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +6 + 32];
							break;
						case 3:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +8 + 32];
							break;
						case 4:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +9 + 32];
							break;
						case 5:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +10+ 32];
							break;
						case 6:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +11+ 32];
							break;
						case 7:
							pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x1B +12+ 32];
							break;
					}
				}
				else
				{
					pvoice->ADSRX.EnvelopeVol -= RateTable[((pvoice->ADSRX.SustainRate^0x7F))-0x0F + 32];
				}

				if (pvoice->ADSRX.EnvelopeVol < 0)
				{
					pvoice->ADSRX.EnvelopeVol = 0;
				}
			}
			pvoice->ADSRX.lVolume = pvoice->ADSRX.EnvelopeVol >> 21;
			return pvoice->ADSRX.lVolume;
		}
	}
	return 0;
}

// simulate SPU2 for 1ms
void SPU2Worker()
{
	u8* start;
	int ch, flags;

	VOICE_PROCESSED* pChannel = voices;
	for (ch = 0;ch < SPU_NUMBER_VOICES;ch++, pChannel++)        // loop em all... we will collect 1 ms of sound of each playing channel
	{
		if (pChannel->bNew)
		{
			pChannel->StartSound();                         // start new sound
			dwEndChannel2[ch/24] &= ~(1 << (ch % 24));            // clear end channel bit
		}

		if (!pChannel->bOn)
		{
			// fill buffer with empty data
			continue;
		}

		if (pChannel->iActFreq != pChannel->iUsedFreq)  // new psx frequency?
			pChannel->VoiceChangeFrequency();

		// loop until 1 ms of data is reached
		int ns = 0;
		while (ns < NSSIZE)
		{
			while (pChannel->spos >= 0x10000)
			{
				if (pChannel->iSBPos == 28)         // 28 reached?
				{
					start = pChannel->pCurr;        // set up the current pos

					// special "stop" sign
					if (start == (u8*) - 1) //!pChannel->bOn
					{
						pChannel->bOn = false;                      // -> turn everything off
						pChannel->ADSRX.lVolume = 0;
						pChannel->ADSRX.EnvelopeVol = 0;
						goto ENDX;                              // -> and done for this channel
					}

					pChannel->iSBPos = 0;

					// decode the 16 byte packet

					flags = (int)start[1];
					start += 16;

					// some callback and irq active?
					if (pChannel->GetCtrl()->irq)
					{
						// if irq address reached or irq on looping addr, when stop/loop flag is set
						u8* pirq = (u8*)pSpuIrq[ch>=24];
						if ((pirq > start - 16  && pirq <= start)
						        || ((flags&1) && (pirq >  pChannel->pLoop - 16 && pirq <= pChannel->pLoop)))
						{
							IRQINFO |= 4 << (int)(ch >= 24);
							irqCallbackSPU2();
						}
					}

					// flag handler
					if ((flags&4) && (!pChannel->bIgnoreLoop))
						pChannel->pLoop = start - 16;            // loop adress

					if (flags&1)                              // 1: stop/loop
					{
						// We play this block out first...
						dwEndChannel2[ch/24] |= (1 << (ch % 24));
						//if(!(flags&2))                          // 1+2: do loop... otherwise: stop
						if (flags != 3 || pChannel->pLoop == NULL)   // PETE: if we don't check exactly for 3, loop hang ups will happen (DQ4, for example)
						{                                      // and checking if pLoop is set avoids crashes, yeah
							start = (u8*) - 1;
							pChannel->bStop = true;
							pChannel->bIgnoreLoop = false;
						}
						else
						{
							start = pChannel->pLoop;
						}
					}

					pChannel->pCurr = start;                  // store values for next cycle
				}

				pChannel->iSBPos++;        // get sample data
				pChannel->spos -= 0x10000;
			}

			MixADSR(pChannel);

			// go to the next packet
			ns++;
			pChannel->spos += pChannel->sinc;
		}
ENDX:
		;
	}

	// mix all channels
	if ((spu2Ru16(REG_C0_MMIX) & 0xC0) && (spu2Ru16(REG_C0_ADMAS) & 0x1) && !(spu2Ru16(REG_C0_CTRL) & 0x30))
	{
		for (int ns = 0;ns < NSSIZE;ns++)
		{
			Adma4.Index += 1;

			if (Adma4.Index == 128 || Adma4.Index == 384)
			{
				if (ADMAS4Write())
				{
					spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;
					irqCallbackDMA4();
				}
				else MemAddr[0] += 1024;
			}

			if (Adma4.Index == 512)
			{
				Adma4.Index = 0;
			}
		}
	}


	if ((spu2Ru16(REG_C1_MMIX) & 0xC0) && (spu2Ru16(REG_C1_ADMAS) & 0x2) && !(spu2Ru16(REG_C1_CTRL) & 0x30))
	{
		for (int ns = 0;ns < NSSIZE;ns++)
		{
			Adma7.Index += 1;

			if (Adma7.Index == 128 || Adma7.Index == 384)
			{
				if (ADMAS7Write())
				{
					spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;
					irqCallbackDMA7();
				}
				else MemAddr[1] += 1024;
			}

			if (Adma7.Index == 512)	Adma7.Index = 0;
		}
	}
}

EXPORT_C_(void) SPU2readDMA4Mem(u16 *pMem, int size)
{
	u32 spuaddr = C0_SPUADDR;
	int i;

	SPU2_LOG("SPU2 readDMA4Mem size %x, addr: %x\n", size, pMem);

	for (i = 0;i < size;i++)
	{
		*pMem++ = *(u16*)(spu2mem + spuaddr);
		if ((spu2Rs16(REG_C0_CTRL)&0x40) && C0_IRQA == spuaddr)
		{
			spu2Ru16(SPDIF_OUT) |= 0x4;
			C0_SPUADDR_SET(spuaddr);
			IRQINFO |= 4;
			irqCallbackSPU2();
		}

		spuaddr++;           // inc spu addr
		if (spuaddr > 0x0fffff) // wrap at 2Mb
			spuaddr = 0;           // wrap
	}

	spuaddr += 19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
	C0_SPUADDR_SET(spuaddr);

	// got from J.F. and Kanodin... is it needed?
	spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;                                    // DMA complete
	SPUStartCycle[0] = SPUCycles;
	SPUTargetCycle[0] = size;
	interrupt |= (1 << 1);
}

EXPORT_C_(void) SPU2readDMA7Mem(u16* pMem, int size)
{
	u32 spuaddr = C1_SPUADDR;
	int i;

	SPU2_LOG("SPU2 readDMA7Mem size %x, addr: %x\n", size, pMem);

	for (i = 0;i < size;i++)
	{
		*pMem++ = *(u16*)(spu2mem + spuaddr);
		if ((spu2Rs16(REG_C1_CTRL)&0x40) && C1_IRQA == spuaddr)
		{
			spu2Ru16(SPDIF_OUT) |= 0x8;
			C1_SPUADDR_SET(spuaddr);
			IRQINFO |= 8;
			irqCallbackSPU2();
		}
		spuaddr++;                            // inc spu addr
		if (spuaddr > 0x0fffff) // wrap at 2Mb
			spuaddr = 0;           // wrap
	}

	spuaddr += 19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
	C1_SPUADDR_SET(spuaddr);

	// got from J.F. and Kanodin... is it needed?
	spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;                                   // DMA complete
	SPUStartCycle[1] = SPUCycles;
	SPUTargetCycle[1] = size;
	interrupt |= (1 << 2);
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
	if (interrupt & 0x2) return 0;
	if (Adma4.AmountLeft <= 0) return 1;

	spuaddr = C0_SPUADDR;
	// SPU2 Deinterleaves the Left and Right Channels
	memcpy((s16*)(spu2mem + spuaddr + 0x2000), (s16*)Adma4.MemAddr, 512);
	Adma4.MemAddr += 256;
	memcpy((s16*)(spu2mem + spuaddr + 0x2200), (s16*)Adma4.MemAddr, 512);
	Adma4.MemAddr += 256;
	spuaddr = (spuaddr + 256) & 511;
	C0_SPUADDR_SET(spuaddr);

	Adma4.AmountLeft -= 512;
	if (Adma4.AmountLeft == 0)
	{
		SPUStartCycle[0] = SPUCycles;
		SPUTargetCycle[0] = 1;//512*48000;
		spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;
		interrupt |= (1 << 1);
	}
	return 0;
}

int ADMAS7Write()
{
	u32 spuaddr;
	if (interrupt & 0x4) return 0;
	if (Adma7.AmountLeft <= 0) return 1;

	spuaddr = C1_SPUADDR;
	// SPU2 Deinterleaves the Left and Right Channels
	memcpy((s16*)(spu2mem + spuaddr + 0x2400), (s16*)Adma7.MemAddr, 512);
	Adma7.MemAddr += 256;
	memcpy((s16*)(spu2mem + spuaddr + 0x2600), (s16*)Adma7.MemAddr, 512);
	Adma7.MemAddr += 256;
	spuaddr = (spuaddr + 256) & 511;
	C1_SPUADDR_SET(spuaddr);

	Adma7.AmountLeft -= 512;
	if (Adma7.AmountLeft == 0)
	{
		SPUStartCycle[1] = SPUCycles;
		SPUTargetCycle[1] = 1;//512*48000;
		spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;
		interrupt |= (1 << 2);
	}
	return 0;
}

EXPORT_C_(void) SPU2writeDMA4Mem(u16* pMem, int size)
{
	u32 spuaddr;

	SPU2_LOG("SPU2 writeDMA4Mem size %x, addr: %x\n", size, pMem);
	
	if ((spu2Ru16(REG_C0_ADMAS) & 0x1) && (spu2Ru16(REG_C0_CTRL) & 0x30) == 0 && size)
	{
		//fwrite(pMem,iSize<<1,1,LogFile);
		memset(&Adma4, 0, sizeof(ADMA));
		C0_SPUADDR_SET(0);
		Adma4.MemAddr = pMem;
		Adma4.AmountLeft = size;
		ADMAS4Write();
		return;
	}

	spuaddr = C0_SPUADDR;
	memcpy((u8*)(spu2mem + spuaddr), (u8*)pMem, size << 1);
	spuaddr += size;
	C0_SPUADDR_SET(spuaddr);

	if ((spu2Ru16(REG_C0_CTRL)&0x40) && C0_IRQA == spuaddr)
	{
		spu2Ru16(SPDIF_OUT) |= 0x4;
		IRQINFO |= 4;
		irqCallbackSPU2();
	}
	if (spuaddr > 0xFFFFE)
		spuaddr = 0x2800;
	C0_SPUADDR_SET(spuaddr);

	MemAddr[0] += size << 1;
	spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;
	SPUStartCycle[0] = SPUCycles;
	SPUTargetCycle[0] = 1;//iSize;
	interrupt |= (1 << 1);
}

EXPORT_C_(void) SPU2writeDMA7Mem(u16* pMem, int size)
{
	u32 spuaddr;

	SPU2_LOG("SPU2 writeDMA7Mem size %x, addr: %x\n", size, pMem);
	
	if ((spu2Ru16(REG_C1_ADMAS) & 0x2) && (spu2Ru16(REG_C1_CTRL) & 0x30) == 0 && size)
	{
		//fwrite(pMem,iSize<<1,1,LogFile);
		memset(&Adma7, 0, sizeof(ADMA));
		C1_SPUADDR_SET(0);
		Adma7.MemAddr = pMem;
		Adma7.AmountLeft = size;
		ADMAS7Write();
		return;
	}

	spuaddr = C1_SPUADDR;
	memcpy((u8*)(spu2mem + spuaddr), (u8*)pMem, size << 1);
	spuaddr += size;
	C1_SPUADDR_SET(spuaddr);

	if ((spu2Ru16(REG_C1_CTRL)&0x40) && C1_IRQA == spuaddr)
	{
		spu2Ru16(SPDIF_OUT) |= 0x8;
		IRQINFO |= 8;
		irqCallbackSPU2();
	}
	if (spuaddr > 0xFFFFE)
		spuaddr = 0x2800;
	C1_SPUADDR_SET(spuaddr);

	MemAddr[1] += size << 1;
	spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;
	SPUStartCycle[1] = SPUCycles;
	SPUTargetCycle[1] = 1;//iSize;
	interrupt |= (1 << 2);
}

EXPORT_C_(void) SPU2interruptDMA4()
{
	SPU2_LOG("SPU2 interruptDMA4\n");
	
	spu2Rs16(REG_C0_CTRL) &= ~0x30;
	spu2Ru16(REG_C0_SPUSTAT) |= 0x80;
}

EXPORT_C_(void) SPU2interruptDMA7()
{
	SPU2_LOG("SPU2 interruptDMA7\n");
	
//	spu2Rs16(REG_C1_CTRL)&= ~0x30;
//	//spu2Rs16(REG__5B0) = 0;
//	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;
	spu2Rs16(REG_C1_CTRL) &= ~0x30;
	spu2Ru16(REG_C1_SPUSTAT) |= 0x80;
}

// turn channels on
void SoundOn(s32 start, s32 end, u16 val)   // SOUND ON PSX COMAND
{
	for (s32 ch = start;ch < end;ch++, val >>= 1)             // loop channels
	{
		if ((val&1) && voices[ch].pStart)                   // mmm... start has to be set before key on !?!
		{
			voices[ch].bNew = true;
			voices[ch].bIgnoreLoop = false;
		}
	}
}

// turn channels off
void SoundOff(s32 start, s32 end, u16 val)  // SOUND OFF PSX COMMAND
{
	for (s32 ch = start;ch < end;ch++, val >>= 1)             // loop channels
	{
		if (val&1)                                          // && s_chan[i].bOn)  mmm...
			voices[ch].bStop = true;
	}
}

void FModOn(s32 start, s32 end, u16 val)    // FMOD ON PSX COMMAND
{
	int ch;

	for (ch = start;ch < end;ch++, val >>= 1)             // loop channels
	{
		if (val&1)                                          // -> fmod on/off
		{
			if (ch > 0)
			{
			}
		}
		else
		{
			// turn fmod off
		}
	}
}

EXPORT_C_(void) SPU2write(u32 mem, u16 value)
{
	u32 spuaddr;
	
	SPU2_LOG("SPU2 write mem %x value %x\n", mem, value);

	assert(C0_SPUADDR < 0x100000);
	assert(C1_SPUADDR < 0x100000);

	spu2Ru16(mem) = value;
	u32 r = mem & 0xffff;

	// channel info
	if ((r >= 0x0000 && r < 0x0180) || (r >= 0x0400 && r < 0x0580))  // some channel info?
	{
		int ch = 0;
		if (r >= 0x400) ch = ((r - 0x400) >> 4) + 24;
		else ch = (r >> 4);

		VOICE_PROCESSED* pvoice = &voices[ch];

		switch (r&0x0f)
		{
			case 0:
			case 2:
				pvoice->SetVolume(mem&0x2);
				break;
			case 4:
				{
					int NP;
					if (value > 0x3fff) 
						NP = 0x3fff;                        // get pitch val
					else          
						NP = value;

					pvoice->pvoice->pitch = NP;

					NP = (44100L * NP) / 4096L;                           // calc frequency
					if (NP < 1) NP = 1;                                   // some security
					pvoice->iActFreq = NP;                             // store frequency
					break;
				}
			case 6:
				{
					pvoice->ADSRX.AttackModeExp = (value & 0x8000) ? 1 : 0;
					pvoice->ADSRX.AttackRate = ((value >> 8) & 0x007f);
					pvoice->ADSRX.DecayRate = (((value >> 4) & 0x000f));
					pvoice->ADSRX.SustainLevel = (value & 0x000f);
					break;
				}
			case 8:
				pvoice->ADSRX.SustainModeExp = (value & 0x8000) ? 1 : 0;
				pvoice->ADSRX.SustainIncrease = (value & 0x4000) ? 0 : 1;
				pvoice->ADSRX.SustainRate = ((value >> 6) & 0x007f);
				pvoice->ADSRX.ReleaseModeExp = (value & 0x0020) ? 1 : 0;
				pvoice->ADSRX.ReleaseRate = ((value & 0x001f));
				break;
		}

		return;
	}

	// more channel info
	if ((r >= 0x01c0 && r <= 0x02E0) || (r >= 0x05c0 && r <= 0x06E0))
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

		switch (rx)
		{
			case 0x1C0:
				pvoice->iStartAddr = (((u32)value & 0x3f) << 16) | (pvoice->iStartAddr & 0xFFFF);
				pvoice->pStart = (u8*)(spu2mem + pvoice->iStartAddr);
				break;
			case 0x1C2:
				pvoice->iStartAddr = (pvoice->iStartAddr & 0x3f0000) | (value & 0xFFFF);
				pvoice->pStart = (u8*)(spu2mem + pvoice->iStartAddr);
				break;
			case 0x1C4:
				pvoice->iLoopAddr = (((u32)value & 0x3f) << 16) | (pvoice->iLoopAddr & 0xFFFF);
				pvoice->pLoop = (u8*)(spu2mem + pvoice->iLoopAddr);
				pvoice->bIgnoreLoop = pvoice->iLoopAddr > 0;
				break;
			case 0x1C6:
				pvoice->iLoopAddr = (pvoice->iLoopAddr & 0x3f0000) | (value & 0xFFFF);
				pvoice->pLoop = (u8*)(spu2mem + pvoice->iLoopAddr);
				pvoice->bIgnoreLoop = pvoice->iLoopAddr > 0;
				break;
			case 0x1C8:
				// unused... check if it gets written as well
				pvoice->iNextAddr = (((u32)value & 0x3f) << 16) | (pvoice->iNextAddr & 0xFFFF);
				break;
			case 0x1CA:
				// unused... check if it gets written as well
				pvoice->iNextAddr = (pvoice->iNextAddr & 0x3f0000) | (value & 0xFFFF);
				break;
		}

		return;
	}

	// process non-channel data
	switch (mem&0xffff)
	{
		case REG_C0_SPUDATA:
			spuaddr = C0_SPUADDR;
			spu2mem[spuaddr] = value;
			spuaddr++;
			if ((spu2Ru16(REG_C0_CTRL)&0x40) && C0_IRQA == spuaddr)
			{
				spu2Ru16(SPDIF_OUT) |= 0x4;
				IRQINFO |= 4;
				irqCallbackSPU2();
			}
			if (spuaddr > 0xFFFFE) spuaddr = 0x2800;
			
			C0_SPUADDR_SET(spuaddr);
			spu2Ru16(REG_C0_SPUSTAT) &= ~0x80;
			spu2Ru16(REG_C0_CTRL) &= ~0x30;
			break;
		case REG_C1_SPUDATA:
			spuaddr = C1_SPUADDR;
			spu2mem[spuaddr] = value;
			spuaddr++;
			if ((spu2Ru16(REG_C1_CTRL)&0x40) && C1_IRQA == spuaddr)
			{
				spu2Ru16(SPDIF_OUT) |= 0x8;
				IRQINFO |= 8;
				irqCallbackSPU2();
			}
			if (spuaddr > 0xFFFFE) spuaddr = 0x2800;
			
			C1_SPUADDR_SET(spuaddr);
			spu2Ru16(REG_C1_SPUSTAT) &= ~0x80;
			spu2Ru16(REG_C1_CTRL) &= ~0x30;
			break;
		case REG_C0_IRQA_HI:
		case REG_C0_IRQA_LO:
			pSpuIrq[0] = spu2mem + (C0_IRQA << 1);
			break;
		case REG_C1_IRQA_HI:
		case REG_C1_IRQA_LO:
			pSpuIrq[1] = spu2mem + (C1_IRQA << 1);
			break;

		case REG_C0_SPUADDR_HI:
		case REG_C1_SPUADDR_HI:
			spu2Ru16(mem) = value & 0xf;
			break;

		case REG_C0_SPUON1:
			SoundOn(0, 16, value);
			break;
		case REG_C0_SPUON2:
			SoundOn(16, 24, value);
			break;
		case REG_C1_SPUON1:
			SoundOn(24, 40, value);
			break;
		case REG_C1_SPUON2:
			SoundOn(40, 48, value);
			break;
		case REG_C0_SPUOFF1:
			SoundOff(0, 16, value);
			break;
		case REG_C0_SPUOFF2:
			SoundOff(16, 24, value);
			break;
		case REG_C1_SPUOFF1:
			SoundOff(24, 40, value);
			break;
		case REG_C1_SPUOFF2:
			SoundOff(40, 48, value);
			break;

			// According to manual all bits are cleared by writing an arbitary value
		case REG_C0_END1:
			dwEndChannel2[0] = 0;
			break;
		case REG_C0_END2:
			dwEndChannel2[0] = 0;
			break;
		case REG_C1_END1:
			dwEndChannel2[1] = 0;
			break;
		case REG_C1_END2:
			dwEndChannel2[1] = 0;
			break;
		case REG_C0_FMOD1:
			FModOn(0, 16, value);
			break;
		case REG_C0_FMOD2:
			FModOn(16, 24, value);
			break;
		case REG_C1_FMOD1:
			FModOn(24, 40, value);
			break;
		case REG_C1_FMOD2:
			FModOn(40, 48, value);
			break;
	}

	assert(C0_SPUADDR < 0x100000);
	assert(C1_SPUADDR < 0x100000);
}

EXPORT_C_(u16) SPU2read(u32 mem)
{
	u32 spuaddr;
	u16 ret;
	u32 r = mem & 0xffff;

	if ((r >= 0x0000 && r <= 0x0180) || (r >= 0x0400 && r <= 0x0580))  // some channel info?
	{
		s32 ch = 0;
		
		if (r >= 0x400) 
			ch = ((r - 0x400) >> 4) + 24;
		else 
			ch = (r >> 4);

		VOICE_PROCESSED* pvoice = &voices[ch];

		switch (r&0x0f)
		{
			case 10:
				return (u16)(pvoice->ADSRX.EnvelopeVol >> 16);
		}
	}

	if ((r > 0x01c0 && r <= 0x02E0) || (r > 0x05c0 && r <= 0x06E0))  // some channel info?
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

		switch (rx)
		{
			case 0x1C0:
				return (u16)(((pvoice->pStart - (u8*)spu2mem) >> 17)&0x3F);
			case 0x1C2:
				return (u16)(((pvoice->pStart - (u8*)spu2mem) >> 1)&0xFFFF);
			case 0x1C4:
				return (u16)(((pvoice->pLoop - (u8*)spu2mem) >> 17)&0x3F);
			case 0x1C6:
				return (u16)(((pvoice->pLoop - (u8*)spu2mem) >> 1)&0xFFFF);
			case 0x1C8:
				return (u16)(((pvoice->pCurr - (u8*)spu2mem) >> 17)&0x3F);
			case 0x1CA:
				return (u16)(((pvoice->pCurr - (u8*)spu2mem) >> 1)&0xFFFF);
		}
	}

	switch (mem&0xffff)
	{
		case REG_C0_SPUDATA:
			spuaddr = C0_SPUADDR;
			ret = spu2mem[spuaddr];
			spuaddr++;
			if (spuaddr > 0xfffff) spuaddr = 0;
			C0_SPUADDR_SET(spuaddr);
			break;
		
		case REG_C1_SPUDATA:
			spuaddr = C1_SPUADDR;
			ret = spu2mem[spuaddr];
			spuaddr++;
			if (spuaddr > 0xfffff) spuaddr = 0;
			C1_SPUADDR_SET(spuaddr);
			break;

		case REG_C0_END1:
			return (dwEndChannel2[0]&0xffff);
		case REG_C0_END2:
			return (dwEndChannel2[0] >> 16);
		case REG_C1_END1:
			return (dwEndChannel2[1]&0xffff);
		case REG_C1_END2:
			return (dwEndChannel2[1] >> 16);

		case REG_IRQINFO:
			ret = IRQINFO;
			IRQINFO = 0;
			break;
		default:
			ret = spu2Ru16(mem);
	}
	
	SPU2_LOG("SPU2 read mem %x: %x\n", mem, ret);

	return ret;
}

EXPORT_C_(void) SPU2WriteMemAddr(int core, u32 value)
{
	MemAddr[core] = value;
}

EXPORT_C_(u32) SPU2ReadMemAddr(int core)
{
	return MemAddr[core];
}

EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)(), void (*DMA4callback)(), void (*DMA7callback)())
{
	irqCallbackSPU2 = SPU2callback;
	irqCallbackDMA4 = DMA4callback;
	irqCallbackDMA7 = DMA7callback;
}

// VOICE_PROCESSED definitions
SPU_CONTROL_* VOICE_PROCESSED::GetCtrl()
{
	return ((SPU_CONTROL_*)(spu2regs + memoffset + REG_C0_CTRL));
}

void VOICE_PROCESSED::SetVolume(int iProcessRight)
{
	u16 vol = iProcessRight ? pvoice->right.word : pvoice->left.word;

	if (vol&0x8000) // sweep not working
	{
		s16 sInc = 1;                                     // -> sweep up?
		if (vol&0x2000) sInc = -1;                          // -> or down?
		if (vol&0x1000) vol ^= 0xffff;                      // -> mmm... phase inverted? have to investigate this
		vol = ((vol & 0x7f) + 1) / 2;                       // -> sweep: 0..127 -> 0..64
		vol += vol / (2 * sInc);                            // -> HACK: we don't sweep right now, so we just raise/lower the volume by the half!
		vol *= 128;
	}
	else                                                  // no sweep:
	{
		if (vol&0x4000)                                     // -> mmm... phase inverted? have to investigate this
			vol = 0x3fff - (vol & 0x3fff);
	}

	vol &= 0x3fff;
	// set volume
	//if( iProcessRight ) right = vol;
	//else left = vol;
}

void VOICE_PROCESSED::StartSound()
{
	ADSRX.lVolume = 1; // and init some adsr vars
	ADSRX.State = 0;
	ADSRX.EnvelopeVol = 0;

	if (bReverb && GetCtrl()->reverb)
	{
		// setup the reverb effects
	}

	pCurr = pStart; // set sample start
	iSBPos = 28;

	bNew = false;       // init channel flags
	bStop = false;
	bOn = true;
	spos = 0x10000L;
}

void VOICE_PROCESSED::VoiceChangeFrequency()
{
	iUsedFreq = iActFreq;             // -> take it and calc steps
	sinc = (u32)pvoice->pitch << 4;
	if (!sinc)
		sinc = 1;
}

void VOICE_PROCESSED::Stop()
{
}

// GUI Routines
EXPORT_C_(s32) SPU2test()
{
	return 0;
}

typedef struct
{
	u32 version;
	u8 spu2regs[0x10000];
} SPU2freezeData;

EXPORT_C_(s32) SPU2freeze(int mode, freezeData *data)
{
	SPU2freezeData *spud;

	if (mode == FREEZE_LOAD)
	{
		spud = (SPU2freezeData*)data->data;
		if (spud->version == 0x11223344)
		{
			memcpy(spu2regs, spud->spu2regs, 0x10000);
		}
		else 
		{
			printf("SPU2null wrong format\n");
		}
	}
	else
		if (mode == FREEZE_SAVE)
		{
			spud = (SPU2freezeData*)data->data;
			spud->version = 0x11223344;
			memcpy(spud->spu2regs, spu2regs, 0x10000);
		}
		else
			if (mode == FREEZE_SIZE)
			{
				data->size = sizeof(SPU2freezeData);
			}

	return 0;
}

