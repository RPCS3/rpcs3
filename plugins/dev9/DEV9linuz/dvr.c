// DVR code is dummy

#include <stdio.h>
#include "DEV9.h"

#define DEV9_DVR_LOG	DEV9_LOG

void CALLBACK DVRinit(){
}

u32  CALLBACK DVRread32(u32 addr, int size) {
	switch(addr) {
	case DVR_R_STAT:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR STAT %dbit read\n", size*8);
#endif
		return dev9Ru32(addr);
	
	case DVR_R_ACK:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR ACK %dbit read\n", size*8);
#endif
		return dev9Ru32(addr);

	case DVR_R_MASK:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR MASK %dbit read\n", size*8);
#endif
		return dev9Ru32(addr);

	case DVR_R_TASK:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR TASK %dbit read\n", size*8);
#endif
		return 0x3E;

	default:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR Unkwnown %dbit read at address %lx\n", size*8, addr);
#endif
		return 0;
	}
}

void CALLBACK DVRwrite32(u32 addr, u32 value, int size) {

	switch(addr & 0x1FFFFFFF) {
	case DVR_R_ACK:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR ACK %dbit write 0x%08lX\n", size*8, value);
#endif
		dev9Ru32(DVR_R_STAT) &= ~value;//ack the intrs
		break;

	case DVR_R_MASK:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR MASK %dbit write 0x%08lX\n", size*8, value);
#endif
		break;
	
	default:
#ifdef DEV9_DVR_LOG
		DEV9_DVR_LOG("*DVR Unkwnown %dbit write at address 0x%08lX= 0x%08lX\n", size*8, addr, value);
#endif
		break;
	}
}
