#pragma once

#include <cmath>
#include <vector>
#include <memory>
#include "stdafx.h"
#include "../../Utilities/Config.h"
#include "../../Utilities/types.h"

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

static const u32 CELL_MAX_PADS = 127;
static const u32 CELL_PAD_MAX_PORT_NUM = 7;
static const u32 CELL_PAD_MAX_CODES = 64;
static const u32 CELL_PAD_MAX_CAPABILITY_INFO = 32;
static const u32 CELL_PAD_ACTUATOR_MAX = 2;

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

struct pad_config : cfg::node
{
	std::string cfg_type = "";
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

	int m_trigger_threshold = 0;
	int m_thumb_threshold = 0;

	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_config = false;
	pad_config m_pad_config;

	template <typename T>
	T lerp(T v0, T v1, T t) {
		return std::fma(t, v1, std::fma(-t, v0, v0));
	}

	// Search an unordered map for a string value and return found keycode
	int FindKeyCode(std::unordered_map<u32, std::string> map, const cfg::string& name, bool fallback = true)
	{
		std::string def = name.def;
		std::string nam = name.to_string();
		int def_code = -1;

		for (auto it = map.begin(); it != map.end(); ++it)
		{
			if (it->second == nam)
				return it->first;

			if (fallback && it->second == def)
				def_code = it->first;
		}

		if (fallback)
		{
			LOG_ERROR(HLE, "int FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);
			if (def_code < 0)
				def_code = 0;
		}

		return def_code;
	};

	long FindKeyCode(std::unordered_map<u64, std::string> map, const cfg::string& name, bool fallback = true)
	{
		std::string def = name.def;
		std::string nam = name.to_string();
		int def_code = -1;

		for (auto it = map.begin(); it != map.end(); ++it)
		{
			if (it->second == nam)
				return it->first;

			if (fallback && it->second == def)
				def_code = it->first;
		}

		if (fallback)
		{
			LOG_ERROR(HLE, "long FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", nam, def_code, def);
			if (def_code < 0)
				def_code = 0;
		}

		return def_code;
	};

	// Search an unordered map for a string value and return found keycode
	int FindKeyCodeByString(std::unordered_map<u32, std::string> map, const std::string& name, bool fallback = true)
	{
		for (auto it = map.begin(); it != map.end(); ++it)
		{
			if (it->second == name)
				return it->first;
		}

		if (fallback)
		{
			LOG_ERROR(HLE, "long FindKeyCodeByString fohr [name = %s] returned with 0", name);
			return 0;
		}

		return -1;
	};

	// Search an unordered map for a string value and return found keycode
	long FindKeyCodeByString(std::unordered_map<u64, std::string> map, const std::string& name, bool fallback = true)
	{
		for (auto it = map.begin(); it != map.end(); ++it)
		{
			if (it->second == name)
				return it->first;
		}

		if (fallback)
		{
			LOG_ERROR(HLE, "long FindKeyCodeByString fohr [name = %s] returned with 0", name);
			return 0;
		}

		return -1;
	};

	// Get normalized trigger value based on the range defined by a threshold
	u16 NormalizeTriggerInput(u16 value, int threshold)
	{
		if (value <= threshold || threshold >= trigger_max)
		{
			return static_cast<u16>(0);
		}
		else if (threshold <= trigger_min)
		{
			return value;
		}
		else
		{
			return (u16)(float(trigger_max) * float(value - threshold) / float(trigger_max - threshold));
		}
	};

	// Get new scaled value between 0 and 255 based on its minimum and maximum
	float ScaleStickInput(s32 raw_value, int minimum, int maximum)
	{
		// value based on max range converted to [0, 1]
		float val = float(Clamp(raw_value, minimum, maximum) - minimum) / float(abs(maximum) + abs(minimum));
		return 255.0f * val;
	};

	// Get new scaled value between -255 and 255 based on its minimum and maximum
	float ScaleStickInput2(s32 raw_value, int minimum, int maximum)
	{
		// value based on max range converted to [0, 1]
		float val = float(Clamp(raw_value, minimum, maximum) - minimum) / float(abs(maximum) + abs(minimum));
		return (510.0f * val) - 255.0f;
	};

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(u16 raw_value, float threshold, float maximum)
	{
		if (threshold >= maximum || maximum <= 0)
		{
			return static_cast<u16>(0);
		}

		float val = float(Clamp(raw_value, 0, maximum)) / maximum; // value based on max range converted to [0, 1]

		if (threshold <= 0)
		{
			return static_cast<u16>(255.0f * val);
		}
		else
		{
			float thresh = threshold / maximum; // threshold converted to [0, 1]
			return static_cast<u16>(255.0f * std::min(1.0f, (val - thresh) / (1.0f - thresh)));
		}
	};

	u16 NormalizeStickInput(s32 raw_value, int threshold, bool ignore_threshold = false)
	{
		if (ignore_threshold)
		{
			return static_cast<u16>(ScaleStickInput(raw_value, 0, thumb_max));
		}
		else
		{
			return NormalizeDirectedInput(raw_value, threshold, thumb_max);
		}
	}

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone)
	{
		const float dzRange = deadzone / float((std::abs(thumb_max) + std::abs(thumb_min)));

		float X = inX / 255.0f;
		float Y = inY / 255.0f;

		if (dzRange > 0.f)
		{
			const float mag = std::min(sqrtf(X*X + Y*Y), 1.f);

			if (mag <= 0)
			{
				return std::tuple<u16, u16>(ConvertAxis(X), ConvertAxis(Y));
			}

			if (mag > dzRange) {
				float pos = lerp(0.13f, 1.f, (mag - dzRange) / (1 - dzRange));
				float scale = pos / mag;
				X = X * scale;
				Y = Y * scale;
			}
			else {
				float pos = lerp(0.f, 0.13f, mag / dzRange);
				float scale = pos / mag;
				X = X * scale;
				Y = Y * scale;
			}
		}
		return std::tuple<u16, u16>( ConvertAxis(X), ConvertAxis(Y) );
	};

