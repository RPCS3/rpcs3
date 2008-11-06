#ifndef __TIMEMAN_H__
#define __TIMEMAN_H__

#define TIMEMAN_VER	0x101

//timids << 2; use AllocHardTimer or ReferHardTimer to get one
#define RTC0		0xBF801100
#define RTC1		0xBF801110
#define RTC2		0xBF801120

#define RTC3		0xBF801480
#define RTC4		0xBF801490
#define RTC5		0xBF8014A0

#define RTC_HOLDREGS	0xBF8014B0
#define RTC_HOLDMODE	(*(volatile unsigned int*)0xBF8014C0)

//source
#define TC_SYSCLOCK	1
#define	TC_PIXEL	2
#define	TC_HLINE	4
#define	TC_HOLD		8

//size
#define	TIMER_SIZE_16	16
#define	TIMER_SIZE_32	32

//prescale
#define TIMER_PRESCALE_1	1
#define TIMER_PRESCALE_8	8
#define TIMER_PRESCALE_16	16
#define TIMER_PRESCALE_256	256

int 		AllocHardTimer(int source, int size, int prescale);	//4
int 		ReferHardTimer(int source, int size, int mode, int modemask);//5
int 		FreeHardTimer(int timid);				//6
void		SetTimerMode(int timid, int mode);			//7
unsigned int	GetTimerStatus(int timid);				//8
void		SetTimerCounter(int timid, unsigned int count);		//9
unsigned int	GetTimerCounter(int timid);				//10
void 		SetTimerCompare(int timid, unsigned int compare);	//11
unsigned int	GetTimerCompare(int timid);				//12
void		SetHoldMode(int holdnum, int mode);                     //13
unsigned long	GetHoldMode(int holdnum);				//14
unsigned long	GetHoldReg(int holdnum);				//15
int		GetHardTimerIntrCode(int timid);			//16

#endif//__TIMEMAN_H__