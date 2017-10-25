#include "stdafx.h"

#include "keyboard_pad_handler.h"

#include <QApplication>

#include "rpcs3qt/pad_settings_dialog.h"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

bool keyboard_pad_handler::Init()
{
	return true;
}

keyboard_pad_handler::keyboard_pad_handler() : QObject()
{
	// Set this handler's type and save location
	m_pad_config.cfg_type = "keyboard";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_kbpad_qt.yml";

	// Set default button mapping
	m_pad_config.ls_left.def  = GetKeyName(Qt::Key_A);
	m_pad_config.ls_down.def  = GetKeyName(Qt::Key_S);
	m_pad_config.ls_right.def = GetKeyName(Qt::Key_D);
	m_pad_config.ls_up.def    = GetKeyName(Qt::Key_W);
	m_pad_config.rs_left.def  = GetKeyName(Qt::Key_Home);
	m_pad_config.rs_down.def  = GetKeyName(Qt::Key_PageDown);
	m_pad_config.rs_right.def = GetKeyName(Qt::Key_End);
	m_pad_config.rs_up.def    = GetKeyName(Qt::Key_PageUp);
	m_pad_config.start.def    = GetKeyName(Qt::Key_Return);
	m_pad_config.select.def   = GetKeyName(Qt::Key_Space);
	m_pad_config.ps.def       = GetKeyName(Qt::Key_Backspace);
	m_pad_config.square.def   = GetKeyName(Qt::Key_Z);
	m_pad_config.cross.def    = GetKeyName(Qt::Key_X);
	m_pad_config.circle.def   = GetKeyName(Qt::Key_C);
	m_pad_config.triangle.def = GetKeyName(Qt::Key_V);
	m_pad_config.left.def     = GetKeyName(Qt::Key_Left);
	m_pad_config.down.def     = GetKeyName(Qt::Key_Down);
	m_pad_config.right.def    = GetKeyName(Qt::Key_Right);
	m_pad_config.up.def       = GetKeyName(Qt::Key_Up);
	m_pad_config.r1.def       = GetKeyName(Qt::Key_E);
	m_pad_config.r2.def       = GetKeyName(Qt::Key_T);
	m_pad_config.r3.def       = GetKeyName(Qt::Key_G);
	m_pad_config.l1.def       = GetKeyName(Qt::Key_Q);
	m_pad_config.l2.def       = GetKeyName(Qt::Key_R);
	m_pad_config.l3.def       = GetKeyName(Qt::Key_F);

	// apply defaults
	m_pad_config.from_default();

	// set capabilities
	b_has_config = true;
}

void keyboard_pad_handler::Key(const u32 code, bool pressed, u16 value)
{
	for (auto pad : bindings)
	{
		for (Button& button : pad->m_buttons)
		{
			if (button.m_keyCode != code)
				continue;

			if (value >= 256) { value = 255; }

			//Todo: Is this flush necessary once games hit decent speeds?
			if (button.m_pressed && !pressed)
			{
				button.m_flush = true;

			}
			else
			{
				button.m_pressed = pressed;
				if (pressed)
					button.m_value = value;
				else
					button.m_value = 0;
			}
		}

		for (AnalogStick& stick : pad->m_sticks)
		{
			if (stick.m_keyCodeMax != code && stick.m_keyCodeMin != code)
				continue;

			//slightly less hack job for key based analog stick
			//	should also fix/make transitions when using keys smoother
			//	the logic here is that when a key is released,
			//	if we are at the opposite end of the axis, dont reset to middle
			if (stick.m_keyCodeMax == code)
			{
				if (pressed) stick.m_value = 255;
				else if (stick.m_value == 0) stick.m_value = 0;
				else stick.m_value = 128;
			}
			if (stick.m_keyCodeMin == code)
			{
				if (pressed) stick.m_value = 0;
				else if (stick.m_value == 255) stick.m_value = 255;
				else stick.m_value = 128;
			}
		}
	}
}


bool keyboard_pad_handler::eventFilter(QObject* target, QEvent* ev)
{
	// !m_target is for future proofing when gsrender isn't automatically initialized on load.
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible()|| target == m_target)
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