	// get clamped value between min and max
	s32 Clamp(f32 input, s32 min, s32 max)
	{
		if (input > max)
			return max;
		else if (input < min)
			return min;
		else return static_cast<s32>(input);
	};

	// get clamped value between 0 and 255
	u16 Clamp0To255(f32 input)
	{
		return static_cast<u16>(Clamp(input, 0, 255));
	};

	// get clamped value between 0 and 1023
	u16 Clamp0To1023(f32 input)
	{
		return static_cast<u16>(Clamp(input, 0, 1023));
	}

	// input has to be [-1,1]. result will be [0,255]
	u16 ConvertAxis(float value)
	{
		return static_cast<u16>((value + 1.0)*(255.0 / 2.0));
	};

	// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
	// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
	// This function assumes inX and inY is already in 0-255 
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY, float squircle_factor)
	{
		// convert inX and Y to a (-1, 1) vector;
		const f32 x = ((f32)inX - 127.5f) / 127.5f;
		const f32 y = ((f32)inY - 127.5f) / 127.5f;

		// compute angle and len of given point to be used for squircle radius
		const f32 angle = std::atan2(y, x);
		const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));

		// now find len/point on the given squircle from our current angle and radius in polar coords
		// https://thatsmaths.com/2016/07/14/squircles/
		const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / (squircle_factor / 1000.f)) * r;

		// we now have len and angle, convert to cartisian
		const int newX = Clamp0To255(((newLen * std::cos(angle)) + 1) * 127.5f);
		const int newY = Clamp0To255(((newLen * std::sin(angle)) + 1) * 127.5f);
		return std::tuple<u16, u16>(newX, newY);
	}

public:
	s32 thumb_min = 0;
	s32 thumb_max = 255;
	s32 trigger_min = 0;
	s32 trigger_max = 255;
	s32 vibration_min = 0;
	s32 vibration_max = 255;
	u32 connected = 0;

	virtual bool Init() { return true; };
	virtual ~PadHandlerBase() = default;

	//Does it have GUI Config?
	bool has_config() { return b_has_config; };
	bool has_rumble() { return b_has_rumble; };
	bool has_deadzones() { return b_has_deadzones; };
	pad_config* GetConfig() { return &m_pad_config; };
	//Sets window to config the controller(optional)
	virtual void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback, bool get_blacklist = false, std::vector<std::string> buttons = {}) {};
	virtual void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) {};
	//Return list of devices for that handler
	virtual std::vector<std::string> ListDevices() = 0;
	//Callback called during pad_thread::ThreadFunc
	virtual void ThreadProc() = 0;
	//Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) = 0;

private:
	virtual void TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold = false) {};
};
