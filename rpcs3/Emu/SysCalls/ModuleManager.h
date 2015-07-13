#pragma once
#include "Modules.h"

class ModuleManager
{
	bool initialized;

public:
	ModuleManager();
	~ModuleManager();

	void Init();
	void Close();
	Module* GetModuleByName(const char* name);
	Module* GetModuleById(u16 id);
	bool CheckModuleId(u16 id);
};
