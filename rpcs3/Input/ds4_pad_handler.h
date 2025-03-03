#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

namespace reports
{
	constexpr u32 DS4_ACC_RES_PER_G = 8192;
	constexpr u32 DS4_GYRO_RES_PER_DEG_S = 86; // technically this could be 1024, but keeping it at 86 keeps us within 16 bits of precision
	constexpr u32 DS4_FEATURE_REPORT_USB_CALIBRATION_SIZE = 37;
	constexpr u32 DS4_FEATURE_REPORT_BLUETOOTH_CALIBRATION_SIZE = 41;
	//constexpr u32 DS4_FEATURE_REPORT_PAIRING_INFO_SIZE = 16;
	//constexpr u32 DS4_FEATURE_REPORT_0x81_SIZE = 7;
	constexpr u32 DS4_FEATURE_REPORT_FIRMWARE_INFO_SIZE = 49;
	constexpr u32 DS4_INPUT_REPORT_USB_SIZE = 64;
	constexpr u32 DS4_INPUT_REPORT_BLUETOOTH_SIZE = 78;
	constexpr u32 DS4_OUTPUT_REPORT_USB_SIZE = 32;
	constexpr u32 DS4_OUTPUT_REPORT_BLUETOOTH_SIZE = 78;
	constexpr u32 DS4_TOUCHPAD_WIDTH = 1920;
	constexpr u32 DS4_TOUCHPAD_HEIGHT = 942;
	constexpr u32 DS4_TOUCH_POINT_INACTIVE = 0x80;

	struct ds4_touch_point
	{
		u8 contact;
		u8 x_lo;
		u8 x_hi : 4;
		u8 y_lo : 4;
		u8 y_hi;
	};
	static_assert(sizeof(ds4_touch_point) == 4);

	struct ds4_touch_report
	{
		u8 timestamp;
		std::array<ds4_touch_point, 2> points;
	};
	static_assert(sizeof(ds4_touch_report) == 9);

	struct ds4_input_report_common
	{
		u8 x;
		u8 y;
		u8 rx;
		u8 ry;
		u8 buttons[3];
		u8 z;
		u8 rz;
		le_t<u16, 1> sensor_timestamp;
		u8 sensor_temperature;
		le_t<u16, 1> gyro[3];
		le_t<u16, 1> accel[3];
		u8 reserved2[5];
		u8 status[2];
		u8 reserved3;
	};
	static_assert(sizeof(ds4_input_report_common) == 32);

	struct ds4_input_report_usb
	{
		u8 report_id;
		ds4_input_report_common common;
		u8 num_touch_reports;
		std::array<ds4_touch_report, 3> touch_reports;
		u8 reserved[3];
	};
	static_assert(sizeof(ds4_input_report_usb) == DS4_INPUT_REPORT_USB_SIZE);

	struct ds4_input_report_bt
	{
		u8 report_id;
		u8 reserved[2];
		ds4_input_report_common common;
		u8 num_touch_reports;
		std::array<ds4_touch_report, 4> touch_reports;
		u8 reserved2[2];
		u8 crc32[4];
	};
	static_assert(sizeof(ds4_input_report_bt) == DS4_INPUT_REPORT_BLUETOOTH_SIZE);

	struct ds4_output_report_common
	{
		u8 valid_flag0;
		u8 valid_flag1;
		u8 reserved;
		u8 motor_right;
		u8 motor_left;
		u8 lightbar_red;
		u8 lightbar_green;
		u8 lightbar_blue;
		u8 lightbar_blink_on;
		u8 lightbar_blink_off;
	};
	static_assert(sizeof(ds4_output_report_common) == 10);

	struct ds4_output_report_usb
	{
		u8 report_id;
		ds4_output_report_common common;
		u8 reserved[21];
	};
	static_assert(sizeof(ds4_output_report_usb) == DS4_OUTPUT_REPORT_USB_SIZE);

	struct ds4_output_report_bt
	{
		u8 report_id;
		u8 hw_control;
		u8 audio_control;
		ds4_output_report_common common;
		u8 reserved[61];
		u8 crc32[4];
	};
	static_assert(sizeof(ds4_output_report_bt) == DS4_OUTPUT_REPORT_BLUETOOTH_SIZE);
}

class DS4Device : public HidDevice
{
public:
	bool bt_controller{false};
	bool has_calib_data{false};
	std::array<CalibData, CalibIndex::COUNT> calib_data{};
	reports::ds4_input_report_usb report_usb{};
	reports::ds4_input_report_bt report_bt{};
};

class ds4_pad_handler final : public hid_pad_handler<DS4Device>
{
	// These are all the possible buttons on a standard DS4 controller
	// The touchpad is restricted to its button for now (or forever?)
	enum DS4KeyCodes
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
		//R2But,
		R3,
		L1,
		//L2But,
		L3,
		Share,
		Options,
		PSButton,
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
		RSYPos
	};

public:
	ds4_pad_handler();
	~ds4_pad_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
	// This function gets us usuable buffer from the rawbuffer of padData
	bool GetCalibrationData(DS4Device* ds4Device) const;

	// Copies data into padData if status is NewData, otherwise buffer is untouched
	DataStatus get_data(DS4Device* ds4Device) override;
	int send_output_report(DS4Device* device) override;
	void check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view serial) override;

	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};
