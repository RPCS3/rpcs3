#include "stdafx.h"
#include "date_time.h"

#include <chrono>

template <>
void fmt_class_string<std::chrono::sys_time<typename std::chrono::system_clock::duration>>::format(std::string& out, u64 arg)
{
	const std::time_t dateTime = std::chrono::system_clock::to_time_t(get_object(arg));
	out += date_time::fmt_time("%Y-%m-%dT%H:%M:%S", dateTime);
}
