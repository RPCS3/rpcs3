#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellRtc_init();
Module cellRtc(0x0009, cellRtc_init);

// Return Codes
enum
{
	CELL_RTC_ERROR_NOT_INITIALIZED		= 0x80010601,
	CELL_RTC_ERROR_INVALID_POINTER		= 0x80010602,
	CELL_RTC_ERROR_INVALID_VALUE		= 0x80010603,
	CELL_RTC_ERROR_INVALID_ARG			= 0x80010604,
	CELL_RTC_ERROR_NOT_SUPPORTED		= 0x80010605,
	CELL_RTC_ERROR_NO_CLOCK				= 0x80010606,
	CELL_RTC_ERROR_BAD_PARSE			= 0x80010607,
	CELL_RTC_ERROR_INVALID_YEAR			= 0x80010621,
	CELL_RTC_ERROR_INVALID_MONTH		= 0x80010622,
	CELL_RTC_ERROR_INVALID_DAY			= 0x80010623,
	CELL_RTC_ERROR_INVALID_HOUR			= 0x80010624,
	CELL_RTC_ERROR_INVALID_MINUTE		= 0x80010625,
	CELL_RTC_ERROR_INVALID_SECOND		= 0x80010626,
	CELL_RTC_ERROR_INVALID_MICROSECOND	= 0x80010627,
};

struct CellRtcTick
{
	u64 tick;
};

struct CellRtcDateTime
{
	u16 year;
	u16 month;
	u16 day;
	u16 hour;
	u16 minute;
	u16 second;
	u32 microsecond;
};

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

int cellRtcGetCurrentTick(mem64_t tick)
{
	cellRtc.Log("cellRtcGetCurrentTick(tick_addr=0x%x)", tick.GetAddr());
	wxDateTime unow = wxDateTime::UNow();
	tick = unow.GetTicks();
	return CELL_OK;
}

int cellRtcGetCurrentClock(u32 clock_addr, int time_zone)
{
	cellRtc.Log("cellRtcGetCurrentClock(clock_addr=0x%x, time_zone=%d)", clock_addr, time_zone);
	wxDateTime unow = wxDateTime::UNow();

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) time_zone, 0, 0);
	unow.Add(tz);

	Memory.Write16(clock_addr, unow.GetYear(wxDateTime::TZ::UTC));
	Memory.Write16(clock_addr + 2, unow.GetMonth(wxDateTime::TZ::UTC));
	Memory.Write16(clock_addr + 4, unow.GetDay(wxDateTime::TZ::UTC));
	Memory.Write16(clock_addr + 6, unow.GetHour(wxDateTime::TZ::UTC));
	Memory.Write16(clock_addr + 8, unow.GetMinute(wxDateTime::TZ::UTC));
	Memory.Write16(clock_addr + 10, unow.GetSecond(wxDateTime::TZ::UTC));
	Memory.Write32(clock_addr + 12, unow.GetMillisecond(wxDateTime::TZ::UTC) * 1000);

	return CELL_OK;
}

int cellRtcGetCurrentClockLocalTime(u32 clock_addr)
{
	cellRtc.Log("cellRtcGetCurrentClockLocalTime(clock_addr=0x%x)", clock_addr);
	wxDateTime unow = wxDateTime::UNow();

	Memory.Write16(clock_addr, unow.GetYear(wxDateTime::TZ::Local));
	Memory.Write16(clock_addr + 2, unow.GetMonth(wxDateTime::TZ::Local));
	Memory.Write16(clock_addr + 4, unow.GetDay(wxDateTime::TZ::Local));
	Memory.Write16(clock_addr + 6, unow.GetHour(wxDateTime::TZ::Local));
	Memory.Write16(clock_addr + 8, unow.GetMinute(wxDateTime::TZ::Local));
	Memory.Write16(clock_addr + 10, unow.GetSecond(wxDateTime::TZ::Local));
	Memory.Write32(clock_addr + 12, unow.GetMillisecond(wxDateTime::TZ::Local) * 1000);

	return CELL_OK;
}

