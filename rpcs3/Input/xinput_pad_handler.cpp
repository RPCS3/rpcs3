
#ifdef _WIN32
#include "stdafx.h"
#include "xinput_pad_handler.h"
#include "Emu/Io/pad_config.h"
#include "util/dyn_lib.hpp"

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
		{ XInputKeyCodes::None,   ""  },
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
		{ XInputKeyCodes::LT_Pos, "LT+" },
		{ XInputKeyCodes::LT_Neg, "LT-" },
		{ XInputKeyCodes::RT_Pos, "RT+" },
		{ XInputKeyCodes::RT_Neg, "RT-" },
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
	thumb_max = 32767;
	trigger_min = 0;
	trigger_max = 255;

	// set capabilities
	b_has_config = true;
	b_has_rumble = true;
	b_has_deadzones = true;
	b_has_battery = true;
	b_has_battery_led = false;
	b_has_orientation = false;

	m_name_string = "XInput Pad #";
	m_max_devices = XUSER_MAX_COUNT;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

xinput_pad_handler::~xinput_pad_handler()
{
	if (library)
	{
		xinputGetExtended = nullptr;
		xinputGetCustomData = nullptr;
		xinputGetState = nullptr;
		xinputSetState = nullptr;
		xinputGetBatteryInformation = nullptr;
	}
}

void xinput_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping
	cfg->ls_left.def  = ::at32(button_list, XInputKeyCodes::LSXNeg);
	cfg->ls_down.def  = ::at32(button_list, XInputKeyCodes::LSYNeg);
	cfg->ls_right.def = ::at32(button_list, XInputKeyCodes::LSXPos);
	cfg->ls_up.def    = ::at32(button_list, XInputKeyCodes::LSYPos);
	cfg->rs_left.def  = ::at32(button_list, XInputKeyCodes::RSXNeg);
	cfg->rs_down.def  = ::at32(button_list, XInputKeyCodes::RSYNeg);
	cfg->rs_right.def = ::at32(button_list, XInputKeyCodes::RSXPos);
	cfg->rs_up.def    = ::at32(button_list, XInputKeyCodes::RSYPos);
	cfg->start.def    = ::at32(button_list, XInputKeyCodes::Start);
	cfg->select.def   = ::at32(button_list, XInputKeyCodes::Back);
	cfg->ps.def       = ::at32(button_list, XInputKeyCodes::Guide);
	cfg->square.def   = ::at32(button_list, XInputKeyCodes::X);
	cfg->cross.def    = ::at32(button_list, XInputKeyCodes::A);
	cfg->circle.def   = ::at32(button_list, XInputKeyCodes::B);
	cfg->triangle.def = ::at32(button_list, XInputKeyCodes::Y);
	cfg->left.def     = ::at32(button_list, XInputKeyCodes::Left);
	cfg->down.def     = ::at32(button_list, XInputKeyCodes::Down);
	cfg->right.def    = ::at32(button_list, XInputKeyCodes::Right);
	cfg->up.def       = ::at32(button_list, XInputKeyCodes::Up);
	cfg->r1.def       = ::at32(button_list, XInputKeyCodes::RB);
	cfg->r2.def       = ::at32(button_list, XInputKeyCodes::RT);
	cfg->r3.def       = ::at32(button_list, XInputKeyCodes::RS);
	cfg->l1.def       = ::at32(button_list, XInputKeyCodes::LB);
	cfg->l2.def       = ::at32(button_list, XInputKeyCodes::LT);
	cfg->l3.def       = ::at32(button_list, XInputKeyCodes::LS);

	cfg->pressure_intensity_button.def = ::at32(button_list, XInputKeyCodes::None);
	cfg->analog_limiter_button.def = ::at32(button_list, XInputKeyCodes::None);
	cfg->orientation_reset_button.def = ::at32(button_list, XInputKeyCodes::None);

	// Set default misc variables
	cfg->lstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->rstick_anti_deadzone.def = static_cast<u32>(0.13 * thumb_max); // 13%
	cfg->lstickdeadzone.def    = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;  // between 0 and 32767
	cfg->rstickdeadzone.def    = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE; // between 0 and 32767
	cfg->ltriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->rtriggerthreshold.def = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;    // between 0 and 255
	cfg->lpadsquircling.def    = 8000;
	cfg->rpadsquircling.def    = 8000;

	// apply defaults
	cfg->from_default();
}

