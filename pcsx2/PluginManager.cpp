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

#include "PrecompiledHeader.h"
#include "Utilities/ScopedPtr.h"

#include <wx/dir.h>
#include <wx/file.h>

#include "IopCommon.h"
#include "GS.h"
#include "HostGui.h"
#include "CDVD/CDVDisoReader.h"

#if _MSC_VER
#	include "svnrev.h"
#endif

SysPluginBindings SysPlugins;

bool SysPluginBindings::McdIsPresent( uint port, uint slot )
{
	return !!Mcd->McdIsPresent( (PS2E_THISPTR) Mcd, port, slot );
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


static s32  CALLBACK fallback_freeze(int mode, freezeData *data) { data->size = 0; return 0; }
static void CALLBACK fallback_keyEvent(keyEvent *ev) {}
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
_GSmakeSnapshot	   GSmakeSnapshot;
_GSmakeSnapshot2   GSmakeSnapshot2;
_GSirqCallback 	   GSirqCallback;
_GSprintf      	   GSprintf;
_GSsetBaseMem 	   GSsetBaseMem;
_GSsetGameCRC		GSsetGameCRC;
_GSsetFrameSkip	   GSsetFrameSkip;
_GSsetupRecording	GSsetupRecording;
_GSreset		   GSreset;
_GSwriteCSR		   GSwriteCSR;

static void CALLBACK GS_makeSnapshot(const char *path) {}
static void CALLBACK GS_setGameCRC(u32 crc, int gameopts) {}
static void CALLBACK GS_irqCallback(void (*callback)()) {}
static void CALLBACK GS_setFrameSkip(int frameskip) {}
static void CALLBACK GS_printf(int timeout, char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	Console.WriteLn(msg);
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
_SPU2async         SPU2async;


// DEV9
_DEV9open          DEV9open;
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
	{	"init",			NULL	},
	{	"close",		NULL	},
	{	"shutdown",		NULL	},

	{	"keyEvent",		(vMeth*)fallback_keyEvent },

	{	"freeze",		(vMeth*)fallback_freeze	},
	{	"test",			(vMeth*)fallback_test },
	{	"configure",	fallback_configure	},
	{	"about",		fallback_about	},

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
	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_GS[] =
{
	{	"GSopen2",			(vMeth**)&GSopen2			},
	{	"GSreset",			(vMeth**)&GSreset			},
	{	"GSsetupRecording",	(vMeth**)&GSsetupRecording	},
	{	"GSchangeSaveState",(vMeth**)&GSchangeSaveState	},
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
	{	"SPU2readDMA4Mem",		(vMeth**)&SPU2readDMA4Mem,	NULL },
	{	"SPU2readDMA7Mem",		(vMeth**)&SPU2readDMA7Mem,	NULL },
	{	"SPU2writeDMA4Mem",		(vMeth**)&SPU2writeDMA4Mem,	NULL },
	{	"SPU2writeDMA7Mem",		(vMeth**)&SPU2writeDMA7Mem,	NULL },
	{	"SPU2interruptDMA4",	(vMeth**)&SPU2interruptDMA4,NULL },
	{	"SPU2interruptDMA7",	(vMeth**)&SPU2interruptDMA7,NULL },
	{	"SPU2ReadMemAddr",		(vMeth**)&SPU2ReadMemAddr,	NULL },
	{	"SPU2irqCallback",		(vMeth**)&SPU2irqCallback,	NULL },

	{ NULL }
};

static const LegacyApi_OptMethod s_MethMessOpt_SPU2[] =
{
	{	"SPU2setClockPtr",		(vMeth**)&SPU2setClockPtr	},
	{	"SPU2async",			(vMeth**)&SPU2async			},
	{	"SPU2WriteMemAddr",		(vMeth**)&SPU2WriteMemAddr	},
	{	"SPU2setDMABaseAddr",	(vMeth**)&SPU2setDMABaseAddr},
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
	{	"DEV9readDMA8Mem",	(vMeth**)&DEV9readDMA8Mem,	NULL },
	{	"DEV9writeDMA8Mem",	(vMeth**)&DEV9writeDMA8Mem,	NULL },
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
//          Plugin Manager Implementation
// ---------------------------------------------------------------------------------

PluginManager::PluginManager( const wxString (&folders)[PluginId_Count] )
{
	Console.WriteLn( Color_StrongBlue, "Loading plugins..." );

	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;

		Console.WriteLn( L"\tBinding %s\t: %s ", tbl_PluginInfo[pid].GetShortname().c_str(), folders[pid].c_str() );

		if( folders[pid].IsEmpty() )
			throw Exception::InvalidArgument( "Empty plugin filename." );

		m_info[pid].Filename = folders[pid];

		if( !wxFile::Exists( folders[pid] ) )
			throw Exception::PluginLoadError( pid, folders[pid],
				wxLt("The configured %s plugin file was not found")
			);

		if( !m_info[pid].Lib.Load( folders[pid] ) )
			throw Exception::PluginLoadError( pid, folders[pid],
				wxLt("The configured %s plugin file is not a valid dynamic library")
			);

		// Try to enumerate the new v2.0 plugin interface first.
		// If that fails, fall back on the old style interface.

		//m_libs[i].GetSymbol( L"PS2E_InitAPI" );

		// Fetch plugin name and version information
		
		_PS2EgetLibName		GetLibName		= (_PS2EgetLibName)m_info[pid].Lib.GetSymbol( L"PS2EgetLibName" );
		_PS2EgetLibVersion2	GetLibVersion2	= (_PS2EgetLibVersion2)m_info[pid].Lib.GetSymbol( L"PS2EgetLibVersion2" );

		if( GetLibName == NULL || GetLibVersion2 == NULL )
			throw Exception::PluginLoadError( pid, m_info[pid].Filename,
				wxsFormat( L"\nMethod binding failure on GetLibName or GetLibVersion2.\n" ),
				_( "Configured plugin is not a PCSX2 plugin, or is for an older unsupported version of PCSX2." )
			);

		m_info[pid].Name = fromUTF8( GetLibName() );
		int version = GetLibVersion2( tbl_PluginInfo[pid].typemask );
		m_info[pid].Version.Printf( L"%d.%d.%d", (version>>8)&0xff, version&0xff, (version>>24)&0xff );

		// Bind Required Functions
		// (generate critical error if binding fails)

		BindCommon( pid );
		BindRequired( pid );
		BindOptional( pid );

		// Bind Optional Functions
		// (leave pointer null and do not generate error)
	} while( ++pi, pi->shortname != NULL );

	CDVDapi_Plugin.newDiskCB( cdvdNewDiskCB );

	// Hack for PAD's stupid parameter passed on Init
	PADinit = (_PADinit)m_info[PluginId_PAD].CommonBindings.Init;
	m_info[PluginId_PAD].CommonBindings.Init = _hack_PADinit;

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

	g_plugins = this;
}

PluginManager::~PluginManager() throw()
{
	try
	{
		Close();
		Shutdown();
	}
	DESTRUCTOR_CATCHALL
	// All library unloading done automatically.

	if( g_plugins == this )
		g_plugins = NULL;
}

void PluginManager::BindCommon( PluginsEnum_t pid )
{
	const LegacyApi_CommonMethod* current = s_MethMessCommon;
	VoidMethod** target = (VoidMethod**)&m_info[pid].CommonBindings;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*target = (VoidMethod*)m_info[pid].Lib.GetSymbol( current->GetMethodName( pid ) );

		if( *target == NULL )
			*target = current->Fallback;

		if( *target == NULL )
		{
			throw Exception::PluginLoadError( pid, m_info[pid].Filename,
				wxsFormat( L"\nMethod binding failure on: %s\n", current->GetMethodName( pid ).c_str() ),
				_( "Configured plugin is not a PCSX2 plugin, or is for an older unsupported version of PCSX2." )
			);
		}

		target++;
		current++;
	}
}

void PluginManager::BindRequired( PluginsEnum_t pid )
{
	const LegacyApi_ReqMethod* current = s_MethMessReq[pid];
	const wxDynamicLibrary& lib = m_info[pid].Lib;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*(current->Dest) = (VoidMethod*)lib.GetSymbol( current->GetMethodName() );

		if( *(current->Dest) == NULL )
			*(current->Dest) = current->Fallback;

		if( *(current->Dest) == NULL )
		{
			throw Exception::PluginLoadError( pid, m_info[pid].Filename,
				wxsFormat( L"\nMethod binding failure on: %s\n", current->GetMethodName().c_str() ),
				_( "Configured plugin is not a valid PCSX2 plugin, or is for an older unsupported version of PCSX2." )
			);
		}

		current++;
	}
}