int cellRtcFormatRfc2822(u32 rfc_addr, u32 tick_addr, int time_zone)
{
	cellRtc.Log("cellRtcFormatRfc2822(rfc_addr=0x%x, tick_addr=0x%x, time_zone=%d)", rfc_addr, tick_addr, time_zone);
	CellRtcTick current_tick;
	current_tick.tick = Memory.Read64(tick_addr);

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) time_zone, 0, 0);

	// Get date from ticks + tz.
	wxDateTime date = wxDateTime::wxDateTime((time_t)current_tick.tick);
	date.Add(tz);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const wxString& str = date.Format("%a, %d %b %Y %T %z", wxDateTime::TZ::UTC);
	Memory.WriteString(rfc_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc2822LocalTime(u32 rfc_addr, u32 tick_addr)
{
	cellRtc.Log("cellRtcFormatRfc2822LocalTime(rfc_addr=0x%x, tick_addr=0x%x)", rfc_addr, tick_addr);
	CellRtcTick current_tick;
	current_tick.tick = Memory.Read64(tick_addr);

	// Get date from ticks.
	wxDateTime date = wxDateTime::wxDateTime((time_t)current_tick.tick);

	// Format date string in RFC2822 format (e.g.: Mon, 01 Jan 1990 12:00:00 +0000).
	const wxString& str = date.Format("%a, %d %b %Y %T %z", wxDateTime::TZ::Local);
	Memory.WriteString(rfc_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339(u32 rfc_addr, u32 tick_addr, int time_zone)
{
	cellRtc.Log("cellRtcFormatRfc3339(rfc_addr=0x%x, tick_addr=0x%x, time_zone=%d)", rfc_addr, tick_addr, time_zone);
	CellRtcTick current_tick;
	current_tick.tick = Memory.Read64(tick_addr);

	// Add time_zone as offset in minutes.
	wxTimeSpan tz = wxTimeSpan::wxTimeSpan(0, (long) time_zone, 0, 0);

	// Get date from ticks + tz.
	wxDateTime date = wxDateTime::wxDateTime((time_t)current_tick.tick);
	date.Add(tz);

	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const wxString& str = date.Format("%FT%T.%zZ", wxDateTime::TZ::UTC);
	Memory.WriteString(rfc_addr, str);

	return CELL_OK;
}

int cellRtcFormatRfc3339LocalTime(u32 rfc_addr, u32 tick_addr)
{
	cellRtc.Log("cellRtcFormatRfc3339LocalTime(rfc_addr=0x%x, tick_addr=0x%x)", rfc_addr, tick_addr);
	CellRtcTick current_tick;
	current_tick.tick = Memory.Read64(tick_addr);

	// Get date from ticks.
	wxDateTime date = wxDateTime::wxDateTime((time_t)current_tick.tick);
	
	// Format date string in RFC3339 format (e.g.: 1990-01-01T12:00:00.00Z).
	const wxString& str = date.Format("%FT%T.%zZ", wxDateTime::TZ::Local);
	Memory.WriteString(rfc_addr, str);

	return CELL_OK;
}

int cellRtcParseDateTime(mem64_t tick, u32 datetime_addr)
{
	cellRtc.Log("cellRtcParseDateTime(tick_addr=0x%x, datetime_addr=0x%x)", tick.GetAddr(), datetime_addr);

	const wxString& format = Memory.ReadString(datetime_addr);

	// Get date from formatted string.
	wxDateTime date;
	date.ParseDateTime(format);

	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcParseRfc3339(mem64_t tick, u32 datetime_addr)
{
	cellRtc.Log("cellRtcParseRfc3339(tick_addr=0x%x, datetime_addr=0x%x)", tick.GetAddr(), datetime_addr);

	const wxString& format = Memory.ReadString(datetime_addr);

	// Get date from RFC3339 formatted string.
	wxDateTime date;
	date.ParseDateTime(format);

	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcGetTick(u32 clock_addr, mem64_t tick)
{
	cellRtc.Log("cellRtcGetTick(clock_addr=0x%x, tick_addr=0x%x)", clock_addr, tick.GetAddr());

	CellRtcDateTime clock;
	clock.year = Memory.Read16(clock_addr);
	clock.month = Memory.Read16(clock_addr + 2);
	clock.day = Memory.Read16(clock_addr + 4);
	clock.hour = Memory.Read16(clock_addr + 6);
	clock.minute = Memory.Read16(clock_addr + 8);
	clock.second = Memory.Read16(clock_addr + 10);
	clock.microsecond = Memory.Read32(clock_addr + 12);

	wxDateTime datetime = wxDateTime::wxDateTime(clock.day, (wxDateTime::Month)clock.month, clock.year, clock.hour, clock.minute, clock.second, (clock.microsecond / 1000));
	tick = datetime.GetTicks();
	
	return CELL_OK;
}

int cellRtcSetTick(u32 clock_addr, u32 tick_addr)
{
	cellRtc.Log("cellRtcSetTick(clock_addr=0x%x, tick_addr=0x%x)", clock_addr, tick_addr);
	CellRtcTick current_tick;
	current_tick.tick = Memory.Read64(tick_addr);

	wxDateTime date = wxDateTime::wxDateTime((time_t)current_tick.tick);

	CellRtcDateTime clock;
	clock.year = date.GetYear(wxDateTime::TZ::UTC);
	clock.month = date.GetMonth(wxDateTime::TZ::UTC);
	clock.day = date.GetDay(wxDateTime::TZ::UTC);
	clock.hour = date.GetHour(wxDateTime::TZ::UTC);
	clock.minute = date.GetMinute(wxDateTime::TZ::UTC);
	clock.second = date.GetSecond(wxDateTime::TZ::UTC);
	clock.microsecond = date.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	Memory.Write16(clock_addr, clock.year);
	Memory.Write16(clock_addr + 2, clock.month);
	Memory.Write16(clock_addr + 4, clock.day);
	Memory.Write16(clock_addr + 6, clock.hour);
	Memory.Write16(clock_addr + 8, clock.minute);
	Memory.Write16(clock_addr + 10, clock.second);
	Memory.Write32(clock_addr + 12, clock.microsecond);

	return CELL_OK;
}

int cellRtcTickAddTicks(mem64_t tick, u32 tick_add_addr, long add)
{
	cellRtc.Log("cellRtcTickAddTicks(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	tick = Memory.Read64(tick_add_addr) + add;

	return CELL_OK;
}

int cellRtcTickAddMicroseconds(mem64_t tick, u32 tick_add_addr, long add)
{
	cellRtc.Log("cellRtcTickAddMicroseconds(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxTimeSpan microseconds = wxTimeSpan::wxTimeSpan(0, 0, 0, add / 1000);
	date.Add(microseconds);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddSeconds(mem64_t tick, u32 tick_add_addr, long add)
{
	cellRtc.Log("cellRtcTickAddSeconds(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxTimeSpan seconds = wxTimeSpan::wxTimeSpan(0, 0, add, 0);
	date.Add(seconds);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMinutes(mem64_t tick, u32 tick_add_addr, long add)
{
	cellRtc.Log("cellRtcTickAddMinutes(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxTimeSpan minutes = wxTimeSpan::wxTimeSpan(0, add, 0, 0);
	date.Add(minutes);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddHours(mem64_t tick, u32 tick_add_addr, int add)
{
	cellRtc.Log("cellRtcTickAddHours(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxTimeSpan hours = wxTimeSpan::wxTimeSpan(add, 0, 0, 0);
	date.Add(hours);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddDays(mem64_t tick, u32 tick_add_addr, int add)
{
	cellRtc.Log("cellRtcTickAddDays(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxDateSpan days = wxDateSpan::wxDateSpan(0, 0, 0, add);
	date.Add(days);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddWeeks(mem64_t tick, u32 tick_add_addr, int add)
{
	cellRtc.Log("cellRtcTickAddWeeks(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxDateSpan weeks = wxDateSpan::wxDateSpan(0, 0, add, 0);
	date.Add(weeks);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddMonths(mem64_t tick, u32 tick_add_addr, int add)
{
	cellRtc.Log("cellRtcTickAddMonths(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxDateSpan months = wxDateSpan::wxDateSpan(0, add, 0, 0);
	date.Add(months);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcTickAddYears(mem64_t tick, u32 tick_add_addr, int add)
{
	cellRtc.Log("cellRtcTickAddYears(tick_addr=0x%x, tick_add_addr=0x%x, add=%l)", tick.GetAddr(), tick_add_addr, add);
	
	wxDateTime date = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_add_addr));
	wxDateSpan years = wxDateSpan::wxDateSpan(add, 0, 0, 0);
	date.Add(years);
	tick = date.GetTicks();

	return CELL_OK;
}

int cellRtcConvertUtcToLocalTime(u32 tick_utc_addr, mem64_t tick_local)
{
	cellRtc.Log("cellRtcConvertUtcToLocalTime(tick_utc_addr=0x%x, tick_local_addr=0x%x)", tick_utc_addr, tick_local.GetAddr());
	wxDateTime time = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_utc_addr));
	wxDateTime local_time = time.FromUTC(false);
	tick_local = local_time.GetTicks();
	return CELL_OK;
}

int cellRtcConvertLocalTimeToUtc(u32 tick_local_addr, mem64_t tick_utc)
{
	cellRtc.Log("cellRtcConvertLocalTimeToUtc(tick_local_addr=0x%x, tick_utc_addr=0x%x)", tick_local_addr, tick_utc.GetAddr());
	wxDateTime time = wxDateTime::wxDateTime((time_t)Memory.Read64(tick_local_addr));
	wxDateTime utc_time = time.ToUTC(false);
	tick_utc = utc_time.GetTicks();
	return CELL_OK;
}

int cellRtcGetDosTime(u32 datetime_addr, mem64_t dos_time)
{
	cellRtc.Log("cellRtcGetDosTime(datetime_addr=0x%x, dos_time_addr=0x%x)", datetime_addr, dos_time.GetAddr());
	CellRtcDateTime datetime;
	datetime.year = Memory.Read16(datetime_addr);
	datetime.month = Memory.Read16(datetime_addr + 2);
	datetime.day = Memory.Read16(datetime_addr + 4);
	datetime.hour = Memory.Read16(datetime_addr + 6);
	datetime.minute = Memory.Read16(datetime_addr + 8);
	datetime.second = Memory.Read16(datetime_addr + 10);
	datetime.microsecond = Memory.Read32(datetime_addr + 12);

	// Convert to DOS time.
	wxDateTime date_time = wxDateTime::wxDateTime(datetime.day, (wxDateTime::Month)datetime.month, datetime.year, datetime.hour, datetime.minute, datetime.second, (datetime.microsecond / 1000));
	dos_time = date_time.GetAsDOS();

	return CELL_OK;
}

int cellRtcGetTime_t(u32 datetime_addr, mem64_t posix_time)
{
	cellRtc.Log("cellRtcGetTime_t(datetime_addr=0x%x, posix_time_addr=0x%x)", datetime_addr, posix_time.GetAddr());
	CellRtcDateTime datetime;
	datetime.year = Memory.Read16(datetime_addr);
	datetime.month = Memory.Read16(datetime_addr + 2);
	datetime.day = Memory.Read16(datetime_addr + 4);
	datetime.hour = Memory.Read16(datetime_addr + 6);
	datetime.minute = Memory.Read16(datetime_addr + 8);
	datetime.second = Memory.Read16(datetime_addr + 10);
	datetime.microsecond = Memory.Read32(datetime_addr + 12);

	// Convert to POSIX time_t.
	wxDateTime date_time = wxDateTime::wxDateTime(datetime.day, (wxDateTime::Month)datetime.month, datetime.year, datetime.hour, datetime.minute, datetime.second, (datetime.microsecond / 1000));
	posix_time = convertToUNIXTime(date_time.GetSecond(wxDateTime::TZ::UTC), date_time.GetMinute(wxDateTime::TZ::UTC),
		date_time.GetHour(wxDateTime::TZ::UTC), date_time.GetDay(wxDateTime::TZ::UTC), date_time.GetYear(wxDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcGetWin32FileTime(u32 datetime_addr, mem64_t win32_time)
{
	cellRtc.Log("cellRtcGetWin32FileTime(datetime_addr=0x%x, win32_time_addr=0x%x)", datetime_addr, win32_time.GetAddr());
	CellRtcDateTime datetime;
	datetime.year = Memory.Read16(datetime_addr);
	datetime.month = Memory.Read16(datetime_addr + 2);
	datetime.day = Memory.Read16(datetime_addr + 4);
	datetime.hour = Memory.Read16(datetime_addr + 6);
	datetime.minute = Memory.Read16(datetime_addr + 8);
	datetime.second = Memory.Read16(datetime_addr + 10);
	datetime.microsecond = Memory.Read32(datetime_addr + 12);

	// Convert to WIN32 FILETIME.
	wxDateTime date_time = wxDateTime::wxDateTime(datetime.day, (wxDateTime::Month)datetime.month, datetime.year, datetime.hour, datetime.minute, datetime.second, (datetime.microsecond / 1000));
	win32_time = convertToWin32FILETIME(date_time.GetSecond(wxDateTime::TZ::UTC), date_time.GetMinute(wxDateTime::TZ::UTC),
		date_time.GetHour(wxDateTime::TZ::UTC), date_time.GetDay(wxDateTime::TZ::UTC), date_time.GetYear(wxDateTime::TZ::UTC));

	return CELL_OK;
}

int cellRtcSetDosTime(u32 datetime_addr, u32 dos_time_addr)
{
	cellRtc.Log("cellRtcSetDosTime(datetime_addr=0x%x, dos_time_addr=0x%x)", datetime_addr, dos_time_addr);
	
	wxDateTime date_time;
	wxDateTime dos_time = date_time.SetFromDOS(Memory.Read32(dos_time_addr));
	
	CellRtcDateTime datetime;
	datetime.year = dos_time.GetYear(wxDateTime::TZ::UTC);
	datetime.month = dos_time.GetMonth(wxDateTime::TZ::UTC);
	datetime.day = dos_time.GetDay(wxDateTime::TZ::UTC);
	datetime.hour = dos_time.GetHour(wxDateTime::TZ::UTC);
	datetime.minute = dos_time.GetMinute(wxDateTime::TZ::UTC);
	datetime.second = dos_time.GetSecond(wxDateTime::TZ::UTC);
	datetime.microsecond = dos_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	Memory.Write16(datetime_addr, datetime.year);
	Memory.Write16(datetime_addr + 2, datetime.month);
	Memory.Write16(datetime_addr + 4, datetime.day);
	Memory.Write16(datetime_addr + 6, datetime.hour);
	Memory.Write16(datetime_addr + 8, datetime.minute);
	Memory.Write16(datetime_addr + 10, datetime.second);
	Memory.Write32(datetime_addr + 12, datetime.microsecond);

	return CELL_OK;
}

int cellRtcSetTime_t(u32 datetime_addr, u32 posix_time_addr)
{
	cellRtc.Log("cellRtcSetTime_t(datetime_addr=0x%x, posix_time_addr=0x%x)", datetime_addr, posix_time_addr);
	
	wxDateTime date_time = wxDateTime::wxDateTime((time_t)Memory.Read64(posix_time_addr));
	
	CellRtcDateTime datetime;
	datetime.year = date_time.GetYear(wxDateTime::TZ::UTC);
	datetime.month = date_time.GetMonth(wxDateTime::TZ::UTC);
	datetime.day = date_time.GetDay(wxDateTime::TZ::UTC);
	datetime.hour = date_time.GetHour(wxDateTime::TZ::UTC);
	datetime.minute = date_time.GetMinute(wxDateTime::TZ::UTC);
	datetime.second = date_time.GetSecond(wxDateTime::TZ::UTC);
	datetime.microsecond = date_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	Memory.Write16(datetime_addr, datetime.year);
	Memory.Write16(datetime_addr + 2, datetime.month);
	Memory.Write16(datetime_addr + 4, datetime.day);
	Memory.Write16(datetime_addr + 6, datetime.hour);
	Memory.Write16(datetime_addr + 8, datetime.minute);
	Memory.Write16(datetime_addr + 10, datetime.second);
	Memory.Write32(datetime_addr + 12, datetime.microsecond);

	return CELL_OK;
}

int cellRtcSetWin32FileTime(u32 datetime_addr, u32 win32_time_addr)
{
	cellRtc.Log("cellRtcSetWin32FileTime(datetime_addr=0x%x, win32_time_addr=0x%x)", datetime_addr, win32_time_addr);
	
	wxDateTime date_time = wxDateTime::wxDateTime((time_t)Memory.Read64(win32_time_addr));
	
	CellRtcDateTime datetime;
	datetime.year = date_time.GetYear(wxDateTime::TZ::UTC);
	datetime.month = date_time.GetMonth(wxDateTime::TZ::UTC);
	datetime.day = date_time.GetDay(wxDateTime::TZ::UTC);
	datetime.hour = date_time.GetHour(wxDateTime::TZ::UTC);
	datetime.minute = date_time.GetMinute(wxDateTime::TZ::UTC);
	datetime.second = date_time.GetSecond(wxDateTime::TZ::UTC);
	datetime.microsecond = date_time.GetMillisecond(wxDateTime::TZ::UTC) * 1000;

	Memory.Write16(datetime_addr, datetime.year);
	Memory.Write16(datetime_addr + 2, datetime.month);
	Memory.Write16(datetime_addr + 4, datetime.day);
	Memory.Write16(datetime_addr + 6, datetime.hour);
	Memory.Write16(datetime_addr + 8, datetime.minute);
	Memory.Write16(datetime_addr + 10, datetime.second);
	Memory.Write32(datetime_addr + 12, datetime.microsecond);

	return CELL_OK;
}

int cellRtcIsLeapYear(int year)
{
	cellRtc.Log("cellRtcIsLeapYear(year=%d)", year);

	wxDateTime datetime;
	return datetime.IsLeapYear(year, wxDateTime::Gregorian);
}

int cellRtcGetDaysInMonth(int year, int month)
{
	cellRtc.Log("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	wxDateTime datetime;
	return datetime.GetNumberOfDays((wxDateTime::Month) month, year, wxDateTime::Gregorian);
}

int cellRtcGetDayOfWeek(int year, int month, int day)
{
	cellRtc.Log("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	wxDateTime datetime;
	datetime.SetToWeekDay((wxDateTime::WeekDay) day, 1, (wxDateTime::Month) month, year);
	return datetime.GetWeekDay();
}

int cellRtcCheckValid(u32 datetime_addr)
{
	cellRtc.Log("cellRtcCheckValid(datetime_addr=0x%x)", datetime_addr);
	CellRtcDateTime datetime;
	datetime.year = Memory.Read16(datetime_addr);
	datetime.month = Memory.Read16(datetime_addr + 2);
	datetime.day = Memory.Read16(datetime_addr + 4);
	datetime.hour = Memory.Read16(datetime_addr + 6);
	datetime.minute = Memory.Read16(datetime_addr + 8);
	datetime.second = Memory.Read16(datetime_addr + 10);
	datetime.microsecond = Memory.Read32(datetime_addr + 12);
	
	if((datetime.year < 1) || (datetime.year > 9999)) return CELL_RTC_ERROR_INVALID_YEAR;
	else if((datetime.month < 1) || (datetime.month > 12)) return CELL_RTC_ERROR_INVALID_MONTH;
	else if((datetime.day < 1) || (datetime.day > 31)) return CELL_RTC_ERROR_INVALID_DAY;
	else if((datetime.hour < 0) || (datetime.hour > 23)) return CELL_RTC_ERROR_INVALID_HOUR;
	else if((datetime.minute < 0) || (datetime.minute > 59)) return CELL_RTC_ERROR_INVALID_MINUTE;
	else if((datetime.second < 0) || (datetime.second > 59)) return CELL_RTC_ERROR_INVALID_SECOND;
	else if((datetime.microsecond < 0) || (datetime.microsecond > 999999)) return CELL_RTC_ERROR_INVALID_MICROSECOND;
	else return CELL_OK;
}

int cellRtcCompareTick(u32 tick_addr_1, u32 tick_addr_2)
{
	cellRtc.Log("cellRtcCompareTick(tick_addr_1=0x%x, tick_addr_2=0x%x)", tick_addr_1, tick_addr_2);
	u64 tick1 = Memory.Read64(tick_addr_1);
	u64 tick2 = Memory.Read64(tick_addr_2);

	if(tick1 < tick2) return -1;
	else if(tick1 > tick2) return 1;
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
