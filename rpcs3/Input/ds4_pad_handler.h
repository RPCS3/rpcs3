#pragma once

#include "hid_pad_handler.h"
#include "Utilities/CRC.h"

#include <unordered_map>

class DS4Device : public HidDevice
{
public:
	bool btCon{false};
	bool hasCalibData{false};
	std::array<CalibData, CalibIndex::COUNT> calibData{};
	u8 batteryLevel{0};
	u8 last_battery_level{0};
	u8 cableState{0};
};

class ds4_pad_handler final : public hid_pad_handler<DS4Device>
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

public:
	ds4_pad_handler();
	~ds4_pad_handler();

	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b, bool battery_led, u32 battery_led_brightness) override;
	u32 get_battery_level(const std::string& padId) override;
	void init_config(pad_config* cfg, const std::string& name) override;

private:
	u32 get_battery_color(u8 battery_level, int brightness);

	// This function gets us usuable buffer from the rawbuffer of padData
	bool GetCalibrationData(DS4Device* ds4Device);

	// Copies data into padData if status is NewData, otherwise buffer is untouched
	DataStatus get_data(DS4Device* ds4Device) override;
	int send_output_report(DS4Device* device) override;
	void check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view serial) override;

	bool get_is_left_trigger(u64 keyCode) override;
	bool get_is_right_trigger(u64 keyCode) override;
	bool get_is_left_stick(u64 keyCode) override;
	bool get_is_right_stick(u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	void get_extended_info(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	void apply_pad_data(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
};
