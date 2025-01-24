#pragma once

#include "pad_types.h"
#include "pad_config.h"
#include "pad_config_types.h"
#include "util/types.hpp"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
#include "3rdparty/fusion/fusion/Fusion/FusionAhrs.h"
}
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

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
	bool new_output_data{true};
	steady_clock::time_point last_output;
	std::set<u64> trigger_code_left{};
	std::set<u64> trigger_code_right{};
	std::array<std::set<u64>, 4> axis_code_left{};
	std::array<std::set<u64>, 4> axis_code_right{};

	struct color
	{
		u8 r{};
		u8 g{};
		u8 b{};
	};
	color color_override{};
	bool color_override_active{};

	bool enable_player_leds{};
	bool update_player_leds{true};

	std::shared_ptr<FusionAhrs> ahrs; // Used to calculate quaternions from sensor data
	u64 last_ahrs_update_time_us = 0; // Last ahrs update

	void update_orientation(ps_move_data& move_data);
	void reset_orientation();
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

	enum class trigger_recognition_mode
	{
		any,             // Add all trigger modes to the button map
		one_directional, // Treat trigger axis as one-directional only
		two_directional  // Treat trigger axis as two-directional only (similar to sticks)
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
		analog_limiter_button,
		orientation_reset_button,

		button_count
	};

	static constexpr u32 MAX_GAMEPADS = 7;
	static constexpr u16 button_press_threshold = 50;
	static constexpr u16 touch_threshold = static_cast<u16>(255 * 0.9f);
	static constexpr auto min_output_interval = 300ms;

	std::array<bool, MAX_GAMEPADS> last_connection_status{{ false, false, false, false, false, false, false }};

	std::string m_name_string;
	usz m_max_devices = 0;
	u32 m_trigger_threshold = 0;
	u32 m_thumb_threshold = 0;
	trigger_recognition_mode m_trigger_recognition_mode = trigger_recognition_mode::any;

	bool b_has_led = false;
	bool b_has_rgb = false;
	bool b_has_player_led = false;
	bool b_has_battery = false;
	bool b_has_battery_led = false;
	bool b_has_deadzones = false;
	bool b_has_rumble = false;
	bool b_has_motion = false;
	bool b_has_config = false;
	bool b_has_pressure_intensity_button = true;
	bool b_has_analog_limiter_button = true;
	bool b_has_orientation = false;

	std::array<cfg_pad, MAX_GAMEPADS> m_pad_configs;
	std::vector<pad_ensemble> m_bindings;
	std::unordered_map<u32, std::string> button_list;
	std::unordered_map<u32, u16> min_button_values;
	std::set<u32> blacklist;

	std::shared_ptr<Pad> m_pad_for_pad_settings;

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
	u16 NormalizeTriggerInput(u16 value, u32 threshold) const;

	// normalizes a directed input, meaning it will correspond to a single "button" and not an axis with two directions
	// the input values must lie in 0+
	u16 NormalizeDirectedInput(s32 raw_value, s32 threshold, s32 maximum) const;

	// This function normalizes stick deadzone based on the DS3's deadzone, which is ~13%
	// X and Y is expected to be in (-255) to 255 range, deadzone should be in terms of thumb stick range
	// return is new x and y values in 0-255 range
	std::tuple<u16, u16> NormalizeStickDeadzone(s32 inX, s32 inY, u32 deadzone, u32 anti_deadzone) const;

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
	static void ConvertToSquirclePoint(u16& inX, u16& inY, u32 squircle_factor);

public:
	// u32 thumb_min = 0; // Unused. Make sure all handlers report 0+ values for sticks in get_button_values.
	u32 thumb_max = 255;
	u32 trigger_min = 0;
	u32 trigger_max = 255;
	u32 connected_devices = 0;

	pad_handler m_type;
	bool m_is_init = false;

	enum class gui_call_type
	{
		normal,
		get_connection,
		reset_input,
		blacklist
	};

	std::vector<pad_ensemble>& bindings() { return m_bindings; }
	const std::string& name_string() const { return m_name_string; }
	usz max_devices() const { return m_max_devices; }
	bool has_config() const { return b_has_config; }
	bool has_rumble() const { return b_has_rumble; }
	bool has_motion() const { return b_has_motion; }
	bool has_deadzones() const { return b_has_deadzones; }
	bool has_led() const { return b_has_led; }
	bool has_rgb() const { return b_has_rgb; }
	bool has_player_led() const { return b_has_player_led; }
	bool has_battery() const { return b_has_battery; }
	bool has_battery_led() const { return b_has_battery_led; }
	bool has_pressure_intensity_button() const { return b_has_pressure_intensity_button; }
	bool has_analog_limiter_button() const { return b_has_analog_limiter_button; }
	bool has_orientation() const { return b_has_orientation; }

	u16 NormalizeStickInput(u16 raw_value, s32 threshold, s32 multiplier, bool ignore_threshold = false) const;
	void convert_stick_values(u16& x_out, u16& y_out, s32 x_in, s32 y_in, u32 deadzone, u32 anti_deadzone, u32 padsquircling) const;
	void set_trigger_recognition_mode(trigger_recognition_mode mode) { m_trigger_recognition_mode = mode; }

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
	virtual bool bindPadToDevice(std::shared_ptr<Pad> pad);
	virtual void init_config(cfg_pad* cfg) = 0;
	virtual connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons);
	virtual void get_motion_sensors(const std::string& pad_id, const motion_callback& callback, const motion_fail_callback& fail_callback, motion_preview_values preview_values, const std::array<AnalogSensor, 4>& sensors);
	virtual std::unordered_map<u32, std::string> get_motion_axis_list() const { return {}; }

	static constexpr f32 PI = 3.14159265f;

	static f32 degree_to_rad(f32 degree)
	{
		return degree * PI / 180.0f;
	}

	static f32 rad_to_degree(f32 radians)
	{
		return radians * 180.0f / PI;
	};

private:
	virtual std::shared_ptr<PadDevice> get_device(const std::string& /*device*/) { return nullptr; }
	virtual bool get_is_left_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_right_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_left_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_right_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual bool get_is_touch_pad_motion(const std::shared_ptr<PadDevice>& /*device*/, u64 /*keyCode*/) { return false; }
	virtual PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& /*device*/) { return connection::disconnected; }
	virtual void get_extended_info(const pad_ensemble& /*binding*/) {}
	virtual void apply_pad_data(const pad_ensemble& /*binding*/) {}
	virtual std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& /*device*/) { return {}; }
	virtual pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& /*data*/) { return {}; }

	void get_orientation(const pad_ensemble& binding) const;

protected:
	virtual std::array<std::set<u32>, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg);
	virtual void get_mapping(const pad_ensemble& binding);
	void TranslateButtonPress(const std::shared_ptr<PadDevice>& device, u64 keyCode, bool& pressed, u16& val, bool use_stick_multipliers, bool ignore_stick_threshold = false, bool ignore_trigger_threshold = false);
	void init_configs();
	cfg_pad* get_config(const std::string& pad_id);

	static void set_raw_orientation(ps_move_data& move_data, f32 accel_x, f32 accel_y, f32 accel_z, f32 gyro_x, f32 gyro_y, f32 gyro_z);
	static void set_raw_orientation(Pad& pad);
};
