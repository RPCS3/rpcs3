#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <ctime>

namespace XINPUT_INFO
{
	const DWORD THREAD_TIMEOUT = 1000;
	const DWORD THREAD_SLEEP = 10;
	const DWORD THREAD_SLEEP_INACTIVE = 100;
	const DWORD GUIDE_BUTTON = 0x0400;
	const LPCWSTR LIBRARY_FILENAMES[] = {
		L"xinput1_4.dll",
		L"xinput1_3.dll",
		L"xinput1_2.dll",
		L"xinput9_1_0.dll"
	};
}

class xinput_pad_handler final : public PadHandlerBase
{
	// These are all the possible buttons on a standard xbox 360 or xbox one controller
	enum XInputKeyCodes
	{
		A,
		B,
		X,
		Y,
		Left,
		Right,
		Up,
		Down,
		LB,
		RB,
		LS,
		RS,
		Start,
		Back,
		Guide,

		LT,
		RT,

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
		{ XInputKeyCodes::A,      "A" },
		{ XInputKeyCodes::B,      "B" },
		{ XInputKeyCodes::X,      "X" },
		{ XInputKeyCodes::Y,      "Y" },
		{ XInputKeyCodes::Left,   "Left" },
		{ XInputKeyCodes::Right,  "Right" },
		{ XInputKeyCodes::Up,     "Up" },
		{ XInputKeyCodes::Down,   "Down" },
		{ XInputKeyCodes::LB,     "LB" },
		{ XInputKeyCodes::RB,     "RB" },
		{ XInputKeyCodes::Back,   "Back" },
		{ XInputKeyCodes::Start,  "Start" },
		{ XInputKeyCodes::LS,     "LS" },
		{ XInputKeyCodes::RS,     "RS" },
		{ XInputKeyCodes::Guide,  "Guide" },
		{ XInputKeyCodes::LT,     "LT" },
		{ XInputKeyCodes::RT,     "RT" },
		{ XInputKeyCodes::LSXNeg, "LS X-" },
		{ XInputKeyCodes::LSXPos, "LS X+" },
		{ XInputKeyCodes::LSYPos, "LS Y+" },
		{ XInputKeyCodes::LSYNeg, "LS Y-" },
		{ XInputKeyCodes::RSXNeg, "RS X-" },
		{ XInputKeyCodes::RSXPos, "RS X+" },
		{ XInputKeyCodes::RSYPos, "RS Y+" },
		{ XInputKeyCodes::RSYNeg, "RS Y-" }
	};

	struct XInputDevice
	{
		u32 deviceNumber{ 0 };
		bool newVibrateData{ true };
		u8 largeVibrate{ 0 };
		u8 smallVibrate{ 0 };
		clock_t last_vibration{ 0 };
		pad_config* config{ nullptr };
	};

public:
	xinput_pad_handler();
	~xinput_pad_handler();

	bool Init() override;
	void Close();

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, int[])>& callback, bool get_blacklist = false, std::vector<std::string> buttons = {}) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);
	typedef DWORD (WINAPI * PFN_XINPUTGETBATTERYINFORMATION)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION *);

private:
	int GetDeviceNumber(const std::string& padId);
	std::array<u16, XInputKeyCodes::KeyCodeCount> GetButtonValues(const XINPUT_STATE& state);
	void TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold = false) override;

	bool is_init{ false };
	HMODULE library{ nullptr };
	PFN_XINPUTGETSTATE xinputGetState{ nullptr };
	PFN_XINPUTSETSTATE xinputSetState{ nullptr };
	PFN_XINPUTENABLE xinputEnable{ nullptr };
	PFN_XINPUTGETBATTERYINFORMATION xinputGetBatteryInformation{ nullptr };

	std::vector<u32> blacklist;
	std::vector<std::pair<std::shared_ptr<XInputDevice>, std::shared_ptr<Pad>>> bindings;
	std::shared_ptr<XInputDevice> m_dev;

	// holds internal controller state change
	XINPUT_STATE state;
	DWORD result;
};
