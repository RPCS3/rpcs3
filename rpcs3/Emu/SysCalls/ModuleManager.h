#pragma once

class Module;

class ModuleManager
{
	bool m_init = false;

public:
	ModuleManager();
	~ModuleManager();

	void Init();
	void Close();

	static Module* GetModuleByName(const char* name);
	static Module* GetModuleById(u16 id);
	static bool CheckModuleId(u16 id);
};
