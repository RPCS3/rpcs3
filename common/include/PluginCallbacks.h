/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
 
#ifndef __PLUGINCALLBACKS_H__
#define __PLUGINCALLBACKS_H__

extern "C" 
{	
// General
typedef u32  (CALLBACK* _PS2EgetLibType)(void);
typedef u32  (CALLBACK* _PS2EgetLibVersion2)(u32 type);
typedef char*(CALLBACK* _PS2EgetLibName)(void);
typedef void (CALLBACK* _PS2EpassConfig)(PcsxConfig Config);

// GS
// NOTE: GSreadFIFOX/GSwriteCSR functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _GSinit)(char *configpath);
typedef s32  (CALLBACK* _GSopen)(void *pDisplay, char *Title, bool multithread);
typedef void (CALLBACK* _GSclose)();
typedef void (CALLBACK* _GSshutdown)();
typedef void (CALLBACK* _GSvsync)(int field);
typedef void (CALLBACK* _GSgifTransfer1)(u32 *pMem, u32 addr);
typedef void (CALLBACK* _GSgifTransfer2)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgifTransfer3)(u32 *pMem, u32 size);
typedef void (CALLBACK* _GSgetLastTag)(u64* ptag); // returns the last tag processed (64 bits)
typedef void (CALLBACK* _GSgifSoftReset)(u32 mask);
typedef void (CALLBACK* _GSreadFIFO)(u64 *pMem);
typedef void (CALLBACK* _GSreadFIFO2)(u64 *pMem, int qwc);

typedef void (CALLBACK* _GSkeyEvent)(keyEvent* ev);
typedef void (CALLBACK* _GSchangeSaveState)(s32 state, const char* filename);
typedef void (CALLBACK* _GSirqCallback)(void (*callback)());
typedef void (CALLBACK* _GSprintf)(s32 timeout, char *fmt, ...);
typedef void (CALLBACK* _GSsetBaseMem)(void*);
typedef void (CALLBACK* _GSsetGameCRC)(s32 crc, s32 gameoptions);

typedef void (CALLBACK* _GSsetFrameSkip)(int frameskip);
typedef bool (CALLBACK* _GSsetupRecording)(bool start);
typedef void (CALLBACK* _GSreset)();
typedef void (CALLBACK* _GSwriteCSR)(u32 value);
typedef void (CALLBACK* _GSgetDriverInfo)(GSdriverInfo *info);
#ifdef _WINDOWS_
typedef s32  (CALLBACK* _GSsetWindowInfo)(winInfo *info);
#endif
typedef void (CALLBACK* _GSmakeSnapshot)(const char *path);
typedef void (CALLBACK* _GSmakeSnapshot2)(const char *path, int*, int);
typedef s32  (CALLBACK* _GSfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _GSconfigure)();
typedef s32  (CALLBACK* _GStest)();
typedef void (CALLBACK* _GSabout)();

// PAD
typedef s32  (CALLBACK* _PADinit)(char *configpath, u32 flags);
typedef s32  (CALLBACK* _PADopen)(void *pDisplay);
typedef void (CALLBACK* _PADclose)();
typedef void (CALLBACK* _PADshutdown)();
typedef keyEvent* (CALLBACK* _PADkeyEvent)();
typedef u8   (CALLBACK* _PADstartPoll)(u8 pad);
typedef u8   (CALLBACK* _PADpoll)(u8 value);
typedef u32  (CALLBACK* _PADquery)();
typedef void (CALLBACK* _PADupdate)(u8 pad);

typedef void (CALLBACK* _PADgsDriverInfo)(GSdriverInfo *info);
typedef s32  (CALLBACK* _PADfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _PADconfigure)();
typedef s32  (CALLBACK* _PADtest)();
typedef void (CALLBACK* _PADabout)();

// SIO
typedef s32  (CALLBACK* _SIOinit)(int types, SIOchangeSlotCB f);
typedef s32  (CALLBACK* _SIOopen)(void *pDisplay);
typedef void (CALLBACK* _SIOclose)();
typedef void (CALLBACK* _SIOshutdown)();
typedef s32   (CALLBACK* _SIOstartPoll)(u8 deviceType, u32 port, u32 slot, u8 *returnValue);
typedef s32   (CALLBACK* _SIOpoll)(u8 value, u8 *returnValue);
typedef u32  (CALLBACK* _SIOquery)();

