#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellRtc.h"

#include "Emu/Cell/lv2/sys_time.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_ss.h"

//#include <iomanip>
//#include <sstream>

LOG_CHANNEL(cellRtc);

// clang-format off
template <>
void fmt_class_string<CellRtcError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_RTC_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_RTC_ERROR_INVALID_POINTER);
			STR_CASE(CELL_RTC_ERROR_INVALID_VALUE);
			STR_CASE(CELL_RTC_ERROR_INVALID_ARG);
			STR_CASE(CELL_RTC_ERROR_NOT_SUPPORTED);
			STR_CASE(CELL_RTC_ERROR_NO_CLOCK);
			STR_CASE(CELL_RTC_ERROR_BAD_PARSE);
			STR_CASE(CELL_RTC_ERROR_INVALID_YEAR);
			STR_CASE(CELL_RTC_ERROR_INVALID_MONTH);
			STR_CASE(CELL_RTC_ERROR_INVALID_DAY);
			STR_CASE(CELL_RTC_ERROR_INVALID_HOUR);
			STR_CASE(CELL_RTC_ERROR_INVALID_MINUTE);
			STR_CASE(CELL_RTC_ERROR_INVALID_SECOND);
			STR_CASE(CELL_RTC_ERROR_INVALID_MICROSECOND);
		}

		return unknown;
	});
}
// clang-format on

// Grabbed from JPCSP
// This is the # of microseconds between January 1, 0001 and January 1, 1970.
const u64 RTC_MAGIC_OFFSET = 62135596800000000ULL;
// This is the # of microseconds between January 1, 0001 and January 1, 1601 (for Win32 FILETIME.)
const u64 RTC_FILETIME_OFFSET = 50491123200000000ULL;

const u64 EPOCH_AS_FILETIME = 116444736000000000ULL;

// Also stores leap year
const u8 DAYS_IN_MONTH[24] = {0x1F, 0x1C, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1F, 0x1D, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F};
const char WEEKDAY_NAMES[7][4]  = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};                                    // 4 as terminator
const char MONTH_NAMES[12][4]   = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; // 4 as terminator

s64 convertToUNIXTime(u16 seconds, u16 minutes, u16 hours, u16 days, s32 years)
{
	return s64{seconds} + s64{minutes} * 60 + s64{hours} * 3600 + s64{days} * 86400 + s64{s64{years} - 70} * 31536000 + s64{(years - 69) / 4} * 86400 - s64{(years - 1) / 100} * 86400 +
		   s64{(years + 299) / 400} * 86400;
}

u64 convertToWin32FILETIME(u16 seconds, u16 minutes, u16 hours, u16 days, s32 years)
{
	s64 unixtime      = convertToUNIXTime(seconds, minutes, hours, days, years);
	u64 win32time     = static_cast<u64>(unixtime) * 10000000 + EPOCH_AS_FILETIME;
	u64 win32filetime = win32time | win32time >> 32;
	return win32filetime;
}

// TODO make all this use tick resolution and figure out the magic numbers
// TODO MULHDU instruction returns high 64 bit of the 128 bit result of two 64-bit regs multiplication
// need to be replaced with utils::mulh64, utils::umulh64, utils::div128, utils::udiv128 in needed places
// cellRtcSetTick / cellRtcSetWin32FileTime /  cellRtcSetCurrentTick / cellRtcSetCurrentSecureTick
// TODO undo optimized division

// Internal helper functions in cellRtc

error_code set_secure_rtc_time(u64 time)
{
	return sys_ss_secure_rtc(0x3003 /* SET_TIME */, time, 0, 0); // TODO
}

error_code get_secure_rtc_time(u64 unk1, u64 unk2, u64 unk3)
{
	return sys_ss_secure_rtc(0x3002 /* GET_TIME */, unk1, unk2, unk3); // TODO
}

// End of internal helper functions

// Helper methods

