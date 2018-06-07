#pragma once

#include <cmath>
#include <vector>
#include <memory>
#include "stdafx.h"
#include "../../Utilities/Config.h"
#include "../../Utilities/types.h"
#include "Emu/System.h"

// TODO: HLE info (constants, structs, etc.) should not be available here

enum PortStatus
{
	CELL_PAD_STATUS_DISCONNECTED   = 0x00000000,
	CELL_PAD_STATUS_CONNECTED      = 0x00000001,
	CELL_PAD_STATUS_ASSIGN_CHANGES = 0x00000002,
};

enum PortSettings
{
	CELL_PAD_SETTING_PRESS_ON      = 0x00000002,
	CELL_PAD_SETTING_SENSOR_ON     = 0x00000004,
	CELL_PAD_SETTING_PRESS_OFF     = 0x00000000,
	CELL_PAD_SETTING_SENSOR_OFF    = 0x00000000,
};

enum Digital1Flags
{
	CELL_PAD_CTRL_LEFT     = 0x00000080,
	CELL_PAD_CTRL_DOWN     = 0x00000040,
	CELL_PAD_CTRL_RIGHT    = 0x00000020,
	CELL_PAD_CTRL_UP       = 0x00000010,
	CELL_PAD_CTRL_START    = 0x00000008,
	CELL_PAD_CTRL_R3       = 0x00000004,
	CELL_PAD_CTRL_L3       = 0x00000002,
	CELL_PAD_CTRL_SELECT   = 0x00000001,
};

enum Digital2Flags
{
	CELL_PAD_CTRL_SQUARE   = 0x00000080,
	CELL_PAD_CTRL_CROSS    = 0x00000040,
	CELL_PAD_CTRL_CIRCLE   = 0x00000020,
	CELL_PAD_CTRL_TRIANGLE = 0x00000010,
	CELL_PAD_CTRL_R1       = 0x00000008,
	CELL_PAD_CTRL_L1       = 0x00000004,
	CELL_PAD_CTRL_R2       = 0x00000002,
	CELL_PAD_CTRL_L2       = 0x00000001,
};

enum DeviceCapability
{
	CELL_PAD_CAPABILITY_PS3_CONFORMITY  = 0x00000001, //PS3 Conformity Controller
	CELL_PAD_CAPABILITY_PRESS_MODE      = 0x00000002, //Press mode supported
	CELL_PAD_CAPABILITY_SENSOR_MODE     = 0x00000004, //Sensor mode supported
	CELL_PAD_CAPABILITY_HP_ANALOG_STICK = 0x00000008, //High Precision analog stick
	CELL_PAD_CAPABILITY_ACTUATOR        = 0x00000010, //Motor supported
};

enum DeviceType
{
	CELL_PAD_DEV_TYPE_STANDARD   = 0,
	CELL_PAD_DEV_TYPE_BD_REMOCON = 4,
	CELL_PAD_DEV_TYPE_LDD        = 5,
};

enum ButtonDataOffset
{
	CELL_PAD_BTN_OFFSET_DIGITAL1       = 2,
	CELL_PAD_BTN_OFFSET_DIGITAL2       = 3,
	CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X = 4,
	CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y = 5,
	CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X  = 6,
	CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y  = 7,
	CELL_PAD_BTN_OFFSET_PRESS_RIGHT    = 8,
	CELL_PAD_BTN_OFFSET_PRESS_LEFT     = 9,
	CELL_PAD_BTN_OFFSET_PRESS_UP       = 10,
	CELL_PAD_BTN_OFFSET_PRESS_DOWN     = 11,
	CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE = 12,
	CELL_PAD_BTN_OFFSET_PRESS_CIRCLE   = 13,
	CELL_PAD_BTN_OFFSET_PRESS_CROSS    = 14,
	CELL_PAD_BTN_OFFSET_PRESS_SQUARE   = 15,
	CELL_PAD_BTN_OFFSET_PRESS_L1       = 16,
	CELL_PAD_BTN_OFFSET_PRESS_R1       = 17,
	CELL_PAD_BTN_OFFSET_PRESS_L2       = 18,
	CELL_PAD_BTN_OFFSET_PRESS_R2       = 19,
	CELL_PAD_BTN_OFFSET_SENSOR_X       = 20,
	CELL_PAD_BTN_OFFSET_SENSOR_Y       = 21,
	CELL_PAD_BTN_OFFSET_SENSOR_Z       = 22,
	CELL_PAD_BTN_OFFSET_SENSOR_G       = 23,
};

