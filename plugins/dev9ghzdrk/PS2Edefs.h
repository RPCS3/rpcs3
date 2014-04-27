/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
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
 *  PS2E Definitions v0.5.5 (beta)
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
	__WIN32__ (win32 OS)

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

#include "PS2Etypes.h"

#ifdef __LINUX__
#define CALLBACK
#else
#include <windows.h>
#endif

/* common defines */

#if defined(GSdefs)  || defined(PADdefs) || \
	defined(SPU2defs)|| defined(CDVDdefs)
#define COMMONdefs
#endif

// PS2EgetLibType returns (may be OR'd)
#define PS2E_LT_GS   0x01
#define PS2E_LT_PAD  0x02
#define PS2E_LT_SPU2 0x04
#define PS2E_LT_CDVD 0x08
#define PS2E_LT_DEV9 0x10
#define PS2E_LT_USB  0x20
#define PS2E_LT_FIREWIRE 0x40

// PS2EgetLibVersion2 (high 16 bits)
#define PS2E_GS_VERSION   0x0005
#define PS2E_PAD_VERSION  0x0002
#define PS2E_SPU2_VERSION 0x0004
#define PS2E_CDVD_VERSION 0x0003
#define PS2E_DEV9_VERSION 0x0003
#define PS2E_USB_VERSION  0x0003
#define PS2E_FIREWIRE_VERSION 0x0002
#ifdef COMMONdefs

u32   CALLBACK PS2EgetLibType(void);
u32   CALLBACK PS2EgetLibVersion2(u32 type);
char* CALLBACK PS2EgetLibName(void);

#endif

// key values:
/* key values must be OS dependant:
	win32: the VK_XXX will be used (WinUser)
	linux: the XK_XXX will be used (XFree86)
*/

// event values:
#define KEYPRESS	1
#define KEYRELEASE	2

typedef struct {
	u32 key;
	u32 event;
} keyEvent;

typedef struct { // NOT bcd coded
	u8 minute;
	u8 second;
	u8 frame;
	u8 type;
} cdvdTD;

typedef struct {
	u8 strack;	//number of the first track (usualy 1)
	u8 etrack;	//number of the last track
} cdvdTN;

// CDVDreadTrack mode values:
#define CDVD_MODE_2352	0	// full 2352 bytes
#define CDVD_MODE_2340	1	// skip sync (12) bytes
#define CDVD_MODE_2328	2	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2048	3	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2368	4	// full 2352 bytes + 16 subq

// CDVDgetType returns:
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

// cdvdLoc:track type
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

typedef struct {
	int size;
	s8 *data;
} freezeData;

typedef struct {
	char name[8];
	void *common;
} GSdriverInfo;

#ifdef __WIN32__
typedef struct { // unsupported values must be set to zero
	HWND hWnd;
	HMENU hMenu;
	HWND hStatusWnd;
} winInfo;
#endif

