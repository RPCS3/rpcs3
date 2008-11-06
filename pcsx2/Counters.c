/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <string.h>
#include <time.h>
#include <math.h>
#include "Common.h"
#include "PsxCommon.h"
#include "GS.h"

u64 profile_starttick = 0;
u64 profile_totalticks = 0;

int gates = 0;
extern u8 psxhblankgate;
int hblankend = 0;
Counter counters[6];
u32 nextCounter, nextsCounter;
static void (*s_prevExecuteVU1Block)() = NULL;
LARGE_INTEGER lfreq;

// its so it doesnt keep triggering an interrupt once its reached its target
// if it doesnt reset the counter it needs stopping
u32 eecntmask = 0;

void rcntUpdTarget(int index) {
	counters[index].sCycleT = cpuRegs.cycle;
}

void rcntUpd(int index) {
	counters[index].sCycle = cpuRegs.cycle;
	rcntUpdTarget(index);
}

void rcntReset(int index) {
	counters[index].count = 0;
	//counters[index].mode&= ~0x00400C00;
	rcntUpd(index);
}

void rcntSet() {
	u32 c;
	int i;

	nextCounter = 0xffffffff;
	nextsCounter = cpuRegs.cycle;

	for (i = 0; i < 4; i++) {
		if (!(counters[i].mode & 0x80) || (counters[i].mode & 0x3) == 0x3) continue; // Stopped
			c = ((0x10000 - counters[i].count) * counters[i].rate) - (cpuRegs.cycle - counters[i].sCycleT);
			if (c < nextCounter) {
				nextCounter = c;
			}
		
		// the + 10 is just in case of overflow
			//if(!(counters[i].mode & 0x100) || counters[i].target > 0xffff) continue;
			c = ((counters[i].target - counters[i].count) * counters[i].rate) - (cpuRegs.cycle - counters[i].sCycleT);
			if (c < nextCounter) {
			nextCounter = c;
			}
	
	}
	//Calculate HBlank
	c = counters[4].CycleT - (cpuRegs.cycle - counters[4].sCycleT);
	if (c < nextCounter) {
		nextCounter = c;
	}
	//if(nextCounter > 0x1000) SysPrintf("Nextcounter %x HBlank %x VBlank %x\n", nextCounter, c, counters[5].CycleT - (cpuRegs.cycle - counters[5].sCycleT));
	//Calculate VBlank
	c = counters[5].CycleT - (cpuRegs.cycle - counters[5].sCycleT);
	if (c < nextCounter) {
		nextCounter = c;
	}
	
}

void rcntInit() {
	int i;

	memset(counters, 0, sizeof(counters));

	for (i=0; i<4; i++) {
		counters[i].rate = 2;
		counters[i].target = 0xffff;
	}
	counters[0].interrupt =  9;
	counters[1].interrupt = 10;
	counters[2].interrupt = 11;
	counters[3].interrupt = 12;

	counters[4].mode = 0x3c0; // The VSync counter mode
	counters[5].mode = 0x3c0;

	
	
	UpdateVSyncRate();
	/*hblankend = 0;
	counters[5].mode &= ~0x10000;
	counters[4].sCycleT = cpuRegs.cycle;
	counters[4].CycleT = HBLANKCNT(1);
	counters[4].count = 1;
	counters[5].CycleT = VBLANKCNT(1);
	counters[5].count = 1;
	counters[5].sCycleT = cpuRegs.cycle;*/
	
#ifdef _WIN32
    QueryPerformanceFrequency(&lfreq);
#endif

	for (i=0; i<4; i++) rcntUpd(i);
	rcntSet();

	assert(Cpu != NULL && Cpu->ExecuteVU1Block != NULL );
	s_prevExecuteVU1Block = Cpu->ExecuteVU1Block;
}

// debug code, used for stats
int g_nCounters[4];
extern u32 s_lastvsync[2];
static int iFrame = 0;	

#ifndef _WIN32
#include <sys/time.h>
#endif

u64 iTicks=0;

u64 GetTickFrequency()
{
#ifdef _WIN32
	return lfreq.QuadPart;
#else
    return 1000000;
#endif
}

