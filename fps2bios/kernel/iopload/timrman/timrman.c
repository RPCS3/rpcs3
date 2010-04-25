//[module]	TIMEMANI
//[processor]	IOP
//[type]	ELF-IRX
//[name]	Timer_Manager
//[version]	0x101
//[memory map]	0xBF801100, 0xBF801110, 0xBF801120,
//		0xBF801450
//		0xBF801480, 0xBF801490, 0xBF8014A0,
//		0xBF8014B0, 0xBF8014C0
//[handlers]	-
//[entry point]	timrman_start, timrman_stub
//[made by]	[RO]man (roman_ps2dev@hotmail.com) 13 October 2002

#include <tamtypes.h>

#include "iopdebug.h"
#include "kloadcore.h"
#include "kintrman.h"

#include "err.h"
#include "ktimrman.h"

#define CONFIG_1450		(*(volatile int*)0xBF801450)

struct TIMRMAN{
	int	hwreg;
	char	source, size;
	short	prescale;
	int	allocated;
} t[6]={
{RTC2, TC_SYSCLOCK,		     TIMER_SIZE_16, TIMER_PRESCALE_8,   0},
{RTC5, TC_SYSCLOCK,		     TIMER_SIZE_32, TIMER_PRESCALE_256, 0},
{RTC4, TC_SYSCLOCK,		     TIMER_SIZE_32, TIMER_PRESCALE_256, 0},
{RTC3, TC_SYSCLOCK|TC_HLINE,	     TIMER_SIZE_32, TIMER_PRESCALE_1,   0},
{RTC0, TC_SYSCLOCK|TC_PIXEL|TC_HOLD, TIMER_SIZE_16, TIMER_PRESCALE_1,   0},
{RTC1, TC_SYSCLOCK|TC_HLINE|TC_HOLD, TIMER_SIZE_16, TIMER_PRESCALE_1,   0}
};

int  _start(int argc, char* argv[]);

///////////////////////////////////////////////////////////////////////[03]
void *GetTimersTable(){
	return t;	//address of the array
}

///////////////////////////////////////////////////////////////////////[04]
int  AllocHardTimer(int source, int size, int prescale){
    register int	i;
	int				x;

	if (QueryIntrContext())	return ERROR_INTR_CONTEXT;	//intrman

	CpuSuspendIntr(&x);					//intrman

	for (i=0; i<6; i++)
		if (!t[i].allocated   && (t[i].source & source) &&
		    (t[i].size==size) && (t[i].prescale>=prescale)){
			t[i].allocated++;	//u8
			CpuResumeIntr(x);			//intrman
			return t[i].hwreg>>2;
		}

	CpuResumeIntr(x);					//intrman
	return ERROR_NO_TIMER;
}

///////////////////////////////////////////////////////////////////////[05]
int  ReferHardTimer(int source, int size, int mode, int modemask){
    register int	i;
	int				x;

	if (QueryIntrContext())	return ERROR_INTR_CONTEXT;	//intrman

	CpuSuspendIntr(&x);					//intrman

	for (i=0; i<6; i++)
		if (t[i].allocated && (t[i].source & source) && (t[i].size==size) &&
		    (int)(GetTimerStatus(t[i].hwreg>>2) & modemask)==mode){
			t[i].allocated++;	//u8
			CpuResumeIntr(x);			//intrman
			return t[i].hwreg>>2;
		}

	CpuResumeIntr(x);					//intrman
	return ERROR_NO_TIMER;
}

///////////////////////////////////////////////////////////////////////[10]
int  GetHardTimerIntrCode(int timid){
	switch(timid<<2){
	case RTC2:	return INT_RTC2;
	case RTC0:	return INT_RTC0;
	case RTC1:	return INT_RTC1;

	case RTC4:	return INT_RTC4;
	case RTC3:	return INT_RTC3;
	case RTC5:	return INT_RTC5;
	}
	return ERROR_UNK;
}

