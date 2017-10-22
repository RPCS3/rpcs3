
#ifdef _MSC_VER
#include "xinput_pad_handler.h"
#include "rpcs3qt/pad_settings_dialog.h"

static std::map<u32, std::string> button_list =
{
	{ XINPUT_GAMEPAD_A, "A" },
	{ XINPUT_GAMEPAD_B, "B" },
	{ XINPUT_GAMEPAD_X, "X" },
	{ XINPUT_GAMEPAD_Y, "Y" },
	{ XINPUT_GAMEPAD_DPAD_LEFT, "Left" },
	{ XINPUT_GAMEPAD_DPAD_RIGHT, "Right" },
	{ XINPUT_GAMEPAD_DPAD_UP, "Up" },
	{ XINPUT_GAMEPAD_DPAD_DOWN, "Down" },
	{ XINPUT_GAMEPAD_LEFT_SHOULDER, "LB" },
	{ XINPUT_GAMEPAD_RIGHT_SHOULDER, "RB" },
	{ XINPUT_GAMEPAD_BACK, "Back" },
	{ XINPUT_GAMEPAD_START, "Start" },
	{ XINPUT_GAMEPAD_LEFT_THUMB, "LS" },
	{ XINPUT_GAMEPAD_RIGHT_THUMB, "RS" },
	{ XINPUT_INFO::TRIGGER_LEFT, "LT" },
	{ XINPUT_INFO::TRIGGER_RIGHT, "RT" },
	{ XINPUT_INFO::STICK_L_LEFT, "LS X-" },
	{ XINPUT_INFO::STICK_L_RIGHT, "LS X+" },
	{ XINPUT_INFO::STICK_L_UP, "LS Y+" },
	{ XINPUT_INFO::STICK_L_DOWN, "LS Y-" },
	{ XINPUT_INFO::STICK_R_LEFT, "RS X-" },
	{ XINPUT_INFO::STICK_R_RIGHT, "RS X+" },
	{ XINPUT_INFO::STICK_R_UP, "RS Y+" },
	{ XINPUT_INFO::STICK_R_DOWN, "RS Y-" }
};

std::string xinput_pad_handler::GetButtonName(u32 button)
{
	if (button_list.find(button) == button_list.end())
	{
		return "FAIL";
	}
	return button_list[button];
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

	if (pos == std::string::npos || device_number >= XINPUT_INFO::MAX_GAMEPADS)
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
			if (button.first < XINPUT_INFO::TRIGGER_LEFT && state.Gamepad.wButtons & button.first)
			{
				return callback(button.first, button.second);
			}
		}
	}
}

xinput_pad_handler::xinput_pad_handler() : library(nullptr), xinputGetState(nullptr), xinputEnable(nullptr), xinputSetState(nullptr), is_init(false)
{
	m_pad_config.cfg_type = "xinput";
	m_pad_config.cfg_name = fs::get_config_dir() + "/config_xinput.yml";

	m_pad_config.left_stick_left.def = XINPUT_INFO::STICK_L_LEFT;
	m_pad_config.left_stick_down.def = XINPUT_INFO::STICK_L_DOWN;
	m_pad_config.left_stick_right.def = XINPUT_INFO::STICK_L_RIGHT;
	m_pad_config.left_stick_up.def = XINPUT_INFO::STICK_L_UP;
	m_pad_config.right_stick_left.def = XINPUT_INFO::STICK_R_LEFT;
	m_pad_config.right_stick_down.def = XINPUT_INFO::STICK_R_DOWN;
	m_pad_config.right_stick_right.def = XINPUT_INFO::STICK_R_RIGHT;
	m_pad_config.right_stick_up.def = XINPUT_INFO::STICK_R_UP;
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
	m_pad_config.r2.def = XINPUT_INFO::TRIGGER_RIGHT;
	m_pad_config.r3.def = XINPUT_GAMEPAD_RIGHT_THUMB;
	m_pad_config.l1.def = XINPUT_GAMEPAD_LEFT_SHOULDER;
	m_pad_config.l2.def = XINPUT_INFO::TRIGGER_LEFT;
	m_pad_config.l3.def = XINPUT_GAMEPAD_LEFT_THUMB;

	m_pad_config.lstickdeadzone.def = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between -32768 and 32767
	m_pad_config.rstickdeadzone.def = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between -32768 and 32767
	m_pad_config.ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD; // between      0 and 255
	m_pad_config.rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD; // between      0 and 255
	m_pad_config.padsquircling.def = 8000;

	m_pad_config.from_default();

	b_has_config = true;
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

	squircle_factor = m_pad_config.padsquircling / 1000.f;
	left_stick_deadzone = m_pad_config.lstickdeadzone;
	right_stick_deadzone = m_pad_config.rstickdeadzone;
	left_trigger_threshold = m_pad_config.ltriggerthreshold;
	right_trigger_threshold = m_pad_config.rtriggerthreshold;

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

std::tuple<u16, u16> xinput_pad_handler::ConvertToSquirclePoint(u16 inX, u16 inY)
{
	// convert inX and Y to a (-1, 1) vector;
	const f32 x = (inX - 127) / 127.f;
	const f32 y = ((inY - 127) / 127.f);

	// compute angle and len of given point to be used for squircle radius
	const f32 angle = std::atan2(y, x);
	const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));

	// now find len/point on the given squircle from our current angle and radius in polar coords
	// https://thatsmaths.com/2016/07/14/squircles/
	const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / squircle_factor) * r;

	// we now have len and angle, convert to cartisian

	const int newX = XINPUT_INFO::Clamp0To255(((newLen * std::cos(angle)) + 1) * 127);
	const int newY = XINPUT_INFO::Clamp0To255(((newLen * std::sin(angle)) + 1) * 127);
	return std::tuple<u16, u16>(newX, newY);
}

