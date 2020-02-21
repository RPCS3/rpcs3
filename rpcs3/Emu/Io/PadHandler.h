#pragma once

#include <cmath>
#include "pad_config.h"
#include "Utilities/types.h"
#include "Emu/GameInfo.h"

// TODO: HLE info (constants, structs, etc.) should not be available here

enum SystemInfo
{
	CELL_PAD_INFO_INTERCEPTED = 0x00000001
};

enum PortStatus
{
	CELL_PAD_STATUS_DISCONNECTED      = 0x00000000,
	CELL_PAD_STATUS_CONNECTED         = 0x00000001,
	CELL_PAD_STATUS_ASSIGN_CHANGES    = 0x00000002,
	CELL_PAD_STATUS_CUSTOM_CONTROLLER = 0x00000004,
};

enum PortSettings
{
	CELL_PAD_SETTING_LDD           = 0x00000001, // Speculative
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
	CELL_PAD_CAPABILITY_PS3_CONFORMITY  = 0x00000001, // PS3 Conformity Controller
	CELL_PAD_CAPABILITY_PRESS_MODE      = 0x00000002, // Press mode supported
	CELL_PAD_CAPABILITY_SENSOR_MODE     = 0x00000004, // Sensor mode supported
	CELL_PAD_CAPABILITY_HP_ANALOG_STICK = 0x00000008, // High Precision analog stick
	CELL_PAD_CAPABILITY_ACTUATOR        = 0x00000010, // Motor supported
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
	u16 m_value = 0;
	bool m_pressed = false;

	Button(u32 offset, u32 keyCode, u32 outKeyCode)
		: m_offset(offset)
		, m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
	{
	}
};

struct AnalogStick
{
	u32 m_offset;
	u32 m_keyCodeMin;
	u32 m_keyCodeMax;
	u16 m_value = 128;

	AnalogStick(u32 offset, u32 keyCodeMin, u32 keyCodeMax)
		: m_offset(offset)
		, m_keyCodeMin(keyCodeMin)
		, m_keyCodeMax(keyCodeMax)
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
	u32 m_device_capability;
	u32 m_device_type;
	u32 m_class_type;

	// Cable State:   0 - 1  plugged in ?
	u8 m_cable_state;

	// DS4: 0 - 9 while unplugged, 0 - 10 while plugged in, 11 charge complete
	// XInput: 0 = Empty, 1 = Low, 2 = Medium, 3 = Full
	u8 m_battery_level;

	std::vector<Button> m_buttons;
	std::vector<AnalogStick> m_sticks;
	std::vector<AnalogSensor> m_sensors;
	std::vector<VibrateMotor> m_vibrateMotors;

	// These hold bits for their respective buttons
	u16 m_digital_1;
	u16 m_digital_2;

	// All sensors go from 0-255
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

	// Except for these...0-1023
	// ~399 on sensor y is a level non moving controller
	u16 m_sensor_x;
	u16 m_sensor_y;
	u16 m_sensor_z;
	u16 m_sensor_g;

	bool ldd = false;
	u8 ldd_data[132] = {};

	void Init(u32 port_status, u32 device_capability, u32 device_type, u32 class_type)
	{
		m_port_status = port_status;
		m_device_capability = device_capability;
		m_device_type = device_type;
		m_class_type = class_type;
	}

	Pad(u32 port_status, u32 device_capability, u32 device_type)
		: m_buffer_cleared(true)
		, m_port_status(port_status)
		, m_device_capability(device_capability)
		, m_device_type(device_type)
		, m_class_type(0)
		, m_cable_state(0)
		, m_battery_level(0)

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

struct PadDevice
{
	pad_config* config{ nullptr };
};

class PadHandlerBase
{
protected:
	enum button
	{
		up,
		down,
		left,
		right,
		cross,
		square,
		circle,
		triangle,
		l1,
		l2,
		l3,
		r1,
		r2,
		r3,
		start,
		select,
		ps,
		//reserved,
		ls_left,
		ls_right,
		ls_down,
		ls_up,
		rs_left,
		rs_right,
		rs_down,
		rs_up,

		button_count
	};

	enum connection
	{
		no_data,
		connected,
		disconnected
	};

	static const u32 MAX_GAMEPADS = 7;

