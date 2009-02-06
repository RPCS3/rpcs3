/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 */

#ifndef __CDVDISO_H__
#define __CDVDISO_H__

#include "CDVDlib.h"

int CDVD_findfile(char* fname, struct TocEntry* tocEntry);
int CDVD_GetDir_RPC_request(char* pathname, char* extensions, unsigned int inc_dirs);
int CDVD_GetDir_RPC_get_entries(struct TocEntry tocEntry[], int req_entries);

#if defined(__WIN32__)
#pragma pack(1)
#endif

struct rootDirTocHeader
{
	u16	length;			//+00
	u32 tocLBA;			//+02
	u32 tocLBA_bigend;	//+06
	u32 tocSize;		//+0A
	u32 tocSize_bigend;	//+0E
	u8	dateStamp[8];	//+12
	u8	reserved[6];	//+1A
	u8	reserved2;		//+20
	u8	reserved3;		//+21
#if defined(__WIN32__)
};						//+22
#else
} __attribute__((packed));
#endif

struct asciiDate
{
	char	year[4];
	char	month[2];
	char	day[2];
	char	hours[2];
	char	minutes[2];
	char	seconds[2];
	char	hundreths[2];
	char	terminator[1];
#if defined(__WIN32__)
};
#else
} __attribute__((packed));
#endif

struct cdVolDesc
{
	u8		filesystemType;	// 0x01 = ISO9660, 0x02 = Joliet, 0xFF = NULL
	u8		volID[5];		// "CD001"
	u8		reserved2;
	u8		reserved3;
	u8		sysIdName[32];
	u8		volName[32];	// The ISO9660 Volume Name
	u8		reserved5[8];
	u32		volSize;		// Volume Size
	u32		volSizeBig;		// Volume Size Big-Endian
	u8		reserved6[32];
	u32		unknown1;
	u32		unknown1_bigend;
	u16		volDescSize;									//+80
	u16		volDescSize_bigend;								//+82
	u32		unknown3;										//+84
	u32		unknown3_bigend;								//+88
	u32		priDirTableLBA;	// LBA of Primary Dir Table		//+8C
	u32		reserved7;										//+90
	u32		secDirTableLBA;	// LBA of Secondary Dir Table	//+94
	u32		reserved8;										//+98
	struct rootDirTocHeader	rootToc;
	u8		volSetName[128];
	u8		publisherName[128];
	u8		preparerName[128];
	u8		applicationName[128];
	u8		copyrightFileName[37];
	u8		abstractFileName[37];
	u8		bibliographyFileName[37];
	struct	asciiDate	creationDate;
	struct	asciiDate	modificationDate;
	struct	asciiDate	effectiveDate;
	struct	asciiDate	expirationDate;
	u8		reserved10;
	u8		reserved11[1166];
#if defined(__WIN32__)
};
#else
} __attribute__((packed));
#endif

struct dirTableEntry
{
	u8	dirNameLength;
	u8	reserved;
	u32	dirTOCLBA;
	u16 dirDepth;
	u8	dirName[32];
#if defined(__WIN32__)
};
#else
} __attribute__((packed));
#endif

struct dirTocEntry
{
	short	length;
	unsigned int	fileLBA;
	unsigned int	fileLBA_bigend;
	unsigned int	fileSize;
	unsigned int	fileSize_bigend;
	unsigned char	dateStamp[6];
	unsigned char	reserved1;
	unsigned char	fileProperties;
	unsigned char	reserved2[6];
	unsigned char	filenameLength;
	unsigned char	filename[128];
#if defined(__WIN32__)
};
#else
} __attribute__((packed));
#endif	// This is the internal format on the CD
// TocEntry structure contains only the important stuff needed for export

#if defined(__WIN32__)
#pragma pack()
#endif

#endif//__CDVDISO_H__