enum
{
	CELL_PAD_ACTUATOR_MAX        = 2,
	CELL_PAD_MAX_PORT_NUM        = 7,
	CELL_PAD_MAX_CAPABILITY_INFO = 32,
	CELL_PAD_MAX_CODES           = 64,
	CELL_MAX_PADS                = 127,
};

struct Button
{
	u32 m_offset;
	u32 m_keyCode;
	u32 m_outKeyCode;
	u16 m_value;
	bool m_pressed;
	bool m_flush;

	Button(u32 offset, u32 keyCode, u32 outKeyCode)
		: m_pressed(false)
		, m_flush(false)
		, m_offset(offset)
		, m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
		, m_value(0)
	{
	}
};

struct AnalogStick
{
	u32 m_offset;
	u32 m_keyCodeMin;
	u32 m_keyCodeMax;
	u16 m_value;

	AnalogStick(u32 offset, u32 keyCodeMin, u32 keyCodeMax)
		: m_offset(offset)
		, m_keyCodeMin(keyCodeMin)
		, m_keyCodeMax(keyCodeMax)
		, m_value(128)
	{
	}
};

struct AnalogSensor
{
	u32 m_offset;
	u16 m_value;

	AnalogSensor(u32 offset, u16 value)
		: m_offset(offset)
		, m_value(value)
	{}
};

struct VibrateMotor
{
	bool m_isLargeMotor;
	u16 m_value;

	VibrateMotor(bool largeMotor, u16 value)
		: m_isLargeMotor(largeMotor)
		, m_value(value)
	{}
};

struct Pad
{
	bool m_buffer_cleared;
	u32 m_port_status;
	u32 m_port_setting;
	u32 m_device_capability;
	u32 m_device_type;

	// Cable State:   0 - 1  plugged in ?
	u8 m_cable_state;

	// DS4: 0 - 9  while unplugged, 0 - 10 while plugged in, 11 charge complete
	// XInput: 0 = Empty, 1 = Low, 2 = Medium, 3 = Full
	u8 m_battery_level;

	std::vector<Button> m_buttons;
	std::vector<AnalogStick> m_sticks;
	std::vector<AnalogSensor> m_sensors;
	std::vector<VibrateMotor> m_vibrateMotors;

	//These hold bits for their respective buttons
	u16 m_digital_1;
	u16 m_digital_2;

	//All sensors go from 0-255
	u16 m_analog_left_x;
	u16 m_analog_left_y;
	u16 m_analog_right_x;
	u16 m_analog_right_y;

	u16 m_press_right;
	u16 m_press_left;
	u16 m_press_up;
	u16 m_press_down;
	u16 m_press_triangle;
	u16 m_press_circle;
	u16 m_press_cross;
	u16 m_press_square;
	u16 m_press_L1;
	u16 m_press_L2;
	u16 m_press_R1;
	u16 m_press_R2;

	//Except for these...0-1023
	//~399 on sensor y is a level non moving controller
	u16 m_sensor_x;
	u16 m_sensor_y;
	u16 m_sensor_z;
	u16 m_sensor_g;

	void Init(u32 port_status, u32 port_setting, u32 device_capability, u32 device_type)
	{
		m_port_status = port_status;
		m_port_setting = port_setting;
		m_device_capability = device_capability;
		m_device_type = device_type;
	}

	Pad(u32 port_status, u32 port_setting, u32 device_capability, u32 device_type)
		: m_buffer_cleared(true)
		, m_port_status(port_status)
		, m_port_setting(port_setting)
		, m_device_capability(device_capability)
		, m_device_type(device_type)

