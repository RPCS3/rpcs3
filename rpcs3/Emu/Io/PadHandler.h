#pragma once

#include <vector>

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

struct Pad
{
	u32 m_port_status;
	u32 m_port_setting;
	u32 m_device_capability;
	u32 m_device_type;

	std::vector<Button> m_buttons;
	std::vector<AnalogStick> m_sticks;

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

	Pad(u32 port_status, u32 port_setting, u32 device_capability, u32 device_type)
		: m_port_status(port_status)
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

		, m_sensor_x(0)
		, m_sensor_y(399)
		, m_sensor_z(0)
		, m_sensor_g(0)
	{
	}
};

struct PadInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 system_info;
};

class PadHandlerBase
{
protected:
	PadInfo m_info;
	std::vector<Pad> m_pads;

public:
	virtual void Init(const u32 max_connect)=0;
	virtual void Close()=0;
	virtual ~PadHandlerBase() = default;

	//Set value to set pressure/axi to certain level, otherwise 0/255 default
	void Key(const u32 code, bool pressed, u16 value=256)
	{
		for(Pad& pad : m_pads)
		{
			for (Button& button : pad.m_buttons)
			{
				if (button.m_keyCode != code)
					continue;

				//This is for reporting when a controller connects/disconnects, shouldn't be here
				//pad.m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;

				if (value >= 256){ value = 255; }

				//Todo: Is this flush necessary once games hit decent speeds?
				if (button.m_pressed && !pressed)
				{
					button.m_flush = true;
					
				}
				else
				{
					button.m_pressed = pressed;
					if (pressed)
						button.m_value = value;
					else
						button.m_value = 0;
				}
			}

			for(AnalogStick& stick : pad.m_sticks)
			{
				if (stick.m_keyCodeMax != code && stick.m_keyCodeMin != code)
					continue;

				//slightly less hack job for key based analog stick
				//	should also fix/make transitions when using keys smoother
				//	the logic here is that when a key is released, 
				//	if we are at the opposite end of the axis, dont reset to middle
				if (stick.m_keyCodeMax == code)
				{
					if (pressed) stick.m_value = 255;
					else if (stick.m_value==0) stick.m_value = 0;
					else stick.m_value = 128;
				}
				if (stick.m_keyCodeMin == code)
				{
					if (pressed) stick.m_value = 0;
					else if (stick.m_value == 255) stick.m_value = 255;
					else stick.m_value = 128;
				}
			}
		}
	}

	PadInfo& GetInfo() { return m_info; }
	std::vector<Pad>& GetPads() { return m_pads; }
	std::vector<Button>& GetButtons(const u32 pad) { return m_pads[pad].m_buttons; }
	std::vector<AnalogStick>& GetSticks(const u32 pad) { return m_pads[pad].m_sticks; }
};