static bool is_leap_year(u32 year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// End of helper methods

error_code cellRtcGetCurrentTick(vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcGetCurrentTick(pTick=*0x%x)", pTick);

	if (!vm::check_addr(pTick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	error_code ret = sys_time_get_current_time(sec, nsec);
	if (ret != CELL_OK)
	{
		return ret;
	}

	pTick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	return CELL_OK;
}

error_code cellRtcGetCurrentClock(vm::ptr<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc.todo("cellRtcGetCurrentClock(pClock=*0x%x, time_zone=%d)", pClock, iTimeZone);

	if (!vm::check_addr(pClock.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	if (!vm::check_addr(tick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	error_code ret = sys_time_get_current_time(sec, nsec);
	if (ret != CELL_OK)
	{
		return ret;
	}

	tick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcTickAddMinutes(tick, tick, iTimeZone);
	cellRtcSetTick(pClock, tick);

	return CELL_OK;
}

error_code cellRtcGetCurrentClockLocalTime(vm::ptr<CellRtcDateTime> pClock)
{
	cellRtc.todo("cellRtcGetCurrentClockLocalTime(pClock=*0x%x)", pClock);

	if (!vm::check_addr(pClock.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret != CELL_OK)
	{
		return ret;
	}

	if (!vm::check_addr(pClock.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	if (!vm::check_addr(tick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	ret = sys_time_get_current_time(sec, nsec);
	if (ret != CELL_OK)
	{
		return ret;
	}

	tick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcTickAddMinutes(tick, tick, s64{*timezone} + s64{*summertime});
	cellRtcSetTick(pClock, tick);

	return CELL_OK;
}

error_code cellRtcFormatRfc2822(vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.todo("cellRtcFormatRfc2822(pszDateTime=*0x%x, pUtc=*0x%x, time_zone=%d)", pszDateTime, pUtc, iTimeZone);

	if (!vm::check_addr(pszDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pUtc.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> rtc_tick;
	if (pUtc->tick == 0ULL)
	{
		cellRtcGetCurrentTick(rtc_tick);
	}
	else
	{
		rtc_tick->tick = pUtc->tick;
	}

	vm::var<CellRtcDateTime> date_time;

	cellRtcTickAddMinutes(rtc_tick, rtc_tick, iTimeZone);
	cellRtcSetTick(date_time, rtc_tick);

	error_code ret = cellRtcCheckValid(date_time);
	if (ret != CELL_OK)
	{
		return ret;
	}

	s32 tzone = iTimeZone;

	s32 weekdayIdx = cellRtcGetDayOfWeek(date_time->year, date_time->month, date_time->day);
	// Day name
	*pszDateTime   = WEEKDAY_NAMES[weekdayIdx][0];
	pszDateTime[1] = WEEKDAY_NAMES[weekdayIdx][1];
	pszDateTime[2] = WEEKDAY_NAMES[weekdayIdx][2];
	pszDateTime[3] = ',';
	pszDateTime[4] = ' ';
	// Day number
	if (date_time->day < 10)
	{
		pszDateTime[5] = '0';
		pszDateTime[6] = date_time->day + '0';
	}
	else
	{
		pszDateTime[5] = (date_time->day / 10) + '0';
		pszDateTime[6] = (date_time->day - ((date_time->day / 10 << 1) + (date_time->day / 10 << 3))) + '0';
	}
	pszDateTime[7] = ' ';

	// month name
	pszDateTime[8]   = MONTH_NAMES[date_time->month - 1][0];
	pszDateTime[9]   = MONTH_NAMES[date_time->month - 1][1];
	pszDateTime[10]  = MONTH_NAMES[date_time->month - 1][2];
	pszDateTime[0xb] = ' ';

	// year
	char yearDigits[5];
	sprintf(yearDigits, "%04hi", u16{date_time->year});
	pszDateTime[0xc]  = yearDigits[0];
	pszDateTime[0xd]  = yearDigits[1];
	pszDateTime[0xe]  = yearDigits[2];
	pszDateTime[0xf]  = yearDigits[3];
	pszDateTime[0x10] = ' ';

	// Hours
	char hourDigits[3];
	sprintf(hourDigits, "%02hi", u16{date_time->hour});
	pszDateTime[0x11] = hourDigits[0];
	pszDateTime[0x12] = hourDigits[1];
	pszDateTime[0x13] = ':';

	// Minutes
	char minDigits[3];
	sprintf(minDigits, "%02hi", u16{date_time->minute});
	pszDateTime[0x14] = minDigits[0];
	pszDateTime[0x15] = minDigits[1];
	pszDateTime[0x16] = ':';

	// Seconds
	char secDigits[3];
	sprintf(secDigits, "%02hi", u16{date_time->second});
	pszDateTime[0x17] = secDigits[0];
	pszDateTime[0x18] = secDigits[1];
	pszDateTime[0x19] = ' ';

	// Timezone -/+
	if (iTimeZone < 0)
	{
		tzone             = -tzone;
		pszDateTime[0x1a] = '-';
	}
	else
	{
		pszDateTime[0x1a] = '+';
	}

	// Timezone - matches lle result.
	u32 unk_1 = tzone >> 31;
	u32 unk_2 = (tzone / 0x3c + unk_1) - unk_1;
	tzone -= (unk_2 << 6) - (unk_2 << 2);
	u32 unk_3 = (unk_2 / 10 + (unk_2 >> 0x1f)) - (unk_2 >> 31);
	u32 unk_4 = (tzone / 10 + unk_1) - unk_1;

	pszDateTime[0x1b] = unk_3 + (unk_3 / 10) * -10 + '0';
	pszDateTime[0x1c] = (unk_2 - ((unk_3 << 1) + (unk_3 << 3))) + '0';
	pszDateTime[0x1d] = unk_4 + (unk_4 / 10) * -10 + '0';
	pszDateTime[0x1e] = (tzone - ((unk_4 << 1) + (unk_4 << 3))) + '0';
	pszDateTime[0x1f] = '\0';

	return CELL_OK;
}

error_code cellRtcFormatRfc2822LocalTime(vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcFormatRfc2822LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	if (!vm::check_addr(pszDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pUtc.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret != CELL_OK)
	{
		return ret;
	}

	return cellRtcFormatRfc2822(pszDateTime, pUtc, *timezone + *summertime);
}

error_code cellRtcFormatRfc3339(vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.todo("cellRtcFormatRfc3339(pszDateTime=*0x%x, pUtc=*0x%x, iTimeZone=%d)", pszDateTime, pUtc, iTimeZone);

	if (!vm::check_addr(pszDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pUtc.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> rtc_tick;
	if (pUtc->tick == 0ULL)
	{
		cellRtcGetCurrentTick(rtc_tick);
	}
	else
	{
		rtc_tick->tick = pUtc->tick;
	}

	vm::var<CellRtcDateTime> date_time;

	cellRtcTickAddMinutes(rtc_tick, rtc_tick, iTimeZone);
	cellRtcSetTick(date_time, rtc_tick);

	error_code ret = cellRtcCheckValid(date_time);
	if (ret != CELL_OK)
	{
		return ret;
	}

	s32 tzone = iTimeZone;

	// Year - XXXX-04-13T10:56:31.35+66:40
	char yearDigits[5];
	sprintf(yearDigits, "%04hi", u16{date_time->year});
	pszDateTime[0x0] = yearDigits[0];
	pszDateTime[0x1] = yearDigits[1];
	pszDateTime[0x2] = yearDigits[2];
	pszDateTime[0x3] = yearDigits[3];
	pszDateTime[0x4] = '-';

	// Month - 2020-XX-13T10:56:31.35+66:40
	char monthDigits[3];
	sprintf(monthDigits, "%02hi", u16{date_time->month});
	pszDateTime[0x5] = monthDigits[0];
	pszDateTime[0x6] = monthDigits[1];
	pszDateTime[0x7] = '-';

	// Day - 2020-04-XXT10:56:31.35+66:40
	char dayDigits[3];
	sprintf(dayDigits, "%02hi", u16{date_time->day});
	pszDateTime[0x8] = dayDigits[0];
	pszDateTime[0x9] = dayDigits[1];
	pszDateTime[0xa] = 'T';

	// Hours - 2020-04-13TXX:56:31.35+66:40
	char hourDigits[3];
	sprintf(hourDigits, "%02hi", u16{date_time->hour});
	pszDateTime[0xb] = hourDigits[0];
	pszDateTime[0xc] = hourDigits[1];
	pszDateTime[0xd] = ':';

	// Minutes - 2020-04-13T10:XX:31.35+66:40
	char minDigits[3];
	sprintf(minDigits, "%02hi", u16{date_time->minute});
	pszDateTime[0xe]  = minDigits[0];
	pszDateTime[0xf]  = minDigits[1];
	pszDateTime[0x10] = ':';

	// Seconds - 2020-04-13T10:56:XX.35+66:40
	char secDigits[3];
	sprintf(secDigits, "%02hi", u16{date_time->second});
	pszDateTime[0x11] = secDigits[0];
	pszDateTime[0x12] = secDigits[1];
	pszDateTime[0x13] = '.';

	// Microseconds - 2020-04-13T10:56:31.XX+66:40
	char microDigits[3];
	sprintf(microDigits, "%02u", u32{date_time->microsecond});
	pszDateTime[0x14] = microDigits[0];
	pszDateTime[0x15] = microDigits[1];

	if (iTimeZone == 0)
	{
		pszDateTime[0x16] = 'Z';
		pszDateTime[0x17] = '\0';
	}
	else
	{
		if (iTimeZone < 0)
		{
			tzone             = -tzone;
			pszDateTime[0x16] = '-';
		}
		else
		{
			pszDateTime[0x16] = '+';
		}
		u32 uVar1 = tzone >> 0x1f;

		u32 lVar9 = (tzone / 0x3c + uVar1) - uVar1;
		tzone -= (lVar9 << 6) - (lVar9 << 2);
		uVar1             = tzone >> 0x1f;
		u32 lVar11        = (lVar9 / 10 + (lVar9 >> 0x1f)) - (lVar9 >> 0x1f);
		u32 lVar8         = (tzone / 10 + uVar1) - uVar1;
		pszDateTime[0x17] = lVar11 + (lVar11 / 10) * -10 + '0';
		pszDateTime[0x18] = (lVar9 - ((lVar11 << 1) + (lVar11 << 3))) + '0';
		pszDateTime[0x19] = ':';
		pszDateTime[0x1a] = lVar8 + (lVar8 / 10) * -10 + '0';
		pszDateTime[0x1b] = (tzone - ((lVar8 << 1) + (lVar8 << 3))) + '0';
		pszDateTime[0x1c] = '\0';
	}

	return CELL_OK;
}

error_code cellRtcFormatRfc3339LocalTime(vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcFormatRfc3339LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	if (!vm::check_addr(pszDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pUtc.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret != CELL_OK)
	{
		return ret;
	}

	return cellRtcFormatRfc3339(pszDateTime, pUtc, *timezone + *summertime);
}

/*
 Takes a RFC2822 / RFC3339 / asctime String, and converts it to a CellRtcTick
*/
error_code cellRtcParseDateTime(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.error("cellRtcParseDateTime(pUtc=*0x%x, pszDateTime=%s) -- Implement me", pUtc, pszDateTime);

	// Below code kinda works
	/*

	std::tm t = {};
	std::string tz;
	vm::var<CellRtcDateTime> date_time;

	// Not done like the library does it in the least..

	s32 timezoneMins;
	std::istringstream iss(std::string(pszDateTime.get_ptr(), strlen(pszDateTime.get_ptr())));
	iss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S") >> tz; // rfc2822
	if (!iss.fail())
	{
		// Looks wrong, works
		if (tz[0] == '+')
		{
			timezoneMins = -std::stoi(tz.substr(1));
		}
		else
		{
			timezoneMins = +std::stoi(tz.substr(1));
		}
	}
	else
	{
		iss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S") >> tz; // rfc3339 2020-04-03T13:23:30.30Z
		if (!iss.fail())
		{
			// TODO timezone
		}
		else
		{
			// TODO asctime
			iss >> std::get_time(&t, "%a %b  %d %H:%M:%S %Y");//Mon Apr  6 21:58:35 2020
			if (!iss.fail())
			{
				// TODO timezone
			}
			else
			{
				return CELL_RTC_ERROR_BAD_PARSE;
			}
		}
	}

	cellRtc.todo("heh year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d, tz: %s, tz_d: %d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tz, timezoneMins);
	date_time->year        = t.tm_year + 1900;
	date_time->month       = t.tm_mon + 1;
	date_time->day         = t.tm_mday;
	date_time->hour        = t.tm_hour;
	date_time->minute      = t.tm_min;
	date_time->second      = t.tm_sec;
	date_time->microsecond = 0;
	cellRtcGetTick(date_time, pUtc);
	cellRtcTickAddMinutes(pUtc, pUtc, timezoneMins);*/

	return CELL_OK;
}

error_code cellRtcParseRfc3339(vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.todo("cellRtcParseRfc3339(pUtc=*0x%x, pszDateTime=%s)", pUtc, pszDateTime);

	if (!vm::check_addr(pUtc.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pszDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;

	// TODO fix unholyness

	s64 l_add;
	u32 pc_var_5;
	u32 u_var_7;
	s32 i_var_6;
	s32 i_var_8;

	char char_1 = pszDateTime[1];
	char char_2 = pszDateTime[2];
	char char_3 = pszDateTime[3];
	if (((((((*pszDateTime - 0x30U) < 10) && ('/' < char_1)) && (char_1 < ':')) && (('/' < char_2 && (char_2 < ':')))) && ('/' < char_3)) && (char_3 < ':'))
	{
		date_time->year = (char_2 << 1) + (char_2 << 3) + *pszDateTime * 1000 + char_1 * 100 + char_3 + 0x2fb0;
	}
	else
	{
		date_time->year = 0xffff;
	}

	if (pszDateTime[4] != '-')
	{
		return CELL_RTC_ERROR_INVALID_YEAR;
	}

	char_2 = pszDateTime[5];
	char_1 = pszDateTime[6];
	if (((9 < (char_2 - 0x30U)) || (char_1 < '0')) || (date_time->month = ((char_2 << 1) + (char_2 << 3) + char_1) - 0x210, '9' < char_1))
	{
		date_time->month = 0xffff;
	}

	if (pszDateTime[7] != '-')
	{
		return CELL_RTC_ERROR_INVALID_MONTH;
	}

	char_2 = pszDateTime[8];
	char_1 = pszDateTime[9];
	if (((9 < (char_2 - 0x30U)) || (char_1 < '0')) || (date_time->day = ((char_2 << 1) + (char_2 << 3) + char_1) - 0x210, '9' < char_1))
	{
		date_time->day = 0xffff;
	}

	if (pszDateTime[10] != 'T' && pszDateTime[10] != 't')
	{
		return CELL_RTC_ERROR_INVALID_DAY;
	}

	char_2 = pszDateTime[0xb];
	char_1 = pszDateTime[0xc];
	if ((9 < (char_2 - 0x30U)) || ((char_1 < '0' || (date_time->hour = ((char_2 << 1) + (char_2 << 3) + char_1) - 0x210, '9' < char_1))))
	{
		date_time->hour = 0xffff;
	}

	if (pszDateTime[0xd] != ':')
	{
		return CELL_RTC_ERROR_INVALID_HOUR;
	}

	char_2 = pszDateTime[0xe];
	char_1 = pszDateTime[0xf];
	if (((9 < (char_2 - 0x30U)) || (char_1 < '0')) || (date_time->minute = ((char_2 << 1) + (char_2 << 3) + char_1) - 0x210, '9' < char_1))
	{
		date_time->minute = 0xffff;
	}

	if (pszDateTime[0x10] != ':')
	{
		return CELL_RTC_ERROR_INVALID_MINUTE;
	}

	char_2 = pszDateTime[0x11];
	char_1 = pszDateTime[0x12];
	if (((9 < (char_2 - 0x30U)) || (char_1 < '0')) || (date_time->second = ((char_2 << 1) + (char_2 << 3) + char_1) - 0x210, '9' < char_1))
	{
		date_time->second = 0xffff;
	}

	pc_var_5 = 0x13;
	if (pszDateTime[0x13] == '.')
	{
		date_time->microsecond = 0;
		pc_var_5                 = 0x14;
		if ((pszDateTime[0x14] - 0x30U) < 10)
		{
			u_var_7                  = 10000;
			date_time->microsecond = (pszDateTime[pc_var_5] + -0x30) * 100000;
			while (true)
			{
				pc_var_5 = pc_var_5 + 1;
				if (9 < (pszDateTime[pc_var_5] - 0x30U))
					break;
				date_time->microsecond += u_var_7 * (pszDateTime[pc_var_5] + -0x30);
				if (u_var_7 == 100000)
				{
					u_var_7 = 10000;
				}
				else
				{
					if (u_var_7 == 10000)
					{
						u_var_7 = 1000;
					}
					else
					{
						if (u_var_7 == 1000)
						{
							u_var_7 = 100;
						}
						else
						{
							if (u_var_7 == 100)
							{
								u_var_7 = 10;
							}
							else
							{
								u_var_7 = ((u_var_7 ^ 10) - 1) >> 0x1f;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		date_time->microsecond = 0;
	}

	char_1 = pszDateTime[pc_var_5];
	if ((char_1 == 'Z') || (char_1 == 'z'))
	{
		l_add = 0;
	}
	else
	{
		if ((char_1 != '+') && (char_1 != '-'))
		{
			return CELL_RTC_ERROR_BAD_PARSE;
		}
		char_2 = pszDateTime[pc_var_5 + 2];
		if (((9 < (pszDateTime[pc_var_5 + 1] - 0x30U)) || (char_2 < '0')) || (i_var_6 = pszDateTime[pc_var_5 + 1] * 10 + char_2 + -0x210, '9' < char_2))
		{
			i_var_6 = -1;
		}
		char_2 = pszDateTime[pc_var_5 + 5];
		if (((9 < (pszDateTime[pc_var_5 + 4] - 0x30U)) || (char_2 < '0')) || (i_var_8 = pszDateTime[pc_var_5 + 4] * 10 + char_2 + -0x210, '9' < char_2))
		{
			i_var_8 = -1;
		}
		if (((i_var_6 < 0) || (pszDateTime[pc_var_5 + 3] != ':')) || (i_var_8 < 0))
		{
			return CELL_RTC_ERROR_BAD_PARSE;
		}
		i_var_8 += i_var_6 * 0x3c;
		l_add = -i_var_8;
		if (char_1 == '-')
		{
			l_add = i_var_8;
		}
	}

	cellRtcGetTick(date_time, pUtc);
	cellRtcTickAddMinutes(pUtc, pUtc, l_add);

	return CELL_OK;
}

error_code cellRtcGetTick(vm::cptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcGetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);

	if (!vm::check_addr(pTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pTime->year >= 10000 || pTime->year == 0)
	{
		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	if (!pTick)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	s64 days_in_years = ((((pTime->year * 365ULL) + ((pTime->year + 3) / 4)) - ((pTime->year + 99) / 100)) + ((pTime->year + 399) / 400) + -366);

	// 1-12
	if (1 < pTime->month)
	{
		u32 month_idx         = pTime->month - 1;
		u32 monthIdx_adjusted = is_leap_year(pTime->year) * 12;
		do
		{
			days_in_years += DAYS_IN_MONTH[monthIdx_adjusted];
			month_idx -= 1;
			monthIdx_adjusted += 1;
		} while (month_idx != 0);
	}

	pTick->tick = ((((days_in_years + (pTime->day - 1)) * 0x18 + pTime->hour) * 0x3c + pTime->minute) * 0x3c + pTime->second) * cellRtcGetTickResolution() + pTime->microsecond;

	return CELL_OK;
}

error_code cellRtcSetTick(vm::ptr<CellRtcDateTime> pTime, vm::cptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcSetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);

	if (!vm::check_addr(pTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	/*
	u32 microseconds = round((pTick->tick % 1000000ULL));
	u16 seconds      = round((pTick->tick / (1000000ULL)) % 60);
	u16 minutes      = round((pTick->tick / (60ULL * 1000000ULL)) % 60);
	u16 hours        = round((pTick->tick / (60ULL * 60ULL * 1000000ULL)) % 24);
	u64 days_tmp     = round((pTick->tick / (24ULL * 60ULL * 60ULL * 1000000ULL)));*/

	u32 microseconds = (pTick->tick % 1000000ULL);
	u16 seconds      = (pTick->tick / (1000000ULL)) % 60;
	u16 minutes      = (pTick->tick / (60ULL * 1000000ULL)) % 60;
	u16 hours        = (pTick->tick / (60ULL * 60ULL * 1000000ULL)) % 24;
	u64 days_tmp     = (pTick->tick / (24ULL * 60ULL * 60ULL * 1000000ULL));

	u16 months = 1;
	u16 years  = 1;

	bool exit_while = false;
	do
	{
		bool leap = is_leap_year(years);
		for (uint32_t m = 0; m <= 11; m++)
		{
			uint8_t daysinmonth = DAYS_IN_MONTH[m + (leap * 12)];
			if (days_tmp >= daysinmonth)
			{
				months++;
				days_tmp -= daysinmonth;
			}
			else
			{
				exit_while = true;
				break;
			}
			if (m == 11)
			{
				months = 1;
				years++;
			}
		}

	} while (!exit_while);

	pTime->microsecond = microseconds;
	pTime->second      = seconds;
	pTime->minute      = minutes;
	pTime->hour        = hours;
	pTime->day         = days_tmp + 1;
	pTime->month       = months;
	pTime->year        = years;

	return CELL_OK;
}

error_code cellRtcTickAddTicks(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddTicks(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd;

	return CELL_OK;
}

error_code cellRtcTickAddMicroseconds(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddMicroseconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd;

	return CELL_OK;
}

error_code cellRtcTickAddSeconds(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddSeconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddMinutes(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddMinutes(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd * 60 * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddHours(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddHours(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddDays(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddDays(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * 24ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddWeeks(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddWeeks(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * 24ULL * 7ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddMonths(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddMonths(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;
	cellRtcSetTick(date_time, pTick1);

	// Not pretty, but works

	s64 total_months     = (date_time->year * 12ULL) + date_time->month + iAdd + -1;
	s32 total_months_s32 = total_months;
	u32 unk_1            = total_months_s32 >> 0x1f;
	u64 unk_2            = ((total_months_s32 / 6 + unk_1) >> 1) - unk_1;
	u32 unk_3            = unk_2;
	unk_1                = unk_3 & 0xffff;
	u64 unk_4            = (total_months - ((u64{unk_3} << 4) - (unk_3 << 2))) + 1;
	if (((unk_2 & 0xffff) == 0) || ((unk_3 = unk_4 & 0xffff, (unk_4 & 0xffff) == 0 || (0xc < unk_3))))
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	u32 uVar1 = ((s64{unk_1} * 0x51eb851f) >> 0x20);

	// Leap year check
	u32 month_idx;
	if ((unk_1 == (uVar1 >> 7) * 400) || ((unk_1 != (uVar1 >> 5) * 100 && ((unk_2 & 3) == 0))))
	{
		month_idx = unk_3 + 0xb;
	}
	else
	{
		month_idx = unk_3 - 1;
	}
	u32 month_days = DAYS_IN_MONTH[month_idx];

	if (month_days < date_time->day)
	{
		date_time->day = month_days;
	}

	date_time->month = unk_4;
	date_time->year  = unk_2;
	cellRtcGetTick(date_time, pTick0);

	return CELL_OK;
}

error_code cellRtcTickAddYears(vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddYears(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	if (!vm::check_addr(pTick0.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick1.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;
	cellRtcSetTick(date_time, pTick1);

	u64 total_years = iAdd + date_time->year;
	u32 unk_1       = total_years & 0xffff;
	if (((total_years & 0xffff) == 0) || ((date_time->month == 0 || (0xc < date_time->month))))
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	u32 uVar1 = ((s64{unk_1} * 0x51eb851f) >> 0x20);

	// Leap year check
	u32 month_idx;
	if ((unk_1 == (uVar1 >> 7) * 400) || ((unk_1 != (uVar1 >> 5) * 100 && ((total_years & 3) == 0))))
	{
		month_idx = date_time->month + 0xb;
	}
	else
	{
		month_idx = date_time->month - 1;
	}
	u32 month_days = DAYS_IN_MONTH[month_idx];

	if (month_days < date_time->day)
	{
		date_time->day = month_days;
	}

	date_time->year = total_years;
	cellRtcGetTick(date_time, pTick0);

	return CELL_OK;
}

error_code cellRtcConvertUtcToLocalTime(vm::cptr<CellRtcTick> pUtc, vm::ptr<CellRtcTick> pLocalTime)
{
	cellRtc.todo("cellRtcConvertUtcToLocalTime(pUtc=*0x%x, pLocalTime=*0x%x)", pUtc, pLocalTime);

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (-1 < ret)
	{
		ret = cellRtcTickAddMinutes(pLocalTime, pUtc, s64{*timezone} + s64{*summertime});
	}

	return ret;
}

error_code cellRtcConvertLocalTimeToUtc(vm::cptr<CellRtcTick> pLocalTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.todo("cellRtcConvertLocalTimeToUtc(pLocalTime=*0x%x, pUtc=*0x%x)", pLocalTime, pUtc);

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (-1 < ret)
	{
		ret = cellRtcTickAddMinutes(pUtc, pLocalTime, -(s64{*timezone} + s64{*summertime}));
	}

	return ret;
}

error_code cellRtcGetCurrentSecureTick(vm::ptr<CellRtcTick> tick)
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

error_code cellRtcGetDosTime(vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<u32> puiDosTime)
{
	cellRtc.todo("cellRtcGetDosTime(pDateTime=*0x%x, puiDosTime=*0x%x)", pDateTime, puiDosTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(puiDosTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pDateTime->year < 1980)
	{
		if (puiDosTime)
		{
			*puiDosTime = 0;
			return -1;
		}
	}
	else if (pDateTime->year >= 2108)
	{
		if (puiDosTime)
		{
			*puiDosTime = 0xff9fbf7d; // kHighDosTime
			return -1;
		}
	}
	else
	{
		if (!puiDosTime)
		{
			return CELL_OK;
		}

		s32 year    = ((pDateTime->year - 1980) & 0x7F) << 9;
		s32 month   = ((pDateTime->month) & 0xF) << 5;
		s32 hour    = ((pDateTime->hour) & 0x1F) << 11;
		s32 minute  = ((pDateTime->minute) & 0x3F) << 5;
		s32 day     = (pDateTime->day) & 0x1F;
		s32 second  = ((pDateTime->second) >> 1) & 0x1F;
		s32 ymd     = year | month | day;
		s32 hms     = hour | minute | second;
		*puiDosTime = (ymd << 16) | hms;

		return CELL_OK;
	}

	return -1;
}

error_code cellRtcGetSystemTime(vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcGetSystemTime(pDateTime=*0x%x, pTick=*0x%x)", pDateTime, pTick);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pTick.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	error_code ret;
	vm::var<CellRtcTick> tick;
	cellRtcGetTick(pDateTime, tick);

	if (tick->tick < 63082281600000000) // Max time
	{
		ret = CELL_RTC_ERROR_INVALID_VALUE;
		if (pTick)
		{
			pTick->tick = 0;
		}
	}
	else
	{
		if (tick->tick < 0xeb5325dc3ec23f) // 66238041600999999
		{
			ret = CELL_OK;
			if (pTick)
			{
				pTick->tick = (tick->tick + 0xff1fe2ffc59c6000) / cellRtcGetTickResolution();
			}
		}
		else
		{
			ret = CELL_RTC_ERROR_INVALID_VALUE;
			if (pTick)
			{
				pTick->tick = 0xbc19137f; // 1 day?
			}
		}
	}

	return ret;
}

error_code cellRtcGetTime_t(vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<s64> piTime)
{
	cellRtc.todo("cellRtcGetTime_t(pDateTime=*0x%x, piTime=*0x%x)", pDateTime, piTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(piTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	cellRtcGetTick(pDateTime, tick);

	error_code ret;
	if (tick->tick < RTC_MAGIC_OFFSET)
	{
		ret = CELL_RTC_ERROR_INVALID_VALUE;
		if (piTime)
		{
			*piTime = 0;
		}
	}
	else
	{
		ret = CELL_OK;
		if (piTime)
		{
			*piTime = (tick->tick + 0xff23400100d44000) / cellRtcGetTickResolution();
		}
	}

	return ret;
}

error_code cellRtcGetWin32FileTime(vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<u64> pulWin32FileTime)
{
	cellRtc.todo("cellRtcGetWin32FileTime(pDateTime=*0x%x, pulWin32FileTime=*0x%x)", pDateTime, pulWin32FileTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (!vm::check_addr(pulWin32FileTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	cellRtcGetTick(pDateTime, tick);

	error_code ret;
	if (tick->tick < RTC_FILETIME_OFFSET)
	{
		ret = CELL_RTC_ERROR_INVALID_VALUE;
		if (pulWin32FileTime)
		{
			*pulWin32FileTime = 0;
		}
	}
	else
	{
		ret = CELL_OK;
		if (pulWin32FileTime)
		{
			*pulWin32FileTime = tick->tick * 10 + 0xf8fe31e8dd890000;
		}
	}

	return ret;
}

error_code cellRtcSetCurrentSecureTick(vm::ptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcSetCurrentSecureTick(pTick=*0x%x)", pTick);

	if (!pTick)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	u64 uVar1 = pTick->tick + 0xff1fe2ffc59c6000;
	if (uVar1 >= 0xb3625a1cbe000) // 3155760000000000
	{
		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	return set_secure_rtc_time(uVar1 / cellRtcGetTickResolution());
}

error_code cellRtcSetCurrentTick(vm::cptr<CellRtcTick> pTick)
{
	cellRtc.todo("cellRtcSetCurrentTick(pTick=*0x%x)", pTick);

	if (!pTick)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	u64 tmp = pTick->tick + 0xff23400100d44000;
	if (!(0xdcbffeff2bbfff < pTick->tick))
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	// TODO syscall not implemented
	/*
	u32 tmp2 = sys_time_get_system_time(tmp / cellRtcGetTickResolution(), (tmp % cellRtcGetTickResolution()) * 1000);

	return (tmp2 & (tmp2 | tmp2 - 1) >> 0x1f);
	*/
	return CELL_OK;
}

error_code cellRtcSetConf(s64 unk1, s64 unk2, u32 timezone, u32 summertime)
{
	cellRtc.todo("cellRtcSetConf(unk1=0x%x, unk2=0x%x, timezone=%d, summertime=%d)", unk1, unk2, timezone, summertime);
	// Seems the first 2 args are ignored :|

	// TODO Syscall not implemented
	// return sys_time_set_timezone(timezone, summertime);

	return CELL_OK;
}

error_code cellRtcSetDosTime(vm::ptr<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc.todo("cellRtcSetDosTime(pDateTime=*0x%x, uiDosTime=0x%x)", pDateTime, uiDosTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	s32 hms = uiDosTime & 0xffff;
	s32 ymd = uiDosTime >> 16;

	pDateTime->year        = (ymd >> 9) + 1980;
	pDateTime->month       = (ymd >> 5) & 0xf;
	pDateTime->day         = ymd & 0x1f;
	pDateTime->hour        = (hms >> 11);
	pDateTime->minute      = (hms >> 5) & 0x3f;
	pDateTime->second      = (hms << 1) & 0x3e;
	pDateTime->microsecond = 0;

	return CELL_OK;
}

u32 cellRtcGetTickResolution()
{
	// Amount of ticks in a second
	return 1000000;
}

error_code cellRtcSetTime_t(vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.todo("cellRtcSetTime_t(pDateTime=*0x%x, iTime=0x%llx)", pDateTime, iTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = iTime * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcSetTick(pDateTime, tick);

	return CELL_OK;
}

error_code cellRtcSetSystemTime(vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.todo("cellRtcSetSystemTime(pDateTime=*0x%x, iTime=0x%llx)", pDateTime, iTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = iTime * cellRtcGetTickResolution() + 0xe01d003a63a000;

	return cellRtcSetTick(pDateTime, tick);
}

error_code cellRtcSetWin32FileTime(vm::ptr<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc.todo("cellRtcSetWin32FileTime(pDateTime=*0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);

	if (!vm::check_addr(pDateTime.addr()))
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = ulWin32FileTime / 10 + RTC_FILETIME_OFFSET;

	return cellRtcSetTick(pDateTime, tick);
}

error_code cellRtcIsLeapYear(s32 year)
{
	cellRtc.todo("cellRtcIsLeapYear(year=%d)", year);

	if (year < 0)
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	return not_an_error(is_leap_year(year));
}

error_code cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc.todo("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	if ((year < 0) || (month < 0) || (month > 12))
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	if (is_leap_year(year))
	{
		return not_an_error(DAYS_IN_MONTH[month + 11]);
	}

	return not_an_error(DAYS_IN_MONTH[month + -1]);
}

error_code cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc.trace("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	if (month - 1 < 2)
	{
		year -= 1;
		month += 0xc;
	}

	return not_an_error(((month * 0xd + 8) / 5 + ((year + (year >> 2) + (year < 0 && (year & 3U) != 0)) - year / 100) + year / 400 + day) % 7);
}

error_code cellRtcCheckValid(vm::cptr<CellRtcDateTime> pTime)
{
	cellRtc.todo("cellRtcCheckValid(pTime=*0x%x)", pTime);
	cellRtc.todo("cellRtcCheckValid year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d, microsecond: %d\n", pTime->year, pTime->month, pTime->day, pTime->hour, pTime->minute, pTime->second, pTime->microsecond);

	if (pTime->year == 0 || pTime->year >= 10000)
	{
		return CELL_RTC_ERROR_INVALID_YEAR;
	}

	if (pTime->month < 1 || pTime->month > 12)
	{
		return CELL_RTC_ERROR_INVALID_MONTH;
	}

	s32 month_idx;

	if (is_leap_year(pTime->year))
	{
		// Leap year check
		month_idx = pTime->month + 11;
	}
	else
	{
		month_idx = pTime->month - 1;
	}

	if (pTime->day == 0 || pTime->day > DAYS_IN_MONTH[month_idx])
	{
		return CELL_RTC_ERROR_INVALID_DAY;
	}

	if (pTime->hour >= 24)
	{
		return CELL_RTC_ERROR_INVALID_HOUR;
	}

	if (pTime->minute >= 60)
	{
		return CELL_RTC_ERROR_INVALID_MINUTE;
	}

	if (pTime->second >= 60)
	{
		return CELL_RTC_ERROR_INVALID_SECOND;
	}

	if (pTime->microsecond >= cellRtcGetTickResolution())
	{
		return CELL_RTC_ERROR_INVALID_MICROSECOND;
	}

	return CELL_OK;
}

error_code cellRtcCompareTick(vm::cptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1)
{
	cellRtc.todo("cellRtcCompareTick(pTick0=*0x%x, pTick1=*0x%x)", pTick0, pTick1);

	s32 ret = -1;
	if (pTick1->tick <= pTick0->tick)
	{
		ret = pTick1->tick < pTick0->tick;
	}

	return not_an_error(ret);
}

DECLARE(ppu_module_manager::cellRtc)
("cellRtc", []() {
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
