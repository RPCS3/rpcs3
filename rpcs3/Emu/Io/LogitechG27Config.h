#pragma once

#include "Utilities/Config.h"

#include <mutex>

enum class sdl_mapping_type
{
	button = 0,
	hat,
	axis,
};

enum class hat_component
{
	none = 0,
	up,
	down,
	left,
	right
};

// this was a bitfield, but juggling at least 3 compilers and OSes means no bitfield
// num_buttons:10 << 52 | num_hats:10 << 42 | num_axes:10 << 32 | vendor_id:16 << 16 | product_id:16
struct emulated_g27_device_type_id
{
	// big types to keep 64bit bit shift operations sane
	u64 product_id = 0;
	u64 vendor_id = 0;
	u64 num_axes = 0;
	u64 num_hats = 0;
	u64 num_buttons = 0;

	u64 as_u64() const
	{
		u64 value = product_id;
		value |= vendor_id << 16;
		value |= (num_axes & ((1 << 10) - 1)) << 32;
		value |= (num_hats & ((1 << 10) - 1)) << 42;
		value |= (num_buttons & ((1 << 10) - 1)) << 52;
		return value;
	}
};

struct emulated_logitech_g27_mapping : cfg::node
{
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> device_type_id;
	cfg::_enum<sdl_mapping_type> type;
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> id;
	cfg::_enum<hat_component> hat;
	cfg::_bool reverse;

	emulated_logitech_g27_mapping(cfg::node* owner, std::string name, u64 device_type_id_def, sdl_mapping_type type_def, u64 id_def, hat_component hat_def, bool reverse_def)
		: cfg::node(owner, std::move(name)),
		  device_type_id(this, "device_type_id", device_type_id_def),
		  type(this, "type", type_def),
		  id(this, "id", id_def),
		  hat(this, "hat", hat_def),
		  reverse(this, "reverse", reverse_def)
	{
	}
};

struct emulated_logitech_g27_config : cfg::node
{
public:
	std::mutex m_mutex;

	// TODO these defaults are for a shifter-less G29 + a xbox controller for shifter testing, perhaps find a new default
	// TODO, when a new default is found, use the new device type id style

	emulated_logitech_g27_mapping steering{this, "steering", 0, sdl_mapping_type::axis, 0, hat_component::none, false};
	emulated_logitech_g27_mapping throttle{this, "throttle", 0, sdl_mapping_type::axis, 2, hat_component::none, false};
	emulated_logitech_g27_mapping brake{this, "brake", 0, sdl_mapping_type::axis, 3, hat_component::none, false};
	emulated_logitech_g27_mapping clutch{this, "clutch", 0, sdl_mapping_type::axis, 1, hat_component::none, false};
	emulated_logitech_g27_mapping shift_up{this, "shift_up", 0, sdl_mapping_type::button, 4, hat_component::none, false};
	emulated_logitech_g27_mapping shift_down{this, "shift_down", 0, sdl_mapping_type::button, 5, hat_component::none, false};

	emulated_logitech_g27_mapping up{this, "up", 0, sdl_mapping_type::hat, 0, hat_component::up, false};
	emulated_logitech_g27_mapping down{this, "down", 0, sdl_mapping_type::hat, 0, hat_component::down, false};
	emulated_logitech_g27_mapping left{this, "left", 0, sdl_mapping_type::hat, 0, hat_component::left, false};
	emulated_logitech_g27_mapping right{this, "right", 0, sdl_mapping_type::hat, 0, hat_component::right, false};

	emulated_logitech_g27_mapping triangle{this, "triangle", 0, sdl_mapping_type::button, 3, hat_component::none, false};
	emulated_logitech_g27_mapping cross{this, "cross", 0, sdl_mapping_type::button, 0, hat_component::none, false};
	emulated_logitech_g27_mapping square{this, "square", 0, sdl_mapping_type::button, 1, hat_component::none, false};
	emulated_logitech_g27_mapping circle{this, "circle", 0, sdl_mapping_type::button, 2, hat_component::none, false};

	emulated_logitech_g27_mapping l2{this, "l2", 0, sdl_mapping_type::button, 7, hat_component::none, false};
	emulated_logitech_g27_mapping l3{this, "l3", 0, sdl_mapping_type::button, 11, hat_component::none, false};
	emulated_logitech_g27_mapping r2{this, "r2", 0, sdl_mapping_type::button, 6, hat_component::none, false};
	emulated_logitech_g27_mapping r3{this, "r3", 0, sdl_mapping_type::button, 10, hat_component::none, false};

	emulated_logitech_g27_mapping plus{this, "plus", 0, sdl_mapping_type::button, 19, hat_component::none, false};
	emulated_logitech_g27_mapping minus{this, "minus", 0, sdl_mapping_type::button, 20, hat_component::none, false};

	emulated_logitech_g27_mapping dial_clockwise{this, "dial_clockwise", 0, sdl_mapping_type::button, 21, hat_component::none, false};
	emulated_logitech_g27_mapping dial_anticlockwise{this, "dial_anticlockwise", 0, sdl_mapping_type::button, 22, hat_component::none, false};

	emulated_logitech_g27_mapping select{this, "select", 0, sdl_mapping_type::button, 8, hat_component::none, false};
	emulated_logitech_g27_mapping pause{this, "pause", 0, sdl_mapping_type::button, 9, hat_component::none, false};

	emulated_logitech_g27_mapping shifter_1{this, "shifter_1", 0, sdl_mapping_type::button, 3, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_2{this, "shifter_2", 0, sdl_mapping_type::button, 0, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_3{this, "shifter_3", 0, sdl_mapping_type::button, 2, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_4{this, "shifter_4", 0, sdl_mapping_type::button, 1, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_5{this, "shifter_5", 0, sdl_mapping_type::hat, 0, hat_component::up, false};
	emulated_logitech_g27_mapping shifter_6{this, "shifter_6", 0, sdl_mapping_type::hat, 0, hat_component::down, false};
	emulated_logitech_g27_mapping shifter_r{this, "shifter_r", 0, sdl_mapping_type::hat, 0, hat_component::left, false};
	emulated_logitech_g27_mapping shifter_press{this, "shifter_press", 0, sdl_mapping_type::hat, 0, hat_component::right, false};

	cfg::_bool reverse_effects{this, "reverse_effects", false};
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> ffb_device_type_id{this, "ffb_device_type_id", 0};
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> led_device_type_id{this, "led_device_type_id", 0};

	cfg::_bool enabled{this, "enabled", false};

	emulated_logitech_g27_config();

	bool load();
	void save(bool lock_mutex = true);
	void reset();

private:
	const std::string m_path;
};

extern emulated_logitech_g27_config g_cfg_logitech_g27;