		, m_digital_1(0)
		, m_digital_2(0)

		, m_analog_left_x(128)
		, m_analog_left_y(128)
		, m_analog_right_x(128)
		, m_analog_right_y(128)

		, m_press_right(0)
		, m_press_left(0)
		, m_press_up(0)
		, m_press_down(0)
		, m_press_triangle(0)
		, m_press_circle(0)
		, m_press_cross(0)
		, m_press_square(0)
		, m_press_L1(0)
		, m_press_L2(0)
		, m_press_R1(0)
		, m_press_R2(0)

		, m_sensor_x(512)
		, m_sensor_y(399)
		, m_sensor_z(512)
		, m_sensor_g(512)
	{
	}
};

struct cfg_player final : cfg::node
{
	pad_handler def_handler = pad_handler::null;
	cfg_player(node* owner, const std::string& name, pad_handler type) : cfg::node(owner, name), def_handler(type) {};

	cfg::_enum<pad_handler> handler{ this, "Handler", def_handler };
	cfg::string device{ this, "Device", handler.to_string() };
	cfg::string profile{ this, "Profile", "Default Profile" };
};

struct cfg_input final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_input.yml";

	cfg_player player1{ this, "Player 1 Input", pad_handler::keyboard };
	cfg_player player2{ this, "Player 2 Input", pad_handler::null };
	cfg_player player3{ this, "Player 3 Input", pad_handler::null };
	cfg_player player4{ this, "Player 4 Input", pad_handler::null };
	cfg_player player5{ this, "Player 5 Input", pad_handler::null };
	cfg_player player6{ this, "Player 6 Input", pad_handler::null };
	cfg_player player7{ this, "Player 7 Input", pad_handler::null };

	cfg_player *player[7]{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc! 

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	};

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	};
};

extern cfg_input g_cfg_input;

struct pad_config final : cfg::node
{
	std::string cfg_name = "";

	cfg::string ls_left { this, "Left Stick Left", "" };
	cfg::string ls_down { this, "Left Stick Down", "" };
	cfg::string ls_right{ this, "Left Stick Right", "" };
	cfg::string ls_up   { this, "Left Stick Up", "" };
	cfg::string rs_left { this, "Right Stick Left", "" };
	cfg::string rs_down { this, "Right Stick Down", "" };
	cfg::string rs_right{ this, "Right Stick Right", "" };
	cfg::string rs_up   { this, "Right Stick Up", "" };
	cfg::string start   { this, "Start", "" };
	cfg::string select  { this, "Select", "" };
	cfg::string ps      { this, "PS Button", "" };
	cfg::string square  { this, "Square", "" };
	cfg::string cross   { this, "Cross", "" };
	cfg::string circle  { this, "Circle", "" };
	cfg::string triangle{ this, "Triangle", "" };
	cfg::string left    { this, "Left", "" };
	cfg::string down    { this, "Down", "" };
	cfg::string right   { this, "Right", "" };
	cfg::string up      { this, "Up", "" };
	cfg::string r1      { this, "R1", "" };
	cfg::string r2      { this, "R2", "" };
	cfg::string r3      { this, "R3", "" };
	cfg::string l1      { this, "L1", "" };
	cfg::string l2      { this, "L2", "" };
	cfg::string l3      { this, "L3", "" };

	cfg::_int<0, 1000000> lstickdeadzone{ this, "Left Stick Deadzone", 0 };
	cfg::_int<0, 1000000> rstickdeadzone{ this, "Right Stick Deadzone", 0 };
	cfg::_int<0, 1000000> ltriggerthreshold{ this, "Left Trigger Threshold", 0 };
	cfg::_int<0, 1000000> rtriggerthreshold{ this, "Right Trigger Threshold", 0 };
	cfg::_int<0, 1000000> padsquircling{ this, "Pad Squircling Factor", 0 };

	cfg::_int<0, 255> colorR{ this, "Color Value R", 0 };
	cfg::_int<0, 255> colorG{ this, "Color Value G", 0 };
	cfg::_int<0, 255> colorB{ this, "Color Value B", 0 };

