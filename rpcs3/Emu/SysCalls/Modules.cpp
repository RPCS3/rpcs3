#include "stdafx.h"
#include "SysCalls.h"
#include "SC_FUNC.h"

Module* g_modules[50];
uint g_modules_count = 0;
ArrayF<ModuleFunc> g_modules_funcs_list;

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

bool CallFunc(u32 id)
{
	for(u32 i=0; i<g_modules_funcs_list.GetCount(); ++i)
	{
		if(g_modules_funcs_list[i].id == id)
		{
			(*g_modules_funcs_list[i].func)();

			return true;
		}
	}

	return false;
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

void UnloadModules()
{
	for(u32 i=0; i<g_modules_count; ++i)
	{
		g_modules[i]->SetLoaded(false);
	}

	g_modules_funcs_list.Clear();
}

Module* GetModuleByName(const wxString& name)
{
	for(u32 i=0; i<g_modules_count; ++i)
	{
		if(g_modules[i]->GetName().Cmp(name) == 0)
		{
			return g_modules[i];
		}
	}

	return nullptr;
}

Module* GetModuleById(u16 id)
{
	for(u32 i=0; i<g_modules_count; ++i)
	{
		if(g_modules[i]->GetID() == id)
		{
			return g_modules[i];
		}
	}

	return nullptr;
}

Module::Module(const char* name, u16 id, void (*init)())
	: m_is_loaded(false)
	, m_name(name)
	, m_id(id)
{
	g_modules[g_modules_count++] = this;
	init();
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
