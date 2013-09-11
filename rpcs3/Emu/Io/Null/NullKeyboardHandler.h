#pragma once

#include "Emu/Io/KeyboardHandler.h"

class NullKeyboardHandler : public KeyboardHandlerBase
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
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(KbInfo));
		m_keyboards.Clear();
	}
};