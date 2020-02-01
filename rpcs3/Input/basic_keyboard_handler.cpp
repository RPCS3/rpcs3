#include "basic_keyboard_handler.h"

#include <QApplication>
#include <QKeyEvent>

#include "Emu/System.h"

#ifdef _WIN32
#include "windows.h"
#endif

LOG_CHANNEL(input_log, "Input");

void basic_keyboard_handler::Init(const u32 max_connect)
{
	for (u32 i = 0; i < max_connect; i++)
	{
		Keyboard kb = Keyboard();

		kb.m_config.arrange = g_cfg.sys.keyboard_type;

		m_keyboards.emplace_back();
	}

	LoadSettings();
	memset(&m_info, 0, sizeof(KbInfo));
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min(::size32(m_keyboards), max_connect);
	m_info.info = 0; // Ownership of keyboard data: 0=Application, 1=System
	m_info.status[0] = CELL_KB_STATUS_CONNECTED; // (TODO: Support for more keyboards)
}

basic_keyboard_handler::basic_keyboard_handler() : QObject()
{
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

bool basic_keyboard_handler::eventFilter(QObject* target, QEvent* ev)
{
	if (!ev)
	{
		return false;
	}

	// !m_target is for future proofing when gsrender isn't automatically initialized on load.
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible() || target == m_target)
	{
		if (ev->type() == QEvent::KeyPress)
		{
			keyPressEvent(static_cast<QKeyEvent*>(ev));
		}
		else if (ev->type() == QEvent::KeyRelease)
		{
			keyReleaseEvent(static_cast<QKeyEvent*>(ev));
		}
	}
	return false;
}

void basic_keyboard_handler::keyPressEvent(QKeyEvent* keyEvent)
{
	if (!keyEvent)
	{
		return;
	}

	if (m_keyboards.empty() || (keyEvent->isAutoRepeat() && !m_keyboards[0].m_key_repeat))
	{
		keyEvent->ignore();
		return;
	}

	const int key = getUnmodifiedKey(keyEvent);

	if (key >= 0)
	{
		Key(static_cast<u32>(key), true);
	}
}

void basic_keyboard_handler::keyReleaseEvent(QKeyEvent* keyEvent)
{
	if (!keyEvent)
	{
		return;
	}

	if (m_keyboards.empty() || (keyEvent->isAutoRepeat() && !m_keyboards[0].m_key_repeat))
	{
		keyEvent->ignore();
		return;
	}

	const int key = getUnmodifiedKey(keyEvent);

	if (key >= 0)
	{
		Key(static_cast<u32>(key), false);
	}
}

