#pragma once

extern std::string rDefaultDateTimeFormat;


struct rTimeSpan
{
	rTimeSpan();
	~rTimeSpan();
	rTimeSpan(const rTimeSpan& other);
	rTimeSpan(int, int, int, int);

	void *handle;
};

struct rDateSpan
{
	rDateSpan();
	~rDateSpan();
	rDateSpan(const rDateSpan& other);
	rDateSpan(int, int, int, int);

	void *handle;
};

struct rDateTime
{
	enum TZ
	{
		Local, GMT0,UTC
	};
	enum Calender
	{
		Gregorian, Julian
	};

	using rTimeZone = TZ;

	enum WeekDay
	{
		Sun = 0,
		Mon,
		Tue,
		Wed,
		Thu,
		Fri,
		Sat,
		Inv_WeekDay
	};

	enum Month {
		Jan = 0,
		Feb = 1,
		Mar = 2,
		Apr = 3,
		May = 4,
		Jun = 5,
		Jul = 6,
		Aug = 7,
		Sep = 8,
		Oct = 9,
		Nov = 10,
		Dec = 11,
		Inv_Month = 12
	};

	rDateTime();
	~rDateTime();
	rDateTime(const rDateTime& other);
	rDateTime(const time_t &time);
	rDateTime(u16 day, rDateTime::Month month, u16 year, u16 hour, u16 minute, u16 second, u32 millisecond);

	static rDateTime UNow();
	rDateTime FromUTC(bool val);
	rDateTime ToUTC(bool val);
	time_t GetTicks();
	void Add(const rTimeSpan& span);
	void Add(const rDateSpan& span);
	void Close();
	std::string Format(const std::string &format = rDefaultDateTimeFormat, const rTimeZone &tz = Local) const;

	void ParseDateTime(const char* format);

	u32 GetAsDOS();
	rDateTime &SetFromDOS(u32 fromdos);

	static bool IsLeapYear(int year, rDateTime::Calender cal);
	static int GetNumberOfDays(rDateTime::Month month, int year, rDateTime::Calender cal);
	void SetToWeekDay(rDateTime::WeekDay day, int n, rDateTime::Month month, int year);
	int GetWeekDay();
	
	u16 GetYear( rDateTime::TZ timezone);
	u16 GetMonth(rDateTime::TZ timezone);
	u16 GetDay(rDateTime::TZ timezone);
	u16 GetHour(rDateTime::TZ timezone);
	u16 GetMinute(rDateTime::TZ timezone);
	u16 GetSecond(rDateTime::TZ timezone);
	u32 GetMillisecond(rDateTime::TZ timezone);

	void *handle;
};