	std::array<bool, MAX_GAMEPADS> last_connection_status{{ false, false, false, false, false, false, false }};

	std::string m_name_string;
	size_t m_max_devices = 0;
	int m_trigger_threshold = 0;
	int m_thumb_threshold = 0;

	bool b_has_led = false;
	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_config = false;
	std::array<pad_config, MAX_GAMEPADS> m_pad_configs;
	std::vector<std::pair<std::shared_ptr<PadDevice>, std::shared_ptr<Pad>>> bindings;
	std::unordered_map<u32, std::string> button_list;
	std::vector<u32> blacklist;

	template <typename T>
	T lerp(T v0, T v1, T t) {
		return std::fma(t, v1, std::fma(-t, v0, v0));
	}

	// Search an unordered map for a string value and return found keycode
	int FindKeyCode(const std::unordered_map<u32, std::string>& map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	long FindKeyCode(const std::unordered_map<u64, std::string>& map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	int FindKeyCodeByString(const std::unordered_map<u32, std::string>& map, const std::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	long FindKeyCodeByString(const std::unordered_map<u64, std::string>& map, const std::string& name, bool fallback = true);

	// Get new scaled value between 0 and 255 based on its minimum and maximum
	float ScaleStickInput(s32 raw_value, int minimum, int maximum);

	// Get new scaled value between -255 and 255 based on its minimum and maximum
	float ScaleStickInput2(s32 raw_value, int minimum, int maximum);

	// Get normalized trigger value based on the range defined by a threshold
	u16 NormalizeTriggerInput(u16 value, int threshold);

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum);

	u16 NormalizeStickInput(u16 raw_value, int threshold, int multiplier, bool ignore_threshold = false);

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone);

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
	u32 connected_devices = 0;

	pad_handler m_type;

	std::string name_string();
	size_t max_devices();
	bool has_config();
	bool has_rumble();
	bool has_deadzones();
	bool has_led();

	static std::string get_config_dir(pad_handler type, const std::string& title_id = "");
	static std::string get_config_filename(int i, const std::string& title_id = "");

	virtual bool Init() { return true; }
	PadHandlerBase(pad_handler type = pad_handler::null);
	virtual ~PadHandlerBase() = default;
	// Sets window to config the controller(optional)
	virtual void SetPadData(const std::string& /*padId*/, u32 /*largeMotor*/, u32 /*smallMotor*/, s32 /*r*/, s32 /*g*/, s32 /*b*/) {}
	// Return list of devices for that handler
	virtual std::vector<std::string> ListDevices() = 0;
	// Callback called during pad_thread::ThreadFunc
	virtual void ThreadProc();
	// Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device);
	virtual void init_config(pad_config* /*cfg*/, const std::string& /*name*/) = 0;
	virtual void get_next_button_press(const std::string& padId, const std::function<void(u16, std::string, std::string, std::array<int, 6>)>& callback, const std::function<void(std::string)>& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons = {});

private:
	virtual std::shared_ptr<PadDevice> get_device(const std::string& /*device*/) { return nullptr; };
	virtual bool get_is_left_trigger(u64 /*keyCode*/) { return false; };
	virtual bool get_is_right_trigger(u64 /*keyCode*/) { return false; };
	virtual bool get_is_left_stick(u64 /*keyCode*/) { return false; };
	virtual bool get_is_right_stick(u64 /*keyCode*/) { return false; };
	virtual PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& /*device*/) { return connection::disconnected; };
	virtual void get_extended_info(const std::shared_ptr<PadDevice>& /*device*/, const std::shared_ptr<Pad>& /*pad*/) {};
	virtual void apply_pad_data(const std::shared_ptr<PadDevice>& /*device*/, const std::shared_ptr<Pad>& /*pad*/) {};
	virtual std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& /*device*/) { return {}; };
	virtual std::array<int, 6> get_preview_values(std::unordered_map<u64, u16> /*data*/) { return {}; };

protected:
	virtual std::array<u32, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& /*device*/, const pad_config* profile);
	virtual void get_mapping(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad);
	void TranslateButtonPress(const std::shared_ptr<PadDevice>& device, u64 keyCode, bool& pressed, u16& val, bool ignore_stick_threshold = false, bool ignore_trigger_threshold = false);
	void init_configs();
};