void xinput_pad_handler::SetPadData(const std::string& padId, u8 /*player_id*/, u8 large_motor, u8 small_motor, s32/* r*/, s32/* g*/, s32/* b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/)
{
	const int device_number = GetDeviceNumber(padId);
	if (device_number < 0)
		return;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects.
	XINPUT_VIBRATION vibrate
	{
		.wLeftMotorSpeed = static_cast<u16>(large_motor * 257), // between 0 to 65535
		.wRightMotorSpeed = static_cast<u16>(small_motor * 257) // between 0 to 65535
	};

	xinputSetState(static_cast<u32>(device_number), &vibrate);
}

u32 xinput_pad_handler::get_battery_level(const std::string& padId)
{
	const int device_number = GetDeviceNumber(padId);
	if (device_number < 0)
		return 0;

	// Receive Battery Info. If device is not on cable, get battery level, else assume full.
	XINPUT_BATTERY_INFORMATION battery_info;
	xinputGetBatteryInformation(device_number, BATTERY_DEVTYPE_GAMEPAD, &battery_info);

	switch (battery_info.BatteryType)
	{
	case BATTERY_TYPE_DISCONNECTED:
		return 0;
	case BATTERY_TYPE_WIRED:
		return 100;
	default:
		break;
	}

	switch (battery_info.BatteryLevel)
	{
	case BATTERY_LEVEL_EMPTY:
		return 0;
	case BATTERY_LEVEL_LOW:
		return 33;
	case BATTERY_LEVEL_MEDIUM:
		return 66;
	case BATTERY_LEVEL_FULL:
		return 100;
	default:
		return 0;
	}
}

int xinput_pad_handler::GetDeviceNumber(const std::string& padId)
{
	if (!Init())
		return -1;

	const usz pos = padId.find(m_name_string);
	if (pos == umax)
		return -1;

	const int device_number = std::stoul(padId.substr(pos + 12)) - 1; // Controllers 1-n in GUI
	if (device_number >= XUSER_MAX_COUNT)
		return -1;

	return device_number;
}

std::unordered_map<u64, u16> xinput_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	PadButtonValues values;
	XInputDevice* dev = static_cast<XInputDevice*>(device.get());
	if (!dev || dev->state != ERROR_SUCCESS) // the state has to be aquired with update_connection before calling this function
		return values;

	// Try SCP first, if it fails for that pad then try normal XInput
	if (dev->is_scp_device)
	{
		return get_button_values_scp(dev->state_scp, m_trigger_recognition_mode);
	}

	return get_button_values_base(dev->state_base, m_trigger_recognition_mode);
}

xinput_pad_handler::PadButtonValues xinput_pad_handler::get_button_values_base(const XINPUT_STATE& state, trigger_recognition_mode trigger_mode)
{
	PadButtonValues values;

	// Triggers
	if (trigger_mode == trigger_recognition_mode::any || trigger_mode == trigger_recognition_mode::one_directional)
	{
		values[XInputKeyCodes::LT] = state.Gamepad.bLeftTrigger;
		values[XInputKeyCodes::RT] = state.Gamepad.bRightTrigger;
	}

	if (trigger_mode == trigger_recognition_mode::any || trigger_mode == trigger_recognition_mode::two_directional)
	{
		const float lTrigger = state.Gamepad.bLeftTrigger / 255.0f;
		const float rTrigger = state.Gamepad.bRightTrigger / 255.0f;

		values[XInputKeyCodes::LT_Pos] = static_cast<u16>(lTrigger > 0.5f ? std::clamp((lTrigger - 0.5f) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
		values[XInputKeyCodes::LT_Neg] = static_cast<u16>(lTrigger < 0.5f ? std::clamp((0.5f - lTrigger) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);

		values[XInputKeyCodes::RT_Pos] = static_cast<u16>(rTrigger > 0.5f ? std::clamp((rTrigger - 0.5f) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
		values[XInputKeyCodes::RT_Neg] = static_cast<u16>(rTrigger < 0.5f ? std::clamp((0.5f - rTrigger) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
	}

	// Sticks
	const int lx = state.Gamepad.sThumbLX;
	const int ly = state.Gamepad.sThumbLY;
	const int rx = state.Gamepad.sThumbRX;
	const int ry = state.Gamepad.sThumbRY;

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
	const WORD buttons = state.Gamepad.wButtons;

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

xinput_pad_handler::PadButtonValues xinput_pad_handler::get_button_values_scp(const SCP_EXTN& state, trigger_recognition_mode trigger_mode)
{
	PadButtonValues values;

	// Triggers
	if (trigger_mode == trigger_recognition_mode::any || trigger_mode == trigger_recognition_mode::one_directional)
	{
		values[xinput_pad_handler::XInputKeyCodes::LT] = static_cast<u16>(state.SCP_L2 * 255.0f);
		values[xinput_pad_handler::XInputKeyCodes::RT] = static_cast<u16>(state.SCP_R2 * 255.0f);
	}

	if (trigger_mode == trigger_recognition_mode::any || trigger_mode == trigger_recognition_mode::two_directional)
	{
		values[XInputKeyCodes::LT_Pos] = static_cast<u16>(state.SCP_L2 > 0.5f ? std::clamp((state.SCP_L2 - 0.5f) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
		values[XInputKeyCodes::LT_Neg] = static_cast<u16>(state.SCP_L2 < 0.5f ? std::clamp((0.5f - state.SCP_L2) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);

		values[XInputKeyCodes::RT_Pos] = static_cast<u16>(state.SCP_R2 > 0.5f ? std::clamp((state.SCP_R2 - 0.5f) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
		values[XInputKeyCodes::RT_Neg] = static_cast<u16>(state.SCP_R2 < 0.5f ? std::clamp((0.5f - state.SCP_R2) * 2.0f * 255.0f, 0.0f, 255.0f) : 0.0f);
	}

	// Sticks
	const float lx = state.SCP_LX;
	const float ly = state.SCP_LY;
	const float rx = state.SCP_RX;
	const float ry = state.SCP_RY;

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

pad_preview_values xinput_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data)
{
	return {
		::at32(data, LT),
		::at32(data, RT),
		::at32(data, LSXPos) - ::at32(data, LSXNeg),
		::at32(data, LSYPos) - ::at32(data, LSYNeg),
		::at32(data, RSXPos) - ::at32(data, RSXNeg),
		::at32(data, RSYPos) - ::at32(data, RSYNeg)
	};
}

bool xinput_pad_handler::Init()
{
	if (m_is_init)
		return true;

	for (auto it : XINPUT_INFO::LIBRARY_FILENAMES)
	{
		library.load(it);
		if (library)
		{
			xinputGetExtended = library.get<PFN_XINPUTGETEXTENDED>("XInputGetExtended");       // Optional
			xinputGetCustomData = library.get<PFN_XINPUTGETCUSTOMDATA>("XInputGetCustomData"); // Optional
			xinputGetState = library.get<PFN_XINPUTGETSTATE>(reinterpret_cast<LPCSTR>(100));
			if (!xinputGetState)
				xinputGetState = library.get<PFN_XINPUTGETSTATE>("XInputGetState");

			xinputSetState = library.get<PFN_XINPUTSETSTATE>("XInputSetState");
			xinputGetBatteryInformation = library.get<PFN_XINPUTGETBATTERYINFORMATION>("XInputGetBatteryInformation");

			if (xinputGetState && xinputSetState && xinputGetBatteryInformation)
			{
				m_is_init = true;
				break;
			}

			xinputGetExtended = nullptr;
			xinputGetCustomData = nullptr;
			xinputGetState = nullptr;
			xinputSetState = nullptr;
			xinputGetBatteryInformation = nullptr;
		}
	}

	b_has_orientation = !!xinputGetCustomData;

	if (!m_is_init)
		return false;

	return true;
}

std::vector<pad_list_entry> xinput_pad_handler::list_devices()
{
	std::vector<pad_list_entry> xinput_pads_list;

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
			xinput_pads_list.emplace_back(m_name_string + std::to_string(i + 1), false); // Controllers 1-n in GUI
	}
	return xinput_pads_list;
}

std::shared_ptr<PadDevice> xinput_pad_handler::get_device(const std::string& device)
{
	// Convert device string to u32 representing xinput device number
	const int device_number = GetDeviceNumber(device);
	if (device_number < 0)
		return nullptr;

	std::shared_ptr<XInputDevice> dev = std::make_shared<XInputDevice>();
	dev->deviceNumber = static_cast<u32>(device_number);

	return dev;
}

bool xinput_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == XInputKeyCodes::LT;
}

bool xinput_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
{
	return keyCode == XInputKeyCodes::RT;
}

bool xinput_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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

bool xinput_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keyCode)
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
	XInputDevice* dev = static_cast<XInputDevice*>(device.get());
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

void xinput_pad_handler::get_extended_info(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	XInputDevice* dev = static_cast<XInputDevice*>(device.get());
	if (!dev || !pad)
		return;

	const auto padnum = dev->deviceNumber;

	// Receive Battery Info. If device is not on cable, get battery level, else assume full
	XINPUT_BATTERY_INFORMATION battery_info;
	xinputGetBatteryInformation(padnum, BATTERY_DEVTYPE_GAMEPAD, &battery_info);
	pad->m_cable_state = battery_info.BatteryType == BATTERY_TYPE_WIRED ? 1 : 0;
	pad->m_battery_level = pad->m_cable_state ? BATTERY_LEVEL_FULL : battery_info.BatteryLevel;

	if (xinputGetCustomData)
	{
		SCP_DS3_ACCEL sensors;
		if (xinputGetCustomData(dev->deviceNumber, 0, &sensors) == ERROR_SUCCESS)
		{
			pad->m_sensors[0].m_value = sensors.SCP_ACCEL_X;
			pad->m_sensors[1].m_value = sensors.SCP_ACCEL_Y;
			pad->m_sensors[2].m_value = sensors.SCP_ACCEL_Z;
			pad->m_sensors[3].m_value = sensors.SCP_GYRO;

			set_raw_orientation(*pad);
		}
	}
}

void xinput_pad_handler::apply_pad_data(const pad_ensemble& binding)
{
	const auto& device = binding.device;
	const auto& pad = binding.pad;

	XInputDevice* dev = static_cast<XInputDevice*>(device.get());
	if (!dev || !pad)
		return;

	const auto padnum = dev->deviceNumber;
	const auto cfg = dev->config;

	// The left motor is the low-frequency rumble motor. The right motor is the high-frequency rumble motor.
	// The two motors are not the same, and they create different vibration effects. Values range between 0 to 65535.
	const u8 speed_large = cfg->get_large_motor_speed(pad->m_vibrateMotors);
	const u8 speed_small = cfg->get_small_motor_speed(pad->m_vibrateMotors);

	dev->new_output_data |= dev->large_motor != speed_large || dev->small_motor != speed_small;

	dev->large_motor = speed_large;
	dev->small_motor = speed_small;

	const auto now = steady_clock::now();
	const auto elapsed = now - dev->last_output;

	// XBox One Controller can't handle faster vibration updates than ~10ms. Elite is even worse. So I'll use 20ms to be on the safe side. No lag was noticable.
	if ((dev->new_output_data && elapsed > 20ms) || elapsed > min_output_interval)
	{
		XINPUT_VIBRATION vibrate
		{
			.wLeftMotorSpeed = static_cast<u16>(speed_large * 257), // between 0 to 65535
			.wRightMotorSpeed = static_cast<u16>(speed_small * 257) // between 0 to 65535
		};

		if (xinputSetState(padnum, &vibrate) == ERROR_SUCCESS)
		{
			dev->new_output_data = false;
			dev->last_output = now;
		}
	}
}

#endif
