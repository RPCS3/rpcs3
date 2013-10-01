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

int cellRtcGetCurrentTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetCurrentClock()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetCurrentClockLocalTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcFormatRfc2822()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcFormatRfc2822LocalTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcFormatRfc3339()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcFormatRfc3339LocalTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcParseDateTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcParseRfc3339()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcSetTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddTicks()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddMicroseconds()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddSeconds()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddMinutes()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddHours()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddDays()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddWeeks()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddMonths()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcTickAddYears()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcConvertUtcToLocalTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcConvertLocalTimeToUtc()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetDosTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetTime_t()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetWin32FileTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcSetDosTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcSetTime_t()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcSetWin32FileTime()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcIsLeapYear()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetDaysInMonth()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcGetDayOfWeek()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcCheckValid()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
}

int cellRtcCompareTick()
{
	UNIMPLEMENTED_FUNC(cellRtc);
	return CELL_OK;
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