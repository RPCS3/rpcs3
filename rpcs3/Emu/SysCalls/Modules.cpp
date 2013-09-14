#include "stdafx.h"
#include "SysCalls.h"
#include "SC_FUNC.h"

Module* g_modules[3][0xff] = {0};
uint g_max_module_id = 0;
uint g_module_2_count = 0;
ArrayF<ModuleFunc> g_modules_funcs_list;

struct ModuleInfo
{
	u32 id;
	char* name;
}
static const g_module_list[] =
{
	{0x0000, "sys_net"},
	{0x0001, "sys_http"},
	{0x0002, "cellHttpUtil"},
	{0x0003, "cellSsl"},
	{0x0004, "cellHttps"},
	{0x0005, "cellVdec"},
	{0x0006, "cellAdec"},
	{0x0007, "cellDmux"},
	{0x0008, "cellVpost"},
	{0x0009, "cellRtc"},
	{0x000a, "cellSpurs"},
	{0x000b, "cellOvis"},
	{0x000c, "cellSheap"},
	{0x000d, "sys_sync"},
	{0x000e, "sys_fs"},
	{0x000f, "cellJpgDec"},
	{0x0010, "cellGcmSys"},
	{0x0011, "cellAudio"},
	{0x0012, "cellPamf"},
	{0x0013, "cellAtrac"},
	{0x0014, "cellNetCtl"},
	{0x0015, "cellSysutil"},
	{0x0016, "sceNp"},
	{0x0017, "sys_io"},
	{0x0018, "cellPngDec"},
	{0x0019, "cellFont"},
	{0x001a, "cellFontFt"},
	{0x001b, "cellFreetype"},
	{0x001c, "cellUsbd"},
	{0x001d, "cellSail"},
	{0x001e, "cellL10n"},
	{0x001f, "cellResc"},
	{0x0020, "cellDaisy"},
	{0x0021, "cellKey2char"},
	{0x0022, "cellMic"},
	{0x0023, "cellCamera"},
	{0x0024, "cellVdecMpeg2"},
	{0x0025, "cellVdecAvc"},
	{0x0026, "cellAdecLpcm"},
	{0x0027, "cellAdecAc3"},
	{0x0028, "cellAdecAtx"},
	{0x0029, "cellAdecAt3"},
	{0x002a, "cellDmuxPamf"},
	{0x002e, "cellLv2dbg"},
	{0x0030, "cellUsbpspcm"},
	{0x0031, "cellAvconfExt"},
	{0x0032, "cellSysutilUserinfo"},
	{0x0033, "cellSysutilSavedata"},
	{0x0034, "cellSubdisplay"},
	{0x0035, "cellSysutilRec"},
	{0x0036, "cellVideoExport"},
	{0x0037, "cellGameExec"},
	{0x0038, "sceNp2"},
	{0x0039, "cellSysutilAp"},
	{0x003a, "cellSysutilNpClans"},
	{0x003b, "cellSysutilOskExt"},
	{0x003c, "cellVdecDivx"},
	{0x003d, "cellJpgEnc"},
	{0x003e, "cellGame"},
	{0x003f, "cellBgdl"},
	{0x0040, "cellFreetypeTT"},
	{0x0041, "cellSysutilVideoUpload"},
	{0x0042, "cellSysutilSysconfExt"},
	{0x0043, "cellFiber"},
	{0x0044, "cellNpCommerce2"},
	{0x0045, "cellNpTus"},
	{0x0046, "cellVoice"},
	{0x0047, "cellAdecCelp8"},
	{0x0048, "cellCelp8Enc"},
	{0x0049, "cellLicenseArea"},
	{0x004a, "cellMusic2"},
	{0x004e, "cellScreenshot"},
	{0x004f, "cellMusicDecode"},
	{0x0050, "cellSpursJq"},
	{0x0052, "cellPngEnc"},
	{0x0053, "cellMusicDecode2"},
	{0x0055, "cellSync2"},
	{0x0056, "cellNpUtil"},
	{0x0057, "cellRudp"},
	{0x0059, "cellNpSns"},
	{0x005a, "cellGem"},
	{0xf00a, "cellCelpEnc"},
	{0xf010, "cellGifDec"},
	{0xf019, "cellAdecCelp"},
	{0xf01b, "cellAdecM2bc"},
	{0xf01d, "cellAdecM4aac"},
	{0xf01e, "cellAdecMp3"},
	{0xf023, "cellImejp"},
	{0xf028, "cellMusic"},
	{0xf029, "cellPhotoExport"},
	{0xf02a, "cellPrint"},
	{0xf02b, "cellPhotoImport"},
	{0xf02c, "cellMusicExport"},
	{0xf02e, "cellPhotoDecode"},
	{0xf02f, "cellSearch"},
	{0xf030, "cellAvchat2"},
	{0xf034, "cellSailRec"},
	{0xf035, "sceNpTrophy"},
	{0xf053, "cellAdecAt3multi"},
	{0xf054, "cellLibatrac3multi"},
};

