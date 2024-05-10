#include "stdafx.h"
#include "KeyboardHandler.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(input_log, "Input");

template <>
void fmt_class_string<CellKbMappingType>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellKbMappingType value)
	{
		switch (value)
		{
		case CELL_KB_MAPPING_101: return "English keyboard (US standard)";
		case CELL_KB_MAPPING_106: return "Japanese keyboard";
		case CELL_KB_MAPPING_106_KANA: return "Japanese keyboard (Kana state)";
		case CELL_KB_MAPPING_GERMAN_GERMANY: return "German keyboard";
		case CELL_KB_MAPPING_SPANISH_SPAIN: return "Spanish keyboard";
		case CELL_KB_MAPPING_FRENCH_FRANCE: return "French keyboard";
		case CELL_KB_MAPPING_ITALIAN_ITALY: return "Italian keyboard";
		case CELL_KB_MAPPING_DUTCH_NETHERLANDS: return "Dutch keyboard";
		case CELL_KB_MAPPING_PORTUGUESE_PORTUGAL: return "Portuguese keyboard (Portugal)";
		case CELL_KB_MAPPING_RUSSIAN_RUSSIA: return "Russian keyboard";
		case CELL_KB_MAPPING_ENGLISH_UK: return "English keyboard (UK standard)";
		case CELL_KB_MAPPING_KOREAN_KOREA: return "Korean keyboard";
		case CELL_KB_MAPPING_NORWEGIAN_NORWAY: return "Norwegian keyboard";
		case CELL_KB_MAPPING_FINNISH_FINLAND: return "Finnish keyboard";
		case CELL_KB_MAPPING_DANISH_DENMARK: return "Danish keyboard";
		case CELL_KB_MAPPING_SWEDISH_SWEDEN: return "Swedish keyboard";
		case CELL_KB_MAPPING_CHINESE_TRADITIONAL: return "Chinese keyboard (Traditional)";
		case CELL_KB_MAPPING_CHINESE_SIMPLIFIED: return "Chinese keyboard (Simplified)";
		case CELL_KB_MAPPING_SWISS_FRENCH_SWITZERLAND: return "French keyboard (Switzerland)";
		case CELL_KB_MAPPING_SWISS_GERMAN_SWITZERLAND: return "German keyboard (Switzerland)";
		case CELL_KB_MAPPING_CANADIAN_FRENCH_CANADA: return "French keyboard (Canada)";
		case CELL_KB_MAPPING_BELGIAN_BELGIUM: return "French keyboard (Belgium)";
		case CELL_KB_MAPPING_POLISH_POLAND: return "Polish keyboard";
		case CELL_KB_MAPPING_PORTUGUESE_BRAZIL: return "Portuguese keyboard (Brazil)";
		case CELL_KB_MAPPING_TURKISH_TURKEY: return "Turkish keyboard";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<keyboard_consumer::identifier>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](keyboard_consumer::identifier value)
	{
		switch (value)
		{
		STR_CASE(keyboard_consumer::identifier::unknown);
		STR_CASE(keyboard_consumer::identifier::overlays);
		STR_CASE(keyboard_consumer::identifier::cellKb);
		}

		return unknown;
	});
}

keyboard_consumer& KeyboardHandlerBase::AddConsumer(keyboard_consumer::identifier id, u32 max_connect)
{
	auto it = m_consumers.find(id);
	if (it == m_consumers.end())
	{
		input_log.notice("Adding keyboard consumer with id %s.", id);
		keyboard_consumer& consumer = m_consumers[id];
		consumer = keyboard_consumer(id);
		Init(consumer, max_connect);
		return consumer;
	}

	return it->second;
}

keyboard_consumer& KeyboardHandlerBase::GetConsumer(keyboard_consumer::identifier id)
{
	auto it = m_consumers.find(id);
	if (it == m_consumers.end())
	{
		fmt::throw_exception("No keyboard consumer with id %s", id);
	}

	return it->second;
}

void KeyboardHandlerBase::RemoveConsumer(keyboard_consumer::identifier id)
{
	auto it = m_consumers.find(id);
	if (it != m_consumers.end())
	{
		input_log.notice("Removing keyboard consumer with id %s.", id);
		m_consumers.erase(id);
	}
}

bool KeyboardHandlerBase::HandleKey(u32 code, bool pressed, bool is_auto_repeat, const std::u32string& key)
{
	bool consumed = false;

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& [id, consumer] : m_consumers)
	{
		consumed |= consumer.ConsumeKey(code, pressed, is_auto_repeat, key);
	}

	return consumed;
}

