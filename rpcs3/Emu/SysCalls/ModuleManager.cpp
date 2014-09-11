#include "stdafx.h"
#include "ModuleManager.h"

extern void cellAdec_init();
extern Module* cellAdec;
extern void cellAtrac_init();
extern Module *cellAtrac;
extern void cellAudio_init();
extern Module *cellAudio;
extern void cellCamera_init();
extern Module *cellCamera;
extern void cellDmux_init();
extern Module *cellDmux;
extern void cellFiber_init();
extern Module *cellFiber;
extern void cellFont_init();
extern void cellFont_load();
extern void cellFont_unload();
extern Module *cellFont;
extern void cellFontFT_init();
extern void cellFontFT_load();
extern void cellFontFT_unload();
extern Module *cellFontFT;
extern void cellGame_init();
extern Module *cellGame;
extern void cellGcmSys_init();
extern void cellGcmSys_load();
extern void cellGcmSys_unload();
extern Module *cellGcmSys;
extern void cellGem_init();
extern Module *cellGem;
extern void cellGifDec_init();
extern Module *cellGifDec;
extern void cellJpgDec_init();
extern Module *cellJpgDec;
extern void cellL10n_init();
extern Module *cellL10n;
extern void cellNetCtl_init();
extern Module *cellNetCtl;
extern void cellOvis_init();
extern Module *cellOvis;
extern void cellPamf_init();
extern Module *cellPamf;
extern void cellPngDec_init();
extern Module *cellPngDec;
extern void cellResc_init();
extern void cellResc_load();
extern void cellResc_unload();
extern Module *cellResc;
extern void cellRtc_init();
extern Module *cellRtc;
extern void cellSail_init();
extern Module *cellSail;
extern void cellSpurs_init();
extern Module *cellSpurs;
extern void cellSync_init();
extern Module *cellSpursJq;
extern void cellSpursJq_init();
extern Module *cellSync;
extern void cellSync2_init();
extern Module *cellSync2;
extern void cellSysmodule_init();
extern Module *cellSysmodule;
extern void cellSysutil_init();
extern void cellSysutil_load();
extern Module *cellSysutil;
extern void cellSysutilAp_init();
extern Module *cellSysutilAp;
extern void cellUserInfo_init();
extern Module *cellUserInfo;
extern void cellVdec_init();
extern Module *cellVdec;
extern void cellVpost_init();
extern Module *cellVpost;
extern void libmixer_init();
extern Module *libmixer;
extern void sceNp_init();
extern Module *sceNp;
extern void sceNpClans_unload();
extern void sceNpClans_init();
extern Module *sceNpClans;
extern void sceNpCommerce2_unload();
extern void sceNpCommerce2_init();
extern Module *sceNpCommerce2;
extern void sceNpSns_unload();
extern void sceNpSns_init();
extern Module *sceNpSns;
extern void sceNpTrophy_unload();
extern void sceNpTrophy_init();
extern Module *sceNpTrophy;
extern void sceNpTus_unload();
extern void sceNpTus_init();
extern Module *sceNpTus;
extern void sysPrxForUser_init();
extern void sysPrxForUser_load();
extern Module *sysPrxForUser;
extern void sys_fs_init();
extern void sys_fs_load();
extern Module *sys_fs;
extern void sys_io_init();
extern Module *sys_io;
extern void sys_net_init();
extern Module *sys_net;

