#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellRtc.h"

#include "Emu/Cell/lv2/sys_time.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_ss.h"

LOG_CHANNEL(cellRtc);

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

static inline char ascii(u8 num)
{
	return num + '0';
}

static inline s8 digit(char c)
{
	return c - '0';
}

static bool is_leap_year(u32 year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

error_code cellRtcGetCurrentTick(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.trace("cellRtcGetCurrentTick(pTick=*0x%x)", pTick);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	error_code ret = sys_time_get_current_time(sec, nsec);
	if (ret < CELL_OK)
	{
		return ret;
	}

	pTick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	return CELL_OK;
}

error_code cellRtcGetCurrentClock(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pClock, s32 iTimeZone)
{
	cellRtc.trace("cellRtcGetCurrentClock(pClock=*0x%x, iTimeZone=%d)", pClock, iTimeZone);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pClock.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	if (sys_memory_get_page_attribute(ppu, tick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	error_code ret = sys_time_get_current_time(sec, nsec);
	if (ret < CELL_OK)
	{
		return ret;
	}

	tick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcTickAddMinutes(ppu, tick, tick, iTimeZone);
	cellRtcSetTick(ppu, pClock, tick);

	return CELL_OK;
}

error_code cellRtcGetCurrentClockLocalTime(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pClock)
{
	cellRtc.trace("cellRtcGetCurrentClockLocalTime(pClock=*0x%x)", pClock);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pClock.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret < CELL_OK)
	{
		return ret;
	}

	if (sys_memory_get_page_attribute(ppu, pClock.addr(), page_attr) != CELL_OK) // Should always evaluate to false, already checked above
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	if (sys_memory_get_page_attribute(ppu, tick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s64> sec;
	vm::var<s64> nsec;

	ret = sys_time_get_current_time(sec, nsec);
	if (ret < CELL_OK)
	{
		return ret;
	}

	tick->tick = *nsec / 1000 + *sec * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcTickAddMinutes(ppu, tick, tick, *timezone + *summertime);
	cellRtcSetTick(ppu, pClock, tick);

	return CELL_OK;
}

error_code cellRtcFormatRfc2822(ppu_thread& ppu, vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.notice("cellRtcFormatRfc2822(pszDateTime=*0x%x, pUtc=*0x%x, iTimeZone=%d)", pszDateTime, pUtc, iTimeZone);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> rtc_tick;
	if (!pUtc) // Should always evaluate to false, nullptr was already checked above
	{
		cellRtcGetCurrentTick(ppu, rtc_tick);
	}
	else
	{
		rtc_tick->tick = pUtc->tick;
	}

	vm::var<CellRtcDateTime> date_time;

	cellRtcTickAddMinutes(ppu, rtc_tick, rtc_tick, iTimeZone);
	cellRtcSetTick(ppu, date_time, rtc_tick);

	error_code ret = cellRtcCheckValid(date_time);
	if (ret != CELL_OK)
	{
		return ret;
	}

	s32 weekdayIdx = cellRtcGetDayOfWeek(date_time->year, date_time->month, date_time->day);
	// Day name
	pszDateTime[0] = std::toupper(WEEKDAY_NAMES[weekdayIdx][0]);
	pszDateTime[1] = WEEKDAY_NAMES[weekdayIdx][1];
	pszDateTime[2] = WEEKDAY_NAMES[weekdayIdx][2];
	pszDateTime[3] = ',';
	pszDateTime[4] = ' ';
	// Day number
	pszDateTime[5] = ascii(date_time->day / 10);
	pszDateTime[6] = ascii(date_time->day % 10);
	pszDateTime[7] = ' ';

	// month name
	pszDateTime[8]   = std::toupper(MONTH_NAMES[date_time->month - 1][0]);
	pszDateTime[9]   = MONTH_NAMES[date_time->month - 1][1];
	pszDateTime[10]  = MONTH_NAMES[date_time->month - 1][2];
	pszDateTime[0xb] = ' ';

	// year
	pszDateTime[0xc]  = ascii(date_time->year / 1000);
	pszDateTime[0xd]  = ascii(date_time->year / 100 % 10);
	pszDateTime[0xe]  = ascii(date_time->year / 10 % 10);
	pszDateTime[0xf]  = ascii(date_time->year % 10);
	pszDateTime[0x10] = ' ';

	// Hours
	pszDateTime[0x11] = ascii(date_time->hour / 10);
	pszDateTime[0x12] = ascii(date_time->hour % 10);
	pszDateTime[0x13] = ':';

	// Minutes
	pszDateTime[0x14] = ascii(date_time->minute / 10);
	pszDateTime[0x15] = ascii(date_time->minute % 10);
	pszDateTime[0x16] = ':';

	// Seconds
	pszDateTime[0x17] = ascii(date_time->second / 10);
	pszDateTime[0x18] = ascii(date_time->second % 10);
	pszDateTime[0x19] = ' ';

	// Timezone -/+
	if (iTimeZone < 0)
	{
		iTimeZone = -iTimeZone;
		pszDateTime[0x1a] = '-';
	}
	else
	{
		pszDateTime[0x1a] = '+';
	}

	const u32 time_zone_hours = iTimeZone / 60 % 100;
	const u32 time_zone_minutes = iTimeZone % 60;

	pszDateTime[0x1b] = ascii(time_zone_hours / 10);
	pszDateTime[0x1c] = ascii(time_zone_hours % 10);
	pszDateTime[0x1d] = ascii(time_zone_minutes / 10);
	pszDateTime[0x1e] = ascii(time_zone_minutes % 10);
	pszDateTime[0x1f] = '\0';

	return CELL_OK;
}

error_code cellRtcFormatRfc2822LocalTime(ppu_thread& ppu, vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc)
{
	cellRtc.notice("cellRtcFormatRfc2822LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret < CELL_OK)
	{
		return ret;
	}

	return cellRtcFormatRfc2822(ppu, pszDateTime, pUtc, *timezone + *summertime);
}

error_code cellRtcFormatRfc3339(ppu_thread& ppu, vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc, s32 iTimeZone)
{
	cellRtc.notice("cellRtcFormatRfc3339(pszDateTime=*0x%x, pUtc=*0x%x, iTimeZone=%d)", pszDateTime, pUtc, iTimeZone);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> rtc_tick;
	if (!pUtc) // Should always evaluate to false, nullptr was already checked above
	{
		cellRtcGetCurrentTick(ppu, rtc_tick);
	}
	else
	{
		rtc_tick->tick = pUtc->tick;
	}

	vm::var<CellRtcDateTime> date_time;

	cellRtcTickAddMinutes(ppu, rtc_tick, rtc_tick, iTimeZone);
	cellRtcSetTick(ppu, date_time, rtc_tick);

	error_code ret = cellRtcCheckValid(date_time);
	if (ret != CELL_OK)
	{
		return ret;
	}

	// Year - XXXX-04-13T10:56:31.35+66:40
	pszDateTime[0x0] = ascii(date_time->year / 1000);
	pszDateTime[0x1] = ascii(date_time->year / 100 % 10);
	pszDateTime[0x2] = ascii(date_time->year / 10 % 10);
	pszDateTime[0x3] = ascii(date_time->year % 10);
	pszDateTime[0x4] = '-';

	// Month - 2020-XX-13T10:56:31.35+66:40
	pszDateTime[0x5] = ascii(date_time->month / 10);
	pszDateTime[0x6] = ascii(date_time->month % 10);
	pszDateTime[0x7] = '-';

	// Day - 2020-04-XXT10:56:31.35+66:40
	pszDateTime[0x8] = ascii(date_time->day / 10);
	pszDateTime[0x9] = ascii(date_time->day % 10);
	pszDateTime[0xa] = 'T';

	// Hours - 2020-04-13TXX:56:31.35+66:40
	pszDateTime[0xb] = ascii(date_time->hour / 10);
	pszDateTime[0xc] = ascii(date_time->hour % 10);
	pszDateTime[0xd] = ':';

	// Minutes - 2020-04-13T10:XX:31.35+66:40
	pszDateTime[0xe]  = ascii(date_time->minute / 10);
	pszDateTime[0xf]  = ascii(date_time->minute % 10);
	pszDateTime[0x10] = ':';

	// Seconds - 2020-04-13T10:56:XX.35+66:40
	pszDateTime[0x11] = ascii(date_time->second / 10);
	pszDateTime[0x12] = ascii(date_time->second % 10);
	pszDateTime[0x13] = '.';

	// Hundredths of a second - 2020-04-13T10:56:31.XX+66:40
	pszDateTime[0x14] = ascii(date_time->microsecond / 100'000);
	pszDateTime[0x15] = ascii(date_time->microsecond / 10'000 % 10);

	// Time zone - 'Z' for UTC
	if (iTimeZone == 0)
	{
		pszDateTime[0x16] = 'Z';
		pszDateTime[0x17] = '\0';
	}
	// Time zone - ±hh:mm
	else
	{
		if (iTimeZone < 0)
		{
			iTimeZone = -iTimeZone;
			pszDateTime[0x16] = '-';
		}
		else
		{
			pszDateTime[0x16] = '+';
		}

		const u32 time_zone_hours = iTimeZone / 60 % 100;
		const u32 time_zone_minutes = iTimeZone % 60;

		pszDateTime[0x17] = ascii(time_zone_hours / 10);
		pszDateTime[0x18] = ascii(time_zone_hours % 10);
		pszDateTime[0x19] = ':';
		pszDateTime[0x1a] = ascii(time_zone_minutes / 10);
		pszDateTime[0x1b] = ascii(time_zone_minutes % 10);
		pszDateTime[0x1c] = '\0';
	}

	return CELL_OK;
}

error_code cellRtcFormatRfc3339LocalTime(ppu_thread& ppu, vm::ptr<char> pszDateTime, vm::cptr<CellRtcTick> pUtc)
{
	cellRtc.notice("cellRtcFormatRfc3339LocalTime(pszDateTime=*0x%x, pUtc=*0x%x)", pszDateTime, pUtc);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (ret < CELL_OK)
	{
		return ret;
	}

	return cellRtcFormatRfc3339(ppu, pszDateTime, pUtc, *timezone + *summertime);
}

u16 rtcParseComponent(vm::cptr<char> pszDateTime, u32& pos, char delimiter, const char* component_name)
{
	if (delimiter != 0)
	{
		if (pszDateTime[pos] != delimiter)
		{
			cellRtc.error("rtcParseComponent(): failed to parse %s: invalid or missing delimiter", component_name);
			return umax;
		}

		pos++;
	}

	if (!std::isdigit(pszDateTime[pos]))
	{
		cellRtc.error("rtcParseComponent(): failed to parse %s: ASCII value 0x%x at position %d is not a digit", component_name, pszDateTime[pos + 1], pos);
		return umax;
	}

	u16 ret = digit(pszDateTime[pos]);

	pos++;

	if (std::isdigit(pszDateTime[pos]))
	{
		ret = ret * 10 + digit(pszDateTime[pos]);

		pos++;
	}

	return ret;
}

template<usz size>
u8 rtcParseName(vm::cptr<char> pszDateTime, u32& pos, const std::array<std::string_view, size>& names, bool allow_short_name = true)
{
	for (u8 name_idx = 0; name_idx < names.size(); name_idx++)
	{
		const u32 name_length = static_cast<u32>(names[name_idx].length());

		u32 ch_idx = 0;

		while (ch_idx < name_length && std::tolower(pszDateTime[pos + ch_idx]) == names[name_idx][ch_idx]) // Not case sensitive
		{
			ch_idx++;
		}

		if (ch_idx == name_length) // Full name matched
		{
			pos += name_length;
			return name_idx;
		}

		if (allow_short_name && ch_idx >= 3) // Short name matched
		{
			pos += 3; // Only increment by 3, even if more letters were matched
			return name_idx;
		}
	}

	return size;
}

error_code rtcParseRfc2822(ppu_thread& ppu, vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime, u32 pos)
{
	// Day: "X" or "XX"
	const u16 day = rtcParseComponent(pszDateTime, pos, 0, "day");

	if (day == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	// Mandatory space or hyphen
	if (pszDateTime[pos] != ' ' && pszDateTime[pos] != '-')
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): invalid or missing delimiter after day" };
	}

	pos++;

	// Month: at least the first three letters
	const u16 month = rtcParseName(pszDateTime, pos, MONTH_NAMES) + 1;

	if (month > MONTH_NAMES.size()) // No match
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): failed to parse month: string at position %d doesn't match any name", pos };
	}

	// Mandatory space or hyphen
	if (pszDateTime[pos] != ' ' && pszDateTime[pos] != '-')
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): invalid or missing delimiter after month" };
	}

	pos++;

	// Year: "XX" or "XXXX"
	u16 year = 0;

	if (!std::isdigit(pszDateTime[pos]) ||
		!std::isdigit(pszDateTime[pos + 1]))
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): failed to parse year: one of the first two ASCII values 0x%x, 0x%x at position %d is not a digit",
			pszDateTime[pos], pszDateTime[pos + 1], pos };
	}

	if (!std::isdigit(pszDateTime[pos + 2]) ||
		!std::isdigit(pszDateTime[pos + 3]))
	{
		year = digit(pszDateTime[pos]) * 10 + digit(pszDateTime[pos + 1]);
		year += (year < 50) ? 2000 : 1900;
		pos += 2;
	}
	else
	{
		year = digit(pszDateTime[pos]) * 1000 + digit(pszDateTime[pos + 1]) * 100 + digit(pszDateTime[pos + 2]) * 10 + digit(pszDateTime[pos + 3]);
		pos += 4;
	}

	// Hour: " X" or " XX"
	const u16 hour = rtcParseComponent(pszDateTime, pos, ' ', "hour");

	if (hour == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	if (hour > 25) // LLE uses 25
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): failed to parse hour: hour greater than 25" };
	}

	// Minute: ":X" or ":XX"
	const u16 minute = rtcParseComponent(pszDateTime, pos, ':', "minute");

	if (minute == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	// Second, optional: ":X" or ":XX"
	// The string can't end with '\0' here, there must a space before it
	u16 second = 0;

	if (pszDateTime[pos] != ' ')
	{
		second = rtcParseComponent(pszDateTime, pos, ':', "second");

		if (second == umax)
		{
			return CELL_RTC_ERROR_BAD_PARSE;
		}
	}
	else
	{
		// If there are no seconds in the string, time zone requires two preceding spaces to be properly parsed
		pos++;
	}

	// Time zone, optional, error if there is no valid time zone after the space
	s32 time_zone = 0;

	if (pszDateTime[pos] == ' ')
	{
		pos++;

		if (pszDateTime[pos] == '+' || pszDateTime[pos] == '-')
		{
			// "±hhmm"

			if (std::isdigit(pszDateTime[pos + 1]) &&
				std::isdigit(pszDateTime[pos + 2]) &&
				std::isdigit(pszDateTime[pos + 3]) &&
				std::isdigit(pszDateTime[pos + 4]))
			{
				const s32 time_zone_hhmm = digit(pszDateTime[pos + 1]) * 1000 + digit(pszDateTime[pos + 2]) * 100 + digit(pszDateTime[pos + 3]) * 10 + digit(pszDateTime[pos + 4]);

				time_zone = time_zone_hhmm / 100 * 60 + time_zone_hhmm % 60; // LLE uses % 60 instead of % 100
			}
			else
			{
				// No error, LLE does this for some reason
				time_zone = -1;
			}

			if (pszDateTime[pos] == '-')
			{
				time_zone = -time_zone;
			}
		}
		else if (pszDateTime[pos] != 'U' && pszDateTime[pos + 1] != 'T') // Case sensitive, should be || but LLE uses &&
		{
			// "GMT", "EST", "EDT", etc.

			const u32 time_zone_idx = rtcParseName(pszDateTime, pos, TIME_ZONE_NAMES, false);

			if (time_zone_idx < TIME_ZONE_NAMES.size())
			{
				time_zone = TIME_ZONE_VALUES[time_zone_idx] * 30;
			}
			else
			{
				// Military time zones
				// "A", "B", "C", ..., not case sensitive
				// These are all off by one ("A" should be UTC+01:00, "B" should be UTC+02:00, etc.)

				const char letter = std::toupper(pszDateTime[pos]);

				if (letter >= 'A' && letter <= 'M' && letter != 'J')
				{
					time_zone = (letter - 'A') * 60;
				}
				else if (letter >= 'N' && letter <= 'Y')
				{
					time_zone = ('N' - letter) * 60;
				}
				else if (letter != 'Z')
				{
					return { CELL_RTC_ERROR_BAD_PARSE, "rtcParseRfc2822(): failed to parse time zone" };
				}
			}
		}
	}

	const vm::var<CellRtcDateTime> date_time{{ year, month, day, hour, minute, second, 0 }};

	cellRtcGetTick(ppu, date_time, pUtc);
	cellRtcTickAddMinutes(ppu, pUtc, pUtc, -time_zone); // The time zone value needs to be subtracted

	return CELL_OK;
}

