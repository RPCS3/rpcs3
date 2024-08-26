#pragma once

#include "Emu/Io/KeyboardHandler.h"

class NullKeyboardHandler final : public KeyboardHandlerBase
{
	using KeyboardHandlerBase::KeyboardHandlerBase;

public:
	void Init(keyboard_consumer& consumer, const u32 max_connect) override
	{
		KbInfo& info = consumer.GetInfo();
		std::vector<Keyboard>& keyboards = consumer.GetKeyboards();

		info = {};
		info.max_connect = max_connect;
		info.is_null_handler = true;
		keyboards.clear();
		for (u32 i = 0; i < max_connect; i++)
		{
			keyboards.emplace_back(Keyboard());
		}
	}
};
