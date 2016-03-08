#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceRtc.h"

u32 sceRtcGetTickResolution()
{
	throw EXCEPTION("");
}

s32 sceRtcGetCurrentTick(vm::ptr<u64> pTick)
{
	throw EXCEPTION("");
}

s32 sceRtcGetCurrentClock(vm::ptr<SceDateTime> pTime, s32 iTimeZone)
{
	throw EXCEPTION("");
}

s32 sceRtcGetCurrentClockLocalTime(vm::ptr<SceDateTime> pTime)
{
	throw EXCEPTION("");
}

s32 sceRtcGetCurrentNetworkTick(vm::ptr<u64> pTick)
{
	throw EXCEPTION("");
}

s32 sceRtcConvertUtcToLocalTime(vm::cptr<u64> pUtc, vm::ptr<u64> pLocalTime)
{
	throw EXCEPTION("");
}

s32 sceRtcConvertLocalTimeToUtc(vm::cptr<u64> pLocalTime, vm::ptr<u64> pUtc)
{
	throw EXCEPTION("");
}

s32 sceRtcIsLeapYear(s32 year)
{
	throw EXCEPTION("");
}

s32 sceRtcGetDaysInMonth(s32 year, s32 month)
{
	throw EXCEPTION("");
}

s32 sceRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	throw EXCEPTION("");
}

s32 sceRtcCheckValid(vm::cptr<SceDateTime> pTime)
{
	throw EXCEPTION("");
}

s32 sceRtcSetTime_t(vm::ptr<SceDateTime> pTime, u32 iTime)
{
	throw EXCEPTION("");
}

s32 sceRtcSetTime64_t(vm::ptr<SceDateTime> pTime, u64 ullTime)
{
	throw EXCEPTION("");
}

s32 sceRtcGetTime_t(vm::cptr<SceDateTime> pTime, vm::ptr<u32> piTime)
{
	throw EXCEPTION("");
}

s32 sceRtcGetTime64_t(vm::cptr<SceDateTime> pTime, vm::ptr<u64> pullTime)
{
	throw EXCEPTION("");
}

s32 sceRtcSetDosTime(vm::ptr<SceDateTime> pTime, u32 uiDosTime)
{
	throw EXCEPTION("");
}

s32 sceRtcGetDosTime(vm::cptr<SceDateTime> pTime, vm::ptr<u32> puiDosTime)
{
	throw EXCEPTION("");
}

s32 sceRtcSetWin32FileTime(vm::ptr<SceDateTime> pTime, u64 ulWin32Time)
{
	throw EXCEPTION("");
}

s32 sceRtcGetWin32FileTime(vm::cptr<SceDateTime> pTime, vm::ptr<u64> ulWin32Time)
{
	throw EXCEPTION("");
}

s32 sceRtcSetTick(vm::ptr<SceDateTime> pTime, vm::cptr<u64> pTick)
{
	throw EXCEPTION("");
}

s32 sceRtcGetTick(vm::cptr<SceDateTime> pTime, vm::ptr<u64> pTick)
{
	throw EXCEPTION("");
}

