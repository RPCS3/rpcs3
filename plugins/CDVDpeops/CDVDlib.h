/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Linux kernel headers
 *  Modified by Florin for PCSX2 emu
 */

#ifndef _CDVDLIB_H
#define _CDVDLIB_H

#define __WIN32__
#define __MSCW32__
#define CDVDdefs
#include "PS2Etypes.h"
#include "PS2Edefs.h"

// Macros for READ Data pattan
#define CdSecS2048		0	// sector size 2048
#define CdSecS2328		1	// sector size 2328	
#define CdSecS2340		2	// sector size 2340

//#define CD_FRAMESIZE_RAW1 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE) /*2340*/
//#define CD_FRAMESIZE_RAW0 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE-CD_HEAD_SIZE) /*2336*/
//#define CD_HEAD_SIZE          4 /* header (address) bytes per raw data frame */
//#define CD_SUBHEAD_SIZE       8 /* subheader bytes per raw XA data frame */
//#define CD_XA_HEAD        (CD_HEAD_SIZE+CD_SUBHEAD_SIZE) /* "before data" part of raw XA frame */

/*
 * A CD-ROM physical sector size is 2048, 2052, 2056, 2324, 2332, 2336, 
 * 2340, or 2352 bytes long.  
 *         Sector types of the standard CD-ROM data formats:
 *
 * format   sector type               user data size (bytes)
 * -----------------------------------------------------------------------------
 *   1     (Red Book)    CD-DA          2352    (CD_FRAMESIZE_RAW)
 *   2     (Yellow Book) Mode1 Form1    2048    (CD_FRAMESIZE)
 *   3     (Yellow Book) Mode1 Form2    2336    (CD_FRAMESIZE_RAW0)
 *   4     (Green Book)  Mode2 Form1    2048    (CD_FRAMESIZE)
 *   5     (Green Book)  Mode2 Form2    2328    (2324+4 spare bytes)
 *
 *
 *       The layout of the standard CD-ROM data formats:
 * -----------------------------------------------------------------------------
 * - audio (red):                  | audio_sample_bytes |
 *                                 |        2352        |
 *
 * - data (yellow, mode1):         | sync - head - data - EDC - zero - ECC |
 *                                 |  12  -   4  - 2048 -  4  -   8  - 276 |
 *
 * - data (yellow, mode2):         | sync - head - data |
 *                                 |  12  -   4  - 2336 |
 *
 * - XA data (green, mode2 form1): | sync - head - sub - data - EDC - ECC |
 *                                 |  12  -   4  -  8  - 2048 -  4  - 276 |
 *
 * - XA data (green, mode2 form2): | sync - head - sub - data - Spare |
 *                                 |  12  -   4  -  8  - 2324 -  4    |
 *
 */

// Macros for Spindle control
#define CdSpinMax		0
#define CdSpinNom		1	// Starts reading data at maximum rotational velocity and if a read error occurs, the rotational velocity is reduced.
#define CdSpinStm		0	// Recommended stream rotation speed.

// Macros for TrayReq
#define CdTrayOpen		0
#define CdTrayClose		1
#define CdTrayCheck		2

/*
 * Macros for sceCdGetDiskType()	//comments translated from japanese;)
 */
#define SCECdIllgalMedia 	0xff
	/* ILIMEDIA (Illegal Media)
             A non-PS / non-PS2 Disc. */
#define SCECdDVDV		0xfe
	/* DVDV (DVD Video)
            A non-PS / non-PS2 Disc, but a DVD Video Disc */
#define SCECdCDDA		0xfd
	/* CDDA (CD DA)
            A non-PS / non-PS2 Disc that include a DA track */
#define SCECdPS2DVD		0x14
	/* PS2DVD  PS2 consumer DVD. */
#define SCECdPS2CDDA		0x13
	/* PS2CDDA PS2 consumer CD that includes a DA track */
#define SCECdPS2CD		0x12
	/* PS2CD   PS2 consumer CD that does not include a DA track */
#define SCECdPSCDDA 		0x11
	/* PSCDDA  PS CD that includes a DA track */
#define SCECdPSCD		0x10
	/* PSCD    PS CD that does not include a DA track */
#define SCECdDETCT		0x01
	/* DETCT (Detecting) Disc distinction action */
#define SCECdNODISC 		0x00
	/* NODISC (No disc) No disc entered */

/*
 *      Media mode
 */
#define SCECdCD         1
#define SCECdDVD        2

typedef struct {
	u8 stat;  			// 0: normal. Any other: error
	u8 second; 			// second (BCD value)
	u8 minute; 			// minute (BCD value)
	u8 hour; 			// hour (BCD value)
	u8 week; 			// week (BCD value)
	u8 day; 			// day (BCD value)
	u8 month; 			// month (BCD value)
	u8 year; 			// year (BCD value)
} CdCLOCK;

typedef struct {
	u32 lsn; 			// Logical sector number of file
	u32 size; 			// File size (in bytes)
	char name[16]; 		// Filename
	u8 date[8]; 		// 1th: Seconds
						// 2th: Minutes
						// 3th: Hours
						// 4th: Date
						// 5th: Month
						// 6th 7th: Year (4 digits)
} CdlFILE;

typedef struct {
	u8 minute; 			// Minutes
	u8 second; 			// Seconds
	u8 sector; 			// Sector
	u8 track; 			// Track number
} CdlLOCCD;

typedef struct {
	u8 trycount; 		// Read try count (No. of error retries + 1) (0 - 255)
	u8 spindlctrl; 		// SCECdSpinStm: Recommended stream rotation speed.
						// SCECdSpinNom: Starts reading data at maximum rotational velocity and if a read error occurs, the rotational velocity is reduced.
	u8 datapattern; 	// SCECdSecS2048: Data size 2048 bytes
						// SCECdSecS2328: 2328 bytes
						// SCECdSecS2340: 2340 bytes
	u8 pad; 			// Padding data produced by alignment.
} CdRMode;

#if defined(__WIN32__)
#pragma pack(1)
#endif

struct TocEntry
{
	u32	fileLBA;
	u32 fileSize;
	u8	fileProperties;
	u8	padding1[3];
	u8	filename[128+1];
	u8	date[7];
#if defined(__WIN32__)
};
#else
} __attribute__((packed));
#endif

#if defined(__WIN32__)
#pragma pack()
#endif

int CDVD_findfile(char* fname, struct TocEntry* tocEntry);
/*
int CdBreak(void);
int CdCallback( void (*func)() );
int CdDiskReady(int mode);
int CdGetDiskType(void);
int CdGetError(void);
u32 CdGetReadPos(void);
int CdGetToc(u8 *toc);
int CdInit(int init_mode);
CdlLOCCD *CdIntToPos(int i, CdlLOCCD *p);
int CdPause(void);
int CdPosToInt(CdlLOCCD *p);*/
int CdRead(u32 lsn, u32 sectors, void *buf, CdRMode *mode);
int DvdRead(u32 lsn, u32 sectors, void *buf, CdRMode *mode);
/*int CdReadClock(CdCLOCK *rtc);
int CdSearchFile (CdlFILE *fp, const char *name);
int CdSeek(u32 lsn);
int CdStandby(void);
int CdStatus(void);
int CdStop(void);
int CdSync(int mode);
int CdTrayReq(int mode, u32 *traycnt);
*/
#endif // _CDVDLIB_H