/* GS plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef GSdefs

// basic funcs

s32  CALLBACK GSinit();
s32  CALLBACK GSopen(void *pDsp, char *Title);
void CALLBACK GSclose();
void CALLBACK GSshutdown();
void CALLBACK GSvsync();
void CALLBACK GSgifTransfer1(u32 *pMem);
void CALLBACK GSgifTransfer2(u32 *pMem, u32 size);
void CALLBACK GSgifTransfer3(u32 *pMem, u32 size);
void CALLBACK GSwrite32(u32 mem, u32 value);
void CALLBACK GSwrite64(u32 mem, u64 value);
u32  CALLBACK GSread32(u32 mem);
u64  CALLBACK GSread64(u32 mem);
void CALLBACK GSreadFIFO(u64 *mem);

// extended funcs

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
void CALLBACK GSkeyEvent(keyEvent *ev);
void CALLBACK GSmakeSnapshot(char *path);
void CALLBACK GSirqCallback(void (*callback)());
void CALLBACK GSprintf(int timeout, char *fmt, ...);
void CALLBACK GSgetDriverInfo(GSdriverInfo *info);
#ifdef __WIN32__
s32  CALLBACK GSsetWindowInfo(winInfo *info);
#endif
s32  CALLBACK GSfreeze(int mode, freezeData *data);
void CALLBACK GSconfigure();
void CALLBACK GSabout();
s32  CALLBACK GStest();

#endif

/* PAD plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef PADdefs

// basic funcs

s32  CALLBACK PADinit(u32 flags);
s32  CALLBACK PADopen(void *pDsp);
void CALLBACK PADclose();
void CALLBACK PADshutdown();
// PADkeyEvent is called every vsync (return NULL if no event)
keyEvent* CALLBACK PADkeyEvent();
u8   CALLBACK PADstartPoll(int pad);
u8   CALLBACK PADpoll(u8 value);
// returns: 1 if supported pad1
//			2 if supported pad2
//			3 if both are supported
u32  CALLBACK PADquery();

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
void CALLBACK SPU2write(u32 mem, u16 value);
u16  CALLBACK SPU2read(u32 mem);
void CALLBACK SPU2readDMA4Mem(u16 *pMem, int size);
void CALLBACK SPU2writeDMA4Mem(u16 *pMem, int size);
void CALLBACK SPU2interruptDMA4();
void CALLBACK SPU2readDMA7Mem(u16* pMem, int size);
void CALLBACK SPU2writeDMA7Mem(u16 *pMem, int size);
void CALLBACK SPU2interruptDMA7();
void CALLBACK SPU2irqCallback(void (*callback)());

// extended funcs

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
s32  CALLBACK CDVDopen();
void CALLBACK CDVDclose();
void CALLBACK CDVDshutdown();
s32  CALLBACK CDVDreadTrack(u32 lsn, int mode);

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer();

s32  CALLBACK CDVDgetTN(cdvdTN *Buffer);		//disk information
s32  CALLBACK CDVDgetTD(u8 Track, cdvdTD *Buffer);	//track info: min,sec,frame,type
s32  CALLBACK CDVDgetType();				//CDVD_TYPE_xxxx
s32  CALLBACK CDVDgetTrayStatus();			//CDVD_TRAY_xxxx

// extended funcs

void CALLBACK CDVDconfigure();
void CALLBACK CDVDabout();
s32  CALLBACK CDVDtest();

#endif

/* DEV9 plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef DEV9defs

// basic funcs

s32  CALLBACK DEV9init();
s32  CALLBACK DEV9open(void *pDsp);
void CALLBACK DEV9close();
void CALLBACK DEV9shutdown();
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
u8   CALLBACK USBread8(u32 addr);
u16  CALLBACK USBread16(u32 addr);
u32  CALLBACK USBread32(u32 addr);
void CALLBACK USBwrite8(u32 addr,  u8 value);
void CALLBACK USBwrite16(u32 addr, u16 value);
void CALLBACK USBwrite32(u32 addr, u32 value);
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
/* Firewire plugin API */

// if this file is included with this define
// the next api will not be skipped by the compiler
#ifdef FIREWIREdefs
// basic funcs

s32  CALLBACK FireWireinit();
s32  CALLBACK FireWireopen(void *pDsp);
void CALLBACK FireWireclose();
void CALLBACK FireWireshutdown();
u32  CALLBACK FireWireread32(u32 addr);
void CALLBACK FireWirewrite32(u32 addr, u32 value);
void CALLBACK FireWireirqCallback(void (*callback)());

// extended funcs

s32  CALLBACK FireWirefreeze(int mode, freezeData *data);
void CALLBACK FireWireconfigure();
void CALLBACK FireWireabout();
s32  CALLBACK FireWiretest();
#endif

// might be useful for emulators
#ifdef PLUGINtypedefs

typedef u32  (CALLBACK* _PS2EgetLibType)(void);
typedef u32  (CALLBACK* _PS2EgetLibVersion2)(u32 type);
typedef char*(CALLBACK* _PS2EgetLibName)(void);

