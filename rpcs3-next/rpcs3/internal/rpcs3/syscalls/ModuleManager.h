#pragma once

template<typename T> class Module;

class ModuleManager
{
	bool m_init = false;

public:
	ModuleManager();
	~ModuleManager();

	void Init();
	void Close();
	void Alloc();

	static Module<void>* GetModuleByName(const char* name);
	static Module<void>* GetModuleById(u16 id);
	static bool CheckModuleId(u16 id);
};
