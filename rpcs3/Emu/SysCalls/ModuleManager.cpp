#include "stdafx.h"
#include "ModuleManager.h"

extern void cellAdec_init(Module* pxThis);
extern void cellAtrac_init(Module* pxThis);
extern void cellAudio_init(Module* pxThis);
extern void cellAvconfExt_init(Module* pxThis);
extern void cellCamera_init(Module* pxThis);
extern void cellCamera_unload();
extern void cellDmux_init(Module *pxThis);
extern void cellFiber_init(Module *pxThis);
extern void cellFont_init(Module *pxThis);
extern void cellFont_load();
extern void cellFont_unload();
extern void cellFontFT_init(Module *pxThis);
extern void cellFontFT_load();
extern void cellFontFT_unload();
extern void cellGame_init(Module *pxThis);
extern void cellGcmSys_init(Module *pxThis);
extern void cellGcmSys_load();
extern void cellGcmSys_unload();
extern void cellGem_init(Module *pxThis);
extern void cellGem_unload();
extern void cellJpgDec_init(Module *pxThis);
extern void cellGifDec_init(Module *pxThis);
extern void cellL10n_init(Module *pxThis);
extern void cellMic_init(Module *pxThis);
extern void cellMic_unload();
extern void cellNetCtl_init(Module *pxThis);
extern void cellNetCtl_unload();
extern void cellOvis_init(Module *pxThis);
extern void cellPamf_init(Module *pxThis);
extern void cellPngDec_init(Module *pxThis);
extern void cellResc_init(Module *pxThis);
extern void cellResc_load();
extern void cellResc_unload();
extern void cellRtc_init(Module *pxThis);
extern void cellSail_init(Module *pxThis);
extern void cellSpurs_init(Module *pxThis);
extern void cellSpursJq_init(Module *pxThis);
extern void cellSync_init(Module *pxThis);
extern void cellSync2_init(Module *pxThis);
extern void cellSysutil_init(Module *pxThis);
extern void cellSysutil_load();
extern void cellSysutilAp_init(Module *pxThis);
extern void cellSysmodule_init(Module *pxThis);
extern void cellUserInfo_init(Module *pxThis);
extern void cellVdec_init(Module *pxThis);
extern void cellVpost_init(Module *pxThis);
extern void libmixer_init(Module *pxThis);
extern void sceNp_init(Module *pxThis);
extern void sceNp_unload();
extern void sceNpClans_init(Module *pxThis);
extern void sceNpClans_unload();
extern void sceNpCommerce2_init(Module *pxThis);
extern void sceNpCommerce2_unload();
extern void sceNpSns_init(Module *pxThis);
extern void sceNpSns_unload();
extern void sceNpTrophy_init(Module *pxThis);
extern void sceNpTrophy_unload();
extern void sceNpTus_init(Module *pxThis);
extern void sceNpTus_unload();
extern void sysPrxForUser_init(Module *pxThis);
extern void sysPrxForUser_load();
extern void sys_fs_init(Module *pxThis);
extern void sys_fs_load();
extern void sys_io_init(Module *pxThis);
extern void sys_net_init(Module *pxThis);

