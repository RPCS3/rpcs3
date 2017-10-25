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

	cfg::int32 left_stick_left{ this, "Left Analog Stick Left", 0 };
	cfg::int32 left_stick_down{ this, "Left Analog Stick Down", 0 };
	cfg::int32 left_stick_right{ this, "Left Analog Stick Right", 0 };
	cfg::int32 left_stick_up{ this, "Left Analog Stick Up", 0 };
	cfg::int32 right_stick_left{ this, "Right Analog Stick Left", 0 };
	cfg::int32 right_stick_down{ this, "Right Analog Stick Down", 0 };
	cfg::int32 right_stick_right{ this, "Right Analog Stick Right", 0 };
	cfg::int32 right_stick_up{ this, "Right Analog Stick Up", 0 };
	cfg::int32 start{ this, "Start", 0 };
	cfg::int32 select{ this, "Select", 0 };
	cfg::int32 square{ this, "Square", 0 };
	cfg::int32 cross{ this, "Cross", 0 };
	cfg::int32 circle{ this, "Circle", 0 };
	cfg::int32 triangle{ this, "Triangle", 0 };
	cfg::int32 left{ this, "Left", 0 };
	cfg::int32 down{ this, "Down", 0 };
	cfg::int32 right{ this, "Right", 0 };
	cfg::int32 up{ this, "Up", 0 };
	cfg::int32 r1{ this, "R1", 0 };
	cfg::int32 r2{ this, "R2", 0 };
	cfg::int32 r3{ this, "R3", 0 };
	cfg::int32 l1{ this, "L1", 0 };
	cfg::int32 l2{ this, "L2", 0 };
	cfg::int32 l3{ this, "L3", 0 };

	cfg::_int<0, 1000000> lstickdeadzone{ this, "Left Stick Deadzone", 0 };
	cfg::_int<0, 1000000> rstickdeadzone{ this, "Right Stick Deadzone", 0 };
	cfg::_int<0, 1000000> ltriggerthreshold{ this, "Left Trigger Threshold", 0 };
	cfg::_int<0, 1000000> rtriggerthreshold{ this, "Right Trigger Threshold", 0 };
	cfg::_int<0, 1000000> padsquircling{ this, "Pad Squircling Factor", 0 };

	cfg::_bool enable_vibration_motor_large{ this, "Enable Large Vibration Motor", true };
	cfg::_bool enable_vibration_motor_small{ this, "Enable Small Vibration Motor", true };
	cfg::_bool switch_vibration_motors{ this, "Switch Vibration Motors", false };
	cfg::_bool has_axis_triggers{ this, "Has Axis Triggers", true };

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

	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_config = false;
	pad_config m_pad_config;

	u16 NormalizeTriggerInput(u16 value, int threshold)
	{
		if (value <= threshold || threshold >= TRIGGER_MAX)
		{
			return static_cast<u16>(0);
		}
		else if (threshold <= TRIGGER_MIN)
		{
			return value;
		}
		else
		{
			return (u16)(float(TRIGGER_MAX) * float(value - threshold) / float(TRIGGER_MAX - threshold));
		}
	};

	u16 NormalizeStickInput(s16 raw_value, int threshold, bool ignore_threshold = false)
	{
		u16 value = abs(raw_value);
		float max = float(THUMB_MAX);
		float val = value / max;

		if (ignore_threshold || threshold <= THUMB_MIN)
		{
			return static_cast<u16>(255.0f * val);
		}
		else if (value <= threshold || threshold >= THUMB_MAX)
		{
			return static_cast<u16>(0);
		}
		else
		{
			float thresh = threshold / max;
			return static_cast<u16>(255.0f * std::min(1.0f, (val - thresh) / (1.0f - thresh)));
		}
	};

	void NormalizeRawStickInput(float& X, float& Y, float deadzone)
	{
		X /= 255.0f;
		Y /= 255.0f;
		deadzone /= float(THUMB_MAX);

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

	u16 Clamp0To255(f32 input)
	{
		if (input > 255.f)
			return 255;
		else if (input < 0.f)
			return 0;
		else return static_cast<u16>(input);
	};

	u16 ConvertAxis(float value)
	{
		return static_cast<u16>((value + 1.0)*(255.0 / 2.0));
	};

	// we get back values from 0 - 255 for x and y from the ds4 packets,
	// and they end up giving us basically a perfect circle, which is how the ds4 sticks are setup
	// however,the ds3, (and i think xbox controllers) give instead a more 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange

	// this might be the best for now, in practice it seems to push the corners to max of 20x20
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY, float squircle_factor)
	{
		// convert inX and Y to a (-1, 1) vector;
		const f32 x = (inX - 127) / 127.f;
		const f32 y = (inY - 127) / 127.f;

		// compute angle and len of given point to be used for squircle radius
		const f32 angle = std::atan2(y, x);
		const f32 r = std::sqrt(std::pow(x, 2.f) + std::pow(y, 2.f));

		// now find len/point on the given squircle from our current angle and radius in polar coords
		// https://thatsmaths.com/2016/07/14/squircles/
		const f32 newLen = (1 + std::pow(std::sin(2 * angle), 2.f) / (squircle_factor / 1000.f)) * r;

		// we now have len and angle, convert to cartisian
		const int newX = Clamp0To255(((newLen * std::cos(angle)) + 1) * 127);
		const int newY = Clamp0To255(((newLen * std::sin(angle)) + 1) * 127);
		return std::tuple<u16, u16>(newX, newY);
	}

public:
	s32 THUMB_MIN;
	s32 THUMB_MAX;
	s32 TRIGGER_MIN;
	s32 TRIGGER_MAX;
	s32 VIBRATION_MIN;
	s32 VIBRATION_MAX;

	virtual bool Init() { return true; };
	virtual ~PadHandlerBase() = default;

	//Does it have GUI Config?
	bool has_config() { return b_has_config; };
	bool has_rumble() { return b_has_rumble; };
	bool has_deadzones() { return b_has_deadzones; };
	//Sets window to config the controller(optional)
	virtual void ConfigController(const std::string& device) {};
	virtual void GetNextButtonPress(const std::string& padId, const std::function<void(u32, std::string)>& callback) {};
	virtual std::string GetButtonName(u32 btn) { return ""; };
	virtual void TranslateButtonPress(u32 keyCode, bool& pressed, u16& value, bool ignore_threshold = false) {};
	virtual void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) {};
	//Return list of devices for that handler
	virtual std::vector<std::string> ListDevices() = 0;
	//Callback called during pad_thread::ThreadFunc
	virtual void ThreadProc() = 0;
	//Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) = 0;
};
