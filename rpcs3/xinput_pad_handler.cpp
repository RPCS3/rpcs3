
#ifdef _MSC_VER
#include "xinput_pad_handler.h"
#include "rpcs3qt/pad_settings_dialog.h"

std::string xinput_pad_handler::GetButtonName(u32 button)
{
	const auto check = button_list.find(button);
	if (check == button_list.end())
	{
		return "FAIL";
	}
	return check->second;
}

void xinput_pad_handler::GetNextButtonPress(const std::string& padId, const std::function<void(u32 btn, std::string)>& callback)
{
	if (!Init())
	{
		return;
	}

	u32 device_number = 0;
	size_t pos = padId.find("Xinput Pad #");

	if (pos != std::string::npos)
	{
		device_number = std::stoul(padId.substr(pos + 12));
	}

	if (pos == std::string::npos || device_number >= XUSER_MAX_COUNT)
	{
		return;
	}
	
	DWORD dwResult;
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	// Simply get the state of the controller from XInput.
	dwResult = (*xinputGetState)(device_number, &state);

	if (dwResult == ERROR_SUCCESS)
	{
		for (const auto& button : button_list)
		{
			u32 keycode = button.first;

			if ((keycode < TriggersNSticks::TRIGGER_LEFT)   && (state.Gamepad.wButtons & keycode)
			 || (keycode == TriggersNSticks::TRIGGER_LEFT)  && (state.Gamepad.bLeftTrigger > m_pad_config.ltriggerthreshold)
			 || (keycode == TriggersNSticks::TRIGGER_RIGHT) && (state.Gamepad.bRightTrigger > m_pad_config.rtriggerthreshold)
			 || (keycode == TriggersNSticks::STICK_L_LEFT)  && (state.Gamepad.sThumbLX < -m_pad_config.lstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_L_RIGHT) && (state.Gamepad.sThumbLX > m_pad_config.lstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_L_DOWN)  && (state.Gamepad.sThumbLY < -m_pad_config.lstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_L_UP)    && (state.Gamepad.sThumbLY > m_pad_config.lstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_R_LEFT)  && (state.Gamepad.sThumbRX < -m_pad_config.rstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_R_RIGHT) && (state.Gamepad.sThumbRX > m_pad_config.rstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_R_DOWN)  && (state.Gamepad.sThumbRY < -m_pad_config.rstickdeadzone)
			 || (keycode == TriggersNSticks::STICK_R_UP)    && (state.Gamepad.sThumbRY > m_pad_config.rstickdeadzone))
			{
				return callback(button.first, button.second);
			}
		}
	}
}

void xinput_pad_handler::TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor)
{
	if (!Init())
	{
		return;
	}

	u32 device_number = 0;
	size_t pos = padId.find("Xinput Pad #");

	if (pos != std::string::npos)
	{
		device_number = std::stoul(padId.substr(pos + 12));
	}

	if (pos == std::string::npos || device_number >= XUSER_MAX_COUNT)
	{
		return;
	}

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects.
	XINPUT_VIBRATION vibrate;

	vibrate.wLeftMotorSpeed = largeMotor;  // between 0 to 65535
	vibrate.wRightMotorSpeed = smallMotor; // between 0 to 65535

	(*xinputSetState)(device_number, &vibrate);
}

