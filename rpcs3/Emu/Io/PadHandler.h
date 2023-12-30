#pragma once

#include "pad_types.h"
#include "pad_config.h"
#include "pad_config_types.h"
#include "util/types.hpp"

#include <cmath>
#include <functional>
#include <string>
#include <set>
#include <vector>
#include <memory>
#include <unordered_map>

LOG_CHANNEL(input_log, "Input");

class PadDevice
{
public:
	virtual ~PadDevice() = default;
	cfg_pad* config{ nullptr };
	u8 player_id{0};
	u8 large_motor{0};
	u8 small_motor{0};
	std::set<u64> trigger_code_left{};
	std::set<u64> trigger_code_right{};
	std::array<std::set<u64>, 4> axis_code_left{};
	std::array<std::set<u64>, 4> axis_code_right{};
};

struct pad_ensemble
{
	std::shared_ptr<Pad> pad;
	std::shared_ptr<PadDevice> device;
	std::shared_ptr<PadDevice> buddy_device;

	explicit pad_ensemble(std::shared_ptr<Pad> _pad, std::shared_ptr<PadDevice> _device, std::shared_ptr<PadDevice> _buddy_device)
		: pad(_pad), device(_device), buddy_device(_buddy_device)
	{}
};

struct pad_list_entry
{
	std::string name;
	bool is_buddy_only = false;

	explicit pad_list_entry(std::string _name, bool _is_buddy_only)
		: name(std::move(_name)), is_buddy_only(_is_buddy_only)
	{}
};

using pad_preview_values = std::array<int, 6>;
using pad_callback = std::function<void(u16 /*button_value*/, std::string /*button_name*/, std::string /*pad_name*/, u32 /*battery_level*/, pad_preview_values /*preview_values*/)>;
using pad_fail_callback = std::function<void(std::string /*pad_name*/)>;

using motion_preview_values = std::array<u16, 4>;
using motion_callback = std::function<void(std::string /*pad_name*/, motion_preview_values /*preview_values*/)>;
using motion_fail_callback = std::function<void(std::string /*pad_name*/, motion_preview_values /*preview_values*/)>;

class PadHandlerBase
{
public:
	enum class connection
	{
		no_data,
		connected,
		disconnected
	};

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

		skateboard_ir_nose,
		skateboard_ir_tail,
		skateboard_ir_left,
		skateboard_ir_right,
		skateboard_tilt_left,
		skateboard_tilt_right,

		pressure_intensity_button,

		button_count
	};

	static constexpr u32 MAX_GAMEPADS = 7;

	std::array<bool, MAX_GAMEPADS> last_connection_status{{ false, false, false, false, false, false, false }};

	std::string m_name_string;
	usz m_max_devices = 0;
	int m_trigger_threshold = 0;
	int m_thumb_threshold = 0;

	bool b_has_led = false;
	bool b_has_rgb = false;
	bool b_has_player_led = false;
	bool b_has_battery = false;
	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_motion = false;
	bool b_has_config = false;
	bool b_has_pressure_intensity_button = true;
	std::array<cfg_pad, MAX_GAMEPADS> m_pad_configs;
	std::vector<pad_ensemble> m_bindings;
	std::unordered_map<u32, std::string> button_list;
	std::set<u32> blacklist;

	static std::set<u32> narrow_set(const std::set<u64>& src);

	// Search an unordered map for a string value and return found keycode
	template <typename S, typename T>
	static std::set<T> FindKeyCodes(const std::unordered_map<S, std::string>& map, const cfg::string& cfg_string, bool fallback = true)
	{
		std::set<T> key_codes;

		const std::string& def = cfg_string.def;
		const std::vector<std::string> names = cfg_pad::get_buttons(cfg_string);
		T def_code = umax;

		for (const std::string& nam : names)
		{
			for (const auto& [code, name] : map)
			{
				if (name == nam)
				{
					key_codes.insert(static_cast<T>(code));
				}

				if (fallback && name == def)
					def_code = static_cast<T>(code);
			}
		}

		if (!key_codes.empty())
		{
			return key_codes;
		}

		if (fallback)
		{
			if (!names.empty())
				input_log.error("FindKeyCode for [name = %s] returned with [def_code = %d] for [def = %s]", cfg_string.to_string(), def_code, def);

			if (def_code != umax)
			{
				return { def_code };
			}
		}

		return {};
	}

	// Search an unordered map for a string value and return found keycode
	template <typename S, typename T>
	static std::set<T> FindKeyCodes(const std::unordered_map<S, std::string>& map, const std::vector<std::string>& names)
	{
		std::set<T> key_codes;

		for (const std::string& name : names)
		{
			for (const auto& [code, nam] : map)
			{
				if (nam == name)
				{
					key_codes.insert(static_cast<T>(code));
					break;
				}
			}
		}

		if (!key_codes.empty())
		{
			return key_codes;
		}

		return {};
	}

	// Get new multiplied value based on the multiplier
	static s32 MultipliedInput(s32 raw_value, s32 multiplier);

	// Get new scaled value between 0 and 255 based on its minimum and maximum
	static f32 ScaledInput(f32 raw_value, f32 minimum, f32 maximum, f32 deadzone, f32 range = 255.0f);

	// Get new scaled value between -255 and 255 based on its minimum and maximum
	static f32 ScaledAxisInput(f32 raw_value, f32 minimum, f32 maximum, f32 deadzone, f32 range = 255.0f);

	// Get normalized trigger value based on the range defined by a threshold
	u16 NormalizeTriggerInput(u16 value, s32 threshold) const;

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum) const;

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone) const;

	// get clamped value between 0 and 255
	static u16 Clamp0To255(f32 input);
	// get clamped value between 0 and 1023
	static u16 Clamp0To1023(f32 input);

	// input has to be [-1,1]. result will be [0,255]
	static u16 ConvertAxis(f32 value);

	// The DS3, (and i think xbox controllers) give a 'square-ish' type response, so that the corners will give (almost)max x/y instead of the ~30x30 from a perfect circle
	// using a simple scale/sensitivity increase would *work* although it eats a chunk of our usable range in exchange
	// this might be the best for now, in practice it seems to push the corners to max of 20x20, with a squircle_factor of 8000
	// This function assumes inX and inY is already in 0-255
	static std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY, int squircle_factor);

