#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

namespace reports
{
	constexpr u32 DUALSENSE_ACC_RES_PER_G = 8192;
	constexpr u32 DUALSENSE_GYRO_RES_PER_DEG_S = 86; // technically this could be 1024, but keeping it at 86 keeps us within 16 bits of precision
	constexpr u32 DUALSENSE_CALIBRATION_REPORT_SIZE = 41;
	constexpr u32 DUALSENSE_FIRMWARE_REPORT_SIZE = 64;
	constexpr u32 DUALSENSE_PAIRING_REPORT_SIZE = 20;
	constexpr u32 DUALSENSE_BLUETOOTH_REPORT_SIZE = 78;
	constexpr u32 DUALSENSE_USB_REPORT_SIZE = 63; // 64 in hid, because of the report ID being added
	constexpr u32 DUALSENSE_COMMON_REPORT_SIZE = 47;
	constexpr u32 DUALSENSE_USB_INPUT_REPORT_SIZE = 64;
	constexpr u32 DUALSENSE_BLUETOOTH_INPUT_REPORT_SIZE = 78;
	constexpr u32 DUALSENSE_INPUT_REPORT_GYRO_X_OFFSET = 15;
	constexpr u32 DUALSENSE_TOUCHPAD_WIDTH  = 1920;
	constexpr u32 DUALSENSE_TOUCHPAD_HEIGHT = 1080;
	constexpr u32 DUALSENSE_TOUCH_POINT_INACTIVE = 0x80;

	enum
	{
		VALID_FLAG_0_COMPATIBLE_VIBRATION            = 0x01,
		VALID_FLAG_0_HAPTICS_SELECT                  = 0x02,
		VALID_FLAG_0_SET_LEFT_TRIGGER_MOTOR          = 0x04,
		VALID_FLAG_0_SET_RIGHT_TRIGGER_MOTOR         = 0x08,
		VALID_FLAG_0_SET_AUDIO_VOLUME                = 0x10,
		VALID_FLAG_0_TOGGLE_AUDIO                    = 0x20,
		VALID_FLAG_0_SET_MICROPHONE_VOLUME           = 0x40,
		VALID_FLAG_0_TOGGLE_MICROPHONE               = 0x80,

		VALID_FLAG_1_TOGGLE_MIC_BUTTON_LED           = 0x01,
		VALID_FLAG_1_POWER_SAVE_CONTROL_ENABLE       = 0x02,
		VALID_FLAG_1_LIGHTBAR_CONTROL_ENABLE         = 0x04,
		VALID_FLAG_1_RELEASE_LEDS                    = 0x08,
		VALID_FLAG_1_PLAYER_INDICATOR_CONTROL_ENABLE = 0x10,
		VALID_FLAG_1_EFFECT_POWER                    = 0x40,

		VALID_FLAG_2_SET_PLAYER_LED_BRIGHTNESS       = 0x01,
		VALID_FLAG_2_LIGHTBAR_SETUP_CONTROL_ENABLE   = 0x02,
		VALID_FLAG_2_IMPROVED_RUMBLE_EMULATION       = 0x04,

		AUDIO_BIT_FORCE_INTERNAL_MIC                 = 0x01,
		AUDIO_BIT_FORCE_EXTERNAL_MIC                 = 0x02,
		AUDIO_BIT_PAD_EXTERNAL_MIC                   = 0x04,
		AUDIO_BIT_PAD_INTERNAL_MIC                   = 0x08,
		AUDIO_BIT_DISABLE_EXTERNAL_SPEAKERS          = 0x10,
		AUDIO_BIT_ENABLE_INTERNAL_SPEAKERS           = 0x20,

		POWER_SAVE_CONTROL_MIC_MUTE                  = 0x10,
		POWER_SAVE_CONTROL_AUDIO_MUTE                = 0x40,
		LIGHTBAR_SETUP_LIGHT_ON                      = 0x01,
		LIGHTBAR_SETUP_LIGHT_OFF                     = 0x02,
		MIC_BUTTON_LED_ON                            = 0x01,
		MIC_BUTTON_LED_PULSE                         = 0x02,
	};

	struct dualsense_touch_point
	{
		u8 contact;
		u8 x_lo;
		u8 x_hi : 4;
		u8 y_lo : 4;
		u8 y_hi;
	};
	static_assert(sizeof(dualsense_touch_point) == 4);

