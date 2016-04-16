#pragma once

#include "Emu/Io/PadHandler.h"

class NullPadHandler final : public PadHandlerBase
{
public:
	void Init(const u32 max_connect) override
	{
		memset(&m_info, 0, sizeof(PadInfo));
		m_info.max_connect = max_connect;
		m_pads.clear();
	}
};
