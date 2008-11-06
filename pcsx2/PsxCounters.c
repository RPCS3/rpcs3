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
#include <math.h>
#include "PsxCommon.h"

psxCounter psxCounters[8];
u32 psxNextCounter, psxNextsCounter;
static int cnts = 6;
u8 psxhblankgate = 0;
u8 psxvblankgate = 0;
u8 psxcntmask = 0;

static void psxRcntUpd16(u32 index) {
	psxCounters[index].sCycleT = psxRegs.cycle;
}

static void psxRcntUpd32(u32 index) {
	psxCounters[index].sCycleT = psxRegs.cycle;
}

static void psxRcntReset16(u32 index) {
	psxCounters[index].count = 0;

	psxCounters[index].mode&= ~0x18301C00;
	psxRcntUpd16(index);
}

static void psxRcntReset32(u32 index) {
	psxCounters[index].count = 0;

	psxCounters[index].mode&= ~0x18301C00;
	psxRcntUpd32(index);
}

static void psxRcntSet() {
	u64 c;
	int i;

	psxNextCounter = 0xffffffff;
	psxNextsCounter = psxRegs.cycle;

	for (i=0; i<3; i++) {
		if(psxCounters[i].rate == PSXHBLANK) continue;
		c = (u64)((0x10000 - psxCounters[i].count) * psxCounters[i].rate) - (psxRegs.cycle - psxCounters[i].sCycleT);
		if (c < psxNextCounter) {
			psxNextCounter = (u32)c;
		}
		//if((psxCounters[i].mode & 0x10) == 0 || psxCounters[i].target > 0xffff) continue;
		c = (u64)((psxCounters[i].target - psxCounters[i].count) * psxCounters[i].rate) - (psxRegs.cycle - psxCounters[i].sCycleT);
		if (c < psxNextCounter) {
			psxNextCounter = (u32)c;
		}
			
	}
	for (i=3; i<6; i++) {
		if(psxCounters[i].rate == PSXHBLANK) continue;
		c = (u64)((0x100000000 - psxCounters[i].count) * psxCounters[i].rate) - (psxRegs.cycle - psxCounters[i].sCycleT);
		if (c < psxNextCounter) {
			psxNextCounter = (u32)c;
		}
		
		//if((psxCounters[i].mode & 0x10) == 0 || psxCounters[i].target > 0xffffffff) continue;
		c = (u64)((psxCounters[i].target - psxCounters[i].count) * psxCounters[i].rate) - (psxRegs.cycle - psxCounters[i].sCycleT);
		if (c < psxNextCounter) {
			psxNextCounter = (u32)c;
		}
		
	}

	if(SPU2async)
	{
		c = (u32)(psxCounters[6].CycleT - (psxRegs.cycle - psxCounters[6].sCycleT)) ;
			if (c < psxNextCounter) {
				psxNextCounter = (u32)c;
			}
	}
	if(USBasync)
	{
		c = (u32)(psxCounters[7].CycleT - (psxRegs.cycle - psxCounters[7].sCycleT)) ;
			if (c < psxNextCounter) {
				psxNextCounter = (u32)c;
			}
	}
}


void psxRcntInit() {
	int i;

	memset(psxCounters, 0, sizeof(psxCounters));

	for (i=0; i<3; i++) {
		psxCounters[i].rate = 1;
		psxCounters[i].mode|= 0x0400;
		psxCounters[i].target = 0x0;
	}
	for (i=3; i<6; i++) {
		psxCounters[i].rate = 1;
		psxCounters[i].mode|= 0x0400;
		psxCounters[i].target = 0x0;
	}

	psxCounters[0].interrupt = 0x10;
	psxCounters[1].interrupt = 0x20;
	psxCounters[2].interrupt = 0x40;

	psxCounters[3].interrupt = 0x04000;
	psxCounters[4].interrupt = 0x08000;
	psxCounters[5].interrupt = 0x10000;

	if (SPU2async != NULL) {
		

		psxCounters[6].rate = 1;
		psxCounters[6].CycleT = ((Config.Hacks & 0x4) ? 768 : 9216);
		psxCounters[6].mode = 0x8;
	}

	if (USBasync != NULL) {
		psxCounters[7].rate = 1;
		psxCounters[7].CycleT = PSXCLK/1000;
		psxCounters[7].mode = 0x8;
	}

	for (i=0; i<3; i++)
		psxCounters[i].sCycleT = psxRegs.cycle;
	for (i=3; i<6; i++)
		psxCounters[i].sCycleT = psxRegs.cycle;
	for (i=6; i<8; i++)
		psxCounters[i].sCycleT = psxRegs.cycle;

	psxRcntSet();
}

