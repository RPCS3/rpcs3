#pragma once

#include <ctime>
#include <string>

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

	static inline std::string fmt_time(const char* fmt, const s64 time)
	{
		tm buf;
		time_t t = time;
#ifdef _MSC_VER
		localtime_s(&buf, &t);
#else
		buf = *localtime(&t);
#endif
		char str[80];
		strftime(str, sizeof(str), fmt, &buf);
		return str;
	}

	static inline std::string current_time()
	{
		char str[80];
		tm now = get_time(nullptr);
		strftime(str, sizeof(str), "%c", &now);
		return str;
	}

	template<char separator = 0>
	static inline std::string current_time_narrow()
	{
		char str[80];
		tm now = get_time(nullptr);

		std::string parse_buf;

		if constexpr(separator != 0)
			parse_buf = std::string("%Y") + separator + "%m" + separator + "%d" + separator + "%H" + separator + "%M" + separator + "%S";
		else
			parse_buf = "%Y%m%d%H%M%S";

		strftime(str, sizeof(str), parse_buf.c_str(), &now);
		return str;
	}
}