void PluginManager::BindOptional( PluginsEnum_t pid )
{
	const LegacyApi_OptMethod* current = s_MethMessOpt[pid];
	const wxDynamicLibrary& lib = m_info[pid].Lib;

	wxDoNotLogInThisScope please;

	while( current->MethodName != NULL )
	{
		*(current->Dest) = (VoidMethod*)lib.GetSymbol( current->GetMethodName() );
		current++;
	}
}

// Exceptions:
//   FileNotFound - Thrown if one of the configured plugins doesn't exist.
//   NotPcsxPlugin - Thrown if one of the configured plugins is an invalid or unsupported DLL

extern bool renderswitch;
extern void spu2DMA4Irq();
extern void spu2DMA7Irq();
extern void spu2Irq();

static bool OpenPlugin_GS()
{
	mtgsThread.Resume();
	return true;
}

static bool OpenPlugin_PAD()
{
	return !PADopen( (void*)&pDsp );
}

static bool OpenPlugin_SPU2()
{
	if( SPU2open((void*)&pDsp) ) return false;

	SPU2irqCallback( spu2Irq, spu2DMA4Irq, spu2DMA7Irq );
	if( SPU2setDMABaseAddr != NULL ) SPU2setDMABaseAddr((uptr)psxM);
	if( SPU2setClockPtr != NULL ) SPU2setClockPtr(&psxRegs.cycle);
	return true;
}