// GS
typedef s32  (CALLBACK* _GSinit)();
typedef s32  (CALLBACK* _GSopen)(void *pDsp, char *Title);
typedef void (CALLBACK* _GSclose)();
typedef void (CALLBACK* _GSshutdown)();
typedef void (CALLBACK* _GSvsync)();
typedef void (CALLBACK* _GSwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _GSwrite64)(u32 mem, u64 value);
typedef u32  (CALLBACK* _GSread32)(u32 mem);
typedef u64  (CALLBACK* _GSread64)(u32 mem);
typedef void (CALLBACK* _GSgifTransfer1)(u32 *pMem);
typedef void (CALLBACK* _GSgifTransfer2)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgifTransfer3)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSreadFIFO)(u64 *pMem);

typedef void (CALLBACK* _GSkeyEvent)(keyEvent* ev);
typedef void (CALLBACK* _GSirqCallback)(void (*callback)());
typedef void (CALLBACK* _GSprintf)(int timeout, char *fmt, ...);
typedef void (CALLBACK* _GSgetDriverInfo)(GSdriverInfo *info);
#ifdef __WIN32__
typedef s32  (CALLBACK* _GSsetWindowInfo)(winInfo *info);
#endif
typedef void (CALLBACK* _GSmakeSnapshot)(char *path);
typedef s32  (CALLBACK* _GSfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _GSconfigure)();
typedef s32  (CALLBACK* _GStest)();
typedef void (CALLBACK* _GSabout)();

// PAD
typedef s32  (CALLBACK* _PADinit)(u32 flags);
typedef s32  (CALLBACK* _PADopen)(void *pDsp);
typedef void (CALLBACK* _PADclose)();
typedef void (CALLBACK* _PADshutdown)();
typedef keyEvent* (CALLBACK* _PADkeyEvent)();
typedef u8   (CALLBACK* _PADstartPoll)(int pad);
typedef u8   (CALLBACK* _PADpoll)(u8 value);
typedef u32  (CALLBACK* _PADquery)();

typedef void (CALLBACK* _PADgsDriverInfo)(GSdriverInfo *info);
typedef void (CALLBACK* _PADconfigure)();
typedef s32  (CALLBACK* _PADtest)();
typedef void (CALLBACK* _PADabout)();

// SPU2
typedef s32  (CALLBACK* _SPU2init)();
typedef s32  (CALLBACK* _SPU2open)(void *pDsp);
typedef void (CALLBACK* _SPU2close)();
typedef void (CALLBACK* _SPU2shutdown)();
typedef void (CALLBACK* _SPU2write)(u32 mem, u16 value);
typedef u16  (CALLBACK* _SPU2read)(u32 mem);
typedef void (CALLBACK* _SPU2readDMA4Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2writeDMA4Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2interruptDMA4)();
typedef void (CALLBACK* _SPU2readDMA7Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2writeDMA7Mem)(u16 *pMem, int size);
typedef void (CALLBACK* _SPU2interruptDMA7)();
typedef void (CALLBACK* _SPU2irqCallback)(void (*callback)());

typedef void (CALLBACK* _SPU2async)(u32 cycles);
typedef s32  (CALLBACK* _SPU2freeze)(int mode, freezeData *data);
typedef void (CALLBACK* _SPU2configure)();
typedef s32  (CALLBACK* _SPU2test)();
typedef void (CALLBACK* _SPU2about)();

// CDVD
typedef s32  (CALLBACK* _CDVDinit)();
typedef s32  (CALLBACK* _CDVDopen)();
typedef void (CALLBACK* _CDVDclose)();
typedef void (CALLBACK* _CDVDshutdown)();
typedef s32  (CALLBACK* _CDVDreadTrack)(u32 lsn, int mode);
typedef u8*  (CALLBACK* _CDVDgetBuffer)();
typedef s32  (CALLBACK* _CDVDgetTN)(cdvdTN *Buffer);
typedef s32  (CALLBACK* _CDVDgetTD)(u8 Track, cdvdTD *Buffer);
typedef s32  (CALLBACK* _CDVDgetType)();
typedef s32  (CALLBACK* _CDVDgetTrayStatus)();

