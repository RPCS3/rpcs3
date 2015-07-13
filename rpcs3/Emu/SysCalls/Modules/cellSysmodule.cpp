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

const char *getModuleName(int id) {
	struct
	{
		const char *name;
		int id;
	} static const entries[] = {
		{"CELL_SYSMODULE_INVALID", 0x0000ffff},
		{"CELL_SYSMODULE_NET", 0x00000000},
		{"CELL_SYSMODULE_HTTP", 0x00000001},
		{"CELL_SYSMODULE_HTTP_UTIL", 0x00000002},
		{"CELL_SYSMODULE_SSL", 0x00000003},
		{"CELL_SYSMODULE_HTTPS", 0x00000004},
		{"CELL_SYSMODULE_VDEC", 0x00000005},
		{"CELL_SYSMODULE_ADEC", 0x00000006},
		{"CELL_SYSMODULE_DMUX", 0x00000007},
		{"CELL_SYSMODULE_VPOST", 0x00000008},
		{"CELL_SYSMODULE_RTC", 0x00000009},
		{"CELL_SYSMODULE_SPURS", 0x0000000a},
		{"CELL_SYSMODULE_OVIS", 0x0000000b},
		{"CELL_SYSMODULE_SHEAP", 0x0000000c},
		{"CELL_SYSMODULE_SYNC", 0x0000000d},
		{"CELL_SYSMODULE_FS", 0x0000000e},
		{"CELL_SYSMODULE_JPGDEC", 0x0000000f},
		{"CELL_SYSMODULE_GCM_SYS", 0x00000010},
		{"CELL_SYSMODULE_GCM", 0x00000010},
		{"CELL_SYSMODULE_AUDIO", 0x00000011},
		{"CELL_SYSMODULE_PAMF", 0x00000012},
		{"CELL_SYSMODULE_ATRAC3PLUS", 0x00000013},
		{"CELL_SYSMODULE_NETCTL", 0x00000014},
		{"CELL_SYSMODULE_SYSUTIL", 0x00000015},
		{"CELL_SYSMODULE_SYSUTIL_NP", 0x00000016},
		{"CELL_SYSMODULE_IO", 0x00000017},
		{"CELL_SYSMODULE_PNGDEC", 0x00000018},
		{"CELL_SYSMODULE_FONT", 0x00000019},
		{"CELL_SYSMODULE_FONTFT", 0x0000001a},
		{"CELL_SYSMODULE_FREETYPE", 0x0000001b},
		{"CELL_SYSMODULE_USBD", 0x0000001c},
		{"CELL_SYSMODULE_SAIL", 0x0000001d},
		{"CELL_SYSMODULE_L10N", 0x0000001e},
		{"CELL_SYSMODULE_RESC", 0x0000001f},
		{"CELL_SYSMODULE_DAISY", 0x00000020},
		{"CELL_SYSMODULE_KEY2CHAR", 0x00000021},
		{"CELL_SYSMODULE_MIC", 0x00000022},
		{"CELL_SYSMODULE_CAMERA", 0x00000023},
		{"CELL_SYSMODULE_VDEC_MPEG2", 0x00000024},
		{"CELL_SYSMODULE_VDEC_AVC", 0x00000025},
		{"CELL_SYSMODULE_ADEC_LPCM", 0x00000026},
		{"CELL_SYSMODULE_ADEC_AC3", 0x00000027},
		{"CELL_SYSMODULE_ADEC_ATX", 0x00000028},
		{"CELL_SYSMODULE_ADEC_AT3", 0x00000029},
		{"CELL_SYSMODULE_DMUX_PAMF", 0x0000002a},
		{"CELL_SYSMODULE_VDEC_AL", 0x0000002b},
		{"CELL_SYSMODULE_ADEC_AL", 0x0000002c},
		{"CELL_SYSMODULE_DMUX_AL", 0x0000002d},
		{"CELL_SYSMODULE_LV2DBG", 0x0000002e},
		{"CELL_SYSMODULE_USBPSPCM", 0x00000030},
		{"CELL_SYSMODULE_AVCONF_EXT", 0x00000031},
		{"CELL_SYSMODULE_SYSUTIL_USERINFO", 0x00000032},
		{"CELL_SYSMODULE_SYSUTIL_SAVEDATA", 0x00000033},
		{"CELL_SYSMODULE_SUBDISPLAY", 0x00000034},
		{"CELL_SYSMODULE_SYSUTIL_REC", 0x00000035},
		{"CELL_SYSMODULE_VIDEO_EXPORT", 0x00000036},
		{"CELL_SYSMODULE_SYSUTIL_GAME_EXEC", 0x00000037},
		{"CELL_SYSMODULE_SYSUTIL_NP2", 0x00000038},
		{"CELL_SYSMODULE_SYSUTIL_AP", 0x00000039},
		{"CELL_SYSMODULE_SYSUTIL_NP_CLANS", 0x0000003a},
		{"CELL_SYSMODULE_SYSUTIL_OSK_EXT", 0x0000003b},
		{"CELL_SYSMODULE_VDEC_DIVX", 0x0000003c},
		{"CELL_SYSMODULE_JPGENC", 0x0000003d},
		{"CELL_SYSMODULE_SYSUTIL_GAME", 0x0000003e},
		{"CELL_SYSMODULE_BGDL", 0x0000003f},
		{"CELL_SYSMODULE_FREETYPE_TT", 0x00000040},
		{"CELL_SYSMODULE_SYSUTIL_VIDEO_UPLOAD", 0x00000041},
		{"CELL_SYSMODULE_SYSUTIL_SYSCONF_EXT", 0x00000042},
		{"CELL_SYSMODULE_FIBER", 0x00000043},
		{"CELL_SYSMODULE_SYSUTIL_NP_COMMERCE2", 0x00000044},
		{"CELL_SYSMODULE_SYSUTIL_NP_TUS", 0x00000045},
		{"CELL_SYSMODULE_VOICE", 0x00000046},
		{"CELL_SYSMODULE_ADEC_CELP8", 0x00000047},
		{"CELL_SYSMODULE_CELP8ENC", 0x00000048},
		{"CELL_SYSMODULE_SYSUTIL_LICENSEAREA", 0x00000049},
		{"CELL_SYSMODULE_SYSUTIL_MUSIC2", 0x0000004a},
		{"CELL_SYSMODULE_SYSUTIL_SCREENSHOT", 0x0000004e},
		{"CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE", 0x0000004f},
		{"CELL_SYSMODULE_SPURS_JQ", 0x00000050},
		{"CELL_SYSMODULE_PNGENC", 0x00000052},
		{"CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE2", 0x00000053},
		{"CELL_SYSMODULE_SYNC2", 0x00000055},
		{"CELL_SYSMODULE_SYSUTIL_NP_UTIL", 0x00000056},
		{"CELL_SYSMODULE_RUDP", 0x00000057},
		{"CELL_SYSMODULE_SYSUTIL_NP_SNS", 0x00000059},
		{"CELL_SYSMODULE_GEM", 0x0000005a},
		{"CELL_SYSMODULE_CELPENC", 0x0000f00a},
		{"CELL_SYSMODULE_GIFDEC", 0x0000f010},
		{"CELL_SYSMODULE_ADEC_CELP", 0x0000f019},
		{"CELL_SYSMODULE_ADEC_M2BC", 0x0000f01b},
		{"CELL_SYSMODULE_ADEC_M4AAC", 0x0000f01d},
		{"CELL_SYSMODULE_ADEC_MP3", 0x0000f01e},
		{"CELL_SYSMODULE_IMEJP", 0x0000f023},
		{"CELL_SYSMODULE_SYSUTIL_MUSIC", 0x0000f028},
		{"CELL_SYSMODULE_PHOTO_EXPORT",	0x0000f029},
		{"CELL_SYSMODULE_PRINT", 0x0000f02a},
		{"CELL_SYSMODULE_PHOTO_IMPORT", 0x0000f02b},
		{"CELL_SYSMODULE_MUSIC_EXPORT", 0x0000f02c},
		{"CELL_SYSMODULE_PHOTO_DECODE", 0x0000f02e},
		{"CELL_SYSMODULE_SYSUTIL_SEARCH", 0x0000f02f},
		{"CELL_SYSMODULE_SYSUTIL_AVCHAT2", 0x0000f030},
		{"CELL_SYSMODULE_SAIL_REC", 0x0000f034},
		{"CELL_SYSMODULE_SYSUTIL_NP_TROPHY", 0x0000f035},
		{"CELL_SYSMODULE_LIBATRAC3MULTI", 0x0000f054},
	};

	for (int i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i)
	{
		if (entries[i].id == id)
		{
			return entries[i].name;
		}
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
	cellSysmodule.Warning("cellSysmoduleLoadModule(id=0x%04x: %s)", id, getModuleName(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		cellSysmodule.Error("cellSysmoduleLoadModule() failed: unknown module (id=0x%04x)", id);
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
	cellSysmodule.Warning("cellSysmoduleUnloadModule(id=0x%04x: %s)", id, getModuleName(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		cellSysmodule.Error("cellSysmoduleUnloadModule() failed: unknown module (id=0x%04x)", id);
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
	cellSysmodule.Warning("cellSysmoduleIsLoaded(id=0x%04x: %s)", id, getModuleName(id));

	if (!Emu.GetModuleManager().CheckModuleId(id))
	{
		cellSysmodule.Error("cellSysmoduleIsLoaded() failed: unknown module (id=0x%04x)", id);
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