void psxVSyncStart() {
	cdvdVsync();
	psxHu32(0x1070)|= 1;
	if(psxvblankgate & 1) psxCheckStartGate(1);
	if(psxvblankgate & (1 << 3)) psxCheckStartGate(3);
}

void psxVSyncEnd() {
	psxHu32(0x1070)|= 0x800;
	if(psxvblankgate & 1) psxCheckEndGate(1);
	if(psxvblankgate & (1 << 3)) psxCheckEndGate(3);
}
void psxCheckEndGate(int counter) { //Check Gate events when Vsync Ends
	int i = counter;
	//SysPrintf("End Gate %x\n", counter);
	if(counter < 3){  //Gates for 16bit counters
		if((psxCounters[i].mode & 0x1) == 0) return; //Ignore Gate
	
		switch((psxCounters[i].mode & 0x6) >> 1) {
			case 0x0: //GATE_ON_count
				psxCounters[i].count += (u16)psxRcntRcount16(i); //Only counts when signal is on
				break;
			case 0x1: //GATE_ON_ClearStart
				if(psxCounters[i].mode & 0x10000000)psxRcntUpd16(i);
				psxCounters[i].mode &= ~0x10000000;
				break;
			case 0x2: //GATE_ON_Clear_OFF_Start
				psxCounters[i].mode &= ~0x10000000;
				psxRcntUpd16(i);
				break;
			case 0x3: //GATE_ON_Start
				break;
			default:
				SysPrintf("PCSX2 Warning: 16bit IOP Counter Gate Not Set!\n");
				break;
		}
	}

	if(counter >= 3){  //Gates for 32bit counters
		if((psxCounters[i].mode & 0x1) == 0) return; //Ignore Gate

		switch((psxCounters[i].mode & 0x6) >> 1) {
			case 0x0: //GATE_ON_count
				psxCounters[i].count += (u32)psxRcntRcount32(i);  //Only counts when signal is on
				break;
			case 0x1: //GATE_ON_ClearStart
				if(psxCounters[i].mode & 0x10000000)psxRcntUpd32(i);
				psxCounters[i].mode &= ~0x10000000;
				break;
			case 0x2: //GATE_ON_Clear_OFF_Start
				psxCounters[i].mode &= ~0x10000000;
				psxRcntUpd32(i);
				break;
			case 0x3: //GATE_ON_Start
				break;
			default:
				SysPrintf("PCSX2 Warning: 32bit IOP Counter Gate Not Set!\n");
				break;
		}
	}
}
void psxCheckStartGate(int counter) {  //Check Gate events when Vsync Starts
	int i = counter;

	if(counter == 0){
		if((psxCounters[1].mode & 0x101) == 0x100 || (psxCounters[1].mode & 0x10000101) == 0x101)psxCounters[1].count++;
		if((psxCounters[3].mode & 0x101) == 0x100 || (psxCounters[3].mode & 0x10000101) == 0x101)psxCounters[3].count++;
		/*if(SPU2async)
			{	
				SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);	
				//SysPrintf("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);
				psxCounters[6].sCycleT = psxRegs.cycle;
			}*/
	}
	
	if(counter < 3){  //Gates for 16bit counters
		if((psxCounters[i].mode & 0x1) == 0) return; //Ignore Gate
		//SysPrintf("PSX Gate %x\n", i);
		switch((psxCounters[i].mode & 0x6) >> 1) {
			case 0x0: //GATE_ON_count
				psxRcntUpd32(i);
				psxCounters[i].mode |= 0x10000000;
				break;
			case 0x1: //GATE_ON_ClearStart
				if(psxCounters[i].mode & 0x10000000)psxRcntUpd16(i);
				psxCounters[i].mode &= ~0x10000000;
				break;
			case 0x2: //GATE_ON_Clear_OFF_Start
				psxRcntReset32(i);
				psxCounters[i].mode |= 0x10000000;
				break;
			case 0x3: //GATE_ON_Start
				psxCounters[i].mode &= ~0x10000000;
				break;
			default:
				SysPrintf("PCSX2 Warning: 16bit IOP Counter Gate Not Set!\n");
				break;
		}
	}

	if(counter >= 3){  //Gates for 32bit counters
		if((psxCounters[i].mode & 0x1) == 0) return; //Ignore Gate
		//SysPrintf("PSX Gate %x\n", i);
		switch((psxCounters[i].mode & 0x6) >> 1) {
			case 0x0: //GATE_ON_count
				psxRcntUpd32(i);
				psxCounters[i].mode &= ~0x10000000;
				break;
			case 0x1: //GATE_ON_ClearStart
				if(psxCounters[i].mode & 0x10000000)psxRcntUpd32(i);
				psxCounters[i].mode &= ~0x10000000;
				break;
			case 0x2: //GATE_ON_Clear_OFF_Start
				psxRcntReset32(i);
				psxCounters[i].mode |= 0x10000000;
				break;
			case 0x3: //GATE_ON_Start
				psxCounters[i].mode &= ~0x10000000;
				break;
			default:
				SysPrintf("PCSX2 Warning: 32bit IOP Counter Gate Not Set!\n");
				break;
		}
	}
}

