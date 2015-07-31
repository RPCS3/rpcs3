#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellSysmodule;

enum
{
	CELL_SYSMODULE_LOADED                     = CELL_OK,
	CELL_SYSMODULE_ERROR_DUPLICATED           = 0x80012001,
	CELL_SYSMODULE_ERROR_UNKNOWN              = 0x80012002,
	CELL_SYSMODULE_ERROR_UNLOADED             = 0x80012003,
	CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER = 0x80012004,
	CELL_SYSMODULE_ERROR_FATAL                = 0x800120ff,
};

const char* get_module_id(u16 id)
{
	switch (id)
	{
	case 0x0000: return "CELL_SYSMODULE_NET";
	case 0x0001: return "CELL_SYSMODULE_HTTP";
	case 0x0002: return "CELL_SYSMODULE_HTTP_UTIL";
	case 0x0003: return "CELL_SYSMODULE_SSL";
	case 0x0004: return "CELL_SYSMODULE_HTTPS";
	case 0x0005: return "CELL_SYSMODULE_VDEC";
	case 0x0006: return "CELL_SYSMODULE_ADEC";
	case 0x0007: return "CELL_SYSMODULE_DMUX";
	case 0x0008: return "CELL_SYSMODULE_VPOST";
	case 0x0009: return "CELL_SYSMODULE_RTC";
	case 0x000a: return "CELL_SYSMODULE_SPURS";
	case 0x000b: return "CELL_SYSMODULE_OVIS";
	case 0x000c: return "CELL_SYSMODULE_SHEAP";
	case 0x000d: return "CELL_SYSMODULE_SYNC";
	case 0x000e: return "CELL_SYSMODULE_FS";
	case 0x000f: return "CELL_SYSMODULE_JPGDEC";
	case 0x0010: return "CELL_SYSMODULE_GCM_SYS";
	case 0x0011: return "CELL_SYSMODULE_AUDIO";
	case 0x0012: return "CELL_SYSMODULE_PAMF";
	case 0x0013: return "CELL_SYSMODULE_ATRAC3PLUS";
	case 0x0014: return "CELL_SYSMODULE_NETCTL";
	case 0x0015: return "CELL_SYSMODULE_SYSUTIL";
	case 0x0016: return "CELL_SYSMODULE_SYSUTIL_NP";
	case 0x0017: return "CELL_SYSMODULE_IO";
	case 0x0018: return "CELL_SYSMODULE_PNGDEC";
	case 0x0019: return "CELL_SYSMODULE_FONT";
	case 0x001a: return "CELL_SYSMODULE_FONTFT";
	case 0x001b: return "CELL_SYSMODULE_FREETYPE";
	case 0x001c: return "CELL_SYSMODULE_USBD";
	case 0x001d: return "CELL_SYSMODULE_SAIL";
	case 0x001e: return "CELL_SYSMODULE_L10N";
	case 0x001f: return "CELL_SYSMODULE_RESC";
	case 0x0020: return "CELL_SYSMODULE_DAISY";
	case 0x0021: return "CELL_SYSMODULE_KEY2CHAR";
	case 0x0022: return "CELL_SYSMODULE_MIC";
	case 0x0023: return "CELL_SYSMODULE_CAMERA";
	case 0x0024: return "CELL_SYSMODULE_VDEC_MPEG2";
	case 0x0025: return "CELL_SYSMODULE_VDEC_AVC";
	case 0x0026: return "CELL_SYSMODULE_ADEC_LPCM";
	case 0x0027: return "CELL_SYSMODULE_ADEC_AC3";
	case 0x0028: return "CELL_SYSMODULE_ADEC_ATX";
	case 0x0029: return "CELL_SYSMODULE_ADEC_AT3";
	case 0x002a: return "CELL_SYSMODULE_DMUX_PAMF";
	case 0x002b: return "CELL_SYSMODULE_VDEC_AL";
	case 0x002c: return "CELL_SYSMODULE_ADEC_AL";
	case 0x002d: return "CELL_SYSMODULE_DMUX_AL";
	case 0x002e: return "CELL_SYSMODULE_LV2DBG";
	case 0x002f: return "CELL_SYSMODULE_SYSUTIL_AVCHAT";
	case 0x0030: return "CELL_SYSMODULE_USBPSPCM";
	case 0x0031: return "CELL_SYSMODULE_AVCONF_EXT";
	case 0x0032: return "CELL_SYSMODULE_SYSUTIL_USERINFO";
	case 0x0033: return "CELL_SYSMODULE_SYSUTIL_SAVEDATA";
	case 0x0034: return "CELL_SYSMODULE_SUBDISPLAY";
	case 0x0035: return "CELL_SYSMODULE_SYSUTIL_REC";
	case 0x0036: return "CELL_SYSMODULE_VIDEO_EXPORT";
	case 0x0037: return "CELL_SYSMODULE_SYSUTIL_GAME_EXEC";
	case 0x0038: return "CELL_SYSMODULE_SYSUTIL_NP2";
	case 0x0039: return "CELL_SYSMODULE_SYSUTIL_AP";
	case 0x003a: return "CELL_SYSMODULE_SYSUTIL_NP_CLANS";
	case 0x003b: return "CELL_SYSMODULE_SYSUTIL_OSK_EXT";
	case 0x003c: return "CELL_SYSMODULE_VDEC_DIVX";
	case 0x003d: return "CELL_SYSMODULE_JPGENC";
	case 0x003e: return "CELL_SYSMODULE_SYSUTIL_GAME";
	case 0x003f: return "CELL_SYSMODULE_BGDL";
	case 0x0040: return "CELL_SYSMODULE_FREETYPE_TT";
	case 0x0041: return "CELL_SYSMODULE_SYSUTIL_VIDEO_UPLOAD";
	case 0x0042: return "CELL_SYSMODULE_SYSUTIL_SYSCONF_EXT";
	case 0x0043: return "CELL_SYSMODULE_FIBER";
	case 0x0044: return "CELL_SYSMODULE_SYSUTIL_NP_COMMERCE2";
	case 0x0045: return "CELL_SYSMODULE_SYSUTIL_NP_TUS";
	case 0x0046: return "CELL_SYSMODULE_VOICE";
	case 0x0047: return "CELL_SYSMODULE_ADEC_CELP8";
	case 0x0048: return "CELL_SYSMODULE_CELP8ENC";
	case 0x0049: return "CELL_SYSMODULE_SYSUTIL_LICENSEAREA";
	case 0x004a: return "CELL_SYSMODULE_SYSUTIL_MUSIC2";
	case 0x004e: return "CELL_SYSMODULE_SYSUTIL_SCREENSHOT";
	case 0x004f: return "CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE";
	case 0x0050: return "CELL_SYSMODULE_SPURS_JQ";
	case 0x0052: return "CELL_SYSMODULE_PNGENC";
	case 0x0053: return "CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE2";
	case 0x0055: return "CELL_SYSMODULE_SYNC2";
	case 0x0056: return "CELL_SYSMODULE_SYSUTIL_NP_UTIL";
	case 0x0057: return "CELL_SYSMODULE_RUDP";
	case 0x0059: return "CELL_SYSMODULE_SYSUTIL_NP_SNS";
	case 0x005a: return "CELL_SYSMODULE_GEM";
	case 0xf00a: return "CELL_SYSMODULE_CELPENC";
	case 0xf010: return "CELL_SYSMODULE_GIFDEC";
	case 0xf019: return "CELL_SYSMODULE_ADEC_CELP";
	case 0xf01b: return "CELL_SYSMODULE_ADEC_M2BC";
	case 0xf01d: return "CELL_SYSMODULE_ADEC_M4AAC";
	case 0xf01e: return "CELL_SYSMODULE_ADEC_MP3";
	case 0xf023: return "CELL_SYSMODULE_IMEJP";
	case 0xf028: return "CELL_SYSMODULE_SYSUTIL_MUSIC";
	case 0xf029: return "CELL_SYSMODULE_PHOTO_EXPORT";
	case 0xf02a: return "CELL_SYSMODULE_PRINT";
	case 0xf02b: return "CELL_SYSMODULE_PHOTO_IMPORT";
	case 0xf02c: return "CELL_SYSMODULE_MUSIC_EXPORT";
	case 0xf02e: return "CELL_SYSMODULE_PHOTO_DECODE";
	case 0xf02f: return "CELL_SYSMODULE_SYSUTIL_SEARCH";
	case 0xf030: return "CELL_SYSMODULE_SYSUTIL_AVCHAT2";
	case 0xf034: return "CELL_SYSMODULE_SAIL_REC";
	case 0xf035: return "CELL_SYSMODULE_SYSUTIL_NP_TROPHY";
	case 0xf054: return "CELL_SYSMODULE_LIBATRAC3MULTI";
	case 0xffff: return "CELL_SYSMODULE_INVALID";
	}

	return "UNKNOWN MODULE";
}