	cfg::_bool enable_vibration_motor_large{ this, "Enable Large Vibration Motor", true };
	cfg::_bool enable_vibration_motor_small{ this, "Enable Small Vibration Motor", true };
	cfg::_bool switch_vibration_motors{ this, "Switch Vibration Motors", false };

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	}

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	}

	bool exist()
	{
		return fs::is_file(cfg_name);
	}
};

class PadHandlerBase
{
protected:
	static const u32 MAX_GAMEPADS = 7;

	std::array<bool, MAX_GAMEPADS> last_connection_status{{ false, false, false, false, false, false, false }};

	std::string m_name_string;
	int m_max_devices = 0;
	int m_trigger_threshold = 0;
	int m_thumb_threshold = 0;

	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_config = false;
	std::array<pad_config, MAX_GAMEPADS> m_pad_configs;

	template <typename T>
	T lerp(T v0, T v1, T t) {
		return std::fma(t, v1, std::fma(-t, v0, v0));
	}

	// Search an unordered map for a string value and return found keycode
	int FindKeyCode(std::unordered_map<u32, std::string> map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	long FindKeyCode(std::unordered_map<u64, std::string> map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	int FindKeyCodeByString(std::unordered_map<u32, std::string> map, const std::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	long FindKeyCodeByString(std::unordered_map<u64, std::string> map, const std::string& name, bool fallback = true);

	// Get new scaled value between 0 and 255 based on its minimum and maximum
	float ScaleStickInput(s32 raw_value, int minimum, int maximum);

	// Get new scaled value between -255 and 255 based on its minimum and maximum
	float ScaleStickInput2(s32 raw_value, int minimum, int maximum);

	// Get normalized trigger value based on the range defined by a threshold
	u16 NormalizeTriggerInput(u16 value, int threshold);

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(u16 raw_value, s32 threshold, s32 maximum);

	u16 NormalizeStickInput(u16 raw_value, int threshold, bool ignore_threshold = false);

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone);

	// get clamped value between min and max
	s32 Clamp(f32 input, s32 min, s32 max);
	// get clamped value between 0 and 255
	u16 Clamp0To255(f32 input);
	// get clamped value between 0 and 1023
	u16 Clamp0To1023(f32 input);

	// input has to be [-1,1]. result will be [0,255]
	u16 ConvertAxis(float value);

	// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
	// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
	// This function assumes inX and inY is already in 0-255
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY, int squircle_factor);

public:
	s32 thumb_min = 0;
	s32 thumb_max = 255;
	s32 trigger_min = 0;
	s32 trigger_max = 255;
	s32 vibration_min = 0;
	s32 vibration_max = 255;
	u32 connected = 0;

	pad_handler m_type = pad_handler::null;

	std::string name_string();
	int max_devices();
	bool has_config();
	bool has_rumble();
	bool has_deadzones();

	static std::string get_config_dir(pad_handler type);
	static std::string get_config_filename(int i);

	virtual bool Init() { return true; };
	PadHandlerBase(pad_handler type = pad_handler::null);
	virtual ~PadHandlerBase() = default;
	//Sets window to config the controller(optional)
	virtual void GetNextButtonPress(const std::string& /*padId*/, const std::function<void(u16, std::string, int[])>& /*callback*/, bool /*get_blacklist*/ = false, std::vector<std::string> /*buttons*/ = {}) {};
	virtual void TestVibration(const std::string& /*padId*/, u32 /*largeMotor*/, u32 /*smallMotor*/) {};
	//Return list of devices for that handler
	virtual std::vector<std::string> ListDevices() = 0;
	//Callback called during pad_thread::ThreadFunc
	virtual void ThreadProc() = 0;
	//Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> /*pad*/, const std::string& /*device*/) = 0;
	virtual void init_config(pad_config* /*cfg*/, const std::string& /*name*/) = 0;

private:
	virtual void TranslateButtonPress(u64 /*keyCode*/, bool& /*pressed*/, u16& /*val*/, bool /*ignore_threshold*/ = false) {};

protected:
	void init_configs();
};