u64 GetCPUTicks()
{
#ifdef _WIN32
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
#else
    struct timeval t;
    gettimeofday(&t, NULL);
    return (u64)t.tv_sec*1000000+t.tv_usec;
#endif
}


void UpdateVSyncRate() {
	if (Config.PsxType & 1) {
		SysPrintf("PAL\n");
		counters[4].Cycle = 227000;
		/*if(Config.PsxType & 2)counters[5].rate = PS2VBLANK_PAL_INT;
		else counters[5].rate = PS2VBLANK_PAL;*/
	
		counters[5].Cycle  = 720;
	} else {
		SysPrintf("NTSC\n");
		counters[4].Cycle = 227000;
		/*if(Config.PsxType & 2)counters[5].rate = PS2VBLANK_NTSC_INT;
		else counters[5].rate = PS2VBLANK_NTSC;*/
		
		counters[5].Cycle  = 720;
	}	

	hblankend = 0;
	counters[5].mode &= ~0x10000;
	counters[4].sCycleT = cpuRegs.cycle;
	counters[4].CycleT = HBLANKCNT(1);
	counters[4].count = 1;
	counters[5].CycleT = VBLANKCNT(1);
	counters[5].count = 1;
	counters[5].sCycleT = cpuRegs.cycle;
	
	//rcntUpdTarget(4);
	//counters[4].CycleT  = counters[4].rate;
	///rcntUpdTarget(5);
	/*counters[5].CycleT  = counters[5].rate;
	counters[5].Cycle  = PS2VBLANKEND;*/
	
	{
		u32 vsyncs = (Config.PsxType&1) ? 50:60;
		if(Config.CustomFps>0) vsyncs = Config.CustomFps;
		iTicks = GetTickFrequency()/vsyncs;
		SysPrintf("Framelimiter rate updated (UpdateVSyncRate): %d fps\n",vsyncs);
	}
	rcntSet();
}

u32 pauses=0;

void FrameLimiter()
{
	static u64 iStart=0, iEnd=0, iExpectedEnd=0;

	if(iStart==0) iStart = GetCPUTicks();

	iExpectedEnd = iStart + iTicks;
	iEnd = GetCPUTicks();

	if(iEnd>=iExpectedEnd)
	{
		u64 diff = iEnd-iExpectedEnd;
		if((diff>>3)>iTicks) iExpectedEnd=iEnd;
	}
	else do {
		Sleep(1);
		pauses++;
		iEnd = GetCPUTicks();
	} while(iEnd<iExpectedEnd);
	iStart = iExpectedEnd; //remember the expected value frame. improves smoothness
}

extern u32 CSRw;
extern u64 SuperVUGetRecTimes(int clear);
extern u32 vu0time;

extern void DummyExecuteVU1Block(void);

static u32 lastWasSkip=0;
//extern u32 unpacktotal;

