#pragma once

#include "util/types.hpp"

#include <vector>

enum SystemInfo
{
	CELL_PAD_INFO_INTERCEPTED = 0x00000001
};

enum PortStatus
{
	CELL_PAD_STATUS_DISCONNECTED = 0x00000000,
	CELL_PAD_STATUS_CONNECTED = 0x00000001,
	CELL_PAD_STATUS_ASSIGN_CHANGES = 0x00000002,
	CELL_PAD_STATUS_CUSTOM_CONTROLLER = 0x00000004,
};

enum PortSettings
{
	CELL_PAD_SETTING_LDD = 0x00000001, // Speculative
	CELL_PAD_SETTING_PRESS_ON = 0x00000002,
	CELL_PAD_SETTING_SENSOR_ON = 0x00000004,
	CELL_PAD_SETTING_PRESS_OFF = 0x00000000,
	CELL_PAD_SETTING_SENSOR_OFF = 0x00000000,
};

enum Digital1Flags
{
	CELL_PAD_CTRL_LEFT = 0x00000080,
	CELL_PAD_CTRL_DOWN = 0x00000040,
	CELL_PAD_CTRL_RIGHT = 0x00000020,
	CELL_PAD_CTRL_UP = 0x00000010,
	CELL_PAD_CTRL_START = 0x00000008,
	CELL_PAD_CTRL_R3 = 0x00000004,
	CELL_PAD_CTRL_L3 = 0x00000002,
	CELL_PAD_CTRL_SELECT = 0x00000001,
};

enum Digital2Flags
{
	CELL_PAD_CTRL_SQUARE = 0x00000080,
	CELL_PAD_CTRL_CROSS = 0x00000040,
	CELL_PAD_CTRL_CIRCLE = 0x00000020,
	CELL_PAD_CTRL_TRIANGLE = 0x00000010,
	CELL_PAD_CTRL_R1 = 0x00000008,
	CELL_PAD_CTRL_L1 = 0x00000004,
	CELL_PAD_CTRL_R2 = 0x00000002,
	CELL_PAD_CTRL_L2 = 0x00000001,
};

enum DeviceCapability
{
	CELL_PAD_CAPABILITY_PS3_CONFORMITY = 0x00000001, // PS3 Conformity Controller
	CELL_PAD_CAPABILITY_PRESS_MODE = 0x00000002, // Press mode supported
	CELL_PAD_CAPABILITY_SENSOR_MODE = 0x00000004, // Sensor mode supported
	CELL_PAD_CAPABILITY_HP_ANALOG_STICK = 0x00000008, // High Precision analog stick
	CELL_PAD_CAPABILITY_ACTUATOR = 0x00000010, // Motor supported
};

enum DeviceType
{
	CELL_PAD_DEV_TYPE_STANDARD = 0,
	CELL_PAD_DEV_TYPE_BD_REMOCON = 4,
	CELL_PAD_DEV_TYPE_LDD = 5,
};

enum ButtonDataOffset
{
	CELL_PAD_BTN_OFFSET_DIGITAL1 = 2,
	CELL_PAD_BTN_OFFSET_DIGITAL2 = 3,
	CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X = 4,
	CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y = 5,
	CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X = 6,
	CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y = 7,
	CELL_PAD_BTN_OFFSET_PRESS_RIGHT = 8,
	CELL_PAD_BTN_OFFSET_PRESS_LEFT = 9,
	CELL_PAD_BTN_OFFSET_PRESS_UP = 10,
	CELL_PAD_BTN_OFFSET_PRESS_DOWN = 11,
	CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE = 12,
	CELL_PAD_BTN_OFFSET_PRESS_CIRCLE = 13,
	CELL_PAD_BTN_OFFSET_PRESS_CROSS = 14,
	CELL_PAD_BTN_OFFSET_PRESS_SQUARE = 15,
	CELL_PAD_BTN_OFFSET_PRESS_L1 = 16,
	CELL_PAD_BTN_OFFSET_PRESS_R1 = 17,
	CELL_PAD_BTN_OFFSET_PRESS_L2 = 18,
	CELL_PAD_BTN_OFFSET_PRESS_R2 = 19,
	CELL_PAD_BTN_OFFSET_SENSOR_X = 20,
	CELL_PAD_BTN_OFFSET_SENSOR_Y = 21,
	CELL_PAD_BTN_OFFSET_SENSOR_Z = 22,
	CELL_PAD_BTN_OFFSET_SENSOR_G = 23,
};

