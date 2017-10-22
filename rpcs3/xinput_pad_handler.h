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

	const DWORD THREAD_TIMEOUT = 1000;
	const DWORD THREAD_SLEEP = 10;
	const DWORD THREAD_SLEEP_INACTIVE = 100;
	const DWORD MAX_GAMEPADS = XUSER_MAX_COUNT;
	const DWORD XINPUT_GAMEPAD_GUIDE = 0x0400;
	const DWORD XINPUT_GAMEPAD_BUTTONS = 16;
	const LPCWSTR LIBRARY_FILENAMES[] = {
		L"xinput1_4.dll",
		L"xinput1_3.dll",
		L"xinput1_2.dll",
		L"xinput9_1_0.dll"
	};

	inline u16 Clamp0To255(f32 input)
	{
		if (input > 255.f)
			return 255;
		else if (input < 0.f)
			return 0;
		else return static_cast<u16>(input);
	}

	inline u16 ConvertAxis(float value)
	{
		return static_cast<u16>((value + 1.0)*(255.0 / 2.0));
	}
}

class xinput_pad_handler final : public PadHandlerBase
{
	struct PAD_INFO
	{
		u32 device_number;
		std::shared_ptr<Pad> pad;
		std::vector<int> mapping;
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
	void GetNextButtonPress(const std::string& padid, const std::function<void(u32, std::string)>& buttonCallback) override;

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);

private:
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY);

private:
	bool is_init;
	float squircle_factor;
	u32 left_stick_deadzone, right_stick_deadzone, left_trigger_threshold, right_trigger_threshold;
	HMODULE library;
	PFN_XINPUTGETSTATE xinputGetState;
	PFN_XINPUTSETSTATE xinputSetState;
	PFN_XINPUTENABLE xinputEnable;

	std::vector<PAD_INFO> bindings;
	std::array<bool, 7> last_connection_status = {};

	// holds internal controller state change
	XINPUT_STATE state;
	DWORD result;
	DWORD online = 0;
};