void _testRcnt16target(int i) {

#ifdef PSXCNT_LOG
	PSXCNT_LOG("[%d] target %x >= %x (CycleT); count=%I64x, target=%I64x, mode=%I64x\n", i, (psxRegs.cycle - psxCounters[i].sCycleT), psxCounters[i].CycleT, psxCounters[i].count, psxCounters[i].target,  psxCounters[i].mode);
#endif

	if(psxCounters[i].target > 0xffff) {
			psxCounters[i].target &= 0xffff;
			//SysPrintf("IOP 16 Correcting target target %x\n", psxCounters[i].target);			
		}
	
	if (psxCounters[i].mode & 0x10){
			if(psxCounters[i].mode & 0x80)psxCounters[i].mode&= ~0x0400; // Interrupt flag
			psxCounters[i].mode|= 0x0800; // Target flag
		}
	
		
	
	if (psxCounters[i].mode & 0x10) { // Target interrupt
		psxHu32(0x1070)|= psxCounters[i].interrupt;			
		}
		
	if (psxCounters[i].mode & 0x08) { // Reset on target
		psxCounters[i].count -= psxCounters[i].target;
		if((psxCounters[i].mode & 0x40) == 0){
				SysPrintf("Counter %x repeat intr not set on zero ret, ignoring target\n", i);
				psxCounters[i].target += 0x1000000000;
				}
		
	} else psxCounters[i].target += 0x1000000000;
	
}

void _testRcnt16overflow(int i) {

#ifdef PSXCNT_LOG
		PSXCNT_LOG("[%d] overflow 0x%x >= 0x%x (Cycle); Rcount=0x%x, count=0x%x\n", i, (psxRegs.cycle - psxCounters[i].sCycle) / psxCounters[i].rate, psxCounters[i].Cycle, psxRcntRcount16(i), psxCounters[i].count);
#endif	

	
	if (psxCounters[i].mode & 0x0020) { // Overflow interrupt
		psxHu32(0x1070)|= psxCounters[i].interrupt;
		psxCounters[i].mode|= 0x1000; // Overflow flag
		if(psxCounters[i].mode & 0x80){
			
			psxCounters[i].mode&= ~0x0400; // Interrupt flag
		}
		//SysPrintf("Overflow 16\n");
	}
	psxCounters[i].count -= 0x10000;
	psxRcntUpd16(i);
	if(psxCounters[i].target > 0xffff) {
		if((psxCounters[i].mode & 0x50) <= 0x40 && (psxCounters[i].mode & 0x50) != 0) SysPrintf("Counter %x overflowing, no repeat interrupt mode = %x\n", i, psxCounters[i].mode);
		psxCounters[i].target &= 0xffff;
		//SysPrintf("IOP 16 Correcting target ovf %x\n", psxCounters[i].target);
		}
}