enum CellPadPeriphGuitarBtnDataOffset
{
	// Basic
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_1      = 24,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_2      = 25,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_3      = 26,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_4      = 27,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_5      = 28,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_STRUM_UP    = 29,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_STRUM_DOWN  = 30,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_WHAMMYBAR   = 31, // 128-255

	// Optional
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H1     = 32, // ROCKBAND Stratocaster
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H2     = 33,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H3     = 34,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H4     = 35,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_FRET_H5     = 36,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_5WAY_EFFECT = 37,
	CELL_PAD_PCLASS_BTN_OFFSET_GUITAR_TILT_SENS   = 38,
};

enum CellPadPeriphDrumBtnDataOffset
{
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_SNARE     = 24,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM       = 25,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM2      = 26,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_TOM_FLOOR = 27,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_KICK      = 28,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_HiHAT = 29,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_CRASH = 30,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_CYM_RIDE  = 31,
	CELL_PAD_PCLASS_BTN_OFFSET_DRUM_KICK2     = 32,
};

enum CellPadPeriphDJBtnDataOffset
{
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_ATTACK     = 24,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_CROSSFADER = 25,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_MIXER_DSP_DIAL   = 26,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM1    = 27,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM2    = 28,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_STREAM3    = 29,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK1_PLATTER    = 30,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM1    = 31,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM2    = 32,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_STREAM3    = 33,
	CELL_PAD_PCLASS_BTN_OFFSET_DJ_DECK2_PLATTER    = 34,
};

enum CellPadPeriphDanceMatBtnDataOffset
{
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_CIRCLE   = 24,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_CROSS    = 25,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_TRIANGLE = 26,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_SQUARE   = 27,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_RIGHT    = 28,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_LEFT     = 29,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_UP       = 30,
	CELL_PAD_PCLASS_BTN_OFFSET_DANCEMAT_DOWN     = 31,
};

enum
{
	CELL_PAD_ACTUATOR_MAX = 2,
	CELL_PAD_MAX_PORT_NUM = 7,
	CELL_PAD_MAX_CAPABILITY_INFO = 32,
	CELL_PAD_MAX_CODES = 64,
	CELL_MAX_PADS = 127,
};

struct Button
{
	u32 m_offset;
	u32 m_keyCode;
	u32 m_outKeyCode;
	u16 m_value    = 0;
	bool m_pressed = false;

	u16 m_actual_value = 0;     // only used in keyboard_pad_handler
	bool m_analog      = false; // only used in keyboard_pad_handler
	bool m_trigger     = false; // only used in keyboard_pad_handler

	Button(u32 offset, u32 keyCode, u32 outKeyCode)
		: m_offset(offset)
		, m_keyCode(keyCode)
		, m_outKeyCode(outKeyCode)
	{
		if (offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
		{
			if (outKeyCode == CELL_PAD_CTRL_LEFT || outKeyCode == CELL_PAD_CTRL_RIGHT || outKeyCode == CELL_PAD_CTRL_UP || outKeyCode == CELL_PAD_CTRL_DOWN)
			{
				m_analog = true;
			}
		}
		else if (offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
		{
			if (outKeyCode == CELL_PAD_CTRL_CROSS || outKeyCode == CELL_PAD_CTRL_CIRCLE || outKeyCode == CELL_PAD_CTRL_SQUARE || outKeyCode == CELL_PAD_CTRL_TRIANGLE || outKeyCode == CELL_PAD_CTRL_L1 ||
			    outKeyCode == CELL_PAD_CTRL_R1)
			{
				m_analog = true;
			}
			else if (outKeyCode == CELL_PAD_CTRL_L2 || outKeyCode == CELL_PAD_CTRL_R2)
			{
				m_trigger = true;
			}
		}
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
	u32 m_class_profile;

	u16 m_vendor_id;
	u16 m_product_id;

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

	void Init(u32 port_status, u32 device_capability, u32 device_type, u32 class_type, u32 class_profile, u16 vendor_id, u16 product_id)
	{
		m_port_status = port_status;
		m_device_capability = device_capability;
		m_device_type = device_type;
		m_class_type = class_type;
		m_class_profile = class_profile;
		m_vendor_id = vendor_id;
		m_product_id = product_id;
	}

	Pad(u32 port_status, u32 device_capability, u32 device_type)
		: m_buffer_cleared(true)
		, m_port_status(port_status)
		, m_device_capability(device_capability)
		, m_device_type(device_type)
		, m_class_type(0)
		, m_class_profile(0)
		, m_vendor_id(0)
		, m_product_id(0)
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
