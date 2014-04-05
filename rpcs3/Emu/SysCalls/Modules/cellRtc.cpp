#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

#include "cellRtc.h"

void cellRtc_init();
Module cellRtc(0x0009, cellRtc_init);

long convertToUNIXTime(u16 seconds, u16 minutes, u16 hours, u16 days, int years)
{
	return (seconds + minutes*60 + hours*3600 + days*86400 + (years-70)*31536000 + ((years-69)/4)*86400 - ((years-1)/100)*86400 + ((years+299)/400)*86400);
}

u64 convertToWin32FILETIME(u16 seconds, u16 minutes, u16 hours, u16 days, int years)
{
	long unixtime = convertToUNIXTime(seconds, minutes, hours, days, years);
	u64 win32time = u64(unixtime) * u64(10000000) + u64(116444736000000000);
	u64 win32filetime = win32time | win32time >> 32;
	return win32filetime;
}

int cellRtcGetCurrentTick(mem_ptr_t<CellRtcTick> pTick)
{
	cellRtc.Log("cellRtcGetCurrentTick(pTick=0x%x)", pTick.GetAddr());

	if (!pTick.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime unow = wxDateTime::UNow();
	pTick->tick = unow.GetTicks();
	return CELL_OK;
}

int cellRtcGetCurrentClock(mem_ptr_t<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc.Log("cellRtcGetCurrentClock(pClock=0x%x, time_zone=%d)", pClock.GetAddr(), iTimeZone);

	if (!pClock.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime unow = wxDateTime::UNow();

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) iTimeZone, 0, 0);
	unow.Add(tz);

	pClock->year = unow.GetYear(wxDateTime::TZ::UTC);
	pClock->month = unow.GetMonth(wxDateTime::TZ::UTC);
	pClock->day = unow.GetDay(wxDateTime::TZ::UTC);
	pClock->hour = unow.GetHour(wxDateTime::TZ::UTC);
	pClock->minute = unow.GetMinute(wxDateTime::TZ::UTC);
	pClock->second = unow.GetSecond(wxDateTime::TZ::UTC);
	pClock->microsecond = unow.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

int cellRtcGetCurrentClockLocalTime(mem_ptr_t<CellRtcDateTime> pClock)
{
	cellRtc.Log("cellRtcGetCurrentClockLocalTime(pClock=0x%x)", pClock.GetAddr());

	if (!pClock.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime unow = wxDateTime::UNow();

	pClock->year = unow.GetYear(wxDateTime::TZ::Local);
	pClock->month = unow.GetMonth(wxDateTime::TZ::Local);
	pClock->day = unow.GetDay(wxDateTime::TZ::Local);
	pClock->hour = unow.GetHour(wxDateTime::TZ::Local);
	pClock->minute = unow.GetMinute(wxDateTime::TZ::Local);
	pClock->second = unow.GetSecond(wxDateTime::TZ::Local);
	pClock->microsecond = unow.GetMillisecond(wxDateTime::TZ::Local) * 1000;

	return CELL_OK;
}

int cellRtcFormatRfc2822(u32 pszDateTime_addr, mem_ptr_t<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.Log("cellRtcFormatRfc2822(pszDateTime_addr=0x%x, pUtc=0x%x, time_zone=%d)", pszDateTime_addr, pUtc.GetAddr(), iTimeZone);

	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	wxDateTime date = wxDateTime::wxDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string& str = fmt::ToUTF8(date.Format("%a, %d %b %Y %T %z", wxDateTime::TZ::UTC));
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc2822LocalTime(u32 pszDateTime_addr, mem_ptr_t<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcFormatRfc2822LocalTime(pszDateTime_addr=0x%x, pUtc=0x%x)", pszDateTime_addr, pUtc.GetAddr());

	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Get date from ticks.
	wxDateTime date = wxDateTime::wxDateTime((time_t)pUtc->tick);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const std::string& str = fmt::ToUTF8(date.Format("%a, %d %b %Y %T %z", wxDateTime::TZ::Local));
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339(u32 pszDateTime_addr, mem_ptr_t<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.Log("cellRtcFormatRfc3339(pszDateTime_addr=0x%x, pUtc=0x%x, iTimeZone=%d)", pszDateTime_addr, pUtc.GetAddr(), iTimeZone);
	
	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) iTimeZone, 0, 0);

	// Get date from ticks + tz.
	wxDateTime date = wxDateTime::wxDateTime((time_t)pUtc->tick);
	date.Add(tz);

	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string& str = fmt::ToUTF8(date.Format("%FT%T.%zZ", wxDateTime::TZ::UTC));
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339LocalTime(u32 pszDateTime_addr, mem_ptr_t<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcFormatRfc3339LocalTime(pszDateTime_addr=0x%x, pUtc=0x%x)", pszDateTime_addr, pUtc.GetAddr());
	
	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Get date from ticks.
	wxDateTime date = wxDateTime::wxDateTime((time_t) pUtc->tick);
	
	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const std::string& str = fmt::ToUTF8(date.Format("%FT%T.%zZ", wxDateTime::TZ::Local));
	Memory.WriteString(pszDateTime_addr, str);

	return CELL_OK;
}

int cellRtcParseDateTime(mem_ptr_t<CellRtcTick> pUtc, u32 pszDateTime_addr)
{
	cellRtc.Log("cellRtcParseDateTime(pUtc=0x%x, pszDateTime_addr=0x%x)", pUtc.GetAddr(), pszDateTime_addr);

	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;
	
	// Get date from formatted string.
	wxDateTime date;
	const std::string& format = Memory.ReadString(pszDateTime_addr);
	date.ParseDateTime(fmt::FromUTF8(format));

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcParseRfc3339(mem_ptr_t<CellRtcTick> pUtc, u32 pszDateTime_addr)
{
	cellRtc.Log("cellRtcParseRfc3339(pUtc=0x%x, pszDateTime_addr=0x%x)", pUtc.GetAddr(), pszDateTime_addr);

	if (!pUtc.IsGood() || !Memory.IsGoodAddr(pszDateTime_addr))
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Get date from RFC3339 formatted string.
	wxDateTime date;
	const std::string& format = Memory.ReadString(pszDateTime_addr);
	date.ParseDateTime(fmt::FromUTF8(format));

	pUtc->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcGetTick(mem_ptr_t<CellRtcDateTime> pTime, mem_ptr_t<CellRtcTick> pTick)
{
	cellRtc.Log("cellRtcGetTick(pTime=0x%x, pTick=0x%x)", pTime.GetAddr(), pTick.GetAddr());

	if (!pTime.IsGood() || !pTick.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime datetime = wxDateTime::wxDateTime(pTime->day, (wxDateTime::Month)pTime->month.ToLE(), pTime->year, pTime->hour, pTime->minute, pTime->second, (pTime->microsecond / 1000));
	pTick->tick = datetime.GetTicks();
	
	return CELL_OK;
}

int cellRtcSetTick(mem_ptr_t<CellRtcDateTime> pTime, mem_ptr_t<CellRtcTick> pTick)
{
	cellRtc.Log("cellRtcSetTick(pTime=0x%x, pTick=0x%x)", pTime.GetAddr(), pTick.GetAddr());
	
	if (!pTime.IsGood() || !pTick.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick->tick);

	pTime->year = date.GetYear(wxDateTime::TZ::UTC);
	pTime->month = date.GetMonth(wxDateTime::TZ::UTC);
	pTime->day = date.GetDay(wxDateTime::TZ::UTC);
	pTime->hour = date.GetHour(wxDateTime::TZ::UTC);
	pTime->minute = date.GetMinute(wxDateTime::TZ::UTC);
	pTime->second = date.GetSecond(wxDateTime::TZ::UTC);
	pTime->microsecond = date.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

int cellRtcTickAddTicks(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddTicks(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.GetAddr(), pTick1.GetAddr(), lAdd);

	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	pTick0->tick = pTick1->tick + lAdd;
	return CELL_OK;
}

int cellRtcTickAddMicroseconds(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddMicroseconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.GetAddr(), pTick1.GetAddr(), lAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxTimeSpan microseconds = wxTimeSpan::wxTimeSpan(0, 0, 0, lAdd / 1000);
	date.Add(microseconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddSeconds(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddSeconds(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.GetAddr(), pTick1.GetAddr(), lAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxTimeSpan seconds = wxTimeSpan::wxTimeSpan(0, 0, lAdd, 0);
	date.Add(seconds);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMinutes(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.Log("cellRtcTickAddMinutes(pTick0=0x%x, pTick1=0x%x, lAdd=%lld)", pTick0.GetAddr(), pTick1.GetAddr(), lAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxTimeSpan minutes = wxTimeSpan::wxTimeSpan(0, lAdd, 0, 0);
	date.Add(minutes);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddHours(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddHours(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.GetAddr(), pTick1.GetAddr(), iAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxTimeSpan hours = wxTimeSpan::wxTimeSpan(iAdd, 0, 0, 0);
	date.Add(hours);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddDays(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddDays(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.GetAddr(), pTick1.GetAddr(), iAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxDateSpan days = wxDateSpan::wxDateSpan(0, 0, 0, iAdd);
	date.Add(days);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddWeeks(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddWeeks(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.GetAddr(), pTick1.GetAddr(), iAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxDateSpan weeks = wxDateSpan::wxDateSpan(0, 0, iAdd, 0);
	date.Add(weeks);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMonths(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddMonths(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.GetAddr(), pTick1.GetAddr(), iAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxDateSpan months = wxDateSpan::wxDateSpan(0, iAdd, 0, 0);
	date.Add(months);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddYears(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.Log("cellRtcTickAddYears(pTick0=0x%x, pTick1=0x%x, iAdd=%d)", pTick0.GetAddr(), pTick1.GetAddr(), iAdd);
	
	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date = wxDateTime::wxDateTime((time_t)pTick1->tick);
	wxDateSpan years = wxDateSpan::wxDateSpan(iAdd, 0, 0, 0);
	date.Add(years);
	pTick0->tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcConvertUtcToLocalTime(mem_ptr_t<CellRtcTick> pUtc, mem_ptr_t<CellRtcTick> pLocalTime)
{
	cellRtc.Log("cellRtcConvertUtcToLocalTime(pUtc=0x%x, pLocalTime=0x%x)", pUtc.GetAddr(), pLocalTime.GetAddr());

	if (!pUtc.IsGood() || !pLocalTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime time = wxDateTime::wxDateTime((time_t)pUtc->tick);
	wxDateTime local_time = time.FromUTC(false);
	pLocalTime->tick = local_time.GetTicks();
	return CELL_OK;
}

int cellRtcConvertLocalTimeToUtc(mem_ptr_t<CellRtcTick> pLocalTime, mem_ptr_t<CellRtcTick> pUtc)
{
	cellRtc.Log("cellRtcConvertLocalTimeToUtc(pLocalTime=0x%x, pUtc=0x%x)", pLocalTime.GetAddr(), pUtc.GetAddr());

	if (!pLocalTime.IsGood() || !pUtc.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime time = wxDateTime::wxDateTime((time_t)pLocalTime->tick);
	wxDateTime utc_time = time.ToUTC(false);
	pUtc->tick = utc_time.GetTicks();
	return CELL_OK;
}

int cellRtcGetDosTime(mem_ptr_t<CellRtcDateTime> pDateTime, mem32_t puiDosTime)
{
	cellRtc.Log("cellRtcGetDosTime(pDateTime=0x%x, puiDosTime=0x%x)", pDateTime.GetAddr(), puiDosTime.GetAddr());

	if (!pDateTime.IsGood() || !puiDosTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Convert to DOS time.
	wxDateTime date_time = wxDateTime::wxDateTime(pDateTime->day, (wxDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	puiDosTime = date_time.GetAsDOS();

	return CELL_OK;
}

int cellRtcGetTime_t(mem_ptr_t<CellRtcDateTime> pDateTime, mem64_t piTime)
{
	cellRtc.Log("cellRtcGetTime_t(pDateTime=0x%x, piTime=0x%x)", pDateTime.GetAddr(), piTime.GetAddr());

	if (!pDateTime.IsGood() || !piTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Convert to POSIX time_t.
	wxDateTime date_time = wxDateTime::wxDateTime(pDateTime->day, (wxDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	piTime = convertToUNIXTime(date_time.GetSecond(wxDateTime::TZ::UTC), date_time.GetMinute(wxDateTime::TZ::UTC),
		date_time.GetHour(wxDateTime::TZ::UTC), date_time.GetDay(wxDateTime::TZ::UTC), date_time.GetYear(wxDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcGetWin32FileTime(mem_ptr_t<CellRtcDateTime> pDateTime, mem64_t pulWin32FileTime)
{
	cellRtc.Log("cellRtcGetWin32FileTime(pDateTime=0x%x, pulWin32FileTime=0x%x)", pDateTime.GetAddr(), pulWin32FileTime.GetAddr());

	if (!pDateTime.IsGood() || !pulWin32FileTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	// Convert to WIN32 FILETIME.
	wxDateTime date_time = wxDateTime::wxDateTime(pDateTime->day, (wxDateTime::Month)pDateTime->month.ToLE(), pDateTime->year, pDateTime->hour, pDateTime->minute, pDateTime->second, (pDateTime->microsecond / 1000));
	pulWin32FileTime = convertToWin32FILETIME(date_time.GetSecond(wxDateTime::TZ::UTC), date_time.GetMinute(wxDateTime::TZ::UTC),
		date_time.GetHour(wxDateTime::TZ::UTC), date_time.GetDay(wxDateTime::TZ::UTC), date_time.GetYear(wxDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcSetDosTime(mem_ptr_t<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc.Log("cellRtcSetDosTime(pDateTime=0x%x, uiDosTime=0x%x)", pDateTime.GetAddr(), uiDosTime);
	
	if (!pDateTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date_time;
	wxDateTime dos_time = date_time.SetFromDOS(uiDosTime);
	
	pDateTime->year = dos_time.GetYear(wxDateTime::TZ::UTC);
	pDateTime->month = dos_time.GetMonth(wxDateTime::TZ::UTC);
	pDateTime->day = dos_time.GetDay(wxDateTime::TZ::UTC);
	pDateTime->hour = dos_time.GetHour(wxDateTime::TZ::UTC);
	pDateTime->minute = dos_time.GetMinute(wxDateTime::TZ::UTC);
	pDateTime->second = dos_time.GetSecond(wxDateTime::TZ::UTC);
	pDateTime->microsecond = dos_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

int cellRtcSetTime_t(mem_ptr_t<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.Log("cellRtcSetTime_t(pDateTime=0x%x, iTime=0x%llx)", pDateTime.GetAddr(), iTime);
	
	if (!pDateTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date_time = wxDateTime::wxDateTime((time_t)iTime);
	
	pDateTime->year = date_time.GetYear(wxDateTime::TZ::UTC);
	pDateTime->month = date_time.GetMonth(wxDateTime::TZ::UTC);
	pDateTime->day = date_time.GetDay(wxDateTime::TZ::UTC);
	pDateTime->hour = date_time.GetHour(wxDateTime::TZ::UTC);
	pDateTime->minute = date_time.GetMinute(wxDateTime::TZ::UTC);
	pDateTime->second = date_time.GetSecond(wxDateTime::TZ::UTC);
	pDateTime->microsecond = date_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

int cellRtcSetWin32FileTime(mem_ptr_t<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc.Log("cellRtcSetWin32FileTime(pDateTime=0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);
	
	if (!pDateTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	wxDateTime date_time = wxDateTime::wxDateTime((time_t)ulWin32FileTime);
	
	pDateTime->year = date_time.GetYear(wxDateTime::TZ::UTC);
	pDateTime->month = date_time.GetMonth(wxDateTime::TZ::UTC);
	pDateTime->day = date_time.GetDay(wxDateTime::TZ::UTC);
	pDateTime->hour = date_time.GetHour(wxDateTime::TZ::UTC);
	pDateTime->minute = date_time.GetMinute(wxDateTime::TZ::UTC);
	pDateTime->second = date_time.GetSecond(wxDateTime::TZ::UTC);
	pDateTime->microsecond = date_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	return CELL_OK;
}

int cellRtcIsLeapYear(s32 year)
{
	cellRtc.Log("cellRtcIsLeapYear(year=%d)", year);

	wxDateTime datetime;
	return datetime.IsLeapYear(year, wxDateTime::Gregorian);
}

int cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc.Log("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	wxDateTime datetime;
	return datetime.GetNumberOfDays((wxDateTime::Month) month, year, wxDateTime::Gregorian);
}

int cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc.Log("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	wxDateTime datetime;
	datetime.SetToWeekDay((wxDateTime::WeekDay) day, 1, (wxDateTime::Month) month, year);
	return datetime.GetWeekDay();
}

int cellRtcCheckValid(mem_ptr_t<CellRtcDateTime> pTime)
{
	cellRtc.Log("cellRtcCheckValid(pTime=0x%x)", pTime.GetAddr());
	
	if (!pTime.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	if ((pTime->year < 1) || (pTime->year > 9999)) return CELL_RTC_ERROR_INVALID_YEAR;
	else if ((pTime->month < 1) || (pTime->month > 12)) return CELL_RTC_ERROR_INVALID_MONTH;
	else if ((pTime->day < 1) || (pTime->day > 31)) return CELL_RTC_ERROR_INVALID_DAY;
	else if ((pTime->hour < 0) || (pTime->hour > 23)) return CELL_RTC_ERROR_INVALID_HOUR;
	else if ((pTime->minute < 0) || (pTime->minute > 59)) return CELL_RTC_ERROR_INVALID_MINUTE;
	else if ((pTime->second < 0) || (pTime->second > 59)) return CELL_RTC_ERROR_INVALID_SECOND;
	else if ((pTime->microsecond < 0) || (pTime->microsecond > 999999)) return CELL_RTC_ERROR_INVALID_MICROSECOND;
	else return CELL_OK;
}

int cellRtcCompareTick(mem_ptr_t<CellRtcTick> pTick0, mem_ptr_t<CellRtcTick> pTick1)
{
	cellRtc.Log("cellRtcCompareTick(pTick0=0x%x, pTick1=0x%x)", pTick0.GetAddr(), pTick1.GetAddr());

	if (!pTick0.IsGood() || !pTick1.IsGood())
		return CELL_RTC_ERROR_INVALID_POINTER;

	if (pTick0->tick < pTick1->tick) return -1;
	else if (pTick0->tick > pTick1->tick) return 1;
	else return CELL_OK;
}

void cellRtc_init()
{
	cellRtc.AddFunc(0x9dafc0d9, cellRtcGetCurrentTick);
	cellRtc.AddFunc(0x32c941cf, cellRtcGetCurrentClock);
	cellRtc.AddFunc(0x2cce9cf5, cellRtcGetCurrentClockLocalTime);

	cellRtc.AddFunc(0x5491b9d5, cellRtcFormatRfc2822);
	cellRtc.AddFunc(0xa07c3d2f, cellRtcFormatRfc2822LocalTime);
	cellRtc.AddFunc(0xd9c0b463, cellRtcFormatRfc3339);
	cellRtc.AddFunc(0x1324948a, cellRtcFormatRfc3339LocalTime);
	cellRtc.AddFunc(0xc5bc0fac, cellRtcParseDateTime);
	cellRtc.AddFunc(0xcf11c3d6, cellRtcParseRfc3339);

	cellRtc.AddFunc(0xc7bdb7eb, cellRtcGetTick);
	cellRtc.AddFunc(0x99b13034, cellRtcSetTick);
	cellRtc.AddFunc(0x269a1882, cellRtcTickAddTicks);
	cellRtc.AddFunc(0xf8509925, cellRtcTickAddMicroseconds);
	cellRtc.AddFunc(0xccce71bd, cellRtcTickAddSeconds);
	cellRtc.AddFunc(0x2f010bfa, cellRtcTickAddMinutes);
	cellRtc.AddFunc(0xd41d3bd2, cellRtcTickAddHours);
	cellRtc.AddFunc(0x75744e2a, cellRtcTickAddDays);
	cellRtc.AddFunc(0x64c63fd5, cellRtcTickAddWeeks);
	cellRtc.AddFunc(0xe0ecbb45, cellRtcTickAddMonths);
	cellRtc.AddFunc(0x332a74dd, cellRtcTickAddYears);
	cellRtc.AddFunc(0xc48d5002, cellRtcConvertUtcToLocalTime);
	cellRtc.AddFunc(0x46ca7fe0, cellRtcConvertLocalTimeToUtc);

	// (TODO: Time Information Manipulation Functions missing)

	cellRtc.AddFunc(0xdfff32cf, cellRtcGetDosTime);
	cellRtc.AddFunc(0xcb90c761, cellRtcGetTime_t);
	cellRtc.AddFunc(0xe7086f05, cellRtcGetWin32FileTime);
	cellRtc.AddFunc(0x9598d4b3, cellRtcSetDosTime);
	cellRtc.AddFunc(0xbb543189, cellRtcSetTime_t);
	cellRtc.AddFunc(0x5f68c268, cellRtcSetWin32FileTime);

	cellRtc.AddFunc(0x5316b4a8, cellRtcIsLeapYear);
	cellRtc.AddFunc(0x5b6a0a1d, cellRtcGetDaysInMonth);
	cellRtc.AddFunc(0xc2d8cf95, cellRtcGetDayOfWeek);
	cellRtc.AddFunc(0x7f1086e6, cellRtcCheckValid);

	cellRtc.AddFunc(0xfb51fc61, cellRtcCompareTick);
}
