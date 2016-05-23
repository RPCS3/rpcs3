#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceAppMgr.h"

logs::channel sceAppMgr("sceAppMgr", logs::level::notice);

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


#define REG_FUNC(nid, name) REG_FNID(SceAppMgrUser, nid, name)

DECLARE(arm_module_manager::SceAppMgr)("SceAppMgrUser", []()
{
	REG_FUNC(0x47E5DD7D, sceAppMgrReceiveEventNum);
	REG_FUNC(0xCFAD5A3A, sceAppMgrReceiveEvent);
	REG_FUNC(0xF3D65520, sceAppMgrAcquireBgmPort);
	REG_FUNC(0x96CBE713, sceAppMgrReleaseBgmPort);
	//REG_FUNC(0x49255C91, sceAppMgrGetRunStatus);
});
