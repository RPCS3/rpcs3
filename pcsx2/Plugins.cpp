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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "RedtapeWindows.h"

#include "IopCommon.h"
#include "GS.h"
#include "HostGui.h"
#include "CDVD/CDVDisoReader.h"

_GSinit            GSinit;
_GSopen            GSopen;
_GSclose           GSclose;
_GSshutdown        GSshutdown;
_GSvsync           GSvsync;
_GSgifTransfer1    GSgifTransfer1;
_GSgifTransfer2    GSgifTransfer2;
_GSgifTransfer3    GSgifTransfer3;
_GSgetLastTag      GSgetLastTag;
_GSgifSoftReset    GSgifSoftReset;
_GSreadFIFO        GSreadFIFO;
_GSreadFIFO2       GSreadFIFO2;

_GSkeyEvent        GSkeyEvent;
_GSchangeSaveState GSchangeSaveState;
_GSmakeSnapshot	   GSmakeSnapshot;
_GSmakeSnapshot2   GSmakeSnapshot2;
_GSirqCallback 	   GSirqCallback;
_GSprintf      	   GSprintf;
_GSsetBaseMem 	   GSsetBaseMem;
_GSsetGameCRC		GSsetGameCRC;
_GSsetFrameSkip	   GSsetFrameSkip;
_GSsetFrameLimit   GSsetFrameLimit;
_GSsetupRecording GSsetupRecording;
_GSreset		   GSreset;
_GSwriteCSR		   GSwriteCSR;
_GSgetDriverInfo   GSgetDriverInfo;
#ifdef _WINDOWS_
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
_PADupdate         PAD1update;

_PADgsDriverInfo   PAD1gsDriverInfo;
_PADconfigure      PAD1configure;
_PADtest           PAD1test;
_PADabout          PAD1about;
_PADfreeze         PAD1freeze;
_PADsetSlot        PAD1setSlot;
_PADqueryMtap      PAD1queryMtap;

// PAD2
_PADinit           PAD2init;
_PADopen           PAD2open;
_PADclose          PAD2close;
_PADshutdown       PAD2shutdown;
_PADkeyEvent       PAD2keyEvent;
_PADstartPoll      PAD2startPoll;
_PADpoll           PAD2poll;
_PADquery          PAD2query;
_PADupdate         PAD2update;

_PADgsDriverInfo   PAD2gsDriverInfo;
_PADconfigure      PAD2configure;
_PADtest           PAD2test;
_PADabout          PAD2about;
_PADfreeze         PAD2freeze;
_PADsetSlot        PAD2setSlot;
_PADqueryMtap      PAD2queryMtap;

// SIO[2]
_SIOinit           SIOinit[2][9];
_SIOopen           SIOopen[2][9];
_SIOclose          SIOclose[2][9];
_SIOshutdown       SIOshutdown[2][9];
_SIOstartPoll      SIOstartPoll[2][9];
_SIOpoll           SIOpoll[2][9];
_SIOquery          SIOquery[2][9];

_SIOconfigure      SIOconfigure[2][9];
_SIOtest           SIOtest[2][9];
_SIOabout          SIOabout[2][9];

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
_SPU2setDMABaseAddr SPU2setDMABaseAddr;
_SPU2interruptDMA7 SPU2interruptDMA7;
_SPU2ReadMemAddr   SPU2ReadMemAddr;
_SPU2setupRecording SPU2setupRecording;
_SPU2WriteMemAddr   SPU2WriteMemAddr;
_SPU2irqCallback   SPU2irqCallback;

_SPU2setClockPtr   SPU2setClockPtr;
_SPU2setTimeStretcher SPU2setTimeStretcher;

_SPU2async         SPU2async;
_SPU2freeze        SPU2freeze;
_SPU2configure     SPU2configure;
_SPU2test          SPU2test;
_SPU2about         SPU2about;

// CDVD
CDVDplugin CDVD_plugin = {0};
CDVDplugin CDVD = {0};

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
_USBasync          USBasync;

_USBirqCallback    USBirqCallback;
_USBirqHandler     USBirqHandler;
_USBsetRAM         USBsetRAM;

_USBconfigure      USBconfigure;
_USBfreeze         USBfreeze;
_USBtest           USBtest;
_USBabout          USBabout;

// FW
_FWinit            FWinit;
_FWopen            FWopen;
_FWclose           FWclose;
_FWshutdown        FWshutdown;
_FWread32          FWread32;
_FWwrite32         FWwrite32;
_FWirqCallback     FWirqCallback;

