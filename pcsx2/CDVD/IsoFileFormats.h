#ifndef __LIBISO_H__
#define __LIBISO_H__

#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#define ISOTYPE_ILLEGAL	0
#define ISOTYPE_CD		1
#define ISOTYPE_DVD		2
#define ISOTYPE_AUDIO	3
#define ISOTYPE_DVDDL	4

#define ISOFLAGS_Z			0x0001
#define ISOFLAGS_Z2			0x0002
#define ISOFLAGS_BLOCKDUMP	0x0004
#define ISOFLAGS_MULTI		0x0008
#define ISOFLAGS_BZ2		0x0010

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

typedef struct
{
	u32 slsn;
	u32 elsn;
	void *handle;
} _multih;

typedef struct
{
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
	_multih multih[8];
	int  buflsn;
	u8 *buffer;
} isoFile;


isoFile *isoOpen(const char *filename);
isoFile *isoCreate(const char *filename, int mode);
int  isoSetFormat(isoFile *iso, int blockofs, int blocksize, int blocks);
int  isoDetect(isoFile *iso);
int  isoReadBlock(isoFile *iso, u8 *dst, int lsn);
int  isoWriteBlock(isoFile *iso, u8 *src, int lsn);
void isoClose(isoFile *iso);

void *_openfile(const char *filename, int flags);
u64 _tellfile(void *handle);
int _seekfile(void *handle, u64 offset, int whence);
int _readfile(void *handle, void *dst, int size);
int _writefile(void *handle, void *src, int size);
void _closefile(void *handle);

#endif /* __LIBISO_H__ */
