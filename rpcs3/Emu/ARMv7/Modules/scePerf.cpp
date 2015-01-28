#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base scePerf;

s32 scePerfArmPmonReset(s32 threadId)
{
	scePerf.Todo("scePerfArmPmonReset(threadId=0x%x)", threadId);

	return SCE_OK;
}

s32 scePerfArmPmonSelectEvent(s32 threadId, u32 counter, u8 eventCode)
{
	throw __FUNCTION__;
}

s32 scePerfArmPmonStart(s32 threadId)
{
	throw __FUNCTION__;
}

s32 scePerfArmPmonStop(s32 threadId)
{
	throw __FUNCTION__;
}

s32 scePerfArmPmonGetCounterValue(s32 threadId, u32 counter, vm::psv::ptr<u32> pValue)
{
	throw __FUNCTION__;
}

s32 scePerfArmPmonSoftwareIncrement(u32 mask)
{
	throw __FUNCTION__;
}

u64 scePerfGetTimebaseValue()
{
	throw __FUNCTION__;
}

u32 scePerfGetTimebaseFrequency()
{
	throw __FUNCTION__;
}

s32 _sceRazorCpuInit(vm::psv::ptr<const void> pBufferBase, u32 bufferSize, u32 numPerfCounters, vm::psv::ptr<vm::psv::ptr<u32>> psceRazorVars)
{
	throw __FUNCTION__;
}

s32 sceRazorCpuPushMarker(vm::psv::ptr<const char> szLabel)
{
	throw __FUNCTION__;
}

s32 sceRazorCpuPopMarker()
{
	throw __FUNCTION__;
}

s32 sceRazorCpuSync()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &scePerf, #name, name)

psv_log_base scePerf("ScePerf", []()
{
	scePerf.on_load = nullptr;
	scePerf.on_unload = nullptr;
	scePerf.on_stop = nullptr;

	REG_FUNC(0x35151735, scePerfArmPmonReset);
	REG_FUNC(0x63CBEA8B, scePerfArmPmonSelectEvent);
	REG_FUNC(0xC9D969D5, scePerfArmPmonStart);
	REG_FUNC(0xD1A40F54, scePerfArmPmonStop);
	REG_FUNC(0x6132A497, scePerfArmPmonGetCounterValue);
	//REG_FUNC(0x12F6C708, scePerfArmPmonSetCounterValue);
	REG_FUNC(0x4264B4E7, scePerfArmPmonSoftwareIncrement);
	REG_FUNC(0xBD9615E5, scePerfGetTimebaseValue);
	REG_FUNC(0x78EA4FFB, scePerfGetTimebaseFrequency);
	REG_FUNC(0x7AD6AC30, _sceRazorCpuInit);
	REG_FUNC(0xC3DE4C0A, sceRazorCpuPushMarker);
	REG_FUNC(0xDC3224C3, sceRazorCpuPopMarker);
	REG_FUNC(0x4F1385E3, sceRazorCpuSync);
});