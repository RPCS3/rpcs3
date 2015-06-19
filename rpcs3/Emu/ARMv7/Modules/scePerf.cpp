#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern u64 get_system_time();

#define RETURN_ERROR(code) { Emu.Pause(); scePerf.Error("%s() failed: %s", __FUNCTION__, #code); return code; }

extern psv_log_base scePerf;

enum
{
	// Error Codes
	SCE_PERF_ERROR_INVALID_ARGUMENT = 0x80580000,
};

enum : s32
{
	// Thread IDs
	SCE_PERF_ARM_PMON_THREAD_ID_ALL = -1,
	SCE_PERF_ARM_PMON_THREAD_ID_SELF = 0,
};

enum : u32
{
	// Counter Numbers
	SCE_PERF_ARM_PMON_CYCLE_COUNTER = 31,
	SCE_PERF_ARM_PMON_COUNTER_5 = 5,
	SCE_PERF_ARM_PMON_COUNTER_4 = 4,
	SCE_PERF_ARM_PMON_COUNTER_3 = 3,
	SCE_PERF_ARM_PMON_COUNTER_2 = 2,
	SCE_PERF_ARM_PMON_COUNTER_1 = 1,
	SCE_PERF_ARM_PMON_COUNTER_0 = 0,

	// Counter Masks
	SCE_PERF_ARM_PMON_COUNTER_MASK_5 = 0x20,
	SCE_PERF_ARM_PMON_COUNTER_MASK_4 = 0x10,
	SCE_PERF_ARM_PMON_COUNTER_MASK_3 = 0x08,
	SCE_PERF_ARM_PMON_COUNTER_MASK_2 = 0x04,
	SCE_PERF_ARM_PMON_COUNTER_MASK_1 = 0x02,
	SCE_PERF_ARM_PMON_COUNTER_MASK_0 = 0x01,
	SCE_PERF_ARM_PMON_COUNTER_MASK_ALL = 0x3f,
};

enum : u8
{
	// Performance Counter Events
	SCE_PERF_ARM_PMON_SOFT_INCREMENT = 0x00,
	SCE_PERF_ARM_PMON_ICACHE_MISS = 0x01,
	SCE_PERF_ARM_PMON_ITLB_MISS = 0x02,
	SCE_PERF_ARM_PMON_DCACHE_MISS = 0x03,
	SCE_PERF_ARM_PMON_DCACHE_ACCESS = 0x04,
	SCE_PERF_ARM_PMON_DTLB_MISS = 0x05,
	SCE_PERF_ARM_PMON_DATA_READ = 0x06,
	SCE_PERF_ARM_PMON_DATA_WRITE = 0x07,
	SCE_PERF_ARM_PMON_EXCEPTION_TAKEN = 0x09,
	SCE_PERF_ARM_PMON_EXCEPTION_RETURN = 0x0A,
	SCE_PERF_ARM_PMON_WRITE_CONTEXTID = 0x0B,
	SCE_PERF_ARM_PMON_SOFT_CHANGEPC = 0x0C,
	SCE_PERF_ARM_PMON_IMMEDIATE_BRANCH = 0x0D,
	SCE_PERF_ARM_PMON_UNALIGNED = 0x0F,
	SCE_PERF_ARM_PMON_BRANCH_MISPREDICT = 0x10,
	SCE_PERF_ARM_PMON_PREDICT_BRANCH = 0x12,
	SCE_PERF_ARM_PMON_COHERENT_LF_MISS = 0x50,
	SCE_PERF_ARM_PMON_COHERENT_LF_HIT = 0x51,
	SCE_PERF_ARM_PMON_ICACHE_STALL = 0x60,
	SCE_PERF_ARM_PMON_DCACHE_STALL = 0x61,
	SCE_PERF_ARM_PMON_MAINTLB_STALL = 0x62,
	SCE_PERF_ARM_PMON_STREX_PASSED = 0x63,
	SCE_PERF_ARM_PMON_STREX_FAILED = 0x64,
	SCE_PERF_ARM_PMON_DATA_EVICTION = 0x65,
	SCE_PERF_ARM_PMON_ISSUE_NO_DISPATCH = 0x66,
	SCE_PERF_ARM_PMON_ISSUE_EMPTY = 0x67,
	SCE_PERF_ARM_PMON_INST_RENAME = 0x68,
	SCE_PERF_ARM_PMON_PREDICT_FUNC_RET = 0x6E,
	SCE_PERF_ARM_PMON_MAIN_PIPE = 0x70,
	SCE_PERF_ARM_PMON_SECOND_PIPE = 0x71,
	SCE_PERF_ARM_PMON_LS_PIPE = 0x72,
	SCE_PERF_ARM_PMON_FPU_RENAME = 0x73,
	SCE_PERF_ARM_PMON_PLD_STALL = 0x80,
	SCE_PERF_ARM_PMON_WRITE_STALL = 0x81,
	SCE_PERF_ARM_PMON_INST_MAINTLB_STALL = 0x82,
	SCE_PERF_ARM_PMON_DATA_MAINTLB_STALL = 0x83,
	SCE_PERF_ARM_PMON_INST_UTLB_STALL = 0x84,
	SCE_PERF_ARM_PMON_DATA_UTLB_STALL = 0x85,
	SCE_PERF_ARM_PMON_DMB_STALL = 0x86,
	SCE_PERF_ARM_PMON_INTEGER_CLOCK = 0x8A,
	SCE_PERF_ARM_PMON_DATAENGINE_CLOCK = 0x8B,
	SCE_PERF_ARM_PMON_ISB = 0x90,
	SCE_PERF_ARM_PMON_DSB = 0x91,
	SCE_PERF_ARM_PMON_DMB = 0x92,
	SCE_PERF_ARM_PMON_EXT_INTERRUPT = 0x93,
	SCE_PERF_ARM_PMON_PLE_LINE_REQ_COMPLETED = 0xA0,
	SCE_PERF_ARM_PMON_PLE_CHANNEL_SKIPPED = 0xA1,
	SCE_PERF_ARM_PMON_PLE_FIFO_FLUSH = 0xA2,
	SCE_PERF_ARM_PMON_PLE_REQ_COMPLETED = 0xA3,
	SCE_PERF_ARM_PMON_PLE_FIFO_OVERFLOW = 0xA4,
	SCE_PERF_ARM_PMON_PLE_REQ_PROGRAMMED = 0xA5,
};

