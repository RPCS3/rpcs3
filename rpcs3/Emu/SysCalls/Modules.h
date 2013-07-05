#pragma once

#define declCPU PPUThread& CPU = GetCurrentPPUThread

class func_caller
{
public:
	virtual void operator()() = 0;
};

static func_caller *null_func = nullptr;

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
	const char* m_name;
	const u16 m_id;
	bool m_is_loaded;

public:
	Array<ModuleFunc> m_funcs_list;

	Module(const char* name, u16 id, void (*init)());

	void Load();
	void UnLoad();
	bool Load(u32 id);
	bool UnLoad(u32 id);

	void SetLoaded(bool loaded = true);
	bool IsLoaded() const;

	u16 GetID() const;
	wxString GetName() const;

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
	__forceinline void Module::AddFunc(u32 id, func_caller* func)
	{
		m_funcs_list.Move(new ModuleFunc(id, func));
	}
};

bool IsLoadedFunc(u32 id);
bool CallFunc(u32 id);
bool UnloadFunc(u32 id);
void UnloadModules();
Module* GetModuleByName(const wxString& name);
Module* GetModuleById(u16 id);
