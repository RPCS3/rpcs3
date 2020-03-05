#pragma once

#include "Emu/Io/PadHandler.h"
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#include <chrono>
#include <optional>

// ScpToolkit defined structure for pressure sensitive button query
struct SCP_EXTN
{
	float SCP_UP;
	float SCP_RIGHT;
	float SCP_DOWN;
	float SCP_LEFT;

	float SCP_LX;
	float SCP_LY;

	float SCP_L1;
	float SCP_L2;
	float SCP_L3;

	float SCP_RX;
	float SCP_RY;

	float SCP_R1;
	float SCP_R2;
	float SCP_R3;

	float SCP_T;
	float SCP_C;
	float SCP_X;
	float SCP_S;

	float SCP_SELECT;
	float SCP_START;

	float SCP_PS;
};

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

	using PadButtonValues = std::unordered_map<u64, u16>;

	struct XInputDevice : public PadDevice
	{
		u32 deviceNumber{ 0 };
		bool newVibrateData{ true };
		u16 largeVibrate{ 0 };
		u16 smallVibrate{ 0 };
		std::chrono::high_resolution_clock::time_point last_vibration;
		bool is_scp_device{ false };
		DWORD state{ ERROR_NOT_CONNECTED }; // holds internal controller state change
		SCP_EXTN state_scp{ 0 };
		XINPUT_STATE state_base{ 0 };
	};

public:
	xinput_pad_handler();
	~xinput_pad_handler();

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	typedef DWORD (WINAPI * PFN_XINPUTGETEXTENDED)(DWORD, SCP_EXTN *);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);
	typedef DWORD (WINAPI * PFN_XINPUTGETBATTERYINFORMATION)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION *);

private:
	int GetDeviceNumber(const std::string& padId);
	PadButtonValues get_button_values_base(const XINPUT_STATE& state);
	PadButtonValues get_button_values_scp(const SCP_EXTN& state);

	bool is_init{ false };
	HMODULE library{ nullptr };
	PFN_XINPUTGETEXTENDED xinputGetExtended{ nullptr };
	PFN_XINPUTGETSTATE xinputGetState{ nullptr };
	PFN_XINPUTSETSTATE xinputSetState{ nullptr };
	PFN_XINPUTGETBATTERYINFORMATION xinputGetBatteryInformation{ nullptr };

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
