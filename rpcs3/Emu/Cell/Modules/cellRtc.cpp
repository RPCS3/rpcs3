#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellRtc.h"

logs::channel cellRtc("cellRtc");

s64 convertToUNIXTime(u16 seconds, u16 minutes, u16 hours, u16 days, s32 years)
{
	return (s64)seconds + (s64)minutes * 60 + (s64)hours * 3600 + (s64)days * 86400 + 
		(s64)(years - 70) * 31536000 + (s64)((years - 69) / 4) * 86400 -
		(s64)((years - 1) / 100) * 86400 + (s64)((years + 299) / 400) * 86400;
}

u64 convertToWin32FILETIME(u16 seconds, u16 minutes, u16 hours, u16 days, s32 years)
{
	s64 unixtime = convertToUNIXTime(seconds, minutes, hours, days, years);
	u64 win32time = u64(unixtime) * u64(10000000) + u64(116444736000000000);
	u64 win32filetime = win32time | win32time >> 32;
	return win32filetime;
}

s32 cellRtcGetCurrentTick(vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcGetCurrentTick(pTick=*0x%x)", pTick);

	return CELL_OK;
}

s32 cellRtcGetCurrentClock(vm::ptr<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc.todo("cellRtcGetCurrentClock(pClock=*0x%x, time_zone=%d)", pClock, iTimeZone);

	return CELL_OK;
}

s32 cellRtcGetCurrentClockLocalTime(vm::ptr<CellRtcDateTime> pClock)
{
	cellRtc.todo("cellRtcGetCurrentClockLocalTime(pClock=*0x%x)", pClock);

	return CELL_OK;
}

s32 cellRtcFormatRfc2822(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.todo("cellRtcFormatRfc2822(pszDateTime=*0x%x, pUtc=*0x%x, time_zone=%d)", pszDateTime, pUtc, iTimeZone);

	return CELL_OK;
}

s32 cellRtcFormatRfc2822LocalTime(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcFormatRfc2822LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	return CELL_OK;
}

s32 cellRtcFormatRfc3339(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.todo("cellRtcFormatRfc3339(pszDateTime=*0x%x, pUtc=*0x%x, iTimeZone=%d)", pszDateTime, pUtc, iTimeZone);
	
	return CELL_OK;
}

s32 cellRtcFormatRfc3339LocalTime(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcFormatRfc3339LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);
	
	return CELL_OK;
}

s32 cellRtcParseDateTime(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.todo("cellRtcParseDateTime(pUtc=*0x%x, pszDateTime=%s)", pUtc, pszDateTime);

	return CELL_OK;
}

s32 cellRtcParseRfc3339(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.todo("cellRtcParseRfc3339(pUtc=*0x%x, pszDateTime=%s)", pUtc, pszDateTime);

	return CELL_OK;
}

s32 cellRtcGetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcGetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);

	return CELL_OK;
}

s32 cellRtcSetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcSetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);
	
	return CELL_OK;
}

s32 cellRtcTickAddTicks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.todo("cellRtcTickAddTicks(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	return CELL_OK;
}

s32 cellRtcTickAddMicroseconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.todo("cellRtcTickAddMicroseconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);
	
	return CELL_OK;
}

s32 cellRtcTickAddSeconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.todo("cellRtcTickAddSeconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	return CELL_OK;
}