struct _InitNullModules
{
	_InitNullModules()
	{
		for(auto& m : g_module_list)
		{
			new Module(m.id, m.name);
		}
	}
} InitNullModules;

bool IsLoadedFunc(u32 id)
{
	for(u32 i=0; i<g_modules_funcs_list.GetCount(); ++i)
	{
		if(g_modules_funcs_list[i].id == id)
		{
			return true;
		}
	}

	return false;
}

bool CallFunc(u32 num)
{
	if(num - 1024 >= g_modules_funcs_list.GetCount())
		return false;

	(*g_modules_funcs_list[num - 1024].func)();
	return true;
}

bool UnloadFunc(u32 id)
{
	for(u32 i=0; i<g_modules_funcs_list.GetCount(); ++i)
	{
		if(g_modules_funcs_list[i].id == id)
		{
			g_modules_funcs_list.RemoveAt(i);

			return true;
		}
	}

	return false;
}

u32 GetFuncNumById(u32 id)
{
	for(u32 i=0; i<g_modules_funcs_list.GetCount(); ++i)
	{
		if(g_modules_funcs_list[i].id == id)
		{
			return 1024 + i;
		}
	}

	return id;
}

void UnloadModules()
{
	for(u32 i=0; i<g_max_module_id; ++i)
	{
		if(g_modules[0][i])
		{
			g_modules[0][i]->SetLoaded(false);
		}

		if(g_modules[1][i])
		{
			g_modules[1][i]->SetLoaded(false);
		}

		if(g_modules[2][i])
		{
			g_modules[2][i]->SetLoaded(false);
		}
	}

	g_modules_funcs_list.Clear();
}

Module* GetModuleByName(const wxString& name)
{
	for(u32 i=0; i<g_max_module_id; ++i)
	{
		if(g_modules[0][i] && g_modules[0][i]->GetName().Cmp(name) == 0)
		{
			return g_modules[0][i];
		}

		if(g_modules[1][i] && g_modules[1][i]->GetName().Cmp(name) == 0)
		{
			return g_modules[1][i];
		}

		if(g_modules[2][i] && g_modules[2][i]->GetName().Cmp(name) == 0)
		{
			return g_modules[2][i];
		}
	}

	return nullptr;
}

Module* GetModuleById(u16 id)
{
	for(u32 i=0; i<g_max_module_id; ++i)
	{
		if(g_modules[0][i] && g_modules[0][i]->GetID() == id)
		{
			return g_modules[0][i];
		}

		if(g_modules[1][i] && g_modules[1][i]->GetID() == id)
		{
			return g_modules[1][i];
		}
	}

	return nullptr;
}

void SetModule(int id, Module* module, bool with_data)
{
	if(id != 0xffff)
	{
		if(u16((u8)id + 1) > g_max_module_id)
		{
			g_max_module_id = u16((u8)id + 1);
		}

		int index;
		switch(id >> 8)
		{
		case 0x00: index = 0; break;
		case 0xf0: index = 1; break;
		default: assert(0); return;
		}

		if(g_modules[index][(u8)id])
		{
			if(with_data)
			{
				module->SetName(g_modules[index][(u8)id]->GetName());
				delete g_modules[index][(u8)id];
				g_modules[index][(u8)id] = module;
			}
			else
			{
				g_modules[index][(u8)id]->SetName(module->GetName());
				delete module;
			}
		}
		else
		{
			g_modules[index][(u8)id] = module;
		}
	}
	else
	{
		g_modules[2][g_module_2_count++] = module;

		if(g_module_2_count > g_max_module_id)
		{
			g_max_module_id = g_module_2_count;
		}
	}
}

