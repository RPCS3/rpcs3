#include "basic_keyboard_handler.h"

#include <QKeyEvent>

void basic_keyboard_handler::Init(const u32 max_connect)
{
	for (u32 i = 0; i<max_connect; i++)
	{
		m_keyboards.emplace_back(Keyboard());
	}

	LoadSettings();
	memset(&m_info, 0, sizeof(KbInfo));
	m_info.max_connect = max_connect;
	m_info.now_connect = std::min<size_t>(m_keyboards.size(), max_connect);
	m_info.info = 0; // Ownership of keyboard data: 0=Application, 1=System
	m_info.status[0] = CELL_KB_STATUS_CONNECTED; // (TODO: Support for more keyboards)
}

basic_keyboard_handler::basic_keyboard_handler(QObject* target, QObject* parent) : QObject(parent), m_target(target)
{
	// Adds event filter to the target to filter keyevents.
	target->installEventFilter(this);
}

bool basic_keyboard_handler::eventFilter(QObject* target, QEvent* ev)
{
	// Commenting target since I don't know how to target game window yet.
	//if (target == m_target)
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
	Key(keyEvent->key(), 1);
}

void basic_keyboard_handler::keyReleaseEvent(QKeyEvent* keyEvent)
{
	Key(keyEvent->key(), 0);
}

