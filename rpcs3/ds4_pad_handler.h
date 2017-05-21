#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"
#include "Utilities/CRC.h"
#include "hidapi.h"
#include <limits>

const u32 MAX_GAMEPADS = 7;


class ds4_thread final : public named_thread
{
private:
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
		bool newVibrateData{true};
		u8 largeVibrate{0};
		u8 smallVibrate{0};
	};

	const u16 DS4_VID = 0x054C;

	// pid's of connected ds4
	const std::array<u16, 3> ds4Pids = {{0xBA0, 0x5C4, 0x09CC }};

	// pseudo 'controller id' to keep track of unique controllers
	std::unordered_map<std::string, DS4Device> controllers;

	std::array<std::array<u8, 64>, MAX_GAMEPADS> padData{};

	void on_task() override;

	std::string get_name() const override { return "DS4 Thread"; }

	semaphore<> mutex;

	CRCPP::CRC::Table<u32, 32> crcTable{ CRCPP::CRC::CRC_32() };

public:
	void on_init(const std::shared_ptr<void>&) override;

	std::array<bool, MAX_GAMEPADS> GetConnectedControllers();

	std::array<std::array<u8, 64>, MAX_GAMEPADS> GetControllerData();

	void SetRumbleData(u32 port, u8 largeVibrate, u8 smallVibrate);


	ds4_thread() = default;


	~ds4_thread();

private:
	bool GetCalibrationData(DS4Device* ds4Device);
	void CheckAddDevice(hid_device* hidDevice, hid_device_info* hidDevInfo);
	void SendVibrateData(const DS4Device& device);
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


class ds4_pad_handler final : public PadHandlerBase
{
public:

	ds4_pad_handler() {}
	~ds4_pad_handler();

	void Init(const u32 max_connect) override;
	void Close();

	PadInfo& GetInfo() override;
	std::vector<Pad>& GetPads() override;
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor) override;

private:
	void ProcessData();

	// holds internal controller state change
	std::array<bool, MAX_GAMEPADS> last_connection_status = {};


	std::shared_ptr<ds4_thread> ds4Thread;
};
