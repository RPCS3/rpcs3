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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "Common.h"
#include "PsxCommon.h"
#include "GS.h"

#ifdef _WIN32
#pragma warning(disable:4244)
#endif

#define CheckErr(func) \
    err = SysLibError(); \
    if (err != NULL) { SysMessage (_("%s: Error loading %s: %s"), filename, func, err); return -1; }

#define LoadSym(dest, src, name, checkerr) \
    dest = (src) SysLoadSym(drv, name); if (checkerr == 1) CheckErr(name); \
    if (checkerr == 2) { err = SysLibError(); if (err != NULL) errval = 1; }

#define TestPS2Esyms(type) { \
	_PS2EgetLibVersion2 PS2EgetLibVersion2; \
	SysLoadSym(drv, "PS2EgetLibType"); CheckErr("PS2EgetLibType"); \
	PS2EgetLibVersion2 = (_PS2EgetLibVersion2) SysLoadSym(drv, "PS2EgetLibVersion2"); CheckErr("PS2EgetLibVersion2"); \
	SysLoadSym(drv, "PS2EgetLibName"); CheckErr("PS2EgetLibName"); \
	if( ((PS2EgetLibVersion2(PS2E_LT_##type) >> 16)&0xff) != PS2E_##type##_VERSION) { \
		SysMessage (_("Can't load '%s', wrong PS2E version (%x != %x)"), filename, (PS2EgetLibVersion2(PS2E_LT_##type) >> 16)&0xff, PS2E_##type##_VERSION); return -1; \
	} \
}

static char *err;
static int errval;

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
void CALLBACK GS_makeSnapshot(char *path) {}
void CALLBACK GS_irqCallback(void (*callback)()) {}
void CALLBACK GS_configure() {}
void CALLBACK GS_about() {}
long CALLBACK GS_test() { return 0; }

#define LoadGSsym1(dest, name) \
	LoadSym(GS##dest, _GS##dest, name, 1);

#define LoadGSsym0(dest, name) \
	LoadSym(GS##dest, _GS##dest, name, 0); \
	if (GS##dest == NULL) GS##dest = (_GS##dest) GS_##dest;

#define LoadGSsymN(dest, name) \
	LoadSym(GS##dest, _GS##dest, name, 0);

int LoadGSplugin(char *filename) {
	void *drv;

	GSplugin = SysLoadLibrary(filename);
	if (GSplugin == NULL) { SysMessage (_("Could Not Load GS Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = GSplugin;
	TestPS2Esyms(GS);
	LoadGSsym1(init,         "GSinit");
	LoadGSsym1(shutdown,     "GSshutdown");
	LoadGSsym1(open,         "GSopen");
	LoadGSsym1(close,        "GSclose");
	LoadGSsym1(gifTransfer1, "GSgifTransfer1");
	LoadGSsym1(gifTransfer2, "GSgifTransfer2");
	LoadGSsym1(gifTransfer3, "GSgifTransfer3");
	LoadGSsym1(readFIFO,     "GSreadFIFO");
    LoadGSsymN(getLastTag,   "GSgetLastTag");
	LoadGSsymN(readFIFO2,    "GSreadFIFO2"); // optional
	LoadGSsym1(vsync,        "GSvsync");

	LoadGSsym0(keyEvent,     "GSkeyEvent");
	LoadGSsymN(changeSaveState, "GSchangeSaveState");
	LoadGSsymN(gifSoftReset, "GSgifSoftReset");
	LoadGSsym0(makeSnapshot, "GSmakeSnapshot");
	LoadGSsym0(irqCallback,  "GSirqCallback");
	LoadGSsym0(printf,       "GSprintf");
	LoadGSsym1(setBaseMem,	"GSsetBaseMem");
	LoadGSsymN(setGameCRC,	"GSsetGameCRC");
	LoadGSsym1(reset,       "GSreset");
	LoadGSsym1(writeCSR,       "GSwriteCSR");
	LoadGSsymN(makeSnapshot2,"GSmakeSnapshot2");
	LoadGSsymN(getDriverInfo,"GSgetDriverInfo");

	LoadGSsymN(setFrameSkip, "GSsetFrameSkip");
    LoadGSsymN(setupRecording, "GSsetupRecording");

#ifdef _WIN32
	LoadGSsymN(setWindowInfo,"GSsetWindowInfo");
#endif
	LoadGSsym0(freeze,       "GSfreeze");
	LoadGSsym0(configure,    "GSconfigure");
	LoadGSsym0(about,        "GSabout");
	LoadGSsym0(test,         "GStest");

	return 0;
}

void *PAD1plugin;

void CALLBACK PAD1_configure() {}
void CALLBACK PAD1_about() {}
long CALLBACK PAD1_test() { return 0; }

#define LoadPAD1sym1(dest, name) \
	LoadSym(PAD1##dest, _PAD##dest, name, 1);

#define LoadPAD1sym0(dest, name) \
	LoadSym(PAD1##dest, _PAD##dest, name, 0); \
	if (PAD1##dest == NULL) PAD1##dest = (_PAD##dest) PAD1_##dest;

#define LoadPAD1symN(dest, name) \
	LoadSym(PAD1##dest, _PAD##dest, name, 0);

int LoadPAD1plugin(char *filename) {
	void *drv;

	PAD1plugin = SysLoadLibrary(filename);
	if (PAD1plugin == NULL) { SysMessage (_("Could Not Load PAD1 Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = PAD1plugin;
	TestPS2Esyms(PAD);
	LoadPAD1sym1(init,         "PADinit");
	LoadPAD1sym1(shutdown,     "PADshutdown");
	LoadPAD1sym1(open,         "PADopen");
	LoadPAD1sym1(close,        "PADclose");
	LoadPAD1sym1(keyEvent,     "PADkeyEvent");
	LoadPAD1sym1(startPoll,    "PADstartPoll");
	LoadPAD1sym1(poll,         "PADpoll");
	LoadPAD1sym1(query,        "PADquery");
    LoadPAD1symN(update,       "PADupdate");

	LoadPAD1symN(gsDriverInfo, "PADgsDriverInfo");
	LoadPAD1sym0(configure,    "PADconfigure");
	LoadPAD1sym0(about,        "PADabout");
	LoadPAD1sym0(test,         "PADtest");

	return 0;
}

void *PAD2plugin;

void CALLBACK PAD2_configure() {}
void CALLBACK PAD2_about() {}
long CALLBACK PAD2_test() { return 0; }

#define LoadPAD2sym1(dest, name) \
	LoadSym(PAD2##dest, _PAD##dest, name, 1);

#define LoadPAD2sym0(dest, name) \
	LoadSym(PAD2##dest, _PAD##dest, name, 0); \
	if (PAD2##dest == NULL) PAD2##dest = (_PAD##dest) PAD2_##dest;

#define LoadPAD2symN(dest, name) \
	LoadSym(PAD2##dest, _PAD##dest, name, 0);

int LoadPAD2plugin(char *filename) {
	void *drv;

	PAD2plugin = SysLoadLibrary(filename);
	if (PAD2plugin == NULL) { SysMessage (_("Could Not Load PAD2 Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = PAD2plugin;
	TestPS2Esyms(PAD);
	LoadPAD2sym1(init,         "PADinit");
	LoadPAD2sym1(shutdown,     "PADshutdown");
	LoadPAD2sym1(open,         "PADopen");
	LoadPAD2sym1(close,        "PADclose");
	LoadPAD2sym1(keyEvent,     "PADkeyEvent");
	LoadPAD2sym1(startPoll,    "PADstartPoll");
	LoadPAD2sym1(poll,         "PADpoll");
	LoadPAD2sym1(query,        "PADquery");
    LoadPAD2symN(update,       "PADupdate");

	LoadPAD2symN(gsDriverInfo, "PADgsDriverInfo");
	LoadPAD2sym0(configure,    "PADconfigure");
	LoadPAD2sym0(about,        "PADabout");
	LoadPAD2sym0(test,         "PADtest");

	return 0;
}

void *SPU2plugin;

s32  CALLBACK SPU2_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK SPU2_configure() {}
void CALLBACK SPU2_about() {}
s32  CALLBACK SPU2_test() { return 0; }

#define LoadSPU2sym1(dest, name) \
	LoadSym(SPU2##dest, _SPU2##dest, name, 1);

#define LoadSPU2sym0(dest, name) \
	LoadSym(SPU2##dest, _SPU2##dest, name, 0); \
	if (SPU2##dest == NULL) SPU2##dest = (_SPU2##dest) SPU2_##dest;

#define LoadSPU2symN(dest, name) \
	LoadSym(SPU2##dest, _SPU2##dest, name, 0);

int LoadSPU2plugin(char *filename) {
	void *drv;

	SPU2plugin = SysLoadLibrary(filename);
	if (SPU2plugin == NULL) { SysMessage (_("Could Not Load SPU2 Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = SPU2plugin;
	TestPS2Esyms(SPU2);
	LoadSPU2sym1(init,         "SPU2init");
	LoadSPU2sym1(shutdown,     "SPU2shutdown");
	LoadSPU2sym1(open,         "SPU2open");
	LoadSPU2sym1(close,        "SPU2close");
	LoadSPU2sym1(write,        "SPU2write");
	LoadSPU2sym1(read,         "SPU2read");
    LoadSPU2sym1(readDMA4Mem,  "SPU2readDMA4Mem");     
    LoadSPU2sym1(writeDMA4Mem, "SPU2writeDMA4Mem");   
	LoadSPU2sym1(interruptDMA4,"SPU2interruptDMA4");
    LoadSPU2sym1(readDMA7Mem,  "SPU2readDMA7Mem");     
    LoadSPU2sym1(writeDMA7Mem, "SPU2writeDMA7Mem");  
	LoadSPU2sym1(interruptDMA7,"SPU2interruptDMA7");
    LoadSPU2symN(setDMABaseAddr, "SPU2setDMABaseAddr");
	LoadSPU2sym1(ReadMemAddr,  "SPU2ReadMemAddr");
	LoadSPU2sym1(WriteMemAddr,  "SPU2WriteMemAddr");
	LoadSPU2sym1(irqCallback,  "SPU2irqCallback");

    LoadSPU2symN(setClockPtr, "SPU2setClockPtr");

	LoadSPU2symN(setupRecording, "SPU2setupRecording");

	LoadSPU2sym0(freeze,       "SPU2freeze");
	LoadSPU2sym0(configure,    "SPU2configure");
	LoadSPU2sym0(about,        "SPU2about");
	LoadSPU2sym0(test,         "SPU2test");
	LoadSPU2symN(async,        "SPU2async");

	return 0;
}

void *CDVDplugin;

void CALLBACK CDVD_configure() {}
void CALLBACK CDVD_about() {}
long CALLBACK CDVD_test() { return 0; }

#define LoadCDVDsym1(dest, name) \
	LoadSym(CDVD##dest, _CDVD##dest, name, 1);

#define LoadCDVDsym0(dest, name) \
	LoadSym(CDVD##dest, _CDVD##dest, name, 0); \
	if (CDVD##dest == NULL) CDVD##dest = (_CDVD##dest) CDVD_##dest;

#define LoadCDVDsymN(dest, name) \
	LoadSym(CDVD##dest, _CDVD##dest, name, 0); \

int LoadCDVDplugin(char *filename) {
	void *drv;

	CDVDplugin = SysLoadLibrary(filename);
	if (CDVDplugin == NULL) { SysMessage (_("Could Not Load CDVD Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = CDVDplugin;
	TestPS2Esyms(CDVD);
	LoadCDVDsym1(init,          "CDVDinit");
	LoadCDVDsym1(shutdown,      "CDVDshutdown");
	LoadCDVDsym1(open,          "CDVDopen");
	LoadCDVDsym1(close,         "CDVDclose");
	LoadCDVDsym1(readTrack,     "CDVDreadTrack");
	LoadCDVDsym1(getBuffer,     "CDVDgetBuffer");
	LoadCDVDsym1(readSubQ,      "CDVDreadSubQ");
	LoadCDVDsym1(getTN,         "CDVDgetTN");
	LoadCDVDsym1(getTD,         "CDVDgetTD");
	LoadCDVDsym1(getTOC,        "CDVDgetTOC");
	LoadCDVDsym1(getDiskType,   "CDVDgetDiskType");
	LoadCDVDsym1(getTrayStatus, "CDVDgetTrayStatus");
	LoadCDVDsym1(ctrlTrayOpen,  "CDVDctrlTrayOpen");
	LoadCDVDsym1(ctrlTrayClose, "CDVDctrlTrayClose");

	LoadCDVDsym0(configure,     "CDVDconfigure");
	LoadCDVDsym0(about,         "CDVDabout");
	LoadCDVDsym0(test,          "CDVDtest");
	LoadCDVDsymN(newDiskCB,     "CDVDnewDiskCB");

	return 0;
}

void *DEV9plugin;

s32  CALLBACK DEV9_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK DEV9_configure() {}
void CALLBACK DEV9_about() {}
long CALLBACK DEV9_test() { return 0; }

#define LoadDEV9sym1(dest, name) \
	LoadSym(DEV9##dest, _DEV9##dest, name, 1);

#define LoadDEV9sym0(dest, name) \
	LoadSym(DEV9##dest, _DEV9##dest, name, 0); \
	if (DEV9##dest == NULL) DEV9##dest = (_DEV9##dest) DEV9_##dest;

int LoadDEV9plugin(char *filename) {
	void *drv;

	DEV9plugin = SysLoadLibrary(filename);
	if (DEV9plugin == NULL) { SysMessage (_("Could Not Load DEV9 Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = DEV9plugin;
	TestPS2Esyms(DEV9);
	LoadDEV9sym1(init,          "DEV9init");
	LoadDEV9sym1(shutdown,      "DEV9shutdown");
	LoadDEV9sym1(open,          "DEV9open");
	LoadDEV9sym1(close,         "DEV9close");
	LoadDEV9sym1(read8,         "DEV9read8");
	LoadDEV9sym1(read16,        "DEV9read16");
	LoadDEV9sym1(read32,        "DEV9read32");
	LoadDEV9sym1(write8,        "DEV9write8");
	LoadDEV9sym1(write16,       "DEV9write16");
	LoadDEV9sym1(write32,       "DEV9write32");
	LoadDEV9sym1(readDMA8Mem,   "DEV9readDMA8Mem");
	LoadDEV9sym1(writeDMA8Mem,  "DEV9writeDMA8Mem");
	LoadDEV9sym1(irqCallback,   "DEV9irqCallback");
	LoadDEV9sym1(irqHandler,    "DEV9irqHandler");

	LoadDEV9sym0(freeze,        "DEV9freeze");
	LoadDEV9sym0(configure,     "DEV9configure");
	LoadDEV9sym0(about,         "DEV9about");
	LoadDEV9sym0(test,          "DEV9test");

	return 0;
}

void *USBplugin;

s32  CALLBACK USB_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK USB_configure() {}
void CALLBACK USB_about() {}
long CALLBACK USB_test() { return 0; }

#define LoadUSBsym1(dest, name) \
	LoadSym(USB##dest, _USB##dest, name, 1);

#define LoadUSBsym0(dest, name) \
	LoadSym(USB##dest, _USB##dest, name, 0); \
	if (USB##dest == NULL) USB##dest = (_USB##dest) USB_##dest;

#define LoadUSBsymX(dest, name) \
	LoadSym(USB##dest, _USB##dest, name, 0); \

int LoadUSBplugin(char *filename) {
	void *drv;

	USBplugin = SysLoadLibrary(filename);
	if (USBplugin == NULL) { SysMessage (_("Could Not Load USB Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = USBplugin;
	TestPS2Esyms(USB);
	LoadUSBsym1(init,          "USBinit");
	LoadUSBsym1(shutdown,      "USBshutdown");
	LoadUSBsym1(open,          "USBopen");
	LoadUSBsym1(close,         "USBclose");
	LoadUSBsym1(read8,         "USBread8");
	LoadUSBsym1(read16,        "USBread16");
	LoadUSBsym1(read32,        "USBread32");
	LoadUSBsym1(write8,        "USBwrite8");
	LoadUSBsym1(write16,       "USBwrite16");
	LoadUSBsym1(write32,       "USBwrite32");
	LoadUSBsym1(irqCallback,   "USBirqCallback");
	LoadUSBsym1(irqHandler,    "USBirqHandler");
	LoadUSBsym1(setRAM,        "USBsetRAM");
	
	LoadUSBsymX(async,         "USBasync");

	LoadUSBsym0(freeze,        "USBfreeze");
	LoadUSBsym0(configure,     "USBconfigure");
	LoadUSBsym0(about,         "USBabout");
	LoadUSBsym0(test,          "USBtest");

	return 0;
}
void *FWplugin;

s32  CALLBACK FW_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
void CALLBACK FW_configure() {}
void CALLBACK FW_about() {}
long CALLBACK FW_test() { return 0; }

#define LoadFWsym1(dest, name) \
	LoadSym(FW##dest, _FW##dest, name, 1);

#define LoadFWsym0(dest, name) \
	LoadSym(FW##dest, _FW##dest, name, 0); \
	if (FW##dest == NULL) FW##dest = (_FW##dest) FW_##dest;

int LoadFWplugin(char *filename) {
	void *drv;

	FWplugin = SysLoadLibrary(filename);
	if (FWplugin == NULL) { SysMessage (_("Could Not Load FW Plugin '%s': %s"), filename, SysLibError()); return -1; }
	drv = FWplugin;
	TestPS2Esyms(FW);
	LoadFWsym1(init,          "FWinit");
	LoadFWsym1(shutdown,      "FWshutdown");
	LoadFWsym1(open,          "FWopen");
	LoadFWsym1(close,         "FWclose");
	LoadFWsym1(read32,        "FWread32");
	LoadFWsym1(write32,       "FWwrite32");
	LoadFWsym1(irqCallback,   "FWirqCallback");

	LoadFWsym0(freeze,        "FWfreeze");
	LoadFWsym0(configure,     "FWconfigure");
	LoadFWsym0(about,         "FWabout");
	LoadFWsym0(test,          "FWtest");

	return 0;
}
static int loadp=0;

int InitPlugins() {
	int ret;

	if( GSsetBaseMem ) {

		if( CHECK_MULTIGS ) {
			extern u8 g_MTGSMem[];
			GSsetBaseMem(g_MTGSMem);
		}
		else {
			GSsetBaseMem(PS2MEM_GS);
		}
	}

	ret = GSinit();
	if (ret != 0) { SysMessage (_("GSinit error: %d"), ret); return -1; }
	ret = PAD1init(1);
	if (ret != 0) { SysMessage (_("PAD1init error: %d"), ret); return -1; }
	ret = PAD2init(2);
	if (ret != 0) { SysMessage (_("PAD2init error: %d"), ret); return -1; }
	ret = SPU2init();
	if (ret != 0) { SysMessage (_("SPU2init error: %d"), ret); return -1; }
	ret = CDVDinit();
	if (ret != 0) { SysMessage (_("CDVDinit error: %d"), ret); return -1; }
	ret = DEV9init();
	if (ret != 0) { SysMessage (_("DEV9init error: %d"), ret); return -1; }
	ret = USBinit();
	if (ret != 0) { SysMessage (_("USBinit error: %d"), ret); return -1; }
	ret = FWinit();
	if (ret != 0) { SysMessage (_("FWinit error: %d"), ret); return -1; }
	return 0;
}

void ShutdownPlugins() {
	GSshutdown();
	PAD1shutdown();
	PAD2shutdown();
	SPU2shutdown();
	CDVDshutdown();
	DEV9shutdown();
	USBshutdown();
    FWshutdown();
}

int LoadPlugins() {
	char Plugin[256];

	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.GS);
	if (LoadGSplugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.PAD1);
	if (LoadPAD1plugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.PAD2);
	if (LoadPAD2plugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.SPU2);
	if (LoadSPU2plugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.CDVD);
	if (LoadCDVDplugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.DEV9);
	if (LoadDEV9plugin(Plugin) == -1) return -1;
	sprintf(Plugin, "%s%s", Config.PluginsDir, Config.USB);
	if (LoadUSBplugin(Plugin) == -1) return -1;
    sprintf(Plugin, "%s%s", Config.PluginsDir, Config.FW);
	if (LoadFWplugin(Plugin) == -1) return -1;
	if (InitPlugins() == -1) return -1;

	loadp=1;

	return 0;
}

uptr pDsp;
static pluginsopened = 0;
extern void spu2DMA4Irq();
extern void spu2DMA7Irq();
extern void spu2Irq();

#if defined(_WIN32) && !defined(WIN32_PTHREADS)
extern HANDLE g_hGSOpen, g_hGSDone;
#else
extern pthread_mutex_t g_mutexGsThread;
extern pthread_cond_t g_condGsEvent;
extern sem_t g_semGsThread;
#endif

int OpenPlugins(const char* pTitleFilename) {
	GSdriverInfo info;
	int ret;

	if (loadp == 0) return -1;

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

	//first we need the data
	if (CDVDnewDiskCB) CDVDnewDiskCB(cdvdNewDiskCB);

    ret = CDVDopen(pTitleFilename);

	if (ret != 0) { SysMessage (_("Error Opening CDVD Plugin")); goto OpenError; }
	cdvdNewDiskCB();

	//video
	GSirqCallback(gsIrq);

	// make sure only call open once per instance
	if( !pluginsopened ) {
		if( CHECK_MULTIGS ) {
#if defined(_WIN32) && !defined(WIN32_PTHREADS)
			SetEvent(g_hGSOpen);
			WaitForSingleObject(g_hGSDone, INFINITE);
#else
			pthread_cond_signal(&g_condGsEvent);
            sem_wait(&g_semGsThread);
            pthread_mutex_lock(&g_mutexGsThread);
            pthread_mutex_unlock(&g_mutexGsThread);           SysPrintf("MTGS thread unlocked\n");
#endif
		}
		else {
			ret = GSopen((void *)&pDsp, "PCSX2", 0);
			if (ret != 0) { SysMessage (_("Error Opening GS Plugin")); goto OpenError; }
		}
	}

	//then the user input
	if (GSgetDriverInfo) {
		GSgetDriverInfo(&info);
		if (PAD1gsDriverInfo) PAD1gsDriverInfo(&info);
		if (PAD2gsDriverInfo) PAD2gsDriverInfo(&info);
	}
	ret = PAD1open((void *)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening PAD1 Plugin")); goto OpenError; }
	ret = PAD2open((void *)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening PAD2 Plugin")); goto OpenError; }

	//the sound

	SPU2irqCallback(spu2Irq,spu2DMA4Irq,spu2DMA7Irq);
    if( SPU2setDMABaseAddr != NULL )
        SPU2setDMABaseAddr((uptr)PSXM(0));

	if(SPU2setClockPtr != NULL)
		SPU2setClockPtr(&psxRegs.cycle);

	ret = SPU2open((void*)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening SPU2 Plugin")); goto OpenError; }

	//and last the dev9
	DEV9irqCallback(dev9Irq);
	dev9Handler = DEV9irqHandler();
	ret = DEV9open(&(psxRegs.pc)); //((void *)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening DEV9 Plugin")); goto OpenError; }

	USBirqCallback(usbIrq);
	usbHandler = USBirqHandler();
	USBsetRAM(psxM);
	ret = USBopen((void *)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening USB Plugin")); goto OpenError; }

	FWirqCallback(fwIrq);
	ret = FWopen((void *)&pDsp);
	if (ret != 0) { SysMessage (_("Error Opening FW Plugin")); goto OpenError; }

	pluginsopened = 1;
#ifdef __LINUX__
    chdir(file);
#endif
	return 0;

OpenError:
#ifdef __LINUX__
    chdir(file);
#endif

    return -1;
}

extern void gsWaitGS();

void ClosePlugins()
{
	gsWaitGS();

	CDVDclose();
	DEV9close();
	USBclose();
	FWclose();
	SPU2close();
	PAD1close();
	PAD2close();
}

void ResetPlugins() {
	gsWaitGS();

	ShutdownPlugins();
	InitPlugins();
}

void ReleasePlugins() {
	if (loadp == 0) return;

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
	loadp=0;
}
