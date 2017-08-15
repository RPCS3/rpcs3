#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"
#include "Utilities/CRC.h"
#include "hidapi.h"
#include <limits>
#include <unordered_map>

const u32 MAX_GAMEPADS = 7;

class ds4_pad_handler final : public PadHandlerBase
{
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

	struct DS4Device
	{
		hid_device* hidDevice{ nullptr };
		std::string path{ "" };
		bool btCon{ false };
		std::array<DS4CalibData, DS4CalibIndex::COUNT> calibData;
		bool newVibrateData{ true };
		u8 largeVibrate{ 0 };
		u8 smallVibrate{ 0 };
		std::array<u8, 64> padData;
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
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;

private:
	bool is_init;

	// holds internal controller state change
	std::array<bool, MAX_GAMEPADS> last_connection_status = {};

	std::vector<std::pair<std::shared_ptr<DS4Device>, std::shared_ptr<Pad>>> bindings;

private:
	void ProcessData();
	void UpdateRumble();
	bool GetCalibrationData(std::shared_ptr<DS4Device> ds4Device);
	void CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo);
	void SendVibrateData(const std::shared_ptr<DS4Device> device);
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
};
