#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceSysmodule.h"

logs::channel sceSysmodule("sceSysmodule");

s32 sceSysmoduleLoadModule(u16 id)
{
	sceSysmodule.warning("sceSysmoduleLoadModule(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // loading succeeded
}

s32 sceSysmoduleUnloadModule(u16 id)
{
	sceSysmodule.warning("sceSysmoduleUnloadModule(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // unloading succeeded
}

s32 sceSysmoduleIsLoaded(u16 id)
{
	sceSysmodule.warning("sceSysmoduleIsLoaded(id=0x%04x) -> SCE_OK", id);

	return SCE_OK; // module is loaded
}

#define REG_FUNC(nid, name) REG_FNID(SceSysmodule, nid, name)

DECLARE(arm_module_manager::SceSysmodule)("SceSysmodule", []()
{
	REG_FUNC(0x79A0160A, sceSysmoduleLoadModule);
	REG_FUNC(0x31D87805, sceSysmoduleUnloadModule);
	REG_FUNC(0x53099B7A, sceSysmoduleIsLoaded);
});
