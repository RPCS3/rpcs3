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
		m_keyboards.Clear();
		for(u32 i=0; i<max_connect; i++)
		{
			m_keyboards.Move(new Keyboard());
		}
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(KbInfo));
		m_keyboards.Clear();
	}
};