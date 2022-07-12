#pragma once

#include "Emu/Io/MouseHandler.h"

class NullMouseHandler final : public MouseHandlerBase
{
	using MouseHandlerBase::MouseHandlerBase;

public:
	void Init(const u32 max_connect) override
	{
		m_info = {};
		m_info.max_connect = max_connect;
		m_info.is_null_handler = true;
		m_mice.clear();
	}
};
