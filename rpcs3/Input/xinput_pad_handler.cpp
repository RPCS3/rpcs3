
#ifdef _WIN32
#include "stdafx.h"
#include "xinput_pad_handler.h"
#include "Emu/Io/pad_config.h"

namespace XINPUT_INFO
{
	const DWORD GUIDE_BUTTON = 0x0400;
	const LPCWSTR LIBRARY_FILENAMES[] = {
		L"xinput1_3.dll", // Prioritizing 1_3 because of SCP
		L"xinput1_4.dll",
		L"xinput9_1_0.dll"
	};
} // namespace XINPUT_INFO

xinput_pad_handler::xinput_pad_handler() : PadHandlerBase(pad_handler::xinput)
{
	// Unique names for the config files and our pad settings dialog
	button_list =
	{
		{ XInputKeyCodes::A,      "A" },
		{ XInputKeyCodes::B,      "B" },
		{ XInputKeyCodes::X,      "X" },
		{ XInputKeyCodes::Y,      "Y" },
		{ XInputKeyCodes::Left,   "Left" },
		{ XInputKeyCodes::Right,  "Right" },
		{ XInputKeyCodes::Up,     "Up" },
		{ XInputKeyCodes::Down,   "Down" },
		{ XInputKeyCodes::LB,     "LB" },
		{ XInputKeyCodes::RB,     "RB" },
		{ XInputKeyCodes::Back,   "Back" },
		{ XInputKeyCodes::Start,  "Start" },
		{ XInputKeyCodes::LS,     "LS" },
		{ XInputKeyCodes::RS,     "RS" },
		{ XInputKeyCodes::Guide,  "Guide" },
		{ XInputKeyCodes::LT,     "LT" },
		{ XInputKeyCodes::RT,     "RT" },
		{ XInputKeyCodes::LSXNeg, "LS X-" },
		{ XInputKeyCodes::LSXPos, "LS X+" },
		{ XInputKeyCodes::LSYPos, "LS Y+" },
		{ XInputKeyCodes::LSYNeg, "LS Y-" },
		{ XInputKeyCodes::RSXNeg, "RS X-" },
		{ XInputKeyCodes::RSXPos, "RS X+" },
		{ XInputKeyCodes::RSYPos, "RS Y+" },
		{ XInputKeyCodes::RSYNeg, "RS Y-" }
	};

	init_configs();

	// Define border values
	thumb_min = -32768;
	thumb_max = 32767;
	trigger_min = 0;
	trigger_max = 255;
	vibration_min = 0;
	vibration_max = 65535;

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;

	m_name_string = "XInput Pad #";
	m_max_devices = XUSER_MAX_COUNT;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

xinput_pad_handler::~xinput_pad_handler()
{
	if (library)
	{
		FreeLibrary(library);
		library = nullptr;
		xinputGetExtended = nullptr;
		xinputGetState = nullptr;
		xinputSetState = nullptr;
		xinputGetBatteryInformation = nullptr;
	}
}

void xinput_pad_handler::init_config(pad_config* cfg, const std::string& name)
{
	// Set this profile's save location
	cfg->cfg_name = name;

	// Set default button mapping
	cfg->ls_left.def  = button_list.at(XInputKeyCodes::LSXNeg);
	cfg->ls_down.def  = button_list.at(XInputKeyCodes::LSYNeg);
	cfg->ls_right.def = button_list.at(XInputKeyCodes::LSXPos);
	cfg->ls_up.def    = button_list.at(XInputKeyCodes::LSYPos);
	cfg->rs_left.def  = button_list.at(XInputKeyCodes::RSXNeg);
	cfg->rs_down.def  = button_list.at(XInputKeyCodes::RSYNeg);
	cfg->rs_right.def = button_list.at(XInputKeyCodes::RSXPos);
	cfg->rs_up.def    = button_list.at(XInputKeyCodes::RSYPos);
	cfg->start.def    = button_list.at(XInputKeyCodes::Start);
	cfg->select.def   = button_list.at(XInputKeyCodes::Back);
	cfg->ps.def       = button_list.at(XInputKeyCodes::Guide);
	cfg->square.def   = button_list.at(XInputKeyCodes::X);
	cfg->cross.def    = button_list.at(XInputKeyCodes::A);
	cfg->circle.def   = button_list.at(XInputKeyCodes::B);
	cfg->triangle.def = button_list.at(XInputKeyCodes::Y);
	cfg->left.def     = button_list.at(XInputKeyCodes::Left);
	cfg->down.def     = button_list.at(XInputKeyCodes::Down);
	cfg->right.def    = button_list.at(XInputKeyCodes::Right);
	cfg->up.def       = button_list.at(XInputKeyCodes::Up);
	cfg->r1.def       = button_list.at(XInputKeyCodes::RB);
	cfg->r2.def       = button_list.at(XInputKeyCodes::RT);
	cfg->r3.def       = button_list.at(XInputKeyCodes::RS);
	cfg->l1.def       = button_list.at(XInputKeyCodes::LB);
	cfg->l2.def       = button_list.at(XInputKeyCodes::LT);
	cfg->l3.def       = button_list.at(XInputKeyCodes::LS);

	// Set default misc variables
	cfg->lstickdeadzone.def    = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between 0 and 32767
	cfg->rstickdeadzone.def    = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between 0 and 32767
	cfg->ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->padsquircling.def     = 8000;

	// apply defaults
	cfg->from_default();
}

void xinput_pad_handler::SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32/* r*/, s32/* g*/, s32/* b*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	int device_number = GetDeviceNumber(padId);
	if (device_number < 0)
		return;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects.
	XINPUT_VIBRATION vibrate;

	vibrate.wLeftMotorSpeed = largeMotor;  // between 0 to 65535
	vibrate.wRightMotorSpeed = smallMotor; // between 0 to 65535

	(*xinputSetState)(static_cast<u32>(device_number), &vibrate);
}

int xinput_pad_handler::GetDeviceNumber(const std::string& padId)
{
	if (!Init())
		return -1;

	size_t pos = padId.find(m_name_string);
	if (pos == umax)
		return -1;

	int device_number = std::stoul(padId.substr(pos + 12)) - 1; // Controllers 1-n in GUI
	if (device_number >= XUSER_MAX_COUNT)
		return -1;

	return device_number;
}

std::unordered_map<u64, u16> xinput_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	PadButtonValues values;
	auto dev = std::static_pointer_cast<XInputDevice>(device);
	if (!dev || dev->state != ERROR_SUCCESS) // the state has to be aquired with update_connection before calling this function
		return values;

	// Try SCP first, if it fails for that pad then try normal XInput
	if (dev->is_scp_device)
	{
		return get_button_values_scp(dev->state_scp);
	}

	return get_button_values_base(dev->state_base);
}

xinput_pad_handler::PadButtonValues xinput_pad_handler::get_button_values_base(const XINPUT_STATE& state)
{
	PadButtonValues values;

	// Triggers
	values[XInputKeyCodes::LT] = state.Gamepad.bLeftTrigger;
	values[XInputKeyCodes::RT] = state.Gamepad.bRightTrigger;

	// Sticks
	int lx = state.Gamepad.sThumbLX;
	int ly = state.Gamepad.sThumbLY;
	int rx = state.Gamepad.sThumbRX;
	int ry = state.Gamepad.sThumbRY;

	// Left Stick X Axis
	values[XInputKeyCodes::LSXNeg] = lx < 0 ? abs(lx) - 1 : 0;
	values[XInputKeyCodes::LSXPos] = lx > 0 ? lx : 0;

	// Left Stick Y Axis
	values[XInputKeyCodes::LSYNeg] = ly < 0 ? abs(ly) - 1 : 0;
	values[XInputKeyCodes::LSYPos] = ly > 0 ? ly : 0;

	// Right Stick X Axis
	values[XInputKeyCodes::RSXNeg] = rx < 0 ? abs(rx) - 1 : 0;
	values[XInputKeyCodes::RSXPos] = rx > 0 ? rx : 0;

	// Right Stick Y Axis
	values[XInputKeyCodes::RSYNeg] = ry < 0 ? abs(ry) - 1 : 0;
	values[XInputKeyCodes::RSYPos] = ry > 0 ? ry : 0;

	// Buttons
	WORD buttons = state.Gamepad.wButtons;

	// A, B, X, Y
	values[XInputKeyCodes::A] = buttons & XINPUT_GAMEPAD_A ? 255 : 0;
	values[XInputKeyCodes::B] = buttons & XINPUT_GAMEPAD_B ? 255 : 0;
	values[XInputKeyCodes::X] = buttons & XINPUT_GAMEPAD_X ? 255 : 0;
	values[XInputKeyCodes::Y] = buttons & XINPUT_GAMEPAD_Y ? 255 : 0;

	// D-Pad
	values[XInputKeyCodes::Left]  = buttons & XINPUT_GAMEPAD_DPAD_LEFT ? 255 : 0;
	values[XInputKeyCodes::Right] = buttons & XINPUT_GAMEPAD_DPAD_RIGHT ? 255 : 0;
	values[XInputKeyCodes::Up]    = buttons & XINPUT_GAMEPAD_DPAD_UP ? 255 : 0;
	values[XInputKeyCodes::Down]  = buttons & XINPUT_GAMEPAD_DPAD_DOWN ? 255 : 0;

	// LB, RB, LS, RS
	values[XInputKeyCodes::LB] = buttons & XINPUT_GAMEPAD_LEFT_SHOULDER ? 255 : 0;
	values[XInputKeyCodes::RB] = buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? 255 : 0;
	values[XInputKeyCodes::LS] = buttons & XINPUT_GAMEPAD_LEFT_THUMB ? 255 : 0;
	values[XInputKeyCodes::RS] = buttons & XINPUT_GAMEPAD_RIGHT_THUMB ? 255 : 0;

	// Start, Back, Guide
	values[XInputKeyCodes::Start] = buttons & XINPUT_GAMEPAD_START ? 255 : 0;
	values[XInputKeyCodes::Back]  = buttons & XINPUT_GAMEPAD_BACK ? 255 : 0;
	values[XInputKeyCodes::Guide] = buttons & XINPUT_INFO::GUIDE_BUTTON ? 255 : 0;

	return values;
}

xinput_pad_handler::PadButtonValues xinput_pad_handler::get_button_values_scp(const SCP_EXTN& state)
{
	PadButtonValues values;

	// Triggers
	values[xinput_pad_handler::XInputKeyCodes::LT] = static_cast<u16>(state.SCP_L2 * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::RT] = static_cast<u16>(state.SCP_R2 * 255.0f);

	// Sticks
	float lx = state.SCP_LX;
	float ly = state.SCP_LY;
	float rx = state.SCP_RX;
	float ry = state.SCP_RY;

	// Left Stick X Axis
	values[xinput_pad_handler::XInputKeyCodes::LSXNeg] = lx < 0.0f ? static_cast<u16>(lx * -32768.0f) : 0;
	values[xinput_pad_handler::XInputKeyCodes::LSXPos] = lx > 0.0f ? static_cast<u16>(lx * 32767.0f) : 0;

	// Left Stick Y Axis
	values[xinput_pad_handler::XInputKeyCodes::LSYNeg] = ly < 0.0f ? static_cast<u16>(ly * -32768.0f) : 0;
	values[xinput_pad_handler::XInputKeyCodes::LSYPos] = ly > 0.0f ? static_cast<u16>(ly * 32767.0f) : 0;

	// Right Stick X Axis
	values[xinput_pad_handler::XInputKeyCodes::RSXNeg] = rx < 0.0f ? static_cast<u16>(rx * -32768.0f) : 0;
	values[xinput_pad_handler::XInputKeyCodes::RSXPos] = rx > 0.0f ? static_cast<u16>(rx * 32767.0f) : 0;

	// Right Stick Y Axis
	values[xinput_pad_handler::XInputKeyCodes::RSYNeg] = ry < 0.0f ? static_cast<u16>(ry * -32768.0f) : 0;
	values[xinput_pad_handler::XInputKeyCodes::RSYPos] = ry > 0.0f ? static_cast<u16>(ry * 32767.0f) : 0;

	// A, B, X, Y
	values[xinput_pad_handler::XInputKeyCodes::A] = static_cast<u16>(state.SCP_X * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::B] = static_cast<u16>(state.SCP_C * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::X] = static_cast<u16>(state.SCP_S * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Y] = static_cast<u16>(state.SCP_T * 255.0f);

	// D-Pad
	values[xinput_pad_handler::XInputKeyCodes::Left]  = static_cast<u16>(state.SCP_LEFT * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Right] = static_cast<u16>(state.SCP_RIGHT * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Up]    = static_cast<u16>(state.SCP_UP * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Down]  = static_cast<u16>(state.SCP_DOWN * 255.0f);

	// LB, RB, LS, RS
	values[xinput_pad_handler::XInputKeyCodes::LB] = static_cast<u16>(state.SCP_L1 * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::RB] = static_cast<u16>(state.SCP_R1 * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::LS] = static_cast<u16>(state.SCP_L3 * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::RS] = static_cast<u16>(state.SCP_R3 * 255.0f);

	// Start, Back, Guide
	values[xinput_pad_handler::XInputKeyCodes::Start] = static_cast<u16>(state.SCP_START * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Back]  = static_cast<u16>(state.SCP_SELECT * 255.0f);
	values[xinput_pad_handler::XInputKeyCodes::Guide] = static_cast<u16>(state.SCP_PS * 255.0f);

	return values;
}

pad_preview_values xinput_pad_handler::get_preview_values(std::unordered_map<u64, u16> data)
{
	return { data[LT], data[RT], data[LSXPos] - data[LSXNeg], data[LSYPos] - data[LSYNeg], data[RSXPos] - data[RSXNeg], data[RSYPos] - data[RSYNeg] };
}

bool xinput_pad_handler::Init()
{
	if (is_init)
		return true;

	for (auto it : XINPUT_INFO::LIBRARY_FILENAMES)
	{
		library = LoadLibrary(it);
		if (library)
		{
			xinputGetExtended = reinterpret_cast<PFN_XINPUTGETEXTENDED>(GetProcAddress(library, "XInputGetExtended")); // Optional
			xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, reinterpret_cast<LPCSTR>(100)));
			if (!xinputGetState)
				xinputGetState = reinterpret_cast<PFN_XINPUTGETSTATE>(GetProcAddress(library, "XInputGetState"));

			xinputSetState = reinterpret_cast<PFN_XINPUTSETSTATE>(GetProcAddress(library, "XInputSetState"));
			xinputGetBatteryInformation = reinterpret_cast<PFN_XINPUTGETBATTERYINFORMATION>(GetProcAddress(library, "XInputGetBatteryInformation"));

			if (xinputGetState && xinputSetState && xinputGetBatteryInformation)
			{
				is_init = true;
				break;
			}

			FreeLibrary(library);
			library = nullptr;
			xinputGetExtended = nullptr;
			xinputGetState = nullptr;
			xinputSetState = nullptr;
			xinputGetBatteryInformation = nullptr;
		}
	}

	if (!is_init)
		return false;

	return true;
}

std::vector<std::string> xinput_pad_handler::ListDevices()
{
	std::vector<std::string> xinput_pads_list;

	if (!Init())
		return xinput_pads_list;

	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
	{
		DWORD result = ERROR_NOT_CONNECTED;

		// Try SCP first, if it fails for that pad then try normal XInput
		if (xinputGetExtended)
		{
			SCP_EXTN state;
			result = xinputGetExtended(i, &state);
		}

		if (result != ERROR_SUCCESS)
		{
			XINPUT_STATE state;
			result = xinputGetState(i, &state);
		}

		if (result == ERROR_SUCCESS)
			xinput_pads_list.push_back(m_name_string + std::to_string(i + 1)); // Controllers 1-n in GUI
	}
	return xinput_pads_list;
}

std::shared_ptr<PadDevice> xinput_pad_handler::get_device(const std::string& device)
{
	// Convert device string to u32 representing xinput device number
	int device_number = GetDeviceNumber(device);
	if (device_number < 0)
		return nullptr;

	std::shared_ptr<XInputDevice> x_device = std::make_shared<XInputDevice>();
	x_device->deviceNumber = static_cast<u32>(device_number);

	return x_device;
}

bool xinput_pad_handler::get_is_left_trigger(u64 keyCode)
{
	return keyCode == XInputKeyCodes::LT;
}

bool xinput_pad_handler::get_is_right_trigger(u64 keyCode)
{
	return keyCode == XInputKeyCodes::RT;
}

bool xinput_pad_handler::get_is_left_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case XInputKeyCodes::LSXNeg:
	case XInputKeyCodes::LSXPos:
	case XInputKeyCodes::LSYPos:
	case XInputKeyCodes::LSYNeg:
		return true;
	default:
		return false;
	}
}

bool xinput_pad_handler::get_is_right_stick(u64 keyCode)
{
	switch (keyCode)
	{
	case XInputKeyCodes::RSXNeg:
	case XInputKeyCodes::RSXPos:
	case XInputKeyCodes::RSYPos:
	case XInputKeyCodes::RSYNeg:
		return true;
	default:
		return false;
	}
}

PadHandlerBase::connection xinput_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	auto dev = std::static_pointer_cast<XInputDevice>(device);
	if (!dev)
		return connection::disconnected;

	dev->state = ERROR_NOT_CONNECTED;
	dev->state_scp = {};
	dev->state_base = {};

	// Try SCP first, if it fails for that pad then try normal XInput
	if (xinputGetExtended)
		dev->state = xinputGetExtended(dev->deviceNumber, &dev->state_scp);

	dev->is_scp_device = dev->state == ERROR_SUCCESS;

	if (!dev->is_scp_device)
		dev->state = xinputGetState(dev->deviceNumber, &dev->state_base);

	if (dev->state == ERROR_SUCCESS)
		return connection::connected;

	return connection::disconnected;
}

void xinput_pad_handler::get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dev = std::static_pointer_cast<XInputDevice>(device);
	if (!dev || !pad)
		return;

	auto padnum = dev->deviceNumber;

	// Receive Battery Info. If device is not on cable, get battery level, else assume full
	XINPUT_BATTERY_INFORMATION battery_info;
	(*xinputGetBatteryInformation)(padnum, BATTERY_DEVTYPE_GAMEPAD, &battery_info);
	pad->m_cable_state = battery_info.BatteryType == BATTERY_TYPE_WIRED ? 1 : 0;
	pad->m_battery_level = pad->m_cable_state ? BATTERY_LEVEL_FULL : battery_info.BatteryLevel;
}

void xinput_pad_handler::apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad)
{
	auto dev = std::static_pointer_cast<XInputDevice>(device);
	if (!dev || !pad)
		return;

	auto padnum = dev->deviceNumber;
	auto profile = dev->config;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects. Values range between 0 to 65535.
	size_t idx_l = profile->switch_vibration_motors ? 1 : 0;
	size_t idx_s = profile->switch_vibration_motors ? 0 : 1;

	u16 speed_large = profile->enable_vibration_motor_large ? pad->m_vibrateMotors[idx_l].m_value : static_cast<u16>(vibration_min);
	u16 speed_small = profile->enable_vibration_motor_small ? pad->m_vibrateMotors[idx_s].m_value : static_cast<u16>(vibration_min);

	dev->newVibrateData |= dev->largeVibrate != speed_large || dev->smallVibrate != speed_small;

	dev->largeVibrate = speed_large;
	dev->smallVibrate = speed_small;

	// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse. So I'll use 20ms to be on the safe side. No lag was noticable.
	if (dev->newVibrateData && (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - dev->last_vibration) > 20ms))
	{
		XINPUT_VIBRATION vibrate;
		vibrate.wLeftMotorSpeed = speed_large * 257;
		vibrate.wRightMotorSpeed = speed_small * 257;

		if ((*xinputSetState)(padnum, &vibrate) == ERROR_SUCCESS)
		{
			dev->newVibrateData = false;
			dev->last_vibration = std::chrono::high_resolution_clock::now();
		}
	}
}

#endif
