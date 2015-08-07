#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "scePerf.h"

extern u64 get_system_time();

s32 scePerfArmPmonReset(ARMv7Thread& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonReset(threadId=0x%x)", threadId);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw EXCEPTION("Unexpected thread");
	}

	context.counters = {};

	return SCE_OK;
}

s32 scePerfArmPmonSelectEvent(ARMv7Thread& context, s32 threadId, u32 counter, u8 eventCode)
{
	scePerf.Warning("scePerfArmPmonSelectEvent(threadId=0x%x, counter=0x%x, eventCode=0x%x)", threadId, counter, eventCode);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw EXCEPTION("Unexpected thread");
	}

	if (counter >= 6)
	{
		return SCE_PERF_ERROR_INVALID_ARGUMENT;
	}

	u32 value = 0; // initial value

	switch (eventCode)
	{
	case SCE_PERF_ARM_PMON_SOFT_INCREMENT: break;

	case SCE_PERF_ARM_PMON_BRANCH_MISPREDICT: 
	case SCE_PERF_ARM_PMON_DCACHE_MISS:
	case SCE_PERF_ARM_PMON_DCACHE_STALL:
	case SCE_PERF_ARM_PMON_ICACHE_STALL:
	case SCE_PERF_ARM_PMON_DATA_EVICTION:
	case SCE_PERF_ARM_PMON_WRITE_STALL:
	case SCE_PERF_ARM_PMON_MAINTLB_STALL:
	case SCE_PERF_ARM_PMON_UNALIGNED:
	{
		value = 1; // these events will probably never be implemented
		break;
	}
		
	case SCE_PERF_ARM_PMON_PREDICT_BRANCH:
	case SCE_PERF_ARM_PMON_DCACHE_ACCESS:
	{
		value = 1000; // these events will probably never be implemented
		break;
	}

	default:
	{
		throw EXCEPTION("Unknown event requested");
	}
	}

	context.counters[counter].event = eventCode;
	context.counters[counter].value = value;

	return SCE_OK;
}

s32 scePerfArmPmonStart(ARMv7Thread& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonStart(threadId=0x%x)", threadId);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw EXCEPTION("Unexpected thread");
	}

	return SCE_OK;
}

s32 scePerfArmPmonStop(ARMv7Thread& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonStop(threadId=0x%x)");

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw EXCEPTION("Unexpected thread");
	}

	return SCE_OK;
}

s32 scePerfArmPmonGetCounterValue(ARMv7Thread& context, s32 threadId, u32 counter, vm::ptr<u32> pValue)
{
	scePerf.Warning("scePerfArmPmonGetCounterValue(threadId=0x%x, counter=%d, pValue=*0x%x)", threadId, counter, pValue);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw EXCEPTION("Unexpected thread");
	}

	if (counter >= 6 && counter != SCE_PERF_ARM_PMON_CYCLE_COUNTER)
	{
		return SCE_PERF_ERROR_INVALID_ARGUMENT;
	}

	if (counter < 6)
	{
		*pValue = context.counters[counter].value;
	}
	else
	{
		throw EXCEPTION("Cycle counter requested");
	}

	return SCE_OK;
}

s32 scePerfArmPmonSoftwareIncrement(ARMv7Thread& context, u32 mask)
{
	scePerf.Warning("scePerfArmPmonSoftwareIncrement(mask=0x%x)", mask);

	if (mask > SCE_PERF_ARM_PMON_COUNTER_MASK_ALL)
	{
		return SCE_PERF_ERROR_INVALID_ARGUMENT;
	}

	for (u32 i = 0; i < 6; i++, mask >>= 1)
	{
		if (mask & 1)
		{
			context.counters[i].value++;
		}
	}

	return SCE_OK;
}

u64 scePerfGetTimebaseValue()
{
	scePerf.Warning("scePerfGetTimebaseValue()");

	return get_system_time();
}

u32 scePerfGetTimebaseFrequency()
{
	scePerf.Warning("scePerfGetTimebaseFrequency()");

	return 1;
}

s32 _sceRazorCpuInit(vm::cptr<void> pBufferBase, u32 bufferSize, u32 numPerfCounters, vm::pptr<u32> psceRazorVars)
{
	throw EXCEPTION("");
}

s32 sceRazorCpuPushMarker(vm::cptr<char> szLabel)
{
	throw EXCEPTION("");
}

s32 sceRazorCpuPopMarker()
{
	throw EXCEPTION("");
}

s32 sceRazorCpuSync()
{
	throw EXCEPTION("");
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &scePerf, #name, name)

psv_log_base scePerf("ScePerf", []()
{
	scePerf.on_load = nullptr;
	scePerf.on_unload = nullptr;
	scePerf.on_stop = nullptr;
	//scePerf.on_error = nullptr; // keep default error handler

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