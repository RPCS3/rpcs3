#ifndef __LIBISO_H__
#define __LIBISO_H__

#ifdef __MSCW32__
#pragma warning(disable:4018)
#endif

#define CDVDdefs
#include "PS2Etypes.h"
#include "PS2Edefs.h"

#define ISOTYPE_ILLEGAL	0
#define ISOTYPE_CD		1
#define ISOTYPE_DVD		2
#define ISOTYPE_AUDIO	3

#define ISOFLAGS_Z			0x1
#define ISOFLAGS_Z2			0x2
#define ISOFLAGS_BLOCKDUMP	0x4

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

typedef struct {
	char filename[256];
	u32  type;
	u32  flags;
	u32  offset;
	u32  blockofs;
	u32  blocksize;
	u32  blocks;
	void *handle;
	void *htable;
	char *Ztable;
	u32  *dtable;
	int  dtablesize;
	char buffer[CD_FRAMESIZE_RAW * 10];
} isoFile;


isoFile *isoOpen(const char *filename);
isoFile *isoCreate(const char *filename, int mode);
int  isoSetFormat(isoFile *iso, int blockofs, int blocksize, int blocks);
int  isoDetect(isoFile *iso);
int  isoReadBlock(isoFile *iso, char *dst, int lsn);
int  isoWriteBlock(isoFile *iso, char *src, int lsn);
void isoClose(isoFile *iso);

void *_openfile(const char *filename, int flags);
u64 _tellfile(void *handle);
int _seekfile(void *handle, u64 offset, int whence);
int _readfile(void *handle, void *dst, int size);
int _writefile(void *handle, void *src, int size);
void _closefile(void *handle);

#endif /* __LIBISO_H__ */