void _testRcnt32target(int i) {
	
#ifdef PSXCNT_LOG
	PSXCNT_LOG("[%d] target %x >= %x (CycleT); count=%I64x, target=%I64x, mode=%I64x\n", i, (psxRegs.cycle - psxCounters[i].sCycleT), psxCounters[i].CycleT, psxCounters[i].count, psxCounters[i].target,  psxCounters[i].mode);
#endif
	if(psxCounters[i].target > 0xffffffff) {
				//SysPrintf("IOP 32 Correcting target on target reset\n");
				psxCounters[i].target &= 0xffffffff;
			}	

	
//	newtarget[i] = 0;

	
		if (psxCounters[i].mode & 0x10){
			if(psxCounters[i].mode & 0x80)psxCounters[i].mode&= ~0x0400; // Interrupt flag
			psxCounters[i].mode|= 0x0800; // Target flag
		}
			

	if (psxCounters[i].mode & 0x10) { // Target interrupt
		psxHu32(0x1070)|= psxCounters[i].interrupt;	
		
		}
	
	if (psxCounters[i].mode & 0x08) { // Reset on target
			psxCounters[i].count -= psxCounters[i].target;
			if((psxCounters[i].mode & 0x40) == 0){
				SysPrintf("Counter %x repeat intr not set on zero ret, ignoring target\n", i);
				psxCounters[i].target += 0x1000000000;
				}
			
	} else psxCounters[i].target += 0x1000000000;
	
}

void _testRcnt32overflow(int i) {

#ifdef PSXCNT_LOG
	PSXCNT_LOG("[%d] overflow 0x%x >= 0x%x (Cycle); Rcount=0x%x, count=0x%x\n", i, (psxRegs.cycle - psxCounters[i].sCycle), psxCounters[i].Cycle, psxRcntRcount32(i), psxCounters[i].count);
#endif	
	//SysPrintf("Overflow 32\n");
	if (psxCounters[i].mode & 0x0020) { // Overflow interrupt
		psxHu32(0x1070)|= psxCounters[i].interrupt;
		psxCounters[i].mode|= 0x1000; // Overflow flag
		if(psxCounters[i].mode & 0x80){
			
			psxCounters[i].mode&= ~0x0400; // Interrupt flag
		}
	}
	psxCounters[i].count -= 0x100000000;
	if(psxCounters[i].target > 0xffffffff) {
		//SysPrintf("IOP 32 Correcting target on overflow\n");
		if((psxCounters[i].mode & 0x50) <= 0x40 && (psxCounters[i].mode & 0x50) != 0) SysPrintf("Counter %x overflowing, no repeat interrupt mode = %x\n", i, psxCounters[i].mode);
		psxCounters[i].target &= 0xffffffff;
		}
}


void _testRcnt16(int i) {

	if (/*psxCounters[i].target > 0 &&*/ (s64)(psxCounters[i].target - psxCounters[i].count) <= 0){
		_testRcnt16target(i);
	}
	if (psxCounters[i].count > 0xffff)
		_testRcnt16overflow(i);
}

void _testRcnt32(int i) {

	if (/*psxCounters[i].target > 0 && */(s64)(psxCounters[i].target - psxCounters[i].count) <= 0){
		_testRcnt32target(i);
	}
	if (psxCounters[i].count > 0xffffffff)
		_testRcnt32overflow(i);
	
	
}
extern int spu2interrupts[2];
void psxRcntUpdate() {
	int i;
	u32 change = 0;

	for (i=0; i<=5; i++) {
		if((psxCounters[i].mode & 0x1) != 0 || psxCounters[i].rate == PSXHBLANK){
			//SysPrintf("Stopped accidental update of psx counter %x when using a gate\hblank source\n", i);
			continue;
			}
		
			change = psxRegs.cycle - psxCounters[i].sCycleT;
			psxCounters[i].count += change / psxCounters[i].rate;
			if(psxCounters[i].rate != 1){
				change -= (change / psxCounters[i].rate) * psxCounters[i].rate;
				psxCounters[i].sCycleT = psxRegs.cycle - change;
			//if(change > 0) SysPrintf("PSX Change saved on %x = %x\n", i, change);
			} else psxCounters[i].sCycleT = psxRegs.cycle;
	}

	_testRcnt16(0);
	_testRcnt16(1);
	_testRcnt16(2);
	_testRcnt32(3);
	_testRcnt32(4);
	_testRcnt32(5);


	if(SPU2async)
			{	
				if((psxRegs.cycle - psxCounters[6].sCycleT) >= psxCounters[6].CycleT){
				SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);	
				//SysPrintf("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);
				psxCounters[6].sCycleT = psxRegs.cycle;
				psxCounters[6].CycleT = ((Config.Hacks & 0x4) ? 768 : 9216);
			}
	}

	if(USBasync)
	{
		if ((psxRegs.cycle - psxCounters[7].sCycleT) >= psxCounters[7].CycleT) {
			USBasync(psxRegs.cycle - psxCounters[7].sCycleT);		
			psxCounters[7].sCycleT = psxRegs.cycle;
		}
	}

	psxRcntSet();
}

