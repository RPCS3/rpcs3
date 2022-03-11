#pragma once

#include "util/types.hpp"

struct perf_monitor
{
	void operator()();
	~perf_monitor();

	static constexpr auto thread_name = "Performance Sensor"sv;
};
