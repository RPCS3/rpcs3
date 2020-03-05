#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/CRC.h"
#include "hidapi.h"

class ds4_pad_handler final : public PadHandlerBase
{
	// These are all the possible buttons on a standard DS4 controller
	// The touchpad is restricted to its button for now (or forever?)
	enum DS4KeyCodes
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
		//R2But,
		R3,
		L1,
		//L2But,
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

	enum DS4CalibIndex
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

	struct DS4CalibData
	{
		s16 bias;
		s32 sensNumer;
		s32 sensDenom;
	};

	enum class DS4DataStatus
	{
		NewData,
		NoNewData,
		ReadError,
	};

	struct DS4Device : public PadDevice
	{
		hid_device* hidDevice{ nullptr };
		std::string path{ "" };
		bool btCon{ false };
		bool hasCalibData{ false };
		std::array<DS4CalibData, DS4CalibIndex::COUNT> calibData{};
		bool newVibrateData{ true };
		u8 largeVibrate{ 0 };
		u8 smallVibrate{ 0 };
		std::array<u8, 64> padData{};
		u8 batteryLevel{ 0 };
		u8 last_battery_level { 0 };
		u8 cableState{ 0 };
		u8 led_delay_on{ 0 };
		u8 led_delay_off{ 0 };
		bool is_initialized{ false };
	};

	const u16 DS4_VID = 0x054C;

	// pid's of connected ds4
	const std::array<u16, 3> ds4Pids = { { 0xBA0, 0x5C4, 0x09CC } };

	// pseudo 'controller id' to keep track of unique controllers
	std::unordered_map<std::string, std::shared_ptr<DS4Device>> controllers;
	CRCPP::CRC::Table<u32, 32> crcTable{ CRCPP::CRC::CRC_32() };

public:
	ds4_pad_handler();
	~ds4_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	bool is_init = false;
	DS4DataStatus status;
	u32 get_battery_color(u8 battery_level, int brightness);

private:
	std::shared_ptr<DS4Device> GetDS4Device(const std::string& padId, bool try_reconnect = false);
	// Copies data into padData if status is NewData, otherwise buffer is untouched
	DS4DataStatus GetRawData(const std::shared_ptr<DS4Device>& ds4Device);
	// This function gets us usuable buffer from the rawbuffer of padData
	// Todo: this currently only handles 'buttons' and not axis or sensors for the time being
	bool GetCalibrationData(const std::shared_ptr<DS4Device>& ds4Device);
	void CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo);
	int SendVibrateData(const std::shared_ptr<DS4Device>& device);
	inline s16 ApplyCalibration(s32 rawValue, const DS4CalibData& calibData)
	{
		const s32 biased = rawValue - calibData.bias;
		const s32 quot = calibData.sensNumer / calibData.sensDenom;
		const s32 rem = calibData.sensNumer % calibData.sensDenom;
		const s32 output = (quot * biased) + ((rem * biased) / calibData.sensDenom);

		if (output > std::numeric_limits<s16>::max())
			return std::numeric_limits<s16>::max();
		else if (output < std::numeric_limits<s16>::min())
			return std::numeric_limits<s16>::min();
		else return static_cast<s16>(output);
	}

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