typedef void (CALLBACK* _CDVDconfigure)();
typedef s32  (CALLBACK* _CDVDtest)();
typedef void (CALLBACK* _CDVDabout)();

// DEV9
typedef s32  (CALLBACK* _DEV9init)();
typedef s32  (CALLBACK* _DEV9open)(void *pDsp);
typedef void (CALLBACK* _DEV9close)();
typedef void (CALLBACK* _DEV9shutdown)();
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

typedef s32  (CALLBACK* _DEV9freeze)(int mode, freezeData *data);
typedef void (CALLBACK* _DEV9configure)();
typedef s32  (CALLBACK* _DEV9test)();
typedef void (CALLBACK* _DEV9about)();

// USB
typedef s32  (CALLBACK* _USBinit)();
typedef s32  (CALLBACK* _USBopen)(void *pDsp);
typedef void (CALLBACK* _USBclose)();
typedef void (CALLBACK* _USBshutdown)();
typedef u8   (CALLBACK* _USBread8)(u32 mem);
typedef u16  (CALLBACK* _USBread16)(u32 mem);
typedef u32  (CALLBACK* _USBread32)(u32 mem);
typedef void (CALLBACK* _USBwrite8)(u32 mem, u8 value);
typedef void (CALLBACK* _USBwrite16)(u32 mem, u16 value);
typedef void (CALLBACK* _USBwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _USBirqCallback)(USBcallback callback);
typedef USBhandler (CALLBACK* _USBirqHandler)(void);
typedef void (CALLBACK* _USBsetRAM)(void *mem);

typedef s32  (CALLBACK* _USBfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _USBconfigure)();
typedef s32  (CALLBACK* _USBtest)();
typedef void (CALLBACK* _USBabout)();

//FireWire
typedef s32  (CALLBACK* _FireWireinit)();
typedef s32  (CALLBACK* _FireWireopen)(void *pDsp);
typedef void (CALLBACK* _FireWireclose)();
typedef void (CALLBACK* _FireWireshutdown)();
typedef u32  (CALLBACK* _FireWireread32)(u32 mem);
typedef void (CALLBACK* _FireWirewrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _FireWireirqCallback)(void (*callback)());

typedef s32  (CALLBACK* _FireWirefreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _FireWireconfigure)();
typedef s32  (CALLBACK* _FireWiretest)();
typedef void (CALLBACK* _FireWireabout)();

#endif

#ifdef PLUGINfuncs

// GS
_GSinit            GSinit;
_GSopen            GSopen;
_GSclose           GSclose;
_GSshutdown        GSshutdown;
_GSvsync           GSvsync;
_GSwrite32         GSwrite32;
_GSwrite64         GSwrite64;
_GSread32          GSread32;
_GSread64          GSread64;
_GSgifTransfer1    GSgifTransfer1;
_GSgifTransfer2    GSgifTransfer2;
_GSgifTransfer3    GSgifTransfer3;
_GSreadFIFO        GSreadFIFO;

_GSkeyEvent        GSkeyEvent;
_GSmakeSnapshot	   GSmakeSnapshot;
_GSirqCallback 	   GSirqCallback;
_GSprintf      	   GSprintf;
_GSgetDriverInfo   GSgetDriverInfo;
#ifdef __WIN32__
_GSsetWindowInfo   GSsetWindowInfo;
#endif
_GSfreeze          GSfreeze;
_GSconfigure       GSconfigure;
_GStest            GStest;
_GSabout           GSabout;

// PAD1
_PADinit           PAD1init;
_PADopen           PAD1open;
_PADclose          PAD1close;
_PADshutdown       PAD1shutdown;
_PADkeyEvent       PAD1keyEvent;
_PADstartPoll      PAD1startPoll;
_PADpoll           PAD1poll;
_PADquery          PAD1query;

_PADgsDriverInfo   PAD1gsDriverInfo;
_PADconfigure      PAD1configure;
_PADtest           PAD1test;
_PADabout          PAD1about;

