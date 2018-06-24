#pragma once

#include <stdafx.h>

const std::string get_current_date_time()
{
	time_t now = time(0);
	tm* tstruct = localtime(&now);
	char buf[80];
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", tstruct);
	delete tstruct;
	return buf;
}
