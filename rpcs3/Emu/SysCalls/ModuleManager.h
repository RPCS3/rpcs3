#pragma once
#include "Modules.h"

class ModuleManager
{
	Module* m_modules[3][0xff];
	uint m_max_module_id;
	uint m_module_2_count;
	std::mutex m_funcs_lock;
	std::vector<ModuleFunc *> m_modules_funcs_list;
	std::vector<Module> m_mod_init;
	bool initialized;
public:
	ModuleManager();
	~ModuleManager();

	void init();
	void AddFunc(ModuleFunc *func);
	void SetModule(int id, Module* module, bool with_data);
	bool IsLoadedFunc(u32 id);
	bool CallFunc(u32 num);
	bool UnloadFunc(u32 id);
	void UnloadModules();
	u32 GetFuncNumById(u32 id);
	Module* GetModuleByName(const std::string& name);
	Module* GetModuleById(u16 id);

};