#include "VU.h"
void VSync() 
{

	if (counters[5].mode & 0x10000) { // VSync End (22 hsyncs)

		// swap the vsync field
		u32 newfield = (*(u32*)(PS2MEM_GS+0x1000)&0x2000) ? 0 : 0x2000;
		*(u32*)(PS2MEM_GS+0x1000) = (*(u32*)(PS2MEM_GS+0x1000) & ~(1<<13)) | newfield;
		iFrame++;

		// wait until GS stops
		if( CHECK_MULTIGS ) {
			GSRingBufSimplePacket(GS_RINGTYPE_VSYNC, newfield, 0, 0);
		}
		else {
			GSvsync(newfield);

            // update here on single thread mode *OBSOLETE*
            if( PAD1update != NULL ) PAD1update(0);
            if( PAD2update != NULL ) PAD2update(1);
		}

		counters[5].mode&= ~0x10000;
		hwIntcIrq(3);		
		psxVSyncEnd();
		
		if(gates)rcntEndGate(0x8);
		SysUpdate();
		// used to limit frames
		switch(CHECK_FRAMELIMIT) {
			case PCSX2_FRAMELIMIT_LIMIT:
				FrameLimiter();
				break;

			case PCSX2_FRAMELIMIT_SKIP:
			case PCSX2_FRAMELIMIT_VUSKIP:
			{
				// the 6 was after trial and error
				static u32 uPrevTimes[6] = {0}, uNextFrame = 0, uNumFrames = 0, uTotalTime = 0;
				static u32 uLastTime = 0;
				static int nConsecutiveSkip = 0, nConsecutiveRender = 0;
				static short int changed = 0;
				static short int nNoSkipFrames = 0;
				
				u32 uExpectedTime;
				u32 uCurTime = timeGetTime();
				u32 uDeltaTime = uCurTime - uLastTime;

				if( uLastTime > 0 ) {

					if( uNumFrames == ARRAYSIZE(uPrevTimes) ) uTotalTime -= uPrevTimes[uNextFrame];

					uPrevTimes[uNextFrame] = uDeltaTime;
					uNextFrame = (uNextFrame + 1) % ARRAYSIZE(uPrevTimes);
					uTotalTime += uDeltaTime;

					if( uNumFrames < ARRAYSIZE(uPrevTimes) ) ++uNumFrames;
				}
				//the +6 accounts for calling FrameLimiter() instead Sleep()
				uExpectedTime = (Config.PsxType&1) ? (ARRAYSIZE(uPrevTimes) * 1000 / 50 +6) : (ARRAYSIZE(uPrevTimes) * 1000 / 60 +6); 

				if( nNoSkipFrames > 0 )  --nNoSkipFrames;

				// hmm... this might be more complicated than it needs to be... or not?
				if( changed != 0 ) {
					if( changed > 0 ) {
						++nConsecutiveRender;
						--changed;

						if( nConsecutiveRender > 20 && uTotalTime + 1 < uExpectedTime ) nNoSkipFrames = ARRAYSIZE(uPrevTimes);
	
					}
					else {
						++nConsecutiveSkip;
						++changed;
					}
				}
				else {
					
					if( nNoSkipFrames == 0 && nConsecutiveRender > 1 && nConsecutiveSkip < 1 &&
						(CHECK_MULTIGS? (uTotalTime >= uExpectedTime + uDeltaTime/4 && (uTotalTime >= uExpectedTime + uDeltaTime*3/4 || nConsecutiveSkip==0)) : 
										(uTotalTime >= uExpectedTime + (uDeltaTime/4))) ) {


						if( nConsecutiveSkip == 0 ) {

							//first freeze GS regs THEN send dummy packet
							if( CHECK_MULTIGS ) GSRingBufSimplePacket(GS_RINGTYPE_FRAMESKIP, 1, 0, 0);
							else GSsetFrameSkip(1);
							if( CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_VUSKIP ) {	
							Cpu->ExecuteVU1Block = DummyExecuteVU1Block;
							}
						}

						changed = -1;
						nConsecutiveSkip++;
					}
					else {

						if( nConsecutiveSkip > 1) {
							//first set VU1 to enabled THEN unfreeze GS regs
							if( CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_VUSKIP ) 
								Cpu->ExecuteVU1Block = s_prevExecuteVU1Block;
							if( CHECK_MULTIGS ) GSRingBufSimplePacket(GS_RINGTYPE_FRAMESKIP, 0, 0, 0);
							else GSsetFrameSkip(0);
				
							nConsecutiveRender = 0;
						}

						changed = 3;
						nConsecutiveRender++;
						nConsecutiveSkip = 0;

						if( nConsecutiveRender > 20 && uTotalTime + 1 < uExpectedTime ) {
							
							nNoSkipFrames = ARRAYSIZE(uPrevTimes);
						}
					}
				}
				uLastTime = uCurTime;
				//dont get too fast, instead keep at smooth full fps
				FrameLimiter();
				break;
			}
		}
	} else { // VSync Start (240 hsyncs) 
		//UpdateVSyncRateEnd();
#ifdef EE_PROFILING
		if( (iFrame%20) == 0 ) {
			SysPrintf("Profiled Cycles at %d frames %d\n", iFrame, profile_totalticks);
			CLEAR_EE_PROFILE();
        }
#endif


		//SysPrintf("c: %x, %x\n", cpuRegs.cycle, *(u32*)&VU1.Micro[16]);
		//if( (iFrame%20) == 0 ) SysPrintf("svu time: %d\n", SuperVUGetRecTimes(1) * 100000 / lfreq.QuadPart);
//		if( (iFrame%10) == 0 ) {
//			SysPrintf("vu0 time: %d\n", vu0time);
//			vu0time = 0;
//		}


#ifdef PCSX2_DEVBUILD
		if( g_TestRun.enabled && g_TestRun.frame > 0 ) {
			if( iFrame > g_TestRun.frame ) {
				// take a snapshot
				if( g_TestRun.pimagename != NULL && GSmakeSnapshot2 != NULL ) {
					if( g_TestRun.snapdone ) {
						g_TestRun.curimage++;
						g_TestRun.snapdone = 0;
						g_TestRun.frame += 20;
						if( g_TestRun.curimage >= g_TestRun.numimages ) {
							// exit
							SysClose();
							exit(0);
						}
					}
					else {
						// query for the image
						GSmakeSnapshot2(g_TestRun.pimagename, &g_TestRun.snapdone, g_TestRun.jpgcapture);
					}
				}
				else {
					// exit
					SysClose();
					exit(0);
				}
			}
		}

		GSVSYNC();

		if( g_SaveGSStream == 1 ) {
			freezeData fP;

			g_SaveGSStream = 2;
			gsFreeze(g_fGSSave, 1);
			
			if (GSfreeze(FREEZE_SIZE, &fP) == -1) {
				gzclose(g_fGSSave);
				g_SaveGSStream = 0;
			}
			else {
				fP.data = (s8*)malloc(fP.size);
				if (fP.data == NULL) {
					gzclose(g_fGSSave);
					g_SaveGSStream = 0;
				}
				else {
					if (GSfreeze(FREEZE_SAVE, &fP) == -1) {
						gzclose(g_fGSSave);
						g_SaveGSStream = 0;
					}
					else {
						gzwrite(g_fGSSave, &fP.size, sizeof(fP.size));
						if (fP.size) {
							gzwrite(g_fGSSave, fP.data, fP.size);
							free(fP.data);
						}
					}
				}
			}
		}
		else if( g_SaveGSStream == 2 ) {
			
			if( --g_nLeftGSFrames <= 0 ) {
				gzclose(g_fGSSave);
				g_fGSSave = NULL;
				g_SaveGSStream = 0;
				SysPrintf("Done saving GS stream\n");
			}
		}
#endif

		

		

		//counters[5].mode&= ~0x10000;
		//UpdateVSyncRate();
		
		//SysPrintf("ctrs: %d %d %d %d\n", g_nCounters[0], g_nCounters[1], g_nCounters[2], g_nCounters[3]);
		//SysPrintf("vif: %d\n", (((LARGE_INTEGER*)g_nCounters)->QuadPart * 1000000) / lfreq.QuadPart);
		//memset(g_nCounters, 0, 16);
		counters[5].mode|= 0x10000;
		if ((CSRw & 0x8))
			GSCSRr|= 0x8;

		if (!(GSIMR&0x800) )
					gsIrq();

		hwIntcIrq(2);
		psxVSyncStart();
		
		if(Config.Patch) applypatch(1);
		if(gates)rcntStartGate(0x8);

//		__Log("%u %u 0\n", cpuRegs.cycle-s_lastvsync[1], timeGetTime()-s_lastvsync[0]);
//		s_lastvsync[0] = timeGetTime();
//		s_lastvsync[1] = cpuRegs.cycle;
	}
}