s32 cellRtcTickAddMinutes(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.todo("cellRtcTickAddMinutes(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);
	
	return CELL_OK;
}

s32 cellRtcTickAddHours(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.todo("cellRtcTickAddHours(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	return CELL_OK;
}

s32 cellRtcTickAddDays(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.todo("cellRtcTickAddDays(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	return CELL_OK;
}

s32 cellRtcTickAddWeeks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.todo("cellRtcTickAddWeeks(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	return CELL_OK;
}

s32 cellRtcTickAddMonths(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.todo("cellRtcTickAddMonths(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	return CELL_OK;
}

s32 cellRtcTickAddYears(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.todo("cellRtcTickAddYears(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	return CELL_OK;
}

s32 cellRtcConvertUtcToLocalTime(vm::ptr<CellRtcTick> pUtc, vm::ptr<CellRtcTick> pLocalTime)
{
	cellRtc.todo("cellRtcConvertUtcToLocalTime(pUtc=*0x%x, pLocalTime=*0x%x)", pUtc, pLocalTime);

	return CELL_OK;
}

s32 cellRtcConvertLocalTimeToUtc(vm::ptr<CellRtcTick> pLocalTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcConvertLocalTimeToUtc(pLocalTime=*0x%x, pUtc=*0x%x)", pLocalTime, pUtc);

	return CELL_OK;
}

s32 cellRtcGetCurrentSecureTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcGetDosTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<u32> puiDosTime)
{
	cellRtc.todo("cellRtcGetDosTime(pDateTime=*0x%x, puiDosTime=*0x%x)", pDateTime, puiDosTime);

	return CELL_OK;
}

s32 cellRtcGetSystemTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcGetTime_t(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<s64> piTime)
{
	cellRtc.todo("cellRtcGetTime_t(pDateTime=*0x%x, piTime=*0x%x)", pDateTime, piTime);

	return CELL_OK;
}

s32 cellRtcGetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<u64> pulWin32FileTime)
{
	cellRtc.todo("cellRtcGetWin32FileTime(pDateTime=*0x%x, pulWin32FileTime=*0x%x)", pDateTime, pulWin32FileTime);

	return CELL_OK;
}

s32 cellRtcSetCurrentSecureTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcSetCurrentTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcSetConf()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcSetDosTime(vm::ptr<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc.todo("cellRtcSetDosTime(pDateTime=*0x%x, uiDosTime=0x%x)", pDateTime, uiDosTime);

	return CELL_OK;
}

s32 cellRtcGetTickResolution()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcSetTime_t(vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.todo("cellRtcSetTime_t(pDateTime=*0x%x, iTime=0x%llx)", pDateTime, iTime);

	return CELL_OK;
}

s32 cellRtcSetSystemTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

s32 cellRtcSetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc.todo("cellRtcSetWin32FileTime(pDateTime=*0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);

	return CELL_OK;
}

s32 cellRtcIsLeapYear(s32 year)
{
	cellRtc.todo("cellRtcIsLeapYear(year=%d)", year);

	return 0;
}

s32 cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc.todo("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	return 0;
}

s32 cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc.todo("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	return 0;
}

s32 cellRtcCheckValid(vm::ptr<CellRtcDateTime> pTime)
{
	cellRtc.todo("cellRtcCheckValid(pTime=*0x%x)", pTime);

	return CELL_OK;
}

s32 cellRtcCompareTick(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1)
{
	cellRtc.todo("cellRtcCompareTick(pTick0=*0x%x, pTick1=*0x%x)", pTick0, pTick1);

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellRtc)("cellRtc", []()
{
	REG_FUNC(cellRtc, cellRtcGetCurrentTick);
	REG_FUNC(cellRtc, cellRtcGetCurrentClock);
	REG_FUNC(cellRtc, cellRtcGetCurrentClockLocalTime);

	REG_FUNC(cellRtc, cellRtcFormatRfc2822);
	REG_FUNC(cellRtc, cellRtcFormatRfc2822LocalTime);
	REG_FUNC(cellRtc, cellRtcFormatRfc3339);
	REG_FUNC(cellRtc, cellRtcFormatRfc3339LocalTime);
	REG_FUNC(cellRtc, cellRtcParseDateTime);
	REG_FUNC(cellRtc, cellRtcParseRfc3339);

	REG_FUNC(cellRtc, cellRtcGetTick);
	REG_FUNC(cellRtc, cellRtcSetTick);

	REG_FUNC(cellRtc, cellRtcTickAddTicks);
	REG_FUNC(cellRtc, cellRtcTickAddMicroseconds);
	REG_FUNC(cellRtc, cellRtcTickAddSeconds);
	REG_FUNC(cellRtc, cellRtcTickAddMinutes);
	REG_FUNC(cellRtc, cellRtcTickAddHours);
	REG_FUNC(cellRtc, cellRtcTickAddDays);
	REG_FUNC(cellRtc, cellRtcTickAddWeeks);
	REG_FUNC(cellRtc, cellRtcTickAddMonths);
	REG_FUNC(cellRtc, cellRtcTickAddYears);

	REG_FUNC(cellRtc, cellRtcConvertUtcToLocalTime);
	REG_FUNC(cellRtc, cellRtcConvertLocalTimeToUtc);

	REG_FUNC(cellRtc, cellRtcGetCurrentSecureTick);
	REG_FUNC(cellRtc, cellRtcGetDosTime);
	REG_FUNC(cellRtc, cellRtcGetTickResolution);
	REG_FUNC(cellRtc, cellRtcGetSystemTime);
	REG_FUNC(cellRtc, cellRtcGetTime_t);
	REG_FUNC(cellRtc, cellRtcGetWin32FileTime);

	REG_FUNC(cellRtc, cellRtcSetConf);
	REG_FUNC(cellRtc, cellRtcSetCurrentSecureTick);
	REG_FUNC(cellRtc, cellRtcSetCurrentTick);
	REG_FUNC(cellRtc, cellRtcSetDosTime);
	REG_FUNC(cellRtc, cellRtcSetTime_t);
	REG_FUNC(cellRtc, cellRtcSetSystemTime);
	REG_FUNC(cellRtc, cellRtcSetWin32FileTime);

	REG_FUNC(cellRtc, cellRtcIsLeapYear);
	REG_FUNC(cellRtc, cellRtcGetDaysInMonth);
	REG_FUNC(cellRtc, cellRtcGetDayOfWeek);
	REG_FUNC(cellRtc, cellRtcCheckValid);

	REG_FUNC(cellRtc, cellRtcCompareTick);
});
