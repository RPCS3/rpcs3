#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "Emu/Io/pad_config_types.h"

#include <map>
#include <set>
#include <vector>

enum class pad_button : u8
{
	dpad_up = 0,
	dpad_down,
	dpad_left,
	dpad_right,
	select,
	start,
	ps,
	triangle,
	circle,
	square,
	cross,
	L1,
	R1,
	L2,
	R2,
	L3,
	R3,

	ls_up,
	ls_down,
	ls_left,
	ls_right,
	ls_x,
	ls_y,
	rs_up,
	rs_down,
	rs_left,
	rs_right,
	rs_x,
	rs_y,

	pad_button_max_enum,

	// Special buttons for mouse input
	mouse_button_1,
	mouse_button_2,
	mouse_button_3,
	mouse_button_4,
	mouse_button_5,
	mouse_button_6,
	mouse_button_7,
	mouse_button_8,
};

u32 pad_button_offset(pad_button button);
u32 pad_button_keycode(pad_button button);

enum class axis_direction : u8
{
	both = 0,
	negative,
	positive
};

u32 get_axis_keycode(u32 offset, u16 value);

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

enum Digital1Flags : u32
{
	CELL_PAD_CTRL_SELECT = 0x00000001,
	CELL_PAD_CTRL_L3     = 0x00000002,
	CELL_PAD_CTRL_R3     = 0x00000004,
	CELL_PAD_CTRL_START  = 0x00000008,
	CELL_PAD_CTRL_UP     = 0x00000010,
	CELL_PAD_CTRL_RIGHT  = 0x00000020,
	CELL_PAD_CTRL_DOWN   = 0x00000040,
	CELL_PAD_CTRL_LEFT   = 0x00000080,
	CELL_PAD_CTRL_PS     = 0x00000100,
};

enum Digital2Flags : u32
{
	CELL_PAD_CTRL_L2       = 0x00000001,
	CELL_PAD_CTRL_R2       = 0x00000002,
	CELL_PAD_CTRL_L1       = 0x00000004,
	CELL_PAD_CTRL_R1       = 0x00000008,
	CELL_PAD_CTRL_TRIANGLE = 0x00000010,
	CELL_PAD_CTRL_CIRCLE   = 0x00000020,
	CELL_PAD_CTRL_CROSS    = 0x00000040,
	CELL_PAD_CTRL_SQUARE   = 0x00000080,
};

enum
{
	CELL_PAD_CTRL_LDD_PS = 0x00000001
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

// Controller types
enum
{
	CELL_PAD_PCLASS_TYPE_STANDARD   = 0x00,
	CELL_PAD_PCLASS_TYPE_GUITAR     = 0x01,
	CELL_PAD_PCLASS_TYPE_DRUM       = 0x02,
	CELL_PAD_PCLASS_TYPE_DJ         = 0x03,
	CELL_PAD_PCLASS_TYPE_DANCEMAT   = 0x04,
	CELL_PAD_PCLASS_TYPE_NAVIGATION = 0x05,
	CELL_PAD_PCLASS_TYPE_SKATEBOARD = 0x8001,

	// these are used together with pad->is_fake_pad to capture input events for usbd/gem/move without conflicting with cellPad
	CELL_PAD_FAKE_TYPE_FIRST               = 0xa000,
	CELL_PAD_FAKE_TYPE_GUNCON3             = 0xa000,
	CELL_PAD_FAKE_TYPE_TOP_SHOT_ELITE      = 0xa001,
	CELL_PAD_FAKE_TYPE_TOP_SHOT_FEARMASTER = 0xa002,
	CELL_PAD_FAKE_TYPE_GAMETABLET          = 0xa003,
	CELL_PAD_FAKE_TYPE_COPILOT_1           = 0xa004,
	CELL_PAD_FAKE_TYPE_COPILOT_2           = 0xa005,
	CELL_PAD_FAKE_TYPE_COPILOT_3           = 0xa006,
	CELL_PAD_FAKE_TYPE_COPILOT_4           = 0xa007,
	CELL_PAD_FAKE_TYPE_COPILOT_5           = 0xa008,
	CELL_PAD_FAKE_TYPE_COPILOT_6           = 0xa009,
	CELL_PAD_FAKE_TYPE_COPILOT_7           = 0xa00a,
	CELL_PAD_FAKE_TYPE_LAST,

