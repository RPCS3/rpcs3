#pragma once

#include "Emu/Io/PadHandler.h"

class WindowsPadHandler
	: public wxWindow
	, public PadHandlerBase
{
	AppConnector m_app_connector;

public:
	WindowsPadHandler() : wxWindow()
	{
		m_app_connector.Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(WindowsPadHandler::KeyDown), (wxObject*)0, this);
		m_app_connector.Connect(wxEVT_KEY_UP, wxKeyEventHandler(WindowsPadHandler::KeyUp), (wxObject*)0, this);
	}

	virtual void KeyDown(wxKeyEvent& event) { Key(event.GetKeyCode(), 1); event.Skip(); }
	virtual void KeyUp(wxKeyEvent& event)   { Key(event.GetKeyCode(), 0); event.Skip(); }

	virtual void Init(const u32 max_connect)
	{
		memset(&m_info, 0, sizeof(PadInfo));
		m_info.max_connect = max_connect;
		LoadSettings();
		m_info.now_connect = min<int>(GetPads().GetCount(), max_connect);
	}

	virtual void Close()
	{
		memset(&m_info, 0, sizeof(PadInfo));
		m_pads.Clear();
	}

	void LoadSettings()
	{
		m_pads.Move(new Pad(
			CELL_PAD_STATUS_CONNECTED, CELL_PAD_SETTING_PRESS_ON | CELL_PAD_SETTING_SENSOR_OFF,
			CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
			CELL_PAD_DEV_TYPE_STANDARD));

		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerLeft.GetValue()), CELL_PAD_CTRL_LEFT));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerDown.GetValue()), CELL_PAD_CTRL_DOWN));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerRight.GetValue()), CELL_PAD_CTRL_RIGHT));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerUp.GetValue()), CELL_PAD_CTRL_UP));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, WXK_RETURN, CELL_PAD_CTRL_START));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerR3.GetValue()), CELL_PAD_CTRL_R3));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, static_cast<char>(Ini.PadHandlerL3.GetValue()), CELL_PAD_CTRL_L3));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL1, WXK_SPACE, CELL_PAD_CTRL_SELECT));

		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerSquare.GetValue()), CELL_PAD_CTRL_SQUARE));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerCross.GetValue()), CELL_PAD_CTRL_CROSS));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerCircle.GetValue()), CELL_PAD_CTRL_CIRCLE));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerTriangle.GetValue()), CELL_PAD_CTRL_TRIANGLE));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerR1.GetValue()), CELL_PAD_CTRL_R1));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerL1.GetValue()), CELL_PAD_CTRL_L1));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerR2.GetValue()), CELL_PAD_CTRL_R2));
		m_pads[0].m_buttons.Move(new Button(CELL_PAD_BTN_OFFSET_DIGITAL2, static_cast<char>(Ini.PadHandlerL2.GetValue()), CELL_PAD_CTRL_L2));

		m_pads[0].m_sticks.Move(new AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, WXK_LEFT, WXK_RIGHT));
		m_pads[0].m_sticks.Move(new AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, WXK_UP, WXK_DOWN));
		m_pads[0].m_sticks.Move(new AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, WXK_HOME, WXK_END));
		m_pads[0].m_sticks.Move(new AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, WXK_PAGEUP, WXK_PAGEDOWN));
	}
};