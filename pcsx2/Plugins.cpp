/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "Common.h"
#include "PsxCommon.h"
#include "GS.h"

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
_GSsetupRecording GSsetupRecording;
_GSreset		   GSreset;
_GSwriteCSR		   GSwriteCSR;
_GSgetDriverInfo   GSgetDriverInfo;
#ifdef _WIN32
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
_CDVDinit          CDVDinit;
_CDVDopen          CDVDopen;
_CDVDclose         CDVDclose;
_CDVDshutdown      CDVDshutdown;
_CDVDreadTrack     CDVDreadTrack;
_CDVDgetBuffer     CDVDgetBuffer;
_CDVDreadSubQ      CDVDreadSubQ;
_CDVDgetTN         CDVDgetTN;
_CDVDgetTD         CDVDgetTD;
_CDVDgetTOC        CDVDgetTOC;
_CDVDgetDiskType   CDVDgetDiskType;
_CDVDgetTrayStatus CDVDgetTrayStatus;
_CDVDctrlTrayOpen  CDVDctrlTrayOpen;
_CDVDctrlTrayClose CDVDctrlTrayClose;

_CDVDconfigure     CDVDconfigure;
_CDVDtest          CDVDtest;
_CDVDabout         CDVDabout;
_CDVDnewDiskCB     CDVDnewDiskCB;

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

// for pad1/2
#define MapSymbolPAD(var,sym,name) MapSymbolVar(var##name,sym##name)
#define MapSymbolPAD_Fallback(var,sym,name) if((MapSymbolVarType(var##name,_##sym##name,sym##name))==NULL) var##name = var##_##name
#define MapSymbolPAD_Error(var,sym,name) MapSymbolVar_Error(var##name,sym##name)

#define TestPS2Esyms(type) if(_TestPS2Esyms(drv,PS2E_LT_##type,PS2E_##type##_VERSION,filename) < 0) return -1;

int _TestPS2Esyms(void* drv, int type, int expected_version, const string& filename)
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

//static const char *err;
//static int errval;

void *GSplugin;