/* Sets the target window for the event handler, and also installs an event filter on the target. */
void keyboard_pad_handler::SetTargetWindow(QWindow* target)
{
	if (target != nullptr)
	{
		m_target = target;
		target->installEventFilter(this);
	}
	else
	{
		QApplication::instance()->installEventFilter(this);
		// If this is hit, it probably means that some refactoring occurs because currently a gsframe is created in Load.
		// We still want events so filter from application instead since target is null.
		LOG_ERROR(GENERAL, "Trying to set pad handler to a null target window.");
	}
}

void keyboard_pad_handler::keyPressEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->ignore();
		return;
	}
	switch (event->key())
	{
		case Qt::Key_L:
			if (!(event->modifiers() == Qt::AltModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		case Qt::Key_Return:
			if (!(event->modifiers() == Qt::AltModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		case Qt::Key_Escape:
			event->ignore();
			break;
		case Qt::Key_P:
			if (!(event->modifiers() == Qt::ControlModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		case Qt::Key_S:
			if (!(event->modifiers() == Qt::ControlModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		case Qt::Key_R:
			if (!(event->modifiers() == Qt::ControlModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		case Qt::Key_E:
			if (!(event->modifiers() == Qt::ControlModifier)) { Key(event->key(), 1); }
			event->ignore();
			break;
		default:
			Key(event->key(), 1);
			event->ignore();
			break;
	}
}

void keyboard_pad_handler::keyReleaseEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->ignore();
		return;
	}

	Key(event->key(), 0);
	event->ignore();
}

std::vector<std::string> keyboard_pad_handler::ListDevices()
{
	std::vector<std::string> list_devices;
	list_devices.push_back("Keyboard");
	return list_devices;
}

const std::string keyboard_pad_handler::GetKeyName(const QKeyEvent* keyEvent)
{
	//TODO what about numpad?
	// Fix some unknown button names
	switch (keyEvent->key())
	{
	case Qt::Key_Alt:
		return "Alt";
	case Qt::Key_AltGr:
		return "AltGr";
	case Qt::Key_Shift:
		return "Shift";
	case Qt::Key_Control:
		return "Ctrl";
	case Qt::Key_NumLock:
		return sstr(QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText));
	default:
		break;
	}
	return sstr(QKeySequence(keyEvent->key() | keyEvent->modifiers()).toString(QKeySequence::NativeText));
}

const std::string keyboard_pad_handler::GetKeyName(const u32& keyCode)
{
	//TODO what about numpad?
	return sstr(QKeySequence(keyCode).toString(QKeySequence::NativeText));
}

const u32 keyboard_pad_handler::GetKeyCode(const std::string& keyName)
{
	if (keyName == "Alt")
		return Qt::Key_Alt;
	else if (keyName == "AltGr")
		return Qt::Key_AltGr;
	else if (keyName == "Shift")
		return Qt::Key_Shift;
	else if (keyName == "Ctrl")
		return Qt::Key_Control;

	QString key = qstr(keyName);
	QKeySequence seq(key);
	u32 keyCode;
	// We should only working with a single key here
	if (seq.count() == 1)
	{
		keyCode = seq[0];
	}
	else
	{
		// Should be here only if a modifier key (e.g. Ctrl, Alt) is pressed.
		if (seq.count() != 0)
		{
			return 0;
		}
		// Add a non-modifier key "A" to the picture because QKeySequence
		// seems to need that to acknowledge the modifier. We know that A has
		// a keyCode of 65 (or 0x41 in hex)
		seq = QKeySequence(key + "+A");
		if (seq.count() != 0 || seq[0] <= 65)
		{
			return 0;
		}
		keyCode = seq[0] - 65;
	}
	return keyCode;
}

bool keyboard_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	if (device != "Keyboard") return false;

	m_pad_config.load();

	//Fixed assign change, default is both sensor and press off
	pad->Init(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, GetKeyCode(m_pad_config.select),   CELL_PAD_CTRL_SELECT);
	//pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.ps),       CELL_PAD_CTRL_PS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, GetKeyCode(m_pad_config.l2),       CELL_PAD_CTRL_L2);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  GetKeyCode(m_pad_config.ls_left), GetKeyCode(m_pad_config.ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  GetKeyCode(m_pad_config.ls_up),   GetKeyCode(m_pad_config.ls_down));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, GetKeyCode(m_pad_config.rs_left), GetKeyCode(m_pad_config.rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, GetKeyCode(m_pad_config.rs_up),   GetKeyCode(m_pad_config.rs_down));

	bindings.push_back(pad);

	return true;
}

void keyboard_pad_handler::ThreadProc()
{

}
