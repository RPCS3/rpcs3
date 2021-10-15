#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

LOG_CHANNEL(cellSysmodule);

constexpr auto CELL_SYSMODULE_LOADED = CELL_OK;

enum CellSysmoduleError : u32
{
	CELL_SYSMODULE_ERROR_DUPLICATED           = 0x80012001,
	CELL_SYSMODULE_ERROR_UNKNOWN              = 0x80012002,
	CELL_SYSMODULE_ERROR_UNLOADED             = 0x80012003,
	CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER = 0x80012004,
	CELL_SYSMODULE_ERROR_FATAL                = 0x800120ff,
};

template<>
void fmt_class_string<CellSysmoduleError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SYSMODULE_ERROR_DUPLICATED);
			STR_CASE(CELL_SYSMODULE_ERROR_UNKNOWN);
			STR_CASE(CELL_SYSMODULE_ERROR_UNLOADED);
			STR_CASE(CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER);
			STR_CASE(CELL_SYSMODULE_ERROR_FATAL);
		}

		return unknown;
	});
}

static const char* get_module_name(u16 id)
{
	switch (id)
	{
	case 0x0000: return "sys_net";
	case 0x0001: return "cellHttp";
	case 0x0002: return "cellHttpUtil";
	case 0x0003: return "cellSsl";
	case 0x0004: return "cellHttps";
	case 0x0005: return "libvdec";
	case 0x0006: return "cellAdec";
	case 0x0007: return "cellDmux";
	case 0x0008: return "cellVpost";
	case 0x0009: return "cellRtc";
	case 0x000a: return "cellSpurs";
	case 0x000b: return "cellOvis";
	case 0x000c: return "cellSheap";
	case 0x000d: return "cellSync";
	case 0x000e: return "sys_fs";
	case 0x000f: return "cellJpgDec";
	case 0x0010: return "cellGcmSys";
	case 0x0011: return "cellAudio";
	case 0x0012: return "cellPamf";
	case 0x0013: return "cellAtrac";
	case 0x0014: return "cellNetCtl";
	case 0x0015: return "cellSysutil";
	case 0x0016: return "sceNp";
	case 0x0017: return "sys_io";
	case 0x0018: return "cellPngDec";
	case 0x0019: return "cellFont";
	case 0x001a: return "cellFontFT";
	case 0x001b: return "cell_FreeType2";
	case 0x001c: return "cellUsbd";
	case 0x001d: return "cellSail";
	case 0x001e: return "cellL10n";
	case 0x001f: return "cellResc";
	case 0x0020: return "cellDaisy";
	case 0x0021: return "cellKey2char";
	case 0x0022: return "cellMic";
	case 0x0023: return "cellCamera";
	case 0x0024: return "cellVdecMpeg2";
	case 0x0025: return "cellVdecAvc";
	case 0x0026: return "cellAdecLpcm";
	case 0x0027: return "cellAdecAc3";
	case 0x0028: return "cellAdecAtx";
	case 0x0029: return "cellAdecAt3";
	case 0x002a: return "cellDmuxPamf";
	case 0x002b: return nullptr;
	case 0x002c: return nullptr;
	case 0x002d: return nullptr;
	case 0x002e: return "sys_lv2dbg";
	case 0x002f: return "cellSysutilAvcExt";
	case 0x0030: return "cellUsbPspcm";
	case 0x0031: return "cellSysutilAvconfExt";
	case 0x0032: return "cellUserInfo";
	case 0x0033: return "cellSaveData";
	case 0x0034: return "cellSubDisplay";
	case 0x0035: return "cellRec";
	case 0x0036: return "cellVideoExportUtility";
	case 0x0037: return "cellGameExec";
	case 0x0038: return "sceNp2";
	case 0x0039: return "cellSysutilAp";
	case 0x003a: return "sceNpClans";
	case 0x003b: return "cellOskExtUtility";
	case 0x003c: return "cellVdecDivx";
	case 0x003d: return "cellJpgEnc";
	case 0x003e: return "cellGame";
	case 0x003f: return "cellBGDLUtility";
	case 0x0040: return "cell_FreeType2";
	case 0x0041: return "cellVideoUpload";
	case 0x0042: return "cellSysconfExtUtility";
	case 0x0043: return "cellFiber";
	case 0x0044: return "sceNpCommerce2";
	case 0x0045: return "sceNpTus";
	case 0x0046: return "cellVoice";
	case 0x0047: return "cellAdecCelp8";
	case 0x0048: return "cellCelp8Enc";
	case 0x0049: return "cellSysutilMisc";
	case 0x004a: return "cellMusicUtility";
	// TODO: Check if those libad are correctly matched.
	// They belong to those IDs but actual order is unknown.
	case 0x004b: return "libad_core";
	case 0x004c: return "libad_async";
	case 0x004d: return "libad_billboard_util";
	case 0x004e: return "cellScreenShotUtility";
	case 0x004f: return "cellMusicDecodeUtility";
	case 0x0050: return "cellSpursJq";
	case 0x0052: return "cellPngEnc";
	case 0x0053: return "cellMusicDecodeUtility";
	case 0x0054: return "libmedi";
	case 0x0055: return "cellSync2";
	case 0x0056: return "sceNpUtil";
	case 0x0057: return "cellRudp";
	case 0x0059: return "sceNpSns";
	case 0x005a: return "libgem";
	case 0x005c: return "cellCrossController";
	case 0xf00a: return "cellCelpEnc";
	case 0xf010: return "cellGifDec";
	case 0xf019: return "cellAdecCelp";
	case 0xf01b: return "cellAdecM2bc";
	case 0xf01d: return "cellAdecM4aac";
	case 0xf01e: return "cellAdecMp3";
	case 0xf023: return "cellImeJpUtility";
	case 0xf028: return "cellMusicUtility";
	case 0xf029: return "cellPhotoUtility";
	case 0xf02a: return "cellPrintUtility";
	case 0xf02b: return "cellPhotoImportUtil";
	case 0xf02c: return "cellMusicExportUtility";
	case 0xf02e: return "cellPhotoDecodeUtil";
	case 0xf02f: return "cellSearchUtility";
	case 0xf030: return "cellSysutilAvc2";
	case 0xf034: return "cellSailRec";
	case 0xf035: return "sceNpTrophy";
	case 0xf044: return "cellSysutilNpEula";
	case 0xf053: return "cellAdecAt3multi";
	case 0xf054: return "cellAtracMulti";
	}

	return nullptr;
}