struct ModuleInfo
{
	u32 id;
	const char* name;
}
static const g_module_list[] =
{
	{ 0x0000, "sys_net" },
	{ 0x0001, "sys_http" },
	{ 0x0002, "cellHttpUtil" },
	{ 0x0003, "cellSsl" },
	{ 0x0004, "cellHttps" },
	{ 0x0005, "libvdec" },
	{ 0x0006, "cellAdec" },
	{ 0x0007, "cellDmux" },
	{ 0x0008, "cellVpost" },
	{ 0x0009, "cellRtc" },
	{ 0x000a, "cellSpurs" },
	{ 0x000b, "cellOvis" },
	{ 0x000c, "cellSheap" },
	{ 0x000d, "sys_sync" },
	{ 0x000e, "sys_fs" },
	{ 0x000f, "cellJpgDec" },
	{ 0x0010, "cellGcmSys" },
	{ 0x0011, "cellAudio" },
	{ 0x0012, "cellPamf" },
	{ 0x0013, "cellAtrac" },
	{ 0x0014, "cellNetCtl" },
	{ 0x0015, "cellSysutil" },
	{ 0x0016, "sceNp" },
	{ 0x0017, "sys_io" },
	{ 0x0018, "cellPngDec" },
	{ 0x0019, "cellFont" },
	{ 0x001a, "cellFontFT" },
	{ 0x001b, "cellFreetype" },
	{ 0x001c, "cellUsbd" },
	{ 0x001d, "cellSail" },
	{ 0x001e, "cellL10n" },
	{ 0x001f, "cellResc" },
	{ 0x0020, "cellDaisy" },
	{ 0x0021, "cellKey2char" },
	{ 0x0022, "cellMic" },
	{ 0x0023, "cellCamera" },
	{ 0x0024, "cellVdecMpeg2" },
	{ 0x0025, "cellVdecAvc" },
	{ 0x0026, "cellAdecLpcm" },
	{ 0x0027, "cellAdecAc3" },
	{ 0x0028, "cellAdecAtx" },
	{ 0x0029, "cellAdecAt3" },
	{ 0x002a, "cellDmuxPamf" },
	{ 0x002e, "cellLv2dbg" },
	{ 0x0030, "cellUsbpspcm" },
	{ 0x0031, "cellAvconfExt" },
	{ 0x0032, "cellUserInfo" },
	{ 0x0033, "cellSysutilSavedata" },
	{ 0x0034, "cellSubdisplay" },
	{ 0x0035, "cellSysutilRec" },
	{ 0x0036, "cellVideoExport" },
	{ 0x0037, "cellGameExec" },
	{ 0x0038, "sceNp2" },
	{ 0x0039, "cellSysutilAp" },
	{ 0x003a, "cellSysutilNpClans" },
	{ 0x003b, "cellSysutilOskExt" },
	{ 0x003c, "cellVdecDivx" },
	{ 0x003d, "cellJpgEnc" },
	{ 0x003e, "cellGame" },
	{ 0x003f, "cellBgdl" },
	{ 0x0040, "cellFreetypeTT" },
	{ 0x0041, "cellSysutilVideoUpload" },
	{ 0x0042, "cellSysutilSysconfExt" },
	{ 0x0043, "cellFiber" },
	{ 0x0044, "cellNpCommerce2" },
	{ 0x0045, "cellNpTus" },
	{ 0x0046, "cellVoice" },
	{ 0x0047, "cellAdecCelp8" },
	{ 0x0048, "cellCelp8Enc" },
	{ 0x0049, "cellLicenseArea" },
	{ 0x004a, "cellMusic2" },
	{ 0x004e, "cellScreenshot" },
	{ 0x004f, "cellMusicDecode" },
	{ 0x0050, "cellSpursJq" },
	{ 0x0052, "cellPngEnc" },
	{ 0x0053, "cellMusicDecode2" },
	{ 0x0055, "cellSync2" },
	{ 0x0056, "cellNpUtil" },
	{ 0x0057, "cellRudp" },
	{ 0x0059, "cellNpSns" },
	{ 0x005a, "cellGem" },
	{ 0xf00a, "cellCelpEnc" },
	{ 0xf010, "cellGifDec" },
	{ 0xf019, "cellAdecCelp" },
	{ 0xf01b, "cellAdecM2bc" },
	{ 0xf01d, "cellAdecM4aac" },
	{ 0xf01e, "cellAdecMp3" },
	{ 0xf023, "cellImejp" },
	{ 0xf028, "cellMusic" },
	{ 0xf029, "cellPhotoExport" },
	{ 0xf02a, "cellPrint" },
	{ 0xf02b, "cellPhotoImport" },
	{ 0xf02c, "cellMusicExport" },
	{ 0xf02e, "cellPhotoDecode" },
	{ 0xf02f, "cellSearch" },
	{ 0xf030, "cellAvchat2" },
	{ 0xf034, "cellSailRec" },
	{ 0xf035, "sceNpTrophy" },
	{ 0xf053, "cellAdecAt3multi" },
	{ 0xf054, "cellLibatrac3multi" }
};

