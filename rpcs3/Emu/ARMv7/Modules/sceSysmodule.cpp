#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceSysmodule;

s32 sceSysmoduleLoadModule(u16 id)
{
	sceSysmodule.Error("sceSysmoduleLoadModule(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // loading succeeded
}

s32 sceSysmoduleUnloadModule(u16 id)
{
	sceSysmodule.Error("sceSysmoduleUnloadModule(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // unloading succeeded
}

s32 sceSysmoduleIsLoaded(u16 id)
{
	sceSysmodule.Error("sceSysmoduleIsLoaded(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // module is loaded
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSysmodule, #name, name)

psv_log_base sceSysmodule("SceSysmodule", []()
{
	sceSysmodule.on_load = nullptr;
	sceSysmodule.on_unload = nullptr;
	sceSysmodule.on_stop = nullptr;

	REG_FUNC(0x79A0160A, sceSysmoduleLoadModule);
	REG_FUNC(0x31D87805, sceSysmoduleUnloadModule);
	REG_FUNC(0x53099B7A, sceSysmoduleIsLoaded);
});
