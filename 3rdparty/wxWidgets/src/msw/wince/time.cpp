/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/time.cpp
// Purpose:     Implements missing time functionality for WinCE
// Author:      Marco Cavallini (MCK) - wx@koansoftware.com
// Modified by: Vadim Zeitlin for VC8 support
// Created:     31-08-2003
// RCS-ID:      $Id: time.cpp 43076 2006-11-04 23:27:15Z VZ $
// Copyright:   (c) Marco Cavallini
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
#endif

#include "wx/msw/wince/time.h"

#if defined(__VISUALC__) && (__VISUALC__ >= 1400)

// VC8 does provide the time functions but not the standard ones
#include <altcecrt.h>

time_t __cdecl time(time_t *t)
{
    __time64_t t64;
    if ( !_time64(&t64) )
        return (time_t)-1;

    if ( t )
        *t = (time_t)t64;

    return (time_t)t64;
}

time_t __cdecl mktime(struct tm *t)
{
    return (time_t)_mktime64(t);
}

#else // !VC8

/////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                         //
//                             strftime() - taken from OpenBSD                             //
//                                                                                         //
/////////////////////////////////////////////////////////////////////////////////////////////

#define IN_NONE	0
#define IN_SOME	1
#define IN_THIS	2
#define IN_ALL	3
#define CHAR_BIT      8

#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)
#define TYPE_SIGNED(type) (((type) -1) < 0)

#define INT_STRLEN_MAXIMUM(type) \
    ((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + 1 + TYPE_SIGNED(type))

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#define MONSPERYEAR	12
#define DAYSPERWEEK	7
#define TM_YEAR_BASE	1900
#define HOURSPERDAY	24
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366

static char		wildabbr[] = "WILDABBR";

char *			tzname[2] = {
	wildabbr,
	wildabbr
};


#define Locale	(&C_time_locale)

struct lc_time_T {
	const char *	mon[MONSPERYEAR];
	const char *	month[MONSPERYEAR];
	const char *	wday[DAYSPERWEEK];
	const char *	weekday[DAYSPERWEEK];
	const char *	X_fmt;
	const char *	x_fmt;
	const char *	c_fmt;
	const char *	am;
	const char *	pm;
	const char *	date_fmt;
};

static const struct lc_time_T	C_time_locale = {
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	}, {
		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"
	}, {
		"Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat"
	}, {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday"
	},

	/* X_fmt */
	"%H:%M:%S",

	/*
	** x_fmt
	** C99 requires this format.
	** Using just numbers (as here) makes Quakers happier;
	** it's also compatible with SVR4.
	*/
	"%m/%d/%y",

	/*
	** c_fmt
	** C99 requires this format.
	** Previously this code used "%D %X", but we now conform to C99.
	** Note that
	**      "%a %b %d %H:%M:%S %Y"
	** is used by Solaris 2.3.
	*/
	"%a %b %e %T %Y",

	/* am */
	"AM",

	/* pm */
	"PM",

	/* date_fmt */
	"%a %b %e %H:%M:%S %Z %Y"
};


static char *
_add(const char * str, char * pt, const char * const ptlim)
{
	while (pt < ptlim && (*pt = *str++) != '\0')
		++pt;
	return pt;
}


static char *
_conv(const int n, const char * const format, char * const pt, const char * const ptlim)
{
	char	buf[INT_STRLEN_MAXIMUM(int) + 1];

	(void) _snprintf(buf, sizeof buf, format, n);
	return _add(buf, pt, ptlim);
}