	CELL_PAD_PCLASS_TYPE_MAX // last item
};

// Profile of a Standard Type Controller
// Profile of a Navigation Type Controller
// Bits 0 – 31 All 0s

// Profile of a Guitar Type Controller
enum
{
	// Basic
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_1       = 0x00000001,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_2       = 0x00000002,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_3       = 0x00000004,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_4       = 0x00000008,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_5       = 0x00000010,
	CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_UP     = 0x00000020,
	CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_DOWN   = 0x00000040,
	CELL_PAD_PCLASS_PROFILE_GUITAR_WHAMMYBAR    = 0x00000080,
	// All Basic                                = 0x000000FF

	// Optional
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H1      = 0x00000100,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H2      = 0x00000200,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H3      = 0x00000400,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H4      = 0x00000800,
	CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H5      = 0x00001000,
	CELL_PAD_PCLASS_PROFILE_GUITAR_5WAY_EFFECT  = 0x00002000,
	CELL_PAD_PCLASS_PROFILE_GUITAR_TILT_SENS    = 0x00004000,
	// All                                      = 0x00007FFF
};

// Profile of a Drum Type Controller
enum
{
	CELL_PAD_PCLASS_PROFILE_DRUM_SNARE     = 0x00000001,
	CELL_PAD_PCLASS_PROFILE_DRUM_TOM       = 0x00000002,
	CELL_PAD_PCLASS_PROFILE_DRUM_TOM2      = 0x00000004,
	CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR = 0x00000008,
	CELL_PAD_PCLASS_PROFILE_DRUM_KICK      = 0x00000010,
	CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT = 0x00000020,
	CELL_PAD_PCLASS_PROFILE_DRUM_CYM_CRASH = 0x00000040,
	CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE  = 0x00000080,
	CELL_PAD_PCLASS_PROFILE_DRUM_KICK2     = 0x00000100,
	// All                                 = 0x000001FF
};

// Profile of a DJ Deck Type Controller
enum
{
	CELL_PAD_PCLASS_PROFILE_DJ_MIXER_ATTACK     = 0x00000001,
	CELL_PAD_PCLASS_PROFILE_DJ_MIXER_CROSSFADER = 0x00000002,
	CELL_PAD_PCLASS_PROFILE_DJ_MIXER_DSP_DIAL   = 0x00000004,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM1    = 0x00000008,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM2    = 0x00000010,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM3    = 0x00000020,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK1_PLATTER    = 0x00000040,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM1    = 0x00000080,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM2    = 0x00000100,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM3    = 0x00000200,
	CELL_PAD_PCLASS_PROFILE_DJ_DECK2_PLATTER    = 0x00000400,
	// All                                      = 0x000007FF
};

// Profile of a Dance Mat Type Controller
enum
{
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_CIRCLE   = 0x00000001,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_CROSS    = 0x00000002,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_TRIANGLE = 0x00000004,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_SQUARE   = 0x00000008,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_RIGHT    = 0x00000010,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_LEFT     = 0x00000020,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_UP       = 0x00000040,
	CELL_PAD_PCLASS_PROFILE_DANCEMAT_DOWN     = 0x00000080,
	// All                                    = 0x000000FF
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

	// Fake helpers
	CELL_PAD_BTN_OFFSET_PRESS_PIGGYBACK,
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

struct CellPadData
{
	be_t<s32> len;
	be_t<u16> button[CELL_PAD_MAX_CODES];
};

static constexpr u16 MOTION_ONE_G = 113;
static constexpr u16 DEFAULT_MOTION_X = 512;
static constexpr u16 DEFAULT_MOTION_Y = 399; // 512 - 113 (113 is 1G gravity)
static constexpr u16 DEFAULT_MOTION_Z = 512;
static constexpr u16 DEFAULT_MOTION_G = 512;

// Fake helper enum
enum PressurePiggybackFlags : u32
{
	CELL_PAD_CTRL_PRESS_RIGHT    = CELL_PAD_BTN_OFFSET_PRESS_RIGHT,
	CELL_PAD_CTRL_PRESS_LEFT     = CELL_PAD_BTN_OFFSET_PRESS_LEFT,
	CELL_PAD_CTRL_PRESS_UP       = CELL_PAD_BTN_OFFSET_PRESS_UP,
	CELL_PAD_CTRL_PRESS_DOWN     = CELL_PAD_BTN_OFFSET_PRESS_DOWN,
	CELL_PAD_CTRL_PRESS_TRIANGLE = CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE,
	CELL_PAD_CTRL_PRESS_CIRCLE   = CELL_PAD_BTN_OFFSET_PRESS_CIRCLE,
	CELL_PAD_CTRL_PRESS_CROSS    = CELL_PAD_BTN_OFFSET_PRESS_CROSS,
	CELL_PAD_CTRL_PRESS_SQUARE   = CELL_PAD_BTN_OFFSET_PRESS_SQUARE,
	CELL_PAD_CTRL_PRESS_L1       = CELL_PAD_BTN_OFFSET_PRESS_L1,
	CELL_PAD_CTRL_PRESS_R1       = CELL_PAD_BTN_OFFSET_PRESS_R1,
	CELL_PAD_CTRL_PRESS_L2       = CELL_PAD_BTN_OFFSET_PRESS_L2,
	CELL_PAD_CTRL_PRESS_R2       = CELL_PAD_BTN_OFFSET_PRESS_R2,
};

constexpr u32 special_button_offset = 666; // Must not conflict with other CELL offsets like ButtonDataOffset

enum special_button_value
{
	pressure_intensity,
	analog_limiter,
	orientation_reset
};

struct Button
{
	u32 m_offset = 0;
	u32 m_outKeyCode = 0;
	u16 m_value    = 0;
	bool m_pressed = false;

