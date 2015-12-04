#pragma once

#include "Emu/Io/MouseHandler.h"

class NullMouseHandler final : public MouseHandlerBase
{
public:
	NullMouseHandler()
	{
	}

	virtual void Init(const u32 max_connect)
	{
		memset(&m_info, 0, sizeof(MouseInfo));
		m_info.max_connect = max_connect;
		m_mice.clear();
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(MouseInfo));
		m_mice.clear();
	}
};