static char *
_fmt(const char * format, const struct tm * const t, char * pt, const char * const ptlim, int * warnp)
{
	for ( ; *format; ++format) {
		if (*format == '%') {
label:
			switch (*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				pt = _add((t->tm_wday < 0 ||
					t->tm_wday >= DAYSPERWEEK) ?
					"?" : Locale->weekday[t->tm_wday],
					pt, ptlim);
				continue;
			case 'a':
				pt = _add((t->tm_wday < 0 ||
					t->tm_wday >= DAYSPERWEEK) ?
					"?" : Locale->wday[t->tm_wday],
					pt, ptlim);
				continue;
			case 'B':
				pt = _add((t->tm_mon < 0 ||
					t->tm_mon >= MONSPERYEAR) ?
					"?" : Locale->month[t->tm_mon],
					pt, ptlim);
				continue;
			case 'b':
			case 'h':
				pt = _add((t->tm_mon < 0 ||
					t->tm_mon >= MONSPERYEAR) ?
					"?" : Locale->mon[t->tm_mon],
					pt, ptlim);
				continue;
			case 'C':
				/*
				** %C used to do a...
				**	_fmt("%a %b %e %X %Y", t);
				** ...whereas now POSIX 1003.2 calls for
				** something completely different.
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_year + TM_YEAR_BASE) / 100,
					"%02d", pt, ptlim);
				continue;
			case 'c':
				{
				int warn2 = IN_SOME;

				pt = _fmt(Locale->c_fmt, t, pt, ptlim, warnp);
				if (warn2 == IN_ALL)
					warn2 = IN_THIS;
				if (warn2 > *warnp)
					*warnp = warn2;
				}
				continue;
			case 'D':
				pt = _fmt("%m/%d/%y", t, pt, ptlim, warnp);
				continue;
			case 'd':
				pt = _conv(t->tm_mday, "%02d", pt, ptlim);
				continue;
			case 'E':
			case 'O':
				/*
				** C99 locale modifiers.
				** The sequences
				**	%Ec %EC %Ex %EX %Ey %EY
				**	%Od %oe %OH %OI %Om %OM
				**	%OS %Ou %OU %OV %Ow %OW %Oy
				** are supposed to provide alternate
				** representations.
				*/
				goto label;
			case 'e':
				pt = _conv(t->tm_mday, "%2d", pt, ptlim);
				continue;
			case 'F':
				pt = _fmt("%Y-%m-%d", t, pt, ptlim, warnp);
				continue;
			case 'H':
				pt = _conv(t->tm_hour, "%02d", pt, ptlim);
				continue;
			case 'I':
				pt = _conv((t->tm_hour % 12) ?
					(t->tm_hour % 12) : 12,
					"%02d", pt, ptlim);
				continue;
			case 'j':
				pt = _conv(t->tm_yday + 1, "%03d", pt, ptlim);
				continue;
			case 'k':
				/*
				** This used to be...
				**	_conv(t->tm_hour % 12 ?
				**		t->tm_hour % 12 : 12, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbins'
				** strftime version 3.0.  That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv(t->tm_hour, "%2d", pt, ptlim);
				continue;
#ifdef KITCHEN_SINK
			case 'K':
				/*
				** After all this time, still unclaimed!
				*/
				pt = _add("kitchen sink", pt, ptlim);
				continue;
#endif /* defined KITCHEN_SINK */
			case 'l':
				/*
				** This used to be...
				**	_conv(t->tm_hour, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbin's
				** strftime version 3.0.  That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_hour % 12) ?
					(t->tm_hour % 12) : 12,
					"%2d", pt, ptlim);
				continue;
			case 'M':
				pt = _conv(t->tm_min, "%02d", pt, ptlim);
				continue;
			case 'm':
				pt = _conv(t->tm_mon + 1, "%02d", pt, ptlim);
				continue;
			case 'n':
				pt = _add("\n", pt, ptlim);
				continue;
			case 'p':
				pt = _add((t->tm_hour >= (HOURSPERDAY / 2)) ?
					Locale->pm :
					Locale->am,
					pt, ptlim);
				continue;
			case 'R':
				pt = _fmt("%H:%M", t, pt, ptlim, warnp);
				continue;
			case 'r':
				pt = _fmt("%I:%M:%S %p", t, pt, ptlim, warnp);
				continue;
			case 'S':
				pt = _conv(t->tm_sec, "%02d", pt, ptlim);
				continue;
			case 's':
				{
					struct tm	tm;
					char		buf[INT_STRLEN_MAXIMUM(
								time_t) + 1];
					time_t		mkt;

					tm = *t;
					mkt = mktime(&tm);
					if (TYPE_SIGNED(time_t))
						(void) _snprintf(buf, sizeof buf,
						    "%ld", (long) mkt);
					else	(void) _snprintf(buf, sizeof buf,
						    "%lu", (unsigned long) mkt);
					pt = _add(buf, pt, ptlim);
				}
				continue;
			case 'T':
				pt = _fmt("%H:%M:%S", t, pt, ptlim, warnp);
				continue;
			case 't':
				pt = _add("\t", pt, ptlim);
				continue;
			case 'U':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
					t->tm_wday) / DAYSPERWEEK,
					"%02d", pt, ptlim);
				continue;
			case 'u':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "ISO 8601: Weekday as a decimal number
				** [1 (Monday) - 7]"
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_wday == 0) ?
					DAYSPERWEEK : t->tm_wday,
					"%d", pt, ptlim);
				continue;
			case 'V':	/* ISO 8601 week number */
			case 'G':	/* ISO 8601 year (four digits) */
			case 'g':	/* ISO 8601 year (two digits) */
				{
					int	year;
					int	yday;
					int	wday;
					int	w;

					year = t->tm_year + TM_YEAR_BASE;
					yday = t->tm_yday;
					wday = t->tm_wday;
					for ( ; ; ) {
						int	len;
						int	bot;
						int	top;

						len = isleap(year) ?
							DAYSPERLYEAR :
							DAYSPERNYEAR;
						/*
						** What yday (-3 ... 3) does
						** the ISO year begin on?
						*/
						bot = ((yday + 11 - wday) %
							DAYSPERWEEK) - 3;
						/*
						** What yday does the NEXT
						** ISO year begin on?
						*/
						top = bot -
							(len % DAYSPERWEEK);
						if (top < -3)
							top += DAYSPERWEEK;
						top += len;
						if (yday >= top) {
							++year;
							w = 1;
							break;
						}
						if (yday >= bot) {
							w = 1 + ((yday - bot) /
								DAYSPERWEEK);
							break;
						}
						--year;
						yday += isleap(year) ?
							DAYSPERLYEAR :
							DAYSPERNYEAR;
					}
					if (*format == 'V')
						pt = _conv(w, "%02d",
							pt, ptlim);
					else if (*format == 'g') {
						*warnp = IN_ALL;
						pt = _conv(year % 100, "%02d",
							pt, ptlim);
					} else	pt = _conv(year, "%04d",
							pt, ptlim);
				}
				continue;
			case 'v':
				pt = _fmt("%e-%b-%Y", t, pt, ptlim, warnp);
				continue;
			case 'W':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
					(t->tm_wday ?
					(t->tm_wday - 1) :
					(DAYSPERWEEK - 1))) / DAYSPERWEEK,
					"%02d", pt, ptlim);
				continue;
			case 'w':
				pt = _conv(t->tm_wday, "%d", pt, ptlim);
				continue;
			case 'X':
				pt = _fmt(Locale->X_fmt, t, pt, ptlim, warnp);
				continue;
			case 'x':
				{
				int	warn2 = IN_SOME;

				pt = _fmt(Locale->x_fmt, t, pt, ptlim, &warn2);
				if (warn2 == IN_ALL)
					warn2 = IN_THIS;
				if (warn2 > *warnp)
					*warnp = warn2;
				}
				continue;
			case 'y':
				*warnp = IN_ALL;
				pt = _conv((t->tm_year + TM_YEAR_BASE) % 100,
					"%02d", pt, ptlim);
				continue;
			case 'Y':
				pt = _conv(t->tm_year + TM_YEAR_BASE, "%04d",
					pt, ptlim);
				continue;
			case 'Z':
				if (t->tm_isdst >= 0)
					pt = _add(tzname[t->tm_isdst != 0],
						pt, ptlim);
				/*
				** C99 says that %Z must be replaced by the
				** empty string if the time zone is not
				** determinable.
				*/
				continue;
			case 'z':
				{
				int		diff = -timezone;
				char const *	sign;

				if (diff < 0) {
					sign = "-";
					diff = -diff;
				} else	sign = "+";
				pt = _add(sign, pt, ptlim);
				diff /= 60;
				pt = _conv((diff/60)*100 + diff%60,
					"%04d", pt, ptlim);
				}
				continue;
			case '+':
				pt = _fmt(Locale->date_fmt, t, pt, ptlim,
					warnp);
				continue;
			case '%':
			default:
				break;
			}
		}
		if (pt == ptlim)
			break;
		*pt++ = *format;
	}
	return pt;
}

