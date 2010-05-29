/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "IopCommon.h"

#include <wx/dir.h>
#include <wx/file.h>

#include "GS.h"
#include "HostGui.h"
#include "CDVD/CDVDisoReader.h"

#include "Utilities/ScopedPtr.h"

#if _MSC_VER
#	include "svnrev.h"
#endif

SysPluginBindings SysPlugins;

bool SysPluginBindings::McdIsPresent( uint port, uint slot )
{
	return !!Mcd->McdIsPresent( (PS2E_THISPTR) Mcd, port, slot );
}

void SysPluginBindings::McdGetSizeInfo( uint port, uint slot, PS2E_McdSizeInfo& outways )
{
	if( Mcd->McdGetSizeInfo )
		Mcd->McdGetSizeInfo( (PS2E_THISPTR) Mcd, port, slot, &outways );
}

void SysPluginBindings::McdRead( uint port, uint slot, u8 *dest, u32 adr, int size )
{
	Mcd->McdRead( (PS2E_THISPTR) Mcd, port, slot, dest, adr, size );
}

void SysPluginBindings::McdSave( uint port, uint slot, const u8 *src, u32 adr, int size )
{
	Mcd->McdSave( (PS2E_THISPTR) Mcd, port, slot, src, adr, size );
}

void SysPluginBindings::McdEraseBlock( uint port, uint slot, u32 adr )
{
	Mcd->McdEraseBlock( (PS2E_THISPTR) Mcd, port, slot, adr );
}

u64 SysPluginBindings::McdGetCRC( uint port, uint slot )
{
	return Mcd->McdGetCRC( (PS2E_THISPTR) Mcd, port, slot );
}


// ----------------------------------------------------------------------------
// Yay, order of this array shouldn't be important. :)
//
const PluginInfo tbl_PluginInfo[] =
{
	{ "GS",		PluginId_GS,	PS2E_LT_GS,		PS2E_GS_VERSION		},
	{ "PAD",	PluginId_PAD,	PS2E_LT_PAD,	PS2E_PAD_VERSION	},
	{ "SPU2",	PluginId_SPU2,	PS2E_LT_SPU2,	PS2E_SPU2_VERSION	},
	{ "CDVD",	PluginId_CDVD,	PS2E_LT_CDVD,	PS2E_CDVD_VERSION	},
	{ "USB",	PluginId_USB,	PS2E_LT_USB,	PS2E_USB_VERSION	},
	{ "FW",		PluginId_FW,	PS2E_LT_FW,		PS2E_FW_VERSION		},
	{ "DEV9",	PluginId_DEV9,	PS2E_LT_DEV9,	PS2E_DEV9_VERSION	},

	{ NULL },

	// See PluginEnums_t for details on the MemoryCard plugin hack.
	{ "Mcd",	PluginId_Mcd,	0,	0	},
};

typedef void CALLBACK VoidMethod();
typedef void CALLBACK vMeth();		// shorthand for VoidMethod

// ----------------------------------------------------------------------------
struct LegacyApi_CommonMethod
{
	const char*		MethodName;

	// fallback is used if the method is null.  If the method is null and fallback is null
	// also, the plugin is considered incomplete or invalid, and an error is generated.
	VoidMethod*	Fallback;

	// returns the method name as a wxString, converted from UTF8.
	wxString GetMethodName( PluginsEnum_t pid ) const
	{
		return tbl_PluginInfo[pid].GetShortname() + fromUTF8( MethodName );
	}
};

// ----------------------------------------------------------------------------
struct LegacyApi_ReqMethod
{
	const char*		MethodName;
	VoidMethod**	Dest;		// Target function where the binding is saved.

	// fallback is used if the method is null.  If the method is null and fallback is null
	// also, the plugin is considered incomplete or invalid, and an error is generated.
	VoidMethod*	Fallback;

	// returns the method name as a wxString, converted from UTF8.
	wxString GetMethodName( ) const
	{
		return fromUTF8( MethodName );
	}
};

// ----------------------------------------------------------------------------
struct LegacyApi_OptMethod
{
	const char*		MethodName;
	VoidMethod**	Dest;		// Target function where the binding is saved.

	// returns the method name as a wxString, converted from UTF8.
	wxString GetMethodName() const { return fromUTF8( MethodName ); }
};


static s32  CALLBACK fallback_freeze(int mode, freezeData *data)
{
	if( mode == FREEZE_SIZE ) data->size = 0;
	return 0;
}

static void CALLBACK fallback_keyEvent(keyEvent *ev) {}
static void CALLBACK fallback_setSettingsDir(const char* dir) {}
static void CALLBACK fallback_SetLogFolder(const char* dir) {}
static void CALLBACK fallback_configure() {}
static void CALLBACK fallback_about() {}
static s32  CALLBACK fallback_test() { return 0; }

_GSvsync           GSvsync;
_GSopen            GSopen;
_GSopen2           GSopen2;
_GSgifTransfer1    GSgifTransfer1;
_GSgifTransfer2    GSgifTransfer2;
_GSgifTransfer3    GSgifTransfer3;
_GSgifSoftReset    GSgifSoftReset;
_GSreadFIFO        GSreadFIFO;
_GSreadFIFO2       GSreadFIFO2;
_GSchangeSaveState GSchangeSaveState;
_GSgetTitleInfo    GSgetTitleInfo;
_GSmakeSnapshot	   GSmakeSnapshot;
_GSmakeSnapshot2   GSmakeSnapshot2;
_GSirqCallback 	   GSirqCallback;
_GSprintf      	   GSprintf;
_GSsetBaseMem		GSsetBaseMem;
_GSsetGameCRC		GSsetGameCRC;
_GSsetFrameSkip		GSsetFrameSkip;
_GSsetVsync			GSsetVsync;
_GSsetExclusive		GSsetExclusive;
_GSsetupRecording	GSsetupRecording;
_GSreset			GSreset;
_GSwriteCSR			GSwriteCSR;

static void CALLBACK GS_makeSnapshot(const char *path) {}
static void CALLBACK GS_setGameCRC(u32 crc, int gameopts) {}
static void CALLBACK GS_irqCallback(void (*callback)()) {}
static void CALLBACK GS_setFrameSkip(int frameskip) {}
static void CALLBACK GS_setVsync(int enabled) {}
static void CALLBACK GS_setExclusive(int isExcl) {}
static void CALLBACK GS_changeSaveState( int, const char* filename ) {}
static void CALLBACK GS_printf(int timeout, char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	Console.WriteLn(msg);
}

void CALLBACK GS_getTitleInfo( char dest[128] )
{
	dest[0] = 'G';
	dest[1] = 'S';
	dest[2] = 0;
}


