#pragma once

enum
{
	SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_NONE = 0,
	SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_RUNNING = 1,
	SCE_NP_UTIL_BANDWIDTH_TEST_STATUS_FINISHED = 2
};

struct SceNpUtilBandwidthTestResult
{
	be_t<f64> upload_bps;
	be_t<f64> download_bps;
	be_t<s32> result;
	s8 padding[4];
};