	std::set<u32> m_key_codes{};

	u16 m_actual_value = 0;              // only used in keyboard_pad_handler
	bool m_analog      = false;          // only used in keyboard_pad_handler
	bool m_trigger     = false;          // only used in keyboard_pad_handler
	std::map<u32, u16> m_pressed_keys{}; // only used in keyboard_pad_handler

	Button(){}
	Button(u32 offset, std::set<u32> key_codes, u32 outKeyCode)
		: m_offset(offset)
		, m_outKeyCode(outKeyCode)
		, m_key_codes(std::move(key_codes))
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
	u32 m_offset = 0;
	u16 m_value = 128;

	std::set<u32> m_key_codes_min{};
	std::set<u32> m_key_codes_max{};

	std::map<u32, u16> m_pressed_keys_min{}; // only used in keyboard_pad_handler
	std::map<u32, u16> m_pressed_keys_max{}; // only used in keyboard_pad_handler

	AnalogStick() {}
	AnalogStick(u32 offset, std::set<u32> key_codes_min, std::set<u32> key_codes_max)
		: m_offset(offset)
		, m_key_codes_min(std::move(key_codes_min))
		, m_key_codes_max(std::move(key_codes_max))
	{}
};

struct AnalogSensor
{
	u32 m_offset = 0;
	u32 m_keyCode = 0;
	b8 m_mirrored = false;
	s16 m_shift = 0;
	u16 m_value = 0;

	AnalogSensor() {}
	AnalogSensor(u32 offset, u32 key_code, b8 mirrored, s16 shift, u16 value)
		: m_offset(offset)
		, m_keyCode(key_code)
		, m_mirrored(mirrored)
		, m_shift(shift)
		, m_value(value)
	{}
};

struct VibrateMotor
{
	bool m_is_large_motor = false;
	u8 m_value = 0;

	VibrateMotor() {}
	VibrateMotor(bool is_large_motor, u8 value)
		: m_is_large_motor(is_large_motor)
		, m_value(value)
	{}
};

struct ps_move_data
{
	u32 external_device_id = 0;
	std::array<u8, 38> external_device_read{};  // CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE
	std::array<u8, 40> external_device_write{}; // CELL_GEM_EXTERNAL_PORT_OUTPUT_SIZE
	std::array<u8, 5> external_device_data{};
	bool external_device_connected = false;
	bool external_device_read_requested = false;
	bool external_device_write_requested = false;

	bool calibration_requested = false;
	bool calibration_succeeded = false;

	bool magnetometer_enabled = false;
	bool orientation_enabled = false;

	static constexpr std::array<f32, 4> default_quaternion { 1.0f, 0.0f, 0.0f, 0.0f };
	std::array<f32, 4> quaternion = default_quaternion; // quaternion orientation (x,y,z,w) of controller relative to default (facing the camera with buttons up)
	f32 accelerometer_x = 0.0f; // linear velocity in m/s²
	f32 accelerometer_y = 0.0f; // linear velocity in m/s²
	f32 accelerometer_z = 0.0f; // linear velocity in m/s²
	f32 gyro_x = 0.0f; // angular velocity in rad/s
	f32 gyro_y = 0.0f; // angular velocity in rad/s
	f32 gyro_z = 0.0f; // angular velocity in rad/s
	f32 magnetometer_x = 0.0f;
	f32 magnetometer_y = 0.0f;
	f32 magnetometer_z = 0.0f;
	s16 temperature = 0;

	void reset_sensors();
};

struct Pad
{
	const pad_handler m_pad_handler;
	const u32 m_player_id;

	bool m_buffer_cleared{true};
	u32 m_port_status{0};
	u32 m_device_capability{0};
	u32 m_device_type{0};
	u32 m_class_type{0};
	u32 m_class_profile{0};