void psxRcntWcount16(int index, u32 value) {
	u32 change = 0;
#ifdef PSXCNT_LOG
	PSXCNT_LOG("writeCcount[%d] = %x\n", index, value);
#endif

	change = psxRegs.cycle - psxCounters[index].sCycleT;
		//psxCounters[i].count += change / psxCounters[i].rate;
		if(psxCounters[index].rate != 1){
			change -= (change / psxCounters[index].rate) * psxCounters[index].rate;
			psxCounters[index].sCycleT = psxRegs.cycle - change;
		if(change > 0) SysPrintf("PSX Change count write 16 %x = %x\n", index, change);
		} 
	psxCounters[index].count = value & 0xffff;
	if(psxCounters[index].target > 0xffff) {
		//SysPrintf("IOP 16 Correcting target on count write\n");
		psxCounters[index].target &= 0xffff;
		}
	if(psxCounters[index].rate == PSXHBLANK)SysPrintf("Whoops 16 IOP\n");
	psxRcntUpd16(index);
	psxRcntSet();
}

void psxRcntWcount32(int index, u32 value) {
	u32 change = 0;
#ifdef PSXCNT_LOG
	PSXCNT_LOG("writeCcount[%d] = %x\n", index, value);
#endif

		change = psxRegs.cycle - psxCounters[index].sCycleT;
		//psxCounters[i].count += change / psxCounters[i].rate;
		if(psxCounters[index].rate != 1){
			change -= (change / psxCounters[index].rate) * psxCounters[index].rate;
			psxCounters[index].sCycleT = psxRegs.cycle - change;
		if(change > 0) SysPrintf("PSX Change count write 32 %x = %x\n", index, change);
		} 
	psxCounters[index].count = value;
	if(psxCounters[index].target > 0xffffffff) {
		//SysPrintf("IOP 32 Correcting target\n");
		psxCounters[index].target &= 0xffffffff;
		}
	if(psxCounters[index].rate == PSXHBLANK)SysPrintf("Whoops 32 IOP\n");
	//psxRcntUpd32(index);
	psxRcntSet();
}

void psxRcnt0Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[0] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 0 Value write %x\n", value & 0x1c00);
	}

	psxCounters[0].mode = value;
	psxCounters[0].mode|= 0x0400;
	psxCounters[0].rate = 1;

	if(value & 0x100) {
		//SysPrintf("Timer 0 Set to Pixel clock %x\n", value);
		psxCounters[0].rate = PSXPIXEL;
		}// else SysPrintf("Timer 0 Set to 1 %x\n", value);
	
	if(psxCounters[0].mode & 0x1){
		SysPrintf("Gate Check set on Counter 0 %x\n", value);
		psxCounters[0].mode|= 0x1000000;
		psxhblankgate |= 1;
	}else
		psxhblankgate &= ~1;

	psxCounters[0].count = 0;
	psxRcntUpd16(0);
	if(psxCounters[0].target > 0xffff) {
		//SysPrintf("IOP 16 Correcting target 0 after mode write\n");
		psxCounters[0].target &= 0xffff;
		}
	psxRcntSet();
	//}
}

void psxRcnt1Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[1] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 1 Value write %x\n", value & 0x1c00);
	}

	psxCounters[1].mode = value;
	psxCounters[1].mode|= 0x0400;
	psxCounters[1].rate = 1;

	if(value & 0x100){
		//SysPrintf("Timer 1 Set to HBlank clock %x\n", value);
		psxCounters[1].rate = PSXHBLANK;
		}// else SysPrintf("Timer 1 Set to 1 clock %x\n", value);

	if(psxCounters[1].mode & 0x1){
		SysPrintf("Gate Check set on Counter 1 %x\n", value);
		psxCounters[1].mode|= 0x1000000;
		psxvblankgate |= 1<<1;
	}else
		psxvblankgate &= ~(1<<1);

	psxCounters[1].count = 0;
	psxRcntUpd16(1);
	if(psxCounters[1].target > 0xffff) {
		//SysPrintf("IOP 16 Correcting target 1 after mode write\n");
		psxCounters[1].target &= 0xffff;
		}
	psxRcntSet();
	//}
}

