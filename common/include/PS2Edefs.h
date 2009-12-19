/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef __PS2EDEFS_H__
#define __PS2EDEFS_H__

/*
 *  PS2E Definitions v0.6.2 (beta)
 *
 *  Author: linuzappz@hotmail.com
 *          shadowpcsx2@yahoo.gr
 *          florinsasu@hotmail.com
 */

/*
 Notes:
 * Since this is still beta things may change.

 * OSflags:
	__LINUX__ (linux OS)
	_WIN32 (win32 OS)

 * common return values (for ie. GSinit):
	 0 - success
	-1 - error

 * reserved keys:
	F1 to F10 are reserved for the emulator

 * plugins should NOT change the current
   working directory.
   (on win32, add flag OFN_NOCHANGEDIR for
    GetOpenFileName)

*/

#include "Pcsx2Defs.h"

#if defined(GSdefs)   || defined(PADdefs)  || defined(SIOdefs)  || \
    defined(SPU2defs) || defined(CDVDdefs) || defined(DEV9defs) || \
    defined(USBdefs)  || defined(FWdefs)
#define COMMONdefs
#endif

// PS2EgetLibType returns (may be OR'd)
#define PS2E_LT_GS   0x01
#define PS2E_LT_PAD  0x02		// -=[ OBSOLETE ]=-
#define PS2E_LT_SPU2 0x04
#define PS2E_LT_CDVD 0x08
#define PS2E_LT_DEV9 0x10
#define PS2E_LT_USB  0x20
#define PS2E_LT_FW   0x40
#define PS2E_LT_SIO  0x80

// PS2EgetLibVersion2 (high 16 bits)
#define PS2E_GS_VERSION   0x0006
#define PS2E_PAD_VERSION  0x0002	// -=[ OBSOLETE ]=-
#define PS2E_SPU2_VERSION 0x0005
#define PS2E_CDVD_VERSION 0x0005
#define PS2E_DEV9_VERSION 0x0003
#define PS2E_USB_VERSION  0x0003
#define PS2E_FW_VERSION   0x0002
#define PS2E_SIO_VERSION  0x0001
#ifdef COMMONdefs

#ifdef __cplusplus
extern "C" {
#endif

u32   CALLBACK PS2EgetLibType(void);
u32   CALLBACK PS2EgetLibVersion2(u32 type);
char* CALLBACK PS2EgetLibName(void);

#ifdef __cplusplus
}
#endif

#endif

// key values:
/* key values must be OS dependant:
	win32: the VK_XXX will be used (WinUser)
	linux: the XK_XXX will be used (XFree86)
*/

// for 64bit compilers
typedef char __keyEvent_Size__[(sizeof(keyEvent) == 8)?1:-1];

// plugin types
#define SIO_TYPE_PAD	0x00000001
#define SIO_TYPE_MTAP	0x00000004
#define SIO_TYPE_RM	0x00000040
#define SIO_TYPE_MC	0x00000100

typedef int (CALLBACK * SIOchangeSlotCB)(int slot);

typedef struct _cdvdSubQ {
	u8 ctrl:4;		// control and mode bits
	u8 mode:4;		// control and mode bits
	u8 trackNum;	// current track number (1 to 99)
	u8 trackIndex;	// current index within track (0 to 99)
	u8 trackM;		// current minute location on the disc (BCD encoded)
	u8 trackS;		// current sector location on the disc (BCD encoded)
	u8 trackF;		// current frame location on the disc (BCD encoded)
	u8 pad;			// unused
	u8 discM;		// current minute offset from first track (BCD encoded)
	u8 discS;		// current sector offset from first track (BCD encoded)
	u8 discF;		// current frame offset from first track (BCD encoded)
} cdvdSubQ;

typedef struct _cdvdTD { // NOT bcd coded
	u32 lsn;
	u8 type;
} cdvdTD;

typedef struct _cdvdTN {
	u8 strack;	//number of the first track (usually 1)
	u8 etrack;	//number of the last track
} cdvdTN;

