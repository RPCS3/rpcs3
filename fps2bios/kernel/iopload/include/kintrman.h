#ifndef __INTRMAN_H__
#define __INTRMAN_H__

#define INTRMAN_VER	0x102

#define INT_VBLANK	0x00

#define INT_CDROM	0x02

#define INT_RTC0        0x04
#define INT_RTC1        0x05
#define INT_RTC2        0x06

#define INT_EVBLANK	0x0B

#define INT_RTC3        0x0E
#define INT_RTC4        0x0F
#define INT_RTC5        0x10

#define INT_DMA9	0x2A	//sif0
#define INT_DMA10	0x2B	//sif1

#define IMODE_DMA_IRM	0x100
#define IMODE_DMA_IQE	0x200

typedef int (*intrh_func)(void*);

int	RegisterIntrHandler(int interrupt, int, intrh_func intrh, void*);
						//4 (13,18)
int	ReleaseIntrHandler(int interrupt);	//5 (18)
int	EnableIntr(int interrupt);		//6 (13,18)
int	DisableIntr(int interrupt, int *oldstat);		//7 (18)
int	CpuEnableIntr();			//9 (18,21,26)
int	CpuSuspendIntr(u32* ictrl);			//17(05,06,07,13,14,18,26)
int	CpuResumeIntr(u32 ictrl);			//18(05,06,07,13,14,18,26)
int	QueryIntrContext();			//23(07,13)

#endif//__INTRMAN_H__
