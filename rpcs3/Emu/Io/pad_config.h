#pragma once

#include "pad_config_types.h"

#include "Utilities/Config.h"

#include <array>

namespace pad
{
	constexpr static std::string_view keyboard_device_name = "Keyboard";
}

struct cfg_pad final : cfg::node
{
	cfg_pad() {};
	cfg_pad(node* owner, const std::string& name) : cfg::node(owner, name) {}

	cfg::string ls_left{ this, "Left Stick Left", "" };
	cfg::string ls_down{ this, "Left Stick Down", "" };
	cfg::string ls_right{ this, "Left Stick Right", "" };
	cfg::string ls_up{ this, "Left Stick Up", "" };
	cfg::string rs_left{ this, "Right Stick Left", "" };
	cfg::string rs_down{ this, "Right Stick Down", "" };
	cfg::string rs_right{ this, "Right Stick Right", "" };
	cfg::string rs_up{ this, "Right Stick Up", "" };
	cfg::string start{ this, "Start", "" };
	cfg::string select{ this, "Select", "" };
	cfg::string ps{ this, "PS Button", "" };
	cfg::string square{ this, "Square", "" };
	cfg::string cross{ this, "Cross", "" };
	cfg::string circle{ this, "Circle", "" };
	cfg::string triangle{ this, "Triangle", "" };
	cfg::string left{ this, "Left", "" };
	cfg::string down{ this, "Down", "" };
	cfg::string right{ this, "Right", "" };
	cfg::string up{ this, "Up", "" };
	cfg::string r1{ this, "R1", "" };
	cfg::string r2{ this, "R2", "" };
	cfg::string r3{ this, "R3", "" };
	cfg::string l1{ this, "L1", "" };
	cfg::string l2{ this, "L2", "" };
	cfg::string l3{ this, "L3", "" };

	cfg::string pressure_intensity_button{ this, "Pressure Intensity Button", "" };
	cfg::uint<0, 100> pressure_intensity{ this, "Pressure Intensity Percent", 50 };

	cfg::uint<0, 200> lstickmultiplier{ this, "Left Stick Multiplier", 100 };
	cfg::uint<0, 200> rstickmultiplier{ this, "Right Stick Multiplier", 100 };
	cfg::uint<0, 1000000> lstickdeadzone{ this, "Left Stick Deadzone", 0 };
	cfg::uint<0, 1000000> rstickdeadzone{ this, "Right Stick Deadzone", 0 };
	cfg::uint<0, 1000000> ltriggerthreshold{ this, "Left Trigger Threshold", 0 };
	cfg::uint<0, 1000000> rtriggerthreshold{ this, "Right Trigger Threshold", 0 };
	cfg::uint<0, 1000000> lpadsquircling{ this, "Left Pad Squircling Factor", 0 };
	cfg::uint<0, 1000000> rpadsquircling{ this, "Right Pad Squircling Factor", 0 };

	cfg::uint<0, 255> colorR{ this, "Color Value R", 0 };
	cfg::uint<0, 255> colorG{ this, "Color Value G", 0 };
	cfg::uint<0, 255> colorB{ this, "Color Value B", 0 };

	cfg::_bool led_low_battery_blink{ this, "Blink LED when battery is below 20%", true };
	cfg::_bool led_battery_indicator{ this, "Use LED as a battery indicator", false };
	cfg::uint<0, 100> led_battery_indicator_brightness{ this, "LED battery indicator brightness", 50 };

	cfg::_bool enable_vibration_motor_large{ this, "Enable Large Vibration Motor", true };
	cfg::_bool enable_vibration_motor_small{ this, "Enable Small Vibration Motor", true };
	cfg::_bool switch_vibration_motors{ this, "Switch Vibration Motors", false };

	cfg::_enum<mouse_movement_mode> mouse_move_mode{ this, "Mouse Movement Mode", mouse_movement_mode::relative };
	cfg::uint<0, 255> mouse_deadzone_x{ this, "Mouse Deadzone X Axis", 60 };
	cfg::uint<0, 255> mouse_deadzone_y{ this, "Mouse Deadzone Y Axis", 60 };
	cfg::uint<0, 999999> mouse_acceleration_x{ this, "Mouse Acceleration X Axis", 200 };
	cfg::uint<0, 999999> mouse_acceleration_y{ this, "Mouse Acceleration Y Axis", 250 };

	cfg::uint<0, 100> l_stick_lerp_factor{ this, "Left Stick Lerp Factor", 100 };
	cfg::uint<0, 100> r_stick_lerp_factor{ this, "Right Stick Lerp Factor", 100 };
	cfg::uint<0, 100> analog_lerp_factor{ this, "Analog Button Lerp Factor", 100 };
	cfg::uint<0, 100> trigger_lerp_factor{ this, "Trigger Lerp Factor", 100 };

	cfg::uint<0, 5> device_class_type{ this, "Device Class Type", 0 };
	cfg::uint<0, 65535> vendor_id{ this, "Vendor ID", 0 };
	cfg::uint<0, 65535> product_id{ this, "Product ID", 0 };
};

struct cfg_player final : cfg::node
{
	pad_handler def_handler = pad_handler::null;
	cfg_player(node* owner, const std::string& name, pad_handler type) : cfg::node(owner, name), def_handler(type) {}

	cfg::_enum<pad_handler> handler{ this, "Handler", def_handler };
	cfg::string device{ this, "Device", handler.to_string() };
	cfg_pad config{ this, "Config" };
};

struct cfg_input final : cfg::node
{
	cfg_player player1{ this, "Player 1 Input", pad_handler::null };
	cfg_player player2{ this, "Player 2 Input", pad_handler::null };
	cfg_player player3{ this, "Player 3 Input", pad_handler::null };
	cfg_player player4{ this, "Player 4 Input", pad_handler::null };
	cfg_player player5{ this, "Player 5 Input", pad_handler::null };
	cfg_player player6{ this, "Player 6 Input", pad_handler::null };
	cfg_player player7{ this, "Player 7 Input", pad_handler::null };

	std::array<cfg_player*, 7> player{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load(const std::string& title_id = "", const std::string& profile = "", bool strict = false);
	void save(const std::string& title_id, const std::string& profile = "") const;
};

struct cfg_profile final : cfg::node
{
	cfg_profile();
	bool load();
	void save() const;

	const std::string path;
	const std::string global_key = "global";
	const std::string default_profile = "Default";

	cfg::map_entry active_profiles{ this, "Active Profiles" };
};

extern cfg_input g_cfg_input;
extern cfg_profile g_cfg_profile;
