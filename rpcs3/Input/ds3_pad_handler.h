#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

class ds3_device : public HidDevice
{
public:
#ifdef _WIN32
	u8 report_id = 0;
#endif
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

	void SetPadData(const std::string& padId, u8 player_id, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
	ds3_pad_handler::DataStatus get_data(ds3_device* ds3dev) override;
	int send_output_report(ds3_device* ds3dev) override;
	void check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view serial) override;

	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};
