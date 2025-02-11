#pragma once

#include <string_view>
using namespace std::literals;

struct perf_monitor
{
	void operator()();
	~perf_monitor();

	static constexpr auto thread_name = "Performance Sensor"sv;
};