void basic_keyboard_handler::LoadSettings()
{
	// Meta Keys
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Control, CELL_KB_MKEY_L_CTRL);
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
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_106_KANJI);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_CapsLock, CELL_KEYC_CAPS_LOCK);
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
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_HOME);
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
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_KANA);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_HENKAN);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_MUHENKAN);

	// CELL_KB_KEYPAD
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_NumLock, CELL_KEYC_KPAD_NUMLOCK);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_division, CELL_KEYC_KPAD_SLASH);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_multiply, CELL_KEYC_KPAD_ASTERISK);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_KPAD_MINUS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Plus, CELL_KEYC_KPAD_PLUS);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Enter, CELL_KEYC_KPAD_ENTER);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_1, CELL_KEYC_KPAD_1);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_2, CELL_KEYC_KPAD_2);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_3, CELL_KEYC_KPAD_3);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_4, CELL_KEYC_KPAD_4);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_5, CELL_KEYC_KPAD_5);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_6, CELL_KEYC_KPAD_6);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_7, CELL_KEYC_KPAD_7);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_8, CELL_KEYC_KPAD_8);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_9, CELL_KEYC_KPAD_9);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_0, CELL_KEYC_KPAD_0);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Delete, CELL_KEYC_KPAD_PERIOD);

	// ASCII Printable characters
	m_keyboards[0].m_buttons.emplace_back('A', CELL_KEYC_A);
	m_keyboards[0].m_buttons.emplace_back('B', CELL_KEYC_B);
	m_keyboards[0].m_buttons.emplace_back('C', CELL_KEYC_C);
	m_keyboards[0].m_buttons.emplace_back('D', CELL_KEYC_D);
	m_keyboards[0].m_buttons.emplace_back('E', CELL_KEYC_E);
	m_keyboards[0].m_buttons.emplace_back('F', CELL_KEYC_F);
	m_keyboards[0].m_buttons.emplace_back('G', CELL_KEYC_G);
	m_keyboards[0].m_buttons.emplace_back('H', CELL_KEYC_H);
	m_keyboards[0].m_buttons.emplace_back('I', CELL_KEYC_I);
	m_keyboards[0].m_buttons.emplace_back('J', CELL_KEYC_J);
	m_keyboards[0].m_buttons.emplace_back('K', CELL_KEYC_K);
	m_keyboards[0].m_buttons.emplace_back('L', CELL_KEYC_L);
	m_keyboards[0].m_buttons.emplace_back('M', CELL_KEYC_M);
	m_keyboards[0].m_buttons.emplace_back('N', CELL_KEYC_N);
	m_keyboards[0].m_buttons.emplace_back('O', CELL_KEYC_O);
	m_keyboards[0].m_buttons.emplace_back('P', CELL_KEYC_P);
	m_keyboards[0].m_buttons.emplace_back('Q', CELL_KEYC_Q);
	m_keyboards[0].m_buttons.emplace_back('R', CELL_KEYC_R);
	m_keyboards[0].m_buttons.emplace_back('S', CELL_KEYC_S);
	m_keyboards[0].m_buttons.emplace_back('T', CELL_KEYC_T);
	m_keyboards[0].m_buttons.emplace_back('U', CELL_KEYC_U);
	m_keyboards[0].m_buttons.emplace_back('V', CELL_KEYC_V);
	m_keyboards[0].m_buttons.emplace_back('W', CELL_KEYC_W);
	m_keyboards[0].m_buttons.emplace_back('X', CELL_KEYC_X);
	m_keyboards[0].m_buttons.emplace_back('Y', CELL_KEYC_Y);
	m_keyboards[0].m_buttons.emplace_back('Z', CELL_KEYC_Z);

	m_keyboards[0].m_buttons.emplace_back('1', CELL_KEYC_1);
	m_keyboards[0].m_buttons.emplace_back('2', CELL_KEYC_2);
	m_keyboards[0].m_buttons.emplace_back('3', CELL_KEYC_3);
	m_keyboards[0].m_buttons.emplace_back('4', CELL_KEYC_4);
	m_keyboards[0].m_buttons.emplace_back('5', CELL_KEYC_5);
	m_keyboards[0].m_buttons.emplace_back('6', CELL_KEYC_6);
	m_keyboards[0].m_buttons.emplace_back('7', CELL_KEYC_7);
	m_keyboards[0].m_buttons.emplace_back('8', CELL_KEYC_8);
	m_keyboards[0].m_buttons.emplace_back('9', CELL_KEYC_9);
	m_keyboards[0].m_buttons.emplace_back('0', CELL_KEYC_0);

	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Return, CELL_KEYC_ENTER);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_ESC);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Tab, CELL_KEYC_TAB);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Space, CELL_KEYC_SPACE);
	m_keyboards[0].m_buttons.emplace_back(Qt::Key_Minus, CELL_KEYC_MINUS);
	m_keyboards[0].m_buttons.emplace_back('=', CELL_KEYC_EQUAL_101);
	m_keyboards[0].m_buttons.emplace_back('^', CELL_KEYC_ACCENT_CIRCONFLEX_106);
	//m_keyboards[0].m_buttons.emplace_back('(', CELL_KEYC_LEFT_BRACKET_101);
	m_keyboards[0].m_buttons.emplace_back('@', CELL_KEYC_ATMARK_106);
	//m_keyboards[0].m_buttons.emplace_back(')', CELL_KEYC_RIGHT_BRACKET_101);
	m_keyboards[0].m_buttons.emplace_back('(', CELL_KEYC_LEFT_BRACKET_106);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_BACKSLASH_101);
	m_keyboards[0].m_buttons.emplace_back('(', CELL_KEYC_RIGHT_BRACKET_106);
	m_keyboards[0].m_buttons.emplace_back(';', CELL_KEYC_SEMICOLON);
	m_keyboards[0].m_buttons.emplace_back('"', CELL_KEYC_QUOTATION_101);
	m_keyboards[0].m_buttons.emplace_back(':', CELL_KEYC_COLON_106);
	m_keyboards[0].m_buttons.emplace_back(',', CELL_KEYC_COMMA);
	m_keyboards[0].m_buttons.emplace_back('.', CELL_KEYC_PERIOD);
	m_keyboards[0].m_buttons.emplace_back('/', CELL_KEYC_SLASH);
	m_keyboards[0].m_buttons.emplace_back('\\', CELL_KEYC_BACKSLASH_106);
	//m_keyboards[0].m_buttons.emplace_back(, CELL_KEYC_YEN_106);
}
