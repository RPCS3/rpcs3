#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceLiveArea.h"

logs::channel sceLiveArea("sceLiveArea", logs::level::notice);

s32 sceLiveAreaResourceReplaceAll(vm::cptr<char> dirpath)
{
	throw EXCEPTION("");
}

s32 sceLiveAreaResourceGetStatus()
{
	throw EXCEPTION("");
}

#define REG_FUNC(nid, name) REG_FNID(SceLiveArea, nid, name)

DECLARE(arm_module_manager::SceLiveArea)("SceLiveArea", []()
{
	REG_FUNC(0xA4B506F9, sceLiveAreaResourceReplaceAll);
	REG_FUNC(0x54A395FB, sceLiveAreaResourceGetStatus);
});
