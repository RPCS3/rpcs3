#pragma once

#include "Emu/Io/KeyboardHandler.h"

class NullKeyboardHandler final : public KeyboardHandlerBase
{
public:
	NullKeyboardHandler()
	{
	}

	virtual void Init(const u32 max_connect)
	{
		memset(&m_info, 0, sizeof(KbInfo));
		m_info.max_connect = max_connect;
		m_keyboards.clear();
		for(u32 i=0; i<max_connect; i++)
		{
			m_keyboards.emplace_back(Keyboard());
		}
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(KbInfo));
		m_keyboards.clear();
	}
};