#pragma once

namespace date_time
{
	static inline tm get_time(time_t* _time)
	{
		tm buf;
		time_t t = time(_time);
#ifdef _MSC_VER
		localtime_s(&buf, &t);
#else
		buf = *localtime(&t);
#endif
		return buf;
	}

	static inline std::string current_time()
	{
		char str[80];
		tm now = get_time(0);
		strftime(str, sizeof(str), "%c", &now);
		return str;
	}

	static inline std::string current_time_narrow()
	{
		char str[80];
		tm now = get_time(0);
		strftime(str, sizeof(str), "%Y%m%d%H%M%S", &now);
		return str;
	}
}
