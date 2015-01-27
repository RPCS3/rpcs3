#include "stdafx.h"
#include "rTime.h"
#pragma warning(disable : 4996)
#include <wx/datetime.h>

std::string rDefaultDateTimeFormat = "%c";

rTimeSpan::rTimeSpan()
{
	handle = static_cast<void *>(new wxTimeSpan());
}

rTimeSpan::~rTimeSpan()
{
	delete static_cast<wxTimeSpan*>(handle);
}

rTimeSpan::rTimeSpan(const rTimeSpan& other)

{
	handle = static_cast<void *>(new wxTimeSpan(*static_cast<wxTimeSpan*>(other.handle)));

}

rTimeSpan::rTimeSpan(int a, int b , int c, int d)
{
	handle = static_cast<void *>(new wxTimeSpan(a,b,c,d));
}


rDateSpan::rDateSpan()
{
	handle = static_cast<void *>(new wxDateSpan());
}

rDateSpan::~rDateSpan()
{
	delete static_cast<wxDateSpan*>(handle);
}

rDateSpan::rDateSpan(const rDateSpan& other)
{
	handle = static_cast<void *>(new wxDateSpan(*static_cast<wxDateSpan*>(other.handle)));
}

rDateSpan::rDateSpan(int a, int b, int c, int d)
{
	handle = static_cast<void *>(new wxDateSpan(a,b,c,d));
}

rDateTime::rDateTime()
{
	handle = static_cast<void *>(new wxDateTime());
}

rDateTime::~rDateTime()
{
	delete static_cast<wxDateTime*>(handle);
}

rDateTime::rDateTime(const rDateTime& other)
{
	handle = static_cast<void *>(new wxDateTime(*static_cast<wxDateTime*>(other.handle)));
}

rDateTime::rDateTime(const time_t& time)
{
	handle = static_cast<void *>(new wxDateTime(time));
}

rDateTime::rDateTime(u16 day, rDateTime::Month month, u16 year, u16 hour, u16 minute, u16 second, u32 millisecond)
{
	handle = static_cast<void *>(new wxDateTime(day,(wxDateTime::Month)month,year,hour,minute,second,millisecond));
}

rDateTime rDateTime::UNow()
{
	rDateTime time;
	delete static_cast<wxDateTime*>(time.handle);
	time.handle = static_cast<void *>(new wxDateTime(wxDateTime::UNow()));

	return time;
}

rDateTime rDateTime::FromUTC(bool val) 
{
	rDateTime time(*this);
	void *temp = time.handle;
	
	time.handle = static_cast<void *>(new wxDateTime(static_cast<wxDateTime*>(temp)->FromTimezone(wxDateTime::GMT0, val)));
	delete static_cast<wxDateTime*>(temp);

	return time;
}

rDateTime rDateTime::ToUTC(bool val)
{
	rDateTime time(*this);
	void *temp = time.handle;

	time.handle = static_cast<void *>(new wxDateTime(static_cast<wxDateTime*>(temp)->ToTimezone(wxDateTime::GMT0, val)));
	delete static_cast<wxDateTime*>(temp);

	return time;
}

time_t rDateTime::GetTicks()
{
	return static_cast<wxDateTime*>(handle)->GetTicks();
}

void rDateTime::Add(const rTimeSpan& span)
{
	static_cast<wxDateTime*>(handle)->Add(*static_cast<wxTimeSpan*>(span.handle));
}

void rDateTime::Add(const rDateSpan& span)
{
	static_cast<wxDateTime*>(handle)->Add(*static_cast<wxDateSpan*>(span.handle));
}

wxDateTime::TimeZone convertTZ(rDateTime::rTimeZone tz)
{
	switch (tz)
	{
	case rDateTime::Local:
		return wxDateTime::Local;
	case rDateTime::GMT0:
		return wxDateTime::GMT0;
	case rDateTime::UTC:
		return wxDateTime::UTC;
	default:
		throw std::string("WRONG DATETIME");
	}
}

std::string rDateTime::Format(const std::string &format, const rTimeZone &tz) const
{
	return fmt::ToUTF8(static_cast<wxDateTime*>(handle)->Format(fmt::FromUTF8(format),convertTZ(tz)));
}

void rDateTime::ParseDateTime(const char* format)
{
	static_cast<wxDateTime*>(handle)->ParseDateTime(format);
}

u32 rDateTime::GetAsDOS()
{
	return static_cast<wxDateTime*>(handle)->GetAsDOS();
}

rDateTime &rDateTime::SetFromDOS(u32 fromdos)
{
	static_cast<wxDateTime*>(handle)->SetFromDOS(fromdos);
	return *this;
}

bool rDateTime::IsLeapYear(int year, rDateTime::Calender cal)
{
	if (cal == Gregorian)
	{
		return wxDateTime::IsLeapYear(year, wxDateTime::Gregorian);
	}
	else
	{
		return wxDateTime::IsLeapYear(year, wxDateTime::Julian);
	}
}

int rDateTime::GetNumberOfDays(rDateTime::Month month, int year, rDateTime::Calender cal)
{
	if (cal == Gregorian)
	{
		return wxDateTime::GetNumberOfDays(static_cast<wxDateTime::Month>(month), year, wxDateTime::Gregorian);
	}
	else
	{
		return wxDateTime::GetNumberOfDays(static_cast<wxDateTime::Month>(month), year, wxDateTime::Julian);
	}
}

void rDateTime::SetToWeekDay(rDateTime::WeekDay day, int n, rDateTime::Month month, int year)
{
	static_cast<wxDateTime*>(handle)->SetToWeekDay(
		static_cast<wxDateTime::WeekDay>(day)
		, n
		, static_cast<wxDateTime::Month>(month)
		, year
		);
}

int rDateTime::GetWeekDay()
{
	return static_cast<wxDateTime*>(handle)->GetWeekDay();
}

u16 rDateTime::GetYear(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetYear(convertTZ(timezone));
}

u16 rDateTime::GetMonth(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetMonth(convertTZ(timezone));
}

u16 rDateTime::GetDay(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetDay(convertTZ(timezone));
}

u16 rDateTime::GetHour(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetHour(convertTZ(timezone));
}

u16 rDateTime::GetMinute(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetMinute(convertTZ(timezone));
}

u16 rDateTime::GetSecond(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetSecond(convertTZ(timezone));
}

u32 rDateTime::GetMillisecond(rDateTime::TZ timezone)
{
	return static_cast<wxDateTime*>(handle)->GetMillisecond(convertTZ(timezone));
}