size_t
strftime(char * const s, const size_t maxsize, const char *format, const struct tm * const t)
{
	char *	p;
	int	warn;

	//tzset();

	warn = IN_NONE;
	p = _fmt(((format == NULL) ? "%c" : format), t, s, s + maxsize, &warn);

	if (p == s + maxsize) {
		if (maxsize > 0)
			s[maxsize - 1] = '\0';
		return 0;
	}
	*p = '\0';
	return p - s;
}

extern "C"
{

/* Not needed in VS Studio 2005 */

size_t wcsftime(wchar_t *s,
                const size_t maxsize,
                const wchar_t *format,
                const struct tm *t)
{
    wxCharBuffer sBuf(maxsize/sizeof(wchar_t));

    wxString formatStr(format);
    wxCharBuffer bufFormatStr(formatStr.mb_str());

    size_t sz = strftime(sBuf.data(), maxsize/sizeof(wchar_t), bufFormatStr, t);

    wxMB2WC(s, sBuf, maxsize);

    return sz;
}

} /* extern "C" */

#define is_leap(y)   (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))
#define SECONDS_IN_ONE_MINUTE      60
#define DAYS_IN_ONE_YEAR          365
#define SECONDS_IN_ONE_MIN         60
#define SECONDS_IN_ONE_HOUR      3600
#define SECONDS_IN_ONE_DAY      86400
#define DEFAULT_TIMEZONE        28800
#define DO_GMTIME                   0
#define DO_LOCALTIME                1


