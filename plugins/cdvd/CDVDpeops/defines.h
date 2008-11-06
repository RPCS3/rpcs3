/***************************************************************************
                          defines.h  -  description
                             -------------------
    begin                : Wed Sep 18 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/09/19 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

/////////////////////////////////////////////////////////

// general buffer for reading several frames

#pragma pack(1)

typedef struct _FRAMEBUF
{
 DWORD         dwFrame; 
 DWORD         dwFrameCnt;
 DWORD         dwBufLen; 
 unsigned char BufData[1024*1024];
} FRAMEBUF;

#pragma pack()

// raw ioctl structs:

typedef enum _TRACK_MODE_TYPE 
{
 YellowMode2,
 XAForm2,
 CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct _RAW_READ_INFO 
{
 LARGE_INTEGER   DiskOffset;
 ULONG           SectorCount;
 TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

// sub cache:

typedef struct
{
 long   addr;
 void * pNext;
 unsigned char subq[10];
} SUB_DATA;

typedef struct
{
 long   addr;
 void * pNext;
} SUB_CACHE;

// ppf cache:

typedef struct
{
 long   addr;
 void * pNext;
 long   pos;
 long   anz;
 // memdata
} PPF_DATA;

typedef struct
{
 long   addr;
 void * pNext;
} PPF_CACHE;

/////////////////////////////////////////////////////////

#define MODE_BE_1       1
#define MODE_BE_2       2
#define MODE_28_1       3 
#define MODE_28_2       4
#define MODE_28_2048    5
#define MODE_28_2048_Ex 6

#define itob(i)      ((i)/10*16 + (i)%10)
#define btoi(b)      ((b)/16*10 + (b)%16)
#define itod(i)      ((((i)/10)<<4) + ((i)%10))
#define dtoi(b)      ((((b)>>4)&0xf)*10 + (((b)&0xf)%10))

/////////////////////////////////////////////////////////

void addr2time(unsigned long addr, unsigned char *time);
void addr2timeB(unsigned long addr, unsigned char *time);
unsigned long time2addr(unsigned char *time);
unsigned long time2addrB(unsigned char *time);
#ifdef _GCC
#define reOrder i386_reOrder
#endif
unsigned long reOrder(unsigned long value);

/////////////////////////////////////////////////////////
// debug helper

#ifndef _IN_CDR
#ifdef DBGOUT  	 
void auxprintf (LPCTSTR pFormat, ...);
#endif
#endif

/////////////////////////////////////////////////////////

typedef DWORD (*READFUNC)(BOOL bWait,FRAMEBUF * f);
typedef DWORD (*DEINITFUNC)(void);
typedef BOOL  (*READTRACKFUNC)(unsigned long addr);
typedef void  (*GETPTRFUNC)(void);

/////////////////////////////////////////////////////////
        
#define WAITFOREVER    0xFFFFFFFF
#define WAITSUB        10000
#define FRAMEBUFEXTRA  12
#define CDSECTOR       2352
#define MAXCACHEBLOCK  26
#define MAXCDBUFFER    (((MAXCACHEBLOCK+1)*(CDSECTOR+16))+240)

/////////////////////////////////////////////////////////

/* some structs from libcdvd by Hiryu & Sjeep (C) 2002 */

#pragma pack(1)

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
};						//+22

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
};

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
};

#pragma pack()