	struct dualsense_input_report_common
	{
		u8 x;
		u8 y;
		u8 rx;
		u8 ry;
		u8 z;
		u8 rz;
		u8 seq_number;
		u8 buttons[4];
		u8 reserved[4];
		le_t<u16, 1> gyro[3];
		le_t<u16, 1> accel[3];
		le_t<u32, 1> sensor_timestamp;
		u8 reserved2;
		std::array<dualsense_touch_point, 2> points;
		u8 reserved3[12];
		u8 status;
		u8 reserved4[10];
	};

	struct dualsense_input_report_usb
	{
		u8 report_id;
		dualsense_input_report_common common;
	};
	static_assert(sizeof(dualsense_input_report_usb) == DUALSENSE_USB_INPUT_REPORT_SIZE);

	struct dualsense_input_report_bt
	{
		u8 report_id;
		u8 something;
		dualsense_input_report_common common;
		u8 reserved[9];
		u8 crc32[4];
	};
	static_assert(sizeof(dualsense_input_report_bt) == DUALSENSE_BLUETOOTH_INPUT_REPORT_SIZE);

	struct dualsense_output_report_common
	{
		u8 valid_flag_0;
		u8 valid_flag_1;
		u8 motor_right;
		u8 motor_left;
		u8 headphone_volume;
		u8 speaker_volume;
		u8 microphone_volume;
		u8 audio_enable_bits;
		u8 mute_button_led;
		u8 power_save_control;
		u8 right_trigger_effect[11];
		u8 left_trigger_effect[11];
		u8 reserved[6];
		u8 valid_flag_2;
		u8 effect_strength_1 : 4; // 0-7 (one for motors, the other for trigger motors, not sure which is which)
		u8 effect_strength_2 : 4; // 0-7 (one for motors, the other for trigger motors, not sure which is which)
		u8 reserved2;
		u8 lightbar_setup;
		u8 led_brightness;
		u8 player_leds;
		u8 lightbar_r;
		u8 lightbar_g;
		u8 lightbar_b;
	};
	static_assert(sizeof(dualsense_output_report_common) == DUALSENSE_COMMON_REPORT_SIZE);

	struct dualsense_output_report_bt
	{
		u8 report_id; // 0x31
		u8 seq_tag;
		u8 tag;
		dualsense_output_report_common common;
		u8 reserved[24];
		u8 crc32[4];
	};
	static_assert(sizeof(dualsense_output_report_bt) == DUALSENSE_BLUETOOTH_REPORT_SIZE);

	struct dualsense_output_report_usb
	{
		u8 report_id; // 0x02
		dualsense_output_report_common common;
		u8 reserved[15];
	};
	static_assert(sizeof(dualsense_output_report_usb) == DUALSENSE_USB_REPORT_SIZE);
}

class DualSenseDevice : public HidDevice
{
public:
	enum class DualSenseDataMode
	{
		Simple,
		Enhanced
	};

	enum class DualSenseFeatureSet
	{
		Normal,
		Edge
	};

	bool bt_controller{false};
	u8 bt_sequence{0};
	bool has_calib_data{false};
	std::array<CalibData, CalibIndex::COUNT> calib_data{};
	reports::dualsense_input_report_common report{}; // No need to have separate reports for usb and bluetooth
	DualSenseDataMode data_mode{DualSenseDataMode::Simple};
	DualSenseFeatureSet feature_set{DualSenseFeatureSet::Normal};
	bool init_lightbar{true};
	bool update_lightbar{true};
	bool release_leds{false};

	// Controls for lightbar pulse. This seems somewhat hacky for now, as I haven't found out a nicer way.
	bool lightbar_on{false};
	bool lightbar_on_old{false};
	steady_clock::time_point last_lightbar_time;
};

class dualsense_pad_handler final : public hid_pad_handler<DualSenseDevice>
{
	enum DualSenseKeyCodes
	{
		None = 0,

		Triangle,
		Circle,
		Cross,
		Square,
		Left,
		Right,
		Up,
		Down,
		R1,
		R3,
		L1,
		L3,
		Share,
		Options,
		PSButton,
		Mic,
		TouchPad,

		Touch_L,
		Touch_R,
		Touch_U,
		Touch_D,

		L2,
		R2,

		LSXNeg,
		LSXPos,
		LSYNeg,
		LSYPos,
		RSXNeg,
		RSXPos,
		RSYNeg,
		RSYPos,

		EdgeFnL,
		EdgeFnR,
		EdgeLB,
		EdgeRB,
	};

public:
	dualsense_pad_handler();
	~dualsense_pad_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
	bool get_calibration_data(DualSenseDevice* dev) const;

	DataStatus get_data(DualSenseDevice* device) override;
	void check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view wide_serial) override;
	int send_output_report(DualSenseDevice* device) override;

	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
};