typedef void (CALLBACK* _SIOkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _SIOfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _SIOconfigure)();
typedef s32  (CALLBACK* _SIOtest)();
typedef void (CALLBACK* _SIOabout)();

// SPU2
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _SPU2init)(char *configpath);
typedef s32  (CALLBACK* _SPU2open)(void *pDisplay);
typedef void (CALLBACK* _SPU2close)();
typedef void (CALLBACK* _SPU2shutdown)();
typedef void (CALLBACK* _SPU2write)(u32 mem, u16 value);
typedef u16  (CALLBACK* _SPU2read)(u32 mem);

typedef void (CALLBACK* _SPU2readDMA4Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2writeDMA4Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2interruptDMA4)();
typedef void (CALLBACK* _SPU2readDMA7Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2writeDMA7Mem)(u16 *pMem, u32 size);
typedef void (CALLBACK* _SPU2interruptDMA7)();

typedef void (CALLBACK* _SPU2readDMAMem)(u16 *pMem, u32 size, u8 core);
typedef void (CALLBACK* _SPU2writeDMAMem)(u16 *pMem, u32 size, u8 core);
typedef void (CALLBACK* _SPU2interruptDMA)(u8 core);
typedef void (CALLBACK* _SPU2setDMABaseAddr)(uptr baseaddr);
typedef void (CALLBACK* _SPU2irqCallback)(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());
typedef bool (CALLBACK* _SPU2setupRecording)(bool start);

typedef void (CALLBACK* _SPU2setClockPtr)(u32*ptr);
typedef void (CALLBACK* _SPU2setTimeStretcher)(short int enable);

typedef u32 (CALLBACK* _SPU2ReadMemAddr)(u8 core);
typedef void (CALLBACK* _SPU2WriteMemAddr)(u8 core,u32 value);
typedef void (CALLBACK* _SPU2async)(u32 cycles);
typedef s32  (CALLBACK* _SPU2freeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _SPU2keyEvent)(keyEvent* ev);
typedef void (CALLBACK* _SPU2configure)();
typedef s32  (CALLBACK* _SPU2test)();
typedef void (CALLBACK* _SPU2about)();

// CDVD
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _CDVDinit)(char *configpath);
typedef s32  (CALLBACK* _CDVDopen)(const char* pTitleFilename);
typedef void (CALLBACK* _CDVDclose)();
typedef void (CALLBACK* _CDVDshutdown)();
typedef s32  (CALLBACK* _CDVDreadTrack)(u32 lsn, int mode);
typedef u8*  (CALLBACK* _CDVDgetBuffer)();
typedef s32  (CALLBACK* _CDVDreadSubQ)(u32 lsn, cdvdSubQ* subq);
typedef s32  (CALLBACK* _CDVDgetTN)(cdvdTN *Buffer);
typedef s32  (CALLBACK* _CDVDgetTD)(u8 Track, cdvdTD *Buffer);
typedef s32  (CALLBACK* _CDVDgetTOC)(void* toc);
typedef s32  (CALLBACK* _CDVDgetDiskType)();
typedef s32  (CALLBACK* _CDVDgetTrayStatus)();
typedef s32  (CALLBACK* _CDVDctrlTrayOpen)();
typedef s32  (CALLBACK* _CDVDctrlTrayClose)();

typedef void (CALLBACK* _CDVDkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _CDVDfreeze)(u8 mode, freezeData *data);
typedef void (CALLBACK* _CDVDconfigure)();
typedef s32  (CALLBACK* _CDVDtest)();
typedef void (CALLBACK* _CDVDabout)();
typedef void (CALLBACK* _CDVDnewDiskCB)(void (*callback)());

// DEV9
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _DEV9init)(char *configpath);
typedef s32  (CALLBACK* _DEV9open)(void *pDisplay);
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

typedef void (CALLBACK* _DEV9keyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _DEV9freeze)(int mode, freezeData *data);
typedef void (CALLBACK* _DEV9configure)();
typedef s32  (CALLBACK* _DEV9test)();
typedef void (CALLBACK* _DEV9about)();

