#pragma once
#include "Emu/Io/PadHandler.h"
#include "Emu/state.h"

class WindowsPadHandler final
	: public wxWindow
	, public PadHandlerBase
{
public:
	WindowsPadHandler() : wxWindow()
	{
		wxGetApp().Bind(wxEVT_KEY_DOWN, &WindowsPadHandler::KeyDown, this);
		wxGetApp().Bind(wxEVT_KEY_UP, &WindowsPadHandler::KeyUp, this);
	}

	virtual void KeyDown(wxKeyEvent& event) { Key(event.GetKeyCode(), 1); event.Skip(); }
	virtual void KeyUp(wxKeyEvent& event)   { Key(event.GetKeyCode(), 0); event.Skip(); }

	virtual void Init(const u32 max_connect)
	{
		memset(&m_info, 0, sizeof(PadInfo));
		m_info.max_connect = max_connect;
		LoadSettings();
		m_info.now_connect = std::min(m_pads.size(), (size_t)max_connect);
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(PadInfo));
		m_pads.clear();
	}

	void LoadSettings()
	{
		//Fixed assign change, default is both sensor and press off
		m_pads.emplace_back(
			CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
			CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
			CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
			CELL_PAD_DEV_TYPE_STANDARD);

		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.left.value(), CELL_PAD_CTRL_LEFT);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.down.value(), CELL_PAD_CTRL_DOWN);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.right.value(), CELL_PAD_CTRL_RIGHT);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.up.value(), CELL_PAD_CTRL_UP);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.start.value(), CELL_PAD_CTRL_START);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.r3.value(), CELL_PAD_CTRL_R3);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.l3.value(), CELL_PAD_CTRL_L3);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, rpcs3::config.io.pad.select.value(), CELL_PAD_CTRL_SELECT);

		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.square.value(), CELL_PAD_CTRL_SQUARE);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.cross.value(), CELL_PAD_CTRL_CROSS);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.circle.value(), CELL_PAD_CTRL_CIRCLE);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.triangle.value(), CELL_PAD_CTRL_TRIANGLE);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.r1.value(), CELL_PAD_CTRL_R1);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.l1.value(), CELL_PAD_CTRL_L1);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.r2.value(), CELL_PAD_CTRL_R2);
		m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, rpcs3::config.io.pad.l2.value(), CELL_PAD_CTRL_L2);

		m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, rpcs3::config.io.pad.left_stick_left.value(), rpcs3::config.io.pad.left_stick_right.value());
		m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, rpcs3::config.io.pad.left_stick_up.value(), rpcs3::config.io.pad.left_stick_down.value());
		m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, rpcs3::config.io.pad.right_stick_left.value(), rpcs3::config.io.pad.right_stick_right.value());
		m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, rpcs3::config.io.pad.right_stick_up.value(), rpcs3::config.io.pad.right_stick_down.value());
	}
};
