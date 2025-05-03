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

struct emulated_logitech_g27_mapping : cfg::node
{
	cfg::uint<0, 0xFFFFFFFF> device_type_id;
	cfg::_enum<sdl_mapping_type> type;
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> id;
	cfg::_enum<hat_component> hat;
	cfg::_bool reverse;

	emulated_logitech_g27_mapping(cfg::node* owner, std::string name, u32 device_type_id_def, sdl_mapping_type type_def, uint64_t id_def, hat_component hat_def, bool reverse_def)
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

	emulated_logitech_g27_mapping steering{this, "steering", 0x046dc24f, sdl_mapping_type::axis, 0, hat_component::none, false};
	emulated_logitech_g27_mapping throttle{this, "throttle", 0x046dc24f, sdl_mapping_type::axis, 2, hat_component::none, false};
	emulated_logitech_g27_mapping brake{this, "brake", 0x046dc24f, sdl_mapping_type::axis, 3, hat_component::none, false};
	emulated_logitech_g27_mapping clutch{this, "clutch", 0x046dc24f, sdl_mapping_type::axis, 1, hat_component::none, false};
	emulated_logitech_g27_mapping shift_up{this, "shift_up", 0x046dc24f, sdl_mapping_type::button, 4, hat_component::none, false};
	emulated_logitech_g27_mapping shift_down{this, "shift_down", 0x046dc24f, sdl_mapping_type::button, 5, hat_component::none, false};

	emulated_logitech_g27_mapping up{this, "up", 0x046dc24f, sdl_mapping_type::hat, 0, hat_component::up, false};
	emulated_logitech_g27_mapping down{this, "down", 0x046dc24f, sdl_mapping_type::hat, 0, hat_component::down, false};
	emulated_logitech_g27_mapping left{this, "left", 0x046dc24f, sdl_mapping_type::hat, 0, hat_component::left, false};
	emulated_logitech_g27_mapping right{this, "right", 0x046dc24f, sdl_mapping_type::hat, 0, hat_component::right, false};

	emulated_logitech_g27_mapping triangle{this, "triangle", 0x046dc24f, sdl_mapping_type::button, 3, hat_component::none, false};
	emulated_logitech_g27_mapping cross{this, "cross", 0x046dc24f, sdl_mapping_type::button, 0, hat_component::none, false};
	emulated_logitech_g27_mapping square{this, "square", 0x046dc24f, sdl_mapping_type::button, 1, hat_component::none, false};
	emulated_logitech_g27_mapping circle{this, "circle", 0x046dc24f, sdl_mapping_type::button, 2, hat_component::none, false};

	emulated_logitech_g27_mapping l2{this, "l2", 0x046dc24f, sdl_mapping_type::button, 7, hat_component::none, false};
	emulated_logitech_g27_mapping l3{this, "l3", 0x046dc24f, sdl_mapping_type::button, 11, hat_component::none, false};
	emulated_logitech_g27_mapping r2{this, "r2", 0x046dc24f, sdl_mapping_type::button, 6, hat_component::none, false};
	emulated_logitech_g27_mapping r3{this, "r3", 0x046dc24f, sdl_mapping_type::button, 10, hat_component::none, false};

	emulated_logitech_g27_mapping plus{this, "plus", 0x046dc24f, sdl_mapping_type::button, 19, hat_component::none, false};
	emulated_logitech_g27_mapping minus{this, "minus", 0x046dc24f, sdl_mapping_type::button, 20, hat_component::none, false};

	emulated_logitech_g27_mapping dial_clockwise{this, "dial_clockwise", 0x046dc24f, sdl_mapping_type::button, 21, hat_component::none, false};
	emulated_logitech_g27_mapping dial_anticlockwise{this, "dial_anticlockwise", 0x046dc24f, sdl_mapping_type::button, 22, hat_component::none, false};

	emulated_logitech_g27_mapping select{this, "select", 0x046dc24f, sdl_mapping_type::button, 8, hat_component::none, false};
	emulated_logitech_g27_mapping pause{this, "pause", 0x046dc24f, sdl_mapping_type::button, 9, hat_component::none, false};

	emulated_logitech_g27_mapping shifter_1{this, "shifter_1", 0x045e028e, sdl_mapping_type::button, 3, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_2{this, "shifter_2", 0x045e028e, sdl_mapping_type::button, 0, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_3{this, "shifter_3", 0x045e028e, sdl_mapping_type::button, 2, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_4{this, "shifter_4", 0x045e028e, sdl_mapping_type::button, 1, hat_component::none, false};
	emulated_logitech_g27_mapping shifter_5{this, "shifter_5", 0x045e028e, sdl_mapping_type::hat, 0, hat_component::up, false};
	emulated_logitech_g27_mapping shifter_6{this, "shifter_6", 0x045e028e, sdl_mapping_type::hat, 0, hat_component::down, false};
	emulated_logitech_g27_mapping shifter_r{this, "shifter_r", 0x045e028e, sdl_mapping_type::hat, 0, hat_component::left, false};

	cfg::_bool reverse_effects{this, "reverse_effects", true};
	cfg::uint<0, 0xFFFFFFFF> ffb_device_type_id{this, "ffb_device_type_id", 0x046dc24f};
	cfg::uint<0, 0xFFFFFFFF> led_device_type_id{this, "led_device_type_id", 0x046dc24f};

	cfg::_bool enabled{this, "enabled", false};

	emulated_logitech_g27_config();

	bool load();
	void save(bool lock_mutex = true);
	void reset();

private:
	const std::string m_path;
};

extern emulated_logitech_g27_config g_cfg_logitech_g27;
