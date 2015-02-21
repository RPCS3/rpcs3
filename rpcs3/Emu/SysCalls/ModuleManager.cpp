#include "stdafx.h"
#include "ModuleManager.h"

extern Module sys_fs;
extern Module cellAdec;
extern Module cellAtrac;
extern Module cellAudio;
extern Module cellAvconfExt;
extern Module cellCamera;
extern Module cellDmux;
extern Module cellFiber;
extern Module cellFont;
extern Module cellFontFT;
extern Module cellGame;
extern Module cellGcmSys;
extern Module cellGem;
extern Module cellGifDec;
extern Module cellJpgDec;
extern Module sys_io;
extern Module cellL10n;
extern Module cellMic;
extern Module sys_io;
extern Module cellSysutil;
extern Module cellNetCtl;
extern Module cellOvis;
extern Module sys_io;
extern Module cellPamf;
extern Module cellPngDec;
extern Module cellResc;
extern Module cellRtc;
extern Module cellSail;
extern Module cellSysutil;
extern Module cellSpurs;
extern Module cellSpursJq;
extern Module cellSubdisplay;
extern Module cellSync;
extern Module cellSync2;
extern Module cellSysmodule;
extern Module cellSysutil;
extern Module cellSysutilAp;
extern Module cellUserInfo;
extern Module cellVdec;
extern Module cellVpost;
extern Module libmixer;
extern Module sceNp;
extern Module sceNpClans;
extern Module sceNpCommerce2;
extern Module sceNpSns;
extern Module sceNpTrophy;
extern Module sceNpTus;
extern Module sys_io;
extern Module sys_net;
extern Module sysPrxForUser;

