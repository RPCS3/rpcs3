#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "scePerf.h"

logs::channel scePerf("scePerf");

extern u64 get_system_time();

template<>
void fmt_class_string<ScePerfError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SCE_PERF_ERROR_INVALID_ARGUMENT);
		}

		return unknown;
	});
}

error_code scePerfArmPmonReset(ARMv7Thread& cpu, s32 threadId)
{
	scePerf.warning("scePerfArmPmonReset(threadId=0x%x)", threadId);

	verify(HERE), threadId == SCE_PERF_ARM_PMON_THREAD_ID_SELF;

	cpu.counters = {};

	return SCE_OK;
}

error_code scePerfArmPmonSelectEvent(ARMv7Thread& cpu, s32 threadId, u32 counter, u8 eventCode)
{
	scePerf.warning("scePerfArmPmonSelectEvent(threadId=0x%x, counter=0x%x, eventCode=0x%x)", threadId, counter, eventCode);

	verify(HERE), threadId == SCE_PERF_ARM_PMON_THREAD_ID_SELF;

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
		fmt::throw_exception("Unknown event requested" HERE);
	}
	}

	cpu.counters[counter].event = eventCode;
	cpu.counters[counter].value = value;

	return SCE_OK;
}

error_code scePerfArmPmonStart(ARMv7Thread& cpu, s32 threadId)
{
	scePerf.warning("scePerfArmPmonStart(threadId=0x%x)", threadId);

	verify(HERE), threadId == SCE_PERF_ARM_PMON_THREAD_ID_SELF;

	return SCE_OK;
}

error_code scePerfArmPmonStop(ARMv7Thread& cpu, s32 threadId)
{
	scePerf.warning("scePerfArmPmonStop(threadId=0x%x)");

	verify(HERE), threadId == SCE_PERF_ARM_PMON_THREAD_ID_SELF;

	return SCE_OK;
}

error_code scePerfArmPmonGetCounterValue(ARMv7Thread& cpu, s32 threadId, u32 counter, vm::ptr<u32> pValue)
{
	scePerf.warning("scePerfArmPmonGetCounterValue(threadId=0x%x, counter=%d, pValue=*0x%x)", threadId, counter, pValue);

	verify(HERE), threadId == SCE_PERF_ARM_PMON_THREAD_ID_SELF;

	if (counter >= 6 && counter != SCE_PERF_ARM_PMON_CYCLE_COUNTER)
	{
		return SCE_PERF_ERROR_INVALID_ARGUMENT;
	}

	if (counter < 6)
	{
		*pValue = cpu.counters[counter].value;
	}
	else
	{
		fmt::throw_exception("Cycle counter requested" HERE);
	}

	return SCE_OK;
}

error_code scePerfArmPmonSoftwareIncrement(ARMv7Thread& cpu, u32 mask)
{
	scePerf.warning("scePerfArmPmonSoftwareIncrement(mask=0x%x)", mask);

	if (mask > SCE_PERF_ARM_PMON_COUNTER_MASK_ALL)
	{
		return SCE_PERF_ERROR_INVALID_ARGUMENT;
	}

	for (u32 i = 0; i < 6; i++, mask >>= 1)
	{
		if (mask & 1)
		{
			cpu.counters[i].value++;
		}
	}

	return SCE_OK;
}

u64 scePerfGetTimebaseValue()
{
	scePerf.warning("scePerfGetTimebaseValue()");

	return get_system_time();
}

u32 scePerfGetTimebaseFrequency()
{
	scePerf.warning("scePerfGetTimebaseFrequency()");

	return 1;
}

s32 _sceRazorCpuInit(vm::cptr<void> pBufferBase, u32 bufferSize, u32 numPerfCounters, vm::pptr<u32> psceRazorVars)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceRazorCpuPushMarker(vm::cptr<char> szLabel)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceRazorCpuPopMarker()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceRazorCpuSync()
{
	fmt::throw_exception("Unimplemented" HERE);
}

#define REG_FUNC(nid, name) REG_FNID(ScePerf, nid, name)

DECLARE(arm_module_manager::ScePerf)("ScePerf", []()
{
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
