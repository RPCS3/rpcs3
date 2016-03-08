#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceAppMgr.h"

s32 sceAppMgrReceiveEventNum(vm::ptr<s32> eventNum)
{
	throw EXCEPTION("");
}

s32 sceAppMgrReceiveEvent(vm::ptr<SceAppMgrEvent> appEvent)
{
	throw EXCEPTION("");
}

s32 sceAppMgrAcquireBgmPort()
{
	throw EXCEPTION("");
}

s32 sceAppMgrReleaseBgmPort()
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceAppMgr, #name, name)

psv_log_base sceAppMgr("SceAppMgr", []()
{
	sceAppMgr.on_load = nullptr;
	sceAppMgr.on_unload = nullptr;
	sceAppMgr.on_stop = nullptr;
	sceAppMgr.on_error = nullptr;

	REG_FUNC(0x47E5DD7D, sceAppMgrReceiveEventNum);
	REG_FUNC(0xCFAD5A3A, sceAppMgrReceiveEvent);
	REG_FUNC(0xF3D65520, sceAppMgrAcquireBgmPort);
	REG_FUNC(0x96CBE713, sceAppMgrReleaseBgmPort);
	//REG_FUNC(0x49255C91, sceAppMgrGetRunStatus);
});