struct ModuleInfo
{
	s32 id; //-1 is used by module with only name
	const char* name;
	Module* module;
}
static const g_module_list[] =
{
	{ 0x0000, "sys_net", &sys_net },
	{ 0x0001, "sys_http", nullptr },
	{ 0x0002, "cellHttpUtil", nullptr },
	{ 0x0003, "cellSsl", nullptr },
	{ 0x0004, "cellHttps", nullptr },
	{ 0x0005, "libvdec", &cellVdec },
	{ 0x0006, "cellAdec", &cellAdec },
	{ 0x0007, "cellDmux", &cellDmux },
	{ 0x0008, "cellVpost", &cellVpost },
	{ 0x0009, "cellRtc", &cellRtc },
	{ 0x000a, "cellSpurs", &cellSpurs },
	{ 0x000b, "cellOvis", &cellOvis },
	{ 0x000c, "cellSheap", nullptr },
	{ 0x000d, "sys_sync", &cellSync },
	{ 0x000e, "sys_fs", &sys_fs },
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
	{ 0x001c, "cellUsbd", nullptr },
	{ 0x001d, "cellSail", &cellSail },
	{ 0x001e, "cellL10n", &cellL10n },
	{ 0x001f, "cellResc", &cellResc },
	{ 0x0020, "cellDaisy", nullptr },
	{ 0x0021, "cellKey2char", nullptr },
	{ 0x0022, "cellMic", &cellMic },
	{ 0x0023, "cellCamera", &cellCamera },
	{ 0x0024, "cellVdecMpeg2", nullptr },
	{ 0x0025, "cellVdecAvc", nullptr },
	{ 0x0026, "cellAdecLpcm", nullptr },
	{ 0x0027, "cellAdecAc3", nullptr },
	{ 0x0028, "cellAdecAtx", nullptr },
	{ 0x0029, "cellAdecAt3", nullptr },
	{ 0x002a, "cellDmuxPamf", nullptr },
	{ 0x002e, "cellLv2dbg", nullptr },
	{ 0x0030, "cellUsbpspcm", nullptr },
	{ 0x0031, "cellAvconfExt", &cellAvconfExt },
	{ 0x0032, "cellUserInfo", &cellUserInfo },
	{ 0x0033, "cellSysutilSavedata", nullptr },
	{ 0x0034, "cellSubdisplay", &cellSubdisplay },
	{ 0x0035, "cellSysutilRec", nullptr },
	{ 0x0036, "cellVideoExport", nullptr },
	{ 0x0037, "cellGameExec", nullptr },
	{ 0x0038, "sceNp2", nullptr },
	{ 0x0039, "cellSysutilAp", &cellSysutilAp },
	{ 0x003a, "cellSysutilNpClans", nullptr },
	{ 0x003b, "cellSysutilOskExt", nullptr },
	{ 0x003c, "cellVdecDivx", nullptr },
	{ 0x003d, "cellJpgEnc", nullptr },
	{ 0x003e, "cellGame", &cellGame },
	{ 0x003f, "cellBgdl", nullptr },
	{ 0x0040, "cellFreetypeTT", nullptr },
	{ 0x0041, "cellSysutilVideoUpload", nullptr },
	{ 0x0042, "cellSysutilSysconfExt", nullptr },
	{ 0x0043, "cellFiber", &cellFiber },
	{ 0x0044, "sceNpCommerce2", &sceNpCommerce2 },
	{ 0x0045, "sceNpTus", &sceNpTus },
	{ 0x0046, "cellVoice", nullptr },
	{ 0x0047, "cellAdecCelp8", nullptr },
	{ 0x0048, "cellCelp8Enc", nullptr },
	{ 0x0049, "cellLicenseArea", nullptr },
	{ 0x004a, "cellMusic2", nullptr },
	{ 0x004e, "cellScreenshot", nullptr },
	{ 0x004f, "cellMusicDecode", nullptr },
	{ 0x0050, "cellSpursJq", &cellSpursJq },
	{ 0x0052, "cellPngEnc", nullptr },
	{ 0x0053, "cellMusicDecode2", nullptr },
	{ 0x0055, "cellSync2", &cellSync2 },
	{ 0x0056, "sceNpUtil", nullptr },
	{ 0x0057, "cellRudp", nullptr },
	{ 0x0059, "sceNpSns", &sceNpSns },
	{ 0x005a, "cellGem", &cellGem },
	{ 0xf00a, "cellCelpEnc", nullptr },
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
	{ 0xf02f, "cellSearch", nullptr },
	{ 0xf030, "cellAvchat2", nullptr },
	{ 0xf034, "cellSailRec", nullptr },
	{ 0xf035, "sceNpTrophy", &sceNpTrophy },
	{ 0xf053, "cellAdecAt3multi", nullptr },
	{ 0xf054, "cellLibatrac3multi", nullptr },

	{ -1, "cellSync", &cellSync },
	{ -1, "cellSysmodule", &cellSysmodule },
	{ -1, "libmixer", &libmixer },
	{ -1, "sysPrxForUser", &sysPrxForUser },
};

void ModuleManager::Init()
{
	if (!initialized)
	{
		clear_ppu_functions();

		for (auto& m : g_module_list)
		{
			if (m.module)
			{
				m.module->Init();
			}
		}

		initialized = true;
	}
}

ModuleManager::ModuleManager()
	: initialized(false)
{
}

ModuleManager::~ModuleManager()
{
}

void ModuleManager::Close()
{
	for (auto& m : g_module_list)
	{
		if (m.module && m.module->on_stop)
		{
			m.module->on_stop();
		}
	}

	initialized = false;
}

Module* ModuleManager::GetModuleByName(const char* name)
{
	for (auto& m : g_module_list)
	{
		if (!strcmp(name, m.name))
		{
			return m.module;
		}
	}

	return nullptr;
}

Module* ModuleManager::GetModuleById(u16 id)
{
	for (auto& m : g_module_list)
	{
		if (m.id == id)
		{
			return m.module;
		}
	}

	return nullptr;
}

bool ModuleManager::CheckModuleId(u16 id)
{
	for (auto& m : g_module_list)
	{
		if (m.id == id)
		{
			return true;
		}
	}

	return false;
}