public:
	// s32 thumb_min = 0; // Unused. Make sure all handlers report 0+ values for sticks in get_button_values.
	s32 thumb_max = 255; // NOTE: Better keep this positive
	s32 trigger_min = 0;
	s32 trigger_max = 255;
	u32 connected_devices = 0;

	pad_handler m_type;
	bool m_is_init = false;

	std::string name_string() const;
	usz max_devices() const;
	bool has_config() const;
	bool has_rumble() const;
	bool has_motion() const;
	bool has_deadzones() const;
	bool has_led() const;
	bool has_rgb() const;
	bool has_player_led() const;
	bool has_battery() const;
	bool has_pressure_intensity_button() const;

	u16 NormalizeStickInput(u16 raw_value, s32 threshold, s32 multiplier, bool ignore_threshold = false) const;
	void convert_stick_values(u16& x_out, u16& y_out, const s32& x_in, const s32& y_in, const s32& deadzone, const s32& padsquircling) const;

	virtual bool Init() { return true; }
	PadHandlerBase(pad_handler type = pad_handler::null);
	virtual ~PadHandlerBase() = default;
	// Sets window to config the controller(optional)
	virtual void SetPadData(const std::string& /*padId*/, u8 /*player_id*/, u8 /*large_motor*/, u8 /*small_motor*/, s32 /*r*/, s32 /*g*/, s32 /*b*/, bool /*player_led*/, bool /*battery_led*/, u32 /*battery_led_brightness*/) {}
	virtual u32 get_battery_level(const std::string& /*padId*/) { return 0; }
	// Return list of devices for that handler
	virtual std::vector<pad_list_entry> list_devices() = 0;
	// Callback called during pad_thread::ThreadFunc
	virtual void process();
	// Binds a Pad to a device
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad, u8 player_id);
	virtual void init_config(cfg_pad* cfg) = 0;
	virtual connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, bool get_blacklist, const std::vector<std::string>& buttons = {});
	virtual void get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors);
	virtual std::unordered_map<u32, std::string> get_motion_axis_list() const { return {}; }

private:
	virtual std::shared_ptr<PadDevice> get_device(const std::string& /*device*/) { return nullptr; }
	virtual bool get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& /*device*/) { return connection::disconnected; }
	virtual void get_extended_info(const pad_ensemble& /*binding*/) {}
	virtual void apply_pad_data(const pad_ensemble& /*binding*/) {}
	virtual std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& /*device*/) { return {}; }
	virtual pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& /*data*/) { return {}; }

protected:
	virtual std::array<std::set<u32>, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg);
	virtual void get_mapping(const pad_ensemble& binding);
	void TranslateButtonPress(const std::shared_ptr<PadDevice>& device, u64 keyCode, bool& pressed, u16& val, bool ignore_stick_threshold = false, bool ignore_trigger_threshold = false);
	void init_configs();
	cfg_pad* get_config(const std::string& pad_id);
};
