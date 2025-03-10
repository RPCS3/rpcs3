#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

namespace reports
{
	struct ds3_rumble
	{
		u8 padding              = 0x00;
		u8 small_motor_duration = 0xFF; // 0xff means forever
		u8 small_motor_on       = 0x00; // 0 or 1 (off/on)
		u8 large_motor_duration = 0xFF; // 0xff means forever
		u8 large_motor_force    = 0x00; // 0 to 255
	};

	struct ds3_led
	{
		u8 duration             = 0xFF; // total duration, 0xff means forever
		u8 interval_duration    = 0xFF; // interval duration in deciseconds
		u8 enabled              = 0x10;
		u8 interval_portion_off = 0x00; // in percent (100% = 0xFF)
		u8 interval_portion_on  = 0xFF; // in percent (100% = 0xFF)
	};

	struct ds3_output_report
	{
	#ifdef _WIN32
		u8 report_id = 0x00;
		u8 idk_what_this_is[3] = {0x02, 0x00, 0x00};
	#else
		u8 report_id = 0x01;
	#endif
		ds3_rumble rumble;
		u8 padding[4]  = {0x00, 0x00, 0x00, 0x00};
		u8 led_enabled = 0x00; // LED 1 = 0x02, LED 2 = 0x04, etc.
		ds3_led led[4];
		ds3_led led_5;         // reserved for another LED
	};

	struct ds3_input_report
	{
		u8 unknown_0[2];
		u8 buttons[3];
		u8 unknown_1;
		u8 lsx;
		u8 lsy;
		u8 rsx;
		u8 rsy;
		u8 unknown_2[4];
		u8 button_values[12];
		u8 unknown_3[4];
		u8 battery_status;
		u8 unknown_4[10];
		le_t<u16, 1> accel_x;
		le_t<u16, 1> accel_z;
		le_t<u16, 1> accel_y;
		le_t<u16, 1> gyro;
	};
	static_assert(sizeof(ds3_input_report) == 49);
}

class ds3_device : public HidDevice
{
public:
#ifdef _WIN32
	u8 report_id = 0;
#endif
	reports::ds3_input_report report{};
};

class ds3_pad_handler final : public hid_pad_handler<ds3_device>
{
	enum DS3KeyCodes
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
		Select,
		Start,
		PSButton,

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

	enum HidRequest
	{
		HID_GETREPORT = 0x01,
		HID_GETIDLE,
		HID_GETPROTOCOL,
		HID_SETREPORT = 0x09,
		HID_SETIDLE,
		HID_SETPROTOCOL
	};

	enum ReportType
	{
		HIDREPORT_INPUT = 0x0100,
		HIDREPORT_OUTPUT = 0x0200,
		HIDREPORT_FEATURE = 0x0300
	};

	enum DS3Endpoints
	{
		DS3_ENDPOINT_OUT = 0x02,
		DS3_ENDPOINT_IN = 0x81
	};

#ifdef _WIN32
	const u8 DS3_HID_OFFSET = 0x01;
#else
	const u8 DS3_HID_OFFSET = 0x00;
#endif

public:
	ds3_pad_handler();
	~ds3_pad_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
	ds3_pad_handler::DataStatus get_data(ds3_device* ds3dev) override;
	int send_output_report(ds3_device* ds3dev) override;
	void check_add_device(hid_device* hidDevice, hid_enumerated_device_view path, std::wstring_view serial) override;

	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};