s32 sceRtcCompareTick(vm::cptr<u64> pTick1, vm::cptr<u64> pTick2)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddTicks(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, u64 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddMicroseconds(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, u64 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddSeconds(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, u64 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddMinutes(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, u64 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddHours(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, s32 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddDays(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, s32 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddWeeks(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, s32 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddMonths(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, s32 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcTickAddYears(vm::ptr<u64> pTick0, vm::cptr<u64> pTick1, s32 lAdd)
{
	throw EXCEPTION("");
}

s32 sceRtcFormatRFC2822(vm::ptr<char> pszDateTime, vm::cptr<u64> pUtc, s32 iTimeZoneMinutes)
{
	throw EXCEPTION("");
}

s32 sceRtcFormatRFC2822LocalTime(vm::ptr<char> pszDateTime, vm::cptr<u64> pUtc)
{
	throw EXCEPTION("");
}

s32 sceRtcFormatRFC3339(vm::ptr<char> pszDateTime, vm::cptr<u64> pUtc, s32 iTimeZoneMinutes)
{
	throw EXCEPTION("");
}

s32 sceRtcFormatRFC3339LocalTime(vm::ptr<char> pszDateTime, vm::cptr<u64> pUtc)
{
	throw EXCEPTION("");
}

s32 sceRtcParseDateTime(vm::ptr<u64> pUtc, vm::cptr<char> pszDateTime)
{
	throw EXCEPTION("");
}

s32 sceRtcParseRFC3339(vm::ptr<u64> pUtc, vm::cptr<char> pszDateTime)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceRtc, #name, name)

psv_log_base sceRtc("SceRtc", []()
{
	sceRtc.on_load = nullptr;
	sceRtc.on_unload = nullptr;
	sceRtc.on_stop = nullptr;
	sceRtc.on_error = nullptr;

	REG_FUNC(0x23F79274, sceRtcGetCurrentTick);
	REG_FUNC(0xCDDD25FE, sceRtcGetCurrentNetworkTick);
	REG_FUNC(0x70FDE8F1, sceRtcGetCurrentClock);
	REG_FUNC(0x0572EDDC, sceRtcGetCurrentClockLocalTime);
	REG_FUNC(0x1282C436, sceRtcConvertUtcToLocalTime);
	REG_FUNC(0x0A05E201, sceRtcConvertLocalTimeToUtc);
	REG_FUNC(0x42CA8EB5, sceRtcFormatRFC2822LocalTime);
	REG_FUNC(0x147F2138, sceRtcFormatRFC2822);
	REG_FUNC(0x742250A9, sceRtcFormatRFC3339LocalTime);
	REG_FUNC(0xCCEA2B54, sceRtcFormatRFC3339);
	REG_FUNC(0xF17FD8B5, sceRtcIsLeapYear);
	REG_FUNC(0x49EB4556, sceRtcGetDaysInMonth);
	REG_FUNC(0x2F3531EB, sceRtcGetDayOfWeek);
	REG_FUNC(0xD7622935, sceRtcCheckValid);
	REG_FUNC(0x3A332F81, sceRtcSetTime_t);
	REG_FUNC(0xA6C36B6A, sceRtcSetTime64_t);
	REG_FUNC(0x8DE6FEB7, sceRtcGetTime_t);
	REG_FUNC(0xC995DE02, sceRtcGetTime64_t);
	REG_FUNC(0xF8B22B07, sceRtcSetDosTime);
	REG_FUNC(0x92ABEBAF, sceRtcGetDosTime);
	REG_FUNC(0xA79A8846, sceRtcSetWin32FileTime);
	REG_FUNC(0x8A95E119, sceRtcGetWin32FileTime);
	REG_FUNC(0x811313B3, sceRtcGetTickResolution);
	REG_FUNC(0xCD89F464, sceRtcSetTick);
	REG_FUNC(0xF2B238E2, sceRtcGetTick);
	REG_FUNC(0xC7385158, sceRtcCompareTick);
	REG_FUNC(0x4559E2DB, sceRtcTickAddTicks);
	REG_FUNC(0xAE26D920, sceRtcTickAddMicroseconds);
	REG_FUNC(0x979AFD79, sceRtcTickAddSeconds);
	REG_FUNC(0x4C358871, sceRtcTickAddMinutes);
	REG_FUNC(0x6F193F55, sceRtcTickAddHours);
	REG_FUNC(0x58DE3C70, sceRtcTickAddDays);
	REG_FUNC(0xE713C640, sceRtcTickAddWeeks);
	REG_FUNC(0x6321B4AA, sceRtcTickAddMonths);
	REG_FUNC(0xDF6C3E1B, sceRtcTickAddYears);
	REG_FUNC(0x2347CE12, sceRtcParseDateTime);
	REG_FUNC(0x2D18AEEC, sceRtcParseRFC3339);
});