static bool OpenPlugin_DEV9()
{
	dev9Handler = NULL;

	if( DEV9open( (void*)&pDsp ) ) return false;
	DEV9irqCallback( dev9Irq );
	dev9Handler = DEV9irqHandler();
	return true;
}

static bool OpenPlugin_USB()
{
	usbHandler = NULL;

	if( USBopen((void*)&pDsp) ) return false;
	USBirqCallback( usbIrq );
	usbHandler = USBirqHandler();
	if( USBsetRAM != NULL )
		USBsetRAM(psxM);
	return true;
}

static bool OpenPlugin_FW()
{
	if( FWopen((void*)&pDsp) ) return false;
	FWirqCallback( fwIrq );
	return true;
}

void PluginManager::Open( PluginsEnum_t pid )
{
	if( m_info[pid].IsOpened ) return;

	Console.WriteLn( "\tOpening %s", tbl_PluginInfo[pid].shortname );

	// Each Open needs to be called explicitly. >_<

	bool result = true;
	switch( pid )
	{
		case PluginId_CDVD:	result = DoCDVDopen();		break;
		case PluginId_GS:	result = OpenPlugin_GS();	break;
		case PluginId_PAD:	result = OpenPlugin_PAD();	break;
		case PluginId_SPU2:	result = OpenPlugin_SPU2();	break;
		case PluginId_USB:	result = OpenPlugin_USB();	break;
		case PluginId_FW:	result = OpenPlugin_FW();	break;
		case PluginId_DEV9:	result = OpenPlugin_DEV9();	break;
		jNO_DEFAULT;
	}
	if( !result )
		throw Exception::PluginOpenError( pid );

	m_info[pid].IsOpened = true;
}

void PluginManager::Open()
{
	Console.WriteLn( Color_StrongBlue, "Opening plugins..." );

	const PluginInfo* pi = tbl_PluginInfo; do {
		Open( pi->id );
		// If GS doesn't support GSopen2, need to wait until call to GSopen
		// returns to populate pDsp.  If it does, can initialize other plugins
		// at same time as GS, as long as GSopen2 does not subclass its window.
		if (pi->id == PluginId_GS && !GSopen2) mtgsThread.WaitForOpen();
	} while( ++pi, pi->shortname != NULL );

	if (GSopen2) mtgsThread.WaitForOpen();

	Console.WriteLn( Color_StrongBlue, "Plugins opened successfully." );
}