s32 cellSysmoduleInitialize()
{
	cellSysmodule.Warning("cellSysmoduleInitialize()");
	return CELL_OK;
}

s32 cellSysmoduleFinalize()
{
	cellSysmodule.Warning("cellSysmoduleFinalize()");
	return CELL_OK;
}

s32 cellSysmoduleSetMemcontainer(u32 ct_id)
{
	cellSysmodule.Todo("cellSysmoduleSetMemcontainer(ct_id=0x%x)", ct_id);
	return CELL_OK;
}

s32 cellSysmoduleLoadModule(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleLoadModule(id=0x%04x: %s)", id, get_module_id(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (Module* m = Emu.GetModuleManager().GetModuleById(id))
	{
		// CELL_SYSMODULE_ERROR_DUPLICATED shouldn't be returned
		m->Load();
	}

	return CELL_OK;
}

s32 cellSysmoduleUnloadModule(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleUnloadModule(id=0x%04x: %s)", id, get_module_id(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (Module* m = Emu.GetModuleManager().GetModuleById(id))
	{
		if (!m->IsLoaded())
		{
			cellSysmodule.Error("cellSysmoduleUnloadModule() failed: module not loaded (id=0x%04x)", id);
			return CELL_SYSMODULE_ERROR_FATAL;
		}

		m->Unload();
	}
	
	return CELL_OK;
}

s32 cellSysmoduleIsLoaded(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleIsLoaded(id=0x%04x: %s)", id, get_module_id(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if (Module* m = Emu.GetModuleManager().GetModuleById(id))
	{
		if (!m->IsLoaded())
		{
			cellSysmodule.Error("cellSysmoduleIsLoaded() failed: module not loaded (id=0x%04x)", id);
			return CELL_SYSMODULE_ERROR_UNLOADED;
		}
	}

	return CELL_SYSMODULE_LOADED;
}

Module cellSysmodule("cellSysmodule", []()
{
	REG_FUNC(cellSysmodule, cellSysmoduleInitialize);
	REG_FUNC(cellSysmodule, cellSysmoduleFinalize);
	REG_FUNC(cellSysmodule, cellSysmoduleSetMemcontainer);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleIsLoaded);
});
