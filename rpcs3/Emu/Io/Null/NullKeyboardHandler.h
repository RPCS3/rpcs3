#pragma once

#include "Emu/Io/KeyboardHandler.h"

class NullKeyboardHandler final : public KeyboardHandlerBase
{
public:
	void Init(const u32 max_connect) override
	{
		m_info = {};
		m_info.max_connect = max_connect;
		m_keyboards.clear();
		for (u32 i = 0; i < max_connect; i++)
		{
			m_keyboards.emplace_back(Keyboard());
		}
	}
};