void psxRcnt2Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[2] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 2 Value write %x\n", value & 0x1c00);
	}

	psxCounters[2].mode = value;
	psxCounters[2].mode|= 0x0400;

	switch(value & 0x200){
		case 0x200:
			//SysPrintf("Timer 2 Set to 8 %x\n", value);
			psxCounters[2].rate = 8;
			break;
		case 0x000:
			//SysPrintf("Timer 2 Set to 1 %x\n", value);
			psxCounters[2].rate = 1;
			break;
	}

	if((psxCounters[2].mode & 0x7) == 0x7 || (psxCounters[2].mode & 0x7) == 0x1){
		//SysPrintf("Gate set on IOP C2, disabling\n");
		psxCounters[2].mode|= 0x1000000;
	}
	// Need to set a rate and target
	psxCounters[2].count = 0;
	psxRcntUpd16(2);
	if(psxCounters[2].target > 0xffff) {
		//SysPrintf("IOP 16 Correcting target 2 after mode write\n");
		psxCounters[2].target &= 0xffff;
		}
	psxRcntSet();
}

void psxRcnt3Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[3] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 3 Value write %x\n", value & 0x1c00);
	}

	psxCounters[3].mode = value;
	psxCounters[3].rate = 1;
	psxCounters[3].mode|= 0x0400;

	if(value & 0x100){
		//SysPrintf("Timer 3 Set to HBLANK clock %x\n", value);
		psxCounters[3].rate = PSXHBLANK;
		}//else SysPrintf("Timer 3 Set to 1 %x\n", value);
  
	if(psxCounters[3].mode & 0x1){
		SysPrintf("Gate Check set on Counter 3\n");
		psxCounters[3].mode|= 0x1000000;
		psxvblankgate |= 1<<3;
	}else
		psxvblankgate &= ~(1<<3);

	psxCounters[3].count = 0;
	psxRcntUpd32(3);
	if(psxCounters[3].target > 0xffffffff) {
		//SysPrintf("IOP 32 Correcting target 3 after mode write\n");
		psxCounters[3].target &= 0xffffffff;
		}
	psxRcntSet();
	//}
}

void psxRcnt4Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[4] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 4 Value write %x\n", value & 0x1c00);
	}

	psxCounters[4].mode = value;
	psxCounters[4].mode|= 0x0400;

	switch(value & 0x6000){
		case 0x0000:
			//SysPrintf("Timer 4 Set to 1 %x\n", value);
            psxCounters[4].rate = 1;
			break;
		case 0x2000:
			SysPrintf("Timer 4 Set to 8 %x\n", value);
			psxCounters[4].rate = 8;
			break;
		case 0x4000:
			SysPrintf("Timer 4 Set to 16 %x\n", value);
			psxCounters[4].rate = 16;
			break;
		case 0x6000:
			SysPrintf("Timer 4 Set to 256 %x\n", value);
			psxCounters[4].rate = 256;
			break;
	}
	// Need to set a rate and target
	if((psxCounters[4].mode & 0x7) == 0x7 || (psxCounters[4].mode & 0x7) == 0x1){
		SysPrintf("Gate set on IOP C4, disabling\n");
		psxCounters[4].mode|= 0x1000000;
	}
	psxCounters[4].count = 0;
	psxRcntUpd32(4);
	if(psxCounters[4].target > 0xffffffff) {
		//SysPrintf("IOP 32 Correcting target 4 after mode write\n");
		psxCounters[4].target &= 0xffffffff;
		}
	psxRcntSet();
}

