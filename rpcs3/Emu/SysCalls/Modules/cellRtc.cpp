#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "Utilities/rTime.h"
#include "cellRtc.h"

//void cellRtc_init();
//Module cellRtc(0x0009, cellRtc_init);
Module *cellRtc = nullptr;

s64 convertToUNIXTime(u16 seconds, u16 minutes, u16 hours, u16 days, int years)
{
	return (s64)seconds + (s64)minutes * 60 + (s64)hours * 3600 + (s64)days * 86400 + 
		(s64)(years - 70) * 31536000 + (s64)((years - 69) / 4) * 86400 -
		(s64)((years - 1) / 100) * 86400 + (s64)((years + 299) / 400) * 86400;
}

u64 convertToWin32FILETIME(u16 seconds, u16 minutes, u16 hours, u16 days, int years)
{
	s64 unixtime = convertToUNIXTime(seconds, minutes, hours, days, years);
	u64 win32time = u64(unixtime) * u64(10000000) + u64(116444736000000000);
	u64 win32filetime = win32time | win32time >> 32;
	return win32filetime;
}

int cellRtcGetCurrentTick(vm::ptr<CellRtcTick> pTick)
{
	cellRtc->Log("cellRtcGetCurrentTick(pTick=0x%x)", pTick.addr());

	rDateTime unow = rDateTime::UNow();
	pTick->tick = unow.GetTicks();
	return CELL_OK;
}