// USB
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
typedef s32  (CALLBACK* _USBinit)(char *configpath);
typedef s32  (CALLBACK* _USBopen)(void *pDisplay);
typedef void (CALLBACK* _USBclose)();
typedef void (CALLBACK* _USBshutdown)();
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

typedef void (CALLBACK* _USBkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _USBfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _USBconfigure)();
typedef s32  (CALLBACK* _USBtest)();
typedef void (CALLBACK* _USBabout)();

//FW
typedef s32  (CALLBACK* _FWinit)(char *configpath);
typedef s32  (CALLBACK* _FWopen)(void *pDisplay);
typedef void (CALLBACK* _FWclose)();
typedef void (CALLBACK* _FWshutdown)();
typedef u32  (CALLBACK* _FWread32)(u32 mem);
typedef void (CALLBACK* _FWwrite32)(u32 mem, u32 value);
typedef void (CALLBACK* _FWirqCallback)(void (*callback)());

typedef void (CALLBACK* _FWkeyEvent)(keyEvent* ev);
typedef s32  (CALLBACK* _FWfreeze)(int mode, freezeData *data);
typedef void (CALLBACK* _FWconfigure)();
typedef s32  (CALLBACK* _FWtest)();
typedef void (CALLBACK* _FWabout)();

// General 
extern _PS2EgetLibType PS2EgetLibType;
extern _PS2EgetLibVersion2 PS2EgetLibVersion2;
extern _PS2EgetLibName PS2EgetLibName;
extern _PS2EpassConfig PS2EpassConfig;

// GS
extern _GSinit            GSinit;
extern _GSopen            GSopen;
extern _GSclose           GSclose;
extern _GSshutdown        GSshutdown;
extern _GSvsync           GSvsync;
extern _GSgifTransfer1    GSgifTransfer1;
extern _GSgifTransfer2    GSgifTransfer2;
extern _GSgifTransfer3    GSgifTransfer3;
extern _GSgetLastTag      GSgetLastTag;
extern _GSgifSoftReset    GSgifSoftReset;
extern _GSreadFIFO        GSreadFIFO;
extern _GSreadFIFO2       GSreadFIFO2;

extern _GSkeyEvent        GSkeyEvent;
extern _GSchangeSaveState GSchangeSaveState;
extern _GSmakeSnapshot	   GSmakeSnapshot;
extern _GSmakeSnapshot2   GSmakeSnapshot2;
extern _GSirqCallback 	   GSirqCallback;
extern _GSprintf      	   GSprintf;
extern _GSsetBaseMem 	   GSsetBaseMem;
extern _GSsetGameCRC		GSsetGameCRC;
extern _GSsetFrameSkip	   GSsetFrameSkip;
extern _GSsetupRecording GSsetupRecording;
extern _GSreset		   GSreset;
extern _GSwriteCSR		   GSwriteCSR;
extern _GSgetDriverInfo   GSgetDriverInfo;
#ifdef _WINDOWS_
extern _GSsetWindowInfo   GSsetWindowInfo;
#endif
extern _GSfreeze          GSfreeze;
extern _GSconfigure       GSconfigure;
extern _GStest            GStest;
extern _GSabout           GSabout;

// PAD1
extern _PADinit           PAD1init;
extern _PADopen           PAD1open;
extern _PADclose          PAD1close;
extern _PADshutdown       PAD1shutdown;
extern _PADkeyEvent       PAD1keyEvent;
extern _PADstartPoll      PAD1startPoll;
extern _PADpoll           PAD1poll;
extern _PADquery          PAD1query;
extern _PADupdate         PAD1update;

extern _PADfreeze          PAD1freeze;
extern _PADgsDriverInfo   PAD1gsDriverInfo;
extern _PADconfigure      PAD1configure;
extern _PADtest           PAD1test;
extern _PADabout          PAD1about;

// PAD2
extern _PADinit           PAD2init;
extern _PADopen           PAD2open;
extern _PADclose          PAD2close;
extern _PADshutdown       PAD2shutdown;
extern _PADkeyEvent       PAD2keyEvent;
extern _PADstartPoll      PAD2startPoll;
extern _PADpoll           PAD2poll;
extern _PADquery          PAD2query;
extern _PADupdate         PAD2update;