Module::Module(u16 id, const char* name)
	: m_is_loaded(false)
	, m_name(name)
	, m_id(id)
{
	SetModule(m_id, this, false);
}

Module::Module(const char* name, void (*init)())
	: m_is_loaded(false)
	, m_name(name)
	, m_id(-1)
{
	SetModule(m_id, this, init != nullptr);
	if(init) init();
}

Module::Module(u16 id, void (*init)())
	: m_is_loaded(false)
	, m_id(id)
{
	SetModule(m_id, this, init != nullptr);
	if(init) init();
}

void Module::Load()
{
	for(u32 i=0; i<m_funcs_list.GetCount(); ++i)
	{
		if(IsLoadedFunc(m_funcs_list[i].id)) continue;

		g_modules_funcs_list.Add(m_funcs_list[i]);
	}
}

void Module::UnLoad()
{
	for(u32 i=0; i<m_funcs_list.GetCount(); ++i)
	{
		UnloadFunc(m_funcs_list[i].id);
	}
}

bool Module::Load(u32 id)
{
	if(IsLoadedFunc(id)) return false;

	for(u32 i=0; i<m_funcs_list.GetCount(); ++i)
	{
		if(m_funcs_list[i].id == id)
		{
			g_modules_funcs_list.Add(m_funcs_list[i]);

			return true;
		}
	}

	return false;
}

bool Module::UnLoad(u32 id)
{
	return UnloadFunc(id);
}

void Module::SetLoaded(bool loaded)
{
	m_is_loaded = loaded;
}

bool Module::IsLoaded() const
{
	return m_is_loaded;
}

u16 Module::GetID() const
{
	return m_id;
}

wxString Module::GetName() const
{
	return m_name;
}

void Module::SetName(const wxString& name)
{
	m_name = name;
}

void Module::Log(const u32 id, wxString fmt, ...)
{
	if(enable_log)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + wxString::Format("[%d]: ", id) + wxString::FormatV(fmt, list));
		va_end(list);
	}
}

void Module::Log(wxString fmt, ...)
{
	if(enable_log)
	{
		va_list list;
		va_start(list, fmt);
		ConLog.Write(GetName() + ": " + wxString::FormatV(fmt, list));
		va_end(list);
	}
}

void Module::Warning(const u32 id, wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	ConLog.Warning(GetName() + wxString::Format("[%d] warning: ", id) + wxString::FormatV(fmt, list));
	va_end(list);
}

void Module::Warning(wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	ConLog.Warning(GetName() + " warning: " + wxString::FormatV(fmt, list));
	va_end(list);
}

void Module::Error(const u32 id, wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	ConLog.Error(GetName() + wxString::Format("[%d] error: ", id) + wxString::FormatV(fmt, list));
	va_end(list);
}

void Module::Error(wxString fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	ConLog.Error(GetName() + " error: " + wxString::FormatV(fmt, list));
	va_end(list);
}

bool Module::CheckId(u32 id) const
{
	return Emu.GetIdManager().CheckID(id) && !Emu.GetIdManager().GetIDData(id).m_name.Cmp(GetName());
}

bool Module::CheckId(u32 id, ID& _id) const
{
	return Emu.GetIdManager().CheckID(id) && !(_id = Emu.GetIdManager().GetIDData(id)).m_name.Cmp(GetName());
}

template<typename T> bool Module::CheckId(u32 id, T*& data)
{
	ID id_data;

	if(!CheckId(id, id_data)) return false;

	data = (T*)id_data.m_data;

	return true;
}

u32 Module::GetNewId(void* data, u8 flags)
{
	return Emu.GetIdManager().GetNewID(GetName(), data, flags);
}