	u16 m_vendor_id{0};
	u16 m_product_id{0};

	u64 m_disconnection_timer{0};

	s32 m_pressure_intensity_button_index{-1}; // Special button index. -1 if not set.
	bool m_pressure_intensity_button_pressed{}; // Last sensitivity button press state, used for toggle.
	bool m_pressure_intensity_toggled{}; // Whether the sensitivity is toggled on or off.
	u8 m_pressure_intensity{127}; // 0-255
	bool m_adjust_pressure_last{}; // only used in keyboard_pad_handler
	bool get_pressure_intensity_button_active(bool is_toggle_mode, u32 player_id);

	s32 m_analog_limiter_button_index{-1}; // Special button index. -1 if not set.
	bool m_analog_limiter_button_pressed{}; // Last sensitivity button press state, used for toggle.
	bool m_analog_limiter_toggled{}; // Whether the sensitivity is toggled on or off.
	bool m_analog_limiter_enabled_last{}; // only used in keyboard_pad_handler
	bool get_analog_limiter_button_active(bool is_toggle_mode, u32 player_id);

	s32 m_orientation_reset_button_index{-1}; // Special button index. -1 if not set.
	bool get_orientation_reset_button_active();

	u64 m_last_rumble_time_us{0};

	// Cable State:   0 - 1  plugged in ?
	u8 m_cable_state{0};

	// DS4: 0 - 9 while unplugged, 0 - 10 while plugged in, 11 charge complete
	// XInput: 0 = Empty, 1 = Low, 2 = Medium, 3 = Full
	u8 m_battery_level{0};

	std::vector<Button> m_buttons;
	std::array<AnalogStick, 4> m_sticks{};
	std::array<AnalogSensor, 4> m_sensors{};
	std::array<VibrateMotor, 2> m_vibrateMotors{};

	std::vector<Button> m_buttons_external;
	std::array<AnalogStick, 4> m_sticks_external{};

	std::vector<std::shared_ptr<Pad>> copilots;

	// These hold bits for their respective buttons
	u16 m_digital_1{0};
	u16 m_digital_2{0};

	// All sensors go from 0-255
	u16 m_analog_left_x{128};
	u16 m_analog_left_y{128};
	u16 m_analog_right_x{128};
	u16 m_analog_right_y{128};

	u16 m_press_right{0};
	u16 m_press_left{0};
	u16 m_press_up{0};
	u16 m_press_down{0};
	u16 m_press_triangle{0};
	u16 m_press_circle{0};
	u16 m_press_cross{0};
	u16 m_press_square{0};
	u16 m_press_L1{0};
	u16 m_press_L2{0};
	u16 m_press_R1{0};
	u16 m_press_R2{0};

	// Except for these...0-1023
	u16 m_sensor_x{DEFAULT_MOTION_X};
	u16 m_sensor_y{DEFAULT_MOTION_Y};
	u16 m_sensor_z{DEFAULT_MOTION_Z};
	u16 m_sensor_g{DEFAULT_MOTION_G};

	bool ldd{false};
	CellPadData ldd_data{};

	bool is_fake_pad = false;

	ps_move_data move_data{};

	explicit Pad(pad_handler handler, u32 player_id, u32 port_status, u32 device_capability, u32 device_type)
		: m_pad_handler(handler)
		, m_player_id(player_id)
		, m_port_status(port_status)
		, m_device_capability(device_capability)
		, m_device_type(device_type)
	{
	}

	void Init(u32 port_status, u32 device_capability, u32 device_type, u32 class_type, u32 class_profile, u16 vendor_id, u16 product_id, u8 pressure_intensity_percent)
	{
		m_port_status = port_status;
		m_device_capability = device_capability;
		m_device_type = device_type;
		m_class_type = class_type;
		m_class_profile = class_profile;
		m_vendor_id = vendor_id;
		m_product_id = product_id;
		m_pressure_intensity = (255 * pressure_intensity_percent) / 100;
	}

	u32 copilot_player() const
	{
		if (m_class_type >= CELL_PAD_FAKE_TYPE_COPILOT_1 && m_class_type <= CELL_PAD_FAKE_TYPE_COPILOT_7)
		{
			return m_class_type - CELL_PAD_FAKE_TYPE_COPILOT_1;
		}

		return umax;
	}

	bool is_copilot() const
	{
		const u32 copilot_player_id = copilot_player();
		return copilot_player_id != umax && copilot_player_id != m_player_id;
	}

	bool is_connected() const
	{
		return !!(m_port_status & CELL_PAD_STATUS_CONNECTED);
	}
};