void xinput_pad_handler::ThreadProc()
{
	for (u32 index = 0; index != bindings.size(); index++)
	{
		auto padnum = bindings[index].device_number;
		auto pad = bindings[index].pad;
		auto mapping = bindings[index].mapping;

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

			for (DWORD j = 0; j != XINPUT_INFO::XINPUT_GAMEPAD_BUTTONS; ++j)
			{
				bool pressed = state.Gamepad.wButtons & (1 << j);
				int idx = mapping[j];
				pad->m_buttons[idx].m_pressed = pressed;
				pad->m_buttons[idx].m_value = pressed ? 255 : 0;
			}

			for (int i = 6; i < 16; i++)
			{
				if (pad->m_buttons[i].m_pressed)
				{
					SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
					break;
				}
			}

			pad->m_buttons[XINPUT_INFO::XINPUT_GAMEPAD_BUTTONS].m_pressed = state.Gamepad.bLeftTrigger > m_pad_config.ltriggerthreshold;
			pad->m_buttons[XINPUT_INFO::XINPUT_GAMEPAD_BUTTONS].m_value = state.Gamepad.bLeftTrigger;
			pad->m_buttons[XINPUT_INFO::XINPUT_GAMEPAD_BUTTONS + 1].m_pressed = state.Gamepad.bRightTrigger > m_pad_config.rtriggerthreshold;
			pad->m_buttons[XINPUT_INFO::XINPUT_GAMEPAD_BUTTONS + 1].m_value = state.Gamepad.bRightTrigger;

			float LX, LY, RX, RY;

			LX = state.Gamepad.sThumbLX;
			LY = state.Gamepad.sThumbLY;
			RX = state.Gamepad.sThumbRX;
			RY = state.Gamepad.sThumbRY;

			auto normalize_input = [](float& X, float& Y, float deadzone)
			{
				X /= 32767.0f;
				Y /= 32767.0f;
				deadzone /= 32767.0f;

				float mag = sqrtf(X*X + Y*Y);

				if (mag > deadzone)
				{
					float legalRange = 1.0f - deadzone;
					float normalizedMag = std::min(1.0f, (mag - deadzone) / legalRange);
					float scale = normalizedMag / mag;
					X = X * scale;
					Y = Y * scale;
				}
				else
				{
					X = 0;
					Y = 0;
				}
			};

			normalize_input(LX, LY, left_stick_deadzone);
			normalize_input(RX, RY, right_stick_deadzone);

			pad->m_sticks[0].m_value = XINPUT_INFO::ConvertAxis(LX);
			pad->m_sticks[1].m_value = 255 - XINPUT_INFO::ConvertAxis(LY);
			pad->m_sticks[2].m_value = XINPUT_INFO::ConvertAxis(RX);
			pad->m_sticks[3].m_value = 255 - XINPUT_INFO::ConvertAxis(RY);

			if (squircle_factor != 0.f)
			{
				std::tie(pad->m_sticks[0].m_value, pad->m_sticks[1].m_value) = ConvertToSquirclePoint(pad->m_sticks[0].m_value, pad->m_sticks[1].m_value);
				std::tie(pad->m_sticks[2].m_value, pad->m_sticks[3].m_value) = ConvertToSquirclePoint(pad->m_sticks[2].m_value, pad->m_sticks[3].m_value);
			}

			// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
			// The two motors are not the same, and they create different vibration effects.
			XINPUT_VIBRATION vibrate;

			vibrate.wLeftMotorSpeed = pad->m_vibrateMotors[0].m_value * 257;  // between 0 to 65535
			vibrate.wRightMotorSpeed = pad->m_vibrateMotors[1].m_value * 257; // between 0 to 65535

			(*xinputSetState)(padnum, &vibrate);

			break;
		}
	}
}

