#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/CRC.h"
#include "hidapi.h"

#include <unordered_map>

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
		Mic,
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

	enum DualSenseCalibIndex
	{
		// gyro
		PITCH = 0,
		YAW,
		ROLL,

		// accel
		X,
		Y,
		Z,
		COUNT
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

	struct DualSenseCalibData
	{
		s16 bias;
		s32 sens_numer;
		s32 sens_denom;
	};

	struct DualSenseDevice : public PadDevice
	{
		hid_device* hidDevice{ nullptr };
		std::string path{ "" };
		bool btCon{ false };
		u8 bt_sequence{ 0 };
		bool has_calib_data{false};
		std::array<DualSenseCalibData, DualSenseCalibIndex::COUNT> calib_data{};
		DualSenseDataMode dataMode{ DualSenseDataMode::Simple };
		std::array<u8, 64> padData{};
		bool newVibrateData{true};
		u8 largeVibrate{0};
		u8 smallVibrate{0};
		bool init_lightbar{true};
		bool update_lightbar{true};
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
	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	bool is_init = false;
	DualSenseDataStatus status;

private:
	std::shared_ptr<DualSenseDevice> GetDualSenseDevice(const std::string& padId);

	DualSenseDataStatus GetRawData(const std::shared_ptr<DualSenseDevice>& dualsenseDevice);
	bool get_calibration_data(const std::shared_ptr<DualSenseDevice>& dualsense_device);

	inline s16 apply_calibration(s32 rawValue, const DualSenseCalibData& calib_data)
	{
		const s32 biased = rawValue - calib_data.bias;
		const s32 quot   = calib_data.sens_numer / calib_data.sens_denom;
		const s32 rem    = calib_data.sens_numer % calib_data.sens_denom;
		const s32 output = (quot * biased) + ((rem * biased) / calib_data.sens_denom);

		if (output > INT16_MAX)
			return INT16_MAX;
		else if (output < INT16_MIN)
			return INT16_MIN;
		else
			return static_cast<s16>(output);
	}

	void CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo);
	int send_output_report(const std::shared_ptr<DualSenseDevice>& device);
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
};