void rcntUpdate()
{
	int i;
	u32 change = 0;
	for (i=0; i<=3; i++) {
		if(gates & (1<<i) ){
			//SysPrintf("Stopped accidental update of ee counter %x when using a gate\n", i);
			continue;
			}
		if ((counters[i].mode & 0x80) && (counters[i].mode & 0x3) != 0x3){
		change = cpuRegs.cycle - counters[i].sCycleT;
		 counters[i].count += (int)(change / counters[i].rate);
		change -= (change / counters[i].rate) * counters[i].rate;
			} else change = 0;
		counters[i].sCycleT = cpuRegs.cycle - change;
		//if(change > 0) SysPrintf("Change saved on %x = %x\n", i, change);
	}
	
	if ((u32)(cpuRegs.cycle - counters[4].sCycleT) >= (u32)counters[4].CycleT && hblankend == 1){
		
		
		if ((CSRw & 0x4))
			GSCSRr |= 4; // signal
			
		
		if (!(GSIMR&0x400) )
				gsIrq();

		if(gates)rcntEndGate(0);
		if(psxhblankgate)psxCheckEndGate(0);
		hblankend = 0;
		
		counters[4].CycleT  = HBLANKCNT(counters[4].count);
	} else 
	if ((u32)(cpuRegs.cycle - counters[4].sCycleT) >= (u32)counters[4].CycleT) {
		
		if(counters[4].count >= counters[4].Cycle){
			//SysPrintf("%x of %x hblanks reorder in %x cycles cpuRegs.cycle = %x\n", counters[4].count, counters[4].Cycle, cpuRegs.cycle - counters[4].sCycleT, cpuRegs.cycle);								
			counters[4].sCycleT += HBLANKCNT(counters[4].Cycle);
			counters[4].count -= counters[4].Cycle;
				
			}
		//counters[4].sCycleT += HBLANKCNT(1);
		counters[4].count++;
			
		counters[4].CycleT = HBLANKCNT(counters[4].count) - (HBLANKCNT(0.5));
		
		rcntStartGate(0);
		psxCheckStartGate(0);
		hblankend = 1;
		//if(cpuRegs.cycle > 0xffff0000 || cpuRegs.cycle < 0x1000
		//SysPrintf("%x hsync done in %x cycles cpuRegs.cycle = %x next will happen on %x\n", counters[4].count, counters[4].CycleT, cpuRegs.cycle, (u32)(counters[4].sCycleT + counters[4].CycleT));
	}
	
	if((counters[5].mode & 0x10000)){
		if ((cpuRegs.cycle - counters[5].sCycleT)  >= counters[5].CycleT){
				//counters[5].sCycleT = cpuRegs.cycle;
				counters[5].CycleT = VBLANKCNT(counters[5].count);
				VSync();
			}
	} else if ((cpuRegs.cycle - counters[5].sCycleT) >= counters[5].CycleT) {
			if(counters[5].count >= counters[5].Cycle){
				//SysPrintf("reset %x  of %x frames done in %x cycles cpuRegs.cycle = %x\n", counters[5].count, counters[5].Cycle, cpuRegs.cycle - counters[5].sCycleT, cpuRegs.cycle);	
				counters[5].sCycleT += VBLANKCNT(counters[5].Cycle);
				counters[5].count -= counters[5].Cycle;
			}
			counters[5].count++;
			//counters[5].sCycleT += VBLANKCNT(1);
			counters[5].CycleT = VBLANKCNT(counters[5].count) - (VBLANKCNT(1)/2);
			//SysPrintf("%x frames done in %x cycles cpuRegs.cycle = %x cycletdiff %x\n", counters[5].Cycle, counters[5].sCycleT, cpuRegs.cycle, (counters[5].CycleT - VBLANKCNT(1)) - (cpuRegs.cycle - counters[5].sCycleT));
			VSync();
		}

	for (i=0; i<=3; i++) {
		if (!(counters[i].mode & 0x80)) continue; // Stopped

		if ((s64)(counters[i].target - counters[i].count) <= 0 /*&& (counters[i].target & 0xffff) > 0*/) { // Target interrupt
				
				if((counters[i].target > 0xffff)) {
							//SysPrintf("EE Correcting target %x after reset on target\n", i);
							counters[i].target &= 0xffff;
						}

				if(counters[i].mode & 0x100 ) {
#ifdef EECNT_LOG
	EECNT_LOG("EE counter %d target reached mode %x count %x target %x\n", i, counters[i].mode, counters[i].count, counters[i].target);
#endif

					counters[i].mode|= 0x0400; // Target flag
					hwIntcIrq(counters[i].interrupt);
					if (counters[i].mode & 0x40) { //The PS2 only resets if the interrupt is enabled - Tested on PS2
						
						counters[i].count -= counters[i].target; // Reset on target	
					} 
					else counters[i].target += 0x10000000;
				} else counters[i].target += 0x10000000;
			
		}
		if (counters[i].count > 0xffff) {
		
		
			if (counters[i].mode & 0x0200) { // Overflow interrupt
#ifdef EECNT_LOG
	EECNT_LOG("EE counter %d overflow mode %x count %x target %x\n", i, counters[i].mode, counters[i].count, counters[i].target);
#endif
				counters[i].mode|= 0x0800; // Overflow flag
				hwIntcIrq(counters[i].interrupt);
				//SysPrintf("counter[%d] overflow interrupt (%x)\n", i, cpuRegs.cycle);
			}
			counters[i].count -= 0x10000;
			if(counters[i].target > 0xffff) {
				//SysPrintf("EE %x Correcting target on overflow\n", i);
				counters[i].target &= 0xffff;
				}
		} 	

	}
	
	
	rcntSet();

}