void ModuleManager::init()
{
	//this is a really bad hack and the whole idea of Modules and how they're implemented should probably be changed
	//the contruction of the modules accessses the global variable for that module.
	//For example cellAdec needs to be set before cellAdec_init is called but it would be called in the constructor of
	//cellAdec, so we need to point cellAdec to where we will construct it in the future
	if (!initialized)
	{
		m_mod_init.reserve(m_mod_init.size() + 160);//currently 131
		for (auto& m : g_module_list)
		{
			m_mod_init.emplace_back(m.id, m.name);
		}
		cellAdec = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0006, cellAdec_init);
		cellAtrac = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0013, cellAtrac_init);
		cellAudio = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0011, cellAudio_init);
		cellCamera = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0023, cellCamera_init);
		cellDmux = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0007, cellDmux_init);
		cellFiber = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0043, cellFiber_init);
		cellFont = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0019, cellFont_init, cellFont_load, cellFont_unload);
		cellFontFT = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x001a, cellFontFT_init, cellFontFT_load, cellFontFT_unload);
		cellGame = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x003e, cellGame_init);
		cellGcmSys = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0010, cellGcmSys_init, cellGcmSys_load, cellGcmSys_unload);
		cellGem = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x005a, cellGem_init);
		cellJpgDec = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x000f, cellJpgDec_init);
		cellGifDec = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0xf010, cellGifDec_init);
		cellL10n = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x001e, cellL10n_init);
		cellNetCtl = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0014, cellNetCtl_init);
		cellOvis = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x000b, cellOvis_init);
		cellPamf = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0012, cellPamf_init);
		cellPngDec = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0018, cellPngDec_init);
		cellResc = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x001f, cellResc_init, cellResc_load, cellResc_unload);
		cellRtc = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0009, cellRtc_init);
		cellSail = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back("cellSail", cellSail_init);
		cellSpurs = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x000a, cellSpurs_init);
		cellSpursJq = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0050, cellSpursJq_init);
		cellSync = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back("cellSync", cellSync_init);
		cellSync2 = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0055, cellSync2_init);
		cellSysutil = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0015, cellSysutil_init, cellSysutil_load, nullptr);
		cellSysutilAp = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0039, cellSysutilAp_init);
		cellSysmodule = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back("cellSysmodule", cellSysmodule_init);
		cellUserInfo = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0032, cellUserInfo_init);
		cellVdec = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0005, cellVdec_init);
		cellVpost = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0008, cellVpost_init);
		libmixer = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back("libmixer", libmixer_init);
		sceNp = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0016, sceNp_init);
		sceNpClans = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x003a, sceNpClans_init, nullptr, sceNpClans_unload);
		sceNpCommerce2 = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0044, sceNpCommerce2_init, nullptr, sceNpCommerce2_unload);
		sceNpSns = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0059, sceNpSns_init, nullptr, sceNpSns_unload);
		sceNpTrophy = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0xf035, sceNpTrophy_init, nullptr, sceNpTrophy_unload);
		sceNpTus = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0045, sceNpTus_init, nullptr, sceNpTus_unload);
		sysPrxForUser = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back("sysPrxForUser", sysPrxForUser_init, sysPrxForUser_load, nullptr);
		sys_fs = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x000e, sys_fs_init, sys_fs_load, nullptr);
		sys_io = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back(0x0017, sys_io_init);
		sys_net = static_cast <Module*>(&(m_mod_init.back())) + 1;
		m_mod_init.emplace_back((u16)0x0000, sys_net_init);
		initialized = true;
	}
}

ModuleManager::ModuleManager() :
m_max_module_id(0),
m_module_2_count(0),
initialized(false)
{
	memset(m_modules, 0, 3 * 0xFF * sizeof(Module*));
}

ModuleManager::~ModuleManager()
{
	UnloadModules();
}

