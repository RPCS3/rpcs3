#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"

// Return Codes
enum CellRtcError
{
	CELL_RTC_ERROR_NOT_INITIALIZED     = 0x80010601,
	CELL_RTC_ERROR_INVALID_POINTER     = 0x80010602,
	CELL_RTC_ERROR_INVALID_VALUE       = 0x80010603,
	CELL_RTC_ERROR_INVALID_ARG         = 0x80010604,
	CELL_RTC_ERROR_NOT_SUPPORTED       = 0x80010605,
	CELL_RTC_ERROR_NO_CLOCK            = 0x80010606,
	CELL_RTC_ERROR_BAD_PARSE           = 0x80010607,
	CELL_RTC_ERROR_INVALID_YEAR        = 0x80010621,
	CELL_RTC_ERROR_INVALID_MONTH       = 0x80010622,
	CELL_RTC_ERROR_INVALID_DAY         = 0x80010623,
	CELL_RTC_ERROR_INVALID_HOUR        = 0x80010624,
	CELL_RTC_ERROR_INVALID_MINUTE      = 0x80010625,
	CELL_RTC_ERROR_INVALID_SECOND      = 0x80010626,
	CELL_RTC_ERROR_INVALID_MICROSECOND = 0x80010627,
};

struct CellRtcTick
{
	be_t<u64> tick;
};

struct CellRtcDateTime
{
	be_t<u16> year;        // 1 to 9999
	be_t<u16> month;       // 1 to 12
	be_t<u16> day;         // 1 to 31
	be_t<u16> hour;        // 0 to 23
	be_t<u16> minute;      // 0 to 59
	be_t<u16> second;      // 0 to 59
	be_t<u32> microsecond; // 0 to 999999
};

constexpr u64 RTC_MAGIC_OFFSET    = 62135596800000000ull; // 1970-01-01 00:00:00.000000
constexpr u64 RTC_FILETIME_OFFSET = 50491123200000000ull; // 1601-01-01 00:00:00.000000
constexpr u64 RTC_SYSTEM_TIME_MIN = 63082281600000000ull; // 2000-01-01 00:00:00.000000
constexpr u64 RTC_SYSTEM_TIME_MAX = 66238041599999999ull; // 2099-12-31 23:59:59.999999

constexpr u32 DAYS_IN_400_YEARS = 365 * 400 + 97;
constexpr u16 DAYS_IN_100_YEARS = 365 * 100 + 24;
constexpr u16 DAYS_IN_4_YEARS   = 365 *   4 +  1;

constexpr std::array<u8, 12> DAYS_IN_MONTH      = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
constexpr std::array<u8, 12> DAYS_IN_MONTH_LEAP = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

constexpr std::array<u16, 13> MONTH_OFFSET      = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, umax };
constexpr std::array<u16, 13> MONTH_OFFSET_LEAP = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, umax };

constexpr std::array<std::string_view, 7>  WEEKDAY_NAMES   = { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday" };
constexpr std::array<std::string_view, 12> MONTH_NAMES     = { "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december" };
constexpr std::array<std::string_view, 70> TIME_ZONE_NAMES = { "gmt", "est", "edt", "cst", "cdt", "mst", "mdt", "pst", "pdt", "nzdt", "nzst", "idle", "nzt", "aesst", "acsst", "cadt", "sadt", "aest", "east", "gst", "ligt", "acst", "sast", "cast", "awsst", "jst", "kst", "wdt", "mt", "awst", "cct", "wadt", "wst", "jt", "wast", "it", "bt", "eetdst", "eet", "cetdst", "fwt", "ist", "mest", "metdst", "sst", "bst", "cet", "dnt", "fst", "met", "mewt", "mez", "nor", "set", "swt", "wetdst", "wet", "wat", "ndt", "adt", "nft", "nst", "ast", "ydt", "hdt", "yst", "ahst", "cat", "nt", "idlw" };
constexpr std::array<s8, 70> TIME_ZONE_VALUES              = {     0,   -10,    -8,   -12,   -10,   -14,   -12,   -16,   -14,     26,     24,     24,    24,      22,      21,     21,     21,     20,     20,    20,     20,     19,     19,     19,      18,    18,    18,    18,   17,     16,    16,     16,    16,   15,     14,    7,    6,        6,     4,        4,     4,     4,      4,        4,     4,     2,     2,     2,     2,     2,      2,     2,     2,     2,     2,        2,     0,    -2,    -3,    -6,    -5,    -5,    -8,   -16,   -18,   -18,    -20,   -20,  -22,    -24 }; // In units of 30 minutes

error_code cellRtcTickAddMinutes(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick0, vm::cptr<CellRtcTick> pTick1, s64 lAdd);

error_code cellRtcSetTick(ppu_thread& ppu, vm::ptr<CellRtcDateTime> pTime, vm::cptr<CellRtcTick> pTick);
constexpr u32 cellRtcGetTickResolution();
error_code cellRtcCheckValid(vm::cptr<CellRtcDateTime> pTime);
s32 cellRtcGetDayOfWeek(s32 year, s32 month, s32 day);
error_code cellRtcGetTick(ppu_thread& ppu, vm::cptr<CellRtcDateTime> pTime, vm::ptr<CellRtcTick> pTick);
error_code cellRtcGetCurrentTick(ppu_thread& ppu, vm::ptr<CellRtcTick> pTick);

CellRtcDateTime tick_to_date_time(u64 tick);
u64 date_time_to_tick(CellRtcDateTime date_time);
