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
	value = Clamp0To255(value);

	for (auto pad : bindings)
	{
		for (Button& button : pad->m_buttons)
		{
			if (button.m_keyCode != code)
				continue;

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

		for (int i = 0; i < static_cast<int>(pad->m_sticks.size()); i++)
		{
			if (pad->m_sticks[i].m_keyCodeMax == code)
				m_stick_max[i] = pressed ? 255 : 128;

			if (pad->m_sticks[i].m_keyCodeMin == code)
				m_stick_min[i] = pressed ? 128 : 0;

			pad->m_sticks[i].m_value = m_stick_max[i] - m_stick_min[i];
		}
	}
}

int keyboard_pad_handler::GetModifierCode(QKeyEvent* e)
{
	switch (e->key())
	{
	case Qt::Key_Control:
	case Qt::Key_Alt:
	case Qt::Key_AltGr:
	case Qt::Key_Shift:
	case Qt::Key_Meta:
	case Qt::Key_NumLock:
		return 0;
	default:
		break;
	}

	if (e->modifiers() == Qt::ControlModifier)
		return Qt::ControlModifier;
	else if (e->modifiers() == Qt::AltModifier)
		return Qt::AltModifier;
	else if (e->modifiers() == Qt::MetaModifier)
		return Qt::MetaModifier;
	else if (e->modifiers() == Qt::ShiftModifier)
		return Qt::ShiftModifier;
	else if (e->modifiers() == Qt::KeypadModifier)
		return Qt::KeypadModifier;

	return 0;
}

