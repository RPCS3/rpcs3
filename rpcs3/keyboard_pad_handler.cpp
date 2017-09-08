#include "stdafx.h"

#include "keyboard_pad_handler.h"

#include <QApplication>

#include "rpcs3qt/pad_settings_dialog.h"

keyboard_pad_config g_kbpad_config;

bool keyboard_pad_handler::Init()
{
	return true;
}

keyboard_pad_handler::keyboard_pad_handler() : QObject()
{
	b_has_config = true;
}

void keyboard_pad_handler::ConfigController(std::string device)
{
	pad_settings_dialog dlg(this);
	dlg.exec();
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

bool keyboard_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	if (device != "Keyboard") return false;

	g_kbpad_config.load();

	//Fixed assign change, default is both sensor and press off
	pad->Init(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.left, CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.down, CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.right, CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.up, CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.start, CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.r3, CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.l3, CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.select, CELL_PAD_CTRL_SELECT);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.square, CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.cross, CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.circle, CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r1, CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l1, CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r2, CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l2, CELL_PAD_CTRL_L2);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, g_kbpad_config.left_stick_left, g_kbpad_config.left_stick_right);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, g_kbpad_config.left_stick_up, g_kbpad_config.left_stick_down);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, g_kbpad_config.right_stick_left, g_kbpad_config.right_stick_right);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, g_kbpad_config.right_stick_up, g_kbpad_config.right_stick_down);

	bindings.push_back(pad);

	return true;
}

void keyboard_pad_handler::ThreadProc()
{

}
