#pragma once

#include <cmath>
#include "pad_types.h"
#include "pad_config.h"
#include "pad_config_types.h"
#include "Utilities/types.h"

struct PadDevice
{
	pad_config* config{ nullptr };
};

using pad_preview_values = std::array<int, 6>;
using pad_callback = std::function<void(u16 /*button_value*/, std::string /*button_name*/, std::string /*pad_name*/, u32 /*battery_level*/, pad_preview_values /*preview_values*/)>;
using pad_fail_callback = std::function<void(std::string /*pad_name*/)>;

class PadHandlerBase
{
protected:
	enum button
	{
		up,
		down,
		left,
		right,
		cross,
		square,
		circle,
		triangle,
		l1,
		l2,
		l3,
		r1,
		r2,
		r3,
		start,
		select,
		ps,
		//reserved,
		ls_left,
		ls_right,
		ls_down,
		ls_up,
		rs_left,
		rs_right,
		rs_down,
		rs_up,

		button_count
	};

	enum connection
	{
		no_data,
		connected,
		disconnected
	};

	static const u32 MAX_GAMEPADS = 7;

	std::array<bool, MAX_GAMEPADS> last_connection_status{{ false, false, false, false, false, false, false }};

	std::string m_name_string;
	size_t m_max_devices = 0;
	int m_trigger_threshold = 0;
	int m_thumb_threshold = 0;

	bool b_has_led = false;
	bool b_has_battery = false;
	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_config = false;
	std::array<pad_config, MAX_GAMEPADS> m_pad_configs;
	std::vector<std::pair<std::shared_ptr<PadDevice>, std::shared_ptr<Pad>>> bindings;
	std::unordered_map<u32, std::string> button_list;
	std::vector<u32> blacklist;

	// Search an unordered map for a string value and return found keycode
	static int FindKeyCode(const std::unordered_map<u32, std::string>& map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	static long FindKeyCode(const std::unordered_map<u64, std::string>& map, const cfg::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	static int FindKeyCodeByString(const std::unordered_map<u32, std::string>& map, const std::string& name, bool fallback = true);

	// Search an unordered map for a string value and return found keycode
	static long FindKeyCodeByString(const std::unordered_map<u64, std::string>& map, const std::string& name, bool fallback = true);

	// Get new scaled value between 0 and 255 based on its minimum and maximum
	static float ScaleStickInput(s32 raw_value, int minimum, int maximum);

	// Get new scaled value between -255 and 255 based on its minimum and maximum
	static float ScaleStickInput2(s32 raw_value, int minimum, int maximum);

	// Get normalized trigger value based on the range defined by a threshold
	u16 NormalizeTriggerInput(u16 value, int threshold);

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum);

	u16 NormalizeStickInput(u16 raw_value, int threshold, int multiplier, bool ignore_threshold = false);

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone);

	// get clamped value between 0 and 255
	static u16 Clamp0To255(f32 input);
	// get clamped value between 0 and 1023
	static u16 Clamp0To1023(f32 input);

	// input has to be [-1,1]. result will be [0,255]
	static u16 ConvertAxis(float value);

	// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
	// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
	// This function assumes inX and inY is already in 0-255
	static std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY, int squircle_factor);

public:
	s32 thumb_min = 0;
	s32 thumb_max = 255;
	s32 trigger_min = 0;
	s32 trigger_max = 255;
	s32 vibration_min = 0;
	s32 vibration_max = 255;
	u32 connected_devices = 0;

	pad_handler m_type;

	std::string name_string() const;
	size_t max_devices() const;
	bool has_config() const;
	bool has_rumble() const;
	bool has_deadzones() const;
	bool has_led() const;
	bool has_battery() const;

	static std::string get_config_dir(pad_handler type, const std::string& title_id = "");
	static std::string get_config_filename(int i, const std::string& title_id = "");

	virtual bool Init() { return true; }
	PadHandlerBase(pad_handler type = pad_handler::null);
	virtual ~PadHandlerBase() = default;
	// Sets window to config the controller(optional)
	virtual void SetPadData(const std::string& /*padId*/, u32 /*largeMotor*/, u32 /*smallMotor*/, s32 /*r*/, s32 /*g*/, s32 /*b*/, bool /*battery_led*/, u32 /*battery_led_brightness*/) {}
	virtual u32 get_battery_level(const std::string& /*padId*/) { return 0; }
	// Return list of devices for that handler
	virtual std::vector<std::string> ListDevices() = 0;
	// Callback called during pad_thread::ThreadFunc
	virtual void ThreadProc();
	// Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device);
	virtual void init_config(pad_config* /*cfg*/, const std::string& /*name*/) = 0;
	virtual void get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons = {});

private:
	virtual std::shared_ptr<PadDevice> get_device(const std::string& /*device*/) { return nullptr; };
	virtual bool get_is_left_trigger(u64 /*keyCode*/) { return false; };
	virtual bool get_is_right_trigger(u64 /*keyCode*/) { return false; };
	virtual bool get_is_left_stick(u64 /*keyCode*/) { return false; };
	virtual bool get_is_right_stick(u64 /*keyCode*/) { return false; };
	virtual PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& /*device*/) { return connection::disconnected; };
	virtual void get_extended_info(const std::shared_ptr<PadDevice>& /*device*/, const std::shared_ptr<Pad>& /*pad*/) {};
	virtual void apply_pad_data(const std::shared_ptr<PadDevice>& /*device*/, const std::shared_ptr<Pad>& /*pad*/) {};
	virtual std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& /*device*/) { return {}; };
	virtual pad_preview_values get_preview_values(std::unordered_map<u64, u16> /*data*/) { return {}; };

protected:
	virtual std::array<u32, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& /*device*/, const pad_config* profile);
	virtual void get_mapping(const std::shared_ptr<PadDevice>& device, const std::shared_ptr<Pad>& pad);
	void TranslateButtonPress(const std::shared_ptr<PadDevice>& device, u64 keyCode, bool& pressed, u16& val, bool ignore_stick_threshold = false, bool ignore_trigger_threshold = false);
	void init_configs();
};
