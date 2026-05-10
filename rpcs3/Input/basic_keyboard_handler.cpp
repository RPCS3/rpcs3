#include "basic_keyboard_handler.h"

#include <QApplication>

#include "Emu/system_config.h"
#include "Emu/Io/interception.h"

#ifdef _WIN32
#include "windows.h"
#endif

LOG_CHANNEL(input_log, "Input");

void basic_keyboard_handler::Init(keyboard_consumer& consumer, const u32 max_connect)
{
	KbInfo& info = consumer.GetInfo();
	std::vector<Keyboard>& keyboards = consumer.GetKeyboards();

	info = {};
	keyboards.clear();

	for (u32 i = 0; i < max_connect; i++)
	{
		Keyboard kb{};
		kb.m_config.arrange = g_cfg.sys.keyboard_type;

		if (consumer.id() == keyboard_consumer::identifier::overlays)
		{
			// Enable key repeat
			kb.m_key_repeat = true;
		}

		LoadSettings(kb);

		keyboards.emplace_back(kb);
	}

	info.max_connect = max_connect;
	info.now_connect = std::min(::size32(keyboards), max_connect);
	info.info        = input::g_keyboards_intercepted ? CELL_KB_INFO_INTERCEPTED : 0; // Ownership of keyboard data: 0=Application, 1=System
	info.status[0]   = CELL_KB_STATUS_CONNECTED; // (TODO: Support for more keyboards)
}

/* Sets the target window for the event handler, and also installs an event filter on the target. */
void basic_keyboard_handler::SetTargetWindow(QWindow* target)
{
	if (target != nullptr)
	{
		m_target = target;
		target->installEventFilter(this);
	}
	else
	{
		// If this is hit, it probably means that some refactoring occurs because currently a gsframe is created in Load.
		// We still want events so filter from application instead since target is null.
		QApplication::instance()->installEventFilter(this);
		input_log.error("Trying to set keyboard handler to a null target window.");
	}
}

bool basic_keyboard_handler::eventFilter(QObject* watched, QEvent* event)
{
	if (!event) [[unlikely]]
	{
		return false;
	}

	if (input::g_active_mouse_and_keyboard != input::active_mouse_and_keyboard::emulated)
	{
		if (!m_keys_released)
		{
			ReleaseAllKeys();
		}
		return false;
	}

	m_keys_released = false;

	// !m_target is for future proofing when gsrender isn't automatically initialized on load.
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible() || watched == m_target)
	{
		switch (event->type())
		{
		case QEvent::KeyPress:
		{
			keyPressEvent(static_cast<QKeyEvent*>(event));
			break;
		}
		case QEvent::KeyRelease:
		{
			keyReleaseEvent(static_cast<QKeyEvent*>(event));
			break;
		}
		case QEvent::FocusOut:
		{
			ReleaseAllKeys();
			break;
		}
		default:
		{
			break;
		}
		}
	}
	return false;
}

void basic_keyboard_handler::keyPressEvent(QKeyEvent* keyEvent)
{
	if (!keyEvent) [[unlikely]]
	{
		return;
	}

	if (m_consumers.empty())
	{
		keyEvent->ignore();
		return;
	}

	const int key = getUnmodifiedKey(keyEvent);

	if (key < 0 || !HandleKey(static_cast<u32>(key), keyEvent->nativeScanCode(), true, keyEvent->isAutoRepeat(), keyEvent->text().toStdU32String()))
	{
		keyEvent->ignore();
	}
}

void basic_keyboard_handler::keyReleaseEvent(QKeyEvent* keyEvent)
{
	if (!keyEvent) [[unlikely]]
	{
		return;
	}

	if (m_consumers.empty())
	{
		keyEvent->ignore();
		return;
	}

	const int key = getUnmodifiedKey(keyEvent);

	if (key < 0 || !HandleKey(static_cast<u32>(key), keyEvent->nativeScanCode(), false, keyEvent->isAutoRepeat(), keyEvent->text().toStdU32String()))
	{
		keyEvent->ignore();
	}
}

