#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/CRC.h"
#include "hidapi.h"

class dualsense_pad_handler final : public PadHandlerBase
{
	enum DualSenseKeyCodes
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
		Share,
		Options,
		PSButton,
		TouchPad,

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


	enum class DualSenseDataStatus
	{
		NewData,
		NoNewData,
		ReadError,
	};

	enum class DualSenseDataMode
	{
		Simple,
		Enhanced
	};

	struct DualSenseDevice : public PadDevice
	{
		hid_device* hidDevice{ nullptr };
		std::string path{ "" };
		bool btCon{ false };
		DualSenseDataMode dataMode{ DualSenseDataMode::Simple };
		std::array<u8, 64> padData{};
		bool newVibrateData{true};
		u8 largeVibrate{0};
		u8 smallVibrate{0};
	};

	const u16 DUALSENSE_VID = 0x054C;
	const u16 DUALSENSE_PID = 0x0CE6;

	std::unordered_map<std::string, std::shared_ptr<DualSenseDevice>> controllers;
	CRCPP::CRC::Table<u32, 32> crcTable{CRCPP::CRC::CRC_32()};

public:
	dualsense_pad_handler();
	~dualsense_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	bool is_init = false;
	DualSenseDataStatus status;

private:
	std::shared_ptr<DualSenseDevice> GetDualSenseDevice(const std::string& padId);

	DualSenseDataStatus GetRawData(const std::shared_ptr<DualSenseDevice>& dualsenseDevice);

	void CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo);
	int SendVibrateData(const std::shared_ptr<DualSenseDevice>& device);
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
};