int cellRtcGetCurrentClock(vm::ptr<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc->Log("cellRtcGetCurrentClock(pClock=0x%x, time_zone=%d)", pClock.addr(), iTimeZone);

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

int cellRtcGetCurrentClockLocalTime(vm::ptr<CellRtcDateTime> pClock)
{
	cellRtc->Log("cellRtcGetCurrentClockLocalTime(pClock=0x%x)", pClock.addr());

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

int cellRtcFormatRfc2822(u32 pszDateTime_addr, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc->Log("cellRtcFormatRfc2822(pszDateTime_addr=0x%x, pUtc=0x%x, time_zone=%d)", pszDateTime_addr, pUtc.addr(), iTimeZone);

	// Add time_zone as offset in minutes.
	rTimeSpan tz = rTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	rDateTime date = rDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string& str = date.Format("%a, %d %b %Y %T %z", rDateTime::TZ::UTC);
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc2822LocalTime(u32 pszDateTime_addr, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc->Log("cellRtcFormatRfc2822LocalTime(pszDateTime_addr=0x%x, pUtc=0x%x)", pszDateTime_addr, pUtc.addr());

	// Get date from ticks.
	rDateTime date = rDateTime((time_t)pUtc->tick);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string& str = date.Format("%a, %d %b %Y %T %z", rDateTime::TZ::Local);
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339(u32 pszDateTime_addr, vm::ptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc->Log("cellRtcFormatRfc3339(pszDateTime_addr=0x%x, pUtc=0x%x, iTimeZone=%d)", pszDateTime_addr, pUtc.addr(), iTimeZone);
	
	// Add time_zone as offset in minutes.
	rTimeSpan tz = rTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	rDateTime date = rDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string& str = date.Format("%FT%T.%zZ", rDateTime::TZ::UTC);
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339LocalTime(u32 pszDateTime_addr, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc->Log("cellRtcFormatRfc3339LocalTime(pszDateTime_addr=0x%x, pUtc=0x%x)", pszDateTime_addr, pUtc.addr());
	
	// Get date from ticks.
	rDateTime date = rDateTime((time_t) pUtc->tick);
	
	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string& str = date.Format("%FT%T.%zZ", rDateTime::TZ::Local);
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcParseDateTime(vm::ptr<CellRtcTick> pUtc, u32 pszDateTime_addr)
{
	cellRtc->Log("cellRtcParseDateTime(pUtc=0x%x, pszDateTime_addr=0x%x)", pUtc.addr(), pszDateTime_addr);

	// Get date from formatted string.
	rDateTime date;
	const std::string& format = Memory.ReadString(pszDateTime_addr);
	date.ParseDateTime(format);

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcParseRfc3339(vm::ptr<CellRtcTick> pUtc, u32 pszDateTime_addr)
{
	cellRtc->Log("cellRtcParseRfc3339(pUtc=0x%x, pszDateTime_addr=0x%x)", pUtc.addr(), pszDateTime_addr);

	// Get date from RFC3339 formatted string.
	rDateTime date;
	const std::string& format = Memory.ReadString(pszDateTime_addr);
	date.ParseDateTime(format);

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcGetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc->Log("cellRtcGetTick(pTime=0x%x, pTick=0x%x)", pTime.addr(), pTick.addr());

	rDateTime datetime = rDateTime(pTime->day, (rDateTime::Month)pTime->month.ToLE(), pTime->year, pTime->hour, pTime->minute, pTime->second, (pTime->microsecond / 1000));
	pTick->tick = datetime.GetTicks();
	
	return CELL_OK;
}

int cellRtcSetTick(vm::ptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc->Log("cellRtcSetTick(pTime=0x%x, pTick=0x%x)", pTime.addr(), pTick.addr());
	
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

int cellRtcTickAddTicks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc->Log("cellRtcTickAddTicks(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);

	pTick0->tick = pTick1->tick + lAdd;
	return CELL_OK;
}

int cellRtcTickAddMicroseconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc->Log("cellRtcTickAddMicroseconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);
	
	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan microseconds = rTimeSpan(0, 0, 0, lAdd / 1000);
	date.Add(microseconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddSeconds(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc->Log("cellRtcTickAddSeconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan seconds = rTimeSpan(0, 0, lAdd, 0);
	date.Add(seconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMinutes(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc->Log("cellRtcTickAddMinutes(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.addr(), pTick1.addr(), lAdd);
	
	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan minutes = rTimeSpan(0, lAdd, 0, 0); // ???
	date.Add(minutes);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddHours(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc->Log("cellRtcTickAddHours(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rTimeSpan hours = rTimeSpan(iAdd, 0, 0, 0); // ???
	date.Add(hours);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddDays(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc->Log("cellRtcTickAddDays(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan days = rDateSpan(0, 0, 0, iAdd); // ???
	date.Add(days);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddWeeks(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc->Log("cellRtcTickAddWeeks(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan weeks = rDateSpan(0, 0, iAdd, 0);
	date.Add(weeks);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMonths(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc->Log("cellRtcTickAddMonths(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan months = rDateSpan(0, iAdd, 0, 0);
	date.Add(months);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddYears(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc->Log("cellRtcTickAddYears(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.addr(), pTick1.addr(), iAdd);

	rDateTime date = rDateTime((time_t)pTick1->tick);
	rDateSpan years = rDateSpan(iAdd, 0, 0, 0);
	date.Add(years);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcConvertUtcToLocalTime(vm::ptr<CellRtcTick> pUtc, vm::ptr<CellRtcTick> pLocalTime)
{
	cellRtc->Log("cellRtcConvertUtcToLocalTime(pUtc=0x%x, pLocalTime=0x%x)", pUtc.addr(), pLocalTime.addr());

	rDateTime time = rDateTime((time_t)pUtc->tick);
	rDateTime local_time = time.FromUTC(false);
	pLocalTime->tick = local_time.GetTicks();
	return CELL_OK;
}

int cellRtcConvertLocalTimeToUtc(vm::ptr<CellRtcTick> pLocalTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc->Log("cellRtcConvertLocalTimeToUtc(pLocalTime=0x%x, pUtc=0x%x)", pLocalTime.addr(), pUtc.addr());

	rDateTime time = rDateTime((time_t)pLocalTime->tick);
	rDateTime utc_time = time.ToUTC(false);
	pUtc->tick = utc_time.GetTicks();
	return CELL_OK;
}

int cellRtcGetDosTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<be_t<u32>> puiDosTime)
{
	cellRtc->Log("cellRtcGetDosTime(pDateTime=0x%x, puiDosTime=0x%x)", pDateTime.addr(), puiDosTime.addr());

	// Convert to DOS time.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*puiDosTime = date_time.GetAsDOS();

	return CELL_OK;
}

int cellRtcGetTime_t(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<be_t<s64>> piTime)
{
	cellRtc->Log("cellRtcGetTime_t(pDateTime=0x%x, piTime=0x%x)", pDateTime.addr(), piTime.addr());

	// Convert to POSIX time_t.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*piTime = convertToUNIXTime(date_time.GetSecond(rDateTime::TZ::UTC), date_time.GetMinute(rDateTime::TZ::UTC),
		date_time.GetHour(rDateTime::TZ::UTC), date_time.GetDay(rDateTime::TZ::UTC), date_time.GetYear(rDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcGetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, vm::ptr<be_t<u64>> pulWin32FileTime)
{
	cellRtc->Log("cellRtcGetWin32FileTime(pDateTime=0x%x, pulWin32FileTime=0x%x)", pDateTime.addr(), pulWin32FileTime.addr());

	// Convert to WIN32 FILETIME.
	rDateTime date_time = rDateTime(pDateTime->day, (rDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	*pulWin32FileTime = convertToWin32FILETIME(date_time.GetSecond(rDateTime::TZ::UTC), date_time.GetMinute(rDateTime::TZ::UTC),
		date_time.GetHour(rDateTime::TZ::UTC), date_time.GetDay(rDateTime::TZ::UTC), date_time.GetYear(rDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcSetDosTime(vm::ptr<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc->Log("cellRtcSetDosTime(pDateTime=0x%x, uiDosTime=0x%x)", pDateTime.addr(), uiDosTime);

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

int cellRtcSetTime_t(vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc->Log("cellRtcSetTime_t(pDateTime=0x%x, iTime=0x%llx)", pDateTime.addr(), iTime);

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

int cellRtcSetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc->Log("cellRtcSetWin32FileTime(pDateTime=0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);

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

int cellRtcIsLeapYear(s32 year)
{
	cellRtc->Log("cellRtcIsLeapYear(year=%d)", year);

	rDateTime datetime;
	return datetime.IsLeapYear(year, rDateTime::Gregorian);
}

int cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc->Log("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	rDateTime datetime;
	return datetime.GetNumberOfDays((rDateTime::Month) month, year, rDateTime::Gregorian);
}

int cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc->Log("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	rDateTime datetime;
	datetime.SetToWeekDay((rDateTime::WeekDay) day, 1, (rDateTime::Month) month, year);
	return datetime.GetWeekDay();
}

int cellRtcCheckValid(vm::ptr<CellRtcDateTime> pTime)
{
	cellRtc->Log("cellRtcCheckValid(pTime=0x%x)", pTime.addr());

	if ((pTime->year < 1) || (pTime->year > 9999)) return CELL_RTC_ERROR_INVALID_YEAR;
	else if ((pTime->month < 1) || (pTime->month > 12)) return CELL_RTC_ERROR_INVALID_MONTH;
	else if ((pTime->day < 1) || (pTime->day > 31)) return CELL_RTC_ERROR_INVALID_DAY;
	else if ((pTime->hour < 0) || (pTime->hour > 23)) return CELL_RTC_ERROR_INVALID_HOUR;
	else if ((pTime->minute < 0) || (pTime->minute > 59)) return CELL_RTC_ERROR_INVALID_MINUTE;
	else if ((pTime->second < 0) || (pTime->second > 59)) return CELL_RTC_ERROR_INVALID_SECOND;
	else if ((pTime->microsecond < 0) || (pTime->microsecond > 999999)) return CELL_RTC_ERROR_INVALID_MICROSECOND;
	else return CELL_OK;
}

int cellRtcCompareTick(vm::ptr<CellRtcTick> pTick0, vm::ptr<CellRtcTick> pTick1)
{
	cellRtc->Log("cellRtcCompareTick(pTick0=0x%x, pTick1=0x%x)", pTick0.addr(), pTick1.addr());

	if (pTick0->tick < pTick1->tick) return -1;
	else if (pTick0->tick > pTick1->tick) return 1;
	else return CELL_OK;
}

void cellRtc_init()
{
	cellRtc->AddFunc(0x9dafc0d9, cellRtcGetCurrentTick);
	cellRtc->AddFunc(0x32c941cf, cellRtcGetCurrentClock);
	cellRtc->AddFunc(0x2cce9cf5, cellRtcGetCurrentClockLocalTime);

	cellRtc->AddFunc(0x5491b9d5, cellRtcFormatRfc2822);
	cellRtc->AddFunc(0xa07c3d2f, cellRtcFormatRfc2822LocalTime);
	cellRtc->AddFunc(0xd9c0b463, cellRtcFormatRfc3339);
	cellRtc->AddFunc(0x1324948a, cellRtcFormatRfc3339LocalTime);
	cellRtc->AddFunc(0xc5bc0fac, cellRtcParseDateTime);
	cellRtc->AddFunc(0xcf11c3d6, cellRtcParseRfc3339);

	cellRtc->AddFunc(0xc7bdb7eb, cellRtcGetTick);
	cellRtc->AddFunc(0x99b13034, cellRtcSetTick);
	cellRtc->AddFunc(0x269a1882, cellRtcTickAddTicks);
	cellRtc->AddFunc(0xf8509925, cellRtcTickAddMicroseconds);
	cellRtc->AddFunc(0xccce71bd, cellRtcTickAddSeconds);
	cellRtc->AddFunc(0x2f010bfa, cellRtcTickAddMinutes);
	cellRtc->AddFunc(0xd41d3bd2, cellRtcTickAddHours);
	cellRtc->AddFunc(0x75744e2a, cellRtcTickAddDays);
	cellRtc->AddFunc(0x64c63fd5, cellRtcTickAddWeeks);
	cellRtc->AddFunc(0xe0ecbb45, cellRtcTickAddMonths);
	cellRtc->AddFunc(0x332a74dd, cellRtcTickAddYears);
	cellRtc->AddFunc(0xc48d5002, cellRtcConvertUtcToLocalTime);
	cellRtc->AddFunc(0x46ca7fe0, cellRtcConvertLocalTimeToUtc);

	// (TODO: Time Information Manipulation Functions missing)

	cellRtc->AddFunc(0xdfff32cf, cellRtcGetDosTime);
	cellRtc->AddFunc(0xcb90c761, cellRtcGetTime_t);
	cellRtc->AddFunc(0xe7086f05, cellRtcGetWin32FileTime);
	cellRtc->AddFunc(0x9598d4b3, cellRtcSetDosTime);
	cellRtc->AddFunc(0xbb543189, cellRtcSetTime_t);
	cellRtc->AddFunc(0x5f68c268, cellRtcSetWin32FileTime);

	cellRtc->AddFunc(0x5316b4a8, cellRtcIsLeapYear);
	cellRtc->AddFunc(0x5b6a0a1d, cellRtcGetDaysInMonth);
	cellRtc->AddFunc(0xc2d8cf95, cellRtcGetDayOfWeek);
	cellRtc->AddFunc(0x7f1086e6, cellRtcCheckValid);

	cellRtc->AddFunc(0xfb51fc61, cellRtcCompareTick);
}