bool ModuleManager::IsLoadedFunc(u32 id) const
{
	for (u32 i = 0; i<m_modules_funcs_list.size(); ++i)
	{
		if (m_modules_funcs_list[i]->id == id)
		{
			return true;
		}
	}

	return false;
}

bool ModuleManager::CallFunc(u32 num)
{
	func_caller* func = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_funcs_lock);

		for (u32 i = 0; i<m_modules_funcs_list.size(); ++i)
		{
			if (m_modules_funcs_list[i]->id == num)
			{
				func = m_modules_funcs_list[i]->func;
				break;
			}
		}
	}

	if (func)
	{
		(*func)();
		return true;
	}
	return false;
}

bool ModuleManager::UnloadFunc(u32 id)
{
	std::lock_guard<std::mutex> lock(m_funcs_lock);

	for (u32 i = 0; i<m_modules_funcs_list.size(); ++i)
	{
		if (m_modules_funcs_list[i]->id == id)
		{
			m_modules_funcs_list.erase(m_modules_funcs_list.begin() + i);

			return true;
		}
	}

	return false;
}

u32 ModuleManager::GetFuncNumById(u32 id)
{
	return id;
}

//to load the default modules after calling this call Init() again
void ModuleManager::UnloadModules()
{
	for (u32 i = 0; i<3; ++i)
	{
		for (u32 j = 0; j<m_max_module_id; ++j)
		{
			if (m_modules[i][j])
			{
				m_modules[i][j]->UnLoad();
			}
		}
	}

	//reset state of the module manager
	//this could be done by calling the destructor and then a placement-new
	//to avoid repeating the initial values here but the defaults aren't
	//complicated enough to complicate this by using the placement-new
	m_mod_init.clear();
	m_max_module_id = 0;
	m_module_2_count = 0;
	initialized = false;
	memset(m_modules, 0, 3 * 0xFF * sizeof(Module*));
	
	std::lock_guard<std::mutex> lock(m_funcs_lock);
	m_modules_funcs_list.clear();
}

Module* ModuleManager::GetModuleByName(const std::string& name)
{
	for (u32 i = 0; i<m_max_module_id; ++i)
	{
		if (m_modules[0][i] && m_modules[0][i]->GetName() == name)
		{
			return m_modules[0][i];
		}

		if (m_modules[1][i] && m_modules[1][i]->GetName() == name)
		{
			return m_modules[1][i];
		}

		if (m_modules[2][i] && m_modules[2][i]->GetName() == name)
		{
			return m_modules[2][i];
		}
	}

	return nullptr;
}

Module* ModuleManager::GetModuleById(u16 id)
{
	for (u32 i = 0; i<m_max_module_id; ++i)
	{
		if (m_modules[0][i] && m_modules[0][i]->GetID() == id)
		{
			return m_modules[0][i];
		}

		if (m_modules[1][i] && m_modules[1][i]->GetID() == id)
		{
			return m_modules[1][i];
		}
	}

	return nullptr;
}

void ModuleManager::SetModule(int id, Module* module, bool with_data)
{
	if (id != 0xffff)
	{
		if (u16((u8)id + 1) > m_max_module_id)
		{
			m_max_module_id = u16((u8)id + 1);
		}

		int index;
		switch (id >> 8)
		{
		case 0x00: index = 0; break;
		case 0xf0: index = 1; break;
		default: assert(0); return;
		}

		if (m_modules[index][(u8)id])
		{
			if (with_data)
			{
				module->SetName(m_modules[index][(u8)id]->GetName());
				// delete m_modules[index][(u8)id];
				m_modules[index][(u8)id] = module;
			}
			else
			{
				m_modules[index][(u8)id]->SetName(module->GetName());
				// delete module;
			}
		}
		else
		{
			m_modules[index][(u8)id] = module;
		}
	}
	else
	{
		m_modules[2][m_module_2_count++] = module;

		if (m_module_2_count > m_max_module_id)
		{
			m_max_module_id = m_module_2_count;
		}
	}
}


void ModuleManager::AddFunc(ModuleFunc *func)
{
	std::lock_guard<std::mutex> guard(m_funcs_lock);

	if (!IsLoadedFunc(func->id))
	{
		m_modules_funcs_list.push_back(func);
	}
}