void CALLBACK GS_printf(int timeout, char *fmt, ...) {
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	SysPrintf(msg);
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
	TestPS2Esyms(GS);
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

int LoadPAD1plugin(const string& filename) {
	void *drv;

	PAD1plugin = SysLoadLibrary(filename.c_str());
	if (PAD1plugin == NULL) { Msgbox::Alert("Could Not Load PAD1 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = PAD1plugin;
	TestPS2Esyms(PAD);
	MapSymbolPAD_Error(PAD1,PAD,init);
	MapSymbolPAD_Error(PAD1,PAD,shutdown);
	MapSymbolPAD_Error(PAD1,PAD,open);
	MapSymbolPAD_Error(PAD1,PAD,close);
	MapSymbolPAD_Error(PAD1,PAD,keyEvent);
	MapSymbolPAD_Error(PAD1,PAD,startPoll);
	MapSymbolPAD_Error(PAD1,PAD,poll);
	MapSymbolPAD_Error(PAD1,PAD,query);
    MapSymbolPAD(PAD1,PAD,update);

	MapSymbolPAD(PAD1,PAD,gsDriverInfo);
	MapSymbolPAD_Fallback(PAD1,PAD,configure);
	MapSymbolPAD_Fallback(PAD1,PAD,about);
	MapSymbolPAD_Fallback(PAD1,PAD,test);

	return 0;
}

void *PAD2plugin;

void CALLBACK PAD2_configure() {}
void CALLBACK PAD2_about() {}
s32  CALLBACK PAD2_test() { return 0; }

int LoadPAD2plugin(const string& filename) {
	void *drv;

	PAD2plugin = SysLoadLibrary(filename.c_str());
	if (PAD2plugin == NULL) { Msgbox::Alert("Could Not Load PAD2 Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = PAD2plugin;
	TestPS2Esyms(PAD);
	MapSymbolPAD_Error(PAD2,PAD,init);
	MapSymbolPAD_Error(PAD2,PAD,shutdown);
	MapSymbolPAD_Error(PAD2,PAD,open);
	MapSymbolPAD_Error(PAD2,PAD,close);
	MapSymbolPAD_Error(PAD2,PAD,keyEvent);
	MapSymbolPAD_Error(PAD2,PAD,startPoll);
	MapSymbolPAD_Error(PAD2,PAD,poll);
	MapSymbolPAD_Error(PAD2,PAD,query);
    MapSymbolPAD(PAD2,PAD,update);

	MapSymbolPAD(PAD2,PAD,gsDriverInfo);
	MapSymbolPAD_Fallback(PAD2,PAD,configure);
	MapSymbolPAD_Fallback(PAD2,PAD,about);
	MapSymbolPAD_Fallback(PAD2,PAD,test);

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
	TestPS2Esyms(SPU2);
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

int LoadCDVDplugin(const string& filename) {
	void *drv;

	CDVDplugin = SysLoadLibrary(filename.c_str());
	if (CDVDplugin == NULL) { Msgbox::Alert("Could Not Load CDVD Plugin '%hs': %s", params &filename, SysLibError()); return -1; }
	drv = CDVDplugin;
	TestPS2Esyms(CDVD);
	MapSymbol_Error(CDVDinit);
	MapSymbol_Error(CDVDshutdown);
	MapSymbol_Error(CDVDopen);
	MapSymbol_Error(CDVDclose);
	MapSymbol_Error(CDVDreadTrack);
	MapSymbol_Error(CDVDgetBuffer);
	MapSymbol_Error(CDVDreadSubQ);
	MapSymbol_Error(CDVDgetTN);
	MapSymbol_Error(CDVDgetTD);
	MapSymbol_Error(CDVDgetTOC);
	MapSymbol_Error(CDVDgetDiskType);
	MapSymbol_Error(CDVDgetTrayStatus);
	MapSymbol_Error(CDVDctrlTrayOpen);
	MapSymbol_Error(CDVDctrlTrayClose);

	MapSymbol_Fallback(CDVDconfigure,CDVD_configure);
	MapSymbol_Fallback(CDVDabout,CDVD_about);
	MapSymbol_Fallback(CDVDtest,CDVD_test);
	MapSymbol(CDVDnewDiskCB);

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
	TestPS2Esyms(DEV9);
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
	TestPS2Esyms(USB);
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
	TestPS2Esyms(FW);
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

static bool loadp=false;

int InitPlugins() {
	int ret;

	ret = GSinit();
	if (ret != 0) { Msgbox::Alert("GSinit error: %d", params ret); return -1; }
	ret = PAD1init(1);
	if (ret != 0) { Msgbox::Alert("PAD1init error: %d", params ret); return -1; }
	ret = PAD2init(2);
	if (ret != 0) { Msgbox::Alert("PAD2init error: %d", params ret); return -1; }
	ret = SPU2init();
	if (ret != 0) { Msgbox::Alert("SPU2init error: %d", params ret); return -1; }
	ret = CDVDinit();
	if (ret != 0) { Msgbox::Alert("CDVDinit error: %d", params ret); return -1; }
	ret = DEV9init();
	if (ret != 0) { Msgbox::Alert("DEV9init error: %d", params ret); return -1; }
	ret = USBinit();
	if (ret != 0) { Msgbox::Alert("USBinit error: %d", params ret); return -1; }
	ret = FWinit();
	if (ret != 0) { Msgbox::Alert("FWinit error: %d", params ret); return -1; }
	return 0;
}

void ShutdownPlugins()
{
	ClosePlugins();

	// GS is a special case: It needs closed first usually.
	// (the GS isn't always closed during emulation pauses)
	if( OpenStatus.GS )
	{
		gsClose();
		OpenStatus.GS = false;
	}

	if( GSshutdown != NULL )
		GSshutdown();
	
	if( PAD1shutdown != NULL )
		PAD1shutdown();
	if( PAD2shutdown != NULL )
		PAD2shutdown();

	if( SPU2shutdown != NULL )
		SPU2shutdown();

	if( CDVDshutdown != NULL )
		CDVDshutdown();

	if( DEV9shutdown != NULL )
		DEV9shutdown();

	if( USBshutdown != NULL )
		USBshutdown();

	if( FWshutdown != NULL )
		FWshutdown();
}

int LoadPlugins() {

	if( loadp ) return 0;
	string Plugin;

	Path::Combine( Plugin, Config.PluginsDir, Config.GS );
	if (LoadGSplugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.PAD1 );
	if (LoadPAD1plugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.PAD2);
	if (LoadPAD2plugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.SPU2);
	if (LoadSPU2plugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.CDVD);
	if (LoadCDVDplugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.DEV9);
	if (LoadDEV9plugin(Plugin) == -1) return -1;

	Path::Combine( Plugin, Config.PluginsDir, Config.USB);
	if (LoadUSBplugin(Plugin) == -1) return -1;

    Path::Combine( Plugin, Config.PluginsDir, Config.FW);
	if (LoadFWplugin(Plugin) == -1) return -1;

	if (InitPlugins() == -1) return -1;

	loadp=true;

	return 0;
}

uptr pDsp;
extern void spu2DMA4Irq();
extern void spu2DMA7Irq();
extern void spu2Irq();

int OpenPlugins(const char* pTitleFilename) {
	GSdriverInfo info;
	int ret;

	if ( !loadp )
		throw Exception::InvalidOperation( "OpenPlugins cannot be called while the plugin state is uninitialized." );

#ifndef _WIN32
    // change dir so that CDVD can find its config file
    char file[255], pNewTitle[255];
    getcwd(file, ARRAYSIZE(file));
    chdir(Config.PluginsDir);

    if( pTitleFilename != NULL && pTitleFilename[0] != '/' ) {
        // because we are changing the dir, we have to set a new title if it is a relative dir
        sprintf(pNewTitle, "%s/%s", file, pTitleFilename);
        pTitleFilename = pNewTitle;
    }
#endif

	if( !OpenStatus.CDVD )
	{
		//first we need the data
		if (CDVDnewDiskCB) CDVDnewDiskCB(cdvdNewDiskCB);

		ret = CDVDopen(pTitleFilename);

		if (ret != 0) { Msgbox::Alert("Error Opening CDVD Plugin"); goto OpenError; }
		OpenStatus.CDVD = true;
		cdvdNewDiskCB();
	}

	if( !OpenStatus.GS ) {
		ret = gsOpen();
		if (ret != 0) { Msgbox::Alert("Error Opening GS Plugin"); goto OpenError; }
		OpenStatus.GS = true;

		//then the user input
		if (GSgetDriverInfo) {
			GSgetDriverInfo(&info);
			if (PAD1gsDriverInfo) PAD1gsDriverInfo(&info);
			if (PAD2gsDriverInfo) PAD2gsDriverInfo(&info);
		}
	}

	if( !OpenStatus.PAD1 )
	{
		ret = PAD1open((void *)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening PAD1 Plugin"); goto OpenError; }
		OpenStatus.PAD1 = true;
	}

	if( !OpenStatus.PAD2 )
	{
		ret = PAD2open((void *)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening PAD2 Plugin"); goto OpenError; }
		OpenStatus.PAD2 = true;
	}

	//the sound

	if( !OpenStatus.SPU2 )
	{
		SPU2irqCallback(spu2Irq,spu2DMA4Irq,spu2DMA7Irq);
		if( SPU2setDMABaseAddr != NULL )
			SPU2setDMABaseAddr((uptr)psxM);

		if(SPU2setClockPtr != NULL)
			SPU2setClockPtr(&psxRegs.cycle);

		ret = SPU2open((void*)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening SPU2 Plugin"); goto OpenError; }
		OpenStatus.SPU2 = true;
	}

	//and last the dev9
	if( !OpenStatus.DEV9 )
	{
		DEV9irqCallback(dev9Irq);
		dev9Handler = DEV9irqHandler();
		ret = DEV9open(&psxRegs.pc); //((void *)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening DEV9 Plugin"); goto OpenError; }
		OpenStatus.DEV9 = true;
	}

	if( !OpenStatus.USB )
	{
		USBirqCallback(usbIrq);
		usbHandler = USBirqHandler();
		USBsetRAM(psxM);
		ret = USBopen((void *)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening USB Plugin"); goto OpenError; }
		OpenStatus.USB = true;
	}

	if( !OpenStatus.FW )
	{
		FWirqCallback(fwIrq);
		ret = FWopen((void *)&pDsp);
		if (ret != 0) { Msgbox::Alert("Error Opening FW Plugin"); goto OpenError; }
		OpenStatus.FW = true;
	}

#ifdef __LINUX__
    chdir(file);
#endif
	return 0;

OpenError:
	ClosePlugins();
#ifdef __LINUX__
    chdir(file);
#endif

    return -1;
}


#define CLOSE_PLUGIN( name ) \
	if( OpenStatus.name ) { \
		name##close(); \
		OpenStatus.name = false; \
	}


void ClosePlugins()
{
	// GS plugin is special and is not always closed during emulation pauses.
	// (that's because the GS is the most complicated plugin and to close it would
	// require we save the GS state -- plus, Gsdx doesn't really support being
	// closed)

	if( OpenStatus.GS )
		mtgsWaitGS();

	CLOSE_PLUGIN( PAD1 );
	CLOSE_PLUGIN( PAD2 );

	CLOSE_PLUGIN( CDVD );
	CLOSE_PLUGIN( DEV9 );
	CLOSE_PLUGIN( USB );
	CLOSE_PLUGIN( FW );
	CLOSE_PLUGIN( SPU2 );
}

void ResetPlugins()
{
	mtgsWaitGS();

	ShutdownPlugins();
	InitPlugins();
}

void ReleasePlugins()
{
	if (!loadp) return;

	if (GSplugin   == NULL || PAD1plugin == NULL || PAD2plugin == NULL ||
		SPU2plugin == NULL || CDVDplugin == NULL || DEV9plugin == NULL ||
		USBplugin  == NULL || FWplugin == NULL) return;

	ShutdownPlugins();

	SysCloseLibrary(GSplugin);   GSplugin = NULL;
	SysCloseLibrary(PAD1plugin); PAD1plugin = NULL;
	SysCloseLibrary(PAD2plugin); PAD2plugin = NULL;
	SysCloseLibrary(SPU2plugin); SPU2plugin = NULL;
	SysCloseLibrary(CDVDplugin); CDVDplugin = NULL;
	SysCloseLibrary(DEV9plugin); DEV9plugin = NULL;
	SysCloseLibrary(USBplugin);  USBplugin = NULL;
	SysCloseLibrary(FWplugin);   FWplugin = NULL;
	loadp = false;
}

void PluginsResetGS()
{
	CLOSE_PLUGIN( PAD1 );
	CLOSE_PLUGIN( PAD2 );

	if( OpenStatus.GS )
	{
		gsClose();
		OpenStatus.GS = false;
	}

	if( OpenStatus.PAD1 )
	{
		PAD1close();
	}

	GSshutdown();

	int ret = GSinit();
	if (ret != 0) { Msgbox::Alert("GSinit error: %d", params ret);  }
}
