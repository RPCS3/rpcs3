#ifndef __ROMDIR_H__
#define __ROMDIR_H__

#include <tamtypes.h>

struct romdir {
	/*following variable must place in designed order*/
	u8  fileName[10];
	u16 extInfoSize;
	u32 fileSize;
} __attribute__ ((packed));

struct rominfo {
	u32 fileOffset;
	u32 fileSize;
};

struct rominfo *romdirGetFile(char *name, struct rominfo *ri);

#endif /* __ROMDIR_H__ */