struct ModuleInfo
{
	u16 id; //0xffff is used by module with only name
	const char* name;
	void(*init)(Module *pxThis);
	void(*load)();
	void(*unload)();
}
static const g_modules_list[] =
{
	{ 0x0000, "sys_net", sys_net_init, nullptr, nullptr },
	{ 0x0001, "sys_http", nullptr, nullptr, nullptr },
	{ 0x0002, "cellHttpUtil", nullptr, nullptr, nullptr },
	{ 0x0003, "cellSsl", nullptr, nullptr, nullptr },
	{ 0x0004, "cellHttps", nullptr, nullptr, nullptr },
	{ 0x0005, "libvdec", cellVdec_init, nullptr, nullptr },
	{ 0x0006, "cellAdec", cellAdec_init, nullptr, nullptr },
	{ 0x0007, "cellDmux", cellDmux_init, nullptr, nullptr },
	{ 0x0008, "cellVpost", cellVpost_init, nullptr, nullptr },
	{ 0x0009, "cellRtc", cellRtc_init, nullptr, nullptr },
	{ 0x000a, "cellSpurs", cellSpurs_init, nullptr, nullptr },
	{ 0x000b, "cellOvis", cellOvis_init, nullptr, nullptr },
	{ 0x000c, "cellSheap", nullptr, nullptr, nullptr },
	{ 0x000d, "sys_sync", nullptr, nullptr, nullptr },
	{ 0x000e, "sys_fs", sys_fs_init, sys_fs_load, nullptr },
	{ 0x000f, "cellJpgDec", cellJpgDec_init, nullptr, nullptr },
	{ 0x0010, "cellGcmSys", cellGcmSys_init, cellGcmSys_load, cellGcmSys_unload },
	{ 0x0011, "cellAudio", cellAudio_init, nullptr, nullptr },
	{ 0x0012, "cellPamf", cellPamf_init, nullptr, nullptr },
	{ 0x0013, "cellAtrac", cellAtrac_init, nullptr, nullptr },
	{ 0x0014, "cellNetCtl", cellNetCtl_init, nullptr, cellNetCtl_unload },
	{ 0x0015, "cellSysutil", cellSysutil_init, cellSysutil_load, nullptr },
	{ 0x0016, "sceNp", sceNp_init, nullptr, sceNp_unload },
	{ 0x0017, "sys_io", sys_io_init, nullptr, nullptr },
	{ 0x0018, "cellPngDec", cellPngDec_init, nullptr, nullptr },
	{ 0x0019, "cellFont", cellFont_init, cellFont_load, cellFont_unload },
	{ 0x001a, "cellFontFT", cellFontFT_init, cellFontFT_load, cellFontFT_unload },
	{ 0x001b, "cellFreetype", nullptr, nullptr, nullptr },
	{ 0x001c, "cellUsbd", nullptr, nullptr, nullptr },
	{ 0x001d, "cellSail", cellSail_init, nullptr, nullptr },
	{ 0x001e, "cellL10n", cellL10n_init, nullptr, nullptr },
	{ 0x001f, "cellResc", cellResc_init, cellResc_load, cellResc_unload },
	{ 0x0020, "cellDaisy", nullptr, nullptr, nullptr },
	{ 0x0021, "cellKey2char", nullptr, nullptr, nullptr },
	{ 0x0022, "cellMic", cellMic_init, nullptr, cellMic_unload },
	{ 0x0023, "cellCamera", cellCamera_init, nullptr, cellCamera_unload },
	{ 0x0024, "cellVdecMpeg2", nullptr, nullptr, nullptr },
	{ 0x0025, "cellVdecAvc", nullptr, nullptr, nullptr },
	{ 0x0026, "cellAdecLpcm", nullptr, nullptr, nullptr },
	{ 0x0027, "cellAdecAc3", nullptr, nullptr, nullptr },
	{ 0x0028, "cellAdecAtx", nullptr, nullptr, nullptr },
	{ 0x0029, "cellAdecAt3", nullptr, nullptr, nullptr },
	{ 0x002a, "cellDmuxPamf", nullptr, nullptr, nullptr },
	{ 0x002e, "cellLv2dbg", nullptr, nullptr, nullptr },
	{ 0x0030, "cellUsbpspcm", nullptr, nullptr, nullptr },
	{ 0x0031, "cellAvconfExt", cellAvconfExt_init, nullptr, nullptr },
	{ 0x0032, "cellUserInfo", cellUserInfo_init, nullptr, nullptr },
	{ 0x0033, "cellSysutilSavedata", nullptr, nullptr, nullptr },
	{ 0x0034, "cellSubdisplay", nullptr, nullptr, nullptr },
	{ 0x0035, "cellSysutilRec", nullptr, nullptr, nullptr },
	{ 0x0036, "cellVideoExport", nullptr, nullptr, nullptr },
	{ 0x0037, "cellGameExec", nullptr, nullptr, nullptr },
	{ 0x0038, "sceNp2", nullptr, nullptr, nullptr },
	{ 0x0039, "cellSysutilAp", cellSysutilAp_init, nullptr, nullptr },
	{ 0x003a, "cellSysutilNpClans", sceNpClans_init, nullptr, sceNpClans_unload },
	{ 0x003b, "cellSysutilOskExt", nullptr, nullptr, nullptr },
	{ 0x003c, "cellVdecDivx", nullptr, nullptr, nullptr },
	{ 0x003d, "cellJpgEnc", nullptr, nullptr, nullptr },
	{ 0x003e, "cellGame", cellGame_init, nullptr, nullptr },
	{ 0x003f, "cellBgdl", nullptr, nullptr, nullptr },
	{ 0x0040, "cellFreetypeTT", nullptr, nullptr, nullptr },
	{ 0x0041, "cellSysutilVideoUpload", nullptr, nullptr, nullptr },
	{ 0x0042, "cellSysutilSysconfExt", nullptr, nullptr, nullptr },
	{ 0x0043, "cellFiber", cellFiber_init, nullptr, nullptr },
	{ 0x0044, "cellNpCommerce2", sceNpCommerce2_init, nullptr, sceNpCommerce2_unload },
	{ 0x0045, "cellNpTus", sceNpTus_init, nullptr, sceNpTus_unload },
	{ 0x0046, "cellVoice", nullptr, nullptr, nullptr },
	{ 0x0047, "cellAdecCelp8", nullptr, nullptr, nullptr },
	{ 0x0048, "cellCelp8Enc", nullptr, nullptr, nullptr },
	{ 0x0049, "cellLicenseArea", nullptr, nullptr, nullptr },
	{ 0x004a, "cellMusic2", nullptr, nullptr, nullptr },
	{ 0x004e, "cellScreenshot", nullptr, nullptr, nullptr },
	{ 0x004f, "cellMusicDecode", nullptr, nullptr, nullptr },
	{ 0x0050, "cellSpursJq", cellSpursJq_init, nullptr, nullptr },
	{ 0x0052, "cellPngEnc", nullptr, nullptr, nullptr },
	{ 0x0053, "cellMusicDecode2", nullptr, nullptr, nullptr },
	{ 0x0055, "cellSync2", cellSync2_init, nullptr, nullptr },
	{ 0x0056, "cellNpUtil", nullptr, nullptr, nullptr },
	{ 0x0057, "cellRudp", nullptr, nullptr, nullptr },
	{ 0x0059, "cellNpSns", sceNpSns_init, nullptr, sceNpSns_unload },
	{ 0x005a, "cellGem", cellGem_init, nullptr, cellGem_unload },
	{ 0xf00a, "cellCelpEnc", nullptr, nullptr, nullptr },
	{ 0xf010, "cellGifDec", cellGifDec_init, nullptr, nullptr },
	{ 0xf019, "cellAdecCelp", nullptr, nullptr, nullptr },
	{ 0xf01b, "cellAdecM2bc", nullptr, nullptr, nullptr },
	{ 0xf01d, "cellAdecM4aac", nullptr, nullptr, nullptr },
	{ 0xf01e, "cellAdecMp3", nullptr, nullptr, nullptr },
	{ 0xf023, "cellImejp", nullptr, nullptr, nullptr },
	{ 0xf028, "cellMusic", nullptr, nullptr, nullptr },
	{ 0xf029, "cellPhotoExport", nullptr, nullptr, nullptr },
	{ 0xf02a, "cellPrint", nullptr, nullptr, nullptr },
	{ 0xf02b, "cellPhotoImport", nullptr, nullptr, nullptr },
	{ 0xf02c, "cellMusicExport", nullptr, nullptr, nullptr },
	{ 0xf02e, "cellPhotoDecode", nullptr, nullptr, nullptr },
	{ 0xf02f, "cellSearch", nullptr, nullptr, nullptr },
	{ 0xf030, "cellAvchat2", nullptr, nullptr, nullptr },
	{ 0xf034, "cellSailRec", nullptr, nullptr, nullptr },
	{ 0xf035, "sceNpTrophy", sceNpTrophy_init, nullptr, sceNpTrophy_unload },
	{ 0xf053, "cellAdecAt3multi", nullptr, nullptr, nullptr },
	{ 0xf054, "cellLibatrac3multi", nullptr, nullptr, nullptr },
	{ 0xffff, "cellSync", cellSync_init, nullptr, nullptr },
	{ 0xffff, "cellSysmodule", cellSysmodule_init, nullptr, nullptr },
	{ 0xffff, "libmixer", libmixer_init, nullptr, nullptr },
	{ 0xffff, "sysPrxForUser", sysPrxForUser_init, sysPrxForUser_load, nullptr }
};