long timezone ; // global variable


////////////////////////////////////////////////////////////////////////
// Common code for localtime and gmtime (static)
////////////////////////////////////////////////////////////////////////

static struct tm * __cdecl common_localtime(const time_t *t, BOOL bLocal)
{
    wxLongLong i64;
    FILETIME   ft;
    wxString str ;
    SYSTEMTIME SystemTime;
    TIME_ZONE_INFORMATION pTz;
    static struct tm st_res ;             // data holder
    static struct tm * res = &st_res ;    // data pointer
    int iLeap;
    const unsigned short int __mon_yday[2][13] =
    {
        // Normal years
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        // Leap years
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    if (!*t)
        ::GetLocalTime(&SystemTime);
    else
    {
        i64 = *t;
        i64 = i64 * 10000000 + 116444736000000000;

        ft.dwLowDateTime  = i64.GetLo();
        ft.dwHighDateTime = i64.GetHi();

        ::FileTimeToSystemTime(&ft, &SystemTime);
    }

    ::GetTimeZoneInformation(&pTz);

    ///////////////////////////////////////////////
    // Set timezone
    timezone = pTz.Bias * SECONDS_IN_ONE_MINUTE ;
    ///////////////////////////////////////////////

    iLeap = is_leap(SystemTime.wYear) ;

    res->tm_hour = SystemTime.wHour;
    res->tm_min  = SystemTime.wMinute;
    res->tm_sec  = SystemTime.wSecond;

    res->tm_mday = SystemTime.wDay;
    res->tm_mon = SystemTime.wMonth - 1; // this the correct month but localtime returns month aligned to zero
    res->tm_year = SystemTime.wYear;     // this the correct year
    res->tm_year = res->tm_year - 1900;  // but localtime returns the value starting at the 1900

    res->tm_wday = SystemTime.wDayOfWeek;
    res->tm_yday = __mon_yday[iLeap][res->tm_mon] + SystemTime.wDay - 1; // localtime returns year-day aligned to zero

    // if localtime behavior and daylight saving
    if (bLocal && pTz.DaylightBias != 0)
        res->tm_isdst = 1;
    else
        res->tm_isdst = 0; // without daylight saving or gmtime

    return res;
}

extern "C"
{

////////////////////////////////////////////////////////////////////////
// Receive the number of seconds elapsed since midnight(00:00:00)
// and convert a time value and corrects for the local time zone
////////////////////////////////////////////////////////////////////////
struct tm * __cdecl localtime(const time_t * t)
{
    return common_localtime(t, DO_LOCALTIME) ;
}

////////////////////////////////////////////////////////////////////////
// Receives the number of seconds elapsed since midnight(00:00:00)
// and converts a time value WITHOUT correcting for the local time zone
////////////////////////////////////////////////////////////////////////
struct tm * __cdecl gmtime(const time_t *t)
{
    return common_localtime(t, DO_GMTIME) ;
}

}

////////////////////////////////////////////////////////////////////////
// Common code for conversion of struct tm into time_t   (static)
////////////////////////////////////////////////////////////////////////
static time_t __cdecl common_tm_to_time(int day, int month, int year, int hour, int minute, int second)
{
#if 1
    // Use mktime since it seems less broken
    tm t;
    t.tm_isdst = -1;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_min = minute;
    t.tm_mon = month-1;
    t.tm_sec = second;
    t.tm_wday = -1;
    t.tm_yday = -1;
    t.tm_year = year - 1900;
    return mktime(& t);
#else
    time_t prog = 0 ;
    static int mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 } ;