///////////////////////////////////////////////////////////////////////[06]
int  FreeHardTimer(int timid){
    register int	i;
	int				x;

	if (QueryIntrContext())	return ERROR_INTR_CONTEXT;	//intrman

	for (i=0; i<6; i++)
		if (t[i].hwreg==(timid<<2))
			break;

	if ((i<6) && (t[i].allocated)){
		CpuSuspendIntr(&x);				//intrman
		t[i].allocated--;
		CpuResumeIntr(x);				//intrman
		return ERROR_OK;
	}
	return ERROR_NO_TIMER;

}

///////////////////////////////////////////////////////////////////////[07]
void SetTimerMode(int timid, int mode){
	*(volatile short*)((timid<<2)+4) = mode;
}

///////////////////////////////////////////////////////////////////////[08]
unsigned int GetTimerStatus(int timid){
	return *(volatile unsigned short*)((timid<<2)+4);
}

///////////////////////////////////////////////////////////////////////[09]
void SetTimerCounter(int timid, unsigned int count){
	if ((timid<<2) < RTC3)
		*(volatile short*)((timid<<2)+0)=count;
	else
		*(volatile   int*)((timid<<2)+0)=count;
}

///////////////////////////////////////////////////////////////////////[0A]
unsigned int GetTimerCounter(int timid){
	if ((timid<<2) < RTC3)
		return *(volatile short*)((timid<<2)+0);
	else
		return *(volatile   int*)((timid<<2)+0);
}

///////////////////////////////////////////////////////////////////////[0B]
void SetTimerCompare(int timid, unsigned int compare){
	if ((timid<<2)+8 < RTC3)
		*(volatile short*)((timid<<2)+8)=compare;
	else
		*(volatile   int*)((timid<<2)+8)=compare;
}

///////////////////////////////////////////////////////////////////////[0C]
unsigned int GetTimerCompare(int timid){
	if ((timid<<2)+8 < RTC3)
		return *(volatile short*)((timid<<2)+8);
	else
		return *(volatile   int*)((timid<<2)+8);
}

///////////////////////////////////////////////////////////////////////[0D]
void SetHoldMode(int holdnum, int mode){
	RTC_HOLDMODE=(RTC_HOLDMODE &
		      (      ~(0xF  << (holdnum*4)))) |
		      ((mode & 0xF) << (holdnum*4));
}

///////////////////////////////////////////////////////////////////////[0E]
unsigned long GetHoldMode(int holdnum){
	return (RTC_HOLDMODE >> (holdnum*4)) & 0xF;
}

///////////////////////////////////////////////////////////////////////[0F]
unsigned long GetHoldReg(int holdnum){
	return *(volatile unsigned int*)(RTC_HOLDREGS + (holdnum*4));
}

///////////////////////////////////////////////////////////////////////[01,02]
void _retonly(){}

//////////////////////////////entrypoint///////////////////////////////
struct export timrman_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"timrman",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)GetTimersTable,
	(func)AllocHardTimer,
	(func)ReferHardTimer,
	(func)FreeHardTimer,
	(func)SetTimerMode,
	(func)GetTimerStatus,
	(func)SetTimerCounter,
	(func)GetTimerCounter,
	(func)SetTimerCompare,
	(func)GetTimerCompare,
	(func)SetHoldMode,
	(func)GetHoldMode,
	(func)GetHoldReg,
	(func)GetHardTimerIntrCode,
	0
};

//////////////////////////////entrypoint///////////////////////////////[00]
int  _start(int argc, char* argv[]){
	int i;
	u32 PRid;

	__asm__ (
		"mfc0   %0, $15\n"
		: "=r"(PRid) :
	);


	if ((PRid<0x10) || (CONFIG_1450 & 8))	return 1;

	for (i=5; i>=0; i--)	t[i].allocated=0;

    if( RegisterLibraryEntries(&timrman_stub) > 0 )
        return 1;

    EnableIntr(INT_RTC5);

	return 0;	//loadcore
}

