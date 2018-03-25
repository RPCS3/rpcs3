#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"
#include "Utilities/CRC.h"
#include "hidapi.h"
#include <limits>
#include <unordered_map>

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

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> button_list =
	{
		{ DS4KeyCodes::Triangle, "Triangle" },
		{ DS4KeyCodes::Circle,   "Circle" },
		{ DS4KeyCodes::Cross,    "Cross" },
		{ DS4KeyCodes::Square,   "Square" },
		{ DS4KeyCodes::Left,     "Left" },
		{ DS4KeyCodes::Right,    "Right" },
		{ DS4KeyCodes::Up,       "Up" },
		{ DS4KeyCodes::Down,     "Down" },
		{ DS4KeyCodes::R1,       "R1" },
		{ DS4KeyCodes::R2,       "R2" },
		{ DS4KeyCodes::R3,       "R3" },
		{ DS4KeyCodes::Options,  "Options" },
		{ DS4KeyCodes::Share,    "Share" },
		{ DS4KeyCodes::PSButton, "PS Button" },
		{ DS4KeyCodes::TouchPad, "Touch Pad" },
		{ DS4KeyCodes::L1,       "L1" },
		{ DS4KeyCodes::L2,       "L2" },
		{ DS4KeyCodes::L3,       "L3" },
		{ DS4KeyCodes::LSXNeg,   "LS X-" },
		{ DS4KeyCodes::LSXPos,   "LS X+" },
		{ DS4KeyCodes::LSYPos,   "LS Y+" },
		{ DS4KeyCodes::LSYNeg,   "LS Y-" },
		{ DS4KeyCodes::RSXNeg,   "RS X-" },
		{ DS4KeyCodes::RSXPos,   "RS X+" },
		{ DS4KeyCodes::RSYPos,   "RS Y+" },
		{ DS4KeyCodes::RSYNeg,   "RS Y-" }
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

	struct DS4Device
	{
		hid_device* hidDevice{ nullptr };
		pad_config* config{ nullptr };
		std::string path{ "" };
		bool btCon{ false };
		bool hasCalibData{ false };
		std::array<DS4CalibData, DS4CalibIndex::COUNT> calibData;
		bool newVibrateData{ true };
		u8 largeVibrate{ 0 };
		u8 smallVibrate{ 0 };
		std::array<u8, 64> padData;
		u8 batteryLevel{ 0 };
		u8 cableState{ 0 };
		u8 led_delay_on{ 0 };
		u8 led_delay_off{ 0 };
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
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& buttonCallback, bool get_blacklist = false, std::vector<std::string> buttons = {}) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	bool is_init = false;

	std::vector<u32> blacklist;
	std::vector<std::pair<std::shared_ptr<DS4Device>, std::shared_ptr<Pad>>> bindings;
	std::shared_ptr<DS4Device> m_dev;

private:
	std::shared_ptr<DS4Device> GetDevice(const std::string& padId);
	void TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold = false) override;
	void ProcessDataToPad(const std::shared_ptr<DS4Device>& ds4Device, const std::shared_ptr<Pad>& pad);
	// Copies data into padData if status is NewData, otherwise buffer is untouched
	DS4DataStatus GetRawData(const std::shared_ptr<DS4Device>& ds4Device);
	// This function gets us usuable buffer from the rawbuffer of padData
	// Todo: this currently only handles 'buttons' and not axis or sensors for the time being
	std::array<u16, DS4KeyCodes::KeyCodeCount> GetButtonValues(const std::shared_ptr<DS4Device>& ds4Device);
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
};