static const char* get_module_id(u16 id)
{
	static thread_local char tls_id_name[8]; // for test

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
	// TODO: Check if those libad are correctly matched.
	// They belong to those IDs but actual order is unknown.
	case 0x004b: return "CELL_SYSMODULE_AD_CORE";
	case 0x004c: return "CELL_SYSMODULE_AD_ASYNC";
	case 0x004d: return "CELL_SYSMODULE_AD_BILLBOARD_UTIL";
	case 0x004e: return "CELL_SYSMODULE_SYSUTIL_SCREENSHOT";
	case 0x004f: return "CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE";
	case 0x0050: return "CELL_SYSMODULE_SPURS_JQ";
	case 0x0052: return "CELL_SYSMODULE_PNGENC";
	case 0x0053: return "CELL_SYSMODULE_SYSUTIL_MUSIC_DECODE2";
	case 0x0054: return "CELL_SYSMODULE_MEDI";
	case 0x0055: return "CELL_SYSMODULE_SYNC2";
	case 0x0056: return "CELL_SYSMODULE_SYSUTIL_NP_UTIL";
	case 0x0057: return "CELL_SYSMODULE_RUDP";
	case 0x0059: return "CELL_SYSMODULE_SYSUTIL_NP_SNS";
	case 0x005a: return "CELL_SYSMODULE_GEM";
	case 0x005c: return "CELL_SYSMODULE_SYSUTIL_CROSS_CONTROLLER";
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
	case 0xf044: return "CELL_SYSMODULE_SYSUTIL_NP_EULA";
	case 0xf053: return "CELL_SYSMODULE_ADEC_AT3MULTI";
	case 0xf054: return "CELL_SYSMODULE_LIBATRAC3MULTI";
	case 0xffff: return "CELL_SYSMODULE_INVALID";
	}

	std::snprintf(tls_id_name, sizeof(tls_id_name), "0x%04X", id);
	return tls_id_name;
}

