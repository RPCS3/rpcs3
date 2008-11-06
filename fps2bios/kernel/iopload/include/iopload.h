#ifndef __IOPLOAD_H__
#define __IOPLOAD_H__

#include <tamtypes.h>

extern char *iopModules[32];

void Kmemcpy(void *dest, const void *src, int n);

#define SBUS_MSFLG		*(volatile int*)0xBD00F220
#define SBUS_SMFLG		*(volatile int*)0xBD00F230
#define SBUS_F240		*(volatile int*)0xBD00F240

#define SBFLG_IOPALIVE	0x10000
#define SBFLG_IOPSYNC	0x40000


#endif /* __IOPLOAD_H__ */
