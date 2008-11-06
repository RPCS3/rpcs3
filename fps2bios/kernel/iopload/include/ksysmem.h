#ifndef __SYSMEM_H__
#define __SYSMEM_H__

#include <tamtypes.h>

#define SYSMEM_VER	0x101

//allocation strategies
#define ALLOC_FIRST	0
#define ALLOC_LAST	1
#define ALLOC_LATER	2

// see QueryBlockTopAddress, QueryBlockSize
#define USED	0x00000000
#define FREE	0x80000000

void		*AllocSysMemory(int flags, int size, void *mem);//4 (11,26)
int		FreeSysMemory(void *mem);			//5 (26)
unsigned int	QueryMemSize();
unsigned int	QueryMaxFreeMemSize();
unsigned int	QueryTotalFreeMemSize();
void		*QueryBlockTopAddress(void *address);
int		QueryBlockSize(void *address);
char		*Kprintf(const char *format,...);		//14(06,14,26)
void		sysmem_call15_set_Kprintf(char* (*newKprintf)(unsigned int unk, const char*, ...), unsigned int newunk);

#endif //__SYSMEM_H__
