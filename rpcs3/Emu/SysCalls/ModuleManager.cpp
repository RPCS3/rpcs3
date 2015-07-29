#include "stdafx.h"
#include "Modules.h"
#include "ModuleManager.h"

extern Module cellAdec;
extern Module cellAtrac;
extern Module cellAtracMulti;
extern Module cellAudio;
extern Module cellAvconfExt;
extern Module cellBGDL;
extern Module cellCamera;
extern Module cellCelp8Enc;
extern Module cellCelpEnc;
extern Module cellDaisy;
extern Module cellDmux;
extern Module cellFiber;
extern Module cellFont;
extern Module cellFontFT;
extern Module cellFs;
extern Module cellGame;
extern Module cellGameExec;
extern Module cellGcmSys;
extern Module cellGem;
extern Module cellGifDec;
extern Module cellHttp;
extern Module cellHttps;
extern Module cellHttpUtil;
extern Module cellJpgDec;
extern Module cellJpgEnc;
extern Module cellKey2char;
extern Module cellL10n;
extern Module cellMic;
extern Module cellNetCtl;
extern Module cellOvis;
extern Module cellPamf;
extern Module cellPngDec;
extern Module cellPngEnc;
extern Module cellResc;
extern Module cellRtc;
extern Module cellRudp;
extern Module cellSail;
extern Module cellSailRec;
extern Module cellSaveData;
extern Module cellMinisSaveData;
extern Module cellScreenshot;
extern Module cellSearch;
extern Module cellSheap;
extern Module cellSpudll;
extern Module cellSpurs;
extern Module cellSpursJq;
extern Module cellSsl;
extern Module cellSubdisplay;
extern Module cellSync;
extern Module cellSync2;
extern Module cellSysmodule;
extern Module cellSysutil;
extern Module cellSysutilAp;
extern Module cellSysutilAvc2;
extern Module cellUsbd;
extern Module cellUsbPspcm;
extern Module cellUserInfo;
extern Module cellVdec;
extern Module cellVoice;
extern Module cellVpost;
extern Module libmixer;
extern Module sceNp;
extern Module sceNp2;
extern Module sceNpClans;
extern Module sceNpCommerce2;
extern Module sceNpSns;
extern Module sceNpTrophy;
extern Module sceNpTus;
extern Module sceNpUtil;
extern Module sys_io;
extern Module sys_net;
extern Module sysPrxForUser;
extern Module sys_libc;
extern Module sys_lv2dbg;

struct ModuleInfo
{
	const s32 id; // -1 if the module doesn't have corresponding CELL_SYSMODULE_* id
	const char* const name;
	Module* const module;

	explicit operator bool() const
	{
		return module != nullptr;
	}

	operator Module*() const
	{
		return module;
	}

