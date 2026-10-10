#pragma once

// PS5: a game's controls remapped: which of the controller's buttons presses
// each of the PS3's, chosen on the pause menu's Controls tab and kept in the
// config folder's controls/<serial>.txt. The pad handler applies the running
// game's (ps5_pad_handler.cpp); never while the pause menu is open, which
// reads the controller as it is, so no mapping can leave it unreachable.
// L1 + R1 + the touch pad's click opens the menu whatever the mapping.

#include "util/types.hpp"

#include <array>
#include <string>

namespace ps5_pad_remap
{
	// The PS3's buttons and the controller's, in one order: Cross, Circle,
	// Square, Triangle, L1, R1, L2, R2, L3, R3, Start (OPTIONS), Select (the
	// touch pad's click), Up, Down, Left, Right; and, as a source, none
	inline constexpr u8 count = 16;
	inline constexpr u8 none = 16;
	using map = std::array<u8, count>; // for each of the PS3's buttons, its source

	const char* ps3_name(u8 button);      // "Cross", "Start"...
	const char* source_name(u8 source);   // "Cross", "OPTIONS", "Touch pad", "Nothing"...

	map identity();
	bool is_identity(const map& m);

	map load(const std::string& serial);
	void save(const std::string& serial, const map& m); // no file when it is the controller's own

	// What the pad handler applies now, and whether the pause menu holds it off
	void set_active(const map& m);
	map active();
	void hold_off(bool held);
	bool held_off();
}