void PluginManager::Close( PluginsEnum_t pid )
{
	if( !m_info[pid].IsOpened ) return;
	Console.WriteLn( "\tClosing %s", tbl_PluginInfo[pid].shortname );

	if( pid == PluginId_GS )
	{
		// force-close PAD before GS, because the PAD depends on the GS window.
		Close( PluginId_PAD );
		mtgsThread.Suspend();
	}
	else if( pid == PluginId_CDVD )
		DoCDVDclose();
	else
		m_info[pid].CommonBindings.Close();

	m_info[pid].IsOpened = false;
}

void PluginManager::Close( bool closegs )
{
	DbgCon.WriteLn( Color_StrongBlue, "Closing plugins..." );

	// Close plugins in reverse order of the initialization procedure.

	for( int i=PluginId_Count-1; i>=0; --i )
	{
		if( closegs || (tbl_PluginInfo[i].id != PluginId_GS) )
			Close( tbl_PluginInfo[i].id );
	}

	DbgCon.WriteLn( Color_StrongBlue, "Plugins closed successfully." );
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
	bool printlog = false;
	const PluginInfo* pi = tbl_PluginInfo; do
	{
		const PluginsEnum_t pid = pi->id;

		if( m_info[pid].IsInitialized ) continue;
		if( !printlog )
		{
			Console.WriteLn( Color_StrongBlue, "Initializing plugins..." );
			printlog = true;
		}
		Console.WriteLn( "\tInit %s", tbl_PluginInfo[pid].shortname );
		if( 0 != m_info[pid].CommonBindings.Init() )
			throw Exception::PluginInitError( pid );

		m_info[pid].IsInitialized = true;
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

	if( printlog )
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
	mtgsThread.Cancel();	// cancel it for speedier shutdown!

	Close();
	DbgCon.WriteLn( Color_StrongGreen, "Shutting down plugins..." );

	// Shutdown plugins in reverse order (probably doesn't matter...
	//  ... but what the heck, right?)

	for( int i=PluginId_Count-1; i>=0; --i )
	{
		const PluginsEnum_t pid = tbl_PluginInfo[i].id;
		if( !m_info[pid].IsInitialized ) continue;
		DevCon.WriteLn( "\tShutdown %s", tbl_PluginInfo[pid].shortname );
		m_info[pid].IsInitialized = false;
		m_info[pid].CommonBindings.Shutdown();
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
	if( (pid == PluginId_GS) && !mtgsThread.IsSelf() )
	{
		// GS needs some thread safety love...

		MTGS_FreezeData woot = { data, 0 };
		mtgsThread.Freeze( mode, woot );
		return woot.retval != -1;
	}
	else
	{
		return m_info[pid].CommonBindings.Freeze( mode, data ) != -1;
	}
}

// Thread Safety:
//   This function should only be called by the Main GUI thread and the GS thread (for GS states only),
//   as it has special handlers to ensure that GS freeze commands are executed appropriately on the
//   GS thread.
//
void PluginManager::Freeze( PluginsEnum_t pid, SaveStateBase& state )
{
	Console.WriteLn( "\t%s %s", state.IsSaving() ? "Saving" : "Loading",
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
			Console.Warning( "\tWarning: No data for this plugin was found. Plugin status may be unpredictable." );
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
	const PluginInfo* pi = tbl_PluginInfo; do {
		if( pi->id != PluginId_PAD )
			m_info[pi->id].CommonBindings.KeyEvent( const_cast<keyEvent*>(&evt) );
	} while( ++pi, pi->shortname != NULL );

	return false;
}

void PluginManager::Configure( PluginsEnum_t pid )
{
	m_info[pid].CommonBindings.Configure();
}

PluginManager* PluginManager_Create( const wxChar* (&folders)[PluginId_Count] )
{
	wxString passins[PluginId_Count];

	const PluginInfo* pi = tbl_PluginInfo; do {
		passins[pi->id] = folders[pi->id];
	} while( ++pi, pi->shortname != NULL );

	return new PluginManager( passins );
}

static PluginManagerBase s_pluginman_placebo;

// retrieves a handle to the current plugin manager.  Plugin manager is assumed to be valid,
// and debug-level assertions are performed on the validity of the handle.
PluginManagerBase& GetPluginManager()
{
	if( g_plugins == NULL ) return s_pluginman_placebo;
	return *g_plugins;
}