void rcntWcount(int index, u32 value) {
	u32 change = 0;
#ifdef EECNT_LOG
	EECNT_LOG("EE count write %d count %x with %x target %x eecycle %x\n", index, counters[index].count, value, counters[index].target, cpuRegs.eCycle);
#endif
	counters[index].count = value & 0xffff;
	if(counters[index].target > 0xffff) {
		counters[index].target &= 0xffff;
		//SysPrintf("EE Counter %x count write, target > 0xffff\n", index);
		}
	//rcntUpd(index);
	if((counters[index].mode & 0x3) != 0x3){
	change = cpuRegs.cycle - counters[index].sCycleT;
	change -= (change / counters[index].rate) * counters[index].rate;
	counters[index].sCycleT = cpuRegs.cycle - change;
	}/* else {
		SysPrintf("EE Counter %x count write %x\n", index, value);
	}*/
	rcntSet();
}

void rcntWmode(int index, u32 value)  
{
	u32 change = 0;


	if (value & 0xc00) { //Clear status flags, the ps2 only clears what is given in the value
		counters[index].mode &= ~(value & 0xc00);
	}

	if(counters[index].mode & 0x80){
		if((counters[index].mode & 0x3) != 0x3){
			change = cpuRegs.cycle - counters[index].sCycleT;
			counters[index].count += (int)(change / counters[index].rate);
			change -= (change / counters[index].rate) * counters[index].rate;
			counters[index].sCycleT = cpuRegs.cycle - change;
		}
		//if(change != 0) SysPrintf("Weee\n");
		//counters[index].sCycleT = cpuRegs.cycle - ((cpuRegs.cycle - counters[index].sCycleT) % counters[index].rate);
		if(!(value & 0x80)) SysPrintf("Stopping\n");
		}
	else {
		SysPrintf("Counter %d not running c%x s%x c%x\n", index, counters[index].count, counters[index].sCycleT, cpuRegs.cycle);
		if(value & 0x80) SysPrintf("Starting %d, v%x\n", index, value);
		counters[index].sCycleT = cpuRegs.cycle;
		}
	//if((value & 0x80) && !(counters[index].mode & 0x80)) rcntUpd(index); //Counter wasnt started, so set the start cycle
		
	counters[index].mode = (counters[index].mode & 0xc00) | (value & 0x3ff);

#ifdef EECNT_LOG
	EECNT_LOG("EE counter set %d mode %x count %x\n", index, counters[index].mode, rcntCycle(index));
#endif
	/*if((value & 0x3) && (counters[index].mode & 0x3) != 0x3){
		//SysPrintf("Syncing %d with HBLANK clock\n", index);
		counters[index].CycleT = counters[4].CycleT;
	}*/

	switch (value & 0x3) {                        //Clock rate divisers *2, they use BUSCLK speed not PS2CLK
		case 0: counters[index].rate = 2; break;
		case 1: counters[index].rate = 32; break;
		case 2: counters[index].rate = 512; break;
		case 3: counters[index].rate = PS2HBLANK; break;
	}
	
	if((counters[index].mode & 0xF) == 0x7) {
		gates &= ~(1<<index);
		SysPrintf("Gate Disabled\n");
		//	counters[index].mode &= ~0x80;
	}else if(counters[index].mode & 0x4){
		   // SysPrintf("Gate enable on counter %x mode %x\n", index, counters[index].mode);
			gates |= 1<<index;
			counters[index].mode &= ~0x80;
			rcntReset(index);
	}
	else gates &= ~(1<<index);
	
	/*if((counters[index].target > 0xffff) && (counters[index].target & 0xffff) > rcntCycle(index)) {
				//SysPrintf("EE Correcting target %x after mode write\n", index);
				counters[index].target &= 0xffff;
				}*/
	rcntSet();

}

