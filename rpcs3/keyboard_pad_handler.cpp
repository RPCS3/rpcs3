#include "stdafx.h"

#include "keyboard_pad_handler.h"

keyboard_pad_config g_kbpad_config;

void keyboard_pad_handler::Init(const u32 max_connect)
{
	memset(&m_info, 0, sizeof(PadInfo));
	m_info.max_connect = max_connect;
	LoadSettings();
	m_info.now_connect = std::min(m_pads.size(), (size_t)max_connect);
}

keyboard_pad_handler::keyboard_pad_handler(QObject* target, QObject* parent) : QObject(parent), m_target(target)
{
	target->installEventFilter(this);
}

bool keyboard_pad_handler::eventFilter(QObject* target, QEvent* ev)
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

void keyboard_pad_handler::keyPressEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
	{
		event->ignore();
		return;
	}

	Key(event->key(), 1);
	event->ignore();
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

void keyboard_pad_handler::LoadSettings()
{
	g_kbpad_config.load();

	//Fixed assign change, default is both sensor and press off
	m_pads.emplace_back(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.left, CELL_PAD_CTRL_LEFT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.down, CELL_PAD_CTRL_DOWN);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.right, CELL_PAD_CTRL_RIGHT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.up, CELL_PAD_CTRL_UP);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.start, CELL_PAD_CTRL_START);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.r3, CELL_PAD_CTRL_R3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.l3, CELL_PAD_CTRL_L3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.select, CELL_PAD_CTRL_SELECT);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.square, CELL_PAD_CTRL_SQUARE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.cross, CELL_PAD_CTRL_CROSS);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.circle, CELL_PAD_CTRL_CIRCLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r1, CELL_PAD_CTRL_R1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l1, CELL_PAD_CTRL_L1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r2, CELL_PAD_CTRL_R2);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l2, CELL_PAD_CTRL_L2);

	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, g_kbpad_config.left_stick_left, g_kbpad_config.left_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, g_kbpad_config.left_stick_up, g_kbpad_config.left_stick_down);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, g_kbpad_config.right_stick_left, g_kbpad_config.right_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, g_kbpad_config.right_stick_up, g_kbpad_config.right_stick_down);
}