std::vector<std::string> xinput_pad_handler::ListDevices()
{
	std::vector<std::string> xinput_pads_list;

	if (!Init()) return xinput_pads_list;

	for (DWORD i = 0; i < XINPUT_INFO::MAX_GAMEPADS; i++)
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

	if (pos == std::string::npos || device_number >= XINPUT_INFO::MAX_GAMEPADS) return false;

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
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, XINPUT_INFO::XINPUT_GAMEPAD_GUIDE, 0x100/*CELL_PAD_CTRL_PS*/);// TODO: PS button support
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, 0, 0x0); // Reserved
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.cross,    CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.circle,   CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.square,   CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.l2,       CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, m_pad_config.r2,       CELL_PAD_CTRL_R2);

	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X,  0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y,  0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, 0, 0);
	pad->m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, 0, 0);

	pad->m_vibrateMotors.emplace_back(true, 0);
	pad->m_vibrateMotors.emplace_back(false, 0);

	auto getButton = [](u32 val)
	{
		switch (val)
		{
		case XINPUT_GAMEPAD_DPAD_UP:        return 0;
		case XINPUT_GAMEPAD_DPAD_DOWN:      return 1;
		case XINPUT_GAMEPAD_DPAD_LEFT:      return 2;
		case XINPUT_GAMEPAD_DPAD_RIGHT:     return 3;
		case XINPUT_GAMEPAD_START:          return 4;
		case XINPUT_GAMEPAD_BACK:           return 5;
		case XINPUT_GAMEPAD_LEFT_THUMB:     return 6;
		case XINPUT_GAMEPAD_RIGHT_THUMB:    return 7;
		case XINPUT_GAMEPAD_LEFT_SHOULDER:  return 8;
		case XINPUT_GAMEPAD_RIGHT_SHOULDER: return 9;
		case XINPUT_GAMEPAD_A:              return 12;
		case XINPUT_GAMEPAD_B:              return 13;
		case XINPUT_GAMEPAD_X:              return 14;
		case XINPUT_GAMEPAD_Y:              return 15;
		default:                            return -1;
		}
	};

	std::vector<int> mapping;
	mapping.push_back(getButton(m_pad_config.up));
	mapping.push_back(getButton(m_pad_config.down));
	mapping.push_back(getButton(m_pad_config.left));
	mapping.push_back(getButton(m_pad_config.right));
	mapping.push_back(getButton(m_pad_config.start));
	mapping.push_back(getButton(m_pad_config.select));
	mapping.push_back(getButton(m_pad_config.l3));
	mapping.push_back(getButton(m_pad_config.r3));
	mapping.push_back(getButton(m_pad_config.l1));
	mapping.push_back(getButton(m_pad_config.r1));
	mapping.push_back(10);
	mapping.push_back(11);
	mapping.push_back(getButton(m_pad_config.cross));
	mapping.push_back(getButton(m_pad_config.circle));
	mapping.push_back(getButton(m_pad_config.square));
	mapping.push_back(getButton(m_pad_config.triangle));

	bindings.emplace_back(PAD_INFO{device_number, pad, mapping});

	return true;
}

#endif
