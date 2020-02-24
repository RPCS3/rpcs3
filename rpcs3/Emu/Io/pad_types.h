#pragma once

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