error_code cellSysmoduleInitialize()
{
	cellSysmodule.warning("cellSysmoduleInitialize()");
	return CELL_OK;
}

error_code cellSysmoduleFinalize()
{
	cellSysmodule.warning("cellSysmoduleFinalize()");
	return CELL_OK;
}

error_code cellSysmoduleSetMemcontainer(u32 ct_id)
{
	cellSysmodule.todo("cellSysmoduleSetMemcontainer(ct_id=0x%x)", ct_id);
	return CELL_OK;
}

error_code cellSysmoduleLoadModule(u16 id)
{
	cellSysmodule.warning("cellSysmoduleLoadModule(id=0x%04X=%s)", id, get_module_id(id));

	const auto name = get_module_name(id);

	if (!name)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	//if (Module<>* m = Emu.GetModuleManager().GetModuleById(id))
	//{
	//	// CELL_SYSMODULE_ERROR_DUPLICATED shouldn't be returned
	//	m->Load();
	//}

	return CELL_OK;
}

error_code cellSysmoduleUnloadModule(u16 id)
{
	cellSysmodule.warning("cellSysmoduleUnloadModule(id=0x%04X=%s)", id, get_module_id(id));

	const auto name = get_module_name(id);

	if (!name)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	//if (Module<>* m = Emu.GetModuleManager().GetModuleById(id))
	//{
	//	if (!m->IsLoaded())
	//	{
	//		cellSysmodule.error("cellSysmoduleUnloadModule() failed: module not loaded (id=0x%04x)", id);
	//		return CELL_SYSMODULE_ERROR_FATAL;
	//	}

	//	m->Unload();
	//}

	return CELL_OK;
}

error_code cellSysmoduleIsLoaded(u16 id)
{
	cellSysmodule.warning("cellSysmoduleIsLoaded(id=0x%04X=%s)", id, get_module_id(id));

	const auto name = get_module_name(id);

	if (!name)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	//if (Module<>* m = Emu.GetModuleManager().GetModuleById(id))
	//{
	//	if (!m->IsLoaded())
	//	{
	//		cellSysmodule.warning("cellSysmoduleIsLoaded(): module not loaded (id=0x%04x)", id);
	//		return CELL_SYSMODULE_ERROR_UNLOADED;
	//	}
	//}

	return CELL_SYSMODULE_LOADED;
}

error_code cellSysmoduleGetImagesize()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleFetchImage()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleUnloadModuleInternal()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleLoadModuleInternal()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleUnloadModuleEx()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleLoadModuleEx()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

error_code cellSysmoduleIsLoadedEx()
{
	UNIMPLEMENTED_FUNC(cellSysmodule);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellSysmodule)("cellSysmodule", []()
{
	REG_FUNC(cellSysmodule, cellSysmoduleInitialize);
	REG_FUNC(cellSysmodule, cellSysmoduleFinalize);
	REG_FUNC(cellSysmodule, cellSysmoduleSetMemcontainer);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModule);
	REG_FUNC(cellSysmodule, cellSysmoduleIsLoaded);
	REG_FUNC(cellSysmodule, cellSysmoduleGetImagesize);
	REG_FUNC(cellSysmodule, cellSysmoduleFetchImage);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModuleInternal);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModuleInternal);
	REG_FUNC(cellSysmodule, cellSysmoduleUnloadModuleEx);
	REG_FUNC(cellSysmodule, cellSysmoduleLoadModuleEx);
	REG_FUNC(cellSysmodule, cellSysmoduleIsLoadedEx);
});