// CDVDreadTrack mode values:
#define CDVD_MODE_2352	0	// full 2352 bytes
#define CDVD_MODE_2340	1	// skip sync (12) bytes
#define CDVD_MODE_2328	2	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2048	3	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2368	4	// full 2352 bytes + 16 subq

// CDVDgetDiskType returns:
#define CDVD_TYPE_ILLEGAL	0xff	// Illegal Disc
#define CDVD_TYPE_DVDV		0xfe	// DVD Video
#define CDVD_TYPE_CDDA		0xfd	// Audio CD
#define CDVD_TYPE_PS2DVD	0x14	// PS2 DVD
#define CDVD_TYPE_PS2CDDA	0x13	// PS2 CD (with audio)
#define CDVD_TYPE_PS2CD		0x12	// PS2 CD
#define CDVD_TYPE_PSCDDA 	0x11	// PS CD (with audio)
#define CDVD_TYPE_PSCD		0x10	// PS CD
#define CDVD_TYPE_UNKNOWN	0x05	// Unknown
#define CDVD_TYPE_DETCTDVDD	0x04	// Detecting Dvd Dual Sided
#define CDVD_TYPE_DETCTDVDS	0x03	// Detecting Dvd Single Sided
#define CDVD_TYPE_DETCTCD	0x02	// Detecting Cd
#define CDVD_TYPE_DETCT		0x01	// Detecting
#define CDVD_TYPE_NODISC 	0x00	// No Disc

// CDVDgetTrayStatus returns:
#define CDVD_TRAY_CLOSE		0x00
#define CDVD_TRAY_OPEN		0x01

// cdvdTD.type (track types for cds)
#define CDVD_AUDIO_TRACK	0x01
#define CDVD_MODE1_TRACK	0x41
#define CDVD_MODE2_TRACK	0x61

#define CDVD_AUDIO_MASK		0x00
#define CDVD_DATA_MASK		0x40
//	CDROM_DATA_TRACK	0x04	//do not enable this! (from linux kernel)

typedef void (*DEV9callback)(int cycles);
typedef int  (*DEV9handler)(void);

typedef void (*USBcallback)(int cycles);
typedef int  (*USBhandler)(void);

// freeze modes:
#define FREEZE_LOAD			0
#define FREEZE_SAVE			1
#define FREEZE_SIZE			2

typedef struct _GSdriverInfo {
	char name[8];
	void *common;
} GSdriverInfo;

#ifdef _WINDOWS_
typedef struct _winInfo { // unsupported values must be set to zero
	HWND hWnd;
	HMENU hMenu;
	HWND hStatusWnd;
} winInfo;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* GS plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef GSdefs

// basic funcs

s32  CALLBACK GSinit();
s32  CALLBACK GSopen(void *pDsp, char *Title, int multithread);
void CALLBACK GSclose();
void CALLBACK GSshutdown();
void CALLBACK GSsetSettingsDir( const char* dir );

void CALLBACK GSvsync(int field);
void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr);
void CALLBACK GSgifTransfer2(u32 *pMem, u32 size);
void CALLBACK GSgifTransfer3(u32 *pMem, u32 size);
void CALLBACK GSgetLastTag(u64* ptag); // returns the last tag processed (64 bits)
void CALLBACK GSgifSoftReset(u32 mask);
void CALLBACK GSreadFIFO(u64 *mem);
void CALLBACK GSreadFIFO2(u64 *mem, int qwc);

// extended funcs

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
void CALLBACK GSkeyEvent(keyEvent *ev);
void CALLBACK GSchangeSaveState(int, const char* filename);
void CALLBACK GSmakeSnapshot(char *path);
void CALLBACK GSmakeSnapshot2(char *pathname, int* snapdone, int savejpg);
void CALLBACK GSirqCallback(void (*callback)());
void CALLBACK GSprintf(int timeout, char *fmt, ...);
void CALLBACK GSsetBaseMem(void*);
void CALLBACK GSsetGameCRC(int crc, int gameoptions);

// controls frame skipping in the GS, if this routine isn't present, frame skipping won't be done
void CALLBACK GSsetFrameSkip(int frameskip);

// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
int CALLBACK GSsetupRecording(int start, void* pData);

void CALLBACK GSreset();
void CALLBACK GSwriteCSR(u32 value);
void CALLBACK GSgetDriverInfo(GSdriverInfo *info);
#ifdef _WIN32
s32  CALLBACK GSsetWindowInfo(winInfo *info);
#endif
s32  CALLBACK GSfreeze(int mode, freezeData *data);
void CALLBACK GSconfigure();
void CALLBACK GSabout();
s32  CALLBACK GStest();

#endif

/* PAD plugin API -=[ OBSOLETE ]=- */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef PADdefs

// basic funcs

s32  CALLBACK PADinit(u32 flags);
s32  CALLBACK PADopen(void *pDsp);
void CALLBACK PADclose();
void CALLBACK PADshutdown();
void CALLBACK PADsetSettingsDir( const char* dir );

// PADkeyEvent is called every vsync (return NULL if no event)
keyEvent* CALLBACK PADkeyEvent();
u8   CALLBACK PADstartPoll(int pad);
u8   CALLBACK PADpoll(u8 value);
// returns: 1 if supported pad1
//			2 if supported pad2
//			3 if both are supported
u32  CALLBACK PADquery();

// call to give a hint to the PAD plugin to query for the keyboard state. A
// good plugin will query the OS for keyboard state ONLY in this function.
// This function is necessary when multithreading because otherwise
// the PAD plugin can get into deadlocks with the thread that really owns
// the window (and input). Note that PADupdate can be called from a different
// thread than the other functions, so mutex or other multithreading primitives
// have to be added to maintain data integrity.
void CALLBACK PADupdate(int pad);

// extended funcs

void CALLBACK PADgsDriverInfo(GSdriverInfo *info);
void CALLBACK PADconfigure();
void CALLBACK PADabout();
s32  CALLBACK PADtest();

#endif