// PAD
_PADinit           PADinit;
_PADopen           PADopen;
_PADstartPoll      PADstartPoll;
_PADpoll           PADpoll;
_PADquery          PADquery;
_PADupdate         PADupdate;
_PADkeyEvent       PADkeyEvent;
_PADsetSlot        PADsetSlot;
_PADqueryMtap      PADqueryMtap;

static void PAD_update( u32 padslot ) { }

// SPU2
_SPU2open          SPU2open;
_SPU2write         SPU2write;
_SPU2read          SPU2read;
#ifdef ENABLE_NEW_IOPDMA_SPU2
_SPU2dmaRead	   SPU2dmaRead;
_SPU2dmaWrite      SPU2dmaWrite;
_SPU2dmaInterrupt  SPU2dmaInterrupt;
#else
_SPU2readDMA4Mem   SPU2readDMA4Mem;
_SPU2writeDMA4Mem  SPU2writeDMA4Mem;
_SPU2interruptDMA4 SPU2interruptDMA4;
_SPU2readDMA7Mem   SPU2readDMA7Mem;
_SPU2writeDMA7Mem  SPU2writeDMA7Mem;
_SPU2setDMABaseAddr SPU2setDMABaseAddr;
_SPU2interruptDMA7 SPU2interruptDMA7;
_SPU2ReadMemAddr   SPU2ReadMemAddr;
_SPU2WriteMemAddr   SPU2WriteMemAddr;
#endif
_SPU2setupRecording SPU2setupRecording;
_SPU2irqCallback   SPU2irqCallback;

_SPU2setClockPtr   SPU2setClockPtr;
_SPU2async         SPU2async;


// DEV9
_DEV9open          DEV9open;
_DEV9read8         DEV9read8;
_DEV9read16        DEV9read16;
_DEV9read32        DEV9read32;
_DEV9write8        DEV9write8;
_DEV9write16       DEV9write16;
_DEV9write32       DEV9write32;
#ifdef ENABLE_NEW_IOPDMA_DEV9
_DEV9dmaRead       DEV9dmaRead;
_DEV9dmaWrite      DEV9dmaWrite;
_DEV9dmaInterrupt  DEV9dmaInterrupt;
#else
_DEV9readDMA8Mem   DEV9readDMA8Mem;
_DEV9writeDMA8Mem  DEV9writeDMA8Mem;
#endif
_DEV9irqCallback   DEV9irqCallback;
_DEV9irqHandler    DEV9irqHandler;

// USB
_USBopen           USBopen;
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

// FW
_FWopen            FWopen;
_FWread32          FWread32;
_FWwrite32         FWwrite32;
_FWirqCallback     FWirqCallback;

DEV9handler dev9Handler;
USBhandler usbHandler;
uptr pDsp;

static s32 CALLBACK _hack_PADinit()
{
	return PADinit( 3 );
}

// ----------------------------------------------------------------------------
// Important: Contents of this array must match the order of the contents of the
// LegacyPluginAPI_Common structure defined in Plugins.h.
//
static const LegacyApi_CommonMethod s_MethMessCommon[] =
{
	{	"init",				NULL	},
	{	"close",			NULL	},
	{	"shutdown",			NULL	},

	{	"keyEvent",			(vMeth*)fallback_keyEvent },
	{	"setSettingsDir",	(vMeth*)fallback_setSettingsDir },
	{	"SetLogFolder",	    (vMeth*)fallback_SetLogFolder },

	{	"freeze",			(vMeth*)fallback_freeze	},
	{	"test",				(vMeth*)fallback_test },
	{	"configure",		fallback_configure	},
	{	"about",			fallback_about	},

	{ NULL }

};