error_code cellRtcParseRfc3339(ppu_thread& ppu, vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime);

/*
 Takes a RFC2822 / RFC3339 / asctime String, and converts it to a CellRtcTick
*/
error_code cellRtcParseDateTime(ppu_thread& ppu, vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.notice("cellRtcParseDateTime(pUtc=*0x%x, pszDateTime=%s)", pUtc, pszDateTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK || sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	u32 pos = 0;

	while (std::isblank(pszDateTime[pos]))
	{
		pos++;
	}

	if (std::isdigit(pszDateTime[pos]) &&
		std::isdigit(pszDateTime[pos + 1]) &&
		std::isdigit(pszDateTime[pos + 2]) &&
		std::isdigit(pszDateTime[pos + 3]))
	{
		return cellRtcParseRfc3339(ppu, pUtc, pszDateTime + pos);
	}

	// Day of the week: at least the first three letters
	if (rtcParseName(pszDateTime, pos, WEEKDAY_NAMES) == WEEKDAY_NAMES.size()) // No match
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): failed to parse day of the week: string at position %d doesn't match any name", pos };
	}

	// Optional comma
	if (pszDateTime[pos] == ',')
	{
		pos++;
	}

	// Skip spaces and tabs
	while (std::isblank(pszDateTime[pos]))
	{
		pos++;
	}

	// Month: at least the first three letters
	const u16 month = rtcParseName(pszDateTime, pos, MONTH_NAMES) + 1;

	if (month > MONTH_NAMES.size()) // No match
	{
		cellRtc.notice("cellRtcParseDateTime(): string uses RFC 2822 format");
		return rtcParseRfc2822(ppu, pUtc, pszDateTime, pos);
	}

	// Mandatory space
	if (pszDateTime[pos] != ' ')
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): no space after month" };
	}

	pos++;

	// Day: " X", "XX" or "X"
	// There may be a second space before day
	u16 day = 0;

	if (pszDateTime[pos] == ' ')
	{
		pos++;

		// Due to using a signed type instead of unsigned, LLE doesn't check if the char is less than '0'
		if (pszDateTime[pos] > '9')
		{
			return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): failed to parse day: ASCII value 0x%x at position %d is not a digit", pszDateTime[pos], pos };
		}

		if (pszDateTime[pos] < '0')
		{
			cellRtc.warning("cellRtcParseDateTime(): ASCII value 0x%x at position %d is not a digit", pszDateTime[pos], pos);
		}

		day = static_cast<u16>(pszDateTime[pos]) - '0'; // Needs to be sign extended first to match LLE for values from 0x80 to 0xb0

		pos++;
	}
	else if (std::isdigit(pszDateTime[pos]))
	{
		day = digit(pszDateTime[pos]);

		pos++;

		if (std::isdigit(pszDateTime[pos]))
		{
			day = day * 10 + digit(pszDateTime[pos]);

			pos++;
		}
	}
	else
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): failed to parse day: ASCII value 0x%x at position %d is not a digit or space", pszDateTime[pos], pos };
	}

	// Hour: " X" or " XX"
	const u16 hour = rtcParseComponent(pszDateTime, pos, ' ', "hour");

	if (hour == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	// Minute: ":X" or ":XX"
	const u16 minute = rtcParseComponent(pszDateTime, pos, ':', "minute");

	if (minute == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	// Second: ":X" or ":XX"
	const u16 second = rtcParseComponent(pszDateTime, pos, ':', "second");

	if (second == umax)
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	// Mandatory space
	if (pszDateTime[pos] != ' ')
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): no space after second" };
	}

	pos++;

	// Year: XXXX
	if (!std::isdigit(pszDateTime[pos]) ||
		!std::isdigit(pszDateTime[pos + 1]) ||
		!std::isdigit(pszDateTime[pos + 2]) ||
		!std::isdigit(pszDateTime[pos + 3]))
	{
		return { CELL_RTC_ERROR_BAD_PARSE, "cellRtcParseDateTime(): failed to parse year: one of the ASCII values 0x%x, 0x%x, 0x%x, or 0x%x is not a digit",
			pszDateTime[pos], pszDateTime[pos + 1], pszDateTime[pos + 2], pszDateTime[pos + 3] };
	}

	const u16 year = digit(pszDateTime[pos]) * 1000 + digit(pszDateTime[pos + 1]) * 100 + digit(pszDateTime[pos + 2]) * 10 + digit(pszDateTime[pos + 3]);

	const vm::var<CellRtcDateTime> date_time{{ year, month, day, hour, minute, second, 0 }};

	cellRtcGetTick(ppu, date_time, pUtc);

	return CELL_OK;
}