bool keyboard_consumer::ConsumeKey(u32 code, bool pressed, bool is_auto_repeat, const std::u32string& key)
{
	bool consumed = false;

	for (Keyboard& keyboard : m_keyboards)
	{
		if (is_auto_repeat && !keyboard.m_key_repeat)
		{
			continue;
		}

		consumed = true;

		KbData& data = keyboard.m_data;
		const KbConfig& config = keyboard.m_config;

		if (auto it = keyboard.m_keys.find(code); it != keyboard.m_keys.end())
		{
			KbButton& button = it->second;

			u16 kcode = CELL_KEYC_NO_EVENT;
			bool is_meta_key = IsMetaKey(code);

			if (!is_meta_key)
			{
				if (config.code_type == CELL_KB_CODETYPE_RAW)
				{
					kcode = button.m_outKeyCode;
				}
				else // config.code_type == CELL_KB_CODETYPE_ASCII
				{
					kcode = cellKbCnvRawCode(config.arrange, data.mkey, data.led, button.m_outKeyCode);
				}
			}

			if (pressed)
			{
				if (data.len == 1 && data.buttons[0].m_keyCode == CELL_KEYC_NO_EVENT)
				{
					data.len = 0;
				}

				// Meta Keys
				if (is_meta_key)
				{
					data.mkey |= button.m_outKeyCode;

					if (config.read_mode == CELL_KB_RMODE_INPUTCHAR)
					{
						data.buttons[0] = KbButton(CELL_KEYC_NO_EVENT, button.m_outKeyCode, true);
					}
					else
					{
						data.buttons[data.len % CELL_KB_MAX_KEYCODES] = KbButton(CELL_KEYC_NO_EVENT, button.m_outKeyCode, true);
					}
				}
				else
				{
					// Led Keys
					if (code == Key_CapsLock)   data.led ^= CELL_KB_LED_CAPS_LOCK;
					if (code == Key_NumLock)    data.led ^= CELL_KB_LED_NUM_LOCK;
					if (code == Key_ScrollLock) data.led ^= CELL_KB_LED_SCROLL_LOCK;
					// if (code == Key_Kana_Lock) data.led ^= CELL_KB_LED_KANA;
					// if (code == ???) data.led ^= CELL_KB_LED_COMPOSE;

					if (config.read_mode == CELL_KB_RMODE_INPUTCHAR)
					{
						data.buttons[0] = KbButton(kcode, button.m_outKeyCode, true);
					}
					else
					{
						data.buttons[data.len % CELL_KB_MAX_KEYCODES] = KbButton(kcode, button.m_outKeyCode, true);
					}
				}

				data.len = std::min<s32>(data.len + 1, CELL_KB_MAX_KEYCODES);
			}
			else
			{
				// Meta Keys
				if (is_meta_key)
				{
					data.mkey &= ~button.m_outKeyCode;
				}

				// Needed to indicate key releases. Without this you have to tap another key before using the same key again
				if (config.read_mode == CELL_KB_RMODE_INPUTCHAR)
				{
					data.buttons[0] = KbButton(CELL_KEYC_NO_EVENT, button.m_outKeyCode, false);
					data.len = 1;
				}
				else
				{
					s32 index = data.len;

					for (s32 i = 0; i < data.len; i++)
					{
						if (data.buttons[i].m_keyCode == kcode && (!is_meta_key || data.buttons[i].m_outKeyCode == button.m_outKeyCode))
						{
							index = i;
							break;
						}
					}

					for (s32 i = index; i < data.len - 1; i++)
					{
						data.buttons[i] = data.buttons[i + 1];
					}

					if (data.len <= 1)
					{
						data.buttons[0] = KbButton(CELL_KEYC_NO_EVENT, button.m_outKeyCode, false);
					}

					data.len = std::max(1, data.len - 1);
				}
			}
		}
		else if (!key.empty())
		{
			if (pressed)
			{
				keyboard.m_extra_data.pressed_keys.insert(key);
			}
			else
			{
				keyboard.m_extra_data.pressed_keys.erase(key);
			}
		}
	}

	return consumed;
}

bool keyboard_consumer::IsMetaKey(u32 code)
{
	return code == Key_Control
		|| code == Key_Shift
		|| code == Key_Alt
		|| code == Key_Super_L
		|| code == Key_Super_R;
}

void KeyboardHandlerBase::SetIntercepted(bool intercepted)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& [id, consumer] : m_consumers)
	{
		consumer.SetIntercepted(intercepted);
	}
}

void keyboard_consumer::SetIntercepted(bool intercepted)
{
	m_info.info = intercepted ? CELL_KB_INFO_INTERCEPTED : 0;

	if (intercepted)
	{
		for (Keyboard& keyboard : m_keyboards)
		{
			keyboard.m_data.mkey = 0;
			keyboard.m_data.len  = 0;

			for (auto& button : keyboard.m_data.buttons)
			{
				button.m_keyCode = CELL_KEYC_NO_EVENT;
			}
		}
	}
}

void KeyboardHandlerBase::ReleaseAllKeys()
{
	for (auto& [id, consumer] : m_consumers)
	{
		consumer.ReleaseAllKeys();
	}
}

void keyboard_consumer::ReleaseAllKeys()
{
	for (Keyboard& keyboard : m_keyboards)
	{
		for (const auto& [key_code, button] : keyboard.m_keys)
		{
			ConsumeKey(button.m_keyCode, false, false, {});
		}

		for (const std::u32string& key : keyboard.m_extra_data.pressed_keys)
		{
			ConsumeKey(CELL_KEYC_NO_EVENT, false, false, key);
		}

		keyboard.m_extra_data.pressed_keys.clear();
	}
}