	Module* operator ->() const
	{
		return module;
	}
}
const g_module_list[] =
{
	{ 0x0000, "sys_net", &sys_net },
	{ 0x0001, "cellHttp", &cellHttp },
	{ 0x0002, "cellHttpUtil", &cellHttpUtil },
	{ 0x0003, "cellSsl", &cellSsl },
	{ 0x0004, "cellHttps", &cellHttps },
	{ 0x0005, "libvdec", &cellVdec },
	{ 0x0006, "cellAdec", &cellAdec },
	{ 0x0007, "cellDmux", &cellDmux },
	{ 0x0008, "cellVpost", &cellVpost },
	{ 0x0009, "cellRtc", &cellRtc },
	{ 0x000a, "cellSpurs", &cellSpurs },
	{ 0x000b, "cellOvis", &cellOvis },
	{ 0x000c, "cellSheap", &cellSheap },
	{ 0x000d, "cellSync", &cellSync },
	{ 0x000e, "sys_fs", &cellFs },
	{ 0x000f, "cellJpgDec", &cellJpgDec },
	{ 0x0010, "cellGcmSys", &cellGcmSys },
	{ 0x0011, "cellAudio", &cellAudio },
	{ 0x0012, "cellPamf", &cellPamf },
	{ 0x0013, "cellAtrac", &cellAtrac },
	{ 0x0014, "cellNetCtl", &cellNetCtl },
	{ 0x0015, "cellSysutil", &cellSysutil },
	{ 0x0016, "sceNp", &sceNp },
	{ 0x0017, "sys_io", &sys_io },
	{ 0x0018, "cellPngDec", &cellPngDec },
	{ 0x0019, "cellFont", &cellFont },
	{ 0x001a, "cellFontFT", &cellFontFT },
	{ 0x001b, "cellFreetype", nullptr },
	{ 0x001c, "cellUsbd", &cellUsbd },
	{ 0x001d, "cellSail", &cellSail },
	{ 0x001e, "cellL10n", &cellL10n },
	{ 0x001f, "cellResc", &cellResc },
	{ 0x0020, "cellDaisy", &cellDaisy },
	{ 0x0021, "cellKey2char", &cellKey2char },
	{ 0x0022, "cellMic", &cellMic },
	{ 0x0023, "cellCamera", &cellCamera },
	{ 0x0024, "cellVdecMpeg2", nullptr },
	{ 0x0025, "cellVdecAvc", nullptr },
	{ 0x0026, "cellAdecLpcm", nullptr },
	{ 0x0027, "cellAdecAc3", nullptr },
	{ 0x0028, "cellAdecAtx", nullptr },
	{ 0x0029, "cellAdecAt3", nullptr },
	{ 0x002a, "cellDmuxPamf", nullptr },
	{ 0x002e, "sys_lv2dbg", &sys_lv2dbg },
	{ 0x0030, "cellUsbPspcm", &cellUsbPspcm },
	{ 0x0031, "cellSysutilAvconfExt", &cellAvconfExt },
	{ 0x0032, "cellUserInfo", &cellUserInfo },
	{ 0x0033, "cellSaveData", &cellSaveData },
	{ 0x0034, "cellSubdisplay", &cellSubdisplay },
	{ 0x0035, "cellSysutilRec", nullptr },
	{ 0x0036, "cellVideoExport", nullptr },
	{ 0x0037, "cellGameExec", &cellGameExec },
	{ 0x0038, "sceNp2", &sceNp2 },
	{ 0x0039, "cellSysutilAp", &cellSysutilAp },
	{ 0x003a, "sceNpClans", &sceNpClans },
	{ 0x003b, "cellSysutilOskExt", nullptr },
	{ 0x003c, "cellVdecDivx", nullptr },
	{ 0x003d, "cellJpgEnc", &cellJpgEnc },
	{ 0x003e, "cellGame", &cellGame },
	{ 0x003f, "cellBGDLUtility", &cellBGDL },
	{ 0x0040, "cellFreetypeTT", nullptr },
	{ 0x0041, "cellSysutilVideoUpload", nullptr },
	{ 0x0042, "cellSysutilSysconfExt", nullptr },
	{ 0x0043, "cellFiber", &cellFiber },
	{ 0x0044, "sceNpCommerce2", &sceNpCommerce2 },
	{ 0x0045, "sceNpTus", &sceNpTus },
	{ 0x0046, "cellVoice", &cellVoice },
	{ 0x0047, "cellAdecCelp8", nullptr },
	{ 0x0048, "cellCelp8Enc", &cellCelp8Enc },
	{ 0x0049, "cellLicenseArea", nullptr },
	{ 0x004a, "cellMusic2", nullptr },
	{ 0x004e, "cellScreenShotUtility", &cellScreenshot },
	{ 0x004f, "cellMusicDecode", nullptr },
	{ 0x0050, "cellSpursJq", &cellSpursJq },
	{ 0x0052, "cellPngEnc", &cellPngEnc },
	{ 0x0053, "cellMusicDecode2", nullptr },
	{ 0x0055, "cellSync2", &cellSync2 },
	{ 0x0056, "sceNpUtil", &sceNpUtil },
	{ 0x0057, "cellRudp", &cellRudp },
	{ 0x0059, "sceNpSns", &sceNpSns },
	{ 0x005a, "libgem", &cellGem },
	{ 0xf00a, "cellCelpEnc", &cellCelpEnc },
	{ 0xf010, "cellGifDec", &cellGifDec },
	{ 0xf019, "cellAdecCelp", nullptr },
	{ 0xf01b, "cellAdecM2bc", nullptr },
	{ 0xf01d, "cellAdecM4aac", nullptr },
	{ 0xf01e, "cellAdecMp3", nullptr },
	{ 0xf023, "cellImejp", nullptr },
	{ 0xf028, "cellMusic", nullptr },
	{ 0xf029, "cellPhotoExport", nullptr },
	{ 0xf02a, "cellPrint", nullptr },
	{ 0xf02b, "cellPhotoImport", nullptr },
	{ 0xf02c, "cellMusicExport", nullptr },
	{ 0xf02e, "cellPhotoDecode", nullptr },
	{ 0xf02f, "cellSearch", &cellSearch },
	{ 0xf030, "cellSysutilAvc2", &cellSysutilAvc2 },
	{ 0xf034, "cellSailRec", &cellSailRec },
	{ 0xf035, "sceNpTrophy", &sceNpTrophy },
	{ 0xf053, "cellAdecAt3multi", nullptr },
	{ 0xf054, "cellAtracMulti", &cellAtracMulti },

	{ -1, "cellSysmodule", &cellSysmodule },
	{ -1, "libmixer", &libmixer },
	{ -1, "sysPrxForUser", &sysPrxForUser },
	{ -1, "sys_libc", &sys_libc },
	{ -1, "cellMinisSaveData", &cellMinisSaveData },
	{ -1, "cellSpudll", &cellSpudll },
};

void ModuleManager::Init()
{
	if (m_init)
	{
		Close();
	}

	clear_ppu_functions();

	for (auto& module : g_module_list)
	{
		if (module)
		{
			module->Init();
		}
	}

	m_init = true;
}

ModuleManager::ModuleManager()
{
}

ModuleManager::~ModuleManager()
{
	Close();
}

void ModuleManager::Close()
{
	if (!m_init)
	{
		return;
	}

	for (auto& module : g_module_list)
	{
		if (module && module->on_stop)
		{
			module->on_stop();
		}
	}

	m_init = false;
}

Module* ModuleManager::GetModuleByName(const char* name)
{
	for (auto& module : g_module_list)
	{
		if (!strcmp(name, module.name))
		{
			return module;
		}
	}

	return nullptr;
}

Module* ModuleManager::GetModuleById(u16 id)
{
	for (auto& module : g_module_list)
	{
		if (module.id == id)
		{
			return module;
		}
	}

	return nullptr;
}

bool ModuleManager::CheckModuleId(u16 id)
{
	for (auto& module : g_module_list)
	{
		if (module.id == id)
		{
			return true;
		}
	}

	return false;
}