void rcntStartGate(int mode){
	int i;

	if(mode == 0){
		for(i = 0; i < 4; i++){ //Update counters using the hblank as the clock
			if((counters[i].mode & 0x83) == 0x83) counters[i].count++;
		}
	}

	for(i=0; i <=3; i++){  //Gates for counters
		if(!(gates & (1<<i))) continue;
		if ((counters[i].mode & 0x8) != mode) continue;
		//SysPrintf("Gate %d mode %d Start\n", i, (counters[i].mode & 0x30) >> 4);
		switch((counters[i].mode & 0x30) >> 4){
			case 0x0: //Count When Signal is low (off)
				counters[i].count = rcntRcount(i);
				rcntUpd(i);
				counters[i].mode &= ~0x80;
				break;
			case 0x1: //Reset and start counting on Vsync start
				counters[i].mode |= 0x80;
				rcntReset(i);
				counters[i].target &= 0xffff;
				break;
			case 0x2: //Reset and start counting on Vsync end
				//Do Nothing
				break;
			case 0x3: //Reset and start counting on Vsync start and end
				counters[i].mode |= 0x80;
				rcntReset(i);
				counters[i].target &= 0xffff;
				break;
			default:
				SysPrintf("EE Start Counter %x Gate error\n", i);
				break;
		}
	}
}
void rcntEndGate(int mode){
	int i;

	for(i=0; i <=3; i++){  //Gates for counters
		if(!(gates & (1<<i))) continue;
		if ((counters[i].mode & 0x8) != mode) continue;
		//SysPrintf("Gate %d mode %d End\n", i, (counters[i].mode & 0x30) >> 4);
		switch((counters[i].mode & 0x30) >> 4){
			case 0x0: //Count When Signal is low (off)
				rcntUpd(i);
				counters[i].mode |= 0x80;
				break;
			case 0x1: //Reset and start counting on Vsync start
				//Do Nothing
				break;
			case 0x2: //Reset and start counting on Vsync end
				counters[i].mode |= 0x80;
				rcntReset(i);
				counters[i].target &= 0xffff;
				break;
			case 0x3: //Reset and start  counting on Vsync start and end
				counters[i].mode |= 0x80;
				rcntReset(i);			
				counters[i].target &= 0xffff;
				break;
			default:
				SysPrintf("EE Start Counter %x Gate error\n", i);
				break;
		}
	}
}
void rcntWtarget(int index, u32 value) {

#ifdef EECNT_LOG
	EECNT_LOG("EE target write %d target %x value %x\n", index, counters[index].target, value);
#endif
	counters[index].target = value & 0xffff;
		if(counters[index].target <= rcntCycle(index)/* && counters[index].target != 0*/) {
			//SysPrintf("EE Saving target %d from early trigger, target = %x, count = %x\n", index, counters[index].target, rcntCycle(index));
			counters[index].target += 0x10000000;
			}
	rcntSet();
}

void rcntWhold(int index, u32 value) {
#ifdef EECNT_LOG
	EECNT_LOG("EE hold write %d value %x\n", index, value);
#endif
	counters[index].hold = value;
}

u16 rcntRcount(int index) {
	u16 ret;

	if ((counters[index].mode & 0x80)) {
		ret = counters[index].count + (int)((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	}else{
		ret = counters[index].count;
	}
#ifdef EECNT_LOG
	EECNT_LOG("EE count read %d value %x\n", index, ret);
#endif
	return (u16)ret;
}

u32 rcntCycle(int index) {

	if ((counters[index].mode & 0x80)) {
		return (u32)counters[index].count + (int)((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	}else{
		return (u32)counters[index].count;
	}
}

int rcntFreeze(gzFile f, int Mode) {
	gzfreezel(counters);
	gzfreeze(&nextCounter, sizeof(nextCounter));
	gzfreeze(&nextsCounter, sizeof(nextsCounter));

	return 0;
}