// ----------------------------------------------------------------------------
//  GS Mess!
// ----------------------------------------------------------------------------
static const LegacyApi_ReqMethod s_MethMessReq_GS[] =
{
	{	"GSopen",			(vMeth**)&GSopen,			NULL	},
	{	"GSvsync",			(vMeth**)&GSvsync,			NULL	},
	{	"GSgifTransfer1",	(vMeth**)&GSgifTransfer1,	NULL	},
	{	"GSgifTransfer2",	(vMeth**)&GSgifTransfer2,	NULL	},
	{	"GSgifTransfer3",	(vMeth**)&GSgifTransfer3,	NULL	},
	{	"GSreadFIFO2",		(vMeth**)&GSreadFIFO2,		NULL	},

	{	"GSmakeSnapshot",	(vMeth**)&GSmakeSnapshot,	(vMeth*)GS_makeSnapshot },
	{	"GSirqCallback",	(vMeth**)&GSirqCallback,	(vMeth*)GS_irqCallback },
	{	"GSprintf",			(vMeth**)&GSprintf,			(vMeth*)GS_printf },
	{	"GSsetBaseMem",		(vMeth**)&GSsetBaseMem,		NULL	},
	{	"GSwriteCSR",		(vMeth**)&GSwriteCSR,		NULL	},
	{	"GSsetGameCRC",		(vMeth**)&GSsetGameCRC,		(vMeth*)GS_setGameCRC },

	{	"GSsetFrameSkip",	(vMeth**)&GSsetFrameSkip,	(vMeth*)GS_setFrameSkip	},
	{	"GSsetVsync",		(vMeth**)&GSsetVsync,		(vMeth*)GS_setVsync	},
	{	"GSsetExclusive",	(vMeth**)&GSsetExclusive,	(vMeth*)GS_setExclusive	},
	{	"GSchangeSaveState",(vMeth**)&GSchangeSaveState,(vMeth*)GS_changeSaveState },
	{	"GSgetTitleInfo",	(vMeth**)&GSgetTitleInfo,	(vMeth*)GS_getTitleInfo },
	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_GS[] =
{
	{	"GSopen2",			(vMeth**)&GSopen2			},
	{	"GSreset",			(vMeth**)&GSreset			},
	{	"GSsetupRecording",	(vMeth**)&GSsetupRecording	},
	{	"GSmakeSnapshot2",	(vMeth**)&GSmakeSnapshot2	},
	{	"GSgifSoftReset",	(vMeth**)&GSgifSoftReset	},
	{	"GSreadFIFO",		(vMeth**)&GSreadFIFO		},
	{ NULL }
};

// ----------------------------------------------------------------------------
//  PAD Mess!
// ----------------------------------------------------------------------------
static s32 CALLBACK PAD_queryMtap( u8 slot ) { return 0; }
static s32 CALLBACK PAD_setSlot(u8 port, u8 slot) { return 0; }

static const LegacyApi_ReqMethod s_MethMessReq_PAD[] =
{
	{	"PADopen",			(vMeth**)&PADopen,		NULL },
	{	"PADstartPoll",		(vMeth**)&PADstartPoll,	NULL },
	{	"PADpoll",			(vMeth**)&PADpoll,		NULL },
	{	"PADquery",			(vMeth**)&PADquery,		NULL },
	{	"PADkeyEvent",		(vMeth**)&PADkeyEvent,	NULL },

	// fixme - Following functions are new as of some revison post-0.9.6, and
	// are for multitap support only.  They should either be optional or offer
	// NOP fallbacks, to allow older plugins to retain functionality.
	{	"PADsetSlot",		(vMeth**)&PADsetSlot,	(vMeth*)PAD_setSlot },
	{	"PADqueryMtap",		(vMeth**)&PADqueryMtap,	(vMeth*)PAD_queryMtap },
	{ NULL },
};

static const LegacyApi_OptMethod s_MethMessOpt_PAD[] =
{
	{	"PADupdate",		(vMeth**)&PADupdate },
	{ NULL },
};

// ----------------------------------------------------------------------------
//  CDVD Mess!
// ----------------------------------------------------------------------------
void CALLBACK CDVD_newDiskCB(void (*callback)()) {}

extern int lastReadSize, lastLSN;
static s32 CALLBACK CDVD_getBuffer2(u8* buffer)
{
	// TEMP: until I fix all the plugins to use this function style
	u8* pb = CDVD->getBuffer();
	if(pb == NULL) return -2;

	memcpy_fast( buffer, pb, lastReadSize );
	return 0;
}

static s32 CALLBACK CDVD_readSector(u8* buffer, u32 lsn, int mode)
{
	if(CDVD->readTrack(lsn,mode) < 0)
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

	lastLSN = lsn;
	return CDVD->getBuffer2(buffer);
}

static s32 CALLBACK CDVD_getDualInfo(s32* dualType, u32* layer1Start)
{
	u8 toc[2064];

	// if error getting toc, settle for single layer disc ;)
	if(CDVD->getTOC(toc))
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

CDVD_API CDVDapi_Plugin =
{
	// All of these are filled by the plugin manager
	NULL
};

CDVD_API* CDVD			= NULL;

static const LegacyApi_ReqMethod s_MethMessReq_CDVD[] =
{
	{	"CDVDopen",			(vMeth**)&CDVDapi_Plugin.open,			NULL },
	{	"CDVDclose",		(vMeth**)&CDVDapi_Plugin.close,			NULL },
	{	"CDVDreadTrack",	(vMeth**)&CDVDapi_Plugin.readTrack,		NULL },
	{	"CDVDgetBuffer",	(vMeth**)&CDVDapi_Plugin.getBuffer,		NULL },
	{	"CDVDreadSubQ",		(vMeth**)&CDVDapi_Plugin.readSubQ,		NULL },
	{	"CDVDgetTN",		(vMeth**)&CDVDapi_Plugin.getTN,			NULL },
	{	"CDVDgetTD",		(vMeth**)&CDVDapi_Plugin.getTD,			NULL },
	{	"CDVDgetTOC",		(vMeth**)&CDVDapi_Plugin.getTOC,		NULL },
	{	"CDVDgetDiskType",	(vMeth**)&CDVDapi_Plugin.getDiskType,	NULL },
	{	"CDVDgetTrayStatus",(vMeth**)&CDVDapi_Plugin.getTrayStatus,	NULL },
	{	"CDVDctrlTrayOpen",	(vMeth**)&CDVDapi_Plugin.ctrlTrayOpen,	NULL },
	{	"CDVDctrlTrayClose",(vMeth**)&CDVDapi_Plugin.ctrlTrayClose,	NULL },
	{	"CDVDnewDiskCB",	(vMeth**)&CDVDapi_Plugin.newDiskCB,		(vMeth*)CDVD_newDiskCB },

	{	"CDVDreadSector",	(vMeth**)&CDVDapi_Plugin.readSector,	(vMeth*)CDVD_readSector },
	{	"CDVDgetBuffer2",	(vMeth**)&CDVDapi_Plugin.getBuffer2,	(vMeth*)CDVD_getBuffer2 },
	{	"CDVDgetDualInfo",	(vMeth**)&CDVDapi_Plugin.getDualInfo,	(vMeth*)CDVD_getDualInfo },

	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_CDVD[] =
{
	{ NULL }
};

// ----------------------------------------------------------------------------
//  SPU2 Mess!
// ----------------------------------------------------------------------------
static const LegacyApi_ReqMethod s_MethMessReq_SPU2[] =
{
	{	"SPU2open",				(vMeth**)&SPU2open,			NULL },
	{	"SPU2write",			(vMeth**)&SPU2write,		NULL },
	{	"SPU2read",				(vMeth**)&SPU2read,			NULL },
#ifdef ENABLE_NEW_IOPDMA_SPU2
	{	"SPU2dmaRead",			(vMeth**)&SPU2dmaRead,		NULL },
	{	"SPU2dmaWrite",			(vMeth**)&SPU2dmaWrite,		NULL },
	{	"SPU2dmaInterrupt",		(vMeth**)&SPU2dmaInterrupt, NULL },
#else
	{	"SPU2readDMA4Mem",		(vMeth**)&SPU2readDMA4Mem,	NULL },
	{	"SPU2readDMA7Mem",		(vMeth**)&SPU2readDMA7Mem,	NULL },
	{	"SPU2writeDMA4Mem",		(vMeth**)&SPU2writeDMA4Mem,	NULL },
	{	"SPU2writeDMA7Mem",		(vMeth**)&SPU2writeDMA7Mem,	NULL },
	{	"SPU2interruptDMA4",	(vMeth**)&SPU2interruptDMA4,NULL },
	{	"SPU2interruptDMA7",	(vMeth**)&SPU2interruptDMA7,NULL },
	{	"SPU2ReadMemAddr",		(vMeth**)&SPU2ReadMemAddr,	NULL },
#endif
	{	"SPU2irqCallback",		(vMeth**)&SPU2irqCallback,	NULL },

	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_SPU2[] =
{
	{	"SPU2setClockPtr",		(vMeth**)&SPU2setClockPtr	},
	{	"SPU2async",			(vMeth**)&SPU2async			},
#ifndef ENABLE_NEW_IOPDMA_SPU2
	{	"SPU2WriteMemAddr",		(vMeth**)&SPU2WriteMemAddr	},
	{	"SPU2setDMABaseAddr",	(vMeth**)&SPU2setDMABaseAddr},
#endif
	{	"SPU2setupRecording",	(vMeth**)&SPU2setupRecording},

	{ NULL }
};

// ----------------------------------------------------------------------------
//  DEV9 Mess!
// ----------------------------------------------------------------------------
static const LegacyApi_ReqMethod s_MethMessReq_DEV9[] =
{
	{	"DEV9open",			(vMeth**)&DEV9open,			NULL },
	{	"DEV9read8",		(vMeth**)&DEV9read8,		NULL },
	{	"DEV9read16",		(vMeth**)&DEV9read16,		NULL },
	{	"DEV9read32",		(vMeth**)&DEV9read32,		NULL },
	{	"DEV9write8",		(vMeth**)&DEV9write8,		NULL },
	{	"DEV9write16",		(vMeth**)&DEV9write16,		NULL },
	{	"DEV9write32",		(vMeth**)&DEV9write32,		NULL },
#ifdef ENABLE_NEW_IOPDMA_DEV9
	{	"DEV9dmaRead",		(vMeth**)&DEV9dmaRead,	NULL },
	{	"DEV9dmaWrite",		(vMeth**)&DEV9dmaWrite,	NULL },
	{	"DEV9dmaInterrupt",	(vMeth**)&DEV9dmaInterrupt,	NULL },
#else
	{	"DEV9readDMA8Mem",	(vMeth**)&DEV9readDMA8Mem,	NULL },
	{	"DEV9writeDMA8Mem",	(vMeth**)&DEV9writeDMA8Mem,	NULL },
#endif
	{	"DEV9irqCallback",	(vMeth**)&DEV9irqCallback,	NULL },
	{	"DEV9irqHandler",	(vMeth**)&DEV9irqHandler,	NULL },

	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_DEV9[] =
{
	{ NULL }
};

// ----------------------------------------------------------------------------
//  USB Mess!
// ----------------------------------------------------------------------------
static const LegacyApi_ReqMethod s_MethMessReq_USB[] =
{
	{	"USBopen",			(vMeth**)&USBopen,			NULL },
	{	"USBread8",			(vMeth**)&USBread8,			NULL },
	{	"USBread16",		(vMeth**)&USBread16,		NULL },
	{	"USBread32",		(vMeth**)&USBread32,		NULL },
	{	"USBwrite8",		(vMeth**)&USBwrite8,		NULL },
	{	"USBwrite16",		(vMeth**)&USBwrite16,		NULL },
	{	"USBwrite32",		(vMeth**)&USBwrite32,		NULL },
	{	"USBirqCallback",	(vMeth**)&USBirqCallback,	NULL },
	{	"USBirqHandler",	(vMeth**)&USBirqHandler,	NULL },
	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_USB[] =
{
	{	"USBasync",		(vMeth**)&USBasync },
	{ NULL }
};

// ----------------------------------------------------------------------------
//  FW Mess!
// ----------------------------------------------------------------------------
static const LegacyApi_ReqMethod s_MethMessReq_FW[] =
{
	{	"FWopen",			(vMeth**)&FWopen,			NULL },
	{	"FWread32",			(vMeth**)&FWread32,			NULL },
	{	"FWwrite32",		(vMeth**)&FWwrite32,		NULL },
	{	"FWirqCallback",	(vMeth**)&FWirqCallback,	NULL },
	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_FW[] =
{
	{ NULL }
};

static const LegacyApi_ReqMethod* const s_MethMessReq[] =
{
	s_MethMessReq_GS,
	s_MethMessReq_PAD,
	s_MethMessReq_SPU2,
	s_MethMessReq_CDVD,
	s_MethMessReq_USB,
	s_MethMessReq_FW,
	s_MethMessReq_DEV9
};

static const LegacyApi_OptMethod* const s_MethMessOpt[] =
{
	s_MethMessOpt_GS,
	s_MethMessOpt_PAD,
	s_MethMessOpt_SPU2,
	s_MethMessOpt_CDVD,
	s_MethMessOpt_USB,
	s_MethMessOpt_FW,
	s_MethMessOpt_DEV9
};

PluginManager *g_plugins = NULL;

// ---------------------------------------------------------------------------------
//       Plugin-related Exception Implementations
// ---------------------------------------------------------------------------------

Exception::PluginLoadError::PluginLoadError( PluginsEnum_t pid, const wxString& objname, const char* eng )
{
	BaseException::InitBaseEx( eng );
	StreamName = objname;
	PluginId = pid;
}

Exception::PluginLoadError::PluginLoadError( PluginsEnum_t pid, const wxString& objname,
	const wxString& eng_msg, const wxString& xlt_msg )
{
	BaseException::InitBaseEx( eng_msg, xlt_msg );
	StreamName = objname;
	PluginId = pid;
}

wxString Exception::PluginLoadError::FormatDiagnosticMessage() const
{
	return wxsFormat( m_message_diag, tbl_PluginInfo[PluginId].GetShortname().c_str() ) +
		L"\n\n" + StreamName;
}

wxString Exception::PluginLoadError::FormatDisplayMessage() const
{
	return wxsFormat( m_message_user, tbl_PluginInfo[PluginId].GetShortname().c_str() ) +
		L"\n\n" + StreamName;
}

wxString Exception::PluginError::FormatDiagnosticMessage() const
{
	return wxsFormat( m_message_diag, tbl_PluginInfo[PluginId].GetShortname().c_str() );
}

wxString Exception::PluginError::FormatDisplayMessage() const
{
	return wxsFormat( m_message_user, tbl_PluginInfo[PluginId].GetShortname().c_str() );
}

wxString Exception::FreezePluginFailure::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"%s plugin returned an error while saving the state.\n\n",
		tbl_PluginInfo[PluginId].shortname
	) + m_stacktrace;
}

wxString Exception::FreezePluginFailure::FormatDisplayMessage() const
{
	// [TODO]
	return m_message_user;
}

wxString Exception::ThawPluginFailure::FormatDiagnosticMessage() const
{
	return wxsFormat(
		L"%s plugin returned an error while loading the state.\n\n",
		tbl_PluginInfo[PluginId].shortname
	) + m_stacktrace;
}

wxString Exception::ThawPluginFailure::FormatDisplayMessage() const
{
	// [TODO]
	return m_message_user;
}

// --------------------------------------------------------------------------------------
//  PCSX2 Callbacks passed to Plugins
// --------------------------------------------------------------------------------------
// This is currently unimplemented, and should be provided by the AppHost (gui) rather
// than the EmuCore.  But as a quickhackfix until the new plugin API is fleshed out, this
// will suit our needs nicely. :)

static BOOL PS2E_CALLBACK pcsx2_GetInt( const char* name, int* dest )
{
	return FALSE;		// not implemented...
}

static BOOL PS2E_CALLBACK pcsx2_GetBoolean( const char* name, BOOL* result )
{
	return FALSE;		// not implemented...
}

static BOOL PS2E_CALLBACK pcsx2_GetString( const char* name, char* dest, int maxlen )
{
	return FALSE;		// not implemented...
}

static char* PS2E_CALLBACK pcsx2_GetStringAlloc( const char* name, void* (PS2E_CALLBACK* allocator)(int size) )
{
	return FALSE;		// not implemented...
}

static void PS2E_CALLBACK pcsx2_OSD_WriteLn( int icon, const char* msg )
{
	return;		// not implemented...
}

// ---------------------------------------------------------------------------------
//  PluginStatus_t Implementations
// ---------------------------------------------------------------------------------
PluginManager::PluginStatus_t::PluginStatus_t( PluginsEnum_t _pid, const wxString& srcfile )
	: Filename( srcfile )
{
	pid = _pid;

	IsInitialized	= false;
	IsOpened		= false;

	if( Filename.IsEmpty() )
		throw Exception::PluginInitError( pid, "Empty plugin filename." );

	if( !wxFile::Exists( Filename ) )
		throw Exception::PluginLoadError( pid, srcfile,
		wxLt("The configured %s plugin file was not found")
	);

	if( !Lib.Load( Filename ) )
		throw Exception::PluginLoadError( pid, Filename,
		wxLt("The configured %s plugin file is not a valid dynamic library")
	);


	// Try to enumerate the new v2.0 plugin interface first.
	// If that fails, fall back on the old style interface.

	//m_libs[i].GetSymbol( L"PS2E_InitAPI" );		// on the TODO list!

	
	// 2.0 API Failed; Enumerate the Old Stuff! -->

	_PS2EgetLibName		GetLibName		= (_PS2EgetLibName)		Lib.GetSymbol( L"PS2EgetLibName" );
	_PS2EgetLibVersion2	GetLibVersion2	= (_PS2EgetLibVersion2)	Lib.GetSymbol( L"PS2EgetLibVersion2" );
	_PS2EsetEmuVersion	SetEmuVersion	= (_PS2EsetEmuVersion)	Lib.GetSymbol( L"PS2EsetEmuVersion" );

	if( GetLibName == NULL || GetLibVersion2 == NULL )
		throw Exception::PluginLoadError( pid, Filename,
			L"\nMethod binding failure on GetLibName or GetLibVersion2.\n",
			_( "Configured plugin is not a PCSX2 plugin, or is for an older unsupported version of PCSX2." )
		);

	if( SetEmuVersion != NULL )
		SetEmuVersion( "PCSX2", (0ul << 24) | (9ul<<16) | (7ul<<8) | 0 );

	Name = fromUTF8( GetLibName() );
	int version = GetLibVersion2( tbl_PluginInfo[pid].typemask );
	Version.Printf( L"%d.%d.%d", (version>>8)&0xff, version&0xff, (version>>24)&0xff );


	// Bind Required Functions
	// (generate critical error if binding fails)

	BindCommon( pid );
	BindRequired( pid );
	BindOptional( pid );

	// Run Plugin's Functionality Test.
	// A lot of plugins don't bother to implement this function and return 0 (success)
	// regardless, but some do so let's go ahead and check it. I mean, we're supposed to. :)

	int testres = CommonBindings.Test();
	if( testres != 0 )
		throw Exception::PluginLoadError( pid, Filename,
			wxsFormat( L"Plugin Test failure, return code: %d", testres ),
			_( "The plugin reports that your hardware or software/drivers are not supported." )
		);
}

void PluginManager::PluginStatus_t::BindCommon( PluginsEnum_t pid )
{
	const LegacyApi_CommonMethod* current = s_MethMessCommon;
	VoidMethod** target = (VoidMethod**)&CommonBindings;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*target = (VoidMethod*)Lib.GetSymbol( current->GetMethodName( pid ) );

		if( *target == NULL )
			*target = current->Fallback;

		if( *target == NULL )
		{
			throw Exception::PluginLoadError( pid, Filename,
				wxsFormat( L"\nMethod binding failure on: %s\n", current->GetMethodName( pid ).c_str() ),
				_( "Configured plugin is not a PCSX2 plugin, or is for an older unsupported version of PCSX2." )
			);
		}

		target++;
		current++;
	}
}

void PluginManager::PluginStatus_t::BindRequired( PluginsEnum_t pid )
{
	const LegacyApi_ReqMethod* current = s_MethMessReq[pid];
	const wxDynamicLibrary& lib = Lib;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*(current->Dest) = (VoidMethod*)lib.GetSymbol( current->GetMethodName() );

		if( *(current->Dest) == NULL )
			*(current->Dest) = current->Fallback;

		if( *(current->Dest) == NULL )
		{
			throw Exception::PluginLoadError( pid, Filename,
				wxsFormat( L"\nMethod binding failure on: %s\n", current->GetMethodName().c_str() ),
				_( "Configured plugin is not a valid PCSX2 plugin, or is for an older unsupported version of PCSX2." )
			);
		}

		current++;
	}
}

void PluginManager::PluginStatus_t::BindOptional( PluginsEnum_t pid )
{
	const LegacyApi_OptMethod* current = s_MethMessOpt[pid];
	const wxDynamicLibrary& lib = Lib;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*(current->Dest) = (VoidMethod*)lib.GetSymbol( current->GetMethodName() );
		current++;
	}
}

// =====================================================================================
//  PluginManager Implementations
// =====================================================================================

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager() throw()
{
	try
	{
		Unload();
	}
	DESTRUCTOR_CATCHALL

	// All library unloading done automatically by wx.
}

void PluginManager::Load( PluginsEnum_t pid, const wxString& srcfile )
{
	ScopedLock lock( m_mtx_PluginStatus );
	pxAssume( (uint)pid < PluginId_Count );
	Console.Indent().WriteLn( L"Binding %s\t: %s ", tbl_PluginInfo[pid].GetShortname().c_str(), srcfile.c_str() );
	m_info[pid] = new PluginStatus_t( pid, srcfile );
}

void PluginManager::Load( const wxString (&folders)[PluginId_Count] )
{
	if( !NeedsLoad() ) return;

	wxDoNotLogInThisScope please;
	
	Console.WriteLn( Color_StrongBlue, "\nLoading plugins..." );

	ConsoleIndentScope indent;
	const PluginInfo* pi = tbl_PluginInfo; do
	{
		Load( pi->id, folders[pi->id] );
		pxYield( 2 );

	} while( ++pi, pi->shortname != NULL );
	indent.EndScope();

	CDVDapi_Plugin.newDiskCB( cdvdNewDiskCB );

	// Hack for PAD's stupid parameter passed on Init
	PADinit = (_PADinit)m_info[PluginId_PAD]->CommonBindings.Init;
	m_info[PluginId_PAD]->CommonBindings.Init = _hack_PADinit;

	Console.WriteLn( Color_StrongBlue, "Plugins loaded successfully.\n" );

	// HACK!  Manually bind the Internal MemoryCard plugin for now, until
	// we get things more completed in the new plugin api.

	static const PS2E_EmulatorInfo myself =
	{
		"PCSX2",

		{ 0, PCSX2_VersionHi, PCSX2_VersionLo, SVN_REV },

		x86caps.PhysicalCores,
		x86caps.LogicalCores,
		sizeof(wchar_t),

		0,0,0,0,0,0,

		pcsx2_GetInt,
		pcsx2_GetBoolean,
		pcsx2_GetString,
		pcsx2_GetStringAlloc,
		pcsx2_OSD_WriteLn
	};

	m_mcdPlugin = FileMcd_InitAPI( &myself );
	if( m_mcdPlugin == NULL )
	{
		// fixme: use plugin's GetLastError (not implemented yet!)
		throw Exception::PluginLoadError( PluginId_Mcd, wxEmptyString, "Internal Memorycard Plugin failed to load." );
	}

	SendLogFolder();
	SendSettingsFolder();
}

void PluginManager::Unload(PluginsEnum_t pid)
{
	ScopedLock lock( m_mtx_PluginStatus );
	pxAssume( (uint)pid < PluginId_Count );
	m_info[pid].Delete();
}

void PluginManager::Unload()
{
	if( NeedsShutdown() )
		Console.Warning( "(SysCorePlugins) Warning: Unloading plugins prior to shutdown!" );

	//Shutdown();

	if( !NeedsUnload() ) return;

	DbgCon.WriteLn( Color_StrongBlue, "Unloading plugins..." );

	for( int i=PluginId_Count-1; i>=0; --i )
		Unload( tbl_PluginInfo[i].id );

	DbgCon.WriteLn( Color_StrongBlue, "Plugins unloaded successfully." );
}

// Exceptions:
//   FileNotFound - Thrown if one of the configured plugins doesn't exist.
//   NotPcsxPlugin - Thrown if one of the configured plugins is an invalid or unsupported DLL

extern bool renderswitch;
extern void spu2DMA4Irq();
extern void spu2DMA7Irq();
extern void spu2Irq();

bool PluginManager::OpenPlugin_CDVD()
{
	return DoCDVDopen();
}

bool PluginManager::OpenPlugin_GS()
{
	GetMTGS().Resume();
	return true;
}

bool PluginManager::OpenPlugin_PAD()
{
	return !PADopen( (void*)&pDsp );
}

bool PluginManager::OpenPlugin_SPU2()
{
	if( SPU2open((void*)&pDsp) ) return false;

#ifdef ENABLE_NEW_IOPDMA_SPU2
	SPU2irqCallback( spu2Irq );
#else
	SPU2irqCallback( spu2Irq, spu2DMA4Irq, spu2DMA7Irq );
	if( SPU2setDMABaseAddr != NULL ) SPU2setDMABaseAddr((uptr)psxM);
#endif
	if( SPU2setClockPtr != NULL ) SPU2setClockPtr(&psxRegs.cycle);
	return true;
}

bool PluginManager::OpenPlugin_DEV9()
{
	dev9Handler = NULL;

	if( DEV9open( (void*)&pDsp ) ) return false;
	DEV9irqCallback( dev9Irq );
	dev9Handler = DEV9irqHandler();
	return true;
}

bool PluginManager::OpenPlugin_USB()
{
	usbHandler = NULL;

	if( USBopen((void*)&pDsp) ) return false;
	USBirqCallback( usbIrq );
	usbHandler = USBirqHandler();
	if( USBsetRAM != NULL )
		USBsetRAM(psxM);
	return true;
}

bool PluginManager::OpenPlugin_FW()
{
	if( FWopen((void*)&pDsp) ) return false;
	FWirqCallback( fwIrq );
	return true;
}

bool PluginManager::OpenPlugin_Mcd()
{
	ScopedLock lock( m_mtx_PluginStatus );

	// [TODO] Fix up and implement PS2E_SessionInfo here!!  (the currently NULL parameter)
	if( SysPlugins.Mcd )
		SysPlugins.Mcd->Base.EmuOpen( (PS2E_THISPTR) SysPlugins.Mcd, NULL );

	return true;
}

void PluginManager::Open( PluginsEnum_t pid )
{
	pxAssume( (uint)pid < PluginId_Count );
	if( IsOpen(pid) ) return;

	Console.Indent().WriteLn( "Opening %s", tbl_PluginInfo[pid].shortname );

	// Each Open needs to be called explicitly. >_<

	bool result = true;
	switch( pid )
	{
		case PluginId_GS:	result = OpenPlugin_GS();	break;
		case PluginId_PAD:	result = OpenPlugin_PAD();	break;
		case PluginId_CDVD:	result = OpenPlugin_CDVD();	break;
		case PluginId_SPU2:	result = OpenPlugin_SPU2();	break;
		case PluginId_USB:	result = OpenPlugin_USB();	break;
		case PluginId_FW:	result = OpenPlugin_FW();	break;
		case PluginId_DEV9:	result = OpenPlugin_DEV9();	break;

		jNO_DEFAULT;
	}
	if( !result )
		throw Exception::PluginOpenError( pid );

	ScopedLock lock( m_mtx_PluginStatus );
	if( m_info[pid] ) m_info[pid]->IsOpened = true;
}

void PluginManager::Open()
{
	Init();

	if( !NeedsOpen() ) return;		// Spam stopper:  returns before writing any logs. >_<

	Console.WriteLn( Color_StrongBlue, "Opening plugins..." );

	SendSettingsFolder();

	const PluginInfo* pi = tbl_PluginInfo; do {
		Open( pi->id );
		// If GS doesn't support GSopen2, need to wait until call to GSopen
		// returns to populate pDsp.  If it does, can initialize other plugins
		// at same time as GS, as long as GSopen2 does not subclass its window.
		if (pi->id == PluginId_GS && !GSopen2) GetMTGS().WaitForOpen();
	} while( ++pi, pi->shortname != NULL );

	if (GSopen2) GetMTGS().WaitForOpen();

	if( !AtomicExchange( m_mcdOpen, true ) )
	{
		DbgCon.Indent().WriteLn( "Opening Memorycards");
		OpenPlugin_Mcd();
	}

	Console.WriteLn( Color_StrongBlue, "Plugins opened successfully." );
}

void PluginManager::_generalclose( PluginsEnum_t pid )
{
	ScopedLock lock( m_mtx_PluginStatus );
	if( m_info[pid] ) m_info[pid]->CommonBindings.Close();
}

void PluginManager::ClosePlugin_GS()
{
	// old-skool: force-close PAD before GS, because the PAD depends on the GS window.

	if( GetMTGS().IsSelf() )
		_generalclose( PluginId_GS );
	else
	{
		if( !GSopen2 ) Close( PluginId_PAD );
		GetMTGS().Suspend();
	}
}

void PluginManager::ClosePlugin_CDVD()
{
	DoCDVDclose();
}

void PluginManager::ClosePlugin_PAD()
{
	_generalclose( PluginId_PAD );
}

void PluginManager::ClosePlugin_SPU2()
{
	_generalclose( PluginId_SPU2 );
}

void PluginManager::ClosePlugin_DEV9()
{
	_generalclose( PluginId_DEV9 );
}

void PluginManager::ClosePlugin_USB()
{
	_generalclose( PluginId_USB );
}

void PluginManager::ClosePlugin_FW()
{
	_generalclose( PluginId_FW );
}

void PluginManager::ClosePlugin_Mcd()
{
	ScopedLock lock( m_mtx_PluginStatus );
	if( SysPlugins.Mcd ) SysPlugins.Mcd->Base.EmuClose( (PS2E_THISPTR) SysPlugins.Mcd );
}

void PluginManager::Close( PluginsEnum_t pid )
{
	pxAssume( (uint)pid < PluginId_Count );

	if( !IsOpen(pid) ) return;
	
	if( !GetMTGS().IsSelf() )		// stop the spam!
		Console.Indent().WriteLn( "Closing %s", tbl_PluginInfo[pid].shortname );

	switch( pid )
	{
		case PluginId_GS:	ClosePlugin_GS();	break;
		case PluginId_PAD:	ClosePlugin_PAD();	break;
		case PluginId_CDVD:	ClosePlugin_CDVD();	break;
		case PluginId_SPU2:	ClosePlugin_SPU2();	break;
		case PluginId_USB:	ClosePlugin_USB();	break;
		case PluginId_FW:	ClosePlugin_FW();	break;
		case PluginId_DEV9:	ClosePlugin_DEV9();	break;
		case PluginId_Mcd:	ClosePlugin_Mcd();	break;
		
		jNO_DEFAULT;
	}

	ScopedLock lock( m_mtx_PluginStatus );
	if( m_info[pid] ) m_info[pid]->IsOpened = false;
}

void PluginManager::Close()
{
	if( !NeedsClose() ) return;	// Spam stopper; returns before writing any logs. >_<

	// Close plugins in reverse order of the initialization procedure, which
	// ensures the GS gets closed last.

	DbgCon.WriteLn( Color_StrongBlue, "Closing plugins..." );

	if( AtomicExchange( m_mcdOpen, false ) )
	{
		DbgCon.Indent().WriteLn( "Closing Memorycards");
		ClosePlugin_Mcd();
	}

	for( int i=PluginId_Count-1; i>=0; --i )
		Close( tbl_PluginInfo[i].id );
	
	DbgCon.WriteLn( Color_StrongBlue, "Plugins closed successfully." );
}

void PluginManager::Init( PluginsEnum_t pid )
{
	ScopedLock lock( m_mtx_PluginStatus );

	if( !m_info[pid] || m_info[pid]->IsInitialized ) return;

	Console.Indent().WriteLn( "Init %s", tbl_PluginInfo[pid].shortname );
	if( NULL != m_info[pid]->CommonBindings.Init() )
		throw Exception::PluginInitError( pid );

	m_info[pid]->IsInitialized = true;
}

void PluginManager::Shutdown( PluginsEnum_t pid )
{
	ScopedLock lock( m_mtx_PluginStatus );

	if( !m_info[pid] || !m_info[pid]->IsInitialized ) return;
	DevCon.Indent().WriteLn( "Shutdown %s", tbl_PluginInfo[pid].shortname );
	m_info[pid]->IsInitialized = false;
	m_info[pid]->CommonBindings.Shutdown();
}

// Initializes all plugins.  Plugin initialization should be done once for every new emulation
// session.  During a session emulation can be paused/resumed using Open/Close, and should be
// terminated using Shutdown().
//
// In a purist emulation sense, Init() and Shutdown() should only ever need be called for when
// the PS2's hardware has received a *full* hard reset.  Soft resets typically should rely on
// the PS2's bios/kernel to re-initialize hardware on the fly.
//
void PluginManager::Init()
{
	if( !NeedsInit() ) return;

	Console.WriteLn( Color_StrongBlue, "\nInitializing plugins..." );
	const PluginInfo* pi = tbl_PluginInfo; do {
		Init( pi->id );
	} while( ++pi, pi->shortname != NULL );

	if( SysPlugins.Mcd == NULL )
	{
		SysPlugins.Mcd = (PS2E_ComponentAPI_Mcd*)m_mcdPlugin->NewComponentInstance( PS2E_TYPE_Mcd );
		if( SysPlugins.Mcd == NULL )
		{
			// fixme: use plugin's GetLastError (not implemented yet!)
			throw Exception::PluginInitError( PluginId_Mcd, "Internal Memorycard Plugin failed to initialize." );
		}
	}

	Console.WriteLn( Color_StrongBlue, "Plugins initialized successfully.\n" );
}


// Shuts down all plugins.  Plugins are closed first, if necessary.
//
// In a purist emulation sense, Init() and Shutdown() should only ever need be called for when
// the PS2's hardware has received a *full* hard reset.  Soft resets typically should rely on
// the PS2's bios/kernel to re-initialize hardware on the fly.
//
void PluginManager::Shutdown()
{
	if( !NeedsShutdown() ) return;

	pxAssumeDev( !NeedsClose(), "Cannot shut down plugins prior to Close()" );
	
	GetMTGS().Cancel();	// cancel it for speedier shutdown!
	
	DbgCon.WriteLn( Color_StrongGreen, "Shutting down plugins..." );

	// Shutdown plugins in reverse order (probably doesn't matter...
	//  ... but what the heck, right?)

	for( int i=PluginId_Count-1; i>=0; --i )
	{
		Shutdown( tbl_PluginInfo[i].id );
	}

	// More memorycard hacks!!

	if( (SysPlugins.Mcd != NULL) && (m_mcdPlugin != NULL) )
	{
		m_mcdPlugin->DeleteComponentInstance( (PS2E_THISPTR)SysPlugins.Mcd );
		SysPlugins.Mcd = NULL;
	}

	DbgCon.WriteLn( Color_StrongGreen, "Plugins shutdown successfully." );
}

// For internal use only, unless you're the MTGS.  Then it's for you too!
// Returns false if the plugin returned an error.
bool PluginManager::DoFreeze( PluginsEnum_t pid, int mode, freezeData* data )
{
	if( (pid == PluginId_GS) && !GetMTGS().IsSelf() )
	{
		// GS needs some thread safety love...

		MTGS_FreezeData woot = { data, 0 };
		GetMTGS().Freeze( mode, woot );
		return woot.retval != -1;
	}
	else
	{
		ScopedLock lock( m_mtx_PluginStatus );
		return !m_info[pid] || m_info[pid]->CommonBindings.Freeze( mode, data ) != -1;
	}
}

// Thread Safety:
//   This function should only be called by the Main GUI thread and the GS thread (for GS states only),
//   as it has special handlers to ensure that GS freeze commands are executed appropriately on the
//   GS thread.
//
void PluginManager::Freeze( PluginsEnum_t pid, SaveStateBase& state )
{
	// No locking leeded -- DoFreeze locks as needed, and this avoids MTGS deadlock.
	//ScopedLock lock( m_mtx_PluginStatus );

	Console.Indent().WriteLn( "%s %s", state.IsSaving() ? "Saving" : "Loading",
		tbl_PluginInfo[pid].shortname );

	freezeData fP = { 0, NULL };
	if( !DoFreeze( pid, FREEZE_SIZE, &fP ) )
		fP.size = 0;

	int fsize = fP.size;
	state.Freeze( fsize );

	if( state.IsLoading() && (fsize == 0) )
	{
		// no state data to read, but the plugin expects some state data.
		// Issue a warning to console...
		if( fP.size != 0 )
			Console.Indent().Warning( "Warning: No data for this plugin was found. Plugin status may be unpredictable." );
		return;

		// Note: Size mismatch check could also be done here on loading, but
		// some plugins may have built-in version support for non-native formats or
		// older versions of a different size... or could give different sizes depending
		// on the status of the plugin when loading, so let's ignore it.
	}

	fP.size = fsize;
	if( fP.size == 0 ) return;

	state.PrepBlock( fP.size );
	fP.data = (s8*)state.GetBlockPtr();

	if( state.IsSaving() )
	{
		if( !DoFreeze(pid, FREEZE_SAVE, &fP) )
			throw Exception::FreezePluginFailure( pid );
	}
	else
	{
		if( !DoFreeze(pid, FREEZE_LOAD, &fP) )
			throw Exception::ThawPluginFailure( pid );
	}

	state.CommitBlock( fP.size );
}

bool PluginManager::KeyEvent( const keyEvent& evt )
{
	ScopedLock lock( m_mtx_PluginStatus );

	// [TODO] : The plan here is to give plugins "first chance" handling of keys.
	// Handling order will be fixed (GS, SPU2, PAD, etc), and the first plugin to
	// pick up the key and return "true" (for handled) will cause the loop to break.
	// The current version of PS2E doesn't support it yet, though.

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( pi->id != PluginId_PAD && m_info[pi->id] )
			m_info[pi->id]->CommonBindings.KeyEvent( const_cast<keyEvent*>(&evt) );
	} while( ++pi, pi->shortname != NULL );

	return false;
}

void PluginManager::SendSettingsFolder()
{
	ScopedLock lock( m_mtx_PluginStatus );
	if( m_SettingsFolder.IsEmpty() ) return;

	wxCharBuffer utf8buffer( m_SettingsFolder.ToUTF8() );

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( m_info[pi->id] ) m_info[pi->id]->CommonBindings.SetSettingsDir( utf8buffer );
	} while( ++pi, pi->shortname != NULL );
}

