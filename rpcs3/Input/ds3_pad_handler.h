#pragma once

#include "Emu/Io/PadHandler.h"

#include "hidapi.h"

class ds3_pad_handler final : public PadHandlerBase
{
	enum DS3KeyCodes
	{
		Triangle = 0,
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
		RSYPos,

		KeyCodeCount
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

	enum DS3Status : u8
	{
		Disconnected,
		Connected,
		NewData
	};

	struct ds3_device : public PadDevice
	{
		std::string device = {};
		hid_device *handle = nullptr;
		u8 buf[64]{ 0 };
		u8 large_motor = 0;
		u8 small_motor = 0;
		u8 status = DS3Status::Disconnected;
	};

	const u16 DS3_VID = 0x054C;
	const u16 DS3_PID = 0x0268;

#ifdef _WIN32
	const u8 DS3_HID_OFFSET = 0x01;
#else
	const u8 DS3_HID_OFFSET = 0x00;
#endif

	// pseudo 'controller id' to keep track of unique controllers
	std::vector<std::shared_ptr<ds3_device>> controllers;

public:
	ds3_pad_handler();
	~ds3_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	std::shared_ptr<ds3_device> get_ds3_device(const std::string& padId);
	ds3_pad_handler::DS3Status get_data(const std::shared_ptr<ds3_device>& ds3dev);
	void send_output_report(const std::shared_ptr<ds3_device>& ds3dev);

private:
	bool init_usb();

private:
	bool is_init = false;

	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(std::unordered_map<u64, u16> data) override;
};