// This should get the actual unmodified key without getting too crazy.
// key() only shows the modifiers and the modified key (e.g. no easy way of knowing that - was pressed in 'SHIFT+-' in order to get _)
s32 basic_keyboard_handler::getUnmodifiedKey(QKeyEvent* keyEvent)
{
	if (!keyEvent) [[unlikely]]
	{
		return -1;
	}

	const int key = keyEvent->key();

	if (key < 0)
	{
		return key;
	}

	u32 raw_key = static_cast<u32>(key);

#ifdef _WIN32
	if (keyEvent->modifiers() != Qt::NoModifier && !keyEvent->text().isEmpty())
	{
		u32 mapped_key = static_cast<u32>(MapVirtualKeyA(static_cast<UINT>(keyEvent->nativeVirtualKey()), MAPVK_VK_TO_CHAR));

		if (raw_key != mapped_key)
		{
			if (mapped_key > 0x80000000) // diacritics
			{
				mapped_key -= 0x80000000;
			}
			raw_key = mapped_key;
		}
	}
#endif

	return static_cast<s32>(raw_key);
}

void basic_keyboard_handler::LoadSettings(Keyboard& keyboard)
{
	std::vector<KbButton> buttons;

	// Meta Keys
	buttons.emplace_back(Qt::Key_Control, CELL_KB_MKEY_L_CTRL);
	buttons.emplace_back(Qt::Key_Shift, CELL_KB_MKEY_L_SHIFT);
	buttons.emplace_back(Qt::Key_Alt, CELL_KB_MKEY_L_ALT);
	buttons.emplace_back(Qt::Key_Meta, CELL_KB_MKEY_L_WIN);
	//buttons.emplace_back(, CELL_KB_MKEY_R_CTRL);  // There is no way to know if it's left or right in Qt at the moment
	//buttons.emplace_back(, CELL_KB_MKEY_R_SHIFT); // There is no way to know if it's left or right in Qt at the moment
	//buttons.emplace_back(, CELL_KB_MKEY_R_ALT);   // There is no way to know if it's left or right in Qt at the moment
	//buttons.emplace_back(, CELL_KB_MKEY_R_WIN);   // There is no way to know if it's left or right in Qt at the moment

	buttons.emplace_back(Qt::Key_Super_L, CELL_KB_MKEY_L_WIN); // The super keys are supposed to be the windows keys, but they trigger the meta key instead. Let's assign the windows keys to both.
	buttons.emplace_back(Qt::Key_Super_R, CELL_KB_MKEY_R_WIN); // The super keys are supposed to be the windows keys, but they trigger the meta key instead. Let's assign the windows keys to both.

	// CELL_KB_RAWDAT
	//buttons.emplace_back(, CELL_KEYC_NO_EVENT); // Redundant, listed for completeness
	//buttons.emplace_back(, CELL_KEYC_E_ROLLOVER);
	//buttons.emplace_back(, CELL_KEYC_E_POSTFAIL);
	//buttons.emplace_back(, CELL_KEYC_E_UNDEF);
	buttons.emplace_back(Qt::Key_Escape, CELL_KEYC_ESCAPE);
	buttons.emplace_back(Qt::Key_Kanji, CELL_KEYC_106_KANJI);
	buttons.emplace_back(Qt::Key_CapsLock, CELL_KEYC_CAPS_LOCK);
	buttons.emplace_back(Qt::Key_F1, CELL_KEYC_F1);
	buttons.emplace_back(Qt::Key_F2, CELL_KEYC_F2);
	buttons.emplace_back(Qt::Key_F3, CELL_KEYC_F3);
	buttons.emplace_back(Qt::Key_F4, CELL_KEYC_F4);
	buttons.emplace_back(Qt::Key_F5, CELL_KEYC_F5);
	buttons.emplace_back(Qt::Key_F6, CELL_KEYC_F6);
	buttons.emplace_back(Qt::Key_F7, CELL_KEYC_F7);
	buttons.emplace_back(Qt::Key_F8, CELL_KEYC_F8);
	buttons.emplace_back(Qt::Key_F9, CELL_KEYC_F9);
	buttons.emplace_back(Qt::Key_F10, CELL_KEYC_F10);
	buttons.emplace_back(Qt::Key_F11, CELL_KEYC_F11);
	buttons.emplace_back(Qt::Key_F12, CELL_KEYC_F12);
	buttons.emplace_back(Qt::Key_Print, CELL_KEYC_PRINTSCREEN);
	buttons.emplace_back(Qt::Key_ScrollLock, CELL_KEYC_SCROLL_LOCK);
	buttons.emplace_back(Qt::Key_Pause, CELL_KEYC_PAUSE);
	buttons.emplace_back(Qt::Key_Insert, CELL_KEYC_INSERT);
	buttons.emplace_back(Qt::Key_Home, CELL_KEYC_HOME);
	buttons.emplace_back(Qt::Key_PageUp, CELL_KEYC_PAGE_UP);
	buttons.emplace_back(Qt::Key_Delete, CELL_KEYC_DELETE);
	buttons.emplace_back(Qt::Key_End, CELL_KEYC_END);
	buttons.emplace_back(Qt::Key_PageDown, CELL_KEYC_PAGE_DOWN);
	buttons.emplace_back(Qt::Key_Right, CELL_KEYC_RIGHT_ARROW);
	buttons.emplace_back(Qt::Key_Left, CELL_KEYC_LEFT_ARROW);
	buttons.emplace_back(Qt::Key_Down, CELL_KEYC_DOWN_ARROW);
	buttons.emplace_back(Qt::Key_Up, CELL_KEYC_UP_ARROW);
	//buttons.emplace_back(, CELL_KEYC_NUM_LOCK);
	//buttons.emplace_back(, CELL_KEYC_APPLICATION); // This is probably the PS key on the PS3 keyboard
	buttons.emplace_back(Qt::Key_Kana_Shift, CELL_KEYC_KANA); // maybe Key_Kana_Lock
	buttons.emplace_back(Qt::Key_Henkan, CELL_KEYC_HENKAN);
	buttons.emplace_back(Qt::Key_Muhenkan, CELL_KEYC_MUHENKAN);

	// CELL_KB_KEYPAD
	buttons.emplace_back(Qt::Key_NumLock, CELL_KEYC_KPAD_NUMLOCK);
	buttons.emplace_back(Qt::Key_division, CELL_KEYC_KPAD_SLASH);    // should ideally be slash but that's occupied obviously
	buttons.emplace_back(Qt::Key_multiply, CELL_KEYC_KPAD_ASTERISK); // should ideally be asterisk but that's occupied obviously
	//buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_KPAD_MINUS);     // should ideally be minus but that's occupied obviously
	buttons.emplace_back(Qt::Key_Plus, CELL_KEYC_KPAD_PLUS);
	buttons.emplace_back(Qt::Key_Enter, CELL_KEYC_KPAD_ENTER);
	//buttons.emplace_back(Qt::Key_1, CELL_KEYC_KPAD_1);
	//buttons.emplace_back(Qt::Key_2, CELL_KEYC_KPAD_2);
	//buttons.emplace_back(Qt::Key_3, CELL_KEYC_KPAD_3);
	//buttons.emplace_back(Qt::Key_4, CELL_KEYC_KPAD_4);
	//buttons.emplace_back(Qt::Key_5, CELL_KEYC_KPAD_5);
	//buttons.emplace_back(Qt::Key_6, CELL_KEYC_KPAD_6);
	//buttons.emplace_back(Qt::Key_7, CELL_KEYC_KPAD_7);
	//buttons.emplace_back(Qt::Key_8, CELL_KEYC_KPAD_8);
	//buttons.emplace_back(Qt::Key_9, CELL_KEYC_KPAD_9);
	//buttons.emplace_back(Qt::Key_0, CELL_KEYC_KPAD_0);
	//buttons.emplace_back(Qt::Key_Delete, CELL_KEYC_KPAD_PERIOD);

	// ASCII Printable characters
	buttons.emplace_back(Qt::Key_A, CELL_KEYC_A);
	buttons.emplace_back(Qt::Key_B, CELL_KEYC_B);
	buttons.emplace_back(Qt::Key_C, CELL_KEYC_C);
	buttons.emplace_back(Qt::Key_D, CELL_KEYC_D);
	buttons.emplace_back(Qt::Key_E, CELL_KEYC_E);
	buttons.emplace_back(Qt::Key_F, CELL_KEYC_F);
	buttons.emplace_back(Qt::Key_G, CELL_KEYC_G);
	buttons.emplace_back(Qt::Key_H, CELL_KEYC_H);
	buttons.emplace_back(Qt::Key_I, CELL_KEYC_I);
	buttons.emplace_back(Qt::Key_J, CELL_KEYC_J);
	buttons.emplace_back(Qt::Key_K, CELL_KEYC_K);
	buttons.emplace_back(Qt::Key_L, CELL_KEYC_L);
	buttons.emplace_back(Qt::Key_M, CELL_KEYC_M);
	buttons.emplace_back(Qt::Key_N, CELL_KEYC_N);
	buttons.emplace_back(Qt::Key_O, CELL_KEYC_O);
	buttons.emplace_back(Qt::Key_P, CELL_KEYC_P);
	buttons.emplace_back(Qt::Key_Q, CELL_KEYC_Q);
	buttons.emplace_back(Qt::Key_R, CELL_KEYC_R);
	buttons.emplace_back(Qt::Key_S, CELL_KEYC_S);
	buttons.emplace_back(Qt::Key_T, CELL_KEYC_T);
	buttons.emplace_back(Qt::Key_U, CELL_KEYC_U);
	buttons.emplace_back(Qt::Key_V, CELL_KEYC_V);
	buttons.emplace_back(Qt::Key_W, CELL_KEYC_W);
	buttons.emplace_back(Qt::Key_X, CELL_KEYC_X);
	buttons.emplace_back(Qt::Key_Y, CELL_KEYC_Y);
	buttons.emplace_back(Qt::Key_Z, CELL_KEYC_Z);

	buttons.emplace_back(Qt::Key_1, CELL_KEYC_1);
	buttons.emplace_back(Qt::Key_2, CELL_KEYC_2);
	buttons.emplace_back(Qt::Key_3, CELL_KEYC_3);
	buttons.emplace_back(Qt::Key_4, CELL_KEYC_4);
	buttons.emplace_back(Qt::Key_5, CELL_KEYC_5);
	buttons.emplace_back(Qt::Key_6, CELL_KEYC_6);
	buttons.emplace_back(Qt::Key_7, CELL_KEYC_7);
	buttons.emplace_back(Qt::Key_8, CELL_KEYC_8);
	buttons.emplace_back(Qt::Key_9, CELL_KEYC_9);
	buttons.emplace_back(Qt::Key_0, CELL_KEYC_0);

	buttons.emplace_back(Qt::Key_Return, CELL_KEYC_ENTER);
	buttons.emplace_back(Qt::Key_Backspace, CELL_KEYC_BS);
	buttons.emplace_back(Qt::Key_Tab, CELL_KEYC_TAB);
	buttons.emplace_back(Qt::Key_Space, CELL_KEYC_SPACE);
	buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_MINUS);
	buttons.emplace_back(Qt::Key_Equal, CELL_KEYC_EQUAL_101);
	buttons.emplace_back(Qt::Key_AsciiCircum, CELL_KEYC_ACCENT_CIRCONFLEX_106);
	buttons.emplace_back(Qt::Key_At, CELL_KEYC_ATMARK_106);
	buttons.emplace_back(Qt::Key_Semicolon, CELL_KEYC_SEMICOLON);
	buttons.emplace_back(Qt::Key_Apostrophe, CELL_KEYC_QUOTATION_101);
	buttons.emplace_back(Qt::Key_Colon, CELL_KEYC_COLON_106);
	buttons.emplace_back(Qt::Key_Comma, CELL_KEYC_COMMA);
	buttons.emplace_back(Qt::Key_Period, CELL_KEYC_PERIOD);
	buttons.emplace_back(Qt::Key_Slash, CELL_KEYC_SLASH);
	buttons.emplace_back(Qt::Key_yen, CELL_KEYC_YEN_106);

	// Some buttons share the same key code on different layouts
	if (keyboard.m_config.arrange == CELL_KB_MAPPING_106)
	{
		buttons.emplace_back(Qt::Key_Backslash, CELL_KEYC_BACKSLASH_106);
		buttons.emplace_back(Qt::Key_BracketLeft, CELL_KEYC_LEFT_BRACKET_106);
		buttons.emplace_back(Qt::Key_BracketRight, CELL_KEYC_RIGHT_BRACKET_106);
	}
	else
	{
		buttons.emplace_back(Qt::Key_Backslash, CELL_KEYC_BACKSLASH_101);
		buttons.emplace_back(Qt::Key_BracketLeft, CELL_KEYC_LEFT_BRACKET_101);
		buttons.emplace_back(Qt::Key_BracketRight, CELL_KEYC_RIGHT_BRACKET_101);
	}

	// Made up helper buttons
	buttons.emplace_back(Qt::Key_Less, CELL_KEYC_LESS);
	buttons.emplace_back(Qt::Key_NumberSign, CELL_KEYC_HASHTAG);
	buttons.emplace_back(Qt::Key_ssharp, CELL_KEYC_SSHARP);
	buttons.emplace_back(Qt::Key_QuoteLeft, CELL_KEYC_BACK_QUOTE);
	buttons.emplace_back(Qt::Key_acute, CELL_KEYC_BACK_QUOTE);

	// Fill our map
	for (const KbButton& button : buttons)
	{
		if (!keyboard.m_keys.try_emplace(button.m_keyCode, button).second)
		{
			input_log.error("basic_keyboard_handler failed to set key code %d", button.m_keyCode);
		}
	}
}