void PluginManager::SetSettingsFolder( const wxString& folder )
{
	ScopedLock lock( m_mtx_PluginStatus );

	wxString fixedfolder( folder );
	if( !fixedfolder.IsEmpty() && (fixedfolder[fixedfolder.length()-1] != wxFileName::GetPathSeparator() ) )
	{
		fixedfolder += wxFileName::GetPathSeparator();
	}
	
	if( m_SettingsFolder == fixedfolder ) return;
	m_SettingsFolder = fixedfolder;
	SendSettingsFolder();
}

void PluginManager::SendLogFolder()
{
	ScopedLock lock( m_mtx_PluginStatus );
	if( m_LogFolder.IsEmpty() ) return;

	wxCharBuffer utf8buffer( m_LogFolder.ToUTF8() );

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( m_info[pi->id] ) m_info[pi->id]->CommonBindings.SetLogFolder( utf8buffer );
	} while( ++pi, pi->shortname != NULL );
}

void PluginManager::SetLogFolder( const wxString& folder )
{
	ScopedLock lock( m_mtx_PluginStatus );

	wxString fixedfolder( folder );
	if( !fixedfolder.IsEmpty() && (fixedfolder[fixedfolder.length()-1] != wxFileName::GetPathSeparator() ) )
	{
		fixedfolder += wxFileName::GetPathSeparator();
	}

	if( m_LogFolder == fixedfolder ) return;
	m_LogFolder = fixedfolder;
	SendLogFolder();
}