void xinput_pad_handler::TranslateButtonPress(u32 keyCode, bool& pressed, u16& value, bool ignore_threshold)
{
	switch (keyCode)
	{
	case TriggersNSticks::TRIGGER_LEFT:
		pressed = state.Gamepad.bLeftTrigger > m_pad_config.ltriggerthreshold;
		value = pressed ? NormalizeTriggerInput(state.Gamepad.bLeftTrigger, m_pad_config.ltriggerthreshold) : 0;
		break;
	case TriggersNSticks::TRIGGER_RIGHT:
		pressed = state.Gamepad.bRightTrigger > m_pad_config.rtriggerthreshold;
		value = pressed ? NormalizeTriggerInput(state.Gamepad.bRightTrigger, m_pad_config.rtriggerthreshold) : 0;
		break;
	case TriggersNSticks::STICK_L_LEFT:
		pressed = state.Gamepad.sThumbLX < (ignore_threshold ? 0 : -m_pad_config.lstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbLX, m_pad_config.lstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_L_RIGHT:
		pressed = state.Gamepad.sThumbLX >(ignore_threshold ? 0 : m_pad_config.lstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbLX, m_pad_config.lstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_L_UP:
		pressed = state.Gamepad.sThumbLY > (ignore_threshold ? 0 : m_pad_config.lstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbLY, m_pad_config.lstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_L_DOWN:
		pressed = state.Gamepad.sThumbLY < (ignore_threshold ? 0 : -m_pad_config.lstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbLY, m_pad_config.lstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_R_LEFT:
		pressed = state.Gamepad.sThumbRX < (ignore_threshold ? 0 : -m_pad_config.rstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbRX, m_pad_config.rstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_R_RIGHT:
		pressed = state.Gamepad.sThumbRX >(ignore_threshold ? 0 : m_pad_config.rstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbRX, m_pad_config.rstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_R_UP:
		pressed = state.Gamepad.sThumbRY > (ignore_threshold ? 0 : m_pad_config.rstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbRY, m_pad_config.rstickdeadzone, ignore_threshold) : 0;
		break;
	case TriggersNSticks::STICK_R_DOWN:
		pressed = state.Gamepad.sThumbRY < (ignore_threshold ? 0 : -m_pad_config.rstickdeadzone);
		value = pressed ? NormalizeStickInput(state.Gamepad.sThumbRY, m_pad_config.rstickdeadzone, ignore_threshold) : 0;
		break;
	default:
		pressed = state.Gamepad.wButtons & keyCode;
		value = pressed ? 255 : 0;
		break;
	}
}

xinput_pad_handler::xinput_pad_handler() : library(nullptr), xinputGetState(nullptr), xinputEnable(nullptr), xinputSetState(nullptr), is_init(false)
{
	THUMB_MIN = 0;
	THUMB_MAX = 32767;
	TRIGGER_MIN = 0;
	TRIGGER_MAX = 255;
	VIBRATION_MIN = 0;
	VIBRATION_MAX = 65535;

	m_pad_config.cfg_type = "xinput";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_xinput.yml";

	m_pad_config.left_stick_left.def = TriggersNSticks::STICK_L_LEFT;
	m_pad_config.left_stick_down.def = TriggersNSticks::STICK_L_DOWN;
	m_pad_config.left_stick_right.def = TriggersNSticks::STICK_L_RIGHT;
	m_pad_config.left_stick_up.def = TriggersNSticks::STICK_L_UP;
	m_pad_config.right_stick_left.def = TriggersNSticks::STICK_R_LEFT;
	m_pad_config.right_stick_down.def = TriggersNSticks::STICK_R_DOWN;
	m_pad_config.right_stick_right.def = TriggersNSticks::STICK_R_RIGHT;
	m_pad_config.right_stick_up.def = TriggersNSticks::STICK_R_UP;
	m_pad_config.start.def = XINPUT_GAMEPAD_START;
	m_pad_config.select.def = XINPUT_GAMEPAD_BACK;
	m_pad_config.square.def = XINPUT_GAMEPAD_X;
	m_pad_config.cross.def = XINPUT_GAMEPAD_A;
	m_pad_config.circle.def = XINPUT_GAMEPAD_B;
	m_pad_config.triangle.def = XINPUT_GAMEPAD_Y;
	m_pad_config.left.def = XINPUT_GAMEPAD_DPAD_LEFT;
	m_pad_config.down.def = XINPUT_GAMEPAD_DPAD_DOWN;
	m_pad_config.right.def = XINPUT_GAMEPAD_DPAD_RIGHT;
	m_pad_config.up.def = XINPUT_GAMEPAD_DPAD_UP;
	m_pad_config.r1.def = XINPUT_GAMEPAD_RIGHT_SHOULDER;
	m_pad_config.r2.def = TriggersNSticks::TRIGGER_RIGHT;
	m_pad_config.r3.def = XINPUT_GAMEPAD_RIGHT_THUMB;
	m_pad_config.l1.def = XINPUT_GAMEPAD_LEFT_SHOULDER;
	m_pad_config.l2.def = TriggersNSticks::TRIGGER_LEFT;
	m_pad_config.l3.def = XINPUT_GAMEPAD_LEFT_THUMB;

	m_pad_config.lstickdeadzone.def = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between 0 and 32767
	m_pad_config.rstickdeadzone.def = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between 0 and 32767
	m_pad_config.ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD; // between 0 and 255
	m_pad_config.rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD; // between 0 and 255
	m_pad_config.padsquircling.def = 8000;

	m_pad_config.from_default();

	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;
}

xinput_pad_handler::~xinput_pad_handler()
{
	Close();
}

void xinput_pad_handler::ConfigController(const std::string& device)
{
	pad_settings_dialog dlg(&m_pad_config, device, *this);
	dlg.exec();
}

bool xinput_pad_handler::Init()
{
	if (is_init) return true;

	for (auto it : XINPUT_INFO::LIBRARY_FILENAMES)
	{
		library = LoadLibrary(it);
		if (library)
		{
			xinputEnable = reinterpret_cast<PFN_XINPUTENABLE>(GetProcAddress(library, "XInputEnable"));
			xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, reinterpret_cast<LPCSTR>(100)));
			if (!xinputGetState)
			{
				xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, "XInputGetState"));
			}

			xinputSetState = reinterpret_cast<PFN_XINPUTSETSTATE>(GetProcAddress(library, "XInputSetState"));

			if (xinputEnable && xinputGetState && xinputSetState)
			{
				is_init = true;
				break;
			}

			FreeLibrary(library);
			library = nullptr;
			xinputEnable = nullptr;
			xinputGetState = nullptr;
		}
	}

	if (!is_init) return false;

	m_pad_config.load();
	if (!m_pad_config.exist()) m_pad_config.save();

	return true;
}

void xinput_pad_handler::Close()
{
	if (library)
	{
		FreeLibrary(library);
		library = nullptr;
		xinputGetState = nullptr;
		xinputEnable = nullptr;
	}
}

void xinput_pad_handler::ThreadProc()
{
	for (u32 index = 0; index != bindings.size(); index++)
	{
		auto padnum = bindings[index].first;
		auto pad = bindings[index].second;

		result = (*xinputGetState)(padnum, &state);
		switch (result)
		{
		case ERROR_DEVICE_NOT_CONNECTED:
			if (last_connection_status[padnum] == true)
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[padnum] = false;
			pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
			break;

		case ERROR_SUCCESS:
			++online;
			if (last_connection_status[padnum] == false)
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
			last_connection_status[padnum] = true;
			pad->m_port_status |= CELL_PAD_STATUS_CONNECTED;

			for (auto& btn : pad->m_buttons)
			{
				TranslateButtonPress(btn.m_keyCode, btn.m_pressed, btn.m_value);
			}

			for (const auto& btn : pad->m_buttons)
			{
				if (btn.m_pressed)
				{
					SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
					break;
				}
			}

			float stick_val[4];

			for (int i = 0; i < static_cast<int>(pad->m_sticks.size()); i++)
			{
				bool pressed;
				u16 val_min, val_max;
				TranslateButtonPress(pad->m_sticks[i].m_keyCodeMin, pressed, val_min, true);
				TranslateButtonPress(pad->m_sticks[i].m_keyCodeMax, pressed, val_max, true);
				stick_val[i] = val_max - val_min;
			}

			NormalizeRawStickInput(stick_val[0], stick_val[1], m_pad_config.lstickdeadzone);
			NormalizeRawStickInput(stick_val[2], stick_val[3], m_pad_config.rstickdeadzone);

			pad->m_sticks[0].m_value = ConvertAxis(stick_val[0]);
			pad->m_sticks[1].m_value = 255 - ConvertAxis(stick_val[1]);
			pad->m_sticks[2].m_value = ConvertAxis(stick_val[2]);
			pad->m_sticks[3].m_value = 255 - ConvertAxis(stick_val[3]);

			if (m_pad_config.padsquircling != 0)
			{
				std::tie(pad->m_sticks[0].m_value, pad->m_sticks[1].m_value) = ConvertToSquirclePoint(pad->m_sticks[0].m_value, pad->m_sticks[1].m_value, m_pad_config.padsquircling);
				std::tie(pad->m_sticks[2].m_value, pad->m_sticks[3].m_value) = ConvertToSquirclePoint(pad->m_sticks[2].m_value, pad->m_sticks[3].m_value, m_pad_config.padsquircling);
			}

			// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
			// The two motors are not the same, and they create different vibration effects. Values range between 0 to 65535.
			XINPUT_VIBRATION vibrate;

			int idx_l = m_pad_config.switch_vibration_motors ? 1 : 0;
			int idx_s = m_pad_config.switch_vibration_motors ? 0 : 1;

			int speed_large = m_pad_config.enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value * 257 : VIBRATION_MIN;
			int speed_small = m_pad_config.enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value * 257 : VIBRATION_MIN;

			vibrate.wLeftMotorSpeed = speed_large;
			vibrate.wRightMotorSpeed = speed_small;

			(*xinputSetState)(padnum, &vibrate);

			break;
		}
	}
}

std::vector<std::string> xinput_pad_handler::ListDevices()
{
	std::vector<std::string> xinput_pads_list;

	if (!Init()) return xinput_pads_list;

	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		XINPUT_STATE state;
		DWORD result = (*xinputGetState)(i, &state);
		if (result == ERROR_SUCCESS)
		{
			xinput_pads_list.push_back(fmt::format("Xinput Pad #%d", i));
		}
	}
	return xinput_pads_list;
}

bool xinput_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device)
{
	//Convert device string to u32 representing xinput device number
	u32 device_number = 0;
	size_t pos = device.find("Xinput Pad #");

	if (pos != std::string::npos) device_number = std::stoul(device.substr(pos + 12));

	if (pos == std::string::npos || device_number >= XUSER_MAX_COUNT) return false;

	m_pad_config.load();

	pad->Init(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_ACTUATOR,
		CELL_PAD_DEV_TYPE_STANDARD
	);

	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.up,     CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.down,   CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.left,   CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.right,  CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.start,  CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.select, CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.l3,     CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, m_pad_config.r3,     CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.l1,     CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.r1,     CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_INFO::GUIDE_BUTTON, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.cross,    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.circle,   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.square,   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.l2,       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.r2,       CELL_PAD_CTRL_R2);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  m_pad_config.left_stick_left,  m_pad_config.left_stick_right);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  m_pad_config.left_stick_down,    m_pad_config.left_stick_up);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, m_pad_config.right_stick_left, m_pad_config.right_stick_right);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, m_pad_config.right_stick_down,   m_pad_config.right_stick_up);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	bindings.emplace_back(device_number, pad);

	return true;
}

#endif
