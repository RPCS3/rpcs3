#pragma once

#include "hid_pad_handler.h"

#include <unordered_map>

class skateboard_device : public HidDevice
{
};

class skateboard_pad_handler final : public hid_pad_handler<skateboard_device>
{
	enum skateboard_key_codes
	{
		none = 0,
	};

public:
	skateboard_pad_handler();
	~skateboard_pad_handler();

	void SetPadData(const std::string& padId, u8 player_id, u8 large_motor, u8 small_motor, s32 r, s32 g, s32 b, bool player_led, bool battery_led, u32 battery_led_brightness) override;
	void init_config(cfg_pad* cfg) override;

private:
	DataStatus get_data(skateboard_device* device) override;
	void check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view wide_serial) override;
	int send_output_report(skateboard_device* device) override;

	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data) override;
	void get_extended_info(const pad_ensemble& binding) override;
	void apply_pad_data(const pad_ensemble& binding) override;
};
