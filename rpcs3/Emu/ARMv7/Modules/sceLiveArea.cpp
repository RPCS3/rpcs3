#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceLiveArea.h"

s32 sceLiveAreaResourceReplaceAll(vm::cptr<char> dirpath)
{
	throw __FUNCTION__;
}

s32 sceLiveAreaResourceGetStatus()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceLiveArea, #name, name)

psv_log_base sceLiveArea("SceLiveArea", []()
{
	sceLiveArea.on_load = nullptr;
	sceLiveArea.on_unload = nullptr;
	sceLiveArea.on_stop = nullptr;
	sceLiveArea.on_error = nullptr;

	REG_FUNC(0xA4B506F9, sceLiveAreaResourceReplaceAll);
	REG_FUNC(0x54A395FB, sceLiveAreaResourceGetStatus);
});