void PluginManager::Configure( PluginsEnum_t pid )
{
	ScopedLock lock( m_mtx_PluginStatus );
	if( m_info[pid] ) m_info[pid]->CommonBindings.Configure();
}

bool PluginManager::AreLoaded() const
{
	ScopedLock lock( m_mtx_PluginStatus );
	for( int i=0; i<PluginId_Count; ++i )
	{
		if( !m_info[i] ) return false;
	}

	return true;
}

bool PluginManager::AreAnyLoaded() const
{
	ScopedLock lock( m_mtx_PluginStatus );
	for( int i=0; i<PluginId_Count; ++i )
	{
		if( m_info[i] ) return true;
	}

	return false;
}

bool PluginManager::AreAnyInitialized() const
{
	ScopedLock lock( m_mtx_PluginStatus );
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( IsInitialized(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}

bool PluginManager::IsOpen( PluginsEnum_t pid ) const
{
	pxAssume( (uint)pid < PluginId_Count );
	ScopedLock lock( m_mtx_PluginStatus );
	return m_info[pid] && m_info[pid]->IsOpened;
}

bool PluginManager::IsInitialized( PluginsEnum_t pid ) const
{
	pxAssume( (uint)pid < PluginId_Count );
	ScopedLock lock( m_mtx_PluginStatus );
	return m_info[pid] && m_info[pid]->IsInitialized;
}

bool PluginManager::IsLoaded( PluginsEnum_t pid ) const
{
	pxAssume( (uint)pid < PluginId_Count );
	return !!m_info[pid];
}

bool PluginManager::NeedsLoad() const
{
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( !IsLoaded(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );
	
	return false;
}		

bool PluginManager::NeedsUnload() const
{
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( IsLoaded(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}		

bool PluginManager::NeedsInit() const
{
	ScopedLock lock( m_mtx_PluginStatus );

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( !IsInitialized(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}

bool PluginManager::NeedsShutdown() const
{
	ScopedLock lock( m_mtx_PluginStatus );

	const PluginInfo* pi = tbl_PluginInfo; do {
		if( IsInitialized(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}

bool PluginManager::NeedsOpen() const
{
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( !IsOpen(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}

bool PluginManager::NeedsClose() const
{
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( IsOpen(pi->id) ) return true;
	} while( ++pi, pi->shortname != NULL );

	return false;
}

const wxString PluginManager::GetName( PluginsEnum_t pid ) const
{
	ScopedLock lock( m_mtx_PluginStatus );
	pxAssume( (uint)pid < PluginId_Count );
	return m_info[pid] ? m_info[pid]->Name : (wxString)_("Unloaded Plugin");
}

const wxString PluginManager::GetVersion( PluginsEnum_t pid ) const
{
	ScopedLock lock( m_mtx_PluginStatus );
	pxAssume( (uint)pid < PluginId_Count );
	return m_info[pid] ? m_info[pid]->Version : L"0.0";
}