extern _PADfreeze          PAD2freeze;
extern _PADgsDriverInfo   PAD2gsDriverInfo;
extern _PADconfigure      PAD2configure;
extern _PADtest           PAD2test;
extern _PADabout          PAD2about;

// SIO[2]
extern _SIOinit           SIOinit[2][9];
extern _SIOopen           SIOopen[2][9];
extern _SIOclose          SIOclose[2][9];
extern _SIOshutdown       SIOshutdown[2][9];
extern _SIOstartPoll      SIOstartPoll[2][9];
extern _SIOpoll           SIOpoll[2][9];
extern _SIOquery          SIOquery[2][9];
extern _SIOkeyEvent      SIOkeyEvent;

extern _SIOfreeze          SIOfreeze[2][9];
extern _SIOconfigure      SIOconfigure[2][9];
extern _SIOtest           SIOtest[2][9];
extern _SIOabout          SIOabout[2][9];

// SPU2
extern _SPU2init          SPU2init;
extern _SPU2open          SPU2open;
extern _SPU2close         SPU2close;
extern _SPU2shutdown      SPU2shutdown;
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

extern _SPU2keyEvent        SPU2keyEvent;
extern _SPU2async         SPU2async;
extern _SPU2freeze        SPU2freeze;
extern _SPU2configure     SPU2configure;
extern _SPU2test          SPU2test;
extern _SPU2about         SPU2about;

// CDVD
extern _CDVDinit          CDVDinit;
extern _CDVDopen          CDVDopen;
extern _CDVDclose         CDVDclose;
extern _CDVDshutdown      CDVDshutdown;
extern _CDVDreadTrack     CDVDreadTrack;
extern _CDVDgetBuffer     CDVDgetBuffer;
extern _CDVDreadSubQ      CDVDreadSubQ;
extern _CDVDgetTN         CDVDgetTN;
extern _CDVDgetTD         CDVDgetTD;
extern _CDVDgetTOC        CDVDgetTOC;
extern _CDVDgetDiskType   CDVDgetDiskType;
extern _CDVDgetTrayStatus CDVDgetTrayStatus;
extern _CDVDctrlTrayOpen  CDVDctrlTrayOpen;
extern _CDVDctrlTrayClose CDVDctrlTrayClose;

extern _CDVDkeyEvent        CDVDkeyEvent;
extern _CDVDfreeze          CDVDfreeze;
extern _CDVDconfigure     CDVDconfigure;
extern _CDVDtest          CDVDtest;
extern _CDVDabout         CDVDabout;
extern _CDVDnewDiskCB     CDVDnewDiskCB;

// DEV9
extern _DEV9init          DEV9init;
extern _DEV9open          DEV9open;
extern _DEV9close         DEV9close;
extern _DEV9shutdown      DEV9shutdown;
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

extern _DEV9keyEvent        DEV9keyEvent;
extern _DEV9configure     DEV9configure;
extern _DEV9freeze        DEV9freeze;
extern _DEV9test          DEV9test;
extern _DEV9about         DEV9about;

// USB
extern _USBinit           USBinit;
extern _USBopen           USBopen;
extern _USBclose          USBclose;
extern _USBshutdown       USBshutdown;
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

extern _USBkeyEvent       USBkeyEvent;
extern _USBconfigure      USBconfigure;
extern _USBfreeze         USBfreeze;
extern _USBtest           USBtest;
extern _USBabout          USBabout;

// FW
extern _FWinit            FWinit;
extern _FWopen            FWopen;
extern _FWclose           FWclose;
extern _FWshutdown        FWshutdown;
extern _FWread32          FWread32;
extern _FWwrite32         FWwrite32;
extern _FWirqCallback     FWirqCallback;

extern _FWkeyEvent        FWkeyEvent;
extern _FWconfigure       FWconfigure;
extern _FWfreeze          FWfreeze;
extern _FWtest            FWtest;
extern _FWabout           FWabout;
}

#endif // __PLUGINCALLBACKS_H__
