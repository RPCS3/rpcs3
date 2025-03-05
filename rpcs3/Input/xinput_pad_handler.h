#pragma once

#include "Emu/Io/PadHandler.h"
#include "util/dyn_lib.hpp"

#include <unordered_map>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>

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

struct SCP_DS3_ACCEL
{
	unsigned short SCP_ACCEL_X;
	unsigned short SCP_ACCEL_Z;
	unsigned short SCP_ACCEL_Y;
	unsigned short SCP_GYRO;
};

class xinput_pad_handler final : public PadHandlerBase
{
	// These are all the possible buttons on a standard xbox 360 or xbox one controller
	enum XInputKeyCodes
	{
		None = 0,

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

		LT_Pos,
		LT_Neg,
		RT_Pos,
		RT_Neg,

		LSXNeg,
		LSXPos,
		LSYNeg,
		LSYPos,
		RSXNeg,
		RSXPos,
		RSYNeg,
		RSYPos
	};

	using PadButtonValues = std::unordered_map<u64, u16>;

	struct XInputDevice : public PadDevice
	{
		u32 deviceNumber{ 0 };
		bool is_scp_device{ false };
		DWORD state{ ERROR_NOT_CONNECTED }; // holds internal controller state change
		SCP_EXTN state_scp{};
		XINPUT_STATE state_base{};
	};

public:
	xinput_pad_handler();
	~xinput_pad_handler();

	bool Init() override;

	std::vector<pad_list_entry> list_devices() override;
	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(cfg_pad* cfg) override;

private:
	using PFN_XINPUTGETEXTENDED = DWORD(WINAPI*)(DWORD, SCP_EXTN*);
	using PFN_XINPUTGETCUSTOMDATA = DWORD(WINAPI*)(DWORD, DWORD, void*);
	using PFN_XINPUTGETSTATE = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
	using PFN_XINPUTSETSTATE = DWORD(WINAPI*)(DWORD, XINPUT_VIBRATION*);
	using PFN_XINPUTGETBATTERYINFORMATION = DWORD(WINAPI*)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);

	int GetDeviceNumber(const std::string& padId);
	static PadButtonValues get_button_values_base(const XINPUT_STATE& state, trigger_recognition_mode trigger_mode);
	static PadButtonValues get_button_values_scp(const SCP_EXTN& state, trigger_recognition_mode trigger_mode);

	PFN_XINPUTGETEXTENDED xinputGetExtended{ nullptr };
	PFN_XINPUTGETCUSTOMDATA xinputGetCustomData{ nullptr };
	PFN_XINPUTGETSTATE xinputGetState{ nullptr };
	PFN_XINPUTSETSTATE xinputSetState{ nullptr };
	PFN_XINPUTGETBATTERYINFORMATION xinputGetBatteryInformation{ nullptr };
	utils::dynamic_library library;

	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};