// Rfc3339: 1995-12-03T13:23:00.00Z
error_code cellRtcParseRfc3339(ppu_thread& ppu, vm::ptr<CellRtcTick> pUtc, vm::cptr<char> pszDateTime)
{
	cellRtc.notice("cellRtcParseRfc3339(pUtc=*0x%x, pszDateTime=%s)", pUtc, pszDateTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pUtc.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pszDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;

	// Year: XXXX-12-03T13:23:00.00Z
	if (std::isdigit(pszDateTime[0]) && std::isdigit(pszDateTime[1]) && std::isdigit(pszDateTime[2]) && std::isdigit(pszDateTime[3]))
	{
		date_time->year = digit(pszDateTime[0]) * 1000 + digit(pszDateTime[1]) * 100 + digit(pszDateTime[2]) * 10 + digit(pszDateTime[3]);
	}
	else
	{
		date_time->year = 0xffff;
	}

	if (pszDateTime[4] != '-')
	{
		return CELL_RTC_ERROR_INVALID_YEAR;
	}

	// Month: 1995-XX-03T13:23:00.00Z
	if (std::isdigit(pszDateTime[5]) && std::isdigit(pszDateTime[6]))
	{
		date_time->month = digit(pszDateTime[5]) * 10 + digit(pszDateTime[6]);
	}
	else
	{
		date_time->month = 0xffff;
	}

	if (pszDateTime[7] != '-')
	{
		return CELL_RTC_ERROR_INVALID_MONTH;
	}

	// Day: 1995-12-XXT13:23:00.00Z
	if (std::isdigit(pszDateTime[8]) && std::isdigit(pszDateTime[9]))
	{
		date_time->day = digit(pszDateTime[8]) * 10 + digit(pszDateTime[9]);
	}
	else
	{
		date_time->day = 0xffff;
	}

	if (pszDateTime[10] != 'T' && pszDateTime[10] != 't')
	{
		return CELL_RTC_ERROR_INVALID_DAY;
	}

	// Hour: 1995-12-03TXX:23:00.00Z
	if (std::isdigit(pszDateTime[11]) && std::isdigit(pszDateTime[12]))
	{
		date_time->hour = digit(pszDateTime[11]) * 10 + digit(pszDateTime[12]);
	}
	else
	{
		date_time->hour = 0xffff;
	}

	if (pszDateTime[13] != ':')
	{
		return CELL_RTC_ERROR_INVALID_HOUR;
	}

	// Minute: 1995-12-03T13:XX:00.00Z
	if (std::isdigit(pszDateTime[14]) && std::isdigit(pszDateTime[15]))
	{
		date_time->minute = digit(pszDateTime[14]) * 10 + digit(pszDateTime[15]);
	}
	else
	{
		date_time->minute = 0xffff;
	}

	if (pszDateTime[16] != ':')
	{
		return CELL_RTC_ERROR_INVALID_MINUTE;
	}

	// Second: 1995-12-03T13:23:XX.00Z
	if (std::isdigit(pszDateTime[17]) && std::isdigit(pszDateTime[18]))
	{
		date_time->second = digit(pszDateTime[17]) * 10 + digit(pszDateTime[18]);
	}
	else
	{
		date_time->second = 0xffff;
	}

	// Microsecond: 1995-12-03T13:23:00.XXZ
	date_time->microsecond = 0;

	u32 pos = 19;
	if (pszDateTime[pos] == '.')
	{
		u32 mul = 100000;

		for (char c = pszDateTime[++pos]; std::isdigit(c); c = pszDateTime[++pos])
		{
			date_time->microsecond += digit(c) * mul;
			mul /= 10;
		}
	}

	const char sign = pszDateTime[pos];

	if (sign != 'Z' && sign != 'z' && sign != '+' && sign != '-')
	{
		return CELL_RTC_ERROR_BAD_PARSE;
	}

	s64 minutes_to_add = 0;

	// Time offset: 1995-12-03T13:23:00.00+02:30
	if (sign == '+' || sign == '-')
	{
		if (!std::isdigit(pszDateTime[pos + 1]) ||
			!std::isdigit(pszDateTime[pos + 2]) ||
			pszDateTime[pos + 3] != ':' ||
			!std::isdigit(pszDateTime[pos + 4]) ||
			!std::isdigit(pszDateTime[pos + 5]))
		{
			return CELL_RTC_ERROR_BAD_PARSE;
		}

		// Time offset (hours): 1995-12-03T13:23:00.00+XX:30
		const s32 hours = digit(pszDateTime[pos + 1]) * 10 + digit(pszDateTime[pos + 2]);

		// Time offset (minutes): 1995-12-03T13:23:00.00+02:XX
		const s32 minutes = digit(pszDateTime[pos + 4]) * 10 + digit(pszDateTime[pos + 5]);

		minutes_to_add = hours * 60 + minutes;

		if (sign == '+')
		{
			minutes_to_add = -minutes_to_add;
		}
	}

	cellRtcGetTick(ppu, date_time, pUtc);
	cellRtcTickAddMinutes(ppu, pUtc, pUtc, minutes_to_add);

	return CELL_OK;
}

error_code cellRtcGetTick(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick)
{
	cellRtc.trace("cellRtcGetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pTime->year >= 10000 || pTime->year == 0)
	{
		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	pTick->tick = date_time_to_tick(*pTime);

	return CELL_OK;
}

CellRtcDateTime tick_to_date_time(u64 tick)
{
	const u32 microseconds = (tick % 1000000ULL);
	const u16 seconds      = (tick / (1000000ULL)) % 60;
	const u16 minutes      = (tick / (60ULL * 1000000ULL)) % 60;
	const u16 hours        = (tick / (60ULL * 60ULL * 1000000ULL)) % 24;
	u32 days_tmp           = static_cast<u32>(tick / (24ULL * 60ULL * 60ULL * 1000000ULL));

	const u32 year_400 = days_tmp / DAYS_IN_400_YEARS;
	days_tmp -= year_400 * DAYS_IN_400_YEARS;

	const u32 year_within_400_interval = (
		days_tmp
		- days_tmp / (DAYS_IN_4_YEARS - 1)
		+ days_tmp / DAYS_IN_100_YEARS
		- days_tmp / (DAYS_IN_400_YEARS - 1)
		) / 365;

	days_tmp -= year_within_400_interval * 365 + year_within_400_interval / 4 - year_within_400_interval / 100 + year_within_400_interval / 400;

	const u16 years = year_400 * 400 + year_within_400_interval + 1;

	const auto& month_offset = is_leap_year(years) ? MONTH_OFFSET_LEAP : MONTH_OFFSET;

	u32 month_approx = days_tmp / 29;

	if (month_offset[month_approx] > days_tmp)
	{
		month_approx--;
	}

	const u16 months = month_approx + 1;
	days_tmp = days_tmp - month_offset[month_approx];

	CellRtcDateTime date_time{
		.year        = years,
		.month       = months,
		.day         = ::narrow<u16>(days_tmp + 1),
		.hour        = hours,
		.minute      = minutes,
		.second      = seconds,
		.microsecond = microseconds
	};
	return date_time;
}

u64 date_time_to_tick(CellRtcDateTime date_time)
{
	const u32 days_in_previous_years =
		date_time.year * 365
		+ (date_time.year + 3) / 4
		- (date_time.year + 99) / 100
		+ (date_time.year + 399) / 400
		- 366;

	// Not checked on LLE
	if (date_time.month == 0u)
	{
		cellRtc.warning("date_time_to_tick(): month invalid, clamping to 1");
		date_time.month = 1;
	}
	else if (date_time.month > 12u)
	{
		cellRtc.warning("date_time_to_tick(): month invalid, clamping to 12");
		date_time.month = 12;
	}

	const u16 days_in_previous_months = is_leap_year(date_time.year) ? MONTH_OFFSET_LEAP[date_time.month - 1] : MONTH_OFFSET[date_time.month - 1];

	const u32 days = days_in_previous_years + days_in_previous_months + date_time.day - 1;

	u64 tick = date_time.microsecond
	         + u64{date_time.second} * 1000000ULL
	         + u64{date_time.minute} * 60ULL * 1000000ULL
	         + u64{date_time.hour}   * 60ULL * 60ULL * 1000000ULL
	         + days                  * 24ULL * 60ULL * 60ULL * 1000000ULL;

	return tick;
}

error_code cellRtcSetTick(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pTime, vm::cptr<CellRtcTick> pTick)
{
	cellRtc.trace("cellRtcSetTick(pTime=*0x%x, pTick=*0x%x)", pTime, pTick);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	*pTime = tick_to_date_time(pTick->tick);

	return CELL_OK;
}

error_code cellRtcTickAddTicks(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddTicks(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd;

	return CELL_OK;
}

error_code cellRtcTickAddMicroseconds(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddMicroseconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd;

	return CELL_OK;
}

error_code cellRtcTickAddSeconds(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddSeconds(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddMinutes(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd)
{
	cellRtc.trace("cellRtcTickAddMinutes(pTick0=*0x%x, pTick1=*0x%x, lAdd=%lld)", pTick0, pTick1, lAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + lAdd * 60 * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddHours(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddHours(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddDays(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddDays(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * 24ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddWeeks(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.trace("cellRtcTickAddWeeks(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	pTick0->tick = pTick1->tick + iAdd * 60ULL * 60ULL * 24ULL * 7ULL * cellRtcGetTickResolution();

	return CELL_OK;
}

error_code cellRtcTickAddMonths(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.notice("cellRtcTickAddMonths(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;
	cellRtcSetTick(ppu, date_time, pTick1);

	const s32 total_months = date_time->year * 12 + date_time->month + iAdd - 1;
	const u16 new_year = total_months / 12;
	const u16 new_month = total_months - new_year * 12 + 1;

	s32 month_days;

	if (new_year == 0u || new_month < 1u || new_month > 12u)
	{
		month_days = CELL_RTC_ERROR_INVALID_ARG; // LLE writes the error to this variable
	}
	else
	{
		month_days = is_leap_year(new_year) ? DAYS_IN_MONTH_LEAP[new_month - 1] : DAYS_IN_MONTH[new_month - 1];
	}

	if (month_days < static_cast<s32>(date_time->day))
	{
		date_time->day = static_cast<u16>(month_days);
	}

	date_time->month = new_month;
	date_time->year = new_year;
	cellRtcGetTick(ppu, date_time, pTick0);

	return CELL_OK;
}

error_code cellRtcTickAddYears(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s32 iAdd)
{
	cellRtc.notice("cellRtcTickAddYears(pTick0=*0x%x, pTick1=*0x%x, iAdd=%d)", pTick0, pTick1, iAdd);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pTick0.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTick1.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcDateTime> date_time;
	cellRtcSetTick(ppu, date_time, pTick1);

	const u16 month = date_time->month;
	const u16 new_year = date_time->year + iAdd;

	s32 month_days;

	if (new_year == 0u || month < 1u || month > 12u)
	{
		month_days = CELL_RTC_ERROR_INVALID_ARG; // LLE writes the error to this variable
	}
	else
	{
		month_days = is_leap_year(new_year) ? DAYS_IN_MONTH_LEAP[month - 1] : DAYS_IN_MONTH[month - 1];
	}

	if (month_days < static_cast<s32>(date_time->day))
	{
		date_time->day = static_cast<u16>(month_days);
	}

	date_time->year = new_year;
	cellRtcGetTick(ppu, date_time, pTick0);

	return CELL_OK;
}

error_code cellRtcConvertUtcToLocalTime(ppu_thread& ppu, vm::cptr<CellRtcTick> pUtc, vm::ptr<CellRtcTick> pLocalTime)
{
	cellRtc.trace("cellRtcConvertUtcToLocalTime(pUtc=*0x%x, pLocalTime=*0x%x)", pUtc, pLocalTime);

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (-1 < ret)
	{
		ret = cellRtcTickAddMinutes(ppu, pLocalTime, pUtc, *timezone + *summertime);
	}

	return ret;
}

error_code cellRtcConvertLocalTimeToUtc(ppu_thread& ppu, vm::cptr<CellRtcTick> pLocalTime, vm::ptr<CellRtcTick> pUtc)
{
	cellRtc.notice("cellRtcConvertLocalTimeToUtc(pLocalTime=*0x%x, pUtc=*0x%x)", pLocalTime, pUtc);

	vm::var<s32> timezone;
	vm::var<s32> summertime;

	error_code ret = sys_time_get_timezone(timezone, summertime);
	if (-1 < ret)
	{
		ret = cellRtcTickAddMinutes(ppu, pUtc, pLocalTime, -(*timezone + *summertime));
	}

	return ret;
}

error_code cellRtcGetCurrentSecureTick(ppu_thread& ppu, vm::ptr<CellRtcTick> tick)
{
	cellRtc.notice("cellRtcGetCurrentSecureTick(tick=*0x%x)", tick);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, tick.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	tick->tick = RTC_SYSTEM_TIME_MIN;

	const vm::var<u64> time{0};
	const vm::var<u64> status{0};

	error_code ret = sys_ss_secure_rtc(0x3002, 0, time.addr(), status.addr());

	if (ret >= CELL_OK)
	{
		tick->tick += *time * cellRtcGetTickResolution();
	}
	else if (ret == static_cast<s32>(SYS_SS_RTC_ERROR_UNK))
	{
		switch (*status)
		{
		case 1: ret = CELL_RTC_ERROR_NO_CLOCK; break;
		case 2: ret = CELL_RTC_ERROR_NOT_INITIALIZED; break;
		case 4: ret = CELL_RTC_ERROR_INVALID_VALUE; break;
		case 8: ret = CELL_RTC_ERROR_NO_CLOCK; break;
		default: return ret;
		}
	}

	return ret;
}

error_code cellRtcGetDosTime(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<u32> puiDosTime)
{
	cellRtc.notice("cellRtcGetDosTime(pDateTime=*0x%x, puiDosTime=*0x%x)", pDateTime, puiDosTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, puiDosTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pDateTime->year < 1980)
	{
		if (puiDosTime) // Should always evaluate to true, nullptr was already checked above
		{
			*puiDosTime = 0;
		}

		return -1;
	}

	if (pDateTime->year >= 2108)
	{
		if (puiDosTime) // Should always evaluate to true, nullptr was already checked above
		{
			*puiDosTime = 0xff9fbf7d; // kHighDosTime
		}

		return -1;
	}

	if (puiDosTime) // Should always evaluate to true, nullptr was already checked above
	{
		s32 year    = ((pDateTime->year - 1980) & 0x7F) << 9;
		s32 month   = ((pDateTime->month) & 0xF) << 5;
		s32 hour    = ((pDateTime->hour) & 0x1F) << 11;
		s32 minute  = ((pDateTime->minute) & 0x3F) << 5;
		s32 day     = (pDateTime->day) & 0x1F;
		s32 second  = ((pDateTime->second) >> 1) & 0x1F;
		s32 ymd     = year | month | day;
		s32 hms     = hour | minute | second;
		*puiDosTime = (ymd << 16) | hms;
	}

	return CELL_OK;
}

error_code cellRtcGetSystemTime(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<s64> pTimeStamp)
{
	cellRtc.notice("cellRtcGetSystemTime(pDateTime=*0x%x, pTimeStamp=*0x%x)", pDateTime, pTimeStamp);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pTimeStamp.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	cellRtcGetTick(ppu, pDateTime, tick);

	if (tick->tick < RTC_SYSTEM_TIME_MIN)
	{
		if (pTimeStamp) // Should always evaluate to true, nullptr was already checked above
		{
			*pTimeStamp = 0;
		}

		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	if (tick->tick >= RTC_SYSTEM_TIME_MAX + cellRtcGetTickResolution())
	{
		if (pTimeStamp) // Should always evaluate to true, nullptr was already checked above
		{
			*pTimeStamp = (RTC_SYSTEM_TIME_MAX - RTC_SYSTEM_TIME_MIN) / cellRtcGetTickResolution();
		}

		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	if (pTimeStamp) // Should always evaluate to true, nullptr was already checked above
	{
		*pTimeStamp = (tick->tick - RTC_SYSTEM_TIME_MIN) / cellRtcGetTickResolution();
	}

	return CELL_OK;
}

error_code cellRtcGetTime_t(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<s64> piTime)
{
	cellRtc.trace("cellRtcGetTime_t(pDateTime=*0x%x, piTime=*0x%x)", pDateTime, piTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, piTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	cellRtcGetTick(ppu, pDateTime, tick);

	if (tick->tick < RTC_MAGIC_OFFSET)
	{
		if (piTime) // Should always evaluate to true, nullptr was already checked above
		{
			*piTime = 0;
		}

		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	if (piTime) // Should always evaluate to true, nullptr was already checked above
	{
		*piTime = (tick->tick - RTC_MAGIC_OFFSET) / cellRtcGetTickResolution();
	}

	return CELL_OK;
}

error_code cellRtcGetWin32FileTime(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pDateTime, vm::ptr<u64> pulWin32FileTime)
{
	cellRtc.notice("cellRtcGetWin32FileTime(pDateTime=*0x%x, pulWin32FileTime=*0x%x)", pDateTime, pulWin32FileTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (sys_memory_get_page_attribute(ppu, pulWin32FileTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	cellRtcGetTick(ppu, pDateTime, tick);

	if (tick->tick < RTC_FILETIME_OFFSET)
	{
		if (pulWin32FileTime) // Should always evaluate to true, nullptr was already checked above
		{
			*pulWin32FileTime = 0;
		}

		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	if (pulWin32FileTime) // Should always evaluate to true, nullptr was already checked above
	{
		*pulWin32FileTime = (tick->tick - RTC_FILETIME_OFFSET) * 10;
	}

	return CELL_OK;
}

error_code cellRtcSetCurrentSecureTick(vm::ptr<CellRtcTick> pTick)
{
	cellRtc.notice("cellRtcSetCurrentSecureTick(pTick=*0x%x)", pTick);

	if (!pTick)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pTick->tick > RTC_SYSTEM_TIME_MAX)
	{
		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	return sys_ss_secure_rtc(0x3003, (pTick->tick - RTC_SYSTEM_TIME_MIN) / cellRtcGetTickResolution(), 0, 0);
}

error_code cellRtcSetCurrentTick(vm::cptr<CellRtcTick> pTick)
{
	cellRtc.notice("cellRtcSetCurrentTick(pTick=*0x%x)", pTick);

	if (!pTick)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	if (pTick->tick < RTC_MAGIC_OFFSET)
	{
		return CELL_RTC_ERROR_INVALID_VALUE;
	}

	const u64 unix_time = pTick->tick - RTC_MAGIC_OFFSET;

	const error_code ret = sys_time_set_current_time(unix_time / cellRtcGetTickResolution(), unix_time % cellRtcGetTickResolution() * 1000);

	return ret >= CELL_OK ? CELL_OK : ret;
}

error_code cellRtcSetConf(s64 unk1, s64 unk2, u32 timezone, u32 summertime)
{
	cellRtc.notice("cellRtcSetConf(unk1=0x%x, unk2=0x%x, timezone=%d, summertime=%d)", unk1, unk2, timezone, summertime);
	// Seems the first 2 args are ignored :|

	return sys_time_set_timezone(timezone, summertime);
}

error_code cellRtcSetDosTime(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pDateTime, u32 uiDosTime)
{
	cellRtc.notice("cellRtcSetDosTime(pDateTime=*0x%x, uiDosTime=0x%x)", pDateTime, uiDosTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
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

constexpr u32 cellRtcGetTickResolution()
{
	// Amount of ticks in a second
	return 1000000;
}

error_code cellRtcSetTime_t(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.notice("cellRtcSetTime_t(pDateTime=*0x%x, iTime=0x%llx)", pDateTime, iTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = iTime * cellRtcGetTickResolution() + RTC_MAGIC_OFFSET;

	cellRtcSetTick(ppu, pDateTime, tick);

	return CELL_OK;
}

error_code cellRtcSetSystemTime(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pDateTime, u64 iTime)
{
	cellRtc.notice("cellRtcSetSystemTime(pDateTime=*0x%x, iTime=0x%llx)", pDateTime, iTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = iTime * cellRtcGetTickResolution() + RTC_SYSTEM_TIME_MIN;

	cellRtcSetTick(ppu, pDateTime, tick);

	return CELL_OK;
}

error_code cellRtcSetWin32FileTime(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pDateTime, u64 ulWin32FileTime)
{
	cellRtc.notice("cellRtcSetWin32FileTime(pDateTime=*0x%x, ulWin32FileTime=0x%llx)", pDateTime, ulWin32FileTime);

	const vm::var<sys_page_attr_t> page_attr;

	if (sys_memory_get_page_attribute(ppu, pDateTime.addr(), page_attr) != CELL_OK)
	{
		return CELL_RTC_ERROR_INVALID_POINTER;
	}

	vm::var<CellRtcTick> tick;
	tick->tick = ulWin32FileTime / 10 + RTC_FILETIME_OFFSET;

	return cellRtcSetTick(ppu, pDateTime, tick);
}

error_code cellRtcIsLeapYear(s32 year)
{
	cellRtc.notice("cellRtcIsLeapYear(year=%d)", year);

	if (year < 1)
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	return not_an_error(is_leap_year(year));
}

error_code cellRtcGetDaysInMonth(s32 year, s32 month)
{
	cellRtc.notice("cellRtcGetDaysInMonth(year=%d, month=%d)", year, month);

	if ((year <= 0) || (month <= 0) || (month > 12))
	{
		return CELL_RTC_ERROR_INVALID_ARG;
	}

	return not_an_error(is_leap_year(year) ? DAYS_IN_MONTH_LEAP[month - 1] : DAYS_IN_MONTH[month - 1]);
}

s32 cellRtcGetDayOfWeek(s32 year, s32 month, s32 day)
{
	cellRtc.trace("cellRtcGetDayOfWeek(year=%d, month=%d, day=%d)", year, month, day);

	if (month == 1 || month == 2)
	{
		year--;
		month += 12;
	}

	return ((month * 13 + 8) / 5 + year + year / 4 - year / 100 + year / 400 + day) % 7;
}

error_code cellRtcCheckValid(vm::cptr<CellRtcDateTime> pTime)
{
	cellRtc.notice("cellRtcCheckValid(pTime=*0x%x)", pTime);

	ensure(!!pTime); // Not checked on LLE

	cellRtc.notice("cellRtcCheckValid year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d, microsecond: %d", pTime->year, pTime->month, pTime->day, pTime->hour, pTime->minute, pTime->second, pTime->microsecond);

	if (pTime->year == 0 || pTime->year >= 10000)
	{
		return CELL_RTC_ERROR_INVALID_YEAR;
	}

	if (pTime->month < 1 || pTime->month > 12)
	{
		return CELL_RTC_ERROR_INVALID_MONTH;
	}

	const auto& days_in_month = is_leap_year(pTime->year) ? DAYS_IN_MONTH_LEAP : DAYS_IN_MONTH;

	if (pTime->day == 0 || pTime->day > days_in_month[pTime->month - 1])
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

s32 cellRtcCompareTick(vm::cptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1)
{
	cellRtc.notice("cellRtcCompareTick(pTick0=*0x%x, pTick1=*0x%x)", pTick0, pTick1);

	ensure(!!pTick0 && !!pTick1); // Not checked on LLE

	s32 ret = -1;
	if (pTick1->tick <= pTick0->tick)
	{
		ret = pTick1->tick < pTick0->tick;
	}

	return ret;
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
