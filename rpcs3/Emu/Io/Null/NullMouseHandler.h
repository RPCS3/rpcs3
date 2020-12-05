#pragma once

#include "Emu/Io/MouseHandler.h"

class NullMouseHandler final : public MouseHandlerBase
{
public:
	void Init(const u32 max_connect) override
	{
		m_info = {};
		m_info.max_connect = max_connect;
		m_mice.clear();
	}
};
