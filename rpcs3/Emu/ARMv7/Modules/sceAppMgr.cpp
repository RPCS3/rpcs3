#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceAppMgr;

struct SceAppMgrEvent
{
	s32 event;
	s32 appId;
	char param[56];
};

s32 sceAppMgrReceiveEventNum(vm::psv::ptr<s32> eventNum)
{
	throw __FUNCTION__;
}

s32 sceAppMgrReceiveEvent(vm::psv::ptr<SceAppMgrEvent> appEvent)
{
	throw __FUNCTION__;
}

s32 sceAppMgrAcquireBgmPort()
{
	throw __FUNCTION__;
}

s32 sceAppMgrReleaseBgmPort()
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)(func_ptr)name>(nid, &sceAppMgr, #name, name)

psv_log_base sceAppMgr("SceAppMgr", []()
{
	sceAppMgr.on_load = nullptr;
	sceAppMgr.on_unload = nullptr;
	sceAppMgr.on_stop = nullptr;

	REG_FUNC(0x47E5DD7D, sceAppMgrReceiveEventNum);
	REG_FUNC(0xCFAD5A3A, sceAppMgrReceiveEvent);
	REG_FUNC(0xF3D65520, sceAppMgrAcquireBgmPort);
	REG_FUNC(0x96CBE713, sceAppMgrReleaseBgmPort);
	//REG_FUNC(0x49255C91, sceAppMgrGetRunStatus);
});
