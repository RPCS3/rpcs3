#pragma once
#include "Modules/cellResc.h"

#define declCPU PPUThread& CPU = GetCurrentPPUThread

class func_caller
{
public:
	virtual void operator()() = 0;
};

//TODO
struct ModuleFunc
{
	u32 id;
	func_caller* func;

	ModuleFunc(u32 id, func_caller* func)
		: id(id)
		, func(func)
	{
	}
};

class Module
{
	wxString m_name;
	const u16 m_id;
	bool m_is_loaded;
	void (*m_load_func)();
	void (*m_unload_func)();

public:
	Array<ModuleFunc> m_funcs_list;

	Module(u16 id, const char* name);
	Module(const char* name, void (*init)(), void (*load)() = nullptr, void (*unload)() = nullptr);
	Module(u16 id, void (*init)(), void (*load)() = nullptr, void (*unload)() = nullptr);

	void Load();
	void UnLoad();
	bool Load(u32 id);
	bool UnLoad(u32 id);

	void SetLoaded(bool loaded = true);
	bool IsLoaded() const;

	u16 GetID() const;
	wxString GetName() const;
	void SetName(const wxString& name);

public:
	void Log(const u32 id, wxString fmt, ...);
	void Log(wxString fmt, ...);

	void Warning(const u32 id, wxString fmt, ...);
	void Warning(wxString fmt, ...);

	void Error(const u32 id, wxString fmt, ...);
	void Error(wxString fmt, ...);

	bool CheckId(u32 id) const;

	bool CheckId(u32 id, ID& _id) const;

	template<typename T> bool CheckId(u32 id, T*& data);

	u32 GetNewId(void* data = nullptr, u8 flags = 0);

	template<typename T>
	__forceinline void AddFunc(u32 id, T func)
	{
		m_funcs_list.Move(new ModuleFunc(id, bind_func(func)));
	}
};

bool IsLoadedFunc(u32 id);
bool CallFunc(u32 num);
bool UnloadFunc(u32 id);
void UnloadModules();
u32 GetFuncNumById(u32 id);
Module* GetModuleByName(const wxString& name);
Module* GetModuleById(u16 id);
