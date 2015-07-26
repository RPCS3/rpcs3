#pragma once

struct SceNpUtilInternal
{
	bool m_bSceNpUtilBandwidthTestInitialized;

	SceNpUtilInternal()
		: m_bSceNpUtilBandwidthTestInitialized(false)
	{
	}
};

extern std::unique_ptr<SceNpUtilInternal> g_sceNpUtil;