// PAD2
_PADinit           PAD2init;
_PADopen           PAD2open;
_PADclose          PAD2close;
_PADshutdown       PAD2shutdown;
_PADkeyEvent       PAD2keyEvent;
_PADstartPoll      PAD2startPoll;
_PADpoll           PAD2poll;
_PADquery          PAD2query;

_PADgsDriverInfo   PAD2gsDriverInfo;
_PADconfigure      PAD2configure;
_PADtest           PAD2test;
_PADabout          PAD2about;

// SPU2
_SPU2init          SPU2init;
_SPU2open          SPU2open;
_SPU2close         SPU2close;
_SPU2shutdown      SPU2shutdown;
_SPU2write         SPU2write;
_SPU2read          SPU2read;
_SPU2readDMA4Mem   SPU2readDMA4Mem;
_SPU2writeDMA4Mem  SPU2writeDMA4Mem;
_SPU2interruptDMA4 SPU2interruptDMA4;
_SPU2readDMA7Mem   SPU2readDMA7Mem;
_SPU2writeDMA7Mem  SPU2writeDMA7Mem;
_SPU2interruptDMA7 SPU2interruptDMA7;
_SPU2irqCallback   SPU2irqCallback;

_SPU2async         SPU2async;
_SPU2freeze        SPU2freeze;
_SPU2configure     SPU2configure;
_SPU2test          SPU2test;
_SPU2about         SPU2about;

// CDVD
_CDVDinit          CDVDinit;
_CDVDopen          CDVDopen;
_CDVDclose         CDVDclose;
_CDVDshutdown      CDVDshutdown;
_CDVDreadTrack     CDVDreadTrack;
_CDVDgetBuffer     CDVDgetBuffer;
_CDVDgetTN         CDVDgetTN;
_CDVDgetTD         CDVDgetTD;
_CDVDgetType       CDVDgetType;
_CDVDgetTrayStatus CDVDgetTrayStatus;

_CDVDconfigure     CDVDconfigure;
_CDVDtest          CDVDtest;
_CDVDabout         CDVDabout;

// DEV9
_DEV9init          DEV9init;
_DEV9open          DEV9open;
_DEV9close         DEV9close;
_DEV9shutdown      DEV9shutdown;
_DEV9read8         DEV9read8;
_DEV9read16        DEV9read16;
_DEV9read32        DEV9read32;
_DEV9write8        DEV9write8;
_DEV9write16       DEV9write16;
_DEV9write32       DEV9write32;
_DEV9readDMA8Mem   DEV9readDMA8Mem;
_DEV9writeDMA8Mem  DEV9writeDMA8Mem;
_DEV9irqCallback   DEV9irqCallback;
_DEV9irqHandler    DEV9irqHandler;

_DEV9configure     DEV9configure;
_DEV9freeze        DEV9freeze;
_DEV9test          DEV9test;
_DEV9about         DEV9about;

// USB
_USBinit           USBinit;
_USBopen           USBopen;
_USBclose          USBclose;
_USBshutdown       USBshutdown;
_USBread8          USBread8;
_USBread16         USBread16;
_USBread32         USBread32;
_USBwrite8         USBwrite8;
_USBwrite16        USBwrite16;
_USBwrite32        USBwrite32;
_USBirqCallback    USBirqCallback;
_USBirqHandler     USBirqHandler;
_USBsetRAM         USBsetRAM;

_USBconfigure      USBconfigure;
_USBfreeze         USBfreeze;
_USBtest           USBtest;
_USBabout          USBabout;

// FireWire
_FireWireinit           FireWireinit;
_FireWireopen           FireWireopen;
_FireWireclose          FireWireclose;
_FireWireshutdown       FireWireshutdown;
_FireWireread32         FireWireread32;
_FireWirewrite32        FireWirewrite32;
_FireWireirqCallback    FireWireirqCallback;

_FireWireconfigure      FireWireconfigure;
_FireWirefreeze         FireWirefreeze;
_FireWiretest           FireWiretest;
_FireWireabout          FireWireabout;
#endif

#endif /* __PS2EDEFS_H__ */