void psxRcnt5Wmode(u32 value)  {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("IOP writeCmode[5] = %lx\n", value);
#endif

	if (value & 0x1c00) {
		SysPrintf("Counter 5 Value write %x\n", value & 0x1c00);
	}

	psxCounters[5].mode = value;
	psxCounters[5].mode|= 0x0400;

	switch(value & 0x6000){
		case 0x0000:
			//SysPrintf("Timer 5 Set to 1 %x\n", value);
            psxCounters[5].rate = 1;
			break;
		case 0x2000:
			SysPrintf("Timer 5 Set to 8 %x\n", value);
			psxCounters[5].rate = 8;
			break;
		case 0x4000:
			SysPrintf("Timer 5 Set to 16 %x\n", value);
			psxCounters[5].rate = 16;
			break;
		case 0x6000:
			SysPrintf("Timer 5 Set to 256 %x\n", value);
			psxCounters[5].rate = 256;
			break;
	}
	// Need to set a rate and target
	if((psxCounters[5].mode & 0x7) == 0x7 || (psxCounters[5].mode & 0x7) == 0x1){
		SysPrintf("Gate set on IOP C5, disabling\n");
		psxCounters[5].mode|= 0x1000000;
	}
	psxCounters[5].count = 0;
	psxRcntUpd32(5);
	if(psxCounters[5].target > 0xffffffff) {
		//SysPrintf("IOP 32 Correcting target 5 after mode write\n");
		psxCounters[5].target &= 0xffffffff;
		}
	psxRcntSet();
}

void psxRcntWtarget16(int index, u32 value) {
#ifdef PSXCNT_LOG
	PSXCNT_LOG("writeCtarget16[%ld] = %lx\n", index, value);
#endif
	psxCounters[index].target = value & 0xffff;
	if(psxCounters[index].target <= psxRcntCycles(index)/* && psxCounters[index].target != 0*/) {
		//SysPrintf("IOP 16 Saving %x target from early trigger target = %x, count = %I64x\n", index, psxCounters[index].target, psxRcntCycles(index));
		psxCounters[index].target += 0x1000000000;
		}

	psxRcntSet();
}

void psxRcntWtarget32(int index, u32 value) {
#ifdef PSXCNT_LOG
		PSXCNT_LOG("writeCtarget32[%ld] = %lx (count=%lx) ; sCycleT: %x CycleT: %x psxRegscycle %x\n",
			   index, value, psxCounters[index].count, psxCounters[index].sCycleT, psxCounters[index].CycleT, psxRegs.cycle);
#endif

	psxCounters[index].target = value;
	if(psxCounters[index].target <= psxRcntCycles(index)/* && psxCounters[index].target != 0*/) {
		//SysPrintf("IOP 32 Saving %x target from early trigger target = %x, count = %I64x\n", index, psxCounters[index].target, psxRcntCycles(index));
		psxCounters[index].target += 0x1000000000;
		}

	psxRcntSet();
}

u16 psxRcntRcount16(int index) {
	if(psxCounters[index].mode & 0x1000000) return (u16)psxCounters[index].count;
	return (u16)(psxCounters[index].count + (u32)((psxRegs.cycle - psxCounters[index].sCycleT) / psxCounters[index].rate));
}

u32 psxRcntRcount32(int index) {
	if(psxCounters[index].mode & 0x1000000) return (u32)psxCounters[index].count;
	return (u32)(psxCounters[index].count + (u32)((psxRegs.cycle - psxCounters[index].sCycleT) / psxCounters[index].rate));
}

u64 psxRcntCycles(int index) {
	if(psxCounters[index].mode & 0x1000000) return psxCounters[index].count;
	return (u64)(psxCounters[index].count + (u32)((psxRegs.cycle - psxCounters[index].sCycleT) / psxCounters[index].rate));
}

extern u32 dwCurSaveStateVer;
int psxRcntFreeze(gzFile f, int Mode)
{
    if( Mode == 0 && (dwCurSaveStateVer < 0x7a300010) ) { // reading
        // struct used to be 32bit count and target
        int i;
        u32 val;
        for(i = 0; i < ARRAYSIZE(psxCounters); ++i) {
            gzfreeze(&val,4); psxCounters[i].count = val;
            gzfreeze(&val,4); psxCounters[i].mode = val;
            gzfreeze(&val,4); psxCounters[i].target = val;
            gzfreeze((u8*)&psxCounters[i].rate, sizeof(psxCounters[i])-20);
        }
    }
    else
	    gzfreezel(psxCounters);


	return 0;
}
