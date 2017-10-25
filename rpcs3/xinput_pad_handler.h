#pragma once

#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>

#include <QString>

namespace XINPUT_INFO
{
	const DWORD THREAD_TIMEOUT = 1000;
	const DWORD THREAD_SLEEP = 10;
	const DWORD THREAD_SLEEP_INACTIVE = 100;
	const DWORD GUIDE_BUTTON = 0x0400;
	const DWORD GAMEPAD_BUTTONS = 16;
	const LPCWSTR LIBRARY_FILENAMES[] = {
		L"xinput1_4.dll",
		L"xinput1_3.dll",
		L"xinput1_2.dll",
		L"xinput9_1_0.dll"
	};
}

class xinput_pad_handler final : public PadHandlerBase
{
	enum TriggersNSticks
	{
		TRIGGER_LEFT = 0x9001,
		TRIGGER_RIGHT,
		STICK_L_LEFT,
		STICK_L_RIGHT,
		STICK_L_UP,
		STICK_L_DOWN,
		STICK_R_LEFT,
		STICK_R_RIGHT,
		STICK_R_UP,
		STICK_R_DOWN,
	};

	const std::unordered_map<u32, std::string> button_list =
	{
		{ XINPUT_GAMEPAD_A, "A" },
		{ XINPUT_GAMEPAD_B, "B" },
		{ XINPUT_GAMEPAD_X, "X" },
		{ XINPUT_GAMEPAD_Y, "Y" },
		{ XINPUT_GAMEPAD_DPAD_LEFT, "Left" },
		{ XINPUT_GAMEPAD_DPAD_RIGHT, "Right" },
		{ XINPUT_GAMEPAD_DPAD_UP, "Up" },
		{ XINPUT_GAMEPAD_DPAD_DOWN, "Down" },
		{ XINPUT_GAMEPAD_LEFT_SHOULDER, "LB" },
		{ XINPUT_GAMEPAD_RIGHT_SHOULDER, "RB" },
		{ XINPUT_GAMEPAD_BACK, "Back" },
		{ XINPUT_GAMEPAD_START, "Start" },
		{ XINPUT_GAMEPAD_LEFT_THUMB, "LS" },
		{ XINPUT_GAMEPAD_RIGHT_THUMB, "RS" },
		{ XINPUT_INFO::GUIDE_BUTTON, "Guide" },
		{ TRIGGER_LEFT, "LT" },
		{ TRIGGER_RIGHT, "RT" },
		{ STICK_L_LEFT, "LS X-" },
		{ STICK_L_RIGHT, "LS X+" },
		{ STICK_L_UP, "LS Y+" },
		{ STICK_L_DOWN, "LS Y-" },
		{ STICK_R_LEFT, "RS X-" },
		{ STICK_R_RIGHT, "RS X+" },
		{ STICK_R_UP, "RS Y+" },
		{ STICK_R_DOWN, "RS Y-" }
	};

public:
	xinput_pad_handler();
	~xinput_pad_handler();

	bool Init() override;
	void Close();

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void ConfigController(const std::string& device) override;
	std::string GetButtonName(u32 btn) override;
	void GetNextButtonPress(const std::string& padid, const std::function<void(u32, std::string)>& callback) override;
	void TestVibration(const std::string& padId, u32 largeMotor, u32 smallMotor) override;
	void TranslateButtonPress(u32 keyCode, bool& pressed, u16& value, bool ignore_threshold = false) override;

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);

private:
	bool is_init;
	HMODULE library;
	PFN_XINPUTGETSTATE xinputGetState;
	PFN_XINPUTSETSTATE xinputSetState;
	PFN_XINPUTENABLE xinputEnable;

	std::vector<std::pair<u32, std::shared_ptr<Pad>>> bindings;
	std::array<bool, 7> last_connection_status = {};

	// holds internal controller state change
	XINPUT_STATE state;
	DWORD result;
	DWORD online = 0;
};