/* SPU2 plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef SPU2defs

// basic funcs

s32  CALLBACK SPU2init();
s32  CALLBACK SPU2open(void *pDsp);
void CALLBACK SPU2close();
void CALLBACK SPU2shutdown();
void CALLBACK SPU2setSettingsDir( const char* dir );

void CALLBACK SPU2write(u32 mem, u16 value);
u16  CALLBACK SPU2read(u32 mem);
void CALLBACK SPU2readDMA4Mem(u16 *pMem, int size);
void CALLBACK SPU2writeDMA4Mem(u16 *pMem, int size);
void CALLBACK SPU2interruptDMA4();
void CALLBACK SPU2readDMA7Mem(u16* pMem, int size);
void CALLBACK SPU2writeDMA7Mem(u16 *pMem, int size);

// all addresses passed by dma will be pointers to the array starting at baseaddr
// This function is necessary to successfully save and reload the spu2 state
void CALLBACK SPU2setDMABaseAddr(uptr baseaddr);

void CALLBACK SPU2interruptDMA7();
u32 CALLBACK SPU2ReadMemAddr(int core);
void CALLBACK SPU2WriteMemAddr(int core,u32 value);
void CALLBACK SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());

// extended funcs
// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
int CALLBACK SPU2setupRecording(int start, void* pData);

void CALLBACK SPU2setClockPtr(u32* ptr);
void CALLBACK SPU2setTimeStretcher(short int enable);

void CALLBACK SPU2async(u32 cycles);
s32  CALLBACK SPU2freeze(int mode, freezeData *data);
void CALLBACK SPU2configure();
void CALLBACK SPU2about();
s32  CALLBACK SPU2test();

#endif

/* CDVD plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef CDVDdefs

// basic funcs

s32  CALLBACK CDVDinit();
s32  CALLBACK CDVDopen(const char* pTitleFilename);
void CALLBACK CDVDclose();
void CALLBACK CDVDshutdown();
void CALLBACK CDVDsetSettingsDir( const char* dir );

s32  CALLBACK CDVDreadTrack(u32 lsn, int mode);

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer();

s32  CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq);//read subq from disc (only cds have subq data)
s32  CALLBACK CDVDgetTN(cdvdTN *Buffer);			//disk information
s32  CALLBACK CDVDgetTD(u8 Track, cdvdTD *Buffer);	//track info: min,sec,frame,type
s32  CALLBACK CDVDgetTOC(void* toc);				//gets ps2 style toc from disc
s32  CALLBACK CDVDgetDiskType();					//CDVD_TYPE_xxxx
s32  CALLBACK CDVDgetTrayStatus();					//CDVD_TRAY_xxxx
s32  CALLBACK CDVDctrlTrayOpen();					//open disc tray
s32  CALLBACK CDVDctrlTrayClose();					//close disc tray

// extended funcs

void CALLBACK CDVDconfigure();
void CALLBACK CDVDabout();
s32  CALLBACK CDVDtest();
void CALLBACK CDVDnewDiskCB(void (*callback)());

// new funcs 

// read a track directly
s32  CALLBACK CDVDreadSector(u8* buffer, s32 lsn, int mode);

// improved getBuffer
s32  CALLBACK CDVDgetBuffer2(u8* buffer);

// tool function
s32  CALLBACK CDVDgetDualInfo(s32* dualType, u32* _layer1start);

#endif

/* DEV9 plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef DEV9defs

// basic funcs

// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
s32  CALLBACK DEV9init();
s32  CALLBACK DEV9open(void *pDsp);
void CALLBACK DEV9close();
void CALLBACK DEV9shutdown();
void CALLBACK DEV9setSettingsDir( const char* dir );

u8   CALLBACK DEV9read8(u32 addr);
u16  CALLBACK DEV9read16(u32 addr);
u32  CALLBACK DEV9read32(u32 addr);
void CALLBACK DEV9write8(u32 addr,  u8 value);
void CALLBACK DEV9write16(u32 addr, u16 value);
void CALLBACK DEV9write32(u32 addr, u32 value);
void CALLBACK DEV9readDMA8Mem(u32 *pMem, int size);
void CALLBACK DEV9writeDMA8Mem(u32 *pMem, int size);
// cycles = IOP cycles before calling callback,
// if callback returns 1 the irq is triggered, else not
void CALLBACK DEV9irqCallback(DEV9callback callback);
DEV9handler CALLBACK DEV9irqHandler(void);

// extended funcs

s32  CALLBACK DEV9freeze(int mode, freezeData *data);
void CALLBACK DEV9configure();
void CALLBACK DEV9about();
s32  CALLBACK DEV9test();

#endif

/* USB plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef USBdefs

// basic funcs

s32  CALLBACK USBinit();
s32  CALLBACK USBopen(void *pDsp);
void CALLBACK USBclose();
void CALLBACK USBshutdown();
void CALLBACK USBsetSettingsDir( const char* dir );

u8   CALLBACK USBread8(u32 addr);
u16  CALLBACK USBread16(u32 addr);
u32  CALLBACK USBread32(u32 addr);
void CALLBACK USBwrite8(u32 addr,  u8 value);
void CALLBACK USBwrite16(u32 addr, u16 value);
void CALLBACK USBwrite32(u32 addr, u32 value);
void CALLBACK USBasync(u32 cycles);

// cycles = IOP cycles before calling callback,
// if callback returns 1 the irq is triggered, else not
void CALLBACK USBirqCallback(USBcallback callback);
USBhandler CALLBACK USBirqHandler(void);
void CALLBACK USBsetRAM(void *mem);

// extended funcs

s32  CALLBACK USBfreeze(int mode, freezeData *data);
void CALLBACK USBconfigure();
void CALLBACK USBabout();
s32  CALLBACK USBtest();

#endif

/* FW plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef FWdefs
// basic funcs

// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
s32  CALLBACK FWinit();
s32  CALLBACK FWopen(void *pDsp);
void CALLBACK FWclose();
void CALLBACK FWshutdown();
void CALLBACK FWsetSettingsDir( const char* dir );

u32  CALLBACK FWread32(u32 addr);
void CALLBACK FWwrite32(u32 addr, u32 value);
void CALLBACK FWirqCallback(void (*callback)());

// extended funcs

s32  CALLBACK FWfreeze(int mode, freezeData *data);
void CALLBACK FWconfigure();
void CALLBACK FWabout();
s32  CALLBACK FWtest();
#endif

// might be useful for emulators
#ifdef PLUGINtypedefs

typedef u32  (CALLBACK* _PS2EgetLibType)(void);
typedef u32  (CALLBACK* _PS2EgetLibVersion2)(u32 type);
typedef char*(CALLBACK* _PS2EgetLibName)(void);

// GS
// NOTE: GSreadFIFOX/GSwriteCSR functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _GSopen)(void *pDsp, char *Title, int multithread);
typedef s32  (CALLBACK* _GSopen2)( void *pDsp, u32 flags );
typedef void (CALLBACK* _GSvsync)(int field);
typedef void (CALLBACK* _GSgifTransfer1)(u32 *pMem, u32 addr);
typedef void (CALLBACK* _GSgifTransfer2)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgifTransfer3)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgifSoftReset)(u32 mask);
typedef void (CALLBACK* _GSreadFIFO)(u64 *pMem);
typedef void (CALLBACK* _GSreadFIFO2)(u64 *pMem, int qwc);

typedef void (CALLBACK* _GSchangeSaveState)(int, const char* filename);
typedef void (CALLBACK* _GSirqCallback)(void (*callback)());
typedef void (CALLBACK* _GSprintf)(int timeout, char *fmt, ...);
typedef void (CALLBACK* _GSsetBaseMem)(void*);
typedef void (CALLBACK* _GSsetGameCRC)(int, int);
typedef void (CALLBACK* _GSsetFrameSkip)(int frameskip);
typedef void (CALLBACK* _GSsetFrameLimit)(int limit);
typedef void (CALLBACK* _GSsetVsync)(int enabled);
typedef void (CALLBACK* _GSsetExclusive)(int isExclusive);
typedef int (CALLBACK* _GSsetupRecording)(int, void*);
typedef void (CALLBACK* _GSreset)();
typedef void (CALLBACK* _GSwriteCSR)(u32 value);
typedef void (CALLBACK* _GSmakeSnapshot)(const char *path);
typedef void (CALLBACK* _GSmakeSnapshot2)(const char *path, int*, int);

// Worthless crap function that returns GS plugin specific data via some
// undocumented void* to a struct.   If any pad plugin actually relies on
// this info, it deserves to fail new newer pcsx2s.  -- air
//typedef void (CALLBACK* _GSgetDriverInfo)(GSdriverInfo *info);

// PAD
typedef s32  (CALLBACK* _PADinit)(u32 flags);
typedef s32  (CALLBACK* _PADopen)(void *pDsp);
typedef u8   (CALLBACK* _PADstartPoll)(int pad);
typedef u8   (CALLBACK* _PADpoll)(u8 value);
typedef u32  (CALLBACK* _PADquery)(int pad);
typedef void (CALLBACK* _PADupdate)(int pad);
typedef keyEvent* (CALLBACK* _PADkeyEvent)();
typedef void (CALLBACK* _PADgsDriverInfo)(GSdriverInfo *info);
typedef s32  (CALLBACK* _PADsetSlot)(u8 port, u8 slot);
typedef s32  (CALLBACK* _PADqueryMtap)(u8 port);

// SPU2
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _SPU2open)(void *pDsp);
typedef void (CALLBACK* _SPU2write)(u32 mem, u16 value);
typedef u16  (CALLBACK* _SPU2read)(u32 mem);
typedef void (CALLBACK* _SPU2readDMA4Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2writeDMA4Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2interruptDMA4)();
typedef void (CALLBACK* _SPU2readDMA7Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2writeDMA7Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2setDMABaseAddr)(uptr baseaddr);
typedef void (CALLBACK* _SPU2interruptDMA7)();
typedef void (CALLBACK* _SPU2irqCallback)(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());
typedef int (CALLBACK* _SPU2setupRecording)(int, void*);

typedef void (CALLBACK* _SPU2setClockPtr)(u32*ptr);
typedef void (CALLBACK* _SPU2setTimeStretcher)(short int enable);

typedef u32 (CALLBACK* _SPU2ReadMemAddr)(int core);
typedef void (CALLBACK* _SPU2WriteMemAddr)(int core,u32 value);
typedef void (CALLBACK* _SPU2async)(u32 cycles);


// CDVD
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _CDVDopen)(const char* pTitleFilename);

// Initiates an asynchronous track read operation.
// Returns -1 on error (invalid track)
// Returns 0 on success.
typedef s32  (CALLBACK* _CDVDreadTrack)(u32 lsn, int mode);

// *OBSOLETE* returns a pointer to the buffer, or NULL if data hasn't finished
// loading yet.
typedef u8*  (CALLBACK* _CDVDgetBuffer)();

// Copies loaded data to the target buffer.
// Returns -2 if the asynchronous read is still pending.
// Returns -1 if the asyncronous read failed.
// Returns 0 on success.
typedef s32  (CALLBACK* _CDVDgetBuffer2)(u8* buffer);

typedef s32  (CALLBACK* _CDVDreadSubQ)(u32 lsn, cdvdSubQ* subq);
typedef s32  (CALLBACK* _CDVDgetTN)(cdvdTN *Buffer);
typedef s32  (CALLBACK* _CDVDgetTD)(u8 Track, cdvdTD *Buffer);
typedef s32  (CALLBACK* _CDVDgetTOC)(void* toc);
typedef s32  (CALLBACK* _CDVDgetDiskType)();
typedef s32  (CALLBACK* _CDVDgetTrayStatus)();
typedef s32  (CALLBACK* _CDVDctrlTrayOpen)();
typedef s32  (CALLBACK* _CDVDctrlTrayClose)();
typedef s32  (CALLBACK* _CDVDreadSector)(u8* buffer, u32 lsn, int mode);
typedef s32  (CALLBACK* _CDVDgetDualInfo)(s32* dualType, u32* _layer1start);

typedef void (CALLBACK* _CDVDnewDiskCB)(void (*callback)());


// DEV9
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _DEV9open)(void *pDsp);
typedef u8   (CALLBACK* _DEV9read8)(u32 mem);
typedef u16  (CALLBACK* _DEV9read16)(u32 mem);
typedef u32  (CALLBACK* _DEV9read32)(u32 mem);
typedef void (CALLBACK* _DEV9write8)(u32 mem, u8 value);
typedef void (CALLBACK* _DEV9write16)(u32 mem, u16 value);
typedef void (CALLBACK* _DEV9write32)(u32 mem, u32 value);
typedef void (CALLBACK* _DEV9readDMA8Mem)(u32 *pMem, int size);
typedef void (CALLBACK* _DEV9writeDMA8Mem)(u32 *pMem, int size);
typedef void (CALLBACK* _DEV9irqCallback)(DEV9callback callback);
typedef DEV9handler (CALLBACK* _DEV9irqHandler)(void);

// USB
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _USBopen)(void *pDsp);
typedef u8   (CALLBACK* _USBread8)(u32 mem);
typedef u16  (CALLBACK* _USBread16)(u32 mem);
typedef u32  (CALLBACK* _USBread32)(u32 mem);
typedef void (CALLBACK* _USBwrite8)(u32 mem, u8 value);
typedef void (CALLBACK* _USBwrite16)(u32 mem, u16 value);
typedef void (CALLBACK* _USBwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _USBasync)(u32 cycles);

typedef void (CALLBACK* _USBirqCallback)(USBcallback callback);
typedef USBhandler (CALLBACK* _USBirqHandler)(void);
typedef void (CALLBACK* _USBsetRAM)(void *mem);

//FW
typedef s32  (CALLBACK* _FWopen)(void *pDsp);
typedef u32  (CALLBACK* _FWread32)(u32 mem);
typedef void (CALLBACK* _FWwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _FWirqCallback)(void (*callback)());
#endif

#ifdef PLUGINfuncs

// GS
extern _GSopen            GSopen;
extern _GSopen2           GSopen2;
extern _GSvsync           GSvsync;
extern _GSgifTransfer1    GSgifTransfer1;
extern _GSgifTransfer2    GSgifTransfer2;
extern _GSgifTransfer3    GSgifTransfer3;
extern _GSgifSoftReset    GSgifSoftReset;
extern _GSreadFIFO        GSreadFIFO;
extern _GSreadFIFO2       GSreadFIFO2;

extern _GSchangeSaveState GSchangeSaveState;
extern _GSmakeSnapshot	  GSmakeSnapshot;
extern _GSmakeSnapshot2   GSmakeSnapshot2;
extern _GSirqCallback 	  GSirqCallback;
extern _GSprintf      	  GSprintf;
extern _GSsetBaseMem 	  GSsetBaseMem;
extern _GSsetGameCRC	  GSsetGameCRC;
extern _GSsetFrameSkip	  GSsetFrameSkip;
extern _GSsetFrameLimit	  GSsetFrameLimit;
extern _GSsetVsync	      GSsetVsync;
extern _GSsetupRecording  GSsetupRecording;
extern _GSreset			  GSreset;
extern _GSwriteCSR		  GSwriteCSR;

// PAD
extern _PADopen           PADopen;
extern _PADstartPoll      PADstartPoll;
extern _PADpoll           PADpoll;
extern _PADquery          PADquery;
extern _PADupdate         PADupdate;
extern _PADkeyEvent       PADkeyEvent;
extern _PADgsDriverInfo   PADgsDriverInfo;
extern _PADsetSlot        PADsetSlot;
extern _PADqueryMtap      PADqueryMtap;

// SPU2
extern _SPU2open          SPU2open;
extern _SPU2write         SPU2write;
extern _SPU2read          SPU2read;
extern _SPU2readDMA4Mem   SPU2readDMA4Mem;
extern _SPU2writeDMA4Mem  SPU2writeDMA4Mem;
extern _SPU2interruptDMA4 SPU2interruptDMA4;
extern _SPU2readDMA7Mem   SPU2readDMA7Mem;
extern _SPU2writeDMA7Mem  SPU2writeDMA7Mem;
extern _SPU2setDMABaseAddr SPU2setDMABaseAddr;
extern _SPU2interruptDMA7 SPU2interruptDMA7;
extern _SPU2ReadMemAddr   SPU2ReadMemAddr;
extern _SPU2setupRecording SPU2setupRecording;
extern _SPU2WriteMemAddr   SPU2WriteMemAddr;
extern _SPU2irqCallback   SPU2irqCallback;

extern _SPU2setClockPtr   SPU2setClockPtr;
extern _SPU2setTimeStretcher SPU2setTimeStretcher;

extern _SPU2async         SPU2async;

// DEV9
extern _DEV9open          DEV9open;
extern _DEV9read8         DEV9read8;
extern _DEV9read16        DEV9read16;
extern _DEV9read32        DEV9read32;
extern _DEV9write8        DEV9write8;
extern _DEV9write16       DEV9write16;
extern _DEV9write32       DEV9write32;
extern _DEV9readDMA8Mem   DEV9readDMA8Mem;
extern _DEV9writeDMA8Mem  DEV9writeDMA8Mem;
extern _DEV9irqCallback   DEV9irqCallback;
extern _DEV9irqHandler    DEV9irqHandler;

// USB
extern _USBopen           USBopen;
extern _USBread8          USBread8;
extern _USBread16         USBread16;
extern _USBread32         USBread32;
extern _USBwrite8         USBwrite8;
extern _USBwrite16        USBwrite16;
extern _USBwrite32        USBwrite32;
extern _USBasync          USBasync;

extern _USBirqCallback    USBirqCallback;
extern _USBirqHandler     USBirqHandler;
extern _USBsetRAM         USBsetRAM;

// FW
extern _FWopen            FWopen;
extern _FWread32          FWread32;
extern _FWwrite32         FWwrite32;
extern _FWirqCallback     FWirqCallback;

#endif

#ifdef __cplusplus
}	// End extern "C"
#endif

#endif /* __PS2EDEFS_H__ */
