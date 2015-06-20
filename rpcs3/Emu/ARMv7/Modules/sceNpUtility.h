#pragma once

#include "sceNpCommon.h"

struct SceNpBandwidthTestResult
{
	le_t<double> uploadBps;
	le_t<double> downloadBps;
	le_t<s32> result;
	char padding[4];
};

extern psv_log_base sceNpUtility;
