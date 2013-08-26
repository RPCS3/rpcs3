#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSysmodule_init();
Module cellSysmodule("cellSysmodule", cellSysmodule_init);

enum
{
	CELL_SYSMODULE_LOADED						= CELL_OK,
	CELL_SYSMODULE_ERROR_DUPLICATED				= 0x80012001,
	CELL_SYSMODULE_ERROR_UNKNOWN				= 0x80012002,
	CELL_SYSMODULE_ERROR_UNLOADED				= 0x80012003,
	CELL_SYSMODULE_ERROR_INVALID_MEMCONTAINER	= 0x80012004,
	CELL_SYSMODULE_ERROR_FATAL					= 0x800120ff,
};

int cellSysmoduleInitialize()
{
	cellSysmodule.Log("cellSysmoduleInitialize()");
	return CELL_OK;
}

int cellSysmoduleFinalize()
{
	cellSysmodule.Log("cellSysmoduleFinalize()");
	return CELL_OK;
}

int cellSysmoduleSetMemcontainer(u32 ct_id)
{
	cellSysmodule.Warning("TODO: cellSysmoduleSetMemcontainer(ct_id=0x%x)", ct_id);
	return CELL_OK;
}

int cellSysmoduleLoadModule(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleLoadModule(id=0x%04x)", id);
	Module* m = GetModuleById(id);

	if(!m)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if(m->IsLoaded())
	{
		return CELL_SYSMODULE_ERROR_DUPLICATED;
	}

	m->Load();
	return CELL_OK;
}

int cellSysmoduleUnloadModule(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleUnloadModule(id=0x%04x)", id);
	Module* m = GetModuleById(id);

	if(!m)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	if(!m->IsLoaded())
	{
		return CELL_SYSMODULE_ERROR_UNLOADED;
	}

	m->UnLoad();
	return CELL_OK;
}

int cellSysmoduleIsLoaded(u16 id)
{
	cellSysmodule.Warning("cellSysmoduleIsLoaded(id=0x%04x)", id);
	Module* m = GetModuleById(id);

	if(!m)
	{
		return CELL_SYSMODULE_ERROR_UNKNOWN;
	}

	return m->IsLoaded() ? CELL_SYSMODULE_LOADED : CELL_SYSMODULE_ERROR_UNLOADED;
}

void cellSysmodule_init()
{
	cellSysmodule.AddFunc(0x63ff6ff9, cellSysmoduleInitialize);
	cellSysmodule.AddFunc(0x96c07adf, cellSysmoduleFinalize);
	cellSysmodule.AddFunc(0xa193143c, cellSysmoduleSetMemcontainer);
	cellSysmodule.AddFunc(0x32267a31, cellSysmoduleLoadModule);
	cellSysmodule.AddFunc(0x112a5ee9, cellSysmoduleUnloadModule);
	cellSysmodule.AddFunc(0x5a59e258, cellSysmoduleIsLoaded);
}