s32 scePerfArmPmonReset(ARMv7Context& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonReset(threadId=0x%x)", threadId);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw __FUNCTION__;
	}

	context.counters = {};

	return SCE_OK;
}

s32 scePerfArmPmonSelectEvent(ARMv7Context& context, s32 threadId, u32 counter, u8 eventCode)
{
	scePerf.Warning("scePerfArmPmonSelectEvent(threadId=0x%x, counter=0x%x, eventCode=0x%x)", threadId, counter, eventCode);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw __FUNCTION__;
	}

	if (counter >= 6)
	{
		RETURN_ERROR(SCE_PERF_ERROR_INVALID_ARGUMENT);
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

	default: throw "scePerfArmPmonSelectEvent(): unknown event requested";
	}

	context.counters[counter].event = eventCode;
	context.counters[counter].value = value;

	return SCE_OK;
}

s32 scePerfArmPmonStart(ARMv7Context& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonStart(threadId=0x%x)", threadId);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw __FUNCTION__;
	}

	return SCE_OK;
}

s32 scePerfArmPmonStop(ARMv7Context& context, s32 threadId)
{
	scePerf.Warning("scePerfArmPmonStop(threadId=0x%x)");

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw __FUNCTION__;
	}

	return SCE_OK;
}

s32 scePerfArmPmonGetCounterValue(ARMv7Context& context, s32 threadId, u32 counter, vm::psv::ptr<u32> pValue)
{
	scePerf.Warning("scePerfArmPmonGetCounterValue(threadId=0x%x, counter=%d, pValue=*0x%x)", threadId, counter, pValue);

	if (threadId != SCE_PERF_ARM_PMON_THREAD_ID_SELF)
	{
		throw __FUNCTION__;
	}

	if (counter >= 6 && counter != SCE_PERF_ARM_PMON_CYCLE_COUNTER)
	{
		RETURN_ERROR(SCE_PERF_ERROR_INVALID_ARGUMENT);
	}

	if (counter < 6)
	{
		*pValue = context.counters[counter].value;
	}
	else
	{
		throw "scePerfArmPmonGetCounterValue(): cycle counter requested";
	}

	return SCE_OK;
}

s32 scePerfArmPmonSoftwareIncrement(ARMv7Context& context, u32 mask)
{
	scePerf.Warning("scePerfArmPmonSoftwareIncrement(mask=0x%x)", mask);

	if (mask > SCE_PERF_ARM_PMON_COUNTER_MASK_ALL)
	{
		RETURN_ERROR(SCE_PERF_ERROR_INVALID_ARGUMENT);
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

s32 _sceRazorCpuInit(vm::psv::ptr<const void> pBufferBase, u32 bufferSize, u32 numPerfCounters, vm::psv::pptr<u32> psceRazorVars)
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