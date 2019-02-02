#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"
#include <libusb.h>
#include <limits>
#include <unordered_map>

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

	const std::unordered_map<u32, std::string> button_list =
	{
		{ DS3KeyCodes::Triangle, "Triangle" },
		{ DS3KeyCodes::Circle,   "Circle" },
		{ DS3KeyCodes::Cross,    "Cross" },
		{ DS3KeyCodes::Square,   "Square" },
		{ DS3KeyCodes::Left,     "Left" },
		{ DS3KeyCodes::Right,    "Right" },
		{ DS3KeyCodes::Up,       "Up" },
		{ DS3KeyCodes::Down,     "Down" },
		{ DS3KeyCodes::R1,       "R1" },
		{ DS3KeyCodes::R2,       "R2" },
		{ DS3KeyCodes::R3,       "R3" },
		{ DS3KeyCodes::Start,    "Start" },
		{ DS3KeyCodes::Select,   "Select" },
		{ DS3KeyCodes::PSButton, "PS Button" },
		{ DS3KeyCodes::L1,       "L1" },
		{ DS3KeyCodes::L2,       "L2" },
		{ DS3KeyCodes::L3,       "L3" },
		{ DS3KeyCodes::LSXNeg,   "LS X-" },
		{ DS3KeyCodes::LSXPos,   "LS X+" },
		{ DS3KeyCodes::LSYPos,   "LS Y+" },
		{ DS3KeyCodes::LSYNeg,   "LS Y-" },
		{ DS3KeyCodes::RSXNeg,   "RS X-" },
		{ DS3KeyCodes::RSXPos,   "RS X+" },
		{ DS3KeyCodes::RSYPos,   "RS Y+" },
		{ DS3KeyCodes::RSYNeg,   "RS Y-" }
	};

	struct ds3_device
	{
		libusb_device *device;
		libusb_device_handle *handle;
		pad_config* config{ nullptr };
		u8 buf[64];
		u8 large_motor = 0;
		u8 small_motor = 0;
		u8 status = DS3Status::Disconnected;
	};

	const u16 DS3_VID = 0x054C;
	const u16 DS3_PID = 0x0268;

	// pseudo 'controller id' to keep track of unique controllers
	std::vector<std::shared_ptr<ds3_device>> controllers;

public:
	ds3_pad_handler();
	~ds3_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, std::string, int[])>& buttonCallback, const std::function<void(std::string)>& fail_callback, bool get_blacklist = false, const std::vector<std::string>& buttons = {}) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	std::shared_ptr<ds3_device> get_device(const std::string& padId);
	ds3_pad_handler::DS3Status get_data(const std::shared_ptr<ds3_device>& ds3dev);
	void process_data(const std::shared_ptr<ds3_device>& ds3dev, const std::shared_ptr<Pad>& pad);
	std::array<std::pair<u16, bool>, ds3_pad_handler::DS3KeyCodes::KeyCodeCount> get_button_values(const std::shared_ptr<ds3_device>& device);
	void send_output_report(const std::shared_ptr<ds3_device>& ds3dev);

private:
	bool is_init = false;

	std::vector<u32> blacklist;

	std::vector<std::pair<std::shared_ptr<ds3_device>, std::shared_ptr<Pad>>> bindings;
	std::shared_ptr<ds3_device> m_dev;
};