    while (--month)
    {
        prog += mdays[month - 1] ;
        if (month == 2 && is_leap(year))
            prog++ ;
    }

    // Calculate seconds in elapsed days
    prog = day - 1 ;        // align first day of the year to zero
    prog += (DAYS_IN_ONE_YEAR * (year - 1970) + (year - 1901) / 4 - 19) ;
    prog *= SECONDS_IN_ONE_DAY ;

    // Add Calculated elapsed seconds in the current day
    prog += (hour * SECONDS_IN_ONE_HOUR + minute *
                               SECONDS_IN_ONE_MIN + second) ;

    return prog ;
#endif
}

extern "C"
{

////////////////////////////////////////////////////////////////////////
// Returns the number of seconds elapsed since
// midnight(00:00:00) of 1 January 1970
////////////////////////////////////////////////////////////////////////
time_t __cdecl time(time_t *t)
{
    time_t prog = 0 ;

    if (t != NULL)
    {
        SYSTEMTIME SystemTime;

        ::GetLocalTime(&SystemTime) ;
        prog = common_tm_to_time(SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear,
                                 SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond) ;
        *t = prog ;
    }

    return prog ;
}

////////////////////////////////////////////////////////////////////////
// Converts the local time provided by struct tm
// into a time_t calendar value
// Implementation from OpenBSD
////////////////////////////////////////////////////////////////////////

#if 1
int month_to_day[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

time_t mktime(struct tm *t)
{
        short  month, year;
        time_t result;

        month = t->tm_mon;
        year = t->tm_year + month / 12 + 1900;
        month %= 12;
        if (month < 0)
        {
                year -= 1;
                month += 12;
        }
        result = (year - 1970) * 365 + (year - 1969) / 4 + month_to_day[month];
        result = (year - 1970) * 365 + month_to_day[month];
        if (month <= 1)
                year -= 1;
        result += (year - 1968) / 4;
        result -= (year - 1900) / 100;
        result += (year - 1600) / 400;
        result += t->tm_mday;
        result -= 1;
        result *= 24;
        result += t->tm_hour;
        result *= 60;
        result += t->tm_min;
        result *= 60;
        result += t->tm_sec;
        return(result);
}

#else
time_t __cdecl mktime(struct tm *t)
{
    return (common_tm_to_time(t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec)) ;
}
#endif

} // extern "C"

#endif // VC8/!VC8
