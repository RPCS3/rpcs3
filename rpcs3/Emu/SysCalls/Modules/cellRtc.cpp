#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "Utilities/rTime.h"
#include "cellRtc.h"

extern Module cellRtc;

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
	cellRtc.Log("cellRtcGetCurrentTick(pTick=0x%x)", pTick.addr());

	rDateTime unow = rDateTime::UNow();
	pTick->tick = unow.GetTicks();
	return CELL_OK;
}

s32 cellRtcGetCurrentClock(vm::ptr<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc.Log("cellRtcGetCurrentClock(pClock=0x%x, time_zone=%d)", pClock.addr(), iTimeZone);

	rDateTime unow = rDateTime::UNow();

	// Add time_zone as offset in minutes.
	rTimeSpan tz = rTimeSpan(0, (long) iTimeZone, 0, 0);
	unow.Add(tz);

	pClock->year = unow.GetYear(rDateTime::TZ::UTC);
	pClock->month = unow.GetMonth(rDateTime::TZ::UTC);
	pClock->day = unow.GetDay(rDateTime::TZ::UTC);
	pClock->hour = unow.GetHour(rDateTime::TZ::UTC);
	pClock->minute = unow.GetMinute(rDateTime::TZ::UTC);
	pClock->second = unow.GetSecond(rDateTime::TZ::UTC);
	pClock->microsecond = unow.GetMillisecond(rDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

s32 cellRtcGetCurrentClockLocalTime(vm::ptr<CellRtcDateTime> pClock)
{
	cellRtc.Log("cellRtcGetCurrentClockLocalTime(pClock=0x%x)", pClock.addr());

	rDateTime unow = rDateTime::UNow();

	pClock->year = unow.GetYear(rDateTime::TZ::Local);
	pClock->month = unow.GetMonth(rDateTime::TZ::Local);
	pClock->day = unow.GetDay(rDateTime::TZ::Local);
	pClock->hour = unow.GetHour(rDateTime::TZ::Local);
	pClock->minute = unow.GetMinute(rDateTime::TZ::Local);
	pClock->second = unow.GetSecond(rDateTime::TZ::Local);
	pClock->microsecond = unow.GetMillisecond(rDateTime::TZ::Local) * 1000;

	return CELL_OK;
}

s32 cellRtcFormatRfc2822(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.Log("cellRtcFormatRfc2822(pszDateTime=*0x%x, pUtc=*0x%x, time_zone=%d)", pszDateTime, pUtc, iTimeZone);

	// Add time_zone as offset in minutes.
	rTimeSpan tz = rTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	rDateTime date = rDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string str = date.Format("%a, %d %b %Y %T %z", rDateTime::TZ::UTC);
	memcpy(pszDateTime.get_ptr(), str.c_str(), str.size() + 1);

	return CELL_OK;
}

s32 cellRtcFormatRfc2822LocalTime(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcFormatRfc2822LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	// Get date from ticks.
	rDateTime date = rDateTime((time_t)pUtc->tick);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string str = date.Format("%a, %d %b %Y %T %z", rDateTime::TZ::Local);
	memcpy(pszDateTime.get_ptr(), str.c_str(), str.size() + 1);

	return CELL_OK;
}

s32 cellRtcFormatRfc3339(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.Log("cellRtcFormatRfc3339(pszDateTime=*0x%x, pUtc=*0x%x, iTimeZone=%d)", pszDateTime, pUtc, iTimeZone);
	
	// Add time_zone as offset in minutes.
	rTimeSpan tz = rTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	rDateTime date = rDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string str = date.Format("%FT%T.%zZ", rDateTime::TZ::UTC);
	memcpy(pszDateTime.get_ptr(), str.c_str(), str.size() + 1);

	return CELL_OK;
}

s32 cellRtcFormatRfc3339LocalTime(vm::ptr<char> pszDateTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcFormatRfc3339LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);
	
	// Get date from ticks.
	rDateTime date = rDateTime((time_t) pUtc->tick);
	
	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string str = date.Format("%FT%T.%zZ", rDateTime::TZ::Local);
	memcpy(pszDateTime.get_ptr(), str.c_str(), str.size() + 1);

	return CELL_OK;
}

s32 cellRtcParseDateTime(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.Log("cellRtcParseDateTime(pUtc=*0x%x, pszDateTime=*0x%x)", pUtc, pszDateTime);

	// Get date from formatted string.
	rDateTime date;
	date.ParseDateTime(pszDateTime.get_ptr());

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcParseRfc3339(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.Log("cellRtcParseRfc3339(pUtc=*0x%x, pszDateTime=*0x%x)", pUtc, pszDateTime);

	// Get date from RFC3339 formatted string.
	rDateTime date;
	date.ParseDateTime(pszDateTime.get_ptr());

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcGetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.Log("cellRtcGetTick(pTime=0x%x, pTick=0x%x)", pTime.addr(), pTick.addr());

	rDateTime datetime = rDateTime(pTime->day, (rDateTime::Month)pTime->month.value(), pTime->year, pTime->hour, pTime->minute, pTime->second, (pTime->microsecond / 1000));
	pTick->tick = datetime.GetTicks();
	
	return CELL_OK;
}

s32 cellRtcSetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.Log("cellRtcSetTick(pTime=0x%x, pTick=0x%x)", pTime.addr(), pTick.addr());
	
	rDateTime date = rDateTime((time_t)pTick->tick);

	pTime->year = date.GetYear(rDateTime::TZ::UTC);
	pTime->month = date.GetMonth(rDateTime::TZ::UTC);
	pTime->day = date.GetDay(rDateTime::TZ::UTC);
	pTime->hour = date.GetHour(rDateTime::TZ::UTC);
	pTime->minute = date.GetMinute(rDateTime::TZ::UTC);
	pTime->second = date.GetSecond(rDateTime::TZ::UTC);
	pTime->microsecond = date.GetMillisecond(rDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

s32 cellRtcTickAddTicks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddTicks(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);

	pTick0->tick = pTick1->tick + lAdd;
	return CELL_OK;
}

s32 cellRtcTickAddMicroseconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddMicroseconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);
	
	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan microseconds = rTimeSpan(0, 0, 0, lAdd / 1000);
	date.Add(microseconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddSeconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddSeconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan seconds = rTimeSpan(0, 0, lAdd, 0);
	date.Add(seconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddMinutes(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddMinutes(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);
	
	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan minutes = rTimeSpan(0, lAdd, 0, 0); // ???
	date.Add(minutes);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddHours(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddHours(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan hours = rTimeSpan(iAdd, 0, 0, 0); // ???
	date.Add(hours);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddDays(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddDays(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan days = rDateSpan(0, 0, 0, iAdd); // ???
	date.Add(days);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddWeeks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddWeeks(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan weeks = rDateSpan(0, 0, iAdd, 0);
	date.Add(weeks);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddMonths(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddMonths(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan months = rDateSpan(0, iAdd, 0, 0);
	date.Add(months);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcTickAddYears(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddYears(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan years = rDateSpan(iAdd, 0, 0, 0);
	date.Add(years);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

s32 cellRtcConvertUtcToLocalTime(vm::ptr<CellRtcTick> pUtc, vm::ptr<CellRtcTick> pLocalTime)
{
	cellRtc.Log("cellRtcConvertUtcToLocalTime(pUtc=0x%x, pLocalTime=0x%x)", pUtc.addr(), pLocalTime.addr());

	rDateTime time = rDateTime((time_t)pUtc->tick);
	rDateTime local_time = time.FromUTC(false);
	pLocalTime->tick = local_time.GetTicks();
	return CELL_OK;
}

s32 cellRtcConvertLocalTimeToUtc(vm::ptr<CellRtcTick> pLocalTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcConvertLocalTimeToUtc(pLocalTime=0x%x, pUtc=0x%x)", pLocalTime.addr(), pUtc.addr());

	rDateTime time = rDateTime((time_t)pLocalTime->tick);
	rDateTime utc_time = time.ToUTC(false);
	pUtc->tick = utc_time.GetTicks();
	return CELL_OK;
}

s32 cellRtcGetDosTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<u32> puiDosTime)
{
	cellRtc.Log("cellRtcGetDosTime(pDateTime=0x%x, puiDosTime=0x%x)", pDateTime.addr(), puiDosTime.addr());

	// Convert to DOS time.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.value(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*puiDosTime = date_time.GetAsDOS();

	return CELL_OK;
}

s32 cellRtcGetTime_t(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<s64> piTime)
{
	cellRtc.Log("cellRtcGetTime_t(pDateTime=0x%x, piTime=0x%x)", pDateTime.addr(), piTime.addr());

	// Convert to POSIX time_t.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.value(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*piTime = convertToUNIXTime(date_time.GetSecond(rDateTime::TZ::UTC), date_time.GetMinute(rDateTime::TZ::UTC),
		date_time.GetHour(rDateTime::TZ::UTC), date_time.GetDay(rDateTime::TZ::UTC), date_time.GetYear(rDateTime::TZ::UTC));

	return CELL_OK;
}

s32 cellRtcGetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<u64> pulWin32FileTime)
{
	cellRtc.Log("cellRtcGetWin32FileTime(pDateTime=0x%x, pulWin32FileTime=0x%x)", pDateTime.addr(), pulWin32FileTime.addr());

	// Convert to WIN32 FILETIME.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.value(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*pulWin32FileTime = convertToWin32FILETIME(date_time.GetSecond(rDateTime::TZ::UTC), date_time.GetMinute(rDateTime::TZ::UTC),
		date_time.GetHour(rDateTime::TZ::UTC), date_time.GetDay(rDateTime::TZ::UTC), date_time.GetYear(rDateTime::TZ::UTC));

	return CELL_OK;
}

s32 cellRtcSetDosTime(vm::ptr<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc.Log("cellRtcSetDosTime(pDateTime=0x%x, uiDosTime=0x%x)", pDateTime.addr(), uiDosTime);

	rDateTime date_time;
	rDateTime dos_time = date_time.SetFromDOS(uiDosTime);
	
	pDateTime->year = dos_time.GetYear(rDateTime::TZ::UTC);
	pDateTime->month = dos_time.GetMonth(rDateTime::TZ::UTC);
	pDateTime->day = dos_time.GetDay(rDateTime::TZ::UTC);
	pDateTime->hour = dos_time.GetHour(rDateTime::TZ::UTC);
	pDateTime->minute = dos_time.GetMinute(rDateTime::TZ::UTC);
	pDateTime->second = dos_time.GetSecond(rDateTime::TZ::UTC);
	pDateTime->microsecond = dos_time.GetMillisecond(rDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

s32 cellRtcSetTime_t(vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.Log("cellRtcSetTime_t(pDateTime=0x%x, iTime=0x%llx)", pDateTime.addr(), iTime);

	rDateTime date_time = rDateTime((time_t)iTime);
	
	pDateTime->year = date_time.GetYear(rDateTime::TZ::UTC);
	pDateTime->month = date_time.GetMonth(rDateTime::TZ::UTC);
	pDateTime->day = date_time.GetDay(rDateTime::TZ::UTC);
	pDateTime->hour = date_time.GetHour(rDateTime::TZ::UTC);
	pDateTime->minute = date_time.GetMinute(rDateTime::TZ::UTC);
	pDateTime->second = date_time.GetSecond(rDateTime::TZ::UTC);
	pDateTime->microsecond = date_time.GetMillisecond(rDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

s32 cellRtcSetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc.Log("cellRtcSetWin32FileTime(pDateTime=*0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);

	rDateTime date_time = rDateTime((time_t)ulWin32FileTime);
	
	pDateTime->year = date_time.GetYear(rDateTime::TZ::UTC);
	pDateTime->month = date_time.GetMonth(rDateTime::TZ::UTC);
	pDateTime->day = date_time.GetDay(rDateTime::TZ::UTC);
	pDateTime->hour = date_time.GetHour(rDateTime::TZ::UTC);
	pDateTime->minute = date_time.GetMinute(rDateTime::TZ::UTC);
	pDateTime->second = date_time.GetSecond(rDateTime::TZ::UTC);
	pDateTime->microsecond = date_time.GetMillisecond(rDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

s32 cellRtcIsLeapYear(s32 year)
{
	cellRtc.Log("cellRtcIsLeapYear(year=%d)", year);

	rDateTime datetime;
	return datetime.IsLeapYear(year, rDateTime::Gregorian);
}

s32 cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc.Log("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	rDateTime datetime;
	return datetime.GetNumberOfDays((rDateTime::Month) month, year, rDateTime::Gregorian);
}

s32 cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc.Log("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	rDateTime datetime;
	datetime.SetToWeekDay((rDateTime::WeekDay) day, 1, (rDateTime::Month) month, year);
	return datetime.GetWeekDay();
}

s32 cellRtcCheckValid(vm::ptr<CellRtcDateTime> pTime)
{
	cellRtc.Log("cellRtcCheckValid(pTime=0x%x)", pTime.addr());

	if ((pTime->year < 1) || (pTime->year > 9999)) return CELL_RTC_ERROR_INVALID_YEAR;
	else if ((pTime->month < 1) || (pTime->month > 12)) return CELL_RTC_ERROR_INVALID_MONTH;
	else if ((pTime->day < 1) || (pTime->day > 31)) return CELL_RTC_ERROR_INVALID_DAY;
	else if (pTime->hour > 23) return CELL_RTC_ERROR_INVALID_HOUR;
	else if (pTime->minute > 59) return CELL_RTC_ERROR_INVALID_MINUTE;
	else if (pTime->second > 59) return CELL_RTC_ERROR_INVALID_SECOND;
	else if (pTime->microsecond > 999999) return CELL_RTC_ERROR_INVALID_MICROSECOND;
	else return CELL_OK;
}

s32 cellRtcCompareTick(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1)
{
	cellRtc.Log("cellRtcCompareTick(pTick0=0x%x, pTick1=0x%x)", pTick0.addr(), pTick1.addr());

	if (pTick0->tick < pTick1->tick) return -1;
	else if (pTick0->tick > pTick1->tick) return 1;
	else return CELL_OK;
}

Module cellRtc("cellRtc", []()
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

	// (TODO: Time Information Manipulation Functions missing)

	REG_FUNC(cellRtc, cellRtcGetDosTime);
	REG_FUNC(cellRtc, cellRtcGetTime_t);
	REG_FUNC(cellRtc, cellRtcGetWin32FileTime);
	REG_FUNC(cellRtc, cellRtcSetDosTime);
	REG_FUNC(cellRtc, cellRtcSetTime_t);
	REG_FUNC(cellRtc, cellRtcSetWin32FileTime);

	REG_FUNC(cellRtc, cellRtcIsLeapYear);
	REG_FUNC(cellRtc, cellRtcGetDaysInMonth);
	REG_FUNC(cellRtc, cellRtcGetDayOfWeek);
	REG_FUNC(cellRtc, cellRtcCheckValid);

	REG_FUNC(cellRtc, cellRtcCompareTick);
});