void ModuleManager::init()
{
	//TODO Refactor every Module part to remove the global pointer defined in each module's file
	//and the need to call init functions after its constructor

	//To define a new module, add it in g_modules_list
	//m.init the function which defines module's own functions and assignes module's pointer to its global pointer
	if (!initialized)
	{
		u32 global_module_nb = sizeof(g_modules_list) / sizeof(g_modules_list[0]);
		m_mod_init.reserve(global_module_nb);
		for (auto& m : g_modules_list)
		{
			m_mod_init.emplace_back(m.id, m.name, m.load, m.unload);
			if (m.init)
				m.init(&m_mod_init.back());
		}

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

bool ModuleManager::CallFunc(PPUThread& CPU, u32 num)
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
		(*func)(CPU);
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
	m_mod_init.clear(); //destroy Module(s), their respective global pointer become invalid
	m_max_module_id = 0;
	m_module_2_count = 0;
	initialized = false;
	memset(m_modules, 0, 3 * 0xFF * sizeof(Module*));
	
	std::lock_guard<std::mutex> lock(m_funcs_lock);
	m_modules_funcs_list.clear();
}

Module* ModuleManager::GetModuleByName(const std::string& name)
{
	for (u32 i = 0; i<m_max_module_id && i<0xff; ++i)
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
	for (u32 i = 0; i<m_max_module_id && i<0xff; ++i)
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

void ModuleManager::SetModule(u16 id, Module* module)
{
	if (id != 0xffff) //id != -1
	{
		u8 index2 = (u8)id;
		if (u16(index2 + 1) > m_max_module_id)
		{
			m_max_module_id = u16(index2 + 1);
		}

		u8 index;
		switch (id >> 8)
		{
		case 0x00: index = 0; break;//id = 0x0000 to 0x00fe go to m_modules[0]
		case 0xf0: index = 1; break;//id = 0xf000 to 0xf0fe go to m_modules[1]
		default: assert(0); return;
		}

		//fill m_modules[index] by truncating id to 8 bits
		if (m_modules[index][index2]) //if module id collision
		{
			//Not sure if this is a good idea to hide collision
			module->SetName(m_modules[index][index2]->GetName());
			m_modules[index][index2] = module;
			//don't need to delete since m_mod_init has the ownership
		}
		else
		{
			m_modules[index][index2] = module;
		}
	}
	else //id = 0xffff go to m_modules[2]
	{
		//fill m_modules[2] from 0 to 0xff
		m_modules[2][m_module_2_count] = module;
		++m_module_2_count;
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