// This should get the actual unmodified key without getting too crazy.
// key() only shows the modifiers and the modified key (e.g. no easy way of knowing that - was pressed in 'SHIFT+-' in order to get _)
s32 basic_keyboard_handler::getUnmodifiedKey(QKeyEvent* keyEvent)
{
	if (!keyEvent)
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
		u32 mapped_key = (u32)MapVirtualKeyA((UINT)keyEvent->nativeVirtualKey(), MAPVK_VK_TO_CHAR);

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

void basic_keyboard_handler::LoadSettings()
{
	if (m_keyboards.empty())
	{
		return;
	}

	// Meta Keys
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_Control, CELL_KB_MKEY_L_CTRL);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Shift, CELL_KB_MKEY_L_SHIFT);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Alt, CELL_KB_MKEY_L_ALT);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Super_L, CELL_KB_MKEY_L_WIN);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KB_MKEY_R_CTRL);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KB_MKEY_R_SHIFT);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KB_MKEY_R_ALT);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Super_R, CELL_KB_MKEY_R_WIN);

	// CELL_KB_RAWDAT
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_NO_EVENT);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_E_ROLLOVER);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_E_POSTFAIL);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_E_UNDEF);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Escape, CELL_KEYC_ESCAPE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Kanji, CELL_KEYC_106_KANJI);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_CapsLock, CELL_KEYC_CAPS_LOCK);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F1, CELL_KEYC_F1);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F2, CELL_KEYC_F2);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F3, CELL_KEYC_F3);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F4, CELL_KEYC_F4);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F5, CELL_KEYC_F5);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F6, CELL_KEYC_F6);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F7, CELL_KEYC_F7);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F8, CELL_KEYC_F8);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F9, CELL_KEYC_F9);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F10, CELL_KEYC_F10);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F11, CELL_KEYC_F11);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F12, CELL_KEYC_F12);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Print, CELL_KEYC_PRINTSCREEN);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_ScrollLock, CELL_KEYC_SCROLL_LOCK);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Pause, CELL_KEYC_PAUSE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Insert, CELL_KEYC_INSERT);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Home, CELL_KEYC_HOME);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_PageUp, CELL_KEYC_PAGE_UP);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Delete, CELL_KEYC_DELETE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_End, CELL_KEYC_END);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_PageDown, CELL_KEYC_PAGE_DOWN);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Right, CELL_KEYC_RIGHT_ARROW);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Left, CELL_KEYC_LEFT_ARROW);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Down, CELL_KEYC_DOWN_ARROW);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Up, CELL_KEYC_UP_ARROW);
	//m_keyboards[0].m_buttons.emplace_back(WXK_NUMLOCK, CELL_KEYC_NUM_LOCK);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_APPLICATION);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Kana_Shift, CELL_KEYC_KANA); // maybe Key_Kana_Lock
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Henkan, CELL_KEYC_HENKAN);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Muhenkan, CELL_KEYC_MUHENKAN);

	// CELL_KB_KEYPAD
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_NumLock, CELL_KEYC_KPAD_NUMLOCK);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_division, CELL_KEYC_KPAD_SLASH);    // should ideally be slash but that's occupied obviously
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_multiply, CELL_KEYC_KPAD_ASTERISK); // should ideally be asterisk but that's occupied obviously
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_KPAD_MINUS);     // should ideally be minus but that's occupied obviously
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Plus, CELL_KEYC_KPAD_PLUS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Enter, CELL_KEYC_KPAD_ENTER);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_1, CELL_KEYC_KPAD_1);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_2, CELL_KEYC_KPAD_2);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_3, CELL_KEYC_KPAD_3);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_4, CELL_KEYC_KPAD_4);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_5, CELL_KEYC_KPAD_5);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_6, CELL_KEYC_KPAD_6);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_7, CELL_KEYC_KPAD_7);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_8, CELL_KEYC_KPAD_8);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_9, CELL_KEYC_KPAD_9);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_0, CELL_KEYC_KPAD_0);
	//m_keyboards[0].m_buttons.emplace_back(Qt::Key_Delete, CELL_KEYC_KPAD_PERIOD);

	// ASCII Printable characters
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_A, CELL_KEYC_A);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_B, CELL_KEYC_B);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_C, CELL_KEYC_C);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_D, CELL_KEYC_D);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_E, CELL_KEYC_E);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_F, CELL_KEYC_F);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_G, CELL_KEYC_G);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_H, CELL_KEYC_H);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_I, CELL_KEYC_I);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_J, CELL_KEYC_J);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_K, CELL_KEYC_K);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_L, CELL_KEYC_L);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_M, CELL_KEYC_M);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_N, CELL_KEYC_N);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_O, CELL_KEYC_O);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_P, CELL_KEYC_P);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Q, CELL_KEYC_Q);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_R, CELL_KEYC_R);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_S, CELL_KEYC_S);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_T, CELL_KEYC_T);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_U, CELL_KEYC_U);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_V, CELL_KEYC_V);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_W, CELL_KEYC_W);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_X, CELL_KEYC_X);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Y, CELL_KEYC_Y);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Z, CELL_KEYC_Z);

	m_keyboards[0].m_buttons.emplace_back(Qt::Key_1, CELL_KEYC_1);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_2, CELL_KEYC_2);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_3, CELL_KEYC_3);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_4, CELL_KEYC_4);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_5, CELL_KEYC_5);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_6, CELL_KEYC_6);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_7, CELL_KEYC_7);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_8, CELL_KEYC_8);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_9, CELL_KEYC_9);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_0, CELL_KEYC_0);

	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Return, CELL_KEYC_ENTER);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Backspace, CELL_KEYC_BS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Tab, CELL_KEYC_TAB);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Space, CELL_KEYC_SPACE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_MINUS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Equal, CELL_KEYC_EQUAL_101);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_AsciiCircum, CELL_KEYC_ACCENT_CIRCONFLEX_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_BracketLeft, CELL_KEYC_LEFT_BRACKET_101);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_At, CELL_KEYC_ATMARK_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_BracketRight, CELL_KEYC_RIGHT_BRACKET_101);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_BracketLeft, CELL_KEYC_LEFT_BRACKET_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Backslash, CELL_KEYC_BACKSLASH_101);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_BracketRight, CELL_KEYC_RIGHT_BRACKET_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Semicolon, CELL_KEYC_SEMICOLON);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Apostrophe, CELL_KEYC_QUOTATION_101);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Colon, CELL_KEYC_COLON_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Comma, CELL_KEYC_COMMA);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Period, CELL_KEYC_PERIOD);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Slash, CELL_KEYC_SLASH);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Backslash, CELL_KEYC_BACKSLASH_106);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_yen, CELL_KEYC_YEN_106);

	// Made up helper buttons
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Less, CELL_KEYC_LESS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_NumberSign, CELL_KEYC_HASHTAG);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_ssharp, CELL_KEYC_SSHARP);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_QuoteLeft, CELL_KEYC_BACK_QUOTE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_acute, CELL_KEYC_BACK_QUOTE);
}