bool keyboard_pad_handler::eventFilter(QObject* target, QEvent* ev)
{
	// !m_target is for future proofing when gsrender isn't automatically initialized on load.
	// !m_target->isVisible() is a hack since currently a guiless application will STILL inititialize a gsrender (providing a valid target)
	if (!m_target || !m_target->isVisible()|| target == m_target)
	{
		switch (ev->type())
		{
		case QEvent::KeyPress:
			keyPressEvent(static_cast<QKeyEvent*>(ev));
			break;
		case QEvent::KeyRelease:
			keyReleaseEvent(static_cast<QKeyEvent*>(ev));
			break;
		case QEvent::MouseButtonPress:
			mousePressEvent(static_cast<QMouseEvent*>(ev));
			break;
		case QEvent::MouseButtonRelease:
			mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
			break;
		default:
			break;
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

void keyboard_pad_handler::processKeyEvent(QKeyEvent* event, bool pressed)
{
	if (event->isAutoRepeat())
	{
		event->ignore();
		return;
	}

	auto handleKey = [this, pressed, event]()
	{
		const QString name = qstr(GetKeyName(event));
		QStringList list = GetKeyNames(event);
		if (list.isEmpty())
			return;

		bool is_num_key = list.contains("Num");
		if (is_num_key)
			list.removeAll("Num");

		// TODO: Edge case: switching numlock keeps numpad keys pressed due to now different modifier

		// Handle every possible key combination, for example: ctrl+A -> {ctrl, A, ctrl+A}
		for (const auto& keyname : list)
		{
			// skip the 'original keys' when handling numpad keys
			if (is_num_key && !keyname.contains("Num"))
				continue;
			// skip held modifiers when handling another key
			if (keyname != name && list.count() > 1 && (keyname == "Alt" || keyname == "AltGr" || keyname == "Ctrl" || keyname == "Meta" || keyname == "Shift"))
				continue;
			Key(GetKeyCode(keyname), pressed);
		}
	};

	// We need to ignore keys when using rpcs3 keyboard shortcuts
	int key = event->key();
	switch (key)
	{
	case Qt::Key_Escape:
		break;
	case Qt::Key_L:
	case Qt::Key_Return:
		if (event->modifiers() != Qt::AltModifier)
			handleKey();
		break;
	case Qt::Key_P:
	case Qt::Key_S:
	case Qt::Key_R:
	case Qt::Key_E:
		if (event->modifiers() != Qt::ControlModifier)
			handleKey();
		break;
	default:
		handleKey();
		break;
	}
	event->ignore();
}

void keyboard_pad_handler::keyPressEvent(QKeyEvent* event)
{
	processKeyEvent(event, 1);
}

void keyboard_pad_handler::keyReleaseEvent(QKeyEvent* event)
{
	processKeyEvent(event, 0);
}

void keyboard_pad_handler::mousePressEvent(QMouseEvent* event)
{
	Key(event->button(), 1);
	event->ignore();
}

void keyboard_pad_handler::mouseReleaseEvent(QMouseEvent* event)
{
	Key(event->button(), 0, 0);
	event->ignore();
}

std::vector<std::string> keyboard_pad_handler::ListDevices()
{
	std::vector<std::string> list_devices;
	list_devices.push_back("Keyboard");
	return list_devices;
}

std::string keyboard_pad_handler::GetMouseName(const QMouseEvent* event)
{
	return GetMouseName(event->button());
}

std::string keyboard_pad_handler::GetMouseName(u32 button)
{
	auto it = mouse_list.find(button);
	if (it != mouse_list.end())
		return it->second;
	return "FAIL";
}

QStringList keyboard_pad_handler::GetKeyNames(const QKeyEvent* keyEvent)
{
	QStringList list;

	if (keyEvent->modifiers() & Qt::ShiftModifier)
	{
		list.append("Shift");
		list.append(QKeySequence(keyEvent->key() | Qt::ShiftModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::AltModifier)
	{
		list.append("Alt");
		list.append(QKeySequence(keyEvent->key() | Qt::AltModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::ControlModifier)
	{
		list.append("Ctrl");
		list.append(QKeySequence(keyEvent->key() | Qt::ControlModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::MetaModifier)
	{
		list.append("Meta");
		list.append(QKeySequence(keyEvent->key() | Qt::MetaModifier).toString(QKeySequence::NativeText));
	}
	if (keyEvent->modifiers() & Qt::KeypadModifier)
	{
		list.append("Num"); // helper object, not used as actual key
		list.append(QKeySequence(keyEvent->key() | Qt::KeypadModifier).toString(QKeySequence::NativeText));
	}

	switch (keyEvent->key())
	{
	case Qt::Key_Alt:
		list.append("Alt");
		break;
	case Qt::Key_AltGr:
		list.append("AltGr");
		break;
	case Qt::Key_Shift:
		list.append("Shift");
		break;
	case Qt::Key_Control:
		list.append("Ctrl");
		break;
	case Qt::Key_Meta:
		list.append("Meta");
		break;
	default:
		list.append(QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText));
		break;
	}

	list.removeDuplicates();
	return list;
}

std::string keyboard_pad_handler::GetKeyName(const QKeyEvent* keyEvent)
{
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
	case Qt::Key_Meta:
		return "Meta";
	case Qt::Key_NumLock:
		return sstr(QKeySequence(keyEvent->key()).toString(QKeySequence::NativeText));
	default:
		break;
	}
	return sstr(QKeySequence(keyEvent->key() | keyEvent->modifiers()).toString(QKeySequence::NativeText));
}

std::string keyboard_pad_handler::GetKeyName(const u32& keyCode)
{
	return sstr(QKeySequence(keyCode).toString(QKeySequence::NativeText));
}

u32 keyboard_pad_handler::GetKeyCode(const std::string& keyName)
{
	return GetKeyCode(qstr(keyName));
}

u32 keyboard_pad_handler::GetKeyCode(const QString& keyName)
{
	if (keyName.isEmpty())
		return 0;
	else if (keyName == "Alt")
		return Qt::Key_Alt;
	else if (keyName == "AltGr")
		return Qt::Key_AltGr;
	else if (keyName == "Shift")
		return Qt::Key_Shift;
	else if (keyName == "Ctrl")
		return Qt::Key_Control;
	else if (keyName == "Meta")
		return Qt::Key_Meta;

	QKeySequence seq(keyName);
	u32 keyCode = 0;

	if (seq.count() == 1)
		keyCode = seq[0];
	else
		LOG_NOTICE(GENERAL, "GetKeyCode(%s): seq.count() = %d", sstr(keyName), seq.count());

	return keyCode;
}

bool keyboard_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	if (device != "Keyboard")
		return false;

	m_pad_config.load();

	auto find_key = [&](const cfg::string& name)
	{
		int key = FindKeyCode(mouse_list, name, false);
		if (key < 0)
			key = GetKeyCode(name);
		if (key < 0)
			key = 0;
		return key;
	};

	//Fixed assign change, default is both sensor and press off
	pad->Init
	(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.left),     CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.down),     CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.right),    CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.up),       CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.start),    CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.r3),       CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.l3),       CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, find_key(m_pad_config.select),   CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.ps),       0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0,                               0x0); // Reserved
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.square),   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.cross),    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.circle),   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.triangle), CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r1),       CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l1),       CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.r2),       CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, find_key(m_pad_config.l2),       CELL_PAD_CTRL_L2);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  find_key(m_pad_config.ls_left), find_key(m_pad_config.ls_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  find_key(m_pad_config.ls_up),   find_key(m_pad_config.ls_down));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, find_key(m_pad_config.rs_left), find_key(m_pad_config.rs_right));
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, find_key(m_pad_config.rs_up),   find_key(m_pad_config.rs_down));

	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_X, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Y, 399);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_Z, 512);
	pad->m_sensors.emplace_back(CELL_PAD_BTN_OFFSET_SENSOR_G, 512);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.push_back(pad);
	connected++;

	return true;
}

void keyboard_pad_handler::ThreadProc()
{
}