_FWconfigure       FWconfigure;
_FWfreeze          FWfreeze;
_FWtest            FWtest;
_FWabout           FWabout;

DEV9handler dev9Handler;
USBhandler usbHandler;

namespace PluginTypes
{
	enum PluginTypes
	{
		GS = 0,
		PAD,
		PAD1,
		PAD2,
		SPU2,
		CDVD,
		DEV9,
		USB,
		FW
	};
}

int PS2E_LT[9] = { 
PS2E_LT_GS, 
PS2E_LT_PAD,PS2E_LT_PAD, PS2E_LT_PAD, 
PS2E_LT_SPU2, 
PS2E_LT_CDVD, 
PS2E_LT_DEV9,
PS2E_LT_USB,
PS2E_LT_FW};

int PS2E_VERSION[9] = {
PS2E_GS_VERSION, 
PS2E_PAD_VERSION,PS2E_PAD_VERSION, PS2E_PAD_VERSION, 
PS2E_SPU2_VERSION, 
PS2E_CDVD_VERSION, 
PS2E_DEV9_VERSION,
PS2E_USB_VERSION,
PS2E_FW_VERSION};

#define Sfy(x) #x
#define Strfy(x) Sfy(x)
#define MapSymbolVarType(var,type,name) var = (type)SysLoadSym(drv,Strfy(name))
#define MapSymbolVar(var,name) MapSymbolVarType(var,_##name,name)
#define MapSymbolVar_Fallback(var,name,fallback) if((MapSymbolVar(var,name))==NULL) var = fallback
#define MapSymbolVar_Error(var,name) if((MapSymbolVar(var,name))==NULL) \
{ \
	const char* errString = SysLibError(); \
	Msgbox::Alert("%s: Error loading %hs: %s", params &filename, #name, errString); \
	return -1; \
}

#define MapSymbol(name) MapSymbolVar(name,name)
#define MapSymbol_Fallback(name,fallback) MapSymbolVar_Fallback(name,name,fallback)
#define MapSymbol_Error(name) MapSymbolVar_Error(name,name)

#define MapSymbol2(base,name) MapSymbolVar(base##_plugin.name,base##name)
#define MapSymbol2_Fallback(base,name,fallback) MapSymbolVar_Fallback(base##_plugin.name,base##name,fallback)
#define MapSymbol2_Error(base,name) MapSymbolVar_Error(base##_plugin.name,base##name)

// for pad1/2
#define MapSymbolPAD(var,name) MapSymbolVar(var##name,PAD##name)
#define MapSymbolPAD_Fallback(var,name) if((MapSymbolVarType(var##name,_PAD##name,PAD##name))==NULL) var##name = var##_##name
#define MapSymbolPAD_Error(var,name) MapSymbolVar_Error(var##name,PAD##name)

void *GSplugin;

static int _TestPS2Esyms(void* drv, int type, int expected_version, const string& filename)
{
	_PS2EgetLibType PS2EgetLibType;
	_PS2EgetLibVersion2 PS2EgetLibVersion2;
	_PS2EgetLibName PS2EgetLibName;

	MapSymbol_Error(PS2EgetLibType);
	MapSymbol_Error(PS2EgetLibVersion2);
	MapSymbol_Error(PS2EgetLibName);

	int actual_version = ((PS2EgetLibVersion2(type) >> 16)&0xff);

	if( actual_version != expected_version) {
		Msgbox::Alert("Can't load '%hs', wrong PS2E version (%x != %x)", params &filename, actual_version, expected_version);
		return -1;
	}

	return 0;
}

static __forceinline bool TestPS2Esyms(void* &drv, PluginTypes::PluginTypes type, const string& filename) 
{
	if (_TestPS2Esyms(drv, PS2E_LT[type],PS2E_VERSION[type],filename) < 0) return false;
	return true;
}

void CALLBACK GS_printf(int timeout, char *fmt, ...) {
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	Console::WriteLn(msg);
}

s32  CALLBACK GS_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK GS_keyEvent(keyEvent *ev) {}
void CALLBACK GS_makeSnapshot(const char *path) {}
void CALLBACK GS_irqCallback(void (*callback)()) {}
void CALLBACK GS_configure() {}
void CALLBACK GS_about() {}
s32  CALLBACK GS_test() { return 0; }

int LoadGSplugin(const string& filename)
{
	void *drv;

	GSplugin = SysLoadLibrary(filename.c_str());
	if (GSplugin == NULL) { Msgbox::Alert ("Could Not Load GS Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = GSplugin;
	if (!TestPS2Esyms(drv, PluginTypes::GS, filename)) return -1;
	MapSymbol_Error(GSinit);
	MapSymbol_Error(GSshutdown);
	MapSymbol_Error(GSopen);
	MapSymbol_Error(GSclose);
	MapSymbol_Error(GSgifTransfer1);
	MapSymbol_Error(GSgifTransfer2);
	MapSymbol_Error(GSgifTransfer3);
	MapSymbol_Error(GSreadFIFO);
	MapSymbol(GSgetLastTag);
	MapSymbol(GSreadFIFO2); // optional
	MapSymbol_Error(GSvsync);

	MapSymbol_Fallback(GSkeyEvent,GS_keyEvent);
	MapSymbol(GSchangeSaveState);
	MapSymbol(GSgifSoftReset);
	MapSymbol_Fallback(GSmakeSnapshot,GS_makeSnapshot);
	MapSymbol_Fallback(GSirqCallback,GS_irqCallback);
	MapSymbol_Fallback(GSprintf,GS_printf);
	MapSymbol_Error(GSsetBaseMem);
	MapSymbol(GSsetGameCRC);
	MapSymbol_Error(GSreset);
	MapSymbol_Error(GSwriteCSR);
	MapSymbol(GSmakeSnapshot2);
	MapSymbol(GSgetDriverInfo);

	MapSymbol(GSsetFrameSkip);
	MapSymbol(GSsetFrameLimit);
	MapSymbol(GSsetupRecording);

#ifdef _WIN32
	MapSymbol(GSsetWindowInfo);
#endif
	MapSymbol_Fallback(GSfreeze,GS_freeze);
	MapSymbol_Fallback(GSconfigure,GS_configure);
	MapSymbol_Fallback(GSabout,GS_about);
	MapSymbol_Fallback(GStest,GS_test);

	return 0;
}

void *PAD1plugin;

void CALLBACK PAD1_configure() {}
void CALLBACK PAD1_about() {}
s32  CALLBACK PAD1_test() { return 0; }
s32  CALLBACK PAD1_freeze(int mode, freezeData *data) { if (mode == FREEZE_SIZE) data->size = 0; return 0; }
s32  CALLBACK PAD1_setSlot(u8 port, u8 slot) { return slot == 1; }
s32  CALLBACK PAD1_queryMtap(u8 port) { return 0; }

int LoadPAD1plugin(const string& filename) {
	void *drv;

	PAD1plugin = SysLoadLibrary(filename.c_str());
	if (PAD1plugin == NULL) { Msgbox::Alert("Could Not Load PAD1 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = PAD1plugin;
	if (!TestPS2Esyms(drv, PluginTypes::PAD, filename)) return -1;
	MapSymbolPAD_Error(PAD1,init);
	MapSymbolPAD_Error(PAD1,shutdown);
	MapSymbolPAD_Error(PAD1,open);
	MapSymbolPAD_Error(PAD1,close);
	MapSymbolPAD_Error(PAD1,keyEvent);
	MapSymbolPAD_Error(PAD1,startPoll);
	MapSymbolPAD_Error(PAD1,poll);
	MapSymbolPAD_Error(PAD1,query);
	MapSymbolPAD(PAD1,update);

	MapSymbolPAD(PAD1,gsDriverInfo);
	MapSymbolPAD_Fallback(PAD1,configure);
	MapSymbolPAD_Fallback(PAD1,about);
	MapSymbolPAD_Fallback(PAD1,test);
	MapSymbolPAD_Fallback(PAD1,freeze);
	MapSymbolPAD_Fallback(PAD1,setSlot);
	MapSymbolPAD_Fallback(PAD1,queryMtap);

	return 0;
}

void *PAD2plugin;

void CALLBACK PAD2_configure() {}
void CALLBACK PAD2_about() {}
s32  CALLBACK PAD2_test() { return 0; }
s32  CALLBACK PAD2_freeze(int mode, freezeData *data) { if (mode == FREEZE_SIZE) data->size = 0; return 0; }
s32  CALLBACK PAD2_setSlot(u8 port, u8 slot) { return slot == 1; }
s32  CALLBACK PAD2_queryMtap(u8 port) { return 0; }

int LoadPAD2plugin(const string& filename) {
	void *drv;

	PAD2plugin = SysLoadLibrary(filename.c_str());
	if (PAD2plugin == NULL) { Msgbox::Alert("Could Not Load PAD2 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = PAD2plugin;
	if (!TestPS2Esyms(drv, PluginTypes::PAD, filename)) return -1;
	MapSymbolPAD_Error(PAD2,init);
	MapSymbolPAD_Error(PAD2,shutdown);
	MapSymbolPAD_Error(PAD2,open);
	MapSymbolPAD_Error(PAD2,close);
	MapSymbolPAD_Error(PAD2,keyEvent);
	MapSymbolPAD_Error(PAD2,startPoll);
	MapSymbolPAD_Error(PAD2,poll);
	MapSymbolPAD_Error(PAD2,query);
	MapSymbolPAD(PAD2,update);

	MapSymbolPAD(PAD2,gsDriverInfo);
	MapSymbolPAD_Fallback(PAD2,configure);
	MapSymbolPAD_Fallback(PAD2,about);
	MapSymbolPAD_Fallback(PAD2,test);
	MapSymbolPAD_Fallback(PAD2,freeze);
	MapSymbolPAD_Fallback(PAD2,setSlot);
	MapSymbolPAD_Fallback(PAD2,queryMtap);

	return 0;
}

void *SPU2plugin;

s32  CALLBACK SPU2_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK SPU2_configure() {}
void CALLBACK SPU2_about() {}
s32  CALLBACK SPU2_test() { return 0; }

int LoadSPU2plugin(const string& filename) {
	void *drv;

	SPU2plugin = SysLoadLibrary(filename.c_str());
	if (SPU2plugin == NULL) { Msgbox::Alert("Could Not Load SPU2 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = SPU2plugin;
	if (!TestPS2Esyms(drv, PluginTypes::SPU2, filename)) return -1;
	MapSymbol_Error(SPU2init);
	MapSymbol_Error(SPU2shutdown);
	MapSymbol_Error(SPU2open);
	MapSymbol_Error(SPU2close);
	MapSymbol_Error(SPU2write);
	MapSymbol_Error(SPU2read);
	MapSymbol_Error(SPU2readDMA4Mem);
	MapSymbol_Error(SPU2writeDMA4Mem);
	MapSymbol_Error(SPU2interruptDMA4);
	MapSymbol_Error(SPU2readDMA7Mem);
	MapSymbol_Error(SPU2writeDMA7Mem);
	MapSymbol_Error(SPU2interruptDMA7);
	MapSymbol(SPU2setDMABaseAddr);
	MapSymbol_Error(SPU2ReadMemAddr);
	MapSymbol_Error(SPU2WriteMemAddr);
	MapSymbol_Error(SPU2irqCallback);

	MapSymbol(SPU2setClockPtr);

	MapSymbol(SPU2setupRecording);

	MapSymbol_Fallback(SPU2freeze,SPU2_freeze);
	MapSymbol_Fallback(SPU2configure,SPU2_configure);
	MapSymbol_Fallback(SPU2about,SPU2_about);
	MapSymbol_Fallback(SPU2test,SPU2_test);
	MapSymbol(SPU2async);

	return 0;
}

void *CDVDplugin;

void CALLBACK CDVD_configure() {}
void CALLBACK CDVD_about() {}
s32  CALLBACK CDVD_test() { return 0; }
void CALLBACK CDVD_newDiskCB(void (*callback)()) {}

extern int lastReadSize;
s32 CALLBACK CDVD_getBuffer2(u8* buffer)
{
	int ret;


	// TEMP: until I fix all the plugins to use this function style
	u8* pb = CDVD.getBuffer();
	if(pb!=NULL)
	{
		memcpy(buffer,pb,lastReadSize);
		ret=0;
	}
	else ret= -1;

	return ret;
}


s32 CALLBACK CDVD_readSector(u8* buffer, u32 lsn, int mode)
{
	if(CDVD.readTrack(lsn,mode)<0)
		return -1;
	
	// TEMP: until all the plugins use the new CDVDgetBuffer style
	switch (mode)
	{
	case CDVD_MODE_2352:
		lastReadSize = 2352;
		break;
	case CDVD_MODE_2340:
		lastReadSize = 2340;
		break;
	case CDVD_MODE_2328:
		lastReadSize = 2328;
		break;
	case CDVD_MODE_2048:
		lastReadSize = 2048;
		break;
	}
	return CDVD.getBuffer2(buffer);
}

s32 CALLBACK CDVD_getDualInfo(s32* dualType, u32* layer1Start)
{
	u8 toc[2064];

	// if error getting toc, settle for single layer disc ;)
	if(CDVD.getTOC(toc))
		return 0;
	if(toc[14] & 0x60)
	{
		if(toc[14] & 0x10)
		{
			// otp dvd
			*dualType = 2;
			*layer1Start = (toc[25]<<16) + (toc[26]<<8) + (toc[27]) - 0x30000 + 1;
		}
		else
		{
			// ptp dvd
			*dualType = 1;
			*layer1Start = (toc[21]<<16) + (toc[22]<<8) + (toc[23]) - 0x30000 + 1;
		}
	}
	else
	{
		// single layer dvd
		*dualType = 0;
		*layer1Start = (toc[21]<<16) + (toc[22]<<8) + (toc[23]) - 0x30000 + 1;
	}

	return 1;
}

int cdvdInitCount;
int LoadCDVDplugin(const string& filename) {
	void *drv;

	CDVDplugin = SysLoadLibrary(filename.c_str());
	if (CDVDplugin == NULL) { Msgbox::Alert("Could Not Load CDVD Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = CDVDplugin;
	if (!TestPS2Esyms(drv, PluginTypes::CDVD, filename)) return -1;
	MapSymbol2_Error(CDVD,init);
	MapSymbol2_Error(CDVD,shutdown);
	MapSymbol2_Error(CDVD,open);
	MapSymbol2_Error(CDVD,close);
	MapSymbol2_Error(CDVD,readTrack);
	MapSymbol2_Error(CDVD,getBuffer);
	MapSymbol2_Error(CDVD,readSubQ);
	MapSymbol2_Error(CDVD,getTN);
	MapSymbol2_Error(CDVD,getTD);
	MapSymbol2_Error(CDVD,getTOC);
	MapSymbol2_Error(CDVD,getDiskType);
	MapSymbol2_Error(CDVD,getTrayStatus);
	MapSymbol2_Error(CDVD,ctrlTrayOpen);
	MapSymbol2_Error(CDVD,ctrlTrayClose);

	MapSymbol2_Fallback(CDVD,configure,CDVD_configure);
	MapSymbol2_Fallback(CDVD,about,CDVD_about);
	MapSymbol2_Fallback(CDVD,test,CDVD_test);
	MapSymbol2_Fallback(CDVD,newDiskCB,CDVD_newDiskCB);

	MapSymbol2_Fallback(CDVD,readSector,CDVD_readSector);
	MapSymbol2_Fallback(CDVD,getBuffer2,CDVD_getBuffer2);
	MapSymbol2_Fallback(CDVD,getDualInfo,CDVD_getDualInfo);

	CDVD.initCount = &cdvdInitCount;
	cdvdInitCount=0;

	return 0;
}

void *DEV9plugin;

s32  CALLBACK DEV9_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK DEV9_configure() {}
void CALLBACK DEV9_about() {}
s32  CALLBACK DEV9_test() { return 0; }

int LoadDEV9plugin(const string& filename) {
	void *drv;

	DEV9plugin = SysLoadLibrary(filename.c_str());
	if (DEV9plugin == NULL) { Msgbox::Alert("Could Not Load DEV9 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = DEV9plugin;
	if (!TestPS2Esyms(drv, PluginTypes::DEV9, filename)) return -1;
	MapSymbol_Error(DEV9init);
	MapSymbol_Error(DEV9shutdown);
	MapSymbol_Error(DEV9open);
	MapSymbol_Error(DEV9close);
	MapSymbol_Error(DEV9read8);
	MapSymbol_Error(DEV9read16);
	MapSymbol_Error(DEV9read32);
	MapSymbol_Error(DEV9write8);
	MapSymbol_Error(DEV9write16);
	MapSymbol_Error(DEV9write32);
	MapSymbol_Error(DEV9readDMA8Mem);
	MapSymbol_Error(DEV9writeDMA8Mem);
	MapSymbol_Error(DEV9irqCallback);
	MapSymbol_Error(DEV9irqHandler);

	MapSymbol_Fallback(DEV9freeze,DEV9_freeze);
	MapSymbol_Fallback(DEV9configure,DEV9_configure);
	MapSymbol_Fallback(DEV9about,DEV9_about);
	MapSymbol_Fallback(DEV9test,DEV9_test);

	return 0;
}

void *USBplugin;

s32  CALLBACK USB_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK USB_configure() {}
void CALLBACK USB_about() {}
s32 CALLBACK USB_test() { return 0; }

int LoadUSBplugin(const string& filename) {
	void *drv;

	USBplugin = SysLoadLibrary(filename.c_str());
	if (USBplugin == NULL) { Msgbox::Alert("Could Not Load USB Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = USBplugin;
	if (!TestPS2Esyms(drv, PluginTypes::USB, filename)) return -1;
	MapSymbol_Error(USBinit);
	MapSymbol_Error(USBshutdown);
	MapSymbol_Error(USBopen);
	MapSymbol_Error(USBclose);
	MapSymbol_Error(USBread8);
	MapSymbol_Error(USBread16);
	MapSymbol_Error(USBread32);
	MapSymbol_Error(USBwrite8);
	MapSymbol_Error(USBwrite16);
	MapSymbol_Error(USBwrite32);
	MapSymbol_Error(USBirqCallback);
	MapSymbol_Error(USBirqHandler);
	MapSymbol_Error(USBsetRAM);

	MapSymbol(USBasync);

	MapSymbol_Fallback(USBfreeze,USB_freeze);
	MapSymbol_Fallback(USBconfigure,USB_configure);
	MapSymbol_Fallback(USBabout,USB_about);
	MapSymbol_Fallback(USBtest,USB_test);

	return 0;
}
void *FWplugin;

s32  CALLBACK FW_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK FW_configure() {}
void CALLBACK FW_about() {}
s32 CALLBACK FW_test() { return 0; }

int LoadFWplugin(const string& filename) {
	void *drv;

	FWplugin = SysLoadLibrary(filename.c_str());
	if (FWplugin == NULL) { Msgbox::Alert("Could Not Load FW Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = FWplugin;
	if (!TestPS2Esyms(drv, PluginTypes::FW, filename)) return -1;
	MapSymbol_Error(FWinit);
	MapSymbol_Error(FWshutdown);
	MapSymbol_Error(FWopen);
	MapSymbol_Error(FWclose);
	MapSymbol_Error(FWread32);
	MapSymbol_Error(FWwrite32);
	MapSymbol_Error(FWirqCallback);

	MapSymbol_Fallback(FWfreeze,FW_freeze);
	MapSymbol_Fallback(FWconfigure,FW_configure);
	MapSymbol_Fallback(FWabout,FW_about);
	MapSymbol_Fallback(FWtest,FW_test);

	return 0;
}

struct PluginOpenStatusFlags
{
	u8	GS : 1
	,	CDVD : 1
	,	DEV9 : 1
	,	USB : 1
	,	SPU2 : 1
	,	PAD1 : 1
	,	PAD2 : 1
	,	FW : 1;

};

static PluginOpenStatusFlags OpenStatus = {0};

static bool plugins_loaded = false;
static bool plugins_initialized = false;
static bool only_loading_elf = false;

int LoadPlugins()
{
	if (plugins_loaded) return 0;

	if (LoadGSplugin(	Path::Combine( Config.PluginsDir, Config.GS )) == -1) return -1;
	if (LoadPAD1plugin(	Path::Combine( Config.PluginsDir, Config.PAD1 )) == -1) return -1;
	if (LoadPAD2plugin(	Path::Combine( Config.PluginsDir, Config.PAD2 )) == -1) return -1;
	if (LoadSPU2plugin(	Path::Combine( Config.PluginsDir, Config.SPU2 )) == -1) return -1;
	if (LoadCDVDplugin(	Path::Combine( Config.PluginsDir, Config.CDVD )) == -1) return -1;
	if (LoadDEV9plugin(	Path::Combine( Config.PluginsDir, Config.DEV9 )) == -1) return -1;
	if (LoadUSBplugin(	Path::Combine( Config.PluginsDir, Config.USB )) == -1) return -1;
	if (LoadFWplugin(	Path::Combine( Config.PluginsDir, Config.FW )) == -1) return -1;

	plugins_loaded = true;

	return 0;
}

bool ReportError(int err, const char *str)
{
	if (err != 0)
	{
		Msgbox::Alert("%s error: %d", params str, err); 
		return true;
	}
	return false;
}

bool ReportError2(int err, const char *str)
{
	if (err != 0)
	{
		Msgbox::Alert("Error Opening %s Plugin", params str, err); 
		return true;
	}
	return false;
}

int InitPlugins()
{
	if (plugins_initialized) return 0;

	// Ensure plugins have been loaded....
	if (LoadPlugins() == -1) return -1;

	//if( !plugins_loaded ) throw Exception::InvalidOperation( "Bad coder mojo - InitPlugins called prior to plugins having been loaded." );

	if (ReportError(GSinit(), "GSinit")) return -1;
	if (ReportError(PAD1init(1), "PAD1init")) return -1;
	if (ReportError(PAD2init(2), "PAD2init")) return -1;
	if (ReportError(SPU2init(), "SPU2init")) return -1;
	
	if (ReportError(DoCDVDinit(), "CDVDinit")) return -1;
	
	if (ReportError(DEV9init(), "DEV9init")) return -1;
	if (ReportError(USBinit(), "USBinit")) return -1;
	if (ReportError(FWinit(), "FWinit")) return -1;

	only_loading_elf = false;
	plugins_initialized = true;
	return 0;
}

void ShutdownPlugins()
{
	if (!plugins_initialized) return;

	mtgsWaitGS();
	ClosePlugins( true );

	if (GSshutdown != NULL) GSshutdown();

	if (PAD1shutdown != NULL) PAD1shutdown();
	if (PAD2shutdown != NULL) PAD2shutdown();

	if (SPU2shutdown != NULL) SPU2shutdown();
	
	//if (CDVDshutdown != NULL) CDVDshutdown();
	DoCDVDshutdown();

	// safety measures, in case ISO is currently loaded.
	if(cdvdInitCount>0)
		CDVD_plugin.shutdown();
	
	if (DEV9shutdown != NULL) DEV9shutdown();
	if (USBshutdown != NULL) USBshutdown();
	if (FWshutdown != NULL) FWshutdown();

	plugins_initialized = false;
}

uptr pDsp;
extern void spu2DMA4Irq();
extern void spu2DMA7Irq();
extern void spu2Irq();

bool OpenGS()
{
	GSdriverInfo info;
	
	if (!OpenStatus.GS) 
	{
		if (ReportError2(gsOpen(), "GS")) 
		{ 
			ClosePlugins(true);
			return false; 
		}

		//Get the user input.
		if (GSgetDriverInfo)
		{
			GSgetDriverInfo(&info);
			if (PAD1gsDriverInfo) PAD1gsDriverInfo(&info);
			if (PAD2gsDriverInfo) PAD2gsDriverInfo(&info);
		}
		OpenStatus.GS = true;
	}
	return true;
}

bool OpenCDVD(const char* pTitleFilename)
{
	// Don't repetitively open the CDVD plugin if directly loading an elf file and open failed once already.
	if (!OpenStatus.CDVD && !only_loading_elf)
	{
		//First, we need the data.
		CDVD.newDiskCB(cdvdNewDiskCB);

		if (DoCDVDopen(pTitleFilename) != 0) 
		{ 
			if (g_Startup.BootMode != BootMode_Elf) 
			{ 
				Msgbox::Alert("Error Opening CDVD Plugin"); 
				ClosePlugins(true); 
				return false;
			}
			else 
			{ 
				Console::Notice("Running ELF File Without CDVD Plugin Support!"); 
				only_loading_elf = true; 
			}
		}
		OpenStatus.CDVD = true;
	}
	return true;
}

bool OpenPAD1()
{
	if (!OpenStatus.PAD1)
	{
		if (ReportError2(PAD1open((void *)&pDsp), "PAD1")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.PAD1 = true;
	}
	return true;
}

bool OpenPAD2()
{	
	if (!OpenStatus.PAD2)
	{
		if (ReportError2(PAD2open((void *)&pDsp), "PAD2")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.PAD2 = true;
	}
	return true;
}

bool OpenSPU2()
{
	if (!OpenStatus.SPU2)
	{
		SPU2irqCallback(spu2Irq,spu2DMA4Irq,spu2DMA7Irq);
		
		if (SPU2setDMABaseAddr != NULL) SPU2setDMABaseAddr((uptr)psxM);
		if (SPU2setClockPtr != NULL) SPU2setClockPtr(&psxRegs.cycle);

		if (ReportError2(SPU2open((void*)&pDsp), "SPU2")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.SPU2 = true;
	}
	return true;
}

bool OpenDEV9()
{
	if (!OpenStatus.DEV9)
	{
		DEV9irqCallback(dev9Irq);
		dev9Handler = DEV9irqHandler();
		
		if (ReportError2(DEV9open(&psxRegs.pc)/*((void *)&pDsp)*/, "DEV9")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.DEV9 = true;
	}
	return true;
}
bool OpenUSB()
{
	if (!OpenStatus.USB)
	{
		USBirqCallback(usbIrq);
		usbHandler = USBirqHandler();
		USBsetRAM(psxM);
		
		if (ReportError2(USBopen((void *)&pDsp), "USB")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.USB = true;
	}
	return true;
}

bool OpenFW()
{
	if (!OpenStatus.FW)
	{
		FWirqCallback(fwIrq);
		
		if (ReportError2(FWopen((void *)&pDsp), "FW")) 
		{ 
			ClosePlugins(true); 
			return false; 
		}
		OpenStatus.FW = true;
	}
	return true;
}

int OpenPlugins(const char* pTitleFilename)
{
	if (!plugins_initialized)
	{
		if( InitPlugins() == -1 ) return -1;
	}

	if ((!OpenCDVD(pTitleFilename)) || (!OpenGS()) || (!OpenPAD1()) || (!OpenPAD2()) ||
	    (!OpenSPU2()) || (!OpenDEV9()) || (!OpenUSB()) || (!OpenFW()))
		return -1;
	
	if (!only_loading_elf) cdvdDetectDisk();
	return 0;
}


#define CLOSE_PLUGIN( name ) \
	if( OpenStatus.name ) { \
		name##close(); \
		OpenStatus.name = false; \
	}

#define CLOSE_PLUGIN2( name ) \
	if( OpenStatus.name ) { \
	name.close(); \
	OpenStatus.name = false; \
	}


void ClosePlugins( bool closegs )
{
	// Close pads first since they attach to the GS's window.

	CLOSE_PLUGIN( PAD1 );
	CLOSE_PLUGIN( PAD2 );

	// GS plugin is special and is not always closed during emulation pauses.
	// (that's because the GS is the most complicated plugin and to close it would
	// require we save the GS state)

	if( OpenStatus.GS )
	{
		if( closegs )
		{
			gsClose();
			OpenStatus.GS = false;
		}
		else
		{
			mtgsWaitGS();
		}
	}

	if( OpenStatus.CDVD )
	{
		DoCDVDclose();
		OpenStatus.CDVD=false;
	}

	CLOSE_PLUGIN( DEV9 );
	CLOSE_PLUGIN( USB );
	CLOSE_PLUGIN( FW );
	CLOSE_PLUGIN( SPU2 );
}

//used to close the GS plugin window and pads, to switch gsdx renderer
void CloseGS()
{
	if( CHECK_MULTIGS ) mtgsWaitGS();

	CLOSE_PLUGIN( PAD1 );
	CLOSE_PLUGIN( PAD2 );

	if( OpenStatus.GS )
	{
		gsClose();
		OpenStatus.GS = false;
	}
}

void ReleasePlugins()
{
	if (!plugins_loaded) return;

	if ((GSplugin == NULL) || (PAD1plugin == NULL) || (PAD2plugin == NULL) ||
		(SPU2plugin == NULL) || (CDVDplugin == NULL) || (DEV9plugin == NULL) ||
		(USBplugin == NULL) || (FWplugin == NULL)) return;

	ShutdownPlugins();

	SysCloseLibrary(GSplugin);   GSplugin = NULL;
	SysCloseLibrary(PAD1plugin); PAD1plugin = NULL;
	SysCloseLibrary(PAD2plugin); PAD2plugin = NULL;
	SysCloseLibrary(SPU2plugin); SPU2plugin = NULL;
	SysCloseLibrary(CDVDplugin); CDVDplugin = NULL;
	SysCloseLibrary(DEV9plugin); DEV9plugin = NULL;
	SysCloseLibrary(USBplugin);  USBplugin = NULL;
	SysCloseLibrary(FWplugin);   FWplugin = NULL;
	
	plugins_loaded = false;
}

void PluginsResetGS()
{
	// PADs are tied to the GS window, so shut them down together with the GS.

	CLOSE_PLUGIN( PAD1 );
	CLOSE_PLUGIN( PAD2 );

	if( OpenStatus.GS )
	{
		gsClose();
		OpenStatus.GS = false;
	}

	GSshutdown();

	int ret = GSinit();
	if (ret != 0) { Msgbox::Alert("GSinit error: %d", params ret